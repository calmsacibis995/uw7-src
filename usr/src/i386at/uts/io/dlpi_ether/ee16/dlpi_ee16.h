#ifndef	_IO_DLPI_ETHER_DLPI_EE16_H	/* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_DLPI_EE16_H	/* subject to change without notice */

#ident	"@(#)dlpi_ee16.h	2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*		Copyright (c) 1991  Intel Corporation		*/
/*			All Rights Reserved			*/

/*		INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifdef	_KERNEL_HEADERS

#include <util/types.h>
#include <io/dlpi_ether/ee16/ee16.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/ee16.h>

#endif	/* _KERNEL_HEADERS */

/*
 *  STREAMS structures
 */
#define	DL_NAME			"ee16"
#define	DLdevflag		ee16devflag
#define	DLrminfo		ee16rminfo
#define	DLwminfo		ee16wminfo
#define	DLrinit			ee16rinit
#define	DLwinit			ee16winit

/*
 *  Functions
 */
#define DLopen			ee16open
#define	DLclose			ee16close
#define DLrput			ee16rput
#define	DLwput			ee16wput
#define	DLioctl			ee16ioctl
#define	DLinfo			ee16info
#define	DLloopback		ee16loopback
#define	DLmk_ud_ind		ee16mk_ud_ind
#define	DLxmit_packet		ee16xmit_packet
#define	DLinfo_req		ee16info_req
#define	DLcmds			ee16cmds
#define	DLprint_eaddr		ee16print_eaddr
#define	DLbind_req		ee16bind_req
#define	DLrsrv			ee16rsrv
#define	DLunbind_req		ee16unbind_req
#define	DLunitdata_req		ee16unitdata_req
#define	DLerror_ack		ee16error_ack
#define	DLuderror_ind		ee16uderror_ind
#define	DLpromisc_off		ee16promisc_off
#define	DLpromisc_on		ee16promisc_on
#define	DLset_eaddr		ee16set_eaddr
#define	DLadd_multicast		ee16add_multicast
#define	DLdel_multicast		ee16del_multicast
#define	DLget_multicast		ee16get_multicast
#define	DLdisable		ee16disable
#define	DLenable		ee16enable
#define	DLreset			ee16reset
#define	DLis_multicast		ee16is_multicast
#define DLrecv			ee16recv
#define DLproc_llc		ee16proc_llc
#define	DLform_80223		ee16form_80223
#define DLis_us			ee16is_us
#define DLis_broadcast		ee16is_broadcast
#define DLis_validsnap		ee16is_validsnap
#define DLis_equalsnap		ee16is_equalsnap
#define DLform_snap		ee16form_snap
#define DLmk_test_con		ee16mk_test_con
#define DLinsert_sap		ee16insert_sap
#define DLsubsbind_req		ee16subsbind_req
#define DLtest_req		ee16test_req
#define DLremove_sap		ee16remove_sap

#define DLbdspecioctl		ee16bdspecioctl
#define DLbdspecclose		ee16bdspecclose

/*
 *  Implementation structures and variables
 */
#define DLboards		ee16boards
#define DLconfig		ee16config
#define DLsaps			ee16saps
#define DLstrlog		ee16strlog
#define DLifstats		ee16ifstats
#define	DLinetstats		ee16inetstats
#define	DLid_string		ee16id_string

#define	read_word		ee16read_word
#define	write_word		ee16write_word
#define	bcopy_to_buffer		ee16bcopy_to_buffer
#define	bcopy_from_buffer	ee16bcopy_from_buffer

/*
 *  Flow control and packet size defines
 *  The size of the 802.2 header is 3 bytes.
 *  The size of the SNAP header includes 5 additional bytes in addition to the
 *  802.2 header.
 */

#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC      	(DL_MAX_PACKET - 3) 
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER		(40 * DL_MAX_PACKET) 
#define	DL_LOWATER		(20 * DL_MAX_PACKET)

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		46

#define TBD_BUF_SIZ 1520
#define RBD_BUF_SIZ 1520

#define	BYTE	0
#define WORD	1

#define read_byte(location, bio, dest) \
			if (inb(bio + AUTOID)); \
			outw(bio + RDPTR, location); \
			dest = inb(bio + DXREG)

#define write_byte(location, value, bio) \
			if (inb(bio + AUTOID)); \
			outw(bio + WRPTR, location); \
			outb(bio + DXREG, value)

#ifdef lint
#ifdef C_PIO
#define read_word(location, bio, dest) \
			if (inb(bio + AUTOID)); \
			outw(bio + RDPTR, location); \
			dest = inw(bio + DXREG)

#define write_word(location, value, bio) \
			if (inb(bio + AUTOID)); \
			outw(bio + WRPTR, location); \
			outw(bio + DXREG, value)
#endif	/* ifdef C_PIO */
#endif	/* ifdef lint */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_DLPI_ETHER_DLPI_EE16_H */
