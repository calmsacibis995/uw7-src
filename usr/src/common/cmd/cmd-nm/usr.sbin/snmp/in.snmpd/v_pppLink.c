#ident	"@(#)v_pppLink.c	1.2"
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
#include <netinet/ip_str.h>

#include <syslog.h>

#include "snmp.h"
#include "snmpuser.h"
#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>
#include "snmpd.h"


#define _PATH_PPP "/dev/ppp"
#define snmp_log  syslog

#define	PPP_ACCMAP_SIZE	sizeof(u_long)

extern int curr_query;
static int prev_if_query = -1;
static int prev_if_index = -2;


#define PPP_LINKSTATPHYSICALINDEX		 1
#define PPP_LINKSTATBADADDRESS			 2
#define PPP_LINKSTATBADCONTROLS			 3
#define PPP_LINKSTATPACKETTOOLONGS		 4
#define PPP_LINKSTATBADFCSS			 5
#define PPP_LINKSTATLOCALMRU			 6
#define PPP_LINKSTATREMOTEMRU			 7
#define PPP_LINKSTATLOCALTOPEERACCMAP		 8
#define PPP_LINKSTATPEERTOLOCALACCMAP		 9
#define PPP_LINKSTATLOCALTOREMOTEPROTOCOMPR	10
#define PPP_LINKSTATREMOTETOLOCALPROTOCOMPR	11
#define PPP_LINKSTATLOCALTOREMOTEPROTOACCCOMPR	12
#define PPP_LINKSTATREMOTETOLOCALPROTOACCCOMPR	13
#define PPP_LINKSTATTRANSMITFCSSIZE		14
#define PPP_LINKSTATRECEIVEFCSSIZE		15

VarBindList *get_next_class();

VarEntry *add_var(VarEntry *var_ptr,
                  OID oid_ptr,
                  unsigned int type,
                  unsigned int rw_flag,
                  unsigned int arg,
                  VarBindList *(*funct_get)(),
                  int (*funct_test_set)(),
                  int (*funct_set)());

void reverse(u_char *str, int size);

/*
 * Retrieve contents of Link Status Table for a PPP interface
 * given an ifIndex value. All objects in this table are readOnly.
 */

static int ppp_fd = -1;
static int didit = 0;

VarBindList *
var_ppp_link_stat_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	int type_search;
{
	VarBindList *vbl_ptr;
	OctetString *os_ptr;
	OID  oid_ptr;
	char buffer[256];
	int  cc;
	static struct linkstatus ppp_link_status;
	u_char accmap[PPP_ACCMAP_SIZE];
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
#ifndef SRV4
			return(NULL);	/* get the next variable */
#else
			return(NULLVB);	/* get the next variable */
#endif
	}

	if_idx = if_num;

	cc = get_ppp_link_status_table(&ppp_link_status, &if_idx);

	if (cc == FALSE) {
		if (type_search == NEXT)
			return(get_next_class(var_next_ptr));	/* get the next variable */
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
	case PPP_LINKSTATPHYSICALINDEX:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.PhyIndex,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATBADADDRESS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				ppp_link_status.BadAddr, 0,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATBADCONTROLS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				ppp_link_status.BadCtrl, 0,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATPACKETTOOLONGS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				ppp_link_status.TooLong, 0,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATBADFCSS:
		vbl_ptr = make_varbind(oid_ptr, COUNTER_TYPE,
				ppp_link_status.BadFcs, 0,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATLOCALMRU:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.LocalMRU,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATREMOTEMRU:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.RemoteMRU,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATLOCALTOPEERACCMAP:
		/*
		 * We need to reverse contents of the ACC Map.
		 */
		bcopy((u_char *)&ppp_link_status.LocalACCM,
			accmap, sizeof(u_long));
		reverse(accmap, sizeof(accmap));
		os_ptr = make_octetstring(accmap, sizeof(u_long));
		vbl_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE,
				0, 0, os_ptr, NULLOID);
		break;

	case PPP_LINKSTATPEERTOLOCALACCMAP:
		bcopy((u_char *)&ppp_link_status.RemoteACCM,
			accmap, sizeof(u_long));
		reverse(accmap, sizeof(accmap));
		os_ptr = make_octetstring(accmap, sizeof(accmap));
		vbl_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE,
				0, 0, os_ptr, NULLOID);
		break;

	case PPP_LINKSTATLOCALTOREMOTEPROTOCOMPR:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.LocalProtComp,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATREMOTETOLOCALPROTOCOMPR:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.RemoteProtComp,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATLOCALTOREMOTEPROTOACCCOMPR:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.LocalACComp,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATREMOTETOLOCALPROTOACCCOMPR:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.RemoteACComp,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATTRANSMITFCSSIZE:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.TrFcsSize,
				NULL, NULLOID);
		break;

	case PPP_LINKSTATRECEIVEFCSSIZE:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_status.ReFcsSize,
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


#define PPP_LINKCONFIGINITIALMRU	1
#define PPP_LINKCONFIGRECEIVEACCMAP	2
#define PPP_LINKCONFIGTRANSMITACCMAP	3
#define PPP_LINKCONFIGMAGICNUMBER	4
#define PPP_LINKCONFIGFCSSIZE		5

/*
 * Retrieve contents of Link Config Table for a
 * PPP interface identified by an ifIndex value.
 */

VarBindList *
var_ppp_link_config_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	int type_search;
{
	OctetString *os_ptr;
	VarBindList *vbl_ptr;
	OID  oid_ptr;
	char buffer[256];
	int  cc;
	static struct linkconfig ppp_link_config;
	u_char accmap[PPP_ACCMAP_SIZE];
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

	/* If a get-next, then get the NEXT interface (+1)  */
	if (type_search == NEXT) {
		if_num++;
		if (if_num == 0)
			return(NULL);	/* get the next variable */
	}

	if ((type_search == EXACT) && (if_num == 0))
		goto no_instance;

	if_idx = if_num;


	cc = get_ppp_link_config_table(&ppp_link_config, &if_idx);

	if (cc == FALSE) {
		if (type_search == NEXT)
			return(get_next_class(var_next_ptr));	/* get the next variable */
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
	case PPP_LINKCONFIGINITIALMRU:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_config.InitialMRU,
				NULL, NULLOID);
		break;

	case PPP_LINKCONFIGRECEIVEACCMAP:
		bcopy((u_char *)&ppp_link_config.ReceiveACCM,
			accmap, sizeof(ulong));
		/* We need to reverse contents of the ACC Map */
		reverse(accmap, sizeof(accmap));
		os_ptr = make_octetstring(accmap, sizeof(accmap));
		vbl_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE,
				0, 0, os_ptr, NULLOID);
		os_ptr = NULL;
		break;

	case PPP_LINKCONFIGTRANSMITACCMAP:
		bcopy((u_char *)&ppp_link_config.TransmitACCM,
			accmap, sizeof(ulong));
		/* We need to reverse contents of the ACC Map */
		reverse(accmap, sizeof(accmap));
		os_ptr = make_octetstring(accmap, sizeof(accmap));
		vbl_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE,
				0, 0, os_ptr, NULLOID);
		os_ptr = NULL;
		break;

	case PPP_LINKCONFIGMAGICNUMBER:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_config.MagicNum,
				NULL, NULLOID);
		break;

	case PPP_LINKCONFIGFCSSIZE:
		vbl_ptr = make_varbind(oid_ptr, INTEGER_TYPE,
				0, ppp_link_config.ConfFcsSize,
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
 * Check validity of values that objects in the
 * PPP Link Config. Table can take.
 */

int
var_ppp_link_config_test(var_name_ptr, in_name_ptr, arg, value, error_status)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	ObjectSyntax *value;
	int  *error_status;
{
	int    cc;
	u_long if_idx, if_num;
	struct linkconfig ppp_link_config;

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
	cc = get_ppp_link_config_table(&ppp_link_config, &if_num);

	if ((cc == FALSE) || (if_num != if_idx)) {
		return(FALSE); 	/* No such interface? */
	}

	switch (arg) {
		case PPP_LINKCONFIGINITIALMRU:
			if (value->sl_value < 0) {
				return(FALSE);
			}
			break;

		case PPP_LINKCONFIGRECEIVEACCMAP:
		case PPP_LINKCONFIGTRANSMITACCMAP:
			if (value->os_value->length != PPP_ACCMAP_SIZE) {
				return(FALSE);
			}
			break;

		case PPP_LINKCONFIGMAGICNUMBER:
			if ((value->sl_value < 1) || (value->sl_value > 2)) {
				return(FALSE);
			}
			break;

		case PPP_LINKCONFIGFCSSIZE:
			if ((value->sl_value < 0) || (value->sl_value > 128)) {
				return(FALSE);
			}
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}


/*
 * Performs SET operations on PPP Link Config table objects.
 * Return TRUE/FALSE indicating result of the SET operation.
 */

int
var_ppp_link_config_set(var_name_ptr, in_name_ptr, arg, value, error_status, action)
	OID  var_name_ptr, in_name_ptr;
	u_int arg;
	ObjectSyntax *value;
	int  *error_status, action;
{
	u_long if_num;
	struct tablewr table;
#ifdef WRITE_MIB
	void reverse();

	if (action == ROLLBACK) {
		return(TRUE);
	}

	if (action == COMMIT) {
		if (in_name_ptr->length != (var_name_ptr->length + 1)) {
			*error_status = NO_CREATION;
			return(FALSE);
		}
		/* Find out which interface they're interested in */
		if_num = in_name_ptr->oid_ptr[var_name_ptr->length];

		table.ifIndex = if_num;
		table.entry = arg - 1;

		switch (arg) {
		case PPP_LINKCONFIGINITIALMRU:
		case PPP_LINKCONFIGMAGICNUMBER:
		case PPP_LINKCONFIGFCSSIZE:
			table.mibval = value->sl_value;
			break;

		case PPP_LINKCONFIGRECEIVEACCMAP:
		case PPP_LINKCONFIGTRANSMITACCMAP:
			/* We need to reverse contents of the ACC Map */
			bcopy((char *)value->os_value->octet_ptr, &table.mibval,
				value->os_value->length);
			reverse((u_char *)&table.mibval, 
				value->os_value->length);
			break;

		default:
			*error_status = NO_ACCESS;
			return(FALSE);
		}

		if (set_ppp_table(SIOCSLKC, &table) == NOTOK) {
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
get_ppp_link_status_table(ppp_lks, if_index)
	struct linkstatus *ppp_lks;
	u_long *if_index;
{
	struct tablerd rdbuf;

	rdbuf.ifIndex = *if_index;

	if ((get_ppp_table(SIOCGLKS, &rdbuf)) < 0) {
		return(FALSE);
	}

	bcopy(&rdbuf.miblks, ppp_lks, sizeof(struct linkstatus));
	*if_index = rdbuf.ifIndex;
	return(TRUE);
}

int
get_ppp_link_config_table(ppp_lkc, if_index)
	struct linkconfig *ppp_lkc;
	u_long *if_index;
{
	struct tablerd rdbuf;

	rdbuf.ifIndex = *if_index;

	if ((get_ppp_table(SIOCGLKC, &rdbuf)) < 0)
		return(FALSE);

	bcopy(&rdbuf.miblkc, ppp_lkc, sizeof(struct linkconfig));
	*if_index = rdbuf.ifIndex;
	return(TRUE);
}

int
set_ppp_table(ioct, table)
	int  ioct;
	struct tablewr *table;
{
	struct strioctl ioc;
	struct strioctl strioc;

	if (!didit) {
		if ((ppp_fd = open(_PATH_PPP, O_RDONLY)) < 0) {
			snmp_log(LOG_ERR,
				"set_ppp_table: open %s: %m", _PATH_PPP);
			didit = 0;
			return(NOTOK);
		}
		strioc.ic_cmd = SIOCSMGMT;
		strioc.ic_dp = (char *)0;
		strioc.ic_len = 0;
		strioc.ic_timout = -1;

		if (ioctl(ppp_fd, I_STR, &strioc) < 0){
			snmp_log(LOG_ERR,
				"set_ppp_table: I_STR for SIOCSMGMT: %m");
			close(ppp_fd);
			ppp_fd = -1;
			didit = 0;
			return(NOTOK);
		}
		didit = 1;
	}
	if (ppp_fd == -1) {
		/* must not be any PPP installed */
		return(NOTOK);
	}
 
 
	ioc.ic_cmd = ioct;
	ioc.ic_dp = (char *) table;
	ioc.ic_len = sizeof(struct tablewr);
	ioc.ic_timout = 0;
	if (ioctl(ppp_fd, I_STR, (char *) &ioc) <0 ){
		/* kernel returns ENXIO if ifindex is bad */
		if (errno != ENXIO) {
			snmp_log(LOG_ERR,
			"set_ppp_table: ioctl I_STR for set table: %m ");
		}
		return(NOTOK);
	}
	return(OK);

}

int
get_ppp_table(cmd, rdbuf)
	int cmd;
	struct tablerd *rdbuf;
{
	struct strioctl strioc;

	if (!didit) {
		if ((ppp_fd = open(_PATH_PPP, O_RDONLY)) < 0) {
			snmp_log(LOG_ERR,
				"get_ppp_table: open %s: %m", _PATH_PPP);
			didit = 0;
			return(NOTOK);
		}
		strioc.ic_cmd = SIOCSMGMT;
		strioc.ic_dp = (char *)0;
		strioc.ic_len = 0;
		strioc.ic_timout = -1;

		if (ioctl(ppp_fd, I_STR, &strioc) < 0) {
			snmp_log(LOG_ERR,
				"get_ppp_table: I_STR for SIOCSMGMT: %m");
			close(ppp_fd);
			ppp_fd = -1;
			didit = 0;
			return(NOTOK);
		}
		didit = 1;
	}
 
	if (ppp_fd == -1) {
		/* must not be any PPP on the system */
		return(NOTOK);
	}

#ifdef TEST
	{
	char buf[256];
	sprintf(buf,"get_ppp_table(): ifindex: %d, cmd: %x, size: %d\n",
		rdbuf->ifIndex, cmd, sizeof(struct tablerd));
	snmp_log(LOG_DEBUG,"%s", buf);
	}
#endif

	strioc.ic_cmd = cmd;
	strioc.ic_dp = (char *) rdbuf;
	strioc.ic_len = sizeof(struct tablerd);
	strioc.ic_timout = 0;
	if (ioctl(ppp_fd, I_STR, (char *) &strioc) < 0) {
		/* kernel returns ENXIO if interface index is bad */
		if (errno != ENXIO) {
		  snmp_log(LOG_ERR,
		  "get_ppp_table: I_STR for table get (cmd %lx) (index %d): %m",
			cmd, rdbuf->ifIndex);
		}
		return(NOTOK);
	}
	return(OK);
}


void
reverse(str, size)
	u_char *str;
	int    size;
{
	u_char temp;
	int    i;

	for (i = 0; i < size / 2; i++) {
		temp = *(str + i);
		*(str + i) = *(str + size - 1 - i);
		*(str + size - 1 - i) = temp;
	}
}


/*
 * An init-function that adds PPP-LCP objects to the
 * linked list of objects pointed to by var_entry_ptr.
 */

VarEntry *
init_ppp_lcp_mib(var_entry_ptr)
	VarEntry *var_entry_ptr;
{
	VarEntry *var_ptr = var_entry_ptr;

	/*==============*/
	/*  PPP-LCP	*/
	/*==============*/

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppLcp"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	/*==============*/
	/*  PPP-Link	*/
	/*==============*/

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppLink"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	/*==========================================*/
	/*  PPP-Link Status Table related variables */
	/*==========================================*/

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)"pppLinkStatusTable"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusEntry"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusPhysicalIndex"),
			INTEGER_TYPE, READ_ONLY, PPP_LINKSTATPHYSICALINDEX,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusBadAddresses"),
			COUNTER_TYPE, READ_ONLY, PPP_LINKSTATBADADDRESS, 
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusBadControls"),
			COUNTER_TYPE, READ_ONLY, PPP_LINKSTATBADCONTROLS,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusPacketTooLongs"),
			COUNTER_TYPE, READ_ONLY, PPP_LINKSTATPACKETTOOLONGS,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusBadFCSs"),
			COUNTER_TYPE, READ_ONLY, PPP_LINKSTATBADFCSS,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusLocalMRU"),
			INTEGER_TYPE, READ_ONLY, PPP_LINKSTATLOCALMRU,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusRemoteMRU"),
			INTEGER_TYPE, READ_ONLY, PPP_LINKSTATREMOTEMRU,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusLocalToPeerACCMap"),
			OCTET_PRIM_TYPE, READ_ONLY,
			PPP_LINKSTATLOCALTOPEERACCMAP,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusPeerToLocalACCMap"),
			OCTET_PRIM_TYPE, READ_ONLY,
			PPP_LINKSTATPEERTOLOCALACCMAP,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkStatusLocalToRemoteProtocolCompression"),
			INTEGER_TYPE, READ_ONLY,
			PPP_LINKSTATLOCALTOREMOTEPROTOCOMPR,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkStatusRemoteToLocalProtocolCompression"),
			INTEGER_TYPE, READ_ONLY,
			PPP_LINKSTATREMOTETOLOCALPROTOCOMPR,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkStatusLocalToRemoteACCompression"),
			INTEGER_TYPE, READ_ONLY,
			PPP_LINKSTATLOCALTOREMOTEPROTOACCCOMPR,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkStatusRemoteToLocalACCompression"),
			INTEGER_TYPE, READ_ONLY,
			PPP_LINKSTATREMOTETOLOCALPROTOACCCOMPR,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusTransmitFcsSize"),
			INTEGER_TYPE, READ_ONLY, PPP_LINKSTATTRANSMITFCSSIZE,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)"pppLinkStatusReceiveFcsSize"),
			INTEGER_TYPE, READ_ONLY, PPP_LINKSTATRECEIVEFCSSIZE,
			var_ppp_link_stat_get, TFNULL, SFNULL);

	/*===========================================*/
	/*  PPP-Link Config. Table-related variables */
	/*===========================================*/

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)
							"pppLinkConfigTable"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr, make_obj_id_from_dot((unsigned char *)
							"pppLinkConfigEntry"),
			Aggregate, NONE, 0, GFNULL, TFNULL,
			SFNULL);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkConfigInitialMRU"),
			INTEGER_TYPE, READ_WRITE, PPP_LINKCONFIGINITIALMRU,
			var_ppp_link_config_get, var_ppp_link_config_test,
			var_ppp_link_config_set);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkConfigReceiveACCMap"),
			OCTET_PRIM_TYPE, READ_WRITE,
			PPP_LINKCONFIGRECEIVEACCMAP,
			var_ppp_link_config_get, var_ppp_link_config_test,
			var_ppp_link_config_set);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkConfigTransmitACCMap"),
			OCTET_PRIM_TYPE, READ_WRITE,
			PPP_LINKCONFIGTRANSMITACCMAP,
			var_ppp_link_config_get, var_ppp_link_config_test,
			var_ppp_link_config_set);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkConfigMagicNumber"),
			INTEGER_TYPE, READ_WRITE, PPP_LINKCONFIGMAGICNUMBER,
			var_ppp_link_config_get, var_ppp_link_config_test,
			var_ppp_link_config_set);

	var_ptr = add_var(var_ptr,
			make_obj_id_from_dot((unsigned char *)
					     "pppLinkConfigFcsSize"),
			INTEGER_TYPE, READ_WRITE, PPP_LINKCONFIGFCSSIZE,
			var_ppp_link_config_get, var_ppp_link_config_test,
			var_ppp_link_config_set);
	return(var_ptr);
}

#endif /* PPP_MIB */
