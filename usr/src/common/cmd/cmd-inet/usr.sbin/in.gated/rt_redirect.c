#ident	"@(#)rt_redirect.c	1.3"
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


#define	INCLUDE_RT_VAR
#include "include.h"
#include "inet.h"
#include "krt.h"

#define	REDIRECT_T_EXPIRE	((time_t) 180)

/*
 * redirect() changes the routing tables in response to a redirect
 * message or indication from the kernel
 */

trace *redirect_trace_options = 0;	/* Trace flags from parser */
int redirect_n_trusted = 0;		/* Number of trusted ICMP gateways */
pref_t redirect_preference = RTPREF_REDIRECT;	/* Preference for ICMP redirects */
gw_entry *redirect_gw_list = 0;		/* Active ICMP gateways */
adv_entry *redirect_import_list = 0;	/* List of nets to import from ICMP */
adv_entry *redirect_int_policy = 0;	/* List of interface policy */
const bits redirect_trace_types[] = {
    { 0, NULL }
} ;

static task *redirect_task;
static flag_t redirect_ignore;
static flag_t redirect_ignore_save;
static rt_parms redirect_rtparms = RTPARMS_INIT(1,
						(metric_t) 0,
						(flag_t) 0,
						(pref_t) 0);

void
redirect __PF4(dst, sockaddr_un *,
	       mask, sockaddr_un *,
	       gateway, sockaddr_un *,
	       src, sockaddr_un *)
{
    rt_entry *rt, *old_rt = (rt_entry *) 0;
    int interior = 0;
    krt_parms *krtp = (krt_parms *) 0;
#ifdef	PROTO_INET
    int change_to_host = 0;
#else
#define	change_to_host	0
#endif	/* PROTO_INET */
    if_addr *ifap;

    rt_open(redirect_task);

    switch (socktype(dst)) {
#ifdef	PROTO_INET
    case AF_INET:
	if (mask && mask != inet_mask_host) {
	    /* We need to treat this as a host redirect */

	    change_to_host++;
	}
	redirect_rtparms.rtp_dest_mask = inet_mask_host;
	break;
#endif	/* PROTO_INET */

    default:
        if (mask) {
	    /* Use supplied mask */

	    redirect_rtparms.rtp_dest_mask = mask;
	} else {
	    /* Derive a host mask */
	    
	    redirect_rtparms.rtp_dest_mask = sockhostmask(dst);
	}
	/* Apply mask to the destination */
	sockmask(dst, redirect_rtparms.rtp_dest_mask);
	break;
    }

    redirect_rtparms.rtp_dest = dst;
    redirect_rtparms.rtp_router = gateway;
    redirect_rtparms.rtp_preference = redirect_preference;
    redirect_rtparms.rtp_state = RTS_GATEWAY|RTS_NOADVISE|RTS_NETROUTE;

    tracef("REDIRECT: redirect from %A: %A/%A via %A",
	   src,
	   redirect_rtparms.rtp_dest,
	   redirect_rtparms.rtp_dest_mask,
	   redirect_rtparms.rtp_router);

    /* check gateway directly reachable */
    if (!if_withdst(redirect_rtparms.rtp_router)) {
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 (": source gateway not on same net"));
	goto Return;
    }

    /* Check if from router in use */
    {
	rt_list *rtl = rthlist_match(redirect_rtparms.rtp_dest);
	rt_head *rth = (rt_head *) 0;

	RT_LIST(rth, rtl, rt_head) {

	    krtp = krt_kernel_rt(rth);
	    if (krtp) {
		break;
	    }
	} RT_LIST_END(rth, rtl, rt_head) ;

	RTLIST_RESET(rtl);

	/* Found the route in the kernel - is this one of the routers? */
	if (krtp) {
	    register int i = krtp->krtp_n_gw;

	    while (i--) {
		if (sockaddrcmp(src, krtp->krtp_routers[i])) {
		    break;
		}
	    }
	}

	if (!krtp) {
	    trace_tf(redirect_trace_options,
		     TR_NORMAL,
		     0,
		     (": not from router in use"));
	    goto Return;
	}
    }

    if (if_withaddr(redirect_rtparms.rtp_router, FALSE)) {
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 (": redirect to myself"));
	goto Return;
    }

    old_rt = rt_locate(redirect_rtparms.rtp_state,
		       redirect_rtparms.rtp_dest,
		       redirect_rtparms.rtp_dest_mask,
		       RTPROTO_BIT_ANY);
    if (old_rt
	&& (krtp
	    || (krtp = krt_kernel_rt(old_rt->rt_head)))
	&& !BIT_TEST(krtp->krtp_state, RTS_GATEWAY)) {
	/* Interface route */
	    
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 (": interface route"));
	goto Return;
    }

    /* Ignore if we are the source of this packet */
    if (if_withlcladdr(src, FALSE)) {
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 (": redirect from myself"));
	goto Return;
    }

    /* At this point we are sure that the route has been installed in the kernel */
#ifdef	KRT_RT_IOCTL
    krt_n_routes++;		/* Count another route in kernel */
#endif	/* KRT_RT_IOCTL */

    if (old_rt && !change_to_host) {
	/* Make note of the current kernel state */
	
	krt_rth_reset(old_rt->rt_head,
		      redirect_rtparms.rtp_state,
		      redirect_rtparms.rtp_n_gw,
		      redirect_rtparms.rtp_routers,
		      (if_addr **) 0);
    }
    
    if (redirect_ignore) {
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 (": redirects not allowed"));
	goto Return;
    }

#ifdef	PROTO_INET
    if (change_to_host) {
	/* Delete net redirects */

	rt = rt_locate(redirect_rtparms.rtp_state,
		       redirect_rtparms.rtp_dest,
		       mask,
		       RTPROTO_BIT_ANY);
	if (rt) {
	    /* Queue the changes to restore the kernel */

	    krt_rth_reset(rt->rt_head,
			  redirect_rtparms.rtp_state,
			  redirect_rtparms.rtp_n_gw,
			  redirect_rtparms.rtp_routers,
			  (if_addr **) 0);
	} else {
	    /* No route - just delete the redirect */
	    
	    krt_delete_dst(krt_task,
			   redirect_rtparms.rtp_dest,
			   mask,
			   RTPROTO_REDIRECT,
			   redirect_rtparms.rtp_state,
			   redirect_rtparms.rtp_n_gw,
			   redirect_rtparms.rtp_routers,
			   (if_addr **) 0);
	}
    }
#endif	/* PROTO_INET */

    /* Determine if this is an interior route */
    if (if_withsubnet(redirect_rtparms.rtp_dest)) {
	interior++;
    }
    /* Locate the gateway entry */
    redirect_rtparms.rtp_gwp = gw_timestamp(&redirect_gw_list,
					    RTPROTO_REDIRECT,
					    redirect_task,
					    (as_t) 0,
					    (as_t) 0,
					    src,
					    (flag_t) 0);

    /* If we have a list of trusted gateways, verify that this gateway is on it */
    if (redirect_n_trusted && !BIT_TEST(redirect_rtparms.rtp_gwp->gw_flags, GWF_TRUSTED)) {
	BIT_SET(redirect_rtparms.rtp_gwp->gw_flags, GWF_REJECT);
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 (": not on trustedgateways list"));
	goto Delete;
    }

    /* Locate an interface */
    ifap = if_withdst(redirect_rtparms.rtp_router);
    if (!ifap) {
	trace_log_tf(redirect_trace_options,
		     0,
		     LOG_ERR,
		     (": can not find interface for gateway"));
	goto Delete;
    }

    /* Check policy */
    if (!import(redirect_rtparms.rtp_dest,
		redirect_rtparms.rtp_dest_mask,
		redirect_import_list,
		ifap->ifa_ps[RTPROTO_REDIRECT].ips_import,
		redirect_rtparms.rtp_gwp ? redirect_rtparms.rtp_gwp->gw_import : (adv_entry *) 0,
		&redirect_rtparms.rtp_preference,
		ifap,
		(void_t) 0)) {
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 (": not valid"));
	BIT_SET(redirect_rtparms.rtp_gwp->gw_flags, GWF_IMPORT);
	goto Delete;
    }

    /* Invalidate any prior redirects to this destination/mask */
    rt = rt_locate(redirect_rtparms.rtp_state,
		   redirect_rtparms.rtp_dest,
		   redirect_rtparms.rtp_dest_mask,
		   RTPROTO_BIT(RTPROTO_REDIRECT));
    if (rt) {
	register rt_entry *rt1;

	RT_ALLRT(rt1, rt->rt_head) {
	    if (!BIT_TEST(rt1->rt_state, RTS_DELETE)) {
		rt_delete(rt1);
		break;
	    }
	} RT_ALLRT_END(rt1, rt->rt_head) ;
    }

    /* Reset the extra bit */
    BIT_RESET(redirect_rtparms.rtp_state, interior ? RTS_EXTERIOR : RTS_INTERIOR);

    trace_tf(redirect_trace_options,
	     TR_NORMAL,
	     0,
	     (NULL));

    /* Attempt to add the route */
    rt = rt_add(&redirect_rtparms);
    if (rt) {
	if (rt == rt->rt_active) {
	    if (!old_rt) {
		/* Make note of kernel state */
		
		krt_rth_reset(rt->rt_head,
			      redirect_rtparms.rtp_state,
			      redirect_rtparms.rtp_n_gw,
			      redirect_rtparms.rtp_routers,
			      (if_addr **) 0);
	    }
	    goto Return;
	} else {
	    rt_delete(rt);
	}
    } else {
	trace_tf(redirect_trace_options,
		 TR_NORMAL,
		 0,
		 ("redirect: error from rt_add"));
    }

  Delete:
    if (!old_rt && !change_to_host) {
	/*
	 *  Delete the entry from the kernel
	 */
	(void) krt_delete_dst(krt_task,
			      redirect_rtparms.rtp_dest,
			      redirect_rtparms.rtp_dest_mask,
			      RTPROTO_REDIRECT,
			      redirect_rtparms.rtp_state,
			      redirect_rtparms.rtp_n_gw,
			      redirect_rtparms.rtp_routers,
			      (if_addr **) 0);
    }

 Return:
    rt_close(redirect_task, redirect_rtparms.rtp_gwp, 0, NULL);

    return;
}


/* Initialize variables to their defaults */
void
redirect_var_init __PF0(void)
{
    redirect_preference = RTPREF_REDIRECT;
    redirect_ignore = (flag_t) 0;
}


void
redirect_delete_router __PF1(deletes, rt_list *)
{
    int changes = 0;
    gw_entry *gwp;
    task *tp = redirect_task;
    register sockaddr_un *router;

    rt_open(tp);

    RT_LIST(router, deletes, sockaddr_un) {
	GW_LIST(redirect_gw_list, gwp) {
	    rt_entry *rt = (rt_entry *) 0;

	    RTQ_LIST(&gwp->gw_rtq, rt) {
		if (sockaddrcmp(RT_ROUTER(rt), router)) {
		    rt_delete(rt);
		    changes++;
		}
	    } RTQ_LIST_END(&gwp->gw_rtq, rt) ;
	} GW_LIST_END(redirect_gw_list, gwp) ;
    } RT_LIST_END(router, deletes, sockaddr_un) ;

    rt_close(tp, (gw_entry *) 0, changes, NULL);
}


/* Delete all redirects in the routing table */
static void
redirect_delete __PF1(tp, task *)
{
    int changes = 0;
    gw_entry *gwp;

    rt_open(tp);

    GW_LIST(redirect_gw_list, gwp) {
	rt_entry *rt = (rt_entry *) 0;

	RTQ_LIST(&gwp->gw_rtq, rt) {
	    rt_delete(rt);
	    changes++;
	} RTQ_LIST_END(&gwp->gw_rtq, rt) ;
    } GW_LIST_END(redirect_gw_list, gwp) ;

    rt_close(tp, (gw_entry *) 0, changes, NULL);
}


/* Get ready for protocol processing */
static void
redirect_reinit __PF1(tp, task *)
{
    trace_inherit_global(redirect_trace_options, redirect_trace_types, (flag_t) 0);
    trace_set(tp->task_trace, redirect_trace_options);

    if (!redirect_ignore_save && redirect_ignore) {
	/* Redirects were enabled but are now disabled */

	redirect_delete(tp);
    } else {
	/* Redirects are (still) enabled.  Re-evaluate preference */
	/* and make sure we don't have any we are not supposed to */

	int entries = 0;
	gw_entry *gwp;

	rt_open(tp);
    
	GW_LIST(redirect_gw_list, gwp) {
	    rt_entry *rt = (rt_entry *) 0;

	    RTQ_LIST(&gwp->gw_rtq, rt) {
		pref_t preference = redirect_preference;

		if (BIT_TEST(RT_IFAP(rt)->ifa_ps[RTPROTO_REDIRECT].ips_state, IFPS_NOIN) ||
		    !import(rt->rt_dest,
			    rt->rt_dest_mask,
			    redirect_import_list,
			    RT_IFAP(rt)->ifa_ps[RTPROTO_REDIRECT].ips_import,
			    rt->rt_gwp->gw_import,
			    &preference,
			    RT_IFAP(rt),
			    (void_t) 0)) {
		    /* Get rid of this route */
		    rt_delete(rt);
		} else {
		    if (rt->rt_preference != preference) {
			/* The preference has changed, change the route */
			(void) rt_change(rt,
					 rt->rt_metric,
					 rt->rt_metric2,
					 rt->rt_tag,
					 preference,
					 (pref_t) 0,
					 rt->rt_n_gw, rt->rt_routers);
		    }
		    entries++;
		}
	    } RTQ_LIST_END(&gwp->gw_rtq, rt) ;
	} GW_LIST_END(redirect_gw_list, gwp) ;

	rt_close(tp, (gw_entry *) 0, entries, NULL);
    }

    redirect_ignore_save = (flag_t) 0;
}


/* Cleanup before re-reading configuratino file */
static void
redirect_cleanup __PF1(tp, task *)
{
    /* Save current state */
    redirect_ignore_save = redirect_ignore;

    /* Free policy structures */
    adv_cleanup(RTPROTO_REDIRECT,
		&redirect_n_trusted,
		(int *) 0,
		redirect_gw_list,
		&redirect_int_policy,
		&redirect_import_list,
		(adv_entry **) 0);

    trace_freeup(tp->task_trace);
    trace_freeup(redirect_trace_options);
}


/* Gated shutting down, clean up and exit */
static void
redirect_terminate __PF1(tp, task *)
{
    if_addr *ifap;

    /* Reset interface state */
    IF_ADDR(ifap) {
	struct ifa_ps *ips = &ifap->ifa_ps[tp->task_rtproto];
    
	ips->ips_state = (flag_t) 0;
    } IF_ADDR_END(ifap) ;

    if (!redirect_ignore) {
	/* Delete any redirects in table */

	redirect_delete(tp);
    }

    redirect_cleanup(tp);
    
    
    task_delete(tp);
}


/* Called by protocols to disable redirects */
void
redirect_disable __PF1(proto, proto_t)
{
    if (!redirect_ignore) {
	/* Redirects were enabled */

	redirect_delete(redirect_task);
    }

    BIT_SET(redirect_ignore, RTPROTO_BIT(proto));
}


/* Called by protocols to reenabled redirects */
void
redirect_enable __PF1(proto, proto_t)
{
    BIT_RESET(redirect_ignore, RTPROTO_BIT(proto));
}


/*
 *	Deal with interface policy
 */
static void
redirect_control_reset __PF2(tp, task *,
			     ifap, if_addr *)
{
    struct ifa_ps *ips = &ifap->ifa_ps[tp->task_rtproto];
    
    BIT_RESET(ips->ips_state, IFPS_RESET);
}


static void
redirect_control_set __PF2(tp, task *,
			   ifap, if_addr *)
{
    struct ifa_ps *ips = &ifap->ifa_ps[tp->task_rtproto];
    config_entry **list = config_resolv_ifa(redirect_int_policy,
					    ifap,
					    REDIRECT_CONFIG_MAX);

    /* Init */
    redirect_control_reset(tp, ifap);

    if (list) {
	int type = IF_CONFIG_MAX;
	config_entry *cp;

	/* Fill in the parameters */
	while (--type) {
	    if ((cp = list[type])) {
		switch (type) {
		case REDIRECT_CONFIG_IN:
		    if (cp->config_data) {
			BIT_RESET(ips->ips_state, IFPS_NOIN);
		    } else {
			BIT_SET(ips->ips_state, IFPS_NOIN);
		    }
		    break;
		}
	    }
	}

	config_resolv_free(list, REDIRECT_CONFIG_MAX);
    }
}


/* Clean up when an interface changes */
static void
redirect_ifachange __PF2(tp, task *,
			 ifap, if_addr *)
{
    int changes = 0;
    gw_entry *gwp;
    
    rt_open(tp);
    
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
    case IFC_ADD:
	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    redirect_control_set(tp, ifap);
	}
	/* Don't believe the interface is up until we see packets from it */
	break;

    case IFC_DELETE:
	break;

    case IFC_DELETE|IFC_UPDOWN:
    Delete:
        {
	    GW_LIST(redirect_gw_list, gwp) {
		rt_entry *rt = (rt_entry *) 0;

		RTQ_LIST(&gwp->gw_rtq, rt) {
		    if (RT_IFAP(rt) == ifap) {
			/* Delete any redirects we learned via this interface */

			rt_delete(rt);
			changes++;
		    }
		} RTQ_LIST_END(&gwp->gw_rtq, rt) ;
	    } GW_LIST_END(redirect_gw_list, gwp) ;

	    redirect_control_reset(tp, ifap);
	}
	break;

    default:
	/* Something has changed */
	if (BIT_TEST(ifap->ifa_change, IFC_NETMASK)) {
	    /* The netmask has changed, remove any routes to gateways we can no longer talk to */

	    GW_LIST(redirect_gw_list, gwp) {
		rt_entry *rt = (rt_entry *) 0;

		RTQ_LIST(&gwp->gw_rtq, rt) {
		    if (RT_IFAP(rt) == ifap &&
			ifap != if_withdstaddr(RT_ROUTER(rt))) {
			/* Delete any redirect whose nexthop gateway */
			/* is no longer on this interface */

			rt_delete(rt);
			changes++;
		    }
		} RTQ_LIST_END(&gwp->gw_rtq, rt) ;
	    } GW_LIST_END(redirect_gw_list, gwp) ;

	}
	if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {

	    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
		/* Transition to up */

		redirect_control_set(tp, ifap);
	    } else {
		/* Transition to down */

		goto Delete;
	    }
	}

	/* BROADCAST - we don't care about */
	/* METRIC - we don't care about */
	/* ADDR - we don't care about */
	/* MTU - we don't care about */
	/* SEL - we don't care aboute */
	break;
    }

    rt_close(tp, (gw_entry *) 0, changes, NULL);
}


/*
 *  Age out REDIRECT routes
 */
static void
redirect_age __PF2(tip, task_timer *,
		   interval, time_t)
{
    time_t expire_to = time_sec - REDIRECT_T_EXPIRE;
    time_t nexttime = time_sec + 1;

    if (expire_to > 0) {
	gw_entry *gwp;

	rt_open(tip->task_timer_task);
    
	GW_LIST(redirect_gw_list, gwp) {
	    rt_entry *rt;

	    if (!gwp->gw_n_routes) {
		/* No routes for this gateway */

		if (!gwp->gw_import
		    && !gwp->gw_export
		    && !BIT_TEST(gwp->gw_flags, GWF_SOURCE|GWF_TRUSTED)) {
		    /* No routes, delete this gateway */

		    /* XXX */
		}
		continue;
	    }

	    /* Age any routes for this gateway */
	    RTQ_LIST(&gwp->gw_rtq, rt) {
		if (rt->rt_time <= expire_to) {
		    /* This route has expired */
		
		    rt_delete(rt);
		} else {
		    /* This is the next route to expire */
		    if (rt->rt_time < nexttime) {
			nexttime = rt->rt_time;
		    }
		    break;
		}
	    } RTQ_LIST_END(&gwp->gw_rtq, rt) ;
	} GW_LIST_END(redirect_gw_list, gwp) ;

	rt_close(tip->task_timer_task, (gw_entry *) 0, 0, NULL);
    }

    if (nexttime > time_sec) {
	/* No routes to expire */

	nexttime = time_sec;
    }

    task_timer_set(tip, (time_t) 0, nexttime + REDIRECT_T_EXPIRE - time_sec);
}


static void
redirect_int_dump __PF2(fd, FILE *,
		     list, config_entry *)
{
    register config_entry *cp;

    CONFIG_LIST(cp, list) {
	switch (cp->config_type) {
	case REDIRECT_CONFIG_IN:
	    (void) fprintf(fd, " %sredirects",
			   cp->config_data ? "" : "no");
	    break;

	default:
	    assert(FALSE);
	    break;
	}
    } CONFIG_LIST_END(cp, list) ;
}


static void
redirect_dump __PF2(tp, task *,
		    fp, FILE *)
{
    (void) fprintf(fp, "\tRedirects: %s",
		   redirect_ignore ? "off" : "on");
    (void) fprintf(fp, "\tPreference: %d",
		   redirect_preference);

    if (redirect_gw_list) {
	(void) fprintf(fp, "\tGateways providing redirects:\n");
	gw_dump(fp,
		"\t\t",
		redirect_gw_list,
		RTPROTO_REDIRECT);
	(void) fprintf(fp, "\n");
    }
    if (redirect_int_policy) {
	(void) fprintf(fp, "\tInterface policy:\n");
	control_interface_dump(fp, 2, redirect_int_policy, redirect_int_dump);
    }
    control_import_dump(fp, 1, RTPROTO_REDIRECT, redirect_import_list, redirect_gw_list);
    (void) fprintf(fp, "\n\n");
    
}


void
redirect_init __PF0(void)
{
    trace_inherit_global(redirect_trace_options, redirect_trace_types, (flag_t) 0);
    /* Allocate the routing table task */
    redirect_task = task_alloc("Redirect",
			       TASKPRI_REDIRECT,
			       redirect_trace_options);
    task_set_cleanup(redirect_task, redirect_cleanup);
    task_set_reinit(redirect_task, redirect_reinit);
    task_set_dump(redirect_task, redirect_dump);
    task_set_terminate(redirect_task, redirect_terminate);
    task_set_ifachange(redirect_task, redirect_ifachange);
    if (!task_create(redirect_task)) {
	task_quit(EINVAL);
    }

    (void) task_timer_create(redirect_task,
			     "Age",
			     (flag_t) 0,
			     (time_t) 0,
			     REDIRECT_T_EXPIRE,
			     redirect_age,
			     (void_t) 0);
    
}
