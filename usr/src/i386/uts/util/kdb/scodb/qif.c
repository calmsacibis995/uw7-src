#ident	"@(#)kern-i386:util/kdb/scodb/qif.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include	"sys/types.h"
#include	"sys/seg.h"
#include	"dbg.h"
#include	"sent.h"

NOTSTATIC
c_quitif(c, v)
	int c;
	char **v;
{
	long seg, val;

	seg = KDSSEL;
	if (!getaddrv(v + 1, &seg, &val)) {
		perr();
		return DB_ERROR;
	}
	return (val ? DB_RETURN : DB_CONTINUE);
}
