#ident "@(#)aas_release.c	1.2"
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
#include <aas/aas.h>
#include "util.h"

char *myname;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s -p pool -t address-type -s service -c client-ID\n\t[-S server] address\n",
		myname);
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

void
main(int argc, char *argv[])
{
	AasConnection *conn;
	AasServer serverp;
	AasAddr free_addr;
	AasClientId id;
	struct in_addr addr, saddr;
	int c, addr_type;
	char *pool, *service;
	struct hostent *hp;

	myname = argv[0];

	pool = NULL;
	service = NULL;
	id.len = 0;
	memset(&serverp, 0, sizeof(serverp));
	serverp.addr_type = AF_UNIX;
	addr_type = AAS_ATYPE_ANY;

	opterr = 0;
	while ((c = getopt(argc, argv, "p:t:s:c:S:")) != -1) {
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

	if (optind == argc) {
		error(UTIL_ERROR_CODE, "No address specified.");
	}

	if (!inet_aton(argv[optind], &addr)) {
		error(UTIL_ERROR_CODE, "Invalid address %s", argv[optind]);
	}

	free_addr.addr = &addr;
	free_addr.len = sizeof(addr);

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

	if (aas_free(conn, pool, addr_type, &free_addr,
	    service, &id) == AAS_FAILURE) {
		error(aas_errno, "aas_free failed: %s",
			aas_strerror(aas_errno));
	}

	exit(0);
}
