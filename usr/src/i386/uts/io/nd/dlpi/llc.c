#ident "@(#)llc.c	26.2"
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

struct llc_header {
	unchar	dsap;
	unchar	ssap;
	unchar	cmd;
};

#pragma pack(1)
struct snap_header {
	ushort	prot_id1;		/* 3 octet prot_id field */
	unchar	prot_id2;
	ushort	type;			/* 2 octet type field */
};
#pragma pack()

#define PROT_ID(s) (*(ulong *)(&(s)->prot_id1))

/*
 * LLC-1 protocol implementation
 */

/*
 * Create an LLC (and possibly the SNAP) header and return the mp
 */
int
dlpi_make_llc_ui(register per_sap_info_t *sp, unchar dsap, unchar *buf)
{
	int sz, sap;
	register struct llc_header	*llcp;

	sz = sizeof(struct llc_header);
	sap= sp->sap;

	if (sp->sap_type == FR_SNAP) {
		sz += sizeof(struct snap_header);
		dsap = sap = 0xAA;
	}

	llcp=(struct llc_header *)buf;
	llcp->dsap = dsap;
	llcp->ssap = sap;
	llcp->cmd = LLC_CMD_UI;

	if (sp->sap_type == FR_SNAP) {
		register struct snap_header	*s;

		s=(struct snap_header *)(buf+sizeof(struct llc_header));
		PROT_ID(s) = sp->sap_protid;
		s->type=mdi_htons((ushort)sp->sap);
	}
	return(sz);
}

/*
 * Create an XID header and return its msgb
 */
int
dlpi_make_llc_xid(per_sap_info_t *sp, unchar dsap, int cmd, int poll_flag,
			unchar *buf)
{
	struct llc_header *lp;

	lp = (struct llc_header *)buf;
	lp->dsap = dsap;
	lp->ssap = sp->sap | (cmd ? 0 : LLC_RESPONSE);
	lp->cmd = LLC_CMD_XID | (poll_flag ? LLC_POLL_FINAL : 0x00);

	return (sizeof(struct llc_header));
}

/*
 * Create a TEST header and return its msgb
 */
int
dlpi_make_llc_test(per_sap_info_t *sp, unchar dsap, int cmd, int poll_flag,
			unchar *buf)
{
	struct llc_header *lp;

	lp = (struct llc_header *)buf;

	lp->dsap = dsap;
	lp->ssap = sp->sap | (cmd ? 0 : LLC_RESPONSE);
	lp->cmd = LLC_CMD_TEST | (poll_flag ? LLC_POLL_FINAL : 0x00);
	return (sizeof(struct llc_header));
}

/*
 * Create an message block for the MAC header, and AUTO route the LLC
 * response back to the requesting machine
 */
STATIC void
send_llc_response(per_sap_info_t *sp, struct per_frame_info *f)
{
	per_card_info_t *cp = sp->card_info;
	mblk_t *mp = f->unitdata;
	mblk_t *hdrmp;
	unchar *dest = f->frame_src;
	int hdr_sz, route_len;

	if (!cp->media_specific)
		goto fail;

	if (cp->issuspended) {
		DLPI_PRINTF1000(("DLPI: dropping packet in send_llc_response\n"));
		ATOMIC_INT_INCR(&cp->suspenddrops);
		freemsg(mp);
		return;
	}

	/* Allocate a buffer big-enough here to take the biggest of the
	 * headers that the DLPI module might create (which is):
	 *	---------------------------
	 *	| MAC-header | Route-Info |
	 *	---------------------------
	 *	   (hdr_sz)   MAX_ROUTE_SZ
	 */
	hdr_sz = cp->media_specific->hdr_sz;
	if (!(hdrmp = allocb (hdr_sz+MAX_ROUTE_SZ, BPRI_LO)) ) {
		goto fail;
	}
	hdrmp->b_wptr += hdr_sz;	/* Reserve space for MAC hdr */
	mp->b_rptr = f->frame_data;	/* Strip the old MAC header */
	linkb(hdrmp, mp);		/* Add new header before data */
	
	if (sp->srmode != SR_NON) {
		route_len = dlpiSR_make_header(cp, dest, hdrmp->b_wptr);
		hdrmp->b_wptr += route_len;
	} else {
		route_len = 0;
	}

	if (cp->media_specific->make_hdr(sp, hdrmp, dest, cp->local_mac_addr,
								route_len))
	{
		freemsg(hdrmp);  /* also does a freemsg on mp too from linkb */
		return;
	}

	dlpi_putnext_down_queue(cp,hdrmp);	/* Ignore flow control */
	return;

fail:
	ATOMIC_INT_INCR(sp->sap_tx.error);
	freemsg(mp);
}

/*
 * Process an inbound XID frame, either respond to it automatically, or
 * pass it up to the DLPI user.
 */
STATIC void
process_xid_frame(per_sap_info_t *sp, struct per_frame_info *f, unchar r)
{
	mblk_t *mp = f->unitdata;
	struct llc_header *lp = (struct llc_header *)f->frame_data;
	per_card_info_t *cp = sp->card_info;
	unchar *info;

	ATOMIC_INT_INCR(sp->sap_rx.ucast_xid);
	if (r) {	/* Received an XID Response */
		DLPI_PRINTF08(("Received LLC-XID Response (DSAP=%b)\n", lp->dsap));
		if (!(sp->llcxidtest_flg & DL_AUTO_XID)) {
			dlpi_send_dl_xid_con(sp, f, r);
		}
	} else {	/* Received an XID Request (Command) */
		DLPI_PRINTF08(("Received LLC-XID Request (DSAP=%b)\n", lp->dsap));
		if (sp->llcxidtest_flg & DL_AUTO_XID) {
			lp->dsap = lp->ssap;
			lp->ssap = sp->sap | LLC_RESPONSE;/* Set response bit */
			info = f->frame_data + sizeof(struct llc_header);
			if (info < mp->b_wptr) {/* Info field present */
				info[0] = 0x81;	/* IEEE Basic Format */
				info[1] = 0x80;	/* Class 1 LLC supported */
				info[2] = 0;	/* Rx Window Size */
			}
			send_llc_response(sp, f);
			ATOMIC_INT_INCR(sp->sap_tx.ucast_xid);
		} else {
			dlpi_send_dl_xid_ind(sp, f, r);
		}
	}
}

/*
 * Process an inbound TEST frame, either respond to it automatically, or
 * pass it up to the DLPI user.
 */
STATIC void
process_test_frame(per_sap_info_t *sp, struct per_frame_info *f, unchar r)
{
	mblk_t *mp = f->unitdata;
	struct llc_header *lp = (struct llc_header *)f->frame_data;
	per_card_info_t *cp = sp->card_info;

	ATOMIC_INT_INCR(sp->sap_rx.ucast_test);
	if (r) {	/* Received a TEST Response */
		DLPI_PRINTF08(("Received LLC-TEST Response (DSAP=%b)\n", lp->dsap));
		if (!(sp->llcxidtest_flg & DL_AUTO_TEST)) {
			dlpi_send_dl_test_con(sp, f, r);
		}
	} else {	/* Received a TEST Request (Command) */
		DLPI_PRINTF08(("Received LLC-TEST Request (DSAP=%b)\n", lp->dsap));
		if (sp->llcxidtest_flg & DL_AUTO_TEST) {
			lp->dsap = lp->ssap;
			lp->ssap = sp->sap | LLC_RESPONSE;/* Set response bit */
			send_llc_response(sp, f);
			ATOMIC_INT_INCR(sp->sap_tx.ucast_test);
		} else {
			dlpi_send_dl_test_ind(sp, f, r);
		}
	}
}

int
dlpi_llc_rx(per_sap_info_t *sp, struct per_frame_info *f)
{
	struct llc_header *lp = (struct llc_header *)f->frame_data;
	unchar cmd, response;

	cmd		= lp->cmd;
	response	= lp->ssap & LLC_RESPONSE;
	f->frame_ssap	= lp->ssap;
	f->frame_dsap	= lp->dsap;

	switch (cmd & ~LLC_POLL_FINAL) {
	case LLC_CMD_UI:	/* UI */
		DLPI_PRINTF08(("Received LLC-UI(DSAP=%b)\n", lp->dsap));
		f->frame_data += sizeof(struct llc_header);
		if (sp->sap_type == FR_SNAP)
			f->frame_data += sizeof(struct snap_header);
		dlpi_send_dl_unitdata_ind(sp, f);
		break;
	case LLC_CMD_XID:	/* XID */
		process_xid_frame(sp, f, response);
		break;
	case LLC_CMD_TEST:	/* TEST */
		process_test_frame(sp, f, response);
		break;
	default:
		DLPI_PRINTF10(("Received LLC-UNKNOWN(DSAP=%b, cmd=%b)\n",
							lp->dsap, cmd));
		ATOMIC_INT_INCR(sp->sap_rx.error);
		freemsg(f->unitdata);
	}
	return(1);
}

/* Find the next SAP structure of the frame type */
STATIC per_sap_info_t *
dlpi_findnextsap(ulong type, per_card_info_t *cp, per_sap_info_t *sp)
{
	int i = cp->maxsaps - (sp - cp->sap_table);

	DLPI_PRINTF10(("DLPI: findnextsap(type=%d,sp#=%d)\n",
						type, sp-cp->sap_table));
	while (i) {
		if (sp->sap_state == SAP_BOUND && sp->sap_type == type)
				return(sp);
		++sp;
		--i;
	}
	return ( (per_sap_info_t *)0 );
}

/* Find the first SAP structure of the frame type */
STATIC per_sap_info_t *
dlpi_findfirstsap(ulong type, per_card_info_t *cp)
{
	per_sap_info_t	*sp = cp->sap_table;
	int		i;

	DLPI_PRINTF10(("DLPI: findfirstsap(type=%d)\n", type));
	return (dlpi_findnextsap(type,cp,cp->sap_table));
}


per_sap_info_t *
dlpi_findfirstllcsap(struct per_frame_info *fp, per_card_info_t *cp)
{
	struct snap_header *s;
	per_sap_info_t *sp;
	ulong sap;

	sap=fp->frame_sap;
	switch (sap) {
	case 0xFF: /* B'Cast SAP */
		return (dlpi_findfirstsap(FR_LLC,cp));
	case 0xAA: /* SNAP SAP */
		s = (struct snap_header *) (fp->frame_data 
											+ sizeof(struct llc_header));
		sap = mdi_ntohs(s->type);
		return (dlpi_findsnapsap(PROT_ID(s) & 0xffffff, sap, cp));
	default:
		sp=dlpi_findsap(FR_LLC, sap, cp);
		if (!sp) {
			/* No-one bound to SAP,
			   Send to LLC_DEFAULT SAP instead. */
			sp=dlpi_findsap(FR_LLC, 0xfffffffe, cp);
		}
		return(sp);
	}
}

per_sap_info_t *
dlpi_findnextllcsap(ulong sap, per_card_info_t *cp, per_sap_info_t *sp)
{
	/* B'cast SAP */
	if ((sap == 0xFF) && ((sp - cp->sap_table) < (signed)cp->maxsaps)) {
		return (dlpi_findnextsap(FR_LLC, cp, sp));
	} else {
		return ((per_sap_info_t *)NULL);
	}
}
