/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_DLPI_EL16_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_DLPI_EL16_H	/* subject to change without notice */

#ident	"@(#)ihvkit:net/dlpi_ether_2.x/dlpi_el16.h	1.1"
#ident  "$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifdef	_KERNEL_HEADERS

#include <util/types.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strstat.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <mem/immu.h>
#ifdef ESMP
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <net/inet/if.h>
#else	/* ESMP	*/
#include <net/tcpip/strioc.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/tcpip/if.h>
#endif	/* ESMP	*/
#include <net/dlpi.h>
#ifndef lint
#ifdef ESMP
#include <net/inet/byteorder.h>
#else
#include <net/tcpip/byteorder.h>
#endif	/* ESMP */
#endif	/* lint	*/
#include <io/rtc/rtc.h>
#include <io/dlpi_ether/el16/el16.h>
#include <io/ddi.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <net/strioc.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <sys/dlpi.h>
#include <sys/immu.h>
#include <sys/systm.h>
#include <sys/byteorder.h>
#include <sys/rtc.h>
#include <sys/cmn_err.h>
#include <sys/el16.h>
#include <sys/ddi.h>

#endif /* _KERNEL_HEADERS */

/*
 *  Device dependent symbol names.
 */

/*
 *  STREAMS structures
 */
#define	DL_NAME			"el16"
#define	DLdevflag		el16devflag
#define	DLrminfo		el16rminfo
#define	DLwminfo		el16wminfo
#define	DLrinit			el16rinit
#define	DLwinit			el16winit

/*
 *  Functions
 */
#define DLopen			el16open
#define	DLclose			el16close
#define DLrput			el16rput
#define	DLwput			el16wput
#define	DLioctl			el16ioctl
#define	DLinfo			el16info
#define	DLloopback		el16loopback
#define	DLmk_ud_ind		el16mk_ud_ind
#define	DLxmit_packet	el16xmit_packet
#define	DLinfo_req		el16info_req
#define	DLcmds			el16cmds
#define	DLprint_eaddr	el16print_eaddr
#define	DLbind_req		el16bind_req
#define	DLrsrv			el16rsrv
#define	DLunbind_req	el16unbind_req
#define	DLunitdata_req	el16unitdata_req
#define	DLerror_ack		el16error_ack
#define	DLuderror_ind	el16uderror_ind
#define	DLpromisc_off	el16promisc_off
#define	DLpromisc_on	el16promisc_on
#define	DLset_eaddr		el16set_eaddr
#define	DLadd_multicast	el16add_multicast
#define	DLdel_multicast	el16del_multicast
#define	DLdisable		el16disable
#define	DLenable		el16enable
#define	DLreset			el16reset
#define	DLis_multicast	el16is_multicast
#define	DLget_multicast	el16get_multicast
#define DLrecv		el16recv
#define DLproc_llc	el16proc_llc
#define DLform_80223	el16form_80223
#define DLmk_test_con	el16mk_test_con
#define DLinsert_sap	el16insert_sap
#define DLsubsbind_req	el16subsbind_req
#define DLtest_req	el16test_req
#define DLremove_sap	el16remove_sap
#define DLis_equalsnap	el16is_equalsnap
#define DLform_snap	el16form_snap
#define DLis_broadcast	el16is_broadcast
#define DLis_us		el16is_us
#define DLis_validsnap	el16is_validsnap

#define DLbdspecioctl	el16bdspecioctl
#define DLbdspecclose	el16bdspecclose

/*
 *  Implementation structures and variables
 */
#define DLboards	el16boards
#define DLconfig	el16config
#define DLsaps		el16saps
#define DLstrlog	el16strlog
#define DLifstats	el16ifstats
#define	DLinetstats	el16inetstats
#define	DLid_string	el16id_string

/*
 *  Flow control defines
 */
#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC	(DL_MAX_PACKET - 3)
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER		(40 * DL_MAX_PACKET)
#define	DL_LOWATER		(20 * DL_MAX_PACKET)

#define TBD_BUF_SIZ 1514
#define RBD_BUF_SIZ 1514

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		46

/* Inline asm routine for block moves */
#ifndef lint
#ifndef C_PIO

asm	void mybcopy(src, dest, count)
{
%mem    src, dest, count;
	pushl	%ecx
	movl    count, %eax             /* save the count */
	pushl   %edi
	pushl   %esi
	movl    src, %esi               /* %esi = source */
	movl    dest, %edi              /* %edi = destination */
	movl    %eax, %ecx
	shrl    $2, %ecx                /* cx <- cx / 4 */
	cld
	repz
	smovl
	movl    %eax, %ecx
	andl    $3, %ecx
	repz
	smovb
	popl    %esi
	popl    %edi
	popl	%ecx
}
#pragma asm partial_optimization mybcopy

#endif	/* ifndef C_PIO */
#endif	/* ifndef lint */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_DLPI_ETHER_DLPI_EL16_H */
