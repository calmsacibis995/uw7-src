#ident	"@(#)paths.h	1.3"
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


/* Gated specific Paths from configuration */

#define	_PATH_CONFIG	"/etc/inet/gated.conf"
#define	_PATH_DEFS	"/etc/netmgt/gated-mib.defs"
#define	_PATH_DUMP	"/usr/tmp/%s_dump"
#define	_PATH_DUMPDIR	"/usr/tmp"
#define	_PATH_PID	"/etc/%s.pid"
#define	_PATH_VERSION	"/etc/%s.version"
#define	_PATH_BINDIR	"/usr/bin"
#define	_PATH_SBINDIR	"/usr/sbin"

/* System specific paths (not everyone runs BSD 4.4, do they) */
#ifndef	_PATH_DEVNULL
#define	_PATH_DEVNULL		"/dev/null"
#endif	/* _PATH_DEVNULL */

#ifndef	_PATH_DEV
#define	_PATH_DEV	"/dev"
#endif	/* _PATH_DEV */

#ifndef	_PATH_KMEM
#define	_PATH_KMEM	"/dev/kmem"
#endif	/* _PATH_KMEM */

#ifndef	_PATH_TTY
#define	_PATH_TTY	"/dev/tty"
#endif	/* _PATH_TTY */

#ifndef	_PATH_UNIX
#define	_PATH_UNIX	"/unix"
#endif	/* _PATH_UNIX */

#ifndef	_PATH_VARTMP
#define	_PATH_VARTMP	"/usr/tmp"
#endif	/* _PATH_VARTMP */

/* Non-path information */
#ifndef	CONFIG_MODE
#define	CONFIG_MODE	0664
#endif	/* CONFIG_MODE */

#ifndef	GDC_GROUP
#define	GDC_GROUP	"gdmaint"
#endif	/* GDC_GROUP */

#ifndef	NAME_GATED
#define	NAME_GATED	"gated"
#endif	/* NAME_GATED */

#ifndef	NAME_GDC
#define	NAME_GDC	"gdc"
#endif	/* NAME_GDC */

#ifndef	NAME_RIPQUERY
#define	NAME_RIPQUERY	"ripquery"
#endif	/* NAME_RIPQUERY */

#ifndef	NAME_OSPF_MONITOR
#define	NAME_OSPF_MONITOR	"ospf_monitor"
#endif	/* NAME_OSPF_MONITOR */
