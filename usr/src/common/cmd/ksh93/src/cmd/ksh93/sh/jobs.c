#ident	"@(#)ksh93:src/cmd/ksh93/sh/jobs.c	1.2"
#pragma prototyped
/*
 *  Job control for UNIX Shell
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 *  Written October, 1982
 *  Rewritten April, 1988
 *  Revised January, 1992
 */

#include	"defs.h"
#include	<ctype.h>
#include	<wait.h>
#include	"io.h"
#include	"jobs.h"
#include	"history.h"

extern char 	*gettxt();

#ifndef WCONTINUED
#   define WCONTINUED	0
#   define WIFCONTINUED(wstat)	(0)
#endif

/*
 * temporary hack to get W* macros to work
 */
#undef wait
#define wait    ______wait
/*
 * This struct saves a link list of processes that have non-zero exit
 * status, have had $! saved, but haven't been waited for
 */
struct jobsave
{
	struct jobsave	*next;
	pid_t		pid;
	unsigned short	exitval;
	int		env;
};


#define BYTE(n)		(((n)+CHAR_BIT-1)/CHAR_BIT)
#define MAXMSG	25
#define SH_STOPSIG	(SH_EXITSIG<<1)

#ifdef VSUSP
#   ifndef CNSUSP
#	ifdef _POSIX_VDISABLE
#	   define CNSUSP	_POSIX_VDISABLE
#	else
#	   define CNSUSP	0
#	endif /* _POSIX_VDISABLE */
#   endif /* CNSUSP */
#   ifndef CSWTCH
#	ifdef CSUSP
#	    define CSWTCH	CSUSP
#	else
#	    define CSWTCH	('z'&037)
#	endif /* CSUSP */
#   endif /* CSWTCH */
#endif /* VSUSP */

/* Process states */
#define P_EXITSAVE	01
#define P_STOPPED	02
#define P_NOTIFY	04
#define P_SIGNALLED	010
#define P_STTY		020
#define P_DONE		040
#define P_COREDUMP	0100
#define P_DISOWN	0200

static struct process	*job_bypid(pid_t);
static struct process	*job_byjid(int);
static char		*job_sigmsg(int);
static int		job_alloc(void);
static void		job_free(int);
static struct process	*job_unpost(struct process*,int);
static void		job_unlink(struct process*);
static void		job_prmsg(struct process*);
static struct process	*freelist;
static char		beenhere;
static char		possible;
static int		savesig;
static struct process	dummy;
static int		bck_count;
static struct jobsave	*bck_list;
static char		by_number;
static Sfio_t		*outfile;
static pid_t		lastpid;

#ifdef JOBS
    static void			job_set(struct process*);
    static void			job_reset(struct process*);
    static void			job_waitsafe(int);
    static struct process	*job_byname(char*);
    static struct process	*job_bystring(char*);
    static struct termios	my_stty;  /* terminal state for shell */
    static char			*job_string;
#else
    extern const char		e_coredump[];
#endif /* JOBS */

#ifdef SIGTSTP
    static void		job_unstop(struct process*);
    static void		job_fgrp(struct process*, int);
#   ifndef _lib_tcgetpgrp
#	ifdef TIOCGPGRP
	   static int _i_;
#	   define tcgetpgrp(a) (ioctl(a, TIOCGPGRP, &_i_)>=0?_i_:-1)	
#	endif /* TIOCGPGRP */
	int tcsetpgrp(int fd,pid_t pgrp)
	{
		int pgid = pgrp;
#		ifdef TIOCGPGRP
			return(ioctl(fd, TIOCSPGRP, &pgid));	
#		else
			return(-1);
#		endif /* TIOCGPGRP */
	}
#   endif /* _lib_tcgetpgrp */
#else
#   define job_unstop(pw)
#   undef CNSUSP
#endif /* SIGTSTP */

#ifndef OTTYDISC
#   undef NTTYDISC
#endif /* OTTYDISC */


#ifdef JOBS

/*
 * This is the SIGCLD interrupt routine
 * When called with sig==0, it does a blocking wait
 */
static void job_waitsafe(register int sig)
{
	register pid_t pid;
	register struct process *pw;
	struct process *px;
	register int flags;
	struct process dummy;
	struct jobsave *jp;
	int wstat;
#ifdef DEBUG
	if(sfprintf(sfstderr,"%d: signal %d critical=%d\n",getpid(),sig,job.in_critical) <=0)
		write(2,"waitsafe\n",9);
	sfsync(sfstderr);
#endif /* DEBUG */
	if(job.in_critical && sig)
	{
		savesig = sig;
		return;
	}
	savesig = 0;
	if(sig)
		flags = WNOHANG|WUNTRACED|WCONTINUED;
	else
		flags = WUNTRACED;
	while(1)
	{
		if(!(flags&WNOHANG))
		{
			if(sh.waitevent && (*sh.waitevent)(-1,0L))
				flags |= WNOHANG;
		}
		pid = waitpid((pid_t)-1,&wstat,flags);
		if(sig && pid<0 && errno==EINTR)
			continue;
		if(pid<=0)
			break;
		flags |= WNOHANG;
		job.waitsafe++;
		if(!(pw=job_bypid(pid)))
		{
#ifdef DEBUG
			sfprintf(sfstderr,"%d: unknown job pid=%d pw=%x\n",getpid(),pid,pw);
#endif /* DEBUG */
			pw = &dummy;
			if(bck_count++ > sh.lim.child_max)
				job_chksave(0);
			jp = new_of(struct jobsave,0);
			jp->next = bck_list;
			bck_list = jp;
			lastpid = pw->p_pid = jp->pid = pid;
			pw->p_flag = 0;
			jp->exitval = 0;
			jp->env = sh.curenv;
			px = 0;
			if(WIFSTOPPED(wstat))
			{
				jp->exitval = SH_STOPSIG;
				continue;
			}
		}
#ifdef SIGTSTP
		else
			px=job_byjid(pw->p_job);
		if(WIFSTOPPED(wstat))
		{
			/* move to top of job list */
			job_unlink(px);
			px->p_nxtjob = job.pwlist;
			job.pwlist = px;
			pw->p_exit = WSTOPSIG(wstat);
			pw->p_flag |= (P_NOTIFY|P_SIGNALLED|P_STOPPED);
			if(pw->p_pgrp && pw->p_pgrp==job.curpgid && sh_isstate(SH_STOPOK))
				sh_fault(pw->p_exit); 
			continue;
		}
		else if (WIFCONTINUED(wstat))
			pw->p_flag &= ~(P_NOTIFY|P_SIGNALLED|P_STOPPED);
		else
#endif /* SIGTSTP */
		{
			/* check for coprocess completion */
			if(pid==sh.cpid)
			{
				sh_close(sh.coutpipe);
				sh_close(sh.cpipe[1]);
				sh.cpipe[1] = -1;
				sh.coutpipe = -1;
			}
			if (WIFSIGNALED(wstat))
			{
				pw->p_flag &= ~P_STOPPED;
				pw->p_flag |= (P_DONE|P_NOTIFY|P_SIGNALLED);
				if (WTERMCORE(wstat))
					pw->p_flag |= P_COREDUMP;
				pw->p_exit = WTERMSIG(wstat);
				/* if process in current jobs terminates from
				 * an interrupt, propogate to parent shell
				 */
				if(pw->p_pgrp && pw->p_pgrp==job.curpgid && pw->p_exit==SIGINT && sh_isstate(SH_STOPOK))
				{
					pw->p_flag &= ~P_NOTIFY;
					sh_offstate(SH_STOPOK);
					sh_fault(SIGINT); 
					sh_onstate(SH_STOPOK);
				}
			}
			else
			{
				pw->p_flag |= (P_DONE|P_NOTIFY);
				pw->p_exit = WEXITSTATUS(wstat);
			}
			if(pw->p_pgrp==0)
				pw->p_flag &= ~P_NOTIFY;
		}
		if(pw == &dummy)
		{
			jp->exitval = pw->p_exit;
			if(pw->p_flag&P_SIGNALLED)
				jp->exitval |= SH_EXITSIG;
		}
#ifdef DEBUG
		sfprintf(sfstderr,"%d: job %d with pid %d flags=%o complete with status=%x exit=%d\n",getpid(),pw->p_job,pid,pw->p_flag,wstat,pw->p_exit);
		sfsync(sfstderr);
#endif /* DEBUG*/
		/* only top-level process in job should have notify set */
		if(px && pw != px)
			pw->p_flag &= ~P_NOTIFY;
	}
	if(sh.st.trapcom[SIGCHLD])
	{
		sh.sigflag[SIGCHLD] |= SH_SIGTRAP;
		sh.trapnote |= SH_SIGTRAP;
	}
	if(sh_isoption(SH_NOTIFY) && sh_isstate(SH_TTYWAIT))
	{
		outfile = sfstderr;
		job_list(pw,JOB_NFLAG|JOB_NLFLAG);
		job_unpost(pw,1);
		sfsync(sfstderr);
	}
	if(sig)
		signal(sig, job_waitsafe);
}

/*
 * initialize job control if possible
 * if lflag is set the switching driver message will not print
 */
void job_init(int lflag)
{
	register int ntry=0;

	job.fd = JOBTTY;
	signal(SIGCHLD,job_waitsafe);
#   if defined(SIGCLD) && (SIGCLD!=SIGCHLD)
	signal(SIGCLD,job_waitsafe);
#   endif
	if(!sh_isoption(SH_INTERACTIVE))
		return;
	/* use new line discipline when available */
#ifdef NTTYDISC
#   ifdef FIOLOOKLD
	if((job.linedisc = ioctl(JOBTTY, FIOLOOKLD, 0)) <0)
#   else
	if(ioctl(JOBTTY,TIOCGETD,&job.linedisc) !=0)
#   endif /* FIOLOOKLD */
		return;
	if(job.linedisc!=NTTYDISC && job.linedisc!=OTTYDISC)
	{
		/* no job control when running with MPX */
#   ifdef SHOPT_VSH
		sh_onoption(SH_VIRAW);
#   endif /* SHOPT_VSH */
		return;
	}
	if(job.linedisc==NTTYDISC)
		job.linedisc = -1;
#endif /* NTTYDISC */

	job.mypgid = getpgrp();
	/* some systems have job control, but not initialized */
	if(job.mypgid<=0)
        {
		/* Get a controlling terminal and set process group */
		/* This should have already been done by rlogin */
                register int fd;
                register char *ttynam;
#ifndef SIGTSTP
                setpgid(0,sh.pid);
#endif /*SIGTSTP */
                if(job.mypgid<0 || !(ttynam=ttyname(JOBTTY)))
                        return;
                close(JOBTTY);
                if((fd = open(ttynam,O_RDWR)) <0)
                        return;
                if(fd!=JOBTTY)
                        sh_iorenumber(fd,JOBTTY);
                job.mypgid = sh.pid;
#ifdef SIGTSTP
                tcsetpgrp(JOBTTY,sh.pid);
                setpgid(0,sh.pid);
#endif /* SIGTSTP */
        }
#ifdef SIGTSTP
	if(possible = (setpgid(0,job.mypgid)>=0) || errno==EPERM)
	{
		/* wait until we are in the foreground */
		while((job.mytgid=tcgetpgrp(JOBTTY)) != job.mypgid)
		{
			if(job.mytgid == -1)
				return;
			/* Stop this shell until continued */
			signal(SIGTTIN,SIG_DFL);
			kill(sh.pid,SIGTTIN);
			/* resumes here after continue tries again */
			if(ntry++ > IOMAXTRY)
			{
				error(0,gettxt(e_no_start_id,e_no_start));
				return;
			}
		}
	}
#endif /* SIGTTIN */

#ifdef NTTYDISC
	/* set the line discipline */
	if(job.linedisc>=0)
	{
		int linedisc = NTTYDISC;
#   ifdef FIOPUSHLD
		tty_get(JOBTTY,&my_stty);
		if (ioctl(JOBTTY, FIOPOPLD, 0) < 0)
			return;
		if (ioctl(JOBTTY, FIOPUSHLD, &linedisc) < 0)
		{
			ioctl(JOBTTY, FIOPUSHLD, &job.linedisc);
			return;
		}
		tty_set(JOBTTY,TCSANOW,&my_stty);
#   else
		if(ioctl(JOBTTY,TIOCSETD,&linedisc) !=0)
			return;
#   endif /* FIOPUSHLD */
		if(lflag==0)
			error(0,gettxt(e_newtty_id,e_newtty));
		else
			job.linedisc = -1;
	}
#endif /* NTTYDISC */
	if(!possible)
		return;

#ifdef SIGTSTP
	/* make sure that we are a process group leader */
	setpgid(0,sh.pid);
#   if defined(SA_NOCLDWAIT) && defined(_lib_sigflag)
	sigflag(SIGCHLD, SA_NOCLDSTOP|SA_NOCLDWAIT, 0);
#   endif /* SA_NOCLDWAIT */
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	/* The shell now handles ^Z */
	signal(SIGTSTP,sh_fault);
	tcsetpgrp(JOBTTY,sh.pid);
#   ifdef CNSUSP
	/* set the switch character */
	tty_get(JOBTTY,&my_stty);
	job.suspend = (unsigned)my_stty.c_cc[VSUSP];
	if(job.suspend == (unsigned char)CNSUSP)
	{
		my_stty.c_cc[VSUSP] = CSWTCH;
		tty_set(JOBTTY,TCSAFLUSH,&my_stty);
	}
#   endif /* CNSUSP */
	sh_onoption(SH_MONITOR);
	job.jobcontrol++;
#endif /* SIGTSTP */
	return;
}


/*
 * see if there are any stopped jobs
 * restore tty driver and pgrp
 */
int job_close(void)
{
	register struct process *pw = job.pwlist;
	register int count = 0, running = 0;
	if(possible && !job.jobcontrol)
		return(0);
	else if(!possible && (!sh_isstate(SH_MONITOR) || sh_isstate(SH_FORKED)))
		return(0);
	if(!tty_check(0))
		beenhere++;
	for(;pw;pw=pw->p_nxtjob)
	{
		if(!(pw->p_flag&P_STOPPED))
		{
			if(!(pw->p_flag&P_DONE))
				running++;
			continue;
		}
		if(beenhere)
			killpg(pw->p_pgrp,SIGTERM);
		count++;
	}
	if(beenhere++ == 0 && job.pwlist)
	{
		if(count)
		{
			error(0,gettxt(e_terminate_id,e_terminate));
			return(-1);
		}
		else if(running && sh.login_sh)
		{
			error(0,gettxt(e_jobsrunning_id,e_jobsrunning));
			return(-1);
		}
	}
#   ifdef SIGTSTP
	if(possible && setpgid(0,job.mypgid)>=0)
		tcsetpgrp(job.fd,job.mypgid);
#   endif /* SIGTSTP */
#   ifdef NTTYDISC
	if(job.linedisc>=0)
	{
		/* restore old line discipline */
#	ifdef FIOPUSHLD
		tty_get(job.fd,&my_stty);
		if (ioctl(job.fd, FIOPOPLD, 0) < 0)
			return(0);
		if (ioctl(job.fd, FIOPUSHLD, &job.linedisc) < 0)
		{
			job.linedisc = NTTYDISC;
			ioctl(job.fd, FIOPUSHLD, &job.linedisc);
			return(0);
		}
		tty_set(job.fd,TCSAFLUSH,&my_stty);
#	else
		if(ioctl(job.fd,TIOCSETD,&job.linedisc) !=0)
			return(0);
#	endif /* FIOPUSHLD */
		error(0,gettxt(e_oldtty_id,e_oldtty));
	}
#   endif /* NTTYDISC */
#   ifdef CNSUSP
	if(possible && job.suspend==CNSUSP)
	{
		tty_get(job.fd,&my_stty);
		my_stty.c_cc[VSUSP] = CNSUSP;
		tty_set(job.fd,TCSAFLUSH,&my_stty);
	}
#   endif /* CNSUSP */
	job.jobcontrol = 0;
	return(0);
}

static void job_set(register struct process *pw)
{
	/* save current terminal state */
	tty_get(job.fd,&my_stty);
	if(pw->p_flag&P_STTY)
	{
		/* restore terminal state for job */
		tty_set(job.fd,TCSAFLUSH,&pw->p_stty);
	}
#ifdef SIGTSTP
	tcsetpgrp(job.fd,pw->p_fgrp);
	/* if job is stopped, resume it in the background */
	job_unstop(pw);
#endif	/* SIGTSTP */
}

static void job_reset(register struct process *pw)
{
	/* save the terminal state for current job */
#ifdef SIGTSTP
	job_fgrp(pw,tcgetpgrp(job.fd));
	if(tcsetpgrp(job.fd,sh.pid) !=0)
		return;
#endif	/* SIGTSTP */
	/* force the following tty_get() to do a tcgetattr() */
	tty_set(-1, 0, NIL(struct termios*));
	if(pw && (pw->p_flag&P_SIGNALLED) && pw->p_exit!=SIGHUP)
	{
		if(tty_get(job.fd,&pw->p_stty) == 0)
			pw->p_flag |= P_STTY;
		/* restore terminal state for job */
		tty_set(job.fd,TCSAFLUSH,&my_stty);
	}
	beenhere = 0;
}
#endif /* JOBS */

/*
 * wait built-in command
 */

void job_bwait(char **jobs)
{
	register char *job;
	register struct process *pw;
	register pid_t pid;
	if(*jobs==0)
		job_wait((pid_t)-1);
	else while(job = *jobs++)
	{
#ifdef JOBS
		if(*job == '%')
		{
			if(pw = job_bystring(job))
				pid = pw->p_pid;
			else
				return;
		}
		else
#endif /* JOBS */
			pid = atoi(job);
		job_wait(-pid);
	}
}

#ifdef JOBS
/*
 * execute function <fun> for each job
 */

int job_walk(Sfio_t *file,int (*fun)(struct process*,int),int arg,char *joblist[])
{
	register struct process *pw = job.pwlist;
	register int r = 0;
	register char *jobid, **jobs=joblist;
	register struct process *px;
	job_string = 0;
	outfile = file;
	by_number = 0;
	if(jobs==0)
	{
		/* do all jobs */
		for(;pw;pw=px)
		{
			px = pw->p_nxtjob;
			if((*fun)(pw,arg))
				r = 2;
		}
	}
	else if(*jobs==0)	/* current job */
	{
		/* skip over non-stop jobs */
		while(pw && pw->p_pgrp==0)
			pw = pw->p_nxtjob;
		if((*fun)(pw,arg))
			r = 2;
	}
	else while(jobid = *jobs++)
	{
		job_string = jobid;
		if(*jobid==0)
			error(ERROR_exit(1),gettxt(e_jobusage_id,e_jobusage),job_string);
		if(*jobid == '%')
			pw = job_bystring(jobid);
		else
		{
			int pid = atoi(jobid);
			if(pid<0)
				jobid++;
			while(isdigit(*jobid))
				jobid++;
			if(*jobid)
				error(ERROR_exit(1),gettxt(e_jobusage_id,e_jobusage),job_string);
			if(!(pw = job_bypid(pid)))
			{
				pw = &dummy;
				pw->p_pid = pid;
				pw->p_pgrp = pid;
			}
			by_number = 1;
		}
		if((*fun)(pw,arg))
			r = 2;
		by_number = 0;
	}
	return(r);
}

/*
 * send signal <sig> to background process group if not disowned
 */
int job_terminate(register struct process *pw,register int sig)
{
	if(pw->p_pgrp && !(pw->p_flag&P_DISOWN))
		job_kill(pw,sig);
	return(0);
}

/*
 * list the given job
 * flag JOB_LFLAG for long listing
 * flag JOB_NFLAG for list only jobs marked for notification
 * flag JOB_PFLAG for process id(s) only
 */

int job_list(struct process *pw,register int flag)
{
	register struct process *px = pw;
	register int  n;
	register const char *msg, *msg_id;
	register int msize;
	if(!pw || pw->p_job<=0)
		return(1);
	if(pw->p_env != sh.jobenv)
		return(0);
	if((flag&JOB_NFLAG) && (!(px->p_flag&P_NOTIFY)||px->p_pgrp==0))
		return(0);
	if((flag&JOB_PFLAG))
	{
		sfprintf(outfile,"%d\n",px->p_pgrp);
		return(0);
	}
	if((px->p_flag&P_DONE) && job.waitall && !(flag&JOB_LFLAG))
		return(0);
	n = px->p_job;
	if(px==job.pwlist)
		msize = '+';
	else if(px==job.pwlist->p_nxtjob)
		msize = '-';
	else
		msize = ' ';
	if(flag&JOB_NLFLAG)
		sfputc(outfile,'\n');
	sfprintf(outfile,"[%d] %c ",n, msize);
	do
	{
		n = 0;
		if(flag&JOB_LFLAG)
			sfprintf(outfile,"%d\t",px->p_pid);
		if(px->p_flag&P_SIGNALLED) {
			if (px->p_exit) {
				msg = job_sigmsg((int)(px->p_exit));
				msg_id = ":0";
			} else {  /* is this correct??? */
				msg = e_done;
				msg_id = e_done_id;
			}
		}
		else if(px->p_flag&P_NOTIFY)
		{
			msg = e_done;
			msg_id = e_done_id;
			n = px->p_exit;
		}
		else {
			msg = e_running;
			msg_id = e_running_id;
		}
		px->p_flag &= ~P_NOTIFY;
		msg = ERROR_translate(gettxt(msg_id,msg),1);
		sfputr(outfile,msg,-1);
		msize = strlen(msg);
		if(n)
		{
			sfprintf(outfile,"(%d)",(int)n);
			msize += (3+(n>10)+(n>100));
		}
		if(px->p_flag&P_COREDUMP)
		{
			msg = ERROR_translate(gettxt(e_coredump_id,e_coredump),1);
			sfputr(outfile, msg, -1);
			msize += strlen(msg);
		}
		sfnputc(outfile,' ',MAXMSG>msize?MAXMSG-msize:1);
		if(flag&JOB_LFLAG)
			px = px->p_nxtproc;
		else
		{
			while(px=px->p_nxtproc)
				px->p_flag &= ~P_NOTIFY;
			px = 0;
		}
		if(!px)
			hist_list(sh.hist_ptr,outfile,pw->p_name,0,";");
		else
			sfputr(outfile, e_nlspace, -1);
	}
	while(px);
	return(0);
}

/*
 * get the process group given the job number
 * This routine returns the process group number or -1
 */
static struct process *job_bystring(register char *ajob)
{
	register struct process *pw=job.pwlist;
	register int c;
	if(*ajob++ != '%' || !pw)
		return(NIL(struct process*));
	c = *ajob;
	if(isdigit(c))
		pw = job_byjid(atoi(ajob));
	else if(c=='+' || c=='%')
		;
	else if(c=='-')
	{
		if(pw)
			pw = job.pwlist->p_nxtjob;
	}
	else
		pw = job_byname(ajob);
	if(pw && pw->p_flag)
		return(pw);
	return(NIL(struct process*));
}

/*
 * Kill a job or process
 */

int job_kill(register struct process *pw,register int sig)
{
	register pid_t pid;
	register int r;
	const char *msg, *msg_id;
#ifdef SIGTSTP
	int stopsig = (sig==SIGSTOP||sig==SIGTSTP||sig==SIGTTIN||sig==SIGTTOU);
#else
#	define stopsig	1
#endif	/* SIGTSTP */
	errno = 0;
	if(pw==0)
		goto error;
	pid = pw->p_pid;
	if(by_number)
	{
		if(pid==0 && job.jobcontrol)
			r = job_walk(outfile, job_kill,sig, (char**)0);
#ifdef SIGTSTP
		if(sig==SIGSTOP && pid==sh.pid && sh.ppid==1)
		{
			/* can't stop login shell */
			errno = EPERM;
			r = -1;
		}
		else
		{
			if(pid>=0)
			{
				if((r = kill(pid,sig))>=0 && !stopsig)
				{
					if(pw->p_flag&P_STOPPED)
						pw->p_flag &= ~(P_STOPPED|P_SIGNALLED);
					kill(pid,SIGCONT);
				}
			}
			else
			{
				if((r = killpg(-pid,sig))>=0 && !stopsig)
				{
					job_unstop(job_bypid(pw->p_pid));
					killpg(-pid,SIGCONT);
				}
			}
		}
#else
		if(pid>=0)
			r = kill(pid,sig);
		else
			r = killpg(-pid,sig);
#endif	/* SIGTSTP */
	}
	else
	{
		if(pid = pw->p_pgrp)
		{
			if((r = killpg(pid,sig))>=0 && (sig==SIGHUP||sig==SIGTERM || sig==SIGCONT))
				job_unstop(pw);
			if(r>=0)
				sh_delay(.05);
		}
		while(pw && pw->p_pgrp==0 && (r=kill(pw->p_pid,sig))>=0) 
		{
#ifdef SIGTSTP
			if(sig==SIGHUP || sig==SIGTERM)
				kill(pw->p_pid,SIGCONT);
#endif	/* SIGTSTP */
			pw = pw->p_nxtproc;
		}
	}
	if(r<0 && job_string)
	{
	error:
		if(pw && by_number) {
			msg = e_no_proc;
			msg_id = e_no_proc_id;
		} else {
			msg = e_no_job;
			msg_id = e_no_job_id;
		}
		if(errno == EPERM) {
			msg = e_access;
			msg_id = e_access_id;
		}
		sfprintf(sfstderr,"kill: %s: %s\n",job_string, ERROR_translate(gettxt(msg_id,msg),1));
		r = 2;
	}
	return(r);
}

/*
 * Get process structure from first letters of jobname
 *
 */

static struct process *job_byname(char *name)
{
	register struct process *pw = job.pwlist;
	register struct process *pz = 0;
	register int *flag = 0;
	register char *cp = name;
	int offset;
	if(!sh.hist_ptr)
		return(NIL(struct process*));
	if(*cp=='?')
		cp++,flag= &offset;
	for(;pw;pw=pw->p_nxtjob)
	{
		if(hist_match(sh.hist_ptr,pw->p_name,cp,flag)>=0)
		{
			if(pz)
				error(ERROR_exit(1),gettxt(e_jobusage_id,e_jobusage),name-1);
			pz = pw;
		}
	}
	return(pz);
}

#else
#   define job_set(x)
#   define job_reset(x)
#endif /* JOBS */



/*
 * Initialize the process posting array
 */

void	job_clear(void)
{
	register struct process *pw, *px;
	register struct process *pwnext;
	register int j = BYTE(sh.lim.child_max);
	register struct jobsave *jp;
	for(pw=job.pwlist; pw; pw=pwnext)
	{
		pwnext = pw->p_nxtjob;
		while(px=pw)
		{
			pw = pw->p_nxtproc;
			free((char*)px);
		}
	}
	for(jp=bck_list; jp;)
	{
		px = (struct process*)jp;
		jp = jp->next;
		free((char*)px);
	}
	bck_list = 0;
	job.pwlist = NIL(struct process*);
	job.numpost=0;
	job.waitall = 0;
	job.curpgid = 0;
	job.toclear = 0;
	if(!job.freejobs)
		job.freejobs = (unsigned char*)malloc((unsigned)(j+1));
	while(j >=0)
		job.freejobs[j--]  = 0;
}

/*
 * put the process <pid> on the process list and return the job number
 * if non-zero, <join> is the process id of the job to join
 */

int job_post(pid_t pid, pid_t join)
{
	register struct process *pw;
	register History_t *hp = sh.hist_ptr;
	sh.jobenv = sh.curenv;
	if(job.toclear)
	{
		job_clear();
		return(0);
	}
	if(join && (pw=job_bypid(join)))
	{
		/* if job to join is not first move it to front */
		if((pw=job_byjid(pw->p_job)) != job.pwlist)
		{
			job_unlink(pw);
			pw->p_nxtjob = job.pwlist;
			job.pwlist = pw;
		}
	}
	if(pw=freelist)
		freelist = pw->p_nxtjob;
	else
		pw = new_of(struct process,0);
	job.numpost++;
	if(join && job.pwlist)
	{
		/* join existing current job */
		pw->p_nxtjob = job.pwlist->p_nxtjob;
		pw->p_nxtproc = job.pwlist;
		pw->p_job = job.pwlist->p_job;
	}
	else
	{
		/* create a new job */
		pw->p_nxtjob = job.pwlist;
		while((pw->p_job = job_alloc()) < 0)
			job_wait((pid_t)1);
		pw->p_nxtproc = 0;
	}
	job.pwlist = pw;
	pw->p_env = sh.curenv;
	pw->p_pid = pid;
	pw->p_flag = P_EXITSAVE;
	if(sh_isstate(SH_MONITOR))
		pw->p_fgrp = job.curpgid;
	else
		pw->p_fgrp = 0;
	pw->p_pgrp = pw->p_fgrp;
#ifdef DEBUG
	sfprintf(sfstderr,"%d: posting %d pid=%d pgid=%d savesig=%d join=%d\n",getpid(),pw->p_job,
		pw->p_pid,pw->p_pgrp,savesig,join);
	sfsync(sfstderr);
#endif /* DEBUG */
#ifdef JOBS
	if(hp && !sh_isstate(SH_PROFILE))
		pw->p_name=hist_tell(sh.hist_ptr,(int)hp->histind-1);
	else
		pw->p_name = -1;
#endif /* JOBS */
	if(pid==lastpid)
	{
		pw->p_exit= job_chksave(pid);
		if(pw->p_exit==SH_STOPSIG)
		{
			pw->p_flag |= (P_SIGNALLED|P_STOPPED);
			pw->p_exit = 0;
		}
		else
			pw->p_flag |= (P_DONE|P_NOTIFY);
	}
	lastpid = 0;
	job.in_critical=0;
	if(savesig)
		job_waitsafe(savesig);
	return(pw->p_job);
}

/*
 * Returns a process structure give a process id
 */

static struct process *job_bypid(pid_t pid)
{
	register struct process  *pw, *px;
	for(pw=job.pwlist; pw; pw=pw->p_nxtjob)
		for(px=pw; px; px=px->p_nxtproc)
		{
			if(px->p_pid==pid)
				return(px);
		}
	return(NIL(struct process*));
}

/*
 * return a pointer to a job given the job id
 */

static struct process *job_byjid(int jobid)
{
	register struct process *pw;
	for(pw=job.pwlist;pw; pw = pw->p_nxtjob)
	{
		if(pw->p_job==jobid)
			break;
	}
	return(pw);
}

/*
 * print a signal message
 */
static void job_prmsg(register struct process *pw)
{
	if(pw->p_exit!=SIGINT && pw->p_exit!=SIGPIPE)
	{
		register const char *msg, *dump;
		msg = job_sigmsg((int)(pw->p_exit));
		if(pw->p_flag&P_COREDUMP)
			dump =  ERROR_translate(gettxt(e_coredump_id,e_coredump),1);
		else
			dump = "";
		if(sh_isstate(SH_INTERACTIVE))
			sfprintf(sfstderr,"%s%s\n",msg,dump);
		else
			error(2,gettxt(":337", "%d: %s%s"),pw->p_pid,msg,dump);
	}
}

/*
 * Wait for process pid to complete
 * If pid < -1, then wait can be interrupted, -pid is waited for (wait builtin)
 * pid=0 to unpost all done processes
 * pid=1 to wait for at least one process to complete
 * pid=-1 to wait for all runing processes
 */

void	job_wait(register pid_t pid)
{
	register struct process *pw=0,*px;
	register int	jobid = 0;
	char		intr = 0;
	if(pid <= 0)
	{
		if(pid==0)
			goto done;
		pid = -pid;
		intr = 1;
	}
	job.in_critical = 1;
	if(pid > 1)
	{
		if(!(pw=job_bypid(pid)))
		{
			/* check to see whether job status has been saved */
			if((sh.exitval = job_chksave(pid)) < 0)
				sh.exitval = ERROR_NOENT;
			exitset();
			job.in_critical = 0;
			if(savesig)
				job_waitsafe(savesig);
			return;
		}
		else if(intr && pw->p_env!=sh.curenv)
		{
			sh.exitval = ERROR_NOENT;
			return;
		}
		jobid = pw->p_job;
		if(!intr)
			pw->p_flag &= ~P_EXITSAVE;
		if(pw->p_pgrp && job.parent!= (pid_t)-1)
			job_set(job_byjid(jobid));
	}
#ifdef DEBUG
	sfprintf(sfstderr,"%d: job_wait job=%d pid=%d\n",getpid(),jobid,pid);
	if(pw)
		sfprintf(sfstderr,"%d: job_wait flags=%o\n",getpid(),pw->p_flag);
#endif /* DEBUG*/

	while(1)
	{
		job.in_critical = 1;
		if(job.waitsafe)
		{
			for(px=job.pwlist;px; px = px->p_nxtjob)
			{
				if(px!=pw && (px->p_flag&P_NOTIFY))
				{
					if(sh_isoption(SH_NOTIFY))
					{
						outfile = sfstderr;
						job_list(px,JOB_NFLAG|JOB_NLFLAG);
						sfsync(sfstderr);
					}
					else if(!sh_isoption(SH_INTERACTIVE) && (px->p_flag&P_SIGNALLED))
					{
						job_prmsg(px);
						px->p_flag &= ~P_NOTIFY;
					}
				}
			}
		}
		if(pw && (pw->p_flag&(P_DONE|P_STOPPED)))
		{
#ifdef SIGTSTP
			if(pw->p_flag&P_STOPPED)
			{
				pw->p_flag |= P_EXITSAVE;
				if(sh_isoption(SH_INTERACTIVE) && !sh_isstate(SH_FORKED))
				{
					if( pw->p_exit!=SIGTTIN && pw->p_exit!=SIGTTOU)
						break;

					killpg(pw->p_pgrp,SIGCONT);
				}
				else /* ignore stop when non-interactive */
					pw->p_flag &= ~(P_NOTIFY|P_SIGNALLED|P_STOPPED|P_EXITSAVE);
			}
			else
#endif /* SIGTSTP */
			{
				if(pw->p_flag&P_SIGNALLED)
				{
					pw->p_flag &= ~P_NOTIFY;
					job_prmsg(pw);
				}
				else if(pw->p_flag&P_DONE)
					pw->p_flag &= ~P_NOTIFY;
				if(pw == job_byjid(jobid))
				{
					/* last process in job */
					sh.exitval = pw->p_exit;
					if(pw->p_flag&P_SIGNALLED)
						sh.exitval |= SH_EXITSIG;
					if(intr)
						pw->p_flag &= ~P_EXITSAVE;
				}
				if(!(px=job_unpost(pw,1)) || !job.waitall) 
					break;
				pw = px;
				continue;
			}
		}
		sfsync(sfstderr);
		job.in_critical = !savesig;
		job.waitsafe = 0;
		job_waitsafe(savesig);
		if(job.waitsafe)
			continue;
		if(errno==ECHILD)
			break;
		if(sh.sigflag[SIGALRM]&SH_SIGTRAP)
			sh_timetraps();
		if((intr && sh.trapnote) || (pid==1 && !intr))
			break;
	}
	job.in_critical = 0;
	if(savesig)
		job_waitsafe(savesig);
	if(pid==1)
		return;
	exitset();
	if(pw->p_pgrp)
	{
		job_reset(pw);
		/* propogate keyboard interrupts to parent */
		if((pw->p_flag&P_SIGNALLED) && pw->p_exit==SIGINT && !(sh.sigflag[SIGINT]&SH_SIGOFF))
			sh_fault(SIGINT); 
#ifdef SIGTSTP
		else if((pw->p_flag&P_STOPPED) && pw->p_exit==SIGTSTP)
		{
			job.parent = 0;
			sh_fault(SIGTSTP); 
		}
#endif /* SIGTSTP */
	}
done:
	if(!sh.intrap)
	{
		for(pw=job.pwlist; pw; pw=px)
		{
			px = pw->p_nxtjob;
			job_unpost(pw,0);
		}
	}
}

/*
 * move job to foreground if bgflag == 'f'
 * move job to background if bgflag == 'b'
 * disown job if bgflag == 'd'
 */

job_switch(register struct process *pw,int bgflag)
{
	register const char *msg;
	if(!pw || !(pw=job_byjid((int)pw->p_job)))
		return(1);
	if(bgflag=='d')
	{
		for(; pw; pw=pw->p_nxtproc)
			pw->p_flag |= P_DISOWN;
		return(0);
	}
#ifdef SIGTSTP
	if(bgflag=='b')
	{
		sfprintf(outfile,"[%d]\t",(int)pw->p_job);
		msg = "&";
	}
	else
	{
		job_unlink(pw);
		pw->p_nxtjob = job.pwlist;
		job.pwlist = pw;
		msg = "";
	}
	hist_list(sh.hist_ptr,outfile,pw->p_name,'&',";");
	sfputr(outfile,msg,'\n');
	sfsync(outfile);
	if(bgflag=='f')
	{
		if(!(pw=job_unpost(pw,1)))
			return(1);
		job.waitall = 1;
		job_wait(pw->p_pid);
		job.waitall = 0;
	}
	else if(pw->p_flag&P_STOPPED)
		job_unstop(pw);
#endif /* SIGTSTP */
	return(0);
}


#ifdef SIGTSTP
/*
 * Set the foreground group associated with a job
 */

static void job_fgrp(register struct process *pw, int newgrp)
{
	for(; pw; pw=pw->p_nxtproc)
		pw->p_fgrp = newgrp;
}

/*
 * turn off STOP state of a process group and send CONT signals
 */

static void job_unstop(register struct process *px)
{
	register struct process *pw;
	register int num = 0;
	for(pw=px ;pw ;pw=pw->p_nxtproc)
	{
		if(pw->p_flag&P_STOPPED)
		{
			num++;
			pw->p_flag &= ~(P_STOPPED|P_SIGNALLED|P_NOTIFY);
		}
	}
	if(num!=0)
	{
		if(px->p_fgrp != px->p_pgrp)
			killpg(px->p_fgrp,SIGCONT);
		killpg(px->p_pgrp,SIGCONT);
	}
}
#endif	/* SIGTSTP */

/*
 * remove a job from table
 * If all the processes have not completed, unpost first non-completed  process
 * Otherwise the job is removed and job_unpost returns NULL.
 * pwlist is reset if the first job is removed
 * if <notify> is non-zero, then jobs with pending notifications are unposted
 */

static struct process *job_unpost(register struct process *pwtop,int notify)
{
	register struct process *pw;
	/* make sure all processes are done */
#ifdef DEBUG
	sfprintf(sfstderr,"%d: unpost pid=%d\n",getpid(),pwtop->p_pid);
	sfsync(sfstderr);
#endif /* DEBUG */
	pwtop = pw = job_byjid((int)pwtop->p_job);
	for(; pw && (pw->p_flag&P_DONE)&&(notify||!(pw->p_flag&P_NOTIFY)); pw=pw->p_nxtproc);
	if(pw)
		return(pw);
	/* all processes complete, unpost job */
	job_unlink(pwtop);
	for(pw=pwtop; pw; pw=pw->p_nxtproc)
	{
		/* save the exit status for background jobs */
		if(pw->p_flag&P_EXITSAVE)
		{
			struct jobsave *jp;
			/* save status for future wait */
			if(bck_count++ > sh.lim.child_max)
				job_chksave(0);
			jp = new_of(struct jobsave,0);
			jp->next = bck_list;
			bck_list = jp;
			jp->pid = pw->p_pid;
			jp->env = sh.curenv;
			jp->exitval = pw->p_exit;
			if(pw->p_flag&P_SIGNALLED)
				jp->exitval |= SH_EXITSIG;
			pw->p_flag &= ~P_EXITSAVE;
		}
		pw->p_flag &= ~P_DONE;
		job.numpost--;
		pw->p_nxtjob = freelist;
		freelist = pw;
	}
#ifdef DEBUG
	sfprintf(sfstderr,"%d: free job=%d\n",getpid(),pwtop->p_job);
	sfsync(sfstderr);
#endif /* DEBUG */
	job_free((int)pwtop->p_job);
	return((struct process*)0);
}

/*
 * unlink a job form the job list
 */
static void job_unlink(register struct process *pw)
{
	register struct process *px;
	if(pw==job.pwlist)
	{
		job.pwlist = pw->p_nxtjob;
		job.curpgid = 0;
		return;
	}
	for(px=job.pwlist;px;px=px->p_nxtjob)
		if(px->p_nxtjob == pw)
		{
			px->p_nxtjob = pw->p_nxtjob;
			return;
		}
}

/*
 * get an unused job number
 * freejobs is a bit vector, 0 is unused
 */

static int job_alloc(void)
{
	register int j=0;
	register unsigned mask = 1;
	register unsigned char *freeword;
	register int jmax = BYTE(sh.lim.child_max);
	/* skip to first word with a free slot */
	for(j=0;job.freejobs[j] == UCHAR_MAX; j++);
	if(j >= jmax)
	{
		register struct process *pw;
		for(j=1; j < sh.lim.child_max; j++)
		{
			if((pw=job_byjid(j))&& !job_unpost(pw,0))
				break;
		}
		if (j == sh.lim.child_max) {
			for(j=1; j < sh.lim.child_max; j++)
			{
				if((pw=job_byjid(j))&& !job_unpost(pw,1))
					break;
			}
		}
		if(j == sh.lim.child_max)
			return(-1);
		j--;
		j /= CHAR_BIT;
	}
	freeword = &job.freejobs[j];
	j *= CHAR_BIT;
	for(j++;mask&(*freeword);j++,mask <<=1);
	*freeword  |= mask;
	return(j);
}

/*
 * return a job number
 */

static void job_free(register int n)
{
	register int j = (--n)/CHAR_BIT;
	register unsigned mask;
	n -= j*CHAR_BIT;
	mask = 1 << n;
	job.freejobs[j]  &= ~mask;
}

static char *job_sigmsg(int sig)
{
	static char signo[20];
#ifdef apollo
	/*
	 * This code handles the formatting for the apollo specific signal
	 * SIGAPOLLO. 
	 */
	extern char *apollo_error(void);
	
	if ( sig == SIGAPOLLO )
		return( apollo_error() );
#endif /* apollo */
	if(sig<sh.sigmax && sh.sigmsg[sig])
		return(sh.sigmsg[sig]);
#if defined(SIGRTMIN) && defined(SIGRTMAX)
	if(sig>=SIGRTMIN && sig<=SIGRTMAX)
	{
		static char sigrt[20];
		sfsprintf(sigrt,sizeof(sigrt),"SIGRTMIN+%d",sig-SIGRTMIN);
		return(sigrt);
	}
#endif
	sfsprintf(signo,sizeof(signo),gettxt(":209","Signal %.4s"),fmtbase((long)sig,10,0));
	return(signo);
}

/*
 * see whether exit status has been saved and delete it
 * if pid==0, then oldest saved process is deleted
 * If pid is not found a -1 is returned.
 */
int job_chksave(register pid_t pid)
{
	register struct jobsave *jp = bck_list, *jpold=0;
	register int r= -1;
	while(jp)
	{
		if(jp->pid==pid)
			break;
		if(pid==0 && !jp->next)
			break;
		jpold = jp;
		jp = jp->next;
	}
	if(jp)
	{
		r = 0;
		if(pid)
		{
			if(jp->env!=sh.curenv)
				return(-1);
			r = jp->exitval;
		}
		if(jpold)
			jpold->next = jp->next;
		else
			bck_list = jp->next;
		bck_count--;
		free((char*)jp);
	}
	return(r);
}
