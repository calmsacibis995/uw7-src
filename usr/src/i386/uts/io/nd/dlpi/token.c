#ident "@(#)token.c	11.1"
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

#define MAC_AC		0x10;		   /* access control field value */
#define MAC_FC_LLC	0x40;		   /* field control for LLC frame */

struct token_header
{
	unchar	ac;
	unchar	fc;
	unchar	dest[6];
	unchar	src[6];
	unchar	dsap;
};
#define TOKEN_HDR_SZ (sizeof(struct token_header)-sizeof(unchar/*ac*/)) /* 14 */

STATIC int mdi_token_make_hdr(per_sap_info_t *sp, mblk_t *mp,
					unchar *dest, unchar *src, int route_len);
STATIC void mdi_token_rx_parse(per_card_info_t *cp, struct per_frame_info *);
STATIC int mdi_token_tx_errors(void *statp);
STATIC int mdi_token_rx_errors(void *statp);

/*
 * This routine is called by the dlpi driver during first
 * open to register the built-in support for Token-Ring.
 */

int
mdi_token_register()
{
	static const per_media_info_t	mi =
	{
		TOKEN_HDR_SZ,			/* hdr_sz */
		sizeof(mac_stats_tr_t),		/* stats_sz */
		MEDIA_SOURCE_ROUTING | MEDIA_LLC | MEDIA_SNAP,	/* flags */
		0,				/* media_private_flags */

		NULL,				/* media_init */
		NULL,                           /* media_close */
		NULL,				/* media_halt */
		NULL,				/* media_svc_reg */
		NULL,				/* media_svc_rcv */
		NULL,				/* media_svc_close */

		mdi_token_make_hdr,		/* make_hdr */
		mdi_token_rx_parse,		/* rx_parse */

		NULL,				/* media_wr_prim */
		NULL,				/* media_rd_prim */
		NULL,				/* media_wr_data */

		mdi_token_tx_errors,		/* rx_errors */
		mdi_token_rx_errors,		/* tx_errors */
		NULL,				/* media_ioctl */
	};

	return mdi_register_media(MEDIA_MAGIC, MAC_TPR, &mi);
}

/*
 * Create the Token Ring MAC Header
 *
 * MP Notes:
 *   We must lock around updating the statistics to avoid having
 *   an inconsistent set of statistics.  Other writers are
 *   from DL_CLR_STATISTICS_REQ.  Readers are from DL_GET_STATISTICS_REQ.
 */

STATIC int
mdi_token_make_hdr(per_sap_info_t *sp, mblk_t *mp,
			unchar *dest, unchar *src, int route_len)
{
	struct token_header	*ep;
	int					 msg_size;

	/* Create MAC header */
	ep = (struct token_header *)mp->b_rptr;

	ep->ac = MAC_AC;
	ep->fc = MAC_FC_LLC;
	bcopy(dest, ep->dest, MAC_ADDR_SZ);
	bcopy(src, ep->src , MAC_ADDR_SZ);
	if (route_len)
		ep->src[0] |= ROUTE_INDICATOR;	

	/* Update statistics */
	msg_size = msgdsize(mp);
	ATOMIC_INT_ADD(sp->sap_tx.octets, msg_size);
	if (*((ulong *) (ep->dest)) == 0xffffffff) /* Broadcast frame ? */
		ATOMIC_INT_INCR(sp->sap_tx.bcast);
	else if (*((ushort *) (ep->dest)) == 0x00c0) /* Multicast frame ? */
		ATOMIC_INT_INCR(sp->sap_tx.mcast);
	else
		ATOMIC_INT_INCR(sp->sap_tx.ucast);

	return 0;
}

/*
 * Parse the Token Ring MAC Header
 */

STATIC void
mdi_token_rx_parse(per_card_info_t *cp, struct per_frame_info *fp)
{
	mblk_t *mp = fp->unitdata;
	struct token_header *ep = (struct token_header *)mp->b_rptr;

	fp->frame_dest	= ep->dest;
	fp->frame_src	= ep->src;
	fp->frame_data	= mp->b_rptr + TOKEN_HDR_SZ;
	fp->frame_len	= msgdsize(mp);
	fp->frame_type	= FR_LLC;
	fp->frame_sap	= ep->dsap;
	fp->route_present = ep->src[0] & ROUTE_INDICATOR;	

	if (*((ulong *)(ep->dest)) == 0xffffffff)		/* Broadcast frame ? */
		fp->dest_type = FR_BROADCAST;
	else if (*((ushort *)(ep->dest)) == 0x00c0)		/* Multicast frame ? */
		fp->dest_type = FR_MULTICAST;
	else
		fp->dest_type = FR_UNICAST;

	ep->src[0] &= ~ROUTE_INDICATOR;

	DLPI_PRINTF10(("TOKEN-rx_parse: type=%x, sap=%x\n",
				fp->frame_type, fp->frame_sap));
}

/*
 * Returns the Total number of media dependent transmit errors
 */

STATIC int
mdi_token_tx_errors(void *statp)
{
	register mac_stats_tr_t	*mp = (mac_stats_tr_t *)statp;

	return	mp->mac_aborttranserrors +
		mp->mac_lostframeerrors +
		mp->mac_badlen;
}

/*
 * Returns the Total number of media dependent receive errors
 */

STATIC int
mdi_token_rx_errors(void *statp)
{
	register mac_stats_tr_t	*mp = (mac_stats_tr_t *)statp;

	return	mp->mac_lineerrors +
		mp->mac_acerrors +
		mp->mac_receivecongestions +
		mp->mac_framecopiederrors +
		mp->mac_frame_nosr +
		mp->mac_baddma +
		mp->mac_timeouts;
}

