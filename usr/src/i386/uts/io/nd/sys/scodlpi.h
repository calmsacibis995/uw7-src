#ifndef _IO_ND_SYS_SCODLPI_H  /* wrapper symbol for kernel use */
#define _IO_ND_SYS_SCODLPI_H  /* subject to change without notice */

#ident "@(#)scodlpi.h	26.1"
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

#include <sys/dlpi.h>

#ifndef dlpi_frame_test
#define dlpi_frame_test
#endif

/*
 * scodlpi.h header for SCO specific extensions to AT&T DLPI V2.0
 */

#define MAC_ADDR_SZ	6

/*
 * Media types not defined by AT&T DLPI
 */
		/* SCO TCP/IP Drivers */
#define	DL_SLIP		0x60	/* Serial-Line IP (RFC 1055) */
#define	DL_PPP		0x61	/* Point-to-point Protocol (RFC 117[12]) */
#define	DL_LOOP		0x62	/* Loopback */

#define LLC_OFF		0		/* LLC and SNAP off */
#define LLC_SNAP	1		/* LLC 1 framing on for SNAP */
#define LLC_1 		2		/* LLC 1 framing on */

/* LLC commands and bits */
#define LLC_CMD_UI	0x03
#define LLC_CMD_XID	0xaf
#define LLC_CMD_TEST	0xe3
#define LLC_POLL_FINAL	0x10
#define LLC_RESPONSE	0x01

	/* Values for dl_saptype */
#define FR_ETHER_II	0
#define FR_XNS		1
#define FR_LLC		2
#define FR_SNAP		3
#define FR_ISDN		4

	/* Values for dl_srmode */
#define SR_NON		0
#define SR_AUTO		1
#define SR_STACK	2

	/* Values for dl_sap in DL_BIND_REQ */
#define SNAP_SAP	0x000000aa
#define LLC_DEFAULT_SAP	0xfffffffe
#define XNS_SAP		0x000000ff

struct mac_counters {
	ulong	mac_bcast;		/* #Frames Broadcast */
	ulong	mac_mcast;		/* #Frames Multicast */
	ulong	mac_ucast;		/* #Frames Unicast */
	ulong	mac_error;		/* #Frames with errors */
	ulong	mac_octets;		/* #MAC Octets sent/received */
	ulong	mac_queue_len;		/* #Frames queued for tx/on rx */
};

struct dlpi_stats {
	/* DLPI module info */
	ulong	dl_nsaps;		/* #SAPs currently bound to DLPI */
	ulong	dl_maxsaps;		/* Max. #SAPs usable */
	ulong	dl_rxunbound;		/* #frames received not delivered */

	/* Source Routing info */
	ulong	dl_nroutes;		/* #Source Routes currently in use */
	ulong	dl_maxroutes;		/* Max #Source Routes usable */

	/* MAC Driver info */
	ulong	mac_driver_version;
	ulong	mac_media_type;		/* Ethernet/T-R/FDDI */
	ulong	mac_max_sdu;		/* SDU MAX at MDI layer */
	ulong	mac_min_sdu;		/* SDU MIN at MDI layer */
	ulong	mac_addr_length;	/* ADDR SIZE at MDI layer */

	ulong	mac_stats_len;		/* Size of h/w dep. stats struct */
	ulong	mac_ifspeed;		/* Speed of interface in bits/sec */
	struct	mac_counters mac_tx;	/* Data collected for sends */
	struct	mac_counters mac_rx;	/* Data collected for receives */

	ulong	mac_suspended;		/* is ddi8 driver suspended? */
	ulong	mac_suspenddrops;	/* # of data frames dropped that 
					 * were sent from stack while driver 
					 * was suspended
					 */

	ulong	mac_reserved[6];	/* reserved */
};

struct dlpi_counters {
	ulong	dl_bcast;		/* #Broadcast frames */
	ulong	dl_mcast;		/* #Multicast frames */
	ulong	dl_ucast_xid;		/* #Unicast LLC XID Frames */
	ulong	dl_ucast_test;		/* #Unicast LLC TEST Frames */
	ulong	dl_ucast;		/* #Unicast frames not covered above */
	ulong	dl_error;		/* #Frames with errors */
	ulong	dl_octets;		/* #MAC Octets sent/received */
	ulong	dl_queue_len;		/* #Frames on the send/receive queue */
};

#define DL_USER_SZ	32
struct dlpi_sapstats {
	ulong	dl_saptype;		/* Ethernet-II/XNS/LLC/SNAP */
	ulong	dl_sap;		/* Bind address */
	char	dl_user[DL_USER_SZ];	/* Name of module bound to DLPI */
	ulong	dl_llcmode;		/* LLC_OFF or LLC_1 */

	struct dlpi_counters dl_tx;	/* Statistics for sends */
	struct dlpi_counters dl_rx;	/* Statistics for receives */

	ulong	dl_srmode;		/* SR_NON or SR_AUTO or SR_STACK */
};

#define SR_ROUTE_SZ	30
struct sr_table_entry {
	unchar	sr_remote_mac[MAC_ADDR_SZ]; /* Remote MAC address */
	unchar	sr_state;		/* State machine's state */
	unchar	sr_route_len;		/* #Valid hops in sr_route */
	time_t	sr_timeout;		/* Time @ next timeout */
	ulong	sr_max_pdu;	/* Max. PDU size allowed to sr_remote_mac */
	ushort	sr_route[SR_ROUTE_SZ];	/* Best Route to sr_remote_mac */
};

typedef struct sco_snap_sap {
	ulong	prot_id;
	ushort	type;
	ushort	reserved;
} sco_snap_sap_t;

/******************************************************************************
 * SCO Specific DLPI Primitives.  This section describes the new DLPI
 * primitives which replace LLI 3.1 MACIOC ioctls.
 *
 ******************************************************************************/

#define SCODLPI_BASE	500
#define DL_CLR_STATISTICS_REQ	(SCODLPI_BASE+0x00)
#define DL_SAP_STATISTICS_REQ	(SCODLPI_BASE+0x01)
#define DL_SAP_STATISTICS_ACK	(SCODLPI_BASE+0x02)
#define DL_SET_SRMODE_REQ	(SCODLPI_BASE+0x03)
#define DL_SRTABLE_REQ		(SCODLPI_BASE+0x04)
#define DL_SRTABLE_ACK		(SCODLPI_BASE+0x05)
#define DL_SR_REQ		(SCODLPI_BASE+0x06)
#define DL_SR_ACK		(SCODLPI_BASE+0x07)
#define DL_SET_SR_REQ		(SCODLPI_BASE+0x08)
#define DL_CLR_SR_REQ		(SCODLPI_BASE+0x09)
#define DL_MCTABLE_REQ		(SCODLPI_BASE+0x0a)
#define DL_MCTABLE_ACK		(SCODLPI_BASE+0x0b)
#define DL_SET_SRPARMS_REQ	(SCODLPI_BASE+0x0c)
#define DL_ISDN_MSG		(SCODLPI_BASE+0x0d)  /* ISDN message, must */
						     /* be same as         */
						     /* MAC_ISDN_MSG in    */
						     /* mdi.h */
#define DL_ENABALLMULTI_REQ	(SCODLPI_BASE+0x0e)
#define DL_DISABALLMULTI_REQ	(SCODLPI_BASE+0x0f)

#define DL_FILTER_REQ		(SCODLPI_BASE+0x10)  /* filter set request */

#ifdef dlpi_frame_test
#define DL_DLPI_BILOOPMODE	(SCODLPI_BASE+0x70)
#endif

#define DLPIMIOC(x)       	(('m' << 8) | (x))
#define DLPI_ISDN_MDATA_ON	DLPIMIOC(11)
#define DLPI_ISDN_MDATA_OFF	DLPIMIOC(12)

struct sco_prim {
	ulong	dl_primitive;
};
typedef struct sco_prim dl_clr_statistics_req_t;
typedef struct sco_prim dl_sap_statistics_req_t;
typedef struct sco_prim dl_srtable_req_t;
typedef struct sco_prim dl_mctable_req_t;
typedef struct sco_prim dl_enaballmulti_req_t;
typedef struct sco_prim dl_disaballmulti_req_t;

#ifdef dlpi_frame_test
typedef struct {
	ulong	dl_primitive;			/* DL_DLPI_BILOOPMODE */
	ulong	dl_biloopmode;
} dl_set_biloopmode_req_t;
#endif

typedef struct {
	ulong	dl_primitive;				/* DL_SET_SRMODE_REQ */
	ulong	dl_srmode;
} dl_set_srmode_req_t;

typedef struct {
	ulong	dl_primitive;				/* DL_SR_REQ */
	ulong	dl_addr_len;
	ulong	dl_addr_offset;
} dl_sr_req_t;

typedef struct {
	ulong	dl_primitive;				/* DL_SET_SR_REQ */
	ulong	dl_addr_len;
	ulong	dl_addr_offset;
	ulong	dl_route_len;
	ulong	dl_route_offset;
} dl_set_sr_req_t;

typedef struct {
	ulong	dl_primitive;				/* DL_CLR_SR_REQ */
	ulong	dl_addr_len;
	ulong	dl_addr_offset;
} dl_clr_sr_req_t;

typedef struct {
	ulong	dl_primitive;				/* DL_SET_SRPARMS_REQ */
	ulong	dl_parms_len;
	ulong 	dl_parms_offset;
} dl_set_srparms_req_t;

typedef struct {
	ulong	dl_primitive;			/* DL_SAP_STATISTICS_ACK */
	ulong	dl_sapstats_len;
	ulong	dl_sapstats_offset;		/* of struct dlpi_sap_stats */
} dl_sap_statistics_ack_t;

typedef struct {
	ulong	dl_primitive;			/* DL_SRTABLE_ACK */
	ulong	dl_srtable_len;
	time_t	dl_time_now;	/* lbolt */
	ulong	dl_route_table_sz;/* Number of entrys in the table */
	ulong	dl_routes_in_use;/* Number of entrys currently being used */
	int	dl_tx_resp;    /* timeout for responding to rx */
	int 	dl_rx_ARE;     /* window for rejecting more AREs */
	int	dl_rx_STE_bcs; /* # STE bcs before invalidating route entry
			        * and find new route */
	int	dl_rx_STE_ucs; /* # STE ucs before invalidating route entry
			        * and find new route */
	int 	dl_max_tx;	/* upper limit for tx "recur" window */
	int	dl_min_tx;	/* lower limit for tx "recur" window */
	int	dl_tx_recur;    /* detected "recurs" before tx STE */
	int	dl_ARE_disa;    /* disable sending ARE frames */
} dl_srtable_ack_t;

typedef struct {
	ulong	dl_primitive;			/* DL_MCTABLE_ACK */
	ulong	dl_mctable_len;
	ulong	dl_mctable_offset;
	ulong	dl_all_mca;	/* all multicasts enabled */
	ulong	dl_mca_count;	/* number of multicast addrs enabled */ 
} dl_mctable_ack_t;

typedef struct {
	ulong	dl_primitive;			/* DL_SR_ACK */
	ulong	dl_srtablee_len;
	ulong	dl_srtablee_offset;
} dl_sr_ack_t;

/* valid values for filter direction */

#define DL_FILTER_INCOMING 0
#define DL_FILTER_OUTGOING 1

/*
 * DL_FILTER_REQ
 */
typedef struct {
        ulong   dl_primitive;           /* DL_FILTER_REQ */
        ulong   sap;                    /* the SAP */
        ulong   sap_protid;             /* for SNAP SAPs */
	int	direction;		/* incoming or outgoing */
} dl_setfilter_req_t ;

#define DL_CLR_SR_REQ_SIZE sizeof(dl_clr_sr_req_t)
#define DL_SET_SRPARMS_REQ_SIZE sizeof(dl_set_srparms_req_t)
#define DL_ENABALLMULTI_REQ_SIZE sizeof(dl_enaballmulti_req_t)
#define DL_DISABALLMULTI_REQ_SIZE sizeof(dl_disaballmulti_req_t)
#define DL_FILTER_REQ_SIZE sizeof(dl_setfilter_req_t)
 
#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_ND_SYS_SCODLPI_H */
