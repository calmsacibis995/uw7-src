#ident	"@(#)debugger:libutil/common/wait.C	1.10"

#include "utility.h"
#include "ProcObj.h"
#include "global.h"
#include "List.h"
#include "Interface.h"
#include "Proctypes.h"
#include <signal.h>
#include <termio.h>
#include <unistd.h>
#if PTRACE
#include <sys/wait.h>
#endif

// wait for current action to complete

static void stop_waiting_procs();
extern void debugtty();
extern void restore_tty();

void
wait_process()
{
	static int		waiting;
	static struct termio	ttybuf;
	static int		ttysaved;
	static sigset_t		waitset; // should be empty
#if PTRACE
	sigset_t		oset;
	int			status;
#endif

	// we could enter wait_process while already waiting
	// if an event fires and an associated command does
	// a run or step
	if (waiting)
		return;

	waiting = 1;

	// restore users tty
	if (ttysaved)
		ioctl(0, TCSETAW, &ttybuf);
	else
		restore_tty();	// original tty settings
	
	while(!waitlist.isempty())
	{
		// do not block signals while waiting
#ifdef PTRACE
		sigprocmask(SIG_SETMASK, &waitset, &oset);
		waitpid(-1, &status, (WUNTRACED|WNOWAIT));
		sigprocmask(SIG_SETMASK, &oset, 0);
		if (prismember(&interrupt, SIGINT))
		{
			prdelset(&interrupt, SIGINT);
			stop_waiting_procs();
			break;
		}
		inform_processes(0, 0, 0);
#else
		sigsuspend(&waitset);
		if (prismember(&interrupt, SIGINT))
		{
			prdelset(&interrupt, SIGINT);
			stop_waiting_procs();
			break;
		}
#endif
	}
	if (get_ui_type() != ui_gui)
	{
		// save user's tty and restore debugger's
		if (ioctl(0, TCGETA, &ttybuf) == 0)
			ttysaved = 1;
		else
			ttysaved = 0;
		debugtty();
	}

	PrintaxSpeakCount = 0;
	waiting = 0;
	return;
}

static void
stop_waiting_procs()
{
	ProcObj	*p = (ProcObj *)(waitlist.first());

	// allow stop directives to be interrupted in case
	// the stop never returns due to some deadlock
	sigrelse(SIGINT);
	for(; p; p = (ProcObj *)(waitlist.next()))
	{
		p->stop();
	}
	sighold(SIGINT);
	waitlist.clear();
}
