#ident	"@(#)ihvkit:net/dlpi_ether/sys/dlpi_ether.h	1.1"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_DLPI_ETHER_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_DLPI_ETHER_H	/* subject to change without notice */

#ident	"$Header$"

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	


#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _IO_STREAM_H
#include <io/stream.h>	/* REQUIRED */
#endif

#ifndef _FS_IOCCOM_H
#include <fs/ioccom.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/stream.h>	/* REQUIRED */
#include <sys/ioccom.h>

#else

#include <sys/types.h>
#include <sys/ioccom.h>
#include <sys/stream.h>

#endif	/* _KERNEL_HEADERS */



/*
 *  DLIP ethernet IOCTL defines.
 */
#define	DLIOCSMIB	_IOW('D', 0, int)	/* Set MIB		     */
#define	DLIOCGMIB	_IOR('D', 1, int)	/* Get MIB		     */
#define	DLIOCSENADDR	_IOW('D', 2, int)	/* Set ethernet address	     */
#define	DLIOCGENADDR	_IOR('D', 3, int)	/* Get ethernet address	     */
#define	DLIOCSLPCFLG	_IOW('D', 4, int)	/* Set local packet copy flag*/
#define	DLIOCGLPCFLG	_IOR('D', 5, int)	/* Get local packet copy flag*/
#define	DLIOCSPROMISC	_IOW('D', 6, int)	/* Toggle promiscuous state  */
#define	DLIOCGPROMISC	_IOR('D', 7, int)	/* Get promiscuous state     */
#define	DLIOCADDMULTI	_IOW('D', 8, int)	/* Add multicast address     */
#define	DLIOCDELMULTI	_IOW('D', 9, int)	/* Delete multicast address  */
#define	DLIOCDISABLE	_IOW('D',10, int)	/* Disable controller        */
#define	DLIOCENABLE	_IOW('D',11, int)	/* Enable controller         */
#define	DLIOCRESET	_IOW('D',12, int)	/* Reset controller          */
#define DLIOCCSMACDMODE _IOW('D',13, int)	/* Toggle CSMA-CD mode       */
#define DLIOCGETSAP	_IOW('D',14, int)	/* List of sap ...temp*/
#define DLIOCGETMULTI	_IOW('D',15, int)	/* Get multicast address list */
/*
 *  Some other defines
 */
#define DL_MAX_PLUS_HDR		1514	/* Absolute MAX Packet (cf. DIX v2.0) */
#define	DL_MAC_ADDR_LEN		6
#define	DL_SAP_LEN		2
#define DL_TOTAL_ADDR_LEN	(DL_MAC_ADDR_LEN + DL_SAP_LEN)
#define	DL_ID			(ENETM_ID)
#define	DL_PRIMITIVES_SIZE	(sizeof(union DL_primitives))

#define	IS_MULTICAST(eaddr)	(eaddr.bytes[ 0 ] & 1)

#if defined(DL_STRLOG) && !defined(lint)
#define	DL_LOG(x)	 DLstrlog ? x : 0
#else
#define DL_LOG(x)
#endif

/*
 *  Special SAP ID's
 */
#define	PROMISCUOUS_SAP	((ushort_t)0xffff)	/* Matches all SAP ID's	     */


/*
 *  Standard DLPI ethernet address type
 */
typedef union {
	uchar_t		bytes[ DL_MAC_ADDR_LEN ];
	ushort_t	words[ DL_MAC_ADDR_LEN / 2 ];
} DL_eaddr_t;

/*
 *  Standard DLPI ethernet and LLC header types
 */
typedef struct DL_ether_hdr {
	DL_eaddr_t      dst;            /* destination address          */
        DL_eaddr_t      src;            /* source address     */
	ushort_t len_type;
}DL_ether_hdr_t;

struct ethertype {
	ushort_t	len_type;	/* len/type field		*/
};

struct llctype {
	ushort_t	llc_length;
	uchar_t		llc_dsap;
	uchar_t		llc_ssap;
	uchar_t		llc_control;
	uchar_t		llc_info[3];
};

struct snaptype {
	ushort_t 	snap_length;
	uchar_t 	snap_dsap;
	uchar_t		snap_ssap;
	uchar_t		snap_control;
	uchar_t		snap_org[3];
	ushort_t	snap_type;
};

struct snap_sap {
	ulong_t		snap_global;
	ulong_t		snap_local;
};

typedef	struct DL_mac_hdr {
	DL_eaddr_t	dst;		/* destination address		*/
	DL_eaddr_t	src;		/* source address		*/
	union {
		struct ethertype ether;
		struct llctype llc;
		struct snaptype snap;
	} mac_llc;
} DL_mac_hdr_t;

/*
 *  Ether statistics structure.
 */
typedef struct {
	ulong_t	etherAlignErrors;	/* Frame alignment errors	     */
	ulong_t	etherCRCerrors;		/* CRC erros			     */
	ulong_t	etherMissedPkts;	/* Packet overflow or missed inter   */
	ulong_t	etherOverrunErrors;	/* Overrun errors		     */
	ulong_t	etherUnderrunErrors;	/* Underrun errors		     */
	ulong_t	etherCollisions;	/* Total collisions		     */
	ulong_t	etherAbortErrors;	/* Transmits aborted at interface    */
	ulong_t	etherCarrierLost;	/* Carrier sense signal lost	     */
	ulong_t	etherReadqFull;		/* STREAMS read queue full	     */
	ulong_t	etherRcvResources;	/* Receive resource alloc faliure    */
	ulong_t	etherDependent1;	/* Device dependent statistic	     */
	ulong_t	etherDependent2;	/* Device dependent statistic	     */
	ulong_t	etherDependent3;	/* Device dependent statistic	     */
	ulong_t	etherDependent4;	/* Device dependent statistic	     */
	ulong_t	etherDependent5;	/* Device dependent statistic	     */
} DL_etherstat_t;

/*
 *  Interface statistics compatible with MIB II SNMP requirements.
 */
typedef	struct {
	int		ifIndex;	/* ranges between 1 and ifNumber     */
	int		ifDescrLen;	/* len of desc. following this struct*/
	int		ifType;		/* type of interface                 */
	int		ifMtu;		/* datagram size that can be sent/rcv*/
	ulong_t		ifSpeed;	/* estimate of bandwith in bits PS   */
	uchar_t		ifPhyAddress[ DL_MAC_ADDR_LEN ];  /* Ethernet Address*/
	int		ifAdminStatus;	/* desired state of the interface    */
	int		ifOperStatus;	/* current state of the interface    */
	ulong_t		ifLastChange;	/* sysUpTime when state was entered  */
	ulong_t		ifInOctets;	/* octets received on interface      */
	ulong_t		ifInUcastPkts;	/* unicast packets delivered         */
	ulong_t		ifInNUcastPkts;	/* non-unicast packets delivered     */
	ulong_t		ifInDiscards;	/* good packets received but dropped */
	ulong_t		ifInErrors;	/* packets received with errors      */
	ulong_t		ifInUnknownProtos; /* packets recv'd to unbound proto*/
	ulong_t		ifOutOctets;	/* octets transmitted on interface   */
	ulong_t		ifOutUcastPkts;	/* unicast packets transmited        */
	ulong_t		ifOutNUcastPkts;/* non-unicast packets transmited    */
	ulong_t		ifOutDiscards;	/* good outbound packets dropped     */
	ulong_t		ifOutErrors;	/* number of transmit errors         */
	ulong_t		ifOutQlen;	/* length of output queue            */
	DL_etherstat_t	ifSpecific;	/* ethernet specific stats           */
} DL_mib_t;
/*
 *  ifAdminStatus and ifOperStatus values
 */
#define			DL_UP	1	/* ready to pass packets             */
#define			DL_DOWN	2	/* not ready to pass packets         */
#define			DL_TEST	3	/* in some test mode                 */

/*
 *  Board related info.
 */
typedef struct bdconfig{
	major_t		major;		/* major number for device	     */
	ulong_t		io_start;	/* start of I/O base address	     */
	ulong_t		io_end;		/* end of I/O base address	     */
	paddr_t		mem_start;	/* start of base mem address	     */
	paddr_t		mem_end;	/* start of base mem address	     */
	int		irq_level;	/* interrupt request level	     */
	int		max_saps;	/* max service access points (minors)*/
	int		bd_number;	/* board number in multi-board setup */
	int		flags;		/* board management flags	     */
#define				BOARD_PRESENT	0x01
#define				BOARD_DISABLED	0x02
#define				TX_BUSY		0x04
#define				TX_QUEUED	0x08
	int		tx_next;	/* round robin service of SAP queues */
	int		timer_id;	/* watchdog timer ID		     */
	int 		timer_val;      /* watchdog timer value              */
	int		promisc_cnt;	/* count of promiscuous bindings     */
	int		multicast_cnt;	/* count of multicast address sets   */
	int		ttl_valid_sap;
	struct sap	*sap_ptr;	/* ptr to SAP array for this board   */
	struct sap	*valid_sap;
	struct ifstats	*ifstats;	/* ptr to IP stats structure (TCP/IP)*/
	DL_eaddr_t	eaddr;		/* Ethernet address storage	     */
	caddr_t		bd_dependent1;	/* board dependent value	     */
	caddr_t		bd_dependent2;	/* board dependent value	     */
	caddr_t		bd_dependent3;	/* board dependent value	     */
	caddr_t		bd_dependent4;	/* board dependent value	     */
	caddr_t		bd_dependent5;	/* board dependent value	     */
	DL_mib_t	mib;		/* SNMP interface statistics	     */
} DL_bdconfig_t;

/*
 *  SAP related info.
 */
typedef struct sap {
	int		state;		/* DLPI state			     */
	ushort_t	sap_addr;	/* bound SAP address		     */
	ushort_t	snap_local;	/* lower order 16 bits of the PIF    */
	ulong_t		snap_global;	/* Higher order 24 bits of the PIF   */
	queue_t		*read_q;	/* the read queue pointer	     */
	queue_t		*write_q;	/* the write queue pointer	     */
	int		flags;		/* SAP management flags		     */

#define				PROMISCUOUS		0x01
#define				SEND_LOCAL_TO_NET	0x02
#define				PRIVILEDGED		0x04
#define				RAWCSMACD		0x08
#define				SNAPCSMACD		0x10

	int		max_spdu;	/* largest amount of user data	     */
	int		min_spdu;	/* smallest amount of user data	     */
	int		mac_type;	/* DLPI mac type		     */
	int		service_mode;	/* DLPI servive mode		     */
	int		provider_style;	/* DLPI provider style		     */
	DL_bdconfig_t	*bd;		/* pointer to controlling bdconfig   */
	struct sap 	*next_sap;	/* pointer to the next valid/idle sap*/
} DL_sap_t;

/* LLC specific definitions and declarations */

#define LLC_UI		0x03	/* unnumbered information field */
#define LLC_XID		0xAF	/* XID with P == 0 */
#define LLC_TEST	0xE3	/* TEST with P == 0 */

#define LLC_P		0x10	/* P bit for use with XID/TEST */
#define LLC_XID_FMTID	0x81	/* XID format identifier */
#define LLC_SERVICES	0x01	/* Services supported */
#define LLC_GLOBAL_SAP	0xFF	/* Global SAP address */
#define LLC_NULL_SAP	0x00	/* NULL SAP address */
#define LLC_GROUP_ADDR	0x01	/* indication in DSAP of a group address */
#define LLC_RESPONSE	0x01	/* indication in SSAP of a response */

#define LLC_XID_INFO_SIZE	3 /* length of the INFO field */


#define LLC_SAP_LEN     1       /* length of sap only field */
#define LLC_LSAP_LEN    2       /* length of sap/type field  */
#define LLC_TYPE_LEN    2       /* ethernet type field length */
#define LLC_ADDR_LEN    6       /* length of 802.3/ethernet address */
#define LLC_LSAP_HDR_SIZE 3
#define LLC_HDR_SIZE    (LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_LSAP_HDR_SIZE+LLC_LSAP_LEN)
#define LLC_EHDR_SIZE   (LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_TYPE_LEN)

#define LLC_LIADDR_LEN  (LLC_ADDR_LEN+LLC_SAP_LEN)
#define LLC_ENADDR_LEN  (LLC_ADDR_LEN+LLC_TYPE_LEN)

#define SNAP_LSAP_HDR_SIZE	5 	/* SNAP HDR excluding LLC header */

#define SNAP_HDR_SIZE 	(LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_LSAP_HDR_SIZE+LLC_LSAP_LEN + SNAP_LSAP_HDR_SIZE)

#define LLC_SNADDR_LEN  (LLC_ADDR_LEN + 2 + 4 + 4 )

union llc_bind_fmt {
	struct llca {
		unsigned char  lbf_addr[LLC_ADDR_LEN];
		unsigned short lbf_sap;
   	} llca;
	struct llcb {
		unsigned char  lbf_addr[LLC_ADDR_LEN];
		unsigned short lbf_sap;
		unsigned long  lbf_xsap;
		unsigned long  lbf_type;
	} llcb;
   	struct llcc {
      		unsigned char lbf_addr[LLC_ADDR_LEN];
      		unsigned char lbf_sap;
   	} llcc;
};

#define SNAP_LENGTH(m) ntohs(((struct DL_mac_hdr *)m)->mac_llc.snap.snap_length)
#define SNAP_DSAP(m)    (((struct DL_mac_hdr *)m)->mac_llc.snap.snap_dsap)
#define SNAP_SSAP(m)    (((struct DL_mac_hdr *)m)->mac_llc.snap.snap_ssap)
#define SNAP_CONTROL(m) (((struct DL_mac_hdr *)m)->mac_llc.snap.snap_control)
#define SNAP_TYPE(m)    ntohs(((struct DL_mac_hdr *)m)->mac_llc.snap.snap_type)

#define LLC_LENGTH(m)   ntohs(((struct DL_mac_hdr *)m)->mac_llc.llc.llc_length)
#define LLC_DSAP(m)     (((struct DL_mac_hdr *)m)->mac_llc.llc.llc_dsap)
#define LLC_SSAP(m)     (((struct DL_mac_hdr *)m)->mac_llc.llc.llc_ssap)
#define LLC_CONTROL(m)  (((struct DL_mac_hdr *)m)->mac_llc.llc.llc_control)
#define ETHER_TYPE(m)   ntohs(((struct wd_machdr *)m)->mac_llc.ether.len_type)

#define MAXSAPVALUE	0xFF 	/* Largest LSAP value */
#define SNAPSAP		0xaa	/* Value of SNAP sap */

#define SNAP_GLOBAL_SIZE	0x3	/*
					Size in bytes of the first 24
					bits of the PIF
					*/
#define SNAP_LOCAL_SIZE		0x2	/*
					Size in bytes of the last 16 
					bits of the PIF
					*/
#define SNAPSAP_SIZE		(SNAP_GLOBAL_SIZE + SNAP_LOCAL_SIZE)

typedef struct rcv_buf {
	unsigned char	status;
	unsigned char	nxtpg;
	short		datalen;
	DL_mac_hdr_t	pkthdr;
} rcv_buf_t;

#define CHK_FLOWCTRL_PUT(sap,mp)                \
                        if (!canput((sap)->read_q)) { \
                                (sap)->bd->mib.ifInDiscards++; \
                                (sap)->bd->mib.ifSpecific.etherReadqFull++;\
				freemsg(mp); \
                                return 1; \
                        } else  { \
                                putq((sap)->read_q,(mp)); \
                                return 0; \
                        }

#define CHK_FLOWCTRL_DUP_PUT(sap,mp1,mp2)		\
		if (canput((sap)->read_q->q_next) && ((mp2) = dupmsg((mp1))) )\
			putnext((sap)->read_q,(mp2)); \
		else \
			continue;

#endif /* _IO_DLPI_ETHER_DLPI_ETHER_H */
