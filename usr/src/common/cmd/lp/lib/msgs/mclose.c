/*		copyright	"%c%" 	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mclose.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#include "lp.h"
#include "msgs.h"

extern MESG	*lp_Md;

int mclose()
{
    MESG	*md = lp_Md;

    lp_Md = 0;

    return(mdisconnect(md));
}
