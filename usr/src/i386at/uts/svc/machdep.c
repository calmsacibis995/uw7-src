#ident	"@(#)kern-i386at:svc/machdep.c	1.10.4.2"
#ident	"$Header$"

/*
 * Highly Machine dependent routines.
 */

#include <fs/buf.h>
#include <proc/disp.h>
#include <mem/pmem.h>
#include <util/cmn_err.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * int
 * calc_delay(int)
 *	Returns a number that can be used in delay loops to wait
 *	for a small amount of time, such as 0.5 second.
 *
 * Calling/Exit State:
 *	Takes a single integer argument.
 *
 *	Returns: an integer whose value depends on the cpu speed.
 *
 * Description:
 *	Return a number to use in spin loops that takes into account
 *	both the cpu rate and the mip rating.
 */

int
calc_delay(int x)
{
	return (x * l.cpu_speed);
}

/*
 * int buscheck(buf *bp)
 *      This platform does nothing with this. Other platforms
 *      may do something more useful.
 *
 * Calling/Exit State:
 *      Just return 0.
 */
/* ARGSUSED */
int
buscheck(struct buf *bp)
{
	return 0;
}

/*
 * int
 * softint_hdlr(void)
 *	Handle software interrupts (sent by sendsoft).
 *
 * Calling/Exit State:
 *	Called at PL1 as if from an interrupt.
 *
 * The value of l.eventflags is used by the interrupt posting machinery
 * to decide whether it is necessary to run this function or not - a
 * non-zero value implies that this function is already in the deferred
 * interrupt queue.  Therefore, we must be careful to read the eventflags
 * while atomically clearing them.
 *
 */
int
softint_hdlr(void)
{
    do {
	extern void runqueues(void);
	extern void cgsoftint(void);
	extern void localsoftint(void);

	if (l.eventflags & EVT_STRSCHED) {
		atomic_and(&l.eventflags, ~EVT_STRSCHED);
		runqueues();
	} else if (l.eventflags & EVT_GLOBCALLOUT) {
		atomic_and(&l.eventflags, ~EVT_GLOBCALLOUT);
		cgsoftint();
	} else if (l.eventflags & EVT_LCLCALLOUT) {
		atomic_and(&l.eventflags, ~EVT_LCLCALLOUT);
		localsoftint();
	}
    } while (l.eventflags & EVT_SOFTINTMASK);
    return 0;
}

