/*
 *  @(#) wd33FillSp.c 11.1 97/10/22
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
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Mon 12-Aug-1993		edb@sco.com
 *              Remove code used to workaround a chip bug in wd90c31
 *              ( problem for alus not needing the source )
 *	S002	Mon 12-Aug-1993		edb@sco.com
 *              DIM_Y_IND was off by one
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
#include "nfb/nfbRop.h"	

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wd33Procs.h"


void
wd33SolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n )
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        register int curBlock = wdPriv->curRegBlock;
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        unsigned long fg;
        unsigned long planemask;
        unsigned char alu;
        int dstAddr;
        int w;

	assert( n );

        fg = pGCPriv->rRop.fg;
        planemask = pGCPriv->rRop.planemask;
        alu = pGCPriv->rRop.alu;

#ifdef DEBUG_PRINT
        ErrorF("wd33SolidFS( ,n=%d, fg=%d,alu=%x,planemask=%x)\n",
                n, fg,alu, planemask);
#endif

        WAITFOR_BUFF( 8 );

        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
	WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_REG( ENG_1, DIM_Y_IND     , 1-1 );
	WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
	WRITE_REG( ENG_1, CNTRL_2_IND   , wdPriv->bit11_10 | BIT_6_5 );
        while( n-- )
        {
             w = *pwidth;
	     assert( w > 0 );

             WAITFOR_BUFF( 4 )
	     WRITE_REG( ENG_1, DEST_X        , ppt->x );
	     WRITE_REG( ENG_1, DEST_Y        , ppt->y );
	     WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );
             WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT  | SRC_IS_FIXED_COL );

	     pwidth++;
	     ppt++;
        }
        wdPriv->curRegBlock = curBlock;
}


void
wd33TiledFS(
        GCPtr pGC,
        DrawablePtr pDraw,
        register DDXPointPtr ppt,
        register unsigned int *pwidth,
        unsigned int n)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	PixmapPtr pm;
        register char curBlock;
	int tilew, tileh;
	int xoff, yoff;
	int srcx, srcy;
        unsigned int x1, x2, dx , wmax;
        int load_stat;
        unsigned long planemask;
        unsigned char alu;
        int pattern;
        int sourceX, sourceY;

	assert( n );

	pm = pGC->tile.pixmap;

	tilew = pm->drawable.width;
	tileh = pm->drawable.height;
        planemask = pGCPriv->rRop.planemask;
        alu = pGCPriv->rRop.alu;

#ifdef DEBUG_PRINT
        ErrorF("wd33TiledFS( ,n=%d, ,tilew=%d, tileh=%d, \n\
                            ,alu=%x,planemask=%x)\n",
                n,tilew, tileh, alu, planemask);
#endif

        if ( (load_stat = wd33LoadTile(pm->devPrivate.ptr, pm->devKind,
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
#ifdef DEBUG_PRINT
  ErrorF("             Tile %s\n", load_stat == PREF_LOADED ? "PREF_LOADED" : "LOADED");
#endif

	xoff = tilew - (pGCPriv->screenPatOrg.x % tilew);
	yoff = tileh - (pGCPriv->screenPatOrg.y % tileh);

        curBlock= wdPriv->curRegBlock;

        WAITFOR_BUFF( 8 );
        WRITE_REG(   ENG_1, RASTEROP_IND    , alu << 8 );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
        WRITE_REG(   ENG_1, CNTRL_2_IND     , wdPriv->bit11_10 | BIT_6_5 );
        WRITE_REG(   ENG_1, DIM_Y_IND       , 1-1 );

        while( n-- )
        {
             srcx   = (xoff + ppt->x) % tilew;
             srcy   = (yoff + ppt->y) % tileh;
             if( load_stat == PREF_LOADED )
	     {
                sourceX = wdPriv->tilePrefX + srcy * WD_TILE_WIDTH + srcx ;
                sourceY = wdPriv->tilePrefY ;
                wmax = *pwidth;
                pattern = PATTERN_8x8;
	     }
             else
	     {
                sourceX = wdPriv->tileX + srcx;
                sourceY = wdPriv->tileY + srcy;
                pattern = 0;
                wmax = tilew;
	     }

             WAITFOR_BUFF( 2 );
             WRITE_REG( ENG_1, SOURCE_X   , sourceX );
             WRITE_REG( ENG_1, SOURCE_Y   , sourceY );
             x1 = ppt->x;
             x2 = x1 + *pwidth;
	     assert( x1 < x2 );
             while( x1 < x2 )
             {
                 dx = ((x2 - x1) > wmax) ? wmax : (x2 - x1);
                 WAITFOR_BUFF( 4 );
                 WRITE_REG( ENG_1, DIM_X_IND  , dx - 1 );
                 WRITE_REG( ENG_1, DEST_X     , x1     );
                 WRITE_REG( ENG_1, DEST_Y     , ppt->y );
                 WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | pattern );
                 x1 += dx;
             }
	     pwidth++;
	     ppt++;
        }
        wdPriv->curRegBlock = curBlock;
}

void
wd33StippledFS(
        GCPtr pGC,
        DrawablePtr pDraw,
        register DDXPointPtr ppt,
        register unsigned int *pwidth,
        unsigned int n)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        register char curBlock;
	PixmapPtr pm;
	int stipw, stiph;
	int xoff, yoff;
	int srcx, srcy;
        unsigned long planemask;
        unsigned long fg;
        unsigned char alu;
        unsigned int x1, x2, dx , wmax;
        int load_stat;
        int pattern;
        int sourceX, sourceY;

	assert( n );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
        planemask = pGCPriv->rRop.planemask;
        alu = pGCPriv->rRop.alu;
        fg  = pGCPriv->rRop.fg;

#ifdef DEBUG_PRINT
        ErrorF("wd33StippledFS( ,n=%d, ,stipw=%d, stiph=%d, \n\
                            fg=%d,alu=%x,planemask=%x)\n",
                n,stipw, stiph, fg,alu, planemask);
#endif
        if ( ( load_stat = wd33LoadStipple(pm->devPrivate.ptr, pm->devKind,
               stipw, stiph, wdPriv,pm->drawable.serialNumber)) == NOT_LOADED ) {
                genStippledFS(pGC, pDraw, ppt, pwidth, n);
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
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
        WRITE_2_REG( ENG_2, TRANSPCOL_IND_0 , wdPriv->planeMaskMask );
        WAITFOR_BUFF( 6 );
        WRITE_2_REG( ENG_2, TRANSPMASK_IND_0, 1 );
        WRITE_REG(   ENG_1, CNTRL_2_IND     , wdPriv->bit11_10 | BIT_6_5 | MONO_TRANSP_ENAB);
        WRITE_REG(   ENG_1, DIM_Y_IND       , 1-1 );

        while( n-- )
        {
             srcx   = (xoff + ppt->x) % stipw;
             srcy   = (yoff + ppt->y) % stiph;
             if( load_stat == PREF_LOADED )
             {
                sourceX = wdPriv->tilePrefX + srcy * WD_TILE_WIDTH + srcx ;
                sourceY = wdPriv->tilePrefY ;
                wmax = *pwidth;
                pattern = PATTERN_8x8;
             }
             else
             {
                sourceX = wdPriv->tileX + srcx;
                sourceY = wdPriv->tileY + srcy;
                pattern = 0;
                wmax = stipw;
             }

             WAITFOR_BUFF( 2 );
             WRITE_REG( ENG_1, SOURCE_X   , sourceX );
             WRITE_REG( ENG_1, SOURCE_Y   , sourceY );
             x1 = ppt->x;
             x2 = x1 + *pwidth;
	     assert( x1 < x2 );
             while( x1 < x2 )
             {
                 dx = ((x2 - x1) > wmax) ? wmax : (x2 - x1);
                 WAITFOR_BUFF( 4 );
                 WRITE_REG( ENG_1, DIM_X_IND  , dx - 1 );
                 WRITE_REG( ENG_1, DEST_X     , x1     );
                 WRITE_REG( ENG_1, DEST_Y     , ppt->y );
                 WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | SRC_IS_MONO_COMP | pattern );
                 x1 += dx;
             }
	     pwidth++;
	     ppt++;
        }
        wdPriv->curRegBlock = curBlock;
}

void
wd33OpStippledFS(
        GCPtr pGC,
        DrawablePtr pDraw,
        register DDXPointPtr ppt,
        register unsigned int *pwidth,
        unsigned int n)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        register char curBlock;
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
        int pattern;
        int sourceX, sourceY;

	assert( n );

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
        planemask = pGCPriv->rRop.planemask ;
        alu = pGCPriv->rRop.alu;
        fg  = pGCPriv->rRop.fg;
        bg  = pGCPriv->rRop.bg;

#ifdef DEBUG_PRINT
        ErrorF("wd33OpStippledFS( ,n=%d, ,stipw=%d, stiph=%d, \n\
                            fg=%d,bg=%d,alu=%x,planemask=%x)\n",
                n,stipw, stiph, fg,bg,alu, planemask);
#endif
        if ( (load_stat = wd33LoadStipple(pm->devPrivate.ptr, pm->devKind,
              stipw, stiph, wdPriv,pm->drawable.serialNumber)) == NOT_LOADED ) {
                genOpStippledFS(pGC, pDraw, ppt, pwidth, n);
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
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WRITE_2_REG( ENG_2, BACKGR_IND_0    , bg );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, TRANSPCOL_IND_0 , wdPriv->planeMaskMask );
        WRITE_2_REG( ENG_2, TRANSPMASK_IND_0, 1 );
        WRITE_REG(   ENG_1, CNTRL_2_IND     , wdPriv->bit11_10 | BIT_6_5 );
        WRITE_REG(   ENG_1, DIM_Y_IND       , 1-1 );

        while( n-- )
        {
             srcx   = (xoff + ppt->x) % stipw;
             srcy   = (yoff + ppt->y) % stiph;
             if( load_stat == PREF_LOADED )
             {
                sourceX = wdPriv->tilePrefX + srcy * WD_TILE_WIDTH + srcx;
                sourceY = wdPriv->tilePrefY ;
                wmax = *pwidth;
                pattern = PATTERN_8x8;
             }
             else
             {
                sourceX = wdPriv->tileX + srcx;
                sourceY = wdPriv->tileY + srcy;
                pattern = 0;
                wmax = stipw;
             }

             WAITFOR_BUFF( 2 );
             WRITE_REG( ENG_1, SOURCE_X   , sourceX );
             WRITE_REG( ENG_1, SOURCE_Y   , sourceY );
             x1 = ppt->x;
             x2 = x1 + *pwidth;
	     assert( x1 < x2 );
             while( x1 < x2 )
             {
                 dx = ((x2 - x1) > wmax) ? wmax : (x2 - x1);
                 WAITFOR_BUFF( 4 );
                 WRITE_REG( ENG_1, DIM_X_IND  , dx - 1 );
                 WRITE_REG( ENG_1, DEST_X     , x1     );
                 WRITE_REG( ENG_1, DEST_Y     , ppt->y );
                 WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | SRC_IS_MONO_COMP | pattern );
                 x1 += dx;
             }
	     pwidth++;
	     ppt++;
        }
        wdPriv->curRegBlock = curBlock;
}
