#ifndef _IO_DLPI_LSL_DLPI_LSL_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_LSL_DLPI_LSL_H	/* subject to change without notice */

#ident	"@(#)lsl.h	9.1"
#ident	"$Header$"

#ifdef  _KERNEL_HEADERS

#include <io/odi/odi.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/confmgr/confmgr.h>

#elif defined(_KERNEL)

#include <sys/odi.h>
#include <sys/cm_i386at.h>
#include <sys/confmgr.h>

#endif

/*
 * DLPI ethernet IOCTL defines.
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
 * to maintain the backward binary compatibility, the following are here
 * fo tokenring
 */
#define DLGBROAD     (('D' << 8) | 3)  /* get broadcast address entry */
#define DLGDEVSTAT   (('D' << 8) | 4)  /* get statistics for the board*/
#define DLGADDR      (('D' << 8) | 5)  /* get physical addr of interface */
#define DLSRAWLLC    (('D' << 8) | 17) /* toggle RAWLLC mode */
#define DLSRAW       (('D' << 8) | 18) /* toggle raw mode */
#define MACIOC(x)    (('M' << 8) | (x))
#define MACIOC_GETADDR MACIOC(8)       /* get mac address */


/*
 * we are adding two more ioctl cmd here for tokenring in the plan to
 * have only one set of ioctl for every odi TSM
 */

#define DLIOCRAWMODE    _IOW('D',16, int)       /* toggle RAWMODE */
#define DLIOCRAWLLC     _IOW('D',17, int)       /* toggle RAWLLC mode */

/*
 * some other defines.
 */
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
 * special SAP ID's
 */
#define	PROMISCUOUS_SAP	((ushort_t)0xffff)	/* Matches all SAP ID's	     */

/*
 * standard DLPI ethernet address type
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
	ushort_t	len_type;
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
 * LLC specific definitions and declarations.
 */
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
#define LLC_LSAP_HDR_SIZE	3

#define LLC_HDR_SIZE    (LLC_ADDR_LEN + LLC_ADDR_LEN + LLC_LSAP_HDR_SIZE + \
				LLC_LSAP_LEN)
#define LLC_EHDR_SIZE   (LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_TYPE_LEN)

#define LLC_LIADDR_LEN  (LLC_ADDR_LEN+LLC_SAP_LEN)
#define LLC_ENADDR_LEN  (LLC_ADDR_LEN+LLC_TYPE_LEN)

#define SNAP_LSAP_HDR_SIZE	5 	/* SNAP HDR excluding LLC header */

#define SNAP_HDR_SIZE 	(LLC_ADDR_LEN + LLC_ADDR_LEN + LLC_LSAP_HDR_SIZE + \
				LLC_LSAP_LEN + SNAP_LSAP_HDR_SIZE)

#define LLC_SNADDR_LEN  (LLC_ADDR_LEN + 2 + 4 + 4 )

#define ETH_RAWCSMA_SIZE      3
#define ETH_RAW_LLC_SIZE      ETH_RAWCSMA_SIZE
#define ETH_RAWMODE_SIZE      (14 + ETH_RAWCSMA_SIZE)
#define TOK_RAW_LLC_SIZE      3
#define TOK_RAWMODE_SIZE      (30 + 14 + 5 + TOK_RAW_LLC_SIZE)
                              /* As per ODI specs 30 bytes source info.
				                  22 bytes MAC header
                                 The 22 bytes for MAC header consists of 
				 1 Access Control
				 1 Frame Control
				 6 dest addr
				 6 src addr
				 3 LLC
				 5 SNAP
                               */

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

#undef	BCOPY
#define BCOPY(from, to, len)	bcopy((caddr_t)(from), (caddr_t)(to), \
					(size_t)(len))
#undef	BCMP
#define BCMP(s1, s2, len)	bcmp( (char*)(s1), (char*)(s2), (size_t)(len))

#define	MB_SIZE(p)		((p)->b_wptr - (p)->b_rptr)

/*
 * structure for msm to pass function pointers to lsl.
 */
struct lsl_msm_funcs {
	void	(*lsl_msm_set_parse)();
	void	(*lsl_msm_enable_interrupts)();
	void	(*lsl_msm_cleanup)();
};

#define	LSL_NAME	"lsl"
#define	DLstrlog	lslstrlog

#define	LSL_TSM_MAX_VIRTUAL_BOARDS	4	/* Maximum number of virtual */
						/* boards for *any* given
						/* TSM. */
#define LSL_MAX_PHYSICAL_BOARDS		8	/* Number of physical boards */
						/* supported */

typedef uchar_t			LSL_macaddr_t[DL_MAC_ADDR_LEN];

/*
 * Like DL_mac_hdr, with the addition of sourceRouting for token ring.
 */
typedef	struct LSL_mac_hdr {
	DL_eaddr_t	dst;
	DL_eaddr_t	src;
	union {
		struct ethertype	ether;
		struct llctype		llc;
		struct snaptype 	snap;
		uchar_t				sourceRoute[18];
	} mac_llc;
} LSL_mac_hdr_t;

typedef struct _8022 {
	uchar_t 	dsap;
	uchar_t		ssap;
	uchar_t		control;
} _8022_t;

/*
 * Data structures to support multicast addresses.
 */
typedef struct lslmaddr {
	unsigned char entry[6];
	unsigned short refCount;
} lslmaddr_t;

typedef struct bdd {
	struct lslmaddr *lsl_multip;
	struct lslmaddr *lsl_multiaddrs;
} bdd_t;

/*
 * needed to get proper sizeof( this)
 */
#pragma pack(1)
typedef union {
	struct {
		uchar_t		b0;
		uchar_t		b1;
		uchar_t		b2;
		uchar_t		b3;
		uchar_t		b4;
		uchar_t		b5;
	} c;
	struct {
		ulong_t		l;
		ushort_t	s;
	} l;
} protocolID_t;
#pragma pack()

#define t1TYPE		c.b0	/* 802.2 Type I, 1 control byte, type field */
#define t1DSAP		c.b3	/* 802.2 Type I, 1 control byte, dsap field */
#define t1SSAP		c.b4	/* 802.2 Type I, 1 control byte, ssap field */
#define t1CONTROL	c.b5	/* 802.2 Type I, 1 control byte, control field*/

#define DSAP		l.s	/* DSAP field for Non-802.2 frame types */
#define SPARE		l.l	/* spare bytes, must be zero in this context */

/*
 * board related info.
 */
typedef struct lslbd {
	/*
	 * all modifiable fields in this struct are protected by bd_lock.
	 */
	major_t		bd_major;	/* major number for device */
	ulong_t		bd_io_start;	/* start of I/O base address */
	ulong_t		bd_io_end;	/* end of I/O base address */
	paddr_t		bd_mem_start;	/* start of base mem address */
	paddr_t		bd_mem_end;	/* start of base mem address */
	int		bd_irq_level;	/* interrupt request level */
	int		bd_max_saps;	/* max service access points (minors) */
	char		*bd_cmdline;	/* configuration command line */
	int		bd_dmac;
	char		*bd_idstring;	/* HSM' id string */

	int		bd_flags;	/* board management flags */
#define			BOARD_PRESENT		0x00001
#define			BOARD_DISABLED		0x00002
#define			BOARD_MSMBASED		0x00004
#define			BOARD_PCI		0x00008

	int		bd_special_sap;	/* board special sap count */
					/* This counter is kept for Ethernet
					 * only. lslrsrv() checks this counter
					 * to determine whether it is necessary
					 * to search through all the saps to
					 * see if anyone else is interested in
					 * this frame.
					 */
	DL_eaddr_t	bd_eaddr;	/* Ethernet address storage */

	struct	lslsap	*bd_sap_ptr;	/* sap array of the board */
	int		bd_promisc_cnt;	/* count of promiscuous bindings */
	int		bd_multicast_cnt; /* count of multicast address sets */
	int		bd_max_multicast;
	struct ifstats	*bd_ifstats;	/* ptr to IP stats struct (TCP/IP) */
	bdd_t		*bd_bdd;
	struct lslsap	*bd_valid_sap;
	DL_mib_t	bd_mib;
	struct lslbd	*bd_nextBoard;	/* pointer to next valid physical bd */
	int		bd_tsmtype;	/* TSM type, like TSM_T_ETHER et.al. */
	int		bd_virtualBoards[LSL_TSM_MAX_VIRTUAL_BOARDS];
	CONFIG_TABLE	*bd_defaultvirtualbd;
					/* adapter's default config table */
	GENERIC_STATS	*bd_genericstats;
					/* adapter's generic stats */
	/*
	 * the TSM's send routine. we keep a copy it's address here,
	 * because it's faster to get to it this way.
	 */
	void		(*bd_TSMSend)(struct ECB *, CONFIG_TABLE *configTable);

	/*
	 * pointer to this board's INFO_BLOCK, which contains a pointer
	 * to an array fo function pointers to handle various control
	 * functions.
	 */
	INFO_BLOCK	*bd_MLIDControlHandler;
	struct ECB	*bd_ecbPoolHead;	/* Head of xmit ECBs */
	int		bd_ecbPoolCount;	/* number of ECBs in pool */
	toid_t		bd_timeid;		/* timeid for stats timer */

	lock_t		*bd_lock;

	/*
	 * To support some PCI drivers.
	 * (Only needed during load/init so should really be alloced and freed)
	 */
	ushort_t	bd_pci_vendor_id;	/* but not '-1' */
	ushort_t	bd_pci_device_id;
	ushort_t	bd_pci_bus_dev_func;	/* bus(8), dev(5), func(3) */

} lsl_bdconfig_t;

/*
 * SAP related info.
 */
typedef struct lslsap {
	/*
	 * all modifiable fields in this struct are protected by the
	 * bd_lock of the board.
	 */
	int		sap_refcnt;
	int		sap_state;
	ushort_t	sap_addr;

	/*
	 * boundSAP is the really bound SAP address. it's the same as
	 * sap_addr in all frame types except SNAP, in which case it's
	 * the same as the former snap_local. this is the sap address
	 * we use to compare against what's handed to us in the ECB when
	 * we're receiving a packet.
	 */
	ushort_t	sap_boundSAP;			
	ulong_t		sap_snap_global;/* Higher order 24 bits of the PIF */
	queue_t		*sap_read_q;	/* the read queue pointer */
	queue_t		*sap_write_q;	/* the write queue pointer */
	int		sap_flags;	/* SAP management flags */
#define	PROMISCUOUS			0x01
#define	SEND_LOCAL_TO_NET		0x02
#define	PRIVILEDGED			0x04
#define	RAWCSMACD			0x08
#define	SNAPCSMACD			0x10
#define RAWMODE				0x20
#define RAW_LLC				0x40

	int		sap_max_spdu;	/* largest amount of user data */
	int		sap_min_spdu;	/* smallest amount of user data */
	int		sap_mac_type;	/* DLPI mac type */

	/*
	 * sap_service_mode and sap_provider_style do not change, once
	 * initialized.
	 */
	int		sap_service_mode;	/* DLPI servive mode */
	int		sap_provider_style;	/* DLPI provider style */

	/*
	 * sap_bd is not changed after init.
	 */
	lsl_bdconfig_t	*sap_bd;		/* ptr to controlling bd */

	/*
	 * sap_next_sap is protected by bd_lock of board.
	 */
	struct lslsap	*sap_next_sap;	/* ptr to the next valid/idle sap */

	/*
	 * virtual board array index for this frame type and ECB proto ID
	 * in High-Low order for TX. set at bind time and reset at close time.
	 */
	int		sap_virtualBoard;
	protocolID_t	sap_ecbProtocolID;

	/*
	 * the Novell Frame ID value. does not change after init.
	 */
	int		sap_frameID;	/* Novell Frame ID value */
} lsl_sap_t;

enum rxAction {
	ra_none = 0,		/* none */
	ra_passUpstream,	/* pass the message upstream */
	ra_test_reply,		/* need to respond to TEST */
	ra_xid_reply		/* need to respond to XID */
};

/*
 * rxData
 */
typedef struct {
	enum rxAction	action;
	lsl_sap_t	*sap;
	ushort_t	checkBoundDSAP;		/* from protocolID, used to */
						/* compare against boundSAP */
	DL_eaddr_t	srcMacAddr;		/* from frame header, used */
						/* to build text|xid response */
	ushort_t	llcSsap;		/* from frame header, used */
						/* to build text|xid response */
	uchar_t		llcControl;		/* from frame header, used */
						/* to build text|xid response */
} rxData_t;

/*
 * functions which are better off as macros.
 */
#define	lsl_isus(bd, dst)	(BCMP((bd)->bd_eaddr.bytes,		\
					(dst)->bytes, DL_MAC_ADDR_LEN) == 0)
#define	lsl_isbroadcast(dst)	(BCMP(lslBroadcastAddr.bytes,		\
					(dst)->bytes, DL_MAC_ADDR_LEN) == 0)

/*
 * At Lookahead time, we allocate 2 message blocks, one to 
 * hold the frame data and another to hold the ECB, rxData_t
 * and DLPI message.  When we allocb the second one, it should
 * be big enough to hold the largest DLPI message we can construct
 * (in addition to the ECB and rxData_t).  Compute that size here.
 */

/*
 * Size needed for maximum-sized UNITDATA_IND
 */
#define UD_SIZE		(DL_UNITDATA_IND_SIZE + 2 * sizeof(struct llcb))

/*
 * Size needed for max TEST_CON
 */
#define TEST_CON_SIZE	(DL_TEST_CON_SIZE + 2 * sizeof(DL_eaddr_t) +	\
				2 * sizeof(ushort_t))
#define RX_STRUCT_SIZE	(sizeof(manage_ecb) + sizeof(struct ECB) +	\
				sizeof(rxData_t) + MAX(UD_SIZE, TEST_CON_SIZE))

#define	LSL_HIERINIT		21
#define	LSL_HIERBDLIST		23
#define	LSL_HIERBD		25

#define SOURCE_ROUTBIT		0x80
/*
 * When "canonical" addresses (Least Significant Bit (LSB) 1st) are used --
 * as in FDDI -- the Source Routing Bit (aka Routing Info Indicator) is at
 * the "other end" of the octet
 */
#define LSB_SR_BIT		0x01

/*
 * Max length of command line
 */
#define	LSL_CMD_LINE_SIZE	512

/*
 * memory management stuff.
 */
#define LSL_BUFSTART_MARK               0xAABBCCDD
#define LSL_BUFEND_MARK                 0xEEEEFFFF

/*
 * flags for scheduling events.
 */
#define	LSL_NOT_PERIODIC		0x1
#define	LSL_PERIODIC			0x2

/*
 * stuff for lsl_get_range.
 */
#define	LSL_IO_RANGE			0x0
#define	LSL_MEM_RANGE			0x1

/*
 * we keep track of all the hacks in ODI support via these flags.
 * note that these are temporary for ODI spec 3.1 and 3.0 drivers.
 * do not add any for ODI spec 3.2 (or beyond) drivers. we want to
 * be clean starting at ODI spec 3.2, cuz that is what we say we
 * support. these are passed to the kernel via HSMPREFIX_miscflag
 * in Space.c.
 */
#define LSL_SETUP_3_0				0x1
#define LSL_SETUP_3_1				0x2

/*
 * values for HSMPREFIX_setupflags.
 */
#define	LSL_MAP_9_to_2				0x00001

/*
 * memtype values for lsl_alloc.
 */
#define LSL_MEM_NONE                            0x00002	/* no special memory */
#define LSL_MEM_DMA_LOGICALTOPHYSICAL_BELOW16   0x00004	/* spec 3.1 reqs. */
#define LSL_MEM_NONE_BELOW16			0x00008	/* simply below 16meg */
#define LSL_MEM_NONE_PAGEALIGN			0x00010	/* simply page align */
#define	LSL_MEM_NONE_PAGEALIGN_BELOW16		0x00020	/* page aligned */
							/* below 16meg */
#define	LSL_MEM_DMA_PAGEALIGN_BELOW16		0x00040	/* page aligned */
							/* contigous */
							/* below 16meg */
#define	LSL_MEM_DMA_PAGEALIGN			0x00080	/* page aligned */
							/* and contiguous */
#define LSL_MEM_DMA_BELOW16                     0x00100 /* physcontiguous */
							/* below 16 Meg */
#define LSL_MEM_DMA                             0x00200	/* physcontiguous */

#define LSL_MEM_DMA_LOGICAL_ADDR		0x00400	/* physcontiguous */
							/* logical addr */
							/* in ecbs */
/*
 * miscflag values for lsl_alloc.
 */
#define	LSL_KMEM				0x0
#define	LSL_ODIMEM				0x1

#define LSL_NO_INIT_IRQ				0x10000	/* interrupts are */
							/* not enabled during */
							/* all of init */
#define LSL_SGCOUNT_LIMIT5                      0x20000
#define LSL_SGCOUNT_LIMIT10                     0x40000

/*
 * these are not used anymore, but we need to define them for our
 * ODI spec 3.1 UW2.0 drivers to compile.
 */
#define LSL_SETUP_3_0_FLAGS     		0x4
#define LSL_SETUP_3_1_DMA       		0x8

#define LSL_PHYSHIER				15

/*
 * exported routines
 */
extern	void	*lsl_alloc(u_int, int, int, int);
extern	void	*lsl_zalloc(u_int, int, int, int);
extern	void	lsl_free(void *, int, int);
extern	uint_t	lsl_send_complete(struct ECB *ecb);
extern	void	lsl_free_ecb(struct ECB *ecb);
extern	uint_t	lsl_getmax_bufsize(void);
extern	ODISTAT	lsl_sched_int_event(INT_AES *, int type);
extern	ODISTAT	lsl_sched_proc_event(AES *, int type);
extern	void	lsl_cancel_int_event(INT_AES *);
extern	void	lsl_cancel_proc_event(AES *);
extern	UINT32	lsl_sched_timeout(void (* routine)(), void *args,
			UINT32 time, UINT32 type);
extern	void	lsl_cancel_timeout(void *timeid);
extern	void	lsl_serv_events();
extern	ODISTAT	lsl_reg_mlid(MLID_REG *mlidHandlers,
			CONFIG_TABLE *mlidConfigTable,
                 	CONFIG_TABLE *default_mlidConfigTable,
                 	GENERIC_STATS *genericstats, void (* tsmsend)(),
                 	int *brdnum);
extern	void	lsl_dereg_mlid(int brdnum, int frame_num, int last_frame);
extern	void	lsl_dereg_hwoptions(CONFIG_TABLE *cfg);
extern	ECB	*lsl_get_ecb(uint_t, int);
extern	int	lsl_getstack_ecb(LOOKAHEAD *la);
extern	void	lsl_reg_msm(struct lsl_msm_funcs *);
extern	void	lsl_reg_tsm(int tsmType, void *tsmParam);
extern	void	lsl_reg_sr(UINT32 (*SR_Handler)());
extern	void	lsl_dereg_sr();
extern	UINT32	lsl_intr_attach(int intnum, void (*int_vect)());
extern	void	lsl_intr_detach(UINT32 cookie);
extern	void	lsl_ioctl_done(ECB *ecb);
extern	ODISTAT	lsl_register_hardware_options(IO_CONFIG *);
extern	void	lsl_deregister_hardware_options(IO_CONFIG *);
extern	void	lsl_printeaddr(uchar_t addr[]);
extern	void	lsl_readeaddr(lsl_bdconfig_t *bd);

extern	char	*lsl_strcpy(char *, const char *);
extern	char	*lsl_itoh(ulong_t n, char *s);

extern int	lsl_pci_get_bd_info(rm_key_t key, lsl_bdconfig_t * bd,
			char * hsm_name, int brdnum);
extern int	lsl_get_interrupt(rm_key_t key, char *hsm_name, int brdnum,
			int *ret_interrupt);
extern int	lsl_get_range(rm_key_t key, char *hsm_name, int brdnum,
			uint_t flags, ulong_t *ret_start, ulong_t *ret_end);
extern int	lsl_get_dmac(rm_key_t key, char *hsm_name, int brdnum,
			int *ret_dmac);
extern int	lsl_get_slot(rm_key_t key, char *hsm_name, int brdnum,
			cm_num_t * ret_slot);
extern int	lsl_get_busnum(rm_key_t key, char *hsm_name, int brdnum,
			cm_num_t *ret_busnum);
extern int	lsl_get_bustype(rm_key_t key, char *hsm_name, int brdnum,
			cm_num_t *bustype);
extern int	lsl_get_brdid(rm_key_t key, char *hsm_name, int brdnum,
			UINT32 brdidlen, char *ret_brdid);
extern int	lsl_get_uid(rm_key_t key, char *hsm_name, int brdnum,
			cm_num_t *ret_uid);
extern int	lsl_get_odistr(rm_key_t key, char *hsm_name, int brdnum,
			char * cmdline);

extern int	lsl_make_isa_cmdline(lsl_bdconfig_t * hsm_config,
			char *cmdline);
extern int	lsl_make_pci_eisa_mca_cmdline(lsl_bdconfig_t * hsm_config,
			char * cmdline, uchar_t slot, cm_num_t bustype);

#endif _IO_DLPI_LSL_DLPI_LSL_H
