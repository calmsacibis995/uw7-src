/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)copystream.c	11.1"
#ident	"@(#)copystream.c	11.1"
#include "libmail.h"
/*
    NAME
	copystream - copy one FILE stream to another

    SYNOPSIS
	int copystream(FILE *infp, FILE *outfp)

    DESCRIPTION
	copystream() copies one stream to another. The stream
	infp must be opened for reading and the stream outfp
	must be opened for writing.

	It returns true if the stream is successively copied;
	false if any writes fail, or if SIGPIPE occurs while
	copying.
*/

static volatile int pipecatcher;

/* ARGSUSED */
static void catchsigpipe(i)
int i;
{
    pipecatcher = 1;
}

int copystream(infp, outfp)
register FILE *infp;
register FILE *outfp;
{
    char buffer[BUFSIZ];
    register int nread;
    void (*savsig)();

    pipecatcher = 0;
    savsig = signal(SIGPIPE, catchsigpipe);

    while (((nread = fread(buffer, sizeof(char), sizeof(buffer), infp)) > 0) &&
           (pipecatcher == 0))
	if (fwrite(buffer, sizeof(char), nread, outfp) != nread)
	    {
	    (void) signal(SIGPIPE, savsig);
	    return 0;
	    }

    if (fflush(outfp) == EOF)
	{
	(void) signal(SIGPIPE, savsig);
	return 0;
	}

    (void) signal(SIGPIPE, savsig);
    return (pipecatcher == 0);
}
