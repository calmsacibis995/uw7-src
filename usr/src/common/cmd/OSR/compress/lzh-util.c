#ident	"@(#)OSRcmds:compress/lzh-util.c	1.1"
#pragma comment(exestr, "@(#) lzh-util.c 26.1 95/06/30 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1991-1995.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * lzh-util.c -- Utility procedures for LZH compression.
 *-----------------------------------------------------------------------------
 * From public domain code in zoo by Rahul Dhesi, who adapted it from "ar"
 * archiver written by Haruhiko Okumura.
 *=============================================================================
 */
/*
 *	Modification History
 *	L001	4 Nov 92	scol!johnfa
 *	For XPG4 Conformance;
 *	- if appending .Z to filename would make name exceed NAME_MAX bytes
 *	  the command will now fail with an error status.
 *	- converted to use getopt() to follow Utility Syntax Guidelines.
 *	- Added message catalogue functionality.
 *	L002	30 Jun 95	scol!ianw
 *	- Fixed a problem spotted by lint, MemAlloc() should return a value
 *	  (this currently works because the value which should be returned
 *	  happens to be on the stack).
 */

#include "lzh-defs.h"
/* #include <errormsg.h> */           	                        /* L001 Start */
#include <errno.h>
#ifdef INTL
#  include <locale.h>
#  include "compress_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str) (str)
#  define catclose(d) /*void*/
#endif /* INTL */						/* L001 Stop */


/*
 * Print out an error message and exit the program.
 */
void
FatalError (char *errorMsg)
{
    errorl(MSGSTR(COMPRESS_FATALERROR, "Error: %s"), errorMsg); /* L001 */
    exit (1);
}

/*
 * Print out an error message and strerror and exit the program.
 */
void
FatalUnixError (char *errorMsg)
{
    psyserrorl(errno, MSGSTR(COMPRESS_FATALERROR, "Error: %s"),
	       errorMsg);					/* L001 */
    exit (1);
}

/*
 * Malloc interface that aborts in no memory is available.
 */
void *
MemAlloc (size_t size)
{
    void *memPtr;

    memPtr = malloc (size);
    if (memPtr == NULL)
        FatalError (MSGSTR(COMPRESS_NOMEM,"No memory available")); /* L001 */

    return(memPtr);						/* L002 */
}
