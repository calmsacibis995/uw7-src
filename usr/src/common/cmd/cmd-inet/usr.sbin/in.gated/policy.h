#ident	"@(#)policy.h	1.3"
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
 *  destination mask internal stuff is for radix trie policy lists
 */
typedef struct _dest_mask_internal {
    struct _dest_mask_internal *dmi_left;	/* where to go when bit zero */
    struct _dest_mask_internal *dmi_right;	/* where to go when bit one */
    adv_entry *dmi_external;			/* external info for node */
    u_short dmi_bit;				/* bit num to test */
    u_char dmi_offset;				/* offset of byte to test */
    u_char dmi_mask;				/* mask to test bit in byte */
} dest_mask_internal;


#define	DMI_WALK(tree, dmi, external_only) \
	do { \
	    dest_mask_internal *Xstack[SOCK_MAXADDRLEN*NBBY+1]; \
	    dest_mask_internal **Xsp = Xstack; \
	    dest_mask_internal *Xnext; \
	    (dmi) = (tree); \
	    while ((dmi)) { \
		if ((dmi)->dmi_left) { \
		    Xnext = (dmi)->dmi_left; \
		    if ((dmi)->dmi_right) { \
			*Xsp++ = (dmi)->dmi_right; \
		    } \
		} else if ((dmi)->dmi_right) { \
		    Xnext = (dmi)->dmi_right; \
		} else if (Xsp != Xstack) { \
		    Xnext = *(--Xsp); \
		} else { \
		    Xnext = (dest_mask_internal *) 0; \
		} \
		if (!(external_only) || dmi->dmi_external) do

#define	DMI_WALK_END(tree, dmi, external_only) \
		while (0); \
		(dmi) = Xnext; \
	    } \
	} while (0)
		 
#define	DMI_WALK_ALL(tree, dmi, adv) \
	do { \
	      DMI_WALK(tree, dmi, TRUE) { \
		  register adv_entry *adv = dmi->dmi_external; \
		  do {
					 
#define	DMI_WALK_ALL_END(list, dmi, adv) \
		  } while ((adv = adv->adv_next)) ; \
	      } DMI_WALK_END(tree, dmi, TRUE) ; \
	 } while (0)

/*
 *	dest/mask list entry
 */
typedef struct _dest_mask {
    flag_t dm_flags;
    /* The following three must remain in this order */
#define	DMF_REFINE	BIT(0x01)	/* Mask must refine our mask */
#define	DMF_EXACT	BIT(0x02)	/* Mask must match exactly */
#define	DMF_ORDERMASK	(DMF_REFINE|DMF_EXACT)
#define DMF_NONCONTIG	BIT(0x04)	/* Mask is non-contiguous */
    sockaddr_un *dm_dest;
    sockaddr_un *dm_mask;
    dest_mask_internal *dm_internal;	/* Internal node on tree */
} dest_mask;


/**/

/* Route queues */

struct _rtq_entry {
    struct _rtq_entry *rtq_forw;
    struct _rtq_entry *rtq_back;
    time_t rtq_time;
};

#define	RT_OFFSET(member)	offsetof(rt_entry, member)

#define	RTQ_RT(rtqp)	((rt_entry *) ((void_t) ((byte *) (rtqp) - RT_OFFSET(rt_rtq))))

#define	RTQ_LIST(rtq, rt) \
	do { \
		register rtq_entry *Xrtq_next = (rtq)->rtq_forw; \
		while (Xrtq_next != (rtq)) { \
		    (rt) = RTQ_RT(Xrtq_next); \
		    Xrtq_next = Xrtq_next->rtq_forw;
#define	RTQ_LIST_END(rtq, rt)	} } while (0)

#define	RTQ_MOVE(old, new) \
	do { \
	    if ((old).rtq_forw != &(old)) { \
		((new).rtq_forw = (old).rtq_forw)->rtq_back = &(new); \
		((new).rtq_back = (old).rtq_back)->rtq_forw = &(new); \
	    } else { \
		(new).rtq_forw = (new).rtq_back = &(new); \
	    } \
	    (old).rtq_forw = (old).rtq_back = &(old); \
	} while (0)

/**/

/*
 *	Structure describing a gateway
 */
typedef struct _gw_entry {
    struct _gw_entry *gw_next;
    proto_t gw_proto;			/* Protocol of this gateway */
    sockaddr_un *gw_addr;		/* Address of this gateway */
    flag_t gw_flags;			/* Flags for this gateway */
    task *gw_task;			/* The task associated with this gateway */
    as_t gw_peer_as;			/* The AS of this gateway */
    as_t gw_local_as;			/* The AS advertised to this gateway */
    u_int gw_n_routes;			/* Number of routes */
    rtq_entry gw_rtq;			/* Queue of routes we own */
#define	gw_time	gw_rtq.rtq_time		/* Time this gateway was last heard from */
    void_t gw_data;			/* Protocol specific */
    struct _adv_entry *gw_import;	/* What to import from this gateway */
    struct _adv_entry *gw_export;	/* What to export to this gateway */
    _PROTOTYPE(gw_rtd_dump,
	       void,
	       (FILE *,
		rt_entry *));		/* Routine to format data */
    _PROTOTYPE(gw_rtd_free,
	       void,
	       (rt_entry *,
		void_t));		/* Routine to cleanup and free */
#if	defined(PROTO_SNMP) && defined(MIB_RIP)
    u_int	gw_bad_packets;		/* Bad packets received from this GW */
    u_int	gw_bad_routes;		/* Bad routes received from this GW */
    u_int	gw_last_version_received; /* rip version of last packet from this GW */
    struct timeval gw_last_update_time;	/* Last update received time for this GW */
#endif	/* PROTO_SNMP && MIB_RIP  */
} gw_entry;

#define	GWF_SOURCE	0x01		/* This is a source gateway */
#define	GWF_TRUSTED	0x02		/* This is a trusted gateway */
#define	GWF_ACCEPT	0x04		/* We accepted a packet from this gateway */
#define	GWF_REJECT	0x08		/* We rejected a packet from this gateway */
#define	GWF_QUERY	0x10		/* RIP query packet received */
#define	GWF_IMPORT	0x20		/* Rejected a network due to import restrictions */
#define	GWF_FORMAT	0x40		/* Packet format error */
#define	GWF_CHECKSUM	0x80		/* Bad checksum */
#define	GWF_AUXPROTO	0x100		/* This is an auxilary protocol */
#define	GWF_AUTHFAIL	0x200		/* Authentication failure */
#define	GWF_NEEDHOLD	0x400		/* This protocol requires holddowns */
#define	GWF_NOHOLD	0x800		/* This protocol (static) should not invoke holddowns */


/*
 *	Structure defining routines to use to process protocol specific data
 */
typedef struct _adv_psfunc {
    _PROTOTYPE(ps_rtmatch,
	       int,
	       (void_t,
		rt_entry *));		/* Routine to match data against route */
    _PROTOTYPE(ps_dstmatch,
	       int,
	       (void_t,
		sockaddr_un *,
		void_t));		/* Routine to match data against destination */
    _PROTOTYPE(ps_compare,
	       int,
	       (void_t,
		void_t));		/* Routine to compare two sets of data */
    _PROTOTYPE(ps_print,
	       char *,
	       (void_t,
		int));			/* Routine to display data */
    _PROTOTYPE(ps_free,
	       void,
	       (adv_entry *));		/* Routine to free data */
} adv_psfunc;

#define	PS_FUNC(adv, func)	control_psfunc[(adv)->adv_proto]->func
#define	PS_FUNC_VALID(adv, func)	(control_psfunc[(adv)->adv_proto] && PS_FUNC(adv, func))
    
extern adv_psfunc *control_psfunc[];


/* Description of results of a policy search */
typedef struct _adv_results {
    union {
	metric_t	resu_metric;
	void_t		resu_void;
    } res_u1;
#define	res_metric	res_u1.resu_metric
#define	res_void	res_u1.resu_void
    union {
	metric_t	resu_metric;
	pref_t		resu_preference;
    } res_u2;
#define	res_metric2	res_u2.resu_metric
#define	res_preference	res_u2.resu_preference
    flag_t	res_flag;
} adv_results;


/* Description of config file info */

typedef struct _config_entry {
    struct _config_entry *config_next;
    short config_type;
    short config_priority;
    void_t config_data;
} config_entry;

#define	CONFIG_LIST(cp, list)	for (cp = list; cp; cp = cp->config_next)
#define	CONFIG_LIST_END(cp, list)


typedef struct _config_list {
    int conflist_refcount;
    config_entry *conflist_list;
    _PROTOTYPE(conflist_free,
	       void,
	       (config_entry *));
} config_list;


#define	CONFIG_PRIO_ANY		1
#define	CONFIG_PRIO_WILD	2
#define	CONFIG_PRIO_NAME	3
#define	CONFIG_PRIO_ADDR	4
#define	CONFIG_PRIO_MAX		5


/*
 *	Structure used for all control lists.  Nested unions are used
 *	to minimize unused space.
 */
struct _adv_entry {
    struct _adv_entry *adv_next;	/* Pointer to next entry in list */
    int adv_refcount;			/* Number of references */
    flag_t adv_flag;			/* Flags */
    proto_t adv_proto;			/* Protocol for this match */

    union {
	adv_results	advru_result;		/* Result of the lookup */
#define	adv_result	adv_ru.advru_result
	config_list	*advru_config;		/* Config list */
#define	adv_config	adv_ru.advru_config
    } adv_ru;

    struct _adv_entry *adv_list;	/* List of functions to match */

    union adv_union {
	dest_mask advu_dm;
#define	adv_dm		adv_u.advu_dm
	gw_entry *advu_gwp;		/* Match a gateway address */
#define	adv_gwp		adv_u.advu_gwp
	if_addr *advu_ifap;		/* Match an interface */
#define	adv_ifap		adv_u.advu_ifap
	if_addr_entry *advu_ifn;	/* Match an interface name */
#define	adv_ifn		adv_u.advu_ifn
	if_addr_entry *advu_ifae;	/* Match on interface address */
#define	adv_ifae	adv_u.advu_ifae
	as_t advu_as;			/* Match an AS */
#define	adv_as		adv_u.advu_as
	tag_t advu_tag;			/* Match on tag */
#define	adv_tag		adv_u.advu_tag
#ifdef	PROTO_ASPATHS
	asmatch_t	*advu_aspath;		/* Match with AS path pattern */
#define	adv_aspath	adv_u.advu_aspath
#endif	/* PROTO_ASPATHS */
	void_t advu_ps;			/* Protocol specific data */
#define	adv_ps		adv_u.advu_ps
    } adv_u;
};

#define	ADVF_TYPE		BIT(0x0f)	/* Type to match */
#define	ADVFT_ANY		BIT(0x00)	/* No type specified */
#define	ADVFT_GW		BIT(0x01)	/* Match gateway address */
#define	ADVFT_IFN		BIT(0x02)	/* Match on interface name */
#define	ADVFT_IFAE		BIT(0x03)	/* Match on interface address */
#define	ADVFT_AS		BIT(0x04)	/* Match on AS */
#define	ADVFT_DM		BIT(0x05)	/* Match on dest/mask pair */
#define	ADVFT_ASPATH		BIT(0x06)	/* Match on AS path */
#define	ADVFT_TAG		BIT(0x07)	/* Match on tag */
#define	ADVFT_PS		BIT(0x08)	/* Match on protocol specific data */

#define	ADVFO_TYPE		BIT(0xf0)	/* Option type */
#define	ADVFOT_NONE		BIT(0x00)	/* No option specified */
#define	ADVFOT_METRIC		BIT(0x10)	/* Result Metric option */
#define	ADVFOT_PREFERENCE	BIT(0x20)	/* Result Preference option */
#define	ADVFOT_METRIC2		ADVFOT_PREFERENCE
#define	ADVFOT_FLAG		BIT(0x40)	/* Result Flag option */
#define	ADVFOT_CONFIG		BIT(0x80)	/* Config structure */

#define	ADVF_NO			BIT(0x1000)	/* Negative (i.e. noannounce, nolisten, nopropogate) */
#define	ADVF_FIRST		BIT(0x2000)	/* First entry in a sequence (of gateways or interfaces) */

#define	ADVF_USER2		BIT(0x40000000)
#define	ADVF_USER1		BIT(0x80000000)

#define	GW_LIST(list, gwp)	for (gwp = list; gwp; gwp = gwp->gw_next)
#define	GW_LIST_END(list, gwp)

#define	ADV_LIST(list, adv)	for (adv = list; adv; adv = adv->adv_next)
#define	ADV_LIST_END(list, adv)


extern unsigned int adv_n_allocated;

PROTOTYPE(control_import_dump,
	  extern void,
	  (FILE *,
	   int,
	   proto_t,
	   adv_entry *,
	   gw_entry *));
PROTOTYPE(control_export_dump,
	  extern void,
	  (FILE *,
	   int,
	   proto_t,
	   adv_entry *,
	   gw_entry *));
PROTOTYPE(control_entry_dump,
	  extern void,
	  (FILE *,
	   int,
	   adv_entry *));
PROTOTYPE(control_dmlist_dump,
	  extern void,
	  (FILE *,
	   int,
	   adv_entry *,
	   adv_entry *,
	   adv_entry *));
PROTOTYPE(control_interior_dump,
	  extern void,
	  (FILE *,
	   int,
	   _PROTOTYPE(func,
		      void,
		      (FILE *,
		       int,
		       proto_t,
		       adv_entry *,
		       gw_entry *)),
	   adv_entry * list));
PROTOTYPE(control_exterior_dump,
	  extern void,
	  (FILE *,
	   int,
	   _PROTOTYPE(func,
		      void,
		      (FILE *,
		       int,
		       proto_t,
		       adv_entry *,
		       gw_entry *)),
	   adv_entry * list));
PROTOTYPE(control_interface_dump,
	  extern void,
	  (FILE *,
	   int,
	   adv_entry *list,
	   _PROTOTYPE(func,
		      void,
		      (FILE *,
		       config_entry *))));
PROTOTYPE(control_interface_import_dump,
	  extern void,
	  (FILE *,
	   int,
	   adv_entry *));
PROTOTYPE(control_interface_export_dump,
	  extern void,
	  (FILE *,
	   int,
	   adv_entry *));
PROTOTYPE(control_exterior_locate,
	  extern adv_entry *,
	  (adv_entry * list,
	   as_t as));
PROTOTYPE(import,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *,
	   adv_entry *,
	   adv_entry *,
	   adv_entry *,
	   pref_t *,
	   if_addr *,
	   void_t));
PROTOTYPE(export,
	  extern int,
	  (struct _rt_entry *,
	   proto_t,
	   adv_entry *,
	   adv_entry *,
	   adv_entry *,
	   adv_results *));
PROTOTYPE(is_martian,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(martian_add,
	  extern void,
	  (sockaddr_un *,
	   sockaddr_un *,
	   flag_t,
	   flag_t));

PROTOTYPE(adv_alloc,
	  extern adv_entry *,
	  (flag_t,
	   proto_t));
PROTOTYPE(adv_free_list,
	  extern void,
	  (adv_entry * adv));
PROTOTYPE(adv_cleanup,
	  extern void,
	  (proto_t,
	   int *,
	   int *,
	   gw_entry *,
	   adv_entry **,
	   adv_entry **,
	   adv_entry **));
PROTOTYPE(adv_psfunc_add,
	  extern void,
	  (proto_t,
	   adv_psfunc *));
#ifdef	DMFEOL
PROTOTYPE(adv_destmask_finish,
	  extern adv_entry *,
	  (adv_entry *));
#else	/* DMF_EOL */
#define	adv_destmask_finish(x)	(x)
#endif	/* DMF_EOL */
PROTOTYPE(adv_destmask_depth,
	  extern void,
	  (adv_entry *));
PROTOTYPE(adv_destmask_insert,
	  adv_entry *,
	  (char *,
	   adv_entry *,
	   adv_entry *));
PROTOTYPE(adv_destmask_match,
	  extern adv_entry *,
	  (adv_entry *,
	   sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(adv_aggregate_match,
	  extern adv_entry *,
	  (adv_entry *,
	   rt_entry *,
	   pref_t *));
PROTOTYPE(gw_locate,
	  extern gw_entry *,
	  (gw_entry **,
	   proto_t,
	   task *,
	   as_t,
	   as_t,
	   sockaddr_un *,
	   flag_t));
PROTOTYPE(gw_timestamp,
	  extern gw_entry *,
	  (gw_entry **,
	   proto_t,
	   task *,
	   as_t,
	   as_t,
	   sockaddr_un *,
	   flag_t));
PROTOTYPE(gw_init,
	  extern gw_entry *,
	  (gw_entry *,
	   proto_t,
	   task *,
	   as_t,
	   as_t,
	   sockaddr_un *,
	   flag_t));
PROTOTYPE(gw_dump,
	  extern void,
	  (FILE *,
	   const char *,
	   gw_entry *,
	   proto_t));
PROTOTYPE(gw_freelist,
	  extern void,
	  (gw_entry *));

/* Config info */
PROTOTYPE(config_alloc,
	  extern config_entry *,
	  (int,
	   void_t));
PROTOTYPE(config_append,
	  extern config_entry *,
	  (config_entry *,
	   config_entry *));
PROTOTYPE(config_resolv_ifa,
	  extern config_entry **,
	  (adv_entry *,
	   if_addr *,
	   int));
PROTOTYPE(config_resolv_ifl,
	  extern config_entry **,
	  (adv_entry *,
	   if_link *,
	   int));
PROTOTYPE(config_resolv_free,
	  extern void,
	  (config_entry **,
	   int));
PROTOTYPE(config_list_alloc,
	  extern config_list *,
	  (config_entry *,
	   _PROTOTYPE(entry_free,
		      void,
		      (config_entry *))));
PROTOTYPE(config_list_free,
	  extern void,
	  (config_list *));
PROTOTYPE(config_list_add,
	  extern config_list *,
	  (config_list *,
	   config_entry *,
	   _PROTOTYPE(entry_free,
		      void,
		      (config_entry *))));

PROTOTYPE(policy_family_init,
	  extern void,
	  (void));

extern const byte first_bit_set[256];
