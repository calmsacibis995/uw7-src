#ident	"@(#)debugger:debug.d/common/sig_handle.c	1.16"

/* signal handling function
 * written in C to get around cfront problems with
 * signal structure definitions
 */

#include "Machine.h"
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "handlers.h"
#include "Proctypes.h"

sigset_t	default_sigs; /* signals caught by default */

sigset_t	debug_sset;
sigset_t	sset_PI;
sigset_t	orig_mask;

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
#include <thread.h>
/* thread specific data key - keep track of whether follower
 * threads have received SIGUSR2.
 */
thread_key_t	thrkey;
#endif


void
signal_setup()
{
	struct sigaction	act;
	/*
	 * SIG_INFORM and SIGPOLL are used internally in the read routine,
	 * but to protect ourselves from unnecessary grief we put
	 * SIG_INFORM and SIGPOLL on hold except in those regions of
	 * the code where we are prepared to field them. SIGINT is
	 * a great deal trickier.
	 *
	 * SIG_INFORM is a macro that allows the informing signal to be
	 * implemented differently on different systems.
	 * Care must be taken if SIGPOLL is used as SIG_INFORM, since
	 * POLL is already used to watch I/O
	 */

	/* signal set is static - should be 0 on startup */
	praddset(&debug_sset, SIG_INFORM);
	praddset(&debug_sset, SIGPOLL);
	praddset(&debug_sset, SIGINT);
	praddset(&sset_PI, SIGPOLL);
	praddset(&sset_PI, SIGINT);

	/* set signal mask and save original mask so we can
	 * restore it for our child processes.
	 */
	sigprocmask(SIG_SETMASK, &debug_sset, &orig_mask);
	/* child processes should block interrupt, however */
	praddset(&orig_mask, SIGINT);

	/* set up handlers with SA_SIGINFO to assure
	 * reliable queuing of signals, even though
	 * we ignore siginfo structure
	 */
	premptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	act.sa_handler = SIG_IGN;
#if SIGALRM != SIG_INFORM
	sigaction(SIGALRM, &act, 0);
#endif
	sigaction(SIGQUIT, &act, 0);
	sigaction(SIGCLD, &act, 0);

	act.sa_handler = (void (*)())fault_handler();
	sigaction(SIGINT, &act, 0);
	sigaction(SIGPIPE, &act, 0);

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
	act.sa_handler = (void (*)())usr2_handler();
	sigaction(SIGUSR2, &act, 0);
	thr_keycreate(&thrkey, 0);
#endif
	/* Make sure debug does not core dump on
	 * top of a user's core file
	 */

	act.sa_handler = (void (*)()) internal_error_handler();
	sigaction(SIGHUP, &act, 0);
	sigaction(SIGILL, &act, 0);
	sigaction(SIGTRAP, &act, 0);
	sigaction(SIGEMT, &act, 0);
	sigaction(SIGFPE, &act, 0);
	sigaction(SIGBUS, &act, 0);
	sigaction(SIGSEGV, &act, 0);
	sigaction(SIGSYS, &act, 0);
	sigaction(SIGTERM, &act, 0);

	/* SIGINT, SIGPOLL and SIG_INFORM are masked in 
	 * the inform_handler and poll handlers
	 */
	praddset(&act.sa_mask, SIGINT);
	praddset(&act.sa_mask, SIGPOLL);
	act.sa_handler = (void (*)())inform_handler();
	sigaction(SIG_INFORM, &act, 0);

	praddset(&act.sa_mask, SIG_INFORM);
	act.sa_handler = (void (*)())poll_handler();
	sigaction(SIGPOLL, &act, 0);

	/* workaround for bug in libedit with sh/csh */
	if (signal(SIGTSTP, SIG_IGN) == SIG_DFL)
		signal(SIGTSTP, suspend_handler());
	
	/* start out with all signals in subject processes
	 * caught by default
	 */
	prfillset(&default_sigs);	
#ifdef DEBUG_THREADS
	/* except for threads, don't catch certain special signals
	 * used by the threads library
	 */
	prdelset(&default_sigs, SIGLWP);
	prdelset(&default_sigs, SIGWAITING);
#endif
}

void
signal_unset()
{
	struct sigaction	act;

	premptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = SIG_DFL;
	sigaction(SIGHUP, &act, 0);
	sigaction(SIGILL, &act, 0);
	sigaction(SIGTRAP, &act, 0);
	sigaction(SIGEMT, &act, 0);
	sigaction(SIGFPE, &act, 0);
	sigaction(SIGBUS, &act, 0);
	sigaction(SIGSEGV, &act, 0);
	sigaction(SIGSYS, &act, 0);
	sigaction(SIGTERM, &act, 0);
}
