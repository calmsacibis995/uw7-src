#ident	"@(#)ihvkit:display/lfb256/lfbGlob.c	1.1"

/*
 * Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	Copyright (c) 1993  Intel Corporation
 *	  All Rights Reserved
 */

#include <lfb.h>

GenFB lfb;
void (*lfbVendorFlush)();

SIGState lfbGStates[LFB_NUM_GSTATES] = {
    {
	PMSK,			/* AllPlanes */
	GXcopy,			/* mode */
	SGStipple,		/* stipple mode */
	SGFillSolidFG,		/* fill mode */
	SGEvenOddRule,		/* polygon fill rule */
	SGArcPieSlice,		/* arc fill rule */
	SGLineSolid,		/* line style */
	0,			/* fg */
	1,			/* bg */
	0,			/* cmapIdx */
	0,			/* visualIdx */
	(SIbitmapP) 0,		/* tile */
	(SIbitmapP) 0,		/* stipple */
	0,			/* lineCnt */
	(SIint32 *)0,		/* line */
	(SIRectP)0,		/* clipList */
	0,			/* clipCnt */
	{{0,0},{2048,2048}}	/* clipExtent */
    }
};

SIGStateP lfb_cur_GStateP = &lfbGStates[0];
int lfb_cur_GState_idx = 0;

ScreenInterface lfbDisplayInterface = {

    /*	MISCELLANEOUS ROUTINES		*/
    /*		MANDATORY		*/

    lfbInitLFB,				/* machine dependant init	*/
    lfbShutdownLFB,     			/* machine dependant cleanup	*/
    (SIBool (*)())NULL,			/* machine dependant vt flip	*/
    (SIBool (*)())NULL,			/* machine dependant vt flip	*/
    (SIBool (*)())NULL,			/* Turn on/off video blank	*/
    (SIBool (*)())NULL,			/* start caching requests	*/
    (SIBool (*)())NULL,			/* write caching requests	*/
    lfbDownLoadState,			/* Set current state info	*/
    lfbGetState,			/* Get current state info	*/
    lfbSelectState,			/* Select current GS entry	*/
    (SIBool (*)())NULL,			/* Enter/Leave a screen		*/

    /*	SCANLINE AT A TIME ROUTINES	*/
    /*		MANDATORY		*/

    lfbGetSL,				/* get pixels in a scanline	*/
    lfbSetSL,				/* set pixels in a scanline	*/
    lfbFreeSL,				/* free scanline buffer		*/

    /*	COLORMAP MANAGEMENT ROUTINES	*/
    /*		MANDATORY		*/

    (SIBool (*)())NULL,			/* Set Colormap entries		*/
    (SIBool (*)())NULL,			/* Get Colormap entries		*/

    /*	CURSOR CONTROL ROUTINES		*/
    /*		MANDATORY		*/

    (SIBool (*)())NULL,			/* Download a cursor		*/
    (SIBool (*)())NULL,			/* Turn on the cursor		*/
    (SIBool (*)())NULL,			/* Turn off the cursor		*/
    (SIBool (*)())NULL,			/* Move the cursor position	*/

    /*	HARDWARE SPANS CONTROL		*/
    /*		OPTIONAL		*/

    lfbFillSpans,			/* fill spans			*/

    /*	HARDWARE BITBLT ROUTINES	*/
    /*		OPTIONAL		*/

    lfbSSbitblt,			/* perform scr->scr bitblt	*/
    lfbMSbitblt,			/* perform mem->scr bitblt	*/
    lfbSMbitblt,			/* perform scr->mem bitblt	*/

    /*	HARDWARE STPLBLT ROUTINES	*/
    /*		OPTIONAL		*/

    lfbSSstplblt,			/* perform scr->scr stipple	*/
    lfbMSstplblt,			/* perform mem->scr stipple	*/
    lfbSMstplblt,			/* perform scr->mem stipple	*/

    /*	HARDWARE POLYGON FILL		*/
    /*		OPTIONAL		*/

    lfbPolygonClip,			/* set polygon clip		*/
    lfbFillConvexPoly,			/* for convex polygons		*/
    lfbFillGeneralPoly,			/* for general polygons		*/
    lfbFillRects,			/* for rectangular regions	*/

    /*	HARDWARE POINT PLOTTING		*/
    /*		OPTIONAL		*/

    lfbPlotPoints,			/* plot points			*/

    /*	HARDWARE LINE DRAWING		*/
    /*		OPTIONAL		*/

    lfbLineClip,			/* set line draw clip		*/
    lfbThinLines,			/* One bit line (connected)	*/
    lfbThinSegments,			/* One bit line segments	*/
    lfbThinRect,			/* One bit line rectangles	*/

    /*	HARDWARE DRAW ARC ROUTINE	*/
    /*		OPTIONAL		*/

    lfbDrawArcClip,			/* set drawarc clip		*/
    lfbDrawArc,				/* draw arc			*/

    /*	HARDWARE FILL ARC ROUTINE	*/
    /*		OPTIONAL		*/

    lfbFillArcClip,			/* set fill arc clip		*/
    lfbFillArc,				/* fill arc			*/

    /*	HARDWARE FONT CONTROL		*/
    /*		OPTIONAL		*/

    lfbCheckDLFont,			/* Check if font downloadable	*/
    lfbDownLoadFont,			/* Download a font command	*/
    lfbFreeFont,			/* free a downloaded font	*/
    lfbFontClip,			/* set font clip		*/
    lfbStplbltFont,			/* stipple a list of glyphs	*/

    /*	SDD MEMORY CACHING CONTROL	*/
    /*		OPTIONAL		*/

    lfbAllocCache,			/* allocate pixmap into cache	*/
    lfbFreeCache,			/* remove pixmap from cache	*/
    lfbLockCache,			/* lock pixmap into cache	*/
    lfbUnlockCache,			/* unlock pixmap from cache	*/


    /*	SDD EXTENSION INITIALIZATION	*/
    /*		OPTIONAL		*/

    lfbInitExten,			/* extension initialization	*/
};
