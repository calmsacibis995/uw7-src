/*		copyright	"%c%"	*/
#ident	"@(#)gethostaddr.c	1.2"
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
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>

/*
 * Exit 1 if argv[1] was an address.  Exit 0 otherwise.
 */

main(int argc, char *argv[])
{
	struct hostent	*hp;
	char		*addr;
	int		i;
	struct sockaddr_in sin;

	if (argc != 2)
		exit(0);

	addr = argv[1];
	if (inet_addr(addr) != -1) {
		printf("%s\n", addr);
		exit(1);
	}

	if ((hp = gethostbyname(addr)) == NULL) {
		exit(0);
		/* NO REACHED */
	}

	for (i = 0; hp->h_addr_list[i]; i++) {
		if (i > 0)
			printf(" ");
		memcpy(&sin.sin_addr, hp->h_addr_list[i], hp->h_length);
		printf("%s", inet_ntoa(sin.sin_addr));
	}
	printf("\n");
	exit(0);
}
