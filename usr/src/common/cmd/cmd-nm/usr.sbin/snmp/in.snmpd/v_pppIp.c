#ident	"@(#)v_pppIp.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

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

#ifdef PPP_MIB
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/time.h>
#include <net/if.h>
#if defined(M_UNIX)
#include <sys/scodlpi.h>
#include <sys/mdi.h>
#endif
#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <sys/ioctl.h>
#include <netinet/in_var.h>
#include <sys/stropts.h>
#include <net/route.h>


#include <syslog.h>

#include "snmp.h"
#include "snmpuser.h"
#include <netinet/ppp_lock.h>

#include <netinet/ppp.h>
#include "snmpd.h"

extern int curr_query;
static int prev_if_query = -1;
static int prev_if_index = -2;


#define PPP_IPOPERSTATUS		1
#define PPP_IPLOCALTOREMOTECOMPRPROTO	2
#define PPP_IPREMOTETOLOCALCOMPRPROTO	3
#define PPP_IPREMOTEMAXSLOTID		4
#define PPP_IPLOCALMAXSLOTID		5

VarBindList *get_next_class();

VarEntry *add_var(VarEntry *var_ptr,
                  OID oid_ptr,
                  unsigned int type,
                  unsigned int rw_flag,
                  unsigned int arg,
                  VarBindList *(*funct_get)(),
                  int (*funct_test_set)(),
                  int (*funct_set)());


/*
 * Retrieve contents of the PPP IP Table. It contains IP
 * parameters and statistics. An entry in this
 * table corresponding to a PPP interface is identified by
 * an ifIndex value.
 */

VarBindList *
var_ppp_ip_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	int type_search;
{
	VarBindList *vbl_ptr;
	OctetString *os_ptr;
	OID  oid_ptr;
	char buffer[256];
	int  cc;
	static struct ipstatus ppp_ip;
	u_long if_idx, if_num;

	if_num = 0;

	/* An exact search only has one sub-id field */
	/* for the interface number - else not exact */
	if ((type_search == EXACT) &&
	    (in_name_ptr->length != (var_name_ptr->length + 1))) {
no_instance:;
		oid_ptr = oid_cpy(in_name_ptr);
		vbl_ptr = make_varbind(oid_ptr, NULL_TYPE,
				0, 0, NULL, NULLOID);
		oid_ptr = NULLOID;
		return(vbl_ptr);
	}

	/* Find out which interface they're interested in */
	if (in_name_ptr->length > var_name_ptr->length)
		if_num = in_name_ptr->oid_ptr[var_name_ptr->length];

	if ((type_search == EXACT) && (if_num == 0))
		goto no_instance;

	/* If a get-next, then get the NEXT interface (+1)  */
	if (type_search == NEXT) {
		if_num++;
		if (if_num == 0)
		        return(NULL);
	}

	if_idx = if_num;



	cc = get_ppp_ip_table(&ppp_ip, &if_idx);

	if (cc == FALSE) {
		if (type_search == NEXT)
		        return(get_next_class(var_next_ptr));
		else	/* EXACT */ 
			goto no_instance;
	}
	if ((type_search == EXACT) && (if_idx != if_num))
		goto no_instance;

	prev_if_query = curr_query;
	prev_if_index = if_num;

by_pass:;
	sprintf(buffer, "%s.%d", sprintoid(var_name_ptr), if_idx);
	oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

	switch (arg) {
	case PPP_IPOPERSTATUS:
		sprintf(buffer, "pppIpOperStatus.%d", if_num);
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_ip.OperStatus, NULL, NULLOID);
		break;

	case PPP_IPLOCALTOREMOTECOMPRPROTO:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_ip.LocalComp, NULL, NULLOID);
		break;

	case PPP_IPREMOTETOLOCALCOMPRPROTO:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_ip.RemoteComp, NULL, NULLOID);
		break;

	case PPP_IPREMOTEMAXSLOTID:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_ip.RemoteSlot, NULL, NULLOID);
		break;

	case PPP_IPLOCALMAXSLOTID:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_ip.LocalSlot, NULL, NULLOID);
		break;

	default:
		(void) free_oid(oid_ptr);
		if (type_search == EXACT) {
			oid_ptr = oid_cpy(in_name_ptr);
			vbl_ptr = make_varbind(oid_ptr, NULL_TYPE,
					0, 0, NULL, NULLOID);
			break;
		}
		else
			return(get_next_class(var_next_ptr));	/* get the next variable */

	}
	oid_ptr = NULLOID;
	return(vbl_ptr);
}


#define PPP_IPCONFIGADMINSTATUS   1
#define PPP_IPCONFIGCOMPRESSION   2

/*
 * Retrieve contents of the PPP IP Config. Table. It contains IP
 * config. parameters. An entry in this table corresponding to a
 * PPP interface is identified by an ifIndex value.
 */

VarBindList *
var_ppp_ip_config_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	int type_search;
{
	VarBindList *vbl_ptr;
	OctetString *os_ptr;
	OID  oid_ptr;
	char buffer[256];
	int  cc;
	static struct ipconfig ppp_ip_conf;
	u_long if_idx, if_num;

	if_num = 0;

	/* An exact search only has one sub-id field */
	/* for the interface number - else not exact */
	if ((type_search == EXACT) &&
	    (in_name_ptr->length != (var_name_ptr->length + 1))) {
no_instance:;
		oid_ptr = oid_cpy(in_name_ptr);
		vbl_ptr = make_varbind(oid_ptr, NULL_TYPE,
				0, 0, NULL, NULLOID);
		oid_ptr = NULLOID;
		return(vbl_ptr);
	}

	/* Find out which interface they're interested in */
	if (in_name_ptr->length > var_name_ptr->length)
		if_num = in_name_ptr->oid_ptr[var_name_ptr->length];

	if ((type_search == EXACT) && (if_num == 0))
		goto no_instance;

	/* If a get-next, then get the NEXT interface (+1)  */
	if (type_search == NEXT) {
		if_num++;
		if (if_num == 0)
			return(NULL);	/* get the next variable */
	}

	if_idx = if_num;


	cc = get_ppp_ip_config_table(&ppp_ip_conf, &if_idx);

	if (cc == FALSE) {
		if (type_search == NEXT)
		        return(get_next_class(var_next_ptr));
		else	/* EXACT */ 
			goto no_instance;
	}
	if ((type_search == EXACT) && (if_idx != if_num))
		goto no_instance;

	prev_if_query = curr_query;
	prev_if_index = if_num;

by_pass:;
	sprintf(buffer, "%s.%d", sprintoid(var_name_ptr), if_idx);
	oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);

	switch (arg) {
	case PPP_IPCONFIGADMINSTATUS:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_ip_conf.AdmStatus,
				NULL, NULLOID);
		break;

	case PPP_IPCONFIGCOMPRESSION:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_ip_conf.ConfComp,
				NULL, NULLOID);
		break;

	default:
		(void) free_oid(oid_ptr);
		if (type_search == EXACT) {
			oid_ptr = oid_cpy(in_name_ptr);
			vbl_ptr = make_varbind(oid_ptr, NULL_TYPE,
					0, 0, NULL, NULLOID);
			break;
		}
		else
			return(get_next_class(var_next_ptr));	/* get the next variable */
	}
	oid_ptr = NULLOID;
	return(vbl_ptr);
}

/*
 * Check validity of values that objects
 * in the PPP IP Config. Table can take.
 */

int
var_ppp_ip_config_test(var_name_ptr, in_name_ptr, arg, value, error_status)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	ObjectSyntax *value;
	int  *error_status;
{
	int  cc;
	u_long if_idx, if_num;
	struct ipconfig ppp_ip_config;

	/* Check that an exact search only has one sub-id */
	/* field for if number - else not valid */
	if (in_name_ptr->length != (var_name_ptr->length + 1)) {
		return(FALSE);
	}
 
	/* Now find out which interface they are interested in */
	if_num = in_name_ptr->oid_ptr[var_name_ptr->length];

	if (!if_num) {
		return(FALSE);		/* invalid ifIndex	*/
	}

	if_idx = if_num;
	cc = get_ppp_ip_config_table(&ppp_ip_config, &if_num);
	if ((cc == FALSE) || (if_num != if_idx)) {
		return(FALSE);
	}

	if ((arg != PPP_IPCONFIGADMINSTATUS) &&
	    (arg != PPP_IPCONFIGCOMPRESSION)) {
		return(FALSE);
	}

	if ((value->sl_value < 1) || (value->sl_value > 2)) {
		return(FALSE);
	}

	return(TRUE);
}

/*
 * Performs SET operations on PPP IP Config. table objects. 
 * Return TRUE/FALSE indicating result of the SET operation.
 */

int
var_ppp_ip_config_set(var_name_ptr, in_name_ptr, arg, value, error_status, action)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	ObjectSyntax *value;
	int  *error_status, action;
{
	u_long if_num;
	struct tablewr table;
#ifdef WRITE_MIB
	if (action == ROLLBACK) {
		return(TRUE);
	}

	if (action == COMMIT) {
		if (in_name_ptr->length != (var_name_ptr->length + 1)) {

			*error_status = NO_CREATION;
			return(FALSE);
		}
		if ((arg != PPP_IPCONFIGADMINSTATUS) &&
		    (arg != PPP_IPCONFIGCOMPRESSION)) {
			*error_status = NO_ACCESS;
			return(FALSE);
		}

		/* Find out which interface they're interested in */
		if_num = in_name_ptr->oid_ptr[var_name_ptr->length];

		table.ifIndex = if_num;
		table.entry = arg - 1;
		table.mibval = value->sl_value;

		if (set_ppp_table(SIOCSIPC, &table) == NOTOK) {
			*error_status = COMMIT_FAILED;
			return(FALSE);
		}
		else
			return(TRUE);
	}
	/* shouldn't be called with any other value for action */
	*error_status = GEN_ERROR;
#endif
	return(FALSE);
}

int
get_ppp_ip_table(ppp_ip, if_index)
	struct ipstatus *ppp_ip;
	u_long *if_index;
{
	struct tablerd rdbuf;

	rdbuf.ifIndex = *if_index;

	if ((get_ppp_table(SIOCGIPS, &rdbuf)) < 0)
		return(FALSE);

	bcopy(&rdbuf.mibips, ppp_ip, sizeof(struct ipstatus));
	*if_index = rdbuf.ifIndex;
	return(TRUE);
}

int
get_ppp_ip_config_table(ppp_ip_conf, if_index)
	struct ipconfig *ppp_ip_conf;
	u_long *if_index;
{
	struct tablerd rdbuf;

	rdbuf.ifIndex = *if_index;

	if ((get_ppp_table(SIOCGIPC, &rdbuf)) < 0)
		return(FALSE);

	bcopy(&rdbuf.mibipc, ppp_ip_conf, sizeof(struct ipconfig));
	*if_index = rdbuf.ifIndex;
	return(TRUE);
}


/*
 * An init-function that adds PPP-IP objects to the
 * linked list of objects pointed to by 'var_entry_ptr'.
 */

VarEntry *
init_ppp_ip_mib(var_entry_ptr)
	VarEntry *var_entry_ptr;
{
	VarEntry *var_ptr = var_entry_ptr;

	/*==================================*/
	/*  PPP/IP-related variables 	    */
	/*==================================*/

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIp"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);
	/*==========================================*/
	/*  PPP/IP table-related variables 	    */
	/*==========================================*/
	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIpTable"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIpEntry"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIpOperStatus"),
			INTEGER_TYPE, READ_ONLY, PPP_IPOPERSTATUS,
			var_ppp_ip_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppIpLocalToRemoteCompressionProtocol"),
			INTEGER_TYPE, READ_ONLY, PPP_IPLOCALTOREMOTECOMPRPROTO,
			var_ppp_ip_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppIpRemoteToLocalCompressionProtocol"),
			INTEGER_TYPE, READ_ONLY, PPP_IPREMOTETOLOCALCOMPRPROTO,
			var_ppp_ip_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIpRemoteMaxSlotId"),
			INTEGER_TYPE, READ_ONLY, PPP_IPREMOTEMAXSLOTID,
			var_ppp_ip_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIpLocalMaxSlotId"),
			INTEGER_TYPE, READ_ONLY, PPP_IPLOCALMAXSLOTID,
			var_ppp_ip_get, TFNULL, SFNULL);

	/*==========================================*/
	/*  PPP/IP Config. table-related variables  */
	/*==========================================*/
	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIpConfigTable"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppIpConfigEntry"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppIpConfigAdminStatus"),
			INTEGER_TYPE, READ_WRITE, PPP_IPCONFIGADMINSTATUS,
			var_ppp_ip_config_get, var_ppp_ip_config_test,
			var_ppp_ip_config_set);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppIpConfigCompression"),
			INTEGER_TYPE, READ_WRITE, PPP_IPCONFIGCOMPRESSION,
			var_ppp_ip_config_get, var_ppp_ip_config_test,
			var_ppp_ip_config_set);

	return(var_ptr);
}
#endif /* PPP_MIB */
