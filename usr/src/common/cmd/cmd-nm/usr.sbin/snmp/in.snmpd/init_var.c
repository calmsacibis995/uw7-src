#ident	"@(#)init_var.c	1.4"
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
static char SNMPID[] = "@(#)init_var.c	1.4";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 * Revision History:
 *
 * L000		jont	01oct97
 *	- add_var() was not correctly filtering out duplicates. If a duplicate
 *	  is located we can free the oid_ptr that we were passed, as it will
 *	  not be needed, and will not be freed otherwise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"
#include "variables.h"

VarEntry *add_var(VarEntry *var_ptr,
		  OID oid_ptr,
		  unsigned int type,
		  unsigned int rw_flag,
		  unsigned int arg,
		  VarBindList *(*funct_get)(),
		  int (*funct_test_set)(),
		  int (*funct_set)());

VarEntry *var_list_root;

extern int log_level;

VarEntry *init_var(void)
{
    VarEntry *var_ptr;

    var_ptr = NULL;

/*=================*/
/* General  stuff  */
/*=================*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot ((unsigned char *)"iso"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot ((unsigned char *)"org"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot ((unsigned char *)"dod"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot ((unsigned char *)"internet"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot ((unsigned char *)"mgmt"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot ((unsigned char *)"mib-2"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);


/*================*/
/* System  stuff  */
/*================*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"system"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"sysDescr"),
		    DisplayString, READ_ONLY, 1, var_system_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"sysObjectID"),
		    OBJECT_ID_TYPE, READ_ONLY, 2, var_system_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"sysUpTime"),
		    TIME_TICKS_TYPE, READ_ONLY, 3, var_system_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"sysContact"),
		    DisplayString, READ_WRITE, 4, var_system_get,
		    var_system_test, var_system_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"sysName"),
		    DisplayString, READ_WRITE, 5, var_system_get,
		    var_system_test, var_system_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"sysLocation"),
		    DisplayString, READ_WRITE, 6, var_system_get, 
		    var_system_test, var_system_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"sysServices"),
		    INTEGER_TYPE, READ_ONLY, 7, var_system_get, NULL, NULL);

/*===================*/
/* Interface  stuff  */
/*===================*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"interfaces"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifNumber"),
 		    INTEGER_TYPE, READ_ONLY, 0, var_if_num_get, NULL, NULL);
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifTable"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifEntry"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

#ifdef notdef /* dme - was NEW_MIB */

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifIndex"),
 		    INTEGER_TYPE, READ_ONLY, 1, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifDescr"),
 		    DisplayString, READ_ONLY, 2, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifType"),
 		    INTEGER_TYPE, READ_ONLY, 3, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifMtu"),
 		    INTEGER_TYPE, READ_ONLY, 4, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifSpeed"),
 		    GAUGE_TYPE, READ_ONLY, 5, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifPhysAddress"),
 		    OCTET_PRIM_TYPE, READ_ONLY, 6, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifAdminStatus"),
 		    INTEGER_TYPE, READ_WRITE, 7, var_interfaces_get, 
		    var_if_adminstatus_test, var_if_adminstatus_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOperStatus"),
 		    INTEGER_TYPE, READ_ONLY, 8, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifLastChange"),
		    TIME_TICKS_TYPE, READ_ONLY, 9, var_interfaces_get, NULL, NULL);


    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInOctets"),
 		    COUNTER_TYPE, READ_ONLY, 10, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInUcastPkts"),
 		    COUNTER_TYPE, READ_ONLY, 11, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInNUcastPkts"),
 		    COUNTER_TYPE, READ_ONLY, 12, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInDiscards"),
 		    COUNTER_TYPE, READ_ONLY, 13, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInErrors"),
 		    COUNTER_TYPE, READ_ONLY, 14, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInUnknownProtos"),
 		    COUNTER_TYPE, READ_ONLY, 15, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutOctets"),
 		    COUNTER_TYPE, READ_ONLY, 16, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutUcastPkts"),
		    COUNTER_TYPE, READ_ONLY, 17, var_interfaces_get, NULL, NULL);
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutNUcastPkts"),
 		    COUNTER_TYPE, READ_ONLY, 18, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutDiscards"),
 		    COUNTER_TYPE, READ_ONLY, 19, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutErrors"),
 		    COUNTER_TYPE, READ_ONLY, 20, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutQLen"),
 		    GAUGE_TYPE, READ_ONLY, 21, var_interfaces_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifSpecific"),
 		    OBJECT_ID_TYPE, READ_ONLY, 22, var_interfaces_get, NULL, NULL);

#else /* !(NEW_MIB) */

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifIndex"),
 		    INTEGER_TYPE, READ_ONLY, 0, var_if_index_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifDescr"),
 		    DisplayString, READ_ONLY, 0, var_if_name_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifType"),
 		    INTEGER_TYPE, READ_ONLY, 0, var_if_type_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifMtu"),
 		    INTEGER_TYPE, READ_ONLY, 0, var_if_mtu_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifSpeed"),
 		    GAUGE_TYPE, READ_ONLY, 0, var_if_speed_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifPhysAddress"),
 		    OCTET_PRIM_TYPE, READ_ONLY, 0, var_if_physaddr_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifAdminStatus"),
 		    INTEGER_TYPE, READ_WRITE, 0, var_if_adminstatus_get, 
		    var_if_adminstatus_test, var_if_adminstatus_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOperStatus"),
 		    INTEGER_TYPE, READ_ONLY, 0, var_if_operstatus_get, NULL, NULL);


    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifLastChange"),
		    TIME_TICKS_TYPE, READ_ONLY, 0, var_if_up_time_get, NULL, NULL);


    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInUcastPkts"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_inucast_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInNUcastPkts"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_innucast_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInOctets"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_inoctets_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInDiscards"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_indiscards_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInUnknownProtos"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_inunkprotos_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifInErrors"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_inerrors_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutUcastPkts"),
		    COUNTER_TYPE, READ_ONLY, 0, var_if_outucast_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutNUcastPkts"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_outnucast_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutOctets"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_outoctets_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutDiscards"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_outdiscards_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutErrors"),
 		    COUNTER_TYPE, READ_ONLY, 0, var_if_outerrors_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifOutQLen"),
 		    GAUGE_TYPE, READ_ONLY, 0, var_if_outqlen_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ifSpecific"),
 		    OBJECT_ID_TYPE, READ_ONLY, 0, var_if_specific_get, NULL, NULL);

#endif /* !(NEW_MIB) */

/*============*/
/* Arp stuff  */
/*============*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"at"),
 		    Aggregate, NONE, 1, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"atTable"),
 		    Aggregate, NONE, 1, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"atEntry"),
 		    Aggregate, NONE, 1, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"atIfIndex"),
 		    INTEGER_TYPE, READ_WRITE, 1, var_at_get, var_at_test,
			var_at_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"atPhysAddress"),
 		    OCTET_PRIM_TYPE, READ_WRITE, 2, var_at_get,
		    var_at_test, var_at_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"atNetAddress"),
 		    IP_ADDR_PRIM_TYPE, READ_WRITE, 3, var_at_get,
		    var_at_test, var_at_set);

/*=============*/
/*  IP stuff   */
/*=============*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ip"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipForwarding"),
 		    INTEGER_TYPE, READ_WRITE, 1, var_ip_stat_get,
		    var_ip_stat_test, var_ip_stat_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipDefaultTTL"),
 		    INTEGER_TYPE, READ_WRITE, 2, var_ip_stat_get,
		    var_ip_stat_test, var_ip_stat_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipInReceives"),
 		    COUNTER_TYPE, READ_ONLY, 3, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipInHdrErrors"),
 		    COUNTER_TYPE, READ_ONLY, 4, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipInAddrErrors"),
 		    COUNTER_TYPE, READ_ONLY, 5, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipForwDatagrams"),
 		    COUNTER_TYPE, READ_ONLY, 6, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipInUnknownProtos"),
 		    COUNTER_TYPE, READ_ONLY, 7, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipInDiscards"),
 		    COUNTER_TYPE, READ_ONLY, 8, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipInDelivers"),
 		    COUNTER_TYPE, READ_ONLY, 9, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipOutRequests"),
 		    COUNTER_TYPE, READ_ONLY, 10, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipOutDiscards"),
 		    COUNTER_TYPE, READ_ONLY, 11, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipOutNoRoutes"),
 		    COUNTER_TYPE, READ_ONLY, 12, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipReasmTimeout"),
 		    COUNTER_TYPE, READ_ONLY, 13, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, 
		      make_obj_id_from_dot((unsigned char *)"ipReasmReqds"),
		      COUNTER_TYPE, READ_ONLY, 14, 
		      var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipReasmOKs"),
 		    COUNTER_TYPE, READ_ONLY, 15, var_ip_stat_get, NULL, NULL);


    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipReasmFails"),
 		    COUNTER_TYPE, READ_ONLY, 16, var_ip_stat_get, NULL, NULL);


    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipFragOKs"),
 		    COUNTER_TYPE, READ_ONLY, 17, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipFragFails"),
 		    COUNTER_TYPE, READ_ONLY, 18, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipFragCreates"),
 		    COUNTER_TYPE, READ_ONLY, 19, var_ip_stat_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipAddrTable"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipAddrEntry"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipAdEntAddr"),
 		    COUNTER_TYPE, READ_ONLY, 1, var_ip_addr_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipAdEntIfIndex"),
 		    COUNTER_TYPE, READ_ONLY, 2, var_ip_addr_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipAdEntNetMask"),
 		    COUNTER_TYPE, READ_ONLY, 3, var_ip_addr_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipAdEntBcastAddr"),
 		    COUNTER_TYPE, READ_ONLY, 4, var_ip_addr_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipAdEntReasmMaxSize"),
 		    COUNTER_TYPE, READ_ONLY, 5, var_ip_addr_get, NULL, NULL);

/*==============*/
/* Route stuff  */
/*==============*/

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteTable"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteEntry"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteDest"),
 		    IP_ADDR_PRIM_TYPE, READ_WRITE, 1, var_ip_route_get, 
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteIfIndex"),
 		    INTEGER_TYPE, READ_WRITE, 2, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteMetric1"),
 		    INTEGER_TYPE, READ_WRITE, 3, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteMetric2"),
 		    INTEGER_TYPE, READ_WRITE, 4, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteMetric3"),
 		    INTEGER_TYPE, READ_WRITE, 5, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteMetric4"),
 		    INTEGER_TYPE, READ_WRITE, 6, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteNextHop"),
 		    IP_ADDR_PRIM_TYPE, READ_WRITE, 7, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteType"),
 		    INTEGER_TYPE, READ_WRITE, 8, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteProto"),
 		    INTEGER_TYPE, READ_ONLY, 9, var_ip_route_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteAge"),
 		    INTEGER_TYPE, READ_WRITE, 10, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteMask"),
 		    IP_ADDR_PRIM_TYPE, READ_WRITE, 11, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteMetric5"),
 		    IP_ADDR_PRIM_TYPE, READ_WRITE, 12, var_ip_route_get,
		    var_ip_route_test, var_ip_route_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRouteInfo"),
 		    OBJECT_ID_TYPE, READ_ONLY, 13, var_ip_route_get, NULL, NULL);

/*===================*/
/* NetToMedia stuff  */
/*===================*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipNetToMediaTable"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipNetToMediaEntry"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipNetToMediaIfIndex"),
 		    INTEGER_TYPE, READ_WRITE, 1, var_ip_net_to_media_get,
		    var_ip_net_to_media_test, var_ip_net_to_media_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipNetToMediaPhysAddress"),
 		    OCTET_PRIM_TYPE, READ_WRITE, 2, var_ip_net_to_media_get,
		    var_ip_net_to_media_test, var_ip_net_to_media_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipNetToMediaNetAddress"),
 		    IP_ADDR_PRIM_TYPE, READ_WRITE, 3, var_ip_net_to_media_get,
		    var_ip_net_to_media_test, var_ip_net_to_media_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipNetToMediaType"),
 		    INTEGER_TYPE, READ_WRITE, 4, var_ip_net_to_media_get,
		    var_ip_net_to_media_test, var_ip_net_to_media_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ipRoutingDiscards"),
 		    COUNTER_TYPE, READ_ONLY, 23, var_ip_stat_get, NULL, NULL);

/*=============*/
/* ICMP stuff  */
/*=============*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmp"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInMsgs"),
 		    COUNTER_TYPE, READ_ONLY, 1, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInErrors"),
 		    COUNTER_TYPE, READ_ONLY, 2, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInDestUnreachs"),
 		    COUNTER_TYPE, READ_ONLY, 3, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInTimeExcds"),
 		    COUNTER_TYPE, READ_ONLY, 4, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInParmProbs"),
 		    COUNTER_TYPE, READ_ONLY, 5, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInSrcQuenchs"),
 		    COUNTER_TYPE, READ_ONLY, 6, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInRedirects"),
 		    COUNTER_TYPE, READ_ONLY, 7, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInEchos"),
 		    COUNTER_TYPE, READ_ONLY, 8, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInEchoReps"),
 		    COUNTER_TYPE, READ_ONLY, 9, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInTimestamps"),
 		    COUNTER_TYPE, READ_ONLY, 10, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInTimestampReps"),
 		    COUNTER_TYPE, READ_ONLY, 11, var_icmp_get, NULL, NULL);

#if !defined ULTRIX && !defined SUNOS35
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInAddrMasks"),
 		    COUNTER_TYPE, READ_ONLY, 12, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpInAddrMaskReps"),
 		    COUNTER_TYPE, READ_ONLY, 13, var_icmp_get, NULL, NULL);
#endif

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutMsgs"),
 		    COUNTER_TYPE, READ_ONLY, 14, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutErrors"),
 		    COUNTER_TYPE, READ_ONLY, 15, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutDestUnreachs"),
 		    COUNTER_TYPE, READ_ONLY, 16, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutTimeExcds"),
 		    COUNTER_TYPE, READ_ONLY, 17, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutParmProbs"),
 		    COUNTER_TYPE, READ_ONLY, 18, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutSrcQuenchs"),
 		    COUNTER_TYPE, READ_ONLY, 19, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutRedirects"),
 		    COUNTER_TYPE, READ_ONLY, 20, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutEchos"),
 		    COUNTER_TYPE, READ_ONLY, 21, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutEchoReps"),
 		    COUNTER_TYPE, READ_ONLY, 22, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutTimestamps"),
 		    COUNTER_TYPE, READ_ONLY, 23, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutTimestampReps"),
 		    COUNTER_TYPE, READ_ONLY, 24, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutAddrMasks"),
 		    COUNTER_TYPE, READ_ONLY, 25, var_icmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"icmpOutAddrMaskReps"),
 		    COUNTER_TYPE, READ_ONLY, 26, var_icmp_get, NULL, NULL);

/*=============*/
/*  TCP stuff  */
/*=============*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcp"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpRtoAlgorithm"),
 		    INTEGER_TYPE, READ_ONLY, 1, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpRtoMin"),
 		    INTEGER_TYPE, READ_ONLY, 2, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpRtoMax"),
 		    INTEGER_TYPE, READ_ONLY, 3, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpMaxConn"),
 		    INTEGER_TYPE, READ_ONLY, 4, var_tcp_get, NULL, NULL);
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpActiveOpens"),
 		    COUNTER_TYPE, READ_ONLY, 5, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpPassiveOpens"),
 		    COUNTER_TYPE, READ_ONLY, 6, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpAttemptFails"),
 		    COUNTER_TYPE, READ_ONLY, 7, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpEstabResets"),
 		    COUNTER_TYPE, READ_ONLY, 8, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpCurrEstab"),
 		    GAUGE_TYPE, READ_ONLY, 9, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpInSegs"),
 		    COUNTER_TYPE, READ_ONLY, 10, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpOutSegs"),
 		    COUNTER_TYPE, READ_ONLY, 11, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpRetransSegs"),
 		    COUNTER_TYPE, READ_ONLY, 12, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpInErrs"),
 		    COUNTER_TYPE, READ_ONLY, 14, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpOutRsts"),
 		    COUNTER_TYPE, READ_ONLY, 15, var_tcp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpConnTable"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpConnEntry"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpConnState"),
 		    INTEGER_TYPE, READ_ONLY, 1, var_tcp_conn_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpConnLocalAddress"),
 		    IP_ADDR_PRIM_TYPE, READ_ONLY, 2, var_tcp_conn_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpConnLocalPort"),
 		    INTEGER_TYPE, READ_ONLY, 3, var_tcp_conn_get, NULL, NULL);


    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpConnRemAddress"),
 		    IP_ADDR_PRIM_TYPE, READ_ONLY, 4, var_tcp_conn_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"tcpConnRemPort"),
 		    INTEGER_TYPE, READ_ONLY, 5, var_tcp_conn_get, NULL, NULL);

/*=============*/
/*  UDP stuff  */
/*=============*/

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udp"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpInDatagrams"),
 		    COUNTER_TYPE, READ_ONLY, 1, var_udp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpNoPorts"),
 		    COUNTER_TYPE, READ_ONLY, 2, var_udp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpInErrors"),
 		    COUNTER_TYPE, READ_ONLY, 3, var_udp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpTable"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpEntry"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpOutDatagrams"),
 		    COUNTER_TYPE, READ_ONLY, 4, var_udp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpLocalAddress"),
		    IP_ADDR_PRIM_TYPE, READ_ONLY, 1, var_udp_table_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"udpLocalPort"),
		    INTEGER_TYPE, READ_ONLY, 2, var_udp_table_get, NULL, NULL);

/*=============*/
/*  EGP stuff  */
/*=============*/

/*==============*/
/*  SNMP stuff  */
/*==============*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmp"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInPkts"),
 		    COUNTER_TYPE, READ_ONLY, 1, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutPkts"),
 		    COUNTER_TYPE, READ_ONLY, 2, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInBadVersions"),
 		    COUNTER_TYPE, READ_ONLY, 3, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInBadCommunityNames"),
 		    COUNTER_TYPE, READ_ONLY, 4, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInBadCommunityUses"),
 		    COUNTER_TYPE, READ_ONLY, 5, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInASNParseErrs"),
 		    COUNTER_TYPE, READ_ONLY, 6, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInTooBigs"),
 		    COUNTER_TYPE, READ_ONLY, 8, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInNoSuchNames"),
 		    COUNTER_TYPE, READ_ONLY, 9, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInBadValues"),
 		    COUNTER_TYPE, READ_ONLY, 10, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInReadOnlys"),
 		    COUNTER_TYPE, READ_ONLY, 11, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInGenErrs"),
 		    COUNTER_TYPE, READ_ONLY, 12, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInTotalReqVars"),
 		    COUNTER_TYPE, READ_ONLY, 13, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInTotalSetVars"),
 		    COUNTER_TYPE, READ_ONLY, 14, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInGetRequests"),
 		    COUNTER_TYPE, READ_ONLY, 15, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInGetNexts"),
 		    COUNTER_TYPE, READ_ONLY, 16, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInSetRequests"),
 		    COUNTER_TYPE, READ_ONLY, 17, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInGetResponses"),
 		    COUNTER_TYPE, READ_ONLY, 18, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpInTraps"),
 		    COUNTER_TYPE, READ_ONLY, 19, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutTooBigs"),
 		    COUNTER_TYPE, READ_ONLY, 20, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutNoSuchNames"),
 		    COUNTER_TYPE, READ_ONLY, 21, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutBadValues"),
 		    COUNTER_TYPE, READ_ONLY, 22, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutGenErrs"),
 		    COUNTER_TYPE, READ_ONLY, 24, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutGetRequests"),
 		    COUNTER_TYPE, READ_ONLY, 25, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutGetNexts"),
 		    COUNTER_TYPE, READ_ONLY, 26, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutSetRequests"),
 		    COUNTER_TYPE, READ_ONLY, 27, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutGetResponses"),
 		    COUNTER_TYPE, READ_ONLY, 28, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpOutTraps"),
 		    COUNTER_TYPE, READ_ONLY, 29, var_snmp_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"snmpEnableAuthenTraps"),
 		    INTEGER_TYPE, READ_WRITE, 30, var_snmp_get,
		    var_snmp_test, var_snmp_set);

/*================*/
/*  Other stuff   */
/*================*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"experimental"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"private"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"enterprises"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"unix"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smux"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxPeerTable"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxPeerEntry"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxPindex"),
		    INTEGER_TYPE, READ_ONLY, 1, var_smuxPeer_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxPidentity"),
		    OBJECT_ID_TYPE, READ_ONLY, 2, var_smuxPeer_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxPdescription"),
		    DisplayString, READ_ONLY, 3, var_smuxPeer_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxPstatus"),
		    INTEGER_TYPE, READ_WRITE, 4, var_smuxPeer_get,
		    var_smuxPeer_test, var_smuxPeer_set);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxTreeTable"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxTreeEntry"),
		    Aggregate, NONE, 0, NULL, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxTsubtree"),
		    OBJECT_ID_TYPE, READ_ONLY, 1, var_smuxTree_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxTpriority"),
		    INTEGER_TYPE, READ_ONLY, 2, var_smuxTree_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxTindex"),
		    INTEGER_TYPE, READ_ONLY, 3, var_smuxTree_get, NULL, NULL);

    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"smuxTstatus"),
		    INTEGER_TYPE, READ_WRITE, 4, var_smuxTree_get,
		    var_smuxTree_test, var_smuxTree_set);

#ifdef PPP_MIB
/*================*/
/* ppp  	  */
/*================*/
    var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"ppp"),
 		    Aggregate, NONE, 0, NULL, NULL, NULL);
#endif  /* PPP_MIB */

    return(var_ptr);
}


VarEntry *add_var(VarEntry *var_ptr,
		  OID oid_ptr,
		  unsigned int type,
		  unsigned int rw_flag,
		  unsigned int arg,
		  VarBindList *(*funct_get)(),
		  int (*funct_test_set)(),
		  int (*funct_set)())
{
    int cc;
    VarEntry *new_var_ptr;

    if ((var_ptr == NULL) || 
	((cc = cmp_oid_total(oid_ptr, var_ptr->class_ptr)) <= 0)) {  /* L000 */

	if ((var_ptr != NULL) && (cc == 0)) {	/* ignore duplicates */
	    if (log_level)					     /* L000 */
		printf(gettxt(":234", "Dropping duplicate in add_var (%s).\n"),
			sprintoid(oid_ptr));			     /* L000 */
	    free_oid(oid_ptr);					     /* L000 */
	    return(var_ptr);
	}

	if ((new_var_ptr = (VarEntry *) malloc (sizeof(VarEntry))) == NULL) {
	    syslog(LOG_ERR, gettxt(":95", "add_var: malloc failed: %m"));
	    exit(-1);	/* go ahead and exit 'cause we are not running */
	}

	/* flesh out the entry */
	new_var_ptr->class_ptr = oid_ptr;
	new_var_ptr->type = type;
	new_var_ptr->rw_flag = rw_flag;
	new_var_ptr->arg = arg;
	new_var_ptr->funct_get = funct_get;
	new_var_ptr->funct_test_set = funct_test_set;
	new_var_ptr->funct_set = funct_set;
	new_var_ptr->child = NULL;
	new_var_ptr->sibling = NULL;
	new_var_ptr->smux = NULL;
	new_var_ptr->next = var_ptr; /* slip in front of this one */
	return(new_var_ptr);
    }

    /* else... keep on truckin' */
    var_ptr->next = add_var(var_ptr->next, oid_ptr, type, rw_flag, arg, 
			    funct_get, funct_test_set, funct_set);
    return (var_ptr);
}


int	cmp_oid_class(OID ptr1, OID ptr2)
{
    int i;
    int min = ((ptr1->length < ptr2->length) ? ptr1->length : ptr2->length);

    for (i = 0; i < min; i++) {
	if (ptr1->oid_ptr[i] != ptr2->oid_ptr[i])
	    return (ptr1->oid_ptr[i] - ptr2->oid_ptr[i]);
    }

    return (0);
}


int	cmp_oid_total(OID ptr1, OID ptr2)
{
    int i;

    if ((i = cmp_oid_class (ptr1, ptr2)) == 0)
	return (ptr1->length - ptr2->length);

    return (i);
}

VarEntry *get_exact_var (VarEntry *var1, VarEntry *var2)
{
    if (var1 == NULL)
	return (NULL);

    if (cmp_oid_total (var1->class_ptr, var2->class_ptr) == 0)
	return (var1);

    return (get_exact_var (var1->next, var2));
}


int add_objects_aux (void)
{
    register VarEntry  *var1, *var2;
    for (var1 = var_list_root; var1; var1 = var1->next) {

	OIDentifier	oids;
	VarEntry temp_var;
	static char s1[128], s2[128];

	if (var1->class_ptr->length <= 1)
	    continue;

	var2 = NULL;

	for (oids.oid_ptr = var1->class_ptr->oid_ptr,
		oids.length = var1->class_ptr->length - 1;
		oids.length > 0; oids.length--) {

	    temp_var.class_ptr = &oids;
	    if (var2 = get_exact_var (var_list_root, &temp_var))
		break;
	}

	if (var2) {
	    var1->sibling = var2->child;
	    var2->child = var1;
	}
	else
	    if (log_level)
		printf(gettxt(":96", "No distant parent for %s.\n"),
					sprintoid (var1->class_ptr));
    }
    return (0);

}

int add_objects (register VarEntry   *var)
{
    register OID oid = var->class_ptr;
    register VarEntry  *var2, **var_ptr;

    for (var_ptr = &var_list_root; var2 = *var_ptr;
	    var_ptr = &var2->next)
	if (cmp_oid_total (var2->class_ptr, oid) > 0)
	    break;
    var->next = var2;
    *var_ptr = var;

    for (var = var_list_root; var; var = var->next)
	var->sibling = var->child = NULL;

    return add_objects_aux ();
}

