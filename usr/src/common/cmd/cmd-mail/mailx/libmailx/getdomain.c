#ident	"@(#)getdomain.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)getdomain.c	1.12 'attmail mail(1) command'"
#include "libmail.h"
#include <ctype.h>
#ifdef SVR4
# include <sys/utsname.h>
# include <sys/systeminfo.h>
#endif

/*
    NAME
	maildomain() - retrieve the domain name

    SYNOPSIS
	char *maildomain()

    DESCRIPTION
	Retrieve the domain name from mgetenv("DOMAIN").
	If that is not set, look in /etc/resolv.conf, /etc/inet/named.boot
	and /etc/named.boot for "^domain[ ]+<domain>".
	Otherwise, return an empty string.
*/

#define NMLN 512
#ifdef SVR4
# if SYS_NMLN > NMLN
#  undef NMLN
#  define NMLN SYS_NMLN
# endif
#endif

/* read a file for the domain */
static char *look4domain(file, buf, size)
char *file, *buf;
int size;
{
    char *ret = 0;
    FILE *fp = fopen(file, "r");
    if (!fp)
	return 0;

    while (fgets(buf, size, fp))
	if (strncmp(buf, "domain", 6) == 0)
	    if (Isspace(buf[6]))
		{
		char *x = (char*)skipspace(buf + 6);
		if (Isgraph(*x))
		    {
		    trimnl(x);
		    strmove(buf, x);
		    ret = buf;
		    break;
		    }
		}

    (void) fclose(fp);
    return ret;
}

static char
    *lookNis(char *buf, int size)
	{
	getdomainname(buf, size);
	return((*buf == '\0')? NULL: buf);
	}

/* read the domain from the xenvironment or one of the files */
static char *readdomain(buf, size)
char *buf;
int size;
{
    char *ret;

    if ((ret = mgetenv("DOMAIN")) != 0)
	{
	(void) strncpy(buf, ret, size);
	return buf;
	}

    ((ret = look4domain("/etc/resolv.conf", buf, size)) != 0) ||
    ((ret = look4domain("/etc/inet/named.boot", buf, size)) != 0) ||
    ((ret = look4domain("/etc/named.boot", buf, size)) != 0) ||
     (ret = lookNis(buf, size));
    if (ret != 0)
	return ret;

    return 0;
}

char *maildomain()
{
    static char *domain = 0;
    static char dombuf[NMLN+1] = ".";

    /* if we've already been here, return the info */
    if (domain != 0)
	return domain;

    domain = readdomain(dombuf+1, NMLN);

    /* Make certain that the domain begins with a single dot */
    /* and does not have one at the end. */
    if (domain)
	{
	int len;
	domain = dombuf;
	while (*domain && *domain == '.')
	    domain++;
	domain--;
	len = strlen(domain);
	while ((len > 0) && (domain[len-1] == '.'))
	    domain[--len] = '\0';
	}

    /* no domain information */
    else
	domain = "";

    return domain;
}
