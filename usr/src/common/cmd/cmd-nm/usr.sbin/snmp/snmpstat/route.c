#ident	"@(#)route.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)route.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)route.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */
/*
 * route.c 
 *
 * print out the routing table entries 
 */

#include <unistd.h>
#include "snmpio.h"

#ifdef BSD
#include <strings.h>
#endif
#if defined(SVR3) || defined(SVR4)
#include <string.h>
#endif

/*
 * The SNMP `type' of the route.
 */
#define DIRECT	3
#define REMOTE	4

int print_route_info();

extern long req_id;

static char *dots[] = {
	"ipRouteDest",
	"ipRouteIfIndex",
	"ipRouteMetric1",
	"ipRouteNextHop",
	"ipRouteType",
	"ipRouteProto",
	""
};

routepr(community)
	char *community;
{

	return(doreq(dots, community, print_route_info));
}

char *ifname();
char *routename();

extern int timeout;
extern char *hostname();
extern char *inet_ntoa();
extern int nflag;

char *route_type_map[] = {
	"???",
	"other",
	"inval",
	"dir",
	"rem"
};
char *route_proto_map[] = {
	"???",
	"other",
	"local",
	"netmgmt",
	"icmp",
	"egp",
	"ggp",
	"hello",
	"rip",
	"is-is",
	"es-is",
	"ciscoIgrp",
	"bbnSpfIgrp",
	"ospf",
	"bgp"
};

print_route_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	VarBindList *vb_ptr;
	unsigned long dest_addr, next_addr;
	int if_num;
	int metric;
	int type;
	int proto;

	index = 0;
	vb_ptr = vb_list_ptr;
	if (!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	dest_addr = ((vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
		     (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
		     (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
		     (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		if_num = 0;
	} else {
		if_num = vb_ptr->vb_ptr->value->sl_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		metric = -1;
	} else {
		metric = vb_ptr->vb_ptr->value->sl_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	next_addr = ((vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
		     (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
		     (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
		     (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	type = vb_ptr->vb_ptr->value->sl_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		proto = -1;
	} else {
		proto = vb_ptr->vb_ptr->value->sl_value;
	}

	if (lineno == 0) {
		printf(gettxt(":20", "Routing Table\n"));
		printf("%-20.20s %-20.20s %-6.6s %-6.6s %-9.9s %s\n",
		      gettxt(":21", "Destination"), gettxt(":22", "Gateway"), 
            gettxt(":23", "Metric"), gettxt(":24", "Type"),
		      gettxt(":25", "Proto"), gettxt(":26", "Interface"));
	}

	printf("%-20.20s ", routename(dest_addr, type));
	printf("%-20.20s ", hostname(next_addr));
	printf("%-6d ", metric);
	printf("%-6.6s ", map_mib(type, route_type_map, sizeof(route_type_map)));
	printf("%-9.9s ", map_mib(proto, route_proto_map, sizeof(route_proto_map)));
	printf("%s\n", if_num != 0 ? ifname(community, if_num) : "");
	return(0);
}

/*
 * Generate a GET request for ifDescr.if_num and return the octet_ptr
 * return.
 */

char *
ifname(community, if_num)
	char *community;
	int if_num;
{
	OID oid_ptr;
	OctetString *community_ptr;
	VarBindList *vb_ptr;
	Pdu *pdu_ptr;
	AuthHeader *auth_ptr;
	char ifdescr[12];
	static char ifnam[50];
	int ifnamlen;
	int cc;
	static int ifnum;

	if (ifnum == if_num) {
		return(ifnam);
	}

	ifnam[0] = '\0';

	sprintf(ifdescr, "ifDescr.%d", if_num);

	pdu_ptr = make_pdu(GET_REQUEST_TYPE, ++req_id, 0L, 0L, NULL,
			   NULL, 0L, 0L, 0L);
	if (pdu_ptr == NULL) {
		return(ifnam);
	}
	if ((oid_ptr = make_obj_id_from_dot((unsigned char *)ifdescr)) == NULL) {
		free_pdu(pdu_ptr);
		return(ifnam);
	}
	vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
	if (vb_ptr == NULL) {
		free_oid(oid_ptr);
		free_pdu(pdu_ptr);
		return(ifnam);
	}
	link_varbind(pdu_ptr, vb_ptr);

	build_pdu(pdu_ptr);

	community_ptr = make_octet_from_text((unsigned char *)community);
	auth_ptr = make_authentication(community_ptr);
	build_authentication(auth_ptr, pdu_ptr);

	cc = send_request(fd, auth_ptr);

	free_authentication(auth_ptr);
	free_pdu(pdu_ptr);

	if (cc != TRUE) {
		return(ifnam);
	}

recv:
	cc = get_response(timeout);

	if (cc == ERROR) {
      		fprintf(stderr, gettxt(":5", "%s: Received an error.\n"), imagename);
      		close_up(fd);
      		exit(-1);
		}

	if (cc == TIMEOUT) {
      		fprintf(stderr, gettxt(":71", "%s: No response. Possible invalid argument. Try again.\n"), imagename);
      		close_up(fd);
      		exit(-1);
		}

	if ((auth_ptr = parse_authentication(packet, packet_len)) == NULL) {
		return(ifnam);
	}
	if ((pdu_ptr = parse_pdu(auth_ptr)) == NULL) {
		free_authentication(auth_ptr);
		return(ifnam);
	}
	if (pdu_ptr->type != GET_RESPONSE_TYPE) {
		free_authentication(auth_ptr);
		free_pdu(pdu_ptr);
		return(ifnam);
	}
	snmpstat->ingetresponses++;
	if (pdu_ptr->u.normpdu.error_status != NO_ERROR) {
		free_authentication(auth_ptr);
		free_pdu(pdu_ptr);
		return(ifnam);
	}
	if (pdu_ptr->u.normpdu.request_id != req_id) {
		free_authentication(auth_ptr);
		free_pdu(pdu_ptr);
		goto recv;
	}
	ifnamlen = Min(pdu_ptr->var_bind_list->vb_ptr->value->os_value->length,
		       sizeof(ifnam) - 1);
	strncpy(ifnam,
		(char *)pdu_ptr->var_bind_list->vb_ptr->value->os_value->octet_ptr,
		ifnamlen);
	ifnam[ifnamlen] = '\0';
	free_authentication(auth_ptr);
	free_pdu(pdu_ptr);
	ifnum = if_num;
	return(ifnam);
}

/*
 * Convert the given IP address to the appropriate network or host name.
 * Addresses which do not convert directly to hostnames are considered
 * to be network names.
 */
char *
routename(addr, type)
	unsigned long addr;
	int type;
{
	char *cp;
	struct netent *np;
	struct hostent *hp;
	struct in_addr in;
#ifdef SVR3
	ulong mask;
	ulong net;
#endif
#if defined(BSD) || defined(SVR4)
	u_long mask;
	u_long net;
#endif
	int subnetshift;
	static char name[50];

	in.s_addr = htonl(addr);
	if (IN_CLASSA(addr)) {
		mask = IN_CLASSA_NET;
		subnetshift = 8;
	} else if (IN_CLASSB(addr)) {
		mask = IN_CLASSB_NET;
		subnetshift = 8;
	} else {
		mask = IN_CLASSC_NET;
		subnetshift = 4;
	}
	while (addr & ~mask)
		/*
		 * The next line used to read:
		 *
		 * mask = (long) mask >> subnetshift;
		 *
		 * However, this does not work on machines that
		 * do not implement arithmetic shifts.  (K&R
		 * does not specify how shifts should be done.)
		 * Thus, this next line accomplishes the same
		 * task in a slightly more machine independent
		 * fashion.
		 */
		mask = (long) mask / (1 << subnetshift);
	net = addr & mask;
	while ((mask & 1) == 0)
		mask >>= 1, net >>= 1;
	if (!nflag && (np = getnetbyaddr(net, AF_INET)) != NULL) {
		cp = np->n_name;
	} else if (!nflag && (type == DIRECT || inet_lnaof(in)) &&
		   (hp = gethostbyaddr((char *)&in, sizeof(in), AF_INET)) != NULL) {
#ifdef BSD
		if ((cp = index(hp->h_name, '.')) != NULL)
#endif
#if defined(SVR3) || defined(SVR4)
		if ((cp = strchr(hp->h_name, '.')) != NULL)
#endif
			*cp = '\0';
		cp = hp->h_name;
	} else {
		cp = inet_ntoa(in);
	}
	strcpy(name, cp);
	return(name);
}
