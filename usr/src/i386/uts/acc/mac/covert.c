#ident	"@(#)covert.c	1.4"
#ifdef CC_PARTIAL

/*
 * This file contains Covert Channel treatment routines.
 */

#include <acc/mac/covert.h>
#include <acc/priv/privilege.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <acc/audit/audit.h>

#define CC_DEBUG	0

/*
 * Covert channel event structure.
 * An array of this structure is defined, one element for each
 * covert channel type to be treated by the limiter.  The address of
 * that structure is passed to the limiter as an argument.
 * The the cc_limit_all() routine sets cc_type to indicate
 * the type of the event (chosen from the list defined below)
 * and cc_bits to indicate the maximum number of bits that
 * could potentially be transmitted by this particular event.
 * Other fields are for use by the limiter.
 */
typedef struct cc_event {
	long	cc_type;	/* event type, see list below */
	short	cc_bits;	/* max possible bits transmitted */
	short	cc_flags;	/* flags for use in limiter */
	long	cc_start;	/* start time in ticks */
	long	cc_count;	/* event counter (bits in cycle) */
	sleep_t	cc_lock;	/* sleep lock */
	char	cc_filler[16];	/* for future compatibility */
#ifdef CC_DEBUG
	long	cc_naudit;	/* number of audit records cut */
	long	cc_ndelay;	/* number of delays */
	long	cc_delayticks;	/* number of ticks delayed */
	long	cc_maxbps;	/* max bps within a cycle */
	long	cc_recstart;	/* start time of recording */
	long	cc_recstop;	/* stop time of recording */
	long	cc_nunpriv;	/* number of bits by unprivileged procs */
	long	cc_npriv;	/* number of bits by privileged procs */
#endif
} ccevent_t;

STATIC ccevent_t ccevents[CC_MAXEVENTS];

LKINFO_DECL(cc_events_lkinfo, "CC:covert channel event lock", 0);

/*
 * The following definition masks apply to cc_flags.
 */
#define	CC_AUDIT	0x4	/* audit every occurrence */

/*
 * The following two macros are used to serialize recording events.
 * These macros go to sleep if a particular event structure is busy,
 * guaranteeing that the event is slowed down, not just the process.
 */

#define	CC_LOCK(e) SLEEP_LOCK(&((e)->cc_lock), PRIMED)

#define	CC_UNLOCK(e) SLEEP_UNLOCK(&((e)->cc_lock))


/*
 * Following are tunable parameters defined in the mac.cf/Space.c file.
 */
extern ulong_t ct_delay;
extern ulong_t ct_audit;
extern ulong_t ct_cycle;
extern ulong_t cc_psearchmin;

#define	CC_PSEARCHMIN	1

/*
 * void cc_init(void)
 *	initialize Covert Channel stuff.
 *
 * Calling/Exit State:
 *	This routine is called during the startup phase, after memory
 *	allocation is operable.  No locks are held at entry and none
 *      held at exit.
 */
void
cc_init(void)
{
	int i;

	for (i = 0; i < CC_MAXEVENTS; i++)
		SLEEP_INIT(&ccevents[i].cc_lock, (uchar_t) 0,
			   &cc_events_lkinfo, KM_NOSLEEP);
}


/*
 * void cc_limiter(ccevent_t *e, struct cred *crp)
 *	Covert Channel generic limiter
 *
 * Description:
 *	The limiter locks the specified event structure before any processing
 *	and unlocks it after processing.
 *
 *	Processing is such that the transfer rate is only checked at
 *	the end of a cycle; if the transfer rate for auditing has been
 *	exceeded, an audit record is generated; if the transfer rate
 *	for delay has been exceeded, the event is slowed down.
 *	Notice that the cycling rate not only determines how often
 *	audit records are cut, but also cuts records only if the
 *	transfer rate within the cycle period has exceeded the audit
 *	threshold.  Further note that the slow down is such that the
 *	transfer rate is reduced to within the delay threshold.
 *
 *	In the event that adt_cc() fails to cut an audit record (this
 *	can happen if the limiter is called after the audit structure for a
 *	process has been freed in exit()), the limiter will slow down the
 *	event to below the audit threshold.
 *
 *	Once the audit threshold for a particular event type has been
 *	exceeded, we will start auditing every occurrence of that event.
 *	These individual event records can be enabled or disabled
 *	independently of the threshold records; see adt_cc(), where
 *	the decision as to whether or not to actually cut an audit
 *	record is made.
 *
 * Calling/Exit State:
 *	No locks can be held when called, since treating a covert channel
 *	involves sleeping.  No locks are held on return.
 */
STATIC void
cc_limiter(ccevent_t *e, struct cred *crp)
{
	register long elapsed;
	register long bps;
	register long ndelay;

	/*
	 * Privileged processes are exempt from event recording.
	 */
	if (pm_privon(crp, P_ALLPRIVS))
		return;
	
	CC_LOCK(e);

	if (u.u_lwpp->l_auditp &&
	    (e->cc_flags & CC_AUDIT))	/* auditing every event? */
		(void)adt_cc(e->cc_type, 0L);

	e->cc_count += (long)e->cc_bits;
	if ((elapsed = (long)TICKS_SINCE(e->cc_start)) == 0)
		elapsed = 1;	/* too quick */
	else if (elapsed < 0)
		elapsed = (long) 0x7fffffff; /* elapsed time way too long */

#ifdef CC_DEBUG
	/* recycle by setting start time to zero */
	if (e->cc_recstart == 0) {
		e->cc_recstart = (long)TICKS();
		e->cc_nunpriv = 0;
		e->cc_npriv = 0;
		e->cc_naudit = 0;
		e->cc_ndelay = 0;
		e->cc_delayticks = 0;
		e->cc_maxbps = 0;
	}
	e->cc_recstop = (long)TICKS();
	if (crp->cr_maxpriv)
		e->cc_npriv += e->cc_bits;
	else
		e->cc_nunpriv += e->cc_bits;
#endif

	/* check on cycle */
	if (elapsed >= ct_cycle) {
		/*
		 * be conservative to improve performance,
		 * i.e., always add one instead of checking
		 * the modulo result
		 */
		bps = e->cc_count * HZ / elapsed + 1;
		ndelay = 0;

		/* check if transfer rate requires auditing */
		if (bps >= ct_audit) {
	
			/* audit every occurrence from now on */
			e->cc_flags |= CC_AUDIT;
	
			if (u.u_lwpp->l_auditp) {
				if (adt_cc(e->cc_type, bps)) {
				/*
				 * Failure to cut an audit record
				 * should not happen often.
				 * If it does, however, delay until
				 * below the audit threshold so
				 * that auditing would not be necessary.
				 */
					ndelay = (e->cc_count * HZ /
						ct_audit) + 1 - elapsed;
				}
#ifdef	CC_DEBUG
				else
					e->cc_naudit++;
#endif

			}

		}
		/* check if transfer rate requires delay */
		if (ndelay == 0 && bps >= ct_delay) {
			/*
			 * reduce rate to below delay threshold;
			 * don't care about performance here
			 */
			ndelay = (e->cc_count * HZ / ct_delay)
					+ 1 - elapsed;
		}

		if (ndelay) {
			delay(ndelay);
#ifdef CC_DEBUG
			e->cc_ndelay++;
			e->cc_delayticks += ndelay;
#endif
		}

#ifdef CC_DEBUG
		if (bps > e->cc_maxbps)
			e->cc_maxbps = bps;
#endif

		/* recycle */
		e->cc_count = 0;
		e->cc_start = (long)TICKS();
	}

	CC_UNLOCK(e);
	return;
}

/*
 * void cc_limit_all(covert_t *lwp_evts; cred_t *cp)
 *	Treat all covert channel events encountered.
 *
 * Description:
 *	Calls cc_limiter() once for each channel type to be treated.
 *
 * Calling/Exit State:
 *	Called immediately before returning to user level.  No locks can
 *	be held, since treating a covert channel involves sleeping.  No
 *	locks are held on return.
 */
void
cc_limit_all(covert_t *lwp_evts, cred_t *cp)
{
	int i;

	for (i=0; lwp_evts->c_bitmap; i++) {
		if (lwp_evts->c_bitmap & 1) {
			ccevents[i].cc_type = i;
			ccevents[i].cc_bits = lwp_evts->c_cnt[i];
			lwp_evts->c_cnt[i] = 0;
			cc_limiter(&ccevents[i], cp);
		}
		lwp_evts->c_bitmap >>= 1;
	}
}


/*
 * int cc_getinfo(int info)
 *	Covert Channel get information
 *
 * Description:
 * 	This routine is to provide CC information to external interfaces.
 *
 * Calling/Exit State:
 *	None.
 */
int
cc_getinfo(int info)
{
	int retval = 0;

	switch (info) {
	case CC_PSEARCHMIN:
		retval = (int)cc_psearchmin;
		break;
	default:
		break;
	}

	return retval;
}

#else /* CC_PARTIAL */

/*
 * Bogus definition to supress "empty translation unit" warning from
 * compiler (and lint).
 */

extern int mac_installed;

#endif /* CC_PARTIAL */
