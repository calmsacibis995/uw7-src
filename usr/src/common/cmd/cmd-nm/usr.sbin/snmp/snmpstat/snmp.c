#ident	"@(#)snmp.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)snmp.c   1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
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
static char SNMPID[] = "@(#)snmp.c  2.1.1.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */

/*
 * snmp.c 
 *
 * print out the snmp entries 
 */

#include <unistd.h>
#include "snmpio.h"

int print_snmp_info();

static char *dots[] = {
   "snmpInPkts",
   "snmpOutPkts",
   "snmpInBadVersions",
   "snmpInBadCommunityNames",
   "snmpInBadCommunityUses",
   "snmpInASNParseErrs",
   "snmpInTooBigs",
   "snmpInNoSuchNames",
   "snmpInBadValues",
   "snmpInReadOnlys",
   "snmpInGenErrs",
   "snmpInTotalReqVars",
   "snmpInTotalSetVars",
   "snmpInGetRequests",
   "snmpInGetNexts",
   "snmpInSetRequests",
   "snmpInGetResponses",
   "snmpInTraps",
   "snmpOutTooBigs",
   "snmpOutNoSuchNames",
   "snmpOutBadValues",
   "snmpOutGenErrs",
   "snmpOutGetRequests",
   "snmpOutGetNexts",
   "snmpOutSetRequests",
   "snmpOutGetResponses",
   "snmpOutTraps",
   "snmpEnableAuthenTraps",
   ""
};

snmppr(community)
   char *community;
{

   return(doreq(dots, community, print_snmp_info));
}

extern char *map_mib();

char *enable_auth_traps_map[3] = { "????", "enabled", "disabled" };

/* ARGSUSED */
print_snmp_info(vb_list_ptr, lineno, community)
   VarBindList *vb_list_ptr;
   int lineno;
   char *community;
{
   int index;
   VarBindList *vb_ptr;
   struct _snmpstat sstat;
   int enable_auth_traps;

   index = 0;
   vb_ptr = vb_list_ptr;
   if (!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.inpkts = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outpkts = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.inbadversions = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.inbadcommunitynames = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.inbadcommunityuses = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.inasnparseerrs = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.intoobigs = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.innosuchnames = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.inbadvalues = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.inreadonlys = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.ingenerrs = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.intotalreqvars = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.intotalsetvars = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.ingetrequests = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.ingetnexts = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.insetrequests = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.ingetresponses = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.intraps = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outtoobigs = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outnosuchnames = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outbadvalues = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outgenerrs = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outgetrequests = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outgetnexts = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outsetrequests = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outgetresponses = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   sstat.outtraps = vb_ptr->vb_ptr->value->ul_value;

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   enable_auth_traps = vb_ptr->vb_ptr->value->sl_value;

   if (lineno == 0) {
      printf(gettxt(":27", "SNMP Statistics\n"));
   }

   printf(gettxt(":28", "\nInput:\n"));
   printf("%-10s %-10s %-10s %-10s %-10s\n", 
         gettxt(":46", "Pkts"), gettxt(":29", "BadVers"), 
         gettxt(":30", "BadCommNam"), gettxt(":31", "BadCommUse"), 
         gettxt(":32", "AsnParErr"));
   printf("%-10d %-10d %-10d %-10d %-10d\n", sstat.inpkts,
          sstat.inbadversions, sstat.inbadcommunitynames,
          sstat.inbadcommunityuses, sstat.inasnparseerrs);
   printf("%-10s %-10s %-10s %-10s %-10s\n",
         gettxt(":33", "TooBig"), gettxt(":34", "NoSuchName"), 
         gettxt(":35", "BadValue"), gettxt(":36", "ReadOnly"), 
         gettxt(":37", "GenError"));
   printf("%-10d %-10d %-10d %-10d %-10d\n",
          sstat.intoobigs, sstat.innosuchnames,
          sstat.inbadvalues, sstat.inreadonlys, sstat.ingenerrs);
   printf("%-10s %-10s %-10s %-10s %-10s %-10s %-10s\n",
         gettxt(":38", "ReqVars"), gettxt(":39", "SetVars"),
         gettxt(":40", "Gets"), gettxt(":41", "GetNexts"), 
         gettxt(":42", "Sets"), gettxt(":43", "GetResps"), 
         gettxt(":44", "Traps"));
   printf("%-10d %-10d %-10d %-10d %-10d %-10d %-10d\n",
          sstat.intotalreqvars, sstat.intotalsetvars,
          sstat.ingetrequests, sstat.ingetnexts, sstat.insetrequests,
          sstat.ingetresponses, sstat.intraps);

   printf(gettxt(":45", "\nOutput:\n"));
   printf("%-10s %-10s %-10s %-10s %-10s\n", 
         gettxt(":46", "Pkts"), gettxt(":33", "TooBig"),
         gettxt(":34", "NoSuchName"), gettxt(":35", "BadValue"),
         gettxt(":37", "GenError"));
   printf("%-10d %-10d %-10d %-10d %-10d\n", sstat.outpkts,
          sstat.outtoobigs, sstat.outnosuchnames, sstat.outbadvalues,
          sstat.outgenerrs);
   printf("%-10s %-10s %-10s %-10s %-10s %-10s\n", 
         gettxt(":40", "Gets"), gettxt(":41", "GetNexts"), 
         gettxt(":42", "Sets"), gettxt(":43", "GetResps"), 
         gettxt(":44", "Traps"), gettxt(":47", "Auth_Traps"));
   printf("%-10d %-10d %-10d %-10d %-10d %-10s\n", sstat.outgetrequests,
          sstat.outgetnexts, sstat.outsetrequests, sstat.outgetresponses,
          sstat.outtraps, map_mib(enable_auth_traps, enable_auth_traps_map,
                   sizeof(enable_auth_traps_map)));

   return(0);
}
