/*
 *  @(#) wdRectOps.c 11.1 97/10/22
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
 * wdRectOps.c
 *               wd rectangular drawing ops.
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *	S001	Fri 09-Oct-1992	buckm@sco.com
 *		get rid of SourceValidate calls,
 *		  this work-around is no longer needed.
 *	S002	Thu  29-Oct-1992	edb@sco.com
 *              GC caching by checking of serial # changes
 *      S003    Thu 05-Oct-1992 edb@sco.com
 *              implement window op wdTileRects
 *	S004	Mon 09-Nov-1992	edb@sco.com
 *              minimize tile loading by checking for serialNumber
 *	S005	Sun 20-Dec-1992	buckm@sco.com
 *              pass 0 as serialNumber to LoadTile
 *      S006    Tue 09-Feb-1993 buckm@sco.com
 *              change parameter checks into assert()'s.
 *		get rid of GC caching.
 *		many other changes.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"


/*
 * wdCopyRect() - Copy a rectangle from screen to screen.
 *	pdstBox - the destination rectangle.
 *	psrc - the source point.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void 
wdCopyRect(
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

#ifdef DEBUG_PRINT
	ErrorF("wdCopyRect(pdstBox=(%d,%d)-(%d,%d), ",
		pdstBox->x1, pdstBox->y1, pdstBox->x2, pdstBox->y2);
	ErrorF("psrc=(%d,%d) alu=%d, planemask=0x%x)\n",
		psrc->x,psrc->y, alu, planemask);
#endif

	w = (pdstBox->x2 - pdstBox->x1) ;
	h = (pdstBox->y2 - pdstBox->y1) ;
	assert( w > 0 && h > 0 );

	srcx = psrc->x;
	srcy = psrc->y;
	dstx = pdstBox->x1;
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
	if( dir == BOT_TOP_RGT_LFT )
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

	WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	WRITE_2_REG( SOURCE_IND    , srcAddr);
	WRITE_2_REG( DEST_IND      , dstAddr);
	WRITE_1_REG( DIM_X_IND     , w );
	WRITE_1_REG( DIM_Y_IND     , h );
	WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );

	WRITE_1_REG( CNTRL_1_IND   , START | PACKED_MODE | dir );
}


/*
 * wdDrawSolidRects() - Draw solid-color rectangles.
 *	pbox - array of rectangles to draw.
 *	nbox - number of rectangles.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawSolidRects(
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

#ifdef DEBUG_PRINT
	ErrorF("wdDrawSolidRects(pbox=%x, nbox=%d, ", pbox,nbox);
	ErrorF("fg=%d, alu=%d, planemask=0x%x)\n",
		fg, alu, planemask);
#endif

	assert( nbox );
	wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);

	dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1;
	w = pbox->x2 - pbox->x1 ;
	h = pbox->y2 - pbox->y1 ;
	assert( w > 0 && h > 0 );

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	WRITE_1_REG( FOREGR_IND    , fg );
	WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
	WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_FIXED_COL );

	WRITE_1_REG( DIM_X_IND     , w );
	WRITE_1_REG( DIM_Y_IND     , h );
	WRITE_2_REG( DEST_IND      , dstAddr);

	while( --nbox )
	{
	    pbox++;
	    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1;
	    w = pbox->x2 - pbox->x1 ;
	    h = pbox->y2 - pbox->y1 ;
	    assert( w > 0 && h > 0 );
	
	    WAITFOR_WD();

	    WRITE_1_REG( DIM_X_IND , w );
	    WRITE_1_REG( DIM_Y_IND , h );
	    WRITE_2_REG( DEST_IND  , dstAddr);
	}
}


/*
 * wdDrawPoints() - Draw points.
 *	ppt - array of points to draw.
 *	npts - number of points.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawPoints(
	register DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	register int dstAddr;

#ifdef DEBUG_PRINT
	ErrorF("wdDrawPoints(ppt=%x, npts=%d, ", ppt,npts);
	ErrorF("fg=%d, alu=%d, planemask=0x%x)\n",
		fg, alu, planemask);
#endif

	assert( npts );

	dstAddr = ppt->y * wdPriv->fbStride + ppt->x;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	WRITE_1_REG( DIM_X_IND     , 1 );
	WRITE_1_REG( DIM_Y_IND     , 1 );
	WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	WRITE_1_REG( FOREGR_IND    , fg );
	WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
	WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_FIXED_COL ); 

	WRITE_2_REG( DEST_IND      , dstAddr);

	while( --npts )
	{
	    ppt++;
	    dstAddr =  ppt->y * wdPriv->fbStride + ppt->x;
	
	    /* WAITFOR_WD(); */
	    WRITE_2_REG( DEST_IND , dstAddr);
	}
}

/*    S003  vvvvvvvvvvvvvvvv  */
/*
 * wdTileRects() - Draw rectangles using a tile.
 *	pbox - array of destination rectangles to draw into.
 *	nbox - the number of rectangles.
 *	tile - pointer to the tile pixels.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within tile.
 *	w - the width  of the tile in pixels.
 *	h - the height of the tile in pixels.
 *	patOrg - the origin of the tile relative to the screen.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */

void
wdTileRects(
	register BoxPtr pbox,
	unsigned int nbox,
	unsigned char *tile,
	unsigned int stride,
	unsigned int tilew,
	unsigned int tileh,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	int xoff, yoff;
	int load_stat;
	int w, h;
	int srcx, srcy;
	int srcAddr, dstAddr;
	int controlCmd_2;

	assert( nbox );

	if((load_stat = wdLoadTile(tile, stride, tilew, tileh,
				   wdPriv, 0)) == NOT_LOADED ) {
	    genTileRects(pbox, nbox, tile, stride, tilew, tileh, patOrg,
			 alu, planemask, pDraw);
	    return;
	}
#ifdef DEBUG_PRINT
  ErrorF("wdTileRects( ,nbox=%d, ,tilew=%d, tileh=%d, ,alu=%x,planemask=%x)\n",
		nbox,tilew, tileh, alu, planemask);
#endif
	/*
	 * Since patOrg may be negative, the following
	 * modulo operations _must_ be signed, not unsigned.
	 */
	xoff = tilew - (patOrg->x % (int)tilew);
	yoff = tileh - (patOrg->y % (int)tileh);

	controlCmd_2 = QUICK_START ;
	if( load_stat == PREF_LOADED )
	      controlCmd_2 |= PATTERN_8x8;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND   , controlCmd_2 );
	WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
	WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE );

	while( nbox-- )
	{
	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % tilew;
	    srcy = (yoff + pbox->y1) % tileh;
	    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1;
       	    pbox++;

	    if( load_stat == PREF_LOADED )
	    {
		srcAddr = wdPriv->tilePrefStart + srcy * WD_TILE_WIDTH + srcx;
		WAITFOR_WD();
		WRITE_1_REG( DIM_X_IND  , w );
		WRITE_1_REG( DIM_Y_IND  , h );
		WRITE_2_REG( SOURCE_IND , srcAddr );
		WRITE_2_REG( DEST_IND   , dstAddr );
	    }
	    else
	    {
		int minw, minh, small = TRUE;

		srcAddr = wdPriv->tileStart + srcy * wdPriv->fbStride + srcx;

		if ((minw = w) > tilew)
		{
		    minw = tilew;
		    small = FALSE;
		}
		if ((minh = h) > tileh)
		{
		    minh = tileh;
		    small = FALSE;
		}

		if( small || (alu == GXcopy) )
		{
		    WAITFOR_WD();
		    WRITE_1_REG( DIM_X_IND  , minw );
		    WRITE_1_REG( DIM_Y_IND  , minh );
		    WRITE_2_REG( SOURCE_IND , srcAddr );
		    WRITE_2_REG( DEST_IND   , dstAddr );

		    if( !small )
			wdReplicateArea( dstAddr, w, h, minw, minh, wdPriv );
		}
		else
		{
		    wdCoverArea( srcAddr, tilew, tileh, dstAddr, w, h, wdPriv );
		}
	    }
	}
}
/*    S003  ^^^^^^^^^^^^^^^^  */
