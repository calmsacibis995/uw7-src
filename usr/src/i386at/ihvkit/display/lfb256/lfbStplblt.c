#ident	"@(#)ihvkit:display/lfb256/lfbStplblt.c	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <lfb.h>

/*	HARDWARE STPLBLT ROUTINES	*/
/*		OPTIONAL		*/

SIBool lfbSSstplblt(sx, sy, dx, dy, wid, hgt, plane, forcetype)
SIint32 sx, sy, dx, dy, wid, hgt, plane;
SIint32 forcetype;

{
    return(SI_FAIL);
}

SIBool lfbMSstplblt(src, sx, sy, dx, dy, wid, hgt, plane, forcetype)
SIbitmapP src;
SIint32 sx, sy, dx, dy, wid, hgt, plane;
SIint32 forcetype;

{
    return(SI_FAIL);
}

SIBool lfbSMstplblt(dst, sx, sy, dx, dy, wid, hgt, plane, forcetype)
SIbitmapP dst;
SIint32 sx, sy, dx, dy, wid, hgt, plane;
SIint32 forcetype;

{
    return(SI_FAIL);
}
