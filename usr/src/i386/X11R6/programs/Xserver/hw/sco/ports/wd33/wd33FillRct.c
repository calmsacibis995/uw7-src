/*
 *  @(#) wd33FillRct.c 11.1 97/10/22
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
 * wd33FillRct.c
 *               wd33 rectangular drawing ops.
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Mon 12-Aug-1993		edb@sco.com
 *              Remove code used to workaround a chip bug in wd90c31
 *              ( problem for alus not needing the source )
 *	S002	Mon 12-Aug-1993		edb@sco.com
 *              wd33CoverArea needs argument src_is_mono
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"	

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wd33Procs.h"


void
wd33SolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox )
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        register int curBlock = wdPriv->curRegBlock;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	unsigned long fg;
	unsigned long planemask;
	unsigned char alu;
	int w, h;

#ifdef DEBUG_PRINT
	ErrorF("wd33SolidFillRects(pbox=%x, nbox=%d, ", pbox,nbox);
	ErrorF("fg=%d, alu=%d, planemask=0x%x)\n",
	    pGCPriv->rRop.fg, pGCPriv->rRop.alu, pGCPriv->rRop.planemask);
#endif

	assert( nbox );

	fg = pGCPriv->rRop.fg;
	planemask = pGCPriv->rRop.planemask;
	alu = pGCPriv->rRop.alu;

	WAITFOR_BUFF( 6 );

	WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
	WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
	WRITE_REG( ENG_1, CNTRL_2_IND   , wdPriv->bit11_10 | BIT_6_5 );

	while( nbox-- )
	{
	    w = pbox->x2 - pbox->x1;
	    h = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );

	    WAITFOR_BUFF( 6 );
	    WRITE_REG( ENG_1, DIM_X_IND        , w - 1 );
	    WRITE_REG( ENG_1, DIM_Y_IND        , h - 1 );
	    WRITE_REG( ENG_1, DEST_X        , pbox->x1 );
	    WRITE_REG( ENG_1, DEST_Y        , pbox->y1 );
	    WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | SRC_IS_FIXED_COL );
	    pbox++;
	}
        wdPriv->curRegBlock = curBlock;
}


void
wd33TiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        register char curBlock;
	PixmapPtr pm;
	int load_stat;
	int tilew, tileh;
	int xoff, yoff;
	int w, h;
	int srcx, srcy;
	unsigned long planemask;
	unsigned char alu;

	assert( nbox );

	pm = pGC->tile.pixmap;

	tilew = pm->drawable.width;
	tileh = pm->drawable.height;
	planemask = pGCPriv->rRop.planemask;
	alu = pGCPriv->rRop.alu;

#ifdef DEBUG_PRINT
	ErrorF("wd33TiledFillRects( ,nbox=%d, ,tilew=%d, tileh=%d, \n\
		           ,alu=%x,planemask=%x)\n",
	       nbox,tilew, tileh, alu, planemask);
#endif

	if( (load_stat = wd33LoadTile(pm->devPrivate.ptr, pm->devKind,
				tilew, tileh, wdPriv,
				pm->drawable.serialNumber)) == NOT_LOADED) {
		genTileRects(pbox, nbox, pm->devPrivate.ptr, pm->devKind,
			tilew, tileh, &(pGCPriv->screenPatOrg),
			alu,planemask, pDraw);
		return;
	}

#ifdef DEBUG_PRINT
  ErrorF("             Tile %s\n", load_stat == PREF_LOADED ? "PREF_LOADED" : "LOADED");
#endif
	xoff = tilew - (pGCPriv->screenPatOrg.x % tilew);
	yoff = tileh - (pGCPriv->screenPatOrg.y % tileh);

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
		    wd33CoverArea( srcx,srcy, tilew, tileh, pbox->x1,pbox->y1, w, h,
                                                             FALSE , wdPriv );
		}
	    }
       	    pbox++;
	}
        wdPriv->curRegBlock = curBlock;
}

void
wd33StippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        register int curBlock;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	PixmapPtr pm;
	int load_stat;
	int stipw, stiph;
	int xoff, yoff;
	int w, h;
	int srcx, srcy;
	unsigned long planemask;
	unsigned long fg;
	unsigned char alu;

	assert( nbox );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
	alu = pGCPriv->rRop.alu;
	fg =  pGCPriv->rRop.fg;
	planemask =  pGCPriv->rRop.planemask & 0xFF;

#ifdef DEBUG_PRINT
	ErrorF("wd33StippledFillRects( ,nbox=%d, ,stipw=%d, stiph=%d, \n\
		        fg=%d,alu=%x,planemask=%x)\n",
	       nbox,stipw, stiph,fg, alu, planemask);
#endif

	if( (load_stat = wd33LoadStipple(pm->devPrivate.ptr, pm->devKind,
				stipw, stiph, wdPriv,
				pm->drawable.serialNumber)) == NOT_LOADED) {
	       genStippledFillRects(pGC, pDraw, pbox, nbox);
	       return;
	}
#ifdef DEBUG_PRINT
  ErrorF("             Stipple %s\n", load_stat == PREF_LOADED ? "PREF_LOADED" : "LOADED");
#endif

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

        curBlock= wdPriv->curRegBlock;

        WAITFOR_BUFF( 8 );
        WRITE_REG(   ENG_1, RASTEROP_IND    , alu << 8 );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_2_REG( ENG_2, TRANSPCOL_IND_0 , wdPriv->planeMaskMask );
        WAITFOR_BUFF( 6 );
	WRITE_2_REG( ENG_2, TRANSPMASK_IND_0, 1 );
	WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WRITE_REG(   ENG_1, CNTRL_2_IND     , wdPriv->bit11_10 | BIT_6_5 | MONO_TRANSP_ENAB);

	while( nbox-- )
	{
	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % stipw;
	    srcy = (yoff + pbox->y1) % stiph;

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
                WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | PATTERN_8x8 | SRC_IS_MONO_COMP);
	    }
	    else
	    {
		int minw, minh, small = TRUE;
                srcx += wdPriv->tileX;
                srcy += wdPriv->tileY;

		if ((minw = w) > stipw)
		{
		    minw = stipw;
		    small = FALSE;
		}
		if ((minh = h) > stiph)
		{
		    minh = stiph;
		    small = FALSE;
		}

		if( small )
		{
                    WAITFOR_BUFF( 8 );
                    WRITE_REG( ENG_1, DEST_X     , pbox->x1 );
                    WRITE_REG( ENG_1, DEST_Y     , pbox->y1 );
                    WRITE_REG( ENG_1, SOURCE_X   , srcx );
                    WRITE_REG( ENG_1, SOURCE_Y   , srcy );
                    WRITE_REG( ENG_1, DIM_X_IND  , minw - 1 );
                    WRITE_REG( ENG_1, DIM_Y_IND  , minh - 1 );
                    WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | SRC_IS_MONO_COMP );
		}
		else
		{
		    wd33CoverArea( srcx,srcy, stipw, stiph, pbox->x1,pbox->y1, w, h,
                                  TRUE, wdPriv );
		}
	    }
       	    pbox++;
	}
       wdPriv->curRegBlock = curBlock;
}

void
wd33OpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        register int curBlock;
	PixmapPtr pm;
	int stipw, stiph;
	int load_stat;
	int xoff, yoff;
	int w, h;
	int srcx, srcy;
	unsigned long planemask;
	unsigned long fg;
	unsigned long bg;
	unsigned char alu;

	assert( nbox );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
	alu = pGCPriv->rRop.alu;
	fg =  pGCPriv->rRop.fg;
	bg =  pGCPriv->rRop.bg;
	planemask =  pGCPriv->rRop.planemask;

#ifdef DEBUG_PRINT
	ErrorF("wd33OpStippledFillRects( ,nbox=%d, ,stipw=%d, stiph=%d, \n\
		        fg=%d,bg=%d,alu=%x,planemask=%x)\n",
	       nbox,stipw, stiph,fg,bg, alu, planemask);
#endif

	if( (load_stat = wd33LoadStipple(pm->devPrivate.ptr, pm->devKind, 
			    stipw, stiph, wdPriv,
			    pm->drawable.serialNumber)) == NOT_LOADED ) {
	       genOpStippledFillRects(pGC, pDraw, pbox, nbox);
	       return;
	}
#ifdef DEBUG_PRINT
  ErrorF("             Stipple %s\n", load_stat == PREF_LOADED ? "PREF_LOADED" : "LOADED");
#endif

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

        curBlock= wdPriv->curRegBlock;
        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
        WRITE_2_REG( ENG_2, TRANSPCOL_IND_0 , wdPriv->planeMaskMask );
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WAITFOR_BUFF( 8 );
	WRITE_2_REG( ENG_2, BACKGR_IND_0    , bg );
        WRITE_2_REG( ENG_2, TRANSPMASK_IND_0, 1 );
        WRITE_REG(   ENG_1, CNTRL_2_IND     , wdPriv->bit11_10 | BIT_6_5 );
        WRITE_REG(   ENG_1, RASTEROP_IND    , alu << 8 );

	while( nbox-- )
	{
	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % stipw;
	    srcy = (yoff + pbox->y1) % stiph;

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
                WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | PATTERN_8x8 | SRC_IS_MONO_COMP);
	    }
	    else
	    {
		int minw, minh, small = TRUE;
                srcx += wdPriv->tileX;
                srcy += wdPriv->tileY;

		if ((minw = w) > stipw)
		{
		    minw = stipw;
		    small = FALSE;
		}
		if ((minh = h) > stiph)
		{
		    minh = stiph;
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
                    WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | SRC_IS_MONO_COMP );

		    if( !small )
			wd33ReplicateArea( pbox->x1,pbox->y1, w, h, minw, minh, wdPriv );
		}
		else
		{
		    wd33CoverArea( srcx,srcy, stipw, stiph, pbox->x1,pbox->y1, w, h,
                                  TRUE , wdPriv );
		}
	    }
            pbox++;
        }
        wdPriv->curRegBlock = curBlock;
}
