#ident	"@(#)variables.h	1.3"
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
 */

/*      @(#)variables.h	1.3

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 *  Revision History:
 *
 *  11/8/89 JDC
 *  Make it print pretty via tgrind
 *
 */


/* system */
VarBindList *var_system_get();
int var_system_test();
int var_system_set();

/* interface */
VarBindList *var_if_num_get();
#ifdef notdef /* dme - was NEW_MIB */
VarBindList *var_interfaces_get();
int var_if_adminstatus_test();
int var_if_adminstatus_set();
#else
VarBindList *var_if_index_get();
VarBindList *var_if_name_get();
VarBindList *var_if_type_get();
VarBindList *var_if_mtu_get();
VarBindList *var_if_speed_get();
VarBindList *var_if_physaddr_get();
VarBindList *var_if_adminstatus_get();

int var_if_adminstatus_test();
int var_if_adminstatus_set();
VarBindList *var_if_operstatus_get();
VarBindList *var_if_up_time_get();
VarBindList *var_if_inucast_get();
VarBindList *var_if_inerrors_get();
VarBindList *var_if_outucast_get();
VarBindList *var_if_outerrors_get();
VarBindList *var_if_outqlen_get();
VarBindList *var_if_innucast_get();
VarBindList *var_if_inoctets_get();
VarBindList *var_if_indiscards_get();
VarBindList *var_if_inunkprotos_get();
VarBindList *var_if_outnucast_get();
VarBindList *var_if_outoctets_get();
VarBindList *var_if_outdiscards_get();
VarBindList *var_if_specific_get();
#endif /* NEW_MIB */

/* AT */
VarBindList *var_at_get();
int var_at_test();
int var_at_set();

/* ip */
VarBindList *var_ip_stat_get();
int var_ip_stat_test();
int var_ip_stat_set();
VarBindList *var_ip_addr_get();
VarBindList *var_ip_route_get();
int var_ip_route_test();
int var_ip_route_set();
VarBindList *var_ip_net_to_media_get();
int var_ip_net_to_media_test();
int var_ip_net_to_media_set();

/* icmp */
VarBindList *var_icmp_get();


/* udp */
VarBindList *var_udp_get();
VarBindList *var_udp_table_get();

/* tcp */
VarBindList *var_tcp_get();
VarBindList *var_tcp_conn_get();

/* snmp */
VarBindList *var_snmp_get();
int var_snmp_test();
int var_snmp_set();

/* smux */
VarBindList *var_smuxPeer_get();
int var_smuxPeer_test();
int var_smuxPeer_set();
VarBindList *var_smuxTree_get();
int var_smuxTree_test();
int var_smuxTree_set();
