#ident	"@(#)OSRcmds:more/help.c	1.1"
#pragma comment(exestr, "@(#) help.c 26.1 95/09/11 ")
/*
 *	Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential. 
 */
/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Modification History
 * L000	scol!anthonys	15 Apr 94
 * - Created from BSD4.4 sources, with modifications to support some old
 *   functionality of the previous implementation, plus modifications
 *   to support the requirements of POSIX.2 and XPG4.
 * L001	scol!ashleyb	29 Aug 1995
 * - Added support for a localised help file.
 */

#ifndef lint
static char sccsid[] = "@)#(help.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include "less.h"
#include "pathnames.h"
#include "../include/osr.h"

#define NLS_DIR "/usr/lib/nls/misc"

extern	char *loc;						/* L001 */

/* Originally, this routine part of OSR libcmd.a, but we'll just place it
 * directly in for UW:
 *
 * Passed in locale (from setlocale(S)), Component name, filename to
 * look for and a default pathname.
 */
char *localised_file(char *loc, char *cmpnt, char *fname, char *dflt)
{
	static char filename[PATHSIZE+1];

	if ( fname != NULL && loc != NULL && cmpnt != NULL )
	{
		sprintf(filename,"%s/%s/%s/%s",NLS_DIR,loc,cmpnt,fname);
		if (access(filename,R_OK) == 0)
			return(filename);
	}

	strcpy(filename,dflt);
	return(filename);
}

help()
{
	char cmd[MAXPATHLEN + 20];

	(void)sprintf(cmd, "-more %s",				/* L001 */
		localised_file(loc, "Unix", "OSRmore.help",_PATH_HELPFILE));
	lsystem(cmd);
}
