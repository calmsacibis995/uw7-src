#ident	"@(#)ksh93:src/cmd/ksh93/sh/fault.c	1.2.1.1"
#pragma prototyped
/*
 * Fault handling routines
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
#include	"io.h"
#include	"history.h"
#include	"shnodes.h"
#include	"jobs.h"
#include	"path.h"

extern char	*gettxt();
static char	setdone;
static char	indone;

/*
 * Most signals caught or ignored by the shell come here
*/
void	sh_fault(register int sig)
{
	register int 	flag;
	register char	*trap;
	register struct checkpt	*pp = (struct checkpt*)sh.jmplist;
	/* reset handler */
	signal(sig, sh_fault);
	/* handle ignored signals */
	if((trap=sh.st.trapcom[sig]) && *trap==0)
		return;
	flag = sh.sigflag[sig];
	if(!trap)
	{
		if(flag&SH_SIGIGNORE)
			return;
		if(flag&SH_SIGDONE)
		{
			if((flag&SH_SIGINTERACTIVE) && sh_isstate(SH_INTERACTIVE) && !sh_isstate(SH_FORKED) && ! sh.subshell)
			{
				if (sig == SIGTERM && job.in_critical) 
					sh.trapnote |= SH_SIGTERM; 
				return;
			}
			sh.lastsig = sig;
			sigrelease(sig);
			if(pp->mode < SH_JMPFUN)
				pp->mode = SH_JMPFUN;
			else
				pp->mode = SH_JMPEXIT;
			sh_exit(SH_EXITSIG);
		}
	}
	errno = 0;
	if(pp->mode==SH_JMPCMD)
		sh.lastsig = sig;
	if(trap)
	{
		/*
		 * propogage signal to foreground group
		 */
		if(sig==SIGHUP && job.curpgid)
			killpg(job.curpgid,SIGHUP);
		flag = SH_SIGTRAP;
	}
	else
	{
		sh.lastsig = sig;
		flag = SH_SIGSET;
#ifdef SIGTSTP
		if(sig==SIGTSTP)
		{
			sh.trapnote |= SH_SIGTSTP;
			if(pp->mode==SH_JMPCMD && sh_isstate(SH_STOPOK))
			{
				sigrelease(sig);
				sh_exit(SH_EXITSIG);
				flag = 0;
			}
		}
#endif /* SIGTSTP */
	}
	sh.trapnote |= flag;
	if(sig < sh.sigmax)
		sh.sigflag[sig] |= flag;
	if(pp->mode==SH_JMPCMD && sh_isstate(SH_STOPOK))
	{
		sigrelease(sig);
		sh_exit(SH_EXITSIG);
	}
}

/*
 * initialize signal handling
 */
void sh_siginit(void)
{
	register int sig, n=SIGTERM+1;
	register const struct shtable4	*tp = shtab_signals;
	sig_begin();
	/* find the largest signal number in the table */
	while(*tp->sh_name)
	{
		if((sig=tp->sh_number&((1<<SH_SIGBITS)-1))>n && sig<SH_TRAP)
			n = sig;
		tp++;
	}
	sh.sigmax = n;
	sh.st.trapcom = (char**)calloc(n,sizeof(char*));
	sh.sigflag = (unsigned char*)calloc(n,1);
	sh.sigmsg = (char**)calloc(n,sizeof(char*));
	for(tp=shtab_signals; sig=tp->sh_number; tp++)
	{
		n = (sig>>SH_SIGBITS);
		if((sig &= ((1<<SH_SIGBITS)-1)) > sh.sigmax)
			continue;
		sig--;
#if defined(_SC_SIGRT_MIN) && defined(_SIGRTMIN)
		if(sig==_SIGRTMIN)
			sig = SIGRTMIN;
#endif
#if defined(_SC_SIGRT_MAX) && defined(_SIGRTMAX)
		if(sig==_SIGRTMAX)
			sig = SIGRTMAX;
#endif
		sh.sigflag[sig] = n;
		if(*tp->sh_name)
			sh.sigmsg[sig] = gettxt(tp->sh_value_id,tp->sh_value);
	}
}

/*
 * Turn on trap handler for signal <sig>
 */
void	sh_sigtrap(register int sig)
{
	register int flag;
	sh.st.otrapcom = 0;
	if(sig==0)
		sh_sigdone();
	else if(!((flag=sh.sigflag[sig])&(SH_SIGFAULT|SH_SIGOFF)))
	{
		/* don't set signal if already set or off by parent */
		if(signal(sig,sh_fault)==SIG_IGN) 
		{
			signal(sig,SIG_IGN);
			flag |= SH_SIGOFF;
		}
		else
			flag |= SH_SIGFAULT;
		flag &= ~(SH_SIGSET|SH_SIGTRAP);
		sh.sigflag[sig] = flag;
	}
}

/*
 * set signal handler so sh_done is called for all caught signals
 */
void	sh_sigdone(void)
{
	register int 	flag, sig = sh.sigmax;
	setdone=1;
	sh.sigflag[0] |= SH_SIGFAULT;
	while(--sig>0)
	{
		flag = sh.sigflag[sig];
		if((flag&(SH_SIGDONE|SH_SIGIGNORE|SH_SIGINTERACTIVE)) && !(flag&(SH_SIGFAULT|SH_SIGOFF)))
			sh_sigtrap(sig);
	}
}

/*
 * Restore to default signals
 * Free the trap strings if mode is non-zero
 * If mode>1 then ignored traps cause signal to be ignored 
 */
void	sh_sigreset(register int mode)
{
	register char	*trap;
	register int 	flag, sig=sh.st.trapmax;
	while(sig-- > 0)
	{
		if(trap=sh.st.trapcom[sig])
		{
			flag  = sh.sigflag[sig]&~(SH_SIGTRAP|SH_SIGSET);
			if(*trap)
			{
				if(mode)
					free(trap);
				sh.st.trapcom[sig] = 0;
			}
			else if(sig && mode>1)
			{
				signal(sig,SIG_IGN);
				flag &= ~SH_SIGFAULT;
				flag |= SH_SIGOFF;
			}
			sh.sigflag[sig] = flag;
		}
	}
	for(sig=SH_DEBUGTRAP;sig>=0;sig--)
	{
		if(trap=sh.st.trap[sig])
		{
			if(mode)
				free(trap);
			sh.st.trap[sig] = 0;
		}
		
	}
	sh.st.trapcom[0] = 0;
	if(mode)
		sh.st.trapmax = 0;
	sh.trapnote=0;
}

/*
 * free up trap if set and restore signal handler if modified
 */
void	sh_sigclear(register int sig)
{
	register int flag = sh.sigflag[sig];
	register char *trap;
	sh.st.otrapcom=0;
	if(!(flag&SH_SIGFAULT))
		return;
	flag &= ~(SH_SIGTRAP|SH_SIGSET);
	if(trap=sh.st.trapcom[sig])
	{
		free(trap);
		sh.st.trapcom[sig]=0;
	}
	sh.sigflag[sig] = flag;
}

/*
 * check for traps
 */

void	sh_chktrap(void)
{
	register int 	sig=sh.st.trapmax;
	register char *trap;
	if(!sh.trapnote)
		sig=0;
	sh.trapnote &= ~SH_SIGTRAP;
	/* execute errexit trap first */
	if(sh_isstate(SH_ERREXIT) && sh.exitval)
	{
		int	sav_trapnote = sh.trapnote;
		sh.trapnote &= ~SH_SIGSET;
		if(sh.st.trap[SH_ERRTRAP])
			sh_trap(sh.st.trap[SH_ERRTRAP],0);
		sh.trapnote = sav_trapnote;
		if(sh_isoption(SH_ERREXIT))
		{
			struct checkpt	*pp = (struct checkpt*)sh.jmplist;
			pp->mode = SH_JMPEXIT;
			sh_exit(sh.exitval);
		}
	}
	if(sh.sigflag[SIGALRM]&SH_SIGTRAP)
		sh_timetraps();
	while(--sig>=0)
	{
		if(sh.sigflag[sig]&SH_SIGTRAP)
		{
			sh.sigflag[sig] &= ~SH_SIGTRAP;
			if(trap=sh.st.trapcom[sig])
				sh_trap(trap,0);
		}
	}
}


/*
 * parse and execute the given trap string, stream or tree depending on mode
 * mode==0 for string, mode==1 for stream, mode==2 for parse tree
 */
int sh_trap(const char *trap, int mode)
{
	int	jmpval, savxit = sh.exitval;
	int	save_states= sh_isstate(SH_HISTORY|SH_VERBOSE);
	int	staktop = staktell();
	char	*savptr = stakfreeze(0);
	struct	checkpt buff;
	Fcin_t	savefc;
	fcsave(&savefc);
	sh_offstate(SH_HISTORY|SH_VERBOSE);
	sh.intrap++;
	sh_pushcontext(&buff,SH_JMPTRAP);
	jmpval = sigsetjmp(buff.buff,0);
	if(jmpval == 0)
	{
		if(mode==2)
			sh_exec((union anynode*)trap,sh_isstate(SH_ERREXIT));
		else
		{
			Sfile_t *sp;
			if(mode)
				sp = (Sfio_t*)trap;
			else
				sp = sfopen(NIL(Sfio_t*),trap,"s");
			sh_eval(sp,0);
		}
	}
	else if(indone)
	{
		if(jmpval==SH_JMPSCRIPT)
			indone=0;
		else
		{
			if(jmpval==SH_JMPEXIT)
				savxit = sh.exitval;
			jmpval=SH_JMPTRAP;
		}
	}
	sh_popcontext(&buff);
	sh.intrap--;
	sfsync(sh.outpool);
	if(jmpval!=SH_JMPEXIT && jmpval!=SH_JMPFUN)
		sh.exitval=savxit;
	stakset(savptr,staktop);
	fcrestore(&savefc);
	sh_onstate(save_states);
	exitset();
	if(jmpval>SH_JMPTRAP)
		siglongjmp(*sh.jmplist,jmpval);
	return(sh.exitval);
}


/*
 * exit the current scope and jump to an earlier one based on pp->mode
 */
void sh_exit(register int xno)
{
	register struct checkpt	*pp = (struct checkpt*)sh.jmplist;
	register int		sig=0;
	sh.exitval=xno;
	if(xno==SH_EXITSIG)
		sh.exitval |= (sig=sh.lastsig);
#ifdef SIGTSTP
	if(sh.trapnote&SH_SIGTSTP)
	{
		/* ^Z detected by the shell */
		sh.trapnote = 0;
		sh.sigflag[SIGTSTP] = 0;
		if(!sh.subshell && sh_isstate(SH_MONITOR) && !sh_isstate(SH_STOPOK))
			return;
		if(sh_isstate(SH_TIMING))
			return;
		/* Handles ^Z for shell builtins, subshells, and functs */
		sh.lastsig = 0;
		sh_onstate(SH_MONITOR);
		sh_offstate(SH_STOPOK);
		sh.trapnote = 0;
		if(!sh.subshell && (sig=sh_fork(0,NIL(int*))))
		{
			job.curpgid = 0;
			job.parent = (pid_t)-1;
			job_wait(sig);
			job.parent = 0;
			sh.sigflag[SIGTSTP] = 0;
			/* wait for child to stop */
			sh.exitval = (SH_EXITSIG|SIGTSTP);
			/* return to prompt mode */
			pp->mode = SH_JMPERREXIT;
		}
		else
		{
			if(sh.subshell)
				sh_subfork();
			/* child process, put to sleep */
			sh_offstate(SH_MONITOR|SH_STOPOK);
			sh.sigflag[SIGTSTP] = 0;
			/* stop child job */
			killpg(job.curpgid,SIGTSTP);
			/* child resumes */
			job_clear();
			sh.forked = 1;
			sh.exitval = (xno&SH_EXITMASK);
			return;
		}
	}
#endif /* SIGTSTP */
	/* unlock output pool */
	sh_offstate(SH_NOTRACK);
	sfclrlock(sfpool(NIL(Sfio_t*),sh.outpool,SF_WRITE));
#ifdef SIGPIPE
	if(sh.lastsig==SIGPIPE)
		sfpurge(sfpool(NIL(Sfio_t*),sh.outpool,SF_WRITE));
#endif /* SIGPIPE */
	sfclrlock(sfstdin);
	if(!pp)
		sh_done(sig);
	sh.prefix = 0;
	if(pp->mode == SH_JMPSCRIPT && !pp->prev) 
		sh_done(sig);
	siglongjmp(pp->buff,pp->mode);
}

/*
 * This is the exit routine for the shell
 */

void sh_done(register int sig)
{
	register char *t;
	register int savxit = sh.exitval;
	sh.trapnote = 0;
	indone=1;
	if(sig==0)
		sig = sh.lastsig;
	if(t=sh.st.trapcom[0])
	{
		sh.st.trapcom[0]=0; /*should free but not long */
		sh.oldexit = savxit;
		sh_trap(t,0);
		savxit = sh.exitval;
	}
	else
	{
		/* avoid recursive call for set -e */
		sh_offstate(SH_ERREXIT);
		sh_chktrap();
	}
	sh_freeup();
#ifdef SHOPT_ACCT
	sh_accend();
#endif	/* SHOPT_ACCT */
#if SHOPT_VSH || SHOPT_ESH
	if(sh_isoption(SH_EMACS|SH_VI|SH_GMACS))
		tty_cooked(-1);
#endif
#ifdef JOBS
	if((sh_isoption(SH_INTERACTIVE) && sh.login_sh) || (!sh_isoption(SH_INTERACTIVE) && (sig==SIGHUP)))
		job_walk(sfstderr,job_terminate,SIGHUP,NIL(char**));
#endif	/* JOBS */
	job_close();
	sfsync((Sfio_t*)sfstdin);
	sfsync((Sfio_t*)sh.outpool);
	sfsync((Sfio_t*)sfstdout);
	if(sig)
	{
		/* generate fault termination code */
		signal(sig,SIG_DFL);
		sigrelease(sig);
		kill(getpid(),sig);
		pause();
	}
#ifdef SHOPT_KIA
	if(sh_isoption(SH_NOEXEC))
		kiaclose();
#endif /* SHOPT_KIA */
	exit(savxit&SH_EXITMASK);
}

