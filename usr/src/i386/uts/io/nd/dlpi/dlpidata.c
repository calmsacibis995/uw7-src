#ident "@(#)dlpidata.c	29.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>
#include <io/nd/dlpi/bpf.h>

#else

#include "include.h"
#include "bpf.h"

#endif

extern unsigned int dlpi_defaultfilter(struct bpf_insn *, mblk_t *); 

unsigned int (*dlpi_filterfunc)(struct bpf_insn *, mblk_t *)=dlpi_defaultfilter;


/* the default filter to use - accept all packets
 */
unsigned int
dlpi_defaultfilter(struct bpf_insn *insn, mblk_t *fmp)
{
   /* no filter means accept all packets */
   return((u_int)-1);
}

/* set the filter to the function specified as an argument
 * generally called by the filter module's load routine.
 */
void
dlpi_setfilter(func)
unsigned int (*func)(struct bpf_insn *, mblk_t *);
{
	dlpi_filterfunc = func;
	return;
}

/* set the default filter.  should be called by a filtering module's unload
 * routine so that function pointer dereferences to dlpi_filterfunc don't
 * go into the black hole later on.
 */
void
dlpi_setdefaultfilter(void)
{
	dlpi_setfilter(dlpi_defaultfilter);
	return;
}

/*
 * An address at the DLPI layer has the following format:
 *	 -----------------------------------
 *	| MAC-ADDR | LLC-SAP | SOURCE-ROUTE |
 *	 -----------------------------------
 *	   6 bytes   1 byte    0-18 bytes
 *
 *	LLC-SAP field exists only if LLC-1 processing is turned on
 *	SOURCE-ROUTE field only if STACK routing
 */
#define MAX_ADDR_SZ	(MAC_ADDR_SZ+1+18)

/*
 * Modification History:
 *   dieters@sco.com            Jan 11, 1994 
 *   - Lock around statistics update
 */

/*
 * Using the 'from' argument, create the DLPI destination addresses.
 * The length of the address is returned.
 */
STATIC int
dlpi_make_dest_addr(per_sap_info_t *sp, struct per_frame_info *from,
			unchar *to)
{
	int addrlen = MAC_ADDR_SZ;

	bcopy(from->frame_dest, to, MAC_ADDR_SZ);

	if (sp->llcmode == LLC_1) {
		*(to+MAC_ADDR_SZ) = from->frame_dsap;
		addrlen += 1;
	}
	return (addrlen);
}

/*
 * Using the 'from' argument, create the DLPI source addresses.
 * The length of the address is returned.
 */
STATIC int
dlpi_make_src_addr(per_sap_info_t *sp, struct per_frame_info *from,
			unchar *to)
{
	int addrlen = MAC_ADDR_SZ;

	bcopy(	from->frame_src, to,	MAC_ADDR_SZ);
	if (sp->llcmode == LLC_1) {
		*(to+MAC_ADDR_SZ) = from->frame_ssap;
		addrlen += 1;
	}
	if (sp->srmode == SR_STACK && from->route_len) {
		bcopy(from->route_info, to+addrlen, from->route_len);
		addrlen += from->route_len;
	}
	return (addrlen);
}

/*
 * Create a DL_UNITDATA_IND and send it to the DLPI user
 */
void
dlpi_send_dl_unitdata_ind(per_sap_info_t *sp, struct per_frame_info *f)
{
	per_card_info_t		*cp = sp->card_info;
	dl_unitdata_ind_t	*ip;
	mblk_t			*mp = f->unitdata;
	mblk_t			*newmp;

	DLPI_PRINTF08(("DLPI: Sending DL_UNITDATA_IND to DLPI user\n"));
#define UDMSGSZ	(sizeof(dl_unitdata_ind_t) + 2 * MAX_ADDR_SZ)
	if (!(newmp = allocb(UDMSGSZ, BPRI_MED))) {
		freemsg(mp);
		f->unitdata = (mblk_t *)0;
		ATOMIC_INT_INCR(sp->sap_rx.error);
		return;
	}

	switch (sp->srmode) {
	case SR_AUTO:
		dlpiSR_auto_rx(cp, f);
		/* Fall through */
	case SR_NON:
		f->route_present = 0;
		break;
	}
	switch (f->dest_type) {
	case FR_UNICAST:
		ATOMIC_INT_INCR(sp->sap_rx.ucast);
		break;
	case FR_MULTICAST:
		ATOMIC_INT_INCR(sp->sap_rx.mcast);
		break;
	case FR_BROADCAST:
		ATOMIC_INT_INCR(sp->sap_rx.bcast);
		break;
	}
	ATOMIC_INT_ADD(sp->sap_rx.octets, f->frame_len);
	mp->b_rptr = f->frame_data;	/* Strip MAC header + R.I. field */

	ip = (dl_unitdata_ind_t *)newmp->b_rptr;
	newmp->b_wptr += sizeof(dl_unitdata_ind_t);
	newmp->b_datap->db_type = M_PROTO;
	linkb(newmp, mp);
	
	ip->dl_primitive = DL_UNITDATA_IND;
	ip->dl_dest_addr_offset = sizeof(dl_unitdata_ind_t);
	ip->dl_dest_addr_length = dlpi_make_dest_addr(sp, f, newmp->b_rptr+
						ip->dl_dest_addr_offset);
	ip->dl_src_addr_offset= ip->dl_dest_addr_offset+ip->dl_dest_addr_length;
	ip->dl_src_addr_length = dlpi_make_src_addr(sp, f, newmp->b_rptr+
						ip->dl_src_addr_offset);
	ip->dl_group_address = (f->dest_type == FR_UNICAST) ? 0 : 1;
	newmp->b_wptr += ip->dl_src_addr_length+ip->dl_dest_addr_length;
	/*
	 * LLI 3.1 stacks need the source and destination addresses
	 * swapped since they are compiled with a header file which has the
	 * src_addr_offset & dest_addr_offset fields the wrong way around.
	 * DLPI 2.0 have their source and destination addresses the right
	 * way around.
	 */
	if (sp->dlpimode == LLI31_STACK) {
		register int x;

		x = ip->dl_dest_addr_offset;
		ip->dl_dest_addr_offset = ip->dl_src_addr_offset;
		ip->dl_src_addr_offset = x;

		x = ip->dl_dest_addr_length;
		ip->dl_dest_addr_length = ip->dl_src_addr_length;
		ip->dl_src_addr_length = x;
	}
	if (sp->sap_state != SAP_BOUND) {
		/* 
		* DWS: this won't happen very often, since we already checked
		* before calling - catches case in which this packet may no
		* longer be validly sent to the dlpi user, because it is
		* shutting down
		*/
		cmn_err(CE_WARN, "sap is no longer SAP_BOUND - discarding message");
		freemsg(newmp);
	}
	else {
		if (sp->filter[DL_FILTER_INCOMING] != NULL) {
			mblk_t *fmp = f->unitdata;
			struct bpf_insn *insns;
			int slen ;

			fmp->b_rptr = f->frame_data;
			insns = (struct bpf_insn *)sp->filter[DL_FILTER_INCOMING]->b_rptr;
			slen = (*dlpi_filterfunc)(insns, fmp);
			if (!slen) {
				freemsg(newmp);
				goto done;
			}
		}
	}
	dlpi_putnext(sp->up_queue, newmp);
done:
	f->unitdata = (mblk_t *)0;
}

/*
 * Create a DL_XID_IND message and send it to the DLPI user
 */
void
dlpi_send_dl_xid_ind(per_sap_info_t *sp, struct per_frame_info *f, int pf)
{
	dl_xid_ind_t		*ip;
	mblk_t			*mp = f->unitdata;

	mblk_t			*newmp;

	DLPI_PRINTF08(("DLPI: Sending DL_XID_IND to DLPI user\n"));
#define XDMSGSZ	(sizeof(dl_xid_ind_t) + 2 * MAX_ADDR_SZ)
	if (!(newmp = allocb(XDMSGSZ, BPRI_MED))) {
		freemsg(mp);
		f->unitdata = (mblk_t *)0;
		ATOMIC_INT_INCR(sp->sap_rx.error);
		return;
	}

	ip = (dl_xid_ind_t *)newmp->b_rptr;
	newmp->b_wptr += sizeof(dl_xid_ind_t);
	newmp->b_datap->db_type = M_PROTO;
	
	ip->dl_primitive = DL_XID_IND;
	ip->dl_dest_addr_offset = sizeof(dl_xid_ind_t);
	ip->dl_dest_addr_length = dlpi_make_dest_addr(sp,f, newmp->b_rptr+
						ip->dl_dest_addr_offset);
	ip->dl_src_addr_offset= ip->dl_dest_addr_offset+ip->dl_dest_addr_length;
	ip->dl_src_addr_length = dlpi_make_src_addr(sp, f, newmp->b_rptr+
						ip->dl_src_addr_offset);
	newmp->b_wptr += ip->dl_src_addr_length+ip->dl_dest_addr_length;

	ip->dl_flag = pf ? DL_POLL_FINAL : 0;

	linkb(newmp, mp);
	dlpi_putnext(sp->up_queue, newmp);
	f->unitdata = (mblk_t *)0;
}


/*
 * Create a DL_TEST_IND message and send it to the DLPI user
 */
void
dlpi_send_dl_test_ind(per_sap_info_t *sp, struct per_frame_info *f, int pf)
{
	dl_test_ind_t		*ip;
	mblk_t			*mp = f->unitdata;

	mblk_t			*newmp;

	DLPI_PRINTF08(("DLPI: Sending DL_TEST_IND to DLPI user\n"));
#define TDMSGSZ	(sizeof(dl_test_ind_t) + 2 * MAX_ADDR_SZ)
	if (!(newmp = allocb(TDMSGSZ, BPRI_MED))) {
		freemsg(mp);
		f->unitdata = (mblk_t *)0;
		ATOMIC_INT_INCR(sp->sap_rx.error);
		return;
	}

	ip = (dl_test_ind_t *)newmp->b_rptr;
	newmp->b_wptr += sizeof(dl_test_ind_t);
	newmp->b_datap->db_type = M_PROTO;
	
	ip->dl_primitive = DL_TEST_IND;
	ip->dl_dest_addr_offset = sizeof(dl_test_ind_t);
	ip->dl_dest_addr_length = dlpi_make_dest_addr(sp,f, newmp->b_rptr+
						ip->dl_dest_addr_offset);
	ip->dl_src_addr_offset= ip->dl_dest_addr_offset+ip->dl_dest_addr_length;
	ip->dl_src_addr_length = dlpi_make_src_addr(sp, f, newmp->b_rptr+
						ip->dl_src_addr_offset);
	newmp->b_wptr += ip->dl_src_addr_length+ip->dl_dest_addr_length;

	ip->dl_flag = pf ? DL_POLL_FINAL : 0;

	linkb(newmp, mp);
	dlpi_putnext(sp->up_queue, newmp);
	f->unitdata = (mblk_t *)0;
}

/*
 * Create a DL_XID_CON message and send it to the DLPI user
 */
void
dlpi_send_dl_xid_con(per_sap_info_t *sp, struct per_frame_info *f, int pf)
{
	dl_xid_con_t		*ip;
	mblk_t			*mp = f->unitdata;

	mblk_t			*newmp;

	DLPI_PRINTF08(("DLPI: Sending DL_XID_CON to DLPI user\n"));
#define XCMSGSZ	(sizeof(dl_xid_con_t) + 2 * MAX_ADDR_SZ)
	if (!(newmp = allocb(XDMSGSZ, BPRI_MED))) {
		freemsg(mp);
		f->unitdata = (mblk_t *)0;
		ATOMIC_INT_INCR(sp->sap_rx.error);
		return;
	}

	ip = (dl_xid_con_t *)newmp->b_rptr;
	newmp->b_wptr += sizeof(dl_xid_con_t);
	newmp->b_datap->db_type = M_PROTO;
	
	ip->dl_primitive = DL_XID_CON;
	ip->dl_dest_addr_offset = sizeof(dl_xid_con_t);
	ip->dl_dest_addr_length = dlpi_make_dest_addr(sp,f, newmp->b_rptr+
						ip->dl_dest_addr_offset);
	ip->dl_src_addr_offset= ip->dl_dest_addr_offset+ip->dl_dest_addr_length;
	ip->dl_src_addr_length = dlpi_make_src_addr(sp, f, newmp->b_rptr+
						ip->dl_src_addr_offset);
	newmp->b_wptr += ip->dl_src_addr_length+ip->dl_dest_addr_length;

	ip->dl_flag = pf ? DL_POLL_FINAL : 0;

	linkb(newmp, mp);
	dlpi_putnext(sp->up_queue, newmp);
	f->unitdata = (mblk_t *)0;
}

/*
 * Create a DL_TEST_CON message and send it to the DLPI user
 */
void
dlpi_send_dl_test_con(per_sap_info_t *sp, struct per_frame_info *f, int pf)
{
	dl_test_con_t		*ip;
	mblk_t			*mp = f->unitdata;

	mblk_t			*newmp;

	DLPI_PRINTF08(("DLPI: Sending DL_TEST_CON to DLPI user\n"));
#define TCMSGSZ	(sizeof(dl_test_con_t) + 2 * MAX_ADDR_SZ)
	if (!(newmp = allocb(TCMSGSZ, BPRI_MED))) {
		freemsg(mp);
		f->unitdata = (mblk_t *)0;
		ATOMIC_INT_INCR(sp->sap_rx.error);
		return;
	}

	ip = (dl_test_con_t *)newmp->b_rptr;
	newmp->b_wptr += sizeof(dl_test_con_t);
	newmp->b_datap->db_type = M_PROTO;
	
	ip->dl_primitive = DL_TEST_CON;
	ip->dl_dest_addr_offset = sizeof(dl_test_con_t);
	ip->dl_dest_addr_length = dlpi_make_dest_addr(sp,f, newmp->b_rptr+
						ip->dl_dest_addr_offset);
	ip->dl_src_addr_offset= ip->dl_dest_addr_offset+ip->dl_dest_addr_length;
	ip->dl_src_addr_length = dlpi_make_src_addr(sp, f, newmp->b_rptr+
						ip->dl_src_addr_offset);
	newmp->b_wptr += ip->dl_src_addr_length+ip->dl_dest_addr_length;

	ip->dl_flag = pf ? DL_POLL_FINAL : 0;

	linkb(newmp, mp);
	dlpi_putnext(sp->up_queue, newmp);
	f->unitdata = (mblk_t *)0;
}

STATIC void
dlpi_send_dl_uderror_ind(per_sap_info_t *sp, unchar *addr, int addrlen,
				int err, int uerr)
{
	dl_uderror_ind_t *ep;
	mblk_t *mp;

	DLPI_PRINTF08(("DLPI: Sending DL_UDERROR_IND(0x%x,%d) to DLPI user\n",
								err, uerr));
	mp = allocb(sizeof (dl_uderror_ind_t) + addrlen, BPRI_HI);
	if (!mp) {
		cmn_err(CE_WARN, "dlpi_send_dl_uderror_ind - STREAMs memory shortage");
		return;
	}

	ep = (dl_uderror_ind_t *)mp->b_rptr;
	mp->b_wptr += sizeof(dl_uderror_ind_t) + addrlen;

	mp->b_datap->db_type = M_PCPROTO;
	ep->dl_primitive = DL_UDERROR_IND;
	ep->dl_dest_addr_length = addrlen;
	ep->dl_dest_addr_offset = sizeof(dl_uderror_ind_t);
	ep->dl_errno = err;
	ep->dl_unix_errno = uerr;
	bcopy	(addr,	mp->b_rptr + sizeof(dl_uderror_ind_t), addrlen);

	putnext(sp->up_queue,mp);		/* Ignore flow control */
}

/*
 * Send a data frame to the MDI driver below the module.
 * This function coordinates the creation of all the necessary headers
 * (MAC header, Source routing information, LLC-UI frame, and SNAP headers)
 */
void
dlpi_send_unitdata(register per_sap_info_t *sp, int prim, mblk_t *mp,
		 int dst_off, int dst_len, int poll_flag)
{
	register per_card_info_t *cp = sp->card_info;
	register mblk_t *hdrmp;
	int len;
	mblk_t *datamp;
	unchar *dest;
	int dst_addr_sz = (sp->llcmode == LLC_1? MAC_ADDR_SZ+1 : MAC_ADDR_SZ);

	DLPI_PRINTF08(("DLPI: dlpi_send_unitdata(sp 0x%x, prim %d, mp 0x%x, dst_off %d, dst_len %d poll_flag %d)\n", sp, prim, mp, dst_off, dst_len, poll_flag));

	/* silently drop DL_UNITDATA_REQ if card is suspended as we know card
	 * can't send them out on the wire.  Likewise, keeping mblks in a
	 * STREAMS queue can quick clog the plumbing beyond repair 
	 * (no Roto-Rooter or lye will help out here :-)).
	 */
	if (cp->issuspended) {
		DLPI_PRINTF1000(("DLPI: dropping packet in dlpi_send_unitdata\n"));
		ATOMIC_INT_INCR(&cp->suspenddrops);
		freemsg(mp);
		return;  /* no point in checking txmon */
	}

	if ((unsigned)dst_off > (unsigned)(mp->b_wptr - mp->b_rptr)) {
		cmn_err(CE_WARN, "dlpi_send_unitdata: bad dst_off 0x%x\n", dst_off);
		goto fail;
	}

	dest = mp->b_rptr + dst_off;

	if ((sp->sap_state != SAP_BOUND) || !cp->media_specific)
	{
		dlpi_send_dl_uderror_ind(sp, dest, dst_len, DL_OUTSTATE, 0);
		goto fail;
	}

	if (dst_len < dst_addr_sz)
	{
		dlpi_send_dl_uderror_ind(sp, dest, dst_len, DL_BADADDR, 0);
		goto fail;
	}

	/* Allocate a buffer big-enough here to take the biggest of the
	 * headers that the DLPI module might create (which is):
	 *	-------------------------------------------
	 *	| MAC-header | Route-Info | LLC UI | SNAP |
	 *	-------------------------------------------
	 *	   (hdr_sz)   MAX_ROUTE_SZ    3        5
	 */
	len = cp->media_specific->hdr_sz;
	if (!(hdrmp = allocb ( len+MAX_ROUTE_SZ+3+5, BPRI_LO )) ) {
		goto fail;
	}
	hdrmp->b_wptr += len;		/* Reserve space for MAC hdr */
	datamp = unlinkb(mp); 		/* Strip the DL_????_REQ header */
	if (datamp != NULL)
		linkb(hdrmp, datamp);	/* Add new header before data */
	
	switch (sp->srmode) {
	case SR_NON:
		len = 0;
		break;
	case SR_STACK:
		len = dst_len - dst_addr_sz;
		if (len)
			bcopy(dest+dst_addr_sz, hdrmp->b_wptr, len);
		break;
	case SR_AUTO:
		len = dlpiSR_make_header(cp, dest, hdrmp->b_wptr);
		break;
	}
	hdrmp->b_wptr += len;

	if (sp->llcmode != LLC_OFF) {
		unchar dsap = dest[MAC_ADDR_SZ];

		switch (prim) {
		case DL_UNITDATA_REQ:
			hdrmp->b_wptr += dlpi_make_llc_ui(sp, dsap,
					hdrmp->b_wptr);
			break;
		case DL_TEST_REQ:
			hdrmp->b_wptr += dlpi_make_llc_test(sp, dsap, 1,
					poll_flag, hdrmp->b_wptr);
			break;
		case DL_TEST_RES:
			hdrmp->b_wptr += dlpi_make_llc_test(sp, dsap, 0,
					poll_flag, hdrmp->b_wptr);
			break;
		case DL_XID_REQ:
			hdrmp->b_wptr += dlpi_make_llc_xid(sp, dsap, 1,
					poll_flag, hdrmp->b_wptr);
			break;
		case DL_XID_RES:
			hdrmp->b_wptr += dlpi_make_llc_xid(sp, dsap, 0,
					poll_flag, hdrmp->b_wptr);
			break;
		}
	}

	if (cp->media_specific->make_hdr(sp, hdrmp, dest, cp->local_mac_addr, len))
	{
		freeb(mp);
		freemsg(hdrmp);
		return;
	}
	/* don't free mp before calling make_hdr above - dest uses it */
	freeb(mp);

#ifdef dlpi_frame_test
	if (cp->dlpi_biloopmode) {
		dlpi_putnext(cp->dlpi_biloopmode->up_queue, hdrmp);
	}
	else
#endif

	if (prim == DL_UNITDATA_REQ) {
		if (sp->filter[DL_FILTER_OUTGOING] != NULL) {
			struct bpf_insn *insns;
			int slen ;

			insns = (struct bpf_insn *)sp->filter[DL_FILTER_OUTGOING]->b_rptr;
			slen = (*dlpi_filterfunc)(insns, datamp);
			if (!slen) {
				freemsg(hdrmp);
				goto done;
			}
		}
	}

	dlpi_putnext_down_queue(cp, hdrmp);		/* Ignore flow control */

done:
	if (cp->init->txmon_enabled) /* driver still alive check */
		dlpi_txmon_check(cp);

	return;

fail:
	ATOMIC_INT_INCR(sp->sap_tx.error);
	freemsg(mp);
}

void
dlpi_send_llc_message(per_sap_info_t *sp, int prim, mblk_t *mp,
				     int dst_off, int dst_len, int poll_flag)
{
	int	xid = (prim == DL_XID_REQ || prim == DL_XID_RES);
	int	flag = poll_flag == DL_POLL_FINAL;

	if (sp->sap_type != FR_LLC || sp->llcmode == LLC_OFF) {
		dlpi_send_dl_error_ack(sp,prim,DL_NOTSUPPORTED,0);
		goto fail;
	}
	if (dst_len != MAC_ADDR_SZ+1) {
		dlpi_send_dl_error_ack(sp,prim,DL_BADADDR,0);
		goto fail;
	}
	if (sp->llcxidtest_flg & (xid ? DL_AUTO_XID : DL_AUTO_TEST)) {
		dlpi_send_dl_error_ack(sp, prim, xid?DL_XIDAUTO:DL_TESTAUTO, 0);
		goto fail;
	}

	dlpi_send_unitdata(sp, prim, mp, dst_off, dst_len, flag);
	return;
fail:
	ATOMIC_INT_INCR(sp->sap_tx.error);
	freemsg(mp);
}

/* dlpi_putnext_down_queue
 *
 * calling putnext(cp->down_queue, mp) is bad on MP machines
 * because while txmon may be running on one cpu another may be sending
 * stuff to the mdi driver, rendering the flushq less effective.  we
 * check to make sure the card is still usable before doing the putnext
 * to eliminate the queue filling up again
 * Note that we do not acquire the txmon spin lock here as this severely
 * eliminates any MP benefits since multiple cpus can not run in this
 * routine concurrently, meaning that we'll never really call the put 
 * routine for the queue concurrently either.  See put(D2).
 * Note that card may be suspended.  It's still ok to send messages to
 * card while in suspended state (M_FLUSH, M_IOCTL, etc.) but not M_DATA, since
 * that would need to go out on the wire, something it obviously can't do.
 * caller should have taken care of this, but we ensure that they have here.
 */
void
dlpi_putnext_down_queue(per_card_info_t *cp, mblk_t *mp)
{
	ASSERT(cp != NULL);
	ASSERT(cp->down_queue != NULL);
	ASSERT(mp != NULL);
	if (cp->issuspended == 1) {
		ASSERT(mp->b_datap->db_type != M_DATA);
	}

	/* if we're initializing card or driver is ready then send downstream */
	if ((cp->mdi_driver_ready == 1) ||
		(cp->mdi_primitive_handler == mdi_INIT_prim_handler)) {
		putnext(cp->down_queue,mp);	/* Ignore flow control */
	} else {
		DLPI_PRINTF2000(("dlpi_putnext_down_queue: dead card freeing message\n"));
		freemsg(mp);
	}
}
