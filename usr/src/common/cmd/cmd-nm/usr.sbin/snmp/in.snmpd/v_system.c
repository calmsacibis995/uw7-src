#ident	"@(#)v_system.c	1.2"
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
static char SNMPID[] = "@(#)v_system.c	3.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */
#include <stdio.h>
#include <sys/time.h>
#ifdef SVR4
#ifdef i386
#include <sys/sysi86.h>
#endif
#ifdef u3b2
#include <sys/sys3b.h>
#endif
#endif

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE	0
#define TRUE	1

VarBindList *get_next_class();

extern char global_sys_descr[];
extern char global_sys_object_ID[];
extern char global_sys_contact[];
extern char global_sys_location[];

extern struct timeval global_tv;
extern struct timezone global_tz;

VarBindList
*var_system_get(OID var_name_ptr, 
		OID in_name_ptr,
		int arg,
		VarEntry *var_next_ptr,
		int type_search)
{
  VarBindList *vb_ptr;
  OctetString *os_ptr;
  OID oid_ptr;
  OID oidvalue_ptr;
  struct timeval tv;
  struct timezone tz;
  long timeticks;
  char sysname[256];
  int services;

  if ((type_search == NEXT) && (cmp_oid_class(in_name_ptr, var_name_ptr) == 0) && (in_name_ptr->length > var_name_ptr->length))
    return(get_next_class(var_next_ptr));

  /* if it is a next or an exact with the .0 instance identifier, make */
  if ((type_search == NEXT) || 
      ((in_name_ptr->length == var_name_ptr->length + 1) 
       && (in_name_ptr->oid_ptr[in_name_ptr->length - 1] == 0))) {
    switch (arg) {
    case 1:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"sysDescr.0");
      os_ptr = make_octet_from_text((unsigned char *)global_sys_descr);
      vb_ptr = make_varbind(oid_ptr, DisplayString, 0,0,os_ptr,NULL);
      oid_ptr = NULL;
      os_ptr = NULL;
      break;
    case 2:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"sysObjectID.0");
      oidvalue_ptr = make_obj_id_from_dot((unsigned char *)global_sys_object_ID);
      vb_ptr = make_varbind(oid_ptr, OBJECT_ID_TYPE, 0,0,NULL,oidvalue_ptr);
      oid_ptr = NULL;
      oidvalue_ptr = NULL;
      break;
    case 3:
      gettimeofday(&tv, &tz);
      timeticks = ((tv.tv_sec - global_tv.tv_sec) * 100) +
		  ((tv.tv_usec - global_tv.tv_usec) / 10000); 
      oid_ptr = make_obj_id_from_dot((unsigned char *)"sysUpTime.0");
      vb_ptr = make_varbind(oid_ptr, TIME_TICKS_TYPE, 0,timeticks,NULL,NULL);
      oid_ptr = NULL;
      break;
    case 4:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"sysContact.0");
      os_ptr = make_octet_from_text((unsigned char *)global_sys_contact);
      vb_ptr = make_varbind(oid_ptr, DisplayString, 0,0,os_ptr,NULL);
      oid_ptr = NULL;
      os_ptr = NULL;
      break;
    case 5:
      if (gethostname(sysname, sizeof(sysname) - 1) < 0) {
	return(NULL);
      }
      sysname[255] = '\0';	/* just in case */
      oid_ptr = make_obj_id_from_dot((unsigned char *)"sysName.0");
      os_ptr = make_octet_from_text((unsigned char *)sysname);
      vb_ptr = make_varbind(oid_ptr, DisplayString, 0,0,os_ptr,NULL);
      oid_ptr = NULL;
      os_ptr = NULL;
      break;
    case 6:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"sysLocation.0");
      os_ptr = make_octet_from_text((unsigned char *)global_sys_location);
      vb_ptr = make_varbind(oid_ptr, DisplayString, 0,0,os_ptr,NULL);
      oid_ptr = NULL;
      os_ptr = NULL;
      break;
    case 7:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"sysServices.0");
#define twoto(n)	(1 << (n))
#ifdef BSD
      services = twoto(7 - 1) + twoto(4 - 1);
#endif
#if defined(SVR3) || defined(SVR4)
      services = twoto(7 - 1) + twoto(4 - 1);
#endif
      vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0,services,NULL,NULL);
      oid_ptr = NULL;
      break;
    default:
      return(NULL);
    }
    return(vb_ptr);
  }
  return(NULL);		/* exact search failed */
}


int var_system_test(OID var_name_ptr,
		    OID in_name_ptr,
		    unsigned int arg,
		    ObjectSyntax *value)
{
        if ((in_name_ptr->length != var_name_ptr->length + 1)
             || (in_name_ptr->oid_ptr[in_name_ptr->length - 1] != 0))
                return (FALSE);

  switch (arg) {
  case 4:
  case 5:
  case 6:
    if (value->os_value->length < 0 || value->os_value->length > 255)
      return(FALSE);
    break;
  default:
    return(FALSE);
  }
  return(TRUE);
}

int var_system_set(OID *var_name_ptr,
		   OID *in_name_ptr,
		   unsigned int arg,
		   ObjectSyntax *value)
{
#ifdef SVR4
  char sysname[256];
#endif

  switch (arg) {
  case 4:
    bcopy((char *)value->os_value->octet_ptr, global_sys_contact,
	   value->os_value->length);
    global_sys_contact[value->os_value->length] = '\0';
    {  /* Update the /etc/netmgt/snmpd.conf file */
      char substr[1024];
      pid_t pid;
      int status;
      sprintf(substr, "s/^contact=.*$/contact=%s/", global_sys_contact);
      if ((pid=fork())==0)
	{
	  /* child */
	  execl("/usr/gnu/bin/perl", "perl", "-p", "-i.bak",
		"-e", substr, SNMPD_CONF_FILE, (char *)0);
	}
      wait(&status);
    }
    break;
  case 5:
#ifdef BSD
    if (sethostname(value->os_value->octet_ptr, value->os_value->length) < 0) {
      return(FALSE);
    }
#endif
#ifdef SVR3
    if (sethostname(value->os_value->octet_ptr, value->os_value->length) < 0) {
      return(FALSE);
    }
#endif
#ifdef SVR4
    bcopy ((char *) value->os_value->octet_ptr, sysname,
	    value->os_value->length);
    sysname[value->os_value->length] = '\0';
#ifdef i386
    if (sysi86(SETNAME, sysname, 0) < 0) {
      return(FALSE);
    }
#endif
#ifdef u3b2
    if (sys3b(SETNAME, sysname, 0) < 0) {
      return(FALSE);
    }
#endif
#endif
    break;
  case 6:
    bcopy((char *)value->os_value->octet_ptr, global_sys_location,
	   value->os_value->length);
    global_sys_location[value->os_value->length] = '\0';
    {  /* Update the /etc/netmgt/snmpd.conf file */
      char substr[1024];
      pid_t pid;
      int status;
      sprintf(substr, "s/^location=.*$/location=%s/", global_sys_location);
      if ((pid=fork())==0)
	{
	  /* child */
	  execl("/usr/gnu/bin/perl", "perl", "-p", "-i.bak",
		"-e", substr, SNMPD_CONF_FILE, (char *)0);
	}
      wait(&status);
    }
    break;
  default:
    return(FALSE);
  }
  return(TRUE);
}
