#ident	"@(#)icmp.h	1.4"
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
 */


#define	ICMP_PACKET_MAX	2048	/* Maximum ICMP packet size we expect */

extern task *icmp_task;
extern trace *icmp_trace_options;
extern const bits icmp_trace_types[];
#ifdef	ICMP_MAXTYPE
extern _PROTOTYPE(icmp_methods[ICMP_MAXTYPE],
		   void,
		   (task *,
		    sockaddr_un *,
		    sockaddr_un *,
		    struct icmp *,
		    size_t));
#endif	/* ICMP_MAXTYPE */

/* Tracing */

#define	TR_ICMP_DETAIL_REDIRECT	TR_DETAIL_1
#define	TR_ICMP_DETAIL_ROUTER	TR_DETAIL_2
#define	TR_ICMP_DETAIL_INFO	TR_DETAIL_3
#define	TR_ICMP_DETAIL_ERROR	TR_DETAIL_4

#define	TR_ICMP_INDEX_PACKETS	0
#define	TR_ICMP_INDEX_REDIRECT	1
#define	TR_ICMP_INDEX_ROUTER	2
#define	TR_ICMP_INDEX_INFO	3
#define	TR_ICMP_INDEX_ERROR	4

   
PROTOTYPE(icmp_var_init,
	  extern void,
	  (void));
PROTOTYPE(icmp_init,
	  extern void,
	  (void));
#if	defined(ICMP_SEND) && defined(ICMP_MAXTYPE)
PROTOTYPE(icmp_send,
	  extern int,
	  (struct icmp *,
	   size_t,
	   sockaddr_un *,
	   if_addr *,
	   flag_t));
#endif	/* defined(ICMP_SEND) && defined(ICMP_MAXTYPE) */
