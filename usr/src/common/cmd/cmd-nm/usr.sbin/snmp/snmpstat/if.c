#ident	"@(#)if.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)if.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)if.c	2.3 INTERACTIVE SNMP source";
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
 * if.c 
 *
 * print out interface entries 
 */
#include <unistd.h>
#include "snmpio.h"

#if defined(SVR3) || defined(SVR4)
#include <string.h>
#endif

int print_if_info();

static char *dots[] = {
	"ifIndex",
	"ifDescr",
	"ifMtu",
	"ifType",
	"ifSpeed",
	"ifPhysAddress",
	"ifOperStatus",
	"ifInOctets",
	"ifInUcastPkts",
	"ifInNUcastPkts",
	"ifInErrors",
	"ifInDiscards",
	"ifOutOctets",
	"ifOutUcastPkts",
	"ifOutNUcastPkts",
	"ifOutErrors",
	"ifOutDiscards",
	"ifOutQLen",
	""
};

ifpr(community)
	char *community;
{

	return(doreq(dots, community, print_if_info));
}

char *ifaddr();

/* ARGSUSED */
print_if_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	VarBindList *vb_ptr;
	char buffer[128];
	int index;
	int ifindex;
	char descr[10 + 1];
	int descrlen;
	char addr[20 + 1];
	int addrlen;
	int mtu;
	char *type;
	int speed;
	int operstatus;
	int inoctets;
	int inucastpkts;
	int innucastpkts;
	int inerrors;
	int indiscards;
	int outoctets;
	int outucastpkts;
	int outnucastpkts;
	int outerrors;
	int outdiscards;
	int outqlen;
	char *lookupiftype();

	index = 0;
	vb_ptr = vb_list_ptr;
	if (!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	ifindex = vb_ptr->vb_ptr->value->sl_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	descrlen = Min(vb_ptr->vb_ptr->value->os_value->length, 10);
	memcpy(descr, (char *) vb_ptr->vb_ptr->value->os_value->octet_ptr, descrlen);
	descr[descrlen] = '\0';

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	mtu = vb_ptr->vb_ptr->value->sl_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		type = "unknown";
	} else
		type = lookupiftype(vb_ptr->vb_ptr->value->sl_value);

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		speed = 0;
	} else
		speed = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		addrlen =  0;
	} else {
		addrlen = Min(vb_ptr->vb_ptr->value->os_value->length, 20);
		memcpy(addr, (char *) vb_ptr->vb_ptr->value->os_value->octet_ptr, 
			addrlen);
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	operstatus = vb_ptr->vb_ptr->value->sl_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno))
		inoctets = 0;
	else
		inoctets = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	inucastpkts = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		innucastpkts = 0;
	} else {
		innucastpkts = vb_ptr->vb_ptr->value->ul_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	inerrors = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		indiscards = 0;
	} else
		indiscards = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		outoctets = 0;
	} else
		outoctets = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	outucastpkts = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		outnucastpkts = 0;
	} else {
		outnucastpkts = vb_ptr->vb_ptr->value->ul_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	outerrors = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		outdiscards = 0;
	} else
		outdiscards = vb_ptr->vb_ptr->value->ul_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		outqlen = 0;
	} else {
		outqlen = vb_ptr->vb_ptr->value->ul_value;
	}

	if (lineno == 0) {
		printf(gettxt(":7", "Interface statistics\n"));
		printf("%-27.27s ", "");
		printf("%-8.8s  ", gettxt(":8", "Type"));
		printf("%-8.8s  ", gettxt(":9", "InOctet"));
		printf("%-8.8s  ", gettxt(":10", "InPckts"));
		printf("%-8.8s  ", gettxt(":11", "InErrs"));
		printf("%-8.8s  ", gettxt(":12", "IfMtu"));
		putchar('\n');
		printf("%-10.10s ", gettxt(":13", "Name"));
		printf("%-16.16s ", gettxt(":14", "Address"));
		printf("%-8.8s  ", gettxt(":15", "Speed"));
		printf("%-8.8s  ", gettxt(":16", "OutOctet"));
		printf("%-8.8s  ", gettxt(":17", "OutPckts"));
		printf("%-8.8s  ", gettxt(":18", "OutErrs"));
		printf("%-8.8s  ", gettxt(":19", "OutQlen"));
		putchar('\n');
	}

	if (operstatus != 1) {
		if (descrlen < 10) {
			descr[descrlen++] = '*';
			descr[descrlen] = '\0';
		} else {
			descr[9] = '*';
		}
	}

	printf("%-10.10s ", descr);
	printf("%-16.16s ", ifaddr(community, ifindex));
	printf("%-8s  ", type);
	printf("%-8u  ", inoctets);
	printf("%-8u  ", inucastpkts + innucastpkts);
	printf("%-8u  ", inerrors + indiscards);
	printf("%-8u  ", mtu);
	putchar('\n');
	printf("%-10.10s ", "");
	for (index = 0; index < addrlen; index++)
		printf("%-2.2x", (unsigned char) addr[index]);
	for ( ; index < 6; index++)
		printf("  ");
	printf("     ");
	printf("%-8u  ", speed);
	printf("%-8u  ", outoctets);
	printf("%-8u  ", outucastpkts + outnucastpkts);
	printf("%-8u  ", outerrors + outdiscards);
	printf("%-8u  ", outqlen);
	putchar('\n');
	return(0);
}

/*
 */

static char *ipdot[] = {
	"ipAdEntIfIndex",
	""
};

int find_addr_index();
int addr_index;
unsigned long addr;

extern char *hostname();

char *
ifaddr(community, if_num)
	char *community;
	int if_num;
{

	addr_index = if_num;
	addr = 0;
	doreq(ipdot, community, find_addr_index);
	if (addr) {
		return(hostname(addr));
	}
	return("");
}

/* ARGSUSED */
find_addr_index(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int if_num;
	int i;

	if (!targetdot(ipdot[0], vb_list_ptr->vb_ptr->name, 1)) {
		return(-1);
	}
	if_num = vb_list_ptr->vb_ptr->value->sl_value;

	if (addr_index == if_num) {
		i = vb_list_ptr->vb_ptr->name->length - 4;
		while (i < vb_list_ptr->vb_ptr->name->length) {
			addr = (addr << 8) + vb_list_ptr->vb_ptr->name->oid_ptr[i++];
		}
		return(-1);
	}
	return(0);
}

char *iftypes[] = {
	"type0", "other", "1822", "hdh1822", "ddn-x25", "877-x25", "enetv2", 
	"802.3", "802.4", "802.5", "802.6", "slan", "pronet10", "pronet80", 
	"hyper", "fddi", "lapb", "sdlc", "t1", "cept", "b-isdn", "p-isdn", 
	"serial", "ppp", "loop", "eon", "enet-3", "nsip", "slip"
};

int n_iftypes = sizeof(iftypes) / sizeof(iftypes[0]);

static char ifbuf[9];

char *
lookupiftype(type)
	int	type;
{
	if (type < 1 || type > n_iftypes) {
		sprintf(ifbuf,"%-8d", type);
		return ifbuf;
	}
	return iftypes[type];
}
