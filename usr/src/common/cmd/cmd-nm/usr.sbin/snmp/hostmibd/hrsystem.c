#ident	"@(#)hrsystem.c	1.2"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#if !defined(NO_SCCS_ID) && !defined(lint) && !defined(SABER)
static char rcsid[] = "@(#)$Header$";
#endif

/****************************************************************************
** Source file:   hrsystem.c
**
** Description:   Module to realize hrSystem group of Host Resources MIB
**                (RFC 1514)
**
** Contained functions:
**                      hrSystemInit();
****                    hrSystemObj()
**
** Author:   Cheng Yang
**
** Date Created:  December 1993.
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

/*
*	MODIFICATION HISTORY
*
* 	L000	townsend 11 Nov 97
*        - used the boot device index saved (in hrdevice.c) to
*          return the value of hrSystemInitialLoadDevice
*/
/* including system include files */
#include <stdio.h>
#include <string.h>
#include <nlist.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/proc.h>
#include <utmp.h>
#include <time.h>

/* including isode and snmp header files */
#include "snmp.h"
#include "objects.h"

/* External Variables */
extern int bootDiskIndex; /* L000 */

/* Local External Variables */

/* Forward References */

void  hrSystemInit(void);

static int hrSystemObj(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset);

static int hrSystemSet(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset);

/* Defines */
#define hrSystemUptime                 11
#define hrSystemDate                   12
#define hrSystemInitialLoadDevice      13
#define hrSystemInitialLoadParameters  14
#define hrSystemNumUsers               15
#define hrSystemProcesses              16
#define hrSystemMaxProcesses           17

void hrSystemInit(void) 
{
  OT ot;
  if(ot = text2obj("hrSystemUptime"))
    ot->ot_getfnx = hrSystemObj, 
    ot->ot_info = (caddr_t) hrSystemUptime;

  if(ot = text2obj("hrSystemDate"))
    ot->ot_getfnx = hrSystemObj, 
    ot->ot_setfnx = hrSystemSet,
    ot->ot_info = (caddr_t) hrSystemDate;
  
  if(ot = text2obj("hrSystemInitialLoadDevice"))
    ot->ot_getfnx = hrSystemObj,
    ot->ot_info = (caddr_t) hrSystemInitialLoadDevice;

  if(ot = text2obj("hrSystemInitialLoadParameters"))
    ot->ot_getfnx = hrSystemObj,
    ot->ot_info = (caddr_t) hrSystemInitialLoadParameters;

  if(ot = text2obj("hrSystemNumUsers"))
    ot->ot_getfnx = hrSystemObj, 
    ot->ot_info = (caddr_t) hrSystemNumUsers;

  if(ot = text2obj("hrSystemProcesses"))
    ot->ot_getfnx = hrSystemObj, 
    ot->ot_info = (caddr_t) hrSystemProcesses;

  if(ot = text2obj("hrSystemMaxProcesses"))
    ot->ot_getfnx = hrSystemObj, 
    ot->ot_info = (caddr_t) hrSystemMaxProcesses;
}

static int hrSystemObj(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset)
{
  int ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  ifvar = (int) ot->ot_info;

  switch(offset) 
    {
    case SMUX__PDUs_get__request:
      if(oid->oid_nelem != ot->ot_name->oid_nelem + 1
	 || oid->oid_elements[oid->oid_nelem - 1] != 0) 
	{
	  return error__status_noSuchName;
	}
      break;

    case SMUX__PDUs_get__next__request:
      if(oid->oid_nelem == ot->ot_name->oid_nelem)
	{
	  OID   new;

	  if((new = oid_extend(oid, 1)) == NULLOID)
	    return NOTOK;
	  new->oid_elements[new->oid_nelem - 1] = 0;
	  
	  if(v->name)
	    free_SNMP_ObjectName(v->name);

	  v->name = new;
	}
      else
	return NOTOK;
      break;

    default:
      return error__status_genErr;
    }

  switch(ifvar)
    {
    case hrSystemUptime:  /* Same algorithm as w */
      {
	int uptime=0;
	time_t now;
	static struct utmp *utmpp=0;
	while ((utmpp=getutent())!=NULL)
	  if (utmpp->ut_type==BOOT_TIME)
	    {
	      time(&now);
	      uptime = now-utmpp->ut_time;
	      break;
	    }
	endutent();
	free(utmpp);
	return o_integer(oi, v, uptime*100);
      }

    case hrSystemDate:
      {
	unsigned char SysDate[11];
	struct tm * tm_local;
	time_t now;
	time_t localnow;
	int timediff;
	time(&now);

	tm_local=localtime(&now);
	localnow=mktime(tm_local);

	/* Assume unsigned short integer is 16 bit */

	*((unsigned short *)SysDate)=
	  htons((unsigned short)(tm_local->tm_year+1900));
	SysDate[2]=(unsigned char)tm_local->tm_mon+1;  /* it started at 0 */
	SysDate[3]=(unsigned char)tm_local->tm_mday;
	SysDate[4]=(unsigned char)tm_local->tm_hour;
	SysDate[5]=(unsigned char)tm_local->tm_min;
	SysDate[6]=(unsigned char)tm_local->tm_sec;
	SysDate[7]=0;
	
	if ((timediff=(int)difftime(localnow, now))>0)
	  {
	    SysDate[8]='+';
	    SysDate[9]=(unsigned char)(timediff/3600);
	    SysDate[10]=(unsigned char)(timediff/60%60);
	  }
	else
	  if (timediff<0)
	    {
	      SysDate[9]='-';
	      SysDate[9]=(unsigned char)(-timediff/3600);
	      SysDate[10]=(unsigned char)(-timediff/60%60);
	    }
	free(tm_local);
	return o_string(oi, v, (char *)SysDate, 11);
      }

    case hrSystemInitialLoadDevice:
      return o_integer(oi, v, bootDiskIndex);	/* L000 */

    case hrSystemInitialLoadParameters:
      return o_string(oi, v, "/stand/unix", strlen("/stand/unix"));

    case hrSystemNumUsers:  /* Same algorithm as who -q */
      {
	int count=0;
	static struct utmp *utmpp=0;
	while ((utmpp=getutent())!=NULL)
	  if (utmpp->ut_type==USER_PROCESS)
	    count++;
	endutent();
	free(utmpp);
	return o_integer(oi, v, count);
      }

    case hrSystemProcesses:   /* Same algorithm as ps */
      { 
	DIR *dp;
	struct dirent *dirp;
	int count=0;

	if ((dp=opendir("/proc"))== NULL)
	  return o_integer(oi, v, count);
	while ((dirp= readdir(dp))!=NULL)
	  {
	    if (dirp->d_name[0]=='.')
	      continue;
	    else
	      count++;
	  }
	closedir(dp);
	return o_integer(oi, v, count);
      }

    case hrSystemMaxProcesses:
      /* There is no limit on number of processes. */
      return o_integer(oi, v, 0);

    default:
      return error__status_noSuchName;
    }
}

static int hrSystemSet(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset)
{
  int ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;
  OS os = ot->ot_syntax;

  ifvar = (int) ot->ot_info;

  if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
    return error__status_noSuchName;

  if (os==NULLOS)      
    return error__status_genErr;

  switch(ifvar)
    {
    case hrSystemDate:
      {
	char * tmpptr;
	char buff[20];
	char buff2[30];
	if (v->value->type == OCTET_PRIM_TYPE)
	  {
	    sprintf(buff, "%.2d%.2d%.2d%.2d%.4d",
		    (int)(((char *)(v->value->os_value->octet_ptr))[2]),
		    (int)(((char *)(v->value->os_value->octet_ptr))[3]),
		    (int)(((char *)(v->value->os_value->octet_ptr))[4]),
		    (int)(((char *)(v->value->os_value->octet_ptr))[5]),
		    ntohs(((unsigned short *)
			   (v->value->os_value->octet_ptr))[0]));
	    if (v->value->os_value->length>8)
	      switch (((char *)(v->value->os_value->octet_ptr))[8])
		{
		case '+':
		case '-':
		  sprintf(buff2, "/usr/bin/date %s", buff);
		  system(buff2);
		  break;
		default:
		  sprintf(buff2, "/usr/bin/date %s", buff);
		  system(buff2);
		  break;
		}
	    return error__status_noError;
	  }
	return error__status_noSuchName;
      }	
	
    default:
      return error__status_noSuchName;
    }
}












