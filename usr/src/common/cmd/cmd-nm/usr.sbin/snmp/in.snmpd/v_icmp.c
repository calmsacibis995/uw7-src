#ident	"@(#)v_icmp.c	1.3"
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
static char SNMPID[] = "@(#)v_icmp.c	1.3";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 * Revision History:
 *  2/4/89 JDC
 *  amended copyright notice
 *  changed references from "gotone" to "snmpd"
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
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <syslog.h>
#include <unistd.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1


#if (!defined ULTRIX && !defined SUNOS35)
#define ICMP_BIGGEST_COUNTED 16
#else
#define ICMP_BIGGEST_COUNTED 14
#endif

VarBindList *get_next_class();

VarBindList
*var_icmp_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     unsigned int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  struct icmpstat icmpstat;
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
  cc = get_icmp_stat(&icmpstat);
#else
  cc = get_icmp_stat(nl[N_ICMPSTAT].n_value, &icmpstat);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  switch (arg) {
  case 1:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInMsgs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_intotal, 0,
			  NULL, NULL);
    oid_ptr = NULL;
    break;
  case 2:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInErrors.0");
    total = icmpstat.icps_badcode + icmpstat.icps_tooshort +
      icmpstat.icps_checksum + icmpstat.icps_badlen;
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, total, 0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 3:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInDestUnreachs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[3], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 4:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInTimeExcds.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[11],
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 5:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInParmProbs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[12], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 6:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInSrcQuenchs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[4], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 7:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInRedirects.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[5], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 8:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInEchos.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[8], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 9:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInEchoReps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[0], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 10:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInTimestamps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[13], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 11:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInTimestampReps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[14], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 12:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInAddrMasks.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[17], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 13:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpInAddrMaskReps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_inhist[18], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 14:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutMsgs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outtotal,
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 15:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutErrors.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outerrors, 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 16:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutDestUnreachs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[3], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 17:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutTimeExcds.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[11], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 18:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutParmProbs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[12], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 19:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutSrcQuenchs.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[4], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 20:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutRedirects.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[5], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 21:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutEchos.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[8], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 22:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutEchoReps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[0], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 23:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutTimestamps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[13], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 24:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutTimestampReps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[14], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
#if !defined ULTRIX && !defined SUNOS35
  case 25:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutAddrMasks.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[17], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 26:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"icmpOutAddrMaskReps.0");
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, icmpstat.icps_outhist[18], 
			  0, NULL, NULL);
    oid_ptr = NULL;
    break;
#endif
  default:			/* should never happen */
    if (type_search == EXACT)
      return(NULL);
    else
      return(get_next_class(var_next_ptr));
  };

  return(vb_ptr);
} 

#ifdef NEW_MIB
extern int icmp_fd;

int
get_icmp_stat(icmpstat)
struct	icmpstat *icmpstat; 
{
    struct strioctl strioc;

    if (icmp_fd < 0) {
	if ((icmp_fd = open (_PATH_ICMP, O_RDONLY)) < 0) {
	    syslog(LOG_WARNING, gettxt(":136", "get_icmp_stat: open of %s failed: %m.\n"),
		_PATH_ICMP);
	    return (FALSE);
	}
	else {
	    strioc.ic_cmd = SIOCSMGMT;
	    strioc.ic_dp = (char *)0;
	    strioc.ic_len = 0;
	    strioc.ic_timout = -1;
	    if (ioctl(icmp_fd, I_STR, &strioc) < 0) {
		syslog(LOG_WARNING, gettxt(":137", "icmp: ioctl SIOCSMGMT failed: %m.\n"));
		(void) close (icmp_fd);
		icmp_fd = -1;
	    }
	}
    }

    strioc.ic_cmd = SIOCGICMPSTATS;
    strioc.ic_dp = (char *)icmpstat;
    strioc.ic_len = sizeof(struct icmpstat);
    strioc.ic_timout = -1;

    if (ioctl(icmp_fd, I_STR, &strioc) < 0) {
	syslog(LOG_WARNING, gettxt(":138", "get_icmp_stat: ioctl: SIOCGICMPSTATS: %m.\n"));
	(void) close(icmp_fd);
	icmp_fd = -1;
	return(FALSE);
    }
 
    return(TRUE);
}
#else
get_icmp_stat(icmpstataddr, icmpstat)
      off_t icmpstataddr;
      struct icmpstat *icmpstat; 
{
  if (icmpstataddr == 0) {
    syslog(LOG_WARNING, gettxt(":139", "icmpstataddr: Symbol not defined.\n"));
    return(FALSE);
  }

  lseek(kmem, icmpstataddr, 0);
  if (read(kmem, icmpstat, sizeof(struct icmpstat)) < 0) {
    syslog(LOG_WARNING, gettxt(":140", "get_icmp_stat: %m.\n"));
    return(FALSE);
  }
  return (TRUE);
}
#endif
