/*
 * @(#) wdFillRct24.c 11.1 97/10/22
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
 * wdFillRct15_24.c
 *               wd rectangular drawing ops.
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 19-Apr-1993	edb@sco.com
 *              Created from wdFillRct.c
 *      S001    Fri 07-May-1993 edb@sco.com
 *              Bugfix
 *      S002    Wdn 19-May-1993 edb@sco.com
 *              Fix for alu GXset and GX invert
 *              
 *
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"



#ifdef NOT_YET

void
wdSolidFillRects15_24(
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
	ErrorF("wdSolidFillRects15_24(pbox=%x, nbox=%d, ", pbox,nbox);
	ErrorF("fg=%d, alu=%d, planemask=0x%x)\n",
	    pGCPriv->rRop.fg, pGCPriv->rRop.planemask, pGCPriv->rRop.alu);
#endif

	assert( nbox );

	fg = pGCPriv->rRop.fg;
	planemask = pGCPriv->rRop.planemask & 0xFF;
	alu = pGCPriv->rRop.alu;

        FIX_CHIP_BUG( alu, fg );

	dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND    , QUICK_START );
	WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	WRITE_1_REG( FOREGR_IND     , fg );
	WRITE_1_REG( PLANEMASK_IND  , planemask );
	WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE );

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
wdTiledFillRects15_24(
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
	ErrorF("wdTiledFillRects15_24( ,nbox=%d, ,tilew=%d, tileh=%d, \n\
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

#endif

void
wdStippledFillRects15_24(
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
        unsigned char * fgp;
        unsigned char * pmp;
        int pixb = wdPriv->pixBytes;
        int p;
	int srcAddr, dstAddr;
	int controlCmd_2;

	assert( nbox );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
	alu = pGCPriv->rRop.alu;
	fg =  pGCPriv->rRop.fg;
	planemask =  pGCPriv->rRop.planemask;

#ifdef DEBUG_PRINT
	ErrorF("wdStippledFillRects15_24( ,nbox=%d, ,stipw=%d, stiph=%d, \n\
		        fg=%d,alu=%x,planemask=%x)\n",
	       nbox,stipw, stiph,fg, alu, planemask);
#endif

	if( (load_stat = wdLoadStipple15_24(pm->devPrivate.ptr, pm->devKind,
				stipw, stiph, wdPriv,
				pm->drawable.serialNumber)) == NOT_LOADED) {
	       genStippledFillRects(pGC, pDraw, pbox, nbox);
	       return;
	}

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

	/* 
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
	    {                               /* S002 */
	    case GXclear:	fg = 0;				alu = GXcopy;	break;
	    case GXset:		fg = (1 << pDraw->depth) -1 ;	alu = GXcopy;	break;
	    case GXinvert:	fg = (1 << pDraw->depth) -1 ;	alu = GXxor;	break;
	    case GXnoop:	return;
	    }
	}

	controlCmd_2 = QUICK_START | MONO_TRANSP_ENAB ;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND    , controlCmd_2 );
	WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	WRITE_1_REG( TRANSPCOL_IND  , 0x00FF );
	WRITE_1_REG( TRANSPMASK_IND , 1 );
	WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_MONO_COMP );

	while( nbox-- )
	{
	    int minw, minh, small = TRUE;

	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % stipw;
	    srcy = (yoff + pbox->y1) % stiph;


	    if ((minw = w ) > stipw)
	    {
	        minw = stipw;
	        small = FALSE;
	    }
	    if ((minh = h) > stiph)
	    {
	        minh = stiph;
	        small = FALSE;
	    }

	    srcAddr = wdPriv->tileStart + srcy * wdPriv->fbStride + srcx * pixb;
	    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * pixb;
       	    pbox++;

            fgp = (unsigned char *)&fg;
            pmp = (unsigned char *)&planemask;
            p = pixb;

            while( p-- )
            {
                WAITFOR_WD();                              /* S001 */
	        WRITE_1_REG( FOREGR_IND     , *fgp );
	        WRITE_1_REG( PLANEMASK_IND  , *pmp );
	        if( small )
	        {
	            WRITE_1_REG( DIM_X_IND   , minw * pixb );
	            WRITE_1_REG( DIM_Y_IND   , minh );
	            WRITE_2_REG( SOURCE_IND  , srcAddr );
	            WRITE_2_REG( DEST_IND    , dstAddr );
	        }
	        else
	        {
	            wdCoverArea( srcAddr, stipw*pixb, stiph, dstAddr, w*pixb, h, wdPriv );
	        }
                dstAddr++;
                fgp++; 
                pmp++;
            }
	}
}


void
wdOpStippledFillRects15_24(
	GCPtr pGC,
	DrawablePtr pDraw,
	register BoxPtr pbox,
	unsigned int nbox)
{
	register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        int pixb = wdPriv->pixBytes;

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
	planemask =  pGCPriv->rRop.planemask ;

#ifdef DEBUG_PRINT
	ErrorF("wdOpStippledFillRects15_24( ,nbox=%d, ,stipw=%d, stiph=%d, \n\
		        fg=%d,bg=%d,alu=%x,planemask=%x)\n",
	       nbox,stipw, stiph,fg,bg, alu, planemask);
#endif

	if( (load_stat = wdLoadStipple15_24Exp(pm->devPrivate.ptr, pm->devKind, 
			    stipw, stiph, fg, bg, wdPriv,
			    pm->drawable.serialNumber)) == NOT_LOADED  ||
            ((planemask ^ (planemask >> 8)) & wdPriv->planeMaskMask) != 0 ) 
        {
	       genOpStippledFillRects(pGC, pDraw, pbox, nbox);
	       return;
	}

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

	controlCmd_2 = QUICK_START ;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND    , controlCmd_2 );
	WRITE_1_REG( RASTEROP_IND   , alu << 8 );
	WRITE_1_REG( PLANEMASK_IND  , planemask & 0xFF );
	WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE );

	while( nbox-- )
	{

	    w  = pbox->x2 - pbox->x1;
	    h  = pbox->y2 - pbox->y1;
	    assert( w > 0 && h > 0 );
	    srcx = (xoff + pbox->x1) % stipw;
	    srcy = (yoff + pbox->y1) % stiph;
	    dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * pixb;
       	    pbox++;

	    srcAddr = wdPriv->tileStart + srcy * wdPriv->fbStride + srcx * pixb;

	    wdCoverArea( srcAddr, stipw * pixb, stiph, dstAddr, w * pixb, h, wdPriv );
	}
}
 
