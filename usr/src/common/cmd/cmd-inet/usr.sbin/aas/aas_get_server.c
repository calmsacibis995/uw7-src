#ident "@(#)aas_get_server.c	1.2"
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
	fprintf(stderr, "Usage: %s [-f path of configuration file]\n", myname);
	exit(UTIL_ERROR_CODE);
}

#define MAX_PASSWORD_LENGTH 26

void
main(int argc, char *argv[])
{
	AasServer server;
	struct in_addr addr;
	char *pathname;
	int c;

	myname = argv[0];

	pathname = NULL;

	opterr = 0;
	while ((c = getopt(argc, argv, "f:")) != -1) {
		switch (c) {
		case 'f':
			pathname = optarg;
			break;
		default:
			usage();
		}
	}

	server.addr_type = 0;

	server.server.addr =  &addr;
	server.server.len = sizeof(addr);

	if (!(server.password = malloc(MAX_PASSWORD_LENGTH))) {
		error(aas_errno, "aas_server failed: %s",
			aas_strerror(aas_errno));
	}
	server.password_len = MAX_PASSWORD_LENGTH;

	if (aas_get_server( pathname, &server) == AAS_FAILURE) {
		error(aas_errno, "aas_server failed: %s",
			aas_strerror(aas_errno));
	}

	if (server.addr_type == AF_INET) {
		printf("aas server %s\n", 
			inet_ntoa(*((struct in_addr *)(server.server.addr))));
		printf("aas server length %d\n", server.server.len);
		printf("aas password %s\n", server.password);
		printf("aas password length %d\n", server.password_len);
	} else {
		printf("No remote host defined\n");
	}

	exit(0);
}
