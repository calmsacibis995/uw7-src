#ifndef __TC_NET_H__
#define	__TC_NET_H__

#include <sys/types.h>
#include <dl.h>

#define	TN_LISTEN		0x1	/* Listener thread */
#define	TN_PHYS_PROMISCUOUS	0x2	/* Phys level Promiscuous thread */
#define	TN_SAP_PROMISCUOUS	0x4	/* SAP  level Promiscuous thread */

#define DL_MAC_STR		18
#define	DL_MAX_NODES		3
#define DL_NODE_NAMESZ		40
#define DL_MAX_PKTLEN		1500
#define	DL_OP_MAX_DATASZ 	1400 	/*64  is good number!!*/
#define DL_RETRY_CNT		3
#define DL_RETRY_TIMEOUT 	5
#define DL_FRTEST_LEN		50
/*
 * DLPI net test packet types..
 */
#define DL_RING_REQ	1	/* pass it to the next neigbour*/
#define DL_ECHO_REQ	2	
#define DL_ECHO_REP	3
#define	DL_ADD_MADDR	4	/* add a multicast address */
#define	DL_DEL_MADDR	5	/* del a multicast address */
#define DL_MADDR_DONE	6
#define DL_START_REQ 	7
#define DL_START_REP	8
#define DL_STOP_REQ	9
#define DL_STOP_REP 	10
#define DL_FRAME_FAIL	12
#define DL_FRAME_OK	13
#define DL_FRAME_NOTOK	14
#define DL_SET_PROMISC	15
#define DL_CLEAR_PROMISC 16
#define DL_MODE_PASS	17
#define DL_MODE_FAIL 	18
#define DL_CHK_FRAME	20
#define DL_CHK_FRAME_OK	21
#define DL_LOOP_REQ	'llll'
/*
 * DLPI net test type.. 
 */
#define DL_LINK_TEST	1	
#define DL_EADDR_TEST	2
#define	DL_MADDR_TEST	3
#define	DL_PROMISC_TEST	4
#define DL_STRESS_TEST  5
#define DL_TESTREQ_TEST 6
#define DL_OSNAP_TEST	7
#define DL_O8022_TEST	8
#define DL_OETHII_TEST	9
#define DL_SLPCFLG_TEST 10
#define DL_MODE_TEST	11
#define	DL_SAPPROMISC_TEST	12
#define	DL_PHYSPROMISC_TEST	13
/*
 * DLPI net minitor event type..
 */
#define DL_NOEVENT	0
#define DL_TIMEOUT	1
#define DL_LINK		2
#define DL_EADDR	3
#define DL_MADDR	4
#define	DL_PROMISC	5
#define DL_START	6
#define DL_STOP		7
#define DL_TESTCON	8
#define DL_FRFAIL	9
#define DL_FROK		10
#define DL_FRNOTOK	11
#define DL_SLPCFLG	12
#define DL_MODEPASS	13
#define DL_MODEFAIL	14
#define DL_LOOPFAIL	15
#define DL_LOOPPASS	16
#define DL_XIDCON	17

#define TN_NEXT_TO_MON 1

/* DLPI net init errors */
#define NET_OPEN_FAIL		-1
#define NET_BIND_FAIL		-2
#define NET_GETMACADDR_FAIL	-3
#define NET_NODE_NAME_FAIL	-4
#define NET_NOT_CONFIGURED	-5
#define NET_GEN_CONFIG_FAIL	-6
#define NET_SAME_PHYSNET_FAIL	-7

#define NET_INIT_ERRS		8

/*
 * Ethernet Frametypes ..
 */
#define ETHERNET_II	0
#define ETHERNET_8022   1
#define ETHERNET_SNAP	2
#define ETHERNET_8023	3
#define TOKEN_RING	4
#define TOKEN_RING_SNAP	5

typedef struct _dl_node {
	char 		dn_name[DL_NODE_NAMESZ];	/* the node name   */	
	struct llcc	dn_llc_addr;			/* the mac address */
	int		dn_status; 			/* the node status */
	char		dn_macstr[DL_MAC_STR];		/* the mac addres str */
} dl_node_t;
 
typedef struct _dl_pkt {
	int		dp_primitive;			/* the opcode */
	int		dp_sender;			/* the sender no */
	int		dp_test;			/* test code */
	int		dp_len;				/* len of rest data */
/*	char		dp_data[DL_OP_MAX_DATASZ]; */	/* you know it!! */	
} dl_pkt_t;

extern dl_node_t	tn_net[]; 
extern int		tn_gui;		/* gui mode or not ? */
extern int		tn_node;	/* my rank in the configuration array */
extern int		tn_listen_fd;	/* the DLPI listen stream fd */
extern int		tn_promis_fd;	/* the DLPI promiscuous stream fd */ 
extern int		*tn_eventp;	/* the monitor event */
extern int		*tn_pktcntp;	/* echo reply cnt in stress test */
extern int 		tn_rawmode;	/* listener in RAWCSMACD mode*/
extern int		tn_listener;	/* the listener pid */
extern int		tn_promisc;	/* the promiscuous pid */
extern char		tn_name[];	/* the node name */
extern char		tn_test_maddr[];
extern char		tn_test_eaddr[]; 
extern char		*nodenames[];
extern int		tn_max_nodes;	
extern char		*dl_sig;

int		getdevname();

#define DL_SHM_KEY	0xdead

extern	void *getshm();

#endif

