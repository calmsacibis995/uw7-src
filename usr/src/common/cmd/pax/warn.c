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

#ident	"@(#)pax:warn.c	1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/* 
 * warn.c - miscellaneous user warning routines 
 *
 * DESCRIPTION
 *
 *	These routines provide the user with various forms of warning
 *	and informational messages.
 *
 * AUTHOR
 *
 *     Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms and that any documentation, advertising materials, and other 
 * materials related to such distribution and use acknowledge that the 
 * software was developed * by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Revision 1.2  89/02/12  10:06:15  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:40  mark
 * Initial revision
 * 
 */

/* Headers */
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"




/* Function Prototypes */


static void prsize(FILE *, OFFSET);



/* warnarch - print an archive-related warning message and offset
 *
 * DESCRIPTION
 *
 *	Present the user with an error message and an archive offset at
 *	which the error occured.   This can be useful for diagnosing or
 *	fixing damaged archives.
 *
 * PARAMETERS
 *
 *	char 	*msg	- A message string to be printed for the user.
 *	OFFSET 	adjust	- An adjustment which is added to the current 
 *			  archive position to tell the user exactly where 
 *			  the error occurred.
 */


void warnarch(char *msg, OFFSET adjust)

{
    pfmt(stderr, MM_WARNING, WARN_OFF":[offset ");
    prsize(stderr, total - adjust);
    pfmt(stderr, MM_NOSTD, ":137:]: %s\n", msg);
}


/* prsize - print a file offset on a file stream
 *
 * DESCRIPTION
 *
 *	Prints a file offset to a specific file stream.  The file offset is
 *	of the form "%dm+%dk+%d", where the number preceeding the "m" and
 *	the "k" stand for the number of Megabytes and the number of
 *	Kilobytes, respectivley, which have been processed so far.
 *
 * PARAMETERS
 *
 *	FILE  *stream	- Stream which is to be used for output 
 *	OFFSET size	- Current archive position to be printed on the output 
 *			  stream in the form: "%dm+%dk+%d".
 *
 */


static void prsize(FILE *stream, OFFSET size)


{
    OFFSET          n;

    if (n = (size / (1024L * 1024L))) {
	pfmt(stream, MM_NOSTD, ":138:%ldm+", n);
	size -= n * 1024L * 1024L;
    }
    if (n = (size / 1024L)) {
	pfmt(stream, MM_NOSTD, ":139:%ldk+", n);
	size -= n * 1024L;
    }
    pfmt(stream, MM_NOSTD, ":140:%ld", size);
}


/* fatal - print fatal message and exit
 *
 * DESCRIPTION
 *
 *	Fatal prints the program's name along with an error message, then
 *	exits the program with a non-zero return code.
 *
 * PARAMETERS
 *
 *	char 	*why	- description of reason for termination 
 *		
 * RETURNS
 *
 *	Returns an exit code of 1 to the parent process.
 */


void fatal(char *why)

{
    pfmt(stderr, MM_ERROR, ":141:%s\n", why);
    exit(1);
}



/* warn - print a warning message
 *
 * DESCRIPTION
 *
 *	Print an error message listing the program name, the actual error
 *	which occurred and an informational message as to why the error
 *	occurred on the standard error device.  The standard error is
 *	flushed after the error is printed to assure that the user gets
 *	the message in a timely fasion.
 *
 *	The exit status the program return is set to 1.
 *
 * PARAMETERS
 *
 *	char *what	- Pointer to string describing what failed.
 *	char *why	- Pointer to string describing why did it failed.
 */


void warn(char *what, char *why)

{
    pfmt(stderr, MM_WARNING, ":142:%s : %s\n", what, why);
    fflush(stderr);
    exit_status = 1;
}
