#ident	"@(#)v_udp.c	1.3"
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
static char SNMPID[] = "@(#)v_udp.c	1.3";
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
#include <sys/sockio.h>
#include <sys/stropts.h>
#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <syslog.h>
#include <unistd.h>

#include "snmp.h" /*"../lib/snmp.h"*/
#include "snmpuser.h" /*"../lib/snmpuser.h"*/
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

VarBindList *get_next_class();

VarBindList
*var_udp_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     unsigned int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  struct udpstat udpstat;
  int total;
  int cc;

  /* see if explicitly determined */
  if ((type_search == EXACT) &&
      ((in_name_ptr->length != (var_name_ptr->length + 1)) ||
       (in_name_ptr->oid_ptr[var_name_ptr->length] != 0)))
    return(NULL);

  if ((type_search == NEXT) && (cmp_oid_class(in_name_ptr, var_name_ptr) == 0) && (in_name_ptr->length >= (var_name_ptr->length + 1)))
    return(get_next_class(var_next_ptr));
  
#ifdef NEW_MIB
  cc = get_udp_stat(&udpstat);
#else
  cc = get_udp_stat(nl[N_UDPSTAT].n_value, &udpstat);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  switch (arg) {
  case 1:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"udpInDatagrams.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, udpstat.udps_indelivers, 0,
			  NULL, NULL);
    oid_ptr = NULL;
    break;
  case 2:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"udpNoPorts.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, udpstat.udps_noports, 0,
			  NULL, NULL);
    oid_ptr = NULL;
    break;
  case 3:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"udpInErrors.0");
    total = udpstat.udps_hdrops + udpstat.udps_badsum +
      udpstat.udps_badlen + udpstat.udps_inerrors;
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, total, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 4:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"udpOutDatagrams.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, udpstat.udps_outtotal, 0,
			  NULL, NULL);
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
get_udp_stat(udpstat)
struct	udpstat *udpstat; 
{
    struct strioctl strioc;

    if (udp_fd < 0) {
	if ((udp_fd = open (_PATH_UDP, O_RDWR)) < 0) {
	    syslog(LOG_WARNING, gettxt(":228", "get_udp_stat: Open of %s failed: %m.\n"),
		_PATH_UDP);
	    return(FALSE);
	}
	else {
	    strioc.ic_cmd = SIOCSMGMT;
	    strioc.ic_dp = (char *)0;
	    strioc.ic_len = 0;
	    strioc.ic_timout = -1;

	    if (ioctl(udp_fd, I_STR, &strioc) < 0) {
		syslog(LOG_WARNING, gettxt(":229", "get_udp_stat: SIOCSMGMT: %m.\n"));
		(void) close(udp_fd);
		udp_fd = -1;
		return (FALSE);
	    }
	}
    }

    strioc.ic_cmd = SIOCGUDPSTATS;
    strioc.ic_dp = (char *)udpstat;
    strioc.ic_len = sizeof(struct udpstat);
    strioc.ic_timout = -1;

    if (ioctl(udp_fd, I_STR, &strioc) < 0) {
	syslog(LOG_WARNING, gettxt(":230", "get_udp_stat: ioctl: SIOCGUDPSTATS: %m.\n"));
	(void) close(udp_fd);
	udp_fd = -1;
	return(FALSE);
    }
 
    return(TRUE);
}
#else
get_udp_stat(udpstataddr, udpstat)
     off_t udpstataddr;
     struct udpstat *udpstat;
{
  if (udpstataddr == 0) {
    syslog(LOG_WARNING, gettxt(":231", "udpstataddr: symbol not defined.\n"));
    return(FALSE);
  }

  lseek(kmem, udpstataddr, 0);
  if (read(kmem, udpstat, sizeof(struct udpstat)) < 0) {
    syslog(LOG_WARNING, gettxt(":232", "get_udp_stat: %m.\n"));
    return(FALSE);
  }
  return(TRUE);
}
#endif
