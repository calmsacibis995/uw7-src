#ident "@(#)isdn.c	29.4"
#ident "$Header$"

/*
 * File isdn.c
 * ISDN Network Media Support driver
 *
 *	Copyright (C) The Santa Cruz Operation, 1996.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated
 *	as Confidential.
 *
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif

#define ISDN_HDR_SZ	0		/* ISDN has no header */
#define EXPECTED_CONF	0xBEAD		/* DATA_B3 confirmation pattern */
#define BAD_INFO_MASK	0x3000		/* DATA_B3 confirmation error mask */
#define BAD_NCCI	0x2002		/* illegal NCCI	*/
#define	DATA_B3_HIGH_WATER	7	/* support only 7 unconfirmed DATA_B3.REQ */
#define	DATA_B3_LOW_WATER	3	/* stop flow control at this point */

STATIC void	mdi_isdn_media_init(per_card_info_t *cp);
STATIC void	mdi_isdn_media_halt(per_card_info_t *cp);
STATIC int	mdi_isdn_media_svc_reg(per_card_info_t *cp, mblk_t *mp,
						    per_sap_info_t *sp);
STATIC int	mdi_isdn_media_svc_rcv(queue_t *q, mblk_t *mp);
STATIC void	mdi_isdn_media_svc_close(per_card_info_t *cp);

STATIC int	mdi_isdn_make_hdr(register per_sap_info_t *sp, mblk_t *mp,
				    unchar *dest, unchar *src, int route_len);
STATIC void	mdi_isdn_rx_parse(per_card_info_t *cp, struct per_frame_info *fp);
STATIC mprimret_t	mdi_isdn_media_wr_data(mblk_t *mp,
						       per_card_info_t *cp,
						       per_sap_info_t *sp);
STATIC mprimret_t	mdi_isdn_media_wr_prim(mblk_t *mp,
						       per_card_info_t *cp,
						       per_sap_info_t *sp);
STATIC mprimret_t	mdi_isdn_media_rd_prim(queue_t *q,
						       mblk_t *mp,
						       per_card_info_t *cp);
STATIC int	mdi_isdn_tx_errors(void *statp);
STATIC int	mdi_isdn_rx_errors(void *statp);

STATIC miocret_t	mdi_isdn_ioctl(mblk_t *mp, per_card_info_t *cp,
						    per_sap_info_t *sp);

STATIC void     mdi_isdn_handle_register_ioctl(per_sap_info_t *sp, mblk_t *mp);

STATIC void     mdi_isdn_handle_release_ioctl(per_sap_info_t *sp, mblk_t *mp);

STATIC void	mdi_isdn_close(queue_t *q, per_sap_info_t *sp);

STATIC void	mdi_isdn_send_daemon_message(per_card_info_t *cp, const char *Str);

/* global variables */

STATIC ulong	isdn_logger_daemon_registered = FALSE;
STATIC ulong	logging = FALSE;
STATIC queue_t  *logging_queue;


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				Global Routines				 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * This is the isdn driver init routine.  It is called once
 * during the boot procedure.
 *
 * Media support drivers should register their intent to support
 * their media type during their init.  Media support goes to
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
 * isdn will not be called upon to support any media.
 */

int
mdi_isdn_register()
{
    static const per_media_info_t	mi =
    {
	ISDN_HDR_SZ,			/* hdr_sz */
	sizeof(mac_stats_isdn_t),	/* stats_sz */
	MEDIA_ISDN,			/* media_flags */
	0,				/* media_private_flags */

	NULL,				/* media_init */
	mdi_isdn_close,                 /* media_close */
	mdi_isdn_media_halt,		/* media_halt */
	mdi_isdn_media_svc_reg,		/* media_svc_reg */
	mdi_isdn_media_svc_rcv,		/* media_svc_rcv */
	mdi_isdn_media_svc_close,	/* media_svc_close */

	mdi_isdn_make_hdr,		/* make_hdr */
	mdi_isdn_rx_parse,		/* rx_parse */

	mdi_isdn_media_wr_prim,		/* media_wr_prim */
	mdi_isdn_media_rd_prim,		/* media_rd_prim */
	mdi_isdn_media_wr_data,		/* media_wr_data */

	mdi_isdn_rx_errors,		/* rx_errors */
	mdi_isdn_tx_errors,		/* tx_errors */
	mdi_isdn_ioctl,			/* media_ioctl */
    };

#ifdef DLPI_DEBUG
    int	Status;

    if (mdi_register_media(MEDIA_MAGIC, MAC_ISDN_BRI, &mi))
	cmn_err(CE_WARN, "xnms_init(): Registration fail: %d\n", Status);
#else
    mdi_register_media(MEDIA_MAGIC, MAC_ISDN_BRI, &mi);
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
mdi_isdn_media_init(per_card_info_t *cp)
{
    DLPI_PRINTF04(("isdn_media_init(cp=0x%x) flags=0x%x private=0x%x\n",
				cp,
				cp->media_specific->media_flags,
				cp->media_private));
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
mdi_isdn_media_halt(per_card_info_t *cp)
{
per_sap_info_t *sp;
mblk_t	       *mp;
int		i;

    DLPI_PRINTF04(("isdn_media_halt(cp=0x%x)\n", cp));

    cp->media_private = NULL;
    sp = cp->sap_table;
    for (i = 0; i < cp->maxsaps; i++, sp++) {
        if ((sp->sap_state == SAP_BOUND) && (sp->sap_type == FR_ISDN)) {
            if ((mp = allocb(1, BPRI_HI)) == NULL) {
                DLPI_PRINTF04(("mdi_isdn_media_halt:  Cannot allocate message"
       		              " block for M_HANGUP message"));
		continue;
	    }
	    mp->b_datap->db_type = M_HANGUP;
            putnext(sp->up_queue, mp);
        }
    }

/*  mdi_isdn_send_daemon_message(cp, "The world has ended"); */
}

/*
 * This routine will be called as a result of a DLPI_MEDIA_REGISTER
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
mdi_isdn_media_svc_reg(per_card_info_t *cp, mblk_t *mp, per_sap_info_t *sp)
{
ulong	*isdn_daemon_id;

    DLPI_PRINTF04(("isdn_media_svc_reg(cp=0x%x, sp=0x%x)\n", cp, sp));

    isdn_daemon_id = (ulong *)mp->b_cont->b_rptr;
    if (*isdn_daemon_id == ISDN_LOGGER_DAEMON) {
        isdn_logger_daemon_registered = TRUE;
	return 0;
    }
    
    return 1;
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
mdi_isdn_media_svc_close(per_card_info_t *cp)
{
    DLPI_PRINTF04(("isdn_media_svc_close(cp=0x%x)\n", cp));

    logging = FALSE;
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
mdi_isdn_media_svc_rcv(queue_t *q, mblk_t *mp)
{
    per_sap_info_t	*sp  = (per_sap_info_t *)q->q_ptr;
    per_card_info_t	*cp = sp->card_info;

    DLPI_PRINTF04(("isdn_media_svc_rcv(mp=0x%x) cp=0x%x\n", mp, cp));

    logging_queue = sp->up_queue;
    logging = TRUE;
    freemsg(mp);
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
mdi_isdn_make_hdr(register per_sap_info_t *sp, mblk_t *mp, unchar *dest,
			unchar *src, int route_len)
{
    DLPI_PRINTF04(("isdn_make_hdr: should not be called!"));
    return 1;    /* DL_UNITDATA_REQ not valid for ISDN */
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
mdi_isdn_rx_parse(per_card_info_t *cp, register struct per_frame_info *fp)
{

    DLPI_PRINTF04(("isdn_rx_parse: should not be called!"));
	fp->frame_type = FR_ISDN;  /* message is freed, won't go upstream */
}

/*
 * Processes an M_DATA message on the write queue.
 */

STATIC mprimret_t
mdi_isdn_media_wr_data(mblk_t *mp, per_card_info_t *cp, per_sap_info_t *sp)
{
isdn_msg_hdr_t	*isdn_hdr;
mblk_t	*hdrmp;
#ifdef DLPI_DEBUG
mblk_t		*logmp;
#endif

    DLPI_PRINTF04(("isdn_media_wr_data: mp=%x cp=%x sp=%x", mp, cp, sp));
	if ((sp->mdata == 1) && (sp->sap_state == SAP_BOUND)
		&& (sp->sap_type = FR_ISDN)) {

		if (sp->blk_data_b3 == 1) /* blocked on DATA_B3 messages */
			return MPRIMRET_PUTBQ;

		if (sp->unconf_data_b3 >= DATA_B3_HIGH_WATER) {
			sp->blk_data_b3 = 1;
	                DLPI_PRINTF04(("DATA_B3 flow control ON appid=%d", sp->sap));
			return MPRIMRET_PUTBQ;
		}

	    /* convert M_DATA to ISDN_DATA_B3.REQ */

		if ((hdrmp = allocb(sizeof(isdn_msg_hdr_t) +
					sizeof(isdnNCCI_t) +
					sizeof(isdnData_t) +
					sizeof(isdnDataLen_t) +
					sizeof(isdnHandle_t) +
					sizeof(isdnFlags_t),
					BPRI_MED)) == NULL) {
	                DLPI_PRINTF04(("mdi_isdn_media_wr_data:"
				  "Cannot allocate message"
       		              " block for ISDN_DATA_B3.REQ message"));
			return MPRIMRET_NOT_MINE;
		}

		hdrmp->b_datap->db_type = M_PROTO;

		isdn_hdr = (isdn_msg_hdr_t *)hdrmp->b_wptr;
		isdn_hdr->DL_prim = DL_ISDN_MSG;
		isdn_hdr->AppID = (ushort)sp->sap;
		isdn_hdr->Cmd = ISDN_DATA_B3;
		isdn_hdr->SubCmd = ISDN_REQ;
		isdn_hdr->MsgNum = EXPECTED_CONF;
		hdrmp->b_wptr += sizeof(isdn_msg_hdr_t);

		*(isdnNCCI_t *)hdrmp->b_wptr = sp->NCCI;
		hdrmp->b_wptr += sizeof(isdnNCCI_t);

		/* data pointer not used on UNIX */
		hdrmp->b_wptr += sizeof(isdnData_t);

		*(isdnDataLen_t *)hdrmp->b_wptr = msgdsize(mp);
		hdrmp->b_wptr += sizeof(isdnDataLen_t);

		/* we'll not use Data Handle */
		hdrmp->b_wptr += sizeof(isdnHandle_t);

		*(isdnFlags_t *)hdrmp->b_wptr = 0;
		hdrmp->b_wptr += sizeof(isdnFlags_t);

		isdn_hdr->Length = hdrmp->b_wptr - hdrmp->b_rptr
						- sizeof(isdnDword_t);

		hdrmp->b_cont = mp;
#ifdef DLPI_DEBUG
		if ((logging) && ((logmp = copymsg(hdrmp)) != NULL)) {
			DLPI_PRINTF04(("logging mdata to datab3_req"));
			putnext(logging_queue, logmp);
		}
#endif
		ATOMIC_INT_ADD(sp->sap_tx.octets, msgdsize(mp));
		ATOMIC_INT_INCR(sp->sap_tx.ucast);

		sp->unconf_data_b3 += 1;

		dlpi_putnext_down_queue(cp, hdrmp); /* Ignore flow control */

/*        	if (cp->init->txmon_enabled) */ /* driver still alive check */
/*                	dlpi_txmon_check(cp); */

		return MPRIMRET_DONE;
	}

	return MPRIMRET_NOT_MINE;
}


/*
 * Processes a media defined primitive on the write queue.
 */

STATIC mprimret_t
mdi_isdn_media_wr_prim(mblk_t *mp, per_card_info_t *cp, per_sap_info_t *sp)
{
isdn_msg_hdr_t	*isdn_hdr;
#ifdef DLPI_DEBUG
mblk_t		*logmp;
#endif

    DLPI_PRINTF04(("isdn_media_wr_prim: mp=%x cp=%x sp=%x", mp, cp, sp));
	if (*(ulong *)mp->b_rptr == DL_ISDN_MSG) {
		isdn_hdr = (isdn_msg_hdr_t *)mp->b_rptr;
		isdn_hdr->AppID = (ushort)sp->sap;
#ifdef DLPI_DEBUG
		if ((logging) && ((logmp = copymsg(mp)) != NULL)) {
			DLPI_PRINTF04(("logging dl_isdn_msg write"));
			putnext(logging_queue, logmp);
		}
#endif
		if ((isdn_hdr->Cmd == ISDN_DATA_B3)
				&& (isdn_hdr->SubCmd == ISDN_REQ)) {
			ATOMIC_INT_ADD(sp->sap_tx.octets, msgdsize(mp->b_cont));
			ATOMIC_INT_INCR(sp->sap_tx.ucast);
		}

		dlpi_putnext_down_queue(cp, mp); /* Ignore flow control */

/*        	if (cp->init->txmon_enabled) */ /* driver still alive check */
/*                	dlpi_txmon_check(cp); */
		return MPRIMRET_DONE;
	}

	return MPRIMRET_NOT_MINE;
}

/*
 * Processes a media defined primitive on the read queue.
 */

STATIC mprimret_t
mdi_isdn_media_rd_prim(queue_t *q, mblk_t *mp, per_card_info_t *cp)
{
per_sap_info_t *sp;
isdn_msg_hdr_t *isdn_hdr;
mblk_t		*upmp, downmp;
isdnByte_t	*msg_p;
unchar		*error;
int		i;
#ifdef DLPI_DEBUG
mblk_t		*logmp;
#endif


	DLPI_PRINTF04(("isdn_media_rd_prim: q=%x mp=%x cp=%x", q, mp, cp));
	isdn_hdr = (isdn_msg_hdr_t *)mp->b_rptr;
	if (isdn_hdr->DL_prim == MAC_ISDN_MSG) {

#ifdef DLPI_DEBUG
		if ((logging) && ((logmp = copymsg(mp)) != NULL)) {
			DLPI_PRINTF04(("logging rd_prim"));
			putnext(logging_queue, logmp);
		}
#endif
		sp = cp->sap_table;
		for (i = 0; i < cp->maxsaps; i++) { /* find SAP */
			if (sp->sap_state == SAP_BOUND && sp->sap_type == FR_ISDN &&
				sp->sap == (ulong)isdn_hdr->AppID)
			break;
			++sp;
		}

		if (i >= cp->maxsaps) {
			DLPI_PRINTF04(("mdi_isdn_media_rd_prim:"
				" Message received for unknown sap"));
			freemsg(mp);
			return MPRIMRET_DONE;
		}

		if ((isdn_hdr->Cmd == ISDN_DATA_B3)
				&& (isdn_hdr->SubCmd == ISDN_IND)) {
			ATOMIC_INT_ADD(sp->sap_rx.octets, msgdsize(mp->b_cont));
			ATOMIC_INT_INCR(sp->sap_rx.ucast);
		}

		if (sp->mdata == 1 ) {

			if (isdn_hdr->Cmd == ISDN_DATA_B3) {
				if (isdn_hdr->SubCmd == ISDN_IND) {
				/* Convert ISDN_DATA_B3.IND to M_DATA,	*/
				/* generate ISDN_DATA_B3.RESP		*/

					isdn_hdr->SubCmd = ISDN_RESP;
					msg_p = (isdnByte_t *)isdn_hdr + sizeof(isdn_msg_hdr_t)
							+ sizeof(isdnNCCI_t)
							+ sizeof(isdnData_t)
							+ sizeof(isdnDataLen_t);
					mp->b_wptr = (isdnByte_t *)isdn_hdr
									+ sizeof(isdn_msg_hdr_t)
									+ sizeof(isdnNCCI_t);
					*(isdnHandle_t *)mp->b_wptr = *(isdnHandle_t *)msg_p;
					mp->b_wptr += sizeof(isdnHandle_t);
					isdn_hdr->Length = mp->b_wptr - mp->b_rptr
											- sizeof(isdnDword_t);

					if (mp->b_cont != NULL) {
						upmp = mp->b_cont;
						mp->b_cont = NULL;

						if (!canput(sp->up_queue))
							return MPRIMRET_PUTBQ;
#ifdef DLPI_DEBUG
						if ((logging) && ((logmp = copymsg(upmp)) != NULL)) {
							DLPI_PRINTF04(("logging datab3_ind.data to mdata"));
							putnext(logging_queue, logmp);
						}
#endif
						putnext(sp->up_queue, upmp);
					}

#ifdef DLPI_DEBUG
	  				if ((logging) && ((logmp = copymsg(mp)) != NULL)) {
						DLPI_PRINTF04(("logging datab3_ind to datab3_resp"));
						putnext(logging_queue, logmp);
					}
#endif
					dlpi_putnext_down_queue(cp, mp); /* Ignore flow control */
					return MPRIMRET_DONE;

				} else if (isdn_hdr->SubCmd == ISDN_CONF) {
					/* drop ISDN_DATA_B3.CONF */

					/* ISDN_DATA_B3 flow control */
					sp->unconf_data_b3 -= 1;
					if ((sp->blk_data_b3 == 1)
							&& (sp->unconf_data_b3 <= DATA_B3_LOW_WATER)) {
						sp->blk_data_b3 = 0;
						qenable(WR(sp->up_queue));
	                			DLPI_PRINTF04(("DATA_B3 flow control OFF appid=%d", sp->sap));
					}

					msg_p = (isdnByte_t *)isdn_hdr + sizeof(isdn_msg_hdr_t)
							+ sizeof(isdnNCCI_t)
							+ sizeof(isdnHandle_t);
					if (((*(isdnInfo_t *)msg_p & BAD_INFO_MASK) == 0)
							|| ((*(isdnInfo_t *)msg_p == BAD_NCCI)
							&& (sp->NCCI == 0))) {
						/* don't do anything if Info is 0	*/
						/* or if Info says illegal NCCI but 	*/
						/* we've already had a disconnect B3	*/
						freemsg(mp);
						return MPRIMRET_DONE;
					} else {
						/* stream M_ERROR because of previous	*/
						/* ISDN_DATA_B3.REQ    			*/
						mp->b_datap->db_type = M_ERROR;
						error = mp->b_rptr;					
						*error = EPROTO;
						mp->b_wptr = mp->b_rptr + 1;
						putnext(sp->up_queue, mp); /* high priority	*/
						/* no flow control	*/	
						ATOMIC_INT_INCR(sp->sap_tx.error);
						DLPI_PRINTF04(("isdn M_ERROR appid=%d", sp->sap));
						return MPRIMRET_DONE;

					}
				}
			} else if ((isdn_hdr->Cmd == ISDN_DISCONNECT_B3) &&
					(isdn_hdr->SubCmd == ISDN_IND)) {

				/* DISCONNECT_B3.IND, issue RESP down, M_HANGUP up */
				isdn_hdr->SubCmd = ISDN_RESP;
				mp->b_wptr = (isdnByte_t *)isdn_hdr + sizeof(isdn_msg_hdr_t)
								+ sizeof(isdnNCCI_t);
				isdn_hdr->Length = mp->b_wptr - mp->b_rptr
									- sizeof(isdnDword_t);
				if ((upmp = allocb(1, BPRI_HI)) != NULL) {
					upmp->b_datap->db_type = M_HANGUP;
					DLPI_PRINTF04(("DISCONNECT_B3.IND, issuing M_HANGUP appid=%d", sp->sap));
					putnext(sp->up_queue, upmp);	/* high priority	*/
													/* no flow control	*/	
				}
				sp->NCCI = 0; /* mark as disconnected */
#ifdef DLPI_DEBUG
	  			if ((logging) && ((logmp = copymsg(mp)) != NULL)) {
					DLPI_PRINTF04(("logging disconnectb3_resp"));
					putnext(logging_queue, logmp);
				}
#endif
				dlpi_putnext_down_queue(cp, mp); /* Ignore flow control */
				return MPRIMRET_DONE;

			} else if ((isdn_hdr->Cmd == ISDN_DISCONNECT) &&
					(isdn_hdr->SubCmd == ISDN_IND)) {

				/* DISCONNECT.IND, issue RESP down	*/						 
				isdn_hdr->SubCmd = ISDN_RESP;
				mp->b_wptr = (isdnByte_t *)isdn_hdr + sizeof(isdn_msg_hdr_t)
								+ sizeof(isdnPLCI_t);
				isdn_hdr->Length = mp->b_wptr - mp->b_rptr
						       		- sizeof(isdnDword_t);
#ifdef DLPI_DEBUG
	  			if ((logging) && ((logmp = copymsg(mp)) != NULL)) {
					DLPI_PRINTF04(("logging disconnect_resp"));
		 			putnext(logging_queue, logmp);
				}
#endif
				dlpi_putnext_down_queue(cp, mp); /* Ignore flow control */
				return MPRIMRET_DONE;
			}

			/* drop message other than DATA_B3, DISCONNECT, or DISCONNECT_B */
			DLPI_PRINTF04(("dropping inbound ISDN message cmd=%x subcmd=%x in M_DATA mode",
								isdn_hdr->Cmd, isdn_hdr->SubCmd));
			freemsg(mp);
			return MPRIMRET_DONE;

		}

   		if (!canput(sp->up_queue))
			return MPRIMRET_PUTBQ;
		putnext(sp->up_queue, mp);
		return MPRIMRET_DONE;
	}

	return MPRIMRET_NOT_MINE;
}

/*
 * Returns the Total number of media dependent transmit errors
 *
 * This routine is optional.  If this routine is not implemented,
 * the errors accumulated into mac_tx.mac_error returned by
 * DL_GET_STATISTICS_REQ will always be zero.
 */

STATIC int
mdi_isdn_tx_errors(void *statp)
{
    register mac_stats_isdn_t	*mp = (mac_stats_isdn_t *)statp;

    DLPI_PRINTF04(("isdn_tx_errors: DOutdiscards=%d DOutErrors=%d\n",
					mp->dchannel.OutDiscards,
					mp->dchannel.OutErrors));

    return	mp->dchannel.OutDiscards +
		mp->dchannel.OutErrors;
}

/*
 * Returns the Total number of media dependent receive errors
 *
 * This routine is optional.  If this routine is not implemented,
 * the errors accumulated into mac_rx.mac_error returned by
 * DL_GET_STATISTICS_REQ will always be zero.
 */

STATIC int
mdi_isdn_rx_errors(void *statp)
{
    register mac_stats_isdn_t	*mp = (mac_stats_isdn_t *)statp;

    DLPI_PRINTF04(("isdn_rx_errors: DInDiscards=%d DInErrors=%d\n",
					mp->dchannel.InDiscards,
					mp->dchannel.InErrors));

    return	mp->dchannel.InDiscards +
		mp->dchannel.InErrors;
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
mdi_isdn_ioctl(mblk_t *mp, per_card_info_t *cp, per_sap_info_t *sp)
{
    struct iocblk	*iocp = (struct iocblk *)mp->b_rptr;

    DLPI_PRINTF04(("mdi_isdn_ioctl: 0x%x\n", iocp->ioc_cmd));

    switch (iocp->ioc_cmd)
    {
	/*
	 * These will be implemented as required to support the
	 * media and is beyond the scope of this simple example.
	 */
	case ISDN_REGISTER:

	    cp->ioctl_sap = sp;
	    cp->ioctl_handler = mdi_isdn_handle_register_ioctl;
	    dlpi_putnext_down_queue(cp, mp);   
	    return MIOCRET_DONE;

	case DLPI_ISDN_MDATA_ON:

            if ((sp->sap_state == SAP_BOUND) && (mp->b_cont != NULL)) {
	        if ((mp->b_cont->b_wptr - mp->b_cont->b_rptr)
		        < sizeof(isdnNCCI_t))
		    mp->b_datap->db_type = M_IOCNAK;
		else {
	    	    sp->NCCI = *(isdnNCCI_t *)mp->b_cont->b_rptr;
			sp->unconf_data_b3 = 0;
			sp->blk_data_b3 = 0;
		    sp->mdata = 1;
			mp->b_datap->db_type = M_IOCACK;
		}
	    }
	    else
	        mp->b_datap->db_type = M_IOCNAK;

	    /* ioctl is either done, or an error occurred:	*/
	    /* let the next ioctl run				*/
	    cp->ioctl_sap = 0;
	    cp->dlpi_iocblk = 0;
	    dlpi_enable_WRqs(cp);
	
	    putnext(sp->up_queue, mp);
	    return MIOCRET_DONE;
						 
	case DLPI_ISDN_MDATA_OFF:
	    
	    if (sp->sap_state == SAP_BOUND) {
		sp->mdata = 0;
		sp->NCCI = 0;
		sp->unconf_data_b3 = 0;
		sp->blk_data_b3 = 0;
		mp->b_datap->db_type = M_IOCACK;
	    } else
		mp->b_datap->db_type = M_IOCNAK;

	    /* ioctl is either done, or an error occurred:	*/
	    /* let the next ioctl run				*/
	    cp->ioctl_sap = 0;
	    cp->dlpi_iocblk = 0;
	    dlpi_enable_WRqs(cp);
	
	    putnext(sp->up_queue, mp);
	    return MIOCRET_DONE;
	
	default:
	    return MIOCRET_NOT_MINE;
    }

}

STATIC void
mdi_isdn_handle_register_ioctl(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t	*cp;
	struct	iocblk	*iocp;
	isdn_register_params_t	*isdn_register_params_p;

	if (sp == (per_sap_info_t *)0) {
		/* race condition: cp->ioctl_sap already cleared by another 
		 * competing close or simply that another ioctl was received 
		 * from mdi driver.
		 */
		if (mp != (mblk_t *)0) {
			/* if mp == 0 then allocb failure in routine
			 * mdi_send_MACIOC_generic.  Don't call dlpi_send_iocnak
			 * as there isn't a valid per_sap_info pointer.  Note we don't 
			 * have a card_info pointer to let next ioctl run
			 */
			freemsg(mp);
		}
		return;
	}
	cp = sp->card_info;

	ASSERT(cp != NULL);
	if (cp == NULL) {
		/* shouldn't happen */
		cmn_err(CE_WARN, "mdi_isdn_handle_release_ioctl: null cp");
		return;
	}
	if (mp == (mblk_t *)0) {
		/* allocb failure in mdi_send_MACIOC_generic, call 
		 * dlpi_send_iocnak if per_card_info is not NULL.
		 */

		dlpi_send_iocnak(sp, EAGAIN);
		return;
	}

	iocp = (struct iocblk *)mp->b_rptr;

	DLPI_PRINTF04(("mdi_isdn_handle_register_ioctl: 0x%x\n", iocp->ioc_cmd));

	switch (mp->b_datap->db_type) {
		case M_IOCACK:

			switch (iocp->ioc_cmd)
				{
				case ISDN_REGISTER:

					if (mp->b_cont == NULL)
						mp->b_datap->db_type = M_IOCNAK;
					else {
						isdn_register_params_p =
							(isdn_register_params_t *)mp->b_cont->b_rptr;
						sp->sap_state = SAP_BOUND;
						sp->sap_type = FR_ISDN;
						sp->mdata = 0;
						sp->NCCI = 0;
						sp->sap = (ulong)isdn_register_params_p->ApplId;
					}
					break;

				default:
					mp->b_datap->db_type = M_IOCNAK;
					break;
				}
			break;

		case M_IOCNAK:
			DLPI_PRINTF04((CE_NOTE,
				"mdi_isdn_handle_register_ioctl: M_IOCNAK ioc_error=%x ioc_rval=%x",
				iocp->ioc_error, iocp->ioc_rval));
			break;

	}
	if (mp->b_datap->db_type == M_IOCNAK) {
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		iocp->ioc_count = 0;
    }

	/* ioctl is either done, or an error occurred: let the next ioctl run */
	cp->ioctl_sap = 0;
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(cp);

	putnext(sp->up_queue, mp);
}


/* handler for ioctls generated within isdn at close time.  
 * needs to use proper locks to eliminate race conditions -- the checks
 * below just reduce the chances of hitting them.
 */
STATIC void
mdi_isdn_handle_release_ioctl(per_sap_info_t *sp, mblk_t *mp)
{
	per_card_info_t *cp; 
	struct iocblk *iocp; 

	if (sp == (per_sap_info_t *)0) {
		/* race condition: cp->ioctl_sap already cleared by another 
		 * competing close or simply that another ioctl was received 
		 * from mdi driver.
		 */
		if (mp != (mblk_t *)0) {
			/* if mp == 0 then allocb failure in routine
			 * mdi_send_MACIOC_generic.  Don't call dlpi_send_iocnak
			 * as user didn't originate the ioctl.  Note we don't 
			 * have a card_info pointer to let next ioctl run
			 */
			freemsg(mp);
		}
		return;
	}
	cp = sp->card_info;

	ASSERT(cp != NULL);
	if (cp == NULL) {
		/* shouldn't happen */
		cmn_err(CE_WARN, "mdi_isdn_handle_release_ioctl: null cp");
		return;
	}
	if (mp == (mblk_t *)0) {
		/* allocb failure in mdi_send_MACIOC_generic, don't call 
		 * dlpi_send_iocnak as user didn't originate the ioctl
		 * sp *shouldn't* be 0 at this point, but since we don't 
		 * use LOCK we can't guarantee this.
		 */
		/* let the next ioctl run */
		cp->ioctl_sap = 0;
		cp->dlpi_iocblk = 0;
		dlpi_enable_WRqs(cp);

		return;
	}

	iocp = (struct iocblk *)mp->b_rptr;

	DLPI_PRINTF04(("mdi_isdn_handle_release_ioctl: 0x%x\n", iocp->ioc_cmd));

	switch (mp->b_datap->db_type) {
		case M_IOCACK:

			freemsg(mp);
			break;

		case M_IOCNAK:

			DLPI_PRINTF04(("ISDN_RELEASE ioctl IOCNAK"));
			freemsg(mp);
			break;
	}

	/* ioctl is either done, or an error occurred: let the next ioctl run */
	cp->ioctl_sap = 0;
	cp->dlpi_iocblk = 0;
	dlpi_enable_WRqs(cp);
}


void
mdi_isdn_close(queue_t *q, per_sap_info_t *sp)
{
    DLPI_PRINTF04(("isdn_close: q=%x sp=%x", q, sp));
	if ((sp->sap_state == SAP_BOUND) && (sp->sap_type = FR_ISDN)
		&& (sp->card_info->down_queue != (queue_t *)0))
		mdi_send_MACIOC_cp_queue(sp,
		                     ISDN_RELEASE, sizeof(ulong),
                                     mdi_isdn_handle_release_ioctl,
											     (unchar *)&(sp->sap));

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
mdi_isdn_send_daemon_message(per_card_info_t *cp, const char *Str)
{
    mblk_t		*mp;
    int			len;
    register queue_t	*up_queue;

    DLPI_PRINTF04(("isdn_send_daemon_message() cp=0x%x q=0x%x %s\n",
					cp, cp->media_svcs_queue, Str));

    if (!cp->media_svcs_queue)
	return;			/* No daemon registered */

    len = strlen(Str) + 1;
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

