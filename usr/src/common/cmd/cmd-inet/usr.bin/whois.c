#ident	"@(#)whois.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>

#ifdef SYSV
#define	bcopy(a,b,c)	memcpy((b),(a),(c))
#endif /* SYSV */

#define	NICHOST	"whois.internic.net."

main(argc, argv)
	int argc;
	char *argv[];
{
	int s;
	register FILE *sfi, *sfo;
	register char c;
	char *host = NICHOST;
	struct sockaddr_in sin;
	struct hostent *hp;
	struct servent *sp;
	extern int	optind;
	extern char	*optarg;

	while ( (s = getopt(argc, argv, "h:")) != -1 ) {
		switch (s) {
		case 'h':
			host = optarg;
			break;
		default:
			fprintf(stderr, "usage: whois [ -h host ] name\n");
			exit(1);
		}
	}
	if ( optind >= argc ) {
		fprintf(stderr, "usage: whois [ -h host ] name\n");
		exit(1);
	}
	hp = gethostbyname(host);
	if (hp == NULL) {
		fprintf(stderr, "whois: %s: host unknown\n", host);
		exit(1);
	}
	host = hp->h_name;
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
		perror("whois: socket");
		exit(2);
	}
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
	sp = getservbyname("whois", "tcp");
	if (sp == NULL) {
		sin.sin_port = htons(IPPORT_WHOIS);
	}
	else sin.sin_port = sp->s_port;
	if (connect(s, &sin, sizeof (sin)) < 0) {
		perror("whois: connect");
		exit(5);
	}
	sfi = fdopen(s, "r");
	sfo = fdopen(s, "w");
	if (sfi == NULL || sfo == NULL) {
		perror("fdopen");
		close(s);
		exit(1);
	}
	fprintf(sfo, "%s\r\n", argv[optind]);
	fflush(sfo);
	while ((c = getc(sfi)) != EOF)
		putchar(c);
	exit(0);
	/* NOTREACHED */
}
