#ident	"@(#)ospf_gated.h	1.3"
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
 * ------------------------------------------------------------------------
 * 
 *                 U   U M   M DDDD     OOOOO SSSSS PPPPP FFFFF
 *                 U   U MM MM D   D    O   O S     P   P F
 *                 U   U M M M D   D    O   O  SSS  PPPPP FFFF
 *                 U   U M M M D   D    O   O     S P     F
 *                  UUU  M M M DDDD     OOOOO SSSSS P     F
 * 
 *     		          Copyright 1989, 1990, 1991
 *     	       The University of Maryland, College Park, Maryland.
 * 
 * 			    All Rights Reserved
 * 
 *      The University of Maryland College Park ("UMCP") is the owner of all
 *      right, title and interest in and to UMD OSPF (the "Software").
 *      Permission to use, copy and modify the Software and its documentation
 *      solely for non-commercial purposes is granted subject to the following
 *      terms and conditions:
 * 
 *      1. This copyright notice and these terms shall appear in all copies
 * 	 of the Software and its supporting documentation.
 * 
 *      2. The Software shall not be distributed, sold or used in any way in
 * 	 a commercial product, without UMCP's prior written consent.
 * 
 *      3. The origin of this software may not be misrepresented, either by
 *         explicit claim or by omission.
 * 
 *      4. Modified or altered versions must be plainly marked as such, and
 * 	 must not be misrepresented as being the original software.
 * 
 *      5. The Software is provided "AS IS". User acknowledges that the
 *         Software has been developed for research purposes only. User
 * 	 agrees that use of the Software is at user's own risk. UMCP
 * 	 disclaims all warrenties, express and implied, including but
 * 	 not limited to, the implied warranties of merchantability, and
 * 	 fitness for a particular purpose.
 * 
 *     Royalty-free licenses to redistribute UMD OSPF are available from
 *     The University Of Maryland, College Park.
 *       For details contact:
 * 	        Office of Technology Liaison
 * 		4312 Knox Road
 * 		University Of Maryland
 * 		College Park, Maryland 20742
 * 		     (301) 405-4209
 * 		FAX: (301) 314-9871
 * 
 *     This software was written by Rob Coltun
 *      rcoltun@ni.umd.edu
 */


#ifndef OSPF_PORT_H
#define OSPF_PORT_H


/* Convert global_tod to char string */
#define ospf_get_ctime() time_string

#define ospf_get_sys_time()

/* Checksum calculations */
#define	ospf_checksum_sum(cp, len, sum)	sum += iso_cksum((void_t) cp, (size_t) len, (byte *) &(cp)->ls_hdr.ls_chksum)

#define	ospf_checksum(cp, len)	(void) iso_cksum((void_t) cp, (size_t) len, (byte *) &(cp)->ls_hdr.ls_chksum)

#define	ospf_checksum_bad(cp, len)	iso_cksum((void_t) cp, (size_t) len, (byte *) 0)

#define	INTF_MTU(intf) \
		(size_t) ((((intf)->type == VIRTUAL_LINK) ? OSPF_MAXVPKT : (intf)->ifap->ifa_mtu) - OSPF_TRL_SIZE(intf))
#define	INTF_ADDR(intf)		sock2ip((intf)->ifap->ifa_addr)
#define	INTF_LCLADDR(intf)	sock2ip((intf)->ifap->ifa_addr_local)
#define	INTF_NET(intf)		sock2ip((intf)->ifap->ifa_net)
#define	INTF_MASK(intf)		sock2ip((intf)->ifap->ifa_netmask)


/* XXX - Maybe we need an intermediate structure? */
#define	ifa_ospf_intf	ifa_ps[RTPROTO_OSPF].ips_datas[0]
#define	ifa_ospf_nh	ifa_ps[RTPROTO_OSPF].ips_datas[1]
#define	ifa_ospf_nh_lcl	ifa_ps[RTPROTO_OSPF].ips_datas[2]

#define	IF_INTF(ifap)	((struct INTF *)(ifap)->ifa_ospf_intf)


#define	OSPF_IFPS_ALLSPF	IFPS_KEEP1	/* Joined All SPF group */
#define	OSPF_IFPS_ALLDR		IFPS_KEEP2	/* Joined All DR group */

#undef	INTF_STATUS_CHANGE

#define	ospf_ifchk(ifap)	BIT_TEST(ifap->ifa_state, IFS_UP)

#define IP_PROTOCOL(IP) (IP)->ip_p
/* The following is to overcome a BSD stupidity in the handling of the
   length field in struct ip (aka struct iphdr). Linux does it correctly. */
#ifdef linux
#define IP_LENGTH(IP)   ntohs((IP)->ip_len) - 4*(IP)->ip_hl
#else
#define IP_LENGTH(IP) 	(IP)->ip_len
#endif /* linux */

#define	OSPF_AUTH_NONE			0	/* No authentication */
#define	OSPF_AUTH_SIMPLE		1	/* Simple password */
#define	OSPF_AUTH_MD5			2	/* MD5 crypto checksum */

/* Export types */
#define	OSPF_EXPORT_TYPE1	0x01
#define	OSPF_EXPORT_TYPE2	0x02

#define	OSPF_EXPORT_TAG		0x04		/* Tag is present */
#define	OSPF_EXPORT_TAG_METRIC2	0x08		/* Tag is in metric2 vs metric */
#define	OSPF_ADV_TAG(adv)	(BIT_TEST((adv)->adv_result.res_flag, OSPF_EXPORT_TAG_METRIC2) ? \
				 (adv)->adv_result.res_metric2 : (adv)->adv_result.res_metric)

/* Default syslog limits */
#define	OSPF_LOG_FIRST	16
#define	OSPF_LOG_EVERY	256

/* Defines for the parser */
#define	OSPF_LIMIT_COST			0, RTRLSInfinity
#define	OSPF_LIMIT_METRIC		0, ASELSInfinity
#define	OSPF_LIMIT_AREA			1, 0xffffffff
#define	OSPF_LIMIT_RETRANSMITINTERVAL	0, 0xffff
#define	OSPF_LIMIT_ROUTERDEADINTERVAL	0, 0xffff
#define	OSPF_LIMIT_HELLOINTERVAL	0, 0xff
#define	OSPF_LIMIT_POLLINTERVAL		0, 0xff
#define	OSPF_LIMIT_TRANSITDELAY		0, 0xffff
#define	OSPF_LIMIT_DRPRIORITY		0, 0xff
#define	OSPF_LIMIT_ACKTIMER		0, 0xffff
#define	OSPF_LIMIT_TAG			0, 0xffffffff
#define	OSPF_LIMIT_EXPORTTYPE		OSPF_EXPORT_TYPE1, OSPF_EXPORT_TYPE2
#define	OSPF_LIMIT_LOG_FIRST	0, (u_int) -1
#define	OSPF_LIMIT_LOG_EVERY	0, (u_int) -1

#define	OSPF_CONFIG_TYPE	1	/* Interface type */
#define	OSPF_CONFIG_COST	2	/* Interface cost */
#define	OSPF_CONFIG_ENABLE	3	/* Enable/disable */
#define	OSPF_CONFIG_RETRANSMIT	4	/* Retransmit interval */
#define	OSPF_CONFIG_TRANSIT	5	/* Transit delay */
#define	OSPF_CONFIG_PRIORITY	6	/* Priority */
#define	OSPF_CONFIG_HELLO	7	/* Hello interval */
#define	OSPF_CONFIG_ROUTERDEAD	8	/* Router dead interval */
#define	OSPF_CONFIG_AUTH	9	/* Authentication */
#define	OSPF_CONFIG_AUTH2	10	/* Authentication */
#define	OSPF_CONFIG_POLL	11	/* NBMA Poll interval */
#define	OSPF_CONFIG_ROUTERS	12	/* NBMA routers */
#define	OSPF_CONFIG_NOMULTI	13	/* For P2P interfaces */
#define	OSPF_CONFIG_MAX		14

typedef struct _ospf_config_router {
    struct _ospf_config_router *ocr_next;
    struct in_addr ocr_router;
    u_int ocr_priority;
} ospf_config_router ;

PROTOTYPE(ospf_config_free,
	  extern void,
	  (config_entry *));
PROTOTYPE(ospf_parse_router_alloc,
	  extern ospf_config_router *,
	  (struct in_addr,
	   u_int));

/* Defaults for ASE imports */
#define	OSPF_DEFAULT_METRIC	1
#define	OSPF_DEFAULT_TAG	PATH_OSPF_TAG_TRUSTED
#define	OSPF_DEFAULT_TYPE	OSPF_EXPORT_TYPE2

#ifdef	PROTO_ASPATHS
#define	ospf_path_tag_dump(as, tag)	sockbuild_str(aspath_tag_dump(as, tag))
#else	/* PROTO_ASPATHS */
#define	ospf_path_tag_dump(as, tag)	sockbuild_in(0, htonl(tag))
#endif	/* PROTO_ASPATHS */

#define	bgp_routesync_ospf(x)	0

#define	OSPF_HOP			1	/* Value to add to ifa_metric to get default interface cost */

extern adv_psfunc ospf_adv_psfunc;

#ifdef	IP_MULTICAST
extern sockaddr_un *ospf_addr_allspf;
extern sockaddr_un *ospf_addr_alldr;
#endif	/* IP_MULTICAST */

extern const bits ospf_trace_types[];		/* OSPF specific tracing flags */

PROTOTYPE(ospf_parse_area_alloc,
	  extern struct AREA *,
	  (u_int32,
	   char *));
PROTOTYPE(ospf_parse_area_check,
	  extern int,
	  (struct AREA *,
	   char *));
PROTOTYPE(ospf_parse_intf_alloc,
	  extern struct INTF *,
	  (struct AREA *,
	   int,
	   if_addr *));
PROTOTYPE(ospf_parse_virt_parse,
	  extern struct INTF *,
	  (struct AREA *,
	   sockaddr_un *,
	   u_int32,
	   config_list *,
	   char *));
PROTOTYPE(ospf_parse_intf_check,
	  extern void,
	  (struct INTF *intf));
PROTOTYPE(ospf_parse_valid_check,
	  extern int,
	  (char *));
PROTOTYPE(ospf_parse_add_net,
	  void,
	  (struct AREA *,
	   sockaddr_un *,
	   sockaddr_un *,
	   u_int));
PROTOTYPE(ospf_parse_add_host,
	  extern void,
	  (struct AREA *,
	   u_int32,
	   metric_t));
PROTOTYPE(ospf_init,
	  extern void,
	  (void));
PROTOTYPE(ospf_var_init,
	  extern void,
	  (void));
PROTOTYPE(ospf_txpkt,
	  extern void,
	  (struct OSPF_HDR *,
	   struct INTF *,
	   u_int,
	   size_t,
	   u_int32,
	   int));
PROTOTYPE(ospf_policy_init,
	  extern void,
	  (task *));
PROTOTYPE(ospf_policy_cleanup,
	  extern void,
	  (task *));
PROTOTYPE(ospf_freeRangeList,
	  extern void,
 	  (struct AREA *));
PROTOTYPE(ospf_freeHostsList,
	  extern void,
 	  (struct AREA *));
PROTOTYPE(ospf_multicast_alldr,
	  extern void,
	  (struct INTF *,
	   int));


/**/
/* Routing table */

#define IS_HOST(R) 		(RT_MASK(R) == HOST_NET_MASK)

/*
 * References for the the routing table access
 */

#define ORT_INFO(rt)	((OSPF_RT_INFO * ) (rt)->rt_data)
#define	ORT_INFO_VALID(rt)	((rt) && (rt)->rt_data)
#define ORT_DTYPE(rt) 	(ORT_INFO(rt)->dtype)
#define ORT_ETYPE(rt) 	(ORT_INFO(rt)->etype)
#define ORT_CHANGE(rt) 	(ORT_INFO(rt)->change)
#define ORT_PTYPE(rt) 	(ORT_INFO(rt)->ptype)
#define ORT_REV(rt) 	(ORT_INFO(rt)->revision)
#define ORT_AREA(rt) 	(ORT_INFO(rt)->area)
#define ORT_COST(rt)	(ORT_INFO(rt)->cost)
#define ORT_NH(rt,I) 	 (ORT_INFO(rt)->nh_ndx[I]->nh_addr)
#define ORT_IO_NDX(rt,I) (ORT_INFO(rt)->nh_ndx[I]->nh_ifap)
#define ORT_NH_NDX(rt,I) (ORT_INFO(rt)->nh_ndx[I])
#define ORT_NH_CNT(rt) 	(ORT_INFO(rt)->nh_cnt)
#define ORT_ADVRTR(rt) 	(ORT_INFO(rt)->advrtr)
#define ORT_OSPF_PREF(rt) (ORT_INFO(rt)->preference)
#define ORT_V(rt) 	(ORT_INFO(rt)->v)

/* OSPF's routing table structure */

#define RT_DEST(rt) 	sock2ip((rt)->rt_dest)
#define	RT_MASK(rt)	sock2ip((rt)->rt_dest_mask)
#define	RT_NEXTHOP(rt)	sock2ip(RT_ROUTER(rt))

/*
 *  For exporting gated routes to OSPF.
 */
typedef struct _ospf_export_entry {
    struct _ospf_export_entry *forw;
    struct _ospf_export_entry *back;
    rt_entry *old_rt;		/* points at route with bit set, if any */
    struct LSDB *db;		/* points to LS db entry, if any */
    rt_entry *new_rt;		/* points at exportable active route, if any */
    metric_t metric;
    u_int32 tag;
    struct in_addr forward;
} ospf_export_entry;


/**/
/* Timers */

PROTOTYPE(ospf_ifdown,
	  extern void,
	  (struct INTF *));
PROTOTYPE(ospf_ifup,
	  extern void,
	  (struct INTF *));
PROTOTYPE(tq_hellotmr,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_adjtmr,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_lsa_lock,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_IntLsa,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_SumLsa,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_retrans,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_ack,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_int_age,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_sum_age,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(tq_ase_age,
	  extern void,
	  (task_timer *,
	   time_t));

/* SNMP support */
#ifdef	PROTO_SNMP
PROTOTYPE(ospf_init_mib,
	  extern void,
	  (int));
PROTOTYPE(o_intf_get,
	  extern void,
	  (void));
PROTOTYPE(o_vintf_get,
	  extern void,
	  (void));
#endif	/* PROTO_SNMP */

#endif	/* OSPF_PORT_H */
