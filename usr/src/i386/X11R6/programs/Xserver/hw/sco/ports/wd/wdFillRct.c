/*
 *  @(#) wdFillRct.c 11.1 97/10/22
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
 * wdFillRct.c
 *               wd rectangular drawing ops.
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 02-Nov-1992	edb@sco.com
 *
 *	S001	Mon 09-Nov-1992	edb@sco.com
 *              minimize tile loading by checking for serialNumber
 *	S002	Thu 03-Dec-1992	edb@sco.com
 *              Fix bug in stippled fill
 *	S003	Thu 03-Dec-1992	edb@sco.com
 *              Fix bug in wdLoadStipple
 *	S004	Sun 20-Dec-1992	buckm@sco.com
 *		use drawable serialNumber for tile/stipple checking;
 *		keep track of the current serialNumber.
 *	S005	Thu 07-Jan-1993 buckm@sco.com
 *              work around mono transparency chip bug.
 *      S006    Tue 09-Feb-1993 buckm@sco.com
 *              change parameter checks into assert()'s.
 *		get rid of GC caching.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"						/* S005 */

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"

void
wdSolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox )
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	unsigned long fg;
	unsigned long planemask;
	unsigned char alu;
	int dstAddr;
	int w, h;

#ifdef DEBUG_PRINT
	ErrorF("wdSolidFillRects(pbox=%x, nbox=%d, ", pbox,nbox);
	ErrorF("fg=%d, alu=%d, planemask=0x%x)\n",
	    pGCPriv->rRop.fg, pGCPriv->rRop.planemask, pGCPriv->rRop.alu);
#endif

	assert( nbox );

	fg = pGCPriv->rRop.fg;
	planemask = pGCPriv->rRop.planemask & 0xFF;
	alu = pGCPriv->rRop.alu;

	dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND    , QUICK_START );
	WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	WRITE_1_REG( FOREGR_IND     , fg );
	WRITE_1_REG( PLANEMASK_IND  , planemask );
	WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_FIXED_COL );

	WRITE_1_REG( DIM_X_IND      , w );
	WRITE_1_REG( DIM_Y_IND      , h );
	WRITE_2_REG( DEST_IND       , dstAddr);
	
	while( --nbox )
	{
	    pbox++;
	    dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1;
	    w = pbox->x2 - pbox->x1;
	    h = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );

	    WAITFOR_WD();
	    WRITE_1_REG( DIM_X_IND        , w );
	    WRITE_1_REG( DIM_Y_IND        , h );
	    WRITE_2_REG( DEST_IND         , dstAddr);
	}
}

void
wdTiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	PixmapPtr pm;
	int load_stat;
	int tilew, tileh;
	int xoff, yoff;
	int w, h;
	int srcx, srcy;
	unsigned long planemask;
	unsigned char alu;
	int srcAddr, dstAddr;
	int controlCmd_2;

	assert( nbox );

	pm = pGC->tile.pixmap;

	tilew = pm->drawable.width;
	tileh = pm->drawable.height;
	planemask = pGCPriv->rRop.planemask & 0xFF;
	alu = pGCPriv->rRop.alu;

#ifdef DEBUG_PRINT
	ErrorF("wdTiledFillRects( ,nbox=%d, ,tilew=%d, tileh=%d, \n\
		           ,alu=%x,planemask=%x)\n",
	       nbox,tilew, tileh, alu, planemask);
#endif

	if( (load_stat = wdLoadTile(pm->devPrivate.ptr, pm->devKind,
				tilew, tileh, wdPriv,
				pm->drawable.serialNumber)) == NOT_LOADED) {
		genTileRects(pbox, nbox, pm->devPrivate.ptr, pm->devKind,
			tilew, tileh, &(pGCPriv->screenPatOrg),
			alu,planemask, pDraw);
		return;
	}

	xoff = tilew - (pGCPriv->screenPatOrg.x % tilew);
	yoff = tileh - (pGCPriv->screenPatOrg.y % tileh);

	controlCmd_2 = QUICK_START ;
	if( load_stat == PREF_LOADED ) 
	    controlCmd_2 |= PATTERN_8x8;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND    , controlCmd_2 );
	WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	WRITE_1_REG( PLANEMASK_IND  , planemask );
	WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE );

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

void
wdStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
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
	int srcAddr, dstAddr;
	int controlCmd_2;

	assert( nbox );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
	alu = pGCPriv->rRop.alu;
	fg =  pGCPriv->rRop.fg;
	planemask =  pGCPriv->rRop.planemask & 0xFF;

#ifdef DEBUG_PRINT
	ErrorF("wdStippledFillRects( ,nbox=%d, ,stipw=%d, stiph=%d, \n\
		        fg=%d,alu=%x,planemask=%x)\n",
	       nbox,stipw, stiph,fg, alu, planemask);
#endif

	if( (load_stat = wdLoadStipple(pm->devPrivate.ptr, pm->devKind,
				stipw, stiph, wdPriv,
				pm->drawable.serialNumber)) == NOT_LOADED) {
	       genStippledFillRects(pGC, pDraw, pbox, nbox);
	       return;
	}

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

	/* S005
	 * Work around apparent chip bug.
	 * During mono transparency operations,
	 * if the alu does not involve the source,
	 * the blt'er seems to lose track of the mono pattern.
	 * We get around this bug by adjusting the alu and fg
	 * to an equivalent operation that does involve the source.
	 */
	if( ! rop_needs_src[alu] )
	{
	    switch( alu )
	    {
	    case GXclear:	fg = 0;		alu = GXcopy;	break;
	    case GXset:		fg = 0xFF;	alu = GXcopy;	break;
	    case GXinvert:	fg = 0xFF;	alu = GXxor;	break;
	    case GXnoop:	return;
	    }
	}

	controlCmd_2 = QUICK_START | MONO_TRANSP_ENAB ;
	if( load_stat == PREF_LOADED)
	    controlCmd_2 |=  PATTERN_8x8;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND    , controlCmd_2 );
	WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	WRITE_1_REG( FOREGR_IND     , fg );
	WRITE_1_REG( TRANSPCOL_IND  , 0x00FF );
	WRITE_1_REG( TRANSPMASK_IND , 1 );
	WRITE_1_REG( PLANEMASK_IND  , planemask );
	WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_MONO_COMP );

	while( nbox-- )
	{
	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % stipw;
	    srcy = (yoff + pbox->y1) % stiph;
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
		    WAITFOR_WD();
		    WRITE_1_REG( DIM_X_IND   , minw );
		    WRITE_1_REG( DIM_Y_IND   , minh );
		    WRITE_2_REG( SOURCE_IND  , srcAddr );
		    WRITE_2_REG( DEST_IND    , dstAddr );
		}
		else
		{
		    wdCoverArea( srcAddr, stipw, stiph, dstAddr, w, h, wdPriv );
		}
	    }
	}
}

void
wdOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
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
	int srcAddr, dstAddr;
	int controlCmd_2;

	assert( nbox );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
	alu = pGCPriv->rRop.alu;
	fg =  pGCPriv->rRop.fg;
	bg =  pGCPriv->rRop.bg;
	planemask =  pGCPriv->rRop.planemask & 0xFF;

#ifdef DEBUG_PRINT
	ErrorF("wdOpStippledFillRects( ,nbox=%d, ,stipw=%d, stiph=%d, \n\
		        fg=%d,bg=%d,alu=%x,planemask=%x)\n",
	       nbox,stipw, stiph,fg,bg, alu, planemask);
#endif

	if( (load_stat = wdLoadStipple(pm->devPrivate.ptr, pm->devKind, 
			    stipw, stiph, wdPriv,
			    pm->drawable.serialNumber)) == NOT_LOADED ) {
	       genOpStippledFillRects(pGC, pDraw, pbox, nbox);
	       return;
	}

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

	controlCmd_2 = QUICK_START ;
	if( load_stat == PREF_LOADED )
	    controlCmd_2 |= PATTERN_8x8;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND    , controlCmd_2 );
	WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	WRITE_1_REG( FOREGR_IND     , fg );
	WRITE_1_REG( BACKGR_IND     , bg );
	WRITE_1_REG( TRANSPCOL_IND  , 0x00FF );
	WRITE_1_REG( TRANSPMASK_IND , 1 );
	WRITE_1_REG( PLANEMASK_IND  , planemask );
	WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_MONO_COMP );

	while( nbox-- )
	{
	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % stipw;
	    srcy = (yoff + pbox->y1) % stiph;
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
		    WAITFOR_WD();
		    WRITE_1_REG( CNTRL_1_IND , PACKED_MODE | SRC_IS_MONO_COMP );
		    WRITE_1_REG( DIM_X_IND   , minw );
		    WRITE_1_REG( DIM_Y_IND   , minh );
		    WRITE_2_REG( SOURCE_IND  , srcAddr );
		    WRITE_2_REG( DEST_IND    , dstAddr );

		    if( !small )
			wdReplicateArea( dstAddr, w, h, minw, minh, wdPriv );
		}
		else
		{
		    wdCoverArea( srcAddr, stipw, stiph, dstAddr, w, h, wdPriv );
		}
	    }
	}
}
