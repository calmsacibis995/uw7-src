#ident "@(#)aas_disable.c	1.2"
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
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <aas/aas.h>
#include "util.h"

char *myname;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-d | -e] -p pool -t address-type [-S server] address\n", myname);
	exit(UTIL_ERROR_CODE);
}

void
main(int argc, char *argv[])
{
	AasConnection *conn;
	AasServer serverp;
	AasAddr disable_addr;
	struct in_addr addr, saddr;
	int addr_type, c;
	char *pool;
	struct hostent *hp;
	int disable;

	myname = argv[0];

	disable = -1;
	pool = NULL;
	memset(&serverp, 0, sizeof(serverp));
	serverp.addr_type = AF_UNIX;
	addr_type = AAS_ATYPE_ANY;

	opterr = 0;
	while ((c = getopt(argc, argv, "dep:t:S:")) != -1) {
		switch (c) {
		case 'd':
			disable = 1;
			break;
		case 'e':
			disable = 0;
			break;
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
		case 'S':
			if (hp = gethostbyname(optarg)) {
				memcpy(&saddr, hp->h_addr, hp->h_length);
			}
			else if (!inet_aton(optarg, &saddr)) {
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

	if (disable == -1) {
		error(UTIL_ERROR_CODE, "Must specify either -d or -e.");
	}

	if (addr_type == AAS_ATYPE_ANY) {
		error(UTIL_ERROR_CODE, "No address type specified.");
	}

	if (!pool) {
		error(UTIL_ERROR_CODE, "No pool specified.");
	}

	if (optind == argc) {
		error(UTIL_ERROR_CODE, "No address specified.");
	}

	if (!inet_aton(argv[optind], &addr)) {
		error(UTIL_ERROR_CODE, "Invalid address %s", argv[optind]);
	}

	disable_addr.addr = &addr;
	disable_addr.len = sizeof(addr);

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

	if (aas_disable(conn, pool, addr_type, &disable_addr,
	    disable) == AAS_FAILURE) {
		error(aas_errno, "aas_disable failed: %s",
			aas_strerror(aas_errno));
	}

	exit(0);
}
