#ident	"@(#)standalone.c	1.3"
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

#define	MALLOC_OK
#include "include.h"

/* Support for stand-alone programs (ripquery, gdc) */

struct gtime task_time;

const bits ll_type_bits[] = {
    { LL_OTHER,		"Unknown" },
    { LL_8022,		"802.2" },
    { LL_X25,		"X.25" },
    { LL_PRONET,	"ProNET" },
    { LL_HYPER,		"HyperChannel" }
};


void
task_assert __PF3(file, const char *,
		  line, const int,
		  test, const char *)
{
    fprintf(stderr,
	    "Assertion failed: file \"%s\", line %d: %s",
	    file,
	    line,
	    test);

    /* Exit with a core dump */
    abort();
}


void_t
task_mem_malloc __PF2(tp, task *,
		      size, size_t)
{
    void_t p;

    p = (void_t) malloc(size);
    if (!p) {
	(void) fprintf(stderr,
		       "malloc: Can not malloc(%d)",
		       size);
	abort();
    }

    return p;
}


void_t
task_mem_calloc __PF3(tp, task *,
		      number, u_int,
		      size, size_t)
{
    void_t p;

    p = (void_t) calloc(number, size);
    if (!p) {
	(void) fprintf(stderr,
		       "calloc: Can not calloc(%d, %d)",
		       number,
		       size);
	abort();
    }

    return p;
}


/*ARGSUSED*/
void
task_mem_free __PF2(tp, task *,
		    p, void_t)
{
    if (p) {
	free((caddr_t) p);
    }
}


/**/

u_short
task_get_port __PF4(tf, trace *,
		    name, const char *,
		    proto, const char *,
		    default_port, u_short)
{
    struct servent *se = getservbyname((char *)name, proto);
    u_short port;

    if (se) {
	port = se->s_port;
    } else {
	port = default_port;
	(void) fprintf(stderr,
		       "task_get_port: getservbyname(\"%s\", \"%s\") failed, using port %d\n",
		       name,
		       proto,
		       htons(port));
    }

    return port;
}

int
task_get_proto __PF3(tf, trace *,
		     name, const char *,
		     default_proto, int)
{
    struct protoent *pe = getprotobyname(name);
    int proto;

    if (pe) {
	proto = pe->p_proto;
    } else {
	proto = default_proto;
	(void) fprintf(stderr,
		       "task_get_proto: getprotobyname(\"%s\") failed, using proto %d\n",
		       name,
		       proto);
    }

    return proto;
}
