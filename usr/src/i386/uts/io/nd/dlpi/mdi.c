#ident "@(#)mdi.c	29.4"
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

int	mdi_INIT_prim_handler(queue_t *, mblk_t *);
STATIC void	mdi_send_mac_bind_req(per_sap_info_t *);

/******************************************************************************
 *
 * MDI Interface Implementation.
 *
 ******************************************************************************/

/*
 * This is a fairly expensive operation, and so should be called with care.
 * These scenarios cause messages to be placed on the upper write queues:
 *	(1) A message is already on the lower write queue
 *	(2) The MDI Driver's write queue is full (canput fails)
 *	(3) An ioctl is already being processed by the MDI Driver, and
 *		another ioctl or primitive request arrives at the DLPI Interface 
 *		  from the protocol stack.
 */
void
dlpi_enable_WRqs(per_card_info_t *cp)
{
	per_sap_info_t *sp = cp->sap_table;
	int i;

	DLPI_PRINTF80(("MDI: Enabling Upper Write Queues\n"));
	/* Enable all the Upper Write Queue's */
	for (i = cp->maxsaps; i; --i) {
		if (sp->sap_state == SAP_BOUND)
			qenable(WR(sp->up_queue));
		++sp;
	}
}
/******************************************************************************
 *
 * MDI Driver Initialize and Shutdown initiation functions
 *
 ******************************************************************************/
/*
 * Send a MAC_INFO_REQ to the MDI Driver (generated as a result of an I_LINK
 * or I_PLINK ioctl).
 *
 * MDI driver initialization sequence, stuff in brackets doesn't always happen:
 *	MAC_INFO_REQ
 *	[ MACIOC_SETALLMCA ]  (if restarting device and allmca is set)
 *	[ MACIOC_SETMCA ]     (if restarting device and mca's are set)
 *	[ MACIOC_GETRADDR ]   (first time device is initialized only)
 *	MACIOC_GETADDR
 *	[ MACIOC_SETADDR ]    (if restarting adapter with non-factory MACaddr)
 *	MAC_BIND_REQ
 */
void
mdi_initialize_driver(per_sap_info_t *sp)
{
	per_card_info_t *cp = sp->card_info;
	mblk_t *mp;

	DLPI_PRINTF40(("MDI: Initializing MDI Driver\n"));
	cp->control_sap = sp;

	if (!(mp = allocb(sizeof (mac_info_req_t), BPRI_LO)))
	{
		cmn_err(CE_WARN,
			"mdi_initialize_driver - "
			"Unable to initialize driver; out of STREAMs blocks");
		cp->mdi_primitive_handler = mdi_inactive_handler;
		dlpi_send_iocnak(sp, ENOSR);
		return;
	}

	mp->b_datap->db_type = M_PCPROTO;
	((mac_info_req_t *)mp->b_wptr)->mac_primitive = MAC_INFO_REQ;
	mp->b_wptr += sizeof(mac_info_req_t);
	cp->mdi_primitive_handler = mdi_INIT_prim_handler;

	/*
	 * The mdi_INIT_prim_handler will handle the information
	 * returned, and continue the initialization process;
	 */
	DLPI_PRINTF40(("MDI: Sending MAC_INFO_REQ to MDI Driver\n"));
	dlpi_putnext_down_queue(cp,mp);		/* Ignore flow control */
}

/*
 * Called when the MAC_INFO_REQ primitive has been processed, and returned
 */

STATIC void
mdi_INIT_ioctl_handler(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t *cp = sp->card_info;
	struct iocblk *ip;

	if (!mp)
	{
		dlpi_send_iocnak(sp, ENOSR);
		return;
	}

	ip = (struct iocblk *)mp->b_rptr;

	if (mp->b_datap->db_type == M_IOCACK)
	{
	    if ((!mp->b_cont) && (ip->ioc_cmd != MACIOC_SETALLMCA)) {
		dlpi_send_iocnak(sp, ENOSR);
		return;
	    }

	    if (ip->ioc_cmd == MACIOC_SETMCA) {
		unchar *m = mp->b_cont->b_rptr;

		/* TODO: verify that addr set matches addr sent down */

		DLPI_PRINTF100(("MDI: MACIOC_SETMCA iocack %b:%b:%b:%b:%b:%b\n",
			m[0], m[1], m[2], m[3], m[4], m[5]));
	    }
	    else if (ip->ioc_cmd == MACIOC_SETALLMCA) {
		DLPI_PRINTF100(("MDI: MACIOC_SETALLMCA iocack\n"));
	    }
	    else if (ip->ioc_cmd == MACIOC_GETRADDR) {
		DLPI_PRINTF100(("MDI: Sending MACIOC_GETADDR to MDI Driver\n"));
		bcopy (mp->b_cont->b_rptr, cp->hw_mac_addr, MAC_ADDR_SZ);
		mdi_send_MACIOC_GETADDR(sp, mdi_INIT_ioctl_handler);
	    }
	    else if (ip->ioc_cmd == MACIOC_GETADDR || 
		     ip->ioc_cmd == MACIOC_SETADDR)
	    {
		DLPI_PRINTF100(("MDI: Sending MAC_BIND_REQ to MDI Driver\n"));
		bcopy (mp->b_cont->b_rptr, cp->local_mac_addr, MAC_ADDR_SZ);
		mdi_send_mac_bind_req(sp);
	    }
	    else {
		cmn_err(CE_WARN,
		 "MDI: Received IOCAK to unknown command %#X from MDI Driver\n",
		 ip->ioc_cmd);
		cp->mdi_primitive_handler = mdi_inactive_handler;
		dlpi_send_iocnak(sp, EIO);
	    }
	} else {				/* M_IOCNAK */
		if (((ip->ioc_cmd == MACIOC_GETADDR)
			|| (ip->ioc_cmd == MACIOC_GETRADDR))
			&& ((ip->ioc_count == 0)
			&& (ip->ioc_rval == -1)
			&& (ip->ioc_error == EINVAL)))
			/* device doesn't support MAC address, continue init */
   				mdi_send_mac_bind_req(sp);
		else {
			/* NOTE: if trying to init MDI driver and it doesn't
			 * immediately respond to ioctls but instead replies
			 * in its interrupt routine then we will have multiple
			 * outstanding ioctls from driver that we're waiting
			 * for a reply on (in particular multiple earlier 
			 * calls to mdi_send_MACIOC_SETMCA to restore MCA 
			 * addresses in mdi_INIT_prim_handler).  Each of these
			 * could fail but yet we can't send multiple NAKs up,
			 * so we let dlpi_send_iocnak figure things out
			 * Note that the first call to dlpi_send_iocnak is 
			 * valid and fails the I_[P]LINK -- it's just any
			 * subsequent failures we want to throw away and not
			 * send upstream (remember, we're buried in init
			 * code and not in the normal mdi_ioctl_handler!)
			 */ 
			DLPI_PRINTF100(("MDI: M_IOCNAK: cmd=%x count=%x rval=%x error=%x\n", ip->ioc_cmd, ip->ioc_count, ip->ioc_rval, ip->ioc_error));
			if (ip->ioc_cmd == MACIOC_GETADDR) 
		 		DLPI_PRINTF100(("MDI: Received MACIOC_GETADDR IOCNAK from MDI Driver\n"));
			else if (ip->ioc_cmd == MACIOC_GETRADDR)
				DLPI_PRINTF100(("MDI: Received MACIOC_GETRADDR IOCNAK from MDI Driver\n"));
			else if (ip->ioc_cmd == MACIOC_SETMCA)
				DLPI_PRINTF100(("MDI: Received MACIOC_SETMCA IOCNAK from MDI Driver\n"));
			else if (ip->ioc_cmd == MACIOC_SETALLMCA)
				DLPI_PRINTF100(("MDI: Received MACIOC_SETALLMCA IOCNAK from MDI Driver\n"));
			else
				DLPI_PRINTF100(("MDI: Received IOCNAK to unknown command from MDI Driver\n"));
			cp->mdi_primitive_handler = mdi_inactive_handler;
			dlpi_send_iocnak(sp, EIO);   /* will set dlpi_iocblk to 0 */

			/* now reset remainder of MDI variables, emulating I_UNLINK code 
			 * necessary otherwise assertion at I_LINK fails because we never
			 * reset essential variables.  Also solves txmon firing race.
			 * note that as soon as dlpid sees that its I_PLINK failed it
			 * will immediately close both the mdi and netX file descriptors
			 * allowing our close routines to run, but we must still call
			 * mdi_shutdown_driver here.
			 */
			mdi_shutdown_driver(sp, 0);  /* don't send IOCACK up stream */
		}
	}

	freemsg(mp);
}

/*
 * Shut down the MDI driver 
 * - acquire txmon lock to prevent it from firing as we will be setting
 *   down_queue to NULL which txmon needs to use.  txmon will check
 *   mdi_driver_ready and not proceed to deference through down_queue if
 *   mdi_driver_ready is 0.
 *   We release lock prior to doing the putnext to not violate 
 *   locking hierarchy
 *   We know txmon lock is valid since it was allocated at first open.
 */
void
mdi_shutdown_driver(per_sap_info_t *sp, u_int sendack)
{
	pl_t opl;
	per_card_info_t *cp = sp->card_info;

	DLPI_PRINTF40(("MDI: Shutting down MDI Driver\n"));

	/* media_halt routines can call putnext, don't acquire lock just yet
	 * we also called media_init earlier so call halt if sendack == 0
	 */
	if (cp->media_specific && cp->media_specific->media_halt)
		cp->media_specific->media_halt(cp);

	opl = LOCK(cp->txmon.trylock, plstr);
	cp->control_sap = (per_sap_info_t *)NULL;
	cp->mdi_driver_ready = 0;
	cp->mdi_primitive_handler = mdi_inactive_handler;
	/* must set down_queue to NULL while holding lock to prevent txmon
	 * problems
	 */
	cp->down_queue = (queue_t *)NULL;

	UNLOCK(cp->txmon.trylock, opl);
	/* past this point, txmon can either:
	 * - start, at which point mdi_driver_ready is 0, so it will 
	 *   immediately return
	 * - resume running after the TRYLOCK at which point it will look 
	 *   at mdi_driver_ready and know not to do any work and 
	 *   immediately return.
	 * but in any case we don't hold lock as send_iocack does putnext.
	 */
	if (sendack == 1) {
		dlpi_send_iocack(sp);
	}
	return;					/* OK */
}

/****************************************************************************** *
* MDI send primitive functions
*
******************************************************************************/
/*
 * Send a MAC_BIND_REQ to the MDI Driver (generated as a result of an I_LINK
 * ioctl).
 */
STATIC void
mdi_send_mac_bind_req(per_sap_info_t *sp)
{
	mblk_t *mp;

	ASSERT(sp->card_info->mdi_primitive_handler == mdi_INIT_prim_handler);

	DLPI_PRINTF100(("DLPI: mdi_send_mac_bind_req(%x)\n",sp));

	/* check if a MAC_HWSUSPEND_IND somehow got sent up */
	if (sp->card_info->issuspended) {
		DLPI_PRINTF1000(("DLPI: suspended driver in mdi_send_mac_bind_req\n"));
		cmn_err(CE_WARN,
			"mdi_send_mac_bind_req: DDI8 driver is currently "
			"suspended, can't I_[P]LINK at this time\n");
		dlpi_send_iocnak(sp, EAGAIN);
		return;
	}

	if (!(mp = allocb(sizeof (mac_bind_req_t), BPRI_LO))) {
		sp->card_info->mdi_primitive_handler = mdi_inactive_handler;
		dlpi_send_iocnak(sp, ENOSR);
		return;
	}
	mp->b_datap->db_type = M_PCPROTO;
	((mac_bind_req_t *)mp->b_wptr)->mac_primitive = MAC_BIND_REQ;
	((mac_bind_req_t *)mp->b_wptr)->mac_cookie = (void *)sp->card_info;
	mp->b_wptr += sizeof(mac_bind_req_t);

	/*
	 * The bind acknowledgement code will handle the information
	 * returned, and will mark the MDI driver UP.
	 */
	dlpi_putnext_down_queue(sp->card_info, mp);	/* Ignore flow control */
}

/******************************************************************************
 * MDI Primitive Handler's:
 *	These functions process inbound MDI messages (coming from the MDI
 *	driver).  They are called mdi_primitive_handler()s.
 ******************************************************************************/

/*
 * Inactive mdi_primitive_handler().  Used when the DLPI module is NOT
 * connected to an MDI driver
 */
int
mdi_inactive_handler( queue_t *q, mblk_t *mp)
{
	DLPI_PRINTF08(("MDI-Inactive: Discarding MDI-message from MDI Driver\n"));
	freemsg(mp);
	return(1);					/* SUCCESS */
}

/*
 * Normal mdi_primitive_handler().  Used once the DLPI module has bound to
 * the MDI Driver and marked the interface UP.
 */
STATIC int
mdi_primitive_handler( queue_t *q, mblk_t *mp )
{
	register per_card_info_t *cp  = (per_card_info_t *)q->q_ptr;
	union MAC_primitives *prim;

	if (mp->b_datap->db_type == M_DATA)
	{
		register per_sap_info_t *sp;
		per_sap_info_t *sp1;
		mblk_t *mp1;
		struct per_frame_info f;
		DLPI_PRINTF08(("mdi primitive handler: Got m_data message\n"));

		ASSERT(msgdsize(mp) >= cp->mac_min_sdu);
		ASSERT(msgdsize(mp) <= cp->mac_max_sdu);

		f.unitdata = mp;
		cp->media_specific->rx_parse(cp, &f);

		if (cp->media_specific->media_flags & MEDIA_SOURCE_ROUTING)
		{
			DLPI_PRINTF200(("mdi primitive handler: checking source routing\n"));
			dlpiSR_rx_parse(cp, &f);
		}

		switch (f.frame_type)
		{
		case FR_ETHER_II:
		case FR_XNS:
			sp = dlpi_findsap(f.frame_type, f.frame_sap,cp);
			if (!sp)
				goto unbound;
			if (f.dest_type == FR_MULTICAST) {
				if (!dlpi_sap_mca_set(cp, sp, f.frame_dest))
					goto unbound;
			}
			if (!dlpi_canput(sp->up_queue))
				return(0);	/* dlpilrsrv frees message */
			dlpi_send_dl_unitdata_ind(sp, &f);
			break;

		case FR_LLC:
			if ((f.frame_sap == 0x00)) {	/* NULL DSAP */
				unchar *cmd = f.frame_data;

				DLPI_PRINTF200(("LLC NULL DSAP: cmd %b\n", cmd[2]));
				if (((cmd[2] & ~LLC_POLL_FINAL) == LLC_CMD_XID) ||
					((cmd[2] & ~LLC_POLL_FINAL) == LLC_CMD_TEST)) {
					per_sap_info_t sp;	/* craft tmp sap struct */

					sp.sap_type = FR_LLC;
					sp.sap = 0;
					sp.sap_state = SAP_BOUND;
					sp.sap_protid = 0;	/* not used here */
					sp.up_queue = 0;	/* not used here */
					sp.card_info = cp;
					sp.srmode = f.route_present ? SR_AUTO : 0;
					sp.llcmode = LLC_1;
					sp.dlpimode = DLPI20_STACK;
					sp.llcxidtest_flg = DL_AUTO_TEST;
					sp.filter[DL_FILTER_INCOMING] = NULL;
					sp.filter[DL_FILTER_OUTGOING] = NULL;
					if ( ! dlpi_alloc_sap_counters(&sp.sap_tx)) {
						break;
					}
					if ( ! dlpi_alloc_sap_counters(&sp.sap_rx)) {
						dlpi_dealloc_sap_counters(&sp.sap_tx);
						break;
					}
					dlpi_llc_rx(&sp, &f);
					dlpi_dealloc_sap_counters(&sp.sap_tx);
					dlpi_dealloc_sap_counters(&sp.sap_rx);
				}
				break;
			}
			sp = dlpi_findfirstllcsap(&f,cp);
			if (!sp)
				goto unbound;
		
			/* If multiple LLC SAPs must receive this frame, then
			 * copy it and send it to them */
			sp1 = sp;
			while (sp1 = dlpi_findnextllcsap(f.frame_sap, cp, 
					 sp1 + 1)) {
				if (f.dest_type == FR_MULTICAST) {
					if (!dlpi_sap_mca_set(cp, sp1, f.frame_dest))
						continue;
				}
				if ((f.unitdata = copymsg(mp)) == NULL) {
					ATOMIC_INT_INCR(sp->sap_rx.error);
					break;
				}
			
				if (!dlpi_canput(sp1->up_queue))
					continue;
				if (sp1->llcmode == LLC_OFF)
					dlpi_send_dl_unitdata_ind(sp1, &f);
				else
					dlpi_llc_rx(sp1, &f);
			}

			/* Send the message to the first SAP picked out above */
			if (!dlpi_canput(sp->up_queue))
				return(0);

			f.unitdata = mp;
			if ((f.dest_type == FR_MULTICAST) &&
				(!dlpi_sap_mca_set(cp, sp, f.frame_dest))) {
					return(0);	/* message freed */
			}
			if (sp->llcmode == LLC_OFF)
				dlpi_send_dl_unitdata_ind(sp, &f);
			else
				dlpi_llc_rx(sp, &f);
			break;

		default:
unbound:
			ATOMIC_INT_INCR(cp->rxunbound);
			freemsg(mp);
		}
		return (1);
	}

	prim = (union MAC_primitives *)mp->b_rptr;
	if (prim->mac_primitive == MAC_HWFAIL_IND)
	{
		per_sap_info_t *sp;
		pl_t opl;
		int s;

		DLPI_PRINTF2000(("mdi_primitive_handler: got MAC_HWFAIL_IND\n"));

		/* acquire txmon lock -- we don't want txmon to fire 
		 * while we're here
		 */
		opl = LOCK(cp->txmon.trylock, plstr);

		/* but txmon could have been running while we waited for 
		 * the lock so see if card is now marked as dead (txmon 
		 * just released the lock -or- we received I_[P]UNLINK on
		 * another cpu -- both set mdi_driver_ready to 0)
		 */
		if (cp->mdi_driver_ready == 0) {
			DLPI_PRINTF2000(("mdi_primitive_handler txmon already sent HWFAIL\n"));
			/* no need for us to send up yet another 
			 * HWFAIL_IND to dlpid 
			 */
			UNLOCK(cp->txmon.trylock, opl);
			freemsg(mp);
			return(1);
		}

		/* card still marked as up */
		ASSERT(cp->mdi_driver_ready == 1);
		s = LOCK_CARDINFO(cp);
		cp->mdi_driver_ready = 0;
		cp->mdi_primitive_handler = mdi_inactive_handler;
		cp->txmon.nrestarts++;  /* for historical data only */

		/*
		 * Pass the message to dlpid
		 */
		sp = cp->control_sap;
		if (sp && cp->dlpidRegistered) {
			/* do not set dlpidRegistered to 0, as txmon depends 
			 * on our being registered and dlpid unregisters upon 
			 * receiving HWFAIL_IND
			 * (DlpiHandler->RestartIf->StopIf)
			 */
			DLPI_PRINTF2000(("mdi_primitive_handler: sending up MACIOC_HWFAIL\n"));
			UNLOCK_CARDINFO(cp, s);
			UNLOCK(cp->txmon.trylock, opl);  /* don't hold across putnext */
			flushq(cp->down_queue->q_next, FLUSHALL);
			putnext(sp->up_queue, mp);
		} else {
			/* haven't seen DLPID_REGISTER ioctl yet; send up 
			 * failure message when we do an I_[P]LINK ioctl.  We 
			 * defer sending fail up because registration doesn't 
			 * affect hardware while an I_[P]LINK does
			 */
			cp->hwfail_waiting = mp;
			DLPI_PRINTF2000(("mdi_primitive_handler: postponing HWFAIL\n"));
			UNLOCK_CARDINFO(cp, s);
			UNLOCK(cp->txmon.trylock, opl);
		}
		return(1);
	} else
	if (prim->mac_primitive == MAC_HWSUSPEND_IND)
	{
		/* when TOS parameters established, if higher stack wants 
		 * notification of this event, build and send a new 
		 * scodlpi.h DL_HWSUSPEND_IND primitive upstream
		 */
		cp->issuspended = 1;
		DLPI_PRINTF1000(("DLPI: received MAC_HWSUSPEND_IND\n"));
		freemsg(mp);
		return(1);
	} else
	if (prim->mac_primitive == MAC_HWRESUME_IND)
	{
		/* when TOS parameters established, if higher stack wants 
		 * notification of this event, build and send a new 
		 * scodlpi.h DL_HWRESUME_IND primitive upstream
		 */
		cp->issuspended = 0;
		DLPI_PRINTF1000(("DLPI: received MAC_HWRESUME_IND\n"));
		freemsg(mp);
		return(1);
	}

	cmn_err(CE_NOTE,
		"mdi_primitive_handler - "
		"Unexpected message from MDI driver(%s), primitive=0x%x",
					MDI_DRIVER_NAME(cp), prim->mac_primitive);
	freemsg(mp);
	return(1);					/* SUCCESS */
}

/*
 * This function is the mdi_primitive_handler() during
 * the initialization of the MDI Driver.
 * can't be STATIC
 */

int
mdi_INIT_prim_handler(queue_t *q, mblk_t *mp)
{
	per_card_info_t *cp  = (per_card_info_t *)q->q_ptr;
	union MAC_primitives *prim = (union MAC_primitives *)mp->b_rptr;
	unchar allzeroMAC[MAC_ADDR_SZ] = {0};
	mblk_t          *mmp;
	mac_mcast_t     *mcast;
	macaddr_t       *mc;
	int i;

	switch (prim->mac_primitive)
	{
	case MAC_INFO_ACK:
	{
		mac_info_ack_t	*ip = (mac_info_ack_t *)prim;

		DLPI_PRINTF40(("mdi_INIT_prim_handler: MAC_INFO_ACK from MDI\n"));
		/*
		 * Since we have just now been told what mac_media_type
		 * to use, This is a good time to init the media
		 * for this card.
		 */

		if ((ip->mac_mac_type < 0) ||
			(ip->mac_mac_type > MAX_MEDIA_TYPE) ||
			!HAVE_MEDIA_SUPPORT(ip->mac_mac_type))
		{
			cmn_err(CE_WARN,
				"mdi_process_mac_info_ack: "
				"Illegal Media-Type from MDI Driver, type=%d\n",
					ip->mac_mac_type);
			cp->mdi_primitive_handler = mdi_inactive_handler;
			dlpi_send_iocnak(cp->control_sap, EIO);
		}
		else
		{
			cp->mac_driver_version = ip->mac_driver_version;
			cp->mac_media_type	 = ip->mac_mac_type;
			cp->mac_max_sdu		= ip->mac_max_sdu;
			cp->mac_min_sdu		= ip->mac_min_sdu;
			cp->mac_ifspeed		= ip->mac_if_speed;
			cp->media_specific	   = &mdi_media_info[ip->mac_mac_type];
			cp->set_mac_addr_pending = 0;

			if (cp->media_specific->media_init)
				cp->media_specific->media_init(cp);

			/* restore multicasting state on adapter restarts */
			if (! (mmp = mdi_get_mctable((void *)cp))) {
				DLPI_PRINTF100(("mdi_INIT_prim_handler: mdi_get_mctable failed\n"));
				cp->mdi_primitive_handler = mdi_inactive_handler;
				dlpi_send_iocnak(cp->control_sap, EIO);
				break;
			} else {
				mcast = (mac_mcast_t *)mmp->b_rptr;
				if (mcast->mac_all_mca) {
					mdi_send_MACIOC_SETALLMCA(cp->control_sap,
						mdi_INIT_ioctl_handler);
				}
				mc = (macaddr_t *)((ulong)mcast + mcast->mac_mca_offset);
				for (i = 0; i < mcast->mac_mca_count; i++, mc++) {
					mdi_send_MACIOC_SETMCA(cp->control_sap,
						mdi_INIT_ioctl_handler,
						(unchar *)mc);
				}
				freemsg(mmp);
			}

			if (bcmp(cp->local_mac_addr, &allzeroMAC, MAC_ADDR_SZ) == 0)
			{
				DLPI_PRINTF100(("mdi_INIT_prim_handler: Sending MACIOC_GETRADDR to MDI Driver\n"));
				mdi_send_MACIOC_GETRADDR(cp->control_sap, 
						mdi_INIT_ioctl_handler);
			}
			else if (bcmp(cp->local_mac_addr, cp->hw_mac_addr, MAC_ADDR_SZ))
			{
				DLPI_PRINTF100(("mdi_INIT_prim_handler: Sending MACIOC_SETADDR to MDI Driver\n"));
				mdi_send_MACIOC_SETADDR(cp->control_sap, 
						mdi_INIT_ioctl_handler,
						cp->local_mac_addr);
			} else {
				mdi_send_MACIOC_GETADDR(cp->control_sap, 
						mdi_INIT_ioctl_handler);
			}

			if (cp->media_specific->media_flags & MEDIA_SOURCE_ROUTING) {
				if ( !dlpiSR_init(cp) ) {
					cp->mdi_primitive_handler = mdi_inactive_handler;
					dlpi_send_iocnak(cp->control_sap, EAGAIN);
				}
			} else {
					/* no source routing on this media so free up any
					 * space allocated from netX Mtune file->config.h
					 */
				if (cp->route_table) {
					kmem_free((void *)cp->route_table->routes, 
						cp->maxroutes*sizeof(struct route));
					kmem_free((void *)cp->route_table,
						sizeof(struct route_table));
					cp->route_table = (struct route_table *)0;
				}
			}
		}
	}
	break;

	case MAC_OK_ACK:
	{
		mac_ok_ack_t *op = (mac_ok_ack_t *)prim;

		DLPI_PRINTF40(("mdi_INIT_prim_handler: MAC_OK_ACK from MDI\n"));
		switch ( op->mac_correct_primitive ) {
		case MAC_BIND_REQ:
			cp->mdi_driver_ready = 1;	/* Mark MDI driver as UP */
			cp->mdi_primitive_handler = mdi_primitive_handler;
			dlpi_send_iocack(cp->control_sap);
			break;
		}
	}
	break;

	case MAC_ERROR_ACK:
	{
		mac_error_ack_t *ep = (mac_error_ack_t *)prim;

		cmn_err(CE_WARN,
				"mdi_INIT_prim_handler - "
				"Received MAC_ERROR_ACK, prim=0x%x errno=0x%x",
					ep->mac_error_primitive, ep->mac_errno);
error:
		cp->mdi_primitive_handler = mdi_inactive_handler;
		dlpi_send_iocnak(cp->control_sap, EIO);
	}
	break;

	/* DDI8 NIC driver got CFG_SUSPEND in time period after dlpid opened
	 * mdi driver and before dlpid issued I_PLINK.  Rather than keep trace
	 * of issuspended throughout I_LINK leading to ugly state problems
	 * we just force the card to be in non-suspended state prior to the 
	 * I_LINK.  We catch MAC_HWSUSPEND_IND primitives that arrive after 
	 * I_LINK in mdi_primitive_handler()
	 * MDI drivers should call mdi_hw_suspended() to get to this point.
	 */
	case MAC_HWSUSPEND_IND:
	{
		DLPI_PRINTF1000(("DLPI: ddi8 driver suspended in INIT_prim_handler\n"));
		cmn_err(CE_WARN,
			"mdi_INIT_prim_handler: DDI8 driver is currently "
			"suspended, can't I_[P]LINK at this time\n");
		cp->mdi_primitive_handler = mdi_inactive_handler;
		cp->issuspended = 1;   /* for future reference */
		dlpi_send_iocnak(cp->control_sap, EAGAIN);
		/* freemsg takes place below */

	}
	break;

	/* DDI8 NIC driver was suspended earlier but now is not. this has no
	 * bearing on I_LINK so just tell user about event
	 * MDI drivers should call mdi_hw_resumed() to get to this point.
	 */
	case MAC_HWRESUME_IND:
	{
		DLPI_PRINTF1000(("DLPI: ddi8 driver resumed in INIT_prim_handler\n"));
		cmn_err(CE_NOTE, 
			"mdi_INIT_prim_handler: DDI8 driver was suspended, "
			"now resuming");
		cp->issuspended = 0;
		/* freemsg takes place below */
	}
	break;

	/*
	 * Messages from an ISDN MDI driver should just be passed up.
	 */
#ifdef ISDN_UNDER_CONSTRUCTION
	case MAC_ISDN_MSG:
		if (##Not bound to a channel)
		##
		else
		## Send up to net[n]
		break;
#endif

	default:
		cmn_err(CE_NOTE,
			"mdi_INIT_prim_handler - "
			"Unexpected message from MDI driver primitive=0x%x",
				prim->mac_primitive);
		goto error;
	}
	freemsg(mp);
	return(1);	/* SUCCESS */
}

/******************************************************************************
 * End of mdi_primitive_handlers()
 ******************************************************************************/

/*
 * Process a message received from the MDI Driver
 */
STATIC int
mdi_message( queue_t *q, mblk_t *mp)
{
	register per_card_info_t *cp  = (per_card_info_t *)q->q_ptr;
	mblk_t	*iocmp, *cp_mp;
	cp_ioctl_blk_t	*cp_iblk;
	struct iocblk	*ip;
	pl_t		s;

	if (cp == NULL) {
		return(0);
	}

#ifdef dlpi_frame_test
	/* want all M_PROTO type messages to get processed, but all others */
	/* to be ignored - this loops back DLPI mp's, but filters out all */
	/* legal mp types from the MDI driver */
	if (cp->dlpi_biloopmode) {
		if (mp->b_datap->db_type == M_PROTO) {
			DLPI_PRINTF08(("loop-back from DLPI, reconverting to M_DATA\n"));
			/* loop-back mp from DLPI, reconvert to M_DATA and process */
			mp->b_datap->db_type = M_DATA;
			/* force use of standard primitive handler */
			return (mdi_primitive_handler(q,mp));
		} else {
			/* let the mp get discarded below */
			mp->b_datap->db_type = M_PROTO; 
		}
	}
#endif
	switch (mp->b_datap->db_type) {
	case M_DATA:
	case M_PCPROTO:
		ASSERT(cp->mdi_primitive_handler);
		return (cp->mdi_primitive_handler(q,mp));
	case M_IOCACK:
	case M_IOCNAK:
		DLPI_PRINTF08(("MDI: Received IOCACK/NAK from MDI Driver\n"));
		ASSERT(cp->ioctl_handler);
		cp->ioctl_handler(cp->ioctl_sap, mp);
		s = LOCK_CARDINFO(cp);
 		/* no ioctl in progress and ioctl waiting */
 		if ((cp->dlpi_iocblk == NULL) && (cp->ioctl_queue != NULL)) {
				/* dequeue waiting ioctl */
 				cp_mp = cp->ioctl_queue;
				cp->ioctl_queue = cp_mp->b_next;

				/* restore values */
				cp_iblk = (cp_ioctl_blk_t *)cp_mp->b_rptr;
				cp->ioctl_sap = cp_iblk->ioctl_sap;
				cp->ioctl_handler = cp_iblk->ioctl_handler;
				iocmp = cp_mp->b_cont;
				cp->dlpi_iocblk = iocmp;

				/* unlock cp struct, clean up, deliver message, return */
				UNLOCK_CARDINFO(cp, s);
				freeb(cp_mp);
				ip = (struct iocblk *)iocmp->b_rptr;
				DLPI_PRINTF100(("MDI: Sending ioctl(0x%x) to MDI Driver\n", ip->ioc_cmd));
				dlpi_putnext_down_queue(cp,iocmp); /* Ignore flow control */
				return(1);
		}
		UNLOCK_CARDINFO(cp, s);
		return(1);		/* Ioctl handler will free the mp */
	case M_PROTO:
                if (cp->media_specific->media_rd_prim)
			switch (cp->media_specific->media_rd_prim(q, mp, cp))
			{
				case MPRIMRET_PUTBQ:
				return 0;

		       		case MPRIMRET_DONE:
		       		return 1;

		       		case MPRIMRET_NOT_MINE:
		       		default:
		       		break;
			}
		break;

	}
	freemsg(mp);
	return (1);					/* SUCCESS */
}

/*
 * Message Received from down queue (MDI Driver)
 */
void
dlpilrput(register queue_t *q, register mblk_t *mp)
{
	switch (mp->b_datap->db_type) {
	case M_PCPROTO:
	case M_PROTO:
	case M_DATA:
	case M_IOCACK:
	case M_IOCNAK:
		putq(q,mp);
		break;
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR) {
			flushq(q, FLUSHDATA);
		}
		if (*mp->b_rptr & FLUSHW) {
			*mp->b_rptr &= ~FLUSHR;
			qreply(q,mp);
		} else
			freemsg(mp);
		break;
	default:
		cmn_err(CE_NOTE, "dlpilrput - Discarding unknown message from MDI driver\n");
		freemsg(mp);
	}
}

/*
 * This queue is used to ensure that no-messages are processed by the DLPI
 * module at interrupt time.  We encourage MDI driver writers to send received
 * frames to the DLPI module at interrupt time and not use a read service
 * queue in their driver at all.
 *
 * This routine consumes data from the lrqueue and multiplexes to the upper
 * queues.  If a queue above is flow controlled, incomming data is dropped.
 * This prevents a process that isn't consuming messages from "hanging" the
 * entire subsystem.  Appropriate upper queue high-water marks must be selected.
 */
void
dlpilrsrv(register queue_t *q)
{
	register mblk_t *mp;

	while ( mp=getq(q) ) {
		if ( !mdi_message(q, mp) )
			freemsg(mp);
	}
}

/*
 * This queue is used for two reasons:
 *	(1) To queue requests generated by the DLPI module, to be sent to
 *		the MDI Driver, but that can't be delivered because the MDI driver's
 *		Write queue is full
 *	(2) To make sure that when the MDI driver's Write queue empties, we
 *		know about it and can enable all the Write queue's on the UP side
 *		of the DLPI module.
 */
void
dlpilwsrv(queue_t *q)
{
	per_card_info_t *cp = (per_card_info_t *)q->q_ptr;
	mblk_t *mp;

	DLPI_PRINTF80(("MDI: lwsrv starts\n"));

	while (mp = getq(q)) {
		/* if underlying DDI8 driver is suspended, silently drop 
		 * M_DATA frames that would be need to be sent out on the 
		 * wire.  M_IOCTL messages can still go through since they do 
		 * not *necessarily* involve the hardware.  If an ioctl 
		 * involves talking to the hardware and the driver is 
		 * suspended then it's the driver's responsibility 
		 * to NAK them with EAGAIN.
		 */
		if (cp->issuspended) {
			if (mp->b_datap->db_type == M_DATA) {
				DLPI_PRINTF1000(("DLPI: dropping packet in dlpilwsrv\n"));
				ATOMIC_INT_INCR(&cp->suspenddrops);
				freemsg(mp);
			}
		} else
		if (canputnext(q)) {
			putnext(q,mp);
		} else {
			putbq(q,mp);
			DLPI_PRINTF80(("MDI: lwsrv ends with full queue queue\n"));
			return;
		}
	}
	dlpi_enable_WRqs( (per_card_info_t *)q->q_ptr );
	DLPI_PRINTF80(("MDI: lwsrv empies queue\n"));
}
