#ident	"@(#)ospf_log.h	1.3"
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


extern const char *ospf_con_types[];
extern const bits ospf_logtype[];
extern u_long ospf_cumlog[];
extern const char *ospf_intf_states[];
extern const char *ospf_nbr_states[];
extern const bits ospf_ls_type_bits[];
extern u_int ospf_log_last_lsa;
extern const bits ospf_nh_bits[];
extern const bits ospf_sched_bits[];
extern const bits ospf_router_type_bits[];

/**************************************************************************
	
			OLOG CODES

***************************************************************************/

#define	TR_OSPF_DETAIL_HELLO	TR_DETAIL_1
#define	TR_OSPF_DETAIL_DD	TR_DETAIL_2
#define	TR_OSPF_DETAIL_LSR	TR_DETAIL_3
#define	TR_OSPF_DETAIL_LSU	TR_DETAIL_4
#define	TR_OSPF_DETAIL_ACK	TR_DETAIL_5

#define	TR_OSPF_INDEX_PACKETS	0
#define	TR_OSPF_INDEX_HELLO	1
#define	TR_OSPF_INDEX_DD	2
#define	TR_OSPF_INDEX_LSR	3
#define	TR_OSPF_INDEX_LSU	4
#define	TR_OSPF_INDEX_ACK	5

#define	TR_OSPF_LSA_BLD	TR_USER_1
#define	TR_OSPF_SPF	TR_USER_2
#define	TR_OSPF_LSA_TX	TR_USER_3
#define	TR_OSPF_LSA_RX	TR_USER_4
#define	TR_OSPF_TRAP	TR_USER_5
#ifdef	DEBUG
#define	TR_OSPF_DEBUG	TR_USER_6
#endif	/* DEBUG */

extern const flag_t ospf_trace_masks[];

#define GOOD_RX			0

/* receive types */
#define	OSPF_RECV_MON		0
#define	OSPF_RECV_HELLO		1
#define	OSPF_RECV_DD		2
#define	OSPF_RECV_LSR		3
#define	OSPF_RECV_LSU		4
#define	OSPF_RECV_LSA		5

#define	OSPF_SEND_BASE		6
#define	OSPF_SEND_MON		6
#define	OSPF_SEND_HELLO		7
#define	OSPF_SEND_DD		8
#define	OSPF_SEND_LSR		9
#define	OSPF_SEND_LSU		10
#define	OSPF_SEND_LSA		11

#define	OSPF_ERR_BASE		12
#define OSPF_ERR_IP_DEST	12
#define OSPF_ERR_IP_PROTO	13
#define	OSPF_ERR_IP_ECHO	14
#define	OSPF_ERR_OSPF_TYPE	15
#define OSPF_ERR_OSPF_VERSION	16
#define	OSPF_ERR_OSPF_CHKSUM	17
#define OSPF_ERR_OSPF_AREAID	18
#define OSPF_ERR_OSPF_ABR	19
#define OSPF_ERR_OSPF_VL	20
#define	OSPF_ERR_OSPF_AUTH_TYPE	21
#define	OSPF_ERR_OSPF_AUTH_KEY	22
#define	OSPF_ERR_OSPF_SHORT	23
#define OSPF_ERR_OSPF_LONG	24
#define	OSPF_ERR_OSPF_SEND	25
#define	OSPF_ERR_OSPF_IFDOWN   	26
#define	OSPF_ERR_OSPF_NBR	27
#define OSPF_ERR_HELLO_MASK  	28
#define OSPF_ERR_HELLO_TIMER 	29
#define OSPF_ERR_HELLO_DEAD	30
#define OSPF_ERR_HELLO_E 	31
#define OSPF_ERR_HELLO_ID	32
#define OSPF_ERR_HELLO_VIRT	33
#define OSPF_ERR_HELLO_NBMA	34
#define OSPF_ERR_DD_STATE	35
#define OSPF_ERR_DD_RTRID	36
#define OSPF_ERR_DD_E		37
#define OSPF_ERR_DD_TYPE	38
#define OSPF_ERR_ACK_STATE	39
#define	OSPF_ERR_ACK_BAD	40
#define	OSPF_ERR_ACK_DUP	41
#define OSPF_ERR_ACK_TYPE	42
#define OSPF_ERR_REQ_STATE	43
#define OSPF_ERR_REQ_EMPTY	44
#define OSPF_ERR_REQ_BOGUS     	45
#define OSPF_ERR_UPD_STATE	46
#define OSPF_ERR_UPD_NEWER	47
#define OSPF_ERR_UPD_CHKSUM   	48
#define OSPF_ERR_UPD_OLDER	49
#define OSPF_ERR_UPD_TYPE	50
#define OSPF_ERR_LAST		51


/* Flags to indicate when we should log */

#define	OSPF_LOGF_NEVER		0x0	/* Never syslog */
#define	OSPF_LOGF_ALWAYS	0x01	/* Always log */
#define	OSPF_LOGF_TIMER		0x02	/* After startup */

/* */

#define	OSPF_LOG_TIME(intf)	(time_sec - (intf)->up_time > (intf)->dead_timer * 2)
#define	OSPF_LOG_RECORD(type)	{assert(type < OSPF_ERR_LAST); ospf_cumlog[type]++;}
#define	OSPF_LOG_RECORD_TX(type)	{assert(type < OSPF_SEND_BASE); ospf_cumlog[type + OSPF_SEND_BASE]++;}
#define	OSPF_LOG_RX(type, intf, src, dst) { OSPF_LOG_RECORD(type); if (type >= OSPF_ERR_BASE) ospf_log_rx(type, intf, src, dst); }
#define	OSPF_LOG_RX_LSA1(type, intf, src, dst, db, desc, age) { \
    OSPF_LOG_RECORD(type); \
    if (ospf_log_last_lsa != (type)) { \
	ospf_log_last_lsa = (type); \
	ospf_log_rx(type, intf, src, dst); \
    } \
    ospf_log_ls_hdr(db, desc, age, (time_t) 0); \
}
#define	OSPF_LOG_RX_LSA2(type, intf, src, dst, lsa, db) { \
    OSPF_LOG_RECORD(type); \
    if (ospf_log_last_lsa != (type)) { \
	ospf_log_last_lsa = (type); \
	ospf_log_rx(type, intf, src, dst); \
    } \
    ospf_log_ls_hdr(lsa, "	RECV", (lsa)->ls_age, (time_t) 0); \
    ospf_log_ls_hdr(&DB_RTR(db)->ls_hdr, "	HAVE", LS_AGE(db), db->lsdb_time); \
}
