#ident	"@(#)str.h	1.3"
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


/* Definitions for putting data into and getting data out of packets */
/* in a machine dependent manner. */
#define	PickUp(s, d)	bcopy((caddr_t) s, (caddr_t)&d, sizeof(d));	s += sizeof(d);
#define	PutDown(s, d)	bcopy((caddr_t)&d, (caddr_t) s, sizeof(d));	s += sizeof(d);
#define	PickUpStr(s, d, l)	bcopy((caddr_t) s, (caddr_t) d, l);	s += l;
#define	PutDownStr(s, d, l)	bcopy((caddr_t) d, (caddr_t) s, l);	s += l;

#ifdef	notdef
#define	PickUp(s, d)	{ \
    register int PickUp_i = sizeof(d); \
    d = 0; \
    while (PickUp_i--) { \
	d <<= 8; \
        d |= *s++;
    } \
}
#define	PutDown(s, d)	{
    register int i = sizeof(d);
    register long ii = d;
    register caddr_t cp;

    cp = (s += i);
    while (i--) {
	*--cp = (ii & 0xff);
	ii >>= 8;
    }
}
#define	PickUpStr(s, d, l) {
    register int i = l;
    register char *cp = d;

    while (i--) {
	*cp++ = *s++;
    }
}
#define	PutDownStr(s, d, l) {
    register int i = l;
    register char *cp = s;

    while (i--) {
	*s++ = *cp++;
    }
}
#endif	/* notdef */

PROTOTYPE(gd_uplow,
	  extern char *,
	  (const char *,
	   int));
#define	gd_upper(str)	gd_uplow(str, TRUE)
#define	gd_lower(str)	gd_uplow(str, FALSE)
PROTOTYPEV(fprintf,
	   extern int,
	   (FILE *,
	    const char *,
	    ...));
PROTOTYPEV(vsprintf,
	   extern int,
	   (char *,
	    const char *,
	    va_list ));
PROTOTYPEV(sprintf,
	   extern int,
	   (char *,
	    const char *,
	    ...));
#ifdef	NEED_STRCASECMP
PROTOTYPE(strcasecmp,
	  extern int,
	  (const char *,
	   const char *));
PROTOTYPE(strncasecmp,
	  extern int,
	  (const char *,
	   const char *,
	   size_t));
#endif	/* NEED_STRCASECMP */
#ifdef	NEED_STRERROR
PROTOTYPE(strerror,
	  extern const char *,
	  (int));
#endif	/* NEED_STRERROR */
