#ident	"@(#)kern-i386at:proc/dispmdep.c	1.6.5.1"
#ident	"$Header$"

#include <util/engine.h>
#include <util/ipl.h>
#include <proc/disp.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>

/*
 * void dispmdepinit(int nglobpris)
 *
 *	Initialize any machine specific functions.
 *
 * Calling/Exit State:
 *
 *	Called from dispinit, early on in startup, hence the processor
 *	is single threaded at this point.
 *
 * Description:
 *
 *	This function initializes some SLIC priorities and therefore
 *	is a no op for the AT architecture.
 */

/* ARGSUSED */
void
dispmdepinit(int nglobpris)
{
	return;
}

/*
 * void dispmdepnewpri(int pri)
 *
 *	Make machine specific adjustments resulting from a new
 *	priority.
 *
 * Calling/Exit State:
 *
 *	Called when the currently running lwp changes its priority.
 *	Currently only called from ts_trapret.
 * Description:
 *
 *	For the AT architecture, this is a no op.
 */

/* ARGSUSED */
void
dispmdepnewpri(int pri)
{
	return;
}

/*
 * void idle(void)
 *
 *	Idle the engine until work arrives.
 *
 * Calling/Exit State:
 *
 *	Called at splhi, drops the priority to spl0, returns again
 *	at splhi.
 *
 * Description:
 *
 *	Set our IPL to zero and call the PSM to idle.
 */
void
idle(void)
{
	(void) spl0();
	ms_idle_self();
	(void) splhi();
}
