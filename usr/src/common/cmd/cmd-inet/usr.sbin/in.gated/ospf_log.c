#ident	"@(#)ospf_log.c	1.3"
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

#define	INCLUDE_TIME
#include "include.h"
#include "inet.h"
#include "ospf.h"

const bits ospf_logtype[] =
{
    { 0,		"Monitor request" },			/* OSPF_RECV_MON */
    { 0,		"Hello" },				/* OSPF_RECV_HELLO */
    { 0,		"DB Description" },			/* OSPF_RECV_DD */
    { 0,		"Link-State Req" },			/* OSPF_RECV_LSR */
    { 0,		"Link-State Update" },			/* OSPF_RECV_LSU */
    { 0,		"Link-State Ack" },			/* OSPF_RECV_LSA */

    { 0,		"Monitor response" },			/* OSPF_SEND_MON */
    { 0,		"Hello" },				/* OSPF_SEND_HELLO */
    { 0,		"DB Description" },			/* OSPF_SEND_DD */
    { 0,		"Link-State Req" },			/* OSPF_SEND_LSR */
    { 0,		"Link-State Update" },			/* OSPF_SEND_LSU */
    { 0,		"Link-State Ack" },			/* OSPF_SEND_LSA */
		     /* -------------------------------- */
    { OSPF_LOGF_ALWAYS,	"IP: bad destination" },		/* OSPF_ERR_IP_DEST */
    { OSPF_LOGF_ALWAYS,	"IP: bad protocol" },			/* OSPF_ERR_IP_PROTO */
    { OSPF_LOGF_ALWAYS,	"IP: received my own packet" },		/* OSPF_ERR_IP_ECHO */
    { OSPF_LOGF_ALWAYS,	"OSPF: bad packet type" },		/* OSPF_ERR_OSPF_TYPE */
    { OSPF_LOGF_ALWAYS,	"OSPF: bad version" },			/* OSPF_ERR_OSPF_VERSION */
    { OSPF_LOGF_ALWAYS,	"OSPF: bad checksum" },			/* OSPF_ERR_OSPF_CHKSUM */
    { OSPF_LOGF_ALWAYS,	"OSPF: bad area id" },			/* OSPF_ERR_OSPF_AREAID */
    { OSPF_LOGF_ALWAYS,	"OSPF: area mismatch" },		/* OSPF_ERR_OSPF_ABR */
    { OSPF_LOGF_ALWAYS,	"OSPF: bad virtual link" },		/* OSPF_ERR_OSPF_VL */
    { OSPF_LOGF_ALWAYS,	"OSPF: bad authentication type" },	/* OSPF_ERR_OSPF_AUTH_TYPE */
    { OSPF_LOGF_ALWAYS,	"OSPF: bad authentication key" },	/* OSPF_ERR_OSPF_AUTH_KEY */
    { OSPF_LOGF_ALWAYS,	"OSPF: packet too small" },		/* OSPF_ERR_OSPF_SHORT */
    { OSPF_LOGF_ALWAYS,	"OSPF: packet size > ip length" },	/* OSPF_ERR_OSPF_LONG */
    { OSPF_LOGF_ALWAYS,	"OSPF: transmit error" },		/* OSPF_ERR_OSPF_SEND */
    { OSPF_LOGF_ALWAYS,	"OSPF: interface down" },		/* OSPF_ERR_OSPF_IFDOWN */
    { OSPF_LOGF_ALWAYS,	"OSPF: unknown neighbor" },		/* OSPF_ERR_OSPF_NBR */
    { OSPF_LOGF_ALWAYS,	"HELLO: netmask mismatch" },		/* OSPF_ERR_HELLO_MASK */
    { OSPF_LOGF_ALWAYS,	"HELLO: hello timer mismatch" },	/* OSPF_ERR_HELLO_TIMER */
    { OSPF_LOGF_ALWAYS,	"HELLO: dead timer mismatch" },		/* OSPF_ERR_HELLO_DEAD */
    { OSPF_LOGF_ALWAYS,	"HELLO: extern option mismatch" },	/* OSPF_ERR_HELLO_E */
    { OSPF_LOGF_ALWAYS,	"HELLO: router id confusion" },		/* OSPF_ERR_HELLO_ID */
    { OSPF_LOGF_TIMER,	"HELLO: virtual neighbor unknown" },	/* OSPF_ERR_HELLO_VIRT */
    { OSPF_LOGF_ALWAYS,	"HELLO: NBMA neighbor unknown" },	/* OSPF_ERR_HELLO_NBMA */
    { OSPF_LOGF_TIMER,	"DD: neighbor state low" },		/* OSPF_ERR_DD_STATE */
    { OSPF_LOGF_ALWAYS,	"DD: router id confusion" },		/* OSPF_ERR_DD_RTRID */
    { OSPF_LOGF_ALWAYS,	"DD: extern option mismatch" },		/* OSPF_ERR_DD_E */
    { 0,		"DD: unknown LSA type" },		/* OSPF_ERR_DD_TYPE */
    { OSPF_LOGF_TIMER,	"LS ACK: neighbor state low" },		/* OSPF_ERR_ACK_STATE */
    { OSPF_LOGF_TIMER,	"LS ACK: bad ack" },			/* OSPF_ERR_ACK_BAD */
    { OSPF_LOGF_TIMER,	"LS ACK: duplicate ack" },		/* OSPF_ERR_ACK_DUP */
    { OSPF_LOGF_ALWAYS,	"LS ACK: Unknown LSA type" },		/* OSPF_ERR_ACK_TYPE */
    { OSPF_LOGF_TIMER,	"LS REQ: neighbor state low" },		/* OSPF_ERR_REQ_STATE */
    { OSPF_LOGF_ALWAYS,	"LS REQ: empty request" },		/* OSPF_ERR_REQ_EMPTY */
    { OSPF_LOGF_ALWAYS,	"LS REQ: bad request" },		/* OSPF_ERR_REQ_BOGUS */
    { OSPF_LOGF_TIMER,	"LS UPD: neighbor state low" },		/* OSPF_ERR_UPD_STATE */
    { OSPF_LOGF_TIMER,	"LS UPD: newer self-gen LSA" },		/* OSPF_ERR_UPD_NEWER */
    { OSPF_LOGF_ALWAYS,	"LS UPD: LSA checksum bad" },		/* OSPF_ERR_UPD_CHKSUM */
    { OSPF_LOGF_TIMER,	"LS UPD: received less recent LSA" },	/* OSPF_ERR_UPD_OLDER */
    { 0,		"LS UPD: unknown LSA type" },		/* OSPF_ERR_UPD_TYPE */
};

u_long ospf_cumlog[OSPF_ERR_LAST] = { 0 };

u_int ospf_log_last_lsa;

void
ospf_log_rx __PF4(type, int,
		  intf, struct INTF *,
		  src, sockaddr_un *,
		  dst, sockaddr_un *)
{
    int pri;

    if (BIT_TEST(ospf_logtype[type].t_bits, OSPF_LOGF_ALWAYS)) {
	pri = LOG_WARNING;
    } else if (BIT_TEST(ospf_logtype[type].t_bits, OSPF_LOGF_TIMER)
	       && (!intf || OSPF_LOG_TIME(intf))) {
	pri = LOG_INFO;
    } else {
	pri = 0;
    }
    /* Only log the first log_first messages per type.  Then spit out */
    /* only one every log_every messages per type. */
    if ((ospf.log_first
	 && ospf_cumlog[type] > ospf.log_first)
	&& (ospf.log_every
	    && ospf_cumlog[type] % ospf.log_every)) {
	pri = 0;
    }

    if (intf && intf->area) {
	trace_log_tf(ospf.trace_options,
		     0,
		     pri,
		     ("OSPF RECV Area %A %-15A -> %A: %s",
		      sockbuild_in(0, intf->area->area_id),
		      src,
		      dst,
		      trace_state(ospf_logtype, type)));
    } else {
	trace_log_tf(ospf.trace_options,
		     0,
		     pri,
		     ("OSPF RECV %-15A -> %A: %s",
		      src,
		      dst,
		      trace_state(ospf_logtype, type)));
    }
}


/*
 * Change dest addr to string
 */
sockaddr_un *
ospf_addr2str __PF1(to, sockaddr_un *)
{
    switch (sock2ip(to)) {
    case ALL_UP_NBRS:
	return sockbuild_str("All_up_nbrs");

    case ALL_ELIG_NBRS:
	return sockbuild_str("All_elig_nbrs");

    case ALL_EXCH_NBRS:
	return sockbuild_str("All_exch_nbrs");

    case DR_and_BDR:
	return sockbuild_str("Dr_and_Bdr");
    }

    return to;
}
