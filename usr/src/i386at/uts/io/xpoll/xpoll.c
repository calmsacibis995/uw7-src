#ident	"@(#)kern-i386at:io/xpoll/xpoll.c	1.1"
#ident	"$Header$"

#include <util/cmn_err.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>

extern void (*xpollfuncs[])();

/*
 * void
 * xpolltime(void *arg)
 *	Timeout routine for xpoll module.
 *
 * Calling/Exit State:
 *	Must be called from CPU 0, at PLHI.
 *
 * Description:
 *	Calls each registered driver poll entry point.
 */
/* ARGSUSED */
void
xpolltime(void *arg)
{
        void (**funcp)();

        for (funcp = xpollfuncs; *funcp; ++funcp)
                (**funcp)(plbase);
}

/*
 * void
 * xpollinit(void)
 *	Initialization for xpoll module.
 *
 * Calling/Exit State:
 *	None.
 */
void
xpollinit(void)
{
	if (xpollfuncs[0] == NULL) {
		/*
		 * Don't bother to do anything if nobody needs it.
		 */
		return;
	}

	if (dtimeout(xpolltime, NULL, 1 | TO_PERIODIC, plhi, 0) == 0) {
		/*
		 *+ Not enough memory for a timeout structure during
		 *+ system initialization.  Add more memory or adjust
		 *+ tunables to reduce memory usage.
		 *+
		 *+ The system will still be brought up, but any driver
		 *+ that has a poll entry point may not function correctly.
		 */
		cmn_err(CE_WARN, "Not enough memory for xpoll timeout;");
		cmn_err(CE_CONT, "\tpoll routines will not be run.\n");
	}
}
