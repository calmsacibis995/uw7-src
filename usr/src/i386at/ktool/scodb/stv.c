#ident	"@(#)ktool:i386at/ktool/scodb/stv.c	1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History:
 *
 *	L000	scol!nadeem	23apr92
 *	- changed argument passed to preprocessor from "-DNO_PROTOTYPE"
 *	  to "-D_NO_PROTOTYPE".
 *	- if no files are specified on the command line then read from
 *	  standard input.  This allows us to construct the preprocessor
 *	  and compiler pipe externally, and use stv as a filter.  Change
 *	  usage message appropriately.
 *	L001	scol!hughd	23jun92
 *	- nadeem added the filter usage in L000 and that's how stv is
 *	  now being used, via mkidef; to avoid issues of __DSROOT and
 *	  suppds here, now reduce stv always to a filter: process the
 *	  output of cc or idcpp | idcomp, don't invoke them within stv
 *	  (which reduces this module to very little)
 */

#include	<stdio.h>
#include	<sys/scodb/stunv.h>

main()
{
	int vr = IDF_VERSION;

	procinp(stdin);
	fwrite(IDF_MAGIC, IDF_MAGICL, 1, stdout);
	fwrite(&vr, sizeof vr, 1, stdout);
	d_stuns(stdout);
	d_varis(stdout);
	exit(0);
}
