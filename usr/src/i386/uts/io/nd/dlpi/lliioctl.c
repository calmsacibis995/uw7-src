#ident "@(#)lliioctl.c	29.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#define LLI31_UNSUPPORTED	/* Pull in extra defines for LLI 3.1 */

#ifdef  _KERNEL_HEADERS

#include <net/socket.h>
#include <net/sockio.h>
#include <net/inet/if.h>
#include <io/nd/sys/lli31.h>
#include <io/nd/dlpi/include.h>

#else

#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include "../sys/lli31.h"
#include "include.h"

#endif

#ifndef I_PLINK
#   define I_PLINK	(STR|026)
#   define I_PUNLINK	(STR|027)
#endif

/******************************************************************************
 *  Ioctl handling code:
 *	I_(P)LINK, I_(P)UNLINK	to link/unlink MDI driver below us
 *	MACIOC_*		emulate some LLI 3.1 ioctls
 ******************************************************************************/

/* Allocate, and send an IOCACK message to the given queue */
void
dlpi_send_iocack(per_sap_info_t *sp)
{
	per_card_info_t	*cp = sp->card_info;
	mblk_t			*mp = cp->dlpi_iocblk;
	struct iocblk	*iocp;

	DLPI_PRINTF100(("DLPI: Sending IOCACK to DLPI user(%x)\n", sp->up_queue));

	mp->b_datap->db_type = M_IOCACK;
	iocp = (struct iocblk *)mp->b_rptr;
	iocp->ioc_error = 0;
	if (!mp->b_cont || iocp->ioc_cmd == I_LINK || iocp->ioc_cmd == I_UNLINK
			|| iocp->ioc_cmd == I_PLINK || iocp->ioc_cmd == I_PUNLINK)
		iocp->ioc_count = 0;
	else
		iocp->ioc_count = mp->b_cont->b_wptr - mp->b_cont->b_rptr;
	iocp->ioc_rval = 0;

	putnext(sp->up_queue,mp);		/* Ignore flow control */

	/* allow other ioctls to be sent to card */
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(cp);
}

/* Allocate, and send an IOCNAK message to the given queue */
void
dlpi_send_iocnak(per_sap_info_t *sp, int err)
{
	per_card_info_t *cp;
	mblk_t *mp;
	struct iocblk *iocp;

	/* sp set to null if NAK received while initializing 
	 * in mdi_shutdown_driver and further iocnaks can arrive from driver 
	 * but now control sap is null 
	 */
	if (sp == (per_sap_info_t *)NULL) {
		DLPI_PRINTF100(("DLPI: dlpi_send_iocnak: no sp to send err=%x\n",err));
		return;
	}
	cp = sp->card_info;
	mp = cp->dlpi_iocblk;

	/* note if initializing we can get multiple NAKs from multiple
	 * commands, so check for dlpi_iocblk to ensure non-zero before
	 * attempting to deference into mp.  Remember that we send M_IOCTLs direct
	 * to MDI driver ourselves and we can't send success/failure up the line
	 * as the thing above us doesn't have any clue what ioctl we're ACK/NAKing!
	 */
	if (mp == (mblk_t *)0) {
		DLPI_PRINTF100(("DLPI: no original mblk to convert to NAK(sp=%x,err=%x)", sp, err));
		return;
	}

	DLPI_PRINTF100(("DLPI: Sending IOCNAK to DLPI user(%x)\n", sp->up_queue));
	mp->b_datap->db_type = M_IOCNAK;

	iocp = (struct iocblk *)mp->b_rptr;
	iocp->ioc_count = 0;
	iocp->ioc_rval = -1;
	iocp->ioc_error = err;
	putnext(sp->up_queue,mp);		/* Ignore flow control */

	/* allow other ioctls to be sent to card */
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(cp);
}

/* Pass-through IOCTL handler, send IOC_ACK/NAK result through to DLPI user */
STATIC void
dlpi_send_ioctl_reply(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t *cp = sp->card_info;
	mblk_t *mp1 = cp->dlpi_iocblk;
	struct iocblk *ip1= (struct iocblk *)mp1->b_rptr;
	struct iocblk *ip;

	if (!mp) {
		dlpi_send_iocnak(sp, ENOSR);
		return;
	}
	ip = (struct iocblk *)mp->b_rptr;
	ip->ioc_id = ip1->ioc_id;

	if ((ip->ioc_cmd == MACIOC_SETADDR) &&
	    (mp->b_datap != NULL) &&
	    (mp->b_datap->db_type == M_IOCACK))
	{
	    if (mp->b_cont &&
		((mp->b_cont->b_wptr - mp->b_cont->b_rptr) == MAC_ADDR_SZ))
		{
		    /*
		     * We should be locking to do this.  It
		     * would require expensive locks on every
		     * packet sent.  I do not want to see that! 
		     * I am expecting that setting the address
		     * is seldom done and that this privilege is
		     * not abused.
		     */
		    bcopy(mp->b_cont->b_rptr, cp->local_mac_addr, MAC_ADDR_SZ);
		}
#ifdef DLPI_DEBUG
	    else
		DLPI_PRINTF(("dlpi_send_ioctl_reply: no address data\n"));
#endif

	    DLPI_PRINTF(("dlpi_send_ioctl_reply: count=%d %b:%b:%b:%b:%b:%b\n",
				ip->ioc_count,
				cp->local_mac_addr[0],
				cp->local_mac_addr[1],
				cp->local_mac_addr[2],
				cp->local_mac_addr[3],
				cp->local_mac_addr[4],
				cp->local_mac_addr[5]));
	}

	/* let the next ioctl run */
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(cp);
	freemsg(mp1);
	putnext(sp->up_queue,mp);	/* Ignore flow control */
}

/*******************************************************************************
 * LLI 3.1 compatibility IOCTL's
 ******************************************************************************/

STATIC void
dlpi_send_MACIOC_GETSTAT_ack(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t	*cp = sp->card_info;
	int i;
	struct iocblk *ip;
	mblk_t			*mp1= cp->dlpi_iocblk;
	mac_stats_eth_t		*statp;
	lli31_mac_stats_t	*statp1;
	per_sap_info_t		*spt;

	if (!mp) {
		dlpi_send_iocnak(sp, ENOSR);
		return;
	}
	ip = (struct iocblk *)mp->b_rptr;
	if (mp->b_datap->db_type == M_IOCNAK) {
		dlpi_send_iocnak(sp, ip->ioc_error);
		freemsg(mp);
		return;
	}
	statp = (mac_stats_eth_t *)mp->b_cont->b_rptr;
	statp1= (lli31_mac_stats_t *)mp1->b_cont->b_rptr;
	bzero(statp1, sizeof(lli31_mac_stats_t));

	statp1->mac_no_resource =	statp->mac_no_resource;
	statp1->mac_frame_xmit = ATOMIC_INT_READ(cp->old_tx.ucast) +
				ATOMIC_INT_READ(cp->old_tx.ucast_test) +
				ATOMIC_INT_READ(cp->old_tx.ucast_xid);
	statp1->mac_bcast_xmit = ATOMIC_INT_READ(cp->old_tx.bcast);
	statp1->mac_mcast_xmit = ATOMIC_INT_READ(cp->old_tx.mcast);

	statp1->mac_frame_recv = ATOMIC_INT_READ(cp->old_rx.ucast) +
				ATOMIC_INT_READ(cp->old_rx.ucast_test) +
				ATOMIC_INT_READ(cp->old_rx.ucast_xid);
	statp1->mac_bcast_recv = ATOMIC_INT_READ(cp->old_rx.bcast);
	statp1->mac_mcast_recv = ATOMIC_INT_READ(cp->old_rx.mcast);

	statp1->mac_frame_nosr += ATOMIC_INT_READ(cp->old_tx.error) +
				 ATOMIC_INT_READ(cp->old_rx.error);
	statp1->mac_ioctets = ATOMIC_INT_READ(cp->old_rx.octets);
	statp1->mac_ooctets = ATOMIC_INT_READ(cp->old_tx.octets);

	for (spt=cp->sap_table, i=cp->maxsaps; i; ++spt, --i) {
		if (spt->sap_state == SAP_FREE)
			continue;
		statp1->mac_frame_xmit += ATOMIC_INT_READ(spt->sap_tx.ucast) +
					  ATOMIC_INT_READ(spt->sap_tx.ucast_test) +
					  ATOMIC_INT_READ(spt->sap_tx.ucast_xid);
		statp1->mac_bcast_xmit += ATOMIC_INT_READ(spt->sap_tx.bcast);
		statp1->mac_mcast_xmit += ATOMIC_INT_READ(spt->sap_tx.mcast);
		statp1->mac_frame_recv += ATOMIC_INT_READ(spt->sap_rx.ucast) +
					  ATOMIC_INT_READ(spt->sap_rx.ucast_test) +
					  ATOMIC_INT_READ(spt->sap_rx.ucast_xid);
		statp1->mac_bcast_recv += ATOMIC_INT_READ(spt->sap_rx.bcast);
		statp1->mac_mcast_recv += ATOMIC_INT_READ(spt->sap_rx.mcast);

		statp1->mac_frame_nosr += ATOMIC_INT_READ(spt->sap_tx.error) +
					  ATOMIC_INT_READ(spt->sap_rx.error);
		statp1->mac_ioctets    += ATOMIC_INT_READ(spt->sap_rx.octets);
		statp1->mac_ooctets    += ATOMIC_INT_READ(spt->sap_tx.octets);
	}

	statp1->mac_lbolt_xmit = 1;
	statp1->mac_lbolt_recv = 1;
	statp1->mac_frame_def =		statp->mac_frame_def;
	for (i=16; i; --i)
		statp1->mac_frame_coll += statp->mac_colltable[i-1];
	statp1->mac_oframe_coll =	statp->mac_oframe_coll;
	statp1->mac_xs_coll =		statp->mac_xs_coll;
	statp1->mac_collisions =	statp1->mac_frame_coll+
					statp1->mac_oframe_coll+
					statp1->mac_xs_coll;
	statp1->mac_frame_nosr =	statp->mac_frame_nosr;
	statp1->mac_badsum =		statp->mac_badsum;
	statp1->mac_align =		statp->mac_align;
	statp1->mac_align =		statp->mac_align;
	statp1->mac_badlen =		statp->mac_badlen;
	statp1->mac_badsap =		ATOMIC_INT_READ(cp->rxunbound);
	statp1->mac_carrier =		statp->mac_carrier;
	statp1->mac_badcts =		statp->mac_carrier;
	statp1->mac_baddma =		statp->mac_baddma;
	statp1->mac_timeouts =		statp->mac_timeouts;
	statp1->mac_intr =		1;
	statp1->mac_spur_intr =		statp->mac_spur_intr;
	statp1->mac_ifspeed = 		cp->mac_ifspeed;

	mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(lli31_mac_stats_t);
	dlpi_send_iocack(sp);

	freemsg(mp);
}

/*
 * Description of MDI - DLPI connection initialization steps
 * 
 * - the dlpid (daemon process) issues I_(P)LINK ioctl
 *   with MDI file descriptor and DLPI file descriptor as arguments
 * - I_(P)LINK code below records:
 *     1) pointer to queue in card_info->down_queue
 *     2) pointer to card_info in the newly associated bottom queue pair's
 *        q_ptr's (RD(q_ptr)->q_ptr and WR(q)->q_ptr) 
 *     3) calls mdi_initialize_driver
 * - mdi_initialize_driver() issues an M_PCPROTO(MAC_INFO_REQ) to driver and
 *   sets cp->mdi_primitive_handler to mdi_INIT_prim_handler, which
 *   is called by mdi_message() when the driver responds with an
 *   M_PCPROTO(MAC_INFO_ACK) message
 * - mdi_INIT_prim_handler copies the information into the card_info
 *   structure, then calls mdi_send_MACIOC_GETADDR to get the mac address
 *   using a M_IOCTL(MACIOC_GETADDR) message to the driver; it sets
 *   cp->ioctl_handler to mdi_INIT_ioctl_handler
 * - when the driver responds with M_IOCACK, mdi_message calls
 *   mdi_INIT_ioctl_handler(), which invokes mdi_send_mac_bind_req()
 * - mdi_send_mac_bind_req creates a message M_PCPROTO(MAC_BIND_REQ),
 *   and sends it to the driver
 * - mdi_INIT_prim_handler is still the active handler, so is invoked
 *   by mdi_message when the M_PCPROTO(MAC_OK_ACK{MAC_BIND_REQ}) arrives;
 *   mdi_primitive_handler() becomes the active MDI handler, the card is marked
 *   as being up, and *FINALLY*, the I_(P)LINK is acknowledged
 *
 * Notes:
 * 1. The queues created from dlpid's open of /dev/mdi and /dev/dlpi
 *    are linked together by the STREAMs head code, but no other
 *    opens of /dev/dlpi are so linked.  The link to /dev/mdi is provided
 *    via cp->down_queue, and each open of /dev/dlpi passes the address of
 *    the card_info struct.
 *
 * Process an ioctl message received at the DLPI from a protocol stack
 * Return TRUE if the ioctl has been handled (ACK/NAK).  A return of
 * zero will put the message back on the queue.
 */

int
dlpi_ioctl(queue_t *q, mblk_t *mp, per_card_info_t *cp)
{
	per_sap_info_t	*sp = (per_sap_info_t *)q->q_ptr;
	struct iocblk	*iocp = (struct iocblk *)mp->b_rptr;
	struct linkblk	*linkp;
	int			s;
	uint		*dlpid_dereg_flag;

	s = LOCK_CARDINFO(cp);
	if (cp->dlpi_iocblk) {
		/* pending ioctl already.  but if txmon has fired and dlpid 
		 * is trying to shut things down via DLPID_REGISTER/I_PUNLINK
		 * then don't queue the ioctl since it doesn't 
		 * involve MDI driver at all
		 */
		switch (iocp->ioc_cmd) {
		case I_LINK:
		case I_PLINK:
		case I_UNLINK:
		case I_PUNLINK:
		case DLPID_REGISTER:
		break;

		default:
			  UNLOCK_CARDINFO(cp, s);
			  DLPI_PRINTF100(("dlpi_ioctl: Queueing IOCTL\n"));
			  return(0);
		}
	}

	/* always do this, even if shutting down via txmon.  this will change 
	 * 0xDEADBEEF that txmon sets but is necessary for dlpi_send_iocack
	 * or dlpi_send_iocnak if shutting down via txmon
	 */
	cp->dlpi_iocblk = mp;

	UNLOCK_CARDINFO(cp, s);

	if (!cp->mdi_driver_ready) {
		switch (iocp->ioc_cmd) {
			case I_LINK:
			case I_PLINK:
			case I_UNLINK:
			case I_PUNLINK:
			case DLPID_REGISTER:
			break;
			default:
				dlpi_send_iocnak(sp, EIO);
				return(1);
		}
	}

	DLPI_PRINTF100(("DLPI: dlpi_ioctl got 0x%x\n", iocp->ioc_cmd));
	switch (iocp->ioc_cmd)
	{

	case DLPI_SETFILTER:
		if (drv_priv((cred_t *) iocp->ioc_cr) == EPERM) {
			cmn_err(CE_NOTE,"!User not privileged to set filter\n");
			dlpi_send_iocnak(sp,EACCES);
			break;
		}
		if (mp->b_cont) {
			int err;
			err=dlpi_setfilter_ack(sp,mp->b_cont);
			if(!err) {
				dlpi_send_iocack(sp); /* success */
			} else {
				dlpi_send_iocnak(sp,err); /* error */
			}
		}
		break;

	case I_LINK:
	case I_PLINK:
		DLPI_PRINTF100(("DLPI: Received I_(P)LINK from DLPI user(%x)\n",sp));

		/*
		 * MDI Driver being linked below us.
		 * The data contains a linkblk structure. Store
		 * queue pointer in 'down_queue' in per_card_info
		 */

		ASSERT(!cp->down_queue);

		/* ensure we have P_DRIVER priviledge on what will be controlling sap.  
		 * this will be the priviledge set used when dlpi internally builds and 
		 * sends down M_IOCTL messages from mdi_send_MACIOC_{dup,alloc,copy} 
		 * later.  These are commonly MACIOC_GETRADDR, MACIOC_GETADDR used in 
		 * dlpi initialization.  Any ioctls sent by user later on (possibly in
		 * response to DLPI M_PROTO but could be M_IOCTL directly) will use a 
		 * different priviledge set.  The net result is that when driver sees
		 * M_IOCTLS from mdi_send_MACIOC_generic the cred_t pointer will be
		 * correct.    The alternative is to have a special dlpi_cred set
		 * which is sys_cred which we use for these ioctls (use drv_getparm with
		 * SYSCRED) but that was too much work for the controlling sap.
		 */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			dlpi_send_iocnak(sp, EACCES);
			return(1);
		}

		linkp = (struct linkblk *)mp->b_cont->b_rptr;
		/* remember, we're a mux.  upper queue q_ptr set in dlpi_allocsap */
		linkp->l_qbot->q_ptr = (caddr_t)cp;
		OTHERQ(linkp->l_qbot)->q_ptr = (caddr_t)cp;
		cp->down_queue = linkp->l_qbot;

		/* cp->txmon.restart_in_progress = 0; */

		/* since we set down_queue a few lines above it's safe to call
		 * mdi_initialize_driver now.  Moved from DLPID_REGISTER case code below.
		 */

		/* send MAC_INFO_REQ to card. mdi_INIT_prim_handler eats the MAC_INFO_ACK 
		 * and sends down MACIOC_GETRADDR/MACIOC_SETADDR and MAC_BIND_REQ
		 * routines under mdi_initialize_driver can call dlpi_sendiocack 
		 * or dlpi_sendiocnak when everything complete.  This will send the
		 * IOCACK/NAK to the I_[P]LINK to ioctl_sap->up_queue by calling
		 * dlpi_send_iocack from mdi_INIT_prim_handler when MAC_OK_ACK received 
		 * after MAC_BIND_REQ sent to MDI driver
		 */
		mdi_initialize_driver(sp);

		/* note check for cp->hwfail_waiting here as MAC_HW_FAIL_IND only valid
		 * after we get MAC_OK_ACK in response to MAC_BIND_REQ.  It's more likely
		 * that we got a MAC_ERROR_ACK of some type earlier on in which case 
		 * we already called dlpi_send_iocnak.
		 */
		if (cp->hwfail_waiting)
		{
			putnext(sp->up_queue, cp->hwfail_waiting);
			cp->hwfail_waiting = (mblk_t *)0;
		}
		break;

	case I_UNLINK:
	case I_PUNLINK:
		/* MDI Driver being unlinked from below us. */
		DLPI_PRINTF100(("DLPI: Received I_(P)UNLINK from DLPI user\n"));

		/* NOTE: be careful if you set cp->variables here that txmon 
		 * or routines txmon calls will not use them as well -- txmon 
		 * can fire here on another cpu.  should put assignments in 
		 * mdi_shutdown_driver() as it acquires txmon lock first.
		 * set sendack to 1 as we want to send iocack up.
		 */
		mdi_shutdown_driver(sp, 1);

		/* IOCACK/NAK sent by mdi_shutdown_driver code */
		break;

	case DLPID_REGISTER: /* dlpid registering with DLPI module */
		DLPI_PRINTF100(("DLPI: Received DLPID_REGISTER from DLPI user(%x)\n",
									sp));
		if (mp->b_cont)
		{
		dlpid_dereg_flag = (uint *)mp->b_cont->b_rptr;
		if (*dlpid_dereg_flag)
			cp->dlpidRegistered = 0;
		dlpi_send_iocack(sp);
		DLPI_PRINTF100(("DLPI: DLPID_REGISTER deregistering from DLPI user(%x)\n", sp));
		} else {
			if (cp->dlpidRegistered)
				dlpi_send_iocnak(sp, EBUSY);
			else {
				/*
				 * If ksl has attached, control_sap will point there
				 * otherwise it will be NULL.  In either case the
				 * newly registered dlpid should get it now.
				 */

				cp->dlpidRegistered = 1;
				cp->control_sap = sp;
				cp->txmon.last_mblk = (mblk_t *)NULL;
				cp->txmon.last_send_time = 0;
				cp->txmon.restart_in_progress = 0;
				/* cp->txmon.nrestarts initialized elsewhere */
				/* since re-registering can be with a different MDI device with
				 * different characterstics, always reset largest_qsize_found
				 */
				cp->txmon.largest_qsize_found = 0;
				dlpi_send_iocack(sp);   /* ACK the REGISTER */
			}
		}
		break;

	case DLPI_MEDA_REGISTER:	/* Media support registering */
		if (!cp->media_specific ||
		!cp->media_specific->media_svc_reg ||
		!cp->media_specific->media_svc_rcv)
			dlpi_send_iocnak(sp, ENXIO);
		else if (cp->media_svcs_queue != NULL)
			dlpi_send_iocnak(sp, EEXIST);
		else
		{
			int	Err;

			if (!(Err = cp->media_specific->media_svc_reg(cp, mp, sp)))
			{
				cp->media_svcs_queue = q;
				dlpi_send_iocack(sp);
			} else {
				cp->media_svcs_queue = NULL;
				dlpi_send_iocnak(sp, Err);
			}
		}
		
		break;

	/*
	 * MACIOC_* Ioctl's Intercepted
	 */
	case MACIOC_DIAG:
	case MACIOC_UNITSEL:
	case MACIOC_GETIFSTAT:
	case MACIOC_CLRMCA:
	case MACIOC_LOCALSTAT:
	case MACIOC_PROMISC:
einval:
		dlpi_send_iocnak(sp, EINVAL);
		break;

	/*
	 * MACIOC_* Ioctl's Emulated
	 */
	case MACIOC_GETSTAT:
		DLPI_PRINTF100(("DLPI: Got MACIOC_GETSTAT ioctl\n"));
		if (iocp->ioc_count != sizeof(lli31_mac_stats_t)) {
			goto einval;
		}
		if (cp->mac_media_type != DL_CSMACD) {
			goto einval;
		}
		mdi_send_MACIOC_GETSTAT(sp, dlpi_send_MACIOC_GETSTAT_ack);
		break;

#ifdef UNIXWARE_TCP
	case 0x4405:
		DLPI_PRINTF100(("DLPI: Got local packet copy UnixWare ioctl\n"));
		break;
	case 0x40044403:
		DLPI_PRINTF100(("DLPI: Got UnixWare getaddr ioctl\n"));
		/* UnixWare 2.1 getaddr ioctl - fall through */
#endif
	case MACIOC_GETADDR:
		if (iocp->ioc_count != MAC_ADDR_SZ)
		goto einval;
		bcopy(cp->local_mac_addr, mp->b_cont->b_rptr, MAC_ADDR_SZ);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr + MAC_ADDR_SZ;
		dlpi_send_iocack(sp);
		break;
	case MACIOC_GETRADDR:
		if (iocp->ioc_count != MAC_ADDR_SZ)
		goto einval;
		bcopy(cp->hw_mac_addr, mp->b_cont->b_rptr, MAC_ADDR_SZ);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr + MAC_ADDR_SZ;
		dlpi_send_iocack(sp);
		break;

#ifdef UNIXWARE_TCP
	/* Unixware 2.1 tcp/ip sends funkey socket ioctls to DLS provider */
	case SIOCGIFFLAGS:
	case SIOCGIFDEBUG:
		DLPI_PRINTF100(("DLPI: SIOCIF socket ioctl %x\n", iocp->ioc_cmd));
		dlpi_send_iocack(sp);
		break;
#endif
	case MACIOC_SETADDR:
		if (iocp->ioc_uid != 0)
			goto einval;
		/* Fall thru to pass the request */
	default:
		if (cp->media_specific && cp->media_specific->media_ioctl)
		switch (cp->media_specific->media_ioctl(mp, cp, sp))
		{
			case MIOCRET_PUTBQ:
			return 0;

			case MIOCRET_DONE:
			return 1;

			case MIOCRET_NOT_MINE:
			default:
			break;
		}

		/* due to MP race conditions when restarting stack, 
		 * prohibit pass through ioctls on netX device if we haven't
		 * finished our I_[P]LINK yet to set mdi_driver_ready to 1
		 * (done by mdi_INIT_prim_handler).  When 
		 * mdi_INIT_prim_handler sends down ioctls it expects that
		 * mdi_INIT_ioctl_handler will receive the replies but yet
		 * when in pass-through mode dlpi_send_ioctl_reply is the
		 * handler -- a problem.
		 */
		if (cp->mdi_driver_ready == 0) {
			DLPI_PRINTF100(("DLPI: no passthrough ioctl(%x) - no card\n", iocp->ioc_cmd));
			goto einval;
		}

		DLPI_PRINTF100(("DLPI: Received unknown ioctl %x from DLPI user\n", iocp->ioc_cmd));
		mdi_send_MACIOC_dup(sp, iocp->ioc_cmd, iocp->ioc_count,
						dlpi_send_ioctl_reply, mp->b_cont);
	}

	return 1;
}

STATIC int
dlpi_setfilter_ack(per_sap_info_t *sp, mblk_t *mp)
{
	dl_setfilter_req_t *dsfp = (dl_setfilter_req_t *)mp->b_rptr;
	ulong sap;
	ulong sap_protid;
	int flags;
	int s;
	int direction;
	int ns;
	int bigfilter = 0;
	per_sap_info_t *filter_sp;
	mblk_t *fptr;

	if (dsfp == NULL) {
		cmn_err(CE_WARN, "dlpi_setfilter_ack: dsfp is NULL\n");
		return(EINVAL);
	}
	sap = dsfp->sap;
	direction = dsfp->direction;

	if ((direction != DL_FILTER_INCOMING) &&
		(direction != DL_FILTER_OUTGOING)) {
		cmn_err(CE_WARN, "!dlpi_setfilter_ack - invalid direction specified ");
		return(EINVAL);
	}

	if(mp->b_cont != NULL) {
		/* set the flag so that we know whether to free new 
		 * mblk_t when we're done
		 */
		bigfilter = 1;
		mp = msgpullup(mp, -1);
		if(mp == NULL) {
			cmn_err(CE_WARN, "dlpi_setfilter_ack: Couldn't do msgpullup");
			return(EINVAL);
		}
	}

	flags = sp->card_info->media_specific->media_flags;

	if ((flags & MEDIA_SNAP) && (sap == SNAP_SAP)) {
		sap_protid = dsfp->sap_protid;
		s = LOCK_SAP_ID(sp->card_info);
		if ((filter_sp=dlpi_findsnapsap(sap_protid, sap, sp->card_info)) == NULL) { /* fail */
			 UNLOCK_SAP_ID(sp->card_info, s);
			 goto not_found;
		}
		/* found it */
		/* set filter, but first skip control data */
		ns = mp->b_wptr - mp->b_rptr - sizeof(dl_setfilter_req_t);

		if (filter_sp->filter[direction] != NULL) {
			freemsg(filter_sp->filter[direction]); 
		}

		if (ns > 0) {
			fptr = allocb(ns, BPRI_HI);
			if (!fptr) {
				UNLOCK_SAP_ID(sp->card_info, s);
				cmn_err(CE_WARN, "dlpi_setfilter_ack - STREAMs memory shortage");
				if(bigfilter)
					freeb(mp); /* free the new mblk_t */
				return(ENOSR);
			}
			bcopy(mp->b_rptr + sizeof(dl_setfilter_req_t), fptr->b_rptr, ns);
			fptr->b_wptr += ns;
			filter_sp->filter[direction] = fptr;
		} else {
			/* NULL FILTER */
			filter_sp->filter[direction] = NULL;
		}

		UNLOCK_SAP_ID(sp->card_info, s);
		/* send OK ack */
		if(bigfilter)
			freeb(mp); /* free new mblk_t if big filter */
		return(0);
	} else {		/* not a SNAP sap */
		s = LOCK_SAP_ID(sp->card_info);
		if ((filter_sp=dlpi_findsap(sp->sap_type, sap, sp->card_info)) == NULL) { /* fail */
			UNLOCK_SAP_ID(sp->card_info,s);
			goto not_found;
		}
		/* found it */
		/* set filter, but first skip control data */
		ns=mp->b_wptr - mp->b_rptr - sizeof(dl_setfilter_req_t);

		if (filter_sp->filter[direction] != NULL) {
			freemsg(filter_sp->filter[direction]); 
		}

		if (ns > 0) {
			fptr = allocb(ns, BPRI_HI);
			if (!fptr) {
				UNLOCK_SAP_ID(sp->card_info, s);
				cmn_err(CE_WARN, "dlpi_setfilter_ack - STREAMs memory shortage");
				if(bigfilter)
					freeb(mp); /* free new mblk_t if big filter */
				return(ENOSR);
			}
			bcopy(mp->b_rptr + sizeof(dl_setfilter_req_t), fptr->b_rptr, ns);
			fptr->b_wptr += ns;
			filter_sp->filter[direction] = fptr;
		} else {
			/* NULL FILTER */
			filter_sp->filter[direction] = NULL;
		}

		UNLOCK_SAP_ID(sp->card_info, s);

		/* send OK ack */
		if(bigfilter)
			freeb(mp); /* free new mblk_t if big filter */
		return(0);
	}

not_found:
	if(bigfilter)
		freeb(mp); /* free new mblk_t if big filter */
	return(EPROTO);
}
