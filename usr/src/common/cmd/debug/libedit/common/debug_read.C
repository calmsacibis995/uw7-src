#ident	"@(#)debugger:libedit/common/debug_read.C	1.16"

// This is the top level read routine which is also
// responsible for fielding events from subject processes.
// It waits on a single character read while fielding SIGUSR1.
// If it catches SIG_INFORM, it goes off and does any appropriate
// actions and reprompts if necessary.
//

#include "utility.h"
#include "Interface.h"
#include "Input.h"
#include "global.h"
#include "Proctypes.h"
#include "Machine.h"
#include "sh_config.h"
#include "dbg_edit.h"
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#if PTRACE
#include <sys/time.h>
#define TIME_INTERVAL	100000
static struct itimerval	itimer = { 0, TIME_INTERVAL, 0, 0 };
#endif

int
debug_read(int fd, void *buf, unsigned int nchar)
{
	int i;

	PrintaxGenNL = 1;	// arrange for a newline to be
				// generated before output.

	PrintaxSpeakCount = 0;

	for(;;) 
	{
		prdelset(&interrupt, SIG_INFORM);
		prdelset(&interrupt, SIGPOLL);
#if PTRACE
		itimer.it_value.tv_usec = itimer.it_interval.tv_usec;
#endif

		// unblock SIG_INFORM and SIGPOLL
		// if not gui, also SIGINT
		if (!processing_query)
		{
			sigprocmask(SIG_UNBLOCK, &debug_sset, 0);
#if PTRACE
			setitimer(ITIMER_REAL, &itimer, 0);
#endif
			if (PrintaxSpeakCount) 
			{
				// event notification occurred;
				// break out so we can process
				// any associated commands and reprompt
				sigprocmask(SIG_BLOCK, &debug_sset, 0);
#if PTRACE
				itimer.it_value.tv_usec = 
					itimer.it_value.tv_sec = 0;
				setitimer(ITIMER_REAL, &itimer, 0);
#endif
				i = 0;
				break;
			}
		}
		i = read(fd, buf, nchar);
		if (!processing_query)
		{
			sigprocmask(SIG_BLOCK, &debug_sset, 0);
#if PTRACE
			itimer.it_value.tv_usec = 
				itimer.it_value.tv_sec = 0;
			setitimer(ITIMER_REAL, &itimer, 0);
#endif

			if (i != -1 || errno != EINTR)
				break;
			if (prismember(&interrupt, SIGINT))
			{
				prdelset(&interrupt, SIGINT);
				i = 0;
				break;
			}
			if (!prismember(&interrupt, SIG_INFORM) &&
				!prismember(&interrupt, SIGPOLL))
				break;
			if (PrintaxSpeakCount) 
			{
				// event notification occurred;
				// break out so we can process
				// any associated commands and reprompt
				i = 0;
				break;
			}
		}
		else if (i >= 0)
			break;

	}
	if (get_ui_type() == ui_gui)
	{
		// cancel any spurious interrupts
		sigrelse(SIGINT);
		prdelset(&interrupt, SIGINT);
		sighold(SIGINT);
	}
	PrintaxGenNL = 0;
	return i;
}
