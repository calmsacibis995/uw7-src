#ident "@(#)dlpi.c	27.1"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1996.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>
#include <io/nd/dlpi/bpf.h>

#else

#include "include.h"
#include "bpf.h"

#endif


/******************************************************************************
 *
 * DLPI Interface Implementation.
 *
 ******************************************************************************/

void
dlpi_send_dl_error_ack(per_sap_info_t *sp, ulong prim, int err, int uerr)
{
	dl_error_ack_t	*ep;
	mblk_t *mp;

	DLPI_PRINTF40(("DLPI: Sending DL_ERROR_ACK(0x%x,%d) to DLPI user\n",
								prim, err));
	mp = allocb(sizeof (dl_error_ack_t), BPRI_HI);
	if (!mp) {
		cmn_err(CE_WARN, "dlpi_send_dl_error_ack - STREAMs memory shortage");
		return;
	}

	ep = (dl_error_ack_t *)mp->b_rptr;
	mp->b_wptr += sizeof(dl_error_ack_t);

	mp->b_datap->db_type = M_PCPROTO;
	ep->dl_primitive = DL_ERROR_ACK;
	ep->dl_error_primitive = prim;
	ep->dl_errno = err;
	ep->dl_unix_errno = uerr;

	putnext(sp->up_queue,mp);		/* Ignore flow control */
}


STATIC void
dlpi_send_dl_ok_ack(per_sap_info_t *sp, ulong prim)
{
	dl_ok_ack_t	*ep;
	mblk_t *mp;

	DLPI_PRINTF40(("DLPI: Sending DL_OK_ACK to DLPI user\n"));
	mp = allocb(sizeof (dl_ok_ack_t), BPRI_HI);
	if (!mp) {
		cmn_err(CE_WARN, "dlpi_send_dl_ok_ack_t - STREAMs memory shortage");
		return;
	}

	ep = (dl_ok_ack_t *)mp->b_rptr;
	mp->b_wptr += sizeof(dl_ok_ack_t);
	mp->b_datap->db_type = M_PCPROTO;

	ep->dl_primitive = DL_OK_ACK;
	ep->dl_correct_primitive = prim;

	putnext(sp->up_queue,mp);		/* Ignore flow control */
}


STATIC void
dlpi_send_dl_bind_ack(per_sap_info_t *sp, int sap, int dlpimode,
			int llcmode, int xidtestflg)
{
	dl_bind_ack_t	*bp;
	mblk_t *mp;

	/* From DLPI Specification Revision 2.0.0, August 20, 1991
	 *
	 * Section 4.1.6, "Message DL_BIND_REQ", pp. 36-37:
	 * ------------------------------------------------
	 * "A stream is viewed as active when the DLS provider may transmit 
	 * and receive protocol data units destined to or originating from 
	 * the stream.  The PPA associated with each stream must be 
	 * initialized upon completion of the processing of the DL_BIND_REQ
	 * (see section 4.1.1, PPA Initialization/Deinitialization).  More
	 * specificially, the DLS user is ensured that the PPA is initialized
	 * when the DL_BIND_ACK is received.  If the PPA cannot be 
	 * initialized, the DL_BIND_REQ will fail."
	 * 
	 * Note that DL_ATTACH_REQ would also be impacted by automatic PPA
	 * initialization, except we don't support style 2 yet.
	 *
	 * Section 4.1.1, "PPA Initialization/De-initialization", p.28:
	 * ------------------------------------------------------------
	 * "If pre-initialization has not been performed and/or automatic
	 * initialization fails, the DLS provider will fail the DL_BIND_REQ.  
	 * Two errors, DL_INITFAILED and DL_NOTINIT, may be returned in the 
	 * DL_ERROR_ACK response to a DL_BIND_REQ if PPA initialization fails.
	 * DL_INITFAILED is returned when a DLS provider supports automatic 
	 * PPA initialization, but the initialization attempt failed.  
	 * DL_NOTINIT is returned when the DLS provider requires 
	 * pre-initialization, but the PPA is not initialized before the 
	 * DL_BIND_REQ is received."
	 *
	 * DDI8 CFG_SUSPEND note:
	 *
	 * If underlying MDI driver is suspended then we know the PPA has been
	 * taken offline (no interrupts, etc) and the MDI driver cannot talk 
	 * to the hardware.  As a result we must fail the DL_BIND_REQ since
	 * subsequent DL_UNITDATA_REQ's would be dropped by the MDI driver.
	 */
	if (sp->card_info->issuspended) {
		DLPI_PRINTF1000(("dlpi_send_dl_bind_ack: MDI driver is suspended\n"));
		dlpi_send_dl_error_ack(sp, DL_BIND_REQ, DL_INITFAILED, 0);
		return;
	}

	DLPI_PRINTF20(("DLPI: Sending DL_BIND_ACK to DLPI user\n"));
	if (sp->sap_state != SAP_ALLOC) {
		DLPI_PRINTF20(("dlpi_send_dl_bind_ack: sap_state %x\n", sp->sap_state));
		dlpi_send_dl_error_ack(sp, DL_BIND_REQ, DL_OUTSTATE, 0);
		return;
	}

	if (!dlpi_dobind(sp, sap, llcmode, xidtestflg)) {
		/* Illegal SAP */
		DLPI_PRINTF20(("dlpi_send_dl_bind_ack: dlpi_dobind failed\n"));
		dlpi_send_dl_error_ack(sp, DL_BIND_REQ, DL_BADSAP, 0);
		return;
	}

	mp = allocb(sizeof (dl_bind_ack_t)+MAC_ADDR_SZ+1, BPRI_HI);
	if (!mp) {
		cmn_err(CE_WARN, "dlpi_send_dl_bind_ack - STREAMs memory shortage");
		dlpi_send_dl_error_ack(sp, DL_BIND_REQ, DL_SYSERR, ENOSR);
		return;
	}

	bp = (dl_bind_ack_t *)mp->b_rptr;
	mp->b_wptr += sizeof(dl_bind_ack_t);
	mp->b_datap->db_type = M_PCPROTO;

	bp->dl_primitive = DL_BIND_ACK;
	bp->dl_sap = sap;
	bp->dl_addr_length = MAC_ADDR_SZ + ((sp->llcmode==LLC_1) ? 1 : 0);
	bp->dl_addr_offset = sizeof(dl_bind_ack_t);
	bp->dl_max_conind = 0;
	bp->dl_xidtest_flg = sp->llcxidtest_flg;
	sp->dlpimode = dlpimode;

	bcopy(sp->card_info->local_mac_addr, mp->b_wptr, MAC_ADDR_SZ);
	mp->b_wptr += MAC_ADDR_SZ;
	/* LLC turned on, address is 7 bytes */
	if (sp->llcmode == LLC_1)
		*mp->b_wptr++ = (unchar)sp->sap;

	putnext(sp->up_queue,mp);		/* Ignore flow control */
}

STATIC void
dlpi_send_dl_subs_bind_ack(per_sap_info_t *sp, mblk_t *mp)
{
	dl_subs_bind_req_t *dp = (dl_subs_bind_req_t *)mp->b_rptr;
	dl_subs_bind_ack_t *rp;
	per_sap_info_t *new_sp;
	mblk_t *mp1;
	int s;

	if (sp->sap_state != SAP_ALLOC) {
		dlpi_send_dl_error_ack(sp, DL_SUBS_BIND_REQ, DL_OUTSTATE, 0);
		return;
	}
	mp1 = allocb(sizeof(dl_subs_bind_ack_t) + dp->dl_subs_sap_length, 
					 BPRI_MED);
	if (!mp1) {
		dlpi_send_dl_error_ack(sp, DL_SUBS_BIND_REQ, DL_SYSERR, ENOSR);
		return;
	}
	if (sp->sap_type == FR_SNAP 
		&& dp->dl_subs_sap_length == sizeof(struct sco_snap_sap) 
#ifdef UNIXWARE_TCP
		)	/* UnixWare 2.1 slink's dl_subs_bind_class is 0 */
#else
		&& dp->dl_subs_bind_class == DL_HIERARCHICAL_BIND)
#endif
	{

		struct sco_snap_sap *snp;

		snp = (struct sco_snap_sap *)(mp->b_rptr + dp->dl_subs_sap_offset);

		switch (snp->prot_id) {
		case 0:
			if (snp->type <= 1500 || snp->type > 0xffff || snp->reserved)
				goto illegal_bind;
				break;
		}

		/* Make sure no-one already bound to this SNAP SAP */
		s = LOCK_SAP_ID(sp->card_info);
			if (dlpi_findsnapsap(snp->prot_id, snp->type, sp->card_info)) {
				UNLOCK_SAP_ID(sp->card_info, s);
				goto illegal_bind;
			}
		sp->sap_state = SAP_BOUND;
		UNLOCK_SAP_ID(sp->card_info, s);

		sp->sap_protid = snp->prot_id;
		sp->sap = snp->type;
		rp = (dl_subs_bind_ack_t *)mp1->b_rptr;
		mp1->b_wptr += sizeof(dl_subs_bind_ack_t) + dp->dl_subs_sap_length;
		mp1->b_datap->db_type = M_PCPROTO;

		rp->dl_primitive = DL_SUBS_BIND_ACK;
		rp->dl_subs_sap_offset = sizeof(dl_subs_bind_ack_t);
		rp->dl_subs_sap_length = dp->dl_subs_sap_length;
		bcopy(mp->b_rptr + dp->dl_subs_sap_offset,
			  mp1->b_rptr + rp->dl_subs_sap_offset, rp->dl_subs_sap_length);
		putnext(sp->up_queue, mp1);		/* Ignore flow control */
		return;
	}
illegal_bind:
	freemsg(mp1);
	dlpi_send_dl_error_ack(sp, DL_SUBS_BIND_REQ, DL_BADADDR, 0);
	return;
}

STATIC void
dlpi_send_dl_subs_unbind_ack(per_sap_info_t *sp, mblk_t *mp)
{
	dl_subs_bind_req_t *dp = (dl_subs_bind_req_t *)mp->b_rptr;
	per_sap_info_t *new_sp = (per_sap_info_t *)0;
	struct sco_snap_sap *snp;

	snp = (struct sco_snap_sap *)(mp->b_rptr + dp->dl_subs_sap_offset);

	if (sp->sap_type == FR_SNAP && sp->sap_state == SAP_BOUND
		&& dp->dl_subs_sap_length == sizeof(struct sco_snap_sap)
		&& snp->type == sp->sap && snp->prot_id == sp->sap_protid) {
		sp->sap_state = SAP_ALLOC;
		dlpi_send_dl_ok_ack(sp, DL_SUBS_UNBIND_REQ);
	}
	else
		dlpi_send_dl_error_ack(sp, DL_SUBS_UNBIND_REQ, DL_BADADDR, 0);
}


STATIC void
dlpi_send_dl_info_ack(per_sap_info_t *sp)
{
	per_card_info_t	*cp = sp->card_info;
	dl_info_ack_t	*bp;
	mblk_t		*mp;
	int		i;

	DLPI_PRINTF40(("DLPI: Sending DL_INFO_ACK to DLPI user\n"));
	mp = allocb(sizeof (dl_info_ack_t)+2*(MAC_ADDR_SZ+1), BPRI_HI);
	if (!mp) {
		cmn_err(CE_WARN, "dlpi_send_dl_info_ack - STREAMs memory shortage");
		dlpi_send_dl_error_ack(sp, DL_INFO_REQ, DL_SYSERR, ENOSR);
		return;
	}

	bp = (dl_info_ack_t *)mp->b_rptr;
	mp->b_wptr += sizeof(dl_info_ack_t);
	mp->b_datap->db_type = M_PCPROTO;

	bp->dl_primitive = DL_INFO_ACK;
	bp->dl_max_sdu =
	cp->mac_max_sdu -
	(cp->media_specific ? cp->media_specific->hdr_sz : 0) -		/* MAC */
	((sp->llcmode == LLC_1) ? 3 : 0) -			/* LLC header */
	((sp->llcmode == LLC_SNAP) ? 8 : 0) -			/* SNAP hdr */
	((sp->srmode == SR_NON) ? 0 : (MAX_ROUTE_SZ + 2));  /* route field */

	bp->dl_min_sdu = 1;
	bp->dl_addr_length =
	bp->dl_brdcst_addr_length = MAC_ADDR_SZ + ((sp->llcmode==LLC_1)?1:0);
	bp->dl_mac_type = cp->mac_media_type;
	bp->dl_current_state = sp->sap_state==SAP_BOUND ? DL_IDLE : DL_UNBOUND;
	bp->dl_sap_length = (sp->llcmode==LLC_1) ? 1 : 0;
	bp->dl_service_mode = DL_CLDLS;
	bp->dl_qos_length = 0;		/* No QOS supported */
	bp->dl_provider_style = DL_STYLE1;	/* PPA implicitly attached */
	bp->dl_addr_offset = 0;
	bp->dl_version = 2;
	bp->dl_brdcst_addr_offset = sizeof(dl_info_ack_t);
	for (i=bp->dl_brdcst_addr_length; i; --i)
		*(mp->b_wptr++) = 0xff;
	bp->dl_addr_offset=bp->dl_brdcst_addr_offset+bp->dl_brdcst_addr_length;
	bcopy(cp->local_mac_addr, mp->b_wptr, MAC_ADDR_SZ);
	mp->b_wptr += MAC_ADDR_SZ;
	if (sp->llcmode == LLC_1)
		*mp->b_wptr++ = (unchar)sp->sap;

	bp->dl_growth = 0;
	putnext(sp->up_queue,mp);		/* Ignore flow control */
}

STATIC void
dlpi_send_dl_phys_addr_ack(per_sap_info_t *sp, ulong dl_addr_type)
{
	per_card_info_t		*cp = sp->card_info;
	dl_phys_addr_ack_t	*pp;
	mblk_t			*mp;

	DLPI_PRINTF40(("DLPI: Sending DL_GET_PHYSADDR_ACK to DLPI User\n"));

#   define PAMSGSZ	(sizeof(dl_phys_addr_ack_t) + MAC_ADDR_SZ)
	if (!(mp = allocb(PAMSGSZ, BPRI_MED))) {
		dlpi_send_dl_error_ack(sp, DL_PHYS_ADDR_REQ, DL_SYSERR, ENOSR);
		return;
	}

	pp = (dl_phys_addr_ack_t *)mp->b_rptr;
	mp->b_wptr += sizeof(dl_phys_addr_ack_t);
	mp->b_datap->db_type = M_PCPROTO;

	pp->dl_primitive = DL_PHYS_ADDR_ACK;
	pp->dl_addr_length = MAC_ADDR_SZ;
	pp->dl_addr_offset = sizeof(dl_phys_addr_ack_t);
	if (dl_addr_type == DL_FACT_PHYS_ADDR)
		bcopy(cp->hw_mac_addr, mp->b_wptr, MAC_ADDR_SZ);
	else
		bcopy(cp->local_mac_addr, mp->b_wptr, MAC_ADDR_SZ);
	mp->b_wptr += MAC_ADDR_SZ;
	putnext(sp->up_queue,mp);		/* Ignore flow control */
}

/*
 * MP Notes:
 * - lock around get of statistics from card: someone could be
 *   in the middle of dlpiclose, in which case we would get a
 *   set of stats which was partially updated
 * - lock around get of sap statistics, dlpiclose clears it, as
 *   does a CLR_STATS ioctl, so we want to make sure we present
 *   a consistent set of numbers
 */
STATIC void
dlpi_send_dl_get_statistics_ack(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t		*cp = sp->card_info;
	mblk_t *mp1;
	struct iocblk		*iocp;

	dl_get_statistics_ack_t	*ap;
	struct dlpi_stats	*ip;

	per_sap_info_t		*spt;
	int			x;

	/* ioctl is either done, or an error occurred: let the next ioctl run */
	cp->ioctl_sap = 0;
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(cp);

	if (!mp) {
		dlpi_send_dl_error_ack(sp, DL_GET_STATISTICS_REQ, DL_SYSERR, ENOSR);
		return;
	}
	iocp = (struct iocblk *)mp->b_rptr;
	if (mp->b_datap->db_type == M_IOCNAK) {
		dlpi_send_dl_error_ack(sp, DL_GET_STATISTICS_REQ, DL_SYSERR, 
							   iocp->ioc_error);
		freemsg(mp);
		return;
	}

	DLPI_PRINTF40(("DLPI: Sending DL_GET_STATS_ACK to DLPI user\n"));

#   define SAMSGSZ sizeof(dl_get_statistics_ack_t) + sizeof (struct dlpi_stats)
	mp1 = allocb(SAMSGSZ, BPRI_MED);
	if (!mp1) {
		cmn_err(CE_WARN, 
				 "dlpi_send_dl_get_statistics_ack - STREAMs memory shortage");
		dlpi_send_dl_error_ack(sp, DL_GET_STATISTICS_REQ, DL_SYSERR, ENOSR);
		freemsg(mp);
		return;
	}
	mp1->b_datap->db_type = M_PCPROTO;
	linkb(mp1, mp->b_cont);		/* Attach the H/W Dependent stats */
	freeb(mp);

	bzero(mp1->b_rptr, SAMSGSZ);
	ap = (dl_get_statistics_ack_t *)mp1->b_rptr;
	mp1->b_wptr += SAMSGSZ;
	ap->dl_primitive	= DL_GET_STATISTICS_ACK;
	ap->dl_stat_length	= sizeof(struct dlpi_stats);
	ap->dl_stat_offset	= sizeof(dl_get_statistics_ack_t);

	ip=(struct dlpi_stats *)(mp1->b_rptr +sizeof(dl_get_statistics_ack_t));
	ip->dl_maxsaps		= cp->maxsaps;
	ip->dl_rxunbound	= ATOMIC_INT_READ(cp->rxunbound);
	ip->mac_driver_version	= cp->mac_driver_version;
	ip->mac_media_type	= cp->mac_media_type;
	ip->mac_max_sdu		= cp->mac_max_sdu;
	ip->mac_min_sdu		= cp->mac_min_sdu;
	if ((cp->mac_media_type == MAC_ISDN_BRI)
			|| (cp->mac_media_type == MAC_ISDN_PRI))
		ip->mac_addr_length = 0;
	else
		ip->mac_addr_length	= MAC_ADDR_SZ;
	ip->mac_stats_len	= cp->media_specific ? cp->media_specific->stats_sz : 0;
	ip->mac_ifspeed		= cp->mac_ifspeed;

	ip->mac_tx.mac_queue_len =	qsize(cp->down_queue) +
					qsize(cp->down_queue->q_next);
	ip->mac_rx.mac_queue_len =	qsize(RD(cp->down_queue));
	ip->mac_tx.mac_ucast = ATOMIC_INT_READ(cp->old_tx.ucast) +
			 	ATOMIC_INT_READ(cp->old_tx.ucast_test) +
				ATOMIC_INT_READ(cp->old_tx.ucast_xid);
	ip->mac_tx.mac_bcast = ATOMIC_INT_READ(cp->old_tx.bcast);
	ip->mac_tx.mac_mcast = ATOMIC_INT_READ(cp->old_tx.mcast);
	ip->mac_tx.mac_error = ATOMIC_INT_READ(cp->old_tx.error);
	ip->mac_tx.mac_octets= ATOMIC_INT_READ(cp->old_tx.octets);
	ip->mac_rx.mac_ucast = ATOMIC_INT_READ(cp->old_rx.ucast) +
				ATOMIC_INT_READ(cp->old_rx.ucast_test) +
				ATOMIC_INT_READ(cp->old_rx.ucast_xid);
	ip->mac_rx.mac_bcast = ATOMIC_INT_READ(cp->old_rx.bcast);
	ip->mac_rx.mac_mcast = ATOMIC_INT_READ(cp->old_rx.mcast);
	ip->mac_rx.mac_error = ATOMIC_INT_READ(cp->old_rx.error);
	ip->mac_rx.mac_octets= ATOMIC_INT_READ(cp->old_rx.octets);
	for (spt=cp->sap_table, x=cp->maxsaps; x; --x, ++spt) {
		if (spt->sap_state == SAP_FREE)
			continue;
		if (spt->sap_state == SAP_BOUND)
			ip->dl_nsaps++;

		ip->mac_tx.mac_ucast += ATOMIC_INT_READ(spt->sap_tx.ucast) +
					ATOMIC_INT_READ(spt->sap_tx.ucast_test) +
					ATOMIC_INT_READ(spt->sap_tx.ucast_xid);
		ip->mac_tx.mac_bcast += ATOMIC_INT_READ(spt->sap_tx.bcast);
		ip->mac_tx.mac_mcast += ATOMIC_INT_READ(spt->sap_tx.mcast);
		ip->mac_tx.mac_error += ATOMIC_INT_READ(spt->sap_tx.error);
		ip->mac_tx.mac_octets +=ATOMIC_INT_READ(spt->sap_tx.octets);
		ip->mac_rx.mac_ucast += ATOMIC_INT_READ(spt->sap_rx.ucast) +
					ATOMIC_INT_READ(spt->sap_rx.ucast_test) +
					ATOMIC_INT_READ(spt->sap_rx.ucast_xid);
		ip->mac_rx.mac_bcast += ATOMIC_INT_READ(spt->sap_rx.bcast);
		ip->mac_rx.mac_mcast += ATOMIC_INT_READ(spt->sap_rx.mcast);
		ip->mac_rx.mac_error += ATOMIC_INT_READ(spt->sap_rx.error);
		ip->mac_rx.mac_octets+= ATOMIC_INT_READ(spt->sap_rx.octets);
		ip->mac_rx.mac_queue_len += qsize(spt->up_queue);
		ip->mac_tx.mac_queue_len += qsize(WR(spt->up_queue));
	}

	/*
	 * Add on the h/w dependent errors
	 */
	ip->mac_tx.mac_error +=
		cp->media_specific && cp->media_specific->tx_errors
			? cp->media_specific->tx_errors(mp1->b_cont->b_rptr) : 0;
	ip->mac_rx.mac_error +=
		cp->media_specific && cp->media_specific->rx_errors
			? cp->media_specific->rx_errors(mp1->b_cont->b_rptr) : 0;

	/* new ddi8 statistics if card is suspended.  Physically located at the end
	 * of dlpi_stats statistics structure so old code won't break
	 */
	ip->mac_suspended = cp->issuspended;
	ip->mac_suspenddrops = ATOMIC_INT_READ(&cp->suspenddrops);

	/* ip->mac_reserved isn't documented but useful for things that read
	 * directly from /dev/kmem that want direct access to these structures
	 * note intepretation of mac_reserved can change in the future!
	 */
	ip->mac_reserved[0] = (ulong) sp;
	ip->mac_reserved[1] = (ulong) cp;
	ip->mac_reserved[2] = (ulong) cp->sap_table;

	putnext(sp->up_queue, mp1);		/* Ignore flow control */
}

STATIC void
dlpi_send_dl_clr_statistics_ack(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t	*cp = sp->card_info;
	per_sap_info_t *x;
	int i;
	struct iocblk *		 iocp;

	/* ioctl is either done, or an error occurred: let the next ioctl run */
	cp->ioctl_sap = 0;
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(cp);

	if (!mp) {
		dlpi_send_dl_error_ack(sp, DL_CLR_STATISTICS_REQ, DL_SYSERR, ENOSR);
		return;
	}

	iocp = (struct iocblk *)mp->b_rptr;
	if (mp->b_datap->db_type == M_IOCNAK)
		dlpi_send_dl_error_ack(sp, DL_CLR_STATISTICS_REQ, DL_SYSERR,
							   iocp->ioc_error);
	else {
		int s;

		dlpi_send_dl_ok_ack(sp, DL_CLR_STATISTICS_REQ);

		ATOMIC_INT_INIT(&cp->suspenddrops, 0);

		x = cp->sap_table;
		for (i = cp->maxsaps; i; --i, ++x) {
			dlpi_clear_sap_counters(&x->sap_tx);
			dlpi_clear_sap_counters(&x->sap_rx);
		}
	}
	freemsg(mp);
}

STATIC void
dlpi_send_dl_sap_statistics_ack(per_sap_info_t *sp)
{
	mblk_t	*mp;
	per_card_info_t *cp = sp->card_info;
	dl_sap_statistics_ack_t *ap;
	int i, n = 0;
	per_sap_info_t *x = cp->sap_table;

	for (i = cp->maxsaps; i; --i) {
		if (x->sap_state == SAP_BOUND)
			++n;
		++x;
	}

#define SAPASIZ n*sizeof(struct dlpi_sapstats)+sizeof(dl_sap_statistics_ack_t)
	mp = allocb(SAPASIZ, BPRI_MED);
	if (!mp) {
		cmn_err(CE_WARN, 
			 "dlpi_send_dl_sap_statistics_ack - STREAMs memory shortage");
		dlpi_send_dl_error_ack(sp,DL_SAP_STATISTICS_REQ,DL_SYSERR,ENOSR);
		return;
	}
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_wptr += SAPASIZ;
	
	ap = (dl_sap_statistics_ack_t *)mp->b_rptr;
	ap->dl_primitive = DL_SAP_STATISTICS_ACK;
	ap->dl_sapstats_offset = sizeof(dl_sap_statistics_ack_t);
	ap->dl_sapstats_len = n*sizeof(struct dlpi_sapstats);
	if (n)
		dlpi_dumpsaps(mp->b_rptr + ap->dl_sapstats_offset,
					  ap->dl_sapstats_len, cp);
	
	putnext(sp->up_queue, mp);		/* Ignore flow control */
}

/*
 * Function: dlpi_mcahashtbl_entry
 *
 * Purpose:
 *   Verify that a given multicast address has been set on an adapter.
 *
 * Returns:
 *   multicast address hash table entry
 *
 * LOCK state:
 *	must be called with lock_mca read or write lock held
 *
 * Notes:
 *   very similar to mdi_valid_mca()
 */
STATIC card_mca_t *
dlpi_mcahashtbl_entry(void *cp, macaddr_t addr)
{
	card_mca_t *cm;
	uint hsh;

	ASSERT(cp);
	hsh = dlpi_hsh(addr, sizeof(macaddr_t));
	DLPI_PRINTF400(("dlpi_mcahashtbl_entry(cp%x, %b:%b:%b:%b:%b:%b) hsh %x tbl %x\n",
		cp, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
		hsh, ((per_card_info_t *)cp)->mcahashtbl[hsh]));
	cm = ((per_card_info_t *)cp)->mcahashtbl[hsh];
	while (cm) {
		if (mdi_addrs_equal(cm->mca, addr))
			break;
		cm = cm->next_mca;
	}
	return(cm);
}

/*
 * dlpi_sap_mca_set()
 *
 * returns 1 if a multicast address is valid on this sap, 0 otherwise
 * called during receive packet processing by mdi_primitive_handler()
 */
dlpi_sap_mca_set(per_card_info_t *cp, per_sap_info_t *sp, unchar *addr)
{
	card_mca_t *cm;
	sap_mca_t *sm;
	pl_t s;

	DLPI_PRINTF400(("dlpi_sap_mca_set(cp%x, sp%x addr%b:%b:%b:%b:%b:%b)\n",
		cp, sp, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]));
	s = RDLOCK_MCA(cp);
	if (sp->all_mca) {
		UNLOCK_MCA(cp, s);
		return(1);
	}
	if (! (cm = dlpi_mcahashtbl_entry(cp, addr))) {
		UNLOCK_MCA(cp, s);
		return(0);
	}
	for (sm = &cm->sap_mca; sm; sm = sm->next_sap) {
		if (sm->sp == sp) {
			UNLOCK_MCA(cp, s);
			return(1);
		}
	}
	UNLOCK_MCA(cp, s);
	return(0);
}

/*
 * Purpose:
 *	Allocate a multicast sap structure and attach to sap_mca list
 *
 * Returns:
 *	TRUE if new sap struct was allocated, FALSE if allocation failure
 *
 * LOCK state:
 *	must be called with multicasting write lock held
 */
STATIC int
dlpi_alloc_mca(per_card_info_t *cp, per_sap_info_t *sp, macaddr_t addr)
{
	card_mca_t *cm, *pcm;
	sap_mca_t *sm;
	uint hsh;
	pl_t s;

	hsh = dlpi_hsh(addr, MAC_ADDR_SZ);
	DLPI_PRINTF400(("dlpi_alloc_mca(cp %x, sp %x hsh %x %b:%b:%b:%b:%b:%b)\n",
		cp, sp, hsh, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]));

	pcm = cm = cp->mcahashtbl[hsh];
	while (cm) {
		if (mdi_addrs_equal(cm->mca, addr)) {
			/* addr in use by another stream */
			sm = &cm->sap_mca;
			while (sm->next_sap) {
				if (sm->sp == sp) {
					/* silly stack already set same mca */
					return(TRUE);
				}
				sm = sm->next_sap;
			}
			if (! (sm->next_sap = (sap_mca_t *)kmem_zalloc(sizeof(sap_mca_t), KM_NOSLEEP))) {
				return(FALSE);
			}
			sm->next_sap->sp = sp;
			return(TRUE);
		}
		pcm = cm;
		cm = cm->next_mca;
	}
	if (! cm) {		/* mca not in use - allocate new card_mca */
		if (! pcm) {
			/* hash bucket empty - fill it */
			if (! (cm = cp->mcahashtbl[hsh] = (card_mca_t *)kmem_zalloc(sizeof(card_mca_t), KM_NOSLEEP))) {
				return(FALSE);
			}
		} else {
			if (! (cm = pcm->next_mca = (card_mca_t *)kmem_zalloc(sizeof(card_mca_t), KM_NOSLEEP))) {
				return(FALSE);
			}
		}
		bcopy(addr, cm->mca, sizeof(macaddr_t));
		cm->sap_mca.sp = sp;
	}

	return(TRUE);
}

/*
 * Delete a sap from the multicast sap list
 *
 * Returns:
 *	 0 if there are still saps attached to this mca (ok to delete card_mca)
 *	 1 if there are no more saps attached to this mca
 *
 * LOCK state:
 *	must be called with multicasting write lock held
 */
STATIC int
dlpi_delete_mca_sap(sap_mca_t *sm, per_sap_info_t *sp)
{
	sap_mca_t *psm;

	DLPI_PRINTF400(("dlpi_delete_mca_sap(sm %x sp %x)\n", sm, sp));

	if (! sm->next_sap)
		return(1);	/* card_mca will be deleted from mca hash table */

	if (sm->sp == sp) {		/* first sap_mca is in card_mca - free next */
		psm = sm;
		sm = sm->next_sap;
		psm->sp = sm->sp;
		psm->next_sap = sm->next_sap;
	} else {
		do {
			psm = sm;
			sm = sm->next_sap;
			if (sm->sp == sp)
				break;
		} while (sm);
		if (! sm) {
			cmn_err(CE_WARN, "dlpi_selete_mca_sap: bad sap list\n");
			return(0);
		} else {
			psm->next_sap = sm->next_sap;
		}
	}
	kmem_free((void *)sm, sizeof(sap_mca_t));

	return(0);
}


/*
 * Used as MACIOC_DELMCA ioctl handler when deleting MCAs due to stream close
 * WRLOCK_MCA still held
 */
STATIC void
dlpi_delmca_ack(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t *cp;
	
	if (sp) {
		cp = sp->card_info;
		/* let the next ioctl run */
		cp->ioctl_sap = 0;
		cp->dlpi_iocblk = 0;
	}
	if (mp) {
		freemsg(mp);
	}
	return;
}

/*
 * Searches through the entire multicast table to cleanup after
 * badly behaved DLS users that close stream without explicitly
 * deleting their multicast addresses and/or ALLMCA reception
 *
 * LOCK state:
 *	Called on sap close with no locks held
 */
void
dlpi_clear_sap_multicasts(per_card_info_t *cp, per_sap_info_t *sp)
{
	register ulong i;
	register per_sap_info_t *tsp, *sap_table = cp->sap_table;
	card_mca_t *cm, *pcm;
	sap_mca_t *sm;
	pl_t s, c;
	int last_stream;

	DLPI_PRINTF400(("dlpi_clear_sap_multicasts(cp%x sp%x)\n", cp, sp));

	s = WRLOCK_MCA(cp);
	for (i = 0; i < cp->nmcahash; i++) {
		cm = cp->mcahashtbl[i];
		while (cm) {
			last_stream = 0;
			sm = &cm->sap_mca;
			while (sm) {
				if (sm->sp == sp) {
					if ((last_stream = dlpi_delete_mca_sap(&cm->sap_mca, sp))) {
						/* deleted last stream for this mca */ 
						c = LOCK_CARDINFO(cp);
						if (cp->dlpi_iocblk && cp->dlpi_iocblk != (mblk_t *)0xBF) {
							DLPI_PRINTF400(("dlpi_clear_sap_multicasts() mca iocblk busy\n", cp, sp));
							UNLOCK_CARDINFO(cp, c);
						} else {
							/*
							 * mdi_valid_mca() and mdi_get_mctbl()
							 * avoid RDLOCK_MCA if iocblk == 0xBF;
							 * OK to hold WRLOCK_MCA across putnext()
							 */
							cp->dlpi_iocblk = (mblk_t *)0xBF;
							UNLOCK_CARDINFO(cp, c);
							mdi_send_MACIOC_DELMCA(sp, dlpi_delmca_ack, cm->mca);
						}
						DLPI_PRINTF400(("dlpi_clear_sap_multicasts() cm%x cp->mcahashtbl %x\n", cm, cp->mcahashtbl[i]));
						if (cm == cp->mcahashtbl[i]) {
							cp->mcahashtbl[i] = cm->next_mca;
							kmem_free((void *)cm, sizeof(card_mca_t));
							cm = cp->mcahashtbl[i];
						} else {
							pcm->next_mca = cm->next_mca;
							kmem_free((void *)cm, sizeof(card_mca_t));
							cm = pcm;	/* this entry not checked yet */
						}
					}
					break;	/* any given sap can't use same mca >1 time */
				}
				sm = sm->next_sap;
			}
			if (! last_stream) {
				pcm = cm;
				cm = cm->next_mca;
			}
		}
	}
	if (cp->all_mca) {
		DLPI_PRINTF400(("dlpi_clear_sap_multicasts() allmca set\n"));
		sp->all_mca = 0;	/* always clear allmca on this sap */
		c = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk && cp->dlpi_iocblk != (mblk_t *)0xBF) {
			DLPI_PRINTF400(("dlpi_clear_sap_multicasts() allmca iocblk busy\n", cp, sp));
			UNLOCK_CARDINFO(cp, c);
		} else {
			/* delete ALLMCA if this is the last sap that set it */
			for (tsp = sap_table; tsp < &sap_table[cp->maxsaps]; tsp++) {
				if (tsp == sp)
					continue;
				if (tsp->all_mca)
					break;
			}
			if (tsp == &sap_table[cp->maxsaps]) {
				/*
				 * mdi_valid_mca() and mdi_get_mctbl()
				 * avoid RDLOCK_MCA if iocblk == 0xBF;
				 * OK to hold WRLOCK_MCA across putnext()
				 */
				cp->dlpi_iocblk = (mblk_t *)0xBF;
				cp->all_mca = 0;	/* no need to wait for ack */
				UNLOCK_CARDINFO(cp, c);
				mdi_send_MACIOC_DELALLMCA(sp, dlpi_delmca_ack);
			} else {
				UNLOCK_CARDINFO(cp, c);
			}
		}
	}
	UNLOCK_MCA(cp, s);
}


/*
 * Send a DL_OK_ACK or DL_ERROR_ACK based on the results of a MACIOC_ ioctl
 */
STATIC void
dlpi_send_dl_ok_or_error_ack(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t *cp = sp->card_info;
	struct iocblk	*ip;
	dl_set_phys_addr_req_t *pp;
	uint		 hsh;
	pl_t s;

	if (! mp) {
		goto mdi_ioctl_error;
	}
	ip = (struct iocblk *)mp->b_rptr;
	if (mp->b_datap->db_type == M_IOCNAK) {
		/* error - let the next ioctl run */
		cp->ioctl_sap = 0;
		cp->dlpi_iocblk = 0;
		dlpi_enable_WRqs(sp->card_info);
		dlpi_send_dl_error_ack(sp, sp->dlpi_prim, DL_SYSERR, ip->ioc_error);
		return;
	}
	switch (ip->ioc_cmd) {
	case MACIOC_SETADDR:	/* copy new addr into percard struct */
		if (! mp->b_cont) {
			goto mdi_ioctl_error;
		}
		s = LOCK_CARDINFO(cp);
		bcopy(mp->b_cont->b_rptr, cp->local_mac_addr, MAC_ADDR_SZ);
		cp->set_mac_addr_pending = 0;
		UNLOCK_CARDINFO(cp, s);
		break;
	case MACIOC_SETMCA:
		if (! mp->b_cont) {
			goto mdi_ioctl_error;
		}
		/* TODO - delete multicast addr from card on failures ??? */
		s = WRLOCK_MCA(cp);
		if (! dlpi_alloc_mca(cp, sp, mp->b_cont->b_rptr)) {
			UNLOCK_MCA(cp, s);
			goto mem_error;
		}
		UNLOCK_MCA(cp, s);
		break;
	case MACIOC_DELMCA:
	{
		card_mca_t *cm;
		card_mca_t *pcm;

		if (! mp->b_cont) {
			goto mdi_ioctl_error;
		}
		hsh = dlpi_hsh(mp->b_cont->b_rptr, MAC_ADDR_SZ);
		s = WRLOCK_MCA(cp);

		DLPI_PRINTF400(("dl_ok_or_error DELMCA cp %x hsh %x tbl %x\n",
			cp, hsh, cp->mcahashtbl[hsh]));

		for (cm = cp->mcahashtbl[hsh]; cm; pcm = cm, cm = cm->next_mca) {
			if (mdi_addrs_equal(cm->mca, mp->b_cont->b_rptr)) {
				DLPI_PRINTF400(("DELMCA cm %x match %b:%b:%b:%b:%b:%b\n", cm,
				cm->mca[0], cm->mca[1], cm->mca[2], cm->mca[3], cm->mca[4], cm->mca[5])); 
				if (dlpi_delete_mca_sap(&cm->sap_mca, sp)) {
					/* deleted last stream for this mca */ 
					if (cm == cp->mcahashtbl[hsh]) {
						cp->mcahashtbl[hsh] = cm->next_mca;
					} else {
						pcm->next_mca = cm->next_mca;
					}
					kmem_free((void *)cm, sizeof(card_mca_t));
				}
				break;
			}
		}
		UNLOCK_MCA(cp, s);
		break;
	}
	case MACIOC_SETALLMCA:
		DLPI_PRINTF400(("SETALLMCA cp %x sp %x\n", cp, sp));
		s = WRLOCK_MCA(cp);
		cp->all_mca = 1;
		sp->all_mca = 1;
		UNLOCK_MCA(cp, s);
		break;
	case MACIOC_DELALLMCA:
		DLPI_PRINTF400(("DELALLMCA cp %x sp %x\n", cp, sp));
		s = WRLOCK_MCA(cp);
		cp->all_mca = 0;
		sp->all_mca = 0;
		UNLOCK_MCA(cp, s);
		break;
	}

	dlpi_send_dl_ok_ack(sp, sp->dlpi_prim);
	/* ioctl is done; let the next ioctl run */
	cp->ioctl_sap = 0;
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(sp->card_info);
	freemsg(mp);
	return;

mdi_ioctl_error:
	dlpi_send_dl_error_ack(sp, sp->dlpi_prim, DL_SYSERR, ENOSR);
	/* error - let the next ioctl run */
	cp->ioctl_sap = 0;
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(sp->card_info);
	freemsg(mp);
	return;

mem_error:
	dlpi_send_dl_error_ack(sp, DL_ENABMULTI_REQ, DL_SYSERR, ENOMEM);
	/* memory allocation error; let the next ioctl run */
	cp->ioctl_sap = 0;
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(sp->card_info);
	freemsg(mp);
	return;
}

/*
 * dlpi_mca_count returns the number of multicast addresses set on this card
 *
 * Lock state:
 *	must be called with multicasting read lock held
 */
ulong
dlpi_mca_count(per_card_info_t *cp, per_sap_info_t *sp)
{
	register int i, cnt = 0;
	card_mca_t *cm;
	sap_mca_t *sm;

	for (i = 0; i < cp->nmcahash; i++) {
		for (cm = cp->mcahashtbl[i]; cm; cm = cm->next_mca) {
			if (sp && (sp->sap_state == SAP_BOUND)) {
				sm = &cm->sap_mca;
				while (sm) {
					if (sm->sp == sp) {
						cnt++;
						break;
					}
					sm = sm->next_sap;
				}
			} else {
				cnt++;
			}
		}
	}
	DLPI_PRINTF400(("dlpi_mca_count(cp%x sp%x) cnt %d\n", cp, sp, cnt));
	return(cnt);
}

/*
 * dlpi_get_mctable returns the number of bytes copied
 *
 * Lock state:
 *	Must be called with multicasting read lock
 */
void
dlpi_get_mctable(per_card_info_t *cp, per_sap_info_t *sp, unchar *dst, ulong cnt)
{
	register int i, copy;
	card_mca_t *cm;
	sap_mca_t *sm;

	DLPI_PRINTF400(("dlpi_get_mctable(cp%x sp%x dst%x cnt%x)\n",
		cp, sp, dst, cnt));

	for (i = 0; i < cp->nmcahash; i++) {
		for (cm = cp->mcahashtbl[i]; cm; cm = cm->next_mca) {
			copy = 0;
			if (sp && sp->sap_state == SAP_BOUND) {
				sm = &cm->sap_mca;
				while (sm) {
					DLPI_PRINTF400(("sm->sp %x sp %x\n", sm->sp, sp));
					if (sm->sp == sp) {
						copy = 1;
						break;
					}
					sm = sm->next_sap;
				}
			} else {
				copy = 1;
			}
			if (copy) {
				DLPI_PRINTF400(("copy cm %x addr %b:%b:%b:%b:%b:%b\n", cm,
				cm->mca[0], cm->mca[1], cm->mca[2], cm->mca[3], cm->mca[4], cm->mca[5])); 
				bcopy(cm->mca, dst, sizeof(macaddr_t));
				dst += sizeof(macaddr_t);
				if (--cnt == 0) {
					/* break out of both loops */
					i = cp->nmcahash;
					break;
				}
			}
		}
	}
}

/*
 * Send a DL_MCTABLE_ACK to the DLS user.
 */
STATIC void
dlpi_send_dl_mctable_ack(per_sap_info_t *sp)
{
	ulong			cnt;
	dl_mctable_ack_t	*ack;
	per_card_info_t		*cp = sp->card_info;
	card_mca_t		*cm;
	mblk_t			*mp;
	pl_t			s;

	/* block out add/delete MCAs */
	s = RDLOCK_MCA(cp);
	cnt = dlpi_mca_count(cp, sp);
	DLPI_PRINTF400(("dlpi_send_dl_mctable_ack(sp%x) cnt %d\n", sp, cnt));

	mp = allocb(sizeof(dl_mctable_ack_t)+(cnt*sizeof(macaddr_t)), BPRI_MED);
	if (!mp) {
		UNLOCK_MCA(cp, s);
		dlpi_send_dl_error_ack(sp,DL_MCTABLE_REQ, DL_SYSERR, ENOSR);
		return;
	}
	dlpi_get_mctable(cp, sp, mp->b_wptr + sizeof(dl_mctable_ack_t), cnt);

	UNLOCK_MCA(cp, s);
	mp->b_datap->db_type = M_PCPROTO;
	ack = (dl_mctable_ack_t *)mp->b_rptr;
	ack->dl_primitive = DL_MCTABLE_ACK;
	ack->dl_mctable_len = cnt * sizeof(macaddr_t);
	ack->dl_mctable_offset = sizeof(dl_mctable_ack_t);
	ack->dl_all_mca = cp->all_mca;
	ack->dl_mca_count = cnt;
	mp->b_wptr += sizeof(dl_mctable_ack_t) + ack->dl_mctable_len;
	putnext(sp->up_queue, mp);		/* Ignore flow control */
}

/*
 * DLPI Primitive Received at the DLPI from protocol stack
 */
STATIC int
dlpi_primitive(mblk_t *mp, per_card_info_t *cp, per_sap_info_t *sp)
{
	register union DL_primitives *prim = (union DL_primitives *)mp->b_rptr;
	register per_sap_info_t *tsp, *sap_table = cp->sap_table;
	int bound_sap_count;
	card_mca_t *cm;
	pl_t s;

	DLPI_PRINTF40(("DLPI: Received Prim(0x%x) from DLPI user\n", 
							prim->dl_primitive));
	/* if MDI driver not ready == 0 then mdi_shutdown_driver() called, 
	 * MAC_HWFAIL_IND received, or txmon fired.
	 */
	if (cp->mdi_driver_ready == 0) {
		switch (prim->dl_primitive) {
		case DL_UNITDATA_REQ:
		case DL_XID_REQ:
		case DL_TEST_REQ:
		case DL_XID_RES:
		case DL_TEST_RES:
			break;
		default:
			/* DL_NOTINIT: DLS provider requires 
			 * pre-initialization(mdi and net0 must be linked), 
			 * but the PPA is not initialized.
			 * See DLPI Rev 2.0.0 section 4.1.1 p. 28 for why 
			 * DL_NOTINIT or read comment about it in 
			 * dlpi_send_dl_bind_ack()
			 * This generally means dlpid isn't running
			 */
			dlpi_send_dl_error_ack(sp, prim->dl_primitive, DL_NOTINIT, 0);
		}
		freemsg(mp);
		return 1;
	}

	switch (prim->dl_primitive) {

	case DL_UNITDATA_REQ:
		/* dlpi_send_unitdata() checks issuspended */
		dlpi_send_unitdata(sp, DL_UNITDATA_REQ, mp,
						   prim->unitdata_req.dl_dest_addr_offset,
						   prim->unitdata_req.dl_dest_addr_length, 0);
		return 1;
	case DL_INFO_REQ:
		dlpi_send_dl_info_ack(sp);
		break;
	case DL_BIND_REQ:
		/* Look at the size of the DL_BIND_REQ to determine of the
		 * stack binding is a stack built with the LLI 3.1 dlpi.h or a
		 * stack built with the AT&T DLPI 2.0 dlpi.h.	
		 */
		if (mp->b_wptr - mp->b_rptr < sizeof(dl_bind_req_t)) {
			dlpi_send_dl_bind_ack(sp, prim->bind_req.dl_sap, LLI31_STACK, 0, 0);
		} else {
			if (prim->bind_req.dl_max_conind || prim->bind_req.dl_conn_mgmt) {
				dlpi_send_dl_error_ack(sp, DL_BIND_REQ, DL_BOUND, 0);
				break;
			}
			dlpi_send_dl_bind_ack(sp, prim->bind_req.dl_sap, 
								  DLPI20_STACK, 
								  prim->bind_req.dl_service_mode, 
								  prim->bind_req.dl_xidtest_flg);
		}
		break;
	case DL_SUBS_BIND_REQ:
		dlpi_send_dl_subs_bind_ack(sp, mp);
		break;
	case DL_UNBIND_REQ:
		sp->sap_state = SAP_ALLOC;
		/* free filters when unbinding if set */
		if (sp->filter[DL_FILTER_INCOMING] != NULL) {
			freemsg(sp->filter[DL_FILTER_INCOMING]); 
		}
		if (sp->filter[DL_FILTER_OUTGOING] != NULL) {
			freemsg(sp->filter[DL_FILTER_OUTGOING]); 
		}
		dlpi_send_dl_ok_ack(sp, DL_UNBIND_REQ);
		break;
	case DL_SUBS_UNBIND_REQ:
		dlpi_send_dl_subs_unbind_ack(sp, mp);
		break;
	case DL_GET_STATISTICS_REQ:
		s = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk) {
			UNLOCK_CARDINFO(cp, s);
			return 0;
		}
		cp->dlpi_iocblk = mp;
		UNLOCK_CARDINFO(cp, s);
		mdi_send_MACIOC_GETSTAT(sp, dlpi_send_dl_get_statistics_ack);
		break;
	case DL_CLR_STATISTICS_REQ:
		s = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk) {
			UNLOCK_CARDINFO(cp, s);
			return 0;
		}
		cp->dlpi_iocblk = mp;
		UNLOCK_CARDINFO(cp, s);
		mdi_send_MACIOC_CLRSTAT(sp, dlpi_send_dl_clr_statistics_ack);
		break;
	case DL_SAP_STATISTICS_REQ:
		dlpi_send_dl_sap_statistics_ack(sp);
		break;
	case DL_PHYS_ADDR_REQ:
		dlpi_send_dl_phys_addr_ack(sp, prim->physaddr_req.dl_addr_type);
		break;
	case DL_SET_PHYS_ADDR_REQ:
		s = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk) {
			UNLOCK_CARDINFO(cp, s);
			return 0;
		}
		if (cp->set_mac_addr_pending) {
			UNLOCK_CARDINFO(cp, s);
			dlpi_send_dl_error_ack(sp, DL_SET_PHYS_ADDR_REQ, DL_BUSY, 0);
			break;
		}
		bound_sap_count = 0;
		for (tsp = sap_table; tsp < &sap_table[cp->maxsaps]; tsp++) {
				if (tsp == sp) continue;
				if (tsp->sap_state == SAP_BOUND)
					++bound_sap_count;
		}
		if (bound_sap_count) { 
			UNLOCK_CARDINFO(cp, s);
			dlpi_send_dl_error_ack(sp, DL_SET_PHYS_ADDR_REQ, DL_BUSY, 0);
			break;
		}
		cp->dlpi_iocblk = mp;
		cp->set_mac_addr_pending = 1;
		UNLOCK_CARDINFO(cp, s);
		sp->dlpi_prim = prim->dl_primitive;
		mdi_send_MACIOC_SETADDR(sp, dlpi_send_dl_ok_or_error_ack,
			mp->b_rptr + prim->set_physaddr_req.dl_addr_offset);
		break;
	case DL_ENABMULTI_REQ:
		s = WRLOCK_MCA(cp);
		if (dlpi_mcahashtbl_entry(cp, mp->b_rptr + prim->enabmulti_req.dl_addr_offset)) {
			if (! dlpi_alloc_mca(cp, sp, mp->b_rptr + prim->enabmulti_req.dl_addr_offset)) {
				UNLOCK_MCA(cp, s);
				dlpi_send_dl_error_ack(sp, DL_ENABMULTI_REQ, DL_SYSERR, ENOMEM);
				break;
			}
			UNLOCK_MCA(cp, s);
			dlpi_send_dl_ok_ack(sp, DL_ENABMULTI_REQ);
			break;
		}
		UNLOCK_MCA(cp, s);
		s = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk) {
			UNLOCK_CARDINFO(cp, s);
			return 0;
		}
		cp->dlpi_iocblk = mp;
		UNLOCK_CARDINFO(cp, s);
		sp->dlpi_prim = prim->dl_primitive;
		mdi_send_MACIOC_SETMCA(sp, dlpi_send_dl_ok_or_error_ack,
							   mp->b_rptr + prim->enabmulti_req.dl_addr_offset);
		break;
	case DL_DISABMULTI_REQ:
		s = WRLOCK_MCA(cp);
		if (! (cm = dlpi_mcahashtbl_entry(cp, mp->b_rptr + prim->disabmulti_req.dl_addr_offset))) {
			UNLOCK_MCA(cp, s);
			dlpi_send_dl_error_ack(sp, DL_DISABMULTI_REQ, DL_NOTENAB, 0);
			break;
		}
		if (cm->sap_mca.next_sap) {
			/* other streams using mca - don't send MACIOC_DELMCA to card */
			dlpi_delete_mca_sap(&cm->sap_mca, sp);
			UNLOCK_MCA(cp, s);
			dlpi_send_dl_ok_ack(sp, DL_DISABMULTI_REQ);
			break;
		}
		UNLOCK_MCA(cp, s);
		s = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk) {
			UNLOCK_CARDINFO(cp, s);
			return 0;
		}
		cp->dlpi_iocblk = mp;
		UNLOCK_CARDINFO(cp, s);
		sp->dlpi_prim = prim->dl_primitive;
		mdi_send_MACIOC_DELMCA(sp, dlpi_send_dl_ok_or_error_ack,
							   mp->b_rptr 
							   + prim->disabmulti_req.dl_addr_offset);
		break;
	case DL_MCTABLE_REQ:
		dlpi_send_dl_mctable_ack(sp);
		break;
	case DL_ENABALLMULTI_REQ:
		s = WRLOCK_MCA(cp);
		if (cp->all_mca) {
			sp->all_mca = 1;
			UNLOCK_MCA(cp, s);
			dlpi_send_dl_ok_ack(sp, DL_ENABALLMULTI_REQ);
			break;
		}
		UNLOCK_MCA(cp, s);
		s = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk) {
			UNLOCK_CARDINFO(cp, s);
			return 0;
		}
		cp->dlpi_iocblk = mp;
		UNLOCK_CARDINFO(cp, s);
		sp->dlpi_prim = prim->dl_primitive;
		mdi_send_MACIOC_SETALLMCA(sp, dlpi_send_dl_ok_or_error_ack);
		break;
	case DL_DISABALLMULTI_REQ:
		s = WRLOCK_MCA(cp);
		if (! cp->all_mca || !sp->all_mca) {
			sp->all_mca = 0;
			UNLOCK_MCA(cp, s);
			dlpi_send_dl_error_ack(sp, DL_DISABALLMULTI_REQ, DL_OUTSTATE, 0);
			break;
		}
		for (tsp = sap_table; tsp < &sap_table[cp->maxsaps]; tsp++) {
			if (tsp == sp)
				continue;
			if (tsp->all_mca)
				break;
		}
		if (tsp < &sap_table[cp->maxsaps]) {
			sp->all_mca = 0;
			UNLOCK_MCA(cp, s);
			break;
		}
		/* only get here on last sap delete all mca request */
		UNLOCK_MCA(cp, s);
		s = LOCK_CARDINFO(cp);
		if (cp->dlpi_iocblk) {
			UNLOCK_CARDINFO(cp, s);
			return 0;
		}
		cp->dlpi_iocblk = mp;
		UNLOCK_CARDINFO(cp, s);
		sp->dlpi_prim = prim->dl_primitive;
		mdi_send_MACIOC_DELALLMCA(sp, dlpi_send_dl_ok_or_error_ack);
		break;
	case DL_TEST_REQ:
		/* winds up at dlpi_send_unitdata which checks issuspended */
		dlpi_send_llc_message(sp, DL_TEST_REQ, mp,
					prim->test_req.dl_dest_addr_offset,
					prim->test_req.dl_dest_addr_length,
					prim->test_req.dl_flag);
		return 1;
	case DL_XID_REQ:
		/* winds up at dlpi_send_unitdata which checks issuspended */
		dlpi_send_llc_message(sp, DL_XID_REQ, mp,
					prim->xid_req.dl_dest_addr_offset,
					prim->xid_req.dl_dest_addr_length,
					prim->xid_req.dl_flag);
		return 1;
	case DL_TEST_RES:
		/* winds up at dlpi_send_unitdata which checks issuspended */
		dlpi_send_llc_message(sp, DL_TEST_RES, mp,
					prim->test_res.dl_dest_addr_offset,
					prim->test_res.dl_dest_addr_length,
					prim->test_res.dl_flag);
		return 1;
	case DL_XID_RES:
		/* winds up at dlpi_send_unitdata which checks issuspended */
		dlpi_send_llc_message(sp, DL_XID_RES, mp,
					prim->xid_res.dl_dest_addr_offset,
					prim->xid_res.dl_dest_addr_length,
					prim->xid_res.dl_flag);
		return 1;
	case DL_SET_SRMODE_REQ:
	{
		dl_set_srmode_req_t *dp = (dl_set_srmode_req_t *)prim;

		if (sp->sap_state == SAP_BOUND)
		{
			dlpi_send_dl_error_ack(sp, DL_SET_SRMODE_REQ, DL_OUTSTATE, 0);
			break;
		}
		if (!cp->media_specific ||
			!(cp->media_specific->media_flags & MEDIA_SOURCE_ROUTING))
		{
			dlpi_send_dl_error_ack(sp, DL_SET_SRMODE_REQ,
											   DL_NOTSUPPORTED, 0);
			break;
		}

		switch (dp->dl_srmode)
		{
			case SR_NON:
			case SR_STACK:
			case SR_AUTO:
				sp->srmode = dp->dl_srmode;
				dlpi_send_dl_ok_ack(sp, DL_SET_SRMODE_REQ);
				break;
			default:
				dlpi_send_dl_error_ack(sp, DL_SET_SRMODE_REQ,
								DL_BADPRIM, 0);
		}
		break;
	}
	case DL_SRTABLE_REQ:
	case DL_SR_REQ:
	case DL_SET_SR_REQ:
	case DL_CLR_SR_REQ:
	case DL_SET_SRPARMS_REQ:
		if (dlpiSR_primitives(cp, WR(sp->up_queue), mp))
			return 1;
#ifdef dlpi_frame_test
		case DL_DLPI_BILOOPMODE: {
			dl_set_biloopmode_req_t *dp = (dl_set_biloopmode_req_t *)prim;
			if (dp->dl_biloopmode) 
				cp->dlpi_biloopmode = sp;
			else
				cp->dlpi_biloopmode = NULL;
			dlpi_send_dl_ok_ack(sp, DL_DLPI_BILOOPMODE);
			break;
		}
#endif

	default:
                if (cp->media_specific->media_wr_prim)
			/* DDI8 CFG_SUSPEND note: we don't know if the
			 * command will involve the hardware or not, so
			 * we don't check issuspended to call freemsg here
			 * right now ISDN is the only thing that uses this
			 */
			switch (cp->media_specific->media_wr_prim(mp, cp, sp))
			{
				case MPRIMRET_PUTBQ:
				return 0;

		       		case MPRIMRET_DONE:
		       		return 1;

		       		case MPRIMRET_NOT_MINE:
		       		default:
		       		break;
			}

		dlpi_send_dl_error_ack(sp,prim->dl_primitive,DL_BADPRIM, 0);
	}
	freemsg(mp);
	return (1);
}

/*
 * Process a message received at the DLPI from a protocol stack
 * Return TRUE if message was handled.  A return of zero will put
 * the message back on the queue.
 */

STATIC int
dlpi_message(queue_t *q, register mblk_t *mp)
{
	register per_sap_info_t	*sp  = (per_sap_info_t *)q->q_ptr;
	register per_card_info_t	*cp = sp->card_info;


	/* if dlpid hasn't assembled things yet, then cp->media_svcs_queue and
	 * cp->media_specific aren't set.  An incoming M_DATA message at this
	 * time (i.e. date > /dev/net0) shouldn't panic the system.
	 * cp->media_specific isn't set until we get a MAC_INFO_ACK back from
	 * system initialization at I_LINK time.   We'll key off of 
	 * mdi_driver_ready which isn't set until the very end of initialization
	 * We want to still allow M_FLUSH and other message types to still
	 * come through since they don't depend on media_specific features
	 * or they check mdi_driver_ready on their own.
	 */
	if ((cp->mdi_driver_ready == 0) && (mp->b_datap->db_type == M_DATA))
	{
		DLPI_PRINTF100(("DLPI: throwing mblk away; no media yet\n"));
		freemsg(mp);
		return(1);
	}

	/*
	 * Neither cp->media_specific nor cp->media_specific->media_svc_rcv
	 * will be NULL unless cp->media_svcs_queue == NULL.
	 * For speed sake, do not check them.
	 * cp->media_specific will be set if mdi_driver_ready is 1
	 */
	if ((cp->mdi_driver_ready == 1) && (cp->media_svcs_queue == q))
		return cp->media_specific->media_svc_rcv(q, mp);

	switch (mp->b_datap->db_type)
	{
	case M_DATA:

		/* since we're sending M_DATA to cp->down_queue we
		 * check issuspended before calling media_wr_data
		 * dlpi_primitive checks issuspended too so this is
		 * also for symmetry.
		 */
		if (cp->issuspended) {
			DLPI_PRINTF1000(("DLPI: dropping packet in dlpi_message\n"));
			ATOMIC_INT_INCR(&cp->suspenddrops);
			freemsg(mp);
			return(1);
		}

                if (cp->media_specific->media_wr_data) {
			switch (cp->media_specific->media_wr_data(mp, cp, sp))
			{
				case MPRIMRET_PUTBQ:
				return 0;

		       		case MPRIMRET_DONE:
		       		return 1;

		       		case MPRIMRET_NOT_MINE:
		       		default:
		       		break;
			}
		}

#ifdef dlpi_frame_test
		DLPI_PRINTF08(("dlpi_message: M_DATA\n"));
		if (!cp->dlpi_biloopmode)
			break;

		/*
		 * Change to M_PROTO to distinguish from MDI driver messages
		 * and loop back to DLPI downqueue read side, but allow other
		 * messages through (needed to turn loopback off again)
		 */

		mp->b_datap->db_type = M_PROTO;
		DLPI_PRINTF08(("putq(RD(cp->down_queue), mp)\n"));
		putq(RD(cp->down_queue), mp);
		return(1);
#endif
		/* FALLTHROUGH */

	case M_PROTO:
	case M_PCPROTO:
		return (dlpi_primitive(mp, cp, sp));

	case M_IOCTL:
		return (dlpi_ioctl(q, mp, cp));

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);

		if (*mp->b_rptr & FLUSHR)
		{
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q,mp);
		}
		else
			freemsg(mp);

		return(1);
	}

	mp->b_datap->db_type = M_ERROR;
	mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
	*mp->b_wptr++ = EINVAL;
	qreply(q, mp);
	return(1);
}

/*
 * Message Received from Up Queue (Protocol Stack)
 */

void
dlpiuwput(queue_t *q, mblk_t *mp)
{
	DLPI_PRINTF80(("dlpiuwput(RD%x WR%x) size(RD%d WR%d) ptr(RD%x,WR%x)\n",
						RD(q), q,
						qsize(RD(q)), qsize(q),
						RD(q)->q_ptr, q->q_ptr));

	if (q->q_first || !dlpi_message(q, mp)) {
		/* MDI Driver write queue is full or busy processing ioctl */
		DLPI_PRINTF80(("dlpiuwput: dlpi_message FAILS, queueing\n"));
		putq(q, mp);
	}
}

/*
 * This queue is used to apply flow-control back-pressure to the protocol stacks
 * when the MDI driver's Write Queue is full.  It has a really small high-water
 * mark, so that whenever a single message is placed on this queue, it causes
 * canput() in the module above to fail.
 */

void
dlpiuwsrv(queue_t *q)
{
	mblk_t *mp;
	per_sap_info_t *sp  = (per_sap_info_t *)q->q_ptr;

	while (mp = getq(q))
		if (!dlpi_message(q, mp))
		{
			putbq(q,mp);
			DLPI_PRINTF80(("dlpiuwsrv: dlpi_message FAILS, queueing\n"));
			return;
		}
}

/*
 * This queue is used to store message bound for the protocol stack
 * when the protocol stack's Read Queue is full.
 */
void
dlpiursrv(queue_t *q)
{
	mblk_t *mp;
	per_sap_info_t *sp  = (per_sap_info_t *)q->q_ptr;


	while ( mp=getq(q) ) {
		if ( !canputnext(q) ) {
			putbq(q,mp);
			DLPI_PRINTF80(("dlpiursrv: canput FAILS, queueing\n"));
			return;
		}
		putnext(q,mp);
	}
	qenable(RD(sp->card_info->down_queue));
}
