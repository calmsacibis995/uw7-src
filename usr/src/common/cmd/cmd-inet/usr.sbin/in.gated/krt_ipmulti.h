#ident	"@(#)krt_ipmulti.h	1.3"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 *
 *  Author: Tom Pusateri <pusateri@netedge.com>
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


/*
 * Create callback list of routines to be called when
 * we are asked to resolve a forwarding cache entry.
 */

struct krt_mfc_recv {
    struct krt_mfc_recv *forw, *back;
    int errno;
    void (*recv_routine)();
};

#define	MFC_RECV_SCAN(cur, head) \
		{ for (cur = (head)->forw; cur != head; cur = cur->forw)
#define MFC_RECV_SCAN_END(cur, head) \
		if (cur == head) cur = (struct krt_mfc_recv *) 0; }

extern struct krt_mfc_recv krt_mfc_recv_head;

PROTOTYPE(krt_init_mfc,
	  extern void,
	  (void));

PROTOTYPE(krt_generate_mfc,
	  extern void,
	  (int,
	   sockaddr_un *,
	   if_addr *,
	   sockaddr_un *));

PROTOTYPE(krt_update_mfc,
	  extern void,
	  (mfc *));

PROTOTYPE(krt_check_mfc,
	  extern void,
	  (int,
	   sockaddr_un *,
	   sockaddr_un *));

PROTOTYPE(krt_register_mfc,
	  extern void,
	  (int,
	  _PROTOTYPE(func,
		     void,
		     (int,
		      if_addr *,
		      mfc *))));

PROTOTYPE(krt_unregister_mfc,
	  extern void,
	  (int,
	  _PROTOTYPE(func,
		     void,
		     (int,
		      if_addr *,
		      mfc *))));

PROTOTYPE(krt_resolve_cache,
	  extern void,
	  (sockaddr_un *,
	   sockaddr_un *,
	   mfc *));

PROTOTYPE(krt_delete_cache,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *));

PROTOTYPE(krt_request_cache,
	  extern int,
	  (mfc *,
	   _PROTOTYPE(callback,
		      void,
		      (mfc *))));

PROTOTYPE(krt_enable_cache,
	  extern void,
	  (task *));

PROTOTYPE(krt_disable_cache,
	  extern void,
	  (task *));

PROTOTYPE(krt_locate_upstream,
	  extern upstream *,
	  (sockaddr_un *,
	   int));

PROTOTYPE(krt_add_vif,
	  extern void,
	  (if_addr *,
	   u_int32,
	   u_int32));

PROTOTYPE(krt_add_tunnel,
	  extern void,
	  (if_addr *,
	   sockaddr_un *,
	   u_int32,
	   u_int32));

PROTOTYPE(krt_del_vif,
	  extern void,
	  (if_addr *));
