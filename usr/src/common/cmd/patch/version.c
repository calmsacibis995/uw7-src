/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)patch_p2:version.c	1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif

/* Header: version.c,v 2.0 86/09/17 15:40:11 lwall Exp
 *
 * Log:	version.c,v
 * Revision 2.0  86/09/17  15:40:11  lwall
 * Baseline for netwide release.
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pfmt.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <nl_types.h>
#include <langinfo.h>
#include <string.h>
#include <ctype.h>
#include "EXTERN.h"
#include "config.h"
#include "common.h"
#include "version.h"
#include "patchlevel.h"
#include "util.h"


/* Print out the version number and die. */

void
version()
{
    extern char patch_rcsid[];

#ifdef lint
    patch_rcsid[0] = patch_rcsid[0];
#else
    say1(gettxt(":87", "OSF/1 version 1.0 - based on:\n"));
    fatal3(":88:%s\nPatch level: %d\n", patch_rcsid, PATCHLEVEL);
#endif
}
