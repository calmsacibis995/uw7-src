#ident	"@(#)ksh93:src/cmd/ksh93/sh/name.c	1.2.1.1"
#pragma prototyped
/*
 * AT&T Laboratories
 *
 */

#include	"defs.h"
#include	<ctype.h>
#include	"variables.h"
#include	"path.h"
#include	"lexstates.h"
#include	"timeout.h"
#include	"FEATURE/locale"
#include	"national.h"

extern char	*gettxt();

#ifdef apollo
    extern void	ev_$delete_var();
    extern void	ev_$set_var();
#endif	/* apollo */

static void	attstore(Namval_t*);
static void	pushnam(Namval_t*);
static char	*staknam(Namval_t*, char*);
static void	utol(const char*,char*);
static void	ltou(const char*,char*);
static void	rightjust(char*, int, int);

static char	**argnam;
static int	attsize;
static char	*attval;
static char	forced;
static char	local;
static void(*nullscan)(Namval_t*);

/* ========	name value pair routines	======== */

#include	"shnodes.h"
#include	"builtins.h"
/*
 * Perform parameter assignment for a linked list of parameters
 * <flags> contains attributes for the parameters
 */
void nv_setlist(register struct argnod *arg,register int flags)
{
	register char *cp;
	register Namval_t *np;
	int traceon = (sh_isoption(SH_XTRACE)!=0);
	if(sh_isoption(SH_ALLEXPORT))
		flags |= NV_EXPORT;
	if(sh.prefix)
	{
		flags &= ~(NV_IDENT|NV_EXPORT);
		flags |= NV_VARNAME;
	}
	for(;arg; arg=arg->argnxt.ap)
	{
		sh.used_pos = 0;
		if(arg->argflag&ARG_MAC)
			cp = sh_mactrim(arg->argval,-1);
		else
		{
			stakseek(0);
			if(*arg->argval==0)
			{
				int flag = (NV_VARNAME|NV_ARRAY|NV_ASSIGN);
				struct fornod *fp=(struct fornod*)arg->argchn.ap;
				register union anynode *tp=fp->fortre;
				char *prefix = sh.prefix;
				cp = fp->fornam;
				error_info.line = fp->fortyp-sh.st.firstline;
				if(sh.fn_depth && (Namval_t*)tp->com.comnamp==SYSTYPESET)
			                flag |= NV_NOSCOPE;
				np = nv_open(cp,sh.var_tree,flag);
				nv_unset(np);
				/* check for array assignment */
				if(tp->tre.tretyp!=TLST && !tp->com.comset)
				{
					int argc;
					char **argv = sh_argbuild(&argc,&tp->com);
					nv_setvec(np,argc,argv);
					if(traceon)
					{
						sh_trace(NIL(char**),0);
						sfputr(sfstderr,nv_name(np),'=');
						sfwrite(sfstderr,"( ",2);
						while(cp= *argv++)
							sfputr(sfstderr,sh_fmtq(cp),' ');
						sfwrite(sfstderr,")\n",2);
					}
					continue;
				}
				if(tp->tre.tretyp==TLST || tp->com.comset->argval[0]!='[')
					nv_setvtree(np);
				else
					nv_setarray(np,nv_associative);
				if(prefix)
				{
					stakputs(prefix);
					stakputc('.');
					stakputs(cp);
					cp = stakfreeze(1);
				}
				sh.prefix = cp;
				sh_exec(tp,sh_isstate(SH_ERREXIT));
				sh.prefix = prefix;
				continue;
			}
			cp = arg->argval;
		}
		np = nv_open(cp,sh.var_tree,flags);
		if(sh.used_pos)
			np->nvsize |= NV_PARAM;
		else
			np->nvsize &= ~NV_PARAM;
		if(traceon)
		{
			register char *sp=cp;
			sh_trace(NIL(char**),0);
			sfputr(sfstderr,nv_name(np),-1);
			if(nv_isattr(np,NV_ARRAY) && (cp=strchr(sp,'[')))
			{
				/* no quoting needed up to the = */
				sp = nv_endsubscript(np,cp,0);
				sfwrite(sfstderr,cp,sp -cp);
				cp = sp;
			}
			else
				cp = strchr(sp,'=');
			if(cp)
				sfprintf(sfstderr,"=%s\n",sh_fmtq(cp+1));
		}
	}
}

/*
 * construct a new name from a prefix and base name on the stack
 */
static char *newname(register const char *prefix, register const char *name, const char *sub)
{
	register int offset = staktell();
	stakputs(prefix);
	if(sub)
	{
		stakputc('[');
		stakputs(sub);
		stakputc(0);	/* replace by ']' later */
	}
	if(*name!='[')
		stakputc('.');
	stakputs(name);
	stakputc(0);
	stakseek(offset);
	return(stakptr(offset));
}

/*
 * Put <arg> into associative memory.
 * If <flags> & NV_ARRAY then subscript is not allowed
 * If <flags> & NV_NOSCOPE then use the current scope only
 * If <flags> & NV_ASSIGN then assignment is allowed
 * If <flags> & NV_IDENT then name must be an identifier
 * If <flags> & NV_VARNAME then name must be a valid variable name
 * If <flags> & NV_NOADD then node will not be added if not found
 * SH_INIT is only set while initializing the environment
 */
Namval_t	*nv_open(const char *name,Hashtab_t *root,int flags)
{
	register char *cp = (char*)name;
	register Namval_t	*np=0;
	register int sep = *cp;
	register char *lastdot = 0;
	register long mode = ((flags&NV_NOADD)?0:NV_ADD);
	if(root==sh.alias_tree)
	{
		while((sep= *(unsigned char*)cp) && (sep!='=') && (sep!='/') && 
			(!(sep=sh_lexstates[ST_NORM][sep]) || sep==S_EPAT))
			cp++;
	}
	else
	{
		if(!root)
			root = sh.var_tree;
		if(!(flags&(NV_IDENT|NV_VARNAME|NV_ASSIGN)))
		{
			if(flags&NV_NOSCOPE)
				mode |= HASH_SCOPE|HASH_NOSCOPE;
			np = nv_search(name,root,mode);
			if(np && !(flags&NV_REF))
			{
				while(nv_isattr(np,NV_REF))
					np=np->nvalue.np;
			}
			return(np);
		}
		if(sh.prefix && (flags&NV_ASSIGN))
			name = cp = newname(sh.prefix,name,0);
		/* first skip over alpha-numeric */
		while(1)
		{
			if(sep=='.')
			{
				if(flags&NV_IDENT)
					goto failed;
				if(root==sh.var_tree)
					flags &= ~(NV_NOSCOPE|NV_EXPORT);
				if(!lastdot && cp!=name && (flags&NV_VARNAME))
				{
					/* see whether first component is ref */
					*cp = 0;
					np = nv_search(name,sh.var_tree,0);
					*cp = '.';
					if(np && nv_isattr(np,NV_REF))
					{
						/* substitute ref name */
						char *sub = np->nvenv;
						while(nv_isattr(np,NV_REF))
							np=np->nvalue.np;
						name = nv_name(np);
						sep = strlen(name);
						name=lastdot=newname(name,cp+1,sub);
						if(sub)
						{
							cp = (char*)name +strlen(name);
							*cp = ']';
							cp += 2;
						}
						else
							cp = lastdot + sep +1;
					}
					else
						lastdot = cp++;
				}
				else
					lastdot = cp++;
			}
			if(sep= *(unsigned char*)cp, !isaletter(sep))
				break;
			while(sep= *(unsigned char*)(++cp),isaname(sep));
		}
#ifdef SHOPT_COMPOUND_ARRAY
	retry:
#endif /* SHOPT_COMPOUND_ARRAY */
		/* if name doesn't have to be an varname or ident skip to '=' */
#ifdef SHOPT_APPEND
		if(sep && sep!='=' && sep!='[' && sep!='+')
#else
		if(sep && sep!='=' && sep!='[')
#endif /* SHOPT_APPEND */
		{
			if(flags&NV_IDENT)
				goto failed;
			else if(flags&NV_VARNAME)
				if (root==sh.var_tree)
					error(ERROR_exit(1),gettxt(e_varname_id,e_varname),name);
				else
					error(ERROR_exit(1),gettxt(e_funname_id,e_funname),name);
			while((sep= *++cp) && sep!='=');
		}
		if(lastdot)
		{
			*cp = 0;
			np = nv_search(name,root,0);
			*cp = sep;
		}
		while(!np && lastdot && lastdot>name)
		{
			*lastdot=0;
			np = nv_search(name,sh.var_tree,0);
			*lastdot='.';
			if(np)
			{
				char *sp;
				Namval_t *nq;
				/* if(nv_isattr(np,NV_TABLE))
					return(nv_open(lastdot+1,np->nvalue.hp,flags|NV_NOSCOPE)); */
				if(np->nvfun && (np->nvfun)->disc->create)
				{
					*cp = 0;
					np = nv_create(nq=np,lastdot+1,(Namfun_t*)np);
					*cp = sep;
					if(np==nq)
						error(ERROR_exit(1),gettxt(e_varname_id,e_varname),name);
				}
				else if((sp=strchr(lastdot+1,'.')) && sp<cp)
					error(ERROR_exit(1),gettxt(e_noparent_id,e_noparent),name);
				else
					np = 0;
				break;
			}
			while(--lastdot>name && *lastdot!='.');
			if(lastdot==name || (!np && root!=sh.var_tree))
				error(ERROR_exit(1),gettxt(e_noparent_id,e_noparent),name);
		}
	}
	if(cp!=name)
	{
		if(sep && sh.subshell && root==sh.alias_tree)
			root = sh_subaliastree();
		if(sep)
			*cp = 0;
		if((flags&NV_NOSCOPE) && hashscope(root) && root==sh.var_tree 
			&& (np=nv_search(name,sh.var_base,0)))
		{
			Namfun_t *disc = nv_cover(np);
			if(np=nv_search((char*)np,root,mode|HASH_NOSCOPE|HASH_SCOPE|HASH_BUCKET))
				np->nvfun = disc;
		}
		else if(!np)
		{
			if(flags&NV_NOSCOPE)
				mode |= HASH_SCOPE|HASH_NOSCOPE;
			np = nv_search(name,root,mode);
		}
		if(sep)
			*cp = sep;
		if((flags&NV_REF) || (!np && (flags&NV_NOADD)))
			return(np);
		while(nv_isattr(np,NV_REF))
			np=np->nvalue.np;
		/* check for subscript*/
		if(sep=='[' && !(flags&NV_ARRAY))
		{
			sep = (flags&NV_ASSIGN?NV_ADD:0);
#ifdef SHOPT_COMPOUND_ARRAY
			lastdot = nv_endsubscript(np,cp,NV_ADD);
			if((sep = *lastdot)=='.')
			{
				char *sub = nv_getsub(np);
				*cp = 0;
				name = newname(name,lastdot+1,sub?sub:"0");
				*cp = '[';
				cp = name + strlen(name);
				*cp++ = ']';
				lastdot = 0;
				while(1)
				{
					while(sep= *(unsigned char*)(++cp),isaname(sep));
					if(sep!='.')
						break;
					lastdot = cp;
				}
				goto retry;
			}
			cp = lastdot;
#else
			cp = nv_endsubscript(np,cp,NV_ADD);
			sep = *cp;
#endif /* SHOPT_COMPOUND_ARRAY */
		}
		else if(nv_isattr(np,NV_ARRAY))
			nv_putsub(np,NIL(char*),ARRAY_UNDEF);
#ifdef SHOPT_APPEND
		if(sep=='+')
			sep = *++cp;
#endif /* SHOPT_APPEND */
		if(sep && ((sep!='=')||!(flags&NV_ASSIGN)))
		{
			if(sh_isstate(SH_INIT))
				return(0);
			goto failed;
		}
		if(root==sh.alias_tree)
		{
			if(nv_isattr(np,NV_TAGGED|NV_NOALIAS))
				nv_offattr(np,(NV_NOALIAS|NV_TAGGED|NV_EXPORT));
		}
		if(sep == '=')
		{
			cp++;
			if(sh_isstate(SH_INIT))
			{
				nv_putval(np, cp, NV_RDONLY);
				if(np==PWDNOD)
					nv_onattr(np,NV_TAGGED);
			}
			else
			{
				nv_putval(np, cp, 0);
#ifdef SHOPT_BSH
				if(flags&NV_EXPORT)
					nv_offattr(np,NV_IMPORT);
#endif /* SHOPT_BSH */
			}
			nv_onattr(np, flags&NV_ATTRIBUTES);
#ifdef apollo
			/*
			 * Set environment variable defined in the underlying
			 * DOMAIN_OS cache. This is done because dsee will only
			 * process the path if it has changed since the last
			 * time it looked.
			 */
			if(nv_isattr(np,NV_EXPORT) && !nv_isattr(np,NV_IMPORT) 
				&& (flags&NV_ASSIGN) && !(flags&(NV_NOSCOPE|NV_ARRAY)))
			{
				short namlen,vallen;
				namlen =strlen(nv_name(np));
				vallen = strlen(cp);
				ev_$set_var(nv_name(np),&namlen,cp,&vallen);
			}
#endif /* apollo */
		}
		return(np);
	}
failed:
	if(!sh_isstate(SH_INIT))
		if (root==sh.alias_tree)
			error(ERROR_exit(1),gettxt(e_aliname_id,e_aliname),name);
		else
			error(ERROR_exit(1),gettxt(e_ident_id,e_ident),name);
	return(0);
}

#ifdef SHOPT_MULTIBYTE
    static char *savep;
    static char savechars[ESS_MAXCHAR+1];
    static int ja_size(char*, int, int);
    static void ja_restore(void);
#endif /* SHOPT_MULTIBYTE */

/*
 * put value <string> into name-value node <np>.
 * If <np> is an array, then the element given by the
 *   current index is assigned to.
 * If <flags> contains NV_RDONLY, readonly attribute is ignored
 * If <flags> contains NV_INTEGER, string is a pointer to a number
 * If <flags> contains NV_NOFREE, previous value is freed, and <string>
 * becomes value of node and <flags> becomes attributes
 */
void nv_putval(register Namval_t *np, const char *string, int flags)
{
	register const char *sp=string;
	register union Value *up;
	register char *cp;
	register int size = 0;
	register int dot;
	if(!(flags&NV_RDONLY) && nv_isattr (np, NV_RDONLY))
		error(ERROR_exit(1),gettxt(e_readonly_id,e_readonly), nv_name(np));
	/* The following could cause the shell to fork if assignment
	 * would cause a side effect
	 */
	if(sh.subshell)
		np = sh_assignok(np,1);
	if(nv_isattr(np, NV_ARRAY))
		array_check(np,ARRAY_ASSIGN);
	if(np->nvfun && !nv_isattr(np,NV_NODISC))
	{
		/* This function contains disc */
		if(!local)
		{
			local=1;
			nv_putv(np,sp,flags,np->nvfun);
			return;
		}
		/* called from disc, assign the actual value */
		local=0;
	}
	flags &= ~NV_NODISC;
	if(flags&(NV_REF|NV_NOFREE))
	{
		nv_unset(np);
		np->nvalue.cp = (char*)sp;
		nv_setattr(np,flags|NV_NOFREE);
		return;
	}
	if(nv_isattr(np, NV_ARRAY))
		up = array_find(np,ARRAY_ASSIGN);
	else
		up= &np->nvalue;
#ifndef SHOPT_BSH
	nv_offattr(np,NV_IMPORT);
	np->nvenv = 0;
#endif /* SHOPT_BSH */
	if(nv_isattr (np, NV_INTEGER))
	{
		if(nv_isattr (np, NV_CPOINTER))
			up->cp = (char*)sp;
		else if(nv_isattr(np, NV_DOUBLE))
		{
			double d;
			if(flags&NV_INTEGER)
				d = *(double*)sp;
			else
				d = sh_arith(sp);
			if(!up->dp)
				up->dp = new_of(double,0);
			*(up->dp) = d;
		}
		else
		{
			long l = 0;
			if(flags&NV_INTEGER)
				l = *(double*)sp;
			else if(sp)
				l = (long)sh_arith(sp);
			else
				sh.lastbase = 10;
			if(nv_size(np) <= 1)
				nv_setsize(np,sh.lastbase);
			if(nv_isattr (np, NV_SHORT))
				up->s = (short)l;
			else
			{
				if(!up->lp)
					up->lp = new_of(long,0);
				*(up->lp) = l;
				if(l && *sp++ == '0')
					nv_onattr(np,NV_UNSIGN);
			}
		}
	}
	else
	{
		const char *tofree=0;
		if(flags&NV_INTEGER)
			sp = sh_etos(*((double*)sp),12);
#ifdef _WIN32
		if(nv_isattr(np, NV_HOST)==NV_HOST && sp)
		{
			/*
			 * return the host file name given the UNIX name
			 */
			sp = uwin_pathname(sp);
		}
#endif	/* _WIN32 */
		if((nv_isattr(np, NV_RJUST|NV_ZFILL|NV_LJUST)) && sp)
		{
			for(;*sp == ' '|| *sp=='\t';sp++);
	        	if((nv_isattr(np,NV_ZFILL)) && (nv_isattr(np,NV_LJUST)))
				for(;*sp=='0';sp++);
			size = nv_size(np);
#ifdef SHOPT_MULTIBYTE
			if(size)
				size = ja_size((char*)sp,size,nv_isattr(np,NV_RJUST|NV_ZFILL));
#endif /* SHOPT_MULTIBYTE */
		}
		if(!nv_isattr(np, NV_NOFREE|NV_NOALLOC))
		{
			/* delay free in case <sp> points into free region */
			tofree = up->cp;
		}
		if(nv_isattr(np, NV_NOALLOC))
			cp = (char*)up->cp;
		else
		{
			nv_offattr(np,NV_NOFREE);
       		 	if (sp)
			{
				dot = strlen(sp);
				if(size==0 && nv_isattr(np,NV_LJUST|NV_RJUST|NV_ZFILL))
					nv_setsize(np,size=dot);
				else if(size > dot)
					dot = size;
				cp = (char*)malloc(((unsigned)dot+1));
			}
			else
				cp = 0;
			up->cp = cp;
		}
		if(sp)
		{
			if(nv_isattr(np, NV_LTOU))
				ltou(sp,cp);
			else if(nv_isattr (np, NV_UTOL))
				utol(sp,cp);
			else
       			 	strcpy(cp, sp);
			if(nv_isattr(np, NV_RJUST) && nv_isattr(np, NV_ZFILL))
				rightjust(cp,size,'0');
			else if(nv_isattr(np, NV_RJUST))
				rightjust(cp,size,' ');
			else if(nv_isattr(np, NV_LJUST))
			{
				register char *dp;
				dp = strlen (cp) + cp;
				*(cp = (cp + size)) = 0;
				for (; dp < cp; *dp++ = ' ');
			 }
#ifdef SHOPT_MULTIBYTE
			/* restore original string */
			if(savep)
				ja_restore();
#endif /* SHOPT_MULTIBYTE */
		}
		if(tofree)
			free((void*)tofree);
	}
#ifdef apollo
	if((flags&NV_RDONLY) && nv_isattr(np,NV_EXPORT))
	{
		short namlen, vallen;
		char *vp = nv_getval(np);
		namlen =strlen(nv_name(np));
		vallen = strlen(vp);
		ev_$set_var(nv_name(np),&namlen,vp,&vallen);
	}
#endif /* apollo */
	return;
}

/*
 *
 *   Right-justify <str> so that it contains no more than
 *   <size> characters.  If <str> contains fewer than <size>
 *   characters, left-pad with <fill>.  Trailing blanks
 *   in <str> will be ignored.
 *
 *   If the leftmost digit in <str> is not a digit, <fill>
 *   will default to a blank.
 */
static void rightjust(char *str, int size, int fill)
{
	register int n;
	register char *cp,*sp;
	n = strlen(str);

	/* ignore trailing blanks */
	for(cp=str+n;n && *--cp == ' ';n--);
	if (n == size)
		return;
	if(n > size)
        {
        	*(str+n) = 0;
        	for (sp = str, cp = str+n-size; sp <= str+size; *sp++ = *cp++);
        	return;
        }
	else *(sp = str+size) = 0;
	if (n == 0)  
        {
        	while (sp > str)
               		*--sp = ' ';
        	return;
        }
	while(n--)
	{
		sp--;
		*sp = *cp--;
	}
	if(!isdigit(*str))
		fill = ' ';
	while(sp>str)
		*--sp = fill;
	return;
}

#ifdef SHOPT_MULTIBYTE
    /*
     * handle left and right justified fields for multi-byte chars
     * given physical size, return a logical size which reflects the
     * screen width of multi-byte characters
     * Multi-width characters replaced by spaces if they cross the boundary
     * <type> is non-zero for right justified  fields
     */

    static int ja_size(char *str,int size,int type)
    {
	register char *cp = str;
	register int c, n=size;
	int oldn;
	wchar_t w;
	while(*cp)
	{
		oldn = n;
		if((c=mbtowc(&w,cp,MB_CUR_MAX))>0)
		{
			register int outsize =  wcwidth(w);
			/* allow room for excess input bytes */
			n += (c-outsize);
			size -= outsize;
		}
		else
		{
			c = 1;
			size -=1;
		}
		if(size<=0 && type==0)
			break;
		cp += c;
	}
	/* check for right justified fields that need truncating */
	if(size <0)
	{
		if(type==0)
		{
			/* left justified and character crosses field boundary */
			n = oldn;
			/* save boundary char and replace with spaces */
			size = c;
			savechars[size] = 0;
			while(size--)
			{
				savechars[size] = cp[size];
				cp[size] = ' ';
			}
			savep = cp;
		}
		size = -size;
		if(type)
			n -= (ja_size(str,size,0)-size);
	}
	return(n);
    }

    static void ja_restore(void)
    {
	register char *cp = savechars;
	while(*cp)
		*savep++ = *cp++;
	savep = 0;
    }
#endif /* SHOPT_MULTIBYTE */

static char *staknam(register Namval_t *np, char *value)
{
	register char *p,*q;
	q = stakalloc(strlen(nv_name(np))+(value?strlen(value):0)+2);
	p=strcopy(q,nv_name(np));
	if(value)
	{
		*p++ = '=';
		strcpy(p,value);
	}
	return(q);
}

/*
 * put the name and attribute into value of attributes variable
 */
static void attstore(register Namval_t *np)
{
	register int flag = np->nvflag;
	if(!(flag&NV_EXPORT) || (flag&NV_FUNCT))
		return;
	flag &= (NV_RDONLY|NV_UTOL|NV_LTOU|NV_RJUST|NV_LJUST|NV_ZFILL|NV_INTEGER);
	*attval++ = '=';
	*attval++ = ' '+flag;
	if(flag&NV_INTEGER)
		*attval = ' ' + nv_size(np);
	else
		*attval = ' ';
	attval = strcopy(++attval,nv_name(np));
}

static void pushnam(Namval_t *np)
{
	register char *value;
	if(nv_isattr(np,NV_IMPORT))
	{
		if(np->nvenv)
			*argnam++ = np->nvenv;
	}
	else if(value=nv_getval(np))
		*argnam++ = staknam(np,value);
	if(nv_isattr(np,NV_RDONLY|NV_UTOL|NV_LTOU|NV_RJUST|NV_LJUST|NV_ZFILL|NV_INTEGER))
		attsize += (strlen(nv_name(np))+4);
}

/*
 * Generate the environment list for the child.
 */

char **sh_envgen(void)
{
	register char **er;
	register int namec;
	register char *cp;
	/* L_ARGNOD gets generated automatically as full path name of command */
	nv_offattr(L_ARGNOD,NV_EXPORT);
	attsize = 6;
	namec = nv_scan(sh.var_tree,nullscan,NV_EXPORT,NV_EXPORT);
	er = (char**)stakalloc((namec+3)*sizeof(char*));
	argnam = ++er;
	nv_scan(sh.var_tree, pushnam, NV_EXPORT, NV_EXPORT);
	*argnam = (char*)stakalloc(attsize);
	cp = attval = strcopy(*argnam,e_envmarker);
	nv_scan(sh.var_tree, attstore,0,(NV_RDONLY|NV_UTOL|NV_LTOU|NV_RJUST|NV_LJUST|NV_ZFILL|NV_INTEGER));
	*attval = 0;
	if(cp!=attval)
		argnam++;
	*argnam = 0;
	return(er);
}

static void (*scanfn)(Namval_t*);
static int scanmask;
static int scanflags;
static int scancount = 0;

static int scanfilter(const char *name, char *arg, void *notused)
{
	register Namval_t *np = (Namval_t*)arg;
	register int k=np->nvflag;
	NOT_USED(name);
	NOT_USED(notused);
	if(scanmask?(k&scanmask)==scanflags:(!scanflags || (k&scanflags)))
	{
		if(!np->nvalue.cp && !nv_isattr(np,~NV_DEFAULT))
			return(0);
		if(scanfn)
		{
			if(nv_isattr(np,NV_ARRAY))
				nv_putsub(np,NIL(char*),0L);
			(*scanfn)(np);
		}
		scancount++;
	}
	return(0);
}

/*
 * Walk through the name-value pairs
 * if <mask> is non-zero, then only nodes with (nvflags&mask)==flags
 *	are visited
 * If <mask> is zero, and <flags> non-zero, then nodes with one or
 *	more of <flags> is visited
 * If <mask> and <flags> are zero, then all nodes are visted
 */
int nv_scan(Hashtab_t *root, void (*fn)(Namval_t*), int mask, int flags)
{
	int (*hashfn)(const char*, char*, void*);
	scanmask = mask;
	scanflags = flags&~NV_NOSCOPE;
	scanfn = fn;
	hashfn = scanfilter;
	scancount = 0;
	hashwalk(root, (flags&NV_NOSCOPE)?HASH_NOSCOPE:0, hashfn,NIL(char*));
	return(scancount);
}

/*
 * create a new environment scope
 */
void nv_scope(struct argnod *envlist)
{
	register Hashtab_t *newscope;
	newscope = hashalloc(sh.var_tree,HASH_set,HASH_SCOPE|HASH_ALLOCATE,0);
	sh.var_tree = newscope;
	nv_setlist(envlist,NV_EXPORT|NV_NOSCOPE|NV_IDENT|NV_ASSIGN);
}

/* 
 * Remove freeable local space associated with the nvalue field
 * of nnod. This includes any strings representing the value(s) of the
 * node, as well as its dope vector, if it is an array.
 */

void	sh_envnolocal (register Namval_t *np)
{
	if(nv_isattr(np,NV_EXPORT|NV_NOFREE))
	{
		if(nv_isattr(np,NV_REF))
		{
			nv_offattr(np,NV_NOFREE|NV_REF);
			np->nvalue.cp = 0;
		}
		return;
	}
	if(nv_isattr(np, NV_ARRAY))
		nv_putsub(np,NIL(char*),ARRAY_SCAN);
	forced = 1;
	nv_unset(np);
	forced = 0;
	nv_setattr(np,0);
}

/*
 * Currently this is a dummy, but someday will be needed
 * for reference counting
 */
void	nv_close(Namval_t *np)
{
	NOT_USED(np);
}

/*
 *
 *   Set the value of <np> to 0, and nullify any attributes
 *   that <np> may have had.  Free any freeable space occupied
 *   by the value of <np>.  If <np> denotes an array member, it
 *   will retain its attributes.
 */
void	nv_unset(register Namval_t *np)
{
	register union Value *up = &np->nvalue;
	register Namarr_t *ap;
	if(!forced && nv_isattr (np,NV_RDONLY))
		error(ERROR_exit(1),gettxt(e_readonly_id,e_readonly), nv_name(np));
	if(sh.subshell && !nv_isnull(np))
		np = sh_assignok(np,0);
	if(ap = nv_arrayptr(np))
		array_check(np,ARRAY_DELETE);
	nv_offattr(np,NV_NODISC);
	if(np->nvfun)
	{
		/* This function contains disc */
		if(!local)
		{
			local=1;
			nv_putv(np,NIL(char*),forced?NV_RDONLY:0,np->nvfun);
			return;
		}
		/* called from disc, assign the actual value */
		local=0;
	}
	forced = 0;
	do
	{
		if(ap)
		{
			if(!(up=array_find(np,ARRAY_DELETE)))
				return;
		}
		if((!nv_isattr (np, NV_NOFREE)) && up->cp)
			free((void*)up->cp);
		up->cp = 0;
	}
	while(ap?nv_nextsub(np):0);
	if(!nv_isattr(np,NV_ARRAY))
	{
		nv_setattr(np,0);
		nv_setsize(np,0);
		if(nv_isattr(np,NV_REF) && np->nvenv)
			free((void*)np->nvenv);
		np->nvenv = 0;
	}
}

/*
 * return the node pointer in the highest level scope
 */
Namval_t *nv_scoped(register Namval_t *np)
{
	if(hashscope(sh.var_tree))
		return(nv_search((char*)np,sh.var_tree,HASH_BUCKET));
	else
		return(np);
}

/*
 *   Return a pointer to a character string that denotes the value
 *   of <np>.  If <np> refers to an array,  return a pointer to
 *   the value associated with the current index.
 *
 *   If the value of <np> is an integer, the string returned will
 *   be overwritten by the next call to nv_getval.
 *
 *   If <np> has no value, 0 is returned.
 */

char *nv_getval(register Namval_t *np)
{
	register union Value *up= &np->nvalue;
	register int numeric;
	register Namarr_t *ap;
	if(!np->nvfun && !nv_isattr(np,NV_ARRAY|NV_INTEGER|NV_FUNCT|NV_REF))
		goto done;
	if(ap = nv_arrayptr(np))
		array_check(np,ARRAY_LOOKUP);
	if(np->nvfun)
	{
		if(!local)
		{
			local=1;
			return(nv_getv(np, np->nvfun));
		}
		local=0;
	}
	numeric = ((nv_isattr (np, NV_INTEGER)) != 0);
	if(ap)
	{
		if(!(up = array_find(np,ARRAY_LOOKUP)))
	        	return (NIL(char*));
	}
	if(numeric)
	{
		register long l;
        	if(!up->cp || nv_isattr (np,NV_CPOINTER))
			return((char*)up->cp);
		else if(nv_isattr (np,NV_DOUBLE))
		{
			double d = *up->dp;
			if(nv_isattr (np,NV_EXPNOTE))
				return(sh_etos(d,nv_size(np)));
			else
				return(sh_ftos(d,nv_size(np)));
		}
        	else if(nv_isattr (np,NV_SHORT))
			l = up->s;
        	else
			l = *(up->lp);
		if((numeric=nv_size(np))==10 && !nv_isattr(np,NV_UNSIGN))
			numeric = 0;
		return(fmtbase(l,numeric, numeric&&numeric!=10));
	}
	if(nv_isattr(np,NV_REF))
		return(nv_name(up->np));
done:
#ifdef SHOPT_OO
	/* This is not nearly complete */
	if(!up->cp && (np=nv_class(np)))
		return(nv_getval(np));
#endif /* SHOPT_OO */
	return ((char*)up->cp);
}

double nv_getnum(register Namval_t *np)
{
	register union Value *up;
	register double r=0;
	register char *str;
     	if(np->nvfun && !nv_isattr(np,NV_NODISC))
	{
		if(!local)
		{
			local=1;
			return(nv_getn(np, np->nvfun));
		}
		local=0;
	}
     	if(nv_isattr (np, NV_INTEGER))
	{
		if(nv_isattr(np, NV_ARRAY))
			up = array_find(np,ARRAY_ASSIGN);
		else
			up= &np->nvalue;
		if(!up->lp)
			r = 0;
		else if(nv_isattr(np, NV_DOUBLE))
                       	r = *up->dp;
		else
                       	r = *up->lp;
	}
	else if((str=nv_getval(np)) && *str!=0)
		r = sh_arith(str);
	return(r);
}
/*
 *   Give <np> the attributes <newatts,> and change its current
 *   value to conform to <newatts>.  The <size> of left and right
 *   justified fields may be given.
 */
void nv_newattr (register Namval_t *np, unsigned newatts, int size)
{
	register char *sp;
	register char *cp = 0;
	register unsigned int n;
	Namarr_t *ap = 0;
	int oldsize,oldatts;

	/* check for restrictions */
	if(sh_isoption(SH_RESTRICTED) && ((sp=nv_name(np))==nv_name(PATHNOD) || sp==nv_name(SHELLNOD) || sp==nv_name(ENVNOD) ))
		error(ERROR_exit(1),gettxt(e_restricted_id,e_restricted),nv_name(np));
	/* handle attributes that do not change data separately */
	n = np->nvflag;
#ifdef SHOPT_BSH
	if(newatts&NV_EXPORT)
		nv_offattr(np,NV_IMPORT);
#endif /* SHOPT_BSH */
#ifdef apollo
	if(((n^newatts)&NV_EXPORT))
	/* record changes to the environment */
	{
		short namlen = strlen(nv_name(np));
		if(n&NV_EXPORT)
			ev_$delete_var(nv_name(np),&namlen);
		else
		{
			char *vp = nv_getval(np);
			short vallen = strlen(vp);
			ev_$set_var(nv_name(np),&namlen,vp,&vallen);
		}
	}
#endif /* apollo */
	if((size==0||(n&NV_INTEGER)) && ((n^newatts)&~NV_NOCHANGE)==0)
	{
		if(size)
			nv_setsize(np,size);
		nv_offattr(np, ~NV_NOFREE);
		nv_onattr(np, newatts);
		return;
	}
	/* for an array, change all the elements */
	if((ap=nv_arrayptr(np)) && ap->nelem>0)
		nv_putsub(np,NIL(char*),ARRAY_SCAN);
	oldsize = nv_size(np);
	oldatts = np->nvflag;
	do
	{
		nv_setsize(np,oldsize);
		np->nvflag = oldatts;
		if (sp = nv_getval(np))
 		{
			cp = (char*)malloc((n=strlen (sp)) + 1);
			strcpy(cp, sp);
			if(ap)	/* add element to prevent array deletion */
			{
				ap->nelem &= ~ARRAY_SCAN;
				ap->nelem++;
			}
			nv_unset(np);
			if(ap)
			{
				ap->nelem--;
				ap->nelem |= ARRAY_SCAN;
			}
			if(size==0 && (newatts&(NV_LJUST|NV_RJUST|NV_ZFILL)))
				size = n;
		}
		else
			nv_unset(np);
		nv_setsize(np,size);
		np->nvflag &= NV_ARRAY;
		np->nvflag |= newatts;
		if (cp)
		{
			nv_putval (np, cp, NV_RDONLY);
			free(cp);
		}
	}
	while(ap && nv_nextsub(np));
	return;
}

#ifndef _NEXT_SOURCE
static char *oldgetenv(const char *string)
{
	register char c0,c1;
	register const char *cp, *sp;
	register char **av = environ;
	if(!string || (c0= *string)==0)
		return(0);
	if((c1=*++string)==0)
		c1= '=';
	while(cp = *av++)
	{
		if(cp[0]!=c0 || cp[1]!=c1) 
			continue;
		sp = string;
		while(*sp && *sp++ == *++cp);
		if(*sp==0 && *++cp=='=')
			return((char*)(cp+1));
	}
	return(0);
}

/*
 * This version of getenv the hash storage to access environment values
 */
char *getenv(const char *name)
/*@
	assume name!=0;
@*/ 
{
	register Namval_t *np;
	if(!sh.var_tree)
		return(oldgetenv(name));
	if((np = nv_search(name,sh.var_tree,0)) && nv_isattr(np,NV_EXPORT))
		return(nv_getval(np));
	return(0);
}
#endif /* _NEXT_SOURCE */

#undef putenv
/*
 * This version of putenv uses the hash storage to assign environment values
 */
int putenv(const char *name)
{
	register Namval_t *np;
	if(name)
	{
		np = nv_open(name,sh.var_tree,NV_EXPORT|NV_IDENT|NV_ARRAY|NV_ASSIGN);
		if(!strchr(name,'='))
			nv_unset(np);
	}
	return(0);
}


/*
 * Override libast setenv()
 */
char* setenviron(const char *name)
{
	register Namval_t *np;
	if(name)
	{
		np = nv_open(name,sh.var_tree,NV_EXPORT|NV_IDENT|NV_ARRAY|NV_ASSIGN);
		if(strchr(name,'='))
			return(nv_getval(np));
		nv_unset(np);
	}
	return("");
}

/*
 * copy <str1> to <str2> changing lower case to upper case
 * <str2> must be big enough to hold <str1>
 * <str1> and <str2> may point to the same place.
 */

static void ltou(register char const *str1,register char *str2)
/*@
	assume str1!=0 && str2!=0;
	return x satisfying strlen(in str1)==strlen(in str2);
@*/ 
{
	register int c;
	for(; c= *((unsigned char*)str1); str1++,str2++)
	{
		if(islower(c))
			*str2 = toupper(c);
		else
			*str2 = c;
	}
	*str2 = 0;
}

/*
 * copy <str1> to <str2> changing upper case to lower case
 * <str2> must be big enough to hold <str1>
 * <str1> and <str2> may point to the same place.
 */

static void utol(register char const *str1,register char *str2)
/*@
	assume str1!=0 && str2!=0
	return x satisfying strlen(in str1)==strlen(in str2);
@*/ 
{
	register int c;
	for(; c= *((unsigned char*)str1); str1++,str2++)
	{
		if(isupper(c))
			*str2 = tolower(c);
		else
			*str2 = c;
	}
	*str2 = 0;
}

/*
 * Push or pop getval and putval functions to node <np>
 * <fp> is null to pop, otherwise, <fp> is pushed onto stack
 * The top of the stack is returned
 */
Namfun_t *nv_stack(register Namval_t *np, register Namfun_t* fp)
{
	register Namfun_t *lp;
	if(fp)
	{
		if((lp=np->nvfun)==fp)
			return(fp);
		/* see if <fp> is on the list already and move to top */
		if(lp) while(lp->next)
		{
			if(lp->next==fp)
			{
				lp->next = fp->next;
				break;
			}
			lp = lp->next;
		}
		/* push */
		fp->next = np->nvfun;
		np->nvfun = fp;
	}
	else if(fp = np->nvfun)
		np->nvfun = fp->next;
	return(fp);
}

/*
 * add or replace built-in version of command commresponding to <path>
 * The <bltin> argument is a pointer to the built-in
 * Special builtins cannot be replaced and return -1
 */
int	sh_addbuiltin(const char *path, int (*bltin)(int, char*[],void*),void *extra)
{
	register const char *cp, *name = path_basename(path);
	register Namval_t *np, *nq=0;
	if(name==path && (cp=strchr(name,'.')) && cp!=name)
	{
		int offset = staktell();
		stakwrite(name,cp-name);
		stakputc(0);
		nq=nv_open(stakptr(offset),0,NV_VARNAME|NV_NOARRAY);
		offset = staktell();
		stakputs(nv_name(nq));
		stakputs(cp);
		stakputc(0);
		path = name = stakptr(offset);
	}
	np = nv_search(name,sh.bltin_tree,NV_ADD);
	if(!np || nv_isattr(np,BLT_SPC))
		return(-1);
	if(np->nvenv && !nv_isattr(np,NV_NOFREE))
		free((void*)np->nvenv);
	np->nvenv = 0;
	np->nvfun = 0;
	nv_setattr(np,0);
	if(bltin)
	{
		np->nvalue.bfp = bltin;
		nv_onattr(np,NV_BLTIN);
		if(path!=name)
			np->nvenv = strdup(path);
		np->nvfun = (Namfun_t*)extra;
	}
	if(nq)
	{
		cp=nv_setdisc(nq,cp+1,np,(Namfun_t*)nq);
		nv_close(nq);
		if(!cp)
			error(ERROR_exit(1),e_baddisc,name);
	}
	return(0);
}

static	int maxbufsize;
static  char *curbuf;

/*
 *   Return a pointer to a string denoting the value of <val>
 *   with <places> places after the decimal point.  The string
 *   will be stored within static variable <numbuf>.
 */
char *sh_ftos(double val,register int places)
{
	register char *cp, *sp;
	int decpt,sign;

	cp = sffcvt(val,places,&decpt,&sign);
	if(decpt>0)
		places += decpt;
	places +=3;	/* room for sign, decimal point and null byte */
	if(!curbuf)
		curbuf = (char*)malloc(maxbufsize=places);
	else if(places > maxbufsize)
		curbuf = (char*)realloc(curbuf,maxbufsize=places);
	sp = curbuf;
	if(sign)
		*sp++ = '-';
	while(decpt-- > 0)
		*sp++ = *cp++;
	*sp++ = GETDECIMAL(0);
	while(++decpt < 0)
		*sp++  = '0';
	while(*sp++ = *cp++);
	return(curbuf);
}

/*
 * convert <val> to a string with <places> significant figures
 * The result is placed in a local buffer and a pointer returned
 */
char *sh_etos(double val,register int places)
{
	register int bufsize = places+8;
	if(!curbuf)
		curbuf = (char*)malloc(maxbufsize=bufsize);
	else if(bufsize > maxbufsize)
		curbuf = (char*)realloc(curbuf,maxbufsize=bufsize);
	sfsprintf(curbuf,bufsize,"%.*g",places,val);
	return(curbuf);
}

/*
 * call the next getval function in the chain
 */
char *nv_getv(Namval_t *np, register Namfun_t *nfp)
{
	register Namfun_t	*fp;
	register char *cp;
	if((fp = nfp) != NIL(Namfun_t*) && !local)
		fp = nfp = nfp->next;
	local=0;
	while(fp && !fp->disc->getnum && !fp->disc->getval)
		fp = fp->next;
	if(fp)
		nfp = fp;
	if(nfp && nfp->disc->getval)
		cp = (*nfp->disc->getval)(np,nfp);
	else if(nfp && nfp->disc->getnum)
		cp = sh_etos((*nfp->disc->getnum)(np,nfp),12);
	else
	{
		local=1;
		cp = nv_getval(np);
	}
	return(cp);
}

/*
 * call the next getnum function in the chain
 */
double nv_getn(Namval_t *np, register Namfun_t *nfp)
{
	register Namfun_t	*fp;
	register double d=0;
	if((fp = nfp) != NIL(Namfun_t*) && !local)
		fp = nfp = nfp->next;
	local=0;
	while(fp && !fp->disc->getnum && !fp->disc->getval)
		fp = fp->next;
	if(fp)
		nfp = fp;
	if(nfp && nfp->disc->getnum)
		d = (*nfp->disc->getnum)(np,nfp);
	else if(nfp && nfp->disc->getval)
	{
		char *str = (*nfp->disc->getval)(np,nfp);
		if(str && *str)
			d = sh_arith(str);
	}
	else
	{
		local=1;
		d = nv_getnum(np);
	}
	return(d);
}

/*
 * call the next assign function in the chain
 */
void nv_putv(Namval_t *np, const char *value, int flags, register Namfun_t *nfp)
{
	register Namfun_t	*fp;
	if((fp=nfp) != NIL(Namfun_t*) && !local)
		fp = nfp = nfp->next;
	local=0;
	while(fp && !fp->disc->putval)
		fp = fp->next;
	if(fp)
		nfp = fp;
	if(nfp && nfp->disc->putval)
		(*nfp->disc->putval)(np,value, flags, nfp);
	else
	{
		local=1;
		if(value)
			nv_putval(np, value, flags);
		else
			nv_unset(np);
	}
}

#define	LOOKUP		0
#define	ASSIGN		1
#define	UNASSIGN	2
#define BLOCKED		((Namval_t*)&local)

struct	vardisc
{
	Namfun_t	fun;
	Namval_t	*disc[3];
};

/*
 * free discipline if no more discipline functions
 */
static void chktfree(register Namval_t *np, register struct vardisc *vp)
{
	register int n;
	for(n=0; n< sizeof(vp->disc)/sizeof(*vp->disc); n++)
	{
		if(vp->disc[n])
			break;
	}
	if(n>=sizeof(vp->disc)/sizeof(*vp->disc))
	{
		/* no disc left so pop */
		Namfun_t *fp;
		if(fp=nv_stack(np, NIL(Namfun_t*)))
			free((void*)fp);
	}
}

/*
 * This function performs an assignment disc on the given node <np>
 */
static void	assign(Namval_t *np,const char* val,int flags,Namfun_t *handle)
{
	register struct vardisc *vp = (struct vardisc*)handle;
	register Namval_t *nq, **disc;
	if(val)
	{
		if(!(nq=vp->disc[ASSIGN]))
		{
			nv_putv(np,val,flags,handle);
			return;
		}
		nv_putval(SH_VALNOD, val, (flags&NV_INTEGER)?NV_NOFREE|NV_INTEGER|NV_DOUBLE|NV_EXPNOTE:NV_NOFREE);
	}
	disc = (val? &(vp->disc[ASSIGN]):&(vp->disc[UNASSIGN]));
	if((nq= *disc) && nq!=BLOCKED)
	{
		*disc=BLOCKED;
		sh_fun(nq,np);
		if(*disc==BLOCKED)
			*disc=nq;
		else if(!*disc)
			chktfree(np,vp);
	}
	if(val)
	{
		register char *cp;
		double d;
		if(nv_isnull(SH_VALNOD))
			cp=0;
		else if(flags&NV_INTEGER)
		{
			d = nv_getnum(SH_VALNOD);
			cp = (char*)(&d);
		}
		else
			cp = nv_getval(SH_VALNOD);
		if(cp)
			nv_putv(np,cp,flags|NV_RDONLY,handle);
		nv_unset(SH_VALNOD);
	}
	else if(!nq || nq==BLOCKED)
	{
		if(vp->disc[ASSIGN])
			nv_unset(vp->disc[ASSIGN]);
		if(vp->disc[LOOKUP])
			nv_unset(vp->disc[LOOKUP]);
		if(vp->disc[UNASSIGN])
			nv_unset(vp->disc[UNASSIGN]);
		if(handle=nv_stack(np, NIL(Namfun_t*)))
			free((void*)handle);
		nv_unset(np);
	}
}

/*
 * This function executes a lookup disc and then performs
 * the lookup on the given node <np>
 */
static char*	lookup(Namval_t *np, Namfun_t *handle)
{
	register struct vardisc *vp = (struct vardisc*)handle;
	register Namval_t *nq;
	register char *cp=0;
	if((nq=vp->disc[LOOKUP]) &&  nq!=BLOCKED)
	{
		nv_unset(SH_VALNOD);
		vp->disc[LOOKUP]=BLOCKED;
		sh_fun(nq,np);
		if(vp->disc[LOOKUP]==BLOCKED)
			vp->disc[LOOKUP]=nq;
		else if(!vp->disc[LOOKUP])
			chktfree(np,vp);
		cp = nv_getval(SH_VALNOD);
	}
	if(!cp)
		cp = nv_getv(np,handle);
	return(cp);
}

/*
 * node creation discipline
 */
Namval_t *nv_create(register Namval_t* np,const char *name,register Namfun_t *fp)
{
	if(np == (Namval_t*)fp)
		fp = np->nvfun;
	else if(fp)
		fp = fp->next;
	while(fp && !fp->disc->create)
		fp = fp->next;
	if(fp && fp->disc->create)
		return((*fp->disc->create)(np,name,fp));
	return(NIL(Namval_t*));
}

static const Namdisc_t shdisc =
{
	sizeof(struct vardisc),
	assign,
	lookup
};


/*
 * Set disc on given <event> to <action>
 * If action==np, the current disc is returned
 * A null return value indicates that no <event> is known for <np>
 * If <event> is NULL, then return the event name after <action>
 * If <event> is NULL, and <action> is NULL, return the first event
 */
char *nv_setdisc(register Namval_t* np,register const char *event,Namval_t *action,register Namfun_t *fp)
{
	register struct vardisc *vp = (struct vardisc*)np->nvfun;
	register int type;
	if(np == (Namval_t*)fp)
	{
		static const char *discnames[] = { "get", "set", "unset", 0 };
		register const char *name;
		register int getname=0;
		/* top level call, check for get/set */
		if(!event)
		{
			if(!action)
				return((char*)discnames[0]);
			getname=1;
			event = (char*)action;
		}
		for(type=0; name=discnames[type]; type++)
		{
			if(strcmp(event,name)==0)
				break;
		}
		if(getname)
		{
			event = 0;
			if(name && !(name = discnames[++type]))
				action = 0;
		}
		if(!name)
		{
			if((fp=(Namfun_t*)vp) && fp->disc->setdisc)
				return((*fp->disc->setdisc)(np,event,action,fp));
		}
		else if(getname)
			return((char*)name);
	}
	if(!fp)
		return(NIL(char*));
	if(np != (Namval_t*)fp)
	{
		/* not the top level */
		while(fp = fp->next)
		{
			if(fp->disc->setdisc)
				return((*fp->disc->setdisc)(np,event,action,fp));
		}
		return(NIL(char*));
	}
	/* Handle GET/SET/UNSET disc */
	if(vp && vp->fun.disc->putval!=assign)
		vp = 0;
	if(!vp)
	{
		if(action==np)
			return((char*)action);
		if(!(vp = newof(NIL(struct vardisc*),struct vardisc,1,0)))
			return(0);
		vp->fun.disc = &shdisc;
		nv_stack(np, (Namfun_t*)vp);
	}
	if(action==np)
		action = vp->disc[type];
	else if(action)
		vp->disc[type] = action;
	else
	{
		action = vp->disc[type];
		vp->disc[type] = 0;
		if(action!=BLOCKED)
			chktfree(np,vp);
	}
	return(action?(char*)action:"");
}

/*
 * Create a reference node from <np>
 */
void nv_setref(register Namval_t *np)
{
	register Namval_t *nq, *nr;
	register char *cp;
	register int flags = NV_ARRAY|NV_VARNAME|NV_REF;
	Hashtab_t *hp=sh.var_tree;
	if(nv_isattr(np,NV_REF))
		return;
	if(nv_isattr(np,NV_ARRAY))
		error(ERROR_exit(1),gettxt(e_badref_id,e_badref),nv_name(np));
	if(!(cp=nv_getval(np)))
		error(ERROR_exit(1),gettxt(e_noref_id,e_noref),nv_name(np));
	if(np->nvsize&NV_PARAM)
	{
		if(!(hp=(Hashtab_t*)sh.st.prevst->save_tree))
		{
			if(!(hp=hashscope(sh.var_tree)))
				hp = sh.var_tree;
		}
	}
	nr= nq = nv_open(cp, hp, NV_ARRAY|NV_VARNAME|NV_REF);
	while(nv_isattr(nr,NV_REF))
		nr = nr->nvalue.np;
	if(nr==np) 
	{
		if(!(hp=hashscope(hp)))
			error(ERROR_exit(1),gettxt(e_selfref_id,e_selfref),nv_name(np));
		/* bind to earlier scope, or add to global scope */
		nq=nv_search((char*)np,hp,NV_ADD|HASH_BUCKET);
	}
#ifdef SHOPT_COMPOUND_ARRAY
	if(cp=strchr(cp,'['))
	{
		/* cause subscript evaluation and return result */
		nv_endsubscript(nq,cp,NV_ADD);
		cp = nv_getsub(nq);
	}
#endif /* SHOPT_COMPOUND_ARRAY */
	nv_unset(np);
	np->nvalue.np = nq;
#ifdef SHOPT_COMPOUND_ARRAY
	if(cp)
		np->nvenv = strdup(cp);
#endif /* SHOPT_COMPOUND_ARRAY */
	nv_onattr(np,NV_REF|NV_NOFREE);
}

/*
 * get the scope corresponding to <index>
 * whence uses the same values as lseeek()
 */
Shscope_t *sh_getscope(int index, int whence)
{
	register struct sh_scoped *sp, *topmost;
	if(whence==SEEK_CUR)
		sp = &sh.st;
	else
	{
		if ((struct sh_scoped*)sh.topscope != sh.st.self)
			topmost = (struct sh_scoped*)sh.topscope;
		else
			topmost = &(sh.st);
		sp = topmost;
		if(whence==SEEK_SET)
		{
			int n =0;
			while(sp = sp->prevst)
				n++;
			index = n - index;
			sp = topmost;
		}
	}
	if(index < 0)
		return((Shscope_t*)0);
	while(index-- && (sp = sp->prevst));
	return((Shscope_t*)sp);
}

/*
 * make <scoped> the top scope and return previous scope
 */
Shscope_t *sh_setscope(Shscope_t *scope)
{
	Shscope_t *old = (Shscope_t*)sh.st.self;
	*sh.st.self = sh.st;
	sh.st = *((struct sh_scoped*)scope);
	sh.var_tree = scope->var_tree;
	return(old);
}
