/*
 * $Copyright:
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)debugger:debug.d/common/handlers.C	1.15"

#include "Machine.h"
#include "global.h"
#include "handlers.h"
#include "Interface.h"
#include "Proctypes.h"

#include <sys/types.h>
#include <signal.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
#include <thread.h>
extern "C" thread_key_t	thrkey;
#endif


extern int		FieldProcessIO(int);
extern int		inform_processes(int, void*, void*);
static void		fault(int);
static void		u2_handler(int);
static void		internal_error(int);
static void		handle_suspend(int);

SIG_TYP poll_handler()
{
	return (SIG_TYP) FieldProcessIO;
}

SIG_TYP inform_handler()
{
	return (SIG_TYP) inform_processes;
}

SIG_TYP fault_handler()
{
	return (SIG_TYP) fault;
}

SIG_TYP internal_error_handler()
{
	return (SIG_TYP) internal_error;
}

SIG_TYP suspend_handler()
{
	return (SIG_TYP) handle_suspend;
}

SIG_TYP usr2_handler()
{
	return (SIG_TYP) u2_handler;
}

sigset_t	interrupt;

static void
fault(int sig)
{
	praddset(&interrupt, sig);
}

static void
u2_handler(int)
{
#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
	// set thread specific indication that signal has been
	// received
	int	*sig_received;
	(void)thr_getspecific(thrkey, (void **)&sig_received);
	*sig_received = 1;
#endif
}

extern char	*msg_internal_error;
extern void	stop_interface();
extern void	restore_tty();
extern void	destroy_all();

/* signal handler - we avoid all core dumps */
static void
internal_error(int sig)
{
	/* restore tty sanity */
	int flags = fcntl(0, F_GETFL, 0);
	flags &= ~O_NDELAY;
	fcntl(0, F_SETFL, flags);
	if (get_ui_type() != ui_gui)
		restore_tty();
	// restore original working dir
	if (original_dir)
		chdir(original_dir);
	quitflag = 1;
	destroy_all();
	stop_interface();
	if (sig != SIGTERM)
		write(2, msg_internal_error, strlen(msg_internal_error));
	exit(sig);
}

#ifdef __cplusplus
extern "C" {
#endif
void tty_cooked(int);
#ifdef __cplusplus
}
#endif

static void
handle_suspend(int sig)
{
	// workaround for problem in libedit with sh and csh
	tty_cooked(2);  // This is what edit.c does if it runs into trouble
	sigset(SIGTSTP, SIG_DFL);
	kill(0, sig);
	sigset(SIGTSTP, suspend_handler());
	// BUG: the terminal is left in cooked mode when resumed;
	// it really should be put back to what it was.  But what
	// was it? If the debugger was:
	//   running a program in the foreground:
	//     the program should handle it; the debugger left the
	//     terminal cooked.  No problem.
	//   taking command input:
	//     this is the case that this routine handles, except that
	//     it doesn't restore it on resume (oops).  It will be restored
	//     once the debugger reprompts (acceptable for now).
	//   producing command output:
	//     the terminal was cooked when the suspend happened, and
	//     will be left that way.  No problem.
}
