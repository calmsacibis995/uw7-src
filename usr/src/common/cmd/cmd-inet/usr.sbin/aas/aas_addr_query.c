#ident "@(#)aas_addr_query.c	1.2"
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
#include <time.h>
#include <aas/aas.h>
#include "util.h"

char *myname;

static void
show_id(AasClientId *id)
{
	unsigned short i;
	unsigned char *idp;

	idp = (unsigned char *) id->id;
	for (i = 0; i < id->len; i++) {
		printf("%02x", idp[i]);
	}
}

static void
show_time(AasTime at, int raw)
{
	struct tm *tm;
	time_t t = at;

	if (raw) {
		printf(" %lu ", t);
	} else {
		tm = localtime(&t);
		printf("%d%02d%02d%02d%02d%02d",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-p pool] [-d] [-r] -t address-type\n\t[-s start-addr] [-e end-addr] [-S server]\n",
		myname);
	exit(UTIL_ERROR_CODE);
}

void
main(int argc, char *argv[])
{
	AasServer serverp;
	AasConnection *conn;
	struct in_addr addr;
	AasAddr min_addr, max_addr, *minp, *maxp;
	struct in_addr min, max;
	AasAddrInfo *addrs, *ai;
	ulong num_addrs, i;
	int c, addr_type, raw;
	time_t t;
	char *pool, *password;
	AasTime server_time;
	struct hostent *hp;

	myname = argv[0];

	raw  = 0;
	minp = NULL;
	maxp = NULL;
	pool = NULL;
	memset(&serverp, 0, sizeof(serverp));
	serverp.addr_type = AF_UNIX;
	addr_type = AAS_ATYPE_ANY;

	opterr = 0;
	while ((c = getopt(argc, argv, "p:drt:s:e:S:")) != -1) {
		switch (c) {
		case 'p':
			pool = optarg;
			break;
		case 'd':
			pool = "";
			break;
		case 'r':
			raw = 1;
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
			/*
			 * If other address types are supported,
			 * the following code will have to be type-specific.
			 */
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

	if (addr_type == AAS_ATYPE_ANY) {
		error(UTIL_ERROR_CODE, "No address type specified.");
	}

	if (!pool) {
		error(UTIL_ERROR_CODE, "No pool specified.");
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

	if (aas_get_addr_info(conn, pool, addr_type,
	    minp, maxp, &addrs, &num_addrs, &server_time) == AAS_FAILURE) {
		error(aas_errno, "aas_get_addr_info failed: %s",
			aas_strerror(aas_errno));
	}

	for (i = 0, ai = addrs; i < num_addrs; i++, ai++) {
		/*
		 * If other address types are supported, the following code
		 * will have to be made type-specific.
		 */
		printf("%s ",
			inet_ntoa(*((struct in_addr *) ai->addr.addr)));
		if (!ai->service) {
			putchar('n');
		}
		else if (ai->flags & AAS_ADDR_INUSE) {
			putchar('a');
		}
		else if (ai->flags & AAS_ADDR_FREED) {
			putchar('r');
		}
		else {
			putchar('e');
		}
		if (ai->flags & AAS_ADDR_TEMP) {
			putchar('t');
		}
		if (ai->flags & AAS_ADDR_DISABLED) {
			putchar('d');
		}
		if (!ai->service) {
			putchar('\n');
			continue;
		}
		putchar(' ');
		show_time(ai->alloc_time, raw);
		if (ai->lease_time == AAS_FOREVER) {
			printf(" infinite ");
		}
		else {
			printf(" %lu ", ai->lease_time);
		}
		if (ai->flags & AAS_ADDR_FREED) {
			show_time(ai->free_time, raw);
		}
		else {
			printf("0");
		}
		printf(" %s ", ai->service);
		show_id(&ai->client_id);
		putchar('\n');
	}

	exit(0);
}
