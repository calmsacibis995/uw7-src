#ident	"@(#)ospf_pkts.h	1.3"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * ------------------------------------------------------------------------
 * 
 *                 U   U M   M DDDD     OOOOO SSSSS PPPPP FFFFF
 *                 U   U MM MM D   D    O   O S     P   P F
 *                 U   U M M M D   D    O   O  SSS  PPPPP FFFF
 *                 U   U M M M D   D    O   O     S P     F
 *                  UUU  M M M DDDD     OOOOO SSSSS P     F
 * 
 *     		          Copyright 1989, 1990, 1991
 *     	       The University of Maryland, College Park, Maryland.
 * 
 * 			    All Rights Reserved
 * 
 *      The University of Maryland College Park ("UMCP") is the owner of all
 *      right, title and interest in and to UMD OSPF (the "Software").
 *      Permission to use, copy and modify the Software and its documentation
 *      solely for non-commercial purposes is granted subject to the following
 *      terms and conditions:
 * 
 *      1. This copyright notice and these terms shall appear in all copies
 * 	 of the Software and its supporting documentation.
 * 
 *      2. The Software shall not be distributed, sold or used in any way in
 * 	 a commercial product, without UMCP's prior written consent.
 * 
 *      3. The origin of this software may not be misrepresented, either by
 *         explicit claim or by omission.
 * 
 *      4. Modified or altered versions must be plainly marked as such, and
 * 	 must not be misrepresented as being the original software.
 * 
 *      5. The Software is provided "AS IS". User acknowledges that the
 *         Software has been developed for research purposes only. User
 * 	 agrees that use of the Software is at user's own risk. UMCP
 * 	 disclaims all warrenties, express and implied, including but
 * 	 not limited to, the implied warranties of merchantability, and
 * 	 fitness for a particular purpose.
 * 
 *     Royalty-free licenses to redistribute UMD OSPF are available from
 *     The University Of Maryland, College Park.
 *       For details contact:
 * 	        Office of Technology Liaison
 * 		4312 Knox Road
 * 		University Of Maryland
 * 		College Park, Maryland 20742
 * 		     (301) 405-4209
 * 		FAX: (301) 314-9871
 * 
 *     This software was written by Rob Coltun
 *      rcoltun@ni.umd.edu
 */


#define NOT_RETRANS 0
#define IS_RETRANS 1

/***************************************************************************

	   		PACKET HEADER STRUCTURES

****************************************************************************/

/* General Link State Advertisement Hdr */

struct LS_HDR {
    u_int16	ls_age;
    u_int8	ls_options;
    u_int8	ls_type;		/* Type of advertisement */
    u_int32	ls_id;		/* Link State Id */
    u_int32	adv_rtr;		/* Advertising router */
    u_int32	ls_seq;
    u_int16	ls_chksum;
    u_int16	length;		/* ls adv length */
};

/* 	MONITOR REQUEST PACKET */
#define MREQUEST 	1
#define MAXPARAMS	12

/*
 *	Umd private Monitor packet header
 */
struct MON_HDR {
    u_int8 type, req, more, param_cnt;
    u_int16 port;		/* port that gw writes to */
    u_int16 local;		/* true if returning info */
    u_int32 p[MAXPARAMS];
};

#define MON_REQ_SIZE ((size_t) (8 + (MAXPARAMS * 4)))

/*	HELLO PACKET */

struct RHF {			/* router heard from */
    u_int32 rtr;		/* part of the hello hdr, IDs of recently
				 * seen (via hellos) rtrs on this net */
};

#define RHF_SIZE ((size_t) 4)

#define OPT_T_bit	0x01	/* TOS */
#define OPT_E_bit	0x02	/* Externals */
#define	OPT_M_bit	0x04	/* Multicast */
#define	OPT_N_bit	0x08	/* NSSA */
#define	OPT_DP_bit	0x10	/* Don't propagate */

struct HELLO_HDR {
    u_int32 netmask;		/* net mask */
    u_int16 HelloInt;		/* seconds between this rtr's Hello packets */
    u_int8 options;		/* options capabilities supported by router */
    u_int8 rtr_priority;	/* this router's priority */
    u_int32 DeadInt;		/* seconds before declaring this router down */
    u_int32 dr;		/* ID of DR for this net */
    u_int32 bdr;		/* ID of the backup DR for this net */
    struct RHF rhf;		/* router heard from */
};

#define HELLO_HDR_SIZE ((size_t) 20)	/* less RHF */

/*	DATABASE HEADER */

#define DB_PIECE_SIZE	((size_t) 20)


struct DB_HDR {
    u_int16 phill2;
    u_int8 options;
    u_int8 I_M_MS;		/* bits: init, more, master/slave */
#define		bit_I	0x04
#define		bit_M 	0x02
#define		bit_MS	0x01
    s_int32 seq;		/* seq num of the database description pkts */
    struct LS_HDR dbp;
};

#define DB_HDR_SIZE	((size_t) 8)


/*	LINK STATE REQUEST PACKET */

struct LS_REQ_PIECE {
    u_int16 phill2;
    u_int8 phill1;
    u_int8 ls_type;		/* Type of the piece */
    u_int32 ls_id;		/* Link State Id */
    u_int32 adv_rtr;		/* Advertising router */
};

#define LS_REQ_PIECE_SIZE ((size_t) 12)

struct LS_REQ_HDR {
    struct LS_REQ_PIECE req;
};


/*	LINK STATE UPDATE PACKETS */

/* Link State Types */

#define LS_STUB		0
#define LS_RTR		1
#define LS_NET		2
#define LS_SUM_NET	3
#define LS_SUM_ASB	4
#define LS_ASE		5
#define	LS_GM		6
#define	LS_NSSA		7

#define	LS_MAX		8


/* Router Advertisement */

struct RTR_LA_METRIC {
    u_int16 tos;		/* type of service; used with mask */
#define		RTR_LS_TOS_MASK  0x0700
    u_int16 metric;		/* cost of using this interface */
};

#define RTR_LA_METRIC_SIZE ((size_t) 4)

struct RTR_LA_PIECES {
    u_int32 lnk_id;		/* interface ID */
    u_int32 lnk_data;		/* net mask */
    u_int8 con_type;		/* what the interface connects to */
#define		RTR_IF_TYPE_RTR		1
#define		RTR_IF_TYPE_TRANS_NET	2
#define		RTR_IF_TYPE_STUB_NET	3
#define		RTR_IF_TYPE_HOST	RTR_IF_TYPE_STUB_NET
#define		RTR_IF_TYPE_VIRTUAL	4
#define 	HOST_NET_MASK 		0xFFFFFFFF
    u_int8 metric_cnt;		/* number of metrics */
    u_int16 tos0_metric;	/* metric for TOS 0 */
};

#define RTR_LA_PIECES_SIZE  ((size_t) 12)


struct RTR_LA_HDR {
    struct LS_HDR ls_hdr;	/* General hdr - type is LS_RTR */
    u_int16 E_B;		/* external or border bits; used with mask */
#define		bit_B	0x100	/* border bit mask */
#define		bit_E	0x200	/* external bit mask */
#define		bit_W	0x400	/* Multicast forwarder */
    u_int16 lnk_cnt;		/* number of links */
    struct RTR_LA_PIECES link;
};

#define RTR_LA_HDR_SIZE ((size_t) 24)	/* less the pieces */

/*  Network Links Advertisement */

struct NET_LA_PIECES {
    u_int32 lnk_id;
};

#define NET_LA_PIECES_SIZE ((size_t) 4)

struct NET_LA_HDR {
    struct LS_HDR ls_hdr;	/* General hdr - type is LS_NET */
    u_int32 net_mask;
    struct NET_LA_PIECES att_rtr;
};

#define NET_LA_HDR_SIZE ((size_t) 24)


/*  Summary links advertisements */

struct SUM_LA_PIECES {
    u_int32 tos_metric;
#define		 SUM_LA_TOS_MASK  0x7000000
};

#define SUM_LA_PIECES_SIZE ((size_t) 4)

struct SUM_LA_HDR {
    struct LS_HDR ls_hdr;	/* General hdr - type is LS_SUM_NET or RTR */
    u_int32 net_mask;
    struct SUM_LA_PIECES tos0;
};

#define SUM_LA_HDR_SIZE ((size_t) 28)


/* AS External links advertisments */


struct ASE_LA_PIECES {
    u_int32 tos_metric;	/* 0 = type 1; like link state
			   1 = type 2; > than link state */
#define		ASE_bit_E	0x80000000
#define		ASE_TOS_MASK	0x7F000000
    u_int32 ForwardAddr;
    u_int32 ExtRtTag;
};

#define ASE_LA_PIECES_SIZE	((size_t) 12)

struct ASE_LA_HDR {
    struct LS_HDR ls_hdr;	/* General LS hdr - type is LS_ASE */
    u_int32 net_mask;
    struct ASE_LA_PIECES tos0;
};

#define ASE_LA_HDR_SIZE ((size_t) 36)

/* Group Membership Link State Advertisement	*/


struct GM_LA_PIECES {
    u_int32 type;		/* vertex type	*/
    u_int32 id;			/* vertex id	*/
};

#define	GM_TYPE_RTR	1	/* Router */
#define	GM_TYPE_NET	2	/* Transit Network */

#define GM_LA_PIECES_SIZE ((size_t) 8)

struct GM_LA_HDR {
    struct LS_HDR ls_hdr;	/* General hdr - type is LS_GM	*/
    struct GM_LA_PIECES vertex;	/* repeats	*/	
};

#define GM_LA_HDR_SIZE	((size_t) 20)

/* Defines for SUM and ASE metrics */
#define METRIC_MASK 0x00FFFFFF
#define	BIG_METRIC(V) (ntohl(DB_ASE((V))->tos0.tos_metric) & METRIC_MASK)
#define	ADV_BIG_METRIC(A) (ntohl((A)->tos0.tos_metric) & METRIC_MASK)

#define ASE_TYPE2(A)  ((ntohl(DB_ASE((A))->tos0.tos_metric) & ASE_bit_E) != 0)
#define ADV_ASE_TYPE2(A)  ((ntohl((A)->tos0.tos_metric) & ASE_bit_E) != 0)
#define ASE_TYPE1(A)  ((ntohl(DB_ASE((A))->tos0.tos_metric) & ASE_bit_E) == 0)


union ADV {
    struct RTR_LA_HDR rtr;
    struct NET_LA_HDR net;
    struct SUM_LA_HDR sum;
    struct ASE_LA_HDR ase;
    struct GM_LA_HDR gm;
};

struct LS_UPDATE_HDR {
    u_int32 adv_cnt;		/* Number of link state advertisements */
    union ADV adv;
};

#define LS_UPDATE_HDR_SIZE ((size_t) 4)

/*	LINK STATE ACK PACKET */

#define ACK_PIECE_SIZE	((size_t) 20)

struct LS_ACK_HDR {
    struct LS_HDR ack_piece;
};

#define LS_ACK_HDR_SIZE ((size_t) 4)

/*	OSPFIGP PACKET HEADER */

union PACKET_TYPES {		/* The rest of the packet */
    struct MON_HDR mon;
    struct HELLO_HDR hello;
    struct DB_HDR database;
    struct LS_REQ_HDR ls_req;
    struct LS_UPDATE_HDR ls_update;
    struct LS_ACK_HDR ls_ack;
};

#define	OSPF_AUTH_SIMPLE_SIZE	8
#define	OSPF_AUTH_MD5_SIZE	16
#define	OSPF_AUTH_SIZE		(MAX(OSPF_AUTH_SIMPLE_SIZE, OSPF_AUTH_MD5_SIZE) / sizeof (u_int32))

struct OSPF_HDR {
    u_int8 ospfh_version;
    u_int8 ospfh_type;
#define		OSPF_PKT_MON	0	/* monitor request */
#define		OSPF_PKT_HELLO	1	/* hello */
#define		OSPF_PKT_DD	2	/* database description */
#define		OSPF_PKT_LSR	3	/* link state request */
#define		OSPF_PKT_LSU	4	/* link state update */
#define		OSPF_PKT_ACK	5	/* link state ack */
#define		OSPF_PKT_MAX	6
    u_int16 ospfh_length;	/* length of entire packet in bytes */
    u_int32 ospfh_rtr_id;		/* router ID */
    u_int32 ospfh_area_id;		/* area ID */
    u_int16 ospfh_checksum;
    u_int16 ospfh_auth_type;	/* Authentication scheme */
    union {
	u_int32 auth_key[OSPF_AUTH_SIMPLE_SIZE / sizeof (u_int32)];
	struct {
	    u_int16 md5_offset;
	    u_int16 md5_instance;
	    u_int32 md5_sequence;
	} ospfh_us;
    } ospfh_u;
#define	ospfh_auth_key		ospfh_u.auth_key
#define	ospfh_md5_offset	ospfh_u.ospfh_us.md5_offset
#define	ospfh_md5_instance	ospfh_u.ospfh_us.md5_instance
#define	ospfh_md5_sequence	ospfh_u.ospfh_us.md5_sequence
    union PACKET_TYPES ospfh_un;
};

#define OSPF_HDR_SIZE ((size_t) 24)

#define	OSPF_TRL_SIZE(intf)	(((intf)->area->authtype == OSPF_AUTH_MD5) ? OSPF_AUTH_MD5_SIZE : 0)

#define OSPF_HDR_NULL	((struct OSPF_HDR *) 0)


#define	NEW_LSU(pkt, adv_cnt, adv)	{ \
    pkt = task_get_send_buffer(struct OSPF_HDR *); \
    adv = (union ADV *) &pkt->ospfh_un.ls_update.adv; \
    adv_cnt = (u_int32 *) &pkt->ospfh_un.ls_update.adv_cnt; \
    *adv_cnt = 0; \
}
