#ident	"@(#)ihvkit:net/dlpi_ether_2.x/dlpi_ether.c	1.1"
#ident	"$Header$"

/*      Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc.		*/
/*			All Rights Reserved.				*/
/*									*/
/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.      */
/*      The copyright notice above does not evidence any        	*/
/*      actual or intended publication of such source code.     	*/

/*
 *  This is the common STREAMS interface for DLPI 802.3/ethernet drivers.
 *  It defines all of the service routines and ioctl's.  Board specific
 *  functions will be called to handle of the details of getting packet
 *  to and from the wire.
 */

#ifdef	_KERNEL_HEADERS
/*
 *	building in the kernel tree
 */
#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#ifdef ESMP
#include <util/ksynch.h>
#endif
#include <util/ipl.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <mem/immu.h>
#include <io/stream.h>
#include <io/strstat.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <io/strmdep.h>
#include <io/stropts.h>
#include <io/termio.h>
#include <io/strlog.h>
#include <svc/errno.h>
#ifndef lint
#ifdef ESMP
#include <net/inet/byteorder.h>
#else
#include <net/tcpip/byteorder.h>
#endif
#endif
#ifdef ESMP
#include <net/inet/strioc.h>
#include <net/inet/if.h>
#include <net/socket.h>
#include <net/sockio.h>
#else
#include <net/tcpip/strioc.h>
#include <net/tcpip/if.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#endif
#include <net/dlpi.h>
#include <io/dlpi_ether/dlpi_ether.h>

#ifdef EGL
#include <io/dlpi_ether/dlpi_egl.h>
#endif

#ifdef ESMP

#ifdef IMX586
#include <io/dlpi_ether/imx586/dlpi_imx586.h>
#endif
 
#ifdef I596
#include <io/dlpi_ether/i596/dlpi_i596.h>
#endif

#ifdef EL16
#include <io/dlpi_ether/el16/dlpi_el16.h>
#endif
 
#ifdef EL3
#include <io/dlpi_ether/el3/dlpi_el3.h>
#endif
 
#ifdef WD
#include <io/dlpi_ether/wd/dlpi_wd.h>
#endif
 
#ifdef EE16
#include <io/dlpi_ether/ee16/dlpi_ee16.h>
#endif
 
#ifdef IE6
#include <io/dlpi_ether/ie6/dlpi_ie6.h>
#endif

#ifdef ELT32
#include <io/dlpi_ether/elt32/dlpi_elt32.h>
#endif

#ifdef NE1K
#include <io/dlpi_ether/ne1k/dlpi_ne1k.h>
#endif

#ifdef NE2K
#include <io/dlpi_ether/ne2k/dlpi_ne2k.h>
#endif

#ifdef NE2
#include <io/dlpi_ether/ne2/dlpi_ne2.h>
#endif

#ifdef NE3200
#include <io/dlpi_ether/ne3200/dlpi_ne3200.h>
#endif

#ifdef EN596
#include <io/dlpi_ether/en596/dlpi_en596.h>
#endif

#ifdef FLASH32
#include <io/dlpi_ether/flash32/dlpi_flash32.h>
#endif

#ifdef NFLXE
#include <io/dlpi_cpq/cet/ether/dlpi_nflxe.h>
#endif

#ifdef PNT
#include <io/dlpi_cpq/pnt/dlpi_pnt.h>
#endif

#else		/* ESMP */

#ifdef IMX586
#include <io/dlpi_ether/dlpi_imx586.h>
#endif
 
#ifdef I596
#include <io/dlpi_ether/dlpi_i596.h>
#endif

#ifdef EL16
#include <io/dlpi_ether/dlpi_el16.h>
#endif
 
#ifdef EL3
#include <io/dlpi_ether/dlpi_el3.h>
#endif
 
#ifdef WD
#include <io/dlpi_ether/dlpi_wd.h>
#endif
 
#ifdef EE16
#include <io/dlpi_ether/dlpi_ee16.h>
#endif
 
#ifdef IE6
#include <io/dlpi_ether/dlpi_ie6.h>
#endif

#ifdef ELT32
#include <io/dlpi_ether/dlpi_elt32.h>
#endif

#ifdef NE1K
#include <io/dlpi_ether/dlpi_ne1k.h>
#endif

#ifdef NE2K
#include <io/dlpi_ether/dlpi_ne2k.h>
#endif

#ifdef NE2
#include <io/dlpi_ether/dlpi_ne2.h>
#endif

#ifdef NE3200
#include <io/dlpi_ether/dlpi_ne3200.h>
#endif

#ifdef EN596
#include <io/dlpi_ether/dlpi_en596.h>
#endif

#ifdef FLASH32
#include <io/dlpi_ether/dlpi_flash32.h>
#endif

#ifdef NFLXE
#include <io/dlpi_cpq/cet/ether/dlpi_nflxe.h>
#endif

#ifdef PNT
#include <io/dlpi_cpq/pnt/dlpi_pnt.h>
#endif

#endif		/* ESMP */

/*
 * ddi.h has to be included after those
 * device dependent dlpi_xxx.h files.
 */
#include <io/ddi.h>

#else		/* _KERNEL_HEADERS */

/*
 *	NOT building in the kernel source tree!
 */

#include <sys/types.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#ifdef ESMP
#include <sys/ksynch.h>
#endif
#include <sys/ipl.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/immu.h>
#include <sys/stream.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <sys/strmdep.h>
#include <sys/stropts.h>
#include <sys/termio.h>
#include <sys/strlog.h>
#include <sys/errno.h>
#ifndef lint
#include <sys/byteorder.h>
#endif
#include <net/strioc.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>

#ifdef EGL
#include <sys/dlpi_egl.h>
#endif

#ifdef IMX586
#include <sys/dlpi_imx586.h>
#endif
 
#ifdef I596
#include <sys/dlpi_i596.h>
#endif

#ifdef EL16
#include <sys/dlpi_el16.h>
#endif
 
#ifdef EL3
#include <sys/dlpi_el3.h>
#endif
 
#ifdef WD
#include <sys/dlpi_wd.h>
#endif
 
#ifdef EE16
#include <sys/dlpi_ee16.h>
#endif
 
#ifdef IE6
#include <sys/dlpi_ie6.h>
#endif

#ifdef ELT32
#include <sys/dlpi_elt32.h>
#endif

#ifdef NE1K
#include <sys/dlpi_ne1k.h>
#endif

#ifdef NE2K
#include <sys/dlpi_ne2k.h>
#endif

#ifdef NE2
#include <sys/dlpi_ne2.h>
#endif

#ifdef NE3200
#include <sys/dlpi_ne3200.h>
#endif

#ifdef EN596
#include <sys/dlpi_en596.h>
#endif

#ifdef FLASH32
#include <sys/dlpi_flash32.h>
#endif

#ifdef NFLXE
#include <sys/dlpi_nflxe.h>
#endif

#ifdef PNT
#include <sys/dlpi_pnt.h>
#endif

/*
 * ddi.h has to be included after those
 * device dependent dlpi_xxx.h files.
 */
#include <sys/ddi.h>

#endif		/* _KERNEL_HEADERS */



	void	DLprint_eaddr( uchar_t * );

STATIC  void	DLcmds( queue_t *, mblk_t * ), 
		DLinfo_req( queue_t * ), 
		DLbind_req( queue_t *, mblk_t * ), 
		DLunbind_req( queue_t *, mblk_t * ),
		DLunitdata_req( queue_t *, mblk_t * ), 
		DLioctl( queue_t *, mblk_t * ), 
		DLerror_ack( queue_t *, mblk_t *, int ), 
		DLuderror_ind( queue_t *, mblk_t *, int ),
		DLtest_req( queue_t *, mblk_t * ),
		DLsubsbind_req( queue_t *, mblk_t * ),
		DLremove_sap( DL_sap_t *, DL_bdconfig_t * ),
		DLinsert_sap( DL_sap_t *, DL_bdconfig_t * );

STATIC  mblk_t	*DLmk_ud_ind( mblk_t *, DL_sap_t * ),
		*DLmk_test_con( DL_eaddr_t *, DL_eaddr_t *, ushort_t, ushort_t, ushort_t ), 
		*DLproc_llc( queue_t *, mblk_t * ), 
		*DLform_snap( DL_sap_t *sap, mblk_t *, uchar_t * ),
		*DLform_80223( uchar_t *, uchar_t *, int, ushort_t, ushort_t, int, ushort_t);

STATIC	uchar_t *copy_local_addr( DL_sap_t *, ushort_t * ), 
		*copy_broad_addr( ushort_t * );

STATIC	int	DLis_us( DL_bdconfig_t *, DL_eaddr_t * ), 
		DLis_equalsnap( DL_sap_t *, DL_sap_t * ), 
		DLopen( queue_t *, dev_t *, int, int, struct cred * ),
		DLclose( queue_t * ),
		DLwput( queue_t *, mblk_t * ), 
		DLrsrv( queue_t * ), 
		DLis_broadcast( DL_eaddr_t * );

extern	int	DLxmit_packet( DL_bdconfig_t *, mblk_t *, DL_sap_t * ), 
		DLpromisc_off( DL_bdconfig_t * ), 
		DLpromisc_on( DL_bdconfig_t * ),
		DLset_eaddr( DL_bdconfig_t *, DL_eaddr_t * ), 
		DLrecv( mblk_t *, DL_sap_t *),
		DLadd_multicast( DL_bdconfig_t *, DL_eaddr_t * ),
		DLdel_multicast( DL_bdconfig_t *, DL_eaddr_t * ),
		DLdisable( DL_bdconfig_t * ), 
		DLenable( DL_bdconfig_t * ), 
		DLreset( DL_bdconfig_t * ), 
		DLis_multicast( DL_bdconfig_t *, DL_eaddr_t * ),
		DLget_multicast( DL_bdconfig_t *, mblk_t * ),
		DLis_validsnap( DL_mac_hdr_t *, DL_sap_t * ) ;

extern	void	DLbdspecioctl( queue_t *, mblk_t * ), 
		DLbdspecopen( queue_t * ),
		DLbdspecclose( queue_t * );

	struct	module_info	DLrminfo = {
	DL_ID, DL_NAME, DL_MIN_PACKET, DL_MAX_PACKET, DL_HIWATER, DL_LOWATER };

	struct	module_info	DLwminfo = {
	DL_ID, DL_NAME, DL_MIN_PACKET, DL_MAX_PACKET, DL_HIWATER, DL_LOWATER };

STATIC  struct	qinit		DLrinit = {
	NULL, DLrsrv, DLopen, DLclose, NULL, &DLrminfo, NULL };

STATIC  struct	qinit		DLwinit = {
	DLwput, NULL, NULL, NULL, NULL, &DLwminfo, NULL };

struct	streamtab DLinfo = { &DLrinit, &DLwinit, NULL, NULL, };

#if defined(EE16) || defined(EL16) || defined(PNT) || defined(EN596) || defined(FLASH32)
/*
 *  ee16 is autoconfig enabled.  It allocates its own structs instead
 *  of using statically declared arrays from space.c
 */
extern	DL_bdconfig_t	*DLconfig;
extern	DL_sap_t	*DLsaps;
#else
extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t	DLsaps[];
#endif
extern	int		DLboards;
extern	int		DLstrlog;
extern	char		DLid_string[];
extern	struct	ifstats	*DLifstats;

#ifdef ESMP
extern	int DLdevflag;
#else
int DLdevflag = 0;
#endif

#define BCOPY(from, to, len) bcopy((caddr_t)(from), (caddr_t)(to), (size_t)(len))

/*
 * int
 * DLopen( queue_t *q, dev_t *dev, int flag, int sflag, struct cred *credp )
 *  create a sap to the device.
 * 
 * Calling/Exit State:
 *      No locking assumptions.
 *	return errno or 0, if successful.
 *	*dev will have the new dev_t.
 */
/* ARGSUSED */
int
DLopen( queue_t *q, dev_t *dev, int flag, int sflag, struct cred *credp )
{
	int		i ;
	DL_bdconfig_t	*bd;
	DL_sap_t	*sap;
	major_t	major = getmajor(*dev);
	minor_t	minor = getminor(*dev);
	int opri ;

		DL_LOG(strlog(DL_ID, 0, 3, SL_TRACE,
			"DLopen - major %d minor %d queue %x",
				(int)major, (int)minor, (int)q));

	/*
	 *  Find the board structure for this major number.
	 */
	for (i = DLboards, bd = DLconfig; i; bd++, i--)
		if (bd->major == major)
			break;
	
	if (i == 0) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
		    "DLopen - invalid major number (%d)", (int)major));
		return (ENXIO);
	}

	/*
	 *  Check if we found a board there.
	 */
	if (!(bd->flags & BOARD_PRESENT)) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
		    "DLopen - board for major (%d) not installed", (int)major));
		return (ENXIO);
	}

	opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;

	/*
	 * check if this stream is opend already
	 */
	if ( q->q_ptr ) {
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		return ( 0 ) ;
	}

	/*
	 *  If it's a clone device, assign a minor number.
	 */
	if (sflag == CLONEOPEN) {
		for (i = 0, sap = bd->sap_ptr; i < bd->max_saps; i++, sap++)
			if (sap->write_q == NULL)
				break;

		if (i == bd->max_saps) {
			DLPI_UNLOCK ( bd->bd_lock, opri ) ;
			DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
			    "DLopen - no minors left"));
			return (ECHRNG);
		}
		else
			minor = (minor_t) i;

	} else if (minor > bd->max_saps) {
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
		    "DLopen - invalid minor number (%d)", (int)minor));
		return (ECHRNG);
	}

	/*
	 *  If this stream has not been opened before, set it up.
	 */
	if (!q->q_ptr) {
		sap->state   = DL_UNBOUND;
		sap->read_q  = q;
		sap->write_q = WR(q);

		q->q_ptr = (caddr_t) sap;
		WR(q)->q_ptr = q->q_ptr;

		/*
		 *  Need to keep track of privilege for later reference in 
		 *  bind requests.
		 */
		if (drv_priv(credp) == 0)
			sap->flags |= PRIVILEDGED;
#ifdef DLbdspecopen
		DLbdspecopen(q);
#endif
#ifdef ESMP
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		if ( DLdevflag == D_MP )
			qprocson( q ) ;
		*dev = makedevice(major, minor);
		return (0);
#endif
	}

	DLPI_UNLOCK ( bd->bd_lock, opri ) ;
	*dev = makedevice(major, minor);

	return (0);
}


/*
 * int
 * DLclose( queue_t *q )
 *  Close the sap to the device, will wait for outstanding packet to be sent.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
int
DLclose( queue_t *q )
{
	DL_sap_t *sap = (DL_sap_t *)q->q_ptr;
	int opri ;

#ifdef ESMP
	qprocsoff( q );
#endif
	DL_LOG(strlog(DL_ID, 1, 3, SL_TRACE, "DLclose queue %x", (int)q));

	/*
	 *  If this was a promiscuous SAP, have board put out of promiscuous
	 *  mode.  If there are still promiscuous SAPs running, the call may
	 *  have no effect.
	 */
	if (sap->flags & PROMISCUOUS)
		(void)DLpromisc_off(sap->bd);

	/*
	 *  Cleanup SAP structure
	 */
	opri = DLPI_LOCK ( sap->bd->bd_lock, plstr ) ;

#ifdef ESMP
	while ( sap->flags & SEND_PENDING ) {
		sap->flags |= CLS_PENDING ;
		SV_WAIT ( sap->sap_sv, prilo, sap->bd->bd_lock ) ;
		opri = DLPI_LOCK ( sap->bd->bd_lock, plstr ) ;
	}
#endif

	sap->state    = DL_UNBOUND;
	sap->sap_addr = 0;
	sap->read_q   = NULL;
	sap->write_q  = NULL;
	sap->flags    = 0;
	DLremove_sap(sap,sap->bd);
	sap->next_sap = NULL;

	DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;

	/* handle any board-specific close processing */
#ifdef DLbdspecclose
	DLbdspecclose(q);
#endif

	return ( 0 );
}


/*
 * int
 * DLwput( queue_t *q, mblk_t *mp )
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 *
 */
STATIC int
DLwput( queue_t *q, mblk_t *mp )
{
	DL_LOG(strlog(DL_ID, 2, 3, SL_TRACE,
	    "DLwput queue %x type %d", (int)q, mp->b_datap->db_type));

	switch(mp->b_datap->db_type) {
		/*
		 *  Process data link commands.
		 */
		case M_PROTO:
		case M_PCPROTO:
			DLcmds(q, mp);
			break;
		/*
		 *  Flush read and write queues.
		 */
		case M_FLUSH:
			if (*mp->b_rptr & FLUSHW)
				flushq(q, FLUSHDATA);

			if (*mp->b_rptr & FLUSHR) {
				flushq(RD(q), FLUSHDATA);
				*mp->b_rptr &= ~FLUSHW;
				qreply(q, mp);
			} else
				freemsg(mp);
			break;
		/*
		 *  Process ioctls.
		 */
		case M_IOCTL:
			DLioctl(q, mp);
			break;
		/* 
		 *  Anything else we dump.
		 */
		default:
			DL_LOG(strlog(DL_ID, 2, 1, SL_TRACE,
			    "DLwput - unsupported type (%d)",
			    mp->b_datap->db_type));
			freemsg(mp);
			break;
	}

	return ( 0 );
}

/*
 * STATIC void
 * DLcmds( queue_t *q, mblk_t *mp )
 *  execute DLPI cmds.
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 */
STATIC void
DLcmds( queue_t	*q, mblk_t *mp )
{
	/* LINTED pointer assignment */
	union DL_primitives  *dl = (union DL_primitives *)mp->b_datap->db_base;

	switch (dl->dl_primitive) {
		case DL_UNITDATA_REQ:
			DLunitdata_req(q, mp);
			return;			/* don't free the message */

		case DL_INFO_REQ:
			DLinfo_req(q);
			break;
		
		case DL_BIND_REQ:
			DLbind_req(q, mp);
			break;
		
		case DL_UNBIND_REQ:
			DLunbind_req(q, mp);
			break;

		case DL_TEST_REQ:
			DLtest_req(q,mp);
			return;

		case DL_SUBS_BIND_REQ:
			DLsubsbind_req(q,mp);
			return;

		case DL_ATTACH_REQ:
		case DL_DETACH_REQ:
		case DL_UDQOS_REQ:
		case DL_CONNECT_REQ:
		case DL_CONNECT_RES:
		case DL_TOKEN_REQ:
		case DL_DISCONNECT_REQ:
		case DL_RESET_REQ:
		case DL_RESET_RES:
		case DL_ENABMULTI_REQ:
		case DL_DISABMULTI_REQ:
		case DL_PROMISCON_REQ:
		case DL_PROMISCOFF_REQ:
		case DL_XID_REQ:
		case DL_XID_RES:
		case DL_TEST_RES:
		case DL_PHYS_ADDR_REQ:
		case DL_SET_PHYS_ADDR_REQ:
		case DL_GET_STATISTICS_REQ:
		case DL_DATA_ACK_REQ:
		case DL_REPLY_REQ:
		case DL_REPLY_UPDATE_REQ:
			DLerror_ack(q, mp, DL_NOTSUPPORTED);
			break;
		
		default:
			DLerror_ack(q, mp, DL_BADPRIM);
			break;
	}

	/*
	 *  Free the request.
	 */
	freemsg(mp);
}

/*
 * mblk_t *
 * DLform_snap( DL_sap_t *sap, mblk_t *mp, uchar_t *dst_addr )
 *
 *  change mp to a snap header.  
 *  return the pointer to the new mblk_t.
 * 
 * Calling/Exit State:
 *      No locking assumptions.
 */
mblk_t *
DLform_snap( DL_sap_t *sap, mblk_t *mp, uchar_t *src_addr )
{
	/* LINTED pointer alignment */
	dl_unitdata_req_t *data_req = (dl_unitdata_req_t *)mp->b_rptr;
	struct llcb	*llcbp;
	mblk_t		*nmp;
	DL_mac_hdr_t	*hdr;
	caddr_t		dst_addr;

	if ( (nmp = allocb(sizeof(DL_mac_hdr_t),BPRI_MED)) == NULL)
		return NULL;

	nmp->b_cont = NULL;

	/* LINTED pointer alignment */
	hdr = (DL_mac_hdr_t *)nmp->b_rptr;
	BCOPY(src_addr, hdr->src.bytes, LLC_ADDR_LEN);
	hdr->mac_llc.snap.snap_length = htons(msgdsize(mp) + LLC_HDR_SAP_SIZE + SNAP_SAP_SIZE);
	hdr->mac_llc.snap.snap_ssap = SNAPSAP;
	hdr->mac_llc.snap.snap_control = LLC_UI;

        if (data_req->dl_dest_addr_length == DL_LLC_ADDR_LEN) {
		dst_addr = (caddr_t)data_req + data_req->dl_dest_addr_offset;
		BCOPY(dst_addr, hdr->dst.bytes, LLC_ADDR_LEN);
		hdr->mac_llc.snap.snap_dsap = SNAPSAP;

		/*
                  The bytes have already been swapped and stored during
                  the DLsubsbind_req processing. In effect, the org field is
                  maintained in the network order to minimize the byte
                  swapping during data req/ind processing.
		*/

                BCOPY(&sap->snap_global, hdr->mac_llc.snap.snap_org, SNAP_GLOBAL_SIZE);
		hdr->mac_llc.snap.snap_type = htons(sap->snap_local);
	}
	else {
                llcbp = (struct llcb *)((caddr_t)data_req + data_req->dl_dest_addr_offset);
           	BCOPY(llcbp->lbf_addr, hdr->dst.bytes, LLC_ADDR_LEN);
        	hdr->mac_llc.snap.snap_dsap = (uchar_t)llcbp->lbf_sap;
		BCOPY(&llcbp->lbf_xsap, hdr->mac_llc.snap.snap_org, SNAP_GLOBAL_SIZE);
		hdr->mac_llc.snap.snap_type = htons((ushort_t)llcbp->lbf_type);
	}
	nmp->b_wptr = nmp->b_rptr + SNAP_HDR_SIZE;
	return nmp;
}

/*
 * mblk_t *
 * DLform_80223( uchar_t *dst_addr, uchar_t *src_addr, 
 * 	int size, ushort_t dsap, ushort_t ssap, int rawsap, ushort_t control )
 * 
 *  change the header of the message to be 802.2 format.
 *  return the pointer to the new message.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
mblk_t *
DLform_80223( uchar_t *dst_addr, uchar_t *src_addr, int size,
		ushort_t dsap, ushort_t ssap, int rawsap, ushort_t control )
{
	mblk_t *nmp;
	DL_mac_hdr_t *hdr;

        if ( (nmp = allocb(sizeof(DL_mac_hdr_t),BPRI_MED)) == NULL)
                return NULL;

	nmp->b_cont = NULL;

	/* LINTED pointer alignment */
        hdr = (struct DL_mac_hdr *)nmp->b_rptr;
        BCOPY(dst_addr, hdr->dst.bytes, LLC_ADDR_LEN);
        BCOPY(src_addr, hdr->src.bytes, LLC_ADDR_LEN);

	if (rawsap) {
		hdr->mac_llc.llc.llc_length = htons(size);
		nmp->b_wptr = nmp->b_rptr + ETH_HDR_SIZE;
	}
	else {
		hdr->mac_llc.llc.llc_length = htons(size + LLC_HDR_SAP_SIZE);
		hdr->mac_llc.llc.llc_dsap = (char)dsap;
		hdr->mac_llc.llc.llc_ssap = (char)ssap;
		hdr->mac_llc.llc.llc_control = (char)control;
		nmp->b_wptr = nmp->b_rptr + LLC_HDR_SIZE;
	}
	return nmp;
}

/*
 * STATIC void
 * DLtest_req( queue_t *q, mblk_t *mp )
 * 
 *  performing test function.  
 *
 * Calling/Exit State:
 *      Can't hold locks on entry.
 */
STATIC void
DLtest_req(queue_t *q, mblk_t *mp)
{
	/* LINTED pointer alignment */
	dl_test_req_t	*test_req = (dl_test_req_t *)mp->b_rptr;
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t	*bd = sap->bd;
	mblk_t		*nmp, *tmp;
	struct llcc	*llccp;
	DL_eaddr_t	*ether_addr;
	int		size,local,multicast;
	ushort_t	dsap,ssap,control;
	int opri ;


	opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;

	if (sap->state != DL_IDLE) {
		DLPI_UNLOCK ( bd->bd_lock, opri );
		DLerror_ack(q,mp,DL_OUTSTATE);
		freemsg(mp);
		return;
	}

	if ((sap->mac_type != DL_CSMACD) || (sap->flags & (SNAPCSMACD | RAWCSMACD)) ){
		DLPI_UNLOCK ( bd->bd_lock, opri );
		DLerror_ack(q,mp,DL_UNDELIVERABLE);
		return;
	}

	DLPI_UNLOCK ( bd->bd_lock, opri );

	size = msgdsize(mp);
	if ((size > sap->max_spdu)) {
		DLerror_ack(q, mp, DL_BADDATA);
		return;
	}

	/*
	 * validate user-supplied address. the offset to the destination
	 * mac address must be beyond the dl_data_req_t, but still within
	 * the message block.
	 */
	if ((test_req->dl_dest_addr_offset < sizeof(dl_test_req_t)) ||
		(test_req->dl_dest_addr_offset + test_req->dl_dest_addr_length
		> mp->b_wptr - mp->b_rptr)) {
		DLerror_ack(q, mp, DL_BADADDR);
		return;
	}

	/* LINTED pointer alignment */
	ether_addr = (DL_eaddr_t*)(mp->b_rptr + test_req->dl_dest_addr_offset);
        local	  = DLis_us(bd, ether_addr);
        multicast = DLis_broadcast(ether_addr) || DLis_multicast(bd,ether_addr);

	/* LINTED pointer alignment */
        llccp	   = (struct llcc *)((caddr_t)test_req + test_req->dl_dest_addr_offset);

        if (test_req->dl_flag & DL_POLL_FINAL)
		control = LLC_TEST |LLC_P;
	else
		control = LLC_TEST;

	dsap = llccp->lbf_sap;
	ssap = sap->sap_addr;

	/* 
	 * At this time we support only end to end TEST pdus. Any test command 
	 * PDU destined locally will be error acked. However, as the protocol 
	 * dictates, all incoming test command PDUs will be responded to by 
	 * the provider. Also incoming TEST pdu responses will be handed over 
	 * to the appropriate sap. Similarly TEST Command PDUs bound for 
	 * Multicast or Broadcast address will only be sent out but not looped 
	 * back.
	 */

	if (local) {
		DLerror_ack(q,mp,DL_UNSUPPORTED);
		freemsg(mp);
		return;
	}

	if ((nmp = DLform_80223(llccp->lbf_addr, bd->eaddr.bytes, size, dsap, ssap, 0, control)) == NULL) {
		DLerror_ack(q,mp,DL_SYSERR);
		freemsg(mp);
		return;
	}

	tmp = rmvb(mp,mp);
	freeb(mp);
	linkb(nmp,tmp);

        size += LLC_HDR_SAP_SIZE;

	if (size < sap->min_spdu) {
		if ((mp = allocb(sap->min_spdu, BPRI_MED)) == NULL) {
			DLuderror_ind(q, mp, DL_UNDELIVERABLE);
			return;
		}
		mp->b_wptr = mp->b_rptr + (sap->min_spdu - size);
		linkb(nmp,mp);
	}

	opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;

        if (bd->flags & TX_BUSY) {
                bd->flags |= TX_QUEUED;
                bd->mib.ifOutQlen++;            /* SNMP */
                putq(q, nmp);
                DL_LOG(strlog(DL_ID, 6, 2, SL_TRACE,
                	"DLtestdata xmit busy - deferred for queue %x",(int)q));
	}
	else {
		if ( DLxmit_packet ( bd, nmp, sap ) == 1 ) {
			/*
			 *	xmit was full -- queue this one up 
			 */
                	bd->flags |= (TX_QUEUED | TX_BUSY);
                	bd->mib.ifOutQlen++;            /* SNMP */
                	putq(q, nmp);
		}
	}

	DLPI_UNLOCK ( bd->bd_lock, opri ) ;
}

/*
 * STATIC void
 * DLinfo_req( queue_t *q )
 *  get information about a sap. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 */
STATIC void
DLinfo_req( queue_t *q )
{
	dl_info_ack_t	*info_ack;
	mblk_t		*resp;
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	
	DL_LOG(strlog(DL_ID, 3, 3, SL_TRACE,
	    "DLinfo request for queue %x", (int)q));

	/*
	 *  If we can't get the memory, just ignore the request.
	 */
			/* VERSION 2 DLPI SUPPORT */

	if ((resp = allocb(DL_PRIMITIVES_SIZE + DL_ETH_ADDR_LEN + LLC_ADDR_LEN, BPRI_MED)) == NULL)
		return;

	/* LINTED pointer assignment */
	info_ack			= (dl_info_ack_t *)resp->b_wptr;
	info_ack->dl_primitive		= DL_INFO_ACK;
	info_ack->dl_max_sdu		= sap->max_spdu;
	info_ack->dl_min_sdu		= sap->min_spdu;
	if ( sap->mac_type == DL_ETHER )
		info_ack->dl_addr_length	= DL_ETH_ADDR_LEN;
	else
		info_ack->dl_addr_length	= DL_LLC_ADDR_LEN;
	info_ack->dl_mac_type		= sap->mac_type;
	info_ack->dl_current_state	= sap->state;
	info_ack->dl_service_mode	= sap->service_mode;
	info_ack->dl_qos_length		= 0;
	info_ack->dl_qos_offset		= 0;
	info_ack->dl_qos_range_length	= 0;
	info_ack->dl_qos_range_offset	= 0;
	info_ack->dl_provider_style	= sap->provider_style;
	info_ack->dl_addr_offset	= DL_INFO_ACK_SIZE;
	info_ack->dl_growth		= 0;
	info_ack->dl_brdcst_addr_length = LLC_ADDR_LEN;
	/* VERSION 2 DLPI SUPPORT */

	info_ack->dl_brdcst_addr_offset = DL_ETH_ADDR_LEN + DL_INFO_ACK_SIZE;
	/* VERSION 2 DLPI SUPPORT */

	resp->b_wptr += DL_INFO_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/*
	 *  DLPI spec says if stream is not bound, address will be 0.
	 */
	if (sap->state == DL_IDLE) {
		/* LINTED pointer assignment */
		resp->b_wptr = copy_local_addr(sap, (ushort_t*)resp->b_wptr);
	} else {
		info_ack->dl_addr_length = 0;
		info_ack->dl_addr_offset = 0;
		info_ack->dl_brdcst_addr_offset = DL_INFO_ACK_SIZE;
		/* VERSION 2 DLPI SUPPORT */
	}
	/* LINTED pointer alignment */
	resp->b_wptr = copy_broad_addr((ushort_t*)resp->b_wptr);
	qreply(q, resp);
}

/*
 * STATIC void
 * DLbind_req( queue_t *q, mblk_t *mp )
 *  bind a sap address to a sap.
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 */
STATIC void
DLbind_req( queue_t *q, mblk_t *mp )
{
	/* LINTED pointer assignment */
	dl_bind_req_t	*bind_req  = (dl_bind_req_t *)mp->b_datap->db_base;
	DL_sap_t	*sap       = (DL_sap_t *)q->q_ptr;
	DL_sap_t	*tmp;
	dl_bind_ack_t	*bind_ack;
	mblk_t		*resp;
	int		 i;
	ushort_t	 dlsap;
	int opri ;

	DL_LOG(strlog(DL_ID, 4, 3, SL_TRACE,
		"DLbind request to sap 0x%x on queue %x",
			((int)bind_req->dl_sap) & 0xffff, (int)q));

	opri = DLPI_LOCK ( sap->bd->bd_lock, plstr ) ;

	/*
	 *  If the stream is already bound, return an error.
	 */
	if (sap->state != DL_UNBOUND) {
		DL_LOG(strlog(DL_ID, 4, 1, SL_TRACE,
		    "DLbind not valid for state %d on queue %x",
		    	sap->state, (int)q));
		DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;
		DLerror_ack(q, mp, DL_OUTSTATE);
		return;
	}

	/*
	 *  Only a prevledged user can bind to the promiscuous SAP or a SAP
	 *  that is already bound.
	 */
	if (!(sap->flags & PRIVILEDGED)) {
		if ((ushort_t)bind_req->dl_sap == PROMISCUOUS_SAP) {
			DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;
			DLerror_ack(q, mp, DL_ACCESS);
			return;
		}

		tmp = sap->bd->sap_ptr;
		i   = sap->bd->max_saps;
		while (i--) {
			if ((tmp->state    == DL_IDLE) &&
			    (tmp->sap_addr == (ushort_t)bind_req->dl_sap)) {
				DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;
				DLerror_ack(q, mp, DL_BOUND);
				return;
			}
			tmp++;
		}

	}

	/*
	 *  Assign the SAP and set state.
	 */
	dlsap = (ushort_t)bind_req->dl_sap;
	if (dlsap <= MAXSAPVALUE) {
		/* Do not allow binds to null saps or group saps */
		if ( (!dlsap) || (dlsap & 0x1) || (dlsap == LLC_GLOBAL_SAP) ) {
			DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;
			DLerror_ack(q, mp, DL_BADADDR);
			return;
		}
		sap->mac_type = DL_CSMACD;
		sap->max_spdu = DL_MAX_PACKET_LLC;
	} else {
		sap->mac_type = DL_ETHER;
		sap->max_spdu = DL_MAX_PACKET;
	}
	sap->sap_addr = dlsap;  /* Stored in host order */
	sap->state    = DL_IDLE;
	sap->next_sap = NULL;
	DLinsert_sap(sap,sap->bd);

	DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;

	if ((resp = allocb(DL_PRIMITIVES_SIZE + DL_ETH_ADDR_LEN, BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 4, 1, SL_TRACE,
		    "DLbind allocb failure on queue %x", (int)q));
		return;
	}

	/* LINTED pointer assignment */
	bind_ack		 = (dl_bind_ack_t *)resp->b_wptr;
	bind_ack->dl_primitive   = DL_BIND_ACK;
	bind_ack->dl_sap	 = sap->sap_addr;
	if ( sap->mac_type == DL_ETHER )
		bind_ack->dl_addr_length = DL_ETH_ADDR_LEN;
	else
		bind_ack->dl_addr_length = DL_LLC_ADDR_LEN;
	bind_ack->dl_addr_offset = DL_BIND_ACK_SIZE;
	bind_ack->dl_max_conind  = 0;
	bind_ack->dl_xidtest_flg = 0;	/* VERSION 2 DLPI SUPPORT */

	resp->b_wptr          += DL_BIND_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/* LINTED pointer assignment */
	resp->b_wptr = copy_local_addr(sap, (ushort_t*)resp->b_wptr);
	qreply(q, resp);
}

/*
 * STATIC void
 * DLsubsbind_req( queue_t *q, mblk_t *mp )
 *  bind subsequent sap address.
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 */
STATIC void
DLsubsbind_req( queue_t *q, mblk_t *mp )
{
	/* LINTED pointer alignment */
	dl_subs_bind_req_t *subs_bind = (dl_subs_bind_req_t *)mp->b_rptr;
	dl_subs_bind_ack_t *subs_bind_ack;
	DL_sap_t	*sap	= q->q_ptr;
	DL_sap_t	*tsap;
	DL_bdconfig_t	*bd	= sap->bd;
	struct snap_sap *subs_sap;
	uchar_t		*ptmp;
	ulong_t		snap_global = 0;
	ulong_t		snap_tmp;
	ushort_t	snap_local = 0;
	int i;
	int opri ;



	opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;

	if (sap->state != DL_IDLE) {
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		DLerror_ack(q, mp, DL_OUTSTATE);
		freemsg(mp);
		return;
	}

	if ( (sap->sap_addr != SNAPSAP) ||
	     (subs_bind->dl_subs_sap_length != sizeof(struct snap_sap))) {
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		DLerror_ack(q, mp, DL_BADADDR);
		freemsg(mp);
		return;
	}

	/* LINTED pointer alignment */
	subs_sap = (struct snap_sap *)((caddr_t)subs_bind + subs_bind->dl_subs_sap_offset);
	sap->state = DL_SUBS_BIND_PND;

        /*
	 * It is probably worth it to do some extra work here so that we can
	 * avoid the byte swapping when unitdata reqs are sent out
	 */
	snap_tmp = htonl(subs_sap->snap_global);
	ptmp = (uchar_t *)&snap_tmp;
	ptmp++;

	BCOPY(ptmp, &snap_global, SNAP_GLOBAL_SIZE);
	snap_local = ((ushort_t)subs_sap->snap_local);

        for (i = 0, tsap = bd->sap_ptr; i < bd->max_saps; i++,tsap++) {
		if ((tsap->state == DL_IDLE) &&
		    (tsap->sap_addr == SNAPSAP) &&
		    (tsap->snap_global == snap_global) &&
		    (tsap->snap_local == snap_local)) {
			DLPI_UNLOCK ( bd->bd_lock, opri ) ;
			DLerror_ack(q, mp, DL_BOUND);
			freemsg(mp);
			return;
		}
	}
	sap->snap_global = snap_global;
	sap->snap_local = snap_local;
	sap->state = DL_IDLE;
	sap->flags |= SNAPCSMACD;
	sap->max_spdu = DL_MAX_PACKET_SNAP;

	/* LINTED pointer alignment */
	subs_bind_ack = (dl_subs_bind_ack_t *)mp->b_rptr;
	subs_bind_ack->dl_primitive = DL_SUBS_BIND_ACK;
	subs_bind_ack->dl_subs_sap_offset = DL_SUBS_BIND_ACK_SIZE;
	subs_bind_ack->dl_subs_sap_length = SNAP_SAP_SIZE;

	ptmp = (uchar_t *)((caddr_t)subs_bind_ack + subs_bind_ack->dl_subs_sap_offset);
	BCOPY(&sap->snap_global, ptmp, SNAP_GLOBAL_SIZE);
	ptmp += SNAP_GLOBAL_SIZE;
	BCOPY(&sap->snap_local, ptmp, SNAP_LOCAL_SIZE);
	mp->b_wptr = mp->b_rptr + DL_SUBS_BIND_ACK_SIZE + SNAP_SAP_SIZE;
	DLPI_UNLOCK ( bd->bd_lock, opri ) ;
	qreply(q,mp);
	return;
}

/*
 * STATIC void
 * DLunbind_req( queue_t *q, mblk_t *mp )
 *  unbind sap address. 
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 */
STATIC void
DLunbind_req( queue_t *q, mblk_t	*mp )
{
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	mblk_t		*resp;
	dl_ok_ack_t	*ok_ack;
	int opri ;

	DL_LOG(strlog(DL_ID, 5, 3, SL_TRACE,
	    "DLunbind request for sap 0x%x on queue %x",
	    sap->sap_addr, (int)q));

	/*
	 *  See if we are in the proper state to honor this request.
	 */
	opri = DLPI_LOCK ( sap->bd->bd_lock, plstr ) ;
	if (sap->state != DL_IDLE) {
		DL_LOG(strlog(DL_ID, 5, 1, SL_TRACE,
		    "DLunbind not valid for state %d on queue %x",
		    sap->state, (int)q));
		DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;
		DLerror_ack(q, mp, DL_OUTSTATE);
		return;
	}
		
	/* 
	 *  Allocate memory for response.  If none, toss the request.
	 */
	if ((resp = allocb(DL_PRIMITIVES_SIZE, BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 5, 1, SL_TRACE,
		    "DLunbind allocb failure on queue %x", (int)q));
		DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;
		return;
	}

	/*
	 *  Mark the SAP out of service and flush both queues
	 */

	sap->state    = DL_UNBOUND;
	sap->sap_addr = 0;
	sap->snap_global = sap->snap_local = 0;
	DLremove_sap(sap,sap->bd);
	sap->next_sap = NULL;

	DLPI_UNLOCK ( sap->bd->bd_lock, opri ) ;

	flushq(q, FLUSHDATA);
	flushq(RD(q), FLUSHDATA);

	/*
	 *  Generate and send the response.
	 */
	/* LINTED pointer assignment */
	ok_ack                       = (dl_ok_ack_t *)resp->b_wptr;
	ok_ack->dl_primitive         = DL_OK_ACK;
	ok_ack->dl_correct_primitive = DL_UNBIND_REQ;

	resp->b_wptr          += DL_OK_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;
	qreply(q, resp);

	return;
}

/*
 * STATIC void
 * DLunitdata_req( queue_t *q, mblk_t *mp )
 *  send packet to the device.
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 *
 */
STATIC void
DLunitdata_req( queue_t	*q, mblk_t *mp )
{
	/* LINTED pointer assignment */
	dl_unitdata_req_t *data_req = (dl_unitdata_req_t *)mp->b_rptr;

	/* LINTED pointer assignment */
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t	*bd = sap->bd;
	DL_eaddr_t	*ether_addr;
	DL_mac_hdr_t	*hdr;
	mblk_t		*nmp,*tmp,*xmp;
	struct llcc	*llccp;
	int		 size = msgdsize(mp->b_cont);
	int		 local, multicast;
	int opri;


	DL_LOG(strlog(DL_ID, 6, 3, SL_TRACE,
	    "DLunitdata request of %d bytes for sap 0x%x on queue %x",
	    	size, sap->sap_addr, (int)q));

	opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;

	/*
	 *  If the board has gone down, reject the request.
	 */
	if((bd->flags & (BOARD_PRESENT | BOARD_DISABLED)) != BOARD_PRESENT) {
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
		  "DLunitdata request on disabled board for queue %x", (int)q));
		DLerror_ack(q, mp, DL_NOTINIT);
		freemsg(mp);
		return;
	}

	/*
	 *  Check for proper state and frame size.
	 */
	if (sap->state != DL_IDLE) {
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
		    "DLunitdata not valid for state %d on queue %x",
		    	sap->state, (int)q));
		DLuderror_ind(q, mp, DL_OUTSTATE);
		return;
	}

	DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		
	if (size > sap->max_spdu ) {
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
		    "DLunitdata frame size of %d is invalid for queue %x",
		    	size, (int)q));
		DLuderror_ind(q, mp, DL_BADDATA);
		return;
	}

	/*
	 * validate user-supplied address. the offset to the destination
	 * mac address must be beyond the dl_data_req_t, but still within
	 * the message block.
	 */
	if ((data_req->dl_dest_addr_offset < sizeof(dl_unitdata_req_t)) ||
		(data_req->dl_dest_addr_offset + data_req->dl_dest_addr_length
		> mp->b_wptr - mp->b_rptr)) {
		DLuderror_ind(q, mp, DL_BADADDR);
		return;
	}


	/*
	 *  Check for frame that we should send to ourself.
	 */
	/* LINTED pointer assignment */
	ether_addr = (DL_eaddr_t*)(mp->b_rptr + data_req->dl_dest_addr_offset);
	local	  = DLis_us(bd, ether_addr);
	multicast = DLis_broadcast(ether_addr) || DLis_multicast(bd,ether_addr);

	/*
	 *  Update SNMP stats
	 */
	if (ether_addr->bytes[0] & 0x01)
		sap->bd->mib.ifOutNUcastPkts++;		/* SNMP */
	else
		sap->bd->mib.ifOutUcastPkts++;		/* SNMP */

	switch(sap->mac_type) {
	case DL_CSMACD:
		if (sap->sap_addr == SNAPSAP) {
			if ( (!(sap->flags & SNAPCSMACD)) || 
			     (!(nmp = DLform_snap(sap,mp,bd->eaddr.bytes)))){
				DLuderror_ind(q, mp, DL_UNDELIVERABLE);
				return;
			}
			size += (LLC_HDR_SAP_SIZE + SNAP_SAP_SIZE);

		}
		else {
			llccp = (struct llcc *)((caddr_t)data_req + data_req->dl_dest_addr_offset);
			if ((nmp = DLform_80223(llccp->lbf_addr,
						bd->eaddr.bytes,
						size, 
						(ushort_t)llccp->lbf_sap,
						sap->sap_addr,
			    			(sap->flags & RAWCSMACD),
						(ushort_t)LLC_UI)) == NULL) {
				DLuderror_ind(q, mp, DL_UNDELIVERABLE);
				return;
			}
		        /* Add the LLC_HDR_SAP_SIZE only for non RAW SAPs */
                        if (!(sap->flags & RAWCSMACD))
                                size += LLC_HDR_SAP_SIZE;
		}
		break;

	case DL_ETHER:
		if ((nmp = allocb(sizeof(DL_mac_hdr_t), BPRI_MED)) == NULL) {
			DLuderror_ind(q, mp, DL_UNDELIVERABLE);
			return;
		}
		nmp->b_cont = NULL;

		/* LINTED pointer alignment */
		hdr = (struct DL_mac_hdr *)nmp->b_rptr;
		BCOPY(ether_addr->bytes, hdr->dst.bytes, LLC_ADDR_LEN);
		BCOPY(bd->eaddr.bytes, hdr->src.bytes, LLC_ADDR_LEN);
		hdr->mac_llc.ether.len_type = htons(sap->sap_addr);
		nmp->b_wptr = nmp->b_rptr + ETH_HDR_SIZE;
		break;

	default:
		freemsg(mp);
		return;
	}

	/* remove the M_PROTO msg block from the message */
	tmp = rmvb(mp,mp);
	freeb(mp);

	/* form a new message, which consists of packet header block and
	 * packet data block */
	linkb(nmp,tmp);

	if (size < sap->min_spdu) {
		if ((mp = allocb(sap->min_spdu, BPRI_MED)) == NULL) {
			DLuderror_ind(q, mp, DL_UNDELIVERABLE);
			return;
		}
		mp->b_wptr = mp->b_rptr + (sap->min_spdu - size);
		linkb(nmp,mp);
	}

	if (local || multicast) {
#ifdef ESMP
		if (((xmp = msgpullup(nmp, -1)) == NULL) ||
						DLrecv(xmp,bd->sap_ptr))
#else
		if ((pullupmsg(nmp, -1) == NULL) ||
				((xmp = dupmsg(nmp))== NULL) ||
						DLrecv(xmp,bd->sap_ptr))
#endif
			bd->mib.ifOutDiscards++;

		if (local & (!(sap->flags & SEND_LOCAL_TO_NET)) ) {
			freemsg(nmp);
			return;
		}
	}

	opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;

	/*
	 *  If controller is not busy, transmit the frame.  Otherwise put
	 *  it on our queue for the tx interrupt routine to handle.
	 */
	if (bd->flags & TX_BUSY) {
		bd->flags |= TX_QUEUED;
		putq(q, nmp);
		bd->mib.ifOutQlen++;		/* SNMP */
		DL_LOG(strlog(DL_ID, 6, 2, SL_TRACE,
		    "DLunitdata hardware busy - xmit deferred for queue %x",
		    	(int)q));
	}
	else {
		if ( DLxmit_packet ( bd, nmp, sap ) == 1 ) {
			/*
			 *	xmit was full -- queue this one up 
			 */
                	bd->flags |= (TX_QUEUED | TX_BUSY);
                	bd->mib.ifOutQlen++;            /* SNMP */
                	putq(q, nmp);
		}
	}

	DLPI_UNLOCK ( bd->bd_lock, opri ) ;
	return;
}

/*
 * STATIC void
 * DLioctl( queue_t *q, mblk_t	*mp )
 *  handle ioctl functions.
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 *
 */
STATIC void
DLioctl( queue_t *q, mblk_t *mp )
{
	/* LINTED pointer assignment */
	struct iocblk	*ioctl_req = (struct iocblk *)mp->b_rptr;
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t	*bd = sap->bd;
	int		failed, size1, size2;
	short		count = 0;
	int opri ;


	DL_LOG(strlog(DL_ID, 8, 3, SL_TRACE,
	    "DLioctl request for command %d on queue %x",
	    	ioctl_req->ioc_cmd, (int)q));

	 if (ioctl_req->ioc_count == TRANSPARENT) {
			ioctl_req->ioc_error = EPERM;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
	}

	/*
	 *  Assume good stuff for now.
	 */
	ioctl_req->ioc_error = 0;
	mp->b_datap->db_type = M_IOCACK;

	/*
	 *  Screen for privileged requests.
	 */
	switch (ioctl_req->ioc_cmd) {

	case DLIOCSMIB:		/* Set MIB */
	case DLIOCSENADDR:	/* Set ethernet address */
	case DLIOCSLPCFLG:	/* Set local packet copy flag */
	case DLIOCSPROMISC:	/* Toggle promiscuous state */
	case DLIOCADDMULTI:	/* Add multicast address */
	case DLIOCDELMULTI:	/* Delete multicast address */
	case DLIOCGETMULTI:	/* Get list of multicast addresses */
	case DLIOCDISABLE:	/* Disable controller */
	case DLIOCENABLE:	/* Enable controller */
	case DLIOCRESET:	/* Reset controller */
	case DLIOCCSMACDMODE:	/* Toggle CSMACD modes */
		/*
		 *  Must be privledged user to do these.
		 */
		if (drv_priv(ioctl_req->ioc_cr)) {
			DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			    "DLioctl invalid privilege for queue %x", (int)q));

			ioctl_req->ioc_error = EPERM;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		}
	}

	/*
	 *  Make sure IOCTL's that require buffers have them.
	 */
	switch (ioctl_req->ioc_cmd) {

	case DLIOCGMIB:		/* Get MIB */
	case DLIOCSMIB:		/* Set MIB */
	case DLIOCGENADDR:	/* Get ethernet address */
	case DLIOCSENADDR:	/* Set ethernet address */
	case DLIOCADDMULTI:	/* Add multicast address */
	case DLIOCDELMULTI:	/* Delete multicast address */
	case DLIOCGETMULTI:	/* Get list of multicast address */
		/*
		 * Must have a non-null b_cont pointer.
		 */
		if (!mp->b_cont) {
			DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			    "DLioctl no data supplied by user for queue %x",
			    	(int)q));
			ioctl_req->ioc_error = EINVAL;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		}
	}

	/*
	 *  Process request.
	 */
	switch (ioctl_req->ioc_cmd) {
	case DLIOCCSMACDMODE:	/* Toggle CSMACD modes */
		opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;
		if ( (sap->state != DL_IDLE) || (sap->mac_type != DL_CSMACD) ||
		    				(sap->sap_addr == SNAPSAP) ) {
			ioctl_req->ioc_error = EINVAL;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			DLremove_sap(sap,bd);
			flushq(sap->read_q, FLUSHDATA);
			flushq(sap->write_q, FLUSHDATA);
			if (sap->flags & RAWCSMACD)
				sap->flags &= ~RAWCSMACD;
			else
				sap->flags |= RAWCSMACD;
			DLinsert_sap(sap,bd);
		}
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		break;

	case DLIOCGETMULTI: 	/* Get list of multicast addresses	*/
		if (ioctl_req->ioc_count % 6) {
			ioctl_req->ioc_error = EINVAL;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		} else
			ioctl_req->ioc_rval = DLget_multicast(bd,mp->b_cont);
		break;

	case DLIOCGMIB:		/* Get MIB */
		/*
		 *  We'll send as much as they asked for.
		 */
		size1 = min(sizeof(DL_mib_t), ioctl_req->ioc_count);
		size2 = min(strlen(DLid_string)+1, ioctl_req->ioc_count-size1);

		/*
		 *  Set some MIB items before copy
		 */
		bd->mib.ifDescrLen = size2;
		/* LINTED pointer assignment */
		(void)copy_local_addr(sap, (ushort_t*)&bd->mib.ifPhyAddress);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr;

		BCOPY(&bd->mib, mp->b_cont->b_wptr, size1);

		mp->b_cont->b_wptr += size1;

		if (size2) {
			strncpy((char*)mp->b_cont->b_wptr, DLid_string, size2);
			*(char*)(mp->b_cont->b_wptr + size2 - 1) = '\0';
			mp->b_cont->b_wptr += size2;
		}

		ioctl_req->ioc_count = size1 + size2;

		break;

	case DLIOCSMIB:		/* Set MIB */
		/*
		 *  We currently don't let them set the "ifDecr".
		 */
		BCOPY(mp->b_cont->b_rptr, &bd->mib, min(sizeof(DL_mib_t), ioctl_req->ioc_count));
		ioctl_req->ioc_count = 0;

		if (!(bd->flags & BOARD_PRESENT))
			bd->mib.ifOperStatus = DL_DOWN;

		break;

	case DLIOCGENADDR:	/* Get ethernet address */
		/*
		 *  We'll send as much as they asked for.
		 */
		size1 = min(sizeof(DL_eaddr_t), ioctl_req->ioc_count);
		BCOPY(&bd->eaddr, mp->b_cont->b_rptr, size1);
		ioctl_req->ioc_count = size1;

		break;

	case DLIOCSENADDR:	/* Set ethernet address */
 
#ifdef ALLOW_SET_EADDR
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
			DLset_eaddr(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
#else
		ioctl_req->ioc_error = EINVAL;
		mp->b_datap->db_type = M_IOCNAK;
#endif

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCGLPCFLG:	/* Get local packet copy flag */
		ioctl_req->ioc_rval  = (bd->flags & SEND_LOCAL_TO_NET);
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCSLPCFLG:	/* Set local packet copy flag */
		bd->flags |= SEND_LOCAL_TO_NET;
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCGPROMISC:	/* Get promiscuous state */
		ioctl_req->ioc_rval  = (sap->flags & PROMISCUOUS);
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCSPROMISC:	/* Toggle promiscuous state */
		if (sap->flags & PROMISCUOUS)
			failed = DLpromisc_off(bd);
		else
			failed = DLpromisc_on(bd);

		if (failed) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		} else
			sap->flags ^= PROMISCUOUS;

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCADDMULTI:	/* Add multicast address */
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
			DLadd_multicast(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCDELMULTI:	/* Delete multicast address */
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
			DLdel_multicast(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCDISABLE:	/* Disable controller */
		if (DLdisable(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		else
			bd->flags |= BOARD_DISABLED;
		break;

	case DLIOCENABLE:	/* Enable controller */
		if (DLenable(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		else
			bd->flags &= ~BOARD_DISABLED;
		break;

	case DLIOCRESET:	/* Reset controller */
		if (DLreset(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		break;

	case SIOCGIFFLAGS:	/* IP get ifnet flags */
		break;		/* just ignore or */

	case SIOCSIFNAME:
		ioctl_req->ioc_error = EINVAL;
		mp->b_datap->db_type = M_IOCNAK;
		break;

	default:
		/* Give the ioctl one more chance to be recognized */
		DLbdspecioctl(q, mp);
		return;
	}

	qreply(q, mp);
}

/*
 *
 * STATIC int
 * DLrsrv( queue_t *q )
 *   send packets upstream when possible.
 *   avoid race condition with close routine.
 * 
 * Calling/Exit State:
 *  can't hold locks on entry.
 *
 */
STATIC int
DLrsrv( queue_t	*q )
{
	DL_sap_t	*sap;
	DL_sap_t	*nsap;
	DL_bdconfig_t	*bd;
	mblk_t		*mp_data, *mp_ind, *mp_dup;
	DL_mac_hdr_t	*hdr;
	int		i;
	ushort_t	sap_id ;
	int	opri ;

	/*
	 *  Process all messages waiting.
	 */

	sap = (DL_sap_t *)q->q_ptr;
	bd  = sap->bd;
	while (mp_data = getq(q)) {

		/* LINTED pointer assignment */
		hdr = (DL_mac_hdr_t*)mp_data->b_rptr;
		switch (sap->mac_type) {
		case DL_ETHER:
			/*
		 	*  Convert the SAP to host order
		 	*/
			sap_id = ntohs(hdr->mac_llc.ether.len_type);

			/*
		 	*  Create an indication message for this frame.
		 	*/
			if ((mp_ind = DLmk_ud_ind(mp_data,sap)) == NULL) {
				freemsg(mp_data);
				cmn_err(CE_WARN,"DLrsrv: No receive resources");
				bd->mib.ifInDiscards++;		/* SNMP */
				continue;
			}
			break;

		case DL_CSMACD:
			sap_id = LLC_DSAP(hdr);
			/*
		 	 *  Create an indication message for this frame.
		 	 */
			if ((mp_ind = DLproc_llc(q,mp_data)) == NULL) {
				freemsg(mp_data);
				cmn_err(CE_WARN,"DLrsrv: No receive resources");
				bd->mib.ifInDiscards++;		/* SNMP */
				continue;
			}
			else if (mp_ind == (mblk_t *)1) {
				freemsg(mp_data);
				continue;
			}
			break;

		default:
			freemsg(mp_data);
			continue;
		}
		/*
		 *  Go through the SAP list and see if anyone else is
		 *  interested in this frame.
		 */

		linkb(mp_ind, mp_data);

		opri = DLPI_LOCK( bd->bd_lock, plstr ) ;

		for (nsap = bd->valid_sap; nsap; nsap = nsap->next_sap) {
			/*
			 *  Skip SAP indication this came in on.
			 */
			if (nsap == sap) {
#ifdef ESMP
				nsap->flags |= SEND_PENDING;
#endif
				continue;
			}

			else if (nsap->sap_addr == PROMISCUOUS_SAP) {
#ifdef ESMP
				nsap->flags |= SEND_PENDING;
#else
                                CHK_FLOWCTRL_DUP_PUT(nsap, mp_ind, mp_dup);
#endif
			}
			else if ((sap->sap_addr == PROMISCUOUS_SAP) || (sap->flags & RAWCSMACD))
				continue;

			else if (nsap->mac_type != sap->mac_type)
				continue;

			else if (nsap->mac_type == DL_ETHER) {
				if (nsap->sap_addr == sap_id) {
#ifdef ESMP
					nsap->flags |= SEND_PENDING;
#else
					CHK_FLOWCTRL_DUP_PUT(nsap, mp_ind, mp_dup);
#endif
				}
			}

			else if (nsap->mac_type == DL_CSMACD) {
			    if (!(nsap->flags & RAWCSMACD) &&
				((nsap->sap_addr == sap_id) || (sap_id  == LLC_GLOBAL_SAP))) {

				if ((sap_id == SNAPSAP) && (!DLis_equalsnap(sap, nsap)))
					continue;
#ifdef ESMP
				nsap->flags |= SEND_PENDING;
#else
				CHK_FLOWCTRL_DUP_PUT(nsap, mp_ind, mp_dup);
#endif
			    }
			}
		}

		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
#ifdef ESMP
		nsap = bd->valid_sap;

		while( nsap ) {
			if (nsap != sap && nsap->flags & SEND_PENDING) {
				if ( canputnext(nsap->read_q) &&
						(mp_dup = dupmsg(mp_ind)))
                        		putnext( nsap->read_q, mp_dup ) ;

				nsap->flags &= ~SEND_PENDING;
			}
			nsap = nsap->next_sap;
		}
#endif
		/*
		 *  Don't forget the one who brougt us.
		 */
#ifdef ESMP
		if (canputnext(sap->read_q)) {
#else
		if (canput(sap->read_q->q_next)) {
#endif
			DL_LOG(strlog(DL_ID, 9, 3, SL_TRACE, "DLrsrv queue %x",
				(int)sap->read_q));
			/*
			 *  Update SNMP stats.
			 */
			if (IS_MULTICAST(hdr->dst))
				bd->mib.ifInNUcastPkts++;
			else
				bd->mib.ifInUcastPkts++;

			putnext(sap->read_q, mp_ind);

		} else {
			freemsg(mp_ind);
			bd->mib.ifInDiscards++;			/* SNMP */
		}
#ifdef ESMP
		sap->flags &= ~SEND_PENDING;

		opri = DLPI_LOCK( bd->bd_lock, plstr );
		nsap = bd->valid_sap;
		while( nsap ) {
			if(nsap->flags & CLS_PENDING) {
				nsap->flags &= ~CLS_PENDING;
				SV_SIGNAL ( nsap->sap_sv, 0 );
			}
			nsap = nsap->next_sap;
		}
		DLPI_UNLOCK ( bd->bd_lock, opri );
#endif
	}
	return ( 1 );
}

/*
 * STATIC mblk_t *
 * DLmk_test_con( DL_eaddr_t *dst, DL_eaddr_t *src, ushort_t dsap_id, 
 * 	ushort_t ssap_id, ushort_t control )
 *  create test confirmation message. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
STATIC mblk_t *
DLmk_test_con( DL_eaddr_t *dst, DL_eaddr_t *src, ushort_t dsap_id, 
	ushort_t ssap_id, ushort_t control )
{
	dl_test_con_t *test_con;
	mblk_t *mp;
	struct llcc *llccp;


	if ((mp = allocb((DL_TEST_CON_SIZE + (sizeof(DL_eaddr_t)  * 2) +
	    (sizeof(ushort_t) * 2)), BPRI_MED)) == NULL)
		return (NULL);
	mp->b_datap->db_type  = M_PROTO;

	/* LINTED pointer alignment */
	test_con = (dl_test_con_t *)mp->b_rptr;
	test_con->dl_primitive = DL_TEST_CON;
	if (control & LLC_P)
		test_con->dl_flag = DL_POLL_FINAL;
	test_con->dl_dest_addr_offset = DL_TEST_CON_SIZE;
	test_con->dl_dest_addr_length = DL_LLC_ADDR_LEN;
	test_con->dl_src_addr_length  = DL_LLC_ADDR_LEN;
	test_con->dl_src_addr_offset  = DL_TEST_CON_SIZE + DL_LLC_ADDR_LEN;
	llccp = (struct llcc *)( (caddr_t)test_con + DL_UNITDATA_IND_SIZE);
	BCOPY(dst, llccp->lbf_addr, LLC_ADDR_LEN);
	llccp->lbf_sap = (unchar)dsap_id;
	llccp++;
	BCOPY(src, llccp->lbf_addr, LLC_ADDR_LEN);
	llccp->lbf_sap = (unchar)ssap_id;
	mp->b_wptr = mp->b_rptr + (DL_TEST_CON_SIZE + 2 * DL_LLC_ADDR_LEN);
	return mp;
}

/*
 * STATIC void
 * DLerror_ack( queue_t *q, mblk_t *mp, int error )
 *  send DL_ERROR_ACK message upstream.
 *
 * Calling/Exit State:
 *  can't hold locks.
 *
 */
STATIC	void
DLerror_ack( queue_t *q, mblk_t *mp, int error )
{
	/* LINTED pointer assignment */
	union	DL_primitives	*prim = (union DL_primitives*)mp->b_rptr;
	dl_error_ack_t		*err_ack;
	mblk_t			*resp;

	/*
	 *  Allocate the response resource.  If we can't, just return.
	 */
	if ((resp = allocb(sizeof(union DL_primitives), BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 10, 1, SL_TRACE,
		    "DLerror_ack - no resources for error response on queue %x",
		    (int)q));
		return;
	}

	/*
	 *  Fill it in.
	 */
	/* LINTED pointer assignment */
	err_ack                     = (dl_error_ack_t*)resp->b_wptr;
	err_ack->dl_primitive       = DL_ERROR_ACK;
	err_ack->dl_error_primitive = prim->dl_primitive;
	err_ack->dl_errno           = (ulong)error;
	err_ack->dl_unix_errno      = 0;

	resp->b_wptr += DL_ERROR_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/*
	 *  Send it
	 */
	qreply(q, resp);
	return;
}

/*
 * STATIC	void
 * DLuderror_ind( queue_t *q, mblk_t *mp, int error )
 *  send DL_UDERROR_IND message up stream. 
 *
 * Calling/Exit State:
 *  can't hold locks on entry.
 *
 */
STATIC	void
DLuderror_ind( queue_t *q, mblk_t *mp, int error )
{
	/* LINTED pointer assignment */
	dl_uderror_ind_t   *uderr_ind = (dl_uderror_ind_t*)mp->b_rptr;

	/*
	 *  The unit data request is guaranteed to accomodate a unit data
	 *  error indication so we will just convert the data request.
	 */
	uderr_ind->dl_primitive = DL_UDERROR_IND;
	uderr_ind->dl_unix_errno = 0;		/* VERSION 2 DLPI SUPPORT */
	uderr_ind->dl_errno = (ulong)error;

	qreply(q, mp);
	return;
}

/*
 * int
 * DLis_us( DL_bdconfig_t *bd, DL_eaddr_t *dst )
 * 
 *  return 1 if the dst points to the local address, return 0 otherwise.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 */
int
DLis_us( DL_bdconfig_t *bd, DL_eaddr_t *dst )
{
	return bcmp((char *)bd->eaddr.bytes,(char *)dst->bytes,
		(size_t)LLC_ADDR_LEN) == 0;
}

/*
 * int
 * DLis_broadcast( DL_eaddr_t *dst )
 *  return 1, if dst points to a broadcast address. return 0 otherwise. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
int
DLis_broadcast( DL_eaddr_t *dst )
{
	int     i;

	for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
		if (dst->words[ i ] != 0xffff)
			return (0);
	return (1);
}

/*
 * STATIC char
 * ntoa( uchar_t val )
 *  change val to ascii.
 *  
 * Calling/Exit State:
 *      No locking assumptions.
 */
static char
ntoa( uchar_t val )
{
	if (val < 10)
		return (val + 0x30);
	else
		return (val + 0x57);
}

/*
 * void
 * DLprint_eaddr( uchar_t	addr[] )
 *  print ethernet address.
 * 
 * Calling/Exit State:
 *      No locking assumptions.
 */
void
DLprint_eaddr( uchar_t	addr[] )
{
	int	i;
	char	a_eaddr[ LLC_ADDR_LEN * 3 ];

	for (i = 0; i < LLC_ADDR_LEN; i++) {
		a_eaddr[ i * 3     ] = ntoa((uchar_t)(addr[ i ] >> 4 & 0x0f));
		a_eaddr[ i * 3 + 1 ] = ntoa((uchar_t)(addr[ i ]      & 0x0f));
		a_eaddr[ i * 3 + 2 ] = ':';
	}

	a_eaddr[ sizeof(a_eaddr) - 1 ] = '\0';
	cmn_err(CE_CONT, "Ethernet Address: %s\n", a_eaddr);
}

/*
 * int
 * STATIC uchar_t*
 * copy_local_addr( DL_sap_t *sap, ushort_t *dst )
 * 
 *  copy local physical address and sap address.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
static uchar_t*
copy_local_addr( DL_sap_t *sap, ushort_t *dst )
{
	ushort_t	*src = sap->bd->eaddr.words;
	int		i;

	for (i = 0; i < (sizeof (DL_eaddr_t)) / 2; i++)
		*dst++ = *src++;
	*dst++ = sap->sap_addr;

	return((uchar_t*)dst);
}

/*
 * STATIC uchar_t *
 * copy_broad_addr( ushort_t *dst )
 *  copy broadcast address. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
static uchar_t *
copy_broad_addr( ushort_t *dst )
{
	int     i;

	for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
		*dst++ = 0xffff;
	return((uchar_t*)dst);
}


/*
 * mblk_t *
 * DLproc_llc( queue_t *q, mblk_t *mp_data )
 * 
 * ( desc )
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * lock / param / return / side effect
 *
 * Description :
 */
mblk_t *
DLproc_llc( queue_t *q, mblk_t *mp_data )
{
	/* LINTED pointer alignment */
	DL_mac_hdr_t	*hdr = (DL_mac_hdr_t *)mp_data->b_rptr;
	DL_mac_hdr_t	*newhdr;
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t	*bd = sap->bd;
	mblk_t		*data_ind = NULL,*nmp,*xmp = NULL;
	ushort_t	dsap,ssap,control,actlen;
	int opri ;


	dsap = LLC_DSAP(hdr);
	ssap = LLC_SSAP(hdr);
	control = LLC_CONTROL(hdr);

	if (sap->flags & RAWCSMACD) {
		if ( (data_ind = DLmk_ud_ind(mp_data,sap)) )
			return data_ind;
		else
			goto llc_discard;
	}

	if (ssap & LLC_RESPONSE) {
		/* 
		 * Discard non test responses and test responses that
		 * cannot be translated into test confirmations.
		 */
		if ( (control == LLC_TEST) || (control == (LLC_TEST|LLC_P)) ) {
#ifdef ESMP
			if (canputnext(sap->read_q)) {
#else
			if (canput(sap->read_q->q_next)) {
#endif
				if ( (xmp = copyb(mp_data)) && (data_ind = DLmk_test_con(&hdr->dst,&hdr->src, dsap,ssap, sap->mac_type)) ) {
                                        newhdr = (DL_mac_hdr_t *)xmp->b_rptr;
                                        actlen = LLC_LENGTH(newhdr);
					xmp->b_rptr += LLC_HDR_SIZE;
                                        xmp->b_wptr = xmp->b_rptr +
                                                (actlen - LLC_HDR_SAP_SIZE);
					linkb(data_ind,xmp);
					putnext(sap->read_q,data_ind);
					return ((mblk_t *)1);
				} else {
					if (xmp)
						freemsg(xmp);
					if (data_ind)
						freemsg(data_ind);
				}
			}
		}
		goto llc_discard;

	} else {
		switch(control) {
		case LLC_UI:
			/* 
			 * Discard  UI messages that cannot be translated into
			 * data indications; Return a valid data indication.
			 */
			if ( (data_ind = DLmk_ud_ind(mp_data,sap)) == NULL)
				goto llc_discard;
			return data_ind;

		case LLC_TEST:
		case LLC_TEST |LLC_P:
			if ( (xmp = copymsg(mp_data)) == NULL) {
				goto llc_discard;
			}
			xmp->b_rptr += LLC_HDR_SIZE;
			if ((nmp = DLform_80223(hdr->src.bytes,
						bd->eaddr.bytes,
						msgdsize(xmp),
						ssap,
						(sap->sap_addr |LLC_RESPONSE),
			    			0, control))== NULL) {
				freemsg(xmp);
				goto llc_discard;
			}
			nmp->b_cont = xmp;
			break;

		case LLC_XID:		/* Need to respond to a XID command */
		case LLC_XID|LLC_P:
			if ((nmp = allocb(sizeof(DL_mac_hdr_t), BPRI_MED)) == NULL)
				goto llc_discard;

			/* LINTED pointer alignment */
			newhdr = (DL_mac_hdr_t *)nmp->b_rptr;
			BCOPY(hdr->src.bytes, newhdr->dst.bytes, LLC_ADDR_LEN);
			BCOPY(bd->eaddr.bytes, newhdr->src.bytes, LLC_ADDR_LEN);
			newhdr->mac_llc.llc.llc_dsap = ( unchar )ssap;
			newhdr->mac_llc.llc.llc_ssap=sap->sap_addr|LLC_RESPONSE;
			newhdr->mac_llc.llc.llc_control = ( unchar )control;
			newhdr->mac_llc.llc.llc_info[0] = LLC_XID_FMTID;
			newhdr->mac_llc.llc.llc_info[1] = LLC_SERVICES;
			newhdr->mac_llc.llc.llc_info[2] = 0;
			newhdr->mac_llc.llc.llc_length = htons(LLC_XID_INFO_SIZE+ LLC_HDR_SAP_SIZE);
			nmp->b_wptr = nmp->b_rptr + LLC_XID_INFO_SIZE + LLC_HDR_SIZE;
			/* A valid XID response can now be transmitted */
			break;

		default:
			goto llc_discard;
		}

		opri = DLPI_LOCK ( bd->bd_lock, plstr ) ;
		if (bd->flags & TX_BUSY) {
			bd->flags |= TX_QUEUED;
			bd->mib.ifOutQlen++;            /* SNMP */
			putq(sap->write_q, nmp);
			DL_LOG(strlog(DL_ID, 6, 2, SL_TRACE,
			    	"DLunitdata xmit busy - deferred for queue %x",
					(int)q));
		}
		else {
			if ( DLxmit_packet ( bd, nmp, sap ) == 1 ) {
				/*
				 *	xmit was full -- queue this one up 
				 */
                		bd->flags |= (TX_QUEUED | TX_BUSY);
                		bd->mib.ifOutQlen++;            /* SNMP */
                		putq(q, nmp);
			}
		}
		DLPI_UNLOCK ( bd->bd_lock, opri ) ;
		return NULL;
	}

llc_discard:
	bd->mib.ifInDiscards++;	/* SNMP */
	return NULL;
}

/*
 * void
 * DLremove_sap( DL_sap_t *sap, DL_bdconfig_t *bd )
 *  remove a sap from the valid_sap list. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
void
DLremove_sap( DL_sap_t *sap, DL_bdconfig_t *bd )
{
	DL_sap_t *t1,*t2;

	for(t1= bd->valid_sap,t2 = t1;t1 && t1 != sap;t2 = t1,t1= t1->next_sap)
		;
	if (!t1)
		return;
	if (t1 == t2)
		bd->valid_sap = t1->next_sap;
	else
		t2->next_sap = t1->next_sap;
	sap->next_sap = NULL;

	bd->ttl_valid_sap = 0;
	t1 = bd->valid_sap;
	while( t1 ) {
		bd->ttl_valid_sap++;
		t1 = t1->next_sap;
	}
}

/*
 * void
 * DLinsert_sap( DL_sap_t *sap, DL_bdconfig_t *bd )
 *  insert a sap into the valid_sap list.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
void
DLinsert_sap( DL_sap_t *sap, DL_bdconfig_t *bd )
{
	DL_sap_t *t1;
	DL_sap_t *t2;

	if (!bd->valid_sap) {
		bd->valid_sap = sap;
		sap->next_sap = NULL;
	} else {
		if (sap->sap_addr == PROMISCUOUS_SAP) {
			t1 = bd->valid_sap;
			while(t1->next_sap)
				t1 = t1->next_sap;
			t1->next_sap = sap;
			sap->next_sap = NULL;
		}
		else if (sap->flags & RAWCSMACD) {
			t1 = t2 = bd->valid_sap;
			while (t1) {
				if ( (t1->sap_addr == PROMISCUOUS_SAP) ||
				    (t1->flags & RAWCSMACD) )
					break;
				t2 = t1;
				t1 = t1->next_sap;
			}
			if (!t1) {
				t2->next_sap  = sap;
				sap->next_sap = NULL;
			}
			else if (t2 == t1) {
				sap->next_sap = t2;
				bd->valid_sap = sap;
			}
			else {
				t2->next_sap = sap;
				sap->next_sap = t1;
			}
		}
		else {
			t1 = bd->valid_sap;
			bd->valid_sap = sap;
			sap->next_sap = t1;
		}
	}

	bd->ttl_valid_sap = 0;
	t1 = bd->valid_sap;
	while( t1 ) {
		bd->ttl_valid_sap++;
		t1 = t1->next_sap;
	}
}

/*
 * int
 * DLis_equalsnap( DL_sap_t *s1, DL_sap_t *s2 )
 *  return 1 if two sap are equal, return 0 otherwise. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
int
DLis_equalsnap( DL_sap_t *s1, DL_sap_t *s2 )
{
	return ((s1->snap_local == s2->snap_local) && 
		(s1->snap_global == s2->snap_global));
}

/*
 * STATIC  mblk_t*
 * DLmk_ud_ind( mblk_t *mp_data, DL_sap_t *sap )
 *  create DL_UNITDATA_IND message.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
STATIC  mblk_t*
DLmk_ud_ind( mblk_t *mp_data, DL_sap_t *sap )
{
	struct llca *llcap;
	struct llcb *llcbp;
	struct llcc *llccp;
	dl_unitdata_ind_t *data_ind;
	mblk_t		*mp;
	ushort_t	*word_p;
	uchar_t		dsap;
	int		i;
	uint_t		msgtype,len_type_field;
	ushort_t	actlen;


	/* LINTED pointer alignment */
	DL_mac_hdr_t *hdr = (DL_mac_hdr_t *)mp_data->b_rptr;

	len_type_field = LLC_LENGTH(hdr);
	msgtype = (len_type_field > DL_MAX_PACKET) ? DL_ETHER : DL_CSMACD;

	if ((mp = allocb((DL_UNITDATA_IND_SIZE + (sizeof(struct llcb) * 2)),
		BPRI_MED)) == NULL)
		return (NULL);

	/* set data type */
	mp->b_datap->db_type = M_PROTO;

	/* LINTED pointer alignment */
	data_ind                      =(dl_unitdata_ind_t *)mp->b_rptr;
	data_ind->dl_primitive        = DL_UNITDATA_IND;
	data_ind->dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;

	switch(msgtype) {
	case DL_ETHER:
		data_ind->dl_dest_addr_length = DL_ETH_ADDR_LEN;
		data_ind->dl_src_addr_length  = DL_ETH_ADDR_LEN;
		data_ind->dl_src_addr_offset  = DL_UNITDATA_IND_SIZE + DL_ETH_ADDR_LEN;

		/* LINTED pointer alignment */
		llcap = (struct llca *)( (caddr_t)data_ind + DL_UNITDATA_IND_SIZE);

		/* LINTED pointer alignment */
		BCOPY(hdr->dst.bytes, llcap->lbf_addr, LLC_ADDR_LEN);
		llcap->lbf_sap = ( ushort )len_type_field;
		llcap++;

		/* LINTED pointer alignment */
		BCOPY(hdr->src.bytes, llcap->lbf_addr, LLC_ADDR_LEN);
		llcap->lbf_sap = ( ushort )len_type_field;
		mp->b_wptr = mp->b_rptr + (DL_UNITDATA_IND_SIZE + 2 * DL_ETH_ADDR_LEN);
		mp_data->b_rptr += ETH_HDR_SIZE;
                return mp;

	case DL_CSMACD:
		dsap = LLC_DSAP(hdr);
		actlen = LLC_LENGTH(hdr);
		if ( (sap->flags & RAWCSMACD) || (dsap != SNAPSAP) ) {
			data_ind->dl_dest_addr_length = DL_LLC_ADDR_LEN;
			data_ind->dl_src_addr_length  = DL_LLC_ADDR_LEN;
			data_ind->dl_src_addr_offset  = DL_UNITDATA_IND_SIZE + DL_LLC_ADDR_LEN;

			/* LINTED pointer alignment */
			llccp = (struct llcc *)( (caddr_t)data_ind + DL_UNITDATA_IND_SIZE);

			/* LINTED pointer alignment */
			BCOPY(hdr->dst.bytes, llccp->lbf_addr, LLC_ADDR_LEN);
			llccp->lbf_sap = dsap;
			llccp++;

			/* LINTED pointer alignment */
			BCOPY(hdr->src.bytes, llccp->lbf_addr, LLC_ADDR_LEN);
			llccp->lbf_sap = LLC_SSAP(hdr);
			mp->b_wptr = mp->b_rptr + (DL_UNITDATA_IND_SIZE + 2 * DL_LLC_ADDR_LEN);
			if (sap->flags & RAWCSMACD) {
				mp_data->b_rptr += ETH_HDR_SIZE;
				mp_data->b_wptr = mp_data->b_rptr + actlen;
			} else {
				mp_data->b_rptr += LLC_HDR_SIZE;
				mp_data->b_wptr = mp_data->b_rptr +
                                        (actlen - LLC_HDR_SAP_SIZE);
			}
		}
		else {
			data_ind->dl_dest_addr_length = sizeof(struct llcb);
			data_ind->dl_src_addr_length = sizeof(struct llcb);
			data_ind->dl_src_addr_offset = DL_UNITDATA_IND_SIZE + sizeof(struct llcb);

			/* LINTED pointer alignment */
			llcbp = (struct llcb *) ((caddr_t)data_ind + data_ind->dl_dest_addr_offset);

			/* LINTED pointer alignment */
			BCOPY(hdr->dst.bytes, llcbp->lbf_addr, LLC_ADDR_LEN);
			llcbp->lbf_sap = dsap;

			llcbp->lbf_xsap = 0;
			BCOPY(hdr->mac_llc.snap.snap_org, &llcbp->lbf_xsap, SNAP_GLOBAL_SIZE);

			llcbp->lbf_type = SNAP_TYPE(hdr);
			llcbp++;

			/* LINTED pointer alignment */
			BCOPY(hdr->src.bytes, llcbp->lbf_addr, LLC_ADDR_LEN);
			llcbp->lbf_sap = sap->sap_addr;
			llcbp->lbf_xsap = sap->snap_global;
			llcbp->lbf_type = SNAP_TYPE(hdr);
			mp->b_wptr = mp->b_rptr + DL_UNITDATA_IND_SIZE + 
				(2 * sizeof(struct llcb));
			mp_data->b_rptr += SNAP_HDR_SIZE;
			mp_data->b_wptr = mp_data->b_rptr +
                                (actlen - SNAP_SAP_SIZE - LLC_HDR_SAP_SIZE);
		}
                return mp;
	}
}

/*
 * int
 * DLrecv( mblk_t *mp, DL_sap_t *tsap )
 *  receive packet for the sap list pointed to by tsap. 
 *
 * Calling/Exit State:
 *	hold lock on entry.
 *
 */
int
DLrecv( mblk_t *mp, DL_sap_t *tsap )
{
	DL_bdconfig_t   *bd = tsap->bd;
	DL_mac_hdr_t    *hdr;
	DL_sap_t        *sap;
	int len_type_field;
	int msgtype;
	int valid = 1;
	uchar_t dsap, ssap;


	if ( (bd->ttl_valid_sap == 0) ||
		((int)(mp->b_wptr - mp->b_rptr) > DL_MAX_PLUS_HDR) ){
		freemsg(mp);
		return(0);
	}

        len_type_field = LLC_LENGTH(mp->b_rptr);
        msgtype = (len_type_field > DL_MAX_PACKET) ? DL_ETHER : DL_CSMACD;

	/* LINTED pointer alignment */
        hdr = (DL_mac_hdr_t *)mp->b_rptr;

        if ( (bd->multicast_cnt > 0) || (bd->promisc_cnt > 0)) {

		/* LINTED pointer alignment */
                valid = DLis_us(bd,(DL_eaddr_t *)&(hdr->dst))
                                ||
			/* LINTED pointer alignment */
                        DLis_broadcast((DL_eaddr_t *)&(hdr->dst))
                                ||
			/* LINTED pointer alignment */
                        DLis_multicast(bd,(DL_eaddr_t *)&(hdr->dst));
        }

        dsap = hdr->mac_llc.llc.llc_dsap;
	ssap = hdr->mac_llc.llc.llc_ssap;

        for ( sap = bd->valid_sap; sap; sap = sap->next_sap) {
                if ( (sap->state != DL_IDLE) || (sap->write_q == NULL) )
                        continue;

                if (sap->sap_addr == PROMISCUOUS_SAP) {
                        CHK_FLOWCTRL_PUT(sap,mp);
                }
		else if (sap->mac_type != msgtype)
                        continue;

                else if ( valid && sap->mac_type == DL_ETHER &&
                                        (sap->sap_addr == len_type_field) ) {
                        CHK_FLOWCTRL_PUT(sap,mp);
                }
		else if (valid && sap->mac_type == DL_CSMACD)  {
			if (dsap == sap->sap_addr) {
				if ((dsap == SNAPSAP) &&
						(!DLis_validsnap(hdr,sap)))
                                        continue;

                                CHK_FLOWCTRL_PUT(sap,mp);
                        }
			else if ((dsap == LLC_GLOBAL_SAP) &&
					(sap->flags & RAWCSMACD)) {
				CHK_FLOWCTRL_PUT(sap,mp);
			}
			else if ((dsap == LLC_GLOBAL_SAP) &&
					(ssap != LLC_GLOBAL_SAP) &&
						(!(sap->flags & SNAPCSMACD))) {
				CHK_FLOWCTRL_PUT(sap,mp);
			}
                }
        }
        bd->mib.ifInUnknownProtos++;
        freemsg(mp);
        return 1;
}

/*
 * int
 * DLis_validsnap( DL_mac_hdr_t *hdr, DL_sap_t *sap )
 *  return 1 if the sap is valid, return 0 otherwise. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
int
DLis_validsnap( DL_mac_hdr_t *hdr, DL_sap_t *sap )
{
	ulong_t snap_global = 0;

	if (!(sap->flags & SNAPCSMACD))
		return 0;
	BCOPY(hdr->mac_llc.snap.snap_org, &snap_global, SNAP_GLOBAL_SIZE);
	if ( (snap_global != sap->snap_global) || (sap->snap_local != SNAP_TYPE(hdr)) )
		return 0;

	return 1;
}
