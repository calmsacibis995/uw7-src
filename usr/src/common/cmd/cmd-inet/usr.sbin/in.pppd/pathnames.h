#ident	"@(#)pathnames.h	1.2"
#ident	"$Header$"
/*      @(#) pathnames.h,v 1.3 1995/02/14 01:14:13 stevea Exp */
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
#include "paths.h"

/* pathname for ppp configuration files */
#define	_PATH_PPPHOSTS	"/etc/inet/ppphosts"
#define	_PATH_PPPAUTH	"/etc/inet/pppauth"
#define	_PATH_PPPFILTER	"/etc/inet/pppfilter"
#define	_PATH_PPPPOOL	"/etc/addrpool"

/* pathname for unix domain socket */
#define	PPP_PATH	"/etc/inet/PPP_ADDR"

/* pathname for pid file */
#define	PIDFILE_PATH	"/etc/inet/pppd.pid"
