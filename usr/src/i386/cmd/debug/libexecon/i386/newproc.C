#ident	"@(#)debugger:libexecon/i386/newproc.C	1.34"

#if !defined(OLD_PROC) && !defined(PTRACE)

#include "Procctl.h"
#include "Proctypes.h"
#include "ProcFollow.h"
#include "PtyList.h"
#include "Process.h"
#include "Machine.h"
#include "global.h"
#include "Interface.h"
#include "utility.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/reg.h>
#include "sys/regset.h"
#include <sys/procfs.h>
#include <sys/procfs_f.h>
#include <sys/stat.h>

#ifdef DEBUG_THREADS
struct Lwpdata {
	short		follow_children;
	lwpid_t		lid;
	int		status_fd;
	lwpstatus_t	lwpstat;
	char		*filename;
			Lwpdata() { memset(this, 0, sizeof(*this)); }
			~Lwpdata() { delete filename; }
};


#endif

struct Procdata {
	short		follow_children;
	short		num_segs;
	short		map_size;
	short		number_of_sigs;
	pid_t		pid;
	int		as_fd;
	int		map_fd;
	int		status_fd;
	int		sigact_fd;
	struct sigaction *sact;
	map_ctl		*seg_map;
	char		*filename;
	pstatus_t	prstat;
			Procdata() { memset(this, 0, sizeof(*this)); }
			~Procdata();
};

Procdata::~Procdata()
{
	delete filename;
	delete sact;
	delete seg_map;
}

// message format for control operations
struct	proc_msg {
	int		msg;
	unsigned long	data;
};

static void
open_failed(const char *fname, int err)
{
	printe(ERR_cant_open, E_ERROR, fname, strerror(errno));
}

Proclive::Proclive()
{
	DPRINT(DBG_PROC, ("Proclive::Proclive()\n"));
	data = 0;
#ifdef DEBUG_THREADS
	parent = 0;
#endif
	check_stat = 0;
	follower = 0;
	poll_index = -1;
	fd = -1;
}

Proclive::~Proclive()
{
	DPRINT(DBG_PROC, ("Proclive::~Proclive(this==%#x)\n",this));
	delete ((Procdata *)data);
	data = 0;
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
			if (((Procdata *)data)->prstat.pr_lwp.pr_context.uc_mcontext.gregs.greg[R_EAX] == 0)
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
	DPRINT(DBG_PROC, ("Proclive::open(this==%#x, pid==%d,)\n",this, pid));

	char	path[sizeof("/proc/") + MAX_LONG_DIGITS];
	sprintf( path, "/proc/%d", pid );
	return open(path, pid, is_child);
}

// common routine to open /proc files for a process
// assumes path is big enough to hold largest file name
static int
open_files(char *filebuf, int pathlen, int &ctl_fd, int &as_fd, 
	int &status_fd, int &map_fd)
{
	char	*strend;
	static	char *self_path;

	if (!self_path)
	{
		self_path = new char[sizeof("/proc/") + MAX_LONG_DIGITS];
		sprintf(self_path, "/proc/%d", getpid());
	}

	if (strcmp(self_path, filebuf) == 0)
	{
		printe(ERR_self_ctl, E_ERROR);
		return 0;
	}

	strend = filebuf + pathlen;

	strcpy(strend, "/ctl");
	do {
		errno = 0;
		ctl_fd = debug_open( filebuf, O_WRONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		open_failed(filebuf, errno);
		*strend = 0;
		return 0;
	}
	strcpy(strend, "/as");
	do {
		errno = 0;
		as_fd = debug_open( filebuf, O_RDWR );
	} while ( errno == EINTR );
	if (errno)
	{
		open_failed(filebuf, errno);
		*strend = 0;
		return 0;
	}
	strcpy(strend, "/status");
	do {
		errno = 0;
		status_fd = debug_open( filebuf, O_RDONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		open_failed(filebuf, errno);
		*strend = 0;
		return 0;
	}
	strcpy(strend, "/map");
	do {
		errno = 0;
		map_fd = debug_open( filebuf, O_RDONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		open_failed(filebuf, errno);
		*strend = 0;
		return 0;
	}
	*strend = 0;
	return 1;
}

// this version of open allows for a pathname so we can
// use remotely mounted /proc directories
int
Proclive::open(const char *path, pid_t pid, int is_child)
{
	char		*filebuf;
	int		len = strlen(path);
	proc_msg	fmsg;

	DPRINT(DBG_PROC, ("Proclive::open(this==%#x, path==%s,pid==%d)\n",this, path, pid));

	data = new Procdata;
	filebuf  = new char[len + sizeof("/status")];
	strcpy(filebuf, path);

	if (!open_files(filebuf, len, fd, ((Procdata *)data)->as_fd, 
		((Procdata *)data)->status_fd, ((Procdata *)data)->map_fd))
	{
		goto open_out;
	}

	// set process flags
	fmsg.msg = PCSET;
	if (is_child != 0)
		// created process - set kill on last close
		fmsg.data = PR_KLC;
	else
		// grabbed process - set run on last close
		fmsg.data = PR_RLC;

	do {
		errno = 0;
		::write( fd, (char *)&fmsg, sizeof(proc_msg) );
	} while ( errno == EINTR );
	if (errno)
	{
		DPRINT(DBG_PROC, ("Proclive::open - set flags :errno==%d\n", errno));
		goto open_out;
	}
	((Procdata *)data)->pid = pid;
	check_stat = 1;
	follower = new ProcFollow;
#ifdef FOLLOWER_PROC
	if (!follower->initialize(filebuf) ||
#else
	if (!follower->initialize(((Procdata *)data)->pid) ||
#endif
		!default_traps())
	{
		goto open_out;
	}
	((Procdata *)data)->filename = filebuf;
	return 1;
open_out:
	if (fd > 0)
		::close(fd);
	if (((Procdata *)data)->as_fd > 0)
		::close(((Procdata *)data)->as_fd);
	if (((Procdata *)data)->map_fd > 0)
		::close(((Procdata *)data)->map_fd);
	if (((Procdata *)data)->status_fd > 0)
		::close(((Procdata *)data)->status_fd);
	fd = ((Procdata *)data)->as_fd = ((Procdata *)data)->map_fd = ((Procdata *)data)->status_fd = -1;
	delete filebuf;
	delete follower;
	follower = 0;
	return 0;
}

void
Proclive::close()
{
	DPRINT(DBG_PROC, ("Proclive::close(this==%#x)\n",this));
	if (fd != -1)
	{
		// clear inherit-on-fork
		proc_msg	fmsg;
		fmsg.msg = PCRESET;
		fmsg.data = PR_FORK;
		do {
			errno = 0;
			::write( fd, (char *)&fmsg, sizeof(proc_msg) );
		} while(errno == EINTR);
		::close( fd );
		::close( ((Procdata *)data)->as_fd );
		::close( ((Procdata *)data)->status_fd );
		::close( ((Procdata *)data)->map_fd );
		if (((Procdata *)data)->sigact_fd > 0)
			::close(((Procdata *)data)->sigact_fd);
		fd = ((Procdata *)data)->as_fd = ((Procdata *)data)->status_fd = ((Procdata *)data)->map_fd 
			= ((Procdata *)data)->sigact_fd = -1;
	}
	delete follower;
	follower = 0;
}

// returns 1 if error is recoverable (exec a setuid program that
// we can recontrol), else 0
int
Proclive::err_handle(int err)
{
	DPRINT(DBG_PROC, ("Proclive::err_handle(this==%#x, err==%d)\n",this, err));
	int	len;
	int	nfd, nas_fd, nstatus_fd, nmap_fd;

	switch(err)
	{
	case 0:
		return 1;
	default:
		printe(ERR_proc_unknown, E_ERROR, ((Procdata *)data)->pid);
		break;
	case EINTR:
	case ENOENT:	
		break;
	case EIO: /*FALLTHROUGH*/
	case EFAULT:	
		printe(ERR_proc_io, E_ERROR, ((Procdata *)data)->pid);
		break;
	case EBADF:
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

		len = strlen(((Procdata *)data)->filename);
		if (!open_files(((Procdata *)data)->filename, len, nfd, nas_fd, 
			nstatus_fd, nmap_fd))
			goto err_out;

		::close(fd);
		::close(((Procdata *)data)->as_fd);
		::close(((Procdata *)data)->status_fd);
		::close(((Procdata *)data)->map_fd);
		if (((Procdata *)data)->sigact_fd > 0)
		{
			::close(((Procdata *)data)->sigact_fd);
			((Procdata *)data)->sigact_fd = -1;
		}
		fd = nfd;
		((Procdata *)data)->as_fd = nas_fd;
		((Procdata *)data)->status_fd = nstatus_fd;
		((Procdata *)data)->map_fd = nmap_fd;
		if (((Procdata *)data)->follow_children)
		{
			// set inherit-on-fork
			proc_msg	fmsg;
			fmsg.msg = PCSET;
			fmsg.data = PR_FORK;
			do {
				errno = 0;
				::write(fd, (char *)&fmsg, 
					sizeof(proc_msg));
			} while ( errno == EINTR );
			if (errno)
			{
				close();
				printe(ERR_proc_setid, E_ERROR,
					((Procdata *)data)->pid);
				break;
			}
		}
		return 1;
	err_out:
		if (nfd > 0)
			::close(nfd);
		if (nas_fd > 0)
			::close(nas_fd);
		if (nstatus_fd > 0)
			::close(nstatus_fd);
		if (nmap_fd > 0)
			::close(nmap_fd);
		printe(ERR_proc_setid, E_ERROR, ((Procdata *)data)->pid);
		break;
	}
	return 0;
}

// set inherit on fork
int
Proclive::follow_children()
{
	proc_msg	fmsg;

	fmsg.msg = PCSET;
	fmsg.data = PR_FORK;
	((Procdata *)data)->follow_children = 1;
	do {
		errno = 0;
		::write( fd, (char *)&fmsg, sizeof(proc_msg) );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return(errno == 0);
}

// set process running
int 
Proclive::run(int sig, follower_mode follow)
{
	DPRINT(DBG_PROC, ("Proclive::run(this==%#x)\n",this));
	proc_msg	rmsg;
	rmsg.msg = PCRUN;
	rmsg.data = PRCFAULT;
	if (sig == 0)
		rmsg.data |= PRCSIG;
	set_check();
	do {
		errno = 0;
		::write(fd, (char *)&rmsg, sizeof(proc_msg));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (!errno)
	{
		// add to list of follower; if follow is follow_yes
		// also starts follower
		if (follow != follow_no)
#ifdef FOLLOWER_PROC
			follower->start_follow();
#else
			poll_index = follower->add(fd, follow);
#endif
		return 1;
	}
	return 0;
}

// single step process
int 
Proclive::step(int sig, follower_mode follow)
{
	DPRINT(DBG_PROC, ("Proclive::step(this==%#x)\n",this));
	proc_msg	rmsg;
	rmsg.msg = PCRUN;
	rmsg.data = PRCFAULT|PRSTEP;
	if (sig == 0)
		rmsg.data |= PRCSIG;
	set_check();
	do {
		errno = 0;
		::write(fd, (char *)&rmsg, sizeof(proc_msg));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (!errno)
	{
		// add to list of follower; if follow is follow_yes
		// also starts follower
		if (follow != follow_no)
#ifdef FOLLOWER_PROC
			follower->start_follow();
#else
			poll_index = follower->add(fd, follow);
#endif
		return 1;
	}
	return 0;
}


// stop process and wait for it to stop
// for stop we do not retry if interrupted; this is because
// of interactions with vfork
int 
Proclive::stop()
{
	DPRINT(DBG_PROC, ("Proclive::stop(this==%#x)\n",this));
	DPRINT(DBG_PROC, ("Proclive::stop ctl fd == %d\n", fd));
	int	msg = PCSTOP;
	do {
		errno = 0;
		::write(fd, (char *)&msg, sizeof(int));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (!errno)
	{
		int	what, why;
		if (status(what, why) == p_dead)
			return 0;
		check_stat = 0;
		return 1;
	}
	if (errno == EINTR)
	{
		printe(ERR_stop_intr, E_WARNING, ((Procdata *)data)->pid);
	}
	set_check();
	return 0;
}

// suspend until process stops
Procstat 
Proclive::wait_for(int &what, int &why, int allow_interrupt)
{
	DPRINT(DBG_PROC, ("Proclive::wait_for(this==%#x)\n",this));

	int	msg = PCWSTOP;
	int	err;
	for(;;)
	{
		// allow break-out if allow_interrupt is set
		err = 0;
		if (allow_interrupt)
			// allow poll here - may have output in code
			// we are waiting for
			sigrelse(SIGPOLL);
		if (prismember(&interrupt, SIGPOLL))
			prdelset(&interrupt, SIGPOLL);
		if (::write(fd, (char *)&msg, sizeof(int)) == -1)
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
		return(status(what, why));
	}
	set_check();
	return p_dead;
}

Procstat 
Proclive::status(int &what, int &why)
{
	DPRINT(DBG_PROC, ("Proclive::status(this==%#x)\n",this));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, 
			"Proclive::status", __LINE__);
		return p_unknown;
	}
	if (check_stat)
	{
		((Procdata *)data)->num_segs = 0;
		do {
			errno = 0;
			lseek(((Procdata *)data)->status_fd, 0, SEEK_SET);
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (errno)
			return p_dead;
		do {
			errno = 0;
			::read(((Procdata *)data)->status_fd, (char *)&(((Procdata *)data)->prstat), 
				sizeof(pstatus_t));
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (errno)
			return p_dead;
	}

	DPRINT(DBG_PROC, ("Proclive::status() - flags == 0x%x, what == %d,why == %d, pc == 0x%x\n",((Procdata *)data)->prstat.pr_lwp.pr_flags,((Procdata *)data)->prstat.pr_lwp.pr_what, ((Procdata *)data)->prstat.pr_lwp.pr_why,((Procdata *)data)->prstat.pr_lwp.pr_context.uc_mcontext.gregs.greg[EIP]));

	if ((((Procdata *)data)->prstat.pr_lwp.pr_flags & 
		(PR_STOPPED|PR_ISTOP)) == (PR_STOPPED|PR_ISTOP))
	{
		what = ((Procdata *)data)->prstat.pr_lwp.pr_what;
		why = ((Procdata *)data)->prstat.pr_lwp.pr_why;
		check_stat = 0;
		return p_stopped;
	}
	check_stat = 1;
	return p_running;
}

// send signal to process
int 
Proclive::kill(int signo)
{
	DPRINT(DBG_PROC, ("Proclive::kill(this==%#x, signo==%d)\n",this,signo));
	proc_msg	smsg;
	smsg.msg = PCKILL;
	smsg.data = signo;
	set_check();
	do {
		errno = 0;
		::write(fd, (char *)&smsg, sizeof(proc_msg));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// cancel pending signal - does not affect current signal
int
Proclive::cancel_sig( int signo )
{
	DPRINT(DBG_PROC, ("Proclive::cancel_sig(this==%#x, signo==%d)\n",this,signo));
	set_check();
	proc_msg	smsg;
	smsg.msg = PCUNKILL;
	smsg.data = signo;
	do {
		errno = 0;
		::write(fd, (char *)&smsg, sizeof(proc_msg));
	} while(errno && ((errno == EINTR) || 
		err_handle(errno)));
	return ( errno == 0 );
}

// cancel current signal only
int
Proclive::cancel_current()
{
	DPRINT(DBG_PROC, ("Proclive::cancel_current(this==%#x)\n",this));
	set_check();
	struct {
		int		msg;
		siginfo_t	sig;
	} sigmess;
	sigmess.msg = PCSSIG;
	sigmess.sig.si_signo = 0;
	do {
		errno = 0;
		::write(fd, (char *)&sigmess, sizeof(sigmess));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return(errno == 0);
}


// get pending signals - does not include current sig
// if process_only is 0, also merge in LWP pending signals
int
Proclive::pending_sigs(sig_ctl *sigs, int process_only)
{
	int	what, why;

	if (status(what, why) == p_dead)
		return 0;
	if (process_only)
		memcpy(&sigs->signals, 
			&((Procdata *)data)->prstat.pr_sigpend, 
			sizeof(sigset_t));
	else
		mysigsetcombine(&((Procdata *)data)->prstat.pr_lwp.pr_lwppend,
			&((Procdata *)data)->prstat.pr_sigpend,
			&sigs->signals);
	return 1;
}

int
Proclive::current_sig(int &sig)
{
	int	what, why;

	if (status(what, why) == p_dead)
		return 0;
	sig = ((Procdata *)data)->prstat.pr_lwp.pr_cursig;
	return 1;
}

// read and return sigaction structures for all signals
// after first time, we save number of signals and buffer
// so we don't have to figure it out again and re-allocate memory
int
Proclive::get_sig_disp(struct sigaction *&act, int &num_sigs)
{
	DPRINT(DBG_PROC, ("Proclive::get_sig_disp()\n"));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, 
			"Proclive::get_sig_disp", __LINE__);
		return p_unknown;
	}

	if (!((Procdata *)data)->sact)
	{
		struct	stat	s_stat;
		char	fname[PATH_MAX];

		sprintf(fname, "%s/sigact", ((Procdata *)data)->filename);
		do {
			errno = 0;
			((Procdata *)data)->sigact_fd = debug_open( fname, O_RDONLY );
		} while ( errno == EINTR );
		if (errno)
		{
			open_failed(fname, errno);
			return 0;
		}
		do {
			errno = 0;
			fstat(((Procdata *)data)->sigact_fd, &s_stat);
		} while(errno == EINTR);
		if (errno)
		{
			DPRINT(DBG_PROC, ("Proclive::get_sig_disp - fstat sigact:errno==%d\n", errno));
			return 0;
		}
		((Procdata *)data)->number_of_sigs =
			(int)s_stat.st_size/sizeof(struct sigaction);
		((Procdata *)data)->sact = 
			new(struct sigaction[((Procdata *)data)->number_of_sigs]);
	}
	do {
		errno = 0;
		lseek(((Procdata *)data)->sigact_fd, 0, SEEK_SET);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno == 0)
	{
		do {
			errno = 0;
			::read(((Procdata *)data)->sigact_fd, (char *)((Procdata *)data)->sact, 
				(((Procdata *)data)->number_of_sigs *
				sizeof(struct sigaction)));
		} while(errno && ((errno == EINTR) || err_handle(errno)));
	}
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

// set signal trace mask
int
Proclive::trace_sigs(sig_ctl *sigs)
{
	DPRINT(DBG_PROC, ("Proclive::trace_sigs(this==%#x)\n",this));
	set_check();
	sigs->ctl = PCSTRACE;
	do {
		errno = 0;
		::write(fd, (char *)sigs, sizeof(sig_ctl));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::cancel_fault()
{
	DPRINT(DBG_PROC, ("Proclive::cancel_fault(this==%#x)\n",this));
	proc_msg	fmsg;

	set_check();
	fmsg.msg = PCCFAULT;
	fmsg.data = 0;
	do {
		errno = 0;
		::write(fd, (char *)&fmsg, sizeof(fmsg));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return(errno == 0);
}


// set fault trace mask
int
Proclive::trace_traps(flt_ctl *flt)
{
	DPRINT(DBG_PROC, ("Proclive::trace_traps(this==%#x)\n",this));
	set_check();
	flt->ctl = PCSFAULT;
	do {
		errno = 0;
		::write(fd, (char *)flt, sizeof(flt_ctl));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// set system call entry trace mask
int
Proclive::sys_entry(sys_ctl *sys)
{
	DPRINT(DBG_PROC, ("Proclive::sys_entry(this==%#x)\n",this));
	set_check();
	sys->ctl = PCSENTRY;
	do {
		errno = 0;
		::write(fd, (char *)sys, sizeof(sys_ctl));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// set system call exit trace mask
int
Proclive::sys_exit(sys_ctl *sys)
{
	DPRINT(DBG_PROC, ("Proclive::sys_exit(this==%#x)\n",this));
	set_check();
	sys->ctl = PCSEXIT;
	do {
		errno = 0;
		::write(fd, (char *)sys, sizeof(sys_ctl));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

const char *
Proclive::psargs()
{
	static psinfo_t		psinfo;
	char			fname[PATH_MAX];
	int			pfd;

	DPRINT(DBG_PROC, ("Proclive::psargs(this==%#x)\n",this));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::psargs", 
			__LINE__);
		return 0;
	}
	sprintf( fname, "%s/psinfo", ((Procdata *)data)->filename );
	do {
		errno = 0;
		pfd = ::open( fname, O_RDONLY );
	} while ( errno == EINTR );
	if (errno) 
	{
		open_failed(fname, errno);
		return 0;
	}
	do {
		errno = 0;
		lseek(pfd, 0, SEEK_SET);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno)
	{
		::close(pfd);
		return 0;
	}
	do {
		errno = 0;
		::read(pfd, (char *)&psinfo, sizeof(psinfo_t));
	} while(errno && ((errno == EINTR) || err_handle(errno)));

	::close(pfd);
	return((errno == 0) ? psinfo.pr_psargs : 0);
}

int
Proclive::read(Iaddr from, void *to, int len)
{
	int	result;

	DPRINT(DBG_PROC, ("Proclive::read(this==%#x, from==%#x, to==%#x, len==%d)\n",this,from, to, len));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::read", 
			__LINE__);
		return 0;
	}
	do {
		errno = 0;
		lseek(((Procdata *)data)->as_fd, from, SEEK_SET);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::read(((Procdata *)data)->as_fd, to, len);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return result;
}

int
Proclive::write(Iaddr to, const void *from, int len)
{
	int	result;

	DPRINT(DBG_PROC, ("Proclive::write(this==%#x, to==%#x, from==%#x, len==%d)\n",this,to, from, len));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::write", 
			__LINE__);
		return 0;
	}
	do {
		errno = 0;
		lseek(((Procdata *)data)->as_fd, to, SEEK_SET);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::write(((Procdata *)data)->as_fd, from, len);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return result;
}

// get up to date stack boundaries for main process stack
// this is not valid for sibling threads
int
Proclive::update_stack(Iaddr &lo, Iaddr &hi)
{
	int		what, why;

	DPRINT(DBG_PROC, ("Proclive::update_stack(this==%#x)\n",this));

	if (status(what, why) == p_dead)
		return 0;
	// stack grows down
	hi = PROUND((Iaddr)((Procdata *)data)->prstat.pr_stkbase);
	lo = hi - (Iaddr)((Procdata *)data)->prstat.pr_stksize;

	DPRINT(DBG_PROC, ("Proclive::update_stack lo == %#x, hi %#x\n", lo, hi));
	return 1;
}

// get file descriptor open for read-only for object
// file associated with a given virtual address - address
// 0 means the a.out itself
int
Proclive::open_object(Iaddr addr, const char *)
{
	int	fdobj;
	int	num;
	prmap_t	*map;
	char	fname[PATH_MAX];

	// first get segment map and find appropriate segment
	DPRINT(DBG_PROC, ("Proclive::open_object(this==%#x, addr==%#x)\n",this, addr));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::open_object", 
			__LINE__);
		return 0;
	}
	if (addr == 0)
	{
		// open a.out itself
		sprintf(fname, "%s/object/a.out", ((Procdata *)data)->filename);
	}
	else
	{
		map = (prmap_t *)seg_map(num);
		if (!map)
		{
			return -1;
		}
		int i;
		for (i = 0; i < num; i++, map++)
		{
			if ((addr >= map->pr_vaddr) && 
				(addr < (map->pr_vaddr + map->pr_size)))
				break;
		}
		if (i >= num)
		{
			return -1;
		}
		if (map->pr_mapname[0] == '\0')
		{
			// no file for this memory range
			return -1;
		}
		sprintf(fname, "%s/object/%s", ((Procdata *)data)->filename, 
			map->pr_mapname);
	}
	do {
		errno = 0;
		fdobj = debug_open(fname, O_RDONLY);
	} while(errno == EINTR);
	return fdobj;
}

// read general registers
gregset_t *
Proclive::read_greg()
{
	int	what, why;

	DPRINT(DBG_PROC, ("Proclive::read_greg(this==%#x)\n",this));
	if (status(what, why) != p_stopped)
		return 0;
	return &((Procdata *)data)->prstat.pr_lwp.pr_context.uc_mcontext.gregs;
}

// write general registers
int
Proclive::write_greg(gregset_t *greg)
{
	DPRINT(DBG_PROC, ("Proclive::write_greg(this==%#x)\n",this));
	greg_ctl	gctl;
	gctl.ctl = PCSREG;
	memcpy((char *)&gctl.gregs, (char *)greg, sizeof(gregset_t));
	set_check();
	do {
		errno = 0;
		::write(fd, (char *)&gctl, sizeof(greg_ctl));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// read floating-point registers
fpregset_t * 
Proclive::read_fpreg()
{
	int	what, why;

	DPRINT(DBG_PROC, ("Proclive::read_fpreg(this==%#x)\n",this));
	if (status(what, why) != p_stopped)
		return 0;
	if (!(((Procdata *)data)->prstat.pr_lwp.pr_context.uc_flags & UC_FP))
		return 0;	// invalid fp regs
	return &((Procdata *)data)->prstat.pr_lwp.pr_context.uc_mcontext.fpregs;
}

// write floating-point registers
int
Proclive::write_fpreg(fpregset_t *fpreg)
{
	DPRINT(DBG_PROC, ("Proclive::write_fpreg(this==%#x)\n",this));
	fpreg_ctl	fctl;
	fctl.ctl = PCSFPREG;
	memcpy((char *)&fctl.fpregs, (char *)fpreg, sizeof(fpregset_t));
	set_check();
	do {
		errno = 0;
		::write(fd, (char *)&fctl, sizeof(fpreg_ctl));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// read debug registers
dbregset_t *
Proclive::read_dbreg()
{
	int	what, why;

	DPRINT(DBG_PROC, ("Proclive::read_dbreg(this==%#x)\n",this));
	if (status(what, why) != p_stopped)
		return 0;
	return &((Procdata *)data)->prstat.pr_lwp.pr_family.pf_dbreg;
}

// write debug registers
int
Proclive::write_dbreg(dbregset_t *dbreg)
{
	DPRINT(DBG_PROC, ("Proclive::write_dbreg(this==%#x)\n",this));
	dbreg_ctl	dctl;
	dctl.ctl = PCSDBREG;
	set_check();
	memcpy((char *)&dctl.dbregs, dbreg, sizeof(dbregset_t));
	do {
		errno = 0;
		::write(fd, (char *)&dctl, sizeof(dbreg_ctl));
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

// get process segment map - this can change with time
map_ctl *
Proclive::seg_map(int &no)
{

	DPRINT(DBG_PROC, ("Proclive::seg_map(this==%#x)\n",this));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::seg_map", 
			__LINE__);
		return 0;
	}
	if (check_stat || !((Procdata *)data)->num_segs)
	{
		struct stat	stbuf;
		int		i;

		do {
			errno = 0;
			fstat(((Procdata *)data)->map_fd, &stbuf);
		} while (errno == EINTR);
		if (errno)
		{
			((Procdata *)data)->num_segs = 0;
			return 0;
		}
		((Procdata *)data)->num_segs = (int)(stbuf.st_size / sizeof(prmap_t));
		int	sz = (int)stbuf.st_size;
		if (sz != ((Procdata *)data)->map_size)
		{
			((Procdata *)data)->map_size = sz;
			delete(((Procdata *)data)->seg_map);
			((Procdata *)data)->seg_map = new map_ctl[((Procdata *)data)->num_segs];
		}
		do {
			errno = 0;
			lseek(((Procdata *)data)->map_fd, 0, SEEK_SET);
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (errno)
		{
			((Procdata *)data)->num_segs = 0;
			return 0;
		}
		do {
			errno = 0;
			i = ::read( ((Procdata *)data)->map_fd, (char *)((Procdata *)data)->seg_map, sz );
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (i != sz)
		{
			((Procdata *)data)->num_segs = 0;
			return 0;
		}
	}
	no = ((Procdata *)data)->num_segs;
	return ((Procdata *)data)->seg_map;
}

// set up to trap single steps and breakpoints
int
Proclive::default_traps()
{
	DPRINT(DBG_PROC, ("Proclive::default_traps(this==%#x)\n",this));
#if STOP_TYPE == STOP_SIGNALLED
	return 1;
#else
	flt_ctl	fs;

	premptyset(&fs.faults);
	// trace trap - for single step
	praddset(&fs.faults, TRACEFAULT);  
	// machine specific trap for bkpt
	praddset(&fs.faults, BKPTFAULT); 
	return trace_traps(&fs);
#endif
}

// set up process for kill on last close; if is_child is
// non-zero, we don't have to do anything, since
// we set kill on last close when we created it
int
Proclive::destroy(int is_child)
{
	proc_msg	fmsg[2];

	// first cancel any signal about to be delivered.
	// assumed ::stop has been called and status info
	// is valid
	if (((Procdata *)data)->prstat.pr_lwp.pr_cursig)
	{
		struct {
			int		msg;
			siginfo_t	sig;
		} sigmess;
		sigmess.msg = PCSSIG;
		sigmess.sig.si_signo = 0;
		::write(fd, (char *)&sigmess, sizeof(sigmess));
	}

	if (is_child != 0)
		return 1;

	// grabbed process - clear RLC and set KLC
	fmsg[0].msg = PCRESET;
	fmsg[0].data = PR_RLC;
	fmsg[1].msg = PCSET;
	fmsg[1].data = PR_KLC;
	do {
		errno = 0;
		::write(fd, (char *)&fmsg, sizeof(fmsg)); 
	} while(errno == EINTR);
	return (errno == 0);
}

// set process up for release
// if created, clear kill on last close flag
// if release suspended and grabbed, clear run on last close
// created and released running, set run on last close
int
Proclive::release(int run, int is_child)
{
	proc_msg	fmsg[2];
	int		count = 1;

	if (is_child)
	{
		fmsg[0].msg = PCRESET;
		fmsg[0].data = PR_KLC;
		if (run)
		{
			fmsg[1].msg = PCSET;
			fmsg[1].data = PR_RLC;
			count++;
		}
	}
	else if (run == 0)
	{
		fmsg[0].msg = PCRESET;
		fmsg[0].data = PR_RLC;
		count = 1;
	}
	else
		// grabbed released running
		return 1;
	do {
		errno = 0;
		::write(fd, (char *)&fmsg, (count * sizeof(proc_msg))); 
	} while(errno == EINTR);
	return(errno == 0);
}

// used for creating a process 
// child process sets itself up to stop on exec
// before executing the exec call

struct stop_ctl {
	sys_ctl		sys;
	proc_msg	msg;
};

int
stop_self_on_exec()
{
	pid_t		pid;
	char		filename[sizeof("/proc//ctl")+MAX_LONG_DIGITS];
	int		fd;
	int		ret;
	stop_ctl	sctl;

	DPRINT(DBG_PROC, ("stop_self_on_exec()\n"));
	do {
		errno = 0;
		pid = getpid();
	} while ( errno == EINTR );
	if (errno)
		return 0;
	sprintf( filename, "/proc/%d/ctl", pid);
	do {
		errno = 0;
		fd = ::open( filename, O_WRONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		open_failed(filename, errno);
		return 0;
	}

	// clear run on last close and set stop on exec
	sctl.sys.ctl = PCSEXIT;
	premptyset( &sctl.sys.scalls);
	praddset( &sctl.sys.scalls, SYS_exec );
	praddset( &sctl.sys.scalls, SYS_execve );
	sctl.msg.data = PR_RLC;
	sctl.msg.msg = PCRESET;
	do {
		errno = 0;
		::write(fd, (char *)&sctl, sizeof(stop_ctl));
	} while(errno == EINTR);
	DPRINT(DBG_PROC, ("stop_self_on_exec() set stop errno =  %d\n", errno));
	ret = (errno == 0);
	close(fd);
	return ret;
}

// test whether path name given on command line is
// a live process - might be a /proc directory on a remote machine
int
live_proc(const char *path)
{
	pstatus_t	pstat;
	int		fd;
	char		filename[PATH_MAX];
	char		*s, *p;
	pid_t		pid;

	sprintf(filename, "%s/status", path);
	if ((fd = open(filename, O_RDONLY)) == -1)
	{
		return 0;
	}

	do {
		errno = 0;
		lseek(fd, 0, SEEK_SET);
	} while(errno == EINTR);
	if (errno != 0)
		return 0;
	do {
		errno = 0;
		::read(fd, (char *)&pstat, sizeof(pstatus_t));
	} while(errno == EINTR);
	if (errno != 0)
		return 0;
	s = strrchr(path, '/');
	if (s)
		s++;
	pid = strtol(s, &p, 10);
	if (*p)
		return 0;
	return (pid == pstat.pr_pid);
}

// for processes that are ignoring fork
// setup child to run on close of last /proc file
int
release_child(pid_t pid)
{
	char		filename[sizeof("/proc//ctl")+MAX_LONG_DIGITS];
	int		fd;
	int		err;
	proc_msg	pmsg;

	DPRINT(DBG_PROC, ("release_child\n"));
	sprintf( filename, "/proc/%d/ctl", pid);
	do {
		errno = 0;
		fd = ::open( filename, O_WRONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		open_failed(filename, errno);
		return 0;
	}
	
	// set run on last close
	pmsg.msg = PCSET;
	pmsg.data = PR_RLC;
	do {
		errno = 0;
		::write(fd, (char *)&pmsg, sizeof(proc_msg)); 
	} while(errno == EINTR);
	err = errno;
	close(fd);
	return (err == 0);
}

#ifndef FOLLOWER_PROC
int
Proclive::remove_from_follower()
{
	int ret = follower->remove(poll_index);
	poll_index = -1;
	return ret;
}
#endif

#ifdef DEBUG_THREADS

// LWP specific routines

// Set asynchronous stop for LWPS - by default if one stops, they
// all stop.  When debugging threads, we want each to stop
// individually
int
Proclive::set_async()
{
	DPRINT(DBG_PROC, ("Proclive::set_async\n"));
	proc_msg	fmsg;
	fmsg.msg = PCSET;
	fmsg.data = PR_ASYNC;
	do {
		errno = 0;
		::write( fd, (char *)&fmsg, sizeof(proc_msg) );
	} while (errno && ((errno == EINTR) || err_handle(errno)));
	return(errno == 0);
}

Lwplive *
Proclive::open_lwp(lwpid_t lid)
{
	Lwplive	*llive;
	char	fname[PATH_MAX];

	DPRINT(DBG_PROC, ("Proclive::open_lwp(this==%#x, lid==%d)\n",this, lid));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::open_lwp",
			__LINE__);
		return 0;
	}
	sprintf(fname, "%s/lwp/%d", ((Procdata *)data)->filename, lid);
	llive = new Lwplive;
	if (!llive->open(fname, lid, this))
	{
		delete llive;
		return 0;
	}
	return llive;
}

pid_t
Proclive::proc_id()
{
	return ((Procdata *)data)->pid;
}

// control and create a list of all lwps in the process
Lwplive *
Proclive::open_all_lwp(const char *pname)
{
	Lwplive		*lhead = 0;
	Lwplive		*ltail = 0;
	char		*strend;
	DIR		*dirp;
	char		fname[PATH_MAX];
	struct	dirent	*direntp;

	DPRINT(DBG_PROC, ("Proclive::open_all_lwp(this==%#x, pname = %s)\n",this, pname));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::open_all_lwp",
			__LINE__);
		return 0;
	}
	// ((Procdata *)data)->filename is procdir/procid
	// name of lwp file is procdir/procid/lwp/lwpid
	sprintf(fname, "%s/lwp/", ((Procdata *)data)->filename);
	if ((dirp = opendir(fname)) == 0)
	{
		open_failed(fname, errno);
		return 0;
	}
	strend = fname + strlen(fname);
	while((direntp = readdir(dirp)) != 0)
	{
		Lwplive	*llive;
		lwpid_t	lid;
		char	*p;

		lid = strtol(direntp->d_name, &p, 10);
		if (*p)
			continue;	// . or .. ?
		strcpy(strend, direntp->d_name);
		llive = new Lwplive;
		if (llive->open(fname, lid, this))
		{
			if (ltail)
			{
				ltail->set_next(llive);
			}
			else
				lhead = llive;
			ltail = llive;
		}
		else
		{
			printe(ERR_lwp_control, E_WARNING, lid, pname);
			delete llive;
		}
	}
	closedir(dirp);
	return lhead;
}

Lwplive::Lwplive() : Proclive()
{
	DPRINT(DBG_PROC, ("Lwplive::Lwplive()\n"));
	_next = 0;
	data = new Lwpdata;
}

Lwplive::~Lwplive()
{
	DPRINT(DBG_PROC, ("Lwplive::~Lwplive(this==%#x)\n",this));
	delete ((Lwpdata *)data);
	data = 0; // shared by base class; prevent double deletion
}

int 
Lwplive::open(const char *fname, lwpid_t lid, Proclive *p)
{
	char	*strend;
	char	*filebuf;
	int	len = strlen(fname);

	DPRINT(DBG_PROC, ("Lwplive::open(this==%#x, fname==%s,lid==%d)\n",this, fname, lid));
	parent = p;
	follower = parent->get_follower();
	filebuf  = new char[len + sizeof("/lwpstatus")];
	strcpy(filebuf, fname);
	strend = filebuf + len;
	strcpy(strend, "/lwpctl");
	// open ctl and status files
	do {
		errno = 0;
		fd = debug_open( filebuf, O_WRONLY );
	} while ( errno == EINTR );
	DPRINT(DBG_PROC, ("Lwplive::open ctl fd == %d\n", fd));
	if (errno)
	{
		open_failed(filebuf, errno);
		delete filebuf;
		return 0;
	}
	strcpy(strend, "/lwpstatus");
	do {
		errno = 0;
		((Lwpdata *)data)->status_fd = debug_open( filebuf, O_RDONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		open_failed(filebuf, errno);
		delete filebuf;
		::close(fd);
		return 0;
	}
	((Lwpdata *)data)->lid = lid;
	*strend = 0;
	((Lwpdata *)data)->filename = filebuf;
	check_stat = 1;
	return 1;
}

void
Lwplive::close()
{
	DPRINT(DBG_PROC, ("Lwplive::close(this==%#x)\n",this));
	if (fd > 0)
		::close(fd);
	if (((Lwpdata *)data)->status_fd > 0)
		::close(((Lwpdata *)data)->status_fd);
}

Procstat 
Lwplive::status(int &what, int &why)
{
	DPRINT(DBG_PROC, ("Lwplive::status(this==%#x, lwpid = %d)\n",this, ((Lwpdata *)data)->lid));
	if (!((Lwpdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Lwplive::status",
			__LINE__);
		return p_unknown;
	}
	if (check_stat)
	{
		do {
			errno = 0;
			lseek(((Lwpdata *)data)->status_fd, 0, SEEK_SET);
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (errno)
			return p_dead;
		do {
			errno = 0;
			::read(((Lwpdata *)data)->status_fd, 
				(char *)&(((Lwpdata *)data)->lwpstat), 
				sizeof(lwpstatus_t));
		} while(errno && ((errno == EINTR) || err_handle(errno)));
		if (errno)
			return p_dead;
		DPRINT(DBG_PROC, ("Lwplive::status() - read new status\n"));
	}
	if ((((Lwpdata *)data)->lwpstat.pr_flags &
		(PR_STOPPED|PR_ISTOP)) == (PR_STOPPED|PR_ISTOP))
	{
		what = ((Lwpdata *)data)->lwpstat.pr_what;
		why = ((Lwpdata *)data)->lwpstat.pr_why;
		check_stat = 0;
		return p_stopped;
	}
	check_stat = 1;
	return p_running;
}

lwpid_t
Lwplive::lwp_id()
{
	return ((Lwpdata *)data)->lid;
}

// get pending signals - does not include current sig
int
Lwplive::pending_sigs(sig_ctl *sigs, int)
{
	int	what, why;

	if (status(what, why) == p_dead)
		return 0;
	memcpy(&sigs->signals, &((Lwpdata *)data)->lwpstat.pr_lwppend, 
		sizeof(sigset_t));
	return 1;
}

int
Lwplive::current_sig(int &sig)
{
	int	what, why;

	if (status(what, why) == p_dead)
		return 0;
	sig = ((Lwpdata *)data)->lwpstat.pr_cursig;
	return 1;
}
// read general registers
gregset_t *
Lwplive::read_greg()
{
	int	what, why;

	DPRINT(DBG_PROC, ("Lwplive::read_greg(this==%#x)\n",this));
	if (status(what, why) != p_stopped)
		return 0;
	return &((Lwpdata *)data)->lwpstat.pr_context.uc_mcontext.gregs;
}

// read floating-point registers
fpregset_t *
Lwplive::read_fpreg()
{
	int	what, why;

	DPRINT(DBG_PROC, ("Lwplive::read_fpreg(this==%#x)\n",this));
	if (status(what, why) != p_stopped)
		return 0;
	if (!(((Lwpdata *)data)->lwpstat.pr_context.uc_flags & UC_FP))
		return 0;  // invalid fp registers
	return &((Lwpdata *)data)->lwpstat.pr_context.uc_mcontext.fpregs;
}

// read debug registers
dbregset_t *
Lwplive::read_dbreg()
{
	int	what, why;

	DPRINT(DBG_PROC, ("Lwplive::read_dbreg(this==%#x)\n",this));
	if (status(what, why) != p_stopped)
		return 0;
	return &((Lwpdata *)data)->lwpstat.pr_family.pf_dbreg;
}

// Print an error message.  If the error was EBADF (exec'd
// setuid or setgid program) we let the parent's error handler
// take care of it, but we do not try to repeat the operation,
// since this LWP has probably gone away.
int
Lwplive::err_handle(int err)
{
	DPRINT(DBG_PROC, ("Lwplive::err_handle(this==%#x, err==%d)\n",this, err));

	switch(err)
	{
	case 0:
		return 1;
	default:
		printe(ERR_lwp_unknown, E_ERROR, ((Lwpdata *)data)->lid);
		break;
	case EINTR:
	case ENOENT:	
		break;
	case EIO: /*FALLTHROUGH*/
	case EFAULT:	
		printe(ERR_proc_io, E_ERROR, 
			parent->proc_id());
		break;
	case EBADF:
		if (parent)
			parent->err_handle(err);
		break;
	}
	return 0;
}
#endif  //DEBUG_THREADS

#endif
