/*
 *  @(#) wdRectOps16.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
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
 * wdRectOps16.c
 *               wd 16-bit rectangular drawing ops.
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Tue Mar 16 14:50:51 PST 1993	buckm
 *		Created.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdBankMap.h"


/*
 * wdCopyRect16() - Copy a rectangle from screen to screen.
 *	pdstBox - the destination rectangle.
 *	psrc - the source point.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void 
wdCopyRect16(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv;
	int srcx, srcy, dstx, dsty;
	int w, h, dir;
	int srcAddr, dstAddr;

	w = (pdstBox->x2 - pdstBox->x1);
	h = (pdstBox->y2 - pdstBox->y1);
	assert(w > 0 && h > 0);

	w *= 2;

	srcx = psrc->x * 2;
	srcy = psrc->y;
	dstx = pdstBox->x1 * 2;
	dsty = pdstBox->y1;

	/*     
	 *  TOP_BOT means start with topmost pixels - copy upwards !!
	 *  LFT_RGT means start with leftmost pixels - copy to left !!
	 */
	if (srcy == dsty)
	{
	    if (srcx > dstx) dir = TOP_BOT_LFT_RGT;
	    else             dir = BOT_TOP_RGT_LFT;
	}
	else
	{
	    if (srcy > dsty) dir = TOP_BOT_LFT_RGT;
	    else             dir = BOT_TOP_RGT_LFT;
	}
	if (dir == BOT_TOP_RGT_LFT)
	{
	    srcx += w-1;
	    dstx += w-1;
	    srcy += h-1;
	    dsty += h-1;
	}

	wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);

	dstAddr =  dsty * wdPriv->fbStride + dstx;
	srcAddr =  srcy * wdPriv->fbStride + srcx;

	WAITFOR_WD();
	WRITE_1_REG( CNTRL_2_IND  , QUICK_START );
	WRITE_1_REG( RASTEROP_IND , alu << 8 );
	WRITE_1_REG( CNTRL_1_IND  , PACKED_MODE | dir );

	/* is planemask identical in each byte ? */
	if( ((planemask ^ (planemask >> 8)) & 0xFF) == 0 )
	{   /* just copy */
	    WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
	    WRITE_1_REG( DIM_X_IND     , w );
	    WRITE_1_REG( DIM_Y_IND     , h );
	    WRITE_2_REG( SOURCE_IND    , srcAddr );
	    WRITE_2_REG( DEST_IND      , dstAddr );
	}
	else if (dir == TOP_BOT_LFT_RGT)
	{   /* copy left to right, vertical 1-byte wide stripes */
	    int pm;

	    WRITE_1_REG( DIM_X_IND , 1 );
	    WRITE_1_REG( DIM_Y_IND , h );

	    do
	    {
		if (pm = planemask & 0xFF)
		{
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( SOURCE_IND    , srcAddr );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++srcAddr; ++dstAddr;
		if (pm = (planemask >> 8) & 0xFF)
		{
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( SOURCE_IND    , srcAddr );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++srcAddr; ++dstAddr;
	    } while ((w -= 2) > 0);
	}
	else
	{   /* copy right to left, vertical 1-byte wide stripes */
	    int pm;

	    WRITE_1_REG( DIM_X_IND , 1 );
	    WRITE_1_REG( DIM_Y_IND , h );

	    do
	    {
		if (pm = (planemask >> 8) & 0xFF)
		{
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( SOURCE_IND    , srcAddr );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		--srcAddr; --dstAddr;
		if (pm = planemask & 0xFF)
		{
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( SOURCE_IND    , srcAddr );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		--srcAddr; --dstAddr;
	    } while ((w -= 2) > 0);
	}
}


/*
 * wdDrawSolidRects16() - Draw solid-color rectangles.
 *	pbox - array of rectangles to draw.
 *	nbox - number of rectangles.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawSolidRects16(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv;
	int w, h;
	int dstAddr;

	assert( nbox );

	wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);

	w = (pbox->x2 - pbox->x1) * 2;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );
	dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 2;

	/* is planemask identical in each byte ? */
	if( ((planemask ^ (planemask >> 8)) & 0xFF) == 0 )
	{
	    /* is foreground identical in each byte (black/white) ? */
	    if( ((fg ^ (fg >> 8)) & 0xFF) == 0 )
	    {   /* just fill solid */
		WAITFOR_WD();
		WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
		WRITE_1_REG( RASTEROP_IND  , alu << 8 );
		WRITE_1_REG( FOREGR_IND    , fg & 0xFF );
		WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
		WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_FIXED_COL );

		WRITE_1_REG( DIM_X_IND     , w );
		WRITE_1_REG( DIM_Y_IND     , h );
		WRITE_2_REG( DEST_IND      , dstAddr);

		while( --nbox )
		{
		    pbox++;
		    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 2;
		    w = (pbox->x2 - pbox->x1) * 2;
		    h = pbox->y2 - pbox->y1;
		    assert( w > 0 && h > 0 );
		
		    WAITFOR_WD();
		    WRITE_1_REG( DIM_X_IND , w );
		    WRITE_1_REG( DIM_Y_IND , h );
		    WRITE_2_REG( DEST_IND  , dstAddr);
		}
	    }
	    else
	    {	/* use 8x8 pattern fills */
		int srcAddr = wdLoadFillTile16(wdPriv, fg);

		WAITFOR_WD();
		WRITE_1_REG( CNTRL_2_IND   , QUICK_START | PATTERN_8x8 );
		WRITE_1_REG( RASTEROP_IND  , alu << 8 );
		WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
		WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE );
		WRITE_2_REG( SOURCE_IND    , srcAddr);
		WRITE_1_REG( DIM_X_IND     , w );
		WRITE_1_REG( DIM_Y_IND     , h );
		WRITE_2_REG( DEST_IND      , dstAddr);

		while( --nbox )
		{
		    pbox++;
		    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 2;
		    w = (pbox->x2 - pbox->x1) * 2;
		    h = pbox->y2 - pbox->y1;
		    assert( w > 0 && h > 0 );
		
		    WAITFOR_WD();
		    WRITE_1_REG( DIM_X_IND , w );
		    WRITE_1_REG( DIM_Y_IND , h );
		    WRITE_2_REG( DEST_IND  , dstAddr);
		}
	    }
	}
	else	/* funky planemask; do vertical 1-byte wide stripes */
	{
	    int pm, f;

	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	    WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	    WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_FIXED_COL );
	    WRITE_1_REG( DIM_X_IND     , 1 );
	    WRITE_1_REG( DIM_Y_IND     , h );

	    do
	    {
		if (pm = planemask & 0xFF)
		{
		    f = fg & 0xFF;
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++dstAddr;
		if (pm = (planemask >> 8) & 0xFF)
		{
		    f = (fg >> 8) & 0xFF;
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++dstAddr;
	    } while ((w -= 2) > 0);

	    while( --nbox )
	    {
		pbox++;
		dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 2;
		w = (pbox->x2 - pbox->x1) * 2;
		h = pbox->y2 - pbox->y1;
		assert( w > 0 && h > 0 );
	    
		WAITFOR_WD();
		WRITE_1_REG( DIM_Y_IND , h );

		do
		{
		    if (pm = planemask & 0xFF)
		    {
			f = fg & 0xFF;
			WAITFOR_WD();
			WRITE_1_REG( PLANEMASK_IND , pm );
			WRITE_1_REG( FOREGR_IND    , f );
			WRITE_2_REG( DEST_IND      , dstAddr );
		    }
		    ++dstAddr;
		    if (pm = (planemask >> 8) & 0xFF)
		    {
			f = (fg >> 8) & 0xFF;
			WAITFOR_WD();
			WRITE_1_REG( PLANEMASK_IND , pm );
			WRITE_1_REG( FOREGR_IND    , f );
			WRITE_2_REG( DEST_IND      , dstAddr );
		    }
		    ++dstAddr;
		} while ((w -= 2) > 0);
	    }
	}
}


/*
 * wdDrawPoints16() - Draw points.
 *	ppt - array of points to draw.
 *	npts - number of points.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawPoints16(
	register DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv;
	int dstAddr;

	assert( npts );
	wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);

	dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 2;

	planemask &= 0xFFFF;

	if ((alu == GXcopy) && (planemask == 0xFFFF))
	{
	    unsigned char *pDst;

	    WD_MAP_AHEAD(wdPriv, dstAddr, 2, pDst);
	    WAITFOR_WD();
	    pDst[0] = ((unsigned char *)&fg)[0];
	    pDst[1] = ((unsigned char *)&fg)[1];

	    while( --npts > 0 )
	    {
		ppt++;
		dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 2;
		WD_MAP_AHEAD(wdPriv, dstAddr, 2, pDst);
		pDst[0] = ((unsigned char *)&fg)[0];
		pDst[1] = ((unsigned char *)&fg)[1];
	    }
	}
	else if( ((planemask ^ (planemask >> 8)) & 0xFF) == 0 )
	{
	    int f;

	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	    WRITE_1_REG( DIM_X_IND     , 1 );
	    WRITE_1_REG( DIM_Y_IND     , 1 );
	    WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	    WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
	    WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_FIXED_COL ); 

	    f = fg & 0xFF;
	    WRITE_1_REG( FOREGR_IND    , f );
	    WRITE_2_REG( DEST_IND      , dstAddr);
	    dstAddr++;
	    f = (fg >> 8) & 0xFF;
	    WRITE_1_REG( FOREGR_IND    , f );
	    WRITE_2_REG( DEST_IND      , dstAddr);

	    while( --npts > 0 )
	    {
		ppt++;
		dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 2;

		f = fg & 0xFF;
		WRITE_1_REG( FOREGR_IND , f );
		WRITE_2_REG( DEST_IND   , dstAddr);
		dstAddr++;
		f = (fg >> 8) & 0xFF;
		WRITE_1_REG( FOREGR_IND , f );
		WRITE_2_REG( DEST_IND   , dstAddr);
	    }
	}
	else
	{
	    int pm, f;

	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	    WRITE_1_REG( DIM_X_IND     , 1 );
	    WRITE_1_REG( DIM_Y_IND     , 1 );
	    WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	    WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_FIXED_COL ); 

	    if (pm = planemask & 0xFF)
	    {
		f = fg & 0xFF;
		WRITE_1_REG( PLANEMASK_IND , pm );
		WRITE_1_REG( FOREGR_IND    , f );
		WRITE_2_REG( DEST_IND      , dstAddr );
	    }
	    ++dstAddr;
	    if (pm = planemask >> 8)
	    {
		f = (fg >> 8) & 0xFF;
		WRITE_1_REG( PLANEMASK_IND , pm );
		WRITE_1_REG( FOREGR_IND    , f );
		WRITE_2_REG( DEST_IND      , dstAddr );
	    }

	    while( --npts > 0 )
	    {
		ppt++;
		dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 2;

		if (pm = planemask & 0xFF)
		{
		    f = fg & 0xFF;
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++dstAddr;
		if (pm = planemask >> 8)
		{
		    f = (fg >> 8) & 0xFF;
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
	    }
	}
}
