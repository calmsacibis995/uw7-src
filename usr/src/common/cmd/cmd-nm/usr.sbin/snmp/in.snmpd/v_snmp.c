#ident	"@(#)v_snmp.c	1.2"
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
static char SNMPID[] = "@(#)v_snmp.c	3.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

#include <stdio.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE	0
#define TRUE	1

VarBindList *get_next_class();
int cmp_oid_class();

extern int auth_traps_enabled;

VarBindList
*var_snmp_get(OID var_name_ptr,
	      OID in_name_ptr,
	      int arg,
	      VarEntry *var_next_ptr,
	      int type_search)
{
  VarBindList *vb_ptr;
  OID oid_ptr;

  if ((type_search == NEXT) && (in_name_ptr->length > var_name_ptr->length) && (cmp_oid_class (in_name_ptr, var_name_ptr) == 0))
    return(get_next_class(var_next_ptr));

  /* if it is a next or an exact with the .0 instance identifier, make */
  if ((type_search == NEXT) || 
      ((in_name_ptr->length == var_name_ptr->length + 1) 
       && (in_name_ptr->oid_ptr[in_name_ptr->length - 1] == 0))) {
    switch (arg) {
    case 1:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInPkts.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->inpkts, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 2:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutPkts.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outpkts, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 3:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInBadVersions.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->inbadversions, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 4:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInBadCommunityNames.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->inbadcommunitynames, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 5:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInBadCommunityUses.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->inbadcommunityuses, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 6:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInASNParseErrs.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->inasnparseerrs, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 8:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInTooBigs.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->intoobigs, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 9:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInNoSuchNames.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->innosuchnames, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 10:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInBadValues.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->inbadvalues, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 11:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInReadOnlys.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->inreadonlys, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 12:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInGenErrs.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->ingenerrs, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 13:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInTotalReqVars.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->intotalreqvars, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 14:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInTotalSetVars.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->intotalsetvars, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 15:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInGetRequests.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->ingetrequests, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 16:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInGetNexts.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->ingetnexts, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 17:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInSetRequests.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->insetrequests, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 18:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInGetResponses.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->ingetresponses, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 19:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpInTraps.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->intraps, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 20:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutTooBigs.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outtoobigs, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 21:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutNoSuchNames.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outnosuchnames, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 22:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutBadValues.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outbadvalues, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 24:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutGenErrs.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outgenerrs, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 25:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutGetRequests.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outgetrequests, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 26:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutGetNexts.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outgetnexts, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 27:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutSetRequests.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outsetrequests, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 28:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutGetResponses.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outgetresponses, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 29:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpOutTraps.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
			    snmpstat->outtraps, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 30:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"snmpEnableAuthenTraps.0");
      vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
			    0, auth_traps_enabled, NULL, NULL);
      oid_ptr = NULL;
      break;
    default:
      return(NULL);
    }
    return(vb_ptr);
  }
  return(NULL);		/* exact search failed */
}


int var_snmp_test(OID var_name_ptr,
		  OID in_name_ptr,
		  unsigned int arg,
		  ObjectSyntax *value)
{
  if ((in_name_ptr->length != var_name_ptr->length + 1)
      || (in_name_ptr->oid_ptr[in_name_ptr->length - 1] != 0)) 
    return (FALSE);

  switch (arg) 
    {
    case 30:
      if (value->sl_value != 1 && value->sl_value != 2)
	return(FALSE);
      break;
    default:
      return(FALSE);
    }
  return(TRUE);
}


int var_snmp_set(OID var_name_ptr, OID in_name_ptr, 
		 unsigned int arg, ObjectSyntax *value)
{
  switch (arg) 
    {
    case 30:
      auth_traps_enabled = value->sl_value;
      break;
    default:
      return(FALSE);
    }
  return(TRUE);
}
