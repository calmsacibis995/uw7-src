#ident "@(#)af_inet.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
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

/*
 * This file contains code for the AF_INET address family.
 * This code is used to for servicing clients connecting to the server
 * over an AF_INET socket.  This has nothing to do with address handling
 * code used to implement the address allocation mechanism.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <memory.h>
#include <aas/aas.h>
#include <aas/aas_proto.h>
#include <aas/aas_util.h>
#include "aasd.h"

/*
 * AddressFamily structure for AF_INET.
 */

static int inet_str2addr(char *str, struct sockaddr **addrp, int *lenp);
static char *inet_addr2str(struct sockaddr *addr, int len);

AddressFamily family_inet = {
	"inet",			/* name */
	AF_INET,		/* family value */
	0,			/* protocol for socket */
	sizeof(struct sockaddr_in),	/* length of sockaddr */
	inet_str2addr,		/* string to address conv func */
	inet_addr2str,		/* address to string conv func */
	bind,			/* bind func */
	NULL			/* no fixed password */
};

/*
 * inet_str2addr -- convert a string representation of an IP address/port
 * to a sockaddr_in.  The format is "address:port".  An empty address
 * defaults to INADDR_ANY.  An empty port defaults to the well-known
 * port.  If no port is specified, the colon can be omitted.  An empty string
 * translates to INADDR_ANY:well-known-port.  address can be a host name or
 * an address in dot notation.  Port can be a TCP service name or a decimal
 * port number.
 * Returns 1 if ok, or 0 if the string is invalid.
 */

static int
inet_str2addr(char *str, struct sockaddr **addrp, int *lenp)
{
	static struct sockaddr_in sin;
	char *p, *astr, *pstr;
	struct hostent *hp;
	struct servent *sp;

	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;

	astr = str;
	if (p = strchr(str, ':')) {
		*p = '\0';
		pstr = p + 1;
	}
	else {
		pstr = "";
	}

	if (isdigit(*astr)) {
		sin.sin_addr.s_addr = inet_addr(astr);
		if (sin.sin_addr.s_addr == INADDR_NONE) {
			return 0;
		}
	}
	else if (*astr) {
		if (!(hp = gethostbyname(astr))) {
			return 0;
		}
		memcpy(&sin.sin_addr.s_addr, hp->h_addr, hp->h_length);
	}
	else {
		sin.sin_addr.s_addr = INADDR_ANY;
	}

	if (isdigit(*pstr)) {
		if (sscanf(pstr, "%hu", &sin.sin_port) != 1) {
			return 0;
		}
		sin.sin_port = htons(sin.sin_port);
	}
	else if (*pstr) {
		if (!(sp = getservbyname(pstr, "tcp"))) {
			return 0;
		}
		sin.sin_port = sp->s_port;
	}
	else {
		if (!(sp = getservbyname(AAS_SERVICE_NAME, "tcp"))) {
			report(LOG_WARNING, "Unknown service \"%s\"; using default port %d",
				AAS_SERVICE_NAME, AAS_PORT);
			sin.sin_port = htons((ushort) AAS_PORT);
		}
		else {
			sin.sin_port = sp->s_port;
		}
	}

	*addrp = (struct sockaddr *) &sin;
	*lenp = sizeof(sin);
	return 1;
}

/*
 * inet_addr2str -- convert an address to a string
 * A string of the form "address:port" is returned, where address is an IP
 * address in dot notation and port is a decimal port number.  The address
 * INADDR_ANY is shown as "*".  Returns a pointer to the address string,
 * which is in a static area.
 */

static char *
inet_addr2str(struct sockaddr *addr, int len)
{
	struct sockaddr_in *sin = (struct sockaddr_in *) addr;
	static char buf[32];

	sprintf(buf, "%s:%d",
		sin->sin_addr.s_addr == INADDR_ANY ?
			"*" : inet_ntoa(sin->sin_addr),
		ntohs(sin->sin_port));
	
	return buf;
}
