#ident	"@(#)hrswinstalled.c	1.2"
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
** Source file:   hrswinstalled.c
**
** Description:   Module to realize hrSWInstalled group of Host Resources MIB
**                (RFC 1514)
**
** Contained functions:
**                      hrSWInstalledInit();
****                    hrSWInstalledObj()
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

void  hrSWInstalledInit(void);

static int hrSWInstalledObj(OI oi, 
			    register struct type_SNMP_VarBind *v, 
			    int offset);

/* Defines */
#define hrSWInstalledLastChange             61
#define hrSWInstalledLastUpdateTime         62

#define hrSWInstalledTable                  63
#define hrSWInstalledIndex                  631
#define hrSWInstalledName                   632
#define hrSWInstalledID                     633
#define hrSWInstalledType                   634
#define hrSWInstalledDate                   635


void hrSWInstalledInit(void) 
{
  register OT ot;

  if(ot = text2obj("hrSWInstalledLastChange"))
    ot->ot_getfnx = hrSWInstalledObj, 
    ot->ot_info = (caddr_t) hrSWInstalledLastChange;

  if(ot = text2obj("hrSWInstalledLastUpdateTime"))
    ot->ot_getfnx = hrSWInstalledObj, 
    ot->ot_info = (caddr_t) hrSWInstalledLastUpdateTime;

  if(ot = text2obj("hrSWInstalledIndex"))
    ot->ot_getfnx = hrSWInstalledObj, 
    ot->ot_info = (caddr_t) hrSWInstalledIndex;

  if(ot = text2obj("hrSWInstalledName"))
    ot->ot_getfnx = hrSWInstalledObj, 
    ot->ot_info = (caddr_t) hrSWInstalledName;

  if(ot = text2obj("hrSWInstalledID"))
    ot->ot_getfnx = hrSWInstalledObj, 
    ot->ot_info = (caddr_t) hrSWInstalledID;

  if(ot = text2obj("hrSWInstalledType"))
    ot->ot_getfnx = hrSWInstalledObj, 
    ot->ot_info = (caddr_t) hrSWInstalledType;

  if(ot = text2obj("hrSWInstalledDate"))
    ot->ot_getfnx = hrSWInstalledObj, 
    ot->ot_info = (caddr_t) hrSWInstalledDate;
}

static int hrSWInstalledObj(OI oi, 
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
    case hrSWInstalledLastChange:
    case hrSWInstalledLastUpdateTime:
    case hrSWInstalledIndex:
    case hrSWInstalledName:
    case hrSWInstalledID:
    case hrSWInstalledType:
    case hrSWInstalledDate:
    default:
      return error__status_noSuchName;
    }
}



