/*	Copyright (c) 1993 UNIVEL					*/

#ifndef _IO_SR_SR_H		/* wrapper symbol for kernel use */
#define _IO_SR_SR_H		/* subject to change without notice */

#ident	"@(#)sr.h	10.1"

#define		MAC_ADDR_LEN		6
#define		MAX_ROUTE_SIZE		18
#define		LLC_NULL_SAP		0
#define		IP_SAP			0x800
#define		ARP_SAP			0x806

#define		SR_NULL_SAP_SIZE	1
#define		INIT_ROUTE_HDR_SIZE	0x2

#define		SOURCE_ROUTE_BIT	0x80
#define		SOURCE_ROUTE_DIR_BIT	0x80
#define		SINGLE_ROUTE_BCAST	0xE0
#define		ALL_ROUTE_BCAST		0xA0
#define		LARGEST_FRAME_SIZE	0x10 	/*This represents 1500 bytes of
						  user info and should be 
						  compatible with ethernet
						  frame sizes.
						*/

#define TEST_DATA_SIZE	(DL_TEST_REQ_SIZE + MAX_ROUTE_SIZE + MAC_ADDR_LEN +\
						SR_NULL_SAP_SIZE * 2)

#define		MAX_TAB_SIZE		13
#define		MAX_WAIT_TAB_SIZE	10

#define		LOCAL_REPLY_WAIT	3
#define		REMOTE_REPLY_WAIT	6

#define		SR_PAGE_SIZE		1024
#define		SR_TIMEOUT		5

#define		SR_HASH(X)	(((unsigned char)(*(X + 5))) % MAX_TAB_SIZE)
#define		MAXMINORS	32

#define		COPY_MACADDR(y, x)	\
	(x[0] = y[0], x[1] = y[1], x[2] = y[2], \
	 x[3] = y[3], x[4] = y[4], x[5] = y[5])

#define		SAME_MACADDR(x, y)	\
	(x[0] == y[0] && x[1] == y[1] && x[2] == y[2] && \
	 x[3] == y[3] && x[4] == y[4] && x[5] == y[5])

#define		BROADCAST(x)	\
	(x[0] == 0xff && x[1] == 0xff && x[2] == 0xff && \
	 x[3] == 0xff && x[4] == 0xff && x[5] == 0xff)

#define		HIWAT		24576
#define		LOWAT		2048
#define		MAXPKT		1526

typedef struct sr_basic_route_info {
	unsigned char 	sr_macaddr[MAC_ADDR_LEN];	/*The dest mac address*/
	ushort		sr_route_size;			/*Size of route field*/
	unsigned char 	sr_route[MAX_ROUTE_SIZE];	/*Src route info */
} sr_basic_route_info_t;

#define 	BASIC_ROUTE_INFO_SIZE		sizeof(sr_basic_route_info_t)
#define		SR_DUMP_ROUTE_TABLE		0xFE /*Only temp */

typedef struct sr_elem {
	unsigned char 	sr_macaddr[MAC_ADDR_LEN];	/*The dest mac address*/
	ushort		sr_route_size;			/*Size of route field*/
	unsigned char 	sr_route[MAX_ROUTE_SIZE];	/*Src route info */
	unsigned char 	sr_netno;			/*Direction of route */
	unsigned char 	sr_avail;			/*Availibility of slot*/
	struct sr_elem	*sr_next;
	caddr_t		sr_memp;
	ulong		sr_time;
} sr_elem_t;

#define	MAX_LIST_SIZE	\
	((SR_PAGE_SIZE - sizeof(struct sr_mem *) - sizeof(ulong)) / sizeof(sr_elem_t))
typedef struct sr_mem {
	struct sr_mem * sr_hdr_next;
	ulong	sr_num_avail;
	sr_elem_t sr_elem_list[MAX_LIST_SIZE];
} sr_mem_t;

#define		SR_UNUSED			0x0
#define		SR_WAITING_FOR_LOCAL_REPLY	0x1
#define		SR_WAITING_FOR_REMOTE_REPLY	0x2
#define		SR_ROUTE_KNOWN			0x4
#define		SR_ROUTE_UNKNOWN		0x8

typedef struct sr_wait_elem {
	unsigned char 	sr_wait_macaddr[MAC_ADDR_LEN];
	unsigned char 	sr_state;
	ushort		sr_timer_val;
	ushort		sr_route_size;
	unsigned char 	sr_route[MAX_ROUTE_SIZE];
	queue_t 	*sr_streamq;
	queue_t 	sr_waitq;
} sr_wait_elem_t;


typedef struct srdev {
	unsigned char 	macaddr[MAC_ADDR_LEN];
	queue_t *sr_qptr;
	ulong	std_addr_length;
	ushort sr_sap;
}srdev_t;

#endif	/* _IO_SR_SR_H */
