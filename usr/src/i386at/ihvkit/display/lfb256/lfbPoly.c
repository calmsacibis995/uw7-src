#ident	"@(#)ihvkit:display/lfb256/lfbPoly.c	1.1"

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

/*	HARDWARE POLYGON FILL		*/
/*		OPTIONAL		*/

SIvoid lfbPolygonClip(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;

{
}

SIBool lfbFillConvexPoly(count, ptsIn)
SIint32 count;
SIPointP ptsIn;

{
    return(SI_FAIL);
}

SIBool lfbFillGeneralPoly(count, ptsIn)
SIint32 count;
SIPointP ptsIn;

{
    return(SI_FAIL);
}

SIBool lfbFillRects(count, prects)
SIint32 count;
SIRectP prects;

{
    return(SI_FAIL);
}
