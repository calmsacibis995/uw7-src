#ifndef _MDI_TOK_H
#define _MDI_TOK_H
#ident "@(#)tok.h	28.1"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifndef STATIC
#ifndef DEBUG
#define STATIC	static
#else
#define STATIC
#endif /* DEBUG */
#endif /* STATIC */



#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <mem/immu.h>
#include <io/nd/sys/mdi.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <util/mod/moddefs.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <util/kdb/xdebug.h>
#include <io/ddi.h>

#else

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/immu.h>
#include <sys/mdi.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/moddefs.h>
#include <sys/cm_i386at.h>
#include <sys/xdebug.h>
#include <sys/ddi.h>

#endif

#ifdef UNIXWARE
# define AT	0x00000001	/* Industry Standard Architecture */
# define MC	0x00000002	/* MicroChannel Architecture */
# define EISA	0x00000004	/* Extended Industry Standard Architecture */
#endif






/**********************************************/
/* Defines for address check (thc-sm/thc-fdx) */
/**********************************************/

#define ADDR_LEN     6
typedef unchar          n_addr [ADDR_LEN];
/*
 * MAC header structure (token ring)
 */
typedef struct mac_hdr_tr {
    unchar         mh_pcf0;          /* Access Control Field */
    unchar         mh_pcf1;          /* Frame Control Field  */
    n_addr         mh_dst;           /* Destination address  */
    n_addr         mh_src;           /* Source address       */
} mac_hdr_tr;


/* macro that determine if the address is a MultiCastAddress */
#define ISMCATR(ma)     (*(u_char *)(ma) == 0xC0 && *((u_char *)(ma)+1) == 0x00)

/* macro that determine if the address is a BroadcastAddress */
#define ISBROADCAST(ma) (*(u_char *)(ma) == 0xFF &&     \
                         *((u_char *)(ma)+1) == 0xFF && \
                         *((u_char *)(ma)+2) == 0xFF && \
                         *((u_char *)(ma)+3) == 0xFF && \
                         *((u_char *)(ma)+4) == 0xFF && \
                         *((u_char *)(ma)+5) == 0xFF)


/*********************************************/
/* End of the Defines for address check      */
/*********************************************/


#define TXBUF_LEN	2048		/* Size of a transmit buffer */
#define RXBUF_LEN	2048		/* Size of a receive buffer */
#define MAX_PDU		(RXBUF_LEN - 6)	/* MAX PDU Size (at MDI I/F) */


struct tokhwconf {	/*	ISA		MCA */
	u_int	slot;	/*	 -		Link-kit */
	u_int	iobase;	/*	Link-kit	Boot-time */
	u_int	rambase;/*	Link-kit	Boot-time */
	u_int	ramsz;	/*	Link-kit	Boot-time */
	u_int	irq;	/*	Boot-time	Boot-time */
};

#define ADDR_TO_LONG(x) (*((ulong *)(((unchar *)(x))+2)))

/* Base addr for MCA adapters is encoded in low bit of POS register 3 */
#define	TOKEN_MCA_BASE(pos3) (((pos3) & 1) ? 0xa24 : 0xa20)
#define ADENABLE	0x96	/* adapter enable/setup on MCA */
#define CH_SETUP	0x08	/* adapter enable/setup on MCA */
#define	POS3		0x103	/* POS register 3 - info port 1 on MCA*/

/* Base addr of PIO on Primary / Alternate card */
#define TOKEN_BASE(unit) (tokhwconf[unit].iobase)

#define	TOKEN_SWTCH_RD(u) TOKEN_BASE(u)	/* a PIO read to this offset
					   returns the contents of the
					   ROS/MMIO domain switches  */
#define	TOKEN_RESET(u) (TOKEN_BASE(u)+ 1) /* any PIO to this offset cause
					   adapter reset  */
#define	TOKEN_SWTCH2_RD(u) (TOKEN_BASE(u)+2)/* a PIO read to this offset
					   returns the Microchannel info */
#define	TOKEN_RELEASE(u) (TOKEN_BASE(u)+2)/* a PIO write to this offset
					   terminates reset state...
					   wait at least 50ms between RESET
					   and RELEASE	*/
#define	TOKEN_INTR_REL(u) (TOKEN_BASE(u)+3)/* PIO write to this offset are
					   used to reenable only the 
					   adapter interrupt after a
					   interrupt has been serviced	*/


#define	ACA		0x1e00		/* attached control area offset	*/
#define	AIP		0x1f00		/* adapter ID PROM		*/
struct aca {
	unchar	rrr_even, rrr_odd;
	ushort	wrb;
	ushort	wwo;
	ushort	wwc;
	unchar	isrp_even, isrp_odd;
	unchar	isra_even, isra_odd;
	ushort	tcr;
	ushort	tvr;
	char	filler1[0x8];
	unchar	srpr_even, srpr_odd;
	char	filler2[0xe];
	unchar	isrp_even_rst, isrp_odd_rst;
	unchar	filler3, isra_odd_rst;
	char	filler4[0x1c];
	unchar	isrp_even_set, isrp_odd_set;
	unchar	filler5, isra_odd_set;
};

/* Interrupts to the PC	(see page 7 in S.G.12.002)			*/
#define	ADAP_CHK	0x40		/* Adapter check		*/
#define	SRB_RESP	0x20		/* SRB response			*/
#define	ARB_CMD		0x08		/* ARB command			*/

/* Interrupts to the PC when in normal mode */             /* //thc-sm */
#define	NM_IMPL_RCV	0x80		/* Received IMPL Force MAC fr	*/
#define	NM_ASB_FREE	0x10		/* ASB free			*/
#define	NM_SSB_RESP	0x04		/* SSB response			*/
#define NM_BR_FWD_CMPLT	0x02		/* Bridge frame forward is complete */

/* Interrupts to the PC when in shalow mode */             /* //thc-sm */
#define	SM_RXBUFF_POST	0x10		/* Receive Buffers Posted  */
#define SM_TXCPLT	0x02		/* Transmit Complete       */

/* Interrupts to the PC (in isrp_high) */
#define TIMER_INT	0x10		/* TVR-even expired */
#define ERROR_INT	0x08		/* Machine check error/Adapter error */
#define ACCESS_INT	0x04		/* Shared RAM Access violation */

/* Interrupts to the card (see page 6 in S.G.12.002)			*/
#define	SRB_CMD		0x20		/* Command in SRB		*/
#define	SRB_FREE_REQ	0x08		/* SRB free request		*/
#define	ARB_FREE	0x02		/* ARB free			*/

/* Interrupts to the card when in normal mode */          /* //thc-sm */
#define	NM_ASB_RESP	0x10		/* Response in ASB		*/
#define	NM_ASB_FREE_REQ	0x04		/* ASB free request		*/
#define	NM_SSB_FREE	0x01		/* SSB free			*/

/* Interrupts to the card when in shallow mode */          /* //thc-sm */
#define	SM_TX_REQUEST   0x40		/* Fast Path Trasnmit Request */
#define	SM_RXBUFF_CPLT	0x10		/* Receive Buffers completed  */

/* SRB Response for Initialization (see page 27 in S.G.12.002)		*/
struct srb_init_status {
	unchar 	command;
	unchar	status;
	unchar	status2;
	unchar 	reserved[3];
	ushort  bringup_code;
	ushort  encoded_address;
	ushort  level_address;
	ushort  adapter_address;
	ushort  parms_address;
	ushort  mac_address;
};

/* SRB Open Command (see page 35 in S.G.12.002)				*/
struct srb_open {
	unchar	command;
	unchar	reserved[7];
	ushort  open_options;
	unchar  node_address[6];
	unchar	group_address[4];
	unchar 	func_address[4];
	ushort	num_rcv_buff;
	ushort	rcv_buf_len;
	ushort	dhb_buf_len;
	unchar	num_dhb;
	unchar	reserv1;
	unchar  dlc_max_sap;
	unchar 	dlc_max_stations;
	unchar	dlc_max_gsap;
	unchar 	dlc_max_gsap_mems;
	unchar	dlc_t1_tick_one;
	unchar	dlc_t2_tick_one;
	unchar	dlc_ti_tick_one;
	unchar	dlc_t1_tick_two;
	unchar	dlc_t2_tick_two;
	unchar	dlc_ti_tick_two;
	unchar	product_id[18];
};

/* SRB Open Response (see page 39 in S.G.12.002)		*/
struct srb_open_response {
	unchar	command;
	unchar	reserved;
	unchar	retc;
	unchar	reserved1[3];
	ushort	open_error_code;
	ushort	asb_address;
	ushort	srb_address;
	ushort	arb_address;
	ushort	ssb_address;
	unchar  open_status;        /* //thc-fdx Extend open status */
};

/* open_options */
#define	MON_CONTENDER	0x0100	/* participate in monitor contention if set */
struct srb_modify_open {
	unchar	command;	/* == 0x01 */
	unchar	reserved;
	unchar	retc;
	unchar	reserved1;
	ushort  open_options;
};

/* SRB Send Command (see page 65 in S.G.12.002)				*/
struct srb_send {
	unchar	command;
	unchar	correlate;
	unchar	retc;
	unchar	reserved;
	ushort	station_id;
};

/* SRB Set Functional Address Command */
struct srb_set_func_addr {
	unchar	command;	/* == 0x07 */
	char	res1;		/* Reserved */
	unchar	retc;		/* Return code */
	char	res2[3];	/* Reserved */
	ulong	func_addr;	/* New functional address */
};
	
/* ARB Transmit Data Request Command (see page 78 in S.G.12.002)	*/
struct arb_xmt_cmd {
	unchar 	command;
	unchar	correlate;
	ushort	reserved;
	ushort	station_id;
	ushort	dhb_address;
};

struct asb_status {
	unchar	command;
	unchar	correlate;
	unchar	retc;
};

/* ASB Response to ARB Transmit Data Request Command (see page 79 in
   S.G.12.002								*/
struct asb_xmt_resp {
	unchar	command;
	unchar	correlate;
	unchar	retc;
	unchar	reserved;
	ushort	station_id;
	ushort	frame_len;
	unchar	hdr_len;
	unchar	rsap_value;
};

/* SSB Response to SRB Send Command (see page 67 in S.G.12.002)		*/
struct ssb_xmt_resp {
	unchar 	command;
	unchar	correlate;
	unchar	retc;
	unchar 	reserved;
	ushort	station_id;
	unchar	xmt_error;
};

/* SRB Close Command (see page 32 in S.G.12.002)			*/
struct srb_close {
	unchar 	command;
	unchar 	reserved;
	unchar 	retc;
};

/* Receive Buffer Format (see page 72 in S.G.12.002)			*/
struct rcv_buf {
	ushort	reserved;
	ushort	rcv_buf_ptr;
	unchar	reserved1;
	unchar	receive_pcfe;
	ushort	frame_len;
	unchar	data[1];
};

/* ARB Received Data Command (see page 73 in S.G.12.002)		*/
struct arb_rcv_data {
	unchar 	command;
	unchar 	reserved[3];
	ushort 	station_id;
	ushort	rcv_buf_ptr;
	unchar	hdr_len;
	unchar	dlc_len;
	ushort	frame_len;
	unchar	msg_type;
};

/* ASB Received Data Command Response (see page 75 in S.G.12.002)	*/
struct asb_rcv_resp {
	unchar	command;
	unchar	reserved;
	unchar	retc;
	unchar	reserved1;
	ushort	station_id;
	ushort	rcv_buf_ptr;
};

struct srb_sap_open {
	unchar	command;
	unchar	reserved;
	unchar	retc;
	unchar	reserved1;
	ushort	station_id;
	unchar	timer_t1;
	unchar	timer_t2;
	unchar	timer_ti;
	unchar	maxout;
	unchar	maxin;
	unchar	maxout_incr;
	unchar	max_retry_count;
	unchar	gsap_max_mem;
	ushort	max_i_field;
	unchar	sap_value;
	unchar	sap_options;
	unchar	station_count;
	unchar	sap_gsap_mem;
	unchar	gsaps[8];
};

struct srb_sap_close {
	unchar	command;
	unchar	reserved;
	unchar	retc;
	unchar	reserved1;
	ushort	station_id;
};

struct srb_read_log {
	unchar	command;
	unchar	reserved;
	unchar	retc;
	unchar	reserved1[3];
		/* ERROR Counters */
	unchar	line, internal, burst, ac, abort;
	unchar	reserved2;
	unchar	lost_frames, congestion, copied, freq, token;
};

struct arb_ring_status {
	unchar	command;
	unchar	reserved[5];
	ushort	ring_status;
	unchar  fdx_error;        /* //thc-fdx */
};

/* ring status codes */
#define RST_SIGNAL_LOSS         0x8000
#define RST_HARD_ERROR          0x4000
#define RST_SOFT_ERROR          0x2000
#define RST_TRANSMIT_BEACON     0x1000
#define RST_LOBE_WIRE_FAULT     0x0800
#define RST_AUTO_RMV_ERROR      0x0400

#define RST_FDX_ERROR           0x0200        /* //thc-fdx */

#define RST_REMOVE_RECEIVED     0x0100
#define RST_COUNTER_OVERFLOW    0x0080
#define RST_SINGLE_STATION      0x0040
#define RST_RING_RECOVERY       0x0020

struct adapter_address_area {
	unchar	node_addr[6];		/* Adapter's node address */
	unchar	group_addr[4];		/* Adapter's group address */
	ulong	func_addr;		/* Adapter's functional address */
};

struct adapter_parameters_area {
	unchar	phys_addr[4];		/* Adapter's physical address */
	unchar	up_node_addr[6];	/* Next active upstream neighbour */
	unchar	up_phys_addr[4];	/* Next active up' b'bor phys addr */
	unchar	poll_addr[6];		/* Last poll address */
	unchar	reserved[2];
	ushort	acc_priority;		/* Transmit access priority */
	ushort	source_class;		/* Source class authority */
	ushort	att_code;		/* Last attention code */
	unchar	source_addr[6];		/* Last source address */
	ushort	beacon_type;		/* Last beacon type */
	ushort	major_vector;		/* Last major vector */
	ushort	netw_status;		/* Network status */
	ushort	soft_error;		/* Soft error timer value */
	ushort	fe_error;		/* Front end error counter */
	ushort	local_ring;		/* Ring number */
	ushort	mon_error;		/* Monitor error code */
	ushort	beacon_transmit;	/* Beacon transmit type */
	ushort	beacon_receive;		/* Beacon receive type */
	ushort	frame_correl;		/* Frame correlator save */
	unchar	beacon_naun[6];		/* Beaconing station's NAUN */
	unchar	reserved2[4];
	unchar	beacon_phys[4];		/* Beaconing station's phys addr */	
};

struct tokdevice {
	queue_t		*up_queue;		/* Read Queue to MDI */
	uint		mdi_state;		/* See below */
	void            *mac_bind_cookie;       /* smy: arg for mca functions */
	uint		wait_srb_complete:1,	/* Sleep() for an SRB to end */
			device_timeout:1,	/* timed out open/closing */
			tokstartio_going:1,	/* tokstartio() called */
			srb_busy:1,		/* SRB in use */
			wait_send_xmt_srb:1,	/* Waiting to send TX SRB */
			wait_send_close_srb:1,	/* Wait-send CLOSE SRB */
			wait_send_log_srb:1,	/* Wait-send READ LOG SRB */
			asb_busy:1,		/* ASB in use */
			wait_send_xmt_asb:1,	/* Waiting to send TX ASB */
			wait_send_rcv_asb:1,	/* Waiting to send RX ASB */
			wait_send_mod_srb:1	/* Waiting on MODIFY OPEN SRB */
			;

	mblk_t		*iocmp;
	int		timeout_id;

	/* Transmit information */
	unchar *		txbufp;		/* Addr of a free tx buffer */
	struct srb_send		srb_send;	/* Local copy of SRB */
	struct asb_xmt_resp	asb_xmt_resp;	/* Local copy of ASB */

	/* Receive information */
	struct asb_rcv_resp	asb_rcv_resp;	/* Local copy of ASB */

	/* Other SRB buffers */
	struct srb_set_func_addr srb_set_func_addr;
	struct srb_close         srb_close;
	struct srb_read_log      srb_read_log;
	struct srb_modify_open   srb_modify_open;

	/* H/W Information */
	unchar		hw_address[6];
	unchar		all_mca;
	ulong		max_pdu;	      /* MAX PDU Size */
	unchar		ring_speed_detector;  /* detect ring speed mismatch */
	unchar		ring_speed_save;      /* auto update ring speed */
	ulong		ring_speed;	      /* 4/16Mb/s */
	unchar          fdx_enable;           /* enable FULL-DUPLEX  //thc-fdx*/
	unchar          fdx_opened;           /* adapter opened FDX  //thc-fdx*/
	unchar          sm_enable;            /* enable SHALLOW-MODE //thc-sm */


	/* Pointers into the adapter's workspace */
	unchar		*mmio;			/* MMIO Area */
	volatile struct aca	*aca;		/* The ACA in the MMIO Area */

	unchar		*sram;			/* Shared Memory */
	struct adapter_address_area *addr;	/* The 'adapter_addr' area in */
						/* the S-RAM area */
	struct adapter_parameters_area *parms;	/* The 'parameters' area in */
						/* the S-RAM area */
	unchar		*srb;			/* Addr of SRB */
	unchar		*asb;			/* Addr of ASB */
	unchar		*ssb;			/* Addr of SSB (R-only) */
	unchar		*arb;			/* Addr of ARB (R-only) */

	/* Shallow Mode adapter's workspace */	/* //thc-sm */
	struct fp_tx_c_area	*fp_tx_ca;	/* fast path transmit */
						/* control area.      */
	struct fp_rx_c_area	*fp_rx_ca;	/* fast path receive  */
						/* control area.      */

	/* Shallow Mode adapter tx operation */	/* //thc-sm */
	volatile ushort	tx_completion_q_head;	/* completion queue   */
						/* head.              */
	volatile int	tx_buffer_count;	/* transmission       */
						/* buffers count.     */

	/* Shallow Mode adapter rx operation */	/* //thc-sm */
	volatile ushort	rx_queue_pointer;	/* receive    queue   */
						/* head.              */

	/* Shallow Mode adapter general operation */	/* //thc-sm */
	volatile int	sm_retc;		/* SM return code     */
						/* of the adapter     */

	mac_stats_tr_t	stats;			/* Statistics */
	unchar		hw_raddress[6];		/* factory MAC address */
	sv_t	 	*sv;			/* syncronization variable */
	lock_t	 	*lock;			/* SV_WAIT lock */
	uint_t		unit;			/* valid minor device number */
};

/*
 * VALID VALUES FOR mdi_state
 */
#define MDI_NOT_PRESENT	0	/* Adapter not present */
#define MDI_CLOSED	1	/* Adapter present */
#define MDI_OPEN	2	/* MDI device is open */
#define MDI_BOUND	3	/* LLI module bound to MDI device */


/* //thc-fdx
 * DEFINES FOR FULL-DUPLEX MODE
 */

/* open_status codes */
#define OPEN_STATUS_FDX         0x08

/* //thc-sm
 * DEFINES FOR SHALOW MODE
 */

#define SM_RX_BUFFER_SIZE   512		/* length of receive buffers.   */
#define SM_RX_BUFHEADER_SZ    8         /* size of the buffer header.   */
#define SM_RX_BUFDATA_SZ    (SM_RX_BUFFER_SIZE - SM_RX_BUFHEADER_SZ)
                                        /* The min. number of received  */
                                        /* buffer must be calculated by:*/
                                        /* (tokframsz[unit]/BUFDATA_SZ) */
                                        /*  + 1                         */

#define SM_OFFSET_RXNEXT     2          /* offset of the next_buffer    */
                                        /* field in rx buffer.          */

#define SM_RX_STATUS_EOF     0x0001     /* end of frame mask for the    */
                                        /* status in rx buffer.         */
                                        /* ALREADY IN ADAPTER FORMAT    */

#define SM_TX_BUFFER_SIZE   512         /* size of transmit buffers     */
#define SM_TX_BUFHEADER_SZ   22         /* size of the buffer header.   */
#define SM_TX_BUFDATA_SZ    (SM_TX_BUFFER_SIZE - SM_TX_BUFHEADER_SZ)
                                        /* size of the buffer header.   */

#define SM_OFFSET_TXNEXT    16          /* offset of the next_buffer    */
                                        /* field in tx buffer.          */

#define SM_TX_RAM_SIZE      2818        /* ram size to allocate for     */
                                        /* transmit buffer (in number   */
                                        /* of eight bytes blocks)       */

#define SM_CNF_EXT_OPTIONS  0x80        /* bit7: 1 -> shallow mode      */
                                        /* bit6: 0 \ no lookahead       */
                                        /* bit5: 0 /                    */


/* SRB Configure Extensions Command */                   /* //thc-sm */
struct srb_cnf_ext {
	unchar	command;
	unchar	reserved [1];
	unchar  retc;
	unchar	reserved1 [3];
	unchar	options_flag;
};
typedef struct srb_cnf_ext srb_cnf_ext;

/* SRB Configure Extensions response */                 /* //thc-sm */
struct srb_cnf_ext_resp {
	unchar	command;
	unchar	reserved [1];
	unchar  retc;
	unchar	reserved1 [5];
};
typedef struct srb_cnf_ext_resp srb_cnf_ext_resp;


/* SRB Configure Fast Path RAM Command */              /* //thc-sm */
struct srb_cnf_fp_ram {
	unchar	command;
	unchar	reserved [1];
	unchar  retc;
	unchar	reserved1 [5];
	ushort  tx_ram_size;
	ushort  tx_buffer_size;
};
typedef struct srb_cnf_fp_ram srb_cnf_fp_ram;

/* SRB Configure Fast Path RAM response */              /* //thc-sm */
struct srb_cnf_fp_ram_resp {
	unchar	command;
	unchar	reserved [1];
	unchar  retc;
	unchar	reserved1 [5];
	ushort  fp_tx_sram;
	ushort  srb_address;
	ushort  fp_rx_sram;
};
typedef struct srb_cnf_fp_ram_resp srb_cnf_fp_ram_resp;


/* Shallow Mode Fast Path Transmit Control Area */        /* //thc-sm */
struct fp_tx_c_area {
	ushort  buffer_count;
	ushort  free_q_head;
	ushort  free_q_tail;
	ushort  adp_q_head;
	ushort  buffer_size;
	ushort  completion_q_tail;
	unchar  reserved [4];
};
typedef struct fp_tx_c_area fp_tx_c_area;

/* Shallow Mode trasmit Buffer Format */                  /* //thc-sm */
struct sm_tx_buf {
	ushort	reserved1;
	unchar  retcode;
	unchar  reserved2;
	ushort  station_id;
	ushort  frame_len;
	unchar  reserved3 [4];
	ushort  last_buffer;
	ushort  frame_pointer;         /* reserved for the adapter */
	ushort	next_buffer;
	unchar  xmit_status;           /* reserved for the adapter */
	unchar	stripped_FS;
	ushort	buffer_len;
	unchar	frame_data[1];
};
typedef struct sm_tx_buf sm_tx_buf;


/* Shallow Mode Fast Path Receive  Control Area */        /* //thc-sm */
struct fp_rx_c_area {
	ushort  posted_count;
	ushort  completion_count;
	ushort  freed_count;
	ushort  posted_q_head;
	ushort  posted_q_tail;
	unchar  reserved [6];
};
typedef struct fp_rx_c_area fp_rx_c_area;

/* Shallow Mode Receive Buffer Format */                  /* //thc-sm */
struct sm_rx_buf {
	ushort	frame_len;
	ushort	next_buffer;
	ushort	receive_status;
	ushort	buffer_len;
	unchar	frame_data[1];
};
typedef struct sm_rx_buf sm_rx_buf;


#endif /* _MDI_TOK_H */
