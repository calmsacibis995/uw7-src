#ident	"@(#)if.h	1.4"
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


/*
 * Interface data definitions.
 */

/*
 * Physical interface information
 */

extern const bits if_change_bits[];             /* Interface flag bits */
extern const bits if_proto_bits[];

struct _if_link {
	struct _if_link *ifl_forw;
	struct _if_link *ifl_back;
	flag_t          ifl_change;		/* What changed on this intf */
	u_int           ifl_transitions;	/* times gone up-down */
	int             ifl_refcount;		/* refcount */
	char            ifl_name[IFL_NAMELEN + 1];	/* name (duplicated and
							 * null terminated) */
	u_int           ifl_index;		/* Interface index */
	sockaddr_un    *ifl_addr;		/* Link level address */
	if_addr_entry  *ifl_addrent;		/* Physical address info */
	if_addr_entry  *ifl_nameent;		/* Pointer to name entry */
	if_addr_entry  *ifl_nameent_wild;	/* Pointer to name entry for
						 * wildcard */
	/* Hints used when scanning system interface list */
	flag_t          ifl_state;		/* flags */
	metric_t        ifl_metric;		/* metric */
	mtu_t           ifl_mtu;		/* mtu */
	/* For protocol use */
	void_t          ifl_ps[RTPROTO_MAX];
};

extern if_link if_plist;

/*
 * Interface information learned from the kernel or config file.
 */
struct _if_info {
	struct _if_info *ifi_forw;
	struct _if_info *ifi_back;
	flag_t          ifi_state;	/* Gated interface flags */
	flag_t          ifi_rtflags;	/* Kernel flags for new routes */
	metric_t        ifi_metric;	/* Configured metric */
	mtu_t           ifi_mtu;	/* Configured MTU */
	if_link        *ifi_link;	/* Pointer to physical information */
	sockaddr_un    *ifi_addr;	/* Interface address */
	sockaddr_un    *ifi_addr_local;	/* Local address of P2P link */
	sockaddr_un    *ifi_addr_broadcast;	/* Broadcast address if
						 * capable */
	sockaddr_un    *ifi_netmask;	/* Subnet mask for this interface */
};


/*
 * Interface flags.  Renamed to IFS_ to avoid conflicts with kernel interface
 * flags
 */
#define	IFS_UP		0x01	/* interface is up */
#define	IFS_BROADCAST	0x02	/* broadcast address valid */
#define	IFS_POINTOPOINT 0x04	/* interface is point-to-point link */
#define	IFS_LOOPBACK	0x08	/* This is a loopback interface */
#define	IFS_MULTICAST	0x10	/* Multicast possible on this interface */
#define	IFS_SIMPLEX	0x20	/* Can't hear my own packets */
#define IFS_ALLMULTI	0x40	/* Can hear all multicast packets */
#define	IFS_NOAGE	0x1000	/* don't time out/age this interface */
#define	IFS_DELETE	0x2000	/* Has been deleted */
#define IFS_OSPFVLINK   0x4000  /* OSPF virtual link */

/*
 * Structure interface stores information about a directly attached
 * interface, such as name, internet address, and bound sockets. The
 * interface structures are in a singly linked list pointed to by external
 * variable "if_list".
 */

struct _if_addr {
	if_info         ifa_info;	/* Address info from kernel */
#define	ifa_forw		ifa_info.ifi_forw
#define	ifa_back		ifa_info.ifi_back
#define	ifa_state		ifa_info.ifi_state
#define	ifa_metric		ifa_info.ifi_metric
#define	ifa_mtu			ifa_info.ifi_mtu
#define	ifa_addr		ifa_info.ifi_addr
#define	ifa_addr_local		ifa_info.ifi_addr_local
#define	ifa_addr_broadcast	ifa_info.ifi_addr_broadcast
#define	ifa_netmask		ifa_info.ifi_netmask
#define	ifa_link		ifa_info.ifi_link
	sockaddr_un    *ifa_net;/* network */
#ifdef	PROTO_ISO
	if_addr_entry  *ifa_systemid;	/* System ID on this interface */
#endif				/* PROTO_ISO */
	if_addr_entry  *ifa_addrent;	/* Pointer to remote address info */
	if_addr_entry  *ifa_addrent_local;	/* Pointer to local address
						 * info */
	flag_t          ifa_state_policy;	/* Interface flags set by
						 * policy (so they can be
						 * reset at reconfig) */
	u_int           ifa_refcount;	/* Saved references to this interface */
	flag_t          ifa_change;	/* What changed on this interface */
	pref_t          ifa_preference;	/* Preference for this interface */
	pref_t          ifa_preference_down;	/* Preference when it is down */
	flag_t          ifa_rtactive;	/* Mask of routing protocols *active*
					 * on this interface */
	rt_entry       *ifa_rt;	/* Pointer to route for this interface */
#ifdef	PROTO_ASPATHS
	as_t            ifa_as;	/* AS for this interface */
#endif				/* PROTO_ASPATHS */
	struct ifa_ps {
		flag_t          ips_state;	/* Flags for this proto */
		metric_t        ips_metric_in;	/* Metric for this proto */
#define	ips_metric	ips_metric_in
		metric_t        ips_metric_out;	/* Metric for this proto */
		adv_entry      *ips_import;	/* Import list for this proto */
		adv_entry      *ips_export;	/* Export list for this proto */
		void_t          ips_datas[6];	/* Protocol specific info */
#define	ips_data	ips_datas[0]
	}               ifa_ps[RTPROTO_MAX];
};


/* Protocol specific flags */
#define	IFPS_METRICIN	0x01	/* Inbound metric was specific */
#define	IFPS_METRICOUT	0x02	/* Outbound metric was specific */
#define	IFPS_NOIN	0x04	/* Ignore inbound packets */
#define	IFPS_NOOUT	0x08	/* Don't send packets */

/* User flags, kept through policy */
#define	IFPS_KEEP1	0x100000
#define	IFPS_KEEP2	0x200000
#define	IFPS_KEEP3	0x400000
#define	IFPS_KEEP4	0x800000

/* User flags, reset by policy */
#define	IFPS_POLICY1	0x10000000
#define	IFPS_POLICY2	0x20000000
#define	IFPS_POLICY3	0x40000000
#define	IFPS_POLICY4	0x80000000

#define	IFPS_RESET	(IFPS_METRICIN|IFPS_METRICOUT|IFPS_NOIN|IFPS_NOOUT|IFPS_POLICY1|IFPS_POLICY2|IFPS_POLICY3|IFPS_POLICY4)


/* Changes to an interface */
#define	IFC_NOCHANGE	0x00	/* No change */
#define	IFC_REFRESH	0x01	/* Still around */
#define	IFC_ADD		0x02	/* Interface with a new address */
#define	IFC_DELETE	0x04	/* Interface has been deleted */
#define	IFC_SCHANGE	(IFC_ADD|IFC_DELETE)

#define	IFC_UPDOWN	0x0100	/* UP <-> DOWN transition */
#define	IFC_NETMASK	0x0200	/* Netmask changed */
#define	IFC_METRIC	0x0400	/* Metric changed */
#define	IFC_BROADCAST	0x0800	/* Broadcast or Destaddr changed */
#define	IFC_MTU		0x1000	/* Interface MTU */
#define	IFC_ADDR	0x2000	/* Local or link level address changed */
#define	IFC_CCHANGE	(IFC_UPDOWN|IFC_NETMASK|IFC_METRIC|IFC_BROADCAST|IFC_MTU|IFC_ADDR)


/*
 * Local address structure.  This keeps track of all interfaces with a given
 * local address
 */
struct _if_addr_entry {
	struct _if_addr_entry *ifae_forw;
	struct _if_addr_entry *ifae_back;
	sockaddr_un    *ifae_addr;	/* The address */
	u_int           ifae_refcount;	/* Number of references to this entry */
	u_short         ifae_n_loop;	/* Number of loopback interfaces */
	u_short         ifae_n_p2p;	/* Number of point-to-point
					 * interfaces */
	u_short         ifae_n_if;	/* Number of total interfaces */
	rt_entry       *ifae_rt;/* Route to loopback for P2P w/o non-P2P */
};

#define	ifn_wildcard(ifae) \
    isdigit((ifae)->ifae_addr->s.gs_string[(ifae)->ifae_addr->s.gs_len - sizeof ((ifae)->ifae_addr->s) - 1])

#define	IFAE_ADDR_EXISTS(ifae) \
    ((ifae)->ifae_n_if)




/*
 * When we find any interfaces marked down we rescan the kernel every
 * CHECK_INTERVAL seconds to see if they've come up.
 */

#define	IF_T_CHECK	((time_t) 60)
#define	IF_T_TIMEOUT	((time_t) 180)

/* Used to scan the list of active interfaces */
#define	IF_ADDR(ifap) { for (ifap = (if_addr *) if_list.ifa_forw; ifap != &if_list; ifap = (if_addr *) ifap->ifa_forw)
#define	IF_ADDR_END(ifap) if (ifap == &if_list) ifap = (if_addr *) 0; }

/* Used to scan a list of interface info pointers */
#define	IF_INFO(ifi, list)	{ for (ifi = (list)->ifi_forw; ifi != (list); ifi = ifi->ifi_forw)
#define	IF_INFO_END(ifi, list)	if (ifi == (list)) ifi = (if_info *) 0; }

/* Used to scan a list of physical interfaces */
#define	IF_LINK(ifl)	{ for (ifl = if_plist.ifl_forw; ifl != &if_plist; ifl = ifl->ifl_forw)
#define	IF_LINK_END(ifl) if (ifl == &if_plist) ifl = (if_link *) 0; }

/* Used to scan a list of interface addresses */
#define	IF_ADDR_LIST(ifae, list)	{ for (ifae = (list)->ifae_forw; ifae != list; ifae = ifae->ifae_forw)
#define IF_ADDR_LIST_END(ifae, list)	if (ifae == list) ifae = (if_addr_entry *) 0; }

/* For parser */
#define	IF_LIMIT_METRIC	0, 16

#define	IF_CONFIG_PREFERENCE_UP		1
#define	IF_CONFIG_PREFERENCE_DOWN	2
#define	IF_CONFIG_PASSIVE		3
#define	IF_CONFIG_SIMPLEX		4
#define	IF_CONFIG_REJECT		5
#define	IF_CONFIG_BLACKHOLE		6
#define	IF_CONFIG_AS			7
#define	IF_CONFIG_MAX			8

typedef struct _if_count {
	u_int           all;
	u_int           up;
}               if_count;

extern if_info  if_config;	/* Interface addresses specified in the
				 * config file */
extern adv_entry *int_import[RTPROTO_MAX];	/* Import clauses for various
						 * protocols */
extern adv_entry *int_export[RTPROTO_MAX];	/* Export clauses for various
						 * protocols */
extern bits const *int_ps_bits[RTPROTO_MAX];	/* Bit defintions for
						 * protocols */
extern if_addr  if_list;	/* direct internet interface list */
extern if_addr_entry if_local_list;	/* List of local addresses */
extern if_addr_entry if_addr_list;	/* List of all addresses */
extern if_addr_entry if_name_list;	/* List of all names and wildcard
					 * names */
extern if_count if_n_link;	/* Number of physical interfaces */
extern if_count if_n_addr[AF_MAX];	/* Number of protocol addresses */
extern const bits if_state_bits[];	/* Interface flag bits */
extern adv_entry *int_policy;	/* Interface control info */
extern rt_parms int_rtparms;	/* Used to install interface routes learned
				 * from the routing socket */

PROTOTYPE(if_family_init,
	  extern void,
	  (void));
PROTOTYPE(if_init,
	  extern void,
	  (void));
PROTOTYPE(if_notify,
	  extern void,
	  (void));
#define	IFA_ALLOC(ifa)	{ if_addr *Xifa = (ifa); if (Xifa) { (Xifa)->ifa_refcount++; } }
#define	IFA_FREE(ifa)	((ifa) ? (--(ifa)->ifa_refcount ? (ifa) : ifa_free(ifa)) : (ifa))
PROTOTYPE(ifa_free,
	  extern if_addr *,
	  (if_addr *));
PROTOTYPE(ifi_withdst,
	  extern if_info *,
	  (sockaddr_un *,
	   if_info *));
PROTOTYPE(ifi_withsubnet,
	  extern if_info *,
	  (sockaddr_un *,
	   if_info *));
PROTOTYPE(ifi_withaddr,
	  extern if_info *,
	  (sockaddr_un *,
	   int,
	   if_info *));
PROTOTYPE(ifi_withdstaddr,
	  extern if_info *,
	  (sockaddr_un *,
	   if_info *));
PROTOTYPE(ifi_withlcladdr,
	  extern if_info *,
	  (sockaddr_un *,
	   int,
	   if_info *));
PROTOTYPE(if_myaddr,
	  extern int,
	  (if_addr *,
	   sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(if_withroute,
	  extern if_addr *,
	  (sockaddr_un *,
	   sockaddr_un *,
	   flag_t));
PROTOTYPE(if_rtupdate,
	  extern void,
	  (if_addr *));
PROTOTYPE(if_policy_match,
	  extern adv_entry *,
	  (if_addr *,
	   adv_entry *));
PROTOTYPE(if_conf_withdst,
	  int,
	  (sockaddr_un *));
PROTOTYPE(if_conf_open,
	  extern void,
	  (task *,
	   int));
PROTOTYPE(if_conf_close,
	  extern void,
	  (task *,
	   int));
PROTOTYPE(if_conf_addaddr,
	  extern void,
	  (task *,
	   if_info *));
PROTOTYPE(if_conf_deladdr,
	  extern void,
	  (task *,
	   if_info *));
PROTOTYPE(if_parse_add,
	  extern int,
	  (if_info *,
	   char *));
PROTOTYPE(if_parse_address,
	  extern adv_entry *,
	  (sockaddr_un *));
PROTOTYPE(if_parse_name,
	  extern adv_entry *,
	  (char *,
	   int));
#define	if_parse_withdst(a)	(ifi_withdstaddr(a, (if_info *) &if_list) || \
				 ifi_withdstaddr(a, &if_config) || \
				 ifi_withsubnet(a, (if_info *) &if_list) || \
				 ifi_withsubnet(a, &if_config))
PROTOTYPE(ifl_locate_index,
	  extern if_link *,
	  (u_int));
PROTOTYPE(ifl_locate_name,
	  extern if_link *,
	  (char *,
	   size_t));
PROTOTYPE(ifl_addup,
	  extern if_link *,
	  (task *,
	   if_link *,
	   u_int,
	   flag_t,
	   metric_t,
	   mtu_t,
	   char *,
	   size_t,
	   sockaddr_un *));
PROTOTYPE(ifae_free,
	  extern void,
	  (if_addr_entry *));
PROTOTYPE(ifae_alloc,
	  extern if_addr_entry *,
	  (if_addr_entry *));
PROTOTYPE(ifae_locate,
	  extern if_addr_entry *,
	  (sockaddr_un *,
	   if_addr_entry *));
PROTOTYPE(ifae_lookup,
	  extern if_addr_entry *,
	  (sockaddr_un *,
	   if_addr_entry *));

#define	if_withaddr(addr, broad_ok)	(if_addr *) ifi_withaddr(addr, broad_ok, (if_info *) &if_list)
#define	if_withlcladdr(addr, broad_ok)	(if_addr *) ifi_withlcladdr(addr, broad_ok, (if_info *) &if_list)
#define	if_withdstaddr(addr)	(if_addr *) ifi_withdstaddr(addr, (if_info *) &if_list)
#define	if_withsubnet(addr)	(if_addr *) ifi_withsubnet(addr, (if_info *) &if_list)
#define	if_withdst(addr)	(if_addr *) ifi_withdst(addr, (if_info *) &if_list)

#define	ifi_addr_free(ifi) \
    if ((ifi)->ifi_addr) { \
	sockfree((ifi)->ifi_addr); \
    } \
    if ((ifi)->ifi_addr_local \
	&& (ifi)->ifi_addr_local != (ifi)->ifi_addr) { \
	sockfree((ifi)->ifi_addr_local); \
    } \
    if ((ifi)->ifi_addr_broadcast) { \
	sockfree((ifi)->ifi_addr_broadcast); \
    } \
    (ifi)->ifi_addr = (ifi)->ifi_addr_local = (ifi)->ifi_addr_broadcast = (ifi)->ifi_netmask = (sockaddr_un *) 0
