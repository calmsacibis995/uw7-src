#ident "@(#)v_ether_stats.c	1.2"
#ident "$Header$"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Legent Corporation
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
/*      SCCS IDENTIFICATION        */

/* transmissions group */

#ifdef _DLPI
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/scodlpi.h>
#include <sys/mdi.h>
/* #include <sys/lli31.h> */
#include <net/if_types.h>
#include <net/if.h>

#include "snmp.h"
#include "snmpuser.h"
/* #include <snmp/party.h> */
#include "snmpd.h"
/* #include "proto.h" */

extern int curr_query;
static int prev_if_query = -1;
static int prev_if_index = -2;

#define	DOT3_STATSINDEX				 1
#define	DOT3_STATSALIGNMENTERRS			 2
#define	DOT3_STATSFCSERRS			 3
#define	DOT3_STATSSINGLECOLLISIONERRS		 4
#define	DOT3_STATSMULTIPLECOLLISIONERRS		 5
#define	DOT3_STATSSQTESTERRS			 6
#define	DOT3_STATSDEFERREDTRANS			 7
#define	DOT3_STATSLATECOLLISIONS		 8
#define	DOT3_STATSEXCESSIVECOLLISIONS		 9
#define	DOT3_STATSINTERNALMACTRANSMITERRS	10
#define	DOT3_STATSCARRIERSENSEERRS		11
#define	DOT3_STATSFRAMETOOLONGS			13
#define	DOT3_STATSINTERNALMACRECEIVEERRS	16

VarBindList *get_next_class();

VarEntry *add_var(VarEntry *var_ptr,
                  OID oid_ptr,
                  unsigned int type,
                  unsigned int rw_flag,
                  unsigned int arg,
                  VarBindList *(*funct_get)(),
                  int (*funct_test_set)(),
                  int (*funct_set)());

VarBindList *
var_ether_stats_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	VarEntry *var_next_ptr;
	int type_search;
{
	static int if_num;
	static char ifname[IFNAMSIZ];
	static struct ifreq_all if_all;
	OID  oid_ptr;
	VarBindList *vbl_ptr;
	char buffer[256];
	int cc, i;
	int collisions, convert = 0;
	static struct dlpi_stats *dlpi_stats;
	static mac_stats_eth_t	*eth_stats;
	static char *dlpi_buf;

	/*
	 * Some of the following code has been borrowed from
	 * the var_interfaces_get function if v_ifEntry.c.
	 */

	if_all.if_entry.if_index = 0;

	/* An exact search only has one sub-id field */
	/* for the interface number - else not exact */
	if ((type_search == EXACT) &&
	    (in_name_ptr->length != (var_name_ptr->length + 1)))
		return NULL;

	/* Find out which interface they're interested in */
	if (in_name_ptr->length > var_name_ptr->length)
		if_all.if_entry.if_index = in_name_ptr->oid_ptr[var_name_ptr->length];

	if (type_search == EXACT && if_all.if_entry.if_index == 0)
		return NULL;

	/* If a get-next, then get the NEXT interface (+1)  */
	if (type_search == NEXT) {
reget:
		if_all.if_entry.if_index++;
		/* wrapped around to zero ??? */
		if (if_all.if_entry.if_index == 0)
			return(get_next_class(var_next_ptr));
						/* get the next variable */
	}

	if ((curr_query == prev_if_query) &&
	    (prev_if_index == if_all.if_entry.if_index)) {
		goto by_pass;
	}

	cc = get_if_all(&if_all.if_entry.if_index,&if_all);

	if (cc == FALSE) {
		if (type_search == NEXT)
			return(get_next_class(var_next_ptr));
						/* get the next variable */
		else	/* EXACT */
			return NULL;
	}

	/* Not an Ethernet interface ??? */
	if (if_all.if_entry.if_type != IFT_ISO88023) {
		if (type_search == NEXT)
			goto reget;	/* get the next variable */
		else	/* EXACT */
			return NULL;
	}

	prev_if_query = curr_query;
	prev_if_index = if_all.if_entry.if_index;

	if_num = if_all.if_entry.if_index;

	bcopy((char *)(if_all.if_entry.if_name), (char *)ifname, IFNAMSIZ);
	ifname[IFNAMSIZ - 1] = '\0';

	if ((if_all.if_entry.if_dl_version < 1) ||
	    (cc = dlpi_get_stats(ifname, NULL, &dlpi_stats, &dlpi_buf, 
			convert))) {
		/* No DLPI stats available */
		if (type_search == NEXT)
			return(get_next_class(var_next_ptr));
						/* get the next variable */
		else	/* EXACT */
			return NULL;
	}

	eth_stats = (mac_stats_eth_t *) dlpi_buf;

by_pass:;
	sprintf(buffer, "%s.%d", sprintoid(var_name_ptr), if_num);
	oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

	switch (arg) {
	case DOT3_STATSINDEX:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, if_num, NULL, NULLOID);
		break;

	case DOT3_STATSALIGNMENTERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				eth_stats->mac_align, 0, NULL, NULLOID);
		break;

	case DOT3_STATSFCSERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				eth_stats->mac_badsum, 0, NULL, NULLOID);
		break;

	case DOT3_STATSSINGLECOLLISIONERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				eth_stats->mac_colltable[0], 0, NULL, NULLOID);
		break;

	case DOT3_STATSMULTIPLECOLLISIONERRS:
		collisions = 0;
		/* More than one collision */
                for (i = 1; i < 16; i++) {
                       collisions += (i+1) * eth_stats->mac_colltable[i];
                }

		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				collisions, 0, NULL, NULLOID);
		break;

	case DOT3_STATSSQTESTERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				0, eth_stats->mac_sqetesterrors, NULL, NULLOID);
		break;

	case DOT3_STATSDEFERREDTRANS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				eth_stats->mac_frame_def, 0,
				NULL, NULLOID);
		break;

	case DOT3_STATSLATECOLLISIONS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,  
				eth_stats->mac_oframe_coll, 0,
				NULL, NULLOID);
		break;

	case DOT3_STATSEXCESSIVECOLLISIONS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,  
				eth_stats->mac_xs_coll, 0,
				NULL, NULLOID);
		break;

	case DOT3_STATSINTERNALMACTRANSMITERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,  
				eth_stats->mac_tx_errors, 0,
				NULL, NULLOID);
		break;

	case DOT3_STATSCARRIERSENSEERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE, 
				eth_stats->mac_carrier, 0,
				NULL, NULLOID); 
		break;

	case DOT3_STATSFRAMETOOLONGS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE, 
				eth_stats->mac_badlen, 0,
				NULL, NULLOID);
		break;

	case DOT3_STATSINTERNALMACRECEIVEERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,  
				eth_stats->mac_no_resource, 0,
				NULL, NULLOID);
		break;

	default:
		(void) free_oid(oid_ptr);
		if (type_search == EXACT)
			return NULL;
		else
			return(get_next_class(var_next_ptr));
						/* get the next variable */
	}
	oid_ptr = NULLOID;
	return(vbl_ptr);
}

static int CollCount = 16;

#define DOT3_COLLINDEX		1
#define DOT3_COLLCOUNT		2
#define DOT3_COLLFREQUENCIES	3


VarBindList *
var_ether_coll_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	VarEntry *var_next_ptr;
	int type_search;
{
	static int if_num;
	static char ifname[IFNAMSIZ];
	static struct ifreq_all if_all;
	OID  oid_ptr;
	VarBindList *vbl_ptr;
	char buffer[256];
	int  cc, convert = 0;
	static u_int coll_num;
	static struct dlpi_stats *dlpi_stats;
	static mac_stats_eth_t	*eth_stats;
	static char *dlpi_buf;

	/*
	 * Some of the following code has been borrowed from
	 * the var_interfaces_get() function in v_ifEntry.c
	 */

	if_all.if_entry.if_index = 0;
	coll_num = 0;

	/* An exact search has two sub-id fields */
	if ((type_search == EXACT) &&
	    (in_name_ptr->length != (var_name_ptr->length + 2)))
		return NULL;

	/* Find out which interface they're interested in */
	if (in_name_ptr->length > var_name_ptr->length) {
		if_all.if_entry.if_index =
			in_name_ptr->oid_ptr[var_name_ptr->length];
	}

	/* Find out which collision index they are interested in */
	if (in_name_ptr->length > var_name_ptr->length + 1) {
		coll_num = in_name_ptr->oid_ptr[var_name_ptr->length + 1];
	}

	if (type_search == NEXT) { 
		if ((coll_num == 0) || (coll_num == CollCount)) {
reget1:
			if_all.if_entry.if_index++;	
			/* wrapped around to zero ??? */
			if (if_all.if_entry.if_index == 0)
				return(get_next_class(var_next_ptr));
						/* get the next variable */
			coll_num = 1;
		}
		else {
			coll_num++;
			/* wrapped around to zero ??? */
			if (coll_num == 0)
				return(get_next_class(var_next_ptr));
						/* get the next variable */
		}
	}

	if ((type_search == EXACT &&
	    ((if_all.if_entry.if_index == 0) ||
	     (coll_num == 0) ||
	     (coll_num > CollCount))))
		return NULL;

	if ((curr_query == prev_if_query) &&
	    (prev_if_index == if_all.if_entry.if_index)) {
		goto by_pass;
	}

	cc = get_if_all(&if_all.if_entry.if_index,&if_all);
	if (cc == FALSE) {
		if (type_search == NEXT)
			return(get_next_class(var_next_ptr));
						/* get the next variable */
		else	/* EXACT */
			return NULL;
	}

	/* Not an Ethernet interface ??? */
	if (if_all.if_entry.if_type != IFT_ISO88023) {
		if (type_search == NEXT)
			goto reget1;	/* get the next variable */
		else	/* EXACT */
			return NULL;
	}

	prev_if_query = curr_query;
	prev_if_index = if_all.if_entry.if_index;

	if_num = if_all.if_entry.if_index;

	bcopy((char *)(if_all.if_entry.if_name), (char *)ifname, IFNAMSIZ);
	ifname[IFNAMSIZ - 1] = '\0';

	if ((if_all.if_entry.if_dl_version < 1) ||
	    (cc = dlpi_get_stats(ifname, NULL, &dlpi_stats, &dlpi_buf,
			convert))) {
		/* No DLPI stats available */
		if (type_search == NEXT)
			return(get_next_class(var_next_ptr));	/* get the next variable */
		else	/* EXACT */
			return NULL;
	}

	eth_stats = (mac_stats_eth_t *)dlpi_buf;

by_pass:;
	sprintf(buffer, "%s.%d.%d",
		sprintoid(var_name_ptr), if_num, coll_num);
	oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

	switch (arg) {
	case DOT3_COLLINDEX:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, if_num, NULL, NULLOID);
		break;

	case DOT3_COLLCOUNT:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,  
				0, coll_num, NULL, NULLOID);
		break;

	case DOT3_COLLFREQUENCIES:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE, 
				eth_stats->mac_colltable[coll_num - 1 ], 0,
				NULL, NULLOID);
		break;

	default:
		if (type_search == EXACT) {
			return NULL;
		}
		else
			return(get_next_class(var_next_ptr));
						/* get the next variable */
	}
	oid_ptr = NULLOID;
	return(vbl_ptr);
}


/* Ethernet MIB stuff */
VarEntry *
init_ether_mib(var_entry_ptr)
	VarEntry *var_entry_ptr;
{
	VarEntry *var_ptr = var_entry_ptr;

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3StatsTable"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3StatsEntry"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3StatsIndex"),
			INTEGER_TYPE, READ_ONLY, DOT3_STATSINDEX,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
				(unsigned char *)"dot3StatsAlignmentErrors"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSALIGNMENTERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3StatsFCSErrors"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSFCSERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
			(unsigned char *)"dot3StatsSingleCollisionFrames"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSSINGLECOLLISIONERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
			(unsigned char *)"dot3StatsMultipleCollisionFrames"),
			COUNTER_TYPE, READ_ONLY,
			DOT3_STATSMULTIPLECOLLISIONERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
				(unsigned char *)"dot3StatsSQETestErrors"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSSQTESTERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
			(unsigned char *)"dot3StatsDeferredTransmissions"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSDEFERREDTRANS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
				(unsigned char *)"dot3StatsLateCollisions"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSLATECOLLISIONS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
			(unsigned char *)"dot3StatsExcessiveCollisions"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSEXCESSIVECOLLISIONS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
			(unsigned char *)"dot3StatsInternalMacTransmitErrors"),
			COUNTER_TYPE, READ_ONLY,
			DOT3_STATSINTERNALMACTRANSMITERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
			(unsigned char *)"dot3StatsCarrierSenseErrors"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSCARRIERSENSEERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
				(unsigned char *)"dot3StatsFrameTooLongs"),
			COUNTER_TYPE, READ_ONLY, DOT3_STATSFRAMETOOLONGS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot(
			(unsigned char *)"dot3StatsInternalMacReceiveErrors"),
			COUNTER_TYPE, READ_ONLY,
			DOT3_STATSINTERNALMACRECEIVEERRS,
			var_ether_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3CollTable"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3CollEntry"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3CollIndex"),
			INTEGER_TYPE, READ_ONLY, DOT3_COLLINDEX,
			var_ether_coll_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3CollCount"),
			INTEGER_TYPE, READ_ONLY, DOT3_COLLCOUNT,
			var_ether_coll_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot3CollFrequencies"),
			COUNTER_TYPE, READ_ONLY, DOT3_COLLFREQUENCIES,
			var_ether_coll_get, TFNULL, SFNULL);

	return(var_ptr);
}
#endif /* _DLPI */
