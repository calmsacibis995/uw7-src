#ident "@(#)aas_pool_query.c	1.2"
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
	fprintf(stderr, "Usage: %s [-p pool] [-t address-type] [-S server]\n",
		myname);
	exit(UTIL_ERROR_CODE);
}

void
main(int argc, char *argv[])
{
	AasConnection *conn;
	AasServer serverp;
	struct in_addr addr;
	AasPoolInfo *pools, *pi;
	ulong num_pools, i;
	char *pool;
	struct hostent *hp;
	int c, type;

	myname = argv[0];

	pool = NULL;
	type = AAS_ATYPE_ANY;
	memset(&serverp, 0, sizeof(serverp));
	serverp.addr_type = AF_UNIX;

	opterr = 0;
	while ((c = getopt(argc, argv, "p:t:S:")) != -1) {
		switch (c) {
		case 'p':
			pool = optarg;
			break;
		case 't':
			if (strcmp(optarg, "INET") == 0) {
				type = AAS_ATYPE_INET;
			}
			else {
				error(UTIL_ERROR_CODE,
					"Unknown address type %s", optarg);
			}
			break;
		case 'S':
			if (hp = gethostbyname(optarg)) {
				memcpy(&addr, hp->h_addr, hp->h_length);
			}
			else if (!inet_aton(optarg, &addr)) {
				error(UTIL_ERROR_CODE, "Unknown host %s",
					optarg);
			}
			serverp.addr_type = AF_INET;
			serverp.port = 0;
			serverp.server.addr = &addr;
			serverp.server.len = sizeof(addr);
			break;
		default:
			usage();
		}
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

	if (aas_get_pool_info(conn, pool, type, &pools,
	    &num_pools) == AAS_FAILURE) {
		error(aas_errno, "aas_get_pool_info failed: %s",
			aas_strerror(aas_errno));
	}

	for (i = 0, pi = pools; i < num_pools; i++, pi++) {
		printf("%s ", pi->pool);
		if (pi->addr_type == AAS_ATYPE_INET) {
			printf("INET");
		}
		else {
			printf("%d", pi->addr_type);
		}
		printf(" %d %d\n", pi->num_addrs, pi->num_alloc);
	}

	exit(0);
}
