#ident	"@(#)debugger:libexecon/i386/oldproc.C	1.20"

#if OLD_PROC

#include "Procctl.h"
#include "ProcFollow.h"
#include "Proctypes.h"
#include "Process.h"
#include "PtyList.h"
#include "global.h"
#include "Interface.h"
#include "utility.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/reg.h>
#include "sys/regset.h"
#include <sys/procfs.h>

#include <stdio.h>

struct Procdata {
	pid_t		pid;
	short		check_stat;
	short		follow_children;
	int		number_of_sigs;
	int		num_segments;
	int		map_size;
	map_ctl		*seg_map;
	fpregset_t	*fpregs;
	dbregset_t	*dbregs;
	struct sigaction *sact;
	prstatus	prstat;
	prrun_t		prrun;
	char		*filename;
			Procdata() { memset(this, 0, sizeof(*this)); }
			~Procdata();
};

Procdata::~Procdata()
{
	delete filename;
	delete sact;
	delete seg_map;
	delete fpregs;
	delete dbregs;
}

Proclive::Proclive()
{
	data = new Procdata;
	fd = -1;
	follower = 0;
}

Proclive::~Proclive()
{
	delete ((Procdata *)data);
}

pid_t
Proclive::create(const char *cmdline, const char *ename,
	int redir, int input, int output, PtyInfo *child_io)
{
	pid_t		ppid;

	if ( (ppid = fork()) == 0 )
	{
		// New child
		// Set up to stop on exec, set up I/O redirection
		// and exec new command.

		char	*execline = new char[ sizeof("exec ./") +
				strlen(cmdline)];
		// if no path specified, always look in 
		// current directory
		if (strchr(ename, '/') == 0)
			strcpy( execline, "exec ./" );
		else
			strcpy( execline, "exec " );
		strcat( execline, cmdline );
		stop_self_on_exec();
		if (redir & REDIR_PTY)
			// redirect I/O to pseudo-tty
			redirect_childio( child_io->pt_fd() );
		if (input >= 0)
		{
			if (redir & REDIR_IN)
			{
				/* save close-on-exec status */
				int	fs = fcntl(0, F_GETFD, 0);
				::close(0);
				fcntl(input, F_DUPFD, 0);
				if (fs == 1)
					fcntl(0, F_SETFD, 1);
			}
			::close(input);
		}
		if (output >= 0)
		{
			if (redir & REDIR_OUT)
			{
				/* save close-on-exec status */
				int	fs = fcntl(1, F_GETFD, 0);
				::close(1);
				fcntl(output, F_DUPFD, 1);
				if (fs == 1)
					fcntl(1, F_SETFD, 1);
			}
			::close(output);
		}
		// turn off lazy binding so we don't need 
		// to go through rtld binding routines
		putenv("LD_BIND_NOW=1");
		// restore signal mask we entered debugger with
		sigprocmask(SIG_SETMASK, &orig_mask, 0);
		// reset disposition of signals debugger itself
		// ignores
		signal(SIGQUIT, SIG_DFL);
		signal(SIGALRM, SIG_DFL);
		signal(SIGCLD, SIG_DFL);

		execlp( "/usr/bin/sh", "sh", "-c", execline, 0 );
		// if here, exec failed
		DPRINT(DBG_PROC, ("Proclive::create: exec of %s failed\n",execline));
		delete child_io;
		_exit(1);  // use _exit to avoid flushing buffers
	}
	else if ( ppid == -1 )
	{
		printe(ERR_fork_failed, E_ERROR, strerror(errno));
		return 0;
	}
	if ( !open(ppid, 1) )
	{
		printe(ERR_proc_unknown, E_ERROR, ppid);
		::kill(ppid, SIGKILL);
		return 0;
	}

	// now run process until shell execs successfully
	// we expect two successful returns from exec:
	// one for the shell, itself and one for the target
	// program
	int	exec_cnt = 2;

	set_check();
	for(;;)
	{
		int	what, why;
		if (wait_for(what, why, 0) == p_dead)
		{
			::kill(ppid, SIGKILL);
			return 0;
		}
		if ((why == PR_SYSEXIT) &&
			((what == SYS_exec) || (what == SYS_execve)))
		{
			// process has executed exec - check value of
			// %eax - if not zero, exec failed; shell
			// is doing path searching
			if (((Procdata *)data)->prstat.pr_reg.greg[R_EAX] == 0)
				exec_cnt--;
		}
		if (!exec_cnt)
			break;
		run(0, follow_no);
	}
	return(ppid);
}

int
Proclive::open(pid_t pid, int is_child)
{
	char	path[sizeof("/proc/") + MAX_LONG_DIGITS];
	sprintf( path, "/proc/%d", pid );
	return open(path, pid, is_child);
}

int
Proclive::open(const char *path, pid_t pid, int is_child)
{
	static	char *self_path;

	if (!self_path)
	{
		self_path = new char[sizeof("/proc/") + MAX_LONG_DIGITS];
		sprintf(self_path, "/proc/%d", getpid());
	}

	if (strcmp(self_path, path) == 0)
	{
		printe(ERR_self_ctl, E_ERROR);
		return 0;
	}

	do {
		errno = 0;
		fd = debug_open( path, O_RDWR );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	if (is_child == 0)
	{
		// grabbed process - set run on last close
		do {
			errno = 0;
			ioctl( fd, PIOCSRLC, 0);
		} while ( errno == EINTR );
		if (errno)
		{
			::close(fd);
			return 0;
		}
	}
	((Procdata *)data)->filename = new char[strlen(path)+1];
	strcpy(((Procdata *)data)->filename, path);
	((Procdata *)data)->pid = pid;
	((Procdata *)data)->prrun.pr_flags = PRSTRACE;
	premptyset(&(((Procdata *)data)->prrun.pr_trace));
	((Procdata *)data)->check_stat = 1;
	follower = new ProcFollow;
	if (!follower->initialize(((Procdata *)data)->filename) ||!default_traps())
	{
		close();
		return 0;
	}
	return 1;
}

void
Proclive::close()
{
	if (fd > 0)
	{
		// clear inherit-on-fork
		do {
			errno = 0;
			ioctl( fd, PIOCRFORK, 0 );
		} while ( errno == EINTR );
		do {
			errno = 0;
			::close( fd );
		} while ( errno == EINTR );
		fd = -1;
	}
	delete follower;
}

int
Proclive::err_handle(int err)
{
	if (!err)
		return 1;
	if (err == EAGAIN)
	{
		int	nfd;

		/* Can't control process;
		 * it may have exec'd a set-uid or set-gid program.
		 * Try re-opening proc file in case we are running
		 * as super-user and can regain control.
		 * Order is important here: must try to re-open
		 * proc file before closing old file descriptor.
		 * If re-open fails, close will take process out of
		 * our control and set it running, before we
		 * have a chance to kill it.
		 */

		do {
			errno = 0;
			nfd = debug_open(((Procdata *)data)->filename, O_RDWR);
		} while (errno == EINTR);
		if (nfd == -1)
		{
			printe(ERR_proc_setid, E_ERROR, ((Procdata *)data)->pid);
			return 0;
		}
		::close(fd);
		fd = nfd;
		if (((Procdata *)data)->follow_children)
		{
			// set inherit-on-fork
			do {
				errno = 0;
				ioctl( fd, PIOCSFORK, 0);
			} while ( errno == EINTR );
			if (errno)
			{
				close();
				printe(ERR_proc_setid, E_ERROR, 
					((Procdata *)data)->pid);
				return 0;
			}
		}
		return 1;
	}
	switch(err)
	{
	case EINTR:
	case ENOENT:	
			break;
	case EIO: /*FALLTHROUGH*/
	case EFAULT:	
			printe(ERR_proc_io, E_ERROR, ((Procdata *)data)->pid);
			break;
	default:
			printe(ERR_proc_unknown, E_ERROR, ((Procdata *)data)->pid);
	}
	return 0;
}

// set inherit on fork
int
Proclive::follow_children()
{
	((Procdata *)data)->follow_children = 1;
	do {
		errno = 0;
		ioctl( fd, PIOCSFORK, 0);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return(errno == 0);
}

int 
Proclive::run(int sig, follower_mode follow)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::run",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	((Procdata *)data)->prrun.pr_flags |= PRCFAULT;
	((Procdata *)data)->prrun.pr_flags &= ~PRSTEP;
	if (sig == 0)
		((Procdata *)data)->prrun.pr_flags |= PRCSIG;
	else
		((Procdata *)data)->prrun.pr_flags &= ~PRCSIG;
	do {
		errno = 0;
		ioctl(fd, PIOCRUN, &((Procdata *)data)->prrun);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (!errno)
	{
		if (follow != follow_no)
			follower->start_follow();
		return 1;
	}
	return 0;
}

int 
Proclive::step(int sig, follower_mode follow)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::step",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	((Procdata *)data)->prrun.pr_flags |= PRCFAULT|PRSTEP;
	if (sig == 0)
		((Procdata *)data)->prrun.pr_flags |= PRCSIG;
	else
		((Procdata *)data)->prrun.pr_flags &= ~PRCSIG;
	do {
		errno = 0;
		ioctl(fd, PIOCRUN, &((Procdata *)data)->prrun);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (!errno)
	{
		if (follow != follow_no)
			follower->start_follow();
		return 1;
	}
	return 0;
}

int 
Proclive::stop()
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::stop",
			__LINE__);
		return 0;
	}
	do {
		errno = 0;
		ioctl(fd, PIOCSTOP, &((Procdata *)data)->prstat);
	} while(errno && (errno != EINTR) && err_handle(errno));
	if (!errno)
	{
		((Procdata *)data)->check_stat = 0;
		return 1;
	}
	if (errno == EINTR)
	{
		printe(ERR_stop_intr, E_WARNING, ((Procdata *)data)->pid);
	}
	((Procdata *)data)->check_stat = 1;
	return 0;
}

Procstat 
Proclive::wait_for(int &what, int &why, int allow_interrupt)
{
	int	err;
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::wait_for",
			__LINE__);
		return p_unknown;
	}
	for(;;)
	{
		err = 0;
		if (allow_interrupt)
			// allow poll here - may have output in code
			// we are waiting for
			sigrelse(SIGPOLL);
		if (prismember(&interrupt, SIGPOLL))
			prdelset(&interrupt, SIGPOLL);
		if (ioctl(fd, PIOCWSTOP, &((Procdata *)data)->prstat) == -1)
			err = errno;
		if (allow_interrupt)
			sighold(SIGPOLL);
		if (err == 0)
			break;
		else if (err == EINTR)
		{
			if (allow_interrupt)
			{
				if (prismember(&interrupt, SIGPOLL))
				{
					prdelset(&interrupt, SIGPOLL);
					continue;
				}
				break;
			}
		}
		else if (!err_handle(err))
			break;
	}
	if (err == 0 || err == EINTR)
	{
		what = ((Procdata *)data)->prstat.pr_what;
		why = ((Procdata *)data)->prstat.pr_why;
		((Procdata *)data)->check_stat = 0;
		return p_stopped;
	}
	((Procdata *)data)->check_stat = 1;
	return p_dead;
}

Procstat 
Proclive::status(int &what, int &why)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::status",
			__LINE__);
		return p_unknown;
	}
	if (((Procdata *)data)->check_stat)
	{
		((Procdata *)data)->num_segments = 0;
		do {
			errno = 0;
			ioctl(fd, PIOCSTATUS, &((Procdata *)data)->prstat);
		} while(errno == EINTR);
		if (errno)
			return p_dead;
	}
	if (((Procdata *)data)->prstat.pr_flags & PR_STOPPED)
	{
		what = ((Procdata *)data)->prstat.pr_what;
		why = ((Procdata *)data)->prstat.pr_why;
		((Procdata *)data)->check_stat = 0;
		return p_stopped;
	}
	return p_running;
}

int 
Proclive::kill(int signo)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::kill",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCKILL, &signo);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// uses pending set, not current signal
int
Proclive::cancel_sig( int signo )
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::cancel_sig",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl( fd, PIOCUNKILL, &signo );
	} while(errno && ((errno == EINTR) || 
		err_handle(errno)));
	return ( errno == 0 );
}

// cancel current signal
int
Proclive::cancel_current()
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::cancel_current",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl( fd, PIOCSSIG, 0 );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return ( errno == 0 );
}

int
Proclive::pending_sigs(sig_ctl *sigs, int)
{
	int	what, why;

	if (status(what, why) == p_dead)
		return 0;
	memcpy(&sigs->signals, &((Procdata *)data)->prstat.pr_sigpend, 
		sizeof(sigset_t));
	return 1;
}

int
Proclive::current_sig(int &sig)
{
	int	what, why;

	if (status(what, why) == p_dead)
		return 0;
	sig = ((Procdata *)data)->prstat.pr_cursig;
	return 1;
}

int
Proclive::trace_sigs(sig_ctl *sigs)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::trace_sigs",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	((Procdata *)data)->prrun.pr_trace = sigs->signals;
	do {
		errno = 0;
		ioctl(fd, PIOCSTRACE, &sigs->signals);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// cancel current fault
int
Proclive::cancel_fault()
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::cancel_fault",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl( fd, PIOCCFAULT, 0 );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return ( errno == 0 );
}

int
Proclive::trace_traps(flt_ctl *flt)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::trace_traps",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCSFAULT, &flt->faults);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::sys_entry(sys_ctl *sys)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::sys_entry",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCSENTRY, &sys->scalls);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::sys_exit(sys_ctl *sys)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::sys_exit",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCSEXIT, &sys->scalls);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

const char *
Proclive::psargs()
{
	static prpsinfo_t	psinfo;
	do {
		errno = 0;
		ioctl(fd, PIOCPSINFO, &psinfo);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return((errno == 0) ? psinfo.pr_psargs : 0);
}

int
Proclive::read(Iaddr from, void *to, int len)
{
	int	result;
	do {
		errno = 0;
		lseek(fd, from, SEEK_SET);
	} while(errno == EINTR);
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::read(fd, to, len);
	} while(errno == EINTR);
	return result;
}

int
Proclive::write(Iaddr to, const void *from, int len)
{
	int	result;
	do {
		errno = 0;
		lseek(fd, to, SEEK_SET);
	} while(errno == EINTR);
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::write(fd, from, len);
	} while(errno == EINTR);
	return result;
}

int
Proclive::update_stack(Iaddr &lo, Iaddr &hi)
{
	int	num; // number of mapped segments
	int 	i;

	prmap_t	*pmap;

	pmap = (prmap_t *)seg_map(num);

	if (!pmap)
		return 0;
	
	for(i = 0; i <= num; i++, pmap++)
		if (pmap->pr_mflags & MA_STACK)
			break;
	
	if (i >= num)
	{
		return 0;
	}
	else
	{
		lo = (Iaddr)(pmap->pr_vaddr);
		hi = (Iaddr)(pmap->pr_vaddr + pmap->pr_size);
	}
	return 1;
}

int
Proclive::open_object(Iaddr addr, const char *)
{
	int	fdobj;
	caddr_t	*p;
	p = ( addr == 0 ) ? 0 : (caddr_t*)&addr;
	do {
		errno = 0;
		fdobj = ioctl( fd, PIOCOPENM, p );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return fdobj;
}

gregset_t *
Proclive::read_greg()
{
	int	what, why;

	if (status(what, why) != p_stopped)
		return 0;
	return &((Procdata *)data)->prstat.pr_reg;
}

int
Proclive::write_greg(gregset_t *greg)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::write_greg",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		ioctl( fd, PIOCSREG, greg );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

fpregset_t *
Proclive::read_fpreg()
{
	if (!((Procdata *)data)->fpregs)
		((Procdata *)data)->fpregs = new fpregset_t;
	do {
		errno = 0;
		ioctl( fd, PIOCGFPREG, ((Procdata *)data)->fpregs);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno != 0)
		return 0;
	return ((Procdata *)data)->fpregs;
}

int
Proclive::write_fpreg(fpregset_t *fpreg)
{
	do {
		errno = 0;
		ioctl( fd, PIOCSFPREG, fpreg);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

dbregset_t *
Proclive::read_dbreg()
{
	if (!((Procdata *)data)->dbregs)
		((Procdata *)data)->dbregs = new dbregset_t;
	do {
		errno = 0;
		ioctl( fd, PIOCGDBREG, ((Procdata *)data)->dbregs );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno != 0)
		return 0;
	return ((Procdata *)data)->dbregs;
}

int
Proclive::write_dbreg(dbregset_t *dbreg)
{
	do {
		errno = 0;
		ioctl( fd, PIOCSDBREG, dbreg );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

map_ctl *
Proclive::seg_map(int &no)
{

	if (check_stat || !((Procdata *)data)->num_segments)
	{
		int	num;

		do {
			errno = 0;
			ioctl( fd, PIOCNMAP, &num);
		} while(errno && ((errno == EINTR) ||
			err_handle(errno)));
		if (errno)
		{
			((Procdata *)data)->num_segments = 0;
			return 0;
		}
		((Procdata *)data)->num_segments = num;
		int	sz = (num + 1) * sizeof(map_ctl);
		if (sz != ((Procdata *)data)->map_size)
		{
			((Procdata *)data)->map_size = sz;
			delete ((Procdata *)data)->seg_map;
			((Procdata *)data)->seg_map = new map_ctl[num+1];
		}
		do {
			errno = 0;
			ioctl( fd, PIOCMAP, ((Procdata *)data)->seg_map);
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (errno != 0)
		{
			((Procdata *)data)->num_segments = 0;
			return 0;
		}
	}
	no = ((Procdata *)data)->num_segments;
	return ((Procdata *)data)->seg_map;
}

// set up to trap single steps and breakpoints
int
Proclive::default_traps()
{
#if STOP_TYPE == STOP_SIGNALLED
	return 1;
#else
	flt_ctl	fs;

	premptyset(&fs.faults);
	praddset(&fs.faults, TRACEFAULT);  // trace trap - for single step
	praddset(&fs.faults, BKPTFAULT); // machine specific trap for brkpt
	if (!trace_traps(&fs))
	{
		return 0;
	}
	return 1;
#endif
}


// read and return sigaction structures for all signals

int
Proclive::get_sig_disp(struct sigaction *&act, int &num_sigs)
{
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::get_sig_disp",
			__LINE__);
		return 0;
	}
	if (!((Procdata *)data)->sact)
	{
		do {
			errno = 0;
			ioctl( fd, PIOCMAXSIG, &((Procdata *)data)->number_of_sigs);
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (errno != 0)
			return 0;
		((Procdata *)data)->sact = 
			new(struct sigaction[((Procdata *)data)->number_of_sigs]);
	}
	do {
		errno = 0;
		ioctl( fd, PIOCACTION, ((Procdata *)data)->sact);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno != 0)
	{
		num_sigs = 0;
		act = 0;
		return 0;
	}
	num_sigs = ((Procdata *)data)->number_of_sigs;
	act = ((Procdata *)data)->sact;
	return 1;
}

// setup for release of process;
// if grabbed and released suspended, reset run on last close
// if created and released running, set run on last close
int
Proclive::release(int run, int is_child)
{
	int cmd;
	if (run)
	{
		if (is_child)
		{
			cmd = PIOCSRLC;
		}
		else
			return 1;
	}
	else if (is_child == 0)
	{
		cmd = PIOCRRLC;
	}
	else
		return 1;
	do {
		errno = 0;
		ioctl( fd, cmd, 0);
	} while ( errno == EINTR );
	return (errno == 0);
}

int
Proclive::destroy(int)
{
	return(kill(SIGKILL));
}

int
stop_self_on_exec()
{
	pid_t		pid;
	sysset_t	sxset;
	char		filename[sizeof("/proc/") + MAX_LONG_DIGITS];
	int		fd;
	int		err;

	do {
		errno = 0;
		pid = getpid();
	} while ( errno == EINTR );
	if (errno)
		return 0;
	sprintf( filename, "/proc/%d", pid);
	do {
		errno = 0;
		fd = ::open( filename, O_RDWR );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	premptyset( &sxset);
	praddset( &sxset, SYS_exec );
	praddset( &sxset, SYS_execve );
	do {
		errno = 0;
		ioctl(fd, PIOCSEXIT, &sxset);
	} while(errno == EINTR);
	err = errno;
	close(fd);
	return (err == 0);
}

int
live_proc(const char *path)
{
	prstatus	p;
	int		fd;

	do {
		errno = 0;
		fd = open(path, O_RDONLY);
	} while(errno == EINTR);
	if (errno != 0)
		return 0;
	do {
		errno = 0;
		ioctl(fd, PIOCSTATUS, &p);
	} while(errno == EINTR);
	return (errno == 0);
}

int
release_child(pid_t pid)
{
	char		filename[sizeof("/proc/") + MAX_LONG_DIGITS];
	int		fd;
	int		err;

	sprintf( filename, "/proc/%d", pid);
	do {
		errno = 0;
		fd = open( filename, O_RDWR );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	
	do {
		errno = 0;
		ioctl(fd, PIOCSRLC);
	} while(errno == EINTR);
	err = errno;
	close(fd);
	return (err == 0);
}

#endif
