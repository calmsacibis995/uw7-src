#ident "@(#)txmon.c	29.3"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1996.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif

/*
 * Function: dlpi_hwfail_handler
 *
 * Purpose:
 *   Create a MAC_HWFAIL_IND message and send it up the 
 *   control sap (dlpi daemon).
 *
 */
STATIC void 
dlpi_hwfail_handler(per_sap_info_t *sp)
{
	per_card_info_t		*cp;
	struct mac_hwfail_ind	*hwf_ind;
	mblk_t			*hwf_mp;
	txmon_t			*txmon_p;

	DLPI_PRINTF2000(("txmon_check: sending MAC_HWFAIL_IND upstream\n"));
	cmn_err(CE_NOTE, "!dlpi_hwfail_handler: txmon fired, sp %x\n", sp);

	cp = sp->card_info;
	txmon_p = &cp->txmon;

	hwf_mp = allocb(sizeof(struct mac_hwfail_ind), BPRI_HI);
	if (!hwf_mp) {
		cmn_err(CE_WARN, "dlpi_txmon_check(hwfail_handler): allocb failed");
		goto err_exit;
	}
	/* send MAC_HWFAIL_IND to dlpi daemon.  Not necessary to save anything in
	 * cp->hwfail_waiting as this is only used for I_LINK ioctl
	 */
	hwf_mp->b_datap->db_type = M_PCPROTO;
	hwf_mp->b_wptr += sizeof(struct mac_hwfail_ind);
	hwf_ind = (struct mac_hwfail_ind *) (hwf_mp->b_rptr);
	hwf_ind->mac_primitive = MAC_HWFAIL_IND;

	/* explicit putnext ok here */
	putnext(sp->up_queue, hwf_mp);	/* Ignore flow control */
	return;

err_exit:
	/* can't send HWFAIL_IND upstream, but we want to send HWFAIL_IND again 
	 * later by decrementing restart_in_progress to pretend we never got
	 * here.
	 * we are still holding txmon lock so this is ok
	 */
	if (txmon_p->restart_in_progress > 0) txmon_p->restart_in_progress--;
	return;
}

/*
 * Function: dlpi_txmon_check
 *
 * Purpose:
 *   Sometimes drivers and/or the hardware they control get into 
 *   an inconsistent state due to bugs in the driver or the board 
 *   they control.  This routine attempts to determine whether the
 *   board/driver has gone out to lunch.  If it has, send a HWFAIL
 *   message up to the dlpi daemon to restart the driver.
 *
 *  CRITERIA YOU CAN USE:
 *   If, in txmon_consume_time ticks, the driver has not taken the *same*
 *   first message off the queue, then either streams or the driver has
 *   serious problems, so consider the card dead.  We must assume that
 *   the streams scheduler is still at work.
 *
 *  CRITERIA YOU CANNOT USE(i.e. old way):
 *   1) A board/driver is out to lunch if we notice that its queue size
 *      is monotonic increasing in size ie. the size always grows, never
 *      stays the same or shrinks. 
 *      Problem is that a fast producer (quad pentium pro doing a flood ping,
 *      remember that stacks, dlpi, and streams are D_MP now)
 *      with a slow consumer (10Mbit ethernet with collisions on non-MP driver) 
 *      will deluge the STREAMS queue, always causing its size to increase 
 *      in txmon_consume_time ticks.  This does not necessarily mean that 
 *      the card is dead!  Indeed, another cpu can be doing putnexts to 
 *      down_queue whilst we are here in txmon!
 *   2) high water marks.
 *   3) knowing *when* a driver consumed a message.
 *
 * Details:
 *   - detect failure based on the driver's ability/non-ability to pull the
 *     *same* first mblk off the queue in txmon_consume_time ticks
 *   - if failure:
 *	    + flush the transmit queue
 *	    + dlpi_send_hwfail allocates/sends up the actual HWFAIL_IND message
 *   - we flag that the restart process is in progress to avoid trying
 *	 to restart more than once simultaneously
 * Assumptions:
 *   - putnext was called immediately before, increasing the queue size
 *   - streams will not re-use/recycle the mblk between runs giving the 
 *     appearance of a failure when in reality the driver DID take the mblk,
 *     use it, free it, the stack called allocb for the same-ish size and
 *     got the same mblk *, which dlpi processed and put back on head of 
 *     the queue(it was empty before), all of this taking place in less 
 *     than txmon_consume_time ticks.  Hey, it could happen, but given that
 *     on UW/Gemini the mblk * returned from allocb is actually the pointer
 *     returned from general kmem_alloc it's not likely to happen, and 
 *     certainly better than the old way.
 *   - we're not doing priority bands 
 *   - no b_flag & MSGNOGET messages appear (they're skipped over).  only
 *     used by sockmod module, they're unlikely appear in MDI write queue.
 *
 * Notes:
 *   - We lock the code here to avoid having an MP bottleneck.
 *   - DDI8 CFG_SUSPEND note:  if card is currently suspended then
 *                             nothing to do; leave.
 *                             queue size may not drain quickly (if at all)
 *                             even though we try not to send anything to the
 *                             MDI driver if we're suspended
 *
 * TODO: currently dlpi_txmon_check is only called from dlpi_send_unitdata
 *       That is, a card could be dead and we would mark it as such but if 
 *       we don't send any data to the card we'd never know it, even if
 *       there's data in the STREAMS queue.  This should be rewritten to
 *       use itimeouts or exist as a separate kernel thread under sysproc
 *       which will handle all netX devices at once.
 */

void
dlpi_txmon_check(per_card_info_t *cp)
{
	int		cursize;
	mblk_t	*mp;
	queue_t *q;
	int		s;
	txmon_t *txmon_p;
	pl_t	opl;

	/* if dlpid hasn't registered itself or (much more likely) has been
	 * deregistered (probably via call to mdi_shutdown_driver) then there's
	 * nobody to receive a potential hw_fail_ind so return, not a horrible
	 * fatal error by any means
	 */
	if (cp->dlpidRegistered == 0) {
		DLPI_PRINTF2000(("txmon_check: dlpid not registered, returning\n"));
		return;
	}

	txmon_p = &cp->txmon;

	/* DLPI_PRINTF2000(("txmon_check\n")); */

	/* note order of operations in if statement below */
	if ((cp->issuspended == 0) && 
			(cp->mdi_driver_ready == 1) &&
			(txmon_p->trylock != NULL) && 
			((opl = TRYLOCK(txmon_p->trylock, plstr)) != invpl)) {

		/* if we've already send one HWFAIL_IND upstream, don't send 
		 * another. this is set back to 0 when dlpid re-registers itself
		 * Check if txmon has sent it first, then see if we got a real
		 * HWFAIL_IND in mdi_primitive_handler.  We do this since we 
		 * have the lock
		 */
		if (txmon_p->restart_in_progress > cp->init->txmon_count_limit) {
			/* release lock; txmon has already sent a 
			 * HWFAIL_IND upstream 
			 */
			DLPI_PRINTF2000(("txmon_check: already sent HWFAIL_IND upstream\n"));
			UNLOCK(txmon_p->trylock, opl);
			return;
		}

		if (cp->mdi_driver_ready == 0) {
			/* release lock and return, one of the following 
			 * occurred:
			 * - we got a real HWFAIL_IND in mdi_primitive_handler
			 *   which has already sent it up - that's why we put 
			 *   this check after restart_in_progress check so 
			 *   we can identify who did the sending
			 * - we received I_[P]UNLINK on another cpu while 
			 *   this cpu is attempting to send data (which is 
			 *   how we got to this routine)
			 */
			DLPI_PRINTF2000(("txmon_check: got I_[P]UNLINK -or- already passed up HWFAIL_IND upstream\n"));
			UNLOCK(txmon_p->trylock, opl);
			return;
		}

		q = cp->down_queue;
		/* the only place where we set down_queue to NULL is in 
		 * I_[P]UNLINK, which acquires txmon lock, but we know we
		 * have txmon lock at this point, so assertion here is valid
		 */
		ASSERT(q != (queue_t *)NULL);   /* match I_[P]UNLINK code */
		q = q->q_next;

		/* note that mp may be non-NULL but cursize is 0 because we 
		 * don't lock the queue or call getq/putbq here an 
		 * interrupt could come in and MDI driver calls getq from its 
		 * interrupt routine or simply that another CPU could be 
		 * running.  Not 100% kosher, but all we want is the 
		 * address of the mblk
		 */
		if ((mp=q->q_first) != NULL) {
			/* note cursize can be null in next line */
			if ((cursize=qsize(q)) > txmon_p->largest_qsize_found) {
				DLPI_PRINTF2000(("txmon_check: new largest_qsize_found=%d\n", cursize));
				txmon_p->largest_qsize_found = cursize;
			}
		} else {
			/* mp is null */
			cursize = 0;
		}

		/*
		 * Determine if driver has taken the same mblk off the
		 * queue. 
		 * This will indicate that the queue is not being drained.
		 * note that it's ok for queue size to get larger between
		 * txmon runs(see extensive comment at top of routine).
		 */
		if ((mp != NULL) && 
			(txmon_p->last_mblk != NULL) && 
			(mp == txmon_p->last_mblk)) {
			clock_t time_diff;

			drv_getparm(LBOLT, &time_diff);
			/* if first time here then time_diff should be 0 */
			if (txmon_p->last_send_time == 0) {
				time_diff = 0;
			} else {
				time_diff -= txmon_p->last_send_time;
			}


			/* we don't want HWFAIL_IND messages unless time 
			 * difference is over threshold and we've done 
			 * an DLPID_REGISTER or I_PLINK.
			 */
			if ((time_diff > cp->init->txmon_consume_time) &&
				(cp->control_sap != NULL)) {
				s = LOCK_CARDINFO(cp);

#if 0
but card can die in the middle of processing a DL_SOMETHING_REQ and send
up HWFAIL_IND at any point in time past MAC_BIND_REQ; we need to be
prepared for this (and mdi_primitive_handler is)
				if (cp->dlpi_iocblk != NULL) {
					/* already processing a 
					 * DL_SOMETHING_REQ from stack or 
					 * ioctl on /dev/netX; don't send 
					 * HWFAIL_IND up stack or, if 
					 * dlpi_iocblk == 0xBF, user has 
					 * closed netX device and there's 
					 * outstanding ack/nak not yet received.
					 * defer sending a bit; we'll try 
					 * again later
					 */
					UNLOCK_CARDINFO(cp, s);
					cmn_err(CE_WARN, "dlpi_txmon_check: "
						"card (0x%x) dead with "
						"outstanding messages", cp);
					drv_getparm(LBOLT, 
						&txmon_p->last_send_time);
				} else 
#endif

				if (++txmon_p->restart_in_progress >= 
					cp->init->txmon_count_limit) {

					/* note mdi_primitive_handler 
					 * duplicates much of this code 
					 */

					/*do 1st for dlpi_putnext_down_queue*/
					cp->mdi_driver_ready = 0; 

					cp->mdi_primitive_handler = 
						mdi_inactive_handler;

					/* don't change dlpi_iocblk because 
					 * dlpid will still need to issue 
					 * DLPID_REGISTER and P_UNLINK ioctls 
					 * which will change dlpi_iocblk yet 
					 * again.
					 * cp->dlpi_iocblk = 
					 *   (mblk_t *)0xDEADBEEF; 
					 */

					/* for historical data only */
					txmon_p->nrestarts++;  

					UNLOCK_CARDINFO(cp, s);
					flushq(q, FLUSHALL);

					txmon_p->last_mblk = mp;

					/* release lock so we don't hold 
					 * txmon lock across putnext 
					 */
					UNLOCK(txmon_p->trylock, opl);

					dlpi_hwfail_handler(cp->control_sap);

					return;
				} else {
					DLPI_PRINTF2000(("txmon_check: failure %d of %d\n", txmon_p->restart_in_progress, cp->init->txmon_count_limit));
					UNLOCK_CARDINFO(cp, s);
					drv_getparm(LBOLT, &txmon_p->last_send_time);
				}
			} else {
				/* either control_sap is NULL (no 
				 * DLPID_REGISTER or I_PLINK yet)
				 * or time difference not big enough (yet) or 
				 * neither is true
				 */
				DLPI_PRINTF2000(("txmon_check: not enough time "
					"or no dlpid yet; time_diff=%d/%d\n",
					time_diff,
					cp->init->txmon_consume_time));
				drv_getparm(LBOLT, &txmon_p->last_send_time);
			}
		} else {
			DLPI_PRINTF2000(("txmon_check: all ok\n"));
			drv_getparm(LBOLT, &txmon_p->last_send_time);
		}

		txmon_p->last_mblk = mp;

		/* release lock */
		UNLOCK(txmon_p->trylock, opl);
	}
	/* txmon entrance criteria could not be met or lock already held, ok
	 * to return.
	 */
	return;
}
