#ident "@(#)aas_release_all.c	1.2"
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
	fprintf(stderr, "Usage: %s -p pool -s service\n\t[-S server]\n",
		myname);
	exit(UTIL_ERROR_CODE);
}

void
main(int argc, char *argv[])
{
	AasConnection *conn;
	AasServer serverp;
	AasAddr free_addr;
	struct in_addr addr, saddr;
	int c, addr_type;
	char *pool, *service;
	struct hostent *hp;

	myname = argv[0];

	pool = NULL;
	service = NULL;
	memset(&serverp, 0, sizeof(serverp));
	serverp.addr_type = AF_UNIX;
	addr_type = AAS_ATYPE_ANY;

	opterr = 0;
	while ((c = getopt(argc, argv, "p:t:s:c:S:")) != -1) {
		switch (c) {
		case 'p':
			pool = optarg;
			break;
		case 's':
			service = optarg;
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

	/*
	 * Allow a null pool, to free ALL pools
	 */

	if (!service) {
		error(UTIL_ERROR_CODE, "No service specified.");
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

	if (aas_free_all(conn, pool, service) == AAS_FAILURE) {
		error(aas_errno, "aas_free failed: %s",
			aas_strerror(aas_errno));
	}

	exit(0);
}
