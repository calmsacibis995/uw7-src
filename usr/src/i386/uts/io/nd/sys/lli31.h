#ifndef _IO_ND_SYS_LLI31_H  /* wrapper symbol for kernel use */
#define _IO_ND_SYS_LLI31_H  /* subject to change without notice */

#ident "@(#)lli31.h	22.1"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * LLI31.h
 *  This header file contains definitions for the now obsolete LLI 3.1
 *  network driver interface.  It is provided for those prototocol stacks
 *  that wish to remain LLI 3.1 driver compatible.
 */

/*
 * This header file has encoded the values so an existing driver 
 * or user which was written with the Logical Link Interface(LLI)
 * can migrate to the DLPI interface in a binary compatible manner.
 * Any fields which require a specific format or value are flagged
 * with a comment containing the message LLI compatibility.
 */

/*
 * Primitives for Local Management Services
 */
#define LLI31_INFO_REQ		0x00	/* Information Req */
#define LLI31_INFO_ACK		0x03	/* Information Ack */
#define LLI31_BIND_REQ		0x01	/* Bind dlsap address */
#define LLI31_BIND_ACK		0x04	/* Dlsap address bound */
#define LLI31_UNBIND_REQ	0x02	/* Unbind dlsap address */
#define LLI31_OK_ACK		0x06	/* Success acknowledgment */
#define LLI31_ERROR_ACK		0x05	/* Error acknowledgment */

/*
 * Primitives used for Connectionless Service
 */
#define LLI31_UNITDATA_REQ	0x07	/* datagram send request */
#define LLI31_UNITDATA_IND	0x08	/* datagram receive indication */
#define LLI31_UDERROR_IND	0x09	/* datagram error indication */

/*
 * LLI3.1 interface states
 */
#define	LLI31_UNBOUND		0x00	/* PPA attached */
#define	LLI31_BIND_PENDING	0x01	/* Waiting ack of LLI31_BIND_REQ */
#define	LLI31_UNBIND_PENDING	0x02	/* Waiting ack of LLI31_UNBIND_REQ */
#define	LLI31_IDLE		0x03	/* dlsap bound, awaiting use */

/*
 * LLI31_ERROR_ACK error return values
 */
#define	LLI31_ACCESS	0x02	/* Improper permissions for request */
#define	LLI31_BADADDR	0x01	/* DLSAP address in improper format or invalid*/
#define	LLI31_BADDATA	0x06	/* User data exceeded provider limit */
#define LLI31_BADPRIM	0x09	/* Prim. received but unknown by DLS provider */
#define	LLI31_BADSAP	0x00	/* Bad LSAP selector */
#define LLI31_BOUND	0x0d	/* Attempted second bind with lli31_max_conind*/
				/* or lli31_conn_mgmt>0 on same DLSAP or PPA */
#define	LLI31_NOTINIT	0x10	/* Physical Link not initialized */
#define	LLI31_OUTSTATE	0x03	/* Primitive issued in improper state */
#define	LLI31_SYSERR	0x04	/* UNIX system error occurred */

/*
 * LLI31 media types supported
 */
#define	LLI31_CSMACD	0x0	/* IEEE 802.3 CSMA/CD network */
#define	LLI31_TPR	0x2	/* IEEE 802.5 Token Passing Ring */

/*
 *	LOCAL MANAGEMENT SERVICE PRIMITIVES
 */

/*
 * LLI31_INFO_REQ, M_PCPROTO type
 */
typedef struct lli31_info_req {
	ulong	dl_primitive;
} lli31_info_req_t;

/*
 * LLI31_INFO_ACK, M_PCPROTO type
 */
typedef struct lli31_info_ack {
	ulong		dl_primitive;
	ulong		dl_max_sdu;
	ulong		dl_min_sdu;
	ulong		dl_addr_length;
	ulong		dl_mac_type;
	ulong		dl_reserved;
	ulong		dl_current_state;
	ulong		dl_max_idu;
	ulong		dl_service_mode;
	ulong		dl_qos_length;
	ulong		dl_qos_offset;
	ulong		dl_qos_range_length;
	ulong		dl_qos_range_offset;
	long		dl_provider_style;
	ulong 		dl_addr_offset;
	ulong		dl_growth;
} lli31_info_ack_t;

/*
 * LLI31_BIND_REQ, M_PROTO type
 */
typedef struct lli31_bind_req {
	ulong	dl_primitive;
	ulong	dl_sap;
	ulong	dl_max_conind;
	ushort	dl_service_mode;
	ushort	dl_conn_mgmt;
} lli31_bind_req_t;

/*
 * LLI31_BIND_ACK, M_PCPROTO type
 */
typedef struct lli31_bind_ack {
	ulong	dl_primitive;
	ulong	dl_sap;
	ulong	dl_addr_length;
	ulong	dl_addr_offset;
	ulong	dl_max_conind;
	ulong	dl_growth;
} lli31_bind_ack_t;

/*
 * LLI31_UNBIND_REQ, M_PROTO type
 */
typedef struct lli31_unbind_req {
	ulong	dl_primitive;
} lli31_unbind_req_t;

/*
 * LLI31_OK_ACK, M_PCPROTO type
 */
typedef struct lli31_ok_ack {
	ulong	dl_primitive;
	ulong	dl_correct_primitive;
} lli31_ok_ack_t;

/*
 * LLI31_ERROR_ACK, M_PCPROTO type
 */
typedef struct lli31_error_ack {
	ulong	dl_primitive;
	ulong	dl_error_primitive;
	ulong	dl_errno;
	ulong	dl_unix_errno;
} lli31_error_ack_t;


/*
 *	CONNECTIONLESS SERVICE PRIMITIVES
 */

/*
 * LLI31_UNITDATA_REQ, M_PROTO type, with M_DATA block(s)
 */
typedef struct lli31_unitdata_req {
	ulong	dl_primitive;
	ulong	dl_dest_addr_length;
	ulong	dl_dest_addr_offset;
	ulong	dl_reserved[2];
} lli31_unitdata_req_t;

/*
 * LLI31_UNITDATA_IND, M_PROTO type, with M_DATA block(s)
 */
typedef struct lli31_unitdata_ind {
	ulong	dl_primitive;
	ulong	dl_src_addr_length;
	ulong	dl_src_addr_offset;
	ulong	dl_dest_addr_length;
	ulong	dl_dest_addr_offset;
	ulong	dl_reserved;
} lli31_unitdata_ind_t;

/*
 * LLI31_UDERROR_IND, M_PCPROTO type
 */
typedef struct lli31_uderror_ind {
	ulong	dl_primitive;
	ulong	dl_dest_addr_length;
	ulong	dl_dest_addr_offset;
	ulong	dl_reserved;
	ulong	dl_errno;
} lli31_uderror_ind_t;

union LLI31_primitives {
	ulong			dl_primitive;
	lli31_info_req_t	info_req;
	lli31_info_ack_t	info_ack;
	lli31_bind_req_t	bind_req;
	lli31_bind_ack_t	bind_ack;
	lli31_unbind_req_t	unbind_req;
	lli31_ok_ack_t		ok_ack;
	lli31_error_ack_t	error_ack;
	lli31_unitdata_req_t	unitdata_req;
	lli31_unitdata_ind_t	unitdata_ind;
	lli31_uderror_ind_t	uderror_ind;
};

#define	LLI31_INFO_REQ_SIZE	sizeof(lli31_info_req_t)
#define	LLI31_INFO_ACK_SIZE	sizeof(lli31_info_ack_t)
#define	LLI31_BIND_REQ_SIZE	sizeof(lli31_bind_req_t)
#define	LLI31_BIND_ACK_SIZE	sizeof(lli31_bind_ack_t)
#define	LLI31_UNBIND_REQ_SIZE	sizeof(lli31_unbind_req_t)
#define	LLI31_OK_ACK_SIZE	sizeof(lli31_ok_ack_t)
#define	LLI31_ERROR_ACK_SIZE	sizeof(lli31_error_ack_t)
#define	LLI31_UNITDATA_REQ_SIZE	sizeof(lli31_unitdata_req_t)
#define	LLI31_UNITDATA_IND_SIZE	sizeof(lli31_unitdata_ind_t)
#define	LLI31_UDERROR_IND_SIZE	sizeof(lli31_uderror_ind_t)
 
/* statistics structure (for the MACIOC_GETSTAT ioctl) */

typedef struct lli31_mac_stats {
	unsigned long mac_frame_xmit;	/* Frames Transmitted */
	unsigned long mac_bcast_xmit;	/* Broadcast Frames Transmitted */
	unsigned long mac_mcast_xmit;	/* Multicast Frames Transmitted */
	unsigned long mac_lbolt_xmit;	/* Frames Transmitted per Second */
	unsigned long mac_frame_recv;	/* Frames Received */
	unsigned long mac_bcast_recv;	/* Broadcast Frames Received */
	unsigned long mac_mcast_recv;	/* Multicast Frames Received */
	unsigned long mac_lbolt_recv;	/* Frames Received per Second */
	unsigned long mac_frame_def;	/* Frames Deferred */
	unsigned long mac_collisions;	/* Total Collisions */
	unsigned long mac_frame_coll;	/* Frames Involved in a Collision */
	unsigned long mac_oframe_coll;	/* Out of Frame Collisions */
	unsigned long mac_xs_coll;	/* Dropped Due To Excess Collisions */
	unsigned long mac_frame_nosr;	/* Dropped Due To Lack of STREAMS */
	unsigned long mac_no_resource;	/* Dropped Due To Lack of Resources */
	unsigned long mac_badsum;	/* Bad Checksum Received */
	unsigned long mac_align;	/* Bad Alignment Received */
	unsigned long mac_badlen;	/* Bad Length Received */
	unsigned long mac_badsap;	/* Bad SAP Received */
	unsigned long mac_mcast_rjct;	/* Multicast Frames Rejected */
	unsigned long mac_carrier;	/* Errors Due To Lost Carrier */
	unsigned long mac_badcts;	/* Errors Due To Lost CTS */
	unsigned long mac_baddma;	/* Errors Due To Over/Under Runs */
	unsigned long mac_timeouts;	/* Device Timeouts */
	unsigned long mac_intr;		/* Device Interrupts */
	unsigned long mac_spur_intr;	/* Spurious Interrupts */
	unsigned long mac_ioctets;	/* received octets */
	unsigned long mac_ooctets;	/* transmitted octets */
	unsigned long mac_ifspeed;	/* net interface speed in bits/sec */
	unsigned long mac_reserved[1];	/* Reserved */
} lli31_mac_stats_t;

/* statistics structure (for the MACIOC_GETIFSTAT ioctl) */
typedef struct lli31_mac_ifstats {
	struct lli31_mac_ifstats *ifs_next;	/* next if on chain */
	char           *ifs_name;	/* interface name */
	short           ifs_unit;	/* unit number */
	short           ifs_active;	/* non-zero if this if is running */
	caddr_t		*ifs_addrs;	/* list of addresses */
	short           ifs_mtu;	/* Maximum transmission unit */

	/* generic interface statistics */
	int             ifs_ipackets;	/* packets received on interface */
	int             ifs_ierrors;	/* input errors on interface */
	int             ifs_opackets;	/* packets sent on interface */
	int             ifs_oerrors;	/* output errors on interface */
	int             ifs_collisions;	/* collisions on csma interfaces */
} lli31_mac_ifstats_t;

#endif	/* _IO_ND_SYS_LLI31_H */
