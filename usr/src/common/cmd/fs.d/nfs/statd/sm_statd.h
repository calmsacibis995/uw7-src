/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)sm_statd.h	1.2"
#ident	"$Header$"

/*      @(#)sm_statd.h 1.2 89/08/18 SMI                              */


struct stat_chge {
	char *name;
	int state;
};
typedef struct stat_chge stat_chge;

#define SM_NOTIFY 6
#define MAXHOSTNAMELEN 64


