#ident	"@(#)lprof:libprof/common/dprofil.c	1.6"
/*
*	dprofil - handle SIGPROF signaling
*/

#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>
#include <sys/siginfo.h>
#include "dprof.h"
#include "mach_type.h"

static const char errsetitimer[] = "setitimer";
static const char errsigaction[] = "sigaction";
static const char errsigprocmask[] = "sigprocmask";

static void
tick(int sig, siginfo_t *si, void *pctxt) /* increment bucket for PC */
{
	ucontext_t *uc = pctxt;
	unsigned long pc;
	SOentry *so;

	if ((so = _curr_SO) == 0) /* profiling disabled */
		return;
	pc = uc->uc_mcontext.gregs[PC]; /* interrupted PC value */
	if ((pc < so->textstart || so->endaddr <= pc)
		&& (so = _search(pc)) == 0)
	{
	badpc:;
		_out_tcnt++; /* out of bounds PC */
		return;
	}
	/*
	* Scaling is hardcoded to 8; see SOinout.c and newmon.c.
	*/
	pc -= so->textstart;
	pc >>= 3;
	if (pc >= so->size)
		goto badpc;
	so->tcnt[pc]++;
}

void
_dprofil(int enable) /* turn on/off profiling timer */
{
	struct sigaction sact;
	struct itimerval itv;

	itv.it_interval.tv_sec = 0;
	itv.it_value.tv_sec = 0;
	if (!enable) {
		/*
		* Turn profiling timer off.
		*/
		itv.it_interval.tv_usec = 0;
		itv.it_value.tv_usec = 0;
		if (setitimer(ITIMER_PROF, &itv, 0) != 0)
			perror(errsetitimer);
		return;
	}
	/*
	* Turn profiling timer on.  Block SIGPROF when
	* setting up the timer and while handling the signal.
	*/
	itv.it_interval.tv_usec = 1;
	itv.it_value.tv_usec = 1;
	sact.sa_flags = SA_RESTART | SA_SIGINFO;
	sact.sa_sigaction = tick;
	sigemptyset(&sact.sa_mask);
	sigaddset(&sact.sa_mask, SIGPROF);
	if (sigprocmask(SIG_BLOCK, &sact.sa_mask, 0) != 0)
		perror(errsigprocmask);
	if (setitimer(ITIMER_PROF, &itv, 0) != 0)
		perror(errsetitimer);
	if (sigaction(SIGPROF, &sact, 0) != 0)
		perror(errsigaction);
	if (sigprocmask(SIG_UNBLOCK, &sact.sa_mask, 0) != 0)
		perror(errsigprocmask);
}
