/*
 *  @(#) wd33Bres.c 11.1 97/10/22
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
 *      wd33Bres.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Mon 11-Oct-1993		buckm@sco.com
 *		DIM_X_IND reg should be written with len-1.
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
wd33SolidZeroSeg(
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
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        register int curBlock = wdPriv->curRegBlock;
        nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
        unsigned long fg;
        unsigned long planemask;
        unsigned char alu;
        int dir   = 0;
        int major = 0;

#ifdef DEBUG_PRINT
	ErrorF("wdSolidZeroSeg( signdx=%d,signdy=%d,x1=%d,y1=%d,",
                                signdx,signdy, x1,y1);
        ErrorF("axis=%d,e=%d,e1=%d,e2=%d,len=%d\n", 
				axis, e, e1, e2, len );
#endif

/*   This is the way NFB calculates     This is what we stick into the WD
 *   the Bresenham parameters
 *
 *          adx = x2 - x1;
 *          ady = y2 - y1;
 *          signdx = sign(adx);         if( signdx < 0 ) dir  = RIGHT_LEFT;
 *          signdy = sign(ady);         if( signdy < 0 ) dir |= BOTTOM_TOP;
 *          adx = abs(adx);
 *          ady = abs(ady);
 *
 *          if (adx > ady) {
 *              axis = X_AXIS;          major = 0;
 *              min = ady;
 *              max = adx;
 *              len = adx;
 *          } else {
 *              axis = Y_AXIS;          major = MAJOR_Y;
 *              min = adx; 
 *              max = ady;
 *              len = ady;
 *          }
 *          e1 = min << 1;              outw( LINE_DRAW_K1, e1 )          
 *          e2 = e1 - (max << 1);       outw( LINE_DRAW_K2, e2 )
 *          e = e1 - max;               if ( signdx < 0 ) e--    WD90C33 odds !!
 *                                      outw( LINE_DRAW_ERR, e )
 *                                      WRITE_REG( ENG_1, DIM_X_IND, len - 1 )
 *                                      WRITE_REG( ENG_1, CNTRL_1_IND, ... | major | dir)
 */
	assert( len );
        if( signdx < 0 ) { 
            dir  = RIGHT_LEFT;   e-- ; 
        }
        if( signdy < 0 )
            dir |= BOTTOM_TOP;
        if( axis == Y_AXIS ) major = MAJOR_Y;

	alu = pGCPriv->rRop.alu;
	planemask = pGCPriv->rRop.planemask;
	fg = pGCPriv->rRop.fg;

        WAITFOR_BUFF( 8 );
        outw( LINE_DRAW_K1,  e1 );
        outw( LINE_DRAW_K2,  e2 );
        outw( LINE_DRAW_ERR,  e );
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
	WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );

        WAITFOR_BUFF( 8 );
	WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
	WRITE_REG( ENG_1, DEST_X        , x1 );
	WRITE_REG( ENG_1, DEST_Y        , y1 );
	WRITE_REG( ENG_1, DIM_X_IND     , len - 1 );
	WRITE_REG( ENG_1, CNTRL_2_IND   , wdPriv->bit11_10 | BIT_6_5 );
        WRITE_REG( ENG_1, CNTRL_1_IND   , BRESENHAM_LINE   | SRC_IS_FIXED_COL
                                         | major | dir );

        wdPriv->curRegBlock = curBlock;
}
