#ident	"@(#)gtphstnamadr.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: gtphstnamadr.c,v 1.2 1994/11/15 01:16:52 neil Exp"
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
#include <stdio.h>
#include <netdb.h>
#include <sys/file.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/conf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>
#include "pppd.h"
#include "pppu_proto.h"

#define bcmp(s1, s2, len)	memcmp(s1, s2, len)
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>


struct ppphostent host;

/* search for a ppp entry using login name */	
struct ppphostent *
getppphostbyname(nam)
	register char *nam;
{
	char lowname[NAME_SIZE];
	register char *lp = lowname;
	FILE	*fp;
	int	ret;
	
	while (*nam)
		if (isupper(*nam))
			*lp++ = tolower(*nam++);
		else
			*lp++ = *nam++;
	*lp = '\0';

	fp = setppphostent();
	while ((ret = getppphostent(fp, &host))) {
		if (ret == -1)
			continue;
		if (strcmp(host.loginname, lowname) == 0){
			endppphostent(fp);
			return (&host);
		}	
	}
	
	endppphostent(fp);
	return (NULL);
}

/* search for a ppp entry using remote host address */	
struct ppphostent *
getppphostbyaddr(addr, length, type)
	char *addr;
	register int length;
	register int type;
{
	FILE	*fp;
	int	ret;

	fp = setppphostent();
	while ((ret = getppphostent(fp, &host))) {
		if (ret == -1)
			continue;
		if (host.loginname[0] != '\0')
			continue;
		if (host.ppp_cnf.remote.sin_family == type
		    && bcmp(&host.ppp_cnf.remote.sin_addr, addr, length) == 0){
			endppphostent(fp);
			return (&host);
		}	
	}
	endppphostent(fp);
	return (NULL);
}

/* search for a ppp entry using attach name */	
struct ppphostent *
getppphostbyattach(nam)
	register char *nam;
{
	FILE	*fp;
	int	ret;
	
	fp = setppphostent();
	while ((ret = getppphostent(fp, &host))) {
		if (ret == -1)
			continue;
		if (strcmp(host.attach, nam) == 0){
			endppphostent(fp);
			return (&host);
		}	
	}
	
	endppphostent(fp);
	return (NULL);
}
