#ident "@(#)af_unix.c	1.2"
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

/*
 * This file contains code for the AF_UNIX address family.
 * This code is used to for servicing clients connecting to the server
 * over an AF_UNIX socket.
 *
 * This code supplies a bind() function which does an unlink before
 * binding, and changes the mode to 600 after binding.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <aas/aas.h>
#include <aas/aas_proto.h>
#include <aas/aas_util.h>
#include "aasd.h"

/*
 * AddressFamily structure for AF_UNIX.
 */

static int unix_str2addr(char *str, struct sockaddr **addrp, int *lenp);
static char *unix_addr2str(struct sockaddr *addr, int len);
static int unix_bind(int s, const struct sockaddr *addr, size_t len);

AddressFamily family_unix = {
	"unix",			/* name */
	AF_UNIX,		/* family value */
	0,			/* protocol for socket */
	sizeof(struct sockaddr_un),	/* length of sockaddr */
	unix_str2addr,		/* string to address conv func */
	unix_addr2str,		/* address to string conv func */
	unix_bind,		/* bind func */
	"UNIX"			/* fixed password */
};

/*
 * unix_str2addr -- string to address conversion for UNIX domain sockets
 * This just copies the given path into a sockaddr_un structure and
 * returns a pointer to it.  Returns 1 if ok, or 0 if invalid (i.e.,
 * the path is too long).
 */

static int
unix_str2addr(char *str, struct sockaddr **addrp, int *lenp)
{
	static struct sockaddr_un sun;

	/*
	 * If the address is an empty string, use the predefined path.
	 */

	if (!*str) {
		str = _PATH_AAS_SOCKET;
	}

	if (strlen(str) >= sizeof(sun.sun_path)) {
		return 0;
	}

	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, str);

	*addrp = (struct sockaddr *) &sun;
	*lenp = sizeof(sun);
	return 1;
}

/*
 * unix_addr2str -- convert an address to a string
 * This copies the path into a static buffer and returns a pointer to it.
 */

static char *
unix_addr2str(struct sockaddr *addr, int len)
{
	static struct sockaddr_un sun;
	struct sockaddr_un *sunp = (struct sockaddr_un *) addr;

	strcpy(sun.sun_path, sunp->sun_path);

	return sun.sun_path;
}

/*
 * unix_bind -- bind a UNIX domain socket
 * This function unlinks the given path, does the bind, and changes
 * the mode of the socket to 600.  Returns 0 if ok, or -1 on failure.
 */

static int
unix_bind(int s, const struct sockaddr *addr, size_t len)
{
	struct sockaddr_un *sun = (struct sockaddr_un *) addr;

	(void) unlink(sun->sun_path);
	if (bind(s, addr, len) == -1) {
		return -1;
	}
	if (chmod(sun->sun_path, 0600) == -1) {
		return -1;
	}
	return 0;
}
