#ident	"@(#)snmp_isode.h	1.3"
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


#if	defined(PROTO_ISODE_SNMP)

extern int doing_snmp;
extern task *snmp_task;
extern trace *snmp_trace_options;
extern const bits snmp_trace_types[];
extern pref_t snmp_preference;
extern u_short snmp_port;
extern int snmp_quantum;
extern int snmp_debug;
#ifdef	INCLUDE_ISODE_SNMP
extern OID snmp_nullSpecific;
#endif	/* INCLUDE_ISODE_SNMP */

#ifdef	SCO_GEMINI
char	PY_pepy[BUFSIZ];
#endif	/* SCO_GEMINI */

#define	SMUX_PORT	199

/* Tracing */

#define	TR_SNMP_RECV		TR_USER_1
#define	TR_SNMP_REGISTER	TR_USER_2
#define	TR_SNMP_RESOLVE		TR_USER_3
#define	TR_SNMP_TRAP		TR_USER_4

#define	oid2ipaddr(ip,addr) \
	oid2mediaddr ((ip), (byte*) (addr), sizeof (struct in_addr), 0)

#ifdef	INCLUDE_ISODE_SNMP
struct object_table {
    const char *ot_object;
    _PROTOTYPE(ot_getfunc,
	       int,
	       (OI,
		struct type_SNMP_VarBind *,
		int));
    _PROTOTYPE(ot_setfunc,
	       int,
	       (OI,
		struct type_SNMP_VarBind *,
		int));
    const int	ot_info;
    OT	ot_type;
};
#define	OTE(oid, read, write)	{ STRINGIFY(oid), read, write, I ## oid }

struct snmp_tree {
    struct snmp_tree *r_forw;
    struct snmp_tree *r_back;
    const char *r_text;			/* Name of this tree */
    OID r_name;				/* OID of the tree */
    const int r_mode;			/* Mode (readWrite, readOnly) */
    struct object_table *const r_table;	/* Table of objects */
    flag_t r_flags;			/* state */
#define	SMUX_TREE_REGISTER	0x01	/* Tree needs to be registered */
#define	SMUX_TREE_REGISTERED	0x02	/* Tree has been registered */
#define	SMUX_TREE_OBJECTS	0x04	/* Objects have been converted */
};

#endif	/* INCLUDE_ISODE_SNMP */

#define	STR_OID(ip, addr, len) { register byte *str_oid_cp = (byte *) addr; \
				    register int str_oid_i = len; \
				    while (str_oid_i--) *ip++ = *str_oid_cp++ & 0xff; }

#define	INT_OID(ip, int) *ip++ = int

#define	ot2object(ot)	((struct object_table *) ((void_t) ot->ot_info))

#define	snmp_last_free(last) \
	do { \
		 task_mem_free((task *) 0, (caddr_t) *(last)); \
		 *(last) = (unsigned int *) 0; \
	 } while (0)



PROTOTYPE(snmp_restart,
	  extern void,
	  (task *));
PROTOTYPE(snmp_init,
	  extern void,
	  (void));
PROTOTYPE(snmp_var_init,
	  extern void,
	  (void));
PROTOTYPE(snmp_last_match,
	  extern int,
	  (unsigned int **,
	   unsigned int *,
	   u_int,
	   int));
PROTOTYPE(oid2mediaddr,
	  extern int,
	  (unsigned int *,
	   byte *,
	   int,
	   int));

#ifdef	INCLUDE_ISODE_SNMP
PROTOTYPE(snmp_trap,
	  extern void,
	  (const char *,
	   OID,
	   int,
	   int,
	   struct type_SNMP_VarBindList *));
PROTOTYPE(snmp_tree_register,
	  extern void,
	  (struct snmp_tree *));
PROTOTYPE(snmp_tree_unregister,
	  extern void,
	  (struct snmp_tree *));

PROTOTYPE(snmp_varbinds_build,
	  extern int,
	  (int,
	   struct type_SNMP_VarBindList *,
	   struct type_SNMP_VarBind *,
	   int *,
	   struct snmp_tree *,
	   _PROTOTYPE(build,
		      int,
		      (OI,
		       struct type_SNMP_VarBind *,
		       int,
		       void_t)),
	   void_t));
PROTOTYPE(snmp_varbinds_free,
	  extern void,
	  (struct type_SNMP_VarBindList *));
#endif	/* INCLUDE_ISODE_SNMP */

#endif		/* defined(PROTO_ISODE_SNMP) */

