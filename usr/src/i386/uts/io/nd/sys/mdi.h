#ifndef _IO_ND_SYS_MDI_H  /* wrapper symbol for kernel use */
#define _IO_ND_SYS_MDI_H  /* subject to change without notice */

#ident "@(#)mdi.h	28.1"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/param.h>
/* for mblk_t */
#include <io/stream.h>
/* for rm_key_t */
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>

#endif

/*
 * mdi.h header for MAC Driver Interface
 */

/*
 * Primitives for Local Management Services
 */
#define MAC_INFO_REQ		0x00	/* Information Req */
#define MAC_INFO_ACK		0x03	/* Information Ack */
#define MAC_BIND_REQ		0x01	/* Bind dlsap address */
#define MAC_ERROR_ACK		0x05	/* Error acknowledgment */
#define MAC_OK_ACK		0x06	/* Success acknowledgment */
#define MAC_HWFAIL_IND		0x07	/* H/W Failure Indication */
#define MAC_HWSUSPEND_IND	0x08	/* H/W Suspend Indication (DDI8) */
#define MAC_HWRESUME_IND	0x09	/* H/W Resume Indication (DDI8) */

#define MAC_ISDN_MSG		(500+0x0d)	/* ISDN Message, must be */
						/* same as DL_ISDN_MSG in */
						/* scodlpi.h */

/*
 * MAC_ERROR_ACK error return values
 */
#define	MAC_OUTSTATE	0x02	/* Primitive issued in improper state */
#define	MAC_BADPRIM	0x03	/* Primitive received unknown by MDI driver */
#define	MAC_INITFAILED	0x04	/* Physical Link initialization failed */
#define	MAC_HWNOTAVAIL	0x05	/* Hardware isn't available at this time */

/*
 * MDI media types supported
 */
#define	MAC_CSMACD	0x0	/* IEEE 802.3 CSMA/CD network */
#define	MAC_TPR		0x02	/* IEEE 802.5 Token Passing Ring */
#define MAC_FDDI	0x08	/* FDDI on Copper/Fiber */

#define MAC_ISDN_BRI	0x10	/* Basic Rate ISDN	1D(16)+2B(64) */
#define MAC_ISDN_PRI	0x11	/* Primary Rate ISDN	1D(64)+23B(64) */

/*
 * Device minor numbers
 *
 * If there are more than 32 channels associated with a single
 * adapter, extended minors must be used.  See mdevice(F) for
 * the procedure for allocating extended minors.
 *
 * The card number is in the low bits so that single channel
 * cards (Ethernet, Token Ring) have consecutive minor numbering
 * and are not bothered with extended minors.
 */

#define MDI_DEV_TO_CARD(dev)		(minor(dev) & 0x07)
#define MDI_DEV_TO_CHANNEL(dev)		(minor(dev) >> 3)

/*
 * MDI interface primitive definitions.
 *
 * Each primitive is sent as a stream message. The
 * the messages may be viewed as a sequence of bytes that have the
 * following form without any padding. The structure definition
 * of the following messages may have to change depending on the
 * underlying hardware architecture and crossing of a hardware
 * boundary with a different hardware architecture.
 *
 * Fields in the primitives having a name of the form
 * mac_reserved cannot be used and have the value of
 * binary zero, no bits turned on.
 *
 * Each message has the name defined followed by the
 * stream message type (M_PROTO, M_PCPROTO, M_DATA)
 */

/*
 *	LOCAL MANAGEMENT SERVICE PRIMITIVES
 */

/*
 * MAC_INFO_REQ, M_PROTO type
 */
typedef struct mac_info_req {
	ulong	mac_primitive;
} mac_info_req_t;

/*
 * MAC_INFO_ACK, M_PROTO type
 */
typedef struct mac_info_ack {
	ulong		mac_primitive;
	ulong		mac_max_sdu;
	ulong		mac_min_sdu;
	ulong		mac_mac_type;
	ulong		mac_driver_version;
	ulong		mac_if_speed;
} mac_info_ack_t;

/*
 * MAC_BIND_REQ, M_PROTO type
 *    Return MAC_OK_ACK/MAC_ERROR_ACK
 */
typedef struct mac_bind_req {
	ulong	mac_primitive;
	void	*mac_cookie;	/* passed to mdi_valid_mca() */
} mac_bind_req_t;

/*
 * MAC_OK_ACK, M_PROTO type
 */
typedef struct mac_ok_ack {
	ulong	mac_primitive;
	ulong	mac_correct_primitive;
} mac_ok_ack_t;

/*
 * MAC_ERROR_ACK, M_PROTO type
 */
typedef struct mac_error_ack {
	ulong	mac_primitive;
	ulong	mac_error_primitive;
	ulong	mac_errno;
} mac_error_ack_t;

/*
 * MAC_HWFAIL_IND, M_PROTO type
 */
typedef struct mac_hwfail_ind {
	ulong	mac_primitive;
} mac_hwfail_ind_t;

/*
 * MAC_HWSUSPEND_IND, M_PCPROTO type
 */
typedef struct mac_hwsuspend_ind {
	ulong	mac_primitive;
} mac_hwsuspend_ind_t;

/*
 * MAC_HWRESUME_IND, M_PCPROTO type
 */
typedef struct mac_hwresume_ind {
	ulong	mac_primitive;
} mac_hwresume_ind_t;

typedef union MAC_primitives {
	ulong			mac_primitive;
	mac_info_req_t		info_req;
	mac_info_ack_t		info_ack;
	mac_bind_req_t		bind_req;
	mac_ok_ack_t		ok_ack;
	mac_error_ack_t		error_ack;
	mac_hwfail_ind_t	hwfail_ind;
	mac_hwsuspend_ind_t	hwsuspend_ind;
	mac_hwresume_ind_t	hwresume_ind;
} mac_prim_t;

#define	MAC_INFO_REQ_SIZE	sizeof(mac_info_req_t)
#define	MAC_INFO_ACK_SIZE	sizeof(mac_info_ack_t)
#define	MAC_BIND_REQ_SIZE	sizeof(mac_bind_req_t)
#define	MAC_OK_ACK_SIZE		sizeof(mac_ok_ack_t)
#define	MAC_ERROR_ACK_SIZE	sizeof(mac_error_ack_t)
#define	MAC_HWFAIL_IND_SIZE	sizeof(mac_hwfail_ind_t)
#define	MAC_MCA_SIZE		sizeof(mac_mcast_t)
#define MAC_HWSUSPEND_IND_SIZE	sizeof(mac_hwsuspend_ind_t)
#define MAC_HWRESUME_IND_SIZE	sizeof(mac_hwresume_ind_t)
 
#define MDI_VERSION 0x0100	/* Value passed back from MDI driver */

/* Valid ioctls for MDI drivers */
#define MACIOC(x)       	(('M' << 8) | (x))

#define MACIOC_SETMCA		MACIOC(3)	/* Multicast set */
#define MACIOC_DELMCA		MACIOC(4)	/* Multicast delete */
#define MACIOC_GETMCA		MACIOC(6)	/* Get multicast table */
#define MACIOC_GETADDR		MACIOC(8)	/* Get MAC address */
#define MACIOC_SETADDR		MACIOC(9)	/* Set MAC address */
#define MACIOC_GETSTAT		MACIOC(10)	/* Get MAC statistics */
#define MACIOC_PROMISC		MACIOC(11)	/* Set Promiscuous Reception */
#define MACIOC_GETRADDR		MACIOC(12)	/* Get factory MAC Address */
#define MACIOC_CLRSTAT		MACIOC(13)	/* Clear MAC statistics */
#define MACIOC_GETMCSIZ		MACIOC(14)	/* Get multicast table size */
#define MACIOC_SETSTAT		MACIOC(16)	/* Set MAC statistics */
#define MACIOC_SETALLMCA	MACIOC(17)      /* Recieve All Multicast */
#define MACIOC_DELALLMCA	MACIOC(18)      /* Dont Recieve All Multicast */

	/* UNSUPPORTED MACIOC ioctl values - DO NOT USE */
#ifdef LLI31_UNSUPPORTED
#define MACIOC_DIAG		MACIOC(1)	/* Set MAC diagnostics */
#define MACIOC_UNITSEL		MACIOC(2)	/* MAC unit select */
#define MACIOC_CLRMCA		MACIOC(5)	/* Flush multicast table */
#define MACIOC_GETIFSTAT	MACIOC(7)	/* Dump BSD statistics */
#define MACIOC_HWDEPEND		MACIOC(15)	/* Hardware dependent */
#define MACIOC_LOCALSTAT	MACIOC(254)	/* dump statistics */
#endif

/*
 * Ethernet statistics structure for the MACIOC_GETSTAT ioctl
 */
typedef struct mac_stats_eth {
	ulong mac_align;            /* Bad Alignment Received */
	ulong mac_badsum;           /* Bad Checksum Received */
	ulong mac_sqetesterrors;    /* NEW SQE Test Errors */
	ulong mac_frame_def;        /* #Frames Deferred (not dropped) */
	ulong mac_oframe_coll;      /* #Times coll' det'ed >512bits into frame*/
	ulong mac_xs_coll;      /* #Frames(tx) Dropped due Excess Collisions */
	ulong mac_tx_errors;    /* NEW #Internal MAC Transmit Errors */
	ulong mac_carrier;      /* #Frames Dropped(tx) Due To Lost Carrier */
	ulong mac_badlen;           /* #Frames(rx) Dropped due >MAX_MACPDUSZ */
	ulong mac_no_resource;      /* #Internal MAC Receive Errors */
	
		/* NEW Table of #frames involved in N collisions, where */
		/* N-1 is the table index (e.g. mac_colltable[0] == #frames */
		/* involved in 1 collision) */
	ulong mac_colltable[16];
	
	/* FIELDS NOT REQUIRED BY MIB */
	ulong mac_spur_intr;        /* Spurious interrupts */
	ulong mac_frame_nosr;       /* Dropped Due To Lack of STREAMS */
		/* FIELDS NOT REQUIRED BY MIB, ARE THEY USEFUL?? */
	ulong mac_baddma;	/* Errors Due To Over/Under Runs */
	ulong mac_timeouts;	/* Device Timeouts */
} mac_stats_eth_t;

/*
 * Token-Ring statistics structure for the MACIOC_GETSTAT ioctl
 */
typedef	struct mac_stats_tr {
	ulong mac_ringstatus;	/* Bit field holding T-R status */
	unchar mac_funcaddr[6];	/* Currently set functional address */
	unchar mac_upstream[6];	/* MAC addr of upstream neighbor */
	unchar mac_actmonparticipate;	/* Active Mon selection participation */
	ulong mac_lineerrors;	/* #Frames copied with FCS error */
	ulong mac_bursterrors;	/* #times no transitions detected */
	ulong mac_acerrors;	/* #times stn. detected with bad A/C response */
	ulong mac_aborttranserrors;	/* #frames aborted on transmission */
	ulong mac_internalerrors;	/* #internal errors */
	ulong mac_lostframeerrors;
	ulong mac_receivecongestions;	/* #frames received and then dropped */
	ulong mac_framecopiederrors;	/* #times duplicate address detected */
	ulong mac_tokenerrors;	/* #times Act' Mon detects token err */
	ulong mac_softerrors;	/* #errors recoverable by MAC protocol */
	ulong mac_harderrors;	/* #times MAC beacon frames tx'ed or rx'ed */
	ulong mac_signalloss;	/* #times detection of ring signal lost */
	ulong mac_transmitbeacons;	/* #MAC beacon frames sent */
	ulong mac_recoverys;	/* #times MAC claim frames tx'ed or rx'ed */
	ulong mac_lobewires;	/* #times open/short circuit in lobe path */
	ulong mac_removes;	/* #MAC remove-ring-stn frames received */
	ulong mac_statssingles;	/* #times only stn. on ring */
	ulong mac_frequencyerrors;	/* #times signal freq. out of range */

	/* FIELDS NOT REQUIRED BY MIB */
	ulong mac_badlen;           /* #Frames(tx) Dropped due >MAX_MACPDUSZ */
	ulong mac_spur_intr;        /* Spurious interrupts */
	ulong mac_frame_nosr;       /* Dropped Due To Lack of STREAMS */
		/* FIELDS NOT REQUIRED BY MIB, ARE THEY USEFUL?? */
	ulong mac_baddma;	/* Errors Due To Over/Under Runs */
	ulong mac_timeouts;	/* Device Timeouts */
} mac_stats_tr_t;

/*
 * FDDI statistics structure for the MACIOC_GETSTAT ioctl
 */
typedef struct mac_stats_fddi {
	unchar smt_station_id[8];		/* SMT Attributes */
	ulong smt_op_version_id;
	ulong smt_hi_version_id;
	ulong smt_lo_version_id;
	unchar smt_user_data[32];		/* read-write */
	ulong smt_mib_version_id;
	ulong smt_mac_cts;
	ulong smt_non_master_cts;
	ulong smt_master_cts;
	ulong smt_available_paths;
	ulong smt_config_capabilities;
	ulong smt_config_policy;		/* read-write */
	ulong smt_connection_policy;		/* read-write */
	ulong smt_t_notify;			/* read-write */
	ulong smt_stat_rpt_policy;		/* read-write */
	ulong smt_trace_max_expiration;		/* read-write */
	ulong smt_bypass_present;
	ulong smt_ecm_state;
	ulong smt_cf_state;
	ulong smt_remote_disconnect_flag;
	ulong smt_station_status;
	ulong smt_peer_wrap_flag;
	ulong smt_time_stamp;	/* RFC1512 32bit signed int, SMT 7.2c 64bits */
	ulong smt_transition_time_stamp;	/* RFC1512 32b, SMT 7.2c 64b */
	ulong mac_frame_status_functions;	/* MAC Attributes */
	ulong mac_t_max_capability;
	ulong mac_tvx_capability;
	ulong mac_available_paths;
	ulong mac_current_path;
	unchar mac_upstream_nbr[6];
	unchar mac_downstream_nbr[6];
	unchar mac_old_upstream_nbr[6];
	unchar mac_old_downstream_nbr[6];
	ulong mac_dup_address_test;
	ulong mac_requested_paths;		/* read-write */
	ulong mac_downstream_port_type;
	unchar mac_smt_address[6];
	ulong mac_t_req;
	ulong mac_t_neg;
	ulong mac_t_max;
	ulong mac_tvx_value;
	ulong mac_frame_cts;
	ulong mac_copied_cts;
	ulong mac_transmit_cts;
	ulong mac_error_cts;
	ulong mac_lost_cts;
	ulong mac_frame_error_threshold;	/* read-write */
	ulong mac_frame_error_ratio;
	ulong mac_rmt_state;
	ulong mac_da_flag;
	ulong mac_una_da_flag;
	ulong mac_frame_error_flag;
	ulong mac_ma_unitdata_available;
	ulong mac_hardware_present;
	ulong mac_ma_unitdata_enable;		/* read-write */
	ulong path_tvx_lower_bound;		/* read-write */
	ulong path_t_max_lower_bound;		/* read-write */
	ulong path_max_t_req;			/* read-write */
	ulong path_configuration[8];		/* PATH Attributes */
	ulong port_my_type[2];			/* PORT Attributes */
	ulong port_neighbor_type[2];
	ulong port_connection_policies[2];	/* read-write */
	ulong port_mac_indicated[2];
	ulong port_current_path[2];
	unchar port_requested_paths[3*2];	/* read-write */
	ulong port_mac_placement[2];
	ulong port_available_paths[2];
	ulong port_pmd_class[2];
	ulong port_connection_capabilities[2];
	ulong port_bs_flag[2];
	ulong port_lct_fail_cts[2];
	ulong port_ler_estimate[2];
	ulong port_lem_reject_cts[2];
	ulong port_lem_cts[2];
	ulong port_ler_cutoff[2];		/* read-write */
	ulong port_ler_alarm[2];		/* read-write */
	ulong port_connect_state[2];
	ulong port_pcm_state[2];
	ulong port_pc_withhold[2];
	ulong port_ler_flag[2];
	ulong port_hardware_present[2];
} mac_stats_fddi_t;

/*
 * ISDN statistics structure for the MACIOC_GETSTAT ioctl
 *
 * The ISDN statistics structures defined below are taken from RFC 2127
 * ISDN Management Information Base using SMIv2.
 *
 */
typedef struct isdn_dchannel_stats {
	ulong	speed;
	ulong	mtu;
	ulong	LapdOperStatus;
	ulong	InOctets;
	ulong	InUcastPkts;
	ulong	InBroadcastPkts;
	ulong	InDiscards;
	ulong	InErrors;
	ulong	InUnkownProtos;
	ulong	OutOctets;
	ulong	OutUcastPkts;
	ulong	OutBroadcastPkts;
	ulong	OutDiscards;
	ulong	OutErrors;
	ulong	filler1;	
	ulong	filler2;
	ulong	filler3;
	ulong	filler4;
} isdn_dchannel_stats_t;

typedef struct isdn_signaling_stats {
	ulong	SignalingProtocol;		/* enum, see RFC 2127 */
	unchar	CallingAddress[2][32];
	unchar	CallingSubAddress[2][32];
	ulong	BchannelCount;
	ulong	InCalls;
	ulong	InConnected;
	ulong	OutCalls;
	ulong	OutConnected;
	ulong	ChargedUnits;
	ulong	filler1;
	ulong	filler2;
	ulong	filler3;
	ulong	filler4;
} isdn_signaling_stats_t;

typedef struct isdn_bchannel_stats {
	ulong	OperStatus;			/* enum, see RFC 2127 */
	ulong	AppID;
	ulong	ChannelNumber;
	unchar	PeerAddress[32];
	unchar	PeerSubAddress[32];
	ulong	CallOrigin;
	ulong	InfoType;			/* enum, see RFC 2127 */
	time_t	CallSetupTime;
	time_t	CallConnectTime;
	ulong	ChargedUnits;
	ulong	filler1;
	ulong	filler2;
	ulong	filler3;
	ulong	filler4;
} isdn_bchannel_stats_t;

typedef struct mac_stats_isdn {
	isdn_dchannel_stats_t	dchannel;
	isdn_signaling_stats_t	signaling;
	isdn_bchannel_stats_t	bchannel[30];
} mac_stats_isdn_t;

/* helpful defintions for reporting ISDN statistics */

/* enumeration for dchannel.LapdOperStatus */
/* RFC 2127 isdnLapdOperStatus             */

/* choose:			   ndstat reports: */
#define	LAPD_NONE	0	/* Device Not Reporting LAPD Status */
#define	LAPD_INACTIVE	1	/* Inactive */
#define	LAPD_L1ACTIVE	2	/* Layer 1 Active */
#define	LAPD_L2ACTIVE	3	/* Layer 2 Active */

/* emuneration for signaling.SignalingProtocol */
/* RFC 2127 IsdnSignalingProtocol              */

/* choose:			   ndstat reports: */
#define	SIGPROT_NONE	0	/* Protocol Information Not Available */
#define	SIGPROT_Q931	2	/* ITU DSS1 Q.931 */
#define	SIGPROT_ETSI	3	/* Europe / ETSI */
#define	SIGPROT_UKDASS2	4	/* U.K. / DASS2 (PRI) */
#define	SIGPROT_USA4ESS	5	/* U.S.A. / AT&T 4ESS */
#define	SIGPROT_USA5ESS	6	/* U.S.A. / AT&T 5ESS */
#define	SIGPROT_USADMS1	7	/* U.S.A / Northern Telecom DMS100 */
#define	SIGPROT_USADMS2	8	/* U.S.A / Northern Telecom DMS250 */
#define	SIGPROT_USANI1	9	/* U.S.A National ISDN 1 (BRI) */
#define	SIGPROT_USANI2	10	/* U.S.A National ISDN 2 (BRI, PRI) */
#define	SIGPROT_FRNVN2	12	/* France / VN2 */
#define	SIGPROT_FRNVN3	13	/* France / VN3 */
#define	SIGPROT_FRNVN4	14	/* France / VN4 (ETSI with changes) */
#define	SIGPROT_FRNVN6	15	/* France / VN6 (ETSI with changes) */
#define	SIGPROT_JPNKDD 	16	/* Japan / KDD */
#define	SIGPROT_JPNNI64	17	/* Japan / NTT INS64 */
#define	SIGPROT_JPNNI15	18	/* Japan / NTT INS1500 */
#define	SIGPROT_GER1TR6	19	/* Germany / 1TR6 (BRI, PRI) */
#define	SIGPROT_GERSIEM	20	/* Germany / Siemens HiCom CORNET */
#define	SIGPROT_AUTS013	21	/* Australia / TS013 */
#define	SIGPROT_AUTS014	22	/* Australia / TS014 */
#define	SIGPROT_QSIG	23	/* Q.SIG */
#define	SIGPROT_SWNET2	24	/* SwissNet-2 */
#define	SIGPROT_SWNET3	25	/* SwissNet-3 */

/* enumeration for bchannel.OperStatus */
/* RFC 2127 isdnBearerOperStatus       */

/* choose:			   ndstat reports: */
#define	BSTATUS_NONE	0	/* Device not reporting B channel statistics */
#define	BSTATUS_IDLE	1	/* Idle */
#define	BSTATUS_CNNCTNG	2	/* Connecting */
#define	BSTATUS_CNNCTD	3	/* Connected */
#define	BSTATUS_ACTIVE	4	/* Active */

/* enumeration for bchannel.CallOrigin */
/* RFC 2127 isdnBearerCallOrigin       */

/* choose:			   ndstat reports */
#define BCALLORIG_OUT	1	/* Outgoing */
#define	BCALLORIG_IN	2	/* Incoming */
#define	BCALLORIG_CBACK	3	/* Callback */

/* enumeration for bchannel.InfoType */
/* RFC 2127 isdnBearerInfoType       */

/* choose:			   ndstat reports */
#define	BINFOTYPE_NONE	0	/* Unknown */
#define	BINFOTYPE_SPCH	2	/* Speech */
#define	BINFOTYPE_UDI	3	/* Unrestricted Digital Information */
#define	BINFOTYPE_UDI56	4	/* Unrestricted Digital Information 56Kbit/s Rate Adoption */
#define	BINFOTYPE_RDI	5	/* Restricted Digital Information */
#define	BINFOTYPE_3KAUD	6	/* 3.1 kHz Audio */
#define	BINFOTYPE_7KAUD	7	/* 7 kHz Audio */
#define	BINFOTYPE_VIDEO	8	/* Video */
#define	BINFOTYPE_PCKT	9	/* Packet Mode */


/*
 *   MACIOC_SETSTAT - support for writeable MIBs.
 */
	/* MACSTATs for the mac_flags bit field */
#define MACSTAT_ACTMONPARTICIPATE 0 	/* Active Mon selection participation */

typedef	long macflags_t[10];

#define NFLBITS	(sizeof(long) * 8)	/* bits per mask */

#define	MACFL_SET(n, p)	((p)->mac_flags[(n)/NFLBITS] |= (1L << ((n) % NFLBITS)))
#define	MACFL_CLR(n, p)	((p)->mac_flags[(n)/NFLBITS] &= ~(1L<< ((n) % NFLBITS)))
#define	MACFL_ISSET(n, p) ((p)->mac_flags[(n)/NFLBITS] &(1L << ((n) % NFLBITS)))
#define MACFL_ZERO(p)	memset((char *)((p)->mac_flags), '\0', sizeof(*(p)))

typedef struct mac_setstat {
	macflags_t	mac_flags;
	union {
		mac_stats_eth_t		eth;
		mac_stats_tr_t		tr;
		mac_stats_fddi_t 	fddi;
		mac_stats_isdn_t	isdn;
	} mac_stats;
} mac_setstat_t;

/*
 * Type: macaddr_t
 *
 * Purpose:
 *   Store a network address.
 */
#define MDI_MACADDRSIZE	6
typedef unsigned char	macaddr_t[MDI_MACADDRSIZE];

/*
 * multicast address state structure - mdi_get_mctable()
 */
typedef struct mac_mcast {
	ulong	mac_all_mca;		/* all multicasts enabled */
	ulong	mac_mca_count;		/* number of multicast addrs enabled */ 
	ulong	mac_mca_length;		/* size (# bytes) of multicast table */
	ulong	mac_mca_offset;		/* beginning of multicast table */
} mac_mcast_t;

/*
 * MDI TX Interface routines/structures
 */

#if defined(_KERNEL) || defined(_KMEMUSER)
#if defined(__STDC__) && !defined(_NO_PROTOTYPE)
typedef int (*mdi_tx_checkavail_t)(void * handle);
typedef void (*mdi_tx_processmsg_t)(void * handle, int txr, mblk_t * mp);
#else
typedef int (*mdi_tx_checkavail_t)();
typedef void (*mdi_tx_processmsg_t)();
#endif

#define MDIDRIVER_HIER_BASE  1  /* lock hierarchy for MDI drivers starts here */

		typedef struct {
    queue_t 			*q;
    int				active;
    lock_t			*txlock;
    mdi_tx_checkavail_t		tx_avail;
    mdi_tx_processmsg_t		process_msg;
    void 			*handle;
    int                         flags;
} mdi_tx_if_t;

#define MDI_TX_MP_SAFE		0x00
#define MDI_TX_MP_MT		0x01

/*
 * MDI NetBoot/ksl stuff
 */
#define MDI_BUS_ISA		0x00000001
#define MDI_BUS_MCA		0x00000002
#define MDI_BUS_EISA		0x00000004
#define MDI_BUS_PCI		0x00000010
#define MDI_BUS_PCMCIA		0x10000000 /* not defined in arch.h yet */

/*
 * lots of adapter docs mix up media types, cabling, and connector type
 * so a small bit of the media mess we live with gets propagated below
 */
#define MDI_MEDIA_AUTO		0x00000001
#define MDI_MEDIA_AUI		0x00000002
#define MDI_MEDIA_BNC		0x00000004
#define MDI_MEDIA_TP		0x00000008
#define MDI_MEDIA_COAX		0x00000010
#define MDI_MEDIA_BNC_TP	0x00000020
#define MDI_MEDIA_TP_COAX	0x00000040

#define MDI_SET_BOOTPARAMS	0
#define MDI_GET_BOOTPARAMS	1

typedef struct {
	char	*lbl;
    int		val;
} mdi_netboot_t;

/* _verify(D2) enhancements for ISA EEPROM/NVRAM autodetection/writeback */
#define MDI_ISAVERIFY_UNKNOWN 0
#define MDI_ISAVERIFY_GET 1 /* retrieve params from eeprom, call again later */
#define MDI_ISAVERIFY_SET 2 /* retrieve params from resmgr & set in firmware */
#define MDI_ISAVERIFY_GET_REPLY 3 /* 2nd time: set params in resmgr from info*/
#define MDI_ISAVERIFY_TRADITIONAL 4 /* normal verify: just confirm io address*/

/*
 * memtype values for dlpibase_alloc.
 */
#define DLPIBASE_MEM_NONE                       0x00002	/* no special memory */
/* DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16 is unsupported */
#define DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16 0x00004 /* spec 3.1 reqs. */
#define DLPIBASE_MEM_NONE_BELOW16		0x00008	/* simply below 16meg */
#define DLPIBASE_MEM_NONE_PAGEALIGN		0x00010	/* simply page align */
#define	DLPIBASE_MEM_NONE_PAGEALIGN_BELOW16	0x00020	/* page aligned */
							/* below 16meg */
#define	DLPIBASE_MEM_DMA_PAGEALIGN_BELOW16	0x00040	/* page aligned */
							/* contigous */
							/* below 16meg */
#define	DLPIBASE_MEM_DMA_PAGEALIGN		0x00080	/* page aligned */
							/* and contiguous */
#define DLPIBASE_MEM_DMA_BELOW16                0x00100 /* physcontiguous */
							/* below 16 Meg */
#define DLPIBASE_MEM_DMA                        0x00200	/* physcontiguous */

#define DLPIBASE_MEM_DMA_LOGICAL_ADDR		0x00400	/* physcontiguous */
							/* logical addr */
							/* in ecbs */
/*
 * miscflag values for dlpibase_alloc.
 */
#define	DLPIBASE_KMEM				0x0
/* DLPIBASE_ODIMEM is unsupported */
#define	DLPIBASE_ODIMEM				0x1

#define DLPIBASE_NO_INIT_IRQ			0x10000	/* interrupts are */
							/* not enabled during */
							/* all of init */
#define DLPIBASE_SGCOUNT_LIMIT5                 0x20000
#define DLPIBASE_SGCOUNT_LIMIT10                0x40000

		/* MDI Library functions */
#if defined(__STDC__) && !defined(_NO_PROTOTYPE)
extern void mdi_set_frtn(frtn_t *, void *, int);
extern int mdi_open_file(char *, uchar_t **, int *);
extern int mdi_open_file_i(char *, uchar_t **, int *, int, int, int);
extern void mdi_close_file(void *ptr);
extern void mdi_close_file_i(void *, int, int);
extern void mdi_printcfg(const char *, unsigned, unsigned, 
                         int, int, const char *, ...);
extern int mdi_AT_verify(rm_key_t rm_key, int *getset,
                         ulong_t *sioa, ulong_t *eioa, int *vector,
                         ulong_t *scma, ulong_t *ecma, int *dmac);
extern mblk_t *	mdi_get_mctable(void * mac_cookie);
extern boolean_t mdi_get_unit(rm_key_t, uint_t *);
extern int	mdi_addrs_equal(macaddr_t addr1, macaddr_t addr2);
extern u_short	mdi_htons(u_short);
extern u_short	mdi_ntohs(u_short);
extern void	mdi_do_loopback(queue_t *, mblk_t *, int);
extern void	mdi_macerrorack(queue_t *, long, int);
extern void	mdi_macokack(queue_t *, long);
extern int      mdi_tx_if_init(mdi_tx_if_t * txint,
                            mdi_tx_checkavail_t tx_avail,
                            mdi_tx_processmsg_t process_msg,
                            void * handle,
                            int flags,
                            uchar_t hierarchy);
extern void     mdi_tx_deinit(mdi_tx_if_t * txint);
extern void     mdi_tx_enable(mdi_tx_if_t * txint,
                              queue_t *q);
extern void     mdi_tx_disable(mdi_tx_if_t * txint);
extern void     mdi_tx_if(mdi_tx_if_t * txint, mblk_t * msg);
extern u_char * mdi_end_of_contig_segment(u_char *, u_char *);
extern int	mdi_isdma(u_char *);
extern mblk_t * mdi_dma_allocb(u_long);
extern void	mdi_dma_freeb(u_long *);
extern int	mdi_hw_suspended(queue_t *, rm_key_t);
extern int	mdi_hw_resumed(queue_t *, rm_key_t);
#endif
#endif  /* if defined(_KERNEL) || defined(_KMEMUSER) */
 
#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_ND_SYS_MDI_H */
