#ident	"@(#)hrswrun.c	1.2"
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
** Source file:   hrswrun.c
**
** Description:   Module to realize hrSWRun group of Host Resources MIB
**                (RFC 1514)
**
** Contained functions:
**                      hrSWRunInit();
****                    hrSWRunObj()
**
** Author:   Cheng Yang
**
** Date Created:  December 1993.
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

/* including system include files */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* including isode and snmp header files */
#include "snmp.h"
#include "objects.h"

/* Include NetWare for Unix headers */

/* External Variables */

/* Local External Variables */


/* Forward References */

void  hrSWRunInit(void);

static int hrSWRunObj(OI oi, 
		       register struct type_SNMP_VarBind *v, 
		       int offset);

/* Defines */
#define hrSWOSIndex                   41
#define hrSWRunTable                  42
#define hrSWRunIndex                  421
#define hrSWRunName                   422
#define hrSWRunID                     423
#define hrSWRunPath                   424
#define hrSWRunParameters             425
#define hrSWRunType                   426
#define hrSWRunStatus                 427


void hrSWRunInit(void) 
{
  register OT ot;
  if(ot = text2obj("hrSWOSIndex"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWOSIndex;

  if(ot = text2obj("hrSWRunIndex"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWRunIndex;

  if(ot = text2obj("hrSWRunName"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWRunName;

  if(ot = text2obj("hrSWRunID"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWRunID;

  if(ot = text2obj("hrSWRunPath"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWRunPath;

  if(ot = text2obj("hrSWRunParameters"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWRunParameters;

  if(ot = text2obj("hrSWRunType"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWRunType;

  if(ot = text2obj("hrSWRunStatus"))
    ot->ot_getfnx = hrSWRunObj, 
    ot->ot_info = (caddr_t) hrSWRunStatus;
}

static int hrSWRunObj(OI oi, 
		       register struct type_SNMP_VarBind *v, 
		       int offset)
{
  int ifvar;
  register OID oid = oi->oi_name;
  register OT ot = oi->oi_type;

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
    case hrSWOSIndex:
    case hrSWRunIndex:
    case hrSWRunName:
    case hrSWRunID:
    case hrSWRunPath:
    case hrSWRunParameters:
    case hrSWRunType:
    case hrSWRunStatus:
    default:
      return error__status_noSuchName;
    }
}



