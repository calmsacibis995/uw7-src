#ident	"@(#)v_tcpConn.c	1.3"
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
static char SNMPID[] = "@(#)v_tcpConn.c	1.3";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 * Revision History:
 *  2/4/89 JDC
 *  amended copyright notice
 *  revised references from "gotone" to "snmpd"
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
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_var.h>
#include <syslog.h>
#include <unistd.h>

#include "snmp.h" /*"../lib/snmp.h"*/
#include "snmpuser.h" /*"../lib/snmpuser.h"*/
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

int map_states[] = {1, 2, 3, 4, 5, 8, 6, 10, 9, 7, 11};

VarBindList *get_next_class();

typedef struct _snmp_tcpconn {
  unsigned long local_ip_addr;
  unsigned long local_port;
  unsigned long rmt_ip_addr;
  unsigned long rmt_port;
} SnmpTcpConn;

VarBindList
*var_tcp_conn_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     unsigned int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int i;
  struct tcpcb tcpcb_entry;
  struct inpcb inpcb_entry;
  SnmpTcpConn tc;
  char buffer[80];
  char buffer2[80];
  unsigned char netaddr[4];
  unsigned long value_ip_addr;
  unsigned long value_port;
  int cc, counter;
  OctetString *os_ptr;

  /* see if explicitly determined */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 10)))
    return(NULL);

  tc.local_ip_addr = 0;
  tc.local_port = 0;
  tc.rmt_ip_addr = 0;
  tc.rmt_port = 0;

  if (in_name_ptr->length > var_name_ptr->length) {	/* pull off first ip addr */
    /* local ip addr */
    for (i=var_name_ptr->length; i < var_name_ptr->length + 4; i++) {
      if (i < in_name_ptr->length) 
	tc.local_ip_addr = (tc.local_ip_addr << 8) + in_name_ptr->oid_ptr[i];  /* first sub-field after name */
      else
	tc.local_ip_addr = (tc.local_ip_addr << 8);
    }
    if (in_name_ptr->length > var_name_ptr->length + 4)
      tc.local_port = in_name_ptr->oid_ptr[var_name_ptr->length+4];

    for (i=var_name_ptr->length+5; i < var_name_ptr->length + 9; i++) {
      if (i < in_name_ptr->length) 
	tc.rmt_ip_addr = (tc.rmt_ip_addr << 8) + in_name_ptr->oid_ptr[i];  /* first sub-field after name */
      else
	tc.rmt_ip_addr = (tc.rmt_ip_addr << 8);
    }
    if (in_name_ptr->length > var_name_ptr->length + 9)
      tc.rmt_port = in_name_ptr->oid_ptr[var_name_ptr->length+9];
  }

  /* bump up value by one if get-next and fully qualified */
  if ((type_search == NEXT) && (in_name_ptr->length >= var_name_ptr->length + 10))
    tc.rmt_port++;

#ifdef BOGUS
  /* debug */
  printf("lip: %x, lp: %x, rip: %x, rp: %x\n", tc.local_ip_addr,
	 tc.local_port, tc.rmt_ip_addr, tc.rmt_port);
#endif

#ifdef NEW_MIB
  counter = 0;
  cc = get_tcp_conn(&counter, &tc, &tcpcb_entry, &inpcb_entry);
#else
  cc = get_tcp_conn(nl[N_TCB].n_value, &tc, &tcpcb_entry, &inpcb_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr));
    if (type_search == EXACT)
      return(NULL);
  }

  /* Check that exact found right one */
  if ((type_search == EXACT) &&
      ((htonl(inpcb_entry.inp_laddr.s_addr) != tc.local_ip_addr) ||
      (htons(inpcb_entry.inp_lport) != tc.local_port) ||
      (htonl(inpcb_entry.inp_faddr.s_addr) != tc.rmt_ip_addr) ||
      (htons(inpcb_entry.inp_fport) != tc.rmt_port)))
    return(FALSE);  /* not right entry - doesn't exist */

  sprintf(buffer2,"%d.%d.%d.%d.%d.%d.%d.%d.%d.%d",
	  (htonl(inpcb_entry.inp_laddr.s_addr) >> 24) & 0xFF,
	  (htonl(inpcb_entry.inp_laddr.s_addr) >> 16) & 0xFF,
	  (htonl(inpcb_entry.inp_laddr.s_addr) >> 8) & 0xFF,
	  htonl(inpcb_entry.inp_laddr.s_addr) & 0xFF,
	  htons(inpcb_entry.inp_lport),
	  (htonl(inpcb_entry.inp_faddr.s_addr) >> 24) & 0xFF,
	  (htonl(inpcb_entry.inp_faddr.s_addr) >> 16) & 0xFF,
	  (htonl(inpcb_entry.inp_faddr.s_addr) >> 8) & 0xFF,
	  htonl(inpcb_entry.inp_faddr.s_addr) & 0xFF,
	  htons(inpcb_entry.inp_fport));

  switch (arg) {
  case 1:
    sprintf(buffer,"tcpConnState.%s", buffer2);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, 
			  map_states[tcpcb_entry.t_state], NULL, NULL);
    oid_ptr = NULL;
    break;
  case 2:
    value_ip_addr = ntohl(inpcb_entry.inp_laddr.s_addr);
    sprintf(buffer,"tcpConnLocalAddress.%s", buffer2);
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
  case 3:
    value_port = ntohs(inpcb_entry.inp_lport);
    sprintf(buffer,"tcpConnLocalPort.%s", buffer2);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, value_port, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 4:
    value_ip_addr = ntohl(inpcb_entry.inp_faddr.s_addr);
    sprintf(buffer,"tcpConnRemAddress.%s", buffer2);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    netaddr[0] = ((value_ip_addr>>24) & 0xFF);
    netaddr[1] = ((value_ip_addr>>16) & 0xFF);
    netaddr[2] = ((value_ip_addr>>8) & 0xFF);
    netaddr[3] = (value_ip_addr & 0xFF);
    os_ptr = make_octetstring(netaddr, 4);
    vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
    oid_ptr = NULL;
    break;
  case 5:
    value_port = ntohs(inpcb_entry.inp_fport);
    sprintf(buffer,"tcpConnRemPort.%s", buffer2);
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
extern int tcp_fd;

int
get_tcp_conn(count, tc, tcpcb_entry, inpcb_entry)
int  *count;
SnmpTcpConn *tc;
struct tcpcb *tcpcb_entry;
struct inpcb *inpcb_entry;
{
    int  tcb_entry_size, size;
    char *buffer, *next, *lim;
    struct gi_arg gi_arg, *gp;
    struct inpcb *temp_inp;
    struct tcpcb *temp_tcp;
    struct strioctl strioc;
    SnmpTcpConn tc_best;
    int  connects = 0;

    tc_best.local_ip_addr = 0xffffffff;
    tc_best.local_port = 0xffffffff;
    tc_best.rmt_ip_addr = 0xffffffff;
    tc_best.rmt_port = 0xffffffff;

    if (tcp_fd < 0) {
	if ((tcp_fd = open (_PATH_TCP, O_RDONLY)) < 0) {
	    syslog(LOG_WARNING, gettxt(":204", "get_tcp_conn: Open of %s failed: %m.\n"),
		_PATH_TCP);
	    return (FALSE);
	}
	else {
	    strioc.ic_cmd = SIOCSMGMT;
	    strioc.ic_dp = (char *)0;
	    strioc.ic_len = 0;
	    strioc.ic_timout = -1;

	    if (ioctl(tcp_fd, I_STR, &strioc) < 0) {
		syslog(LOG_WARNING, gettxt(":205", "get_tcp_conn: ioctl SIOCSMGMT failed: %m.\n"));
		(void) close(tcp_fd);
		tcp_fd = -1;
		return (FALSE);
	    }
	}
    }

    gi_arg.gi_size = 0;
    gi_arg.gi_where = (caddr_t)&gi_arg;

    if ((size = ioctl(tcp_fd, STIOCGTCB, (char *)&gi_arg)) < 0) {
	syslog(LOG_WARNING, gettxt(":206", "get_tcp_conn: STIOCGTCB: %m.\n"));
	(void) close(tcp_fd);
	tcp_fd = -1;
	return (FALSE);
    }

    tcb_entry_size = sizeof(struct inpcb) + sizeof(struct tcpcb);
    if ((buffer = (char *) malloc(size * tcb_entry_size)) == NULL) {
	syslog(LOG_WARNING, gettxt(":207", "get_tcp_conn: malloc: %m.\n"));
	return (FALSE);
    }

    gp = (struct gi_arg *)buffer;
    gp->gi_size = size;
    gp->gi_where = (caddr_t)buffer;

    if ((size = ioctl(tcp_fd, STIOCGTCB, buffer)) < 0) {
	syslog(LOG_WARNING, gettxt(":206", "get_tcp_conn: STIOCGTCB: %m.\n"));
	(void) close(tcp_fd);
	tcp_fd = -1;
	(void) free(buffer);
	return (FALSE);
    }

    lim = buffer + size * tcb_entry_size;
    for (next = buffer; next < lim; next += tcb_entry_size) {
	temp_inp = (struct inpcb *)next;
	temp_tcp = (struct tcpcb *)(next + sizeof(struct inpcb));

	/* Just counting connections */
	if (((temp_tcp->t_state == TCPS_ESTABLISHED) || 
	    (temp_tcp->t_state == TCPS_CLOSE_WAIT)) &&
	    (*count)) {

	    connects++;
	    continue;
	}

	/* check that entry is >= tc */
	if ((test_a_tc(temp_inp, tc) >= 0) && 
	    (test_a_tc(temp_inp, &tc_best) < 0)) {

	    /* update best */
	    tc_best.local_ip_addr = htonl(temp_inp->inp_laddr.s_addr);
	    tc_best.local_port = htons(temp_inp->inp_lport);
	    tc_best.rmt_ip_addr = htonl(temp_inp->inp_faddr.s_addr);
	    tc_best.rmt_port = htons(temp_inp->inp_fport);

	    /* save values */
	    if (inpcb_entry)
		    bcopy ((char *)temp_inp, inpcb_entry, sizeof(struct inpcb));
	    if (tcpcb_entry)
		    bcopy ((char *)temp_tcp, tcpcb_entry, sizeof(struct tcpcb));
	}
    }

    (void) free(buffer);
    if (*count == 1) {
	*count = connects;
	return(TRUE);
    }

    /* Check to see if one was found - signal if not */
    if (tc_best.rmt_port == 0xffffffff)
	return(FALSE);

    return(TRUE);			/* One was found */

}

#else
get_tcp_conn(tcbaddr, tc, tcpcb_entry, inpcb_entry)
     off_t tcbaddr;
     SnmpTcpConn *tc;
     struct tcpcb *tcpcb_entry;
     struct inpcb *inpcb_entry;
{
  int i;
  SnmpTcpConn tc_best;
  struct inpcb temp_inpcb_entry, *orig;

  tc_best.local_ip_addr = 0xffffffff;
  tc_best.local_port = 0xffffffff;
  tc_best.rmt_ip_addr = 0xffffffff;
  tc_best.rmt_port = 0xffffffff;

  if (tcbaddr == 0) {
    syslog(LOG_WARNING, gettxt(":208", "tcbaddr: Symbol not defined.\n"));
    return(FALSE);
  }

  lseek(kmem, tcbaddr, 0);
  if (read(kmem, &temp_inpcb_entry, sizeof(struct inpcb)) < 0) {
    syslog(LOG_WARNING, gettxt(":208", "get_tcp_conn: %m.\n"));
    return(FALSE);
  }

#ifdef BOGUS
/* debug */
printf("FOO-head: l: %x, lp: %x, f: %x, fp: %d\n", 
       htonl(temp_inpcb_entry.inp_laddr.s_addr),
       htons(temp_inpcb_entry.inp_lport),
       htonl(temp_inpcb_entry.inp_faddr.s_addr),
       htons(temp_inpcb_entry.inp_fport));
#endif

  orig = (struct inpcb *) tcbaddr;
  /* loop */
  while (temp_inpcb_entry.inp_next != (struct inpcb *) tcbaddr) {
    lseek(kmem, (off_t)temp_inpcb_entry.inp_next, 0);
    read(kmem, &temp_inpcb_entry, sizeof(struct inpcb));

#ifdef BOGUS
/* for debug */
printf("BAH-circle: l: %x, lp: %x, f: %x, fp: %d\n", 
       htonl(temp_inpcb_entry.inp_laddr.s_addr),
       htons(temp_inpcb_entry.inp_lport),
       htonl(temp_inpcb_entry.inp_faddr.s_addr),
       htons(temp_inpcb_entry.inp_fport));
#endif

    /* check that entry is >= tc */
    if ((test_a_tc(&temp_inpcb_entry, tc) >= 0) && 
	(test_a_tc(&temp_inpcb_entry, &tc_best) < 0)) {
      /* update best */
      tc_best.local_ip_addr = htonl(temp_inpcb_entry.inp_laddr.s_addr);
      tc_best.local_port = htons(temp_inpcb_entry.inp_lport);
      tc_best.rmt_ip_addr = htonl(temp_inpcb_entry.inp_faddr.s_addr);
      tc_best.rmt_port = htons(temp_inpcb_entry.inp_fport);

      /* save values */
      bcopy ((char *) &temp_inpcb_entry, inpcb_entry, sizeof(struct inpcb));
      lseek(kmem, (off_t)temp_inpcb_entry.inp_ppcb, 0);
      read(kmem, tcpcb_entry, sizeof (struct tcpcb)); /* get tcp stat */
    }
  } /* end of while */

  /* Check to see if one was found - signal if not */
  if (tc_best.rmt_port == 0xffffffff)
    return(FALSE);

  return(TRUE);			/* One was found */
}
#endif

/* return -1 if temp < tc, 0 if ==, and 1 if > */
test_a_tc(temp_inpcb_entry, tc)
     struct inpcb *temp_inpcb_entry;
     SnmpTcpConn *tc;
{
  if (htonl(temp_inpcb_entry->inp_laddr.s_addr) < tc->local_ip_addr)
    return(-1);
  if (htonl(temp_inpcb_entry->inp_laddr.s_addr) > tc->local_ip_addr)
    return(1);
  
  /* local ip addr's are equal, continue testing */
  if (htons(temp_inpcb_entry->inp_lport) < tc->local_port)
    return(-1);
  if (htons(temp_inpcb_entry->inp_lport) > tc->local_port)
    return(1);

  /* local ip and port are equal, cont. */
  if (htonl(temp_inpcb_entry->inp_faddr.s_addr) < tc->rmt_ip_addr)
    return(-1);
  if (htonl(temp_inpcb_entry->inp_faddr.s_addr) > tc->rmt_ip_addr)
    return(1);
  
  /* local ip addr's, local port, and for. addr. are equal, continue testing */
  if (htons(temp_inpcb_entry->inp_fport) < tc->rmt_port)
    return(-1);
  if (htons(temp_inpcb_entry->inp_fport) > tc->rmt_port)
    return(1);

  /* they are equal */
  return (0);
}

#ifdef NEW_MIB
count_curr_tcp_conn_estab()
{
    int  cc, size;

    size = 1;

    cc = get_tcp_conn(&size, NULL, NULL, NULL);
    if (cc == FALSE) {
	return(0);
    }

    return(size);

}
#else
count_curr_tcp_conn_estab(tcbaddr)
     off_t tcbaddr;
{
  int i;
  struct inpcb temp_inpcb_entry, *orig;
  struct tcpcb temp_tcpcb_entry;
  int counter;

  counter = 0;

  if (tcbaddr == 0) {
    syslog(LOG_WARNING, gettxt(":208", "tcbaddr: Symbol not defined.\n"));
    return(FALSE);
  }

  lseek(kmem, tcbaddr, 0);
  if (read(kmem, &temp_inpcb_entry, sizeof(struct inpcb)) < 0) {
    syslog(LOG_WARNING, gettxt(":210", "count_curr_tcp_conn_estab: %m.\n"));
    return(FALSE);
  }

  orig = (struct inpcb *) tcbaddr;
  /* loop */
  while (temp_inpcb_entry.inp_next != (struct inpcb *) tcbaddr) {
    lseek(kmem, (off_t)temp_inpcb_entry.inp_next, 0);
    read(kmem, &temp_inpcb_entry, sizeof(struct inpcb));

    lseek(kmem, (off_t)temp_inpcb_entry.inp_ppcb, 0);
    read(kmem, &temp_tcpcb_entry, sizeof (struct tcpcb)); /* get tcp stat */
  
    if ((temp_tcpcb_entry.t_state == TCPS_ESTABLISHED) || 
	(temp_tcpcb_entry.t_state == TCPS_CLOSE_WAIT))
      counter++;
  } /* end of while */

  return(counter);
}
#endif  /* not NEW_MIB */
