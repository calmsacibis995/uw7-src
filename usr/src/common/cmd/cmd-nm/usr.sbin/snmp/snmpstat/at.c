#ident	"@(#)at.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)at.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)at.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */

/*
 * at.c 
 *
 * print out the address translation table entries 
 */

#include <unistd.h>
#include "snmpio.h"

#include <string.h>

int print_at_info();
int print_atII_info();

int at_table_header_printed;

static char *dots[] = {
	"atIfIndex",
	"atPhysAddress",
	"atNetAddress",
	""
};
static char *dotsII[] = {
	"ipNetToMediaIfIndex",
	"ipNetToMediaPhysAddress",
	"ipNetToMediaNetAddress",
	"ipNetToMediaType",
	""
};

atpr(community)
	char *community;
{
	int ret;

	at_table_header_printed = 0;
	ret = doreq(dotsII, community, print_atII_info);
	if(at_table_header_printed == 0)
		return(doreq(dots, community, print_at_info));
	return(ret);
}

char *atname();
char *atphysaddr();
extern char *ifname();
extern int nflag;

print_at_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	int if_num;
	VarBindList *vb_ptr;
	OctetString *physp;
	unsigned long net_addr;

	index = 0;
	vb_ptr = vb_list_ptr;
	if(!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	if_num = vb_ptr->vb_ptr->value->sl_value;

	vb_ptr = vb_ptr->next;
	if(!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	physp = vb_ptr->vb_ptr->value->os_value;

	vb_ptr = vb_ptr->next;
	if(!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	net_addr = ((vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));

	if(at_table_header_printed++ == 0) {
		printf(gettxt(":2", "Address Translation table\n"));
		printf("%-20.20s %-20.20s %s\n",
		      gettxt(":3", "Host"), gettxt(":4", "Physical Address"), 
            gettxt(":5", "Interface"));
	}

	printf("%-20.20s ", atname(net_addr));
	printf("%-20.20s ", atphysaddr(physp));
	printf("%s\n", ifname(community, if_num));
	return(0);
}

extern char *map_mib();

char *at_type_map[] = { "????", "other", "inval", "dyna", "perm" };

print_atII_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	int if_num;
	VarBindList *vb_ptr;
	OctetString *physp;
	unsigned long net_addr;
	int type;

	index = 0;
	vb_ptr = vb_list_ptr;
	if(!targetdot(dotsII[index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	if_num = vb_ptr->vb_ptr->value->sl_value;

	vb_ptr = vb_ptr->next;
	if(!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	physp = vb_ptr->vb_ptr->value->os_value;

	vb_ptr = vb_ptr->next;
	if(!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	net_addr = ((vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));

	vb_ptr = vb_ptr->next;
	if(!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	type = vb_ptr->vb_ptr->value->sl_value;

	if(at_table_header_printed++ == 0) {
		printf(gettxt(":2", "Address Translation table\n"));
		printf("%-20.20s %-20.20s %-5.5s %s\n",
		      gettxt(":3", "Host"), gettxt(":4", "Physical Address"), 
            gettxt(":6", "Flags"), gettxt(":5", "Interface"));
	}

	printf("%-20.20s ", atname(net_addr));
	printf("%-20.20s ", atphysaddr(physp));
	printf("%-5.5s ", map_mib(type, at_type_map, sizeof(at_type_map)));
	printf("%s\n", ifname(community, if_num));
	return(0);
}

char *
atname(addr)
	unsigned long addr;
{
	char *cp;
	struct in_addr in;
	struct hostent *hp;
	static char line[80];
	extern char *inet_ntoa();

	if(addr != INADDR_ANY) {
		in.s_addr = htonl(addr);
		if(nflag ||
		    (hp = gethostbyaddr((char *)&in, sizeof(in), AF_INET)) == NULL) {
			cp = inet_ntoa(in);
		} else {
#ifdef BSD
			if((cp = index(hp->h_name, '.')) != NULL)
#endif
#if defined(SVR3) || defined(SVR4)
			if((cp = strchr(hp->h_name, '.')) != NULL)
#endif
				*cp = '\0';
			cp = hp->h_name;
		}
	} else {
		cp = "*";
	}

	strncpy(line, cp, sizeof(line));

	return(line);
}

char *
atphysaddr(physp)
	OctetString *physp;
{
	int i;
	char *cp;
	static char line[80];

	cp = line;
	*cp = '\0';
	for (i = 0; i < physp->length; i++) {
		if(i == 0)
			sprintf(cp, "%x", physp->octet_ptr[i] & 0xff);
		else
			sprintf(cp, ":%x", physp->octet_ptr[i] & 0xff);
#ifdef BSD
		cp = index(cp, '\0');
#endif
#if defined(SVR3) || defined(SVR4)
		cp = strchr(cp, '\0');
#endif
	}
	return(line);
}
