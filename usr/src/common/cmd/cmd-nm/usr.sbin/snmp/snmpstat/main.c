#ident	"@(#)main.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)main.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)main.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */

/*
 * snmpstat
 *
 * print out SNMP statistics
 */

#include <locale.h>
#include <unistd.h>
#include "snmpio.h"

long req_id;

int nflag;
int prall;

int timeout;

void usage(char *argv[]);

int main(int argc, char *argv[])
{
	int c;
	extern char *optarg;
	extern int optind;

  	int errflg = 0;
 
	int tflag;
	int rflag;
	int aflag;
	int sflag;
	int iflag;
	int Sflag;
	char *opts;
	char *community;

	timeout = SECS;

	(void)setlocale(LC_ALL, "");
	(void)setcat("nmsnmpstat");
	(void)setlabel("NM:snmpstat");

	tflag = rflag = aflag = sflag = iflag = Sflag = nflag = 0;
	prall = 0;

  	while((c = getopt(argc, argv, "T:t|r|a|s|i|n|S")) != EOF )
        	{
        	switch(c)
                	{
                	case 'T':
                        	timeout = atoi(optarg);
                	break;

                	case '?':
                        	errflg++;
                	break;

			case 't':
				if (rflag || aflag || sflag || iflag || Sflag)
					goto bad_usage;
				tflag = 1;
				prall = 1;
			break;

			case 'r':
				if (tflag || aflag || sflag || iflag || Sflag)
					goto bad_usage;
				rflag = 1;
			break;

			case 'a':
				if (tflag || rflag || sflag || iflag || Sflag)
					goto bad_usage;
				aflag = 1;
			break;

			case 's':
				if (tflag || rflag || aflag || iflag || Sflag)
					goto bad_usage;
				sflag = 1;
			break;

			case 'i':
				if (tflag || rflag || aflag || sflag || Sflag)
					goto bad_usage;
				iflag = 1;
			break;

			case 'S':
				if (tflag || rflag || aflag || sflag || iflag)
					goto bad_usage;
				Sflag = 1;
			break;

			case 'n':
				nflag = 1;
			break;

			default:
bad_usage:
				usage(argv);
			exit(-1);
                	}
        	}
 
  if(((argc - optind) < 2) || (errflg == 1))
        {
        usage(argv);
        exit(-1);
        }

	initialize_io(argv[0], argv[optind]);

	req_id = make_req_id();

	if (rflag) {
		exit(routepr(argv[optind + 1]));
	}

	if (aflag) {
		exit(atpr(argv[optind + 1]));
	}

	if (sflag) {
		exit(syspr(argv[optind + 1]));
	}

	if (iflag) {
		exit(ifpr(argv[optind + 1]));
	}

	if (Sflag) {
		exit(snmppr(argv[optind + 1]));
	}

	exit(tablepr(argv[optind + 1]));

	/* NOTREACHED */
}

void usage(char *argv[])
{
	fprintf(stderr, gettxt(":1", "Usage: %s [-T timeout] [-t|r|a|s|i|n|S] entity_addr community_string\n"), argv[0]);
}

int table_header_printed;

tablepr(community)
	char *community;
{

	table_header_printed = 0;
	tcppr(community);
	udppr(community);
}
