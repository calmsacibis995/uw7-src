#ident "@(#)xnms.c	7.1"
#ident "$Header$"

/*
 * File xnms.c
 * eXample Network Media Support driver
 *
 *	Copyright (C) The Santa Cruz Operation, 1993-1996.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated
 *	as Confidential.
 *
 * If you use this example to develop support for additional media,
 * DO NOT use the driver prefix xnms.  It will surely conflict with
 * someone else doing the same thing.
 *
 * This example supports Ethernet and is similar to the built-in
 * support for Ethernet.
 */

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                       *
 * NOTE -- NOTE -- NOTE -- NOTE -- NOTE -- NOTE -- NOTE -- NOTE -- NOTE  *
 *                                                                       *
 *      This driver is currently for discussion of concept only.         *
 *      It has not been tested and probably has some things wrong!!!     *
 *      Go ahead, blame me directly for all errors: donwo@sco.com        *
 *      I would also like to hear about good stuff.                      *
 *                                                                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>

#include "sys/mdi.h"
#include <sys/dlpimod.h>
#include <sys/scodlpi.h>

#include "xnms.h"

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

STATIC void	xnms_media_init(per_card_info_t *cp);
STATIC void	xnms_media_halt(per_card_info_t *cp);
STATIC int	xnms_media_svc_reg(per_card_info_t *cp, mblk_t *mp,
						    per_sap_info_t *sp);
STATIC int	xnms_media_svc_rcv(queue_t *q, mblk_t *mp);
STATIC void	xnms_media_svc_close(per_card_info_t *cp);

STATIC int	xnms_make_hdr(register per_sap_info_t *sp, mblk_t *mp,
				    unchar *dest, unchar *src, int route_len);
STATIC void	xnms_rx_parse(per_card_info_t *cp, struct per_frame_info *fp);
STATIC int	xnms_tx_errors(void *statp);
STATIC int	xnms_rx_errors(void *statp);

STATIC miocret_t	xnms_ioctl(mblk_t *mp, per_card_info_t *cp,
						    per_sap_info_t *sp);

STATIC void	xnms_send_daemon_message(per_card_info_t *cp, const char *Str);

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				Global Routines				 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * This is the xnms driver init routine.  It is called once
 * during the boot procedure.  Just like any other driver.
 *
 * Media support drivers should register their intent to support
 * there media type during there init.  Media support goes to
 * the first one to ask (with the correct words).  The built-in
 * support for Ethernet, Token-Ring, ... are registered at dlpi
 * first open, which is after all init routines have been called.
 *
 * The return values from mdi_register_media() are:
 *	EDOM    Invalid media_magic
 *	EEXIST  Media type already supported
 *	ERANGE  Invalid media type
 *	ENOENT  mi was NULL
 *	EINVAL  Missing required routine
 *	0       Successful
 *
 * If mdi_register_media() returns a non successful value,
 * xnms will not be called upon to support any media.
 */

int
xnms_init()
{
    static const per_media_info_t	mi =
    {
	ETHER_HDR_SZ,			/* hdr_sz */
	sizeof(mac_stats_eth_t),	/* stats_sz */
	MEDIA_ETHERNET_II | MEDIA_XNS | MEDIA_LLC | MEDIA_SNAP,	/*media_flags */
	0,				/* media_private_flags */

	xnms_media_init,		/* media_init */
	xnms_media_halt,		/* media_halt */
	xnms_media_svc_reg,		/* media_svc_reg */
	xnms_media_svc_rcv,		/* media_svc_rcv */
	xnms_media_svc_close,		/* media_svc_close */

	xnms_make_hdr,			/* make_hdr */
	xnms_rx_parse,			/* rx_parse */

	xnms_tx_errors,			/* rx_errors */
	xnms_rx_errors,			/* tx_errors */
	xnms_ioctl,			/* media_ioctl */
    };

#ifdef DEBUG
    int	Status;

    if (!(mdi_register_media(MEDIA_MAGIC, MAC_CSMACD, &mi)))
	CMN_ERR(CE_CONT, "xnms_init(): Registration successful\n");
    else
	CMN_ERR(CE_CONT, "xnms_init(): Registration fail: %d\n", Status);
#else
    mdi_register_media(MEDIA_MAGIC, MAC_CSMACD, &mi);
#endif

    return 0;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *			      Registered Routines			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * This routine is called for each network card that indicates
 * that it uses the media type that this driver supports.  The
 * call is made during the processing of the MAC_INFO_ACK message
 * requested during the initialization of the MDI Driver.  This
 * is the first time we know what kind of media the card likes.
 * Only one media type will be assigned to any single card.
 *
 * At the time this routine is called, the media services daemon
 * may not yet have registered.  We must also be prepared to get
 * along the best we can without help.  Maybe with lesser
 * functionality.
 *
 * The card may have provided special flags for our private use
 * in media_private_flags.  Such information might include
 * compression or special framing that the card can do or needs.
 *
 * The variable media_private is a place in the cards structure
 * dedicated to holding anything needed by card specific media
 * support.
 *
 * This routine is optional.  If this routine is not implemented,
 * it is assumed that there is no special action required to begin
 * support for this media type.
 */

STATIC void
xnms_media_init(per_card_info_t *cp)
{
    CMN_ERR(CE_CONT,"xnms_media_init(cp=0x%x) flags=0x%x private=0x%x\n",
				cp,
				cp->media_specific->media_flags,
				cp->media_specific->media_private);

    cp->media_private = NULL;	/* Private.  For our use */
}

/*
 * This routine is called when the MDI driver is about to be
 * unlinked from below the dlpi module.  This may be a result of
 * error detection/correction or may be a result of network stop.
 *
 * When the card is restored to service, xnms_media_init() will
 * again be called.
 *
 * This routine is optional.  If this routine is not implemented,
 * it is assumed that there is no special action required to end
 * support for this media type.
 */

STATIC void
xnms_media_halt(per_card_info_t *cp)
{
    CMN_ERR(CE_CONT, "xnms_media_halt(cp=0x%x)\n", cp);

    cp->media_private = NULL;
    xnms_send_daemon_message(cp, "The world has ended");
}

/*
 * This routine will be called as a result of a DLPI_MEDA_REGISTER
 * ioctl only if there is currently no daemon registered for this
 * card.  The purpose is to qualify that the requesting daemon is
 * of a type known to us and is capable of providing our support.
 *
 * Return:
 *	0	The DLPI_MEDA_REGISTER will be successful.
 *		This stream will be assigned to xnms_media_svc_rcv()
 *
 *	else	The stream will NOT be assigned to xnms_media_svc_rcv()
 *		The DLPI_MEDA_REGISTER ioctl fail with the error value
 *		returned from this routine.
 *
 * Since the response to the ioctl remains outstanding and the
 * stream has not yet been assigned to us, this is NOT a good
 * time to be sending messages back to the daemon.  Wait until
 * the first call to xnms_media_svc_rcv() or expect that the
 * daemon will call back with an introductory greeting.
 *
 * This routine is optional.  If this routine or xnms_media_svc_rcv
 * are not implemented, daemon processes will be refused the request
 * to register since there will be no way to connect him.
 */

STATIC int
xnms_media_svc_reg(per_card_info_t *cp, mblk_t *mp, per_sap_info_t *sp)
{
    CMN_ERR(CE_CONT, "xnms_media_svc_reg(cp=0x%x, sp=0x%x)\n", cp, sp);

    /*
     * A real driver would probably check some magic in the
     * message body.  We just take the caller in good faith.
     */
    return 0;
}

/*
 * This routine is called when the media support daemon closes
 * the stream registered by DLPI_MEDA_REGISTER.  Following this
 * call the daemon will no longer be registered.
 *
 * This routine is optional.  If this routine is not implemented,
 * it is assumed that there is no special action required when
 * the daemon exits.
 */

STATIC void
xnms_media_svc_close(per_card_info_t *cp)
{
    CMN_ERR(CE_CONT, "xnms_media_svc_close(cp=0x%x)\n", cp);
}

/*
 * This routine will be called for every message sent by the daemon.
 *
 *	q	The down queue from the daemon
 *	mp	The current message from the daemon
 *
 * Return:
 *	0	The message will be placed back on the queue and
 *		later sent back to us.
 *
 *	else	The packet was handled by this routine as per
 *		requirements known only to this media support.
 *
 * Remember: You may have kept your good stuff in cp->media_private
 *
 * This routine is optional.  If this routine or xnms_media_svc_reg
 * are not implemented, daemon processes will be refused the request
 * to register since there will be no way to connect him.
 */

STATIC int
xnms_media_svc_rcv(queue_t *q, mblk_t *mp)
{
    per_sap_info_t	*sp  = (per_sap_info_t *)q->q_ptr;
    per_card_info_t	*cp = sp->card_info;

    CMN_ERR(CE_CONT, "xnms_media_svc_rcv(mp=0x%x) cp=0x%x\n", mp, cp);

    /*
     * A real driver would do something with this message
     * from our service daemon.
     */
    freemsg(mp);	/* /dev/null */
    return 1;
}

/*
 * This routine is called for every packet given to the dlpi module
 * and intended for our MDI driver.  This routine should:
 *	Create a header of the type expected by our MDI driver
 *		(In this case an Ethernet MAC Header)
 *	Update statistics for our media
 *
 * Return:
 *	0	Normal.  Pass the message to the MDI driver.
 *
 *	else	For reasons only known to this media support module,
 *		This packet should not be passed to the MDI driver.
 *		Returning a non zero value will cause the packet to
 *		be freed and not sent to the MDI driver.  If this was
 *		due to a transmission error, this routine should update
 *		the statistics by calling qincl(&sp->sap_tx.error).
 *
 * This routine is mandatory.  mdi_register_media() will not register
 * support for media if this routine is not provided.
 */

STATIC int
xnms_make_hdr(register per_sap_info_t *sp, mblk_t *mp, unchar *dest,
			unchar *src, int route_len)
{
    register struct ether_header	*ep;
    int					len;
    int					msg_size;
    per_card_info_t			*cp = sp->card_info;
    LOCK_STAT_VAR(s);

    CMN_ERR(CE_CONT, "xnms_make_hdr() cp=0x%x\n", cp);

    msg_size = msgdsize(mp);
    len = msg_size - route_len - ETHER_HDR_SZ;
    ep = (struct ether_header *)mp->b_rptr;
    bcopy(dest, ep->dest, MAC_ADDR_SZ);
    bcopy(src, ep->src, MAC_ADDR_SZ);
    ep->type_len =
	mdi_htons((short)(sp->sap_type == FR_ETHER_II ? sp->sap : len));

    /*
     * Update statistics
     */

    LOCK_SAPSTATS(sp, s);
    qaddl(&sp->sap_tx.octets, msg_size);

    if (*((ulong *)(ep->dest)) == 0xffffffff)	/* Broadcast frame ? */
	qincl(&sp->sap_tx.bcast);
    else if (ep->dest[0] & 0x01)		/* Multicast frame ? */
	qincl(&sp->sap_tx.mcast);
    else
	qincl(&sp->sap_tx.ucast);

    UNLOCK_SAPSTATS(sp, s);
    return 0;
}

/*
 * This routine is called for every M_DATA packet delivered to the
 * dlpi module by the MDI driver and intended for the stacks.  This
 * routine should:
 *	Parse the MAC header (In this case an Ethernet MAC Header)
 *
 * This routine is mandatory.  mdi_register_media() will not register
 * support for media if this routine is not provided.
 */

STATIC void
xnms_rx_parse(per_card_info_t *cp, register struct per_frame_info *fp)
{
    mblk_t *mp = fp->unitdata;
    int	len;
    register struct ether_header *ep = (struct ether_header *)mp->b_rptr;

    len = mdi_ntohs(ep->type_len);
    CMN_ERR(CE_CONT, "xnms_rx_parse: type/len=%x, dsap=%x ssap=%x\n",
					len, (uint)ep->dsap, (uint)ep->ssap);

    fp->frame_dest	= ep->dest;
    fp->frame_src	= ep->src;
    fp->frame_data	= mp->b_rptr + ETHER_HDR_SZ;
    fp->route_present = 0;
    if ( len > ETHER_MAX_PDU )
    {
	fp->frame_type	= FR_ETHER_II;
	fp->frame_sap	= len;
	fp->frame_len	= msgdsize(mp);
    }
    else
    {
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

    CMN_ERR(CE_CONT, "xnms_rx_parse: type=%x, sap=%x\n",
					    fp->frame_type, fp->frame_sap);
}

/*
 * Returns the Total number of media dependent transmit errors
 *
 * This routine is optional.  If this routine is not implemented,
 * the errors accumulated into mac_tx.mac_error returned by
 * DL_GET_STATISTICS_REQ will always be zero.
 */


STATIC int
xnms_tx_errors(void *statp)
{
    register mac_stats_eth_t	*mp = (mac_stats_eth_t *)statp;

    CMN_ERR(CE_CONT, "xnms_tx_errors: oframe_coll=%d tx_errors=%d carrier=%d\n",
					mp->mac_oframe_coll,
					mp->mac_tx_errors,
					mp->mac_carrier);

    return	mp->mac_oframe_coll +
		mp->mac_tx_errors +
		mp->mac_carrier;
}

/*
 * Returns the Total number of media dependent receive errors
 *
 * This routine is optional.  If this routine is not implemented,
 * the errors accumulated into mac_rx.mac_error returned by
 * DL_GET_STATISTICS_REQ will always be zero.
 */

STATIC int
xnms_rx_errors(void *statp)
{
    register mac_stats_eth_t	*mp = (mac_stats_eth_t *)statp;

    CMN_ERR(CE_CONT, "xnms_rx_errors: "
	     "align=%d badsum=%d badlen=%d no_resource=%d frame_nosr=%d\n",
					mp->mac_align,
					mp->mac_badsum,
					mp->mac_badlen,
					mp->mac_no_resource,
					mp->mac_frame_nosr);

    return	mp->mac_align +
		mp->mac_badsum +
		mp->mac_badlen +
		mp->mac_no_resource +
		mp->mac_frame_nosr;
}

/*
 * Process an ioctl message received at the DLPI from a protocol stack.
 *
 * Return:
 *	MIOCRET_PUTBQ		Put the message back on the queue for
 *				later processing
 *
 *	MIOCRET_DONE		the ioctl has been handled (ACK/NAK)
 *
 *	MIOCRET_NOT_MINE	This ioctl is not recognized by the
 *				media support. Send it to the MDI driver.
 *
 * This routine is optional.  If this routine is not implemented,
 * it is assumed that the media support does not support additional
 * I/O controls.
 */

STATIC miocret_t
xnms_ioctl(mblk_t *mp, per_card_info_t *cp, per_sap_info_t *sp)
{
    struct iocblk	*iocp = (struct iocblk *)mp->b_rptr;

    CMN_ERR(CE_CONT, "xnms_ioctl: 0x%x\n", iocp->ioc_cmd);

    switch (iocp->ioc_cmd)
    {
	/*
	 * These will be implemented as required to support the
	 * media and is beyond the scope of this simple example.
	 */
#ifdef NOT_YET
	case MEDIA_SPECIFIC_IOCTL:
	    handle_media_ioctl();
	    send_back_ack(sp);
	    return MIOCRET_DONE;
#endif
    }

    return MIOCRET_NOT_MINE;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				Private Routines			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * This is a simple message sending routine.
 * A more complete routine would also support non text data.
 *
 * If no daemon is registered to serve, no message is sent.
 */

STATIC void
xnms_send_daemon_message(per_card_info_t *cp, const char *Str)
{
    mblk_t		*mp;
    int			len;
    register queue_t	*up_queue;

    CMN_ERR(CE_CONT, "xnms_send_daemon_message() cp=0x%x q=0x%x %s\n",
					cp, cp->media_svcs_queue, Str);

    if (!cp->media_svcs_queue)
	return;			/* No daemon registered */

    len = kstrlen(Str) + 1;
    if (!(mp = allocb(len, BPRI_LO)))
	return;

    bcopy((caddr_t)Str, mp->b_rptr, len);
    mp->b_wptr += len;

    /*
     * We do not want to wind up with the entire kernels message
     * pool sitting on this guys queue.  On the other hand, we
     * should have a way to know that something was lost.  A
     * real driver should have a way to deal with this.
     */

    up_queue = RD(cp->media_svcs_queue);

    if (canput(up_queue))
	putq(up_queue, mp);
    else
	freemsg(mp);
}

