#ident	"@(#)rdisc.c	1.3"
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


#include "include.h"
#include "ip_icmp.h"
#include "icmp.h"
#include "rdisc.h"
#include "inet.h"

#define	RDISC_SERVER
#define	RDISC_CLIENT

/* Common data */
static task *rdisc_task = (task *) 0;

int doing_rdisc;
adv_entry *rdisc_interface_policy = (adv_entry *) 0; /* List of interface policy */

trace *rdisc_trace_options = { 0 };   /* Trace flags */
const bits rdisc_trace_types[] = {
  { 0, NULL }
};

#ifdef	IP_MULTICAST
#define	RDISC_IS_GROUP(group, addr)	(sockaddrcmp_in((group), inet_addr_limitedbroadcast) \
					 || sockaddrcmp_in((group), (addr)))
#else	/* IP_MULTICAST */
#define	RDISC_IS_GROUP(group, addr)	sockaddrcmp_in((group), inet_addr_limitedbroadcast)
#endif	/* IP_MULTICAST */

/**/

/* Server support */

#ifdef	RDISC_SERVER

#define	RDISC_NAME_LENGTH	(sizeof (struct in_addr) * 4)

typedef struct _rdisc_entry {
    struct _rdisc_entry *rde_forw, *rde_back;

    struct _rdisc_group *rde_group;		/* Up pointer to the group */
    if_addr *rde_ifa;			/* Interface we belong to (except on deletes) */
    struct in_addr rde_address;		/* Address to announce */
    metric_t rde_preference;		/* It's preference */
} rdisc_entry;


typedef struct _rdisc_response {
    struct _rdisc_response *rdr_forw, *rdr_back;

    struct _rdisc_group *rdr_group;		/* Group we belong to */
    sockaddr_un *rdr_dest;			/* Address to respond to */
    task_timer *rdr_timer;			/* Pointer to our timer */
#define	RDISC_TIMER_RESPONSE_NAME	"Response"
    char rdr_name[sizeof RDISC_TIMER_RESPONSE_NAME + RDISC_NAME_LENGTH];
} rdisc_response;


typedef struct _rdisc_group {
    struct _rdisc_group *rdg_forw, *rdg_back;
    
    if_link *rdg_link;			/* Physical interface we belong to */
    time_t rdg_adv_max;			/* Max advertisement interval */
    time_t rdg_adv_min;			/* Min advertisement interval */
    time_t rdg_lifetime;		/* Lifetime */
    sockaddr_un *rdg_group;		/* Group address (broadcast or multicast) */

    if_addr *rdg_primary;		/* Primary address */

    task_timer *rdg_timer;		/* Timer for this group */
    task_job *rdg_job;			/* Foreground job for deletion announcments */

    struct _qelement rdg_addresses;	/* Queue of active addresses */
    struct _qelement rdg_deletes;	/* Queue of pending deletions */
    struct _qelement rdg_responses;	/* Queue of pending responses */

    u_int rdg_init;			/* Counts down to zero for each broadcast during init state */
    char rdg_name[RDISC_NAME_LENGTH];
} rdisc_group;

#define	ifa_rd_entry		ifa_ps[RTPROTO_RDISC].ips_datas[0]


typedef struct _rdisc_link {
    struct _rdisc_link *rdl_forw, *rdl_back;

    if_link *rdl_link;		/* Physical interface we belong to */

    struct _qelement rdl_groups;	/* List of groups */
} rdisc_link;

#define	ifl_rd_link	ifl_ps[RTPROTO_RDISC]


#define	RD_LINKS_FIRST(rdlp)	(((rdlp) = (rdisc_link *) rdisc_links.q_forw) != (rdisc_link *) &rdisc_links)
#define	RD_NOLINKS()	(rdisc_links.q_forw == &rdisc_links)
#define	RD_LINKS(rdlp)	for ((rdlp) = (rdisc_link *) rdisc_links.q_forw; \
			     (rdlp) != (rdisc_link *) &rdisc_links; \
			     (rdlp) = (rdlp)->rdl_forw)
#define	RD_LINKS_END(rdlp)


#define	RDL_GROUPS_FIRST(rdlp, rdgp) \
	(((rdgp) = (rdisc_group *) (rdlp)->rdl_groups.q_forw) != (rdisc_group *) &(rdlp)->rdl_groups)
#define	RDL_NOGROUPS(rdlp)	((rdlp)->rdl_groups.q_forw == &(rdlp)->rdl_groups)
#define	RDL_GROUPS(rdlp, rdgp)	for ((rdgp) = (rdisc_group *) (rdlp)->rdl_groups.q_forw; \
				     (rdgp) != (rdisc_group *) &(rdlp)->rdl_groups; \
				     (rdgp) = (rdgp)->rdg_forw)
#define	RDL_GROUPS_END(rdlp, rdgp)


#define	RD_ENTRIES_FIRST(head, rdep)	(((rdep) = (rdisc_entry *) (head)->q_forw) != (rdisc_entry *) (head))
#define	RD_NOENTRIES(head)	((head)->q_forw == (head))
#define	RD_ENTRIES(head, rdep)	for ((rdep) = (rdisc_entry *) (head)->q_forw; \
				     (rdep) != (rdisc_entry *) (head); \
				     (rdep) = (rdep)->rde_forw)
#define	RD_ENTRIES_END(head, rdep)

				 
#define	RDG_RESPONSES_FIRST(rdgp, rdep) \
	(((rdrp) = (rdisc_response *) (rdgp)->rdg_responses.q_forw) != (rdisc_response *) &(rdgp)->rdg_responses)
#define	RDG_NORESPONSES(rdgp)	((rdgp)->rdg_responses.q_forw == &(rdgp)->rdg_responses)
#define	RDG_RESPONSES(rdgp, rdrp) for ((rdrp) = (rdisc_response *) (rdgp)->rdg_responses.q_forw; \
				       (rdrp) != (rdisc_response *) &(rdgp)->rdg_responses; \
				       (rdrp) = (rdrp)->rdr_forw)
#define	RDG_RESPONSES_END(rdgp, rdrp)

				 
static struct _qelement rdisc_links = { &rdisc_links, &rdisc_links };
static block_t rdisc_link_index;
static block_t rdisc_group_index;
static block_t rdisc_entry_index;
static block_t rdisc_response_index;

adv_entry *rdisc_server_address_policy = (adv_entry *) 0;	/* List of address policy */

/**/
/*
 *  Send an advertisement to a group
 */
static void
rdisc_server_send __PF5(rdgp, rdisc_group *,
			head, qelement,
			ifap, if_addr *,
			dest, sockaddr_un *,
			lifetime, time_t)
{
    struct icmp *icmp = (struct icmp *) task_send_buffer; 
    register struct id_rdisc *rdp = &icmp->icmp_rdisc;
    register rdisc_entry *rdep;

    /* If broadcast or multicast, figure out address */
    if (!dest) {
	dest = rdgp->rdg_group;
    }
    
    /* Init our fields in the packet */
    icmp->icmp_type = ICMP_ROUTERADVERT;
    icmp->icmp_code = 0;
    icmp->icmp_addrsiz = sizeof (struct id_rdisc) / sizeof (u_int32);
    icmp->icmp_lifetime = htons(lifetime);

    RD_ENTRIES(head, rdep) {
	if ((size_t) ((byte *) rdp - (byte *) icmp) > ifap->ifa_mtu) {
	    /* Packet is full */

	    /* Add count */
	    icmp->icmp_addrnum = rdp - &icmp->icmp_rdisc;
	    /* Send the packet */
	    icmp_send(icmp,
		      (size_t) ((byte *) rdp - (byte *) icmp),
		      dest,
		      ifap,
		      MSG_DONTROUTE);

	    /* Reset fill pointer */
	    rdp = &icmp->icmp_rdisc;
	}

	/* Add this address */
	rdp->ird_addr = rdep->rde_address;	/* struct copy */
	rdp->ird_pref = htonl(rdep->rde_preference);
	rdp++;
    } RD_ENTRIES_END(head, rdep) ;

    /* Add count */
    icmp->icmp_addrnum = rdp - &icmp->icmp_rdisc;
    /* Send the packet */
    icmp_send(icmp,
	      (size_t) ((byte *) rdp - (byte *) icmp),
	      dest,
	      ifap,
	      MSG_DONTROUTE);

}


/*
 *  When the timer fires, send a soliciation.
 *  New interval is calculated by rdisc_send_advert().
 */
static void
rdisc_server_timer __PF2(tip, task_timer *,
			 real_interval, time_t)
{
    time_t max, min, offset;
    rdisc_group *rdgp = (rdisc_group *) tip->task_timer_data;

    /* Send an advertisement */
    rdisc_server_send(rdgp,
		      &rdgp->rdg_addresses,
		      rdgp->rdg_primary,
		      (sockaddr_un *) 0,
		      rdgp->rdg_lifetime);

    /* Recalculate timer */
    if (rdgp->rdg_init
	&& --rdgp->rdg_init) {
	/* Still in init mode */

	min = MIN(RDISC_MAX_INITIAL_ADVERT_INTERVAL, rdgp->rdg_adv_min);
	max = MIN(RDISC_MAX_INITIAL_ADVERT_INTERVAL, rdgp->rdg_adv_max);
    } else {
	min = rdgp->rdg_adv_min;
	max = rdgp->rdg_adv_max;
    }

    offset = min + grand((u_int32) (max - min));
    task_timer_set(rdgp->rdg_timer,
		   (time_t) 0,
		   offset);
    trace_tp(rdgp->rdg_timer->task_timer_task,
	     TR_STATE,
	     0,
	     ("rdisc_server_timer: group %s timer set to %#T",
	      rdgp->rdg_name,
	      offset));
}


/*
 * Send a response to a query.  Usually to a unicast group,
 * but possibly a response to a broadcast request when configured
 * for multicast
 */
static void
rdisc_server_timer_response __PF2(tip, task_timer *,
				  interval, time_t)
{
    rdisc_response *rdrp = (rdisc_response *) tip->task_timer_data;
    rdisc_group *rdgp = rdrp->rdr_group;

    /* Send an advertisement */
    rdisc_server_send(rdgp,
		      &rdgp->rdg_addresses,
		      rdgp->rdg_primary,
		      rdrp->rdr_dest,
		      rdgp->rdg_lifetime);

    sockfree(rdrp->rdr_dest);
    REMQUE(rdrp);
    task_block_free(rdisc_response_index, (void_t) rdrp);
}


/*
 * Send an indication of a deletion and delete the interface from
 * the group.  Delete the group and link if necessary.
 */
static void
rdisc_server_delete __PF2(tp, task *,
			  rdgp, rdisc_group *)
{
    register rdisc_entry *rdep;
    register if_addr *ifap = rdgp->rdg_primary;

    /* Send a deletion message if we can find an interface */
    if (!ifap) {
	/* Attempt to find an ifap */

	RD_ENTRIES(&rdgp->rdg_deletes, rdep) {
	    if (rdep->rde_ifa
		&& BIT_TEST(rdep->rde_ifa->ifa_state, IFS_UP)) {
		/* Found one */
		
		ifap = rdep->rde_ifa;
		break;
	    }
	} RD_ENTRIES_END(&rdgp->rdg_deletes, rdep) ;
    }
    if (ifap) {
	rdisc_server_send(rdgp,
			  &rdgp->rdg_deletes,
			  ifap,
			  (sockaddr_un *) 0,
			  (time_t) 0);
    }


    /* Process deletions */
    while ((rdep = (rdisc_entry *) rdgp->rdg_deletes.q_forw) != (rdisc_entry *) &rdgp->rdg_deletes) {

	/* Remove from the list */
	REMQUE(rdep);

#ifdef	IP_MULTICAST
	if (sockaddrcmp_in(rdgp->rdg_group, inet_addr_allhosts)
	    && !rdgp->rdg_primary) {
	    /* Remove ourselves from the all routers group */

	    task_set_option(icmp_task, 
			    TASKOPTION_GROUP_DROP,
			    rdep->rde_ifa,
			    inet_addr_allrouters);
	}
#endif	/* IP_MULTICAST */

	/* Release the interface */
	if (rdep->rde_ifa) {
	    rdep->rde_ifa->ifa_rd_entry = (void_t) 0;
	    IFA_FREE(rdep->rde_ifa);
	}
	task_block_free(rdisc_entry_index, (void_t) rdep);
    }

    if (RD_NOENTRIES(&rdgp->rdg_addresses)) {
	rdisc_link *rdlp = rdgp->rdg_link->ifl_rd_link;
	rdisc_response *rdrp;
	    
	/* Delete group */

	REMQUE(rdgp);

	sockfree(rdgp->rdg_group);
	if (rdgp->rdg_timer) {
	    task_timer_delete(rdgp->rdg_timer);
	}

	/* Delete any queued responses */
	while (RDG_RESPONSES_FIRST(rdgp, rdrp)) {
	    REMQUE(rdrp);
	    task_timer_delete(rdrp->rdr_timer);
	    sockfree(rdrp->rdr_dest);
	    task_block_free(rdisc_response_index, (void_t) rdrp);
	}
	
	task_block_free(rdisc_group_index, (void_t) rdgp);
	rdgp = (rdisc_group *) 0;

	if (RDL_NOGROUPS(rdlp)) {
	    /* Delete interface */

	    REMQUE(rdlp);
	    rdlp->rdl_link->ifl_rd_link = (void_t) 0;
	    task_block_free(rdisc_link_index, (void_t) rdlp);
	}
    }
}


static void
rdisc_server_delete_job __PF1(jp, task_job *)
{
    register rdisc_group *rdgp = (rdisc_group *) jp->task_job_data;

    rdgp->rdg_job = (task_job *) 0;
    rdisc_server_delete(jp->task_job_task, rdgp);
}


/**/

static int
rdisc_server_group_policy __PF2(tp, task *,
				rdgp, register rdisc_group *)
{
    time_t max, min, lifetime;
    config_entry **ifl_list = config_resolv_ifl(rdisc_interface_policy,
						rdgp->rdg_link,
						RDISC_CONFIG_MAX);

    max = RDISC_MAX_ADINTERVAL_DEFAULT;
    min = (time_t) 0;
    lifetime = (time_t) 0;
		
    if (ifl_list) {
	int type = RDISC_CONFIG_MAX;
	config_entry *cp;

	/* Fill in the parameters */
	while (--type) {
	    if ((cp = ifl_list[type])) {
		switch (type) {
		case RDISC_CONFIG_MAXADVINT:
		    max = (time_t) GA2S(cp->config_data);
		    break;
	  
		case RDISC_CONFIG_MINADVINT:
		    min = (time_t) GA2S(cp->config_data);
		    break;
	  
		case RDISC_CONFIG_LIFETIME:
		    lifetime = (time_t) GA2S(cp->config_data);
		    break;
		}
	    }
	}

	config_resolv_free(ifl_list, RDISC_CONFIG_MAX);
    }

    if (!min) {
	/* Default is 3/4 * max */

	min = (max + (max << 1)) >> 2;
    } else if (min > max) {
	/* Out of range, use max valid value */
	
	min = max;
    }
    
    if (!lifetime) {
	/* Default is 3 * max */

	lifetime = (max + (max << 1));	/* 3 * max */
    } else if (lifetime < max) {
	/* Out of range, use min valid value */
	
	lifetime = max;
    }

    if (max != rdgp->rdg_adv_max
	|| min != rdgp->rdg_adv_min
	|| lifetime != rdgp->rdg_lifetime) {
	/* Something has changed */

	rdgp->rdg_adv_max = max;
	rdgp->rdg_adv_min = min;
	rdgp->rdg_lifetime = lifetime;

	trace_tp(tp,
		 TR_STATE, 
		 0, 
		 ("rdisc_server_group_policy: group %s MaxAdvInt %#T  MinAdvInt = %#T  Lifetime %#T",
		  rdgp->rdg_name,
		  rdgp->rdg_adv_max,
		  rdgp->rdg_adv_min,
		  rdgp->rdg_lifetime));

	return TRUE;
    }    

    return FALSE;
}


static void
rdisc_server_ifachange __PF2(tp, task *,
			     ifap, if_addr *)
{
    int add = 0;
    int change = 0;
    int delete = 0;
    int policy = 0;
    metric_t preference = RDISC_PREFERENCE_DEFAULT;
    sockaddr_un *group = (sockaddr_un *) 0;
    register rdisc_entry *rdep = ifap->ifa_rd_entry;
    register rdisc_group *rdgp = rdep ? rdep->rde_group : (rdisc_group *) 0;

    if (!BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT|IFS_BROADCAST)) {
	/* We only support interfaces where we can reach all members */

	return;
    }
  
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
    case IFC_ADD:
	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    policy++;
	}
	break;
    
    case IFC_DELETE:
	return;
    
    case IFC_DELETE|IFC_UPDOWN:
	if (!rdep) {
	    return;
	}
	delete = 2;
	break;
    
    default:
	/* Something has changed */
  
	if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
	    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
		/* Transition to UP */

		policy++;
	    } else {
		/* Transition to DOWN */
	
		if (!rdep) {
		    return;
		}
		delete = 2;
	    }
	    break;
	}
	/* METRIC, NETMASK, SEL - Don't care */
	/* MTU - will take effect on output */

	if (BIT_TEST(ifap->ifa_change, IFC_BROADCAST|IFC_ADDR)) {
	    /* These changes are handled by policy */

	    policy++;
	}
    }

    /* Recalculate parameters */
    if (policy) {
	int broadcast = 0;
	int multicast = 0;
	config_entry **ifa_list = config_resolv_ifa(rdisc_server_address_policy,
						    ifap,
						    RDISC_CONFIG_MAX);
	/* Run policy */

	if (ifa_list) {
	    int type = RDISC_IFA_CONFIG_MAX;
	    config_entry *cp;
    
	    /* Fill in the parameters */  
	    while (--type) {
		if ((cp = ifa_list[type])) {
		    switch (type) {
		    case RDISC_CONFIG_IFA_IGNORE:
			if (GA2S(cp->config_data)) {
			    delete = 2;
			    trace_tp(tp,
				     TR_STATE,
				     0, 
				     ("rdisc_ifa_change: Ignoring address %A on %s due to policy",
				      ifap->ifa_addr_local,
				      ifap->ifa_link->ifl_name));
			}
			break;

		    case RDISC_CONFIG_IFA_BROADCAST:
			if (GA2S(cp->config_data)) {
			    broadcast++;
			} else {
			    multicast++;
			}			
			break;
			
		    case RDISC_CONFIG_IFA_PREFERENCE:
			preference = (metric_t) GA2S(cp->config_data);
			trace_tp(tp,
				 TR_STATE, 
				 0, 
				 ("rdisc_ifa_change: Preference for address %A(%s) set to %d",
				  ifap->ifa_addr_local,
				  ifap->ifa_link->ifl_name,
				  preference));
			break;
		    }
		}

	    }
	    config_resolv_free(ifa_list, RDISC_IFA_CONFIG_MAX);
	}

	if (delete) {
	    if (!rdep) {
		/* Can't delete if we don't have one */
		delete = 0;
	    }
	} else {
	    /* This is interface is valid, check for only changes */
	    
#ifdef	IP_MULTICAST
	    if (!broadcast
		&& BIT_TEST(ifap->ifa_state, IFS_MULTICAST)
		&& (task_set_option(icmp_task,
				    TASKOPTION_GROUP_ADD,
				    ifap,
				    inet_addr_allrouters) == 0
		    || errno == EADDRINUSE)) {
		/* This inteface supports multicast */
		
		group = inet_addr_allhosts;
	    } else if (multicast) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING, 
			     ("rdisc_server_ifachange: Ignoring interface %A on %s - multicast not available",
			      ifap->ifa_addr,
			      ifap->ifa_link->ifl_name));
	    } else
#endif	/* IP_MULTICAST */
		if (BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT)) {
		    group = ifap->ifa_addr;
		} else {
		    group = ifap->ifa_addr_broadcast;
		}

	    /* Figure out what (if anything) changed */
	    if (!rdgp) {
		/* No existing group */
	    
		add++;
	    } else if (sock2ip(ifap->ifa_addr_local) != rdep->rde_address.s_addr) {
		/* We need to forget about the old entry */
		/* and create a new one */
		delete = 2;
		add++;
	    } else if (!sockaddrcmp_in(group, rdgp->rdg_group)) {
		/* Remove from the old group and add to the new one */
		delete = 1;
		add++;
	    } else if (preference != rdep->rde_preference) {
		/* Just send a change */
		rdep->rde_preference = preference;
		change++;
	    }
	}
    }

    if (delete) {
	/* Remove it */

	/* Remove from announce queue and add to the end of the delete queue */
	REMQUE(rdep);

	if (delete > 1) {
	    INSQUE(rdep, rdgp->rdg_deletes.q_back);

	    /* Save the interface pointer in case this interface comes back up */
	    if (add) {
		/* Except if we have been told not to */
	    
		IFA_FREE(ifap);
		rdep->rde_ifa = (if_addr *) 0;
	    }

	    /* forget about this entry */
	    rdep = (rdisc_entry *) 0;
	    ifap->ifa_rd_entry = (void_t) 0;
	}

	if (ifap == rdgp->rdg_primary) {
	    /* Choose a new primary address */
	    
	    /* Free primary address */
	    IFA_FREE(ifap);

	    /* See if there is a new one */
	    if (RD_NOENTRIES(&rdgp->rdg_addresses)) {
		rdgp->rdg_primary = (if_addr *) 0;
	    } else {
		IFA_ALLOC(rdgp->rdg_primary = ((rdisc_entry *) rdgp->rdg_addresses.q_forw)->rde_ifa);
	    }
	}

	/* If there are no entries for the group, delete the timer */
	if (rdgp->rdg_timer
	    && RD_NOENTRIES(&rdgp->rdg_addresses)) {
	    rdisc_response *rdrp;

	    task_timer_delete(rdgp->rdg_timer);
	    rdgp->rdg_timer = (task_timer *) 0;

	    /* Delete any queued responses */
	    while (RDG_RESPONSES_FIRST(rdgp, rdrp)) {
		REMQUE(rdrp);
		task_timer_delete(rdrp->rdr_timer);
		sockfree(rdrp->rdr_dest);
		task_block_free(rdisc_response_index, (void_t) rdrp);
	    }
	}

	/* Create a job to process the deletion if necesary */
	if (!rdgp->rdg_job) {
	    rdgp->rdg_job = task_job_create(tp,
					    TASK_JOB_FG,
					    rdgp->rdg_name,
					    rdisc_server_delete_job,
					    (void_t) rdgp);
	}

	rdgp = (rdisc_group *) 0;
    }

    if (add) {
	register rdisc_link *rdlp = ifap->ifa_link->ifl_rd_link;

	/* Find the right group */

	if (!rdlp) {
	    /* Create a link */

	    rdlp = (rdisc_link *) task_block_alloc(rdisc_link_index);
	    ifap->ifa_link->ifl_rd_link = rdlp;

	    rdlp->rdl_link = ifap->ifa_link;
	    rdlp->rdl_groups.q_forw = rdlp->rdl_groups.q_back = &rdlp->rdl_groups;

	    /* Add to our list of links */
	    INSQUE(rdlp, rdisc_links.q_back);
	}

	/* Search for group */
	RDL_GROUPS(rdlp, rdgp) {
	    if (sockaddrcmp_in(group, rdgp->rdg_group)) {
		register rdisc_entry *rdep2;

		/* Found it! */

		/* Search for an entry with this address on delete queue */
		RD_ENTRIES(&rdgp->rdg_deletes, rdep2) {
		    if (sock2ip(ifap->ifa_addr_local) == rdep2->rde_address.s_addr) {
			/* Remove and free this entry */
			
			REMQUE(rdep2);
			if (rdep2 != rdep) {
			    if (rdep2->rde_ifa) {
				IFA_FREE(rdep2->rde_ifa);
			    }
			    task_block_free(rdisc_entry_index, (void_t) rdep2);
			}
			
			/* If there are not more entries, delete teh job */
			if (RD_NOENTRIES(&rdgp->rdg_deletes)) {
			    task_job_delete(rdgp->rdg_job);
			    rdgp->rdg_job = (task_job *) 0;
			}
			break;
		    }
		} RD_ENTRIES_END(&rdgp->rdg_deletes, rdep2) ;

		goto found_group;
	    }
	} RDL_GROUPS_END(rdlp, rdgp) ;

	/* Create a group */
	rdgp = (rdisc_group *) task_block_alloc(rdisc_group_index);
	rdgp->rdg_group = sockdup(group);
	rdgp->rdg_link = ifap->ifa_link;
	if (RDISC_IS_GROUP(group, inet_addr_allhosts)) {
	    strcpy(rdgp->rdg_name, rdgp->rdg_link->ifl_name);
	} else
	    (void) sprintf(rdgp->rdg_name, "%A", rdgp->rdg_group);
	rdgp->rdg_addresses.q_forw = rdgp->rdg_addresses.q_back = &rdgp->rdg_addresses;
	rdgp->rdg_deletes.q_forw = rdgp->rdg_deletes.q_back = &rdgp->rdg_deletes;
	rdgp->rdg_responses.q_forw = rdgp->rdg_responses.q_back = &rdgp->rdg_responses;
	(void) rdisc_server_group_policy(tp, rdgp) ;

	/* Insert on the end of the list */
	INSQUE(rdgp, rdlp->rdl_groups.q_forw->q_back);
    found_group:;

	/* Insert on address list */
	/* XXX - Should this be done in some order? */
	/* XXX - What about duplicates on a P2P interface? */
	if (!rdep) {
	    rdep = (rdisc_entry *) task_block_alloc(rdisc_entry_index);
	    rdep->rde_address = sock2in(ifap->ifa_addr_local);	/* struct copy */
	}
	if (!rdep->rde_ifa) {
	    IFA_ALLOC(rdep->rde_ifa = ifap);
	}
	rdep->rde_preference = preference;
	rdep->rde_group = rdgp;
	INSQUE(rdep, rdgp->rdg_addresses.q_back);

	/* Link address to group and indicate it's new */
	ifap->ifa_rd_entry = rdep;

	/* If there is no primary address, we are it */
	if (!rdgp->rdg_primary) {
	    IFA_ALLOC(rdgp->rdg_primary = ifap);
	}
	
	/* Create or reset timer */
	if (rdgp->rdg_timer) {
	    /* Force a quick update, then go back to normal */

	    change++;
	} else {
	    /* Create a new timer */
	    
	    rdgp->rdg_init = RDISC_MAX_INITIAL_ADVERTISEMENTS;
	    rdgp->rdg_timer = task_timer_create(tp, 
						rdgp->rdg_name,
						(flag_t) 0,
						(time_t) 0,
						RDISC_MAX_RESPONSE_DELAY,
						rdisc_server_timer,
						(void_t) rdgp);
	}
    }

    if (change) {
	/* Just a change, force an update */
	
	rdgp->rdg_init = MAX(rdgp->rdg_init, 1);
	task_timer_set(rdgp->rdg_timer,
		       (time_t) 0,
		       RDISC_MAX_RESPONSE_DELAY);
    }
}


/*
 * rdisc_server_recv
 * called from icmp recv routine
 */
static void 
rdisc_server_recv __PF5(icmp_tp, task *,
			src, sockaddr_un *,
			dst, sockaddr_un *,
			icmp, struct icmp *,
			len, size_t)
{
    if_addr *ifap;
    rdisc_group *rdgp;
    rdisc_entry *rdep;
    task *tp = rdisc_task;

    /* first determine the interface on which the message was received */
    if (dst
	&& (sock2ip(dst) == sock2ip(inet_addr_limitedbroadcast)
#ifdef  IP_MULTICAST
	    || inet_class_of(dst) != INET_CLASSC_MULTICAST
#endif  /* IP_MULTICAST */
	    )) {
	/* Try to find interface by looking up the destination address */

	ifap = if_withlcladdr(dst, TRUE);
    } else {
	/* Try to find interface by looking up the source address */

	ifap = if_withdst(src);
    }

    if (!ifap) {
	trace_log_tp(tp,
		     0,
		     LOG_WARNING, 
		     ("rdisc_server_recv: Can not locate interface for Router Solicitation from %A to %A",
		      src,
		      dst));
	return;
    }

    if (sockaddrcmp_in(src, ifap->ifa_addr_local)) {
	/* Ignore our packets */

	return;
    }

#ifdef	notdef
    /* kernel does checksum */
    /* checksum is valid */
    if (inet_cksum(icmp, len)) {
	trace_log_tp(tp,
		     0,
		     LOG_WARNING,
		     ("rdisc_server_recv: Bad checksum for Router Solicitation from %A to %A",
		      src,
		      dst));
	return;
    }
#endif	/* notdef */
		 
    /* icmp code is 0 */
    if (icmp->icmp_code) {
	trace_log_tp(tp, 
		     0,
		     LOG_WARNING, 
		     ("rdisc_server_recv: ICMP code not zero (%u) for Router Solicitation from %A to %A",
		      icmp->icmp_code,
		      src,
		      dst));
	return;
    }

    /* Verify length */
    if (len < ICMP_MINLEN) {
	trace_log_tp(tp,
		     0,
		     LOG_WARNING,
		     ("rdisc_server_recv: Insufficient length (%u) for Router Solicitation from %A to %A",
		      len,
		      src,
		      dst));
	return;
    }    

    rdep = (rdisc_entry *) ifap->ifa_rd_entry;
    if (!rdep) {
	/* Not running router discovery on this interface */

	return;
    }
    rdgp = rdep->rde_group;

    if (!rdgp->rdg_primary) {
	/* Group is being deleted */

	return;
    }
    
#ifdef	IP_MULTICAST
    if (dst
	&& sockaddrcmp_in(rdgp->rdg_group, inet_addr_allhosts)
	&& sockaddrcmp_in(dst, BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT) ? ifap->ifa_addr : ifap->ifa_addr_broadcast)) {
	/* Hmm, he's supposed to send to the multicast address */

	trace_log_tp(tp,
		      0,
		      LOG_WARNING,
		      ("rdisc_server_recv: Expected multicast (%A) for Router Solicitation from %A to %A",
		       inet_addr_allrouters,
		       src,
		       dst));

	/* We should send a response to the broadcast address, but */
	/* that would require pruning the responses to those one the */
	/* same network.  Instead, we'll response directly to the */
	/* router if we can. */
	if (sock2ip(src)) {
	    goto Unicast;
	} else {
	    goto Respond;
	}
    }
#endif	/* IP_MULTICAST */
    
    if (sock2ip(src)) {
	rdisc_response *rdrp;

	if (rdgp->rdg_timer->task_timer_next_time - time_sec <= RDISC_MAX_RESPONSE_DELAY) {
	    /* We will be sending an advertisement within MAX_RESPONSE_DELAY */

	    return;
	}

#ifdef	IP_MULTICAST
    Unicast:;
#endif	/* IP_MULTICAST */

	/* First see if one is already queued */
	RDG_RESPONSES(rdgp, rdrp) {
	    if (sockaddrcmp_in(src, rdrp->rdr_dest)) {
		/* Already queued */
		goto Duplicate;
	    }
	} RDG_RESPONSES_END(rdgp, rdrp) ;

	/* Not queued, create one */
	rdrp = (rdisc_response *) task_block_alloc(rdisc_response_index);
	INSQUE(rdrp, rdgp->rdg_responses.q_back);
	rdrp->rdr_group = rdgp;
	rdrp->rdr_dest = sockdup(src);
	(void) sprintf(rdrp->rdr_name, "%s%A",
		       RDISC_TIMER_RESPONSE_NAME,
		       src);

	rdrp->rdr_timer = task_timer_create(tp, 
					    rdrp->rdr_name,
					    (flag_t) 0,
					    (time_t) 0,
					    RDISC_MAX_RESPONSE_DELAY,
					    rdisc_server_timer_response,
					    (void_t) rdrp);

    Duplicate:;
    } else {
	/* Reply after a short random interval */

#ifdef	IP_MULTICAST
    Respond:;
#endif	/* IP_MULTICAST */
	rdgp->rdg_init = MAX(rdgp->rdg_init, 1);
	task_timer_set(rdgp->rdg_timer,
		       (time_t) 0,
		       RDISC_MAX_RESPONSE_DELAY);
    }
}


/**/

static void
rdisc_server_interface_dump __PF2(fp, FILE *,
				  list, config_entry *)
{
    register config_entry *cp;

    CONFIG_LIST(cp, list) {
	switch (cp->config_type) {
	case RDISC_CONFIG_MAXADVINT:
	    (void) fprintf(fp, " MaxAdvInt %#T",
			   (time_t) GA2S(cp->config_data));
	    break;

	case RDISC_CONFIG_MINADVINT:
	    (void) fprintf(fp, " MinAdvInt %#T",
			   (time_t) GA2S(cp->config_data));
	    break;
	    
	case RDISC_CONFIG_LIFETIME:
	    (void) fprintf(fp, " Lifetime %#T",
			   (time_t) GA2S(cp->config_data));
	    break;
	    
	default:
	    assert(FALSE);
	    break;
	}
    } CONFIG_LIST_END(cp, list) ;
}


static void
rdisc_server_address_dump __PF2(fp, FILE *,
				list, config_entry *)
{
    register config_entry *cp;

    CONFIG_LIST(cp, list) {
	switch (cp->config_type) {
	case RDISC_CONFIG_IFA_IGNORE:
	    (void) fprintf(fp, (int) GA2S(cp->config_data) ? " Ignore" : " Advertise");
	    break;
	    
	case RDISC_CONFIG_IFA_BROADCAST:
	    (void) fprintf(fp, " Broadcast");
	    break;
	    
	case RDISC_CONFIG_IFA_PREFERENCE:
	    (void) fprintf(fp, " Preference %d",
			   (metric_t) GA2S(cp->config_data));
	    break;
	    
	default:
	    assert(FALSE);
	    break;
	}
    } CONFIG_LIST_END(cp, list) ;
}


static void
rdisc_server_dump __PF2(tp, task *,
			fp, FILE *)
{
    register rdisc_link *rdlp;
    register if_link *ifl = (if_link *) 0;

    (void) fprintf(fp, "\tInterfaces:\n");

    RD_LINKS(rdlp) {
	register rdisc_group *rdgp;

	if (!ifl
	    || ifl != rdlp->rdl_link) {
	    /* New interface */

	    ifl = rdlp->rdl_link;
	    
	    (void) fprintf(fp, "\t\tInterface %s:\n",
			   ifl->ifl_name);
	}

	RDL_GROUPS(rdlp, rdgp) {
	    register rdisc_entry *rdep;
	    
	    (void) fprintf(fp, "\t\t\tGroup %A:\n\t\t\t\tminadvint %#T maxadvint %#T lifetime %#T\n\n",
			   rdgp->rdg_group,
			   rdgp->rdg_adv_min,
			   rdgp->rdg_adv_max,
			   rdgp->rdg_lifetime);

	    RD_ENTRIES(&rdgp->rdg_addresses, rdep) {
		(void) fprintf(fp, "\t\t\t\tAddress %A:\tPreference: %d\n",
			       sockbuild_in(0, rdep->rde_address.s_addr),
			       (s_int32) rdep->rde_preference);
	    } RD_ENTRIES_END(&rdgp->rdg_addresses, rdep) ;

	    if (!RDG_NORESPONSES(rdgp)) {
		register rdisc_response *rdrp;
		
		(void) fprintf(fp, "\t\t\tResponses pending to:\n");

		RDG_RESPONSES(rdgp, rdrp) {
		    (void) fprintf(fp, "\t\t\t\t%A\n",
				   rdrp->rdr_dest);
		} RDG_RESPONSES_END(rdgp, rdrp) ;
	     }

	    if (!RD_NOENTRIES(&rdgp->rdg_deletes)) {

		(void) fprintf(fp, "\t\t\tDeletions pending for:\n");

		RD_ENTRIES(&rdgp->rdg_deletes, rdep) {
		    (void) fprintf(fp, "\t\t\t\t%-15A preference %d\n",
				   rdep->rde_address,
				   rdep->rde_preference);
		} RD_ENTRIES_END(&rdgp->rdg_deletes, rdep) ;
	    } 
	} RDL_GROUPS_END(rdlp, rdgp) ;
    } RD_LINKS_END(rdlp) ;

    /* Print policy */
    if (rdisc_interface_policy) {
	(void) fprintf(fp, "\n\tInterface policy:\n");
	control_interface_dump(fp, 2, rdisc_interface_policy, rdisc_server_interface_dump);
    }

    if (rdisc_server_address_policy) {
	(void) fprintf(fp, "\n\tAddress policy:\n");
	control_interface_dump(fp, 2, rdisc_server_address_policy, rdisc_server_address_dump);
    }
}


static void
rdisc_server_reinit __PF1(tp, task *)
{
    register rdisc_link *rdlp;
    
    trace_set(tp->task_trace, rdisc_trace_options);

    /* Re-evaluate group policy */
    RD_LINKS(rdlp) {
	register rdisc_group *rdgp;

	RDL_GROUPS(rdlp, rdgp) {
	    if (rdisc_server_group_policy(tp, rdgp)) {
		/* Schedule a quick update if something changed */
	
		rdgp->rdg_init = MAX(rdgp->rdg_init, 1);
		task_timer_set(rdgp->rdg_timer,
			       (time_t) 0,
			       RDISC_MAX_RESPONSE_DELAY);
	    }
	} RDL_GROUPS_END(rdlp, rdgp) ;
    } RD_LINKS_END(rdlp) ;
}


static void
rdisc_server_cleanup __PF1(tp, task *)
{
    adv_free_list(rdisc_server_address_policy);
    rdisc_server_address_policy = (adv_entry *) 0;
    adv_free_list(rdisc_interface_policy);
    rdisc_interface_policy = (adv_entry *) 0;

    trace_freeup(rdisc_trace_options);
    trace_freeup(tp->task_trace);
}

static void
rdisc_server_terminate __PF1(tp, task *)
{
    register rdisc_link *rdlp;
    register rdisc_group *rdgp;

    RD_LINKS(rdlp) {

	/* First clean up a bit */
	RDL_GROUPS(rdlp, rdgp) {
	    register rdisc_entry *rdep;
	    register rdisc_response *rdrp;

	    /* Delete the job */
	    if (rdgp->rdg_job) {
		task_job_delete(rdgp->rdg_job);
		rdgp->rdg_job = (task_job *) 0;
	    }

	    while (RD_ENTRIES_FIRST(&rdgp->rdg_addresses, rdep)) {
		REMQUE(rdep);
		INSQUE(rdep, rdgp->rdg_deletes.q_back);
	    }

	    /* Delete any queued responses */
	    while (RDG_RESPONSES_FIRST(rdgp, rdrp)) {
		REMQUE(rdrp);
		task_timer_delete(rdrp->rdr_timer);
		sockfree(rdrp->rdr_dest);
		task_block_free(rdisc_response_index, (void_t) rdrp);
	    }
	} RDL_GROUPS_END(rdlp, rdgp) ;
    } RD_LINKS_END(rdlp) ;

    /* Call rdisc_server_delete() to send a notification and delete each */
    /* group and link */
    while (RD_LINKS_FIRST(rdlp)
	   && RDL_GROUPS_FIRST(rdlp, rdgp)) {
	rdisc_server_delete(tp, rdgp);
    }
    
    rdisc_server_cleanup(tp);

#ifdef	IP_MULTICAST    
    krt_multicast_delete(inet_addr_allrouters);
#endif	/* IP_MULTICAST */

    task_delete(tp);
    rdisc_task = (task *) 0;
}
#endif	/* RDISC_SERVER */

/**/

/* Client support */

#ifdef	RDISC_CLIENT

/* For controlling who does solicitations */
typedef struct _rdisc_solicit {
    struct _rdisc_solicit *rds_forw, *rds_back;

    flag_t rds_flags;
#define	RDSF_QUIET	BIT(0x01)	/* Don't solicit */

    if_link *rds_link;		/* Pointer to physical interface */

    if_addr *rds_addrs;		/* List of interface addresses */

    sockaddr_un *rds_group;	/* Group to solicit */
    char rds_name[RDISC_NAME_LENGTH];

    task_timer *rds_timer;	/* Solicitation timer */
    u_int rds_solicits;		/* Number of solicitations remaining */
    u_int rds_n_valid;		/* Number of routes via this interface */
} rdisc_solicit;

#define	RD_SOLICITS_FIRST(rdsp)	(((rdsp) = (rdisc_solicit *) rdisc_solicits.q_forw) != (rdisc_solicit *) &rdisc_solicits)
#define	RD_NOSOLICITS()	(rdisc_solicits.q_forw == &rdisc_solicits)
#define	RD_SOLICITS(rdsp)	for ((rdsp) = (rdisc_solicit *) rdisc_solicits.q_forw; \
			     (rdsp) != (rdisc_solicit *) &rdisc_solicits; \
			     (rdsp) = (rdsp)->rds_forw)
#define	RD_SOLICITS_END(rdsp)


/* Pointer to solicitation entry */
#define	ifa_rd_solicit		ifa_ps[RTPROTO_RDISC].ips_datas[0]
#define	ifa_to_solicit(ifap)	((rdisc_solicit *) (ifap)->ifa_rd_solicit)
/* Pointer to next if_addr * for this solicitation entry */
#define	ifa_rd_next		ifa_ps[RTPROTO_RDISC].ips_datas[1]

/* Pointer to solicitation entry for mulitcast interfaces */
#define	ifl_rd_solicit		ifl_ps[RTPROTO_RDISC]
#define	ifl_to_solicit(iflp)	((rdisc_solicit *) (iflp)->ifl_rd_solicit)

/* A place to save the lifetime */
#define	rt_rd_lifetime		rt_data
#define	rt_to_lifetime(rt)	((time_t) GA2S(rt->rt_rd_lifetime))

/* Which interface we were learned from */
#define	gw_rd_ifap		gw_data
#define	gw_to_ifa(gwp)	((if_addr *) (gwp)->gw_rd_ifap)

pref_t rdisc_client_preference = RTPREF_RDISC;

static gw_entry *rdisc_client_gw_list = (gw_entry *) 0;
static block_t rdisc_solicit_index = (block_t) 0;
static struct _qelement rdisc_solicits = { &rdisc_solicits, &rdisc_solicits };
static task_timer *rdisc_client_timer_age = (task_timer *) 0;
static const bits rdisc_solicit_bits[] = {
    { RDSF_QUIET,	"Quiet" },
    { 0,	NULL }
} ;

/* Hack because rt_change() does not recalculate interfaces */
#define	RT_CHANGE(rt, preference) \
	do { \
	    sockaddr_un *Xrouter = sockdup(RT_ROUTER(rt)); \
	    (void) rt_change((rt), \
			     (rt)->rt_metric, \
			     (rt)->rt_metric2, \
			     (rt)->rt_tag, \
			     (rt)->rt_preference, \
			     (rt)->rt_preference2, \
			     0, (sockaddr_un **) 0); \
	    (void) rt_change((rt), \
			     (rt)->rt_metric, \
			     (rt)->rt_metric2, \
			     (rt)->rt_tag, \
			     (preference), \
			     (rt)->rt_preference2, \
			     1, &Xrouter); \
	    sockfree(Xrouter); \
	} while (0)

/**/

static void
rdisc_client_send __PF2(ifap, if_addr *,
			dest, sockaddr_un *)
{
    struct icmp *icmp = (struct icmp *) task_send_buffer; 

    /* Init our fields in the packet */
    icmp->icmp_type = ICMP_ROUTERSOLICIT;
    icmp->icmp_code = 0;
    /* zero the reserved field */
    icmp->icmp_void = 0;

    icmp_send(icmp,
	      (size_t) ICMP_MINLEN,
	      dest,
	      ifap,
	      MSG_DONTROUTE);

}


static void
rdisc_client_solicit __PF2(tip, task_timer *,
			   interval, time_t)
{
    rdisc_solicit *rdsp = (rdisc_solicit *) tip->task_timer_data;
    if_addr *ifap = rdsp->rds_addrs;

    assert(ifap && rdsp->rds_solicits);

    /* Send a solicitation */
    rdisc_client_send(ifap, rdsp->rds_group);

    if (--rdsp->rds_solicits) {
	/* Set next interval */

	task_timer_set(tip,
		       (time_t) 0,
		       RDISC_SOLICITATION_INTERVAL);
    } else {
	/* Reset timer */

	task_timer_delete(tip);
	rdsp->rds_timer = (task_timer *) 0;
    }
}


/**/


static void
rdisc_client_rt_dump __PF2(fp, FILE *,
			   rt, rt_entry *)
{
    (void) fprintf(fp, "\t\t\tLearned via interface %A  Lifetime %#T\n",
		   gw_to_ifa(rt->rt_gwp)->ifa_addr,
		   rt_to_lifetime(rt));
}


static void
rdisc_client_interface_dump __PF2(fp, FILE *,
				  list, config_entry *)
{
    register config_entry *cp;

    CONFIG_LIST(cp, list) {
	switch (cp->config_type) {
	case RDISC_CONFIG_CLIENT_DISABLE:
	    (void) fprintf(fp,
			   GA2S(cp->config_data) ? " disable" : " enable");
	    break;

	case RDISC_CONFIG_CLIENT_BROADCAST:
	    (void) fprintf(fp,
			   GA2S(cp->config_data) ? " broadcast" : " multicast");
	    break;

	case RDISC_CONFIG_CLIENT_QUIET:
	    (void) fprintf(fp,
			   GA2S(cp->config_data) ? " quiet" : " solicit");
	    break;
	    
	default:
	    assert(FALSE);
	    break;
	}
    } CONFIG_LIST_END(cp, list) ;
}


static void
rdisc_client_dump __PF2(tp, task *,
			fp, FILE *)
{
    register rdisc_solicit *rdsp;

    (void) fprintf(fp, "\tPreference: %u\n\n",
		   rdisc_client_preference);

    (void) fprintf(fp, "\tSolicitation Groups:\n");
    
    RD_SOLICITS(rdsp) {
	if_addr *ifap;
	
	(void) fprintf(fp, "\t\t%-15s  flags %s  address %-15A interface %s  valid routes %u  solicits %u\n",
		       rdsp->rds_name,
		       trace_bits(rdisc_solicit_bits, rdsp->rds_flags),
		       rdsp->rds_group,
		       rdsp->rds_link->ifl_name,
		       rdsp->rds_n_valid,
		       rdsp->rds_solicits);

	for (ifap = rdsp->rds_addrs;
	     ifap;
	     ifap = (if_addr *) ifap->ifa_rd_next) {

	    (void) fprintf(fp, "\t\t\t%A\n",
			   ifap->ifa_addr);
	}
    } RD_SOLICITS_END(rdsp) ;
    
    if (rdisc_client_gw_list) {
	(void) fprintf(fp, "\n\tActive gateways:\n");
	gw_dump(fp,
		"\t\t",
		rdisc_client_gw_list,
		tp->task_rtproto);
	(void) fprintf(fp, "\n");
    }

    if (rdisc_interface_policy) {
	(void) fprintf(fp, "\tInterface policy:\n");
	control_interface_dump(fp, 2, rdisc_interface_policy, rdisc_client_interface_dump);
    }
}

/**/

static void
rdisc_client_ifachange __PF2(tp, task *,
			     ifap, if_addr *)
{
    int policy = 0;
    int delete = 0;
    int solicit = 0;
    int quiet = 0;
    int add = 0;
    int broadcast = 0;
    int multicast = 0;
    int netmask = 0;
    int route_recalc = 0;
    rdisc_solicit *rdsp = (rdisc_solicit *) ifap->ifa_rd_solicit;
    sockaddr_un *group = (sockaddr_un *) 0;
    
    if (!BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT|IFS_BROADCAST)) {
	/* We only support interfaces where we can reach all members */

	return;
    }
  
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
    case IFC_ADD:
	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    policy++;
	}
	break;
    
    case IFC_DELETE:
	return;
    
    case IFC_DELETE|IFC_UPDOWN:
	delete++;
	break;
    
    default:
	/* Something has changed */
  
	if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
	    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
		/* Transition to UP */

		policy++;
	    } else {
		/* Transition to DOWN */

		delete++;
	    }
	    break;
	}
	/* METRIC, ADDR, SEL - Don't care */
	/* MTU - will take effect on output */

	if (BIT_TEST(ifap->ifa_change, IFC_NETMASK)) {
	    /* Need to re-evaluate gateways and routes so do it the hard way */

	    netmask++;
	}

	if (BIT_TEST(ifap->ifa_change, IFC_BROADCAST)) {
	    /* We need to join a new solicitation */

	    policy++;
	}
    }

    if (policy) {
	config_entry **ifl_list = config_resolv_ifl(rdisc_interface_policy,
						    ifap->ifa_link,
						    RDISC_CONFIG_MAX);

	if (ifl_list) {
	    int type = RDISC_CONFIG_CLIENT_MAX;
	    config_entry *cp;

	    /* Fill in the parameters */
	    while (--type) {
		if ((cp = ifl_list[type])) {
		    switch (type) {
		    case RDISC_CONFIG_CLIENT_DISABLE:
			if (GA2S(cp->config_data)) {
			    /* Ignore this interface */
			    
			    delete++;
			}
			break;
	  
		    case RDISC_CONFIG_CLIENT_BROADCAST:
			if (GA2S(cp->config_data)) {
			    broadcast++;
			} else {
			    multicast++;
			}
			break;
	  
		    case RDISC_CONFIG_CLIENT_QUIET:
			if (GA2S(cp->config_data)) {
			    quiet = 1;
			}
			break;
		    }
		}
	    }

	    config_resolv_free(ifl_list, RDISC_CONFIG_MAX);
	}
    }

    /* Now figure out what we need to do */
    if (delete) {
	if (!rdsp) {
	    delete = 0;
	}
    } else {
	/* This is interface is valid, check for only changes */
	    
#ifdef	IP_MULTICAST
	if (!broadcast
	    && BIT_TEST(ifap->ifa_state, IFS_MULTICAST)) {
	    /* This inteface supports multicast */
		
	    group = inet_addr_allrouters;
	} else if (multicast) {
	    trace_log_tp(tp,
			 0,
			 LOG_WARNING, 
			 ("rdisc_server_ifachange: Ignoring interface %A on %s - multicast not available",
			  ifap->ifa_addr,
			  ifap->ifa_link->ifl_name));
	} else
#endif	/* IP_MULTICAST */
	    if (BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT)) {
		group = ifap->ifa_addr;
	    } else {
		group = ifap->ifa_addr_broadcast;
	    }

	/* Figure out what (if anything) changed */
	if (!rdsp) {
	    /* No existing group */
	    
	    add++;
	} else if (!sockaddrcmp_in(group, rdsp->rds_group)) {
	    /* Remove from the old group and add to the new one */

	    if (RDISC_IS_GROUP(group, inet_addr_allrouters)
		== RDISC_IS_GROUP(rdsp->rds_group, inet_addr_allrouters)) {
		/* Simple case, handle it */

		tracef("rdisc_client_ifachange: CHANGE old group %s address %A",
		       rdsp->rds_name,
		       rdsp->rds_group);

		/* Change group address */
		sockfree(rdsp->rds_group);
		rdsp->rds_group = sockdup(group);

		/* Change name */
		if (RDISC_IS_GROUP(group, inet_addr_allrouters)) {
		    strcpy(rdsp->rds_name, ifap->ifa_link->ifl_name);
		} else {
		    sprintf(rdsp->rds_name, "%A",
			    rdsp->rds_group);
		}

		trace_tp(tp,
			 TR_STATE,
			 0,
			 (" new group %s address %A  interface %s",
			  rdsp->rds_name,
			  rdsp->rds_group,
			  rdsp->rds_link->ifl_name));
	    } else {
		/* This the hard case, do it the hard way */
		
		delete++;
		add++;
	    }
	} else if (BIT_MATCH(rdsp->rds_flags, RDSF_QUIET) != quiet) {
	    if (quiet) {
		/* Make us quiet */
		
		BIT_SET(rdsp->rds_flags, RDSF_QUIET);
		if (rdsp->rds_timer) {
		    task_timer_delete(rdsp->rds_timer);
		    rdsp->rds_timer = (task_timer *) 0;
		    rdsp->rds_solicits = 0;
		}
	    } else {
		/* Make us noisy */

		solicit++;
	    }
	}
    }

    /* Handle a delete */
    if (delete) {
	int freeit = 0;

	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("rdisc_client_ifachange: DELETE address %A interface %s group %s",
		  ifap->ifa_addr,
		  ifap->ifa_link->ifl_name,
		  rdsp->rds_name));

	if (RDISC_IS_GROUP(rdsp->rds_group, inet_addr_allrouters)) {
	    if_addr **ifapp;

	    /* Remove our interface from the group */
	    for (ifapp = &rdsp->rds_addrs;
		 *ifapp != ifap;
		 ifapp = (if_addr **) &(*ifapp)->ifa_rd_next) {
		assert(*ifapp);
	    }
	    *ifapp = ifap->ifa_rd_next;

	    if (!*ifapp) {
		/* Last interface in group */

		freeit++;
		if (ifap->ifa_link->ifl_rd_solicit == rdsp) {
		    ifap->ifa_link->ifl_rd_solicit = (void_t) 0;
		}
	    }
	} else {
	    freeit++;
	}
	
	if (freeit) {
	    gw_entry *gwp;
	    
	    trace_tp(tp,
		     TR_STATE,
		     0,
		     ("rdisc_client_ifachange: DELETE group %s address %A interface %s",
		      rdsp->rds_name,
		      rdsp->rds_group,
		      rdsp->rds_link->ifl_name));

	    REMQUE(rdsp);
	    sockfree(rdsp->rds_group);
	    if (rdsp->rds_timer) {
		task_timer_delete(rdsp->rds_timer);
	    }
	    task_block_free(rdisc_solicit_index, (void_t) rdsp);

	    if (add) {
		/* Cause a route recalculation to make sure */
		/* the counts are right */

		route_recalc++;
	    } else {
		rt_list *deletes = (rt_list *) 0;
		
		/* Delete any routes via this interface */

		rt_open(tp);
		
		GW_LIST(rdisc_client_gw_list, gwp) {
		    rt_entry *rt;

		    if (gw_to_ifa(gwp) == ifap) {
			/* Gateway via this interface - delete it */

			RTQ_LIST(&gwp->gw_rtq, rt) {
			    if (rt->rt_preference > 0) {
				ifa_to_solicit(gw_to_ifa(gwp))->rds_n_valid--;
			    }
			    RTLIST_ADD(deletes, sockdup(RT_ROUTER(rt)));
			    rt_delete(rt);
			} RTQ_LIST_END(&gwp->gw_rtq, rt) ;

			gwp->gw_data = (void_t) 0;
		    } else {
			/* Check routes */

			route_recalc++;
		    }
		} GW_LIST_END(rdisc_client_gw_list, gwp) ;

		rt_close(tp, (gw_entry *) 0, 0, NULL);

		if (deletes) {
		    sockaddr_un *router;
		    
		    redirect_delete_router(deletes);

		    RT_LIST(router,deletes, sockaddr_un) {
			sockfree(router);
		    } RT_LIST_END(router,deletes, sockaddr_un) ;

		    RTLIST_RESET(deletes);
		}
	    }
	}
	ifap->ifa_rd_solicit = ifap->ifa_rd_next = (void_t) 0;
	IFA_FREE(ifap);
    }

    /* Handle a new one */
    if (add) {
	if (RDISC_IS_GROUP(group, inet_addr_allrouters)) {
	    if ((rdsp = ifap->ifa_link->ifl_rd_solicit)
		&& sockaddrcmp(group, rdsp->rds_group)) {
		if_addr **ifapp;

		/* Insert at the end of the list */
		for (ifapp = &rdsp->rds_addrs;
		     *ifapp;
		     ifapp = (if_addr **) &(*ifapp)->ifa_rd_next) ;
		IFA_ALLOC(*ifapp = ifap);

		/* We'll need to recalculate routes */
		route_recalc++;
	    } else {
		/* Create a new one for this multicast group */

		rdsp = (rdisc_solicit *) task_block_alloc(rdisc_solicit_index);
		rdsp->rds_group = sockdup(group);
		strcpy(rdsp->rds_name, ifap->ifa_link->ifl_name);
		rdsp->rds_link = ifap->ifa_link;
		IFA_ALLOC(rdsp->rds_addrs = ifap);
		ifap->ifa_link->ifl_rd_solicit = rdsp;
		INSQUE(rdsp, rdisc_solicits.q_back);
		if (!quiet) {
		    solicit++;
		}
		if (delete) {
		    /* Cause a recalculation to make sure the counts */
		    /* are right */

		    route_recalc++;
		}
		
		trace_tp(tp,
			 TR_STATE,
			 0,
			 ("rdisc_client_ifachange: ADD group %s address %A interface %s",
			  rdsp->rds_name,
			  rdsp->rds_group,
			  rdsp->rds_link->ifl_name));
	    }
	} else {
	    if (!rdsp) {
		/* Create a new one */

		rdsp = (rdisc_solicit *) task_block_alloc(rdisc_solicit_index);
	    }

	    rdsp->rds_group = sockdup(group);
	    sprintf(rdsp->rds_name, "%A",
		    rdsp->rds_group);
	    rdsp->rds_link = ifap->ifa_link;
	    IFA_ALLOC(rdsp->rds_addrs = ifap);
	    INSQUE(rdsp, rdisc_solicits.q_back);
	    if (!quiet) {
		solicit++;
	    }

	    trace_tp(tp,
		     TR_STATE,
		     0,
		     ("rdisc_client_ifachange: ADD group %s address %A interface %s",
		      rdsp->rds_name,
		      rdsp->rds_group,
		      rdsp->rds_link->ifl_name));
	}
	ifap->ifa_rd_solicit = (void_t) rdsp;
	
	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("rdisc_client_ifachange: ADD address %A interface %s group %s",
		  ifap->ifa_addr,
		  ifap->ifa_link->ifl_name,
		  rdsp->rds_name));

    }

    if (netmask
	&& rdsp
	&& (!add && !delete)) {
	gw_entry *gwp;
	rt_list *deletes = (rt_list *) 0;
	
	/* Just the netmask changed, re-evaluate the routes */

	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("rdisc_client_ifachange: processing netmask change"));

	rt_open(tp);
	
	GW_LIST(rdisc_client_gw_list, gwp) {
	    if_addr *gw_ifap = if_withdst(gwp->gw_addr);
	    rt_entry *rt;

	    if (gw_ifap != gw_to_ifa(gwp)
		&& (gw_to_ifa(gwp) == ifap
		    || gw_ifap == ifap)) {
		/* We changed either to or from this interface */

		if (!gw_ifap
		    || (gw_ifap->ifa_link != gw_to_ifa(gwp)->ifa_link)) {
		    /* Gateway no longer reachable the same physical interface */

		    RTQ_LIST(&gwp->gw_rtq, rt) {
			if (rt->rt_preference > 0) {
			    ifa_to_solicit(gw_to_ifa(gwp))->rds_n_valid--;
			}
			/* Delete any redirects */
			RTLIST_ADD(deletes, sockdup(RT_ROUTER(rt)));
			rt_delete(rt);
		    } RTQ_LIST_END(&gwp->gw_rtq, rt) ;

		    gwp->gw_data = (void_t) 0;
		} else {
#ifdef	IP_MULTICAST
		    if (ifa_to_solicit(ifap) != ifa_to_solicit(gw_to_ifa(gwp)))
#endif	/* IP_MULTICAST */
		    {
			/* Fix counts */

			RTQ_LIST(&gwp->gw_rtq, rt) {
			    if (rt->rt_preference > 0) {
				ifa_to_solicit(gw_to_ifa(gwp))->rds_n_valid--;
				ifa_to_solicit(gw_ifap)->rds_n_valid++;
			    }
			} RTQ_LIST_END(&gwp->gw_rtq, rt) ;
		    }
		    gwp->gw_data = (void_t) gw_ifap;
		}

		route_recalc++;

	    } 
	} GW_LIST_END(rdisc_client_gw_list, gwp) ;

	rt_close(tp, (gw_entry *) 0, 0, NULL);

	if (deletes) {
	    sockaddr_un *router;
	    
	    redirect_delete_router(deletes);

	    RT_LIST(router,deletes, sockaddr_un) {
		sockfree(router);
	    } RT_LIST_END(router,deletes, sockaddr_un) ;

	    RTLIST_RESET(deletes);
	}

	/* Send a query */
	solicit++;
    }

    /* See if any routes need to be changed */
    if (route_recalc) {
	int changes = 0;
	register gw_entry *gwp;
	register rdisc_solicit *rdsp2;

	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("rdisc_client_ifachange: recalculating routes"));

	rt_open(tp);

	/* Reset the route count on all groups */
	RD_SOLICITS(rdsp2) {
	    rdsp2->rds_n_valid = 0;
	} RD_SOLICITS_END(rdsp2) ;
	
	GW_LIST(rdisc_client_gw_list, gwp) {
	    register rt_entry *rt;

	    RTQ_LIST(&gwp->gw_rtq, rt) {
		if_addr *router_ifap = if_withdst(RT_ROUTER(rt));
		pref_t preference = rdisc_client_preference;

		if (!router_ifap
		    || router_ifap->ifa_link != gw_to_ifa(gwp)->ifa_link
		    || rt->rt_metric2 == RDISC_PREFERENCE_INELIGIBLE) {
		    preference = -preference;
		}

		if (preference > 0) {
		    ifa_to_solicit(router_ifap)->rds_n_valid++;
		}
		
		if (preference != rt->rt_preference
		    || router_ifap != RT_IFAP(rt)) {
		    RT_CHANGE(rt, preference);
		    changes++;
		}
	    
	    } RTQ_LIST_END(&gwp->gw_rtq, rt) ;
	} GW_LIST_END(rdisc_client_gw_list, gwp) ;
	    
	rt_close(tp, (gw_entry *) 0, changes, NULL);
    }

    if (solicit
	&& !quiet
	&& (!rdsp
	    || !rdsp->rds_n_valid)) {
	utime_t offset;
	
	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("rdisc_client_ifachange: scheduling solicitation"));

	rdsp->rds_solicits = RDISC_MAX_SOLICITATIONS;
	offset.ut_sec = 0;
	offset.ut_usec = (time_t) grand((u_int32) 1000000);

	if (rdsp->rds_timer) {
	    /* Timer already exists */
	
	    task_timer_uset(rdsp->rds_timer,
			    &offset,
			    (utime_t *) 0,
			    (utime_t *) 0);
	} else {
	    /* Create timer */

	    rdsp->rds_timer = task_timer_ucreate(rdisc_task,
						 rdsp->rds_name,
						 (flag_t) 0,
						 (utime_t *) 0,
						 &offset,
						 (utime_t *) 0,
						 rdisc_client_solicit,
						 (void_t) rdsp);
	}
    }
}


static void
rdisc_client_age __PF2(tip, task_timer *,
		       interval, time_t)
{
    time_t nexttime = time_sec + RDISC_LIFETIME_MAX + 1;
    gw_entry *gwp;
    rt_list *deletes = (rt_list *) 0;

    rt_open(tip->task_timer_task);
    
    GW_LIST(rdisc_client_gw_list, gwp) {
	rt_entry *rt;
	if (!gwp->gw_n_routes) {
	    /* No routes, delete this gateway */

	    /* XXX */
	    continue;
	}

	/* Age any routes for this gateway */
	RTQ_LIST(&gwp->gw_rtq, rt) {
	    time_t lifetime = rt_to_lifetime(rt);
	    time_t expire_to = time_sec - lifetime;

	    if (time_sec + lifetime < nexttime) {
		nexttime = time_sec + lifetime;
	    }
	
	    if (expire_to <= 0) {
		/* We have not been up long enough */
		
		continue;
	    }

	    if (rt->rt_time <= expire_to) {
		if (rt->rt_preference > 0) {
		    ifa_to_solicit(gw_to_ifa(gwp))->rds_n_valid--;
		}
		RTLIST_ADD(deletes, sockdup(RT_ROUTER(rt)));
		rt_delete(rt);
	    } else {
		/* This is the next route to expire */
		if (rt->rt_time + lifetime < nexttime) {
		    nexttime = rt->rt_time + lifetime;
		}
		break;
	    }
	} RTQ_LIST_END(&gwp->gw_rtq, rt) ;
    } GW_LIST_END(rdisc_client_gw_list, gwp) ;

    rt_close(tip->task_timer_task, (gw_entry *) 0, 0, NULL);

    if (deletes) {
	sockaddr_un *router;
	
	redirect_delete_router(deletes);
	
	RT_LIST(router,deletes, sockaddr_un) {
	    sockfree(router);
	} RT_LIST_END(router,deletes, sockaddr_un) ;

	RTLIST_RESET(deletes);
    }

    if (nexttime > time_sec + RDISC_LIFETIME_MAX) {
	/* No routes to expire, let timer idle */

	nexttime = time_sec;
    }

    task_timer_set(tip, (time_t) 0, nexttime - time_sec);
}


static void
rdisc_client_recv __PF5(icmp_tp, task *,
			src, sockaddr_un *,
			dst, sockaddr_un *,
			icmp, struct icmp *,
			len, size_t)
{
    if_addr *ifap;
    task *tp = rdisc_task;
    rdisc_solicit *rdsp;

    /* first determine the interface on which the message was received */
    if (dst
	&& (sock2ip(dst) == sock2ip(inet_addr_limitedbroadcast)
#ifdef  IP_MULTICAST
	    || inet_class_of(dst) != INET_CLASSC_MULTICAST
#endif  /* IP_MULTICAST */
	    )) {
	/* Try to find interface by looking up the destination address */

	ifap = if_withlcladdr(dst, TRUE);
    } else {
	/* Try to find interface by looking up the source address */

	ifap = if_withdst(src);
    }

    if (!ifap) {
	register struct id_rdisc *rdp = &icmp->icmp_rdisc;
	register struct id_rdisc *lp
	    = (struct id_rdisc *) (void_t) ((byte *) rdp + (icmp->icmp_addrnum * icmp->icmp_addrsiz * sizeof (u_int32)));

	/* The originating IP address does not match on of our interfaces. */
	/* We scan the list of addresses provided looking for one that */
	/* does match one of our interfaces.  If more than one match, make */
	/* sure that they all specify the same physical interface.  If */
	/* more than one physical interface is involved, ignore the packet. */

	for (; rdp < lp;
	     rdp = (struct id_rdisc *) (void_t) ((byte *) rdp + (icmp->icmp_addrsiz * sizeof (u_int32)))) {
	    sockaddr_un *router = sockbuild_in(0, rdp->ird_addr.s_addr);
	    if_addr *router_ifap = if_withdst(router);

	    if (!router_ifap || !router_ifap->ifa_rd_solicit) {
		/* Not on an attached interface, ignore it for now */

		continue;
	    }

	    if (!ifap) {
		/* This is our best guess */

		ifap = router_ifap;
		continue;
	    }

	    if (router_ifap->ifa_link != ifap->ifa_link) {
		/* More than one physical interfaces apply */

		return;
	    }
	}

	if (!ifap) {
	    /* Unable to determine an interface */

	    return;
	}
    } else if (sockaddrcmp_in(src, ifap->ifa_addr_local)) {
	/* Ignore our packets */

	return;
    }

#ifdef	notdef
    /* kernel does checksum */
    /* checksum is valid */
    if (inet_cksum(icmp, len)) {
	/* Bad checksum */

	return;
    }
#endif	/* notdef */
		 
    /* icmp code is 0 */
    if (icmp->icmp_code) {

	return;
    }

    if (!icmp->icmp_addrnum
	|| icmp->icmp_addrsiz < 2) {
	/* bogus entries */

	return;
    }
    
    /* Verify length */
    if (len < ICMP_MINLEN + (icmp->icmp_addrnum * icmp->icmp_addrsiz * sizeof (u_int32))) {
	/* Insufficient length */

	return;
    }    

    rdsp = (rdisc_solicit *) ifap->ifa_rd_solicit;
    if (rdsp) {
	int changes = 0;
	gw_entry *gwp;
	rt_list *deletes = (rt_list *) 0;
	time_t lifetime = ntohs(icmp->icmp_lifetime);
	time_t min_lifetime = RDISC_LIFETIME_MAX;
	register struct id_rdisc *rdp = &icmp->icmp_rdisc;
	register struct id_rdisc *lp
	    = (struct id_rdisc *) (void_t) ((byte *) rdp + (icmp->icmp_addrnum * icmp->icmp_addrsiz * sizeof (u_int32)));

	rt_open(tp);
	
	gwp = gw_timestamp(&rdisc_client_gw_list,
			   RTPROTO_RDISC,
			   tp,
			   (as_t) 0,
			   (as_t) 0,
			   src,
			   (flag_t) 0);
	if (!gwp->gw_data) {
	    /* A virgin! */
	    
	    gwp->gw_rtd_dump = rdisc_client_rt_dump;
	    gwp->gw_data = (void_t) ifap;
	} else {
	    /* The ifa_change code should catch any changes that could cause */
	    /* the interface for this gateway to change */

	    assert(gw_to_ifa(gwp) == ifap);
	}

	
	for (; rdp < lp;
	     rdp = (struct id_rdisc *) (void_t) ((byte *) rdp + (icmp->icmp_addrsiz * sizeof (u_int32)))) {
	    sockaddr_un *router = sockbuild_in(0, rdp->ird_addr.s_addr);
	    metric_t rd_pref = htonl(rdp->ird_pref);
	    metric_t metric = ~(rd_pref + 0x80000000);
	    pref_t preference = rdisc_client_preference;
	    if_addr *router_ifap = if_withdst(router);
	    rt_entry *rt;

	    if (!router_ifap
		|| router_ifap->ifa_link != ifap->ifa_link
		|| rd_pref == RDISC_PREFERENCE_INELIGIBLE) {
		/* Ineligible or not on the same interface */

		preference = -preference;
	    } else if (rdsp->rds_timer) {
		/* Valid advertisement - stop sending solicitations */

		task_timer_delete(rdsp->rds_timer);
		rdsp->rds_timer = (void_t) 0;
		rdsp->rds_solicits = 0;
	    }

	    RTQ_LIST(&gwp->gw_rtq, rt) {
		if (sockaddrcmp_in(RT_ROUTER(rt), router)) {
		    if (lifetime) {

			/* Update route's lifetime */
			if (lifetime != rt_to_lifetime(rt)) {
			    rt->rt_rd_lifetime = GS2A(lifetime);
			    if (min_lifetime > lifetime) {
				min_lifetime = lifetime;
			    }
			}

			/* Update or refresh route */
			if (metric != rt->rt_metric
			    || preference != rt->rt_preference) {
			    /* Something changed */

			    /* Fix counts */
			    if (rt->rt_preference > 0 && preference <= 0) {
				rdsp->rds_n_valid--;
			    } else if (rt->rt_preference <= 0 && preference > 0) {
				rdsp->rds_n_valid++;
			    }

			    (void) rt_change(rt,
					     metric,
					     rd_pref,
					     rt->rt_tag,
					     preference,
					     (pref_t) 0,
					     1, &router);
			    changes++;
			}

			/* Update time stamp */
			rt_refresh(rt);
		    } else {
			/* It's history */

			if (rt->rt_preference > 0) {
			    rdsp->rds_n_valid--;
			}
			RTLIST_ADD(deletes, sockdup(RT_ROUTER(rt)));
			rt_delete(rt);
			changes++;
		    }
		    goto found;
		}
	    } RTQ_LIST_END(&gwp->gw_rtq, rt) ;

	    if (lifetime) {
		rt_parms rtparms;
		
		/* Create a new route */

		bzero((caddr_t) &rtparms, sizeof rtparms);
		
		rtparms.rtp_dest = inet_addr_default;
		rtparms.rtp_dest_mask = inet_mask_default;
		rtparms.rtp_n_gw = 1;
		rtparms.rtp_router = router;
		rtparms.rtp_gwp = gwp;
		rtparms.rtp_metric = metric;
		rtparms.rtp_metric2 = rd_pref;
		rtparms.rtp_state = RTS_INTERIOR|RTS_GATEWAY|RTS_NOADVISE;
		if (preference < 0) {
		    BIT_SET(rtparms.rtp_state, RTS_NOTINSTALL);
		}
		rtparms.rtp_preference = preference;
		rtparms.rtp_rtd = (void_t) ifap;
		rt = rt_add(&rtparms);
		rt->rt_rd_lifetime = GS2A(lifetime);

		if (rt->rt_preference > 0) {
		    /* Count this route */
		    rdsp->rds_n_valid++;
		}
		changes++;
		if (min_lifetime > lifetime) {
		    min_lifetime = lifetime;
		}
	    }
	    
	found:;
	}

	rt_close(tp, gwp, changes, NULL);

	if (deletes) {
	    sockaddr_un *router;
	    
	    redirect_delete_router(deletes);

	    RT_LIST(router,deletes, sockaddr_un) {
		sockfree(router);
	    } RT_LIST_END(router,deletes, sockaddr_un) ;

	    RTLIST_RESET(deletes);
	}

	/* Make sure timer will fire in time to age these routes */
	if (!rdisc_client_timer_age->task_timer_next_time
	    || rdisc_client_timer_age->task_timer_next_time - time_sec > min_lifetime) {

	    task_timer_set(rdisc_client_timer_age,
			   (time_t) 0,
			   min_lifetime);
	}

    }
}


/**/

static void
rdisc_client_cleanup __PF1(tp, task *)
{
    adv_free_list(rdisc_interface_policy);
    rdisc_interface_policy = (adv_entry *) 0;

    trace_freeup(rdisc_trace_options);
    trace_freeup(tp->task_trace);
}


static void
rdisc_client_reinit __PF1(tp, task *)
{
    int changes = 0;
    register gw_entry *gwp;

    trace_set(tp->task_trace, rdisc_trace_options);

    rt_open(tp);
    
    GW_LIST(rdisc_client_gw_list, gwp) {
	register rt_entry *rt;

	RTQ_LIST(&gwp->gw_rtq, rt) {
	    if_addr *ifap = if_withdst(RT_ROUTER(rt));
	    pref_t preference = rdisc_client_preference;

	    if (!ifap
		|| ifap->ifa_link != gw_to_ifa(gwp)->ifa_link
		|| rt->rt_metric2 == RDISC_PREFERENCE_INELIGIBLE) {
		preference = -preference;
	    }

	    if (preference != rt->rt_preference) {
		(void) rt_change(rt,
				 rt->rt_metric,
				 rt->rt_metric2,
				 rt->rt_tag,
				 preference,
				 rt->rt_preference2,
				 1, &RT_ROUTER(rt));
		changes++;
	    }
	    
	} RTQ_LIST_END(&gwp->gw_rtq, rt) ;
    } GW_LIST_END(rdisc_client_gw_list, gwp) ;
	    
    rt_close(tp, (gw_entry *) 0, changes, NULL);
}


static void
rdisc_client_terminate __PF1(tp, task *)
{
    register gw_entry *gwp;
    register rdisc_solicit *rdsp;
    rt_list *deletes = (rt_list *) 0;

    rt_open(tp);
    
    GW_LIST(rdisc_client_gw_list, gwp) {
	register rt_entry *rt;

	RTQ_LIST(&gwp->gw_rtq, rt) {
	    if (rt->rt_preference > 0) {
		ifa_to_solicit(gw_to_ifa(gwp))->rds_n_valid--;
	    }
	    RTLIST_ADD(deletes, sockdup(RT_ROUTER(rt)));
	    rt_delete(rt);
	} RTQ_LIST_END(&gwp->gw_rtq, rt) ;
    } GW_LIST_END(rdisc_client_gw_list, gwp) ;

    rt_close(tp, (gw_entry *) 0, 0, NULL);

    if (deletes) {
	sockaddr_un *router;
	
	redirect_delete_router(deletes);

	RT_LIST(router,deletes, sockaddr_un) {
	    sockfree(router);
	} RT_LIST_END(router,deletes, sockaddr_un) ;

	RTLIST_RESET(deletes);
    }

    while (RD_SOLICITS_FIRST(rdsp)) {
	if_addr *ifap;


	REMQUE(rdsp);

	while ((ifap = rdsp->rds_addrs)) {

	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("rdisc_client_terminate: DELETE address %A interface %s group %s",
		  ifap->ifa_addr,
		  ifap->ifa_link->ifl_name,
		  rdsp->rds_name));

	    rdsp->rds_addrs = ifap->ifa_rd_next;
	    ifap->ifa_rd_solicit = ifap->ifa_rd_next = (void_t) 0;
	    IFA_FREE(ifap);
	}

	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("rdisc_client_terminate: DELETE group %s address %A interface %s",
		  rdsp->rds_name,
		  rdsp->rds_group,
		  rdsp->rds_link->ifl_name));

	if (RDISC_IS_GROUP(rdsp->rds_group, inet_addr_allrouters)) {
	    rdsp->rds_link->ifl_rd_solicit = (void_t) 0;
	}

	sockfree(rdsp->rds_group);
	if (rdsp->rds_timer) {
	    task_timer_delete(rdsp->rds_timer);
	}
	
	task_block_free(rdisc_solicit_index, (void_t) rdsp);
    }

    rdisc_client_cleanup(tp);

    task_delete(tp);
    rdisc_task = (task *) 0;
    rdisc_client_timer_age = (task_timer *) 0;
}
#endif	/* RDISC_CLIENT */

/**/

/*
 * router discovery initialization
 *
 * start timers for each advertising interface
 */
void
rdisc_init __PF0(void)
{
    if (doing_rdisc) {
	/* Set tracing */
	trace_inherit_global(rdisc_trace_options, rdisc_trace_types, (flag_t) 0);
    }
    
    switch (doing_rdisc) {
#ifdef	RDISC_SERVER
    case RDISC_DOING_SERVER:
#ifdef	RDISC_CLIENT
	if (rdisc_task
	    && GA2S(rdisc_task->task_data) != doing_rdisc) {
	    /* Terminate client */

	    rdisc_client_terminate(rdisc_task);
	    rdisc_task = (task *) 0;
	}
#endif	/* RDISC_CLIENT */
	if (!rdisc_task) {
	    /* Create us a task */
	    rdisc_task = task_alloc("RouterDiscoveryServer",
				    TASKPRI_ICMP,
				    rdisc_trace_options);
	    task_set_cleanup(rdisc_task, rdisc_server_cleanup);
	    task_set_reinit(rdisc_task, rdisc_server_reinit);
	    task_set_terminate(rdisc_task, rdisc_server_terminate);
	    task_set_ifachange(rdisc_task, rdisc_server_ifachange);
	    task_set_dump(rdisc_task, rdisc_server_dump);
	    rdisc_task->task_data = GS2A(doing_rdisc);
	    if (!task_create(rdisc_task)) {
		task_quit(EINVAL);
	    }

#ifdef	IP_MULTICAST
	    /* Tell kernel code about our address */
	    krt_multicast_add(inet_addr_allrouters);
#endif	/* IP_MULTICAST */

	    /* Allocate a block index */
	    if (!rdisc_group_index) {
		rdisc_link_index = task_block_init(sizeof (rdisc_link), "rdisc_link");
		rdisc_group_index = task_block_init(sizeof (rdisc_group), "rdisc_group");
		rdisc_entry_index = task_block_init(sizeof (rdisc_entry), "rdisc_entry");
		rdisc_response_index = task_block_init(sizeof (rdisc_response), "rdisc_response");
	    }
	}

	/* Tell ICMP we want router discovery solicitations */
	icmp_methods[ICMP_ROUTERSOLICIT] = rdisc_server_recv;
	break;
#endif	/* RDISC_SERVER */

#ifdef	RDISC_CLIENT
    case RDISC_DOING_CLIENT:
#ifdef	RDISC_SERVER
	if (rdisc_task
	    && GA2S(rdisc_task->task_data) != doing_rdisc) {
	    /* Terminate server */
	    
	    rdisc_server_terminate(rdisc_task);
	    rdisc_task = (task *) 0;
	}
#endif	/* RDISC_SERVER */
	if (!rdisc_task) {
	    /* Create us a task */

	    rdisc_task = task_alloc("RouterDiscoveryClient",
				    TASKPRI_ICMP,
				    rdisc_trace_options);
	    task_set_cleanup(rdisc_task, rdisc_client_cleanup);
	    task_set_reinit(rdisc_task, rdisc_client_reinit);
	    task_set_terminate(rdisc_task, rdisc_client_terminate);
	    task_set_ifachange(rdisc_task, rdisc_client_ifachange);
	    task_set_dump(rdisc_task, rdisc_client_dump);
	    rdisc_task->task_data = GS2A(doing_rdisc);
	    if (!task_create(rdisc_task)) {
		task_quit(EINVAL);
	    }

	    /* Create the age timer, leave it inactive for now */
	    rdisc_client_timer_age = task_timer_create(rdisc_task,
						       "Age",
						       (flag_t) 0,
						       (time_t) 0,
						       (time_t) 0,
						       rdisc_client_age,
						       (void_t) 0);

	    if (!rdisc_solicit_index) {
		rdisc_solicit_index = task_block_init(sizeof (rdisc_solicit), "rdisc_solicit");
	    }
	}
	    
	/* Tell ICMP we want router discovery advertisements */
	icmp_methods[ICMP_ROUTERADVERT] = rdisc_client_recv;
	break;
#endif	/* RDISC_CLIENT */

    case RDISC_DOING_OFF:
	if (rdisc_task) {
	    /* Cleanup */

	    switch (GA2S(rdisc_task->task_data)) {
#ifdef	RDISC_SERVER
	    case RDISC_DOING_SERVER:
		rdisc_server_terminate(rdisc_task);
		break;
#endif	/* RDISC_SERVER */

#ifdef	RDISC_CLIENT
	    case RDISC_DOING_CLIENT:
		rdisc_client_terminate(rdisc_task);
		break;
#endif	/* RDISC_CLIENT */

	    default:
		assert(FALSE);
	    }

	    rdisc_task = (task *) 0;
	}
	break;

    default:
	assert(FALSE);
    }
}


void
rdisc_var_init __PF0(void)
{
    doing_rdisc = RDISC_DOING_OFF;
#ifdef	RDISC_CLIENT
    rdisc_client_preference = RTPREF_RDISC;
#endif	/* RDISC_CLIENT */
}
