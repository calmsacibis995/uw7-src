/*		copyright	"%c%" 	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mneeds.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

/**
 ** mneeds() -  RETURN NUMBER OF FILE DESCRIPTORS NEEDED BY mopen()
 **/

int mneeds ( )
{
    /*
     * This is the expected number of file descriptors needed.
     */
    return (4);
}
