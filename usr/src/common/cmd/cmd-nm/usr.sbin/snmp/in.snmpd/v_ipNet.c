#ident	"@(#)v_ipNet.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
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
static char SNMPID[] = "@(#)v_ipNet.c	4.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 *  Revision History:
 *  1/28/89 JDC
 *  Fix bug where get arp entries returned from lo (loopback) interfaces
 *  some think it is not reasonable for loopback interface to have an arp cache
 *
 *  2/4/89 JDC
 *  amended copyright notice
 *  changed references from "gotone" to "snmpd"
 *
 *  4/24/89 JDC
 *  turned off some of the verbose debug output to cut down on the noise
 *
 *  5/17/89 KWK
 *  changed SUN3 to SUNOS35 and SUN4 to SUNOS40, since that's what they really 
 *  meant and were causing confusion
 *
 *  11/8/89 JDC
 *  Make it print pretty via tgrind
 *
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

#include <sys/types.h>
#include <sys/socket.h>
#if !defined(SVR3) && !defined(SVR4)
#include <sys/mbuf.h>
#endif
#include <net/if.h>
#include <net/af.h>
#include <netinet/in.h>
#ifndef SUNOS35
#include <netinet/in_var.h>
#endif
#if defined(SVR3) || defined(SVR4)
#include <net/if_arp.h>
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#include <sys/stropts.h>
#include <net/route.h>
#include <netinet/ip_str.h>
#endif
#include <netinet/if_ether.h>
#ifdef SVR4
#include <sys/sockio.h>
#endif
#ifdef BSD
#include <sys/ioctl.h>
#endif

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

VarBindList *get_next_class();

VarBindList *
var_ip_net_to_media_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     unsigned int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  OctetString *os_ptr;
  int i;
  unsigned long ip_addr;
  int ip_addr_len;
  struct arptab arptab_entry;
  int if_num = 0;
  int cc;
  char ifname[16];
  int interface, in_interface;
  char buffer[256];
  unsigned char netaddr[4];
  unsigned long new_ip_addr;
  int type;
  struct ifreq_all if_all;

  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 5)))
    return(NULL);

  /* determine interface */
  interface = 1;
  if (in_name_ptr->length > var_name_ptr->length)
    interface = in_name_ptr->oid_ptr[var_name_ptr->length];  /* first sub-field after name */

  /* check that the interface exists */
  /*  
   *  bug fix for arp table entries on loopback interface
   *
   *  changed 255 to interface in the line below
   *  1/28/89 JDC
   */
  in_interface = interface;
again: 
  if_num = interface;
  /* end of fix 1/28/89 JDC */

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  if (interface > if_num) {
    if (type_search == EXACT)
      return(NULL);
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr));
  }
  /*
   *  fix bug where get arp table entries for the loopback interface
   *  JDC 1/28/89
   */ 
#ifdef BSD
  if (strcmp(ifname, "lo") == 0) {
#endif
#if defined(SVR3) || defined(SVR4)
  if (if_all.if_entry.if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) {
#endif
    if (type_search == EXACT)
      return(NULL);
    if (type_search == NEXT) {
      interface++;
      goto again;
    }
  }
   /*
    *  end bug fix
    */

  /* determine ip address */
  ip_addr = 0;
  if (in_name_ptr->length > var_name_ptr->length + 1 && interface == in_interface) {
    for (i=var_name_ptr->length + 1; i < var_name_ptr->length + 5; i++) {
      if (i < in_name_ptr->length)
	ip_addr = (ip_addr << 8) + in_name_ptr->oid_ptr[i];  /* first sub-field after name */
      else
	ip_addr = ip_addr << 8;
    }
  }

  if (type_search == NEXT)
    ip_addr++;

  ip_addr_len = 4;

#ifdef NEW_MIB
  cc = get_arp_entry(ip_addr, ip_addr_len, &arptab_entry);
#else
  cc = get_arp_entry(nl[N_ARPTAB].n_value, nl[N_ARPTAB_SIZE].n_value, 
		     ip_addr, ip_addr_len, &arptab_entry);
#endif

  /* If past end of arp table and exact, we fail */
  if ((cc == FALSE) && (type_search == EXACT))
    return(NULL);

  /* if failed (and type=NEXT implied */
  if (cc == FALSE) {
    interface++;
    goto again;
  }

  new_ip_addr = ntohl(arptab_entry.at_iaddr.s_addr);

  if ((type_search == EXACT) && (ip_addr != new_ip_addr))
    return(NULL);

  switch (arg) {
  case 1:
    sprintf(buffer,"ipNetToMediaIfIndex.%d.%d.%d.%d.%d", interface,
	    ((new_ip_addr>>24) & 0xff), ((new_ip_addr>>16) & 0xff),
	    ((new_ip_addr>>8) & 0xff), (new_ip_addr & 0xff));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, interface, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 2:
    sprintf(buffer,"ipNetToMediaPhysAddress.%d.%d.%d.%d.%d", interface,
	    ((new_ip_addr>>24) & 0xff), ((new_ip_addr>>16) & 0xff),
	    ((new_ip_addr>>8) & 0xff), (new_ip_addr & 0xff));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#if defined(SUNOS35) || defined(SUNOS40)
    os_ptr = make_octetstring(&arptab_entry.at_enaddr, 6);
#else
    os_ptr = make_octetstring(arptab_entry.at_enaddr, 6);
#endif
    vb_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE, 0, 0, os_ptr, NULL);
    os_ptr = NULL;
    oid_ptr = NULL;
    break;
  case 3:
    sprintf(buffer,"ipNetToMediaNetAddress.%d.%d.%d.%d.%d", interface,
	    ((new_ip_addr>>24) & 0xff), ((new_ip_addr>>16) & 0xff),
	    ((new_ip_addr>>8) & 0xff), (new_ip_addr & 0xff));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    netaddr[0] = ((new_ip_addr >> 24) & 0xff);
    netaddr[1] = ((new_ip_addr >> 16) & 0xff);
    netaddr[2] = ((new_ip_addr >> 8) & 0xff);
    netaddr[3] = (new_ip_addr & 0xff);
    os_ptr = make_octetstring(netaddr, 4);
    vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
    os_ptr = NULL;
    oid_ptr = NULL;
    break;
  case 4:
    sprintf(buffer, "ipNetToMediaType.%d.%d.%d.%d.%d", interface,
	    ((new_ip_addr>>24) & 0xff), ((new_ip_addr>>16) & 0xff),
	    ((new_ip_addr>>8) & 0xff), (new_ip_addr & 0xff));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    if (!(arptab_entry.at_flags & ATF_COM))
      type = 2;
    else if (arptab_entry.at_flags & ATF_PERM)
      type = 4;
    else
      type = 3;
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, type, NULL, NULL);
    oid_ptr = NULL;
    break;
  default:
    if (type_search == EXACT)
      return(NULL);
    else
      return(get_next_class(var_next_ptr));
  };

  return(vb_ptr);
}

/* Operations to change the ARP table */

static struct nmstate_s {
  int flags;
  int in_index;
  unsigned long in_ip_addr;
  int set_index;
  struct arptab arp;
  int type;
} nmstate;

#define GOTTEN		0x1
#define DELETE		0x2
#define NOTEXIST	0x4
#define PHYSADDR	0x8

#define clear(x)	bzero((char *)&(x), sizeof(x))

int var_ip_net_to_media_test(var_name_ptr, in_name_ptr, arg, value)
     OID var_name_ptr;
     OID in_name_ptr;
     unsigned int arg;
     ObjectSyntax *value;
{
  int interface;
  int i, cc;
  unsigned long ip_addr;
  int ip_addr_len;

  if (in_name_ptr->length != var_name_ptr->length + 5) {
    goto bad;
  }

  interface = in_name_ptr->oid_ptr[var_name_ptr->length];

  if (arpcheckinterface(interface) == FALSE) {
    goto bad;
  }

  if ((nmstate.flags & GOTTEN) && nmstate.in_index != interface) {
    goto bad;
  }

  nmstate.in_index = interface;

  ip_addr = 0;
  for (i=var_name_ptr->length + 1; i < var_name_ptr->length + 5; i++) {
    ip_addr = (ip_addr << 8) + in_name_ptr->oid_ptr[i];
  }
  ip_addr_len = 4;

  if ((nmstate.flags & GOTTEN) && ip_addr != nmstate.in_ip_addr) {
    goto bad;
  }

  if (!(nmstate.flags & GOTTEN)) {
#ifdef NEW_MIB
    cc = get_arp_entry(ip_addr, ip_addr_len, &nmstate.arp);
#else
    cc = get_arp_entry(nl[N_ARPTAB].n_value, nl[N_ARPTAB_SIZE].n_value,
		       ip_addr, ip_addr_len, &nmstate.arp);
#endif
    if (cc == FALSE ||
        ntohl(nmstate.arp.at_iaddr.s_addr) != ip_addr) {
      nmstate.flags |= NOTEXIST;
      clear(nmstate.arp);
      nmstate.arp.at_iaddr.s_addr = htonl(ip_addr);
    }
    nmstate.flags |= GOTTEN;
    nmstate.in_ip_addr = ip_addr;
  }

  switch (arg) {
  case 1:
    if (value->type == NULL_TYPE) {
      nmstate.flags |= DELETE;
      break;
    }
    if (arpcheckinterface(value->sl_value) == FALSE) {
      goto bad;
    }
    nmstate.set_index = value->sl_value;
    break;
  case 2:
    if (value->type == NULL_TYPE) {
      nmstate.flags |= DELETE;
      break;
    }
    if (value->os_value->length != sizeof(nmstate.arp.at_enaddr)) {
      goto bad;
    }
    bcopy((char *)value->os_value->octet_ptr, nmstate.arp.at_enaddr,
      value->os_value->length);
    nmstate.flags |= PHYSADDR;
    break;
  case 3:
    if (value->type == NULL_TYPE) {
      nmstate.flags |= DELETE;
      break;
    }
    if (value->os_value->length != sizeof(nmstate.arp.at_iaddr)) {
      goto bad;
    }
    bcopy((char *)value->os_value->octet_ptr, &nmstate.arp.at_iaddr,
          value->os_value->length);
    break;
  case 4:
    if (value->type == NULL_TYPE) {
      nmstate.flags |= DELETE;
      break;
    }
    if (value->sl_value < 2 || value->sl_value > 4) {
      goto bad;
    }
    nmstate.type = value->sl_value;
    if (nmstate.type == 2)
      nmstate.flags |= DELETE;
    break;
  default:
    goto bad;
  }
  return(TRUE);

bad:
  clear(nmstate);
  return(FALSE);
}

int var_ip_net_to_media_set(var_name_ptr, in_name_ptr, arg, value)
     OID var_name_ptr;
     OID in_name_ptr;
     unsigned int arg;
     ObjectSyntax *value;
{
  int cc;

  if (nmstate.flags == 0) {
    return(TRUE);
  }
  if (!(nmstate.flags & DELETE)) {
    if ((nmstate.flags & (NOTEXIST|PHYSADDR)) == NOTEXIST) {
      clear(nmstate);
      return(FALSE);
    }
  }

  cc = nmsettable();

  clear(nmstate);

  return(cc);
}

nmsettable()
{
#ifdef BSD
  struct arpreq ar;
  struct sockaddr_in *sin;
  int s;
  int ret;

  bzero((caddr_t)&ar, sizeof(struct arpreq));
  sin = (struct sockaddr_in *)&ar.arp_pa;
  sin->sin_family = AF_INET;
  bcopy((caddr_t)nmstate.arp.at_iaddr, (caddr_t)&sin->sin_addr,
        sizeof(sin->sin_addr));
  ar.arp_ha.sa_family = AF_UNSPEC;
  bcopy((caddr_t)nmstate.arp.at_enaddr, (caddr_t)ar.arp_ha.sa_data,
        sizeof(nmstate.arp.at_enaddr));
  ar.arp_flags = (nmstate.type == 4) ? ATF_PERM : 0;

  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    clear(nmstate);
    return(FALSE);
  }
  ret = ioctl(s, (nmstate.flags & DELETE) ? SIOCDARP : SIOCSARP, (caddr_t)&ar);
  close(s);
  clear(nmstate);
  if (ret < 0) {
    return(FALSE);
  }
  return(TRUE);
#endif
#if defined(SVR3) || defined(SVR4)
  struct strioctl strioc;
  struct sockaddr_in *sin;
  struct arpreq ar;
  int fd;
  int ret;

  bzero((caddr_t)&ar, sizeof(struct arpreq));
  sin = (struct sockaddr_in *)&ar.arp_pa;
  sin->sin_family = AF_INET;
  bcopy((caddr_t)&nmstate.arp.at_iaddr, (caddr_t)&sin->sin_addr,
         sizeof(sin->sin_addr));
  ar.arp_ha.sa_family = AF_UNSPEC;
  bcopy((caddr_t)nmstate.arp.at_enaddr, (caddr_t)ar.arp_ha.sa_data,
         sizeof(nmstate.arp.at_enaddr));
  ar.arp_flags = (nmstate.type == 4) ? ATF_PERM : 0;

  if ((fd = open(_PATH_ARP, 0)) < 0) {
    clear(nmstate);
    return(FALSE);
  }
  strioc.ic_cmd = (nmstate.flags & DELETE) ? SIOCDARP : SIOCSARP;
  strioc.ic_timout = -1;
  strioc.ic_len = sizeof(struct arpreq);
  strioc.ic_dp = (caddr_t)&ar;
  ret = ioctl(fd, I_STR, (caddr_t) & strioc);
  close(fd);
  clear(nmstate);
  if (ret < 0) {
    return(FALSE);
  }
  return(TRUE);
#endif
#if !defined(BSD) && !defined(SVR3) && !defined(SVR4)
  clear(nmstate);
  return(FALSE);
#endif
}
