#ident	"@(#)ksh93:src/cmd/ksh93/sh/subshell.c	1.2.1.1"
#pragma prototyped
/*
 *   Create and manage subshells avoiding forks when possible
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
#include	<ls.h>
#include	"io.h"
#include	"fault.h"
#include	"shnodes.h"
#include	"shlex.h"
#include	"jobs.h"
#include	"variables.h"
#include	"path.h"


/*
 * The following structure is used for command substitution and (...)
 */
static struct subshell
{
	struct subshell	*prev;	/* previous subshell data */
	struct subshell	*pipe;	/* subshell where output goes to pipe on fork */
	Hashtab_t	*var;	/* variable table at time of subshell */
	Hashtab_t	*svar;	/* save shell variable table */
	Hashtab_t	*sfun;	/* function scope for subshell */
	Hashtab_t	*salias;	/* alias scope for subshell */
	struct errorcontext *errcontext;
	Shopt_t		options;/* save shell options */
	pid_t		subpid;	/* child process id */
	Sfio_t*	saveout;/*saved standard output */
	char		*pwd;	/* present working directory */
	char		*shpwd;	/* saved pointer to sh.pwd */
	int		mask;	/* present umask */
	short		tmpfd;	/* saved tmp file descriptor */
	char		jobcontrol;
	char		monitor;
	unsigned char	fdstatus;
} *subshell_data;

static int subenv;

/*
 * This routine will turn the sftmp() file into a real /tmp file
 */
void	sh_subtmpfile(void)
{
	if(sfset(sfstdout,0,0)&SF_STRING)
	{
		register int fd;
		register struct checkpt	*pp = (struct checkpt*)sh.jmplist;
		register struct subshell *sp = subshell_data->pipe;
		/* save file descriptor 1 if open */
		if((sp->tmpfd = fd = fcntl(1,F_DUPFD,10)) >= 0)
		{
			fcntl(fd,F_SETFD,FD_CLOEXEC);
			sh.fdstatus[fd] = sh.fdstatus[1]|IOCLEX;
			close(1);
		}
		/* popping a discipline forces a /tmp file create */
		sfdisc(sfstdout,SF_POPDISC);
		sh.fdstatus[fd=sffileno(sfstdout)] = IOREAD|IOWRITE;
		sfsync(sfstdout);
		if(fd==1)
			fcntl(1,F_SETFD,0);
		else
		{
			sfsetfd(sfstdout,1);
			sh.fdstatus[1] = sh.fdstatus[fd];
			sh.fdstatus[fd] = IOCLOSE;
		}
		sh_iostream(1);
		sfset(sfstdout,SF_SHARE|SF_PUBLIC,1);
		sfpool(sfstdout,sh.outpool,SF_WRITE);
		if(pp && pp->olist  && pp->olist->strm == sfstdout)
			pp->olist->strm = 0;
	}
}

/*
 * This routine creates a temp file if necessary and creates a subshell.
 * The parent routine longjmps back to sh_subshell()
 * The child continues possibly with its standard output replaced by temp file
 */
void sh_subfork(void)
{
	register struct subshell *sp = subshell_data;
	pid_t pid;
	/* see whether inside $(...) */
	if(sp->pipe)
		sh_subtmpfile();
	if(pid = sh_fork(0,NIL(int*)))
	{
		/* this is the parent part of the fork */
		if(sp->subpid==0)
			sp->subpid = pid;
		siglongjmp(*sh.jmplist,SH_JMPSUB);
	}
	else
	{
		/* this is the child part of the fork */
		/* setting subpid to 1 causes subshell to exit when reached */
		sh_onstate(SH_FORKED|SH_NOLOG);
		sh_offstate(SH_MONITOR);
		subshell_data = 0;
		sh.subshell = 0;
		sp->subpid=0;
	}
}

/*
 * This routine will make a copy of the given node in the
 * layer created by the most recent subshell_fork if the
 * node hasn't already been copied
 */
Namval_t *sh_assignok(register Namval_t *np,int add)
{
	register Namval_t *mp;
	if(nv_isnull(np) && !add)	/* don't bother with this */
		return(np);
	/* don't bother to save if in newer scope */
	if(nv_search((char*)np,subshell_data->var,HASH_BUCKET)!=np)
		return(np);
	mp = nv_search((char*)np,subshell_data->svar,NV_ADD|HASH_BUCKET);
	if(mp->nvflag || mp->nvalue.cp)		/* see if already saved */
		return(np);
	if(nv_isnull(np))
	{
		/* mark so that it can be restored */
		nv_onattr(mp,NV_NOFREE);
		return(np);
	}
	nv_setsize(mp,nv_size(np));
	mp->nvenv = np->nvenv;
	mp->nvfun = np->nvfun;
	mp->nvalue.cp = np->nvalue.cp;
	mp->nvflag = np->nvflag;
	nv_onattr(np,NV_NOFREE);
	return(np);
}

/*
 * restore the variables
 */
static void nv_restore(struct subshell *sp)
{
	register Namval_t *mp, *np;
	register Hashpos_t *pos = hashscan(sp->svar,0);
	while(np=(Namval_t*)hashnext(pos))
	{
		if(mp = nv_search((char*)np,sp->var,HASH_BUCKET))
		{
			nv_unset(mp);
			nv_setsize(mp,nv_size(np));
			mp->nvenv = np->nvenv;
			mp->nvfun = np->nvfun;
			mp->nvalue.cp = np->nvalue.cp;
			mp->nvflag = np->nvflag;
		}
		np->nvalue.cp = 0;
		np->nvflag = NV_DEFAULT;
	}
	hashdone(pos);
	hashfree(sp->svar);
}

/*
 * return pointer to function tree
 * create a new one if necessary
 */
Hashtab_t *sh_subfuntree(void)
{
	register struct subshell *sp = subshell_data;
	if(!sp)
		return(sh.fun_tree);
	if(!sp->sfun)
		sp->sfun = sh.fun_tree = hashalloc(sh.fun_tree,HASH_set,HASH_SCOPE|HASH_ALLOCATE, 0);
	return(sp->sfun);
}

/*
 * return pointer to alias tree
 * create a new one if necessary
 */
Hashtab_t *sh_subaliastree(void)
{
	register struct subshell *sp = subshell_data;
	if(!sp)
		return(sh.alias_tree);
	if(!sp->salias)
		sp->salias = sh.alias_tree = hashalloc(sh.alias_tree,HASH_set,HASH_SCOPE|HASH_ALLOCATE, 0);
	return(sp->salias);
}

/*
 * Run command tree <t> in a virtual sub-shell
 * If comsub is not null, then output will be placed in temp file (or buffer)
 * If comsub is not null, the return value will be a stream consisting of
 * output of command <t>.  Otherwise, NULL will be returned.
 */

Sfio_t *sh_subshell(union anynode *t, int flags, int comsub)
{
	struct subshell sub_data;
	register struct subshell *sp = &sub_data;
	int jmpval,nsig;
	int savecurenv = sh.curenv;
	char *savstak;
	Sfio_t *iop=0;
	struct checkpt buff;
	struct sh_scoped savst;
	struct dolnod   *argsav=0;
	sfsync(sh.outpool);
	argsav = sh_arguse();
	sh.curenv = ++subenv;
	savst = sh.st;
	sh_pushcontext(&buff,SH_JMPSUB);
	sh.subshell++;
	sp->prev = subshell_data;
	sp->subpid = 0;
	sp->errcontext = &buff.err;
	sp->var = sh.var_tree;
	sp->options = sh.options;
	sp->pipe = 0;
	sp->sfun = 0;
	sp->salias = 0;
	if(!sh.pwd)
		path_pwd(0);
	sp->shpwd = sh.pwd;
	sp->pwd = (sh.pwd?strdup(sh.pwd):0);
	umask(sp->mask=umask(0));
	subshell_data = sp;
	/* save trap table */
	sh.st.otrapcom = 0;
	if((nsig=sh.st.trapmax*sizeof(char*))>0 || sh.st.trapcom[0])
	{
		nsig += sizeof(char*);
		memcpy(savstak=stakalloc(nsig),(char*)&sh.st.trapcom[0],nsig);
		/* this nonsense needed for $(trap) */
		sh.st.otrapcom = (char**)savstak;
	}
	sh_sigreset(0);
	sp->svar = hashalloc(sh.var_tree,HASH_set,HASH_ALLOCATE,0);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval==0)
	{
		if(comsub)
		{
			/* disable job control */
			sp->jobcontrol = job.jobcontrol;
			sp->monitor = (sh_isstate(SH_MONITOR)!=0);
			job.jobcontrol=0;
			sh_offstate(SH_MONITOR);
			sp->pipe = sp;
			/* save sfstdout and status */
			sp->saveout = sfswap(sfstdout,NIL(Sfio_t*));
			sp->fdstatus = sh.fdstatus[1];
			sp->tmpfd = -1;
			/* use sftmp() file for standard output */
			iop = sftmp(IOBSIZE+1);
			sfswap(iop,sfstdout);
			sh.fdstatus[1] = IOWRITE;
		}
		else if(sp->prev)
		{
			sp->pipe = sp->prev->pipe;
			flags &= ~SH_NOFORK;
		}
		sh_exec(t,flags);
		if(sh.pwd != sp->shpwd)
			sp->shpwd = sh.pwd;

	}
	if(jmpval!=SH_JMPSUB && sh.st.trapcom[0] && sh.subshell)
	{
		/* trap on EXIT not handled by child */
		char *trap=sh.st.trapcom[0];
		sh.st.trapcom[0] = 0;	/* prevent recursion */
		sh.oldexit = sh.exitval;
		sh_trap(trap,0);
		free(trap);
	}
	sh_popcontext(&buff);
	if(sh.subshell==0)	/* must be child process */
	{
		if(jmpval==SH_JMPSCRIPT)
			siglongjmp(*sh.jmplist,jmpval);
		sh_done(0);
	}
	if(comsub)
	{
		/* re-enable job control */
		job.jobcontrol = sp->jobcontrol;
		if(sp->monitor)
			sh_onstate(SH_MONITOR);
		/* move tmp file to iop and restore sfstdout */
		iop = sfswap(sfstdout,NIL(Sfio_t*));
		if(sffileno(iop)==1)
		{
			int fd=sfsetfd(iop,3);
			if(fd<0)
				error(ERROR_system(1),gettxt(e_toomany_id,e_toomany));
			sh.sftable[fd] = iop;
			fcntl(fd,F_SETFD,FD_CLOEXEC);
			sh.fdstatus[fd] = (sh.fdstatus[1]|IOCLEX);
			sh.fdstatus[1] = IOCLOSE;
		}
		sfswap(sp->saveout,sfstdout);
		/*  check if standard output was preserved */
		if(sp->tmpfd>=0)
		{
			close(1);
			fcntl(sp->tmpfd,F_DUPFD,1);
			sh_close(sp->tmpfd);
		}
		sh.fdstatus[1] = sp->fdstatus;
	}
	if(sp->subpid)
		job_wait(sp->subpid);
	if(comsub && iop)
		sfseek(iop,(off_t)0,SEEK_SET);
	if(sh.subshell)
		sh.subshell--;
	sh.options = sp->options;
	nv_restore(sp);
	if(sp->sfun)
		sh.fun_tree = hashfree(sp->sfun);
	if(sp->salias)
		sh.alias_tree = hashfree(sp->salias);
	sh_argfree(argsav,0);
	sh_sigreset(1);
	sh.st = savst;
	sh.jobenv = sh.curenv = savecurenv;
	if(nsig)
		memcpy((char*)&sh.st.trapcom[0],savstak,nsig);
	sh.trapnote = 0;
	if(nsig)
		stakset(savstak,0);
	sh.options = sp->options;
	subshell_data = sp->prev;
	if(!sh.pwd || strcmp(sp->pwd,sh.pwd))
	{
		/* restore PWDNOD */
		Namval_t *pwdnod = nv_scoped(PWDNOD);
		if(sh.pwd)
			chdir(sh.pwd=sp->pwd);
		if(nv_isattr(pwdnod,NV_NOFREE))
			pwdnod->nvalue.cp = (const char*)sp->pwd;
	}
	else if(sp->shpwd != sh.pwd)
	{
		sh.pwd = sp->pwd;
		if(PWDNOD->nvalue.cp==sp->shpwd)
			PWDNOD->nvalue.cp = sp->pwd;
	}
	else {
		if (strcmp(PWDNOD->nvalue.cp,sp->pwd)) {
			Namval_t *pwdnod = nv_scoped(PWDNOD);
			if(sh.pwd)
				chdir(sh.pwd=sp->pwd);
			if(nv_isattr(pwdnod,NV_NOFREE))
				pwdnod->nvalue.cp = (const char*)sp->pwd;
		} else
			free((void*)sp->pwd);
	}
	umask(sp->mask);
	if(sh.topfd != buff.topfd)
		sh_iorestore(buff.topfd);
	if(sh.exitval > SH_EXITSIG)
		sh_fault(sh.exitval&SH_EXITMASK);
	return(iop);
}
