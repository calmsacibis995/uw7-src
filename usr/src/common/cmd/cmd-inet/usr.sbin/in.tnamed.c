/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)in.tnamed.c	1.2"
#ident  "$Header$"

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

/*
 * This program implements a UDP basic name server as specified in IEN116
 * The extended name server functionality is NOT implemented here (yet).
 * This is generally used in conjunction with MIT's PC/IP software.
 */

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef SYSV
#define	bzero(s,n)	memset((s), 0, (n))
#define	bcopy(a,b,c)	memcpy((b), (a), (c))
#endif /* SYSV */

/*
 * These command codes come from IEN116
 */
#define NAMECODE	1
#define ADDRESSCODE	2
#define ERRORCODE	3
/*
 * These error codes are used to qualify ERRORCODE
 */
#define UNDEFINEDERROR	0
#define NOTFOUNDERROR	1
#define SYNTAXERROR	2
	
#define BUFLEN    2000
#define MAX_LIFE    60      /* Daemon lives only some seconds if forked by inetd */
#define TNAMED_PORT 42      /* well known Internet portummer */

main(argc, argv)
	int argc;
	char **argv;
{
	int s;
	struct sockaddr_in client;
	int length, clientlength;
	register struct hostent	*hp;
	char hostname[BUFLEN];
	char buffer[BUFLEN];
	register int replylength;
	int request;
	struct in_addr x;
	int forked_by_inetd = 0;

	if (argc > 1) {
		/* the daemon is run by hand and never exits */
		struct servent temp;
		register struct servent *sp;
		struct sockaddr_in server;

		if((sp = getservbyname("name","udp")) == NULL) {
			fprintf(stderr,
			   "in.tnamed: UDP name server not in /etc/services\n");
			sp = &temp;
			sp->s_port = htons(TNAMED_PORT);
		}
		if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("in.tnamed: socket error");
			exit(1);
		}
		bzero((char *)&server, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_port = sp->s_port;
		if(bind(s, &server, sizeof(server)) != 0) {
			perror("in.tnamed: bind error");
			exit(1);
		}
		fprintf(stderr, "in.tnamed: UDP name server running\n");
	} else {
		/* daemon forked by inetd and is short lived */

		/*
		 * inetd makes filedescriptors 0 and 1 point to the socket.
		 * Redirect them to /dev/null to allow fprintf, perror ...
		 */

		close(1);
		close(2);
		(void) open("/dev/null", O_RDWR);
		(void) dup(1);
		forked_by_inetd = 1;
		s = 0;  /* by inetd conventions */
	}

	for (;;) {
		if (forked_by_inetd) {
			struct timeval tv;
			int rfds = 1<<0;

			tv.tv_sec = MAX_LIFE;
			tv.tv_usec = 0;

			if (select(32, &rfds, 0, 0, &tv) <= 0)
				exit(0);
		}
		clientlength = sizeof(client);
		length = recvfrom(s, buffer, sizeof(buffer), 0, &client, &clientlength);
		if (length < 0) {
			perror("in.tnamed: recvfrom error. Try in.tnamed -v ?");
			continue;
		}
		if (length == 0 || buffer[1] > length) {
			fprintf(stderr, "invalid request length %d\n", length);
			continue;
		}
		request = buffer[0];
		length = buffer[1];
		replylength = length + 2;  /* reply is appended to request */
		if (length < sizeof(hostname)) {
			strncpy(hostname, &buffer[2], length);
			hostname[length]= 0;
		} else {
			hostname[0] = 0;
		}

		if(request != NAMECODE) {
			fprintf(stderr, "in.tnamed: bad request from %s\n",
			    inet_ntoa(client.sin_addr));
			buffer[replylength++] = ERRORCODE;
			buffer[replylength++] = 3;  /* no error msg yet */
			buffer[replylength++] = SYNTAXERROR;
			fprintf(stderr,
			    "in.tnamed: request (%d) not NAMECODE\n", request);
			sleep(5);  /* pause before saying something negative */
			goto sendreply;
		}
		
		if(hostname[0] == '!') {
			/*
			 * !host!net name format is not implemented yet,
			 * only host alone.
			 */
			fprintf(stderr,
			  "in.tnamed: %s using !net!host format name request\n",
			    inet_ntoa(client.sin_addr));
			buffer[replylength++] = ERRORCODE;
			buffer[replylength++] = 0;  /* no error msg yet */
			buffer[replylength++] = UNDEFINEDERROR;
			fprintf(stderr,
			    "in.tnamed: format (%s) not supported\n", hostname);
			sleep(5);  /* pause before saying something negative */
			goto sendreply;
		}

		if((hp = gethostbyname(hostname)) == NULL) {
			buffer[replylength++] = ERRORCODE;
			buffer[replylength++] = 0;  /* no error msg yet */
			buffer[replylength++] = NOTFOUNDERROR;
			fprintf(stderr,
			    "in.tnamed: name (%s) not found\n", hostname);
			sleep(5);  /* pause before saying something negative */
			goto sendreply;
		}

		if(hp->h_addrtype != AF_INET) {
			buffer[replylength++] = ERRORCODE;
			buffer[replylength++] = 0;  /* no error msg yet */
			buffer[replylength++] = UNDEFINEDERROR;
			fprintf(stderr,
			    "in.tnamed: address type (%d) not AF_INET\n",
			    hp->h_addrtype);
			sleep(5);  /* pause before saying something negative */
			goto sendreply;
		}

		fprintf(stderr, "in.tnamed: %s asked for address of %s",
				inet_ntoa(client.sin_addr), hostname);
		bcopy (hp->h_addr, (char *)&x, sizeof x);
		fprintf(stderr, " - it's %s\n", inet_ntoa(x));
			
		buffer[replylength++] = ADDRESSCODE;
		buffer[replylength++] = hp->h_length;
		bcopy(hp->h_addr, &buffer[replylength], hp->h_length);
		replylength += hp->h_length;

	sendreply:
		if (sendto(s, buffer, replylength, 0, &client, clientlength)
		    != replylength) {
			perror("in.tnamed: sendto error");
			continue;
		}
	}
}
