/*
 *  @(#) wdBres.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 * wdBres.c   Bresenham linedrawer
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *	      copied fr. R4 driver
 *	S001	Th  29-Oct-1992	edb@sco.com
 *	      Fix bug in linedrawer ( timing )
 *	      GC caching by checking of serial # changes
 *	S002	Wed 16-Dec-1992 buckm@sco.com
 *		Ifdef out unused fast line drawing code.
 *	S003	Fri 12-Feb-1993 buckm@sco.com
 *		Delete code ifdef'd in S002. 
 *		Get rid of GC caching.
 *		Tighten-up inner loop.
 *		Use direct frame buffer access for GXcopy.
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "ddxScreen.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"
#include "wdBankMap.h"


#define	STEP(addr) \
	if( e > 0 ) \
	{ \
	    (addr) += diagStep;		/* make diagonal step */ \
	    e += e2; \
	} \
	else \
	{ \
	    (addr) += linearStep;	/* make linear step   */ \
	    e += e1; \
	}


void
wdSolidZeroSeg(
	GCPtr pGC,
	DrawablePtr pDraw,
	int signdx,
	int signdy,
	int axis,
	int x1,
	int y1,
	register int e,
	int e1,
	int e2,
	int len )
{
	register wdScrnPrivPtr wdPriv;
	nfbGCPrivPtr pGCPriv;
	int dstRow;
	int xStep, yStep;
	int diagStep, linearStep;
	int alu, planemask, fg;

#ifdef DEBUG_PRINT
	ErrorF("wdSolidZeroSeg( x1=%d, y1=%d, ... len=%d\n", 
				 x1,y1,len );
#endif

	assert( len );

	wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	pGCPriv = NFB_GC_PRIV(pGC);

	alu = pGCPriv->rRop.alu;
	planemask = pGCPriv->rRop.planemask & 0xFF;
	fg = pGCPriv->rRop.fg;

	dstRow = y1 * wdPriv->fbStride;

	xStep =  signdx;
	yStep =  signdy * wdPriv->fbStride;
	diagStep = xStep + yStep;
	linearStep = (axis == X_AXIS) ? xStep : yStep;

	WAITFOR_WD();

	if ((alu == GXcopy) && (planemask == 0xFF))
	{
	    register unsigned char *p;
	    int bumper = wdPriv->fbStride * 4;

	    if (diagStep < 0)
	    {
		dstRow += wdPriv->fbStride;
		WD_MAP_BEHIND(wdPriv, dstRow, bumper, p);
		*(p -= wdPriv->fbStride - x1) = fg;
		switch( --len & 3 )
		{ 
		case 3:  STEP( p );  *p = fg;
		case 2:  STEP( p );  *p = fg;
		case 1:  STEP( p );  *p = fg;
		}
		while( (len -= 4) >= 0 )
		{
		    STEP( p );
		    WD_REMAP_BEHIND(wdPriv, p, bumper);
				*p = fg;
		    STEP( p );  *p = fg;
		    STEP( p );  *p = fg;
		    STEP( p );  *p = fg;
		}
	    }
	    else
	    {
		WD_MAP_AHEAD(wdPriv, dstRow, bumper, p);
		*(p += x1) = fg;
		switch( --len & 3 )
		{ 
		case 3:  STEP( p );  *p = fg;
		case 2:  STEP( p );  *p = fg;
		case 1:  STEP( p );  *p = fg;
		}
		while( (len -= 4) >= 0 )
		{
		    STEP( p );
		    WD_REMAP_AHEAD(wdPriv, p, bumper);
				*p = fg;
		    STEP( p );  *p = fg;
		    STEP( p );  *p = fg;
		    STEP( p );  *p = fg;
		}
	    }
	}
	else
	{
	    int dstAddr = dstRow + x1;

	    WRITE_1_REG( CNTRL_2_IND    , QUICK_START );
	    WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	    WRITE_1_REG( FOREGR_IND     , fg );
	    WRITE_1_REG( PLANEMASK_IND  , planemask );
	    WRITE_1_REG( DIM_X_IND      , 1 );
	    WRITE_1_REG( DIM_Y_IND      , 1 );
	    WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_FIXED_COL );

	    WRITE_2_REG( DEST_IND, dstAddr);

	    while ( --len > 0 )
	    { 
		STEP( dstAddr );
		/* WAITFOR_WD();	should be able to skip this */
		WRITE_2_REG( DEST_IND, dstAddr );
	    }
	}
}

