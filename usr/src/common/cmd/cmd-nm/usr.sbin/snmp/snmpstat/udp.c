#ident	"@(#)udp.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)udp.c 1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)udp.c   2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */

/*
 * Revision History: 
 *
 * 2/6/89 JDC Amended copyright notice 
 *
 * Professionalized error messages a bit 
 *
 * Expanded comments and documentation 
 *
 * Added code for genErr 
 *
 */

/*
 * udp.c 
 *
 * print out the udp endpoint table entries 
 */

#include <unistd.h>
#include "snmpio.h"

#ifdef BSD
#include <strings.h>
#endif
#if defined(SVR3) || defined(SVR4)
#include <string.h>
#endif

int print_udp_info();

static char *dots[] = {
   "udpLocalAddress",
   "udpLocalPort",
   ""
};

udppr(community)
   char *community;
{

   return(doreq(dots, community, print_udp_info));
}

extern char *inetname();

extern int table_header_printed;
extern int prall;

/* ARGSUSED */
print_udp_info(vb_list_ptr, lineno, community)
   VarBindList *vb_list_ptr;
   int lineno;
   char *community;
{
   int index;
   VarBindList *vb_ptr;
   unsigned long local_addr;
   int local_port;

   index = 0;
   vb_ptr = vb_list_ptr;
   if (!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   local_addr = ((vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
            (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
            (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
            (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));

   vb_ptr = vb_ptr->next;
   if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
      return(-1);
   }
   local_port = vb_ptr->vb_ptr->value->sl_value;

   if (!prall && local_addr == INADDR_ANY) {
      return(0);
   }

   if (!table_header_printed) {
      table_header_printed = 1;
      printf(gettxt(":68", "Active Internet connections"));
      if (prall)
         printf(gettxt(":63", " (including servers)"));
      putchar('\n');
      printf("%-5.5s  %-22.22s %-22.22s %s\n", 
            gettxt(":25", "Proto"), gettxt(":64", "Local Address"), 
            gettxt(":65", "Foreign Address"), gettxt(":66", "(state)"));
   }

   printf("%-5.5s  ", gettxt(":69", "udp"));
   printf("%-22.22s ", inetname(local_addr, local_port, gettxt(":69", "udp")));
   printf("%-22.22s ", inetname(INADDR_ANY, 0, gettxt(":69", "udp")));
   printf("\n");
   return(0);
}
