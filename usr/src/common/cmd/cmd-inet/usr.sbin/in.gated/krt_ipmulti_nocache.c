#ident	"@(#)krt_ipmulti_nocache.c	1.4"
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


#define	INCLUDE_IOCTL
#define	INCLUDE_ROUTE
#include "include.h"

#ifdef	IP_MULTICAST

#include "inet.h"
#include "krt.h"
#include "krt_var.h"

typedef struct _krt_multicast_entry {
    struct _krt_multicast_entry *kme_forw;
    struct _krt_multicast_entry *kme_back;
    flag_t kme_flags;
#define	KMEF_INSTALLED	0x01		/* We installed it in the kernel */
#define	KMEF_EXISTS	0x02		/* Not installed by us */
    flag_t kme_refcount;
    sockaddr_un *kme_group;
    sockaddr_un *kme_router;		/* Router it is installed with */
} krt_multicast_entry;

static krt_multicast_entry krt_multicast_entries = { &krt_multicast_entries, &krt_multicast_entries } ;

#define	KRT_MC(p)	{ for (p = krt_multicast_entries.kme_forw; p != & krt_multicast_entries; p = p->kme_forw)
#define	KRT_MC_END(p)	if (p == &krt_multicast_entries) p = (krt_multicast_entry *) 0; }


static const bits krt_multicast_bits[] = {
    { KMEF_INSTALLED,	"Installed" },
    { KMEF_EXISTS,	"Exists" },
    { 0,		NULL }
} ;


/**/

/* The Deering multicast mods for the kernel require the existence of */
/* a default route (0.0.0.0), a default multicast route (224.0.0.0) or */
/* a specific multicast route to determine the default interface. */
/* This information is ignored since we explicitly set our interface */
/* when sending the packets, but lack of this information would prevent */
/* us from sending to the multicast address.  To work around this we */
/* add our two multicast addresses to the routing table pointing at the */
/* loopback interface.  If these were ever actually used we would get */
/* an error because the loopback interface does not support multicast, */
/* but since we do specify the interface before sending packets this */
/* should *never* happen.  */
	
block_t krt_mc_block = 0;


static krt_multicast_entry *
krt_multicast_create __PF1(group, sockaddr_un *)
{
    krt_multicast_entry *kp;

    if (!krt_mc_block) {
	krt_mc_block = task_block_init(sizeof (*kp), "krt_multicast_entry");
    }

    kp = (krt_multicast_entry *) task_block_alloc(krt_mc_block);
    INSQUE(kp, krt_multicast_entries.kme_back);

    kp->kme_group = sockdup(group);

    return kp;
}


static void
krt_multicast_request __PF2(type, u_int,
			    kp, krt_multicast_entry *)
{
    krt_parms parms;
    if_addr *ifap = (if_addr *) 0;
    sockaddr_un *router;

    parms.krtp_n_gw = 1;
    parms.krtp_state = 0;
    parms.krtp_protocol = RTPROTO_ANY;
    parms.krtp_ifaps = &ifap;
    parms.krtp_routers = &router;
    
    switch (type) {
    case RTM_ADD:
	router = inet_addr_loopback;
	(void) krt_change(krt_task,
			  kp->kme_group,
			  inet_mask_host,
			  (krt_parms *) 0,
			  &parms);

	BIT_SET(kp->kme_flags, KMEF_INSTALLED);
	BIT_RESET(kp->kme_flags, KMEF_EXISTS);
	break;

    case RTM_DELETE:
	router = kp->kme_router ? kp->kme_router : inet_addr_loopback;
	(void) krt_change(krt_task,
			  kp->kme_group,
			  inet_mask_host,
			  &parms,
			  (krt_parms *) 0);

	BIT_RESET(kp->kme_flags, KMEF_INSTALLED|KMEF_EXISTS);
	break;

    default:
	assert(FALSE);
    }

    if (kp->kme_router) {
	sockfree(kp->kme_router);
	kp->kme_router = (sockaddr_un *) 0;
    }
}


static void
krt_multicast_free __PF1(kp, krt_multicast_entry *)
{
    if (BIT_TEST(kp->kme_flags, KMEF_INSTALLED)) {
	krt_multicast_request(RTM_DELETE, kp);
    }
		
    REMQUE(kp);

    sockfree(kp->kme_group);
    task_block_free(krt_mc_block, (void_t) kp);
}


void
krt_multicast_add __PF1(group, sockaddr_un *)
{
    krt_multicast_entry *kp;

    KRT_MC(kp) {
	if (sockaddrcmp(group, kp->kme_group)) {
	    break;
	}
    } KRT_MC_END(kp);

    if (kp) {
	/* Exists */

	if (BIT_TEST(kp->kme_flags, KMEF_EXISTS)) {
	    /* Already exists in kernel */

	    /* Delete and reinstall the way we want it */
	    krt_multicast_request(RTM_DELETE, kp);
	    krt_multicast_request(RTM_ADD, kp);
	}
    } else {
	/* Allocate a new one */
	kp = krt_multicast_create(group);

	/* Install it in kernel */
	krt_multicast_request(RTM_ADD, kp);
    }

    kp->kme_refcount++;
}


void
krt_multicast_delete __PF1(group, sockaddr_un *)
{
    krt_multicast_entry *kp;

    KRT_MC(kp) {
	if (sockaddrcmp(group, kp->kme_group)) {
	    /* Found it */

	    if (!--kp->kme_refcount) {
		/* Last reference, remove it */

		krt_multicast_free(kp);
	    }
	    break;
	}
    } KRT_MC_END(kp);
}


int
krt_multicast_install __PF2(group, sockaddr_un *,
			    router, sockaddr_un *)
{
    krt_multicast_entry *kp;

    KRT_MC(kp) {
	if (sockaddrcmp(group, kp->kme_group)) {
	    if (BIT_TEST(kp->kme_flags, KMEF_EXISTS)) {
		/* Delete duplicate entries */

		return TRUE;
	    }

	    kp->kme_router = sockdup(router);
	    if (kp->kme_refcount) {
		/* One of ours, fix the router */

		krt_multicast_request(RTM_DELETE, kp);
		krt_multicast_request(RTM_ADD, kp);
	    }

	    return FALSE;
	}
    } KRT_MC_END(kp);

    kp = krt_multicast_create(group);

    kp->kme_router = sockdup(router);
    BIT_SET(kp->kme_flags, KMEF_EXISTS);

    return FALSE;
}


#ifdef	RTM_CHANGE
void
krt_multicast_change __PF2(type, int,
			   rtp, rt_parms *)
{
    krt_multicast_entry *kp;
    
    KRT_MC(kp) {
	if (sockaddrcmp(rtp->rtp_dest, kp->kme_group)) {
	    break;
	}
    } KRT_MC_END(kp);

    switch (type) {
    case RTM_DELETE:
    case RTM_OLDDEL:
	if (kp->kme_refcount) {
	    /* We are using this, force it to be reinstalled */

	    krt_multicast_request(RTM_ADD, kp);
	} else {
	    /* We are not using it, release it */

	    krt_multicast_free(kp);
	}
	break;

    case RTM_ADD:
    case RTM_OLDADD:
	krt_multicast_install(rtp->rtp_dest, rtp->rtp_router);
	break;

    case RTM_CHANGE:
	if (kp) {
	    if (kp->kme_refcount > 1) {
		/* We are using this, don't let it change */
		
		krt_multicast_request(RTM_DELETE, kp);
		krt_multicast_request(RTM_ADD, kp);
	    }
	} else {
	    /* We don't yet know about it */

	    krt_multicast_install(rtp->rtp_dest, rtp->rtp_router);
	}
	break;

    default:
	/* Ignore */
	break;
    }
}
#endif	/* RTM_CHANGE */


void
krt_multicast_dump __PF2(tp, task *,
			 fd, FILE *)
{
    if (krt_multicast_entries.kme_forw != &krt_multicast_entries) {
	krt_multicast_entry *kp;
	
	(void) fprintf(fd, "\tIP Multicast default interfaces:\n");

	KRT_MC(kp) {
	    (void) fprintf(fd, "\t\t%-15A\trefcount %d  flags <%s>\n",
			   kp->kme_group,
			   kp->kme_refcount,
			   trace_bits(krt_multicast_bits, kp->kme_flags));
	} KRT_MC_END(kp) ;

	(void) fprintf(fd, "\n");
    }
}

#endif	/* IP_MULTICAST */
