#ident	"@(#)ksh93:src/cmd/ksh93/sh/macro.c	1.4"
#pragma prototyped
/*
 * Shell macro expander
 * expands ~
 * expands ${...}
 * expands $(...)
 * expands $((...))
 * expands `...`
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   research!dgk
 *
 */

#include	"defs.h"
#include	<fcin.h>
#include	<pwd.h>
#include	"name.h"
#include	"variables.h"
#include	"shlex.h"
#include	"io.h"
#include	"shnodes.h"
#include	"path.h"
#include	"national.h"

static int	_c_;
static struct  _mac_
{
	Sfio_t		*sp;		/* stream pointer for here-document */
	struct argnod	**arghead;	/* address of head of argument list */
	char		*ifsp;		/* pointer to IFS value */
	int		fields;		/* number of fields */
	short		quoted;		/* set when word has quotes */
	unsigned char	ifs;		/* first char of IFS */
	char		quote;		/* set within double quoted contexts */
	char		lit;		/* set within single quotes */
	char		split;		/* set when word splittin is possible */
	char		pattern;	/* set when file expansion follows */
	char		assign;		/* set for assignments */
	char		arith;		/* set for ((...)) */
} mac;

#undef ESCAPE
#define ESCAPE		'\\'
#define isescchar(s)	((s)>S_QUOTE)
#define isqescchar(s)	((s)>=S_QUOTE)
#define isbracechar(c)	((c)==RBRACE || (_c_=sh_lexstates[ST_BRACE][c])==S_MOD1 ||_c_==S_MOD2)
#define ltos(x)		fmtbase((long)(x),0,0)

/* type of macro expansions */
#define M_BRACE		1	/* ${var}	*/
#define M_SIZE		2	/* ${#var}	*/
#define M_VNAME		3	/* ${!var}	*/
#define M_SUBNAME	4	/* ${!var[sub]}	*/
#define M_NAMESCAN	5	/* ${!var*)	*/
#define M_CLASS		6	/* ${-var)	*/

static char	*nextname(const char*, int);
static int	substring(const char*, const char*, int[], int);
static void	copyto(int, int);
static void	comsubst(int);
static int	varsub(void);
static void	mac_copy(const char*, int);
static void	tilde_expand2(int);
static char 	*sh_tilde(const char*);
static char	*special(int);
static void	endfield(int);
static void	mac_error(Namval_t*);
static char	*mac_getstring(char*);
#ifdef SHOPT_MULTIBYTE
    static int	charlen(const char*);
    static char	*lastchar(const char*,const char*);
#endif /* SHOPT_MULTIBYTE */

/*
 * perform only parameter substitution and catch failures
 */
char *sh_mactry(register char *string)
{
	if(string)
	{
		int		jmp_val;
		struct checkpt	buff;
		sh_pushcontext(&buff,SH_JMPSUB);
		jmp_val = sigsetjmp(buff.buff,0);
		if(jmp_val == 0)
			string = sh_mactrim(string,0);
		sh_popcontext(&buff);
		return(string);
	}
	return("");
}

/*
 * Perform parameter expansion, command substitution, and arithmetic
 * expansion on <str>. 
 * If <mode> greater than 1 file expansion is performed if the result 
 * yields a single pathname.
 * If <mode> negative, than expansion rules for assignment are applied.
 */
char *sh_mactrim(char *str, register int mode)
{
	struct _mac_		savemac;
	savemac = mac;
	stakseek(0);
	mac.arith = (mode==3);
	mac.pattern = (mode==1||mode==2);
	mac.assign = (mode<0);
	mac.quote = mac.lit = mac.split = mac.quoted = 0;
	mac.sp = 0;
	if(mac.ifsp=nv_getval(nv_scoped(IFSNOD)))
		mac.ifs = *mac.ifsp;
	else
		mac.ifs = ' ';
	stakseek(0);
	fcsopen(str);
	copyto(0,0);
	str = stakfreeze(1);
	if(mode==2)
	{
		/* expand only if unique */
		struct argnod *arglist=0;
		if((mode=path_expand(str,&arglist))==1)
			str = arglist->argval;
		else if(mode>1)
			error(ERROR_exit(1),gettxt(e_ambiguous_id,e_ambiguous),str);
		sh_trim(str);
	}
	mac = savemac;
	return(str);
}

/*
 * Perform all the expansions on the argument <argp>
 */
int sh_macexpand(register struct argnod *argp, struct argnod **arghead)
{
	register int flags = argp->argflag;
	register char *str = argp->argval;
	struct _mac_		savemac;
	savemac = mac;
	stakseek(ARGVAL);
	mac.sp = 0;
	mac.arghead = arghead;
	mac.quote = mac.lit = mac.quoted = mac.arith = 0;
	mac.split = !(flags&ARG_ASSIGN);
	mac.assign = !mac.split;
	mac.pattern = mac.split && !sh_isoption(SH_NOGLOB);
	if(mac.ifsp=nv_getval(nv_scoped(IFSNOD)))
		mac.ifs = *mac.ifsp;
	else
		mac.ifs = ' ';
	str = argp->argval;
	fcsopen(str);
	mac.fields = 0;
	copyto(0,0);
	endfield(mac.quoted);
	flags = mac.fields;
	mac = savemac;
	return(flags);
}

/*
 * Expand here document which is stored in <infile> or <string>
 * The result is written to <outfile>
 */
void sh_machere(Sfio_t *infile, Sfio_t *outfile, char *string)
{
	register int	c,n;
	register const char	*state = sh_lexstates[ST_QUOTE];
	register char	*cp;
	Fcin_t		save;
	struct _mac_		savemac;
	savemac = mac;
	stakseek(0);
	mac.sp = outfile;
	mac.split = mac.assign = mac.pattern = mac.lit = mac.arith = 0;
	mac.quote = 1;
	mac.ifs = ' ';
	fcsave(&save);
	if(infile)
		fcfopen(infile);
	else
		fcsopen(string);
	cp = fcseek(0);
	while(1)
	{
		while((n=state[*(unsigned char*)cp++])==0);
		if(n==S_NL || n==S_QUOTE || n==S_RBRA)
			continue;
		if(c=(cp-1)-fcseek(0))
			sfwrite(outfile,fcseek(0),c);
		cp = fcseek(c+1);
		switch(n)
		{
		    case S_EOF:
			if((n=fcfill()) <=0)
			{
				/* ignore 0 byte when reading from file */
				if(n==0 && fcfile())
					continue;
				fcrestore(&save);
				mac = savemac;
				return;
			}
			cp = fcseek(-1);
			continue;
		    case S_ESC:
			fcgetc(c);
			cp=fcseek(-1);
			if(c>0)
				cp++;
			if(!isescchar(state[c]))
				sfputc(outfile,ESCAPE);
			continue;
		    case S_GRAVE:
			comsubst(0);
			break;
		    case S_DOL:
			c = fcget();
			if(c=='.')
				goto regular;
		    again:
			switch(n=sh_lexstates[ST_DOL][c])
			{
			    case S_ALP: case S_SPC1: case S_SPC2:
			    case S_DIG: case S_LBRA:
			    {
				Fcin_t	save2;
				int	offset = staktell();
				int	offset2;
				stakputc(c);
				if(n==S_LBRA)
					sh_lexskip(RBRACE,1,ST_BRACE);
				else if(n==S_ALP)
				{
					while(fcgetc(c),isaname(c))
						stakputc(c);
					fcseek(-1);
				}
				stakputc(0);
				offset2 = staktell();
				fcsave(&save2);
				fcsopen(stakptr(offset));
				varsub();
				if(c=staktell()-offset2)
					sfwrite(outfile,(char*)stakptr(offset2),c);
				fcrestore(&save2);
				stakseek(offset);
				break;
			    }
			    case S_PAR:
				comsubst(1);
				break;
			    case S_EOF:
				if((c=fcfill()) > 0)
					goto again;
				/* FALL THRU */
			    default:
			    regular:
				sfputc(outfile,'$');
				fcseek(-1);
				break;
			}
		}
		cp = fcseek(0);
	}
}

/*
 * Process the characters up to <endch> or end of input string 
 */
static void copyto(int endch, int newquote)
{
	register int	c,n;
	register const char	*state = sh_lexstates[ST_MACRO];
	register char	*cp,*first;
	int		tilde = -1;
	int		oldquote = mac.quote;
	int		ansi_c = 0;
	Sfio_t		*sp = mac.sp;
	mac.sp = NIL(Sfio_t*);
	mac.quote = newquote;
	first = cp = fcseek(0);
	if(!mac.quote && *cp=='~')
		tilde = staktell();
	while(1)
	{
		while((n=state[*(unsigned char*)cp++])==0);
		c = (cp-1) - first;
		switch(n)
		{
		    case S_ESC:
			if(ansi_c)
			{
				/* process ANSI-C escape character */
				char *addr= --cp;
				if(c)
					stakwrite(first,c);
				c = chresc(cp,&addr);
				cp = addr;
				first = fcseek(cp-first);
				stakputc(c);
				if(c==ESCAPE && mac.pattern)
					stakputc(ESCAPE);
				break;
			}
			else if(mac.split && endch && !mac.quote && !mac.lit)
			{
				if(c)
					mac_copy(first,c);
				cp = fcseek(c+2);
				if(c= cp[-1])
				{
					stakputc(c);
					if(c==ESCAPE)
						stakputc(ESCAPE);
				}
				else
					cp--;
				first = cp;
				break;
			}
			n = state[*(unsigned char*)cp];
			if(n==S_ENDCH && *cp!=endch)
				n = S_PAT;
			if(mac.pattern)
			{
				/* preserve \digit for pattern matching */
				if(!mac.lit && !mac.quote && n==S_DIG)
					break;
				/* followed by file expansion */
				if(!mac.lit && (n==S_ESC || (!mac.quote && 
					(n==S_PAT||n==S_ENDCH||n==S_SLASH||n==S_BRACT||*cp=='-'))))
				{
					cp += (n!=S_EOF);
					break;
				}
				if(mac.lit || (mac.quote && !isqescchar(n) && n!=S_ENDCH))
				{
					/* add \ for file expansion */
					stakwrite(first,c+1);
					first = fcseek(c);
					break;
				}
			}
			if(mac.lit)
				break;
			if(!mac.quote || isqescchar(n) || n==S_ENDCH) 
			{
				/* eliminate \ */
				if(c)
					stakwrite(first,c);
				/* check new-line joining */
				first = fcseek(c+1);
			}
			cp += (n!=S_EOF);
			break;
		    case S_GRAVE: case S_DOL:
			if(mac.lit)
				break;
			if(c)
			{
				if(mac.split && !mac.quote && endch)
					mac_copy(first,c);
				else
					stakwrite(first,c);
			}
			first = fcseek(c+1);
			if(n==S_GRAVE)
				comsubst(0);
			else if((n= *cp)==0 || !varsub())
			{
				if(n=='\'' && !mac.quote)
					ansi_c = 1;
				else if(mac.quote || n!='"')
					stakputc('$');
			}
			cp = first = fcseek(0);
			break;
		    case S_ENDCH:
			if((mac.lit || cp[-1]!=endch || mac.quote!=newquote))
				goto pattern;
		    case S_EOF:
			if(c)
			{
				if(mac.split && !mac.quote && !mac.lit && endch)
					mac_copy(first,c);
				else
					stakwrite(first,c);
			}
			c += (n!=S_EOF);
			first = fcseek(c);
			if(tilde>=0)
				tilde_expand2(tilde);
			goto done;
		    case S_QUOTE:
			if(mac.lit)
				break;
		    case S_LIT:
			if(n==S_LIT && mac.quote)
				break;
			if(c)
			{
				if(mac.split && endch && !mac.quote && !mac.lit)
					mac_copy(first,c);
				else
					stakwrite(first,c);
			}
			first = fcseek(c+1);
			if(n==S_LIT)
			{
				if(mac.quote)
					continue;
				if(mac.lit)
					mac.lit = ansi_c = 0;
				else
					mac.lit = 1;
			}
			else
				mac.quote = !mac.quote;
			mac.quoted++;
			break;
		    case S_BRACT:
			if((mac.assign==1 || mac.arith || endch==RBRACT) &&
				 !(mac.quote || mac.lit))
			{
				int oldpat = mac.pattern;
				stakwrite(first,++c);
				first = fcseek(c);
				mac.pattern = mac.assign|mac.arith;
				copyto(RBRACT,0);
				stakputc(RBRACT);
				mac.pattern = oldpat;
				cp = first = fcseek(0);
				break;
			}
		    case S_PAT:
		    pattern:
			if(!mac.pattern || !(mac.quote || mac.lit))
			{
				if(n==S_SLASH && mac.pattern==2)
					mac.pattern=3;
				break;
			}
			if(c)
				stakwrite(first,c);
			first = fcseek(c);
			stakputc(ESCAPE);
			break;
		    case S_EQ:
			if(mac.assign==1)
			{
				if(*cp=='~' && !endch && !mac.quote && !mac.lit)
					tilde = staktell()+(c+1);
				mac.assign = 2;
			}
			break;
		    case S_SLASH:
		    case S_COLON:
			if(tilde >=0)
			{
				if(c)
					stakwrite(first,c);
				first = fcseek(c);
				tilde_expand2(tilde);
				tilde = -1;
				c=0;
			}
			if(n==S_COLON && mac.assign==2 && *cp=='~' && endch==0 && !mac.quote &&!mac.lit)
				tilde = staktell()+(c+1);
			else if(n==S_SLASH && mac.pattern==2)
				goto pattern;
			break;
		}
	}
done:
	mac.sp = sp;
	mac.quote = oldquote;
}

/* #ifndef STR_MAXIMAL					*/
/* #   define STR_MAXIMAL	01			*/
/* #   define STR_LEFT	02				*/
/* #   define STR_RIGHT	04				*/
/*
 * This will be in libast some day
 */
#define strgrpmatch ksh_strgrpmatch /* don't clash with libast's name -- jv */
static int strgrpmatch(char *string, char *pattern, int match[], int nmatch,int flags)
{
	register char *cp=string, *dp;
	int c=0,anchor = (flags&STR_LEFT);
	flags &= ~STR_LEFT;
	/* optimize a little */
	if(!anchor)
	{
		c = *(unsigned char*)pattern;
		if(c=='[' || sh_lexstates[ST_MACRO][c]==S_PAT)
			c=0;
		else if(c==ESCAPE)
			c = (unsigned char)pattern[1];
		else if(pattern[1]==LPAREN)
			c=0;
	}
	do
	{
		if(c)
		{
			while(*cp && *cp!=c)
				cp++;
			if(*cp==0)
				break;
		}
		if(dp=strsubmatch(cp,pattern,flags))
		{
			if(nmatch)
			{
				match[0] = cp-string;
				match[1] = dp-string;
			}
			return(1);
		}
	}
	while(!anchor && *cp++);
	return(0);
}
/* #endif /* !STR_MINIMAL */

/*
 * copy <str> to stack performing sub-expression substitutions
 */
static void mac_substitute(register char *cp,char *str,register int subexp[],int subsize)
{
	register int c,n;
	register char *first=cp;
	while(1)
	{
		while((c= *cp++) && c!=ESCAPE);
		if(c==0)
			break;
		if((n= *cp++) >='0' && n<='9' || n=='\\' || n==RBRACE)
		{
			c = cp-first-2;
			if(c)
				mac_copy(first,c);
			first=cp;
			if(n=='\\' || n==RBRACE)
			{
				first--;
				continue;
			}
			if((n-='0') < subsize && (c=subexp[2*n])>=0)
			{
				if((n=subexp[2*n+1]-c)>0)
					mac_copy(str+c,n);
			}
		}
		else if(n==0)
			break;
	}
	if(n=cp-first-1)
		mac_copy(first,n);
}

/*
 * This routine handles $param,  ${parm}, and ${param op word}
 * The input stream is assumed to be a string
 */
static int varsub(void)
{
	static char	idbuff[2];
	register int	c;
	register int	type=0; /* M_xxx */
	register char	*v,*argp=0;
	register Namval_t	*np = NIL(Namval_t*);
	register int 	dolg=0, mode=0;
	Namarr_t	*ap=0;
	int		dolmax=0, vsize= -1, offset, nulflg, replen=0, bysub=0;
	char		*id = idbuff, *pattern=0, *repstr;
	*id = 0;
retry1:
	switch(sh_lexstates[ST_DOL][c=fcget()])
	{
	    case S_RBRA:
		if(type<M_SIZE)
			goto nosub;
		/* This code handles ${#} */
		c = mode;
		mode = type = 0;
		/* FALL THRU */
	    case S_SPC1:
		if(type==M_BRACE)
		{
			if(isaletter(mode=fcpeek(0)) || mode=='.')
			{
				if(c=='#')
					type = M_SIZE;
#ifdef SHOPT_OO
				else if(c=='-')
					type = M_CLASS;
#endif /* SHOPT_OO */
				else
					type = M_VNAME;
				mode = c;
				goto retry1;
			}
			else if(c=='#' && (isadigit(mode)||fcpeek(1)==RBRACE))
			{
				type = M_SIZE;
				mode = c;
				goto retry1;
			}
		}
		/* FALL THRU */
	    case S_SPC2:
		*id = c;
		v = special(c);
		if(isastchar(c))
		{
			mode = c;
			dolg = (v!=0);
			dolmax = sh.st.dolc+1;
		}
		break;
	    case S_LBRA:
		if(type)
			goto nosub;
		type = M_BRACE;
		goto retry1;
	    case S_PAR:
		if(type)
			goto nosub;
		comsubst(1);
		return(1);
	    case S_DIG:
		c -= '0';
		if(type)
		{
			register int d;
			while((d=fcget()),isadigit(d))
				c = 10*c + (d-'0');
			fcseek(-1);
		}
		if(c==0)
			v = special(c);
		else if(c <= sh.st.dolc)
		{
			sh.used_pos = 1;
			v = sh.st.dolv[c];
		}
		else
			v = 0;
		break;
	    case S_ALP:
		if(c=='.' && type==0)
			goto nosub;
		offset = staktell();
#ifdef SHOPT_COMPOUND_ARRAY
	    more:
#endif /* SHOPT_COMPOUND_ARRAY */
		do
			stakputc(c);
		while((c=fcget()),isaname(c)||(c=='.' && type));
		stakputc(0);
		id=stakptr(offset);
		if(isastchar(c) && type)
		{
			if(type==M_VNAME)
			{
				idbuff[0] = mode = c;
				type = M_NAMESCAN;
				dolg = -1;
				break;
			}
			goto nosub;
		}
		np = nv_open(id,sh.var_tree,NV_NOASSIGN|NV_VARNAME);
		if(type)
		{
			if(c==LBRACT)
			{
				if(type==M_VNAME)
					type = M_SUBNAME;
#ifdef SHOPT_OO
				else if(type==M_CLASS)
					mac_error(np);
#endif /* SHOPT_OO */
				if(((c=fcpeek(0)),isastchar(c)) && fcpeek(1)==']' && fcpeek(2)!='.')
				{
					/* ${id[*]} or ${id[@]} */
					mode=c;
					if(nv_arrayptr(np))
						nv_putsub(np,NIL(char*),ARRAY_SCAN);
					fcseek(2);

				}
				else
				{
					int split = mac.split;
					int xpattern = mac.pattern;
					int loc = staktell();
					mac.split = 0;
					mac.pattern = 0;
					copyto(RBRACT,0);
					mac.pattern = xpattern;
					mac.split = split;
					stakputc(0);
					nv_putsub(np,stakptr(loc),ARRAY_ADD);
#ifdef SHOPT_COMPOUND_ARRAY

					if(fcpeek(0)=='.')
					{
						stakseek(loc-1);
						stakputc('[');
						if(!(id=nv_getsub(np)))
							id = "0";
						stakputs(id);
						stakputc(']');
						c = fcget();
						goto more;
					}
#endif /* SHOPT_COMPOUND_ARRAY */
				}
			}
			else if(!isbracechar(c))
				goto nosub;
			else
				fcseek(-1);
		}
		else
			fcseek(-1);
		ap = nv_arrayptr(np);
		c = (type>M_BRACE && isastchar(mode));
		id = nv_name(np);
		if(!c || !ap)
		{
			if(type==M_VNAME)
			{
				type = M_BRACE;
				v = nv_name(np);
			}
#ifdef SHOPT_OO
			else if(type==M_CLASS)
			{
				type = M_BRACE;
				if(np = nv_class(np))
					v = nv_name(np);
				else
					v = 0;
			}
#endif /* SHOPT_OO */
			else
				v = nv_getval(np);
		}
		else
			v = 0;
		stakseek(offset);
		if(ap)
		{
			if(isastchar(mode) && array_elem(ap)> !c)
				dolg = -1;
			else
				dolg = 0;
		}
		break;
	    case S_EOF:
		fcseek(-1);
	    default:
		goto nosub;
	}
	c = fcget();
	if(type>M_BRACE)
	{
		if(c!=RBRACE)
			mac_error(np);
		if(type==M_NAMESCAN)
		{
			id = strdup(id);
			dolmax = staktell()-offset-1;
			stakseek(offset);
			nextname(NIL(char*),0);
			v = nextname(id,dolmax);
		}
		else if(type==M_SUBNAME)
		{
			if(dolg<0)
			{
				v = nv_getsub(np);
				bysub=1;
			}
			else if(v)
			{
				if(!ap || isastchar(mode))
					v = "0";
				else
					v = nv_getsub(np);
			}
		}
		else
		{
			if(!isastchar(mode))
#ifdef SHOPT_MULTIBYTE
				c = (v?charlen(v):0);
#else
				c = (v?strlen(v):0);
#endif /* SHOPT_MULTIBYTE */
			else if(dolg>0)
				c = sh.st.dolc;
			else if(dolg<0)
				c = array_elem(ap);
			else
				c = (v!=0);
			dolg = dolmax = 0;
			v = ltos(c);
		}
		c = RBRACE;
	}
	/* check for quotes @ */
	if(mode=='@' && mac.quote && !v)
		mac.quoted-=2;
	nulflg = 0;
	if(type)
	{
		if(c==':')
		{
			nulflg=1;
			c = fcget();
		}
		if(!isbracechar(c))
		{
			if(!nulflg)
				mac_error(np);
			fcseek(-1);
			c = ':';
		}
		if(c!=RBRACE)
		{
			int newops = (c=='#' || c == '%' || c=='/');
			offset = staktell();
			if(c=='/' ||c==':' || ((!v || (nulflg && *v==0)) ^ (c=='+'||c=='#'||c=='%')))
			{
				int newquote = mac.quote;
				int xpattern = mac.pattern;
				int split = mac.split;
				if(newops)
				{
					type = fcget();
					fcseek(-1);
					mac.pattern = 1+(c=='/');
					mac.split = 0;
					newquote = 0;
				}
				else if(c=='?' || c=='=')
					mac.split = mac.pattern = 0;
				copyto(RBRACE,newquote);
				mac.pattern = xpattern;
				mac.split = split;
				/* add null byte */
				stakputc(0);
				stakseek(staktell()-1);
			}
			else
			{
				sh_lexskip(RBRACE,0,(!newops&&mac.quote)?ST_QUOTE:ST_NESTED);
				stakseek(offset);
			}
			argp=stakptr(offset);
		}
	}
	else
	{
		fcseek(-1);
		c=0;
	}
	if(c==':')  /* ${name:expr1[:expr2]} */
	{
		char *ptr;
		if((type = (int)sh_strnum(argp,&ptr,1)) < 0)
			type=0;
		if(isastchar(mode))
		{
			if(!ap)  /* ${@} or ${*} */
			{
				if(type==0)
					v = special(dolg=0);
				else if(type < dolmax)
					v = sh.st.dolv[dolg=type];
				else
					v =  0;
			}
			else
			{
				if(array_assoc(ap))
				{
					while(type-- >0 && (v=0,nv_nextsub(np)))
						v = nv_getval(np);
				}
				else if(type > 0)
				{
					nv_putsub(np,NIL(char*),type|ARRAY_SCAN);
					v = nv_getval(np);
				}
			}
			
		}
		else if(v)
		{
			if((vsize=strlen(v)) < type)
				v = 0;
			else
			{
				v += type;
				vsize -= type;
			}
		}
		if(*ptr==':')
		{
			if((type = (int)sh_strnum(ptr+1,&ptr,1)) <=0)
				v = 0;
			else if(isastchar(mode))
			{
				if(dolg>0)
				{
					if(dolg+type < dolmax)
						dolmax = dolg+type;
				}
				else
					dolmax = type;
			}
			else if(type < vsize)
				vsize = type;
		}
		if(*ptr)
			mac_error(np);
		stakseek(offset);
		argp = 0;
	}
	/* check for substring operations */
	else if(c == '#' || c == '%' || c=='/')
	{
		if(c=='/')
		{
			if(type=='/' || type=='#' || type=='%')
			{
				c = type;
				type = '/';
				argp++;
			}
			else
				type = 0;
		}
		else
		{
			if(type==c) /* ## or %% */
				argp++;
			else
				type = 0;
		}
		pattern = strdup(argp);
		if((type=='/' || c=='/') && (repstr = mac_getstring(pattern)))
			replen = strlen(repstr);
		if(v || c=='/')
			stakseek(offset);
	}
retry2:
	if(v && (!nulflg || *v ) && c!='+')
	{
		register int d = (mode=='@'?' ':mac.ifs);
		int match[20], nmatch;
		while(1)
		{
			if(!v)
				v= "";
			if(c=='/' || c=='#' || c== '%')
			{
				int flag = (type || c=='/')?STR_MAXIMAL:0;
				if(c!='/')
					flag |= STR_LEFT;
				while(1)
				{
					vsize = strlen(v);
					if(c=='%')
						nmatch=substring(v,pattern,match,flag&STR_MAXIMAL);
					else
						nmatch=strgrpmatch(v,pattern,match,10,flag);
					if(nmatch)
						vsize = match[0];
					else if(c=='#')
						vsize = 0;
					if(vsize)
						mac_copy(v,vsize);
					if(nmatch && replen>0)
						mac_substitute(repstr,v,match,nmatch);
					if(nmatch==0)
						v += vsize;
					else
						v += match[1];
					if(*v &&  c=='/' && type)
					{
						/* avoid infinite loop */
						if(nmatch && match[1]==0)
							v++;
						continue;
					}
					vsize = -1;
					break;
				}
			}
			if(vsize)
				mac_copy(v,vsize>0?vsize:strlen(v));
			if(dolg==0 && dolmax==0)
				 break;
			if(dolg>=0)
			{
				if(++dolg >= dolmax)
					break;
				v = sh.st.dolv[dolg];
			}
			else if(!np)
			{
				if(!(v = nextname(id,dolmax)))
					break;
			}
			else
			{
				if(dolmax &&  --dolmax <=0)
				{
					nv_putsub(np,NIL(char*),ARRAY_UNDEF);
					break;
				}
				if(nv_nextsub(np) == 0)
					break;
				if(bysub)
					v = nv_getsub(np);
				else
					v = nv_getval(np);
			}
			if(mac.split && (!mac.quote || mode=='@'))
				endfield(mac.quoted);
			else if(d)
			{
				if(mac.sp)
					sfputc(mac.sp,d);
				else
					stakputc(d);
			}
		}
		if(pattern)
			free((void*)pattern);
	}
	else if(argp)
	{
		if(c=='/' && replen>0 && pattern && strmatch("",pattern))
			mac_substitute(repstr,v,0,0);
		if(c=='?')
		{
			if(*argp)
			{
				stakputc(0);
				error(ERROR_exit(1),gettxt(":108","%s: %s"),id,argp);
			}
			else
				error(ERROR_exit(1),gettxt(e_nullset_id,e_nullset),id);
		}
		else if(c=='=')
		{
			if(np)
			{
				if(sh.subshell)
					np = sh_assignok(np,1);
				nv_putval(np,argp,0);
				v = nv_getval(np);
				nulflg = 0;
				stakseek(offset);
				goto retry2;
			}
		else
			mac_error(np);
		}
	}
	else if(sh_isoption(SH_NOUNSET))
	{
		nv_close(np);
		error(ERROR_exit(1),gettxt(e_notset_id,e_notset),id);
	}
	nv_close(np);
	return(1);
nosub:
	if(type)
		mac_error(np);
	fcseek(-1);
	nv_close(np);
	return(0);
}

/*
 * This routine handles command substitution
 * <type> is 0 for older `...` version
 */
static void comsubst(int type)
{
	register int		c;
	register char		*str;
	Sfio_t			*sp;
	Fcin_t			save;
	struct slnod            *saveslp = sh.st.staklist;
	struct _mac_		savemac;
	int			savtop = staktell();
	char			*buff;
	char			*savptr = stakfreeze(0);
	int			saveflags = sh_isstate(SH_HISTORY|SH_VERBOSE);
	int			newlines;
	register union anynode	*t;
	savemac = mac;
	sh.st.staklist=0;
	if(type)
	{
		sp = 0;
		fcseek(-1);
		t = sh_dolparen();
		if(t && t->tre.tretyp==TARITH)
		{
			double num;
			char *cp =  t->ar.arexpr->argval;
			fcsave(&save);
			if(!(t->ar.arexpr->argflag&ARG_RAW))
				cp = sh_mactrim(cp,0);
			num = sh_arith(cp);
			stakset(savptr,savtop);
			mac = savemac;
			if((long)num==num)
				str = ltos(num);
			else
				str = sh_etos(num,12);
			mac_copy(str,strlen(str));
			sh.st.staklist = saveslp;
			fcrestore(&save);
			return;
		}
	}
	else
	{
		while(fcgetc(c)!='`' && c)
		{
			if(c==ESCAPE)
			{
				fcgetc(c);
				if(!(isescchar(sh_lexstates[ST_QUOTE][c]) ||
				  (c=='"' && mac.quote)) || (c=='$' && fcpeek(0)=='\''))
					stakputc(ESCAPE);
			}
			stakputc(c);
		}
		c = staktell();
		str=stakfreeze(1);
		/* disable verbose and don't save in history file */
		sh_offstate(SH_HISTORY|SH_VERBOSE);
		if(mac.sp)
			sfsync(mac.sp);	/* flush before executing command */
		sp = sfnew(NIL(Sfio_t*),str,c,-1,SF_STRING|SF_READ);
		c = sh.inlineno;
		sh.inlineno = error_info.line+sh.st.firstline;
		t = (union anynode*)sh_parse(sp,SH_EOF|SH_NL);
		sh.inlineno = c;
	}
#ifdef KSHELL
	if(t)
	{
		fcsave(&save);
		sfclose(sp);
		if(t->tre.tretyp==0 && !t->com.comarg)
		{
			/* special case $( < file) */
			register int fd;
			struct checkpt buff;
			sh_pushcontext(&buff,SH_JMPIO);
			if(t->tre.treio && !(((t->tre.treio)->iofile)&IOUFD) &&
				sigsetjmp(buff.buff,0)==0)
				fd = sh_redirect(t->tre.treio,3);
			else
				fd = sh_chkopen((char*)"/dev/null");
			sh_popcontext(&buff);
			sp = sfnew(NIL(Sfio_t*),(char*)malloc(IOBSIZE+1),IOBSIZE,fd,SF_READ|SF_MALLOC);
		}
		else
			sp = sh_subshell(t,sh_isstate(SH_ERREXIT),1);
		fcrestore(&save);
	}
	else
		sp = sfopen(NIL(Sfio_t*),"","sr");
	sh_freeup();
	sh.st.staklist = saveslp;
	sh_onstate(saveflags);
#else
	sp = sfpopen(NIL(Sfio_t*),str,"r");
#endif
	mac = savemac;
	stakset(savptr,savtop);
	newlines = 0;
	/* read command substitution output and put on stack or here-doc */
	while((str=buff=(char*)sfreserve(sp,SF_UNBOUND,0)) && (c = sfslen())>0)
	{
#ifdef SHOPT_CRNL
		/* eliminate <cr> */
		register char *dp;
		while(c-->0 && *str !='\r')
			str++;
		dp = str;
		while(c>0)
		{
			str++;
			c--;
			while(c-->0 && *str!='\r')
				*dp++ = *str++;
		}
		*dp = 0;
		str = buff;
		c = dp-str;
#endif /* SHOPT_CRNL */
		if(newlines >0)
		{
			if(mac.sp)
				sfnputc(mac.sp,'\n',newlines);
			else if(!mac.quote && mac.split && sh.ifstable['\n'])
				endfield(0);
			else	while(newlines--)
					stakputc('\n');
			newlines = 0;
		}
		/* delay appending trailing new-lines */
		while(str[--c]=='\n')
			newlines++;
		str[++c] = 0;
		mac_copy(str,c);
	}
	sfclose(sp);
	sh_onstate(saveflags);
	return;
}

/*
 * copy <str> onto the stack
 */
static void mac_copy(register const char *str, register int size)
{
	register char		*state;
	register const char	*cp=str;
	register int		c,n,nopat;
	nopat = (mac.quote||mac.assign==1||mac.arith);
	if(mac.sp)
		sfwrite(mac.sp,str,size);
	else if(mac.pattern>=2 || (mac.pattern && nopat))
	{
		state = sh_lexstates[ST_MACRO];
		/* insert \ before file expansion characters */
		while(size-->0)
		{
			c = state[*(unsigned char*)cp++];
			if(nopat&&(c==S_PAT||c==S_ESC||c==S_BRACT||c==S_ENDCH))
				c=1;
			else if(mac.pattern==2 && c==S_SLASH)
				c=1;
			else if(mac.pattern==3 && c==S_ESC && state[*(unsigned char*)cp]==S_DIG)
				c=1;
			else
				c=0;
			if(c)
			{
				if(c = (cp-1) - str)
					stakwrite(str,c);
				stakputc(ESCAPE);
				str = cp-1;
			}
		}
		if(c = cp-str)
			stakwrite(str,c);
	}
	else if(!mac.quote && mac.split && (mac.ifs||mac.pattern))
	{
		/* split words at ifs characters */
		state = sh.ifstable;
		if(mac.pattern)
		{
			char *sp = "&|()";
			while(c = *sp++)
			{
				if(state[c]==0)
					state[c] = S_PAT;
			}
			if(sh.ifstable[ESCAPE]==0)
				sh.ifstable[ESCAPE] = S_ESC;
		}
		while(size-->0)
		{
			if((n=state[c= *(unsigned char*)cp++])==S_ESC)
			{
				stakputc(ESCAPE);
				if((n=sh_lexstates[ST_MACRO][*(unsigned char*)cp])==S_PAT || n==S_DIG || *cp=='-')
				{
					stakputc(ESCAPE);
					stakputc(ESCAPE);
					size--;
					c = *cp++;
				}
			}
			else if(n==S_PAT)
				stakputc(ESCAPE);
			else if(n && mac.ifs)
			{
#ifdef SHOPT_MULTIBYTE
				if(n==S_MBYTE)
				{
					if(sh_strchr(mac.ifsp,cp-1)<0)
						continue;
					n = mblen(cp-1,MB_CUR_MAX)-1;
					cp += n;
					size -= n;
					n= S_DELIM;
				}
#endif /* SHOPT_MULTIBYTE */
				endfield(n==S_DELIM||mac.quoted);
				if(n==S_SPACE || n==S_NL)
				{
					while(size>0 && ((n=state[c= *(unsigned char*)cp++])==S_SPACE||n==S_NL))
						size--;
#ifdef SHOPT_MULTIBYTE
					if(n==S_MBYTE && sh_strchr(mac.ifsp,cp-1)>=0)
					{
						n = mblen(cp-1,MB_CUR_MAX)-1;
						cp += n;
						size -= n;
						n=S_DELIM;
					}
#endif /* SHOPT_MULTIBYTE */
				}
				if(n==S_DELIM)
					while(size>0 && ((n=state[c= *(unsigned char*)cp++])==S_SPACE||n==S_NL))
						size--;
				if(size<=0)
					break;
				cp--;
				continue;

			}
			stakputc(c);
		}
		if(mac.pattern)
		{
			cp = "&|()";
			while(c = *cp++)
			{
				if(state[c]==S_PAT)
					state[c] = 0;
			}
			if(sh.ifstable[ESCAPE]==S_ESC)
				sh.ifstable[ESCAPE] = 0;
		}
	}
	else
		stakwrite(str,size);
}

/*
 * Terminate field.
 * If field is null count field if <split> is non-zero
 * Do filename expansion of required
 */
static void endfield(int split)
{
	register struct argnod *argp;
	if(staktell() > ARGVAL || split)
	{
		argp = (struct argnod*)stakfreeze(1);
		argp->argflag = 0;
		if(mac.pattern)
#ifdef SHOPT_BRACEPAT
			mac.fields += path_generate(argp,mac.arghead);
#else
			mac.fields += path_expand(argp->argval,mac.arghead);
#endif /* SHOPT_BRACEPAT */
		else
		{
			argp->argchn.ap = *mac.arghead;
			*mac.arghead = argp;
			mac.fields++;
		}
		(*mac.arghead)->argflag |= ARG_MAKE;
		if(mac.assign || sh_isoption(SH_NOGLOB))
			argp->argflag |= ARG_RAW;
		stakseek(ARGVAL);
	}
	mac.quoted = mac.quote;
}

/*
 * Finds the right substring of STRING using the expression PAT
 * the longest substring is found when FLAG is set.
 */
static int substring(register const char *string,const char *pat,int match[], int flag)
{
	register const char *sp=string;
	register int size,len,nmatch,n;
	int smatch[20];
	sp += (len=strlen(sp));
	size = sp-string;
	while(sp>=string)
	{
#ifdef SHOPT_MULTIBYTE
		if(MB_CUR_MAX>1)
			sp = lastchar(string,sp);
#endif /* SHOPT_MULTIBYTE */
		 if(n=strgrpmatch((char *)sp,(char *)pat,smatch,10,STR_RIGHT|STR_LEFT|STR_MAXIMAL))
		{
			nmatch = n;
			memcpy(match,smatch,n*2*sizeof(int));
			size = sp-string;
			if(flag==0)
				break;
		}
		sp--;
	}
	if(size==len)
		return(0);
	if(nmatch)
	{
		nmatch *=2;
		while(--nmatch>=0)
			match[nmatch] += size;
	}
	return(1);
}

#ifdef SHOPT_MULTIBYTE
	static char	*lastchar(const char *string, const char *endstring)
	{
		register char *str = (char*)string;
		register int c;
		mblen(NIL(char*),MB_CUR_MAX);
		while(*str)
		{
			if((c=mblen(str,MB_CUR_MAX))<0)
				c = 1;
			if(str+c > endstring)
				break;
			str += c;
		}
		return(str);
	}
	static int	charlen(const char *string)
	{
		register const char *str = string;
		register int n=0;
		register int c;
		int width;
		wchar_t w;
		mblen(NIL(char*),MB_CUR_MAX);
		while(*str)
		{
			if((c=mbtowc(&w,str,MB_CUR_MAX))>0)
			{
				width = wcwidth(w);
				if (width < 0)
					width = 1;
				n += width;
				str += c;
			}
			else
				str++;
		}
		return(n);
	}
#endif /* SHOPT_MULTIBYTE */

/*
 * <offset> is byte offset for beginning of tilde string
 */

static void tilde_expand2(register int offset)
{
	register char *cp;
	int curoff = staktell();
	stakputc(0);
	if(cp = sh_tilde(stakptr(offset)))
	{
		stakseek(offset);
		stakputs(cp);
	}
	else
		stakseek(curoff);
}

/*
 * This routine is used to resolve ~ expansion.
 * A ~ by itself is replaced with the users login directory.
 * A ~- is replaced by the previous working directory in shell.
 * A ~+ is replaced by the present working directory in shell.
 * If ~name  is replaced with login directory of name.
 * If string doesn't start with ~ or ~... not found then 0 returned.
 */
                                                            
static char *sh_tilde(register const char *string)
{
	register char		*cp;
	register int		c;
	register struct passwd	*pw;
	register Namval_t *np;
	static Hashtab_t *logins_tree;
	if(*string++!='~')
		return(NIL(char*));
	if((c = *string)==0)
	{
		if(!(cp=nv_getval(nv_scoped(HOME))))
			cp = getlogin();
		return(cp);
	}
	if((c=='-' || c=='+') && string[1]==0)
	{
		if(c=='+')
			cp = nv_getval(nv_scoped(PWDNOD));
		else
			cp = nv_getval(nv_scoped(OLDPWDNOD));
		return(cp);
	}
	if(logins_tree && (np=nv_search(string,logins_tree,0)))
		return(nv_getval(np));
	if(!(pw = getpwnam(string)))
		return(NIL(char*));
	if(!logins_tree)
		logins_tree = hashalloc(sh.bltin_tree,HASH_set,HASH_ALLOCATE,0);
	if(np=nv_search(string,logins_tree,NV_ADD))
		nv_putval(np, pw->pw_dir,0);
	return(pw->pw_dir);
}
 
/*
 * return values for special macros
 */
static char *special(register int c)
{
	register Namval_t *np;
	switch(c)
	{
	    case '@':
	    case '*':
		return(sh.st.dolc>0?sh.st.dolv[1]:NIL(char*));
	    case '#':
		return(ltos(sh.st.dolc));
	    case '!':
		if(sh.bckpid)
			return(ltos(sh.bckpid));
		break;
	    case '$':
		if(nv_isnull(SH_DOLLARNOD))
			return(ltos(sh.pid));
		return(nv_getval(SH_DOLLARNOD));
	    case '-':
		return(sh_argdolminus());
	    case '?':
		return(ltos(sh.savexit));
	    case 0:
		if(sh_isstate(SH_PROFILE) || !error_info.id || ((np=nv_search(error_info.id,sh.bltin_tree,0)) && nv_isattr(np,BLT_SPC)))
			return(sh.shname);
		else
			return(error_info.id);
	}
	return(NIL(char*));
}

/*
 * Handle macro expansion errors
 */
static void mac_error(Namval_t *np)
{
	nv_close(np);
	error(ERROR_exit(1),gettxt(e_subst_id,e_subst),fcfirst());
}

/*
 * Given pattern/string, replace / with 0 and return pointer to string
 * \ characters are stripped from string.
 */ 
static char *mac_getstring(char *pattern)
{
	register char *cp = pattern;
	register int c;
	while(c = *cp++)
	{
		if(c==ESCAPE)
			cp++;
		else if(c=='/')
		{
			cp[-1] = 0;
			return(cp);
		}
	}
	return(NIL(char*));
}

/*
 * return the next name starting with the given prefix
 */
static char *nextname(const char *prefix, int len)
{
	static Hashpos_t *hp;
	register Namval_t *np;
	register char *cp;
	if(!prefix)
		hp = hashscan(sh.var_tree,0);
	else if(hp)
	{
		while(np = (Namval_t*)hashnext(hp))
		{
#ifdef SHOPT_OO
			Namval_t *nq=np;
			while(nq && nv_isnull(nq))
				nq = nv_class(nq);
			if(!nq)
				continue;
				
#else
			if(nv_isnull(np))
				continue;
#endif /* SHOPT_OO */
			cp = nv_name(np);
			if(memcmp(cp,prefix,len)==0)
				return(cp);
		}
		free((char*)prefix);
	}
	return(NIL(char*));
}
