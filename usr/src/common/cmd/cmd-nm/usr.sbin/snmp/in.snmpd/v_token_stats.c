#ident "@(#)v_token_stats.c	1.2"
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

#define dot5commands  1
#define dot5ringstate  1
#define dot5ringopenstatus  11

extern int curr_query;
static int prev_if_query = -1;
static int prev_if_index = -2;

#define DOT5_IFINDEX			 1
#define DOT5_COMMANDS			 2
#define DOT5_RINGSTATUS			 3
#define DOT5_RINGSTATE			 4
#define DOT5_RINGOPENSTATUS		 5
#define DOT5_RINGSPEED			 6
#define DOT5_UPSTREAM			 7
#define DOT5_ACTMONPARTICIPATE		 8
#define DOT5_FUNCTIONAL			 9

#define DOT5_STATSIFINDEX		10
#define DOT5_STATSLINEERRS		11
#define DOT5_STATSBURSTERRS		12
#define DOT5_STATSACERRS		13
#define DOT5_STATSABORTTRANSERRS	14
#define DOT5_STATSINTERNALERRS		15
#define DOT5_STATSLOSTFRAMEERRS		16
#define DOT5_STATSRECEIVECONGESTIONS	17
#define DOT5_STATSFRAMECOPIEDERRS	18
#define DOT5_STATSTOKENERRS		19
#define DOT5_STATSSOFTERRS		20
#define DOT5_STATSHARDERRS		21
#define DOT5_STATSSIGNALLOSS		22
#define DOT5_STATSTRANSMITBEACONS	23
#define DOT5_STATSRECOVERYS		24
#define DOT5_STATSLOBEWIRES		25
#define DOT5_STATSREMOVE		26
#define DOT5_STATSSINGLES		27
#define DOT5_STATSFREQERRS		28


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
var_token_stats_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	VarEntry *var_next_ptr;
	int type_search;
{
	static int if_num;
	static char ifname[IFNAMSIZ];
	static struct ifreq_all if_all;
	OID  oid_ptr;
	OctetString *os_ptr;
	VarBindList *vbl_ptr;
	char buffer[256];
	int  cc, i, speed;
	int  convert = 0;
	static struct dlpi_stats *dlpi_stats;
	static mac_stats_tr_t	*tr_stats;
	static char *dlpi_buf;

	if_all.if_entry.if_index = 0;

	/* An exact search only has one sub-id field */
	/* for the interface number - else not exact */
	if ((type_search == EXACT) &&
	    (in_name_ptr->length != (var_name_ptr->length + 1))) {
		return NULL;
	}

	/* Find out which interface they're interested in */
	if (in_name_ptr->length > var_name_ptr->length) {
		if_all.if_entry.if_index =
			in_name_ptr->oid_ptr[var_name_ptr->length];
	}

	if (type_search == EXACT && if_all.if_number == 0)
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

	if ((type_search == EXACT) &&
	    (if_all.if_entry.if_index != in_name_ptr->oid_ptr[var_name_ptr->length]))
		return NULL;

	/* Not a token ring interface ??? */
	if (if_all.if_entry.if_type != IFT_ISO88025) {
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
	    (cc = dlpi_get_stats(ifname, NULL, &dlpi_stats,
			&dlpi_buf, convert))) {
		/* No DLPI stats available */
		if (type_search == NEXT)
			return(get_next_class(var_next_ptr));
						/* get the next variable */
		else	/* EXACT */
			return NULL;
	}

	tr_stats = (mac_stats_tr_t *) dlpi_buf;

by_pass:;
	sprintf(buffer, "%s.%d", sprintoid(var_name_ptr), if_num);
	oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

	switch (arg) {
	case DOT5_IFINDEX :
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, if_num, NULL, NULLOID);
		break;

	case DOT5_COMMANDS:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, dot5commands,
				NULL, NULLOID);
		break;

	case DOT5_RINGSTATUS:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, tr_stats->mac_ringstatus,
				NULL, NULLOID);
		break;

	case DOT5_RINGSTATE:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, dot5ringstate,
				NULL, NULLOID);
		break;

	case DOT5_RINGOPENSTATUS:
		/* We break; statuts as always been successful */
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, dot5ringopenstatus,
				NULL, NULLOID);
		break;

	case DOT5_RINGSPEED:
		/* dot5RingSpeed should be : 1, 2, 3, or 4.  */
		/* but not the actual bandwith, enumerate it. */
		if (dlpi_stats->mac_ifspeed == 16777216)
			speed = 4;
		else if (dlpi_stats->mac_ifspeed == 4194304)
			speed = 3;
		else if (dlpi_stats->mac_ifspeed == 1048576)
			speed = 2;
		else
			speed = 1;
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, speed, NULL, NULLOID);
		break;

	case DOT5_UPSTREAM:
		os_ptr = make_octetstring(&tr_stats->mac_upstream[0], 6);
		vbl_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE, 
				0, 0, os_ptr, NULLOID);
		os_ptr = NULL;
		break;

	case DOT5_ACTMONPARTICIPATE:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, tr_stats->mac_actmonparticipate,
				NULL, NULLOID);
		break;

	case DOT5_FUNCTIONAL:
		os_ptr = make_octetstring(&tr_stats->mac_funcaddr[0], 6);
		vbl_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE,
				0, 0, os_ptr, NULLOID);
		os_ptr = NULL;
		break;

	case DOT5_STATSIFINDEX:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, if_num, NULL, NULLOID);
		break;

	case DOT5_STATSLINEERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_lineerrors, 0,
				NULL, NULLOID); 
		break;

	case DOT5_STATSBURSTERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_bursterrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSACERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_acerrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSABORTTRANSERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_aborttranserrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSINTERNALERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_internalerrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSLOSTFRAMEERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_lostframeerrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSRECEIVECONGESTIONS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_receivecongestions, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSFRAMECOPIEDERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_framecopiederrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSTOKENERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_tokenerrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSSOFTERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_softerrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSHARDERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_harderrors, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSSIGNALLOSS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_signalloss, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSTRANSMITBEACONS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_transmitbeacons, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSRECOVERYS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				 tr_stats->mac_recoverys, 0,
				 NULL, NULLOID);
		break;

	case DOT5_STATSLOBEWIRES:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				 tr_stats->mac_lobewires, 0,
				 NULL, NULLOID);
		break;

	case DOT5_STATSREMOVE:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				tr_stats->mac_removes, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSSINGLES:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,  
				tr_stats->mac_statssingles, 0,
				NULL, NULLOID);
		break;

	case DOT5_STATSFREQERRS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,  
				tr_stats->mac_frequencyerrors, 0,
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


/* Token Ring MIB stuff */
VarEntry *
init_token_mib(var_entry_ptr)
	VarEntry *var_entry_ptr;
{
	VarEntry *var_ptr = var_entry_ptr;

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5Table"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5Entry"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5IfIndex"),
			INTEGER_TYPE, READ_ONLY, DOT5_IFINDEX,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5Commands"),
			INTEGER_TYPE, READ_ONLY, DOT5_COMMANDS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5RingStatus"),
			INTEGER_TYPE, READ_ONLY, DOT5_RINGSTATUS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5RingState"),
			INTEGER_TYPE, READ_ONLY, DOT5_RINGSTATE,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5RingOpenStatus"),
			INTEGER_TYPE, READ_ONLY, DOT5_RINGOPENSTATUS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5RingSpeed"),
			INTEGER_TYPE, READ_ONLY, DOT5_RINGSPEED,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5UpStream"),
			OCTET_PRIM_TYPE, READ_ONLY, DOT5_UPSTREAM,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5ActMonParticipate"),
			INTEGER_TYPE, READ_ONLY, DOT5_ACTMONPARTICIPATE,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5Functional"),
			OCTET_PRIM_TYPE, READ_ONLY, DOT5_FUNCTIONAL,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5StatsTable"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5StatsEntry"),
			Aggregate, NONE, 0, GFNULL, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5StatsIfIndex"),
			INTEGER_TYPE, READ_ONLY, DOT5_STATSIFINDEX,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5StatsLineErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSLINEERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsBurstErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSBURSTERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
					(unsigned char *)"dot5StatsACErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSACERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsAbortTransErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSABORTTRANSERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsInternalErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSINTERNALERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsLostFrameErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSLOSTFRAMEERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsReceiveCongestions"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSRECEIVECONGESTIONS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsFrameCopiedErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSFRAMECOPIEDERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsTokenErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSTOKENERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsSoftErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSSOFTERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsHardErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSHARDERRS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsSignalLoss"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSSIGNALLOSS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsTransmitBeacons"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSTRANSMITBEACONS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsRecoverys"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSRECOVERYS,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsLobeWires"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSLOBEWIRES,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsRemove"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSREMOVE,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsSingles"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSSINGLES,
			var_token_stats_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot(
				(unsigned char *)"dot5StatsFreqErrors"),
			COUNTER_TYPE, READ_ONLY, DOT5_STATSFREQERRS,
			var_token_stats_get, TFNULL, SFNULL);

	return(var_ptr);
}
#endif /* _DLPI */
