#ident	"@(#)v_udpTable.c	1.3"
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
static char SNMPID[] = "@(#)v_udpTable.c	1.3";
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
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#include <sys/stropts.h>
#endif
#if !defined(SVR3) && !defined(SVR4)
#include <net/route.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/in.h>
#if defined(SVR3) || defined(SVR4)
#include <net/route.h>
#endif
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <syslog.h>
#include <unistd.h>

#include "snmp.h" 
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

VarBindList *get_next_class();

typedef struct _udpconn {
  unsigned long local_ip_addr;
  unsigned long local_port;
} UdpConn;

VarBindList
*var_udp_table_get(OID var_name_ptr,
		   OID in_name_ptr,
		   unsigned int arg,
		   VarEntry *var_next_ptr,
		   int type_search)
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int i;
  struct inpcb inpcb_entry;
  UdpConn uc;
  char buffer[80];
  char buffer2[80];
  unsigned char netaddr[4];
  unsigned long value_ip_addr;
  unsigned long value_port;
  int cc;
  OctetString *os_ptr;

  /* see if explicitly determined */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 5)))
    return(NULL);

  uc.local_ip_addr = 0;
  uc.local_port = 0;

  if (in_name_ptr->length > var_name_ptr->length) {	/* pull off ip addr */
    /* local ip addr */
    for (i=var_name_ptr->length; i < var_name_ptr->length + 4; i++) {
      if (i < in_name_ptr->length) 
	uc.local_ip_addr = (uc.local_ip_addr << 8) + in_name_ptr->oid_ptr[i];  /* first sub-field after name */
      else
	uc.local_ip_addr = (uc.local_ip_addr << 8);
    }
    if (in_name_ptr->length > var_name_ptr->length + 4)
      uc.local_port = in_name_ptr->oid_ptr[var_name_ptr->length+4];
  }

  /* bump up value by one if get-next and fully qualified */
  if ((type_search == NEXT) && (in_name_ptr->length >= var_name_ptr->length + 5))
    uc.local_port++;

#ifdef NEW_MIB
  cc = get_udp_table(&uc, &inpcb_entry);
#else
  cc = get_udp_table(nl[N_UDB].n_value, &uc, &inpcb_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr));
    if (type_search == EXACT)
      return(NULL);
  }

  /* Check that exact found right one */
  if ((type_search == EXACT) &&
      ((htonl(inpcb_entry.inp_laddr.s_addr) != uc.local_ip_addr) ||
      (htons(inpcb_entry.inp_lport) != uc.local_port)))
    return(FALSE);  /* not right entry - doesn't exist */

  sprintf(buffer2,"%d.%d.%d.%d.%d",
	  (htonl(inpcb_entry.inp_laddr.s_addr) >> 24) & 0xFF,
	  (htonl(inpcb_entry.inp_laddr.s_addr) >> 16) & 0xFF,
	  (htonl(inpcb_entry.inp_laddr.s_addr) >> 8) & 0xFF,
	  htonl(inpcb_entry.inp_laddr.s_addr) & 0xFF,
	  htons(inpcb_entry.inp_lport));

  switch (arg) {
  case 1:
    value_ip_addr = ntohl(inpcb_entry.inp_laddr.s_addr);
    sprintf(buffer,"udpLocalAddress.%s", buffer2);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    netaddr[0] = ((value_ip_addr>>24) & 0xFF);
    netaddr[1] = ((value_ip_addr>>16) & 0xFF);
    netaddr[2] = ((value_ip_addr>>8) & 0xFF);
    netaddr[3] = (value_ip_addr & 0xFF);
    os_ptr = make_octetstring(netaddr, 4);
    vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
    oid_ptr = NULL;
    os_ptr = NULL;
    break;
  case 2:
    value_port = ntohs(inpcb_entry.inp_lport);
    sprintf(buffer,"udpLocalPort.%s", buffer2);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, value_port, NULL, NULL);
    oid_ptr = NULL;
    break;
  default:			/* should never happen */
    if (type_search == EXACT)
      return(NULL);
    else
      return(get_next_class(var_next_ptr));
  };

  return(vb_ptr);
} 

#ifdef NEW_MIB
extern int udp_fd;

int
get_udp_table(uc, inpcb_entry)
UdpConn *uc;
struct inpcb *inpcb_entry;
{
    int  size;
    char *buffer, *next, *lim;
    struct gi_arg gi_arg, *gp;
    struct inpcb *temp_inp;
    struct strioctl strioc;
    UdpConn uc_best;

    uc_best.local_ip_addr = 0xffffffff;
    uc_best.local_port = 0xffffffff;

    if (udp_fd < 0) {
	if ((udp_fd = open(_PATH_UDP, O_RDWR)) < 0) {
	    syslog(LOG_WARNING, gettxt(":211", "get_udp_table: Open of %s failed: %m."),
		_PATH_UDP);
	    return (FALSE);
	}
	else {
	    strioc.ic_cmd = SIOCSMGMT;
	    strioc.ic_dp = (char *)0;
	    strioc.ic_len = 0;
	    strioc.ic_timout = -1;

	    if (ioctl(udp_fd, I_STR, &strioc) < 0) {
		syslog(LOG_WARNING, gettxt(":212", "get_udp_table: SIOCSMGMT: %m.\n"));
		(void) close(udp_fd);
		udp_fd = -1;
		return (FALSE);
	    }
	}
    }

    gi_arg.gi_size = 0;
    gi_arg.gi_where = (caddr_t)&gi_arg;

    if ((size = ioctl(udp_fd, STIOCGUDB, (char *)&gi_arg)) < 0) {
	syslog(LOG_WARNING, gettxt(":213", "get_udp_table: STIOCGUDB: %m.\n"));
	(void) close(udp_fd);
	udp_fd = -1;
	return (FALSE);
    }

    if ((buffer = (char *) malloc(sizeof(struct inpcb) * size)) == NULL) {
	syslog(LOG_WARNING, gettxt(":214", "get_udp_table: malloc: %m.\n"));
	return (FALSE);
    }

    gp = (struct gi_arg *)buffer;
    gp->gi_size = size;
    gp->gi_where = (caddr_t)buffer;

    if ((size = ioctl(udp_fd, STIOCGUDB, buffer)) < 0) {
	syslog(LOG_WARNING, gettxt(":213", "get_udp_table: STIOCGUDB: %m.\n"));
	(void) free(buffer);
	(void) close(udp_fd);
	udp_fd = -1;
	return (FALSE);
    }

    lim = buffer + size * sizeof(struct inpcb);
    for (next = buffer; next < lim; next += sizeof(struct inpcb)) {
	temp_inp = (struct inpcb *)next;

	/* check that entry is >= tc */
	if ((test_a_uc(temp_inp, uc) >= 0) && 
	    (test_a_uc(temp_inp, &uc_best) < 0)) {
	    /* update best */
	    uc_best.local_ip_addr = htonl(temp_inp->inp_laddr.s_addr);
	    uc_best.local_port = htons(temp_inp->inp_lport);

	    /* save values */
	    bcopy ((char *) temp_inp, inpcb_entry, sizeof(struct inpcb));
	}
    }

    (void) free(buffer);
    /* Check to see if one was found - signal if not */
    if (uc_best.local_port == 0xffffffff)
	return(FALSE);

    return(TRUE);			/* One was found */
}

#else
get_udp_table(off_t ucbaddr,
	      UdpConn *uc,
	      struct inpcb *inpcb_entry)
{
  int i;
  UdpConn uc_best;
  struct inpcb temp_inpcb_entry, *orig;

  uc_best.local_ip_addr = 0xffffffff;
  uc_best.local_port = 0xffffffff;

  if (ucbaddr == 0) {
    syslog(LOG_WARNING, gettxt(":215", "ucbaddr: Symbol not defined.\n"));
    return(FALSE);
  }

  lseek(kmem, ucbaddr, 0);
  if (read(kmem, &temp_inpcb_entry, sizeof(struct inpcb)) < 0) {
    syslog(LOG_WARNING, gettxt(":216", "get_udp_conn: %m.\n"));
    return(FALSE);
  }

  orig = (struct inpcb *) ucbaddr;
  /* loop */
  while (temp_inpcb_entry.inp_next != (struct inpcb *) ucbaddr) {
    lseek(kmem, (off_t)temp_inpcb_entry.inp_next, 0);
    read(kmem, &temp_inpcb_entry, sizeof(struct inpcb));

    /* check that entry is >= tc */
    if ((test_a_uc(&temp_inpcb_entry, uc) >= 0) && 
	(test_a_uc(&temp_inpcb_entry, &uc_best) < 0)) {
      /* update best */
      uc_best.local_ip_addr = htonl(temp_inpcb_entry.inp_laddr.s_addr);
      uc_best.local_port = htons(temp_inpcb_entry.inp_lport);

      /* save values */
      bcopy ((char *) &temp_inpcb_entry, inpcb_entry, sizeof(struct inpcb));
    }
  } /* end of while */

  /* Check to see if one was found - signal if not */
  if (uc_best.local_port == 0xffffffff)
    return(FALSE);

  return(TRUE);			/* One was found */
}
#endif

/* return -1 if temp < tc, 0 if ==, and 1 if > */
test_a_uc(temp_inpcb_entry, uc)
     struct inpcb *temp_inpcb_entry;
     UdpConn *uc;
{
  if (htonl(temp_inpcb_entry->inp_laddr.s_addr) < uc->local_ip_addr)
    return(-1);
  if (htonl(temp_inpcb_entry->inp_laddr.s_addr) > uc->local_ip_addr)
    return(1);
  
  /* local ip addr's are equal, continue testing */
  if (htons(temp_inpcb_entry->inp_lport) < uc->local_port)
    return(-1);
  if (htons(temp_inpcb_entry->inp_lport) > uc->local_port)
    return(1);

  /* they are equal */
  return (0);
}
