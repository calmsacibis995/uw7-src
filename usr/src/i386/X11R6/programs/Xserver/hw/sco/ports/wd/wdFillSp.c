/*
 *  @(#) wdFillSp.c 11.1 97/10/22
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
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *	S001	Th  29-Oct-1992	edb@sco.com
 *              GC caching by checking of serial # changes
 *      S002    Thu 05-Oct-1992 edb@sco.com
 *              implement tiled and stippled GC ops
 *	S003	Mon 09-Nov-1992	edb@sco.com
 *              minimize tile loading by checking for serialNumber
 *	S004	Thu 03-Dec-1992	edb@sco.com
 *              Fix bug in stippled fill
 *	S005	Thu  07-Jan-1993 buckm@sco.com
 *              work around mono transparency chip bug.
 *      S006    Tue 09-Feb-1993 buckm@sco.com
 *              change parameter checks into assert()'s.
 *		get rid of GC caching.
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
#include "nfb/nfbRop.h"						/* S005 */

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"

void
wdSolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n )
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        unsigned long fg;
        unsigned long planemask;
        unsigned char alu;
        int dstAddr;
        int w;

	assert( n );

        fg = pGCPriv->rRop.fg;
        planemask = pGCPriv->rRop.planemask & 0xFF;
        alu = pGCPriv->rRop.alu;

#ifdef DEBUG_PRINT
        ErrorF("wdSolidFS( ,n=%d, fg=%d,alu=%x,planemask=%x)\n",
                n, fg,alu, planemask);
#endif

        dstAddr =  ppt->y * wdPriv->fbStride + ppt->x;
        w = *pwidth;
	assert( w > 0 );

        WAITFOR_WD();

        WRITE_1_REG( CNTRL_2_IND    , QUICK_START );
        WRITE_1_REG( DIM_Y_IND      , 1 );
        WRITE_1_REG( RASTEROP_IND   , alu << 8 );
        WRITE_1_REG( FOREGR_IND     , fg );
        WRITE_1_REG( PLANEMASK_IND  , planemask );
        WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_FIXED_COL );

        WRITE_1_REG( DIM_X_IND      , w );
        WRITE_2_REG( DEST_IND       , dstAddr);
        
        while( --n )
        {
	     pwidth++;
	     ppt++;
             dstAddr =  ppt->y * wdPriv->fbStride + ppt->x;
             w = *pwidth;
	     assert( w > 0 );

             WAITFOR_WD();
             WRITE_1_REG( DIM_X_IND        , w );
             WRITE_2_REG( DEST_IND         , dstAddr);
        }
}

void
wdTiledFS(
        GCPtr pGC,
        DrawablePtr pDraw,
        register DDXPointPtr ppt,
        register unsigned int *pwidth,
        unsigned int n)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	PixmapPtr pm;
	int tilew, tileh;
	int xoff, yoff;
	int srcx, srcy;
        unsigned int x1, x2, dx , wmax;
        int load_stat;
        unsigned long planemask;
        unsigned char alu;
        int fb_stride;
        int srcAddr, dstAddr;
        int controlCmd_2;

	assert( n );

	pm = pGC->tile.pixmap;

	tilew = pm->drawable.width;
	tileh = pm->drawable.height;
        planemask = pGCPriv->rRop.planemask & 0xFF;
        alu = pGCPriv->rRop.alu;

#ifdef DEBUG_PRINT
        ErrorF("wdTiledFS( ,n=%d, ,tilew=%d, tileh=%d, \n\
                            ,alu=%x,planemask=%x)\n",
                n,tilew, tileh, alu, planemask);
#endif

        if ( (load_stat = wdLoadTile(pm->devPrivate.ptr, pm->devKind,
              tilew, tileh, wdPriv,pm->drawable.serialNumber)) == NOT_LOADED ) {
                while (n-- > 0) {
                        BoxRec box;

                        box.x1 = ppt->x;
                        box.x2 = box.x1 + *pwidth;
                        box.y1 = ppt->y;
                        box.y2 = box.y1 + 1;

                        pwidth++;
                        ppt++;

                        genTileRects(&box, 1, pm->devPrivate.ptr, pm->devKind,
                                tilew, tileh, &(pGCPriv->screenPatOrg),
                                alu,planemask,
                                pDraw);
                }
                return;
        }

	xoff = tilew - (pGCPriv->screenPatOrg.x % tilew);
	yoff = tileh - (pGCPriv->screenPatOrg.y % tileh);

        fb_stride = wdPriv->fbStride;

        controlCmd_2 = QUICK_START ;
        if( load_stat == PREF_LOADED ) 
               controlCmd_2 |= PATTERN_8x8;

        WAITFOR_WD();

        WRITE_2_REG( CNTRL_2_IND    , controlCmd_2 );
        WRITE_1_REG( DIM_Y_IND      , 1 );
        WRITE_1_REG( RASTEROP_IND   , alu << 8 );
        WRITE_1_REG( PLANEMASK_IND  , planemask );
        WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE );

        while( n-- )
        {
             srcx   = (xoff + ppt->x) % tilew;
             srcy   = (yoff + ppt->y) % tileh;
             if( load_stat == PREF_LOADED )
	     {
		srcAddr = wdPriv->tilePrefStart + srcy * WD_TILE_WIDTH + srcx;
                wmax = *pwidth;
	     }
             else
	     {
		srcAddr = wdPriv->tileStart + srcy * fb_stride + srcx;
                wmax = tilew;
	     }

             WRITE_2_REG( SOURCE_IND       , srcAddr);
             x1 = ppt->x;
             x2 = x1 + *pwidth;
	     assert( x1 < x2 );
             while( x1 < x2 )
             {
                 dx = ((x2 - x1) > wmax) ? wmax : (x2 - x1);
                 dstAddr =  ppt->y * fb_stride + x1;
                 WRITE_1_REG( DIM_X_IND        , dx );
                 WRITE_2_REG( DEST_IND         , dstAddr);
        
                 WAITFOR_WD();
                 x1 += dx;
             }
	     pwidth++;
	     ppt++;
        }
}

void
wdStippledFS(
        GCPtr pGC,
        DrawablePtr pDraw,
        register DDXPointPtr ppt,
        register unsigned int *pwidth,
        unsigned int n)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	PixmapPtr pm;
	int stipw, stiph;
	int xoff, yoff;
	int srcx, srcy;
        unsigned long planemask;
        unsigned long fg;
        unsigned char alu;
        unsigned int x1, x2, dx , wmax;
        int load_stat;
        int fb_stride;
        int srcAddr, dstAddr;
        int controlCmd_2;

	assert( n );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
        planemask = pGCPriv->rRop.planemask & 0xFF;
        alu = pGCPriv->rRop.alu;
        fg  = pGCPriv->rRop.fg;

#ifdef DEBUG_PRINT
        ErrorF("wdStippledFS( ,n=%d, ,stipw=%d, stiph=%d, \n\
                            fg=%d,alu=%x,planemask=%x)\n",
                n,stipw, stiph, fg,alu, planemask);
#endif
        if ( ( load_stat = wdLoadStipple(pm->devPrivate.ptr, pm->devKind,
               stipw, stiph, wdPriv,pm->drawable.serialNumber)) == NOT_LOADED ) {
                genStippledFS(pGC, pDraw, ppt, pwidth, n);
                return;
        }

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

        fb_stride = wdPriv->fbStride;

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
        if( load_stat == PREF_LOADED ) 
               controlCmd_2 |= PATTERN_8x8;

        WAITFOR_WD();

        WRITE_2_REG( CNTRL_2_IND    , controlCmd_2 );
        WRITE_1_REG( DIM_Y_IND      , 1 );
        WRITE_1_REG( RASTEROP_IND   , alu << 8 );
        WRITE_1_REG( FOREGR_IND     , fg );
        WRITE_1_REG( TRANSPCOL_IND  , 0x00FF );
        WRITE_1_REG( TRANSPMASK_IND , 1 );
        WRITE_1_REG( PLANEMASK_IND  , planemask );
        WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_MONO_COMP );

        while( n-- )
        {
             srcx   = (xoff + ppt->x) % stipw;
             srcy   = (yoff + ppt->y) % stiph;
             if( load_stat == PREF_LOADED )
	     {
		srcAddr = wdPriv->tilePrefStart + srcy * WD_TILE_WIDTH + srcx;
                wmax = *pwidth;
	     }
             else
	     {
		srcAddr = wdPriv->tileStart + srcy * fb_stride + srcx;
                wmax = stipw;
	     }

             WRITE_2_REG( SOURCE_IND       , srcAddr);
             x1 = ppt->x;
             x2 = x1 + *pwidth;
	     assert( x1 < x2 );
             while( x1 < x2 )
             {
                 dx = ((x2 - x1) > wmax) ? wmax : (x2 - x1);
                 dstAddr =  ppt->y * fb_stride + x1;
                 WRITE_1_REG( DIM_X_IND        , dx );
                 WRITE_2_REG( DEST_IND         , dstAddr);
        
                 WAITFOR_WD();
                 x1 += dx;
             }
	     pwidth++;
	     ppt++;
        }
}

void
wdOpStippledFS(
        GCPtr pGC,
        DrawablePtr pDraw,
        register DDXPointPtr ppt,
        register unsigned int *pwidth,
        unsigned int n)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	PixmapPtr pm;
	int stipw, stiph;
	int xoff, yoff;
	int srcx, srcy;
        unsigned long planemask;
        unsigned long fg;
        unsigned long bg;
        unsigned char alu;
        unsigned int x1, x2, dx , wmax;
        int load_stat;
        int fb_stride;
        int srcAddr, dstAddr;
        int controlCmd_2;

	assert( n );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
        planemask = pGCPriv->rRop.planemask & 0xFF;
        alu = pGCPriv->rRop.alu;
        fg  = pGCPriv->rRop.fg;
        bg  = pGCPriv->rRop.bg;

#ifdef DEBUG_PRINT
        ErrorF("wdOpStippledFS( ,n=%d, ,stipw=%d, stiph=%d, \n\
                            fg=%d,bg=%d,alu=%x,planemask=%x)\n",
                n,stipw, stiph, fg,bg,alu, planemask);
#endif
        if ( (load_stat = wdLoadStipple(pm->devPrivate.ptr, pm->devKind,
              stipw, stiph, wdPriv,pm->drawable.serialNumber)) == NOT_LOADED ) {
                genOpStippledFS(pGC, pDraw, ppt, pwidth, n);
                return;
        }

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

        fb_stride = wdPriv->fbStride;

        controlCmd_2 = QUICK_START;
        if( load_stat == PREF_LOADED ) 
               controlCmd_2 |= PATTERN_8x8;

        WAITFOR_WD();

        WRITE_2_REG( CNTRL_2_IND    , controlCmd_2 );
        WRITE_1_REG( DIM_Y_IND      , 1 );
        WRITE_1_REG( RASTEROP_IND   , alu << 8 );
        WRITE_1_REG( FOREGR_IND     , fg );
        WRITE_1_REG( BACKGR_IND     , bg );
        WRITE_1_REG( TRANSPCOL_IND  , 0x00FF );
        WRITE_1_REG( TRANSPMASK_IND , 1 );
        WRITE_1_REG( PLANEMASK_IND  , planemask );
        WRITE_1_REG( CNTRL_1_IND    , PACKED_MODE | SRC_IS_MONO_COMP );

        while( n-- )
        {
             srcx   = (xoff + ppt->x) % stipw;
             srcy   = (yoff + ppt->y) % stiph;
             if( load_stat == PREF_LOADED )
	     {
		srcAddr = wdPriv->tilePrefStart + srcy * WD_TILE_WIDTH + srcx;
                wmax = *pwidth;
	     }
             else
	     {
		srcAddr = wdPriv->tileStart + srcy * fb_stride + srcx;
                wmax = stipw;
	     }

             WRITE_2_REG( SOURCE_IND       , srcAddr);
             x1 = ppt->x;
             x2 = x1 + *pwidth;
	     assert( x1 < x2 );
             while( x1 < x2 )
             {
                 dx = ((x2 - x1) > wmax) ? wmax : (x2 - x1);
                 dstAddr =  ppt->y * fb_stride + x1;
                 WRITE_1_REG( DIM_X_IND        , dx );
                 WRITE_2_REG( DEST_IND         , dstAddr);
        
                 WAITFOR_WD();
                 x1 += dx;
             }
	     pwidth++;
	     ppt++;
        }
}
