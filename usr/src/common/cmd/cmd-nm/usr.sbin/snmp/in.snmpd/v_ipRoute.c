#ident	"@(#)v_ipRoute.c	1.5"
#ident   "$Header$"

/*
 * STREAMware TCP
 * Copyright 1987, 1993 Lachman Technology, Inc.
 * All Rights Reserved.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef lint
static char SNMPID[] = "@(#)v_ipRoute.c	1.5";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

#include <sys/param.h>

#if !defined(SVR3) && !defined(SVR4)
#include <sys/vmmac.h>
#include <machine/pte.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#if !defined(SVR3) && !defined(SVR4)
#include <sys/mbuf.h>
#endif

#if defined(SVR3) || defined(SVR4)

#ifdef PSE
#include <common.h>
#endif

#include <sys/stream.h>
#include <sys/stropts.h>
#endif

#include <net/if.h>
#include <net/af.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <net/route.h>

#ifndef SUNOS35
#include <netinet/in_var.h>
#endif

#if defined(SVR3) || defined(SVR4)
#include <netinet/ip_str.h>
#endif

#ifdef SVR4
#include <sys/sockio.h>
#endif

#ifdef BSD
#include <sys/ioctl.h>
#endif

#include <syslog.h>
#include <unistd.h>

#ifdef NEW_MIB
#include <paths.h>
#endif /* NEW_MIB */

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

VarBindList *get_next_class();

#ifdef NEW_MIB
struct mib_rt_entry {
   struct   sockaddr rt_dst;
   struct   sockaddr rt_gateway;
   struct   sockaddr rt_mask;
   short rt_flags;
   short rt_refcnt;
   short rt_use;
   short rt_index;
   struct   rt_metrics rt_rmx;
   int   rt_proto;
   time_t   rt_age;
};
#endif

struct z_rtype {
   int type;
   char *oid;
};

struct z_rtype rtypes[] = {
   RTP_OTHER, "0.0",
   RTP_LOCAL, "0.0",
   RTP_NETMGMT, "0.0",
   RTP_ICMP, "icmp.0",
   RTP_EGP, "egp.0",
   RTP_GGP, "0.0",
   RTP_HELLO, "0.0",
   RTP_RIP, "0.0",
   RTP_IS_IS, "0.0",
   RTP_ES_IS, "0.0",
   RTP_CISCOIGRP, "0.0",
   RTP_BBNSPFIGP, "0.0",
   RTP_OSPF, "mib-2.14",
   RTP_BGP, "mib-2.15"
};
int n_rtypes = sizeof(rtypes) / sizeof(struct z_rtype);

VarBindList *
var_ip_route_get(OID var_name_ptr, OID in_name_ptr, 
                 unsigned int arg, VarEntry *var_next_ptr, int type_search)
{
   VarBindList *vb_ptr;
   OID oid_ptr;
   OID oidvalue_ptr;
   OctetString *os_ptr;
   unsigned long test_addr, temp_ip_addr, final_ip_addr;

#ifdef NEW_MIB
   struct mib_rt_entry route_entry;
#else
   struct rtentry route_entry;
#endif
#if defined(BSD) || defined(SVR4) || defined(TCP40)
   struct sockaddr_in *sin;
#endif
#if defined(SVR3) && !defined(TCP40)
   struct in_addr *in;
#endif

   int cc;
   char buffer[255];
   unsigned long metric1, type;
   unsigned char netaddr[4];
   int i;
   int if_num;
   char *mibp;
   time_t curtime;

  /* see if explicitly determined */
   if((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 4)))
      return(NULL);

   test_addr = 0;

   if(in_name_ptr->length > var_name_ptr->length) 
      {
      for(i=var_name_ptr->length; 
            ((i < in_name_ptr->length) && (i < var_name_ptr->length + 4)); 
            i++)
         test_addr = (test_addr << 8) + in_name_ptr->oid_ptr[i];  /* first sub-field after name */

      for(; i < var_name_ptr->length + 4; i++)
         test_addr = test_addr << 8;
      }

  /* If get-next and was fully qualified, bump up one */
   if((type_search == NEXT) && (in_name_ptr->length >= var_name_ptr->length+4))
      test_addr++;
#ifdef NEW_MIB
   cc = get_route_entry(test_addr, 4, &route_entry);
#else
   cc = get_route_entry(nl[N_RTHOST].n_value, nl[N_RTNET].n_value,
                        nl[N_RTHASHSIZE].n_value, test_addr, 4, &route_entry);
#endif

   if(cc == FALSE) 
      {
      if(type_search == NEXT)
         return(get_next_class(var_next_ptr));

      if(type_search == EXACT)
         return(NULL);
      }

#if defined(BSD) || defined(SVR4) || defined(TCP40)
   sin = (struct sockaddr_in *)&route_entry.rt_dst;
   final_ip_addr = ntohl(sin->sin_addr.s_addr);
#endif

#if defined(SVR3) && !defined(TCP40)
   in = &route_entry.rt_dst;
   final_ip_addr = ntohl(in->s_addr);
#endif

   if((type_search == EXACT) && (final_ip_addr != test_addr))
      return(NULL);

  /* Now determine which variable */
   switch(arg) 
      {
      case 1:
         sprintf(buffer,"ipRouteDest.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
         netaddr[0] = ((final_ip_addr>>24) & 0xFF);
         netaddr[1] = ((final_ip_addr>>16) & 0xFF);
         netaddr[2] = ((final_ip_addr>>8) & 0xFF);
         netaddr[3] = (final_ip_addr & 0xFF);

         os_ptr = make_octetstring(netaddr, 4);
         vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
         os_ptr = NULL;
         oid_ptr = NULL;
      break;

      case 2:
         sprintf(buffer,"ipRouteIfIndex.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
         cc = get_if_number_for_route(nl[N_IFNET].n_value, &if_num, route_entry.rt_ifp);
#endif

#if(defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
         cc = get_if_number_for_route(nl[N_PROVIDER].n_value, &if_num, route_entry.rt_ifp);
#endif

#ifdef NEW_MIB
         cc = get_if_number_for_route(&if_num, route_entry.rt_index);
#endif

    /* begin mod 11/8/89 JDC */
         if(cc == FALSE) 
            {
            syslog(LOG_WARNING, gettxt(":182", "get_route_entry: if_num: %m.\n"));
            if_num = -1;
            }
    /* end mod 11/8/89 JDC */

         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_num, os_ptr, NULL);
         os_ptr = NULL;
         oid_ptr = NULL;
      break;
  
      case 3:
         sprintf(buffer,"ipRouteMetric1.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

#ifdef NEW_MIB
         metric1 = route_entry.rt_rmx.rmx_hopcount;
#else
         metric1 = route_entry.rt_metric;
#endif   /* NEW_MIB */

         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, metric1, NULL, NULL);
         oid_ptr = NULL;
      break;

      case 4:
         sprintf(buffer,"ipRouteMetric2.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

#ifdef CISCO_STUFF
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, 0, NULL, NULL);
#else
#ifdef NEW_MIB
         metric1 = route_entry.rt_rmx.rmx_rtt;
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, metric1, NULL, NULL);
#else
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, -1, NULL, NULL);
#endif
#endif

         oid_ptr = NULL;
      break;

      case 5:
         sprintf(buffer,"ipRouteMetric3.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef CISCO_STUFF

         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, 0, NULL, NULL);
#else
#ifdef NEW_MIB
         metric1 = route_entry.rt_rmx.rmx_rttvar;
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, metric1, NULL, NULL);
#else
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, -1, NULL, NULL);
#endif
#endif
         oid_ptr = NULL;
      break;

      case 6:
         sprintf(buffer,"ipRouteMetric4.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

#ifdef CISCO_STUFF
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, 0, NULL, NULL);
#else
#ifdef NEW_MIB
         metric1 = route_entry.rt_rmx.rmx_sendpipe;
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, metric1, NULL, NULL);
#else
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, -1, NULL, NULL);
#endif
#endif

         oid_ptr = NULL;
      break;

      case 7:
         sprintf(buffer,"ipRouteNextHop.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

#ifdef NEW_MIB
#if defined(BSD) || defined(SVR4) || defined(TCP40)
         sin = (struct sockaddr_in *) &(route_entry.rt_gateway);
         temp_ip_addr = ntohl(sin->sin_addr.s_addr);
#endif
#if defined(SVR3) && !defined(TCP40)
         in = route_entry.rt_gateway;
         temp_ip_addr = ntohl(in->s_addr);
#endif
#else

#if defined(BSD) || defined(SVR4) || defined(TCP40)
         sin = (struct sockaddr_in *) &route_entry.rt_gateway;
         temp_ip_addr = ntohl(sin->sin_addr.s_addr);
#endif
#if defined(SVR3) && !defined(TCP40)
         in = &route_entry.rt_gateway;
         temp_ip_addr = ntohl(in->s_addr);
#endif
#endif   /* NEW_MIB */
         netaddr[0] = ((temp_ip_addr>>24) & 0xFF);
         netaddr[1] = ((temp_ip_addr>>16) & 0xFF);
         netaddr[2] = ((temp_ip_addr>>8) & 0xFF);
         netaddr[3] = (temp_ip_addr & 0xFF);
         os_ptr = make_octetstring(netaddr, 4);
         vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
         oid_ptr = NULL;
         os_ptr = NULL;
      break;

      case 8:
         sprintf(buffer,"ipRouteType.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

    /* Determine route type (not protocol, that's next var. */
         if((route_entry.rt_flags & RTF_UP) == 0)
            type = 2;

         if(route_entry.rt_flags & RTF_GATEWAY)
            type = 4;         /* remote net */
         else
            type = 3;         /* directly connected */

         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, type, NULL, NULL);
         oid_ptr = NULL;
      break;

      case 9:
         sprintf(buffer,"ipRouteProto.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
         type = route_entry.rt_proto;
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, type, NULL, NULL);
         oid_ptr = NULL;
      break;

      case 10:
         sprintf(buffer,"ipRouteAge.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

    /* Determine route type (not protocol, that's next var. */
         curtime = time((long *)0);
         type = (int)(curtime - route_entry.rt_age);
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, type, NULL, NULL);
         oid_ptr = NULL;
      break;

      case 11:
         sprintf(buffer,"ipRouteMask.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

#if !defined(NEW_MIB)
         if(final_ip_addr == INADDR_ANY)
            temp_ip_addr = INADDR_ANY;
         else if(IN_CLASSA(final_ip_addr))
            temp_ip_addr = IN_CLASSA_NET;
         else if(IN_CLASSB(final_ip_addr))
            temp_ip_addr = IN_CLASSB_NET;
         else if(IN_CLASSC(final_ip_addr))
            temp_ip_addr = IN_CLASSC_NET;
#else
#if defined(BSD) || defined(TCP40)
         sin = (struct sockaddr_in *)&route_entry.rt_mask;
         temp_ip_addr = ntohl(sin->sin_addr.s_addr);
#endif
#ifdef notdef /* dme */
#if !defined(TCP40)
         in = &route_entry.rt_dst;
         temp_ip_addr = ntohl(in->s_addr);
#endif
#endif /* notdef dme */
#endif 

         netaddr[0] = ((temp_ip_addr>>24) & 0xFF);
         netaddr[1] = ((temp_ip_addr>>16) & 0xFF);
         netaddr[2] = ((temp_ip_addr>>8) & 0xFF);
         netaddr[3] = (temp_ip_addr & 0xFF);
         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
         os_ptr = make_octetstring(netaddr, 4);
         vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
         oid_ptr = NULL;
         os_ptr = NULL;
      break;

      case 12:
         sprintf(buffer,"ipRouteMetric5.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

#ifdef CISCO_STUFF
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, 0, NULL, NULL);
#else
#ifdef NEW_MIB
         metric1 = route_entry.rt_rmx.rmx_tos;
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, metric1, NULL, NULL);
#else
         vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, -1, NULL, NULL);
#endif
#endif
         oid_ptr = NULL;
      break;

      case 13:
         sprintf(buffer,"ipRouteInfo.%d.%d.%d.%d",
                  ((final_ip_addr>>24) &0xFF),
                  ((final_ip_addr>>16) &0xFF),
                  ((final_ip_addr>>8) &0xFF),
                  (final_ip_addr &0xFF));

         oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

         if(route_entry.rt_proto < RTP_OTHER)
            route_entry.rt_proto = RTP_OTHER;

         for(i = 0; i < n_rtypes; i++) 
            {
            if(route_entry.rt_proto == rtypes[i].type) 
               {
               mibp = rtypes[i].oid;
               break;
               }
            }

         if(i >= n_rtypes)
            mibp = rtypes[0].oid;

         oidvalue_ptr = make_obj_id_from_dot((unsigned char *)mibp);
         vb_ptr = make_varbind(oid_ptr, OBJECT_ID_TYPE, 0, 0, NULL, oidvalue_ptr);
         oid_ptr = NULL;
         oidvalue_ptr = NULL;
      break;

      default:
         if(type_search == EXACT)
            return(NULL);
         else
            return(get_next_class(var_next_ptr));
      };

   return(vb_ptr);
   }

#ifdef NEW_MIB
extern int rte_fd;

int get_route_entry(unsigned long ip_addr, int ip_addr_len, 
                     struct mib_rt_entry *route_entry)
   {
   int   i;
   char  *buffer, *start, *limit, *next;
   struct rt_msghdr *rtm, *best_rtm;
   struct sockaddr_in sin1, *sin2;
   struct sockaddr *sa;
   struct sockaddr_in *sa_dst, *sa_gw;
   struct in_addr best_ip_addr;
   unsigned long t2_addr, t1_addr, b_addr;
   struct rt_giarg gi_arg, *gp;
   unsigned long final_ip_addr;

   sin1.sin_family = AF_INET;
   sin1.sin_port = 0;
   sin1.sin_addr.s_addr = htonl(ip_addr);
   best_ip_addr.s_addr = 0xffffffff;

   if(rte_fd < 0) 
      {
      if((rte_fd = open(_PATH_ROUTE, O_WRONLY)) < 0) 
         {
         syslog(LOG_WARNING, gettxt(":183", "get_route_entry: Open of %s failed: %m.\n"),
               _PATH_ROUTE);
         return(FALSE);
         }
      }

   gi_arg.gi_op = KINFO_RT_DUMP;
   gi_arg.gi_where = (caddr_t)0;
   gi_arg.gi_size = 0;
   gi_arg.gi_arg = 0;

   if(ioctl(rte_fd, RTSTR_GETROUTE, &gi_arg) < 0) 
      {
      syslog(LOG_WARNING, gettxt(":184", "get_route_entry: RTSTR_GETROUTE: %m.\n"));
      (void) close(rte_fd);
      rte_fd = -1;
      return(FALSE);
      }

   if((buffer = (char *) malloc(gi_arg.gi_size)) == NULL) 
      {
      syslog(LOG_WARNING, gettxt(":185", "get_route_entry: malloc: %m.\n"));
      return(FALSE);
      }

   gp = (struct rt_giarg *) buffer;
   gp->gi_op = KINFO_RT_DUMP;
   gp->gi_where = (caddr_t)buffer;
   gp->gi_size = gi_arg.gi_size;
   gp->gi_arg = 0;

   if(ioctl(rte_fd, RTSTR_GETROUTE, buffer) < 0) 
      {
      syslog(LOG_WARNING, gettxt(":184", "get_route_entry: RTSTR_GETROUTE: %m.\n"));
      (void) free(buffer);
      (void) close(rte_fd);
      rte_fd = -1;
      return(FALSE);
      }

   limit = buffer + gp->gi_size;
   start = buffer + sizeof(gi_arg);
   for(next = start; next < limit; next += rtm->rtm_msglen) 
      {
      rtm = (struct rt_msghdr *)next;
      sa = (struct sockaddr *)(rtm + 1);

      if (rtm->rtm_flags & RTF_LLINFO)
	      continue;
   /* bcopy((char *)sa, (char *)sa_dst, sizeof(struct sockaddr_in)); */
      sin2 = (struct sockaddr_in *)sa;
      t2_addr = ntohl(sin2->sin_addr.s_addr);
      t1_addr = ntohl(sin1.sin_addr.s_addr);
      b_addr = ntohl(best_ip_addr.s_addr);

      if(((t2_addr >= t1_addr) && (t2_addr < b_addr) && (ip_addr_len != 5)) ||
         ((t2_addr > t1_addr) && (t2_addr < b_addr))) 
            {
            best_ip_addr.s_addr = sin2->sin_addr.s_addr;
            best_rtm = rtm;
            }
      }

    /* must be the last entry, hop to next class */
   if(best_ip_addr.s_addr == 0xffffffff) 
      {
      (void) free(buffer);
      return(FALSE);
      }

    /* extract the desired info. here ... */
   route_entry->rt_flags = best_rtm->rtm_flags;
   route_entry->rt_refcnt = best_rtm->rtm_refcnt;
   route_entry->rt_use = best_rtm->rtm_use;
   route_entry->rt_index = best_rtm->rtm_index;
   route_entry->rt_rmx = best_rtm->rtm_rmx;
   route_entry->rt_proto = best_rtm->rtm_proto;
   route_entry->rt_age = best_rtm->rtm_age;

   sa = (struct sockaddr *)(best_rtm + 1);
   bcopy((char *)sa, (char *)&(route_entry->rt_dst),
         sizeof(struct sockaddr));

   sa++;

   if(best_rtm->rtm_addrs & RTA_GATEWAY) 
      {
      bcopy((char *)sa, (char *)&(route_entry->rt_gateway),
            sizeof(struct sockaddr));
      }

   sa++;
   if(best_rtm->rtm_addrs & RTA_NETMASK) 
      {
      bcopy((char *)sa, (char *)&(route_entry->rt_mask),
            sizeof(struct sockaddr));
      }

   (void) free(buffer);

   return(TRUE);
   }

#else
get_route_entry(off_t hostaddr, off_t netaddr, off_t hashsizeaddr, 
               unsigned long ip_addr, int ip_addr_len, 
               struct rtentry *route_entry)
   {
#ifdef BSD
#ifdef ULTRIX
   struct rtentry mb;
   register struct rtentry *rt;
   register struct rtentry *m;
   struct rtentry **routehash;
#else
   struct mbuf mb;
   register struct rtentry *rt;
   register struct mbuf *m;
   struct mbuf **routehash;
#endif
#endif
#if defined(SVR3) || defined(SVR4)
   mblk_t mb;
   register struct rtentry *rt;
   register mblk_t *m;
   mblk_t **routehash;
   struct rtentry rtentry;
#endif
  /*  struct ifnet ifnet; */
   int hashsize;
   struct sockaddr_in sin, *sin2;
#if defined(SVR3) && !defined(TCP40)
   struct in_addr *in;
#endif
#ifdef BSD
#ifdef ULTRIX
   register struct rtentry *m_best_rtentry;
#else
   register struct mbuf *m_best_rtentry;
#endif
#endif
#if defined(SVR3) || defined(SVR4)
   register mblk_t *m_best_rtentry;
#endif
   struct in_addr best_ip_addr;
   int i;
   unsigned long t2_addr, t1_addr, b_addr;
   off_t seekaddr;

   sin.sin_family = AF_INET;
   sin.sin_port = 0;
   sin.sin_addr.s_addr = htonl(ip_addr);
  
   if(hostaddr == 0) 
      {
      syslog(LOG_WARNING, gettxt(":186", "rthost: Symbol not in namelist.\n"));
      return(FALSE);
      }

   if(netaddr == 0) 
      {
      syslog(LOG_WARNING, gettxt(":187", "rtnet: Symbol not in namelist.\n"));
      return(FALSE);
      }

#if defined(SUNOS35) || defined(SUNOS40)
   hashsize = SUNROUTEHASHSIZE;
#else
   if(hashsizeaddr == 0) 
      {
      syslog(LOG_WARNING, gettxt(":188", "rthashsize: Symbol not in namelist.\n"));
      return(FALSE);
      }
   lseek(kmem, hashsizeaddr, 0);

   if(read(kmem, &hashsize, sizeof(hashsize)) < 0)
      return(FALSE);
#endif

#ifdef BSD
#ifdef ULTRIX
   routehash = (struct rtentry **) malloc(hashsize*sizeof(struct rtentry *));
#else
   routehash = (struct mbuf **) malloc(hashsize*sizeof(struct mbuf *));
#endif
#endif
#if defined(SVR3) || defined(SVR4)
   routehash = (mblk_t **) malloc(hashsize*sizeof(mblk_t *));
#endif
   if(routehash == NULL) 
      {
      syslog(LOG_WARNING, gettxt(":189", "get_route_entry: malloc failed: %m.\n"));
      return(FALSE);
      }

   best_ip_addr.s_addr = 0xffffffff;
   m_best_rtentry = NULL;
  
   seekaddr = netaddr;

again:
   lseek(kmem, seekaddr, 0);
#ifdef BSD
#ifdef ULTRIX
   read(kmem, routehash, hashsize*sizeof(struct rtentry *));
#else
   read(kmem, routehash, hashsize*sizeof(struct mbuf *));
#endif
#endif
#if defined(SVR3) || defined(SVR4)
   read(kmem, routehash, hashsize*sizeof(mblk_t *));
#endif
  
/*  inet_hash(&sin, &h);  now work this in */
/*  printf("Hash size: %d, hash%d \n", hashsize, (h.afh_nethash % hashsize)); */
  
   for(i = 0; i < hashsize; i++) 
      {
      m = routehash[i];
    
      while(m) 
         {
         lseek(kmem, (off_t)m, 0);
         read(kmem, &mb, sizeof(mb));
#ifdef BSD
#ifdef ULTRIX
         rt = &mb;
#else
         rt = mtod(&mb, struct rtentry *);
#endif
      /*    rt = mtod(m, struct rtentry *); */
#endif
#if defined(SVR3) || defined(SVR4)
         lseek(kmem, (off_t)mb.b_rptr, 0);

         if(read(kmem, &rtentry, sizeof(rtentry)) < 0)
            return(FALSE);

         rt = &rtentry;
#endif

#if defined(BSD) || defined(SVR4) || defined(TCP40)
         sin2 = (struct sockaddr_in *)&rt->rt_dst;
         t2_addr = ntohl(sin2->sin_addr.s_addr);
#endif
#if defined(SVR3) && !defined(TCP40)
         in = &rt->rt_dst;
         t2_addr = ntohl(in->s_addr);
#endif
         t1_addr = ntohl(sin.sin_addr.s_addr);
         b_addr = ntohl(best_ip_addr.s_addr);

         if(((t2_addr >= t1_addr) && 
            (t2_addr < b_addr) && (ip_addr_len != 5)) || 
            ((t2_addr > t1_addr) && 
            (t2_addr < b_addr))) 
               {
#if defined(BSD) || defined(SVR4) || defined(TCP40)
               best_ip_addr.s_addr = sin2->sin_addr.s_addr;
#endif
#if defined(SVR3) && !defined(TCP40)
               best_ip_addr.s_addr = in->s_addr;
#endif
               m_best_rtentry = m;
               }
#ifdef BSD
#ifdef ULTRIX
         m = mb.rt_next;
#else
         m = mb.m_next;
#endif
#endif
#if defined(SVR3) || defined(SVR4)
         m = mb.b_cont;
#endif

         } /* end of while loop */
      } /* end of for i < hashsize loop */

   if(seekaddr == netaddr) 
      {
      seekaddr = hostaddr;
      goto again;
      }

   if(best_ip_addr.s_addr == 0xffffffff) 
      { /* must've gotten last entry, hop to next class */
      free(routehash);
      return(FALSE);
      }

  /* return the best one */
   lseek(kmem, (off_t)m_best_rtentry, 0);
   read(kmem, &mb, sizeof(mb));
#ifdef BSD
#ifdef ULTRIX
   rt = &mb;
#else
   rt = mtod(&mb, struct rtentry *);
  /* we need to check and make sure this is not junk! */
#endif
#endif
#if defined(SVR3) || defined(SVR4)
   lseek(kmem, (off_t)mb.b_rptr, 0);

   if(read(kmem, &rtentry, sizeof(rtentry)) < 0)
      return(FALSE);

   rt = &rtentry;
#endif

   route_entry->rt_hash = rt->rt_hash;
   route_entry->rt_dst = rt->rt_dst;
   route_entry->rt_gateway = rt->rt_gateway;
   route_entry->rt_flags = rt->rt_flags;
   route_entry->rt_refcnt = rt->rt_refcnt;
   route_entry->rt_use = rt->rt_use;

#ifdef BSD
   route_entry->rt_ifp = rt->rt_ifp;
#endif
#if defined(SVR3) || defined(SVR4)
   route_entry->rt_ifp = rt->rt_ifp;
#endif

   route_entry->rt_metric = rt->rt_metric;
   route_entry->rt_proto = rt->rt_proto;
   route_entry->rt_age = rt->rt_age;
  
   free(routehash);
  /* send back TRUE */

   return(TRUE);
   }
#endif

#ifdef BSD
/* Find out which interface number corresponds to the ip addr given */
get_if_number_for_route(off_t ifnetaddr, int *if_num, struct ifnet *ifnet_ptr)
   {
   int i;
   off_t ifaddraddr;
   struct ifnet ifnet_entry;

#ifdef SUNOS35
   struct sockaddr_in ifaddr_entry;
#else
   struct ifaddr ifaddr_entry;
#endif

   if(ifnetaddr == 0) 
      {
      syslog(LOG_WARNING, gettxt(":190", "ifnet: Symbol not defined.\n"));
      return(FALSE);
      }

   lseek(kmem, ifnetaddr, 0);

   if(read(kmem, &ifnetaddr, sizeof(ifnetaddr)) < 0) 
      {
      syslog(LOG_WARNING, gettxt(":191", "get_if_number_for_route: ifnetaddr: %m.\n"));
      return(FALSE);
      }

   if(ifnetaddr == NULL)  
      return(FALSE);

   i = 1;

   while(ifnetaddr != (off_t)ifnet_ptr) 
      {
      if(ifnetaddr == NULL) 
         {
         return(FALSE);
         }
    
      lseek(kmem, ifnetaddr, 0);
      
      if(read(kmem, &ifnet_entry, sizeof(struct ifnet)) < 0) 
         {
         syslog(LOG_WARNING, gettxt(":192", "get_if_number_for_route: ifnet_entry: %m.\n"));
         return(FALSE);
         }
    
#ifdef SUNOS35
      if(ifnet_entry.if_addr.sa_family == AF_INET)
         i++;
#else
      ifaddraddr = (off_t) ifnet_entry.if_addrlist;
    
      if(ifaddraddr) 
         {
      /* now find an internet address on the interface, if any */
         do 
            {
            lseek(kmem, ifaddraddr, 0);
            if(read(kmem, &ifaddr_entry, sizeof(struct ifaddr)) < 0) 
               {
               syslog(LOG_WARNING, gettxt(":193", "get_if_number_for_route: ifaddr_entry: %m.\n"));
               return(FALSE);
               }

            ifaddraddr = (off_t)ifaddr_entry.ifa_next;
            } 
         while((ifaddraddr) && (ifaddr_entry.ifa_addr.sa_family != AF_INET));
  
      /* If this has an IP addr, then this counts as an IP interface.  */
         if(ifaddr_entry.ifa_addr.sa_family == AF_INET)
            i++; /* We've got an IP interface */
         }
#endif
    
      ifnetaddr = (off_t)ifnet_entry.if_next;
      } 

   *if_num = i;

   return(TRUE);
   }
#endif /* BSD */
#if(defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
/* Find out which interface number corresponds to the ip addr given */
get_if_number_for_route(off_t provaddr, int *if_num, 
                        struct ifnet *ifnet_ptr)
   {
   struct ifnet ifnet;

   lseek(kmem, (off_t) ifnet_ptr, 0);
   if(read(kmem, &ifnet, sizeof(ifnet)) < 0)
      {
      syslog(LOG_WARNING, gettxt(":233", "get_if_number_for_route: lastprov: %m.\n"));
      return(FALSE);
      }

    if (ifnet.if_qbot == NULL)
      return(FALSE); /* interface must have gone away */

   *if_num = ifnet.if_index;

   return(TRUE);
   }
#endif /* SVR3 || SVR4 */

#ifdef NEW_MIB
extern int ip_fd, nfds;
extern fd_set ifds;

/* Find out which interface number corresponds to the ip addr given */
get_if_number_for_route(int *if_num, int index)
   {
   struct ifreq_all  if_all;
   struct strioctl strioc;

   if(ip_fd < 0) 
      {
      if((ip_fd = open(_PATH_IP, O_RDWR)) < 0) 
         {
         syslog(LOG_WARNING, gettxt(":196", 
               "get_if_number_for_route: Open of %s failed: %m.\n"),
               _PATH_IP);
         *if_num = -1;
         return(FALSE);
         }
      else 
         {
       /* Set up to receive link-up/down traps */
         strioc.ic_cmd = SIOCSIPTRAP;
         strioc.ic_dp = (char *)0;
         strioc.ic_len = 0;
         strioc.ic_timout = -1;

         if(ioctl(ip_fd, I_STR, &strioc) < 0) 
            {
            syslog(LOG_WARNING, gettxt(":197", 
                  "get_if_number_for_route: SIOCSIPTRAP: %m.\n"));
            (void) close(ip_fd);
            ip_fd = -1;
            return(FALSE);
            }

         if(ip_fd >= nfds)
            nfds = ip_fd + 1;
       
         FD_SET(ip_fd, &ifds);
         }
      }

   if_all.if_number = 0;
   if_all.if_entry.if_index = index;

   strioc.ic_cmd = SIOCGIFALL;
   strioc.ic_dp = (char *)&if_all;
   strioc.ic_len = sizeof(struct ifreq_all);
   strioc.ic_timout = -1;

   if(ioctl(ip_fd, I_STR, &strioc) < 0) 
      {
      syslog(LOG_WARNING, gettxt(":198", 
            "get_if_number_for_route: SIOCGIFALL: %m.\n"));
      (void) close(ip_fd);
      ip_fd = -1;
      *if_num = -1;
      return(FALSE);
      }

   *if_num = if_all.if_number;

   return(TRUE);
   }
#endif

/* Operations to change the Route table */

static struct rtstate_s 
   {
   int flags;
   unsigned long in_ip_addr;

#ifdef NEW_MIB
   struct mib_rt_entry route;
#else
   struct rtentry route;
#endif
  
   int index;
   int type;
   unsigned long mask;
   } rtstate;

#define GOTTEN    0x001
#define DELETE    0x002
#define NOTEXIST  0x004
#define IFINDEX   0x008
#define METRIC1   0x010
#define NEXTHOP   0x020
#define TYPE      0x040
#define AGE       0x080
#define MASK      0x100

#define clear(x)  bzero((char *)&(x), sizeof(x))

int var_ip_route_test(OID var_name_ptr, OID in_name_ptr, unsigned int arg, 
                     ObjectSyntax *value)
   {
   int i;
   int cc;
   int if_num;
   unsigned long ip_addr;
   unsigned long rt_ip_addr;
   time_t curtime;

   if(in_name_ptr->length != var_name_ptr->length + 4) 
      {
      goto bad;
      }

   ip_addr = 0;

   for(i = var_name_ptr->length; i < in_name_ptr->length; i++)
      ip_addr = (ip_addr << 8) + in_name_ptr->oid_ptr[i];

   if((rtstate.flags & GOTTEN) && ip_addr != rtstate.in_ip_addr) 
      {
      goto bad;
      }

   if(!(rtstate.flags & GOTTEN)) 
      {
#ifdef NEW_MIB
      cc = get_route_entry(ip_addr, 4, &rtstate.route);
#else
      cc = get_route_entry(nl[N_RTHOST].n_value, nl[N_RTNET].n_value,
                           nl[N_RTHASHSIZE].n_value, ip_addr, 4,
                           &rtstate.route);
#endif

#if defined(BSD) || defined(SVR4) || defined(TCP40)
      rt_ip_addr = ntohl(((struct sockaddr_in *)(&rtstate.route.rt_dst))->sin_addr.s_addr);
#endif
#if defined(SVR3) && !defined(TCP40)
      rt_ip_addr = ntohl(rtstate.route.rt_dst.s_addr);
#endif

      if(cc == FALSE || rt_ip_addr != ip_addr) 
         {
         rtstate.flags |= NOTEXIST;
         clear(rtstate.route);
#if defined(BSD) || defined(SVR4) || defined(TCP40)
         ((struct sockaddr_in *)(&rtstate.route.rt_dst))->sin_addr.s_addr = htonl(ip_addr);
#endif
#if defined(SVR3) && !defined(TCP40)
         rtstate.route.rt_dst.s_addr = htonl(ip_addr);
#endif

         } 
      else 
         {

#ifdef BSD
         cc = get_if_number_for_route(nl[N_IFNET].n_value, &rtstate.index,
                                       rtstate.route.rt_ifp);
#endif

#if(defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
         cc = get_if_number_for_route(nl[N_PROVIDER].n_value, &rtstate.index,
                                       rtstate.route.rt_ifp);
#endif

#ifdef NEW_MIB
         cc = get_if_number_for_route(&rtstate.index, rtstate.route.rt_index);
#endif

         if(cc == FALSE)
            rtstate.index = -1;

         if((rtstate.route.rt_flags & RTF_UP) == 0)
            rtstate.type = 2;
         else if(rtstate.route.rt_flags & RTF_GATEWAY)
            rtstate.type = 4;
         else
            rtstate.type = 3;
         }

      rtstate.flags |= GOTTEN;
      rtstate.in_ip_addr = ip_addr;
      }

   switch(arg) 
      {
      case 1:
         if(value->type == NULL_TYPE) 
            {
            rtstate.flags |= DELETE;
            break;
            }

#if defined(BSD) || defined(SVR4) || defined(TCP40)
         if(value->os_value->length != sizeof(((struct sockaddr_in *)(&rtstate.route.rt_dst))->sin_addr)) 
            {
#endif

#if defined(SVR3) && !defined(TCP40)
         if(value->os_value->length != sizeof(rtstate.route.rt_dst)) 
            {
#endif

            goto bad;
            }

#ifdef BSD
         bcopy(value->os_value->octet_ptr,
               &((struct sockaddr_in *)(&rtstate.route.rt_dst))->sin_addr,
               value->os_value->length);
#endif

#if defined(SVR3) && !defined(TCP40)
         bcopy(value->os_value->octet_ptr, &rtstate.route.rt_dst,
               value->os_value->length);
#endif

#if defined(SVR4) || defined(TCP40)
         bcopy(value->os_value->octet_ptr, 
               &((struct sockaddr_in *)(&rtstate.route.rt_dst))->sin_addr,
               value->os_value->length);
#endif

      break;

      case 2:
         if(value->type == NULL_TYPE) 
            {
            rtstate.flags |= DELETE;
            break;
            }

         rtstate.index = value->sl_value;
         rtstate.flags |= IFINDEX;
      break;

      case 3:
         if(value->type == NULL_TYPE) 
            {
            rtstate.flags |= DELETE;
            break;
            }
    
#ifdef NEW_MIB
         rtstate.route.rt_rmx.rmx_hopcount = value->sl_value;
#else
         rtstate.route.rt_metric = value->sl_value;
#endif
         rtstate.flags |= METRIC1;
      break;

      case 7:
         if(value->type == NULL_TYPE) 
            {
            rtstate.flags |= DELETE;
            break;
            }

#if defined(BSD) || defined(SVR4) || defined(TCP40)
         if(value->os_value->length != sizeof(((struct sockaddr_in *)(&rtstate.route.rt_gateway))->sin_addr)) 
            {
#endif

#if defined(SVR3) && !defined(TCP40)
         if(value->os_value->length != sizeof(rtstate.route.rt_gateway)) 
            {
#endif

            goto bad;
            }

#ifdef BSD
         bcopy(value->os_value->octet_ptr,
               &((struct sockaddr_in *)(&rtstate.route.rt_gateway))->sin_addr,
               value->os_value->length);
#endif

#if defined(SVR3) && !defined(TCP40)
         bcopy(value->os_value->octet_ptr, &rtstate.route.rt_gateway,
               value->os_value->length);
#endif

#if defined(SVR4) || defined(TCP40)
         bcopy(value->os_value->octet_ptr,
               &((struct sockaddr_in *)(&rtstate.route.rt_gateway))->sin_addr,
               value->os_value->length);
#endif

         rtstate.flags |= NEXTHOP;
      break;

      case 8:
         if(value->type == NULL_TYPE) 
            {
            rtstate.flags |= DELETE;
            break;
            }

         if(value->sl_value < 2 || value->sl_value > 4) 
            {
            goto bad;
            }

         rtstate.type = value->sl_value;

         if(rtstate.type == 2)
            rtstate.flags |= DELETE;

         rtstate.flags |= TYPE;
      break;

      case 10:
         if(value->type == NULL_TYPE) 
            {
            rtstate.flags |= DELETE;
            break;
            }

         curtime = time((long *)0);
         rtstate.route.rt_age = curtime + value->sl_value;
         rtstate.flags |= AGE;
      break;

      case 11:
         if(value->type == NULL_TYPE) 
            {
            rtstate.flags |= DELETE;
            break;
            }

         if(value->os_value->length != sizeof(rtstate.mask)) 
            {
            goto bad;
            }

         bcopy((char *)value->os_value->octet_ptr, &rtstate.mask,
               value->os_value->length);
      break;

      default:
         goto bad;
      }
   return(TRUE);

bad:
   clear(rtstate);
   return(FALSE);
   }

int var_ip_route_set(OID var_name_ptr, OID in_name_ptr, unsigned int arg, 
                     ObjectSyntax *value)
   {
   int cc;

   if(rtstate.flags == 0) 
      {
      return(TRUE);
      }
  /*
   * If doing a delete, then if the old entry did not exist and
   * the nexthop was not specified, then just return an error.
   * There is not enough information to do a delete.
   */
   if(rtstate.flags & DELETE) 
      {
      if((rtstate.flags & (NOTEXIST|NEXTHOP)) == NOTEXIST) 
         {
         clear(rtstate);
         return(FALSE);
         }
      } 
   else 
      {
      if(!(rtstate.flags & METRIC1)) 
         {
         if(rtstate.type == 3)
#ifdef NEW_MIB
            rtstate.route.rt_rmx.rmx_hopcount = 0;
#else
            rtstate.route.rt_metric = 0;
#endif
         else
#ifdef NEW_MIB
            rtstate.route.rt_rmx.rmx_hopcount = 1;
#else
            rtstate.route.rt_metric = 1;
#endif
         }

      rtstate.route.rt_proto = RTP_NETMGMT;

      if(!(rtstate.flags & AGE)) 
         {
         rtstate.route.rt_age = time((long *)0);
         }
      }

   cc = rtsettable();
   clear(rtstate);
   return(cc);
   }

rtsettable()
   {
#if defined(BSD) || defined(SVR3) || defined(SVR4)
#ifdef NEW_MIB
   struct mib_rt_entry *route;
#else
   struct rtentry *route;
#endif

#if defined(BSD) || defined(SVR4) || defined(TCP40)
   struct sockaddr_in *sin;
#endif

   int s;
   int rv;
   extern int errno;

#ifdef SVR4
   struct strioctl ioc;
#endif

#if defined(BSD) || defined(SVR3)
   if((s = socket(AF_INET, SOCK_RAW, 0)) < 0) 
      {
      clear(rtstate);
      syslog(LOG_WARNING, gettxt(":199", "rtsettable: Socket open failed: %m.\n"));
      return(FALSE);
      }
#endif

#ifdef SVR4
   if((s = open(_PATH_IP, O_RDONLY)) < 0) 
      {
      clear(rtstate);
      syslog(LOG_WARNING, gettxt(":200", "rtsettable: Open failed: %m.\n"));
      return(FALSE);
      }
#endif

   route = &rtstate.route;
#if defined(BSD) || defined(SVR4) || defined(TCP40)
   sin = (struct sockaddr_in *)&route->rt_dst;
   sin->sin_family = AF_INET;
/* #endif

#if defined(BSD) || defined(SVR4) || defined(TCP40) */
   sin = (struct sockaddr_in *)&route->rt_gateway;
   sin->sin_family = AF_INET;
#endif

   route->rt_flags = RTF_UP;

   if(rtstate.type == 4)
      route->rt_flags |= RTF_GATEWAY;

#if defined(BSD) || defined(SVR4) || defined(TCP40)
   route->rt_flags |= rtishost(s,((struct sockaddr_in *)&(route->rt_dst))->sin_addr.s_addr);
#endif

#if defined(SVR3) && !defined(TCP40)
   route->rt_flags |= rtishost(s,route->rt_dst.s_addr);
#endif

#if defined(BSD) || defined(SVR3)
   if(rv = ioctl(s,(rtstate.flags & DELETE) ? SIOCDELRT:SIOCADDRT,(caddr_t)route))
      perror("snmpd:  rtsettable");
#endif

#ifdef SVR4
   ioc.ic_cmd = (rtstate.flags & DELETE) ? SIOCDELRT : SIOCADDRT;
   ioc.ic_timout = 0;
   ioc.ic_len = sizeof(struct rtentry);
   ioc.ic_dp = (char *) route;
   if((rv = ioctl(s, I_STR, (char *) &ioc)) < 0)
      syslog(LOG_WARNING, gettxt(":201", "rtsettable: %m.\n"));
#endif

   close(s);
   clear(rtstate);
   return(rv < 0 ? FALSE : TRUE);
#endif

#if !defined(BSD) && !defined(SVR3) && !defined(SVR4)
   return(FALSE);
#endif
   }

/*
 * Return RTF_HOST if the address is
 * for an Internet host, RTF_SUBNET for a subnet,
 * 0 for a network.
 */

#define RTF_SUBNET   0x8000

int rtishost(int s, unsigned long addr)
   {
   register unsigned long i = ntohl(addr);
   register unsigned long net, host;
   struct ifconf ifc;
   struct ifreq ifreq, *ifr;
   char buf[BUFSIZ];
   unsigned long subnetmask;
   unsigned long netmask;
   unsigned long iaddr;
   unsigned long inet;
   struct sockaddr_in *sin;
   int n = 0;

#ifdef SVR4
   struct strioctl ioc;
#endif

   if(IN_CLASSA(i)) 
      {
      net = i & IN_CLASSA_NET;
      host = i & IN_CLASSA_HOST;
      } 
   else if(IN_CLASSB(i)) 
      {
      net = i & IN_CLASSB_NET;
      host = i & IN_CLASSB_HOST;
      } 
   else 
      {
      net = i & IN_CLASSC_NET;
      host = i & IN_CLASSC_HOST;
      }

   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;

#if defined(BSD) || defined(SVR3)
   if(ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) 
      {
      perror("SIOCIFCONF ");
      } 
   else 
      {
      ifr = ifc.ifc_req;
      n = ifc.ifc_len / sizeof(struct ifreq);
      }
#endif

#ifdef SVR4
   ioc.ic_cmd = SIOCGIFCONF;
   ioc.ic_timout = 0;
   ioc.ic_len = sizeof(buf);
   ioc.ic_dp = buf;
           ;
   if(ioctl(s, I_STR, (char *)&ioc) < 0) 
      {
      syslog(LOG_WARNING, gettxt(":202", "SICGIFCONF failed: %m.\n"));
      } 
   else 
      {
      n = ioc.ic_len / sizeof(ifreq);
      ifr = (struct ifreq *)buf;
      }
#endif

  /*
   * Check whether this network is subnetted;
   * if so, check whether this is a subnet or a host.
   */
   for(i = 0; i < n; i++,ifr++) 
      {
      sin = (struct sockaddr_in *)&(ifr->ifr_addr);
      inet = ntohl(sin->sin_addr.s_addr);
      netmask = inet;

      if(IN_CLASSA(netmask))
         netmask = IN_CLASSA_NET;
      else if(IN_CLASSB(netmask))
         netmask = IN_CLASSB_NET;
      else
         netmask = IN_CLASSC_NET;

      ifreq = *ifr;

#if defined(BSD) || defined(SVR3)
      if(ioctl(s, SIOCGIFNETMASK, (char *)&ifreq) < 0) 
         {
         perror("SIOCGIFNETMASK ");
         continue;
         }
#endif

#ifdef SVR4
      ioc.ic_cmd = SIOCGIFNETMASK;
      ioc.ic_timout = 0;
      ioc.ic_len = sizeof(ifreq);
      ioc.ic_dp = (char *)&ifreq;

      if(ioctl(s, I_STR, (char *)&ioc) < 0) 
         {
         syslog(LOG_WARNING, gettxt(":203", "SICGIFNETMASK failed: %m.\n"));
         continue;
         }
#endif    

      sin = (struct sockaddr_in *)&ifreq.ifr_addr;
      subnetmask = ntohl(sin->sin_addr.s_addr);

      if(net == (inet & netmask)) 
         {
         if(host &~ subnetmask)
            return(RTF_HOST);
         else if(subnetmask != netmask)
            return(RTF_SUBNET);
         else
            return(0);
         }
      }

   if(host == 0)
      return(0);
   else
      return(RTF_HOST);
   }
