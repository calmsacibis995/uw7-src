#ident	"@(#)ksh93:src/cmd/ksh93/sh/xec.c	1.2.1.1"
#pragma prototyped
/*
 * UNIX shell parse tree executer
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
#include	"variables.h"
#include	"path.h"
#include	"io.h"
#include	"shnodes.h"
#include	"jobs.h"
#include	"test.h"
#include	"builtins.h"
#include	"FEATURE/time"
#include	"FEATURE/externs"
#include	"FEATURE/locale"

#ifdef _lib_nice
    extern int	nice(int);
#endif /* _lib_nice */
#ifndef _lib_spawnveg
#   define spawnveg(a,b,c,d)	spawnve(a,b,c)
#endif /* !_lib_spawnveg */
#if 0
#undef _lib_fork
#endif
#ifndef _lib_fork
#   define SHOPT_SPAWN	1
#endif /* !_lib_fork */
#ifdef SHOPT_SPAWN
    static pid_t sh_ntfork(const union anynode*,char*[],int*);
#endif /* SHOPT_SPAWN */

static void	sh_funct(Namval_t*, int, char*[], struct argnod*,int);
static int	trim_eq(const char*, const char*);
static char	*word_trim(struct argnod*, int);
static void	p_time(Sfio_t*, clock_t, int);
static void	coproc_init(int pipes[]);

static void	*timeout;
static char	pipejob;

struct funenv
{
	Namval_t	*node;
	struct argnod	*env;
};

/* ========	command execution	========*/

/*
 * Given stream <iop> compile and execute
 */
int sh_eval(register Sfio_t *iop, int mode)
{
	register union anynode *t;
	struct slnod *saveslp = sh.st.staklist;
	int jmpval;
	struct checkpt *pp = (struct checkpt*)sh.jmplist;
	struct checkpt buff;
	static Sfio_t *io_save;
	io_save = iop; /* preserve correct value across longjmp */
	sh_pushcontext(&buff,SH_JMPEVAL);
	buff.olist = pp->olist;
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval==0)
	{
		t = (union anynode*)sh_parse(iop,SH_NL);
		sfclose(iop);
		io_save = 0;
		if(!sh_isoption(SH_VERBOSE))
			sh_offstate(SH_VERBOSE);
		if(mode && sh.hist_ptr)
			hist_flush(sh.hist_ptr);
		sh_exec(t,sh_isstate(SH_ERREXIT));
	}
	sh_popcontext(&buff);
	if(io_save)
		sfclose(io_save);
	sh_freeup();
	sh.st.staklist = saveslp;
	if(jmpval>SH_JMPEVAL)
		siglongjmp(*sh.jmplist,jmpval);
	return(sh.exitval);
}

#ifdef SHOPT_FASTPIPE
static int pipe_exec(int pv[], union anynode *t, int errorflg)
{
	struct checkpt buff;
	register union anynode *tchild = t->fork.forktre;
	Namval_t *np;
	Sfile_t *iop;
	int jmpval,r;
	if((tchild->tre.tretyp&COMMSK)!=TCOM || !(np=(Namval_t*)(tchild->com.comnamp)))
	{
		sh_pipe(pv);
		return(sh_exec(t,errorflg));
	}
	pv[0] = sh.lim.open_max;
	sh.fdstatus[pv[0]] = IOREAD|IODUP|IOSEEK;
	pv[1] = sh.lim.open_max+1;
	sh.fdstatus[pv[1]] = IOWRITE|IOSEEK;
	iop = sftmp(IOBSIZE+1);
	sh.sftable[sh.lim.open_max+1] = iop;
	sh_pushcontext(&buff,SH_JMPIO);
	if(t->tre.tretyp&FPIN)
		sh_iosave(0,sh.topfd);
	sh_iosave(1,sh.topfd);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval==0)
	{
		if(t->tre.tretyp&FPIN)
			sh_iorenumber(sh.inpipe[0],0);
		sh_iorenumber(sh.lim.open_max+1,1);
		r = sh_exec(tchild,errorflg);
		if(sffileno(sfstdout)>=0)
			pv[0] = sfsetfd(sfstdout,10);
		iop = sfswap(sfstdout,0);
	}
	sh_popcontext(&buff);
	sh.sftable[pv[0]] = iop;
	sh.fdstatus[pv[0]] = IOREAD|IODUP|IOSEEK;
	sfset(iop,SF_WRITE,0);
	sfseek(iop,0L,SEEK_SET);
	sh_iorestore(buff.topfd);
	if(jmpval>SH_JMPIO)
		siglongjmp(*sh.jmplist,jmpval);
	return(r);
}
#endif /* SHOPT_FASTPIPE */

sh_exec(register const union anynode *t, int flags)
{
	sh_sigcheck();
	if(t && !sh.st.execbrk && !sh_isoption(SH_NOEXEC))
	{
		register int 	type = flags;
		register char	*com0 = 0;
		int 		errorflg = (type&SH_ERREXIT);
		int 		execflg = (type&SH_NOFORK);
		int 		mainloop = (type&SH_INTERACTIVE);
		int		topfd = sh.topfd;
		char 		*sav=stakptr(0);
		char		**com=0;
		int		argn;
		int 		skipexitset = 0;
		int		save_states= sh_isstate(SH_ERREXIT|SH_MONITOR);
		if(flags&SH_INTERACTIVE)
		{
			pipejob = 0;
			job.curpgid = 0;
			flags &= ~SH_INTERACTIVE;
		}
		sh_offstate(SH_ERREXIT|SH_DEFPATH);
		sh_onstate(save_states&flags);
		type = t->tre.tretyp;
		if(!sh.intrap)
			sh.oldexit=sh.exitval;
		sh.exitval=0;
		sh.lastsig = 0;
		sh.lastpath = 0;
		switch(type&COMMSK)
		{
		    case TCOM:
		    {
			register struct argnod	*argp;
			char		*trap;
			Namval_t	*np;
			struct ionod	*io;
			int		command=0;
			error_info.line = t->com.comline-sh.st.firstline;
			com = sh_argbuild(&argn,&(t->com));
			if(t->tre.tretyp&COMSCAN)
			{
				argp = t->com.comarg;
				if(argp && *com && !(argp->argflag&ARG_RAW))
					sh_sigcheck();
			}
			com0 = com[0];
			if(!(np=(Namval_t*)(t->com.comnamp)) && com0)
			{
				register char *cp;
				Namval_t *nq=0;
				/* check for reference to discipline function */
				if((cp=strchr(com0,'.')) && cp!=com0) 
				{
					type = staktell();
					stakwrite(com0,cp-com0);
					stakputc(0);
					stakseek(type);
					if((nq=nv_search(stakptr(type),sh.var_tree,0)) && nv_isattr(nq,NV_REF))
					{
						while(nv_isattr(nq,NV_REF))
							nq=nq->nvalue.np;
						type = staktell();
						stakputs(nv_name(nq));
						stakputs(cp);
						stakseek(type);
						np = nv_search(stakptr(type),sh.fun_tree,0);
					}
					else
						np=0;
				}
				if(!np)
				{
					if(!(np=nv_search(com0,sh.fun_tree,0)))
						np=nv_search(com0,sh.bltin_tree,0);
				}
#ifdef SHOPT_OO
				if(!np && cp)
				{
					cp = strrchr(com0,'.');
					if(!nq)
					{
						*cp=0;
						nq = nv_search(com0,sh.var_tree,0);
						*cp = '.';
					}
					while(1)
					{
						if(!nq || !(nq=nv_class(nq)))
							break;
						stakputs(nv_name(nq));
						stakputs(cp);
						stakseek(type);
						if(np=nv_search(stakptr(type),sh.fun_tree,0))
							break;
					}
				}
#endif /* SHOPT_OO */
			}
			while(np==SYSCOMMAND)
			{
				register int n = b_command(0,com,0);
				if(n==0)
					break;
				command += n;
				np = 0;
				if(!(com0= *(com+=n)))
					break;
				np = nv_search(com0,sh.fun_tree,0);
			}
			argn -= command;
			io = t->tre.treio;
			if(command && np && is_afunction(np))
				np = nv_search(com0,sh.bltin_tree,0);
			if(trap=sh.st.trap[SH_DEBUGTRAP])
			{
				sh.st.trap[SH_DEBUGTRAP] = 0;
				sh_trap(trap,0);
				sh.st.trap[SH_DEBUGTRAP] = trap;
			}
			if(sh.envlist = argp = t->com.comset)
			{
				if(argn==0 || (np && !command && nv_isattr(np,BLT_SPC)))
				{
					register int flags=NV_VARNAME|NV_ASSIGN;
					if(np==SYSTYPESET)
					{
						if(sh.fn_depth)
							flags |= NV_NOSCOPE;
					}
					else if(np)
						flags = NV_IDENT|NV_ASSIGN;
					nv_setlist(argp,flags);
					argp = NULL;
				}
			}
			if((io||argn))
			{
				static char *argv[1];
				if(argn==0)
				{
					/* fake 'true' built-in */
					argn=1;
					np = SYSTRUE;
					*argv = nv_name(np);
					com = argv;
				}
				/* set +x doesn't echo */
				else if((np!=SYSSET) && sh_isoption(SH_XTRACE))
					sh_trace(com-command,1);
				if(io)
					sfsync(sh.outpool);
				sh.lastpath = 0;
				if((!np || (np->nvenv && is_abuiltin(np))) && !strchr(com0,'/'))
				{
					if(path_search(com0,(np?np->nvenv:NIL(char*)),1))
						np=nv_search(com0,sh.fun_tree,0);
					else if(np && (!sh.lastpath || strcmp(np->nvenv,sh.lastpath)))
						np = 0;
				}
				/* check for builtins */
				if(np && is_abuiltin(np))
				{
					int scope=0, jmpval, save_prompt;
					struct checkpt buff;
					unsigned long edopts=0;
					if(np->nvenv)
					{
						/*
						 * disable editors for built-in
						 * versions of commands on PATH
						 */
						edopts = sh_isoption(SH_VI|SH_EMACS|SH_GMACS);
						sh_offoption(SH_VI|SH_EMACS|SH_GMACS);
					}
					sh_pushcontext(&buff,SH_JMPCMD);
					jmpval = sigsetjmp(buff.buff,1);
					if(jmpval == 0)
					{
						type= !(nv_isattr(np,BLT_ENV));
						errorpush(&buff.err,type?ERROR_SILENT:0);
						if(io)
						{
							struct openlist *item;
							if(np==SYSLOGIN)
								type=1;
							else if(np==SYSEXEC)
								type=1+!com[1];
							else
								type = (execflg && !sh.subshell);
							sh_redirect(io,type);
							for(item=buff.olist;item;item=item->next)
								item->strm=0;
						}
						if(!(nv_isattr(np,BLT_ENV)))
						{
							sh_onstate(SH_STOPOK);
							sfpool(sfstderr,NIL(Sfio_t*),SF_WRITE);
							save_prompt = sh.nextprompt;
							sh.nextprompt = 0;
						}
						if(argp)
						{
							scope++;
							nv_scope(argp);
						}
						opt_index = opt_char = 0;
						error_info.id = *com;
						sh.exitval = 0;
						sh.exitval = (*funptr(np))(argn,com,(void*)np->nvfun);
						if(!nv_isattr(np,BLT_EXIT) && sh.exitval!=SH_RUNPROG)
							sh.exitval &= SH_EXITMASK;
					}
					else
					{
						struct openlist *item;
						for(item=buff.olist;item;item=item->next)
						{
							if(item->strm)
							{
								sfclrlock(item->strm);
								sfclose(item->strm);
							}
						}
						/* failure on special built-ins fatal */
						if(jmpval<=SH_JMPCMD  && (!nv_isattr(np,BLT_SPC) || command))
							jmpval=0;
					}
					if(!(nv_isattr(np,BLT_ENV)))
					{
						sh_offstate(SH_STOPOK);
						sfpool(sfstderr,sh.outpool,SF_WRITE);
						sfpool(sfstdin,NIL(Sfio_t*),SF_WRITE);
						sh.nextprompt = save_prompt;
					}
					sh_popcontext(&buff);
					if(buff.olist)
					{
						/* free list */
						struct openlist *item,*next;
						for(item=buff.olist;item;item=next)
						{
							next = item->next;
							free((void*)item);
						}
					}
					if(edopts)
						sh_onoption(edopts);
					if(scope)
						nv_unscope();
					if(io)
						sh_iorestore(buff.topfd);
					if(jmpval)
						siglongjmp(*sh.jmplist,jmpval);
					if(sh.exitval >=0)
						goto setexit;
					np = 0;
					type=0;
				}
				/* check for functions */
				if(!command && np && nv_isattr(np,NV_FUNCTION))
				{
					int indx;
					register struct slnod *slp;
					if(!np->nvalue.ip)
					{
						path_search(com0,NIL(char*),0);
						if(!np->nvalue.ip)
							error(ERROR_exit(1),gettxt(e_found2_id,e_found2));
					}
					/* increase refcnt for unset */
					slp = (struct slnod*)np->nvenv;
					sh_funstaks(slp->slchild,1);
					staklink(slp->slptr);
					if(io)
						indx = sh_redirect(io,execflg);
					sh_funct(np,argn,com,t->com.comset,flags);
					if(io)
						sh_iorestore(indx);
					sh_funstaks(slp->slchild,-1);
					stakdelete(slp->slptr);
					goto setexit;
				}
			}
			else if(!io)
			{
			setexit:
				exitset();
				break;
			}
		    }
		    case TFORK:
		    {
			register pid_t parent;
			int no_fork,jobid;
			int pipes[2];
			no_fork = (execflg && !(type&(FAMP|FPOU)) &&
				!sh.subshell && !sh.st.trapcom[0] && 
				!sh.st.trap[SH_ERRTRAP] && sh.fn_depth==0);
			if(sh.subshell)
				sh_subtmpfile();
			if(sh_isstate(SH_PROFILE) || sh.dot_depth)
			{
				/* disable foreground job monitor */
				if(!(type&FAMP))
					sh_offstate(SH_MONITOR);
#ifdef SHOPT_DEVFD
				else if(!(type&FINT))
					sh_offstate(SH_MONITOR);
#endif /* SHOPT_DEVFD */
			}
			if(no_fork)
				job.parent=parent=0;
			else
			{
				if(type&FCOOP)
					coproc_init(pipes);
				nv_getval(RANDNOD);
#ifdef SHOPT_SPAWN
#   ifdef _lib_fork
				if(com)
					parent = sh_ntfork(t,com,&jobid);
				else
					parent = sh_fork(type,&jobid);
#   else
				if((parent = sh_ntfork(t,com,&jobid))<=0)
					break;
#   endif /* _lib_fork */
				if(parent<0)
					break;
#else
				parent = sh_fork(type,&jobid);
#endif /* SHOPT_SPAWN */
			}
			if(job.parent=parent)
			/* This is the parent branch of fork
			 * It may or may not wait for the child
			 */
			{
				if(type&FPCL)
					sh_close(sh.inpipe[0]);
				if(type&FCOOP)
					sh.bckpid = sh.cpid = parent;
				else if(type&FAMP)
					sh.bckpid = parent;
				if(!(type&(FAMP|FPOU)))
				{
					if(sh.topfd > topfd)
						sh_iorestore(topfd);
					job_wait(parent);
				}
				if(type&FAMP)
				{
					if(sh_isstate(SH_PROFILE|SH_INTERACTIVE))
					{
						/* print job number */
#ifdef JOBS
						sfprintf(sfstderr,"[%d]\t%d\n",jobid,parent);
#else
						sfprintf(sfstderr,"%d\n",parent);
#endif /* JOBS */
					}
				}
				break;
			}
			else
			/*
			 * this is the FORKED branch (child) of execute
			 */
			{
				int jmpval;
				struct checkpt buff;
				if(no_fork)
					sh_sigreset(2);
				sh_pushcontext(&buff,SH_JMPEXIT);
				jmpval = sigsetjmp(buff.buff,0);
				if(jmpval)
					goto done;
				if((type&FINT) && !sh_isstate(SH_MONITOR))
				{
					/* default std input for & */
					signal(SIGINT,SIG_IGN);
					signal(SIGQUIT,SIG_IGN);
					if(!sh.st.ioset)
					{
						if(sh_close(0)>=0)
							sh_chkopen(e_devnull);
					}
				}
				sh_offstate(SH_MONITOR);
				/* pipe in or out */
#ifdef _lib_nice
				if((type&FAMP) && sh_isoption(SH_BGNICE))
					nice(4);
#endif /* _lib_nice */
				if(type&FPIN)
				{
					sh_iorenumber(sh.inpipe[0],0);
					if(!(type&FPOU) || (type&FCOOP))
						sh_close(sh.inpipe[1]);
				}
				if(type&FPOU)
				{
					sh_iorenumber(sh.outpipe[1],1);
					sh_pclose(sh.outpipe);
				}
				if((type&COMMSK)!=TCOM)
					error_info.line = t->fork.forkline-sh.st.firstline;
				sh_redirect(t->tre.treio,1);
				if(sh.topfd)
					sh_iounsave();
				if((type&COMMSK)!=TCOM)
				{
					/* don't clear job table for out
					   pipes so that jobs comand can
					   be used in a pipeline
					 */
					if(!no_fork && !(type&FPOU))
						job_clear();
					sh_exec(t->fork.forktre,flags|SH_NOFORK);
				}
				else if(com0)
				{
					sh_offoption(SH_ERREXIT);
					sh_freeup();
					path_exec(com0,com,t->com.comset);
				}
			done:
				sh_popcontext(&buff);
				if(jmpval>SH_JMPEXIT)
					siglongjmp(*sh.jmplist,jmpval);
				sh_done(0);
			}
		    }

		    case TSETIO:
		    {
		    /*
		     * don't create a new process, just
		     * save and restore io-streams
		     */
			pid_t	pid;
			int jmpval, waitall;
			struct checkpt buff;
			if(sh.subshell)
			{
				flags &= ~SH_NOFORK;
				execflg = 0;
			}
			sh_pushcontext(&buff,SH_JMPIO);
			if(type&FPIN)
			{
				save_states |= sh_isstate(SH_INTERACTIVE);
				sh_offstate(SH_INTERACTIVE);
				if(!execflg)
					sh_iosave(0,sh.topfd);
				sh_iorenumber(sh.inpipe[0],0);
				/*
				 * if read end of pipe is a simple command
				 * treat as non-sharable to improve performance
				 */
				if((t->fork.forktre->tre.tretyp&COMMSK)==TCOM)
					sfset(sfstdin,SF_PUBLIC|SF_SHARE,0);
				waitall = job.waitall;
				job.waitall = 0;
				pid = job.parent;
			}
			else
				error_info.line = t->fork.forkline-sh.st.firstline;
			jmpval = sigsetjmp(buff.buff,0);
			if(jmpval==0)
			{
				sh_redirect(t->fork.forkio,execflg);
				sh_exec(t->fork.forktre,flags);
			}
			sh_popcontext(&buff);
			sh_iorestore(buff.topfd);
			if(type&FPIN)
			{
				job.waitall = waitall;
				type = sh.exitval;
				if(!(type&SH_EXITSIG))
				{
					/* wait for remainder of pipline */
					job_wait(waitall?pid:0);
					sh.exitval = type;
				}
				sh.st.ioset = 0;
			}
			if(jmpval>SH_JMPIO)
				siglongjmp(*sh.jmplist,jmpval);
			break;
		    }

		    case TPAR:
			if(!sh.subshell && (flags&SH_NOFORK))
			{
				int jmpval;
				struct checkpt buff;
				sh_pushcontext(&buff,SH_JMPEXIT);
				jmpval = sigsetjmp(buff.buff,0);
				if(jmpval==0)
					sh_exec(t->par.partre,flags);
				sh_popcontext(&buff);
				if(jmpval > SH_JMPEXIT)
					siglongjmp(*sh.jmplist,jmpval);
				sh_done(0);
			}
			else
				sh_subshell(t->par.partre,flags,0);
			break;

		    case TFIL:
		    {
		    /*
		     * This code sets up a pipe.
		     * All elements of the pipe are started by the parent.
		     * The last element executes in current environment
		     */
			int	pvo[2];	/* old pipe for multi-stage */
			int	pvn[2];	/* current set up pipe */
			int	savepipe = pipejob;
			pid_t	savepgid = job.curpgid;
			if(sh.subshell)
				sh_subtmpfile();
			sh.inpipe = pvo;
			sh.outpipe = pvn;
			pvo[1] = -1;
			job.waitall |= !pipejob && sh_isstate(SH_MONITOR);
			do
			{
#ifdef SHOPT_FASTPIPE
				type = pipe_exec(pvn,t->lst.lstlef, errorflg);
#else
				/* create the pipe */
				sh_pipe(pvn);
				/* execute out part of pipe no wait */
				type = sh_exec(t->lst.lstlef, errorflg);
#endif /* SHOPT_FASTPIPE */
				pipejob=1;
				/* save the pipe stream-ids */
				pvo[0] = pvn[0];
				/* close out-part of pipe */
				sh_close(pvn[1]);
				/* pipeline all in one process group */
				t = t->lst.lstrit;
			}
			/* repeat until end of pipeline */
			while(!type && t->tre.tretyp==TFIL);
			sh.inpipe = pvn;
			sh.outpipe = 0;
			if(type == 0)
				/*
				 * execute last element of pipeline
				 * in the current process
				 */
				sh_exec(t,flags);
			else
				/* execution failure, close pipe */
				sh_pclose(pvn);
			pipejob = savepipe;
			job.curpgid = savepgid;
			break;
		    }

		    case TLST:
		    {
			/*  a list of commands are executed here */
			do
			{
				sh_exec(t->lst.lstlef,errorflg);
				t = t->lst.lstrit;
			}
			while(t->tre.tretyp == TLST);
			sh_exec(t,flags);
			break;
		    }

		    case TAND:
			if(type&TTEST)
				skipexitset++;
			if(sh_exec(t->lst.lstlef,0)==0)
				sh_exec(t->lst.lstrit,flags);
			break;

		    case TORF:
			if(type&TTEST)
				skipexitset++;
			if(sh_exec(t->lst.lstlef,0)!=0)
				sh_exec(t->lst.lstrit,flags);
			break;

		    case TFOR: /* for and select */
		    {
			register char **args;
			register int nargs;
			register Namval_t *np;
			struct dolnod	*argsav=0;
			struct comnod	*tp;
			char *cp, *nullptr = 0;
			int nameref, refresh=1;
			if(!(tp=t->for_.forlst))
			{
				args=sh.st.dolv+1;
				nargs = sh.st.dolc;
				argsav=sh_arguse();
			}
			else
			{
				args=sh_argbuild(&argn,tp);
				nargs = argn;
			}
			np = nv_open(t->for_.fornam, sh.var_tree,NV_NOASSIGN|NV_ARRAY|NV_VARNAME|NV_REF);
			nameref = nv_isattr(np,NV_REF)!=0;
			sh.st.loopcnt++;
			cp = *args;
			while(cp && sh.st.execbrk==0)
			{
				if(t->tre.tretyp==TSELECT)
				{
					char *val;
					int save_prompt;
					/* reuse register */
					if(refresh)
					{
						sh_menu(sfstderr,nargs,args);
						refresh = 0;
					}
					save_prompt = sh.nextprompt;
					sh.nextprompt = 3;
					sh.timeout = 0;
					sh.exitval=sh_readline(&nullptr,0,1,1000*sh.st.tmout);
					sh.nextprompt = save_prompt;
					if(sh.exitval||sfeof(sfstdin)||sferror(sfstdin))
					{
						sh.exitval = 1;
						break;
					}
					if(!(val=nv_getval(nv_scoped(REPLYNOD))))
						continue;
					else
					{
						if(*(cp=val) == 0)
						{
							refresh++;
							goto check;
						}
						while(type = *cp++)
							if(type < '0' && type > '9')
								break;
						if(type!=0)
							type = nargs;
						else
							type = atoi(val)-1;
						if(type<0 || type >= nargs)
							cp = "";
						else
							cp = args[type];
					}
				}
				if(nameref)
					nv_offattr(np,NV_REF);
				else if(nv_isattr(np, NV_ARRAY))
					nv_putsub(np,NIL(char*),0L);
				nv_putval(np,cp,0);
				if(nameref)
					nv_setref(np);
				sh_exec(t->for_.fortre,errorflg);
				if(t->tre.tretyp == TSELECT)
				{
					if((cp=nv_getval(nv_scoped(REPLYNOD))) && *cp==0)
						refresh++;
				}
				else
					cp = *++args;
			check:
				if(sh.st.breakcnt<0)
					sh.st.execbrk = (++sh.st.breakcnt !=0);
			}
			if(sh.st.breakcnt>0)
				sh.st.execbrk = (--sh.st.breakcnt !=0);
			sh.st.loopcnt--;
			sh_argfree(argsav,0);
			nv_close(np);
			break;
		    }

		    case TWH: /* while and until */
		    {
			register int 	r=0;
			sh.st.loopcnt++;
			while(sh.st.execbrk==0 && (sh_exec(t->wh.whtre,0)==0)==(type==TWH))
			{
				r = sh_exec(t->wh.dotre,errorflg);
				if(sh.st.breakcnt<0)
					sh.st.execbrk = (++sh.st.breakcnt !=0);
				/* This is for the arithmetic for */
				if(sh.st.execbrk==0 && t->wh.whinc)
					sh_exec((union anynode*)t->wh.whinc,0);
			}
			if(sh.st.breakcnt>0)
				sh.st.execbrk = (--sh.st.breakcnt !=0);
			sh.st.loopcnt--;
			sh.exitval= r;
			break;
		    }
		    case TARITH: /* (( expression )) */
		    {
			register char *trap;
			static char *arg[4]=  {"((", 0, "))"};
			error_info.line = t->ar.arline-sh.st.firstline;
			if(trap=sh.st.trap[SH_DEBUGTRAP])
			{
				sh.st.trap[SH_DEBUGTRAP] = 0;
				sh_trap(trap,0);
				sh.st.trap[SH_DEBUGTRAP] = trap;
			}
			if(!(t->ar.arexpr->argflag&ARG_RAW))
				arg[1] = word_trim(t->ar.arexpr,3);
			else
				arg[1] = t->ar.arexpr->argval;
			if(sh_isoption(SH_XTRACE))
			{
				sh_trace(NIL(char**),0);
				sfprintf(sfstderr,"((%s))\n",arg[1]);
			}
			sh.exitval = !sh_arith(arg[1]);
			break;
		    }

		    case TIF:
			if(sh_exec(t->if_.iftre,0)==0)
				sh_exec(t->if_.thtre,flags);
			else if(t->if_.eltre)
				sh_exec(t->if_.eltre, flags);
			else
				sh.exitval=0; /* force zero exit for if-then-fi */
			break;

		    case TSW:
		    {
			char *r = word_trim(t->sw.swarg,0);
			t= (union anynode*)(t->sw.swlst);
			while(t)
			{
				register struct argnod	*rex=(struct argnod*)t->reg.regptr;
				while(rex)
				{
					register char *s;
					if(rex->argflag&ARG_MAC)
					{
						s = sh_mactrim(rex->argval,1);
						while(*s=='\\' && s[1]==0)
							s+=2;
					}
					else
						s = rex->argval;
					type = (rex->argflag&ARG_RAW);
					if((type && strcmp(r,s)==0) ||
						(!type && (strmatch(r,s)
						|| trim_eq(r,s))))
					{
						do	sh_exec(t->reg.regcom,flags);
						while(t->reg.regflag &&
							(t=(union anynode*)t->reg.regnxt));
						t=0;
						break;
					}
					else
						rex=rex->argnxt.ap;
				}
				if(t)
					t=(union anynode*)t->reg.regnxt;
			}
			break;
		    }

		    case TTIME:
		    {
			/* time the command */
			struct tms before,after;
			clock_t at, bt;
#ifdef _lib_gettimeofday
			struct timeval tb,ta;
#endif	/* _lib_gettimeofday */
			if(type!=TTIME)
			{
				sh_exec(t->par.partre,0);
				sh.exitval = !sh.exitval;
				break;
			}
			if(t->par.partre)
			{
				long timer_on;
				timer_on = sh_isstate(SH_TIMING);
#ifdef _lib_gettimeofday
				gettimeofday(&tb,NIL(struct timezone*));
#endif	/* _lib_gettimeofday */
				bt = times(&before);
				job.waitall = 1;
				sh_onstate(SH_TIMING);
				sh_exec(t->par.partre,0);
				if(!timer_on)
					sh_offstate(SH_TIMING);
				job.waitall = 0;
			}
			else
			{
				bt = 0;
				before.tms_utime = before.tms_cutime = 0;
				before.tms_stime = before.tms_cstime = 0;
			}
			at = times(&after);
#ifdef _lib_gettimeofday
			gettimeofday(&ta,NIL(struct timezone*));
			at = sh.lim.clk_tck*(ta.tv_sec-tb.tv_sec);
			at +=  ((sh.lim.clk_tck*(((1000000L/2)/sh.lim.clk_tck)+(ta.tv_usec-tb.tv_usec)))/1000000L);
#else
			at -= bt;
#endif	/* _lib_gettimeofday */
			if(t->par.partre)
			{
				sfputr(sfstderr,e_real,'\t');
				p_time(sfstderr,at,'\n');
			}
			sfputr(sfstderr,e_user,'\t');
			at = after.tms_utime - before.tms_utime;
			at += after.tms_cutime - before.tms_cutime;
			p_time(sfstderr,at,'\n');
			sfputr(sfstderr,e_sys,'\t');
			at = after.tms_stime - before.tms_stime;
			at += after.tms_cstime - before.tms_cstime;
			p_time(sfstderr,at,'\n');
			break;
		    }
		    case TFUN:
		    {
			register Namval_t *np;
			register struct slnod *slp;
			register char *fname = ((struct functnod*)t)->functnam;
			register char *cp;
			register Namval_t *npv=0;
			/* look for discipline functions */
			error_info.line = t->funct.functline-sh.st.firstline;
			if(cp = strrchr(fname,'.'))
			{
				int offset = staktell();
				stakwrite(fname,cp-fname);
				stakputc(0);
				npv = nv_open(stakptr(offset),sh.var_tree,NV_NOASSIGN|NV_ARRAY|NV_VARNAME|NV_NOSCOPE);
				offset = staktell();
				stakputs(nv_name(npv));
				stakputs(cp);
				stakputc(0);
				fname = stakptr(offset);
			}
			/* Function names cannot be special builtin */
			if((np=nv_search(fname,sh.bltin_tree,0)) && nv_isattr(np,BLT_SPC))
				error(ERROR_exit(1),gettxt(e_badfun_id,e_badfun),fname);
			np = nv_open(fname,sh_subfuntree(),NV_NOASSIGN|NV_ARRAY|NV_VARNAME|NV_NOSCOPE);
			if(np->nvalue.rp)
			{
				slp = (struct slnod*)np->nvenv;
				sh_funstaks(slp->slchild,-1);
				stakdelete(slp->slptr);
			}
			else
				np->nvalue.rp = new_of(struct Ufunction,0);
			if(t->funct.functstak)
			{
				slp = t->funct.functstak;
				sh_funstaks(slp->slchild,1);
				staklink(slp->slptr);
				np->nvenv = (char*)slp;
				nv_funtree(np) = (int*)(t->funct.functtre);
				np->nvalue.rp->hoffset = t->funct.functloc;
				np->nvalue.rp->lineno = t->funct.functline;
				nv_offattr(np,NV_FPOSIX);
			}
			else
				nv_unset(np);
			if(type&FPOSIX)
				nv_onattr(np,NV_FUNCTION|NV_FPOSIX);
			else
				nv_onattr(np,NV_FUNCTION);
			if(npv)
			{
				cp = nv_setdisc(npv,cp+1,np,(Namfun_t*)npv);
				nv_close(npv);
				if(!cp)
					error(ERROR_exit(1),gettxt(e_baddisc_id,e_baddisc),fname);
			}
			break;
		    }

		    /* new test compound command */
		    case TTST:
		    {
			register int n;
			register char *left;
			if(type&TTEST)
				skipexitset++;
			error_info.line = t->tst.tstline-sh.st.firstline;
			if((type&TPAREN)==TPAREN)
			{
				sh_exec(t->lst.lstlef,0);
				n = !sh.exitval;
			}
			else
			{
				register int traceon=0;
				register char *right;
				register char *trap;
				n = type>>TSHIFT;
				left = word_trim(&(t->lst.lstlef->arg),0);
				if(type&TBINARY)
					right = word_trim(&(t->lst.lstrit->arg),(n==TEST_PEQ||n==TEST_PNE));
				if(trap=sh.st.trap[SH_DEBUGTRAP])
				{
					sh.st.trap[SH_DEBUGTRAP] = 0;
					sh_trap(trap,0);
					sh.st.trap[SH_DEBUGTRAP] = trap;
				}
				if(sh_isoption(SH_XTRACE))
				{
					traceon = sh_trace(NIL(char**),0);
					sfwrite(sfstderr,e_tstbegin,(type&TNEGATE?5:3));
				}
				if(type&TUNARY)
				{
					if(traceon)
						sfprintf(sfstderr,"-%c %s",n,sh_fmtq(left));
					n = test_unop(n,left);
				}
				else if(type&TBINARY)
				{
					char *op;
					if(traceon)
						op = (char*)(shtab_testops+(n&037)-1)->sh_name;
					n = test_binop(n,left,right);
					if(traceon)
					{
						sfprintf(sfstderr,"%s %s ",sh_fmtq(left),op);
						type >>= TSHIFT;
						if(type==TEST_PEQ || type==TEST_PNE)
						{
							while(type= *right++)
							{
								switch(type)
								{
								    case '\n':
									sfputr(sfstderr,"$'\\n",'\'');
									continue;
								    case '\\':
									if(type= *right)
										right++;
								    case ' ':
								    case '<':
								    case '>':
								    case ';':
								    case '$':
								    case '`':
								    case '\t':
									sfputc(sfstderr,'\\');
								}
								sfputc(sfstderr,type);
							}
						}
						else
							sfputr(sfstderr,sh_fmtq(right),-1);
					}
				}
				if(traceon)
					sfwrite(sfstderr,e_tstend,4);
			}
			sh.exitval = ((!n)^((type&TNEGATE)!=0));
			break;
		    }
		}
		if(sh.trapnote || (sh.exitval && sh_isstate(SH_ERREXIT)) &&
			t && (t->tre.tretyp&COMMSK)==TCOM) 
			sh_chktrap();
		/* set $_ */
		if(mainloop && com0)
		{
			/* store last argument here if it fits */
			static char	lastarg[32];
			if(sh_isstate(SH_FORKED))
				sh_done(0);
			if(sh.lastarg!= lastarg && sh.lastarg)
				free(sh.lastarg);
			if(strlen(com[argn-1]) < sizeof(lastarg))
			{
				nv_onattr(L_ARGNOD,NV_NOFREE);
				sh.lastarg = strcpy(lastarg,com[argn-1]);
			}
			else
			{
				nv_offattr(L_ARGNOD,NV_NOFREE);
				sh.lastarg = strdup(com[argn-1]);
			}
		}
		if(!skipexitset)
			exitset();
		if(sav != stakptr(0))
			stakset(sav,0);
		else if(staktell())
			stakseek(0);
		if(sh.trapnote&SH_SIGSET)
			sh_exit(SH_EXITSIG|sh.lastsig);
		sh_onstate(save_states);
	}
	return(sh.exitval);
}

/*
 * test for equality with second argument trimmed
 * returns 1 if r == trim(s) otherwise 0
 */

static trim_eq(register const char *r,register const char *s)
{
	register char c;
	while(c = *s++)
	{
		if(c=='\\')
			c = *s++;
		if(c && c != *r++)
			return(0);
	}
	return(*r==0);
}

/*
 * print out the command line if set -x is on
 */

int sh_trace(register char *argv[], register int nl)
{
	register char *cp;
	register int bracket = 0;
	if(sh_isoption(SH_XTRACE))
	{
		/* make this trace atomic */
		sfset(sfstderr,SF_SHARE|SF_PUBLIC,0);
		if(!(cp=nv_getval(nv_scoped(PS4NOD))))
			cp = "+ ";
		else
		{
			sh_offoption(SH_XTRACE);
			cp = sh_mactry(cp);
			sh_onoption(SH_XTRACE);
		}
		if(*cp)
			sfputr(sfstderr,cp,-1);
		if(argv)
		{
			char *argv0 = *argv;
			nl = (nl?'\n':-1);
			/* don't quote [ and [[ */
			if(*(cp=argv[0])=='[' && (!cp[1] || !cp[2]&&cp[1]=='['))  
			{
				sfputr(sfstderr,cp,*++argv?' ':nl);
				bracket = 1;
			}
			while(cp = *argv++)
			{
				if(bracket==0 || *argv || *cp!=']')
					cp = sh_fmtq(cp);
				if(sh.prefix && cp!=argv0 && *cp!='-')
					sfputr(sfstderr,sh.prefix,'.');
				sfputr(sfstderr,cp,*argv?' ':nl);
			}
			sfset(sfstderr,SF_SHARE|SF_PUBLIC,1);
		}
		return(1);
	}
	return(0);
}


static char *word_trim(register struct argnod *arg, int flag)
{
	register char *sp = arg->argval;
	if((arg->argflag&ARG_RAW))
		return(sp);
	return(sh_mactrim(sp,flag));
}


/*
 * This routine creates a subshell by calling fork() or vfork()
 * If ((flags&COMASK)==TCOM), then vfork() is permitted
 * If fork fails, the shell sleeps for exponentially longer periods
 *   and tries again until a limit is reached.
 * SH_FORKLIM is the max period between forks - power of 2 usually.
 * Currently shell tries after 2,4,8,16, and 32 seconds and then quits
 * Failures cause the routine to error exit.
 * Parent links to here-documents are removed by the child
 * Traps are reset by the child
 * The process-id of the child is returned to the parent, 0 to the child.
 */

static void timed_out(void *handle)
{
	NOT_USED(handle);
	timeout = 0;
}


/*
 * called by parent and child after fork by sh_fork()
 */
pid_t _sh_fork(register pid_t parent,int flags,int *jobid)
{
	register struct checkpt *pp = (struct checkpt*)sh.jmplist;
	static long forkcnt = 1000L;
	pid_t	curpgid = job.curpgid;
	pid_t	postid = (flags&FAMP)?0:curpgid;
	if(parent<0)
	{
		if((forkcnt *= 2) > 1000L*SH_FORKLIM)
		{
			forkcnt=1000L;
			error(ERROR_system(ERROR_NOEXEC),gettxt(e_nofork_id,e_nofork));
		}
		sh_sigcheck();
		timeout = (void*)timeradd(forkcnt, 0, timed_out, NIL(void*));
		job_wait((pid_t)-1);
		if(timeout)
		{
			timerdel(timeout);
			forkcnt /= 2;
		}
		return(-1);
	}
	forkcnt=1000L;
	if(parent)
	{
		int myjob;
		sh.nforks++;
		if(job.toclear)
			job_clear();
#ifdef JOBS
		/* first process defines process group */
		if(sh_isstate(SH_MONITOR))
		{
			/*
			 * errno==EPERM means that an earlier processes
			 * completed.  Make parent the job group id.
			 */
			if(postid==0)
				job.curpgid = parent;
			if(job.jobcontrol || (flags&FAMP))
			{
				if(setpgid(parent,job.curpgid)<0 && errno==EPERM)
					setpgid(parent,parent);
			}
		}
#endif /* JOBS */
		myjob = job_post(parent,postid);
		if(flags&FAMP)
			job.curpgid = curpgid;
		if(jobid)
			*jobid = myjob;
		return(parent);
	}
	/* This is the child process */
	if (sh.trapnote & SH_SIGTERM) 
		sh_exit(SH_EXITSIG|SIGTERM);
	sh.nforks=0;
	timerdel(NIL(void*));
#ifdef JOBS
	if(!job.jobcontrol && !(flags&FAMP))
		sh_offstate(SH_MONITOR);
	if(sh_isstate(SH_MONITOR))
	{
		parent = getpid();
		if(postid==0)
			job.curpgid = parent;
		while(setpgid(0,job.curpgid)<0 && job.curpgid!=parent)
			job.curpgid = parent;
#   ifdef SIGTSTP
		if(job.curpgid==parent &&  !(flags&FAMP))
			tcsetpgrp(job.fd,job.curpgid);
#   endif /* SIGTSTP */
	}
#   ifdef SIGTSTP
	if(job.jobcontrol)
	{
		signal(SIGTTIN,SIG_DFL);
		signal(SIGTTOU,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
	}
#   endif /* SIGTSTP */
	job.jobcontrol = 0;
#endif /* JOBS */
	job.toclear = 1;
	sh.login_sh = 0;
	sh_onstate(SH_FORKED|SH_NOLOG);
	sh.fn_depth = 0;
	job.waitsafe = 0;
	job.in_critical = 0;
#ifdef SHOPT_ACCT
	sh_accsusp();
#endif	/* SHOPT_ACCT */
	/* Reset remaining signals to parent */
	/* except for those `lost' by trap   */
	sh_sigreset(2);
	sh.subshell = 0;
	return(0);
}

pid_t sh_fork(int flags, int *jobid)
{
	register pid_t parent;
#ifdef SHOPT_FASTPIPE
	if(sffileno(sfstdin)<0)
	{
		off_t current = sfseek(sfstdin,(off_t)0,SEEK_CUR);
		sfseek(sfstdin,(off_t)0,SEEK_END);
		sfdisc(sfstdin,SF_POPDISC);
		fcntl(sffileno(sfstdin),F_SETFD,0);
		sh_iostream(0);
		sfseek(sfstdin,current,SEEK_SET);
	}
#endif /* SHOPT_FASTPIPE */
	sfsync(NIL(Sfio_t*));
	sh.trapnote &= ~SH_SIGTERM;
	job.in_critical = 1;
	while(_sh_fork(parent=fork(),flags,jobid) < 0);
	return(parent);
}

/*
 * This routine is used to execute the given function <fun> in a new scope
 * If <fun> is NULL, then arg points to a structure containing a pointer
 *  to a function that will be executed in the current environment.
 */
int sh_funscope(int argn, char *argv[],int(*fun)(void*),void *arg,int execflg)
{
	register char	*trap;
	register int	nsig;
	struct dolnod	*argsav=0,*saveargfor;
	struct sh_scoped savst, *prevscope = sh.st.self;
	struct argnod	*envlist=0;
	Shopt_t		savopt;
	int		jmpval;
	int		r = 0;
	char 		*savstak;
	struct funenv	*fp;
	struct checkpt	buff;
	savopt = sh.options;
	sh_offoption(SH_ERREXIT);
	sh_offstate(SH_ERREXIT);
	*prevscope = sh.st;
	sh.st.prevst = prevscope;
	sh.st.self = &savst;
	sh.topscope = (Shscope_t*)sh.st.self;
	sh.st.opterror = sh.st.optchar = 0;
	sh.st.optindex = 1;
	sh.st.loopcnt = 0;
	if(!fun)
	{
		fp = (struct funenv*)arg;
		envlist = fp->env;
		sh.st.firstline = (fp->node)->nvalue.rp->lineno;
	}
	prevscope->save_tree = sh.var_tree;
#ifndef _lib_hashview
	if(hashscope(sh.var_tree))
		sh.var_tree = hashscope(sh.var_tree);
#endif /* _lib_hashview */
	nv_scope(envlist);
#ifdef _lib_hashview
	if(hashscope(prevscope->save_tree))
	{
		/* eliminate parent scope */
		hashview(sh.var_tree,(Hashtab_t*)0);
		hashview(sh.var_tree,hashscope(prevscope->save_tree));
	}
#endif /* _lib_hashview */
	sh.st.save_tree = sh.var_tree;
	if(!fun)
	{
		if(nv_isattr(fp->node,NV_TAGGED))
			sh_onoption(SH_XTRACE);
		else
			sh_offoption(SH_XTRACE);
	}
	sh.st.cmdname = argv[0];
	/* save trap table */
	if((nsig=sh.st.trapmax*sizeof(char*))>0 || sh.st.trapcom[0])
	{
		nsig += sizeof(char*);
		memcpy(savstak=stakalloc(nsig),(char*)&sh.st.trapcom[0],nsig);
	}
	sh_sigreset(0);
	argsav = sh_argnew(argv,&saveargfor);
	sh_pushcontext(&buff,SH_JMPFUN);
	errorpush(&buff.err,0);
	error_info.id = argv[0];
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		if(sh.fn_depth++ > MAXDEPTH)
			siglongjmp(*sh.jmplist,SH_JMPERRFN);
		else if(fun)
			r= (*fun)(arg);
		else
		{
			sh_exec((union anynode*)(nv_funtree((fp->node))),execflg|SH_ERREXIT);
			r = sh.exitval;
		}
	}
	if(--sh.fn_depth==1 && jmpval==SH_JMPERRFN)
		error(ERROR_exit(1),gettxt(e_toodeep_id,e_toodeep),argv[0]);
	sh_popcontext(&buff);
	if (sh.st.self != &savst)
		sh.var_tree = (Hashtab_t*)savst.save_tree;
	nv_unscope();
	sh.var_tree = (Hashtab_t*)prevscope->save_tree;
	sh_argreset(argsav,saveargfor);
	trap = sh.st.trapcom[0];
	sh.st.trapcom[0] = 0;
	sh_sigreset(1);
	if (sh.st.self != &savst)
		*sh.st.self = sh.st;
	sh.st = *prevscope;
	sh.topscope = (Shscope_t*)prevscope;
	nv_getval(nv_scoped(IFSNOD));
	if(nsig)
		memcpy((char*)&sh.st.trapcom[0],savstak,nsig);
	sh.trapnote=0;
	if(nsig)
		stakset(savstak,0);
	sh.options = savopt;
	if(trap)
	{
		sh_trap(trap,0);
		free(trap);
	}
	if(sh.exitval > SH_EXITSIG)
		sh_fault(sh.exitval&SH_EXITMASK);
	if(jmpval > SH_JMPFUN)
	{
		sh_chktrap();
		siglongjmp(*sh.jmplist,jmpval);
	}
	return(r);
}

static void sh_funct(Namval_t *np,int argn, char *argv[],struct argnod *envlist,int execflg)
{
	struct funenv fun;
	if(nv_isattr(np,NV_FPOSIX))
	{
		sh.posix_fun = np;
		opt_index = opt_char = 0;
		error_info.errors = 0;
		b_dot_cmd(argn+1,argv-1,0);
		return;
	}
	fun.env = envlist;
	fun.node = np;
	sh_funscope(argn,argv,0,&fun,execflg);
}

/*
 * external interface to execute a function without arguments
 * <np> is the function node
 * If <nq> is not-null, then sh.name and sh.subscript will be set
 */
int sh_fun(Namval_t *np, Namval_t *nq)
{
	char *argv[2];
	register Namarr_t *ap;
	register int offset;
	register char *base;
	Fcin_t save;
	fcsave(&save);
	if((offset=staktell())>0)
		base=stakfreeze(0);
	argv[0] = nv_name(np);
	argv[1]=0;
	if(nq)
	{
		nv_putval(SH_NAMENOD, nv_name(nq), NV_NOFREE);
		if(ap = nv_arrayptr(nq))
			nv_putval(SH_SUBSCRNOD,nv_getsub(nq),NV_NOFREE);
	}
	if(is_abuiltin(np))
	{
		int jmpval;
		struct checkpt buff;
		sh_pushcontext(&buff,SH_JMPCMD);
		jmpval = sigsetjmp(buff.buff,1);
		if(jmpval == 0)
		{
			errorpush(&buff.err,0);
			error_info.id = argv[0];
			opt_index = opt_char = 0;
			sh.exitval = 0;
			sh.exitval = (*funptr(np))(1,argv,(void*)np->nvfun);
		}
		sh_popcontext(&buff);
		if(jmpval>SH_JMPCMD)
			siglongjmp(*sh.jmplist,jmpval);
	}
	else
		sh_funct(np,1,argv,(struct argnod*)0,sh_isstate(SH_ERREXIT));
	if(nq)
	{
		nv_unset(SH_NAMENOD);
		nv_unset(SH_SUBSCRNOD);
	}
	fcrestore(&save);
	if(offset>0)
		stakset(base,offset);
	return(sh.exitval);
}

/*
 * print a time and a separator 
 */
static void	p_time(Sfio_t *outfile,register clock_t t,int c)
{
	register int  min, sec, frac;
	register int hr;
	frac = t%sh.lim.clk_tck;
	frac = (frac*100)/sh.lim.clk_tck;
	t /= sh.lim.clk_tck;
	sec = t%60;
	t /= 60;
	min = t%60;
	if(hr=t/60)
		sfprintf(outfile,"%dh",hr);
	sfprintf(outfile,"%dm%d%c%02ds%c",min,sec,GETDECIMAL(0),frac,c);
}


/*
 * This dummy routine is called by built-ins that do recursion
 * on the file system (chmod, chgrp, chown).  It causes
 * the shell to invoke the non-builtin version in this case
 */
int cmdrecurse(int argc, char* argv[], int ac, char* av[])
{
	NOT_USED(argc);
	NOT_USED(argv[0]);
	NOT_USED(ac);
	NOT_USED(av[0]);
	return(SH_RUNPROG);
}

/*
 * set up pipe for cooperating process 
 */
static void coproc_init(int pipes[])
{
	if(sh.cpid)
		error(ERROR_exit(1),gettxt(e_pexists_id,e_pexists));
	if(sh.cpipe[0]<=0 || sh.cpipe[1]<=0)
	{
		/* first co-process */
		sh_pipe(sh.cpipe);
		if(fcntl(*sh.cpipe,F_SETFD,FD_CLOEXEC)>=0)
			sh.fdstatus[sh.cpipe[0]] |= IOCLEX;
		sh.fdptrs[sh.cpipe[0]] = sh.cpipe;
			
		if(fcntl(sh.cpipe[1],F_SETFD,FD_CLOEXEC) >=0)
			sh.fdstatus[sh.cpipe[1]] |= IOCLEX;
	}
	sh.outpipe = sh.cpipe;
	sh_pipe(sh.inpipe=pipes);
	sh.coutpipe = sh.inpipe[1];
	sh.fdptrs[sh.coutpipe] = &sh.coutpipe;
	if(fcntl(sh.outpipe[0],F_SETFD,FD_CLOEXEC)>=0)
		sh.fdstatus[sh.outpipe[0]] |= IOCLEX;
}

#ifdef SHOPT_SPAWN


#ifndef _lib_fork
/*
 * print out function definition
 */
static void print_fun(register Namval_t* np)
{
	register char *format;
	if(!is_afunction(np) || !np->nvalue.ip)
		return;
	if(nv_isattr(np,NV_FPOSIX))
		format="%s()\n{ ";
	else
		format="function %s\n{ ";
	sfprintf(sfstdout,format,nv_name(np));
	sh_deparse(sfstdout,(union anynode*)(nv_funtree(np)),0);
	sfwrite(sfstdout,"}\n",2);
}

/*
 * create a shell script consisting of t->fork.forktre and execute it
 */
static int run_subshell(const union anynode *t,pid_t grp)
{
	static char prolog[] = "(print $(typeset +A);set; typeset -p; print .sh.dollar=$$)";
	register int i, fd, trace = sh_isoption(SH_XTRACE);
	int pin,pout;
	pid_t pid;
	char *arglist[3], devfd[12], *cp;
	Sfio_t *sp = sftmp(0);
	arglist[0] = error_info.id?error_info.id:sh.shname;
	if(*arglist[0]=='-')
		arglist[0]++;
	arglist[1] = devfd;
	strncpy(devfd,e_devfdNN,sizeof(devfd));
	arglist[2] = 0;
	sfstack(sfstdout,sp);
	if(trace)
		sh_offoption(SH_XTRACE);
	sfwrite(sfstdout,"typeset -A -- ",14);
	sh_trap(prolog,0);
	nv_scan(sh.fun_tree, print_fun, 0, 0);
	if(sh.st.dolc>0)
	{
		/* pass the positional parameters */
		char **argv = sh.st.dolv+1;
		sfwrite(sfstdout,"set --",6);
		while(*argv)
			sfprintf(sfstdout," %s",sh_fmtq(*argv++));
		sfputc(sfstdout,'\n');
	}
	pin = (sh.inpipe?sh.inpipe[1]:0);
	pout = (sh.outpipe?sh.outpipe[0]:0);
	for(i=3; i < 10; i++)
	{
		if(sh.fdstatus[i]&IOCLEX && i!=pin && i!=pout)
		{
			sfprintf(sfstdout,"exec %d<&%d\n",i,i);
			fcntl(i,F_SETFD,0);
		}
	}
	sfprintf(sfstdout,"LINENO=%d\n",t->fork.forkline);
	if(trace)
	{
		sfwrite(sfstdout,"set -x\n",7);
		sh_onoption(SH_XTRACE);
	}
	sfstack(sfstdout,NIL(Sfio_t*));
	sh_deparse(sp,t->fork.forktre,0);
	sfseek(sp,0L,SEEK_SET);
	fd = sh_dup(sffileno(sp));
	sfclose(sp);
	cp = devfd+8;
	if(fd>9)
		*cp++ = '0' + (fd/10);
	*cp++ = '0' + fd%10;
	*cp = 0;
	sfsync(NIL(Sfio_t*));
	pid = spawnveg(pathshell(),arglist,arglist+2,grp);
	close(fd);
	for(i=3; i < 10; i++)
	{
		if(sh.fdstatus[i]&IOCLEX && i!=pin && i!=pout)
			fcntl(i,F_SETFD,FD_CLOEXEC);
	}
	if(pid <=0)
		error(ERROR_system(ERROR_NOEXEC),gettxt(e_exec_id,e_exec),arglist[0]);
	return(pid);
}
#endif /* !_lib_fork */

static void sigreset(int mode)
{
	register char   *trap;
	register int sig=sh.st.trapmax;
	while(sig-- > 0)
	{
		if((trap=sh.st.trapcom[sig]) && *trap==0)
			signal(sig,mode?sh_fault:SIG_IGN);
	}
}

/*
 * A combined fork/exec for systems with slow or non-existent fork()
 */
static pid_t sh_ntfork(const union anynode *t,char *argv[],int *jobid)
{
	static pid_t	spawnpid;
	static int	savetype;
	static int	savejobid;
	struct checkpt buff;
	int scope=0, jmpval;
	int otype=savetype,restore=0;
	char **arge, *path;
	pid_t grp = 0;
	savetype=0;
#   ifndef _lib_fork
	if(!argv)
	{
		register union anynode *tchild = t->fork.forktre;
		int optimize=0;
		otype = t->tre.tretyp;
		savetype = otype;
		spawnpid = 0;
		if((tchild->tre.tretyp&COMMSK)==TCOM)
		{
			Namval_t *np = (Namval_t*)(tchild->com.comnamp);
			if(np)
			{
				path = nv_name(np);
				if(!nv_isattr(np,BLT_ENV))
					np=0;
				else if(strcmp(path,"echo")==0 || memcmp(path,"print",5)==0)
					np=0;
			}
			else if(!tchild->com.comarg)
				optimize=1;
			else if(tchild->com.comtyp&COMSCAN)
			{
				if(tchild->com.comarg->argflag&ARG_RAW)
					path = tchild->com.comarg->argval;
				else
					path = 0;
			}
			else
				path = ((struct dolnod*)tchild->com.comarg)->dolval[ARG_SPARE];
			if(!np && path && !nv_search(path,sh.fun_tree,0))
				optimize=1;
		}
		sh_pushcontext(&buff,SH_JMPIO);
		jmpval = sigsetjmp(buff.buff,0);
		{
			if((otype&FINT) && !sh_isstate(SH_MONITOR))
			{
				signal(SIGQUIT,SIG_IGN);
				signal(SIGINT,SIG_IGN);
				if(!sh.st.ioset)
				{
					sh_iosave(0,buff.topfd);
					sh_iorenumber(sh_chkopen(e_devnull),0);
				}
			}
			if(otype&FPIN)
			{
				int fd = sh.inpipe[1];
				sh_iosave(0,buff.topfd);
				sh_iorenumber(sh.inpipe[0],0);
				if(fd>=0 && (!(otype&FPOU) || (otype&FCOOP)) && fcntl(fd,F_SETFD,FD_CLOEXEC)>=0)
					sh.fdstatus[fd] |= IOCLEX;
			}
			if(otype&FPOU)
			{
				sh_iosave(1,buff.topfd);
				sh_iorenumber(sh_dup(sh.outpipe[1]),1);
				if(fcntl(sh.outpipe[0],F_SETFD,FD_CLOEXEC)>=0)
					sh.fdstatus[sh.outpipe[0]] |= IOCLEX;
			}
	
			if(t->fork.forkio)
				sh_redirect(t->fork.forkio,0);
			if(optimize==0)
			{
#ifdef SIGTSTP
				if(job.jobcontrol)
				{
					signal(SIGTTIN,SIG_DFL);
					signal(SIGTTOU,SIG_DFL);
				}
#endif /* SIGTSTP */
#ifdef JOBS
				if(sh_isstate(SH_MONITOR) && (job.jobcontrol || (otype&FAMP)))
				{
					if((otype&FAMP) || job.curpgid==0)
						grp = 1;
					else
						grp = job.curpgid;
				}
#endif /* JOBS */
				spawnpid = run_subshell(t,grp);
			}
			else
			{
				sh_exec(tchild,0);
				if(jobid)
					*jobid = savejobid;
			}
		}
		sh_popcontext(&buff);
		if((otype&FINT) && !sh_isstate(SH_MONITOR))
		{
			signal(SIGQUIT,sh_fault);
			signal(SIGINT,sh_fault);
		}
		if((otype&FPIN) && (!(otype&FPOU) || (otype&FCOOP)) && fcntl(sh.inpipe[1],F_SETFD,FD_CLOEXEC)>=0)
			sh.fdstatus[sh.inpipe[1]] &= ~IOCLEX;
		if(t->fork.forkio || otype)
			sh_iorestore(buff.topfd);
		if(optimize==0)
		{
#ifdef SIGTSTP
			if(job.jobcontrol)
			{
				signal(SIGTTIN,SIG_IGN);
				signal(SIGTTOU,SIG_IGN);
			}
#endif /* SIGTSTP */
			if(spawnpid>0)
				_sh_fork(spawnpid,otype,jobid);
			if(grp==1 && !(otype&FAMP))
				tcsetpgrp(job.fd,job.curpgid);
		}
		savetype=0;
		if(jmpval>SH_JMPIO)
			siglongjmp(*sh.jmplist,jmpval);
		return(spawnpid);
	}
#   endif /* !_lib_fork */
	if(!strchr(path=argv[0],'/')) 
		path = sh.lastpath;
	sh_pushcontext(&buff,SH_JMPCMD);
	errorpush(&buff.err,ERROR_SILENT);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		error_info.id = *argv;
		if((otype&FINT) && !sh_isstate(SH_MONITOR))
		{
			signal(SIGQUIT,SIG_IGN);
			signal(SIGINT,SIG_IGN);
		}
		if(t->com.comio)
			sh_redirect(t->com.comio,0);
		if(t->com.comset)
		{
			scope++;
			nv_scope(t->com.comset);
		}
		if(!path)
		{
			spawnpid = -1;
			errno = ENOENT;
			goto fail;
		}
		arge = sh_envgen();
		sh.exitval = 0;
#ifdef SIGTSTP
		if(job.jobcontrol)
		{
			signal(SIGTTIN,SIG_DFL);
			signal(SIGTTOU,SIG_DFL);
		}
#endif /* SIGTSTP */
#ifdef JOBS
		if(sh_isstate(SH_MONITOR) && (job.jobcontrol || (otype&FAMP)))
		{
			if((otype&FAMP) || job.curpgid==0)
				grp = 1;
			else
				grp = job.curpgid;
		}
#endif /* JOBS */

		sfsync(NIL(Sfio_t*));
		sigreset(0);	/* set signals to ignore */
		spawnpid = spawnveg(path,argv,arge,grp);
		if(spawnpid < 0 && errno==ENOEXEC)
		{
			char *sp = argv[0];
			if(sh.lastpath)
				*argv = path;
			*--argv = "ksh";
			spawnpid = spawnveg(pathshell(),argv,arge,grp);
			*++argv = sp;
		}
	fail:
		if(spawnpid < 0) switch(errno)
		{
		    case ENOENT:
			error(ERROR_system(ERROR_NOENT),gettxt(e_found_id,e_found),argv[0]);
		    default:
			error(ERROR_system(ERROR_NOEXEC),gettxt(e_exec_id,e_exec),argv[0]);
		}
	}
	sh_popcontext(&buff);
#ifdef SIGTSTP
	if(job.jobcontrol)
	{
		signal(SIGTTIN,SIG_IGN);
		signal(SIGTTOU,SIG_IGN);
	}
#endif /* SIGTSTP */
	sigreset(1);	/* restore ignored signals */
	if(scope)
		nv_unscope();
	if(t->com.comio)
		sh_iorestore(buff.topfd);
	else if(jmpval>SH_JMPCMD)
		siglongjmp(*sh.jmplist,jmpval);
	if(spawnpid>0)
	{
		_sh_fork(spawnpid,otype,jobid);
#ifdef JOBS
		if(grp == 1)
		{
			job.curpgid = spawnpid;
#   ifdef SIGTSTP
			if(!(otype&FAMP))
				tcsetpgrp(job.fd,job.curpgid);
#   endif /* SIGTSTP */
		}
#endif /* JOBS */
		savejobid = *jobid;
		if(otype)
			return(0);
	}
	return(spawnpid);
}

#   ifndef _lib_fork
	pid_t fork(void)
	{
		error(ERROR_exit(3),gettxt(e_notimp_id,e_notimp),"fork");
		return(-1);
	}
#   endif /* _lib_fork */
#endif /* SHOPT_SPAWN */
