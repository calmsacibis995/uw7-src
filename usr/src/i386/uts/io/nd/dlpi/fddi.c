#ident "@(#)fddi.c	11.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif

#define MAC_FC_LLC	0x50;	/* FC - async, 48 bit addr, LLC, priority 0 */

struct fddi_header
{
	unchar	fc;
	unchar	dest[6];
	unchar	src[6];
	unchar	dsap;
};
#define FDDI_HDR_SZ (sizeof(struct fddi_header)-sizeof(unchar/*dsap*/)) /*13*/

STATIC int mdi_fddi_make_hdr(per_sap_info_t *sp, mblk_t *mp,
						unchar *dest, unchar *src, int route_len);
STATIC void mdi_fddi_rx_parse(per_card_info_t *cp, struct per_frame_info *);
STATIC int mdi_fddi_tx_errors(void *statp);
STATIC int mdi_fddi_rx_errors(void *statp);

/*
 * This routine is called by the dlpi driver during
 * first open to register the built-in support for FDDI.
 */

int
mdi_fddi_register()
{
	static const per_media_info_t	mi =
	{
		FDDI_HDR_SZ,			/* hdr_sz */
		sizeof(mac_stats_fddi_t),	/* stats_sz */
		MEDIA_SOURCE_ROUTING | MEDIA_LLC | MEDIA_SNAP,	/* media_flags */
		0,				/* media_private_flags */

		NULL,				/* media_init */
		NULL,                           /* media_close */
		NULL,				/* media_halt */
		NULL,				/* media_svc_reg */
		NULL,				/* media_svc_rcv */
		NULL,				/* media_svc_close */

		mdi_fddi_make_hdr,		/* make_hdr */
		mdi_fddi_rx_parse,		/* rx_parse */

		NULL,				/* media_wr_prim */
		NULL,				/* media_rd_prim */
		NULL,				/* media_wr_data */

		mdi_fddi_tx_errors,		/* rx_errors */
		mdi_fddi_rx_errors,		/* tx_errors */
		NULL,				/* media_ioctl */
	};

	return mdi_register_media(MEDIA_MAGIC, MAC_FDDI, &mi);
}

/*
 * Create the FDDI MAC Header
 *
 * MP Notes:
 *   We must lock around updating the statistics to avoid having
 *   an inconsistent set of statistics.  Other writers are
 *   from DL_CLR_STATISTICS_REQ.  Readers are from DL_GET_STATISTICS_REQ.
 */

STATIC int
mdi_fddi_make_hdr(per_sap_info_t *sp, mblk_t *mp,
			unchar *dest, unchar *src, int route_len)
{
	struct fddi_header *ep;
	int msg_size;

	/* Create MAC header */
	ep = (struct fddi_header *)mp->b_rptr;

	ep->fc = MAC_FC_LLC;
	bcopy(dest, ep->dest, MAC_ADDR_SZ);
	bcopy(src, ep->src , MAC_ADDR_SZ);
	if (route_len)
		ep->src[0] |= FDDI_ROUTE_INDICATOR;	

	/* Update statistics */
	msg_size = msgdsize(mp);
	ATOMIC_INT_ADD(sp->sap_tx.octets, msg_size);
	if (ep->dest[0] & 0x01) {
		if (*((ulong *) (ep->dest)) == 0xffffffff)
			ATOMIC_INT_INCR(sp->sap_tx.bcast);
		else
			ATOMIC_INT_INCR(sp->sap_tx.mcast);
	} else {
		ATOMIC_INT_INCR(sp->sap_tx.ucast);
	}

	return 0;
}

/*
 * Parse the FDDI MAC Header
 */

STATIC void
mdi_fddi_rx_parse(per_card_info_t *cp, struct per_frame_info *fp)
{
	mblk_t *mp = fp->unitdata;
	struct fddi_header *ep = (struct fddi_header *)mp->b_rptr;

	fp->frame_dest	= ep->dest;
	fp->frame_src	= ep->src;
	fp->frame_data	= mp->b_rptr + FDDI_HDR_SZ;
	fp->frame_len	= msgdsize(mp);
	fp->frame_type	= FR_LLC;
	fp->frame_sap	= ep->dsap;
	fp->route_present = ep->src[0] & FDDI_ROUTE_INDICATOR;

	if (ep->dest[0] & 0x01) {
		if (*((ulong *)(ep->dest)) == 0xffffffff)
			fp->dest_type = FR_BROADCAST;
	else
		fp->dest_type = FR_MULTICAST;
	}
	else
		fp->dest_type = FR_UNICAST;

	ep->src[0] &= ~FDDI_ROUTE_INDICATOR;

	DLPI_PRINTF04(("FDDI-rx_parse: type=%x, sap=%x\n",
						fp->frame_type, fp->frame_sap));

}

/*
 * Returns the Total number of media dependent transmit errors
 */

STATIC int
mdi_fddi_tx_errors(void *statp)
{
	register mac_stats_fddi_t	*mp = (mac_stats_fddi_t *)statp;

	return mp->mac_error_cts;
}

/*
 * Returns the Total number of media dependent receive errors
 */

STATIC int
mdi_fddi_rx_errors(void *statp)
{
	register mac_stats_fddi_t	*mp = (mac_stats_fddi_t *)statp;

	return mp->mac_lost_cts;
}
