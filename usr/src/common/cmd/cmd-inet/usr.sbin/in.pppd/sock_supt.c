#ident	"@(#)sock_supt.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: sock_supt.c,v 1.2 1994/11/15 01:17:07 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
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
/*      SCCS IDENTIFICATION        */

#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include "pathnames.h"

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

int
pppd_sockinit(maxpend)
	int	maxpend;
{
	int	s;
	struct sockaddr_un sin_addr;
	struct servent *serv;

	unlink(PPP_PATH);
 
	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_INFO, gettxt(":343", "socket AF_UNIX SOCK_STREAM fail: %m"));
		exit(1); 
	}

	sin_addr.sun_family = AF_UNIX;
	strcpy(sin_addr.sun_path, PPP_PATH);

	if (bind(s, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
		syslog(LOG_INFO, gettxt(":344", "bind error: %m"));
		exit(1);
	}

	listen(s, maxpend);
	return s;
}

int
ppp_sockinit()
{
	int	s;
	struct sockaddr_un sin_addr;
	struct servent *serv;
	struct linger  linger;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_INFO, gettxt(":343", "socket AF_UNIX SOCK_STREAM fail: %m"));
		return(-1);
	}

	linger.l_onoff = 1;
	linger.l_linger = 5;


	sin_addr.sun_family = AF_UNIX;
	strcpy(sin_addr.sun_path, PPP_PATH);

	if (connect(s, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
		syslog(LOG_INFO, gettxt(":346", "unable to connect to socket %s: %m"),
			PPP_PATH);
		return(-1);
	}
	return s;
}
