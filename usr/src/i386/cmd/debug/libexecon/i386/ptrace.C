#ident	"@(#)debugger:libexecon/i386/ptrace.C	1.5"

#if PTRACE
// assumes OSR5 style ptrace

#include "Procctl.h"
#include "ProcFollow.h"
#include "Process.h"
#include "PtyList.h"
#include "Proctypes.h"
#include "global.h"
#include "Interface.h"
#include "utility.h"
#include "str.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/reg.h>
#include "sys/paccess.h"
#include "sys/regset.h"
#include <sys/wait.h>

// ptrace support routines; this version is geared to
// the OpenServer version of ptrace.

extern "C" int syscall(int, ...);

#define _SCO_PACCESS	9768

// OSR5 ptrace commands
#define PTRACEME	0
#define PRUSER		3
#define PWUSER		6
#define PRUN		7
#define PKILL		8
#define PSTEP		9
#define PATTACH		10
#define PDETACH		11
#define PSTOP_FORK	12

// The uoffsets structure is filled in by the paccess system
// call on OSR5; it contains offsets for within the U area
// for certain kernel data objects.  It insulates the debugger
// to some degree from changes in the kernel from release to
// release.

static struct uoffsets	*uoffs;

struct Procdata {
	pid_t		pid;
	short		check_stat;
	short		number_of_sigs;
	short		stop_requested;
	int		cancel_requested;
	int		signal_set;
	int		status;
	gregset_t	gregs;
	fpregset_t	*fpregs;
	dbregset_t	*dbregs;
	struct sigaction *sact;
			Procdata() { memset(this, 0, sizeof(*this)); }
			~Procdata();
};

// We put the stub for paccess here instead of in libc since it
// is not supported on any platform other than OSR5.
static int 
paccess(int pid, int cmd, int offset, int count, char *ptr)
{
	return(syscall(_SCO_PACCESS, pid, cmd, offset, count, ptr));
}

Procdata::~Procdata()
{
	delete fpregs;
	delete dbregs;
	delete sact;
}

Proclive::Proclive()
{
	// other Proclive fields not used by ptrace version
	data = new Procdata;
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
#ifndef GEMINI_ON_OSR5
	const char	*shell = "/bin/sh";
#else
	const char	*shell = ALT_PREFIX "/bin/sh";
#endif

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

		execlp( shell, shell, "-c", execline, 0 );

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
	// we keep running until the argv[0] for the process
	// is not "sh";
	// this doesn't work if the target program is itself "sh" 

	int shlen = strlen(shell);

	set_check();
	for(;;)
	{
		int	what, why;
		if (wait_for(what, why, 0) == p_dead)
		{
			::kill(ppid, SIGKILL);
			return 0;
		}
		if ((why == STOP_SIGNALLED) && (what == STOP_TRACE))
		{
			const char	*args = psargs();
			DPRINT(DBG_PROC, ("Proclive::create: SIGTRAP: psargs = %s\n",args ? args : "0"));
			if (args && (strncmp(args, shell, shlen) != 0))
				break;
			what = 0;
		}
		run(what, follow_no);
	}
	return(ppid);
}

int
Proclive::open(pid_t pid, int is_child)
{
	DPRINT(DBG_PROC, ("Proclive::open(this==%#x, pid==%d, is_child = %d)\n",this, pid, is_child));
	((Procdata *)data)->pid = pid;
	if (!uoffs)
	{
		// one copy per debug invocation - get
		// offsets within kernel U area for certain
		// needed data objects.
		uoffs = new(struct uoffsets);
		if (paccess(pid, P_RUOFFS, 0, sizeof(struct uoffsets), 
			(char *)uoffs) != sizeof(struct uoffsets))
		{
			delete(uoffs);
			uoffs = 0;
			return 0;
		}
	}
	if (!is_child)
	{
		if (ptrace(PATTACH, pid, 0, 0) == -1)
			return 0;
	}
	return 1;
}

int
Proclive::open(const char *path, pid_t pid, int is_child)
{
	return 0;
}

void
Proclive::close()
{
}

int
Proclive::err_handle(int err)
{
	if (!err)
		return 1;
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
	DPRINT(DBG_PROC, ("Proclive::follow_children\n"))
	do {
		errno = 0;
		ptrace(PSTOP_FORK, ((Procdata *)data)->pid, 0, 0);
	} while(errno == EINTR);
	return(errno == 0);
}

int 
Proclive::run(int sig, follower_mode)
{
	DPRINT(DBG_PROC, ("Proclive::run(this==%#x, sig==%d)\n",this, sig ));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::run",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	if (!sig)
	{
		// signal_set is set by ::kill; when process
		// resumes it will receive the signal
		if (sig = ((Procdata *)data)->signal_set)
		{
			((Procdata *)data)->signal_set = 0;
		}
	}
	if (((Procdata *)data)->cancel_requested)
	{
		((Procdata *)data)->cancel_requested = 0;
		sig = 0;
	}
	errno = 0;
	if (ptrace(PRUN, ((Procdata *)data)->pid, 1, sig) != sig)
	{
		return(err_handle(errno));
	}
	return 1;
}

int 
Proclive::step(int sig, follower_mode)
{
	DPRINT(DBG_PROC, ("Proclive::step(this==%#x, sig==%d)\n",this, sig ));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::step",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	if (!sig)
	{
		// signal_set is set by ::kill; when process
		// resumes it will receive the signal
		if (sig = ((Procdata *)data)->signal_set)
		{
			((Procdata *)data)->signal_set = 0;
		}
	}
	if (((Procdata *)data)->cancel_requested)
	{
		((Procdata *)data)->cancel_requested = 0;
		sig = 0;
	}
	errno = 0;
	if (ptrace(PSTEP, ((Procdata *)data)->pid, 1, sig) != sig)
	{
		return(err_handle(errno));
	}
	return 1;
}

int 
Proclive::stop()
{
	int	pstatus;
	int	ret;

	DPRINT(DBG_PROC, ("Proclive::stop(this==%#x)\n",this));
	::kill(((Procdata *)data)->pid, SIGTRAP);
	errno = 0;
	ret = waitpid(((Procdata *)data)->pid, &pstatus, WUNTRACED);
	if (ret == ((Procdata *)data)->pid)
	{
		((Procdata *)data)->check_stat = 0;
		((Procdata *)data)->stop_requested = 1;
		((Procdata *)data)->status = pstatus;
		do {
			errno = 0;
			paccess(((Procdata *)data)->pid, P_RUREGS, 0,
				sizeof(gregset_t), 
				(char *)&((Procdata *)data)->gregs);
		} while(errno == EINTR);
		if (errno)
		{
			return 0;
		}
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
	int	pstatus;

	DPRINT(DBG_PROC, ("Proclive::wait_for(this==%#x)\n",this));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::wait_for",
			__LINE__);
		return p_unknown;
	}
	for(;;)
	{
		int	wret;
		err = 0;
		if (allow_interrupt)
			// allow poll here - may have output in code
			// we are waiting for
			sigrelse(SIGPOLL);
		if (prismember(&interrupt, SIGPOLL))
			prdelset(&interrupt, SIGPOLL);
		if ((wret = waitpid(((Procdata *)data)->pid, &pstatus,
			(WUNTRACED|WNOWAIT))) == -1)
			err = errno;
		DPRINT(DBG_PROC, ("Proclive::wait_for: wait returned %d, status 0%o\n",wret, pstatus));
		if (allow_interrupt)
			sighold(SIGPOLL);
		if (err == EINTR)
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
		else 
			break;
	}
	((Procdata *)data)->check_stat = 1;
	if (err == 0)
	{
		return(status(what, why));
	}
	return p_dead;
}

Procstat 
Proclive::status(int &what, int &why)
{
	DPRINT(DBG_PROC, ("Proclive::status(this==%#x)\n",this));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::status",
			__LINE__);
		return p_unknown;
	}
	if (((Procdata *)data)->check_stat)
	{
		int	wret;
		int	pstatus;

		do {
			errno = 0;
			wret = waitpid(((Procdata *)data)->pid, 
				&pstatus, WNOHANG);
		} while(errno == EINTR);
		DPRINT(DBG_PROC, ("Proclive::status: wait returned %d, status 0%o\n",wret, pstatus));
		if (errno)
			return p_dead;
		if (wret != ((Procdata *)data)->pid)
		{
			((Procdata *)data)->status = 0;
			return p_running;
		}
		((Procdata *)data)->stop_requested = 0;
		((Procdata *)data)->status = pstatus;
		((Procdata *)data)->check_stat = 0;
		do {
			errno = 0;
			paccess(((Procdata *)data)->pid, P_RUREGS, 0,
				sizeof(gregset_t), 
				(char *)&((Procdata *)data)->gregs);
		} while(errno == EINTR);
		if (errno)
		{
			return p_dead;
		}

	}
	if (WIFSTOPPED(((Procdata *)data)->status))
	{
		what = WSTOPSIG(((Procdata *)data)->status);
		if (((Procdata *)data)->stop_requested)
			why = STOP_REQUESTED;
		else
			why = STOP_SIGNALLED;
		return p_stopped;
	}
	return p_dead;
}

int 
Proclive::kill(int signo)
{
	DPRINT(DBG_PROC, ("Proclive::kill(this==%#x, signo = %d)\n",this,signo));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::kill",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	((Procdata *)data)->signal_set = signo;
	return 1;
}

int
Proclive::cancel_sig(int)
{
	DPRINT(DBG_PROC, ("Proclive::cancel_sig(this==%#x)\n",this));
	return (cancel_current());
}

// cancel current signal
int
Proclive::cancel_current()
{
	DPRINT(DBG_PROC, ("Proclive::cancel_current(this==%#x)\n",this));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::cancel_current",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	// will be processed next time process is resumed
	((Procdata *)data)->cancel_requested = 1;
	return (1);
}

int
Proclive::pending_sigs(sig_ctl *sigs, int)
{
	DPRINT(DBG_PROC, ("Proclive::pending_sigs(this==%#x)\n",this));
	int	current;
	if (!current_sig(current))
		return 0;
	sigemptyset(&sigs->signals);
	if (current)
		sigaddset(&sigs->signals, current);
	return 1;
}

int
Proclive::current_sig(int &sig)
{
	int	what, why;

	DPRINT(DBG_PROC, ("Proclive::current_sig(this==%#x)\n",this));
	if (status(what, why) == p_dead)
		return 0;
	if (why == STOP_SIGNALLED)
		sig = what;
	else
		sig = 0;
	return 1;
}

int
Proclive::trace_sigs(sig_ctl *sigs)
{
	return 1;
}

// cancel current fault
int
Proclive::cancel_fault()
{
	return 1;
}

int
Proclive::trace_traps(flt_ctl *)
{
	return(1);
}

const char *
Proclive::psargs()
{
	// HACK!!! - hardcode offset of u_psarg within uarea
	// form OSR5 user.h; why isn't this in uoffsets?

	char	*ptr;
	char	buf[PSARGSZ];
	int	off = PSARG_OFF;
	int	index = 0;
	union	{ unsigned int	l;
		  char		c[sizeof(unsigned int)];
	} cbuf;

	buf[0] = 0;
	for(int i = 0; i < PSARGSZ; i += sizeof(unsigned int))
	{
		errno = 0;
		cbuf.l = ptrace(PRUSER, ((Procdata *)data)->pid,
			off, 0);
		if (errno)
			return 0;
		off += sizeof(unsigned int);
		for(int j = 0; j < sizeof(unsigned int); j++)
		{
			buf[index++] = cbuf.c[j];
			if (cbuf.c[j] == 0)
				goto done;
		}
	}
done:
	buf[PSARGSZ-1] = 0;
	ptr = makestr(buf);
	DPRINT(DBG_PROC, ("Proclive::psargs: return %s\n", ptr));
	return(ptr);
}

// transfer data to/from process using paccess.
static int
transfer_data(pid_t pid, int cmd, int len, int offset, char *ptr)
{
	int	bytes_transferred = 0;

	while(len > 0)
	{
		int	result;
		int	cnt;

		cnt = (len <= MAXIPCDATA) ? len : MAXIPCDATA;
		do {
			errno = 0;
			result = paccess(pid, cmd, offset, cnt, ptr);
		} while(errno == EINTR);
		if (result < cnt)
		{
			if (result == -1)
				return bytes_transferred;
			bytes_transferred += result;
			break;
		}
		bytes_transferred += result;
		offset += result;
		ptr += result;
		len -= result;
	}
	return bytes_transferred;
}

int
Proclive::read(Iaddr from, void *to, int len)
{
	DPRINT(DBG_PROC, ("Proclive::read(this==%#x, from = %#x, to = %#x, len = %d)\n",this, from, to, len));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::cancel_current",
			__LINE__);
		return 0;
	}
	return transfer_data(((Procdata *)data)->pid, P_RDUSER,
		len, (int)from, (char *)to);
}

int
Proclive::write(Iaddr to, const void *from, int len)
{
	DPRINT(DBG_PROC, ("Proclive::write(this==%#x, to = %#x, from = %#x, len = %d)\n",this, to, from, len));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::cancel_current",
			__LINE__);
		return 0;
	}
	return transfer_data(((Procdata *)data)->pid, P_WDUSER, 
		len, (int)to, (char *)from);
}

int
Proclive::update_stack(Iaddr &lo, Iaddr &hi)
{
	DPRINT(DBG_PROC, ("Proclive::update_stack(this==%#x)\n",this));
	int	sz;

	lo = (Iaddr)ptrace(PRUSER, ((Procdata *)data)->pid, 
		uoffs->u_sub, 0);
	sz = (Iaddr)ptrace(PRUSER, ((Procdata *)data)->pid, 
		uoffs->u_ssize, 0);
	hi = lo + (sz * pagesize);
	return 1;
}

int
Proclive::open_object(Iaddr , const char *pathname)
{
	int	fd;
	if ((fd = debug_open(pathname, O_RDONLY)) == -1)
		printe(ERR_cant_open, E_ERROR, pathname,
			strerror(errno));
	return(fd);
}

gregset_t *
Proclive::read_greg()
{
	int	what, why;

	DPRINT(DBG_PROC, ("Proclive::read_greg(this==%#x)\n",this));
	if (status(what, why) != p_stopped)
		return 0;
	return &((Procdata *)data)->gregs;
}

int
Proclive::write_greg(gregset_t *greg)
{
	DPRINT(DBG_PROC, ("Proclive::write_greg(this==%#x)\n",this));
	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::write_greg",
			__LINE__);
		return 0;
	}
	((Procdata *)data)->check_stat = 1;
	do {
		errno = 0;
		paccess(((Procdata *)data)->pid, P_WUREGS, 0,	
			sizeof(gregset_t), (char *)greg);
	} while(errno == EINTR);
	return (errno == 0);
}

fpregset_t *
Proclive::read_fpreg()
{
	DPRINT(DBG_PROC, ("Proclive::read_fpreg(this==%#x)\n",this));
	int	len = sizeof(fpstate_t);
	char	*to;

	if (!((Procdata *)data)->fpregs)
		((Procdata *)data)->fpregs = new fpregset_t;
	to = (char *)((Procdata *)data)->fpregs;
	if (transfer_data(((Procdata *)data)->pid, P_RUFREGS, 
		len, 0, to) <= 0)
		return 0;
	return ((Procdata *)data)->fpregs;
}

int
Proclive::write_fpreg(fpregset_t *fpreg)
{
	DPRINT(DBG_PROC, ("Proclive::write_fpreg(this==%#x)\n",this));
	int	len = sizeof(fpstate_t);

	if (transfer_data(((Procdata *)data)->pid, P_WUFREGS,
		len, 0, (char *)fpreg) <= 0)
		return 0;
	return 1;
}

dbregset_t *
Proclive::read_dbreg()
{
	DPRINT(DBG_PROC, ("Proclive::read_dbreg(this==%#x)\n",this));
	int	result;
	if (!((Procdata *)data)->dbregs)
		((Procdata *)data)->dbregs = new dbregset_t;
	do {
		errno = 0;
		result = paccess(((Procdata *)data)->pid,
			P_RUDREGS, 0, sizeof(dbregset),
			(char *)((Procdata *)data)->dbregs);
	} while(errno == EINTR);
	if (result <= 0)
		return 0;
	return ((Procdata *)data)->dbregs;
}

int
Proclive::write_dbreg(dbregset_t *dbreg)
{
	DPRINT(DBG_PROC, ("Proclive::write_dbreg(this==%#x)\n",this));
	do {
		errno = 0;
		paccess(((Procdata *)data)->pid,
			P_WUDREGS, 0, sizeof(dbregset), (char *)dbreg);
	} while(errno == EINTR);
	return (errno == 0);
}

map_ctl *
Proclive::seg_map(int &)
{
	return 0;
}

// set up to trap single steps and breakpoints
int
Proclive::default_traps()
{
	return 1;
}


// read and return sigaction structures for all signals

int
Proclive::get_sig_disp(struct sigaction *&act, int &num_sigs)
{
	struct sigaction	*sptr;
	int			off;

	if (!((Procdata *)data))
	{
		printe(ERR_internal, E_ERROR, "Proclive::get_sig_disp",
			__LINE__);
		return 0;
	}
	if (!((Procdata *)data)->sact)
	{
		((Procdata *)data)->sact = 
			new(struct sigaction[MAX_NO_SIGS]);
	}
	sptr = ((Procdata *)data)->sact;
	off = uoffs->u_signal;
	for(int i = 0; i < MAX_NO_SIGS; i++, sptr++, off += sizeof(int))
	{
		errno = 0;
		sptr->sa_handler = (void (*)(int))ptrace(PRUSER,
			((Procdata *)data)->pid, off, 0);
		if (errno != 0)
		{
			num_sigs = 0;
			act = 0;
			return 0;
		}
	}
	num_sigs = MAX_NO_SIGS;
	act = ((Procdata *)data)->sact;
	return 1;
}

// setup for release of process;
// if grabbed and released suspended, reset run on last close
// if created and released running, set run on last close
int
Proclive::release(int run, int is_child)
{
	if (is_child || !run)
		return 0;
	do {
		errno = 0;
		ptrace(PDETACH, ((Procdata *)data)->pid, 1, 0);
	} while ( errno == EINTR );
	return (errno == 0);
}

int
Proclive::destroy(int)
{
	do {
		errno = 0;
		::kill(((Procdata *)data)->pid, SIGKILL);
	} while(errno == EINTR);
	return(errno == 0);
}

int
stop_self_on_exec()
{
	DPRINT(DBG_PROC, ("stop_self_on_exec()\n"));
	do {
		errno = 0;
		ptrace(PTRACEME, 0, 0, 0);
	} while ( errno == EINTR );
	if (errno)
		return 0;
	return 1;
}

int
live_proc(const char *path)
{
	return 0;
}

int
release_child(pid_t pid)
{
	do {
		errno = 0;
		ptrace(PDETACH, pid, 0, 0);
	} while ( errno == EINTR );
	return (errno == 0);
}

#endif
