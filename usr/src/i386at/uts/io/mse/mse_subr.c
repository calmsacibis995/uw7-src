#ident	"@(#)mse_subr.c	1.6"
#ident	"$Header$"

#include <util/param.h>
#include <util/types.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <io/termio.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <io/ws/chan.h>
#include <io/mouse.h>
#include <io/mse/mse.h>
#include <util/cmn_err.h>
#include <io/ddi.h>

#ifdef DEBUG
	#ifdef STATIC
		#undef STATIC 
	#endif 
	#define STATIC 
	STATIC int mse_subr_debug = 0;
	#define DEBUG1(a)	if (mse_subr_debug == 1) printf a
	#define DEBUG2(a)   if (mse_subr_debug >= 2) printf a /* allocations */
#else
	#ifdef STATIC
		#undef STATIC 
	#endif 
	#define STATIC static 
	#define DEBUG1(a)
	#define DEBUG2(a)
#endif /* DEBUG */

/* L999 begin */

/* 
 * Complement of button state bits, ie. bit 1 == UP, bit 0 == DOWN.
 * Held in 3 lsbs of the mse_event code field created by the mseproc()
 * called by each HW driver to send the event packets upstream. The 
 * drivers set the fields in the strmseinfo structures.
 */

#define L_BUTTON 	4
#define M_BUTTON 	2
#define R_BUTTON 	1

#define MSE_DELBUT	4

#define DEFAULT_3BE_TIMEOUT 	3


/* 
 * Public functions: called by mouse hardware drivers.
 */

void mseproc(struct strmseinfo *); 		/* Process events	*/

/* 
 * Local prototypes.
 */

STATIC int mse_b2fsm(struct strmseinfo *, mblk_t *);
STATIC void mse_3bproc(struct strmseinfo *,mblk_t *);
STATIC void mse_3bto(void *);

/* End L999 */

/*
 * void
 * mse_iocack(queue_t *, mblk_t *, struct iocblk *, int)
 *
 * Calling/Exit State:
 *	None.
 */

void
mse_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	qreply(qp, mp);
}

/* 
 * void
 * mse_iocnack(queue_t *, mblk_t *, struct iocblk *, int, int)
 *
 * Calling/Exit State:
 *	None.
 */

void
mse_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}


/*
 * void
 * mse_copyout(queue_t *, register mblk_t *, register mblk_t *, uint,
 *		unsigned long)
 *
 * Calling/Exit State:
 *	None.
 */

void
mse_copyout(queue_t *qp, register mblk_t *mp, register mblk_t *nmp, 
		uint size, unsigned long state)
{
	register struct copyreq *cqp;
	struct strmseinfo *cp;
	struct msecopy	*copyp;


	DEBUG1(("In mse_copyout\n"));

	cp = (struct strmseinfo *)qp->q_ptr;
	copyp = &cp->copystate;
	copyp->state = state;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) * (long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;

	if (mp->b_cont) 
		freemsg(mp->b_cont);
	mp->b_cont = nmp;

	qreply(qp, mp);

	DEBUG1(("leaving mse_copyout\n"));
}


/*
 * void
 * mse_copyin(queue_t *, register mblk_t *, int, unsigned long)
 *
 * Calling/Exit State:
 *	None.
 */

void
mse_copyin(queue_t *qp, register mblk_t *mp, int size, unsigned long state)
{
	register struct copyreq *cqp;
	struct msecopy *copyp;
	struct strmseinfo *cp;


	DEBUG1(("In mse_copyin\n"));

	cp = (struct strmseinfo *) qp->q_ptr;
	copyp = &cp->copystate;

	copyp->state = state;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) * (long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	/* Free the associated message attached to b_cont	*/
	if (mp->b_cont) {
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
	}

	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

	qreply(qp, mp);

	DEBUG1(("leaving mse_copyin\n"));
}

/*
 * void
 * mseproc(struct strmseinfo *)
 *
 * Calling/Exit State:
 *	None.
 * Remarks:
 *	The mse driver is not a STREAMS module,and has no associated queue. Since 
 *	it cannot flow control, the caller (ie. mouse HW driver) should call the 
 *	canputnext(D3) flow control test prior to calling the mseproc (which will
 *	send events upstream regardless.
 */

void
mseproc(struct strmseinfo *qp)
{
	register mblk_t 	*bp;
	register mblk_t 	*mp;
	struct ch_protocol	*protop;
	struct mse_event 	*minfo;

	/* 
	 * Need to save the state of buttons delta until the 3rd button emul
	 * code sees us. Add an extra flag to the type and discard on use in 
	 * original cases. 
	 */

	if (qp->x | qp->y)
		qp->type = MSE_MOTION;
	else 
		qp->type = 0;							/* L999 */

	if (qp->button != qp->old_buttons)			/* L999	*/
		qp->type |= MSE_DELBUT;					/* L999 */

	if (!qp->type) {
		/* Busmouse sends null event packets: comment fills up logfile	*/
		/* cmn_err(CE_WARN,"!mseproc: Bad mouse event: no button/delta XY"); */
		return;
	}

	qp->mseinfo.status = (~qp->button & 7) | 
				((qp->button ^ qp->old_buttons) << 3) | 
				 (qp->mseinfo.status & BUTCHNGMASK) | 
				 (qp->mseinfo.status & MOVEMENT);

	if ((qp->type & 1) == MSE_MOTION) {				/* L999	*/
		register int sum;

		qp->mseinfo.status |= MOVEMENT;

		/*
		 * See sys/mouse.h for UPPERLIM = 127 and LOWERLIM = -128
		 */

		sum = qp->mseinfo.xmotion + qp->x;

		if (sum > UPPERLIM)
			qp->mseinfo.xmotion = UPPERLIM;
		else if (sum < LOWERLIM)
			qp->mseinfo.xmotion = LOWERLIM;
		else
			qp->mseinfo.xmotion = (char)sum;

		sum = qp->mseinfo.ymotion + qp->y;

		if (sum > UPPERLIM)
			qp->mseinfo.ymotion = UPPERLIM;
		else if (sum < LOWERLIM)
			qp->mseinfo.ymotion = LOWERLIM;
		else
			qp->mseinfo.ymotion = (char)sum;
	}

	/* Note the button state */
	qp->old_buttons = qp->button;


	/* 
	 * Allocate and fill out a ch_proto_t packet. All data to go via the 
	 * cnux channel multiplexor is in ch_proto_t format. 
	 */ 

	if ((bp = allocb(sizeof(struct ch_protocol), BPRI_MED)) == NULL) { 
		return;
	}

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += sizeof(struct ch_protocol);

	/* LINTED pointer alignment */
	protop = (struct ch_protocol *) bp->b_rptr;
	protop->chp_type = CH_DATA;
	protop->chp_stype = CH_MSE;
	drv_getparm(LBOLT, (clock_t *)&protop->chp_tstmp);

	if ((mp = allocb(sizeof(struct mse_event), BPRI_MED)) == NULL) { 
		freemsg(bp);
		return;
	}

	bp->b_cont = mp;

	minfo = (struct mse_event *)mp->b_rptr;
	minfo->type = qp->type;	
	qp->type &= 1;							/* L999	*/
	minfo->code = qp->button;	
	minfo->x = qp->x;	
	minfo->y = qp->y;	
	mp->b_wptr += sizeof(struct mse_event);

	/* 
	 * Pass the event packet through the 3rd button emulation code. 
	 * State per event queue is held in the strmseinfo structure.
	 * mse_3bproc() sends the packets upstream whether 3rd button emulation
	 * is on or not. 
	 */

	mse_3bproc(qp, bp); 

	return; 
}

/* L999 begin */

/* 
 * STATIC void mse_3bproc(strmseinfo *qp, mblk_t *bp)
 * 
 * Handle third button emulation, do on packets that are already processed
 * by the mseproc() , and pass the packets onto the event queue in qp->rqp
 * on completion. If called then assume three button emulation is enabled 
 * and use the timeout value in the strmseinfo structure.
 */

STATIC void
mse_3bproc(struct strmseinfo *qp, mblk_t *bp)
{ 
	struct mse_event 	*ep; 
	static clock_t		tlast = 0;

	ep = (struct mse_event *)bp->b_cont->b_rptr; 

	/* 
	 * Convert the fsm_timeout value from the default indicator 
	 * to the default value. 
	 */ 

	if (qp->fsm_timeout == -1) 
		qp->fsm_timeout = DEFAULT_3BE_TIMEOUT; 

	/* 
	 * If fsm_timeout 0, 3rd button emulation is disabled.
	 * Restore the original (type = 0,1) type field. 
	 */

	if (qp->fsm_timeout == 0) { 

		/* 
		 * If there's a pending stashed event, then send it, since we 
		 * are no longer waiting, and clear all the state flags, They'll
		 * not be set until emulation is re-enabled. 
		 */
		if (!qp->timed_out && qp->stash_event) { 
			putnext(qp->rqp,qp->stash_event); 
			qp->stash_event = 0;
			untimeout(qp->timeout_id);
			qp->timed_out = 1; 
		}
		ep->type &= 1; 
		putnext(qp->rqp, bp);
		return; 
	}

	/* 
	 * If the middle button is set on input then it's done in hardware, 
	 * so disable the emulation and cmn_err.
	 */

	if (~ep->code & M_BUTTON) { 

		cmn_err(CE_NOTE,"!mse_3bproc: Mid button event: disable emulation");

		/* 
		 * Disable later input. If there's a stashed event, then send it 
		 * as well as this event, and cancel timeouts, etc. 
		 */

		if (!qp->timed_out && qp->stash_event) { 
			putnext(qp->rqp,qp->stash_event); 
			qp->stash_event = 0;
			untimeout(qp->timeout_id);
			qp->timed_out = 1; 
		}

		qp->fsm_timeout = 0;	
		ep->type &= 1; 
		putnext(qp->rqp, bp);
		return; 
	}	

	switch( mse_b2fsm ( qp, bp )){ 

		/* Event has been saved, leave it alone.	*/
		case DISCARD_EVENT:	
			return;					

		/* Pass through current event 	*/
		case SEND_EVENT:
			break;		

		/* 
		 * Retrieve saved event and send upstream, then send current 
		 * event too. 
		 */

		case UNSTASH_EVENT:		
			putnext(qp->rqp, qp->stash_event);
			qp->stash_event = 0;
			break;
		
		/* Bad return value from the state machine.	*/

		default:
			cmn_err(CE_WARN, "event: unknown return from b2_fsm"); 
			return;
	}
	
	/* 
	 * Send the current event upstream, STREAMS queues cant overrun 
	 * Try and check that the data is OK ?  
	 */

	ep->type &= 1;	
	putnext(qp->rqp, bp); 
	return; 

}

/* 
 * STATIC void mse_3bto ( void * vp )
 *
 * Mouse 3rd button emulation timeout routine, if it fires send the 
 * event upstream. Runs at plstr, invoked from plbase only. Indicates
 * the end of a window in which pressing the other button would have
 * emulated a middle button. If we have made the transition to S2 then
 * we are emulating, so discard the saved (1 button alone) state.
 * NB. Runs at plstr so prevents any other STREAMS procedure running.
 * If finds that the saved event is not to be sent don't zero.
 *
 */

STATIC void 
mse_3bto ( void * vp )
{
	struct strmseinfo *qp = (struct strmseinfo *)vp; 

	/* 
	 * If timeout has not been cancelled and we're in the correct 
	 * state then send the event upstream.
	 */

	if( qp->fsm_state == STATE_1 && !qp->timed_out ){
		putnext ( qp->rqp, qp->stash_event );
		qp->stash_event = 0;
		qp->timed_out = 1; 
	} 

	return ; 
}

/* 
 * mse_b2fsm 
 * 
 * Emulation state machine. 
 * mouse_event structure 
 * uchar_t 	type;		MSE_BUTTON(0) | MSE_MOTION(1) (MSE_DELBUT(4))
 * uchar_t 	code;		b0 = RB b1 = MB, b2 = LB	; 0 = D, 1 = U
 * char		x			delta x (signed)	
 * char		y			delta y (signed)	
 * 
 * All motion events with no button-change pass should be passed 
 * through, possibly with a button modification.
 * 
 * If the user depresses button 1 or 3 and then moves the mouse
 * before the timeout expires, take this to be a drag. So just
 * cancel the timeout and unstash the old event, because we
 * won't be getting a middle button now. Go to state 1 because
 * the button up will get processed there.
 *
 * Motion events in state 2 with button 1 and 3 down have their 
 * button field modified to have button 2 down. All their middle
 * button released emulation is done from S2.
 *
 * Motion events in state 2 with either button 1 or button 3 down 
 * also have their button field modified to show button 2.
 * 
 */


STATIC int 
mse_b2fsm(struct strmseinfo *qp, mblk_t *bp) 
{
	uchar_t b_state;
	struct mse_event *ep; 

	ep = (struct mse_event *)bp->b_cont->b_rptr ;
	b_state = (~ep->code) & 0x7;

	/* 
	 * Filter out motion only events, since these cannot change the 
	 * state (although they convert state 1 wait to a state 1 timeout).
	 */ 

	if ((ep->type & MSE_MOTION) && !(ep->type & MSE_DELBUT)) {

		/* 
		 * Movement kills waiting for emulation trigger: the event is a 
		 * select and move, ie. drag. Send saved event, and this one.
		 */ 

		if((qp->fsm_state == STATE_1) && !qp->timed_out ){
			untimeout (qp->timeout_id);
			qp->timed_out = 1;
			return UNSTASH_EVENT;
		}

		/* 
		 * No button change in emulation active state: just keep 
		 * the Middle Down state and passthru'
		 */

		if( qp->fsm_state == STATE_2 )
			ep->code &= ~2; 

		return SEND_EVENT;
	}

	/* state machine */

	switch( qp->fsm_state ){

		/*
		 * State 0: all buttons up, no timeouts pending, no events
		 * in storage, no emulation in force.
		 */

		case STATE_0: 

			if (b_state == (L_BUTTON + R_BUTTON)) {
				ep->code = 5; 	
				ep->type &= 1;
				qp->fsm_state = STATE_2; 
				return SEND_EVENT;
			}

			/*
			 * Normal emulation window case: allow presses within 
			 * a tunable period to be accepted as middle button 
			 * press events.
			 */

			if ((b_state == L_BUTTON)||(b_state == R_BUTTON)) {

				qp->fsm_state = STATE_1;
				ep->type &= 1;
				qp->stash_event = bp;	

				qp->timeout_id = itimeout(mse_3bto, qp, qp->fsm_timeout, plstr);

				/* 
				 * Error : no timeout slot available. Just don't try and 
				 * do the emulation on this event. 
				 */ 

				if ( !qp->timeout_id) {
					cmn_err(CE_WARN,"mse:No timeout for mouse emulation");
					qp->stash_event = 0;	
					qp->timed_out = 1;
					qp->fsm_state = STATE_1;
					return (SEND_EVENT);
				}

				qp->timed_out = 0;
				return( DISCARD_EVENT );
			}
			break;

		/*
		 * State 1: a single button pressed, initiating wait period from 
		 * state S0. If further events are got during the timeout, they 
		 * may cause us to change state to S0 (all buttons up), S2 (both
		 * buttons down), or end the timeout (motion, no button change)
		 * (drag operation denies middle emulation required). After the
		 * preset delay the timer fires anyway, sending the stored data 
		 * upstream if no further actions occurred, informing the upstream
		 * of the initial single press. (philk: split to S1A,S1B)
		 */

		case STATE_1:

			untimeout( qp->timeout_id );

			/* Buttons released: send old press then this release.	*/

			if ( b_state == 0 ){
				qp->fsm_state = STATE_0;
				if( !qp->timed_out ){
					return( UNSTASH_EVENT );
				}
			}

			/*
			 * Emulation state again: L&&R && timer_running. Set the 
			 * state to S2, buttons to UDU, and kill the timeout.
			 */

			if ((b_state == (L_BUTTON+R_BUTTON)) && !qp->timed_out) {

				/* 
				 * Free/discard the old intermediate state. 
				 */ 
				ep->code = 5;
				qp->fsm_state = STATE_2;
				if (qp->stash_event) {
					freemsg(qp->stash_event);
					qp->stash_event = 0;
				}
			}	

			/* 
			 * All events have killed the timer, motion only events
			 * were filtered out before. So, in all cases, kill the 
			 * timeout flag, and, if the stashed event is not sent,
			 * free it? It is outdated.
			 */

			qp->timed_out = 1;

			return( SEND_EVENT );
			/* NOTREACHED */
			break;

		/*
		 * Emulation is enabled and in force. Had a L && R, inside limit
		 * and no subsequent both up state. Pass thru' all events except 
		 * MU (U,x,U), and set MD state en passant.
		 */

		case STATE_2:

			/* 
			 * When we detect an all buttons up event from the S2 state
			 * of emulating middle button down, we clear the emulation.
			 * The event may update both motion and buttons and state.
			 */

			if( b_state == 0 ) {
				qp->fsm_state = STATE_0;
				return( SEND_EVENT );
			}
		
			/* 
			 * For other events: if buttons, ie. back to a single button
			 * we still have MD set, until we see a LR==UU state, so set 
			 * the MD state. Then can either pass the motion upstream, or
			 * can ignore (ie. ignore any state out of UDU except UUU).
			 * Need to send motion upstream to get MD drag events.
			 */

			ep->code &= ~2;
			return (SEND_EVENT);
			/* NOTREACHED */

		default:

			cmn_err(CE_WARN,"mse: mouse emulation in unknown state");
			qp->fsm_state = STATE_0;
			return( SEND_EVENT );

	}

	/* NOTREACHED */
	cmn_err(CE_WARN,"mse: mouse emulation in bad state");

}



