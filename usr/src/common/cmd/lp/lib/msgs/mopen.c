/*		copyright	"%c%" 	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mopen.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

# include	<errno.h>

# include	"lp.h"
# include	"msgs.h"


MESG	*lp_Md = 0;

/*
** mopen() - OPEN A MESSAGE PATH
*/

int
#ifdef	__STDC__
mopen (void)
#else
mopen ()
#endif
{
    if (lp_Md)
    {
	errno = EEXIST;
	return (-1);
    }

    if ((lp_Md = mconnect(Lp_FIFO, 0, 0)) == NULL)
	return(-1);

    return(0);
}
