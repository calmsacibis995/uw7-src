#ident "@(#)aas_alloc.c	1.2"
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <time.h>
#include <aas/aas.h>
#include "util.h"

char *myname;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s -p pool -t address-type -s service -c client-ID\n\t-l lease [-r required address] [-b start address] [-e end address]\n\t[-f flag[P|S]] [-S server]\n", myname);
	exit(UTIL_ERROR_CODE);
}

static void
convert_id(char *str, AasClientId *id)
{
	int len, c, which;
	char *p;
	u_char last, *buf, *bp;

	len = strlen(str);

	/*
	 * Check for even length.
	 */

	if ((len % 2) == 1) {
		error(UTIL_ERROR_CODE,
			"Invalid client ID (odd number of digits).");
	}

	len /= 2;

	if (!(buf = malloc(len))) {
		error(UTIL_ERROR_CODE, "Unable to allocate memory.");
	}

	bp = buf;
	which = 0;
	for (p = str; *p; p++) {
		c = *p;
		if (c >= '0' && c <= '9') {
			c -= '0';
		}
		else if (c >= 'a' && c <= 'f') {
			c = c - 'a' + 10;
		}
		else if (c >= 'A' && c <= 'F') {
			c = c - 'A' + 10;
		}
		else {
			error(UTIL_ERROR_CODE,
				"Invalid client ID (invalid hex digit).");
		}
		if (which == 0) {
			last = c;
			which = 1;
		}
		else {
			*bp++ = (last << 4) | c;
			which = 0;
		}
	}

	id->id = buf;
	id->len = len;
}

static void
show_time(AasTime at)
{
	struct tm *tm;
	time_t t = at;

	tm = localtime(&t);
	printf("%d%02d%02d%02d%02d%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void
main(int argc, char *argv[])
{
	AasConnection *conn;
	AasServer serverp;
	AasAddr min_addr, max_addr, req_addr, *minp, *maxp, *reqp, all_addr;
	AasClientId id;
	AasTime lease;
	struct in_addr addr, saddr, reqaddr, min, max;
	int c, addr_type, flags;
	char *pool, *service;
	struct hostent *hp;

	myname = argv[0];

	pool = NULL;
	service = NULL;
	id.len = 0;
	reqp = NULL;
	minp = NULL;
	maxp = NULL;
	memset(&serverp, 0, sizeof(serverp));
	serverp.addr_type = AF_UNIX;
	addr_type = AAS_ATYPE_ANY;
	flags = 0;
	lease = 0;

	opterr = 0;
	while ((c = getopt(argc, argv, "p:t:s:c:r:b:e:f:l:S:")) != -1) {
		switch (c) {
		case 'p':
			pool = optarg;
			break;
		case 't':
			if (strcmp(optarg, "INET") == 0) {
				addr_type = AAS_ATYPE_INET;
			}
			else {
				error(UTIL_ERROR_CODE,
					"Unknown address type %s\n", optarg);
			}
			break;
		case 's':
			service = optarg;
			break;
		case 'c':
			convert_id(optarg, &id);
			break;

		case 'r':
			/*
			 * If other address types are supported,
			 * the following code will have to be type-specific.
			 */
			if (!inet_aton(optarg, &reqaddr)) {
				error(UTIL_ERROR_CODE,
					"Invalid requested address %s", optarg);
			}
			req_addr.addr = &reqaddr;
			req_addr.len = sizeof(reqaddr);
			reqp = &req_addr;
			break;
		case 'b':
			if (!inet_aton(optarg, &min)) {
				error(UTIL_ERROR_CODE,
					"Invalid start address %s", optarg);
			}
			min_addr.addr = &min;
			min_addr.len = sizeof(min);
			minp = &min_addr;
			break;
		case 'e':
			if (!inet_aton(optarg, &max)) {
				error(UTIL_ERROR_CODE,
					"Invalid end address %s", optarg);
			}
			max_addr.addr = &max;
			max_addr.len = sizeof(max);
			maxp = &max_addr;
			break;
		case 'f':
			if (strcmp(optarg, "P") == 0) {
				flags = AAS_ALLOC_PREFER_PREV;
			} 
			else if (strcmp(optarg, "S") == 0) {
				flags = AAS_ALLOC_SPECIFIC;
			} else {
				error(UTIL_ERROR_CODE,
					"Invalid flag %s", optarg);
			}
			break;
		case 'l':
			lease = atoi(optarg);
			break;
		case 'S':
			if (hp = gethostbyname(optarg)) {
				memcpy(&saddr, hp->h_addr, hp->h_length);
			}
			else if (!inet_aton((const char *)optarg, &saddr)) {
				error(UTIL_ERROR_CODE, "Unknown host %s",
					optarg);
			}
			serverp.addr_type = AF_INET;
			serverp.port = 0;
			serverp.server.addr = &saddr;
			serverp.server.len = sizeof(saddr);
			break;
		default:
			usage();
		}
	}

	if (addr_type == AAS_ATYPE_ANY) {
		error(UTIL_ERROR_CODE, "No address type specified.");
	}

	if (!pool) {
		error(UTIL_ERROR_CODE, "No pool specified.");
	}

	if (!service) {
		error(UTIL_ERROR_CODE, "No service specified.");
	}

	if (id.len == 0) {
		error(UTIL_ERROR_CODE, "No client ID specified.");
	}

	if (lease == 0) {
		error(UTIL_ERROR_CODE, "No lease specified.");
	}

	if ((minp != NULL) && (maxp == NULL)) {
		error(UTIL_ERROR_CODE, "Minimum address but no maximum address specified.");
	}
		
	if ((minp == NULL) && (maxp != NULL)) {
		error(UTIL_ERROR_CODE, "Maximum address but no minimum address specified.");
	}

	/*
	 * If a server was specified, we need to get a password.
	 */

	if (serverp.addr_type == AF_INET) {
		if (!(serverp.password = read_password())) {
			error(UTIL_ERROR_CODE, "No password specified.");
		}
		serverp.password_len = strlen(serverp.password);
	}

	if (aas_open(&serverp, AAS_MODE_BLOCK, &conn) == AAS_FAILURE) {
		error(aas_errno, "aas_open failed: %s",
			aas_strerror(aas_errno));
	}

	all_addr.len = sizeof(addr);
	all_addr.addr = &addr;

	if (aas_alloc(conn, pool, addr_type, reqp, minp, maxp,
	    flags, lease, service, &id, &all_addr) == AAS_FAILURE) {
		error(aas_errno, "aas_alloc failed: %s",
			aas_strerror(aas_errno));
	}

	if (all_addr.len == 0) {
		printf("No address returned\n");
	} else {
		printf("%s\n", inet_ntoa(*((struct in_addr *) all_addr.addr)));
	}
	exit(0);
}
