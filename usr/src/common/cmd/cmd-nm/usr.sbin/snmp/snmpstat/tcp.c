#ident	"@(#)tcp.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)tcp.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)tcp.c	1.8 INTERACTIVE SNMP source";
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
 * tcp.c 
 *
 * print out the tcp connection table entries 
 */

#include <unistd.h>
#include "snmpio.h"

int print_tcp_info();

static char *dots[] = {
	"tcpConnState",
	"tcpConnLocalAddress",
	"tcpConnLocalPort",
	"tcpConnRemAddress",
	"tcpConnRemPort",
	""
};

tcppr(community)
	char *community;
{

	return(doreq(dots, community, print_tcp_info));
}

extern char *inetname();
char *tcpstate();

extern int table_header_printed;
extern int prall;

/* ARGSUSED */
print_tcp_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	VarBindList *vb_ptr;
	int state;
	unsigned long local_addr;
	int local_port;
	unsigned long rem_addr;
	int rem_port;

	index = 0;
	vb_ptr = vb_list_ptr;
	if (!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	state = vb_ptr->vb_ptr->value->sl_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
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

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	rem_addr = ((vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
		    (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	rem_port = vb_ptr->vb_ptr->value->sl_value;

	if (!prall && local_addr == INADDR_ANY) {
		return(0);
	}

	if (!table_header_printed) {
		table_header_printed = 1;
		printf(gettxt(":62", "Active Internet connections"));
		if (prall)
			printf(gettxt(":63", " (including servers)"));
		putchar('\n');
		printf("%-5.5s  %-22.22s %-22.22s %s\n", 
            gettxt(":25", "Proto"), gettxt(":64", "Local Address"), 
            gettxt(":65", "Foreign Address"), gettxt(":66", "(state)"));
	}

	printf("%-5.5s  ", gettxt(":67", "tcp"));
	printf("%-22.22s ", inetname(local_addr, local_port, gettxt(":67", "tcp")));
	printf("%-22.22s ", inetname(rem_addr, rem_port, gettxt(":67", "tcp")));
	printf("%s\n", tcpstate(state));
	return(0);
}

char *
tcpstate(state)
	int state;
{

	switch (state) {
	case 1:
		return("CLOSED");
	case 2:
		return("LISTEN");
	case 3:
		return("SYNSENT");
	case 4:
		return("SYNRECEIVED");
	case 5:
		return("ESTABLISHED");
	case 6:
		return("FINWAIT1");
	case 7:
		return("FINWAIT2");
	case 8:
		return("CLOSEWAIT");
	case 9:
		return("LASTACK");
	case 10:
		return("CLOSING");
	case 11:
		return("TIMEWAIT");
	default:
		return("UNKNOWN");
	}
	/* NOTREACHED */
}
