#ident	"@(#)v_tcp.c	1.2"
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
static char SNMPID[] = "@(#)v_tcp.c	4.3 INTERACTIVE SNMP source";
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
#include <sys/stropts.h>
#include <sys/stream.h>
#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
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
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_var.h>
#include <syslog.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

VarBindList *get_next_class();

VarBindList
*var_tcp_get(OID var_name_ptr,
	     OID in_name_ptr,
	     unsigned int arg,
	     VarEntry *var_next_ptr,
	     int type_search)
{
  VarBindList *vb_ptr;
  OID oid_ptr;
#ifdef NEW_MIB
  struct tcp_stuff tcp_stuff;
#endif
  struct tcpstat tcpstat;
  int total;
  int i;
  int cc;
  int algo;
  unsigned long u_value;
  long s_value;

  /* see if explicitly determined */
  if ((type_search == EXACT) &&
      ((in_name_ptr->length != (var_name_ptr->length + 1)) ||
       (in_name_ptr->oid_ptr[var_name_ptr->length] != 0)))
    return(NULL);

  if ((type_search == NEXT) && (cmp_oid_class(in_name_ptr, var_name_ptr) == 0) && (in_name_ptr->length >= (var_name_ptr->length + 1)))
    return(get_next_class(var_next_ptr));
  
#ifdef NEW_MIB
  cc = get_tcp_stat(&tcp_stuff);

  bcopy ((char *)&tcp_stuff.tcp_stat, (char *)&tcpstat,
	sizeof(struct tcpstat));
#else
  cc = get_tcp_stat(nl[N_TCPSTAT].n_value, &tcpstat);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  switch (arg) {
  case 1:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpRtoAlgorithm.0");
#ifdef NEW_MIB
    algo = tcp_stuff.tcp_rto_algorithm; 
#else
    algo = 4; 
#endif	/* NEW_MIB */
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, algo, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 2:
#ifdef NEW_MIB
    total = tcp_stuff.tcp_min_rto; 
#else
    if (nl[N_TCPMINREXMT].n_value == 0) {
      syslog(LOG_WARNING, "tcp_minrexmttimeout: symbol not defined");
      if (type_search == NEXT)
        return(get_next_class(var_next_ptr)); /* get next variable */
      if (type_search == EXACT)
        return(NULL);		/* Signal failure */
    }
    lseek(kmem, nl[N_TCPMINREXMT].n_value, 0);
    if (read(kmem, (char *) &total, sizeof(total)) < 0) {
      syslog(LOG_WARNING, "tcp_minrexmttimeout: %m");
      if (type_search == NEXT)
        return(get_next_class(var_next_ptr)); /* get next variable */
      if (type_search == EXACT)
        return(NULL);		/* Signal failure */
    }
#endif
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpRtoMin.0");
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, total, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 3:
#ifdef NEW_MIB
    total = tcp_stuff.tcp_max_rto; 
#else
    if (nl[N_TCPMAXREXMT].n_value == 0) {
      syslog(LOG_WARNING, "tcp_maxrexmttimeout: symbol not defined");
      if (type_search == NEXT)
        return(get_next_class(var_next_ptr)); /* get next variable */
      if (type_search == EXACT)
        return(NULL);		/* Signal failure */
    }
    lseek(kmem, nl[N_TCPMAXREXMT].n_value, 0);
    if (read(kmem, (char *) &total, sizeof(total)) < 0) {
      syslog(LOG_WARNING, "tcp_maxrexmttimeout: %m");
      if (type_search == NEXT)
        return(get_next_class(var_next_ptr)); /* get next variable */
      if (type_search == EXACT)
        return(NULL);		/* Signal failure */
    }
#endif
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpRtoMax.0");
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, total, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 4:
#ifdef BSD
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpMaxConn.0");
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, -1, NULL, NULL);
#endif
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
    if (nl[N_NTCP].n_value == 0) {
      syslog(LOG_WARNING, "ntcp: symbol not defined");
      if (type_search == NEXT)
        return(get_next_class(var_next_ptr)); /* get next variable */
      if (type_search == EXACT)
        return(NULL);		/* Signal failure */
    }
    lseek(kmem, nl[N_NTCP].n_value, 0);
    if (read(kmem, (char *) &total, sizeof(total)) < 0) {
      syslog(LOG_WARNING, "ntcp: %m");
      if (type_search == NEXT)
        return(get_next_class(var_next_ptr)); /* get next variable */
      if (type_search == EXACT)
        return(NULL);		/* Signal failure */
    }
    total = ((total + 7) / 8) * 8;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpMaxConn.0");
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, total, NULL, NULL);
#endif
#ifdef NEW_MIB
    total = tcp_stuff.tcp_max_conn; 
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpMaxConn.0");
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, total, NULL, NULL);
#endif
    oid_ptr = NULL;
    break;
  case 5:
    u_value = tcpstat.tcps_connattempt;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpActiveOpens.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 6:
    u_value = tcpstat.tcps_accepts;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpPassiveOpens.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 7:
    u_value = tcpstat.tcps_attemptfails;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpAttemptFails.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 8:
    u_value = tcpstat.tcps_estabresets;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpEstabResets.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 9:
#ifdef NEW_MIB
    u_value = count_curr_tcp_conn_estab(); /* in var_tcpConn.c */
#else
    u_value = count_curr_tcp_conn_estab(nl[N_TCB].n_value); /* in var_tcpConn.c */
#endif
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpCurrEstab.0");
    vb_ptr = make_varbind(oid_ptr, GAUGE_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 10:
    u_value = tcpstat.tcps_rcvtotal;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpInSegs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 11:
    u_value = tcpstat.tcps_sndtotal - tcpstat.tcps_sndrexmitpack;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpOutSegs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 12:
    u_value = tcpstat.tcps_sndrexmitpack;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpRetransSegs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 14:
    u_value = tcpstat.tcps_rcvbadsum + tcpstat.tcps_rcvbadoff +
	      tcpstat.tcps_rcvshort;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpInErrs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 15:
    u_value = tcpstat.tcps_sndrsts;
    oid_ptr = make_obj_id_from_dot((unsigned char *)"tcpOutRsts.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, u_value, 0, NULL, NULL);
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
get_tcp_stat(tcp_stuff)
struct	tcp_stuff *tcp_stuff; 
{
    struct strioctl strioc;

    if (tcp_fd < 0) {
	if ((tcp_fd = open(_PATH_TCP, O_RDONLY)) < 0) {
	    syslog(LOG_WARNING, "get_tcp_stat: open of %s failed: %m",
		_PATH_TCP);
	    return(FALSE);
	}
	else {
	    strioc.ic_cmd = SIOCSMGMT;
	    strioc.ic_dp = (char *)0;
	    strioc.ic_len = 0;
	    strioc.ic_timout = -1;

	    if (ioctl(tcp_fd, I_STR, &strioc) < 0) {
		syslog(LOG_WARNING, "get_tcp_stat: ioctl SIOCSMGMT failed: %m");
		(void) close (tcp_fd);
		tcp_fd = -1;
		return (FALSE);
	    }
	}
    }

    strioc.ic_cmd = SIOCGTCPSTUFF;
    strioc.ic_dp = (char *)tcp_stuff;
    strioc.ic_len = sizeof(struct tcp_stuff);
    strioc.ic_timout = -1;

    if (ioctl(tcp_fd, I_STR, &strioc) < 0) {
	syslog(LOG_WARNING, "get_tcp_stat: ioctl: SIOCGTCPSTUFF: %m");
	(void) close (tcp_fd);
	tcp_fd = -1;
	return(FALSE);
    }
 
    return(TRUE);
}
#else
get_tcp_stat(tcpstataddr, tcpstat)
     off_t tcpstataddr;
     struct tcpstat *tcpstat;
{
  if (tcpstataddr == 0) {
    syslog(LOG_WARNING,"tcpstataddr: symbol not defined");
    return(FALSE);
  }

  lseek(kmem, tcpstataddr, 0);
  if (read(kmem, tcpstat, sizeof(struct tcpstat)) < 0) {
    syslog(LOG_WARNING, "get_tcp_stat: %m");
    return(FALSE);
  }
  return(TRUE);
}
#endif
