/*		copyright	"%c%" 	*/

#ident	"@(#)ksh:sh/builtin.c	1.5.7.15"

/*
 *  builtin routines for the shell
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 3C-526B
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 */

#include	<errno.h>
#include	"defs.h"
#include	"history.h"
#include	"builtins.h"
#include	"jobs.h"
#include	"sym.h"

#define NOT_USED(x)	(&x,1)	/* prevents not used messages */

#ifdef _sys_resource_
#   ifndef included_sys_time_
#	include <sys/time.h>
#   endif
#   include	<sys/resource.h>/* needed for ulimit */
#   define	LIM_FSIZE	RLIMIT_FSIZE
#   define	LIM_DATA	RLIMIT_DATA
#   define	LIM_STACK	RLIMIT_STACK
#   define	LIM_CORE	RLIMIT_CORE
#   define	LIM_CPU		RLIMIT_CPU
#   define	INFINITY	RLIM_INFINITY
#   ifdef RLIMIT_RSS
#	define	LIM_MAXRSS	RLIMIT_RSS
#   endif /* RLIMIT_RSS */
#else
#   ifdef VLIMIT
#	include	<sys/vlimit.h>
#   endif /* VLIMIT */
#endif	/* _sys_resource_ */
#ifdef UNIVERSE
    static int att_univ = -1;
#   ifdef _sys_universe_
#	include	<sys/universe.h>
#	ifdef sequent
	    define NUMUNIV 2
	    static char *univ_name[] = { "ucb", "att", 0};
#	    define getuniverse(x) (flag=universe(U_GET),\
			(flag>-1?strncpy(x,univ_name[flag],TMPSIZ):0),\
			flag)
#	    define univ_index(n)	(n)
#	    define setuniverse(x) universe(x)

#	else /* !sequent */
#	    define getuniverse(x)	(setuniverse(flag=setuniverse(0)),\
			(--flag>=0?strncpy(x,univ_name[flag],TMPSIZ):0),\
			flag)
#	    define univ_index(n)	((n)+1)
#	endif /* sequent */
#   else
#	define univ_number(x)	(x)
#   endif /* _sys_universe_ */
#endif /* UNIVERSE */

#ifdef ECHOPRINT
#   undef ECHO_RAW
#   undef ECHO_N
#endif /* ECHOPRINT */

#define DOTMAX	32	/* maximum level of . nesting */
#define PHYS_MODE	H_FLAG
#define LOG_MODE	N_LJUST

/* This module references these external routines */
#ifdef ECHO_RAW
    extern char		*echo_mode();
#endif	/* ECHO_RAW */
extern int		gscan_all();
extern char		*utos();
extern void		ltou();

static int	b_common();
static int	b_unall();
static int	flagset();
static int	sig_number();
static int	scanargs();
#ifdef JOBS
    static void	sig_list();
#endif	/* JOBS */
static int	argnum;
static int 	aflag;
static int	newflag;
static int	echon;
static int	scoped;

int    b_exec(argn,com)
char **com;
{
        st.ioset = 0;
        if(*++com)
                b_login(argn,com);
	return(0);
}

int    b_login(argn,com)
char **com;
{
	NOT_USED(argn);
	if(is_option(RSHFLG))
		sh_cfail(gettxt(E_RESTRICTED));
	else
        {
#ifdef JOBS
		if(job_close() < 0)
			return(1);
#endif /* JOBS */
		/* force bad exec to terminate shell */
		st.states &= ~(PROFILE|PROMPT|BUILTIN|LASTPIPE);
		sig_reset(0);
		hist_close();
		sh_freeup();
		if(st.states&RM_TMP)
		/* clean up all temp files */
			rm_files(io_tmpname);
		path_exec(com,(struct argnod*)0);
		sh_done(0);
        }
	return(1);
}

int    b_pwd(argn,com)
char **com;
{
	register int flag=0;
	register char *a1 = com[1];
	NOT_USED(argn);
#if defined(LSTAT) || defined(FS_3D)
	while(a1 && *a1=='-'&& 
		(flag = flagset(a1,~(PHYS_MODE|LOG_MODE))))
	{
		if(flag&LOG_MODE)
			flag = 0;
		com++;
		a1 = com[1];
	}
#endif /* LSTAT||FS_3D */
	if(*(a1 = path_pwd(0))!='/')
		sh_cfail(gettxt(E_PWD));
#if defined(LSTAT) || defined(FS_3D)
	if(flag)
	{
		char *path = a1;
		/* reserve PATH_MAX bytes on stack */
		int offset = staktell();
		stakseek(offset+PATH_MAX);
		a1 = stakseek(offset)+offset;
#   ifdef FS_3D
		if(umask(flag=umask(0)),flag&01000)
			mount(".", a1, 10|(PATH_MAX)<<4);
		else
#   endif /* FS_3D */
		a1 = strncpy(stakseek(offset),path,PATH_MAX);
#ifdef LSTAT
		path_physical(a1);
		stakseek(offset);
#endif /* LSTAT */
	}
#endif /* LSTAT||FS_3D */
	p_setout(st.standout);
	p_str(a1,NL);
	return(0);
}

#ifndef ECHOPRINT
int    b_echo(argn,com)
int argn;
char **com;
{
	register int r;
	char *save = *com;
#   ifdef ECHO_RAW
	/* This mess is because /bin/echo on BSD is archaic */
#     ifdef UNIVERSE
	if(att_univ < 0)
	{
		int flag;
		char Xuniverse[TMPSIZ];
       		if(getuniverse(Xuniverse) >= 0)
			att_univ = (strcmp(Xuniverse,"att")==0);
	}
	if(att_univ>0)
		*com = (char*)e_minus;
	else
#     endif /* UNIVERSE */
	*com = echo_mode();
	r = b_print(argn+1,com-1);
	*com = save;
	return(r);
#   else
#	ifdef ECHO_N
	    /* same as echo except -n special */
	    echon = 1;
	    return(b_print(argn,com));
#	else
	    /* equivalent to print - */
	    *com = (char*)e_minus;
	    r = b_print(argn+1,com-1);
	    *com = save;
	    return(r);
#	endif /* ECHO_N */
#   endif	/* ECHO_RAW */
}
#endif /* ECHOPRINT */

int    b_print(argn,com)
int argn;
char **com;
{
	register int fd;
	register char *a1 = com[1];
	register int flag = 0;
	register int r=0;
	const char *msg = gettxt(E_FILE);
	int raw = 0;
	NOT_USED(argn);
	argnum =  1;
	while(a1 && *a1 == '-')
	{
		int c = *(a1+1);
		com++;
		/* echon set when only -n is legal */
		if(echon)
		{
			if(strcmp(a1,"-n")==0)
				c = 0;
			else
			{
				com--;
				break;
			}
		}
		newflag = flagset(a1,~(N_FLAG|R_FLAG|P_FLAG|U_FLAG|S_FLAG|N_RJUST));
		flag |= newflag;
		/* handle the -R flag for BSD style echo */
		if(flag&N_RJUST)
			echon = 1;
		if(c==0 || newflag==0)
			break;
		a1 = com[1];
	}
	echon = 0;
	argnum %= 10;
	if(flag&(R_FLAG|N_RJUST))
		raw = 1;
	if(flag&S_FLAG)
	{
		/* print to history file */
		if(!hist_open())
			sh_cfail(gettxt(E_HISTORY));
		fd = hist_ptr->fixfd;
		st.states |= FIXFLG;
		goto skip;
	}
	else if(flag&P_FLAG)
	{
		fd = COTPIPE;
		msg = gettxt(E_QUERY);
	}
	else if(flag&U_FLAG)
		fd = argnum;
	else	
		fd = st.standout;
	if(r = !fiswrite(fd))
	{
		if(fd==st.standout)
			return(r);
		sh_cfail(msg);
	}
	if(fd==input)
		sh_cfail(msg);
skip:
	p_setout(fd);
	if(echo_list(raw,com+1) && (flag&N_FLAG)==0)
		newline();
	if(flag&S_FLAG)
		hist_flush();
	return(r);
}

int    b_let(argn,com)
register int argn;
char **com;
{
	register int r;
	if(argn < 2)
		sh_cfail(gettxt(E_NARGS));
	while(--argn)
		r = !sh_arith(*++com);
	return(r);
}

/*
 * The following few builtins are provided to set,print,
 * and test attributes and variables for shell variables,
 * aliases, and functions.
 * In addition, typeset -f can be used to test whether a
 * function has been defined or to list all defined functions
 * Note readonly is same as typeset -r.
 * Note export is same as typeset -x.
 */

int    b_readonly(argn,com)
char **com;
{
	NOT_USED(argn);
	aflag = '-';
	argnum = scoped = 0;
	return(b_common(com,R_FLAG,sh.var_tree));
}

int    b_export(argn,com)
char **com;
{
	NOT_USED(argn);
	aflag = '-';
	argnum = scoped = 0;
	return(b_common(com,X_FLAG,sh.var_tree));
}


int    b_alias(argn,com)
register char **com;
{
	register int flag = 0;
	register struct Amemory *troot;
	NOT_USED(argn);
	argnum = scoped = 0;
	if(com[1])
	{
		if( (aflag= *com[1]) == '-' || aflag == '+')
		{
			com += scanargs(com,~(T_FLAG|N_EXPORT));
			flag = newflag;
		}
	}
	else
		aflag = 0;
	if(flag&T_FLAG)
		troot = sh.track_tree;
	else
		troot = sh.alias_tree;
	return(b_common(com,flag,troot));
}


int    b_typeset(argn,com)
register char **com;
{
	register int flag = 0;
	struct Amemory *troot;
	NOT_USED(argn);
	argnum = scoped = 0;
	if(com[1])
	{
		if( (aflag= *com[1]) == '-' || aflag == '+')
		{
			com += scanargs(com,~(N_LJUST|N_RJUST|N_ZFILL
				|N_INTGER|N_LTOU |N_UTOL|X_FLAG|R_FLAG
				|F_FLAG|T_FLAG|N_HOST
				|N_DOUBLE|N_EXPNOTE));
			flag = newflag;
		}
	}
	else
		aflag = 0;
	/* G_FLAG forces name to be in newest scope */
	if(st.fn_depth)
		scoped = G_FLAG;
	if((flag&N_INTGER) && (flag&(N_LJUST|N_RJUST|N_ZFILL|F_FLAG)))
		sh_cfail(gettxt(E_OPTION));
	else if(flag&F_FLAG)
	{
		if(flag&~(N_EXPORT|F_FLAG|T_FLAG|U_FLAG))
			sh_cfail(gettxt(E_OPTION));
		troot = sh.fun_tree;
		flag &= ~F_FLAG;
	}
	else
		troot = sh.var_tree;
	return(b_common(com,flag,troot));
}

static int     b_common(com,flag,troot)
register int flag;
char **com;
struct Amemory *troot;
{
	register int fd;
	register char *a1;
	register int type = 0;
	int r = 0;
	fd = st.standout;
	p_setout(fd);
	if(troot==sh.alias_tree)
		/* env_namset treats this value specially */
		type = (V_FLAG|G_FLAG);
	if(aflag == 0)
	{
		if(type)
			env_scan(fd,0,troot,0);
		else
			gscan_all(env_prattr,troot);
		return(0);
	}
	if(com[1])
	{
		while(a1 = *++com)
		{
			register unsigned newflag;
			register struct namnod *np;
			unsigned curflag;
			if(st.subflag && (flag || strchr(a1,'=')))
				continue;
			if(troot == sh.fun_tree)
			{
				/*
				 *functions can be exported or
				 * traced but not set
				 */
				if(flag&U_FLAG)
					np = env_namset(a1,sh.fun_tree,P_FLAG|V_FLAG);
				else
					np = nam_search(a1,sh.fun_tree,0);
				if(np && is_abuiltin(np))
					np = 0;
				if(np && ((flag&U_FLAG) || !isnull(np) || nam_istype(np,U_FLAG)))
				{
					if(flag==0)
					{
						env_prnamval(np,0);
						continue;
					}
					if(aflag=='-')
						nam_ontype(np,flag|N_FUNCTION);
					else if(aflag=='+')
						nam_offtype(np,~flag);
				}
				else
					r++;
				continue;
			}
			np = env_namset(a1,troot,(type|scoped));
			/* tracked alias */
			if(troot==sh.track_tree && aflag=='-')
			{
				nam_ontype(np,NO_ALIAS);
				path_alias(np,path_absolute(np->namid));
				continue;
			}
			if(flag==0 && aflag!='-' && strchr(a1,'=') == NULL)
			{
				/* type==0 for TYPESET */
				if(type && (isnull(np) || !env_prnamval(np,0)))
				{
					p_setout(ERRIO);
					SFLAG_E;
					p_str(a1,':');
					p_str(gettxt(E_ALIAS),NL);
					r++;
					p_setout(st.standout);
				}
				continue;
			}
			curflag = namflag(np);
			if (aflag == '-')
			{
				newflag = curflag;
				if(flag&~NO_CHANGE)
					newflag &= NO_CHANGE;
				newflag |= flag;
				if (flag & (N_LJUST|N_RJUST))
				{
					if (flag & N_LJUST)
						newflag &= ~N_RJUST;
					else
						newflag &= ~N_LJUST;
				}
				if (flag & N_UTOL)
					newflag &= ~N_LTOU;
				else if (flag & N_LTOU)
					newflag &= ~N_UTOL;
			}
			else
			{
				if((flag&R_FLAG) && (curflag&R_FLAG))
					sh_fail(np->namid,gettxt(E_READONLY));
				newflag = curflag & ~flag;
			}
			if (aflag && (argnum>0 || (curflag!=newflag)))
			{
				if(type)
					namflag(np) = newflag;
				else
					nam_newtype (np, newflag,argnum);
				nam_newtype (np, newflag,argnum);
			}
		}
	}
	else
		env_scan(fd,flag,troot,aflag=='+');
	return(r);
}

/*
 * The removing of Shell variable names, aliases, and functions
 * is performed here.
 * Unset functions with unset -f
 * Non-existent items being deleted give non-zero exit status
 */

int    b_unalias(argn,com)
char **com;
{
	return(b_unall(argn,com,sh.alias_tree));
}

int    b_unset(argn,com)
register char **com;
{
	struct Amemory *troot;
	if(com[1] && *com[1] == '-')
	{
		flagset(com[1],~F_FLAG);
		com++;
		argn--;
		troot = sh.fun_tree;
	}
	else
		troot = sh.var_tree;
	return(b_unall(argn,com,troot));
}

static int	b_unall(argn,com,troot)
register int argn;
char **com;
struct Amemory *troot;
{
	register char *a1;
	register struct namnod *np;
	register struct slnod *slp;
	int r = 0;
	if(st.subflag)
		return(0);
	if(argn < 2)
		sh_cfail(gettxt(E_NARGS));
	while(a1 = *++com)
	{
		np=env_namset(a1,troot,P_FLAG);
		if(np && !isnull(np))
		{
			if(troot==sh.var_tree)
			{
				if (nam_istype (np, N_RDONLY))
					sh_fail(np->namid,gettxt(E_READONLY));
				else if (nam_istype (np, N_RESTRICT))
					sh_fail(np->namid,gettxt(E_RESTRICTED));
#ifdef apollo
				{
					short namlen;
					namlen =strlen(np->namid);
					ev_$delete_var(np->namid,&namlen);
				}
#endif /* apollo */
			}
			else if(is_abuiltin(np))
			{
				r = 1;
				continue;
			}
			else if(slp=(struct slnod*)(np->value.namenv))
			{
				/* free function definition */
				stakdelete(slp->slptr);
				np->value.namenv = 0;
			}
			nam_free(np);
		}
		else
			r = 1;
	}
	return(r);
}

int	b_dot(argn,com)
char **com;
{
	register char *a1 = com[1];
	st.states &= ~MONITOR;
	if(a1)
	{
		register int flag;
		jmp_buf retbuf;
		jmp_buf *savreturn = sh.freturn;
#ifdef POSIX
		/* check for function first */
		register struct namnod *np;
		np = nam_search(a1,sh.fun_tree,0);
		if(np && !np->value.namval.ip)
		{
			if(!nam_istype(np,N_FUNCTION))
				np = 0;
			else
			{
				path_search(a1,0);
				if(np->value.namval.ip==0)
					sh_fail(a1,gettxt(E_FOUND));
			}
		}
		if(!np)
#endif /* POSIX */
		if((sh.un.fd=path_open(a1,path_get(a1))) < 0)
			sh_fail(a1,gettxt(E_FOUND));
		else
		{
			if(st.dot_depth++ > DOTMAX)
				sh_cfail(gettxt(E_RECURSIVE));
			if(argn > 2)
				arg_set(com+1);
			st.states |= BUILTIN;
			sh.freturn = (jmp_buf*)retbuf;
			flag = SETJMP(retbuf);
			if(flag==0)
			{
#ifdef POSIX
				if(np)
					sh_exec((union anynode*)(funtree(np))
					,(int)(st.states&(ERRFLG|MONITOR)));
				else
#endif /* POSIX */
				sh_eval((char*)0);
			}
			st.states &= ~BUILTIN;
			sh.freturn = savreturn;
			st.dot_depth--;
			if(flag && flag!=2)
				sh_exit(sh.exitval);
		}
	}
	else
		sh_cfail(gettxt(E_ARGEXP));
	return(sh.exitval);
}

int	b_times()
{
	struct tms tt;
	times(&tt);
	p_setout(st.standout);
	p_time(tt.tms_utime,' ');
	p_time(tt.tms_stime,NL);
	p_time(tt.tms_cutime,' ');
	p_time(tt.tms_cstime,NL);
	return(0);
}
		

/*
 * return and exit
 */

int	b_ret_exit(argn,com)
register char **com;
{
	register int flag;
	register int isexit = (**com=='e');
	NOT_USED(argn);
	if(st.subflag)
		return(0);
	flag = ((com[1]?atoi(com[1]):sh.oldexit)&EXITMASK);
	if(st.fn_depth>0 || (!isexit && (st.dot_depth>0||(st.states&PROFILE))))
	{
		sh.exitval = flag;
		LONGJMP(*sh.freturn,isexit?4:2);
	}
	/* force exit */
	st.states &= ~(PROMPT|PROFILE|BUILTIN|FUNCTION|LASTPIPE);
	sh_exit(flag);
	return(1);
}
		
/*
 * null command
 */
int	b_null(argn,com)
register char **com;
{
	NOT_USED(argn);
	return(**com=='f');
}
		
int	b_continue(argn,com)
register char **com;
{
	NOT_USED(argn);
	if(!st.subflag && st.loopcnt)
	{
		st.execbrk = st.breakcnt = 1;
		if(com[1])
			st.breakcnt = atoi(com[1]);
		if(st.breakcnt > st.loopcnt)
			st.breakcnt = st.loopcnt;
		else
			st.breakcnt = -st.breakcnt;
	}
	return(0);
}
		
int	b_break(argn,com)
register char **com;
{
	NOT_USED(argn);
	if(!st.subflag && st.loopcnt)
	{
		st.execbrk = st.breakcnt = 1;
		if(com[1])
			st.breakcnt = atoi(com[1]);
		if(st.breakcnt > st.loopcnt)
			st.breakcnt = st.loopcnt;
	}
	return(0);
}
		
int	b_trap(argn,com)
char **com;
{
	register char *a1 = com[1];
	register int sig;
	NOT_USED(argn);
	if(a1)
	{
		register int	clear;
		char *action = a1;
		if(st.subflag)
			return(0);
		/* first argument all digits or - means clear */
		while(isdigit(*a1))
			a1++;
		clear = (a1!=action && *a1==0);
		if(!clear)
		{
			++com;
			if(*action=='-' && action[1]==0)
				clear++;
		}
		while(a1 = *++com)
		{
			sig = sig_number(a1);
			if(sig>=MAXTRAP || sig<MINTRAP)
				sh_fail(a1,gettxt(E_TRAP));
			else if(clear)
				sig_clear(sig);
			else
			{
				if(a1=st.trapcom[sig])
					free(a1);
				st.trapcom[sig] = sh_heap(action);
				if(*action)
					sig_ontrap(sig);
				else
					sig_ignore(sig);
			}
		}
	}
	else /* print out current traps */
#ifdef POSIX
		sig_list(-1);
#else
	{
		p_setout(st.standout);
		for(sig=0; sig<MAXTRAP; sig++)
			if(st.trapcom[sig])
			{
				p_num(sig,':');
				p_str(st.trapcom[sig],NL);
			}
	}
#endif /* POSIX */
	return(0);
}
		
int	b_chdir(argn,com)
char **com;
{
	register char *a1 = com[1];
	register const char *dp;
	register char *cdpath = NULLSTR;
	int flag = 0;
	int rval = -1;
	char *oldpwd;
	if(st.subflag)
		return(0);
	if(is_option(RSHFLG))
		sh_cfail(gettxt(E_RESTRICTED));
#ifdef LSTAT
#ifdef apollo
	/* 
	 * support for the apollo "set -o physical" feature.
	 */
	flag = is_option(PHYSICAL);
#endif /* apollo */
	while(a1 && *a1=='-' && a1[1]) 
	{
		flag = flagset(a1,~(PHYS_MODE|LOG_MODE));
		if(flag&LOG_MODE)
			flag = 0;
		com++;
		argn--;
		a1 =  com[1];
	}
#endif /* LSTAT */
	if(argn >3)
		sh_cfail(gettxt(E_NARGS));
	oldpwd = sh.pwd;
	if(argn==3)
		a1 = sh_substitute(oldpwd,a1,com[2]);
	else if(a1==0 || *a1==0)
		a1 = nam_strval(HOME);
	else if(*a1 == '-' && *(a1+1) == 0)
		a1 = nam_strval(OLDPWDNOD);
	if(a1==0 || *a1==0)
		sh_cfail(argn==3?gettxt(E_SUBST):gettxt(E_DIRECT));
	if(*a1 != '/')
	{
		cdpath = nam_fstrval(CDPNOD);
		if(oldpwd==0)
			oldpwd = path_pwd(1);
	}
	if(cdpath==0)
		cdpath = NULLSTR;
	if(*a1=='.')
	{
		/* test for pathname . ./ .. or ../ */
		if(*(dp=a1+1) == '.')
			dp++;
		if(*dp==0 || *dp=='/')
			cdpath = NULLSTR;
	}
	do
	{
		dp = cdpath;
		cdpath=path_join((char*)dp,a1);
		if(*stakptr(OPATH)!='/')
		{
			char *last=(char*)stakfreeze(1);
			stakseek(OPATH);
			stakputs(oldpwd);
			stakputc('/');
			stakputs(last+OPATH);
			stakputc(0);
		}
#ifdef LSTAT
		if(!flag)
#endif /* LSTAT */
		{
			register char *cp;
#ifdef FS_3D
			if(!(cp = pathcanon(stakptr(OPATH))))
				continue;
			/* eliminate trailing '/' */
			while(*--cp == '/' && cp>stakptr(OPATH))
				*cp = 0;
#else
			if(*(cp=stakptr(OPATH))=='/')
				if(!pathcanon(cp))
					continue;
#endif /* FS_3D */
		}
		rval = chdir(path_relative(stakptr(OPATH)));
	}
	while(rval<0 && cdpath);
	/* use absolute chdir() if relative chdir() fails */
	if(rval<0 && *a1=='/' && *(path_relative(stakptr(OPATH)))!='/')
		rval = chdir(a1);
#ifdef apollo
	/*
	 * The label is used to display the error message if path_physical()
	 * routine fails.(See below)
	 */
unavoidable_goto:
#endif /* apollo */
	if(rval<0)
	{
		switch(errno)
		{
#ifdef ENAMETOOLONG
		case ENAMETOOLONG:
			dp = gettxt(E_LONGNAME);
			break;
#endif /* ENAMETOOLONG */
#ifdef EMULTIHOP
		case EMULTIHOP:
			dp = gettxt(E_MULTIHOP);
			break;
#endif /* EMULTIHOP */
		case ENOTDIR:
			dp = gettxt(E_NOTDIR);
			break;

		case ENOENT:
			dp = gettxt(E_FOUND);
			break;

		case EACCES:
			dp = gettxt(E_ACCESS);
			break;
#ifdef ENOLINK
		case ENOLINK:
			dp = gettxt(E_LINK);
			break;
#endif /* ENOLINK */
		default: 	
			dp = gettxt(E_DIRECT);
			break;
		}
		sh_fail(a1,dp);
	}
	if(a1 == nam_strval(OLDPWDNOD) || argn==3)
		dp = a1;	/* print out directory for cd - */
#ifdef LSTAT
	if(flag)
	{
		/* make sure at least PATH_MAX bytes on stack */
		if((flag=staktell()) < OPATH+PATH_MAX)
			stakseek(OPATH+PATH_MAX);
		a1 = stakseek(OPATH)+OPATH;
#ifdef apollo
		/*
		 * check the return status of path_physical().
		 * if the return status is 0 then the getwd() has
		 * failed, so print an error message.
		 */
		if ( !path_physical(a1) )
		{
			rval = -1;
			goto unavoidable_goto;
		}
#else
		path_physical(a1);
#endif /* apollo */
		stakseek(OPATH+strlen(a1));
		a1 = (char*)stakfreeze(1)+OPATH;
	}
	else
#endif /* LSTAT */
		a1 = (char*)stakfreeze(1)+OPATH;
	if(*dp && *dp!= ':' && (st.states&PROMPT) && strchr(a1,'/'))
	{
		p_setout(st.standout);
		p_str(a1,NL);
	}
	if(*a1 != '/')
		return(0);
	nam_fputval(OLDPWDNOD,oldpwd);
	if(sh.pwd)
		free(sh.pwd);
	nam_free(PWDNOD);
	nam_fputval(PWDNOD,a1);
	nam_ontype(PWDNOD,N_FREE|N_EXPORT);
	sh.pwd = PWDNOD->value.namval.cp;
	return(0);
}
		
int	b_shift(argn,com)
register char **com;
{
	register int flag = (com[1]?(int)sh_arith(com[1]):1);
	NOT_USED(argn);
	if(flag<0 || st.dolc<flag)
		sh_cfail(gettxt(E_NUMBER));
	else
	{
		if(st.subflag)
			return(0);
		st.dolv += flag;
		st.dolc -= flag;
	}
	return(0);
}
		
int	b_wait(argn,com)
register char **com;
{
	NOT_USED(argn);
	st.states &= ~MONITOR;
	if(!st.subflag)
		job_bwait(com+1);
	return(sh.exitval);
}
		
int	b_read(argn,com)
char **com;
{
	register char *a1 = com[1];
	register int flag;
	register int fd;
	int r;
	NOT_USED(argn);
	st.states &= ~MONITOR;
	argnum = 0;
	com += scanargs(com,~(R_FLAG|P_FLAG|U_FLAG|S_FLAG));
	a1 = com[1];
	flag = newflag;
	if(flag&P_FLAG)
	{
		if((fd = sh.cpipe[INPIPE])<=0)
			sh_cfail(gettxt(E_QUERY));
	}
	else if(flag&U_FLAG)
		fd = argnum;
	else
		fd = 0;
	if(fd && !fisread(fd))
		sh_cfail(gettxt(E_FILE));
	/* look for prompt */
	if(a1 && (a1=strchr(a1,'?')) && tty_check(fd))
	{
		p_setout(ERRIO);
		p_str(a1+1,0);
	}
	env_readline(&com[1],fd,flag&(R_FLAG|S_FLAG));
	if(r=(fiseof(io_ftable[fd])!=0))
	{
		if(flag&P_FLAG)
		{
			io_pclose(sh.cpipe);
			return(1);
		}
	}
	clearerr(io_ftable[fd]);
	return(r);
}
	
int	b_set(argn,com)
register char **com;
{
	if(com[1])
	{
		arg_opts(argn,com);
		st.states &= ~(READPR|MONITOR);
		st.states |= is_option(READPR|MONITOR);
	}
	else
		/*scan name chain and print*/
		env_scan(st.standout,0,sh.var_tree,0);
	return(sh.exitval);
}

int	b_eval(argn,com)
register char **com;
{
	NOT_USED(argn);
	st.states &= ~MONITOR;
	if(com[1])
	{
		sh.un.com = com+2;
		sh_eval(com[1]);
	}
	return(sh.exitval);
}

int	b_fc(argn,com)
char **com;
{
	register char *a1;
	register int flag;
	register struct history *fp;
	struct stat statb;
	time_t	before = 0;
	int fdo;
	char *argv[2];
	char fname[TMPSIZ];
	int index2;
	int indx = -1; /* used as subscript for range */
	char *edit = NULL;		/* name of editor */
	char *replace = NULL;		/* replace old=new */
	int incr;
	int range[2];	/* upper and lower range of commands */
	int lflag = 0;
	int nflag = 0;
	int rflag = 0;
	histloc location;
	NOT_USED(argn);
	if(!hist_open())
		sh_cfail(gettxt(E_HISTORY));
	fp = hist_ptr;
	while((a1=com[1]) && *a1 == '-')
	{
		argnum = -1;
		flag = flagset(a1,~(E_FLAG|L_FLAG|N_FLAG|R_FLAG));
		if(flag==0)
		{
			if(argnum < 0)
			{
				com++;
				break;
			}
			flag = fp->fixind - argnum-1;
			if(flag <= 0)
				flag = 1;
			range[++indx] = flag;
			argnum = 0;
			if(indx==1)
				break;
		}
		else
		{
			if(flag&E_FLAG)
			{
				/* name of editor specified */
				com++;
				if((edit=com[1]) == NULL)
					sh_cfail(gettxt(E_ARGEXP));
			}
			if(flag&N_FLAG)
				nflag++;
			if(flag&L_FLAG)
				lflag++;
			if(flag&R_FLAG)
				rflag++;
		}
		com++;
	}
	flag = indx;
	while(flag<1 && (a1=com[1]))
	{
		/* look for old=new argument */
		if(replace==NULL && strchr(a1+1,'='))
		{
			replace = a1;
			com++;
			continue;
		}
		else if(isdigit(*a1) || *a1 == '-')
		{
			/* see if completely numeric */
			do	a1++;
			while(isdigit(*a1));
			if(*a1==0)
			{
				a1 = com[1];
				range[++flag] = atoi(a1);
				if(*a1 == '-')
					range[flag] += (fp->fixind-1);
				com++;
				continue;
			}
		}
		/* search for last line starting with string */
		location = hist_find(com[1],fp->fixind-1,0,-1);
		if((range[++flag] = location.his_command) < 0)
			sh_fail(com[1],gettxt(E_FOUND));
		com++;
	}
	if(flag <0)
	{
		/* set default starting range */
		if(lflag)
		{
			flag = fp->fixind-16;
			if(flag<1)
				flag = 1;
		}
		else
			flag = fp->fixind-2;
		range[0] = flag;
		flag = 0;
	}
	if(flag==0)
		/* set default termination range */
		range[1] = (lflag?fp->fixind-1:range[0]);
	if((index2 = fp->fixind - fp->fixmax) <=0)
	index2 = 1;
	/* check for valid ranges */
	for(flag=0;flag<2;flag++)
		if(range[flag]<index2 ||
			range[flag]>=(fp->fixind-(lflag==0)))
			sh_cfail(gettxt(E_NUMBER));
	if(edit && *edit=='-' && range[0]!=range[1])
		sh_cfail(gettxt(E_NUMBER));
	/* now list commands from range[rflag] to range[1-rflag] */
	incr = 1;
	flag = rflag>0;
	if(range[1-flag] < range[flag])
		incr = -1;
	if(lflag)
	{
		fdo = st.standout;
		a1 = "\n\t";
	}
	else
	{
		fdo = io_mktmp(fname);
		a1 = "\n";
		nflag++;
	}
	p_setout(fdo);
	while(1)
	{
		if(nflag==0)
			p_num(range[flag],'\t');
		else if(lflag)
			p_char('\t');
		hist_list(hist_position(range[flag]),0,a1);
		if(lflag && (sh.trapnote&SIGSET))
			sh_exit(SIGFAIL);
		if(range[flag] == range[1-flag])
			break;
		range[flag] += incr;
	}
	if(lflag)
		return(0);
	if(fstat(fdo,&statb)>=0)
		before = statb.st_mtime;
	io_fclose(fdo);
	hist_eof();
	p_setout(ERRIO);
	a1 = edit;
	if(a1==NULL && (a1=nam_strval(FCEDNOD)) == NULL)
		a1 = (char*)e_defedit;
#ifdef apollo
	/*
	 * Code to support the FC using the pad editor.
	 * Exampled of how to use: FCEDIT=pad
	 */
	if (strcmp (a1, "pad") == 0)
	{
		int pad_create();
		io_fclose(fdo);
		fdo = pad_create(fname);
		pad_wait(fdo);
		unlink(fname);
		strcat(fname, ".bak");
		unlink(fname);
		io_seek(fdo,(off_t)0,SEEK_SET);
	}
	else
	{
#endif /* apollo */
	if(*a1 != '-')
	{
		sh.un.com = argv;
		argv[0] =  fname;
		argv[1] = NULL;
		sh_eval(a1);
	}
	fdo = io_fopen(fname);
	/* if the file hasn't changed, treat this as a error */
	if(*a1!='-' && fstat(fdo,&statb)>=0 && before==statb.st_mtime)
		sh.exitval = 1;
	unlink(fname);
#ifdef apollo
	}
#endif /* apollo */
	/* don't history fc itself unless forked */
	if(!(st.states&FORKED))
		hist_cancel();
	st.states |= (READPR|FIXFLG);	/* echo lines as read */
	st.exec_flag--;  /* needed for command numbering */
	if(replace!=NULL)
		hist_subst(sh.cmdname,fdo,replace);
	else if(sh.exitval == 0)
	{
		/* read in and run the command */
		st.states &= ~BUILTIN;
		sh.un.fd = fdo;
		sh_eval((char*)0);
	}
	else
	{
		io_fclose(fdo);
		if(!is_option(READPR))
			st.states &= ~(READPR|FIXFLG);
	}
	st.exec_flag++;
	return(sh.exitval);
}

int	b_getopts(argn,com)
char **com;
{
	register char *a1 = com[1];
	register int flag;
	register struct namnod *n;
	int r = 0;
	extern char opt_option[];
	extern char *opt_arg;
	static char value[2];
	const char *message = gettxt(E_ARGEXP);
	if(argn < 3)
		sh_cfail(gettxt(E_ARGEXP));
	n = env_namset(com[2],sh.var_tree,P_FLAG);
	if(argn>3)
	{
		com +=2;
		argn -=2;
	}
	else
	{
		com = st.dolv;
		argn = st.dolc;
	}
	switch(opt_index<=argn?flag=optget(com,a1):0)
	{
		case '?':
			message = gettxt(E_OPTION);
			/* fall through */
		case ':':
			if(*a1==':')
				opt_arg = opt_option+1;
			else
			{
				SFLAG_E;
				p_setout(ERRIO);
				p_str(e_colon,opt_option[1]);
				p_char(' ');
				p_str(message,NL);
				flag = '?';
			}
			*(a1 = value) = flag;
			break;

		case 0:
			a1 = NULLSTR;
			r = 1;
			opt_char = 0;
			break;

		default:
			a1 = opt_option + (*opt_option=='-');
	}
	nam_putval(n, a1);
	return(r);
}
	
int	b_whence(argn,com)
register char **com;
{
	NOT_USED(argn);
	com += scanargs(com,~(V_FLAG|P_FLAG));
	if(com[1] == 0)
		sh_cfail(gettxt(E_NARGS));
	p_setout(st.standout);
	return(sh_whence(com,newflag));
}


int	b_umask(argn,com)
char **com;
{
	register char *a1 =com[1];
	register int flag = 0;
	NOT_USED(argn);
	if(a1)
	{
		register int c;	
		if(st.subflag)
			return(0);
		if(isdigit(*a1))
		{
			while(c = *a1++)
			{
				if (c>='0' && c<='7')	
					flag = (flag<<3) + (c-'0');	
				else
					sh_cfail(gettxt(E_NUMBER));
			}
			if( flag < 0 || flag > 0777 )
				sh_cfail(gettxt(E_NUMBER));
		}
		else
		{
			char **cp = com+1;
			flag = umask(0);
			c = strperm(a1,cp,~flag);
			if(**cp)
			{
				umask(flag);
				sh_cfail(gettxt(E_FORMAT));
			}
			flag = (~c&0777);
		}
		umask(flag);	
	}	
	else
	{
		p_setout(st.standout);
		a1 = utos((ulong)(flag=umask(0)),8);
		umask(flag);
		*++a1 = '0';
		p_str(a1,NL);
	}
	return(0);
}

#ifdef LIM_CPU
#		define HARD	1
#		define SOFT	2
		 /* BSD style ulimit */
int	b_ulimit(argn,com)
char **com;
{
	register char *a1;
	register int flag = 0;
#   ifdef RLIMIT_CPU
	struct rlimit rlp;
#   endif /* RLIMIT_CPU */
	const struct sysnod *sp;
	long i;
	int label;
	register int n;
	register int mode = 0;
	int unit;
	int noargs;
	int save_index = opt_index;
	int save_char = opt_char;
	NOT_USED(argn);
	opt_char = 0;
	opt_index = 1;
	while((n = optget(com,":HSacdfmnstv")))
	{
		switch(n)
		{
			case 'H':
				mode |= HARD;
				continue;
			case 'S':
				mode |= SOFT;
				continue;
			case 'a':
				flag = (0x2f
#   ifdef LIM_MAXRSS
				|(1<<4)
#   endif /* LIM_MAXRSS */
#   ifdef RLIMIT_NOFILE
				|(1<<6)
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
				|(1<<7)
#   endif /* RLIMIT_VMEM */
					);
				break;
			case 't':
				flag |= 1;
				break;
#   ifdef LIM_MAXRSS
			case 'm':
				flag |= (1<<4);
				break;
#   endif /* LIM_MAXRSS */
			case 'd':
				flag |= (1<<2);
				break;
			case 's':
				flag |= (1<<3);
				break;
			case 'f':
				flag |= (1<<1);
				break;
			case 'c':
				flag |= (1<<5);
				break;
#   ifdef RLIMIT_NOFILE
			case 'n':
				flag |= (1<<6);
				break;
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
			case 'v':
				flag |= (1<<7);
				break;
#   endif /* RLIMIT_VMEM */
			default:
				sh_cfail(gettxt(E_OPTION));
		}
	}
	com += opt_index;
	a1 = *com;
	opt_index = save_index;
	opt_char = save_char;
	/* default to -f */
	if(noargs=(flag==0))
		flag |= (1<<1);
	/* only one option at a time for setting */
	label = (flag&(flag-1));
	if(a1)
	{
		if(label)
			sh_cfail(gettxt(E_OPTION));
		if(com[1])
			sh_cfail(gettxt(E_NARGS));
	}
	sp = limit_names;
	if(mode==0)
		mode = (HARD|SOFT);
	for(; flag; sp++,flag>>=1)
	{
		if(!(flag&1))
			continue;
		n = sp->sysval>>11;
		unit = sp->sysval&0x7ff;
		if(a1)
		{
			if(st.subflag)
				return(0);
			if(strcmp(a1,e_unlimited)==0)
				i = INFINITY;
			else
			{
				if((i=sh_arith(a1)) < 0)
					sh_cfail(gettxt(E_NUMBER));
				i *= unit;
			}
#   ifdef RLIMIT_CPU
			if(getrlimit(n,&rlp) <0)
				sh_cfail(gettxt(E_NUMBER));
			if(mode&HARD)
				rlp.rlim_max = i;
			if(mode&SOFT)
				rlp.rlim_cur = i;
			if(setrlimit(n,&rlp) <0)
				sh_cfail(gettxt(E_ULIMIT));
#   endif /* RLIMIT_CPU */
		}
		else
		{
#   ifdef  RLIMIT_CPU
			if(getrlimit(n,&rlp) <0)
				sh_cfail(gettxt(E_NUMBER));
			if(mode&HARD)
				i = rlp.rlim_max;
			if(mode&SOFT)
				i = rlp.rlim_cur;
#   else
			i = -1;
		}
		if((i=vlimit(n,i)) < 0)
			sh_cfail(gettxt(E_NUMBER));
		if(a1==0)
		{
#   endif /* RLIMIT_CPU */
			SFLAG_N;
			p_setout(st.standout);
			if(label)
				p_str(sp->sysnam,SP);
			/* ulimit without args give numerical limit */
			if(i!=INFINITY || noargs)
			{
				if(!noargs)
					i += (unit-1);
				i /= unit;
				p_str(utos((ulong)i,10),NL);
			}
			else
				p_str(gettxt(E_UNLIMITED),NL);
		}
	}
	return(0);
}
#else
int	b_ulimit(argn,com)
char **com;
{
	register char *a1 = com[1];
	register int flag = 0;
#   ifndef VENIX
	long i;
	long ulimit();
	register int mode = 2;
	NOT_USED(argn);
	if(a1 && *a1 == '-')
	{
#	ifdef RT
		flag = flagset(a1,~(F_FLAG|P_FLAG));
#	else
		flag = flagset(a1,~F_FLAG);
#	endif /* RT */
		a1 = com[2];
	}
	if(flag&P_FLAG)
		mode = 5;
	if(a1)
	{
		if(st.subflag)
			return(0);
		if((i=sh_arith(a1)) < 0)
			sh_cfail(gettxt(E_NUMBER));
	}
	else
	{
		mode--;
		i = -1;
	}
	if((i=ulimit(mode,i)) < 0)
		sh_cfail(gettxt(E_NUMBER));
	if(a1==0)
	{
		p_setout(st.standout);
		p_str(utos((ulong)i,10),NL);
	}
#   endif /* VENIX */
	return(0);
}
#endif /* LIM_CPU */

#ifdef JOBS
#   ifdef SIGTSTP
int	b_bgfg(argn,com)
register char **com;
{
	register int flag = (**com=='b');
	NOT_USED(argn);
	if(!is_option(MONITOR) || !job.jobcontrol)
	{
		if(st.states&PROMPT)
			sh_cfail(gettxt(E_NO_JCTL));
		return(1);
	}
	if(job_walk(job_switch,flag,com+1))
		sh_cfail(gettxt(E_NO_JOB));
	return(sh.exitval);
}
#    endif /* SIGTSTP */

int	b_jobs(argn,com)
char **com;
{
	NOT_USED(argn);
	com += scanargs(com,~(N_FLAG|L_FLAG|P_FLAG));
	if(*++com==0)
		com = 0;
	p_setout(st.standout);
	if(job_walk(job_list,newflag,com))
		sh_cfail(gettxt(E_NO_JOB));
	return(sh.exitval);
}

int	b_kill(argn,com)
char **com;
{
	register int flag;
	register char *a1 = com[1];
	if(argn < 2)
		sh_cfail(gettxt(E_NARGS));
	/* just in case we send a kill -9 $$ */
	p_flush();
	flag = SIGTERM;
	if(*a1 == '-')
	{
		a1++;
		if(*a1 == 'l')
		{
#ifdef POSIX
			if(argn>2)
			{
				com++;
				while(a1= *++com)
				{
					if(isdigit(*a1))
						sig_list((atoi(a1)&0177)+1);
					else
					{
						if((flag=sig_number(a1)) < 0)
							sh_cfail(gettxt(E_OPTION));
						p_num(flag,NL);
					}
				}
			}
			else
#endif /* POSIX */
			sig_list(0);
			return(0);
		}
		else if(*a1=='s')
		{
			com++;
			a1 = com[1];
			if (! a1) goto kill_err;
		}
		if(!a1 || (flag=sig_number(a1)) <0 || flag >= NSIG)
			sh_cfail(gettxt(E_OPTION));
		com++;
	}
kill_err:
	if(*++com==0)
		sh_cfail(gettxt(E_NARGS));
	if(job_walk(job_kill,flag,com))
		sh.exitval = 2;
	return(sh.exitval);
}
#endif

#ifdef LDYNAMIC
#   ifdef apollo
	/*
	 *  Apollo system support library loads into the virtual address space
	 */

	int	b_inlib(argn,com)
	char **com;
	{
		register char *a1 = com[1];
		int status;
		short len;
		std_$call void loader_$inlib();
		if(!st.subflag && a1)
		{
			len = strlen(a1);
			loader_$inlib(*a1, len, status);
			if(status!=0)
				sh_fail(a1,gettxt(E_BADINLIB));
		}
		return(0);
	}
#else
	/*
	 * dynamic library loader from Ted Kowalski
	 */

	int	b_inlib(argn,com)
	char **com;
	{
		register char *a1;
		if(!st.subflag)
		{
			ldinit();
			addfunc(ldname("nam_putval", 0), (int (*)())nam_putval);
			addfunc(ldname("nam_strval", 0), (int (*)())nam_strval);
			addfunc(ldname("p_setout", 0), (int (*)())p_setout);
			addfunc(ldname("p_str", 0), (int (*)())p_str);
			addfunc(ldname("p_flush", 0), (int (*)())p_flush);
			while(a1= *++com)
			{
				if(!ldfile(a1))
					sh_fail(a1,gettxt(E_BADINLIB));
			}
			loadend();
			if(undefined()!=0)
				sh_cfail(gettxt(E_UNDEF_SYM));
		}
	}
	/*
	 * bind a built-in name to the function that implements it
	 * uses Ted Kowalski's run-time loader
	 */
	int	b_builtin(argn,com)
	char **com;
	{
		register struct namnod *np;
		int (*fn)();
		int (*ret_func())();
		if(argn!=3)
			sh_cfail(gettxt(E_NARGS));
		if(!(np = nam_search(com[1],sh.fun_tree,N_ADD)))
			sh_fail(com[1],gettxt(E_CREATE));
		if(!isnull(np))
			sh_fail(com[1],gettxt(IS_BUILTIN));
		if(!(fn = ret_func(ldname(com[2],0))))
			sh_fail(com[2],gettxt(E_FOUND));
		funptr(np) = fn;
		nam_ontype(np,N_BLTIN);
	}
#   endif	/* !apollo */
#endif /* LDYNAMIC */


#ifdef FS_3D
#   define VLEN		14
    int	b_vpath_map(argn,com)
    char **com;
    {
	register int flag = (com[0][1]=='p'?2:4);
	register char *a1 = com[1];
	char version[VLEN+1];
	char *vend; 
	int n;
	switch(argn)
	{
	case 1:
	case 2:
		flag |= 8;
		p_setout(st.standout);
		if((n = mount(a1,version,flag)) >= 0)
		{
			vend = stakalloc(++n);
			n = mount(a1,vend,flag|(n<<4));
		}
		if(n < 0)
		{
			if(flag==2)
				sh_cfail(gettxt(E_NGET_MAP));
			else
				sh_cfail(gettxt(E_NGET_VER));
		}
		if(argn==2)
		{
			p_str(vend,NL);
			break;
		}
		n = 0;
		while(flag = *vend++)
		{
			if(flag==' ')
			{
				flag  = e_sptbnl[n+1];
				n = !n;
			}
			p_char(flag);
		}
		if(n)
			newline();
		break;
	default:
		if(!(argn&1))
			sh_cfail(gettxt(E_NARGS));
		/*FALLTHROUGH*/
	case 3:
		if(st.subflag)
			break;
 		for(n=1;n<argn;n+=2)
			if(mount(com[n+1],com[n],flag)<0)
			{
				if(flag==2)
					sh_cfail(gettxt(E_NSET_MAP));
				else
					sh_cfail(gettxt(E_NSET_VPH));
			}
	}
	return(0);
    }
#endif /* FS_3D */

#ifdef UNIVERSE
    /*
     * there are three styles of universe 
     * Pyramid and Sequent universes have <sys/universe.h> file
     * Masscomp universes do not
     */

    int	b_universe(argn,com)
    char **com;
    {
	register char *a1 = com[1];
	if(a1)
	{
		if(setuniverse(univ_number(a1)) < 0)
			sh_cfail(gettxt(E_INVNAME));
		att_univ = (strcmp(a1,"att")==0);
		/* set directory in new universe */
		if(*(a1 = path_pwd(0))=='/')
			chdir(a1);
		/* clear out old tracked alias */
		stakseek(0);
		stakputs((PATHNOD)->namid);
		stakputc('=');
		stakputs(nam_strval(PATHNOD));
		a1 = stakfreeze(1);
		env_namset(a1,sh.var_tree,nam_istype(PATHNOD,~0));
	}
	else
	{
		int flag;
		char Xuniverse[TMPSIZ];
		if(getuniverse(Xuniverse) < 0)
			sh_cfail(gettxt(E_NACCESS));
		else
			p_str(Xuniverse,NL);
	}
	return(0);
    }
#endif /* UNIVERSE */


#ifdef SYSSLEEP
    /* fine granularity sleep builtin someday */
    int	b_sleep(argn,com)
    char **com;
    {
	extern double atof();
	register char *a1 = com[1];
	if(a1)
	{
		if(strmatch(a1,"*([0-9])?(.)*([0-9])"))
			sh_delay(atof(a1));
		else
			sh_cfail(gettxt(E_NUMBER));
	}
	else
		sh_cfail(gettxt(E_ARGEXP));
	return(0);
    }
#endif /* SYSSLEEP */

static const char flgchar[] = "efgilmnprstuvxEFHLPRZ";
static const int flgval[] = {E_FLAG,F_FLAG,G_FLAG,I_FLAG,L_FLAG,M_FLAG,
			N_FLAG,P_FLAG,R_FLAG,S_FLAG,T_FLAG,U_FLAG,V_FLAG,
			X_FLAG,N_DOUBLE|N_INTGER|N_EXPNOTE,N_DOUBLE|N_INTGER,
			N_HOST,N_LJUST,H_FLAG,N_RJUST,N_RJUST|N_ZFILL};
/*
 * process option flags for built-ins
 * flagmask are the invalid options
 */

static int flagset(flaglist,flagmask)
char *flaglist;
{
	register int flag = 0;
	register int c;
	register char *cp,*sp;
	int numset = 0;

	for(cp=flaglist+1;c = *cp;cp++)
	{
		if(isdigit(c))
		{
			if(argnum < 0)
			{
				argnum = 0;
				numset = -100;
			}
			else
				numset++;
			argnum = 10*argnum + (c - '0');
		}
		else if(sp=strchr(flgchar,c))
			flag |= flgval[sp-flgchar];
		else if(c!= *flaglist)
			goto badoption;
	}
	if(numset>0 && flag==0)
		goto badoption;
	if((flag&flagmask)==0)
		return(flag);
badoption:
	sh_cfail(gettxt(E_OPTION));
	/* NOTREACHED */
}

/*
 * process command line options and store into newflag
 */

static int scanargs(com,flags)
char *com[];
{
	register char **argv = ++com;
	register int flag;
	register char *a1;
	newflag = 0;
	if(*argv)
		aflag = **argv;
	else
		aflag = 0;
	if(aflag!='+' && aflag!='-')
		return(0);
	while((a1 = *argv) && *a1==aflag)
	{
		if(a1[1] && a1[1]!=aflag)
			flag = flagset(a1,flags);
		else
			flag = 0;
		argv++;
		if(flag==0)
			break;
		newflag |= flag;
	}
	return(argv-com);
}

/*
 * evaluate the string <s> or the contents of file <un.fd> as shell script
 * If <s> is not null, un is interpreted as an argv[] list instead of a file
 */

void sh_eval(s)
register char *s;
{
	struct fileblk	fb;
	union anynode *t;
	char inbuf[IOBSIZE+1];
	struct ionod *saviotemp = st.iotemp;
	struct slnod *saveslp = st.staklist;
	io_push(&fb);
	if(s)
	{
		io_sopen(s);
		if(sh.un.com)
		{
			fb.feval=sh.un.com;
			if(*fb.feval)
				fb.ftype = F_ISEVAL;
		}
	}
	else if(sh.un.fd>=0)
	{
		io_init(input=sh.un.fd,&fb,inbuf);
	}
	sh.un.com = 0;
	st.exec_flag++;
	t = sh_parse(NL,NLFLG|MTFLG);
	st.exec_flag--;
	if(is_option(READPR)==0)
		st.states &= ~READPR;
	if(s==NULL && hist_ptr)
		hist_flush();
	p_setout(ERRIO);
	io_pop(0);
	sh_exec(t,(int)(st.states&(ERRFLG|MONITOR)));
	sh_freeup();
	st.iotemp = saviotemp;
	st.staklist = saveslp;
}


/*
 * Given the name or number of a signal return the signal number
 */

static int sig_number(string)
register char *string;
{
	register int n;
	if(isdigit(*string))
		n = atoi(string);
	else
	{
		ltou(string,string);
		n = sh_lookup(string,sig_names);
		n &= (1<<SIGBITS)-1;
		n--;
	}
	return(n);
}

#ifdef JOBS
/*
 * list all the possible signals
 * If flag is 1, then the current trap settings are displayed
 */
static void sig_list(flag)
{
	register const struct sysnod	*syscan;
	register int n = MAXTRAP;
	const char *names[MAXTRAP+3];
	syscan=sig_names;
	p_setout(st.standout);
	/* not all signals may be defined */
#ifdef POSIX
	if(flag<0)
		n += 2;
#else
	NOT_USED(flag);
#endif /* POSIX */
	while(--n >= 0)
		names[n] = e_trap;
	while(*syscan->sysnam)
	{
		n = syscan->sysval;
		n &= ((1<<SIGBITS)-1);
		names[n] = syscan->sysnam;
		syscan++;
	}
	n = MAXTRAP-1;
#ifdef POSIX
	if(flag<0)
		n += 2;
#endif /* POSIX */
	while(names[--n]==e_trap);
	names[n+1] = NULL;
#ifdef POSIX
	if(flag<0)
	{
		while(--n >= 0)
		{
			if(st.trapcom[n])
			{
				p_str("trap",' ');
				p_qstr(st.trapcom[n],' ');
				p_str(names[n+1],NL);
			}
		}
	}
	else if(flag)
	{
		if(flag <= n && names[flag])
			p_str(names[flag],NL);
		else
			p_num(flag-1,NL);
	}
	else
#endif /* POSIX */
		p_list(n-1,(char**)(names+2));
}
#endif	/* JOBS */

#ifdef SYSSLEEP
/*
 * delay execution for time <t>
 */

#ifdef _poll_
#   include	<poll.h>
#endif /* _poll_ */
#ifndef TIC_SEC
#   ifdef HZ
#	define TIC_SEC	HZ	/* number of ticks per second */
#   else
#	define TIC_SEC	60	/* number of ticks per second */
#   endif /* HZ */
#endif /* TIC_SEC */


int	sh_delay(t)
double t;
{
	register int n = t;
#ifdef _poll_
	struct pollfd fd;
	if(t<=0)
		return;
	else if(n > 30)
	{
		sleep(n);
		t -= n;
	}
	if(n=1000*t)
		poll(&fd,0,n);
#else
#   ifdef _SELECT5_
	struct timeval timeloc;
	if(t<=0)
		return;
	timeloc.tv_sec = n;
	timeloc.tv_usec = 1000000*(t-(double)n);
	select(0,(fd_set*)0,(fd_set*)0,(fd_set*)0,&timeloc);
#   else
#	ifdef _SELECT4_
		/* for 9th edition machines */
		if(t<=0)
			return;
		if(n > 30)
		{
			sleep(n);
			t -= n;
		}
		if(n=1000*t)
			select(0,(fd_set*)0,(fd_set*)0,n);
#	else
		struct tms tt;
		if(t<=0)
			return;
		sleep(n);
		t -= n;
		if(t)
		{
			clock_t begin = times(&tt);
			if(begin==0)
				return;
			t *= TIC_SEC;
			n += (t+.5);
			while((times(&tt)-begin) < n);
		}
#	endif /* _SELECT4_ */
#   endif /* _SELECT5_ */
#endif /* _poll_ */
}
#endif /* SYSSLEEP */

#ifdef UNIVERSE
#   ifdef _sys_universe_
	int univ_number(str)
	char *str;
	{
		register int n = 0;
		while( n < NUMUNIV)
		{
			if(strcmp(str,univ_name[n])==0)
				return(univ_index(n));
			n++;
		}
		return(-1);
	}
#   endif /* _sys_universe_ */
#endif /* UNIVERSE */
