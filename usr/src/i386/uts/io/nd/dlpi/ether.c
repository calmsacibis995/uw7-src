#ident "@(#)ether.c	25.1"
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

#define ETHER_MAX_PDU	1500
#define ETHER_HDR_SZ	14		/* sizeof(dest+src+type_len) only */

struct ether_header
{
	unchar		dest[6];
	unchar		src[6];
	unsigned short	type_len;	/* E-II frames=type, 802.3 frames=len */
	unsigned char	dsap;		/* 802.2 frame DSAP */
	unsigned char	ssap;		/* 802.2 frame SSAP */
};

STATIC int mdi_ether_make_hdr(per_sap_info_t *sp, mblk_t *mp,
						unchar *dest, unchar *src, int route_len);
STATIC void mdi_ether_rx_parse(per_card_info_t *cp, struct per_frame_info *);
STATIC int mdi_ether_tx_errors(void *mp);
STATIC int mdi_ether_rx_errors(void *mp);

/*
 * This routine is called by the dlpi driver during first
 * open to register the built-in support for Ethernet.
 */

int
mdi_ether_register()
{
	static const per_media_info_t	mi =
	{
		ETHER_HDR_SZ,			/* hdr_sz */
		sizeof(mac_stats_eth_t),	/* stats_sz */
		MEDIA_ETHERNET_II | MEDIA_XNS | MEDIA_LLC | MEDIA_SNAP,	/* flags */
		0,				/* media_private_flags */

		NULL,				/* media_init */
		NULL,                           /* media_close */
		NULL,				/* media_halt */
		NULL,				/* media_svc_reg */
		NULL,				/* media_svc_rcv */
		NULL,				/* media_svc_close */

		mdi_ether_make_hdr,		/* make_hdr */
		mdi_ether_rx_parse,		/* rx_parse */

		NULL,				/* media_wr_prim */
		NULL,				/* media_rd_prim */
		NULL,				/* media_wr_data */

		mdi_ether_tx_errors,		/* rx_errors */
		mdi_ether_rx_errors,		/* tx_errors */
		NULL,				/* media_ioctl */
	};

	return mdi_register_media(MEDIA_MAGIC, MAC_CSMACD, &mi);
}

/*
 * Create the Ethernet MAC Header
 *
 * MP Notes:
 *   We lock around updating the statistics to avoid having
 *   an inconsistent set of statistics.  Other writers are
 *   from DL_CLR_STATISTICS_REQ.  Readers are from DL_GET_STATISTICS_REQ.
 */

STATIC int
mdi_ether_make_hdr(register per_sap_info_t *sp, mblk_t *mp, unchar *dest,
			unchar *src, int route_len)
{
	register struct ether_header	*ep;
	int					msg_size;
	int					len;

	msg_size = msgdsize(mp);
	len = msg_size - route_len - ETHER_HDR_SZ;

	ep = (struct ether_header *) mp->b_rptr;
	bcopy(dest, ep->dest, MAC_ADDR_SZ);
	bcopy(src, ep->src, MAC_ADDR_SZ);
	ep->type_len = mdi_htons((short) (sp->sap_type == FR_ETHER_II 
				      ? sp->sap : len));

	/*
	 * Update statistics
	 */

	ATOMIC_INT_ADD(sp->sap_tx.octets, msg_size);

	if (*((ulong *)(ep->dest)) == 0xffffffff)	/* Broadcast frame ? */
		ATOMIC_INT_INCR(sp->sap_tx.bcast);
	else if (ep->dest[0] & 0x01)			/* Multicast frame ? */
		ATOMIC_INT_INCR(sp->sap_tx.mcast);
	else
		ATOMIC_INT_INCR(sp->sap_tx.ucast);

	return 0;
}

/*
 * Parse the Ethernet MAC Header
 */
STATIC void
mdi_ether_rx_parse(per_card_info_t *cp, register struct per_frame_info *fp)
{
	mblk_t *mp = fp->unitdata;
	int	len;
	register struct ether_header *ep = (struct ether_header *)mp->b_rptr;

	len = mdi_ntohs(ep->type_len);
	DLPI_PRINTF04(("ETHER-rx_parse: type/len=%x, dsap=%x ssap=%x\n",
					len, (uint)ep->dsap, (uint)ep->ssap));

	fp->frame_dest	= ep->dest;
	fp->frame_src	= ep->src;
	fp->frame_data	= mp->b_rptr + ETHER_HDR_SZ;
	fp->route_present = 0;
	if ((len > ETHER_MAX_PDU) && (!cp->disab_ether)) {
		fp->frame_type	= FR_ETHER_II;
		fp->frame_sap	= len;
		fp->frame_len	= msgdsize(mp);
	} else {
		fp->frame_len	= len + ETHER_HDR_SZ;
		mp->b_wptr = mp->b_rptr + fp->frame_len; /* Strip padding */
		if (ep->dsap == 0xff && ep->ssap == 0xff) {
			fp->frame_type	= FR_XNS;
			fp->frame_sap	= 0;
		} else {
			fp->frame_type	= FR_LLC;
			fp->frame_sap	= ep->dsap;
		}
	}

	if (*((ulong *)(ep->dest)) == 0xffffffff)	/* Broadcast frame ? */
		fp->dest_type = FR_BROADCAST;
	else if (ep->dest[0] & 0x01)		/* Multicast frame ? */
		fp->dest_type = FR_MULTICAST;
	else
		fp->dest_type = FR_UNICAST;

	DLPI_PRINTF04(("ETHER-rx_parse: type=%x, sap=%x\n",
					fp->frame_type, fp->frame_sap));
}

/*
 * Returns the Total number of media dependent transmit errors
 */

STATIC int
mdi_ether_tx_errors(void *statp)
{
	register mac_stats_eth_t	*mp = (mac_stats_eth_t *)statp;

	return	mp->mac_oframe_coll +
		mp->mac_tx_errors +
		mp->mac_carrier;
}

/*
 * Returns the Total number of media dependent receive errors
 */

STATIC int
mdi_ether_rx_errors(void *statp)
{
	register mac_stats_eth_t	*mp = (mac_stats_eth_t *)statp;

	return	mp->mac_align +
		mp->mac_badsum +
		mp->mac_badlen +
		mp->mac_no_resource +
		mp->mac_frame_nosr;
}

