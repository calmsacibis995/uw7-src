#ident	"@(#)ksh93:src/cmd/ksh93/bltins/misc.c	1.1"
#pragma prototyped
/*
 * exec [arg...]
 * eval [arg...]
 * jobs [-lnp] [job...]
 * login [arg...]
 * let expr...
 * . file [arg...]
 * :, true, false
 * vpath [top] [base]
 * vmap [top] [base]
 * wait [job...]
 * shift [n]
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
#include	"variables.h"
#include	"shnodes.h"
#include	"path.h"
#include	"io.h"
#include	"name.h"
#include	"history.h"
#include	"builtins.h"
#include	"jobs.h"

#define DOTMAX	32	/* maximum level of . nesting */

static int	clear;
static void     noexport(Namval_t*);
static char	*arg0;

int    b_exec(int argc,char *argv[], void *extra)
{
	register int n;
        sh.st.ioset = 0;
	clear = 0;
	NOT_USED(extra);
	while (n = optget(argv, (const char *)gettxt(sh_optexec_id,sh_optexec))) switch (n)
	{
	    case 'a':
		arg0 = opt_arg;
		argc = 0;
		break;
	    case 'c':
		clear=1;
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(0), opt_arg);
		return(2);
	}
	argv += opt_index;
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	if(*argv)
                b_login(argc,argv,0);
	return(0);
}

static void     noexport(register Namval_t* np)
{
	nv_offattr(np,NV_EXPORT);
}

int    b_login(int argc,char *argv[],void *extra)
{
	const char *pname;
	struct checkpt *pp = (struct checkpt*)sh.jmplist;
	NOT_USED(extra);
	if(sh_isoption(SH_RESTRICTED))
		error(ERROR_exit(1),gettxt(e_restricted_id,e_restricted),argv[0]);
	else
        {
		register struct argnod *arg=sh.envlist;
		register Namval_t* np;
		register char *cp;
		if(sh.subshell)
			sh_subfork();
		if(clear)
			nv_scan(sh.var_tree,noexport,NV_EXPORT,NV_EXPORT);
		while(arg)
		{
			if((cp=strchr(arg->argval,'=')) &&
				(*cp=0,np=nv_search(arg->argval,sh.var_tree,0)))
				nv_onattr(np,NV_EXPORT);
			if(cp)
				*cp = '=';
			arg=arg->argnxt.ap;
		}
		pname = argv[0];
		if(argc==0)
			argv[0] = arg0;
#ifdef JOBS
		if(job_close() < 0)
			return(1);
#endif /* JOBS */
		/* force bad exec to terminate shell */
		pp->mode = SH_JMPEXIT;
		sh_sigreset(2);
		sh_freeup();
		path_exec(pname,argv,NIL(struct argnod*));
		sh_done(0);
        }
	return(1);
}

int    b_let(int argc,char *argv[],void *extra)
{
	register int r;
	register char *arg;
	NOT_USED(argc);
	NOT_USED(extra);
	while (r = optget(argv,(const char *)gettxt(sh_optlet_id,sh_optlet))) switch (r)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	if(error_info.errors || !*argv)
		error(ERROR_usage(2),optusage((char*)0));
	while(arg= *argv++)
		r = !sh_arith(arg);
	return(r);
}

int    b_eval(int argc,char *argv[], void *extra)
{
	register int r;
	NOT_USED(argc);
	NOT_USED(extra);
	while (r = optget(argv,(const char *)gettxt(sh_opteval_id,sh_opteval))) switch (r)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(0), opt_arg);
		return(2);
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	argv += opt_index;
	if(*argv && **argv)
	{
		sh_offstate(SH_MONITOR);
		sh_eval(sh_sfeval(argv),0);
	}
	return(sh.exitval);
}

int    b_dot_cmd(register int n,char *argv[],void *extra)
{
	register char *script;
	register Namval_t *np;
	register int jmpval;
	int	fd;
	struct dolnod   *argsave=0, *saveargfor;
	char **saveargv;
	struct checkpt buff;
	Sfio_t *iop=0;
	NOT_USED(extra);
	while (n = optget(argv,(const char *)gettxt(sh_optdot_id,sh_optdot))) switch (n)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(0), opt_arg);
		return(2);
	}
	argv += opt_index;
	script = *argv;
	if(error_info.errors || !script)
		error(ERROR_usage(2),optusage((char*)0));
	if(!(np=sh.posix_fun))
	{
		/* check for KornShell style function first */
		np = nv_search(script,sh.fun_tree,0);
		if(np && is_afunction(np) && !nv_isattr(np,NV_FPOSIX))
		{
			if(!np->nvalue.ip)
			{
				path_search(script,NIL(char*),0);
				if(np->nvalue.ip)
				{
					if(nv_isattr(np,NV_FPOSIX))
						np = 0;
				}
				else
					error(ERROR_exit(1),gettxt(e_found_id,e_found),script);
			}
		}
		else
			np = 0;
		if(!np  && (fd=path_open(script,path_get(script))) < 0)
			error(ERROR_system(1),gettxt(e_open_id,e_open),script);
	}
	sh.posix_fun = 0;
	if(sh.dot_depth++ > DOTMAX)
		error(ERROR_exit(1),gettxt(e_toodeep_id,e_toodeep),script);
	if(np || argv[1])
	{
		n = sh.st.dolc;
		saveargv = sh.st.dolv;
		argsave = sh_argnew(argv,&saveargfor);
	}
	sh_pushcontext(&buff,SH_JMPDOT);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		if(np)
			sh_exec((union anynode*)(nv_funtree(np)),sh_isstate(SH_ERREXIT));
		else
		{
			char buff[IOBSIZE+1];
			iop = sfnew(NIL(Sfio_t*),buff,IOBSIZE,fd,SF_READ);
			sh_eval(iop,0);
		}
	}
	sh_popcontext(&buff);
	sh.dot_depth--;
	if((np || argv[1]) && jmpval!=SH_JMPSCRIPT)
	{
		sh_argreset(argsave,saveargfor);
		sh.st.dolc = n;
		sh.st.dolv = saveargv;
	}
	if(sh.exitval > SH_EXITSIG)
		sh_fault(sh.exitval&SH_EXITMASK);
	if(jmpval && jmpval!=SH_JMPFUN)
		siglongjmp(*sh.jmplist,jmpval);
	return(sh.exitval);
}

/*
 * null, true  command
 */
int    b_true(int argc,register char *argv[],void *extra)
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(extra);
	return(0);
}

/*
 * false  command
 */
int    b_false(int argc,register char *argv[], void *extra)
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(extra);
	return(1);
}

int    b_shift(register int n, register char *argv[], void *extra)
{
	register char *arg;
	NOT_USED(extra);
	while((n = optget(argv,sh_optcflow))) switch(n)
	{
		case ':':
			error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(0), opt_arg);
			return(2);
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	argv += opt_index;
	n = ((arg= *argv)?(int)sh_arith(arg):1);
	if(n<0 || sh.st.dolc<n)
		error(ERROR_exit(1),gettxt(e_number_id,e_number),arg);
	else
	{
		sh.st.dolv += n;
		sh.st.dolc -= n;
	}
	return(0);
}

int    b_wait(register int n,register char *argv[],void *extra)
{
	NOT_USED(extra);
	while((n = optget(argv,(const char *)gettxt(sh_optjoblist_id,sh_optjoblist)))) switch(n)
	{
		case ':':
			error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(2), opt_arg);
			break;
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	argv += opt_index;
	job_bwait(argv);
	return(sh.exitval);
}

#ifdef JOBS
int    b_bg_fg(register int n,register char *argv[],void *extra)
{
	register int flag = **argv;
	NOT_USED(extra);
	while((n = optget(argv,(const char *)gettxt(sh_optjoblist_id,sh_optjoblist)))) switch(n)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	argv += opt_index;
	if(!sh_isoption(SH_MONITOR) || !job.jobcontrol)
	{
		if(sh_isstate(SH_INTERACTIVE))
			error(ERROR_exit(1),gettxt(e_no_jctl_id,e_no_jctl));
		return(1);
	}
	if(flag=='d' && *argv==0)
		argv = (char**)0;
	if(job_walk(sfstdout,job_switch,flag,argv))
		error(ERROR_exit(1),gettxt(e_no_job_id,e_no_job));
	return(sh.exitval);
}

int    b_jobs(register int n,char *argv[],void *extra)
{
	register int flag = 0;
	NOT_USED(extra);
	while((n = optget(argv,(const char *)gettxt(sh_optjobs_id,sh_optjobs)))) switch(n)
	{
	    case 'l':
		flag = JOB_LFLAG;
		break;
	    case 'n':
		flag = JOB_NFLAG;
		break;
	    case 'p':
		flag = JOB_PFLAG;
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	if(*argv==0)
		argv = (char**)0;
	if(job_walk(sfstdout,job_list,flag,argv))
		error(ERROR_exit(1),gettxt(e_no_job_id,e_no_job));
	job_wait((pid_t)0);
	return(sh.exitval);
}
#endif

#ifdef _cmd_universe
/*
 * There are several universe styles that are masked by the getuniv(),
 * setuniv() calls.
 */
int	b_universe(int argc, char *argv[],void *extra)
{
	register char *arg;
	register int n;
	NOT_USED(extra);
	while((n = optget(argv,(const char *)gettxt(sh_optuniverse_id,sh_optuniverse)))) switch(n)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	argc -= opt_index;
	if(error_info.errors || argc>1)
		error(ERROR_usage(2),optusage((char*)0));
	if(arg = argv[0])
	{
		if(!astconf("_AST_UNIVERSE",0,arg))
			error(ERROR_exit(1),gettxt(e_badname_id,e_badname),arg);
	}
	else
	{
		if(!(arg=astconf("_AST_UNIVERSE",0,0)))
			error(ERROR_exit(1),gettxt(e_nouniverse_id,e_nouniverse));
		else
			sfputr(sfstdout,arg,'\n');
	}
	return(0);
}
#endif /* cmd_universe */

#ifdef SHOPT_FS_3D
    int	b_vpath_map(register int argc,char *argv[], void *extra)
    {
	register int flag, n;
	register const char *optstr; 
	register char *vend; 
	NOT_USED(extra);
	if(argv[0][1]=='p')
	{
		optstr = (const char *)gettxt(sh_optvpath_id,sh_optvpath);
		flag = FS3D_VIEW;
	}
	else
	{
		optstr = (const char *)gettxt(sh_optvmap_id,sh_optvmap);
		flag = FS3D_VERSION;
	}
	while(n = optget(argv, optstr)) switch(n)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	argv += opt_index;
	argc -= opt_index;
	switch(argc)
	{
	    case 0:
	    case 1:
		flag |= FS3D_GET;
		if((n = mount(*argv,(char*)0,flag,0)) >= 0)
		{
			vend = stakalloc(++n);
			n = mount(*argv,vend,flag|FS3D_SIZE(n),0);
		}
		if(n < 0)
			goto failed;
		if(argc==1)
		{
			sfprintf(sfstdout,"%s\n",vend);
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
			sfputc(sfstdout,flag);
		}
		if(n)
			sfputc(sfstdout,'\n');
		break;
	     default:
		if((argc&1))
			error(ERROR_usage(2),optusage((char*)0));
		/*FALLTHROUGH*/
	     case 2:
		if(!sh.lim.fs3d)
			goto failed;
		if(sh.subshell)
			sh_subfork();
 		for(n=0;n<argc;n+=2)
		{
			if(mount(argv[n+1],argv[n],flag,0)<0)
				goto failed;
		}
	}
	return(0);
failed:
	if(argc>1)
		if (flag==2)
			error(ERROR_exit(1),gettxt(e_cantsetmap_id,e_cantsetmap));
		else
			error(ERROR_exit(1),gettxt(e_cantsetver_id,e_cantsetver));
	else
		if (flag==2)
			error(ERROR_exit(1),gettxt(e_cantgetmap_id,e_cantgetmap));
		else
			error(ERROR_exit(1),gettxt(e_cantgetver_id,e_cantgetver));
	return(1);
    }
#endif /* SHOPT_FS_3D */

