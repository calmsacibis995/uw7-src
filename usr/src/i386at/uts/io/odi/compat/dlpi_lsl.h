#ifndef _IO_DLPI_LSL_DLPI_LSL_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_LSL_DLPI_LSL_H	/* subject to change without notice */

#ident	"@(#)dlpi_lsl.h	2.1"
#ident	"$Header$"

/*
 *	dlpi_lsl.h: header file for Unixware 1.1 compatibility.
 */

#define	TSM_T_ETHER	0
#define	TSM_T_TOKEN	1

typedef struct _DRIVER_DATA_ {
	void	*FakeData;
} DRIVER_DATA;

typedef struct _ADAPTER_DATA_ {
	void	*FakeData;
} ADAPTER_DATA;

#include <sys/odi.h>

#define	LSL_NAME	"lsl"

typedef struct lslmaddr {
	unsigned char entry[6];
} lslmaddr_t;

typedef struct bdd {
	struct lslmaddr	*lsl_multip;
	struct lslmaddr	*lsl_multiaddrs;
} bdd_t;

#pragma pack(1)
typedef union {
	struct {
		uchar_t	b0;
		uchar_t	b1;
		uchar_t	b2;
		uchar_t	b3;
		uchar_t	b4;
		uchar_t	b5;
	} c;
	struct {
		ulong_t		l;
		ushort_t	s;
	} l;
} protocolID_t;
#pragma pack()

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
#define			BOARD_PRESENT	0x01
#define			BOARD_DISABLED	0x02
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
	int		bd_virtualBoards[4];
	ADAPTER_DATA	*bd_adapterDataSpace;	/* Adapter Data */
						/* Space for this bd */
	/*
	 * the TSM's send routine. we keep a copy it's address here,
	 * because it's faster than (*sap->bd->adapterDataSpace->
	 * MAD_MediaParameters.MediaSendPtr)(ecb);
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
} LSL_bdconfig_t;

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
#define LLC2MODE			0x40

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
	LSL_bdconfig_t	*sap_bd;		/* ptr to controlling bd */

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
} LSL_sap_t;

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
	LSL_sap_t	*sap;
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
#define SOURCE_ROUTBIT		0x80

#endif _IO_DLPI_LSL_DLPI_LSL_H
