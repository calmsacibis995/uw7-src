 /*
 *  @(#) wd33RectOps.c 11.1 97/10/22
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
 * wd33RectOps.c
 *               wd33 rectangular drawing ops.
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Mon 12-Aug-1993		edb@sco.com
 *              wd33CoverArea needs argument src_is_mono
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wd33Procs.h"

/*
 * wd33CopyRect() - Copy a rectangle from screen to screen.
 *	pdstBox - the destination rectangle.
 *	psrc - the source point.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void 
wd33CopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        register int curBlock= wdPriv->curRegBlock;
	int srcx, srcy, dstx, dsty;
	int w, h, dir = 0;

#ifdef DEBUG_PRINT
	ErrorF("wd33CopyRect(pdstBox=(%d,%d)-(%d,%d), ",
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
	 *  BOTTOM_TOP means start with bottommost pixel - progress upwards !!
	 *  RIGHT_LEFT means start with rightmost pixels - progress towards left !!
	 */
        if (srcx < dstx)  {
             dir = RIGHT_LEFT;
             srcx += w-1; 
             dstx += w-1;
        }
	if (srcy < dsty)  { 
             dir |= BOTTOM_TOP;
             srcy += h-1;
             dsty += h-1; 
        }

        WAITFOR_BUFF( 8 );

	WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_REG( ENG_1, SOURCE_X      , srcx );
	WRITE_REG( ENG_1, SOURCE_Y      , srcy );
	WRITE_REG( ENG_1, DEST_X        , dstx );
	WRITE_REG( ENG_1, DEST_Y        , dsty );
	WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );
	WRITE_REG( ENG_1, DIM_Y_IND     , h - 1 );

        WAITFOR_BUFF( 3 ); 
	WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
	WRITE_REG( ENG_1, CNTRL_2_IND   , wdPriv->bit11_10 | BIT_6_5 );
	WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | dir );
        wdPriv->curRegBlock = curBlock;
}


/*
 * wd33DrawSolidRects() - Draw solid-color rectangles.
 *	pbox - array of rectangles to draw.
 *	nbox - number of rectangles.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wd33DrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        register int curBlock= wdPriv->curRegBlock;
	int w, h;

#ifdef DEBUG_PRINT
	ErrorF("wd33DrawSolidRects(pbox=%x, nbox=%d, ", pbox,nbox);
	ErrorF("fg=%d, alu=%d, planemask=0x%x)\n",
		fg, alu, planemask);
#endif

	assert( nbox );

	WAITFOR_BUFF( 8 );

	WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
	WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
	WRITE_REG( ENG_1, CNTRL_2_IND   , wdPriv->bit11_10 | BIT_6_5 );

	while( nbox-- )
	{
	    w = pbox->x2 - pbox->x1 ;
	    h = pbox->y2 - pbox->y1 ;
	    assert( w > 0 && h > 0 );
	
 	    WAITFOR_BUFF( 5 );
	    WRITE_REG( ENG_1, DEST_X        , pbox->x1 );
	    WRITE_REG( ENG_1, DEST_Y        , pbox->y1 );
	    WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );
	    WRITE_REG( ENG_1, DIM_Y_IND     , h - 1 );
	    WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT  | SRC_IS_FIXED_COL );
	    pbox++;
	}
        wdPriv->curRegBlock = curBlock;;
}


/*
 * wd33DrawPoints() - Draw points.
 *	ppt - array of points to draw.
 *	npts - number of points.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wd33DrawPoints(
	register DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        register int curBlock= wdPriv->curRegBlock;

#ifdef DEBUG_PRINT
	ErrorF("wd33DrawPoints(ppt=%x, npts=%d, ", ppt,npts);
	ErrorF("fg=%d, alu=%d, planemask=0x%x)\n",
		fg, alu, planemask);
#endif

	assert( npts );

        WAITFOR_BUFF( 8 );

	WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
	WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
	WRITE_REG( ENG_1, DIM_X_IND     , 0 );
	WRITE_REG( ENG_1, DIM_Y_IND     , 0 );
	WRITE_REG( ENG_1, CNTRL_2_IND   , wdPriv->bit11_10 | BIT_6_5 );

	while( npts-- )
	{
 	    WAITFOR_BUFF( 3 );
	    WRITE_REG( ENG_1, DEST_X        , ppt->x );
	    WRITE_REG( ENG_1, DEST_Y        , ppt->y );
	    WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT  | SRC_IS_FIXED_COL );
	    ppt++;
	}
        wdPriv->curRegBlock = curBlock;
}


/*
 * wd33TileRects() - Draw rectangles using a tile.
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
wd33TileRects(
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
        register int curBlock;
	int xoff, yoff;
	int load_stat;
	int w, h;
	int srcx, srcy;

	assert( nbox );

#ifdef DEBUG_PRINT
  ErrorF("wd33TileRects( ,nbox=%d, ,tilew=%d, tileh=%d, ,alu=%x,planemask=%x)\n",
		nbox,tilew, tileh, alu, planemask);
#endif
	if((load_stat = wd33LoadTile(tile, stride, tilew, tileh,
				   wdPriv, 0)) == NOT_LOADED ) {
	    genTileRects(pbox, nbox, tile, stride, tilew, tileh, patOrg,
			 alu, planemask, pDraw);
	    return;
	}
#ifdef DEBUG_PRINT
  ErrorF("             Tile %s\n", load_stat == PREF_LOADED ? "PREF_LOADED" : "LOADED");
#endif
	/*
	 * Since patOrg may be negative, the following
	 * modulo operations _must_ be signed, not unsigned.
	 */
	xoff = tilew - (patOrg->x % (int)tilew);
	yoff = tileh - (patOrg->y % (int)tileh);

        curBlock= wdPriv->curRegBlock;

	WAITFOR_BUFF( 4 );
	WRITE_REG(   ENG_1, RASTEROP_IND    , alu << 8 );
	WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_REG(   ENG_1, CNTRL_2_IND     , wdPriv->bit11_10 | BIT_6_5 );

	while( nbox-- )
	{
	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % tilew;
	    srcy = (yoff + pbox->y1) % tileh;

	    if( load_stat == PREF_LOADED )
	    {
                int sourceX, sourceY;
                /*  The prefered pattern is stored linear */
                sourceX = wdPriv->tilePrefX + srcy * WD_TILE_WIDTH + srcx;
                sourceY = wdPriv->tilePrefY ;
		WAITFOR_BUFF( 8 );
		WRITE_REG( ENG_1, DEST_X     , pbox->x1 );
		WRITE_REG( ENG_1, DEST_Y     , pbox->y1 );
		WRITE_REG( ENG_1, SOURCE_X   , sourceX );
		WRITE_REG( ENG_1, SOURCE_Y   , sourceY );
		WRITE_REG( ENG_1, DIM_X_IND  , w - 1 );
		WRITE_REG( ENG_1, DIM_Y_IND  , h - 1 );
	        WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | PATTERN_8x8 );
	    }
	    else
	    {
		int minw, minh, small = TRUE;
                srcx += wdPriv->tileX;
                srcy += wdPriv->tileY;

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
		    WAITFOR_BUFF( 8 );
		    WRITE_REG( ENG_1, DEST_X     , pbox->x1 );
		    WRITE_REG( ENG_1, DEST_Y     , pbox->y1 );
		    WRITE_REG( ENG_1, SOURCE_X   , srcx );
		    WRITE_REG( ENG_1, SOURCE_Y   , srcy );
		    WRITE_REG( ENG_1, DIM_X_IND  , minw - 1 );
		    WRITE_REG( ENG_1, DIM_Y_IND  , minh - 1 );
	            WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT );

		    if( !small )
			wd33ReplicateArea( pbox->x1,pbox->y1, w, h, minw, minh, wdPriv );
		}
		else
		{
		    wd33CoverArea( srcx,srcy, tilew,tileh, pbox->x1,pbox->y1, w,h,
                                                     FALSE, wdPriv );
		}
	    }
       	    pbox++;
	}
        wdPriv->curRegBlock = curBlock;
}
