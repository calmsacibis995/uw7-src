#ident "@(#)mhome.c	11.1"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "multihome.h"

#define debug printf

#define MAXNAME 256

/*
 * routine to map multihome users to real users
 * returns malloced string of new user
 */
char *
mhomeuser(char *vname)
{
	struct sockaddr_in cs;
	int sp;
	size_t len;
	char *cp;
	char fqdn[MAXNAME];
	char *user;

	sp = 0;
	len = sizeof(cs);
	if (getsockname(sp, (struct sockaddr *)&cs, &len) < 0) {
		/* no peer name */
		return(strdup(vname));
	}

	/* get ip addr in character format */
	cp = inet_ntoa(cs.sin_addr);

	/* have address in character format, look up fqdn */
	cp = mhome_ip_virtual_name(cp);

	/* the normal case for non-virtual domains */
	if (cp == 0) {
		return(strdup(vname));
	}

	strcpy(fqdn, cp);
	user = mhome_user_map(vname, fqdn);
	mhome_user_map_close();

	/* non-mapped users fail */
	if (user == 0) {
#ifndef OpenServer
		return(strdup("noaccess"));
#else
		return(strdup("nouser"));
#endif
	}

	/* found one */
	return(strdup(user));
}
