#ifndef _IO_ND_SYS_DLPIMOD_H  /* wrapper symbol for kernel use */
#define _IO_ND_SYS_DLPIMOD_H  /* subject to change without notice */

#ident "@(#)dlpimod.h	29.3"
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
/* for atomic_int_t, lock_t */
#include <mem/kmem.h>
#include <util/ksynch.h>
/* for cred_t */
#include <proc/cred.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/kmem.h>
#include <sys/ksynch.h>
#include <sys/cred.h>

#endif



#if defined(_KERNEL) || defined(_KMEMUSER)

#define dlpi_frame_test	/* for dlpi_biloop support */

typedef struct per_card_info per_card_info_t;
typedef struct per_sap_info per_sap_info_t;

/******************************************************************************
 * Media dependent frame processing information:
 * This section is used to store information about a frame as it is being
 * processed.
 ******************************************************************************/
struct per_frame_info {
	unchar *	frame_dest;	/* Ptr to MAC Destination Address */
	unchar		dest_type;	/*FR_UNICAST/FR_MULTICAST/FR_BROADCAST*/
	unchar		frame_dsap;	/* LLC DSAP (if llcmode is LLC_1) */
	unchar *	frame_src;	/* Ptr to MAC Source Address */
	unchar		frame_ssap;	/* LLC SSAP (if llcmode is LLC_1) */

	ulong		frame_type;	/* FR_ETHER_II/FR_LLC/FR_XNS */
	ulong		frame_sap;	/* SAP Bound */

	uint		frame_len;	/* Frame length */
	unchar *	frame_data;	/* Pointer to MAC Data */

	uint		route_present;	/* Route Information field present */
	uint		route_len;	/* Source Route length (if any) */
	unchar *	route_info;	/* Pointer to Route Info field */

	mblk_t		*unitdata;	/* DL_UNITDATA(REQ/IND) received */
};
/* Valid values for dest_type field */
#define FR_UNICAST	0
#define FR_MULTICAST	1
#define FR_BROADCAST	2

/******************************************************************************
 * DLPI Module Information
 * This section is used to store information used internally by the DLPI
 * module.
 ******************************************************************************/
struct sap_counters {
    atomic_int_t  *bcast;         /* #Frames Broadcast */
    atomic_int_t  *mcast;         /* #Frames Multicast */

    atomic_int_t  *ucast_xid;     /* #LLC XID Frames (Unicast) */
    atomic_int_t  *ucast_test;    /* #LLC TEST Frames (Unicast) */
    atomic_int_t  *ucast;         /* #Frames Unicast (not covered above) */

    atomic_int_t  *error;         /* #Frames with errors */
    atomic_int_t  *octets;        /* #MAC Octets sent/received */
};

struct per_sap_info {
	/* BIND Related information */
	ulong			sap_type;	/* FR_ETHER_II/FR_LLC/FR_XNS */
	ulong			sap;		/* Bind address */
	ulong			sap_state;	/* SAP_FREE/ALLOC/BOUND */
	ulong			sap_protid;	/* used for SNAP SAPs only */

	queue_t			*up_queue;	/* (READ)queue to proto'stk */
	per_card_info_t		*card_info;

	ulong			srmode;		/* SR_NON/SR_AUTO/SR_STACK */
	ulong			llcmode;	/* LLC_OFF/LLC_1 */
	ulong			dlpimode;	/* LLI31_STACK/DLPI20_STACK */
	
	ulong			llcxidtest_flg;	/* dl_xidtest_flg in DL_BIND */

	struct sap_counters	sap_tx;		/* Statistics for sends */
	struct sap_counters	sap_rx;		/* Statistics for receives */

		/* DLPI PRIMITIVE currently being processed */
	ulong			dlpi_prim;
	ulong			NCCI;		/* ISDN connection          */
	int			unconf_data_b3;	/* unconfirmed DATA_B3 msgs */
	ulong			all_mca:1,	/* recv all multicast addrs */
				mdata:1,	/* M_DATA messages upstream */
				blk_data_b3:1;	/* blocked on DATA_B3 msgs  */

		/* the filter currently in use */
	mblk_t			*filter[2];

		/* credentials */
	cred_t		*crp;
};

/*
 * Type: dlpi_init_t
 *
 * Purpose:
 *   Configurable variables that get set in the Space.c file.
 *   Its real purpose is to have the initializers be a manageable
 *   enough size to avoid errors.
 */
typedef struct dlpi_init_s {
        clock_t         txmon_consume_time;  /* # ticks to get first message 
                                              * off queue
                                              */
        u_int           txmon_enabled;       /* 0 or 1 */
        u_int           txmon_count_limit;   /* # times over txmon_consume_time
                                              * before MACIOC_HWFAIL_IND sent
                                              */
} dlpi_init_t;

/*
 * Type: txmon_t
 *
 * Purpose:
 *   Transmit traffic monitoring data.
 * Fields:
 *   trylock            : lock access to code
 *   last_mblk          : the mblk at head of queue last time we checked
 *   last_send_time     : time of last time we thought the 
 *                        card consumed a message ie. delivered
 *                        a packet to the card
 *   restart_in_progress: Counter used by txmon to send HWFAIL
 *                        to dlpi daemon, and is expecting it
 *                        to re-open/I_LINK mdi driver
 *   nrestarts          : number of txmon restarts
 */

          /* hierarchies, should be in new file dlpimdihier.h */

/* LSL/MSM end at 25 so we'll start at 26.  We try to preserve lock hierarchy
 * as an MDI driver can call mdi_valid_mca/mdi_get_mctable with its 
 * own locks held, typically with hierarchy values starting at 1.
 */
#define DLPI_HIER_BASE   26
#define DLPIOPENHIER		(DLPI_HIER_BASE+0) /* open lock */
#define DLPIHIER		(DLPI_HIER_BASE+1) /* lock hierarchy - no locks held */
#define TXMONHIER		(DLPI_HIER_BASE+2) /* hierarchy: txmon trylock acquired */

typedef struct txmon_s {
        lock_t                  *trylock;
        mblk_t                  *last_mblk;
        clock_t                 last_send_time;
        u_int                   restart_in_progress;
        u_int                   nrestarts;
        u_int                   largest_qsize_found;
} txmon_t;

/* Valid values for sap_state */
#define SAP_FREE	0	/* SAP not in use */
#define SAP_ALLOC	1	/* SAP allocated, but not in use */
#define SAP_BOUND	2	/* SAP allocated, and in use */

/* Valid values for dlpimode */
#define	LLI31_STACK	0	/* LLI 3.1 compatible stack */
#define DLPI20_STACK	1	/* DLPI 2.0 compatible stack */

typedef enum
{
    MIOCRET_PUTBQ,	/* Put the message back on the queue for */
			/*	later processing */
    MIOCRET_DONE,	/* The ioctl has been handled (ACK/NAK) */
    MIOCRET_NOT_MINE	/* This ioctl is not recognized by the media */
			/*	manager. Send it to the MDI driver */
} miocret_t;

typedef enum
{
    MPRIMRET_PUTBQ,	/* Put the message back on the queue for */
			/*	later processing */
    MPRIMRET_DONE,	/* The primitive has been handled */
    MPRIMRET_NOT_MINE	/* This primitive is not recognized by the media */
			/*	manager. */
} mprimret_t;


typedef void	(*media_init_t)();
typedef void    (*media_close_t)();
typedef void	(*media_halt_t)();
typedef int	(*media_svc_reg_t)();
typedef int	(*media_svc_rcv_t)();
typedef void	(*media_svc_close_t)();

typedef int	(*make_hdr_t)();
typedef void	(*rx_parse_t)();

typedef mprimret_t	(*media_wr_prim_t)();
typedef mprimret_t	(*media_rd_prim_t)();
typedef mprimret_t	(*media_wr_data_t)();

typedef int	(*rx_errors_t)();
typedef int	(*tx_errors_t)();

typedef miocret_t	(*media_ioctl_t)();

typedef struct per_media_info
{
    int			hdr_sz;
    int			stats_sz;
    ulong		media_flags;		/* Used by dlpi */
    ulong		media_private_flags;	/* Used by media mgr */

    media_init_t	media_init;		/* These may be NULL */
    media_close_t       media_close;
    media_halt_t	media_halt;
    media_svc_reg_t	media_svc_reg;
    media_svc_rcv_t	media_svc_rcv;
    media_svc_close_t	media_svc_close;

    make_hdr_t		make_hdr;		/* These may not be NULL */
    rx_parse_t		rx_parse;

    media_wr_prim_t	media_wr_prim;		/* These may be NULL */
    media_rd_prim_t	media_rd_prim;
    media_wr_data_t	media_wr_data;

    rx_errors_t		rx_errors;		/* These may be NULL */
    tx_errors_t		tx_errors;
    media_ioctl_t	media_ioctl;
} per_media_info_t;

#define MAX_MEDIA_TYPE	0x20		/* See MAC_CSMACD, MAC_TPR ... */
#define HAVE_MEDIA_SUPPORT(type)	mdi_media_info[type].make_hdr

#define MEDIA_MAGIC	0x5a5a0100	/* Registration validation - v1.00 */

/*
 * Values for the per_media_info_t->media_flags
 */
#define MEDIA_SOURCE_ROUTING	0x01
#define MEDIA_ETHERNET_II	0x02
#define MEDIA_XNS		0x04
#define MEDIA_LLC		0x08
#define MEDIA_SNAP		0x10
#define MEDIA_ISDN		0x0100

typedef struct sap_mca {
    per_sap_info_t	*sp;
    struct sap_mca	*next_sap;
} sap_mca_t;

typedef struct card_mca {
    unsigned char	mca[6];			/* multicast address */
    struct card_mca	*next_mca;
    sap_mca_t		sap_mca;
} card_mca_t;

typedef struct cp_ioctl_blk {
    per_sap_info_t	*ioctl_sap;		/* SAP issuing the ioctl */
    void		(*ioctl_handler)();	/* IOCACK/NAK handler */
} cp_ioctl_blk_t;

struct per_card_info {
    lock_t		*lock_sap_id;		/* lock sap id allocations */
    lock_t		*lock_card;		/* lock card struct */
    rwlock_t		*lock_mca;		/* multicasting lock */
    int			nopens;			/* #opens at LLI layer */

    ulong		maxsaps;		/* SAP Table size */
    ulong		maxroutes;		/* Route Table size */

    atomic_int_t	*rxunbound;

    per_sap_info_t	*sap_table;		/*ptr to 1st SAP Table struct */
    struct route_table	*route_table;		/*ptr to route table */

    /* MDI Interface Information */
    unsigned char	local_mac_addr[6];
    queue_t		*down_queue;		/* (WRITE)Queue to MDI driver */
    ulong		mdi_driver_ready:1,
			dlpidRegistered:1,
    			set_mac_addr_pending:1,	/* MAC address being set */
			all_mca:1,		/* receive all multicasts */
			disab_ether:1;		/* disable EthernetII framing */

    ulong		mac_driver_version;	/* 0x3200 */
    ulong		mac_media_type;		/* Ethernet/T-R/FDDI */
    ulong		mac_max_sdu;		/* SDU MAX at MDI layer */
    ulong		mac_min_sdu;		/* SDU MIN at MDI layer */
    ulong		mac_ifspeed;		/* Speed of i/f in bits/sec */

    per_media_info_t	*media_specific;	/* Media switch */
    queue_t		*media_svcs_queue;	/* The daemon connection */
    void		*media_private;		/* media mgr private data */

    per_sap_info_t	*control_sap;		/* SAP issuing I_(UN)LINK */
    mblk_t		*hwfail_waiting;	/* storage for HWFAIL_IND */
    int			(*mdi_primitive_handler)();

    /* MDI IOCTL Issue/Wait-forACK/NAK information */
    per_sap_info_t	*ioctl_sap;		/* SAP issuing the ioctl */
    mblk_t		*dlpi_iocblk;		/* ioctl rx'ed from DLPI user*/
    mblk_t		*ioctl_queue;		/* queue of pending ioctls */

    /* Info about the ioctl being issued to the MDI driver */
    void		(*ioctl_handler)();	/* IOCACK/NAK handler */

    struct sap_counters	old_tx, old_rx;
    struct txmon_s	txmon;			/* transmit traffic monitor */
    dlpi_init_t		*init;			/* Space.c settable stuff */

    unsigned char	hw_mac_addr[6];		/* factory MAC address */
    card_mca_t		**mcahashtbl;		/* MCA address hash table */
    ulong		nmcahash;		/* number of MCA hash buckets */

#ifdef dlpi_frame_test
    /* enables dlpi write loopback to the sap that requested loopback */
    per_sap_info_t	*dlpi_biloopmode; 
#endif

    ulong		issuspended;		/* DDI8 MDI driver suspended? */
    atomic_int_t	suspenddrops;		/* # M_DATA dropped when susp */
};

/******************************************************************************
 * Macros
 ******************************************************************************/
#define MDI_DRIVER_NAME(cp) ((cp)->down_queue->q_next->q_qinfo->qi_minfo->mi_idname)
#define PROTOCOL_STACK_NAME(sp) ((sp)->up_queue->q_next->q_qinfo->qi_minfo->mi_idname)

/*
 * MP Lock Macros
 */
#define LOCK_CARDINFO(cp)         LOCK((cp)->lock_card, plstr)
#define UNLOCK_CARDINFO(cp, spl)  UNLOCK((cp)->lock_card, spl)
#define RDLOCK_MCA(cp)            RW_RDLOCK((cp)->lock_mca, plstr)
#define WRLOCK_MCA(cp)            RW_WRLOCK((cp)->lock_mca, plstr)
#define UNLOCK_MCA(cp, spl)       RW_UNLOCK((cp)->lock_mca, spl)
#define LOCK_SAP_ID(cp)           LOCK((cp)->lock_sap_id, plstr)
#define UNLOCK_SAP_ID(cp, spl)    UNLOCK((cp)->lock_sap_id, spl)
#define ADD_STAT(s,v)             ATOMIC_INT_ADD(s, ATOMIC_INT_READ(v))
#define SET_STAT(s,v)             ATOMIC_INT_WRITE(s, ATOMIC_INT_READ(v))

/*
 * MP DLPI Module switch
 * - set to non-zero, means that the dlpi's queues will be 
 *   marked as MP-safe
 */
#define DLPI_IS_MP              1

#endif    /* if defined(_KERNEL) || defined(_KMEMUSER) */
/*
 ******************************************************************************
 * DLPId Ioctls
 ******************************************************************************
 */

#define DLPIDIOC(x)       	(('D' << 8) | (x))
#define DLPID_REGISTER		DLPIDIOC(1)
#define DLPI_SETFILTER		DLPIDIOC(2)

/*
 ******************************************************************************
 * Media Support
 ******************************************************************************
 */

int mdi_register_media();

#define DLPIMIOC(x)       	(('m' << 8) | (x))
#define DLPI_MEDA_REGISTER	DLPIMIOC(10)

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_ND_SYS_DLPIMOD_H */
