#ident "@(#)mditx.c	22.1"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1997
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 *
 * mditx.c
 * - implements the "Service From Anywhere" interface,
 *   which provides access locking to the streams q and hardware
 *   interface routines to avoid frame re-ordering
 * - the "service" part means that the queue is always serviced 
 *   before a new message is procesed
 * - the "anywhere" part means that the interface routine can be
 *   called from multiple contexts: put, service, and interrupt 
 *   routines
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif

LKINFO_DECL(mdi_tx_txlkinfo, "DLPI::mdi mutex tx lock", 0);

/*
 * Function: mdi_tx_if_init
 * 
 * Purpose:
 *   Set up the interface data structure to be used in subsequent
 *   calls to the interface (must be enabled before it may actually
 *   be called, see mdi_tx_if_enable).  Should be called from a driver's
 *   load routine.
 *
 *   Return values:  zero with success, non-zero with failure
 *
 * Note:
 *   mdi_tx_if_t should be declared as part of the driver's device
 *   table entry.
 *
 *   See mdi_tx_if for description of arguments.
 */
int
mdi_tx_if_init(mdi_tx_if_t * txint, 
	    mdi_tx_checkavail_t tx_avail, 
	    mdi_tx_processmsg_t process_msg,
	    void * handle,
	    int flags,
	    uchar_t hierarchy)
{
	txint->q = NULL;
	txint->active = 0;
	txint->tx_avail = tx_avail;
	txint->process_msg = process_msg;
	txint->handle = handle;
	txint->flags = flags;
	if ( !(txint->txlock = LOCK_ALLOC(hierarchy, plstr,
			&mdi_tx_txlkinfo, KM_NOSLEEP)) ) {
		return(1);
	}
	return(0);
}


/*
 * Function: mdi_tx_if_deinit
 * 
 * Purpose:
 *   De-allocates memory allocated by mdi_tx_if_init.  Should be called
 *   from a driver's unload routine.
 */

void
mdi_tx_if_deinit(mdi_tx_if_t * txint) 
{
	LOCK_DEALLOC(txint->txlock);
	return;
}

/*
 * Function: mdi_tx_if_enable
 * 
 * Purpose:
 *   Initialize the q pointer in the interface data structure and enable
 *   the interface.
 *
 *   Should be called from a driver's open routine.
 */
void
mdi_tx_if_enable(mdi_tx_if_t * txint, queue_t *q)
{
	txint->q = q;
	return;
}

/*
 * Function: mdi_tx_if_disable
 * 
 * Purpose:
 *   Zero out the q pointer in the interface data structure and disable
 *   the interface - the interface will no longer allow calls to the
 *   drivers's get_txr and process_msg routines once disabled.  The call
 *   returns after all outstanding calls to the above two driver routines
 *   have completed, so the driver can de-allocate tx resources as soon
 *   as the interface is disabled. 
 *
 *   Should be called from a driver's close routine.
 */
void
mdi_tx_if_disable(mdi_tx_if_t * txint)
{
	int plev;

	plev = LOCK(txint->txlock, plstr);
	txint->q = NULL;
	UNLOCK(txint->txlock, plev);
	while (txint->active)  		/* wait for any active process_msg */
		;	       		/* threads (when multi-threaded)   */
	return;
}

/*
 * Function: mdi_tx_if
 *
 * Purpose:
 *   MP-safe or Multi-threaded transmit interface. 
 *   Provides framework that the driver
 *   uses to ensure frames are not re-ordered.
 *
 * Note:
 *
 * called from putnext, service, and interrupt routines
 * 
 * Interface routines
 *
 * 1. void * (*tx_avail)(void * handle);
 *
 *	This function has different semantics, depending on whether the
 *	interface was initialized to be MP_SAFE or MP_Multi-Threaded
 *	(MP_MT).
 *	
 *	When the driver is declared to be MP_SAFE, tx_avail returns a
 *	NULL value when no tx resource(s) are available at the time,
 *	and returns any non-NULL value when tx resources are available.

 *	When the driver is declared to be MP_MT, tx_avail will return
 *	a  tx resource instance to be used with a single streams message. 
 *	The instance usually corresponds to a transmit buffer, or buffer
 *	descriptor.
 *
 *	With MP_MT, tx_avail must ensure that any tx resource returned is
 *	properly sequenced in the subsequent transmission.  That is: if
 *	A calls tx_avail then B calls tx_avail, the driver transmits A's 
 *	frame then B's frame, regardless of when A and B call process_msg 
 *	(see below).
 *
 *	tx_avail returns NULL if no resources available, a valid
 *	resource otherwise.
 *
 *	handle is the driver-specific value specified to mdi_tx_init,
 *	(usually the device table pointer).
 *
 * 2. void (*process_msg)(void * handle, void * txr, mblk_t * mp)
 *	
 *	This function also has different semantics, depending on whether 
 *	the interface was initialized to be MP_SAFE or MP_Multi-Threaded
 *	(MP_MT).
 *	
 *	Driver routine which commits the given msg to the card to be
 *	sent at the next available opportunity.  When MP_Multi-Threaded,
 *	frames are sent in the order in which their resources were 
 *	acquired, not the order in which they were committed.  When 
 *	MP_SAFE, the mdi_tx_if ensures correct ordering of msgs passed
 *	to process_msg.
 *	
 *	handle is as before, txr is the non-NULL value returned from
 *	tx_avail, mp is the message to process (txr is not strictly re-
 *	quired when the driver isn't multi-threaded, but may be handy for
 *	debugging support).
 *
 * NOTE:  we may want to examine txint->q->q_hiwat relative to
 *        txint->q->q_maxpsz to get a rough idea of how many frames
 *        are pending.  while txmon may find the queue size to be draining,
 *        spikes in the interim may cause streams mblk starvation elsewhere.
 *        could even call strqget for QHIWAT/QMAXPSZ and retrieve these values
 *        on priority band 0 and store them in the mdi_tx_if_t structure
 *        since these values are not expected to change once initialized.
 *        We don't have access to cp->mac_max_sdu here.
 *        If "too many" frames are in the queue then drop the frame?
 */

void
mdi_tx_if(mdi_tx_if_t * txint, mblk_t * msg)
{
	int		plev;
	int     	txr;
	mblk_t *	qmsg;

	if (txint->flags == MDI_TX_MP_SAFE) {
		plev = TRYLOCK(txint->txlock, plstr);
		if (plev != invpl) {
			if (txint->q == NULL) {
				UNLOCK(txint->txlock, plev);
				/* should never get here from putnext */
				/*  - hence there is no msg to free   */
				return;
			}
			/* the following while loop will, optimally, get msgs */
			/* from the q for processing until the q is empty or  */
			/* further processing is held up, then process any    */
			/* passed msg, then resume q processing until the q is*/
		 	/* empty again.  This algorithm preserves msg ordering*/

			while (((txint->q->q_first != NULL) || (msg != NULL)) &&
		   	  (txr = (txint->tx_avail)(txint->handle)) != NULL) {
				if (txint->q->q_first != NULL) 
					/* flushq could happen right here */
					/* in a multi-processor environment */
					qmsg = getq(txint->q);
				if (qmsg == NULL) {
					qmsg = msg;
					msg = NULL;
				}
				if (qmsg != NULL)
					 /* flushq could have happened */
					(*txint->process_msg)(txint->handle,
						 txr, qmsg);
			}
		}
	} else {
		/* txint->flags = MDI_TX_MP_MT code follows */
		plev = LOCK(txint->txlock, plstr);
		if (txint->q == NULL) {
			UNLOCK(txint->txlock, plev);
			/* should never get here from putnext */
			/* - hence there is no msg to free    */
			return;
		}
		while (((txint->q->q_first != NULL) || (msg != NULL)) && 
				(txr = (*txint->tx_avail)(txint->handle)) != NULL) {
			if (txint->q->q_first != NULL) 
				qmsg = getq(txint->q);
				/* may get NULL when some other process */
				/* flushed the q here (like txmon?)     */
			else {
				qmsg = msg;
				msg = NULL;
			}
			txint->active++;

			UNLOCK(txint->txlock, plev);

			DLPI_PRINTF800(("MDI: process_msg(%x,%x,%x)\n",
				txint->handle,txr,qmsg));

			/* process_msg must be prepared to handle a NULL     */
			/* msg, which can happen when flushq occured between */
			/* test of q->q_first and getq in mp context 	     */
			(*txint->process_msg)(txint->handle, txr, qmsg);
			plev = LOCK(txint->txlock, plstr);
			txint->active--;
		}
	}
	UNLOCK(txint->txlock, plev);
	if (msg != NULL) 
		putq(txint->q, msg);
		/* the put routine should never call mdi_tx_if at */
		/* a time when the interface is disabled, so we   */
		/* don't worry about txint->q being NULL.         */
		/* Note that srv and interrupt routines may still */
		/* call the if after the if was disabled by the   */
		/* driver close routine, but there are no msgs    */
		/* passed by those routines.			  */
	return;
}

