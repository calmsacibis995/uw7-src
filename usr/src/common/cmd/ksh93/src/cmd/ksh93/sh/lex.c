#ident	"@(#)ksh93:src/cmd/ksh93/sh/lex.c	1.2"
#pragma prototyped
/*
 * KornShell  lexical analyzer
 *
 * Written by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	<ast.h>
#include	<stak.h>
#include	<fcin.h>
#include	<nval.h>
#include	"FEATURE/options"
#include	"argnod.h"
#include	"shlex.h"
#include	"test.h"
#include	"lexstates.h"

extern char	*gettxt();

#ifdef KSHELL
#   include	"defs.h"
#else
#   include	<shell.h>
#   define	nv_getval(np)	((np)->nvalue)
    Shell_t sh  =  {1};
#endif /* KSHELL */

#define SYNBAD		3	/* exit value for syntax errors */
#define HERE_MEM	512	/* size of here-docs kept in memory */
#define STACK_ARRAY	3	/* size of depth match stack growth */
#define isblank(c)	(c==' ' || c=='\t')

/*
 * This structure allows for arbitrary depth nesting of (...), {...}, [...]
 */
static struct lexstate
{
	char		incase;		/* 1 for case pattern, 2 after case */
	char		intest;		/* 1 inside [[...]] */
	char		testop1;	/* 1 when unary test op legal */
	char		testop2;	/* 1 when binary test op legal */
	char		reservok;	/* >0 for reserved word legal */
	char		skipword;	/* next word can't be reserved */
	char		last_quote;	/* last multi-line quote character */
} lex;

static struct lexdata
{
	char		nocopy;
	char		paren;
	char		dolparen;
	char		nest;
	char		docword;
	char 		*docend;
	char		noarg;
	char		balance;
	char		warn;
	char		message;
	char 		*first;
	int		level;
	int		lastc;
} lexd;

static int		lex_max;
static int		*lex_match;
static int		lex_state;


#define	pushlevel(c,s)	((lexd.level>=lex_max?stack_grow():1) &&\
				((lex_match[lexd.level++]=lexd.lastc),\
				lexd.lastc=(((s)<<CHAR_BIT)|(c))))
#define oldmode()	(lexd.lastc>>CHAR_BIT)	
#define endchar()	(lexd.lastc&0xff)	
#define setchar(c)	(lexd.lastc = ((lexd.lastc&~0xff)|(c)))
#define poplevel()	(lexd.lastc=lex_match[--lexd.level])

static char		*fmttoken(int);
static int 		alias_exceptf(Sfio_t*, int, Sfdisc_t*);
static void		setupalias(const char*, Namval_t*);
static int		comsub(void);
static void		nested_here(void);
static int		here_copy(struct ionod*);
static int 		stack_grow(void);
static const Sfdisc_t alias_disc = { NULL, NULL, NULL, alias_exceptf, NULL };

#ifdef SHOPT_KIA
static off_t kiaoff;

static void refvar(int type)
{
	off_t off = (fcseek(0)-(type+1))-(lexd.first?lexd.first:fcfirst());
	unsigned long r;
	if(lexd.first)
	{
		off = (fcseek(0)-(type+1)) - lexd.first;
		r=kiaentity(lexd.first+kiaoff+type,off-kiaoff,'v',-1,-1,shlex.current,'v',0,"");
	}
	else
	{
		int n,offset = staktell();
		char *savptr,*begin;
		off = offset + (fcseek(0)-(type+1)) - fcfirst();
		if(kiaoff < offset)
		{
			/* variable starts on stak, copy remainder */
			if(off>offset)
				stakwrite(fcfirst()+type,off-offset);
			n = staktell()-kiaoff;
			begin = stakptr(kiaoff);
		}
		else
		{
			/* variable in data buffer */
			begin = fcfirst()+(type+kiaoff-offset);
			n = off-kiaoff;
		}
		savptr = stakfreeze(0);
		r=kiaentity(begin,n,'v',-1,-1,shlex.current,'v',0,"");
		stakset(savptr,offset);
	}
	sfprintf(shlex.kiatmp,"p;%..64d;v;%..64d;%d;%d;r;\n",shlex.current,r,sh.inlineno,sh.inlineno);
}
#endif /* SHOPT_KIA */

/*
 * This routine gets called when reading across a buffer boundary
 * If lexd.nocopy is off, then current token is saved on the stack
 */
static void lex_advance(Sfio_t *iop, const char *buff, register int size)
{
#ifdef KSHELL
	/* write to history file and to stderr if necessary */
	if(!sfstacked(iop))
	{
		if(sh_isstate(SH_HISTORY) && sh.hist_ptr && sh.hist_ptr->histfp)
			sfwrite(sh.hist_ptr->histfp, (void*)buff, size);
		if(sh_isstate(SH_VERBOSE))
			sfwrite(sfstderr, buff, size);
	}
#endif
	if(lexd.nocopy)
		return;
	if(lexd.first)
	{
		size -= (lexd.first-(char*)buff);
		buff = lexd.first;
		if(!lexd.noarg)
			shlex.arg = (struct argnod*)stakseek(ARGVAL);
#ifdef SHOPT_KIA
		kiaoff += ARGVAL;
#endif /* SHOPT_KIA */
	}
	if(size>0 && shlex.arg)
	{
		stakwrite(buff,size);
		lexd.first = 0;
	}
}

/*
 * fill up another input buffer
 * preserves lexical state
 */
static int lexfill(void)
{
	register int c;
	struct lexdata savedata;
	struct lexstate savestate;
	savedata = lexd;
	savestate = lex;
	c = fcfill();
	lex = savestate;
	lexd = savedata;
	lexd.first = 0;
	return(c);
}

/*
 * mode=1 for reinitialization  
 */
void sh_lexinit(int mode)
{
	fcnotify(lex_advance);
	lex.intest = lex.incase = lex.skipword = lexd.warn = 0;
	lex.reservok = 1;
	if(!sh_isoption(SH_DICTIONARY) && sh_isoption(SH_NOEXEC))
		lexd.warn=1;
	if(!mode)
	{
		lexd.noarg = lexd.level= lexd.dolparen = 0;
		lexd.docword = lexd.nest = lexd.paren = 0;
	}
}

#ifdef DBUG
int sh_lex(void)
{
	register int flag;
	extern int lextoken(void);
	char *quoted, *macro, *split, *expand; 
	register int tok = lextoken();
	quoted = macro = split = expand = "";
	if(tok==0 && (flag=shlex.arg->argflag))
	{
		if(flag&ARG_MAC)
			macro = "macro:";
		if(flag&ARG_EXP)
			expand = "expand:";
		if(flag&ARG_QUOTED)
			quoted = "quoted:";
	}
	sfprintf(sfstderr,"line %d: %o:%s%s%s%s %s\n",sh.inlineno,tok,quoted,
		macro, split, expand, fmttoken(tok));
	return(tok);
}
#define sh_lex	lextoken
#endif

#ifdef SHOPT_MULTIBYTE
static void mb_stateskip(const char *state, int *c, int *n, int *len)
{
	int curChar, lexState;
	lexState = 0;
	do
	{
		*len = mblen((char *)_Fcin.fcptr, MB_CUR_MAX);
		switch(*len)
		{
		    case -1: /* bogus multiByte char - parse as bytes? */
		    case 0: /* NULL byte */
		    case 1:
				lexState = state[curChar=fcget()];
				break;
		    default:
			/*
			 * None of the state tables contain entries
			 * for multibyte characters.  However, they
			 * should be treated the same as any other
			 * alpha character, so we'll use the state
			 * which would normally be assigned to the
			 * 'a' character.
			 */
			curChar = fcpeek(0);
			_Fcin.fcptr += *len;
			lexState = state['a'];
		}
	}
	while(lexState == 0);
	*c = curChar;
	*n =lexState;
}
#endif /* SHOPT_MULTIBYTE */
/*
 * Get the next word and put it on the top of the stak
 * A pointer to the current word is stored in shlex.arg
 * Returns the token type
 */
int sh_lex(void)
{
	register const char	*state;
	register int	n, c, mode=ST_BEGIN, wordflags=0;
	int		inlevel=lexd.level, assignment=0, ingrave=0;
#ifdef SHOPT_MULTIBYTE
        int LEN;
#else
#       define LEN      1
#endif /* SHOPT_MULTIBYTE */
	Sfio_t *sp;
	if(lexd.paren)
	{
		lexd.paren = 0;
		return(shlex.token=LPAREN);
	}
	if(lex.incase)
		shlex.assignok = 0;
	else
		shlex.assignok |= lex.reservok;
	if(lexd.nest)
	{
		pushlevel(lexd.nest,ST_NONE);
		lexd.nest = 0;
		mode = lex_state;
	}
	else if(lexd.docword)
	{
		if(fcgetc(c)=='-')
		{
			lexd.docword++;
			shlex.digits=1;
		}
		else if(c>0)
			fcseek(-1);
	}
	if(!lexd.dolparen)
	{
		shlex.arg = 0;
		if(mode!=ST_BEGIN)
			lexd.first = fcseek(0);
		else
			lexd.first = 0;
	}
	while(1)
	{
		/* skip over characters in the current state */
		state = sh_lexstates[mode];
#ifdef SHOPT_MULTIBYTE
		LEN=1;
		if (MB_CUR_MAX > 1)
		{
			int curchar, lexstate;
			mb_stateskip(state, &curchar, &lexstate, &LEN);
			n = lexstate;
			c = curchar;
		}
		else
#endif /* SHOPT_MULTIBYTE */
		while((n = state[c=fcget()])==0);
		switch(n)
		{
			case S_BREAK:
				fcseek(-1);
				goto breakloop;
			case S_EOF:
				sp = fcfile();
				if((n=lexfill()) > 0)
				{
					fcseek(-1);
					continue;
				}
				/* check for zero byte in file */
				if(n==0 && fcfile())
				{
					if(sh.readscript)
					{
						char *cp = error_info.id;
						errno = ENOEXEC;
						error_info.id = sh.readscript;
						error(ERROR_system(ERROR_NOEXEC),gettxt(e_exec_id,e_exec),cp);
					}
					else
					{
						shlex.token = -1;
						sh_syntax();
					}
				}
				/* end-of-file */
				if(mode==ST_BEGIN)
					return(shlex.token=EOFSYM);
				if(mode >ST_NORM && lexd.level>0)
				{
					switch(c=endchar())
					{
						case '$':
							if(mode==ST_LIT)
							{
								c = '\'';
								break;
							}
							mode = oldmode();
							poplevel();
							continue;
						case RBRACT:
							c = LBRACT;
							break;
						case RPAREN:
							c = LPAREN;
							break;
						default:
							c = LBRACE;
							break;
						case '"': case '`': case '\'':
							lexd.balance = c;
							break;
					}
					if(sp && !(sfset(sp,0,0)&SF_STRING))
					{
						shlex.lasttok = c;
						shlex.token = EOFSYM;
						sh_syntax();
					}
					lexd.balance = c;
				}
				goto breakloop;
			case S_COM:
				/* skip one or more comment line(s) */
				lex.reservok = !lex.intest;
				if(lexd.dolparen)
					lexd.nocopy--;
				do
				{
					while(fcgetc(c)>0 && c!='\n');
					if(c<=0)
						break;
					while(sh.inlineno++,fcpeek(0)=='\n')
						fcseek(1);
					while(state[c=fcpeek(0)]==0)
						fcseek(1);
				}
				while(c=='#');
				if(lexd.dolparen)
					lexd.nocopy++;
				if(c<0)
					return(shlex.token=EOFSYM);
				n = S_NLTOK;
				sh.inlineno--;
				/* FALL THRU */
			case S_NLTOK:
				/* check for here-document */
				if(shlex.heredoc)
				{
					if(!lexd.dolparen)
						lexd.nocopy++;
					c = sh.inlineno;
					if(here_copy(shlex.heredoc)<=0 && shlex.lasttok)
					{
						shlex.lasttok = IODOCSYM;
						shlex.token = EOFSYM;
						shlex.lastline = c;
						sh_syntax();
					}
					if(!lexd.dolparen)
						lexd.nocopy--;
					shlex.heredoc = 0;
				}
				lex.reservok = !lex.intest;
				lex.skipword = 0;
				/* FALL THRU */
			case S_NL:
				/* skip over new-lines */
				lex.last_quote = 0;
				while(sh.inlineno++,fcget()=='\n');
				fcseek(-1);
				if(n==S_NLTOK)
					return(shlex.token='\n');
				continue;
			case S_OP:
				/* return operator token */
				if(c=='<' || c=='>')
				{
					if(lex.testop2)
						lex.testop2 = 0;
					else
					{
						shlex.digits = (c=='>');
						lex.skipword = 1;
						shlex.aliasok = lex.reservok;
						lex.reservok = 0;
					}
				}
				else
				{
					lex.reservok = !lex.intest;
					if(c==RPAREN)
					{
						if(!lexd.dolparen)
							lex.incase = 0;
						return(shlex.token=c);
					}
					lex.testop1 = lex.intest;
				}
				if(fcgetc(n)>0)
					fcseek(-1);
				if(state[n]==S_OP)
				{
					if(n==c)
					{
						if(c=='<')
							lexd.docword=1;
						else if(n==LPAREN)
						{
							lexd.nest=1;
							lex_state = ST_NESTED;
							fcseek(1);
							return(sh_lex());
						}
						c  |= SYMREP;
					}
					else if(c=='(' || c==')')
						return(shlex.token=c);
					else if(c=='&')
						n = 0;
					else if(n=='&')
						c  |= SYMAMP;
					else if(c!='<' && c!='>')
						n = 0;
					else if(n==LPAREN)
						c  |= SYMLPAR;
					else if(n=='|')
						c  |= SYMPIPE;
					else if(c=='<' && n=='>')
						c = IORDWRSYM;
					else
						n = 0;
					if(n)
					{
						fcseek(1);
						lex.incase = (c==BREAKCASESYM || c==FALLTHRUSYM);
					}
					else
					{
						if((n=fcpeek(0))!=RPAREN && n!=LPAREN && lexd.warn)
							error(ERROR_warn(0),gettxt(e_lexspace_id,e_lexspace),sh.inlineno,c,n);
					}
				}
				return(shlex.token=c);
			case S_ESC:
				/* check for \<new-line> */
				fcgetc(n);
				c=2;
#ifdef SHOPT_CRNL
				if(n=='\r')
				{
					if(fcgetc(n)=='\n')
						c=3;
					else
					{
						n='\r';
						fcseek(-1);
					}
				}
#endif /* SHOPT_CRNL */
				if(n=='\n')
				{
					Sfio_t *sp;
					sh.inlineno++;
					/* synchronize */
					if(!(sp=fcfile()))
						state=fcseek(0);
					fcclose();
					if(sp)
						fcfopen(sp);
					else
						fcsopen((char*)state);
					/* remove \new-line */
					n = staktell()-c;
					stakseek(n);
					if(n<=ARGVAL)
					{
						mode = 0;
						lexd.first = 0;
					}
					continue;
				}
				wordflags |= ARG_QUOTED;
				if(mode==ST_DOL)
					goto err;
#ifndef STR_MAXIMAL
				else if(mode==ST_NESTED && lexd.warn && 
					endchar()==RBRACE &&
					sh_lexstates[ST_DOL][n]==S_DIG
				)
					error(ERROR_warn(0),gettxt(e_lexfuture_id,e_lexfuture),sh.inlineno,n);
#endif /* STR_MAXIMAL */
				break;
			case S_NAME:
				if(!lex.skipword)
					lex.reservok *= 2;
				/* FALL THRU */
			case S_TILDE:
			case S_RES:
				if(!lexd.dolparen)
					lexd.first = fcseek(0)-LEN;
				else if(lexd.docword)
					lexd.docend = fcseek(0)-LEN;
				mode = ST_NAME;
				if(c=='.')
					fcseek(-1);
				if(n!=S_TILDE)
					continue;
				wordflags = ARG_MAC;
				mode = ST_NORM;
				continue;
			case S_REG:
				if(mode==ST_BEGIN)
				{
					/* skip new-line joining */
					if(c=='\\' && fcpeek(0)=='\n')
					{
						sh.inlineno++;
						fcseek(1);
						continue;
					}
					fcseek(-1);
					if(!lexd.dolparen)
						lexd.first = fcseek(0);
					else if(lexd.docword)
						lexd.docend = fcseek(0);
					if(c=='[' && shlex.assignok>=SH_ASSIGN)
					{
						mode = ST_NAME;
						continue;
					}
				}
				mode = ST_NORM;
				continue;
			case S_LIT:
				wordflags |= ARG_QUOTED;
				if(mode==ST_DOL)
				{
					if(endchar()!='$')
						goto err;
					if(oldmode()==ST_QUOTE) /* $' within "" or `` */
					{
						if(lexd.warn)
							error(ERROR_warn(0),gettxt(e_lexslash_id,e_lexslash),sh.inlineno);
						mode = ST_LIT;
					}
				}
				if(mode!=ST_LIT)
				{
					if(lexd.warn && lex.last_quote && sh.inlineno > shlex.lastline)
						error(ERROR_warn(0),gettxt(e_lexlongquote_id,e_lexlongquote),shlex.lastline,lex.last_quote);
					lex.last_quote = 0;
					shlex.lastline = sh.inlineno;
					if(mode!=ST_DOL)
						pushlevel('\'',mode);
					mode = ST_LIT;
					continue;
				}
				/* check for multi-line single-quoted string */
				else if(sh.inlineno > shlex.lastline)
					lex.last_quote = '\'';
				mode = oldmode();
				poplevel();
				break;
			case S_ESC2:
				/* \ inside '' */
				if(endchar()=='$')
					fcgetc(n);
				continue;
			case S_GRAVE:
				if(lexd.warn && (mode!=ST_QUOTE || endchar()!='`'))
					error(ERROR_warn(0),gettxt(e_lexobsolete1_id,e_lexobsolete1),sh.inlineno);
				wordflags |=(ARG_MAC|ARG_EXP);
				if(mode==ST_QUOTE)
					ingrave = !ingrave;
				/* FALL THRU */
			case S_QUOTE:
				if(n==S_QUOTE)
					wordflags |=ARG_QUOTED;
				if(mode!=ST_QUOTE)
				{
					if(c!='"' || mode!=ST_QNEST)
					{
						if(lexd.warn && lex.last_quote && sh.inlineno > shlex.lastline)
							error(ERROR_warn(0),gettxt(e_lexlongquote_id,e_lexlongquote),shlex.lastline,lex.last_quote);
						lex.last_quote=0;
						shlex.lastline = sh.inlineno;
						pushlevel(c,mode);
					}
					ingrave = (c=='`');
					mode = ST_QUOTE;
					continue;
				}
				else if((n=endchar())==c)
				{
					if(sh.inlineno > shlex.lastline)
						lex.last_quote = c;
					mode = oldmode();
					poplevel();
				}
				else if(c=='"' && n==RBRACE)
					mode = ST_QNEST;
				break;
			case S_DOL:
				/* don't check syntax inside `` */
				if(mode==ST_QUOTE && ingrave)
					continue;
#ifdef SHOPT_KIA
				if(lexd.first)
					kiaoff = fcseek(0)-lexd.first;
				else
					kiaoff = staktell()+fcseek(0)-fcfirst();
#endif /* SHOPT_KIA */
				pushlevel('$',mode);
				mode = ST_DOL;
				continue;
			case S_PAR:
				wordflags |= ARG_MAC;
				mode = oldmode();
				poplevel();
				fcseek(-1);
				wordflags |= comsub();
				continue;
			case S_RBRA:
				if((n=endchar()) == '$')
					goto err;
				if(mode!=ST_QUOTE || n==RBRACE)
				{
					mode = oldmode();
					poplevel();
				}
				break;
			case S_EDOL:
				/* end $identifier */
#ifdef SHOPT_KIA
				if(shlex.kiafile)
					refvar(0);
#endif /* SHOPT_KIA */
				if(lexd.warn && c==LBRACT)
					error(ERROR_warn(0),gettxt(e_lexusebrace_id,e_lexusebrace),sh.inlineno);
				fcseek(-1);
				mode = oldmode();
				poplevel();
				break;
			case S_DOT:
				/* make sure next character is alpha */
				if(fcgetc(n)>0)
					fcseek(-1);
				if(isaletter(n))
					continue;
				if(mode==ST_NAME)
					break;
				if(isastchar(n))
					continue;
				goto err;
			case S_SPC1:
				wordflags |= ARG_MAC;
				if(endchar()==RBRACE)
				{
					setchar(c);
					continue;
				}
				/* FALL THRU */
			case S_ALP:
				if(c=='.' && endchar()=='$')
					goto err;
			case S_SPC2:
			case S_DIG:
				wordflags |= ARG_MAC;
				switch(endchar())
				{
					case '$':
						if(n==S_ALP) /* $identifier */
							mode = ST_DOLNAME;
						else
						{
							mode = oldmode();
							poplevel();
						}
						break;
#ifdef SHOPT_OO
					case '-':
#endif /* SHOPT_OO */
					case '!':
						if(n!=S_ALP)
							goto dolerr;
					case '#':
					case RBRACE:
						if(n==S_ALP)
						{
							setchar(RBRACE);
							if(c=='.')
								fcseek(-1);
							mode = ST_BRACE;
						}
						else if(n==S_DIG)
							setchar('0');
						else
							setchar('!');
						break;
					case '0':
						if(n==S_DIG)
							break;
					default:
						goto dolerr;
				}
				break;
			dolerr:
			case S_ERR:
				if((n=endchar()) == '$')
					goto err;
				if((n=sh_lexstates[ST_BRACE][c])!=S_MOD1 && n!=S_MOD2)
				{
					/* see whether inside `...` */
					mode = oldmode();
					poplevel();
					if((n = endchar()) != '`')
						goto err;
					pushlevel(RBRACE,mode);
				}
				else
					setchar(RBRACE);
				mode = ST_NESTED;
				continue;
			case S_MOD1:
				if(oldmode()==ST_QUOTE || oldmode()==ST_NONE)
				{
					/* allow ' inside "${...}" */
					if(c==':' && fcgetc(n)>0)
					{
						n = state[n];
						fcseek(-1);
					}
					if(n==S_MOD1)
					{
						mode = ST_QUOTE;
						continue;
					}
				}
				/* FALL THRU */
			case S_MOD2:
#ifdef SHOPT_KIA
				if(shlex.kiafile)
					refvar(1);
#endif /* SHOPT_KIA */
				mode = ST_NESTED;
				continue;
			case S_LBRA:
				if((c=endchar()) == '$')
				{
					setchar(RBRACE);
					continue;
				}
			err:
				n = endchar();
				mode = oldmode();
				poplevel();
				if(n!='$')
				{
					shlex.token = c;
					sh_syntax();
				}
				else
				{
					if(lexd.warn && c!='/' && sh_lexstates[ST_NORM][c]!=S_BREAK && c!='"')
						error(ERROR_warn(0),gettxt(e_lexslash_id,e_lexslash),sh.inlineno);
					else if(c=='"')
						wordflags |= ARG_MESSAGE;
					fcseek(-1);
				}
				continue;
			case S_META:
				if(lexd.warn && endchar()==RBRACE)
					error(ERROR_warn(0),gettxt(e_lexusequote_id,e_lexusequote),sh.inlineno,c);
				continue;
			case S_PUSH:
				pushlevel(RPAREN,mode);
				mode = ST_NESTED;
				continue;
			case S_POP:
				if(lexd.level <= inlevel)
					break;
				n = endchar();
				if(c==RBRACT  && !(n==RBRACT || n==RPAREN))
					continue;
				if(c==';' && n!=';')
				{
					if(lexd.warn && n==RBRACE)
						error(ERROR_warn(0),gettxt(e_lexusequote_id,e_lexusequote),sh.inlineno,c);
					continue;
				}
				if(mode==ST_QNEST)
				{
					if(lexd.warn)
						error(ERROR_warn(0),gettxt(e_lexescape_id,e_lexescape),sh.inlineno,c);
					continue;
				}
				mode = oldmode();
				poplevel();
				/* quotes in subscript need expansion */
				if(mode==ST_NAME && (wordflags&ARG_QUOTED))
					wordflags |= ARG_MAC;
				/* check for ((...)) */
				if(n==1 && c==RPAREN)
				{
					if(fcgetc(n)==RPAREN)
					{
						if(mode==ST_NONE && !lexd.dolparen)
							goto breakloop;
						lex.reservok = 1;
						lex.skipword = 0;
						return(shlex.token=EXPRSYM);
					}
					/* backward compatibility */
					if(lexd.dolparen)
						fcseek(-1);
					else
					{
						if(lexd.warn)
							error(ERROR_warn(0),gettxt(e_lexnested_id,e_lexnested),sh.inlineno);
						if(!(state=lexd.first))
							state = fcfirst();
						fcseek(state-fcseek(0));
						if(shlex.arg)
						{
							shlex.arg = (struct argnod*)stakfreeze(1);
							setupalias(shlex.arg->argval,NIL(Namval_t*));
						}
						lexd.paren = 1;
					}
					return(shlex.token=LPAREN);
				}
				if(mode==ST_NONE)
					return(0);
				if(c!=n)
				{
					shlex.token = c;
					sh_syntax();
				}
				continue;
			case S_EQ:
				assignment = shlex.assignok;
				/* FALL THRU */
			case S_COLON:
				if(assignment)
				{
					if((c=fcget())=='~')
						wordflags |= ARG_MAC;
					fcseek(-1);
				}
				break;
			case S_LABEL:
				if(lex.reservok && !lex.incase)
				{
					c = fcget();
					fcseek(-1);
					if(state[c]==S_BREAK)
					{
						assignment = -1;
						goto breakloop;
					}
				}
				break;
			case S_BRACT:
				/* check for possible subscript */
				if((n=endchar())==RBRACT || n==RPAREN || 
					(mode==ST_BRACE) ||
					(mode==ST_NAME && (shlex.assignok||lexd.level)))
				{
					pushlevel(RBRACT,mode);
					mode = ST_NESTED;
					continue;
				}
				wordflags |= ARG_EXP;
				break;
			case S_BRACE:
			{
				int isfirst;
				if(lexd.dolparen)
					break;
				isfirst = (lexd.first&&fcseek(0)==lexd.first+1);
				fcgetc(n);
				/* check for {} */
				if(c==LBRACE && n==RBRACE)
					break;
				if(n>0)
					fcseek(-1);
				else if(lex.reservok)
					break;
				/* check for reserved word { or } */
				if(lex.reservok && state[n]==S_BREAK && isfirst)
					break;
#ifdef SHOPT_BRACEPAT
				if(c==LBRACE && !assignment && state[n]!=S_BREAK
					&& !lex.incase && !lex.intest
					&& !lex.skipword)
				{
					wordflags |= ARG_EXP;
					pushlevel(RBRACE,mode);
					mode = ST_NESTED;
					continue;
				}
#endif /* SHOPT_BRACEPAT */
				if(lexd.warn)
					error(ERROR_warn(0),gettxt(e_lexquote_id,e_lexquote),sh.inlineno,c);
				break;
			}
			case S_PAT:
				wordflags |= ARG_EXP;
				/* FALL THRU */
			case S_EPAT:
				if(fcgetc(n)==LPAREN)
				{
					wordflags |= ARG_EXP;
					pushlevel(RPAREN,mode);
					mode = ST_NESTED;
					continue;
				}
				if(n>0)
					fcseek(-1);
#ifdef SHOPT_APPEND
				if(n=='=' && c=='+' && mode==ST_NAME)
					continue;
				break;
#endif /* SHOPT_APPEND */
		}
		if(mode==ST_NAME)
			mode = ST_NORM;
		else if(mode==ST_NONE)
			return(0);
	}
breakloop:
	if(lexd.dolparen)
	{
		lexd.balance = 0;
		if(lexd.docword)
			nested_here();
		lexd.message = (wordflags&ARG_MESSAGE);
		return(shlex.token=0);
	}
	if(!(state=lexd.first))
		state = fcfirst();
	n = fcseek(0)-(char*)state;
	if(!shlex.arg)
		shlex.arg = (struct argnod*)stakseek(ARGVAL);
	if(n>0)
		stakwrite(state,n);
	/* add balancing character if necessary */
	if(lexd.balance)
	{
		stakputc(lexd.balance);
		lexd.balance = 0;
	}
	state = stakptr(ARGVAL);
	n = staktell()-ARGVAL;
	lexd.first=0;
	if(n==1)
	{
		/* check for numbered redirection */
		n = state[0];
		if((c=='<' || c=='>') && isadigit(n))
		{
			c = sh_lex();
			shlex.digits = (n-'0'); 
			return(c);
		}
		if(n==LBRACT)
			c = 0;
		else if(n=='~')
			c = ARG_MAC;
		else
			c = (wordflags&ARG_EXP);
		n = 1;
	}
	else
		c = wordflags;
	if(assignment<0)
	{
		stakseek(staktell()-1);
		shlex.arg = (struct argnod*)stakfreeze(1);
		lex.reservok = 1;
		return(shlex.token=LABLSYM);
	}
	if(assignment || (lex.intest&&!lex.incase) || mode==ST_NONE)
		c &= ~ARG_EXP;
	if((c&ARG_EXP) && (c&ARG_QUOTED))
		c |= ARG_MAC;
	if(mode==ST_NONE)
	{
		/* eliminate trailing )) */
		stakseek(staktell()-2);
	}
	if(c&ARG_MESSAGE)
	{
		if(sh_isoption(SH_DICTIONARY))
			shlex.arg = sh_endword(2);
		if(!sh_isoption(SH_NOEXEC))
		{
			shlex.arg = sh_endword(1);
			c &= ~ARG_MESSAGE;
		}
	}
	if(c==0 || (c&(ARG_MAC|ARG_EXP)) || (lexd.warn && !lexd.docword))
	{
		shlex.arg = (struct argnod*)stakfreeze(1);
		shlex.arg->argflag = (c?c:ARG_RAW);
	}
	else
		shlex.arg = sh_endword(0);
	state = shlex.arg->argval;
	if(assignment)
		shlex.arg->argflag |= ARG_ASSIGN;
	else if(!lex.skipword)
		shlex.assignok = 0;
	shlex.arg->argchn.len = 0;
	shlex.arg->argnxt.ap = 0;
	if(mode==ST_NONE)
		return(shlex.token=EXPRSYM);
	if(lex.intest)
	{
		if(lex.testop1)
		{
			lex.testop1 = 0;
			if(n==2 && state[0]=='-' && state[2]==0 &&
				strchr(test_opchars,state[1]))
			{
				if(lexd.warn && state[1]=='a')
					error(ERROR_warn(0),gettxt(e_lexobsolete2_id,e_lexobsolete2),sh.inlineno);
				shlex.digits = state[1];
				shlex.token = TESTUNOP;
			}
			else if(n==1 && state[0]=='!' && state[1]==0)
			{
				lex.testop1 = 1;
				shlex.token = '!';
			}
			else
			{
				lex.testop2 = 1;
				shlex.token = 0;
			}
			return(shlex.token);
		}
		lex.incase = 0;
		c = sh_lookup(state,shtab_testops);
		switch(c)
		{
		case TEST_END:
			lex.testop2 = lex.intest = 0;
			lex.reservok = 1;
			shlex.token = ETESTSYM;
			return(shlex.token);

		case TEST_SEQ:
			if(lexd.warn && state[1]==0)
				error(ERROR_warn(0),gettxt(e_lexobsolete3_id,e_lexobsolete3),sh.inlineno);
			/* FALL THRU */
		default:
			if(lex.testop2)
			{
				if(lexd.warn && (c&TEST_ARITH))
					error(ERROR_warn(0),gettxt(e_lexobsolete4_id,e_lexobsolete4),sh.inlineno,state);
				if(c&TEST_PATTERN)
					lex.incase = 1;
				lex.testop2 = 0;
				shlex.digits = c;
				shlex.token = TESTBINOP;	
				return(shlex.token);	
			}

		case TEST_OR: case TEST_AND:
		case 0:
			return(shlex.token=0);
		}
	}
	if(lex.reservok /* && !lex.incase*/ && n<=2)
	{
		/* check for {, }, ! */
		c = state[0];
		if(n==1 && (c=='{' || c=='}' || c=='!'))
		{
			if(lexd.warn && c=='{' && lex.incase==2)
				error(ERROR_warn(0),gettxt(e_lexobsolete6_id,e_lexobsolete6),sh.inlineno);
			return(shlex.token=c);
		}
		else if(!lex.incase && c==LBRACT && state[1]==LBRACT)
		{
			lex.intest = lex.testop1 = 1;
			lex.testop2 = lex.reservok = 0;
			return(shlex.token=BTESTSYM);
		}
	}
	c = 0;
	if(!lex.skipword)
	{
		if(n>1 && lex.reservok==1 && mode==ST_NAME && 
			(c=sh_lookup(state,shtab_reserved)))
		{
			if(lex.incase)
			{
				if(lex.incase >1)
					lex.incase = 1;
				else if(c==ESACSYM)
					lex.incase = 0;
				else
					c = 0;
			}
			else if(c==FORSYM || c==CASESYM || c==SELECTSYM || c==FUNCTSYM)
			{
				lex.skipword = 1;
				lex.incase = 2*(c==CASESYM);
			}
			else
				lex.skipword = 0;
			if(c==INSYM)
				lex.reservok = 0;
			else if(c==TIMESYM)
			{
				/* yech - POSIX requires time -p */
				while(fcgetc(n)==' ' || n=='\t');
				if(n>0)
					fcseek(-1);
				if(n=='-')
					c=0;
			}
			return(shlex.token=c);
		}
		if(!(wordflags&ARG_QUOTED) && (lex.reservok||shlex.aliasok))
		{
			/* check for aliases */
			Namval_t* np;
			if(!lex.incase && !assignment && fcpeek(0)!=LPAREN &&
				(np=nv_search(state,sh.alias_tree,HASH_SCOPE))
				&& !nv_isattr(np,NV_NOEXPAND)
#ifdef KSHELL
				&& (!sh_isstate(SH_NOALIAS) || nv_isattr(np,NV_EXPORT))
#endif /* KSHELL */
				&& (state=nv_getval(np)))
			{
				setupalias(state,np);
				nv_onattr(np,NV_NOEXPAND);
				shlex.assignok++;
				lex.reservok = 1;
				return(sh_lex());
			}
		}
		lex.reservok = 0;
	}
	lex.skipword = lexd.docword = 0;
	return(shlex.token=c);
}

/*
 * read to end of command substitution
 */
static int comsub(void)
{
	register int	n,c,count=1;
	register int	line=sh.inlineno;
	char word[5];
	int messages=0;
	struct lexstate	save;
	save = lex;
	sh_lexinit(1);
	lexd.dolparen++;
	lex.incase=0;
	if(sh_lex()==LPAREN)
	{
		while(1)
		{
			/* look for case and esac */
			n=0;
			while(1)
			{
				fcgetc(c);
				/* skip leading white space */
				if(n==0 && !sh_lexstates[ST_BEGIN][c])
					continue;
				if(n==4)
					break;
				if(sh_lexstates[ST_NAME][c])
					goto skip;
				word[n++] = c;
			}
			if(sh_lexstates[ST_NAME][c]==S_BREAK)
			{
				if(memcmp(word,"case",4)==0)
					lex.incase=1;
				else if(memcmp(word,"esac",4)==0)
					lex.incase=0;
			}
		skip:
			if(c)
				fcseek(-1);
			switch(sh_lex())
			{
			    case LPAREN:
				if(!lex.incase)
					count++;
				break;
			    case RPAREN:
				if(lex.incase)
					lex.incase=0;
				else if(--count<=0)
					goto done;
				break;
			    case EOFSYM:
				shlex.lastline = line;
				shlex.lasttok = LPAREN;
				sh_syntax();
			    case IODOCSYM:
				sh_lex();
				break;
			    case 0:
				messages |= lexd.message;
			}
		}
	}
done:
	lexd.dolparen--;
	lex = save;
	return(messages);
}

/*
 * here-doc nested in $(...)
 * allocate ionode with delimiter filled in without disturbing stak
 */
static void nested_here(void)
{
	register struct ionod *iop;
	register int n,offset;
	struct argnod *arg = shlex.arg;
	char *base;
	if(offset=staktell())
		base = stakfreeze(0);
	n = fcseek(0)-lexd.docend;
	iop = newof(0,struct ionod,1,n+1);
	iop->iolst = shlex.heredoc;
	stakseek(ARGVAL);
	stakwrite(lexd.docend,n);
	shlex.arg = sh_endword(0);
	iop->ioname = (char*)(iop+1);
	strcpy(iop->ioname,shlex.arg->argval);
	iop->iofile = (IODOC|IORAW);
	if(lexd.docword>1)
		iop->iofile |= IOSTRIP;
	shlex.heredoc = iop;
	shlex.arg = arg;
	lexd.docword = 0;
	if(offset)
		stakset(base,offset);
	else
		stakseek(0);
}

/*
 * skip to <close> character
 * if <copy> is non,zero, then the characters are copied to the stack
 * <state> is the initial lexical state
 */
void sh_lexskip(int close, register int copy, int  state)
{
	register char	*cp;
	lexd.nest = close;
	lex_state = state;
	lexd.nocopy += !copy;
	lexd.noarg = 1;
	sh_lex();
	lexd.noarg = 0;
	lexd.nocopy -= !copy;
	if(copy)
	{
		if(!(cp=lexd.first))
			cp = fcfirst();
		if((copy = fcseek(0)-cp) > 0)
			stakwrite(cp,copy);
	}
}

/*
 * read in here-document from script
 * quoted here documents, and here-documents without special chars are
 * noted with the IOQUOTE flag
 * returns 1 for complete here-doc, 0 for EOF
 */

static int here_copy(register struct ionod *iop)
{
	register const char	*state;
	register int		c,n;
	register char		*bufp,*cp;
	register Sfio_t	*sp;
	int			stripflg, nsave, special=0;
	if(iop->iolst)
		here_copy(iop->iolst);
	if(!(sp=sh.heredocs))
	{
		sh.heredocs = sp = sftmp(HERE_MEM);
		iop->iooffset = (off_t)0;
	}
	else
		iop->iooffset = sfseek(sp,(off_t)0,SEEK_END);
	iop->iosize = 0;
	iop->iodelim=iop->ioname;
	/* check for and strip quoted characters in delimiter string */
	if(stripflg=iop->iofile&IOSTRIP)
	{
		while(*iop->iodelim=='\t')
			iop->iodelim++;
		/* skip over leading tabs in document */
		while(fcgetc(c)=='\t');
		if(c>0)
			fcseek(-1);
	}
	if(iop->iofile&IOQUOTE)
		state = sh_lexstates[ST_LIT];
	else
		state = sh_lexstates[ST_QUOTE];
	bufp = fcseek(0);
	n = S_NL;
	while(1)
	{
		if(n!=S_NL)
		{
			/* skip over regular characters */
			while((n=state[fcget()])==0);
		}
		if(n==S_EOF || !(c=fcget()))
		{
			if(!lexd.dolparen && (c=(fcseek(0)-1)-bufp))
			{
				if((c=sfwrite(sp,bufp,c))>0)
					iop->iosize += c;
			}
			if((c=lexfill())<=0)
				break;
			bufp = fcseek(-1);
		}
		else
			fcseek(-1);
		switch(n)
		{
		    case S_NL:
			sh.inlineno++;
			if(stripflg && c=='\t')
			{
				if(!lexd.dolparen)
				{
					/* write out line */
					n = fcseek(0)-bufp;
					if((n=sfwrite(sp,bufp,n))>0)
						iop->iosize += n;
				}
				/* skip over tabs */
				while(c=='\t')
					fcgetc(c);
				if(c<=0)
					goto done;
				bufp = fcseek(-1);
			}
			if(c!=iop->iodelim[0])
				break;
			cp = fcseek(0);
			nsave = n = 0;
			while(1)
			{
				if(!(c=fcget())) 
				{
					if(!lexd.dolparen && (c=cp-bufp))
					{
						if((c=sfwrite(sp,cp=bufp,c))>0)
							iop->iosize+=c;
					}
					nsave = n;
					if((c=lexfill())<=0)
					{
						c = iop->iodelim[n]==0;
						goto done;
					}
				}
#ifdef SHOPT_CRNL
				if(c=='\r' && (c=fcget())!=NL)
				{
					if(c)
						fcseek(-1);
					c='\r';
				}
#endif /* SHOPT_CRNL */
				if(c==NL && (sh.inlineno++,iop->iodelim[n])==0)
				{
					if(!lexd.dolparen && (n=cp-bufp))
					{
						if((n=sfwrite(sp,bufp,n))>0)
							iop->iosize += n;
					}
					sh.inlineno--;
					goto done;
				}
				if(iop->iodelim[n++]!=c)
				{
					/*
					 * The match for delimiter failed.
					 * nsave>0 only when a buffer boundary
					 * was crossed while checking the
					 * delimiter
					 */
					if(!lexd.dolparen && nsave>0)
					{
						if((n=sfwrite(sp,bufp,nsave))>0)
							iop->iosize += n;
						bufp = fcfirst();
					}
					if(c==NL)
						fcseek(-1);
					break;
				}
			}
			break;
		    case S_ESC:
			n=1;
#ifdef SHOPT_CRNL
			if(c=='\r')
			{
				fcseek(1);
				if(c=fcget())
					fcseek(-1);
				if(c==NL)
					n=2;
				else
				{
					special++;
					break;
				}
			}
#endif /* SHOPT_CRNL */
			if(c==NL)
			{
				/* new-line joining */
				sh.inlineno++;
				if(!lexd.dolparen && (n=(fcseek(0)-bufp)-n)>0)
				{
					if((n=sfwrite(sp,bufp,n))>0)
						iop->iosize += n;
					bufp = fcseek(0)+1;
				}
			}
			else
				special++;
			fcget();
			break;
				
		    case S_GRAVE:
		    case S_DOL:
			special++;
			break;
		}
		n=0;
	}
done:
	if(lexd.dolparen)
		free((void*)iop);
	else if(!special)
		iop->iofile |= IOQUOTE;
	return(c);
}

/*
 * generates string for given token
 */
static char	*fmttoken(register int sym)
{
	static char tok[3];
	if(sym < 0)
		return(gettxt(e_lexzerobyte_id,e_lexzerobyte));
	if(sym==0)
		return(shlex.arg?shlex.arg->argval:"?");
	if(sym&SYMRES)
	{
		register const Shtable_t *tp=shtab_reserved;
		while(tp->sh_number && tp->sh_number!=sym)
			tp++;
		return((char*)tp->sh_name);
	}
	if(sym==EOFSYM)
		return(gettxt(e_endoffile_id,e_endoffile));
	if(sym==NL)
		return(gettxt(e_newline_id,e_newline));
	tok[0] = sym;
	if(sym&SYMREP)
	{
		tok[1] = sym;
		if(sym==EXPRSYM)
		{
			int offset =staktell();
			stakputs(tok);
			stakputs(shlex.arg->argval);
			stakputs("))");
			return(stakptr(offset));
		}
	}
	else
	{
		switch(sym&SYMMASK)
		{
			case SYMAMP:
				sym = '&';
				break;
			case SYMPIPE:
				sym = '|';
				break;
			case SYMGT:
				sym = '>';
				break;
			case SYMLPAR:
				sym = LPAREN;
				break;
			default:
				sym = 0;
		}
		tok[1] = sym;
	}
	return(tok);
}

/*
 * print a bad syntax message
 */

void	sh_syntax(void)
{
	register short cp = 0;
	register int tok = shlex.token;
	register char *tokstr;
	Sfio_t *sp;
	if((tok==EOFSYM) && shlex.lasttok)
	{
		tok = shlex.lasttok;
		cp = 1;
	}
	else
		shlex.lastline = sh.inlineno;
	tokstr = fmttoken(tok);
	if((sp=fcfile()) || (sh.infd>=0 && (sp=sh.sftable[sh.infd])))
	{
		/* clear out any pending input */
		register Sfio_t *top;
		while(fcget()>0);
		fcclose();
		while(top=sfstack(sp,SF_POPSTACK))
			sfclose(top);
	}
	else
		fcclose();
	sh.inlineno = shlex.inlineno;
	sh.st.firstline = shlex.firstline;
#ifdef KSHELL
	if(!sh_isstate(SH_INTERACTIVE|SH_PROFILE))
#else
	if(sh.inlineno!=1)
#endif
		if (cp)
			error(ERROR_exit(SYNBAD),gettxt(e_lexsyntax1b_id,e_lexsyntax1b),shlex.lastline,tokstr);
		else
			error(ERROR_exit(SYNBAD),gettxt(e_lexsyntax1a_id,e_lexsyntax1a),shlex.lastline,tokstr);
	else
		if (cp)
			error(ERROR_exit(SYNBAD),gettxt(e_lexsyntax2b_id,e_lexsyntax2b),tokstr);
		else
			error(ERROR_exit(SYNBAD),gettxt(e_lexsyntax2a_id,e_lexsyntax2a),tokstr);
}

/*
 * Assumes that current word is unfrozen on top of the stak
 * If <mode> is zero, gets rid of quoting and consider argument as string
 *    and returns pointer to frozen arg
 * If mode==1, just replace $"..." strings with international strings
 *    The result is left on the stak
 * If mode==2, the each $"" string is printed on standard output
 */
struct argnod *sh_endword(int mode)
{
	register const char *state = sh_lexstates[ST_NESTED];
	register int n;
	register char *sp,*dp;
	register int inquote=0, inlit=0; /* set within quoted strings */
	struct argnod* argp;
	char	*ep=0;
	int offset = staktell();
	stakputc(0);
	sp =  stakptr(ARGVAL);
#ifdef SHOPT_MULTIBYTE
	if (MB_CUR_MAX > 1)
	{
		do
		{
			int len = mblen(sp, MB_CUR_MAX);
			switch(len)
			{
			    case -1:    /* illegal multi-byte char */
			    case 0:
			    case 1:
				n=state[*sp++];
				break;
			    default:
				/*
				 * None of the state tables contain
				 * entries for multibyte characters,
				 * however, they should be treated
				 * the same as any other alph
				 * character.  Therefore, we'll use
				 * the state of the 'a' character.
				 */
				n=state['a'];
				sp += len;
			}
		}
		while(n == 0);
	}
	else
#endif /* SHOPT_MULTIBYTE */
	while((n=state[*sp++])==0);
	dp = sp;
	while(1)
	{
		switch(n)
		{
		    case S_EOF:
			stakseek(dp-stakptr(0));
			if(mode==0)
			{
				argp = (struct argnod*)stakfreeze(0);
				argp->argflag = ARG_RAW|ARG_QUOTED;
			}
			return(argp);
		    case S_LIT:
			if(!(inquote&1))
			{
				inlit = !inlit;
				if(mode==0)
				{
					dp--;
					if(ep)
					{
						*dp = 0;
						dp = ep+stresc(ep);
					}
					ep = 0;
				}
			}
			break;
		    case S_QUOTE:
			if(!inlit)
			{
				if(mode==0)
					dp--;
				inquote = inquote^1;
				if(ep)
				{
					char *msg;
					if(mode==2)
					{
						sfprintf(sfstdout,"%.*s\n",dp-ep,ep);
						ep = 0;
						break;
					}
					*--dp = 0;
					msg = ERROR_translate(ep,2);
					n = strlen(msg);
					dp = ep+n;
					if(sp-dp <= 1)
					{
						int left = offset-(sp-stakptr(0));
						int shift = (dp+1-sp);
						offset += shift;
						stakseek(offset);
						sp = stakptr(offset);
						ep = sp - shift;
						while(left--)
							*--sp = *--ep;
						dp = sp-1;
						ep = dp-n;
					}
					memcpy(ep,msg,n);
					*dp++ = '"';
				}
				ep = 0;
			}
			break;
		    case S_DOL:	/* check for $'...'  and $"..." */
			if(inlit)
				break;
			if(*sp==LPAREN || *sp==LBRACE)
			{
				inquote <<= 1;
				break;
			}
			if(inquote&1)
				break;
			if(*sp=='\'' || *sp=='"')
			{
				if(*sp=='"')
					inquote |= 1;
				else
					inlit = 1;
				sp++;
				if(mode==0 || (inquote&1))
				{
					if(mode==2)
						ep = dp++;
					else if(mode==1)
						(ep=dp)[-1] = '"';
					else
						ep = --dp;
				}
			}
			break;
		    case S_ESC:
#ifdef SHOPT_CRNL
			if(*sp=='\r' && sp[1]=='\n')
				sp++;
#endif /* SHOPT_CRNL */
			if(inlit || mode)
			{
				if(ep)
					*dp++ = *sp++;
				break;
			}
			n = *sp;
			if(!(inquote&1) || (sh_lexstates[ST_QUOTE][n] && n!=RBRACE))
			{
				if(n=='\n')
					dp--;
				else
					dp[-1] = n;
				sp++;
			}
			break;
		    case S_POP:
			if(!inlit && !(inquote&1))
				inquote >>= 1;
		}
#ifdef SHOPT_MULTIBYTE
		if (MB_CUR_MAX > 1)
		{
			do
			{
				int len = mblen(sp, MB_CUR_MAX);
				switch(len)
				{
				    case -1: /* illegal multi-byte char */
				    case 0:
				    case 1:
					n=state[*dp++ = *sp++];
					break;
				    default:
					/*
					 * None of the state tables contain
					 * entries for multibyte characters,
					 * however, they should be treated
					 * the same as any other alph
					 * character.  Therefore, we'll use
					 * the state of the 'a' character.
					 */
					while(len--)
						*dp++ = *sp++;
					n=state['a'];
				}
			}
			while(n == 0);
		}
		else
#endif /* SHOPT_MULTIBYTE */
		while((n=state[*dp++ = *sp++])==0);
	}
}

struct alias
{
	Sfdisc_t	disc;
	Namval_t	*np;
	int		nextc;
	int		line;
};

/*
 * This code gets called whenever an end of string is found with alias
 */

/*
 * This code gets called whenever an end of string is found with alias
 */
static int alias_exceptf(Sfio_t *iop,int type,Sfdisc_t *handle)
{
	register struct alias *ap = (struct alias*)handle;
	register Namval_t *np;
	static char buf[2];
	if(type==0 || !ap)
		return(0);
	np = ap->np;
	if(type!=SF_READ)
	{
		if(type==SF_CLOSE)
		{
			register Sfdisc_t *dp = sfdisc(iop,SF_POPDISC);
			if(dp!=handle)
				sfdisc(iop,dp);
			else if(ap)
				free((void*)ap);
		}
		goto done;
	}
	if(ap->nextc)
	{
		/* if last character is a blank, then next work can be alias */
		register int c = fcpeek(-1);
		if(isblank(c))
			shlex.aliasok = 1;
		*buf = ap->nextc;
		ap->nextc = 0;
		sfsetbuf(iop,buf,1);
		return(1);
	}
done:
	if(np)
		nv_offattr(np,NV_NOEXPAND);
	return(0);
}


static void setupalias(const char *string,Namval_t *np)
{
	register Sfio_t *iop, *base;
	struct alias *ap = (struct alias*)malloc(sizeof(struct alias));
	ap->disc = alias_disc;
	iop = sfopen(NIL(Sfio_t*),(char*)string,"s");
	if(ap->np = np)
	{
#ifdef SHOPT_KIA
		if(shlex.kiafile)
		{
			unsigned long r;
			r=kiaentity(nv_name(np),-1,'p',0,0,shlex.current,'a',0,"");
			sfprintf(shlex.kiatmp,"p;%..64d;p;%..64d;%d;%d;e;\n",shlex.current,r,sh.inlineno,sh.inlineno);
		}
#endif /* SHOPT_KIA */
		if((ap->nextc=fcget())==0)
			ap->nextc = ' ';
	}
	else
		ap->nextc = 0;
	sfdisc(iop, &ap->disc);
	lexd.nocopy++;
	if(!(base=fcfile()))
		base = sfopen(NIL(Sfio_t*),fcseek(0),"s");
	fcclose();
	sfstack(base,iop);
	fcfopen(base);
	lexd.nocopy--;
}

/*
 * grow storage stack for nested constructs by STACK_ARRAY
 */
static int stack_grow(void)
{
	lex_max += STACK_ARRAY;
	if(lex_match)
		lex_match = (int*)realloc((char*)lex_match,sizeof(int)*lex_max);
	else
		lex_match = (int*)malloc(sizeof(int)*STACK_ARRAY);
	return(lex_match!=0);
}

