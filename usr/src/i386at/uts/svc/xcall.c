#ident	"@(#)kern-i386at:svc/xcall.c	1.10.4.3"
#ident	"$Header$"

#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/emask.h>
#include <util/engine.h>
#include <util/ghier.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <svc/psm.h>


STATIC lock_t xcall_lock;	/* mutex multiple xcall usage */
	/*+ xcall serialization lock */
STATIC LKINFO_DECL(xcall_lkinfo, "KU::xcall_lock", 0);

#ifndef UNIPROC

STATIC struct xcall_params {
	void (*xc_func)();	/* target function to call */
	void *xc_arg;		/* argument to pass to target function */
} xcall_params;

STATIC union {
	volatile uint_t u_i;
/*	cache_line_t u_c;*/	/* To force onto separate cache lines */
} xcall_handshake[MAXNUMCPU];

#endif /* UNIPROC */

/*
 * void
 * xcall_init(void)
 *	Initialization for xcall().
 *
 * Calling/Exit State:
 *	None.
 */
void
xcall_init(void)
{
	LOCK_INIT(&xcall_lock, XCALL_HIER, PLHI, &xcall_lkinfo, 0);
}

#ifndef UNIPROC

/*
 * void
 * xcall(emask_t *targets, emask_t *responders, void (*func)(), void *arg)
 *	Send an xcall interrupt to the specified engines.
 *
 * Calling/Exit State:
 *	Must be called at PLHI or lower, with no FSPINs held.
 *
 *	For all of the engines in the targets engine mask, except the
 *	current engine, (*func)(arg) will be called; the target function
 *	may or may not have completed, or even started, by the time xcall()
 *	returns, but it is guaranteed that the target engine(s) are not,
 *	and will not be, executing anything other than (possibly other)
 *	xcall target functions (or NMI handlers) until they complete
 *	this target function.
 *
 *	The optional engine mask, responders, is filled in with all engines
 *	which responded within a reasonable amount of time.  If responders
 *	is NULL, there is no timeout, and xcall() waits indefinitely.
 */
void
xcall(emask_t *targets, emask_t *responders, void (*func)(), void *arg)
{
	emask_t targs, targs2;
	int engnum;
	pl_t pl;
	uint_t n;
	int s;

	ASSERT(getpl() != PLXCALL);

	/*
	 * Make a local copy of the target engine mask,
	 * since we'll clear each bit as we process that engine.
	 */
	targs = *targets;

	/*
	 * Remove our engine from the mask.
	 */
	EMASK_CLRS(&targs, &l.eng_mask);

	if (responders)
		*responders = targs;
	targs2 = targs;

	/*
	 * All xcalls must be serialized to avoid deadlock.
	 * This A-B/B-A deadlock could occur if two xcall initiators
	 * were waiting for each other to acknowledge an xcall interrupt.
	 */
	pl = LOCK(&xcall_lock, PLHI);

	/*
	 * Set up the parameters for the target engines to pick up.
	 */
	xcall_params.xc_func = func;
	xcall_params.xc_arg = arg;

	/*
	 * First pass to send the interrupt to all target engines.
	 */
	s = intr_disable();
	while ((engnum = EMASK_FFSCLR(&targs)) != -1) {
		ASSERT(engnum != myengnum);
		/*
		 * Set up a handshake for the engine we're going to signal,
		 * so we can know when it's accepted the parameters.
		 */
		xcall_handshake[engnum].u_i = 1;

		/*
		 * Send an interrupt to the target engine.
		 */
		ms_xpost(engnum, MS_EVENT_XCALL);
	}
	intr_restore(s);

	/*
	 * Second pass to collect responses.
	 * Wait for the target engine(s) to respond by clearing handshake.
	 */
	while ((engnum = EMASK_FFSCLR(&targs2)) != -1) {
		n = 100000;  /* TEMP - need to get right loop value */
		while (xcall_handshake[engnum].u_i != 0) {
			if (responders != NULL && n-- == 0) {
				/*
				 * We timed out; cancel the request.
				 * Note that there are races here, if a stalled
				 * target engine eventually allows interrupts
				 * through again; we try to keep the windows
				 * small, but they're not totally closed.
				 */
				if (atomic_fnc(&xcall_handshake[engnum].u_i)
						== 0)
					break;
				/*
				 * Remove this engine from the responders mask.
				 */
				EMASK_CLR1(responders, engnum);
				break;
			}
		}
	}
	UNLOCK(&xcall_lock, pl);
}

/*
 * void
 * xcall_all(emask_t *responders, void (*func) (), void *arg)
 *	Send an xcall interrupt to all active engines.
 *
 * Calling/Exit State:
 *	Must be called at PLHI or lower, with no FSPINs held.
 *
 *	For all currently active engines, except the current engine,
 *	(*func)(arg) will be called; the target function may or may not have
 *	completed, or even started, by the time xcall_all() returns,
 *	but it is guaranteed that the target engine(s) are not,
 *	and will not be, executing anything other than (possibly other)
 *	xcall target functions (or NMI handlers) until they complete
 *	this target function.
 *
 *	The engine mask, responders, is filled in with all engines
 *	which responded within a reasonable amount of time.  If timed is false,
 *	there is no timeout, and xcall_all() waits indefinitely.
 */
void
xcall_all(emask_t *responders, boolean_t timed, void (*func) (), void *arg)
{
	engine_t *eng;
	int engnum;

	/* Initialize the responders mask with all active engines. */
	EMASK_CLRALL(responders);
	for (eng = &engine[engnum = Nengine]; engnum-- != 0;) {
		if (!((--eng)->e_flags & E_NOWAY))
			EMASK_SET1(responders, engnum);
	}

	if (!EMASK_TESTALL(responders))
		return;

	/* Send the xcall to all of these engine. */
	xcall(responders, timed ? responders : NULL, func, arg);
}

/*
 * int
 * xcall_intr(void)
 *	Interrupt handler for xcall cross-processor interrupts.
 *
 * Calling/Exit State:
 *	While the actual interrupt occurred at PLXCALL, this routine
 *	is called at PLHI.
 *
 * Description:
 *	Even though we're at a lower level than our interrupt, we won't be
 *	reentered until we clear the xcall_handshake[]; the interrupter
 *	is waiting for this handshake, and all interrupters are serialized.
 *
 *	We must save a local copy of xcall_params before we give the handshake,
 *	since the global may be reused any time after that.
 *
 * Remarks:
 *	It checks the xcall_handshake in order to make sure that there
 *	really is an xcall interrupt to do.  This check resolves several
 *	potential races.
 */
int
xcall_intr(void)
{

	if (xcall_handshake[myengnum].u_i != 0) {
		struct xcall_params params = xcall_params;

		if (atomic_fnc(&xcall_handshake[myengnum].u_i) != 0)
			(*params.xc_func)(params.xc_arg);
	}

	return 0;
}
#endif /* UNIPROC */
