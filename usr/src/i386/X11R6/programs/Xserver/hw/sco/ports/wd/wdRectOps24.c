/*
 *  @(#) wdRectOps24.c 11.1 97/10/22
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
 * wdRectOps24.c
 *               wd 24-bit rectangular drawing ops.
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon Feb 15 23:07:03 PST 1993	buckm
 *		Created.
 *	S001	Wdn 19-May-93  edb@sco.com
 *              Add debug print
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
 * wdCopyRect24() - Copy a rectangle from screen to screen.
 *	pdstBox - the destination rectangle.
 *	psrc - the source point.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void 
wdCopyRect24(
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

	w *= 3;

	srcx = psrc->x * 3;
	srcy = psrc->y;
	dstx = pdstBox->x1 * 3;
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
	if( ((planemask ^ (planemask >> 8)) & 0xFFFF) == 0 )
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
		if (pm = (planemask >> 16) & 0xFF)
		{
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( SOURCE_IND    , srcAddr );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++srcAddr; ++dstAddr;
	    } while ((w -= 3) > 0);
	}
	else
	{   /* copy right to left, vertical 1-byte wide stripes */
	    int pm;

	    WRITE_1_REG( DIM_X_IND , 1 );
	    WRITE_1_REG( DIM_Y_IND , h );

	    do
	    {
		if (pm = (planemask >> 16) & 0xFF)
		{
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( SOURCE_IND    , srcAddr );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		--srcAddr; --dstAddr;
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
	    } while ((w -= 3) > 0);
	}
}


/* * wdDrawSolidRects24() - Draw solid-color rectangles.
 *	pbox - array of rectangles to draw.
 *	nbox - number of rectangles.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawSolidRects24(
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

#ifdef DEBUG_PRINT
        ErrorF("wdDrawSolidRects24( nbox=%d,fg=%x,alu=%x,planemask=%x )\n",
             nbox,fg,alu,planemask);
#endif
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );
	dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 3;

	/* is planemask identical in each byte ? */
	if( ((planemask ^ (planemask >> 8)) & 0xFFFF) == 0 )
	{
	    /* is foreground identical in each byte (black/white/gray) ? */
	    if( ((fg ^ (fg >> 8)) & 0xFFFF) == 0 )
	    {   /* just fill solid */
		WAITFOR_WD();
		WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
		WRITE_1_REG( RASTEROP_IND  , alu << 8 );
		WRITE_1_REG( FOREGR_IND    , fg & 0xFF );
		WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
		WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_FIXED_COL );

		WRITE_1_REG( DIM_X_IND     , w * 3 );
		WRITE_1_REG( DIM_Y_IND     , h );
		WRITE_2_REG( DEST_IND      , dstAddr);

		while( --nbox )
		{
		    pbox++;
		    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 3;
		    w = pbox->x2 - pbox->x1;
		    h = pbox->y2 - pbox->y1;
		    assert( w > 0 && h > 0 );
		    WAITFOR_WD();
		    WRITE_1_REG( DIM_X_IND , w * 3 );
		    WRITE_1_REG( DIM_Y_IND , h );
		    WRITE_2_REG( DEST_IND  , dstAddr);
		}
	    }
	    else if (alu == GXcopy)
	    {	/* set top-left pixel, then replicate it */
		unsigned char *pDst;

		w *= 3;
		WD_MAP_AHEAD(wdPriv, dstAddr, w, pDst);
		WAITFOR_WD();
		pDst[0] = ((unsigned char *)&fg)[0];
		pDst[1] = ((unsigned char *)&fg)[1];
		pDst[2] = ((unsigned char *)&fg)[2];
		WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
		WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
		if (w > 3 || h > 1)
		    wdReplicateArea(dstAddr, w, h, 3, 1, wdPriv);

		while( --nbox )
		{
		    pbox++;
		    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 3;
		    w = pbox->x2 - pbox->x1;
		    h = pbox->y2 - pbox->y1;
		    assert( w > 0 && h > 0 );
		
		    w *= 3;
		    WD_MAP_AHEAD(wdPriv, dstAddr, w, pDst);
		    WAITFOR_WD();
		    pDst[0] = ((unsigned char *)&fg)[0];
		    pDst[1] = ((unsigned char *)&fg)[1];
		    pDst[2] = ((unsigned char *)&fg)[2];
		    if (w > 3 || h > 1)
			wdReplicateArea(dstAddr, w, h, 3, 1, wdPriv);
		}
	    }
	    else
	    {	/* do vertical 2-pixel (6-byte) wide stripes */
		int srcAddr = wdLoadFillTile24(wdPriv, fg);

		WAITFOR_WD();
		WRITE_1_REG( CNTRL_2_IND   , QUICK_START | PATTERN_8x8 );
		WRITE_1_REG( RASTEROP_IND  , alu << 8 );
		WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
		WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE );
		WRITE_2_REG( SOURCE_IND    , srcAddr);
		WRITE_1_REG( DIM_X_IND     , 6 );
		WRITE_1_REG( DIM_Y_IND     , h );

		while ((w -= 2) >= 0)
		{
		    WAITFOR_WD();
		    WRITE_2_REG( DEST_IND  , dstAddr);
		    dstAddr += 6;
		}
		if (w & 1)
		{
		    WAITFOR_WD();
		    WRITE_1_REG( DIM_X_IND , 3 );
		    WRITE_2_REG( DEST_IND  , dstAddr);
		}

		while( --nbox )
		{
		    pbox++;
		    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 3;
		    w = pbox->x2 - pbox->x1;
		    h = pbox->y2 - pbox->y1;
		    assert( w > 0 && h > 0 );
		
		    WAITFOR_WD();
		    WRITE_1_REG( DIM_X_IND     , 6 );
		    WRITE_1_REG( DIM_Y_IND     , h );

		    while ((w -= 2) >= 0)
		    {
			WAITFOR_WD();
			WRITE_2_REG( DEST_IND  , dstAddr);
			dstAddr += 6;
		    }
		    if (w & 1)
		    {
			WAITFOR_WD();
			WRITE_1_REG( DIM_X_IND , 3 );
			WRITE_2_REG( DEST_IND  , dstAddr);
		    }
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
		if (pm = (planemask >> 16) & 0xFF)
		{
		    f = (fg >> 16) & 0xFF;
		    WAITFOR_WD();
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++dstAddr;
	    } while (--w > 0);

	    while( --nbox )
	    {
		pbox++;
		dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 3;
		w = pbox->x2 - pbox->x1;
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
		    if (pm = (planemask >> 16) & 0xFF)
		    {
			f = (fg >> 16) & 0xFF;
			WAITFOR_WD();
			WRITE_1_REG( PLANEMASK_IND , pm );
			WRITE_1_REG( FOREGR_IND    , f );
			WRITE_2_REG( DEST_IND      , dstAddr );
		    }
		    ++dstAddr;
		} while (--w > 0);
	    }
	}
}


/*
 * wdDrawPoints24() - Draw points.
 *	ppt - array of points to draw.
 *	npts - number of points.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawPoints24(
	register DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	int dstAddr;

	assert( npts );

	dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 3;

	planemask &= 0xFFFFFF;

	if ((alu == GXcopy) && (planemask == 0xFFFFFF))
	{
	    unsigned char *pDst;

	    WD_MAP_AHEAD(wdPriv, dstAddr, 3, pDst);
	    WAITFOR_WD();
	    pDst[0] = ((unsigned char *)&fg)[0];
	    pDst[1] = ((unsigned char *)&fg)[1];
	    pDst[2] = ((unsigned char *)&fg)[2];

	    while( --npts > 0 )
	    {
		ppt++;
		dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 3;
		WD_MAP_AHEAD(wdPriv, dstAddr, 3, pDst);
		pDst[0] = ((unsigned char *)&fg)[0];
		pDst[1] = ((unsigned char *)&fg)[1];
		pDst[2] = ((unsigned char *)&fg)[2];
	    }
	}
	else if( ((planemask ^ (planemask >> 8)) & 0xFFFF) == 0 )
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
	    dstAddr++;
	    f = (fg >> 16) & 0xFF;
	    WRITE_1_REG( FOREGR_IND    , f );
	    WRITE_2_REG( DEST_IND      , dstAddr);

	    while( --npts > 0 )
	    {
		ppt++;
		dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 3;

		f = fg & 0xFF;
		WRITE_1_REG( FOREGR_IND , f );
		WRITE_2_REG( DEST_IND   , dstAddr);
		dstAddr++;
		f = (fg >> 8) & 0xFF;
		WRITE_1_REG( FOREGR_IND , f );
		WRITE_2_REG( DEST_IND   , dstAddr);
		dstAddr++;
		f = (fg >> 16) & 0xFF;
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
	    if (pm = (planemask >> 8) & 0xFF)
	    {
		f = (fg >> 8) & 0xFF;
		WRITE_1_REG( PLANEMASK_IND , pm );
		WRITE_1_REG( FOREGR_IND    , f );
		WRITE_2_REG( DEST_IND      , dstAddr );
	    }
	    ++dstAddr;
	    if (pm = (planemask >> 16) & 0xFF)
	    {
		f = (fg >> 16) & 0xFF;
		WRITE_1_REG( PLANEMASK_IND , pm );
		WRITE_1_REG( FOREGR_IND    , f );
		WRITE_2_REG( DEST_IND      , dstAddr );
	    }

	    while( --npts > 0 )
	    {
		ppt++;
		dstAddr = ppt->y * wdPriv->fbStride + ppt->x * 3;

		if (pm = planemask & 0xFF)
		{
		    f = fg & 0xFF;
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++dstAddr;
		if (pm = (planemask >> 8) & 0xFF)
		{
		    f = (fg >> 8) & 0xFF;
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
		++dstAddr;
		if (pm = (planemask >> 16) & 0xFF)
		{
		    f = (fg >> 16) & 0xFF;
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_1_REG( FOREGR_IND    , f );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		}
	    }
	}
}
