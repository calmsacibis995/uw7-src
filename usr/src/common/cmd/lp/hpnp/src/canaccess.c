/*		copyright	"%c%"	*/
#ident	"@(#)canaccess.c	1.3"

/*
 * (c)Copyright Hewlett-Packard Company 1991.  All Rights Reserved.
 * (c)Copyright 1983 Regents of the University of California
 * (c)Copyright 1988, 1989 by Carnegie Mellon University
 * 
 *                          RESTRICTED RIGHTS LEGEND
 * Use, duplication, or disclosure by the U.S. Government is subject to
 * restrictions as set forth in sub-paragraph (c)(1)(ii) of the Rights in
 * Technical Data and Computer Software clause in DFARS 252.227-7013.
 *
 *                          Hewlett-Packard Company
 *                          3000 Hanover Street
 *                          Palo Alto, CA 94304 U.S.A.
 */

#include	<stdio.h>
#include	<string.h>
#include	<libgen.h>
#include	<locale.h>
#include	<sys/types.h>
#include	<unistd.h>
#include	"snmp.h"

#include <locale.h>
#include "hpnp_msg.h"

nl_catd catd;

#define PORT	9100

extern int errno;

char *ProgName;
char            *community = COMMUNITY;


static int verbose = 0;

static void usage(void);
static int process(char *, unsigned int);

main(int argc, char *argv[])
{
	char *s, option;
	unsigned int dataport = PORT;
	char *netperiph = (char *)NULL;
	int	ret;
	extern char *optarg;
	extern int optind;

	setlocale(LC_ALL, "");
	catd = catopen( MF_HPNP, MC_FLAGS);

	ProgName = basename(argv[0]);

	while ((option = getopt(argc, argv, "c:p:v")) != EOF) {
		switch (option) {
			case 'c':	/* community name */
				community = optarg;
				break;

			case 'p':	/* port number */
				dataport = atoi(optarg);
				break;

			case 'v':	/* verbose */
				verbose++;
				break;

			case '?':	/* usage */
				usage();
				/* NOTREACHED */

			default:	/* illegal option */
				usage();
				/* NOTREACHED */
		}
	}

	if (--argc > optind || (netperiph = argv[optind]) == NULL) {
		usage();
		/* NOTREACHED */
	}

	close(2);

	ret = process(netperiph, dataport);
	exit(ret);
	/* NOTREACHED */
}

static int
process(char *netperiph, unsigned int dataport)
{
	int fd = -1;

	if (verbose)
		printf
                  (
		  MSGSTR( HPNP_CASS_PPORT, "processing %s on port %d\n"),
                  netperiph, dataport
		  );

	if (!ResolveAddress(netperiph)) {
		if (verbose)
			printf
			  (
			  MSGSTR
			    (
			    HPNP_CASS_NRADR,
			    "could not resolve address for %s\n"
			    ),
			  netperiph
			  );
		return 1;
	}

	if (verbose)
		printf
                  (
		  MSGSTR( HPNP_CASS_CPORT, "connecting to port %d on %s ..."),
		  dataport, netperiph
		  );

	if ((fd = OpenSocket()) < 0) {
		if (verbose)
			printf(MSGSTR( HPNP_CASS_NCONN, "could not connect\n"));
		return 1;
	}
	if (verbose)
		printf( MSGSTR( HPNP_CASS_SUCCD, "succeeded\n"));
	if (check_access(netperiph)) {
		if (verbose)
		printf( MSGSTR( HPNP_CASS_HACCS, "have access\n"));
		return 0;
	}
	if (verbose)
		printf( MSGSTR( HPNP_CASS_NACCS, "no access\n"));
	return 1;
}

static void
usage(void)
{
	fprintf
          (
	  stderr,
	  MSGSTR
	    (
	    HPNP_CASS_USAGE,
	    "Usage: %s [-c community] [-p port] [-v] peripheral\n"
	    ),
	  ProgName
	  );
	exit(2);
	/* NOTREACHED */
}
