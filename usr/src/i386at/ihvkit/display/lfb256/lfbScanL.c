#ident	"@(#)ihvkit:display/lfb256/lfbScanL.c	1.1"

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

#include <string.h>
#include <lfb.h>

/*	SCANLINE AT A TIME ROUTINES	*/
/*		MANDATORY		*/

SILine lfbGetSL(y)
SIint32 y;

{
    if (y >= lfb.height)
	return((SILine)NULL);

    /* 
     * Flush the graphics hardware
     *
     */
    if (lfbVendorFlush)
	(*lfbVendorFlush)();

    return(ScreenAddr(0, y));
}

SIvoid lfbSetSL(y, ptr)
SIint32 y;
SILine ptr;

{
    if ((ptr != ScreenAddr(0, y)) &&
	(y < lfb.height)) {

	/* 
	 * Flush the graphics hardware
	 *
	 */
	if (lfbVendorFlush)
	    (*lfbVendorFlush)();

	memmove(ScreenAddr(0, y), ptr, lfb.stride*sizeof(Pixel));
    }
}

SIvoid lfbFreeSL()

{
    /* No op.  We do not allocate scan lines in lfbGetSL */
}
