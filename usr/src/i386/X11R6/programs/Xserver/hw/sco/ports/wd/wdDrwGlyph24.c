/*
 *  @(#) wdDrwGlyph24.c 11.1 97/10/22
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
 *   wdDrwGlyph24.c      Load glyph in cache and draw
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Tue 25-May-1993	edb@sco.com
 *              - modified from wdDrwGlyph.c
 *      S001    Thu 27-May-1993 edb@sco.com
 *		- changes from the 8 bit to the 15..24 bit version
 *              
 */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"


#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbRop.h"	
#include "nfb/nfbGlyph.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"
#include "wdBitswap.h"



/*
 * wdLoadGlyph15_24() - Convert the monochrome image into an 8 bit deep
 *      pixmap -  0 bits to 0x00 and 1 bits to 0xFF
 *      and store in offscreen memory as linear sequence of pixels.
 *                   
 *	w, h  - the width and height of the glyph
 *	image - the bits to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window to draw on.
 */
void
wdLoadGlyph15_24(w, h, image, stride, dstAddr, dstPlane, pDraw)
	int w, h;
	unsigned char *image;
	unsigned int stride;
	int dstAddr;
	int dstPlane;
	DrawablePtr pDraw;
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        unsigned char * tab = wdPriv->expandTab;
        int pixb = wdPriv->pixBytes;
	unsigned char *image_pnt;
        unsigned char *image_row;
        int exp_nibbles;

#ifdef DEBUG_PRINT
	ErrorF("wdLoadGlyph15_24(w=%d, h=%d, image=0x%x, stride=%d, ",
		w, h, image, stride);
	ErrorF("dstAddr=%d, dstPlane=%x)\n",
		dstAddr, dstPlane);
#endif

	exp_nibbles = (w * pixb +3 ) >> 2;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	WRITE_2_REG( SOURCE_IND    , 0 );
	WRITE_2_REG( DEST_IND      , dstAddr );
	WRITE_1_REG( DIM_X_IND     , w * pixb );
	WRITE_1_REG( DIM_Y_IND     , h );
	WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
	WRITE_1_REG( FOREGR_IND    , 0xFF );
	WRITE_1_REG( BACKGR_IND    , 0x00 );
	WRITE_1_REG( PLANEMASK_IND , 1 << dstPlane );

	WRITE_1_REG( CNTRL_1_IND   , START | PACKED_MODE | DST_LINEAR |
				     SRC_IS_MONO | SRC_IS_IO_PORT );

        image_row = image;
	while (h-- > 0)
        {
               int c, n  = exp_nibbles;
               unsigned int bits;
               image_pnt = image_row;
               do
               {
                        bits = (unsigned int)(*image_pnt++)* pixb;
                        c = BITSWAP( *(tab + bits ));
                        outw(BITBLT_IO_PORT_L, c >> 4); if( --n <= 0 ) break;
                        outw(BITBLT_IO_PORT_L, c);      if( --n <= 0 ) break;
                        c = BITSWAP( *(tab + bits +1 ));
                        outw(BITBLT_IO_PORT_L, c >> 4); if( --n <= 0 ) break;
                        outw(BITBLT_IO_PORT_L, c);      if( --n <= 0 ) break;
                        if( pixb < 3) continue;
                        c = BITSWAP( *(tab + bits +2 ));
                        outw(BITBLT_IO_PORT_L, c >> 4); if( --n <= 0 ) break;
                        outw(BITBLT_IO_PORT_L, c);          --n;
                } while ( n );
		image_row    += stride;
        }
}

/*
 * wdDrawGlyph15_24() - Assume glyph is already cached and starts at cacheAddr
 *                   as a linear colored pixmap. 
 *                   Copy glyph from cache to desired position pbox
 *                   Convert foreground color , use alu and planemask 
 *                   
 *	pbox       - Destination rectangle to draw into.
 *	cacheAddr
 *	cachePlane - Location of glyph in cache
 *	fg         - desired foreground color
 *	alu        - the raster op to use when drawing.
 *	planemask  - the window planes to be affected.
 *	pDraw      - the window to draw on.
 */

void
wdDrawGlyph15_24(
	BoxPtr pbox,
	int cacheAddr,
	int cachePlane,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int pixb = wdPriv->pixBytes;
	int w, h;
	int  dstAddrBlt;
        unsigned char * fgp;
        unsigned char * pmp;

#ifdef DEBUG_PRINT
	ErrorF("wdDrawGlyph15_24(box=(%d,%d)-(%d,%d), cacheAddr=%d \n",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2, cacheAddr );
	ErrorF(" fg=0x%x, alu=%d, planemask=0x%x)\n",
		fg, alu, planemask);
#endif

	w =  pbox->x2 - pbox->x1;
	h =  pbox->y2 - pbox->y1;
	dstAddrBlt =  pbox->y1 * wdPriv->fbStride + pbox->x1 * pixb;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_MONO_COMP | SRC_LINEAR );
	WRITE_1_REG( CNTRL_2_IND   , MONO_TRANSP_ENAB | QUICK_START );
	WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	WRITE_1_REG( TRANSPCOL_IND , 0xFF );
	WRITE_1_REG( TRANSPMASK_IND, 1 << cachePlane );

        WRITE_2_REG( SOURCE_IND       , cacheAddr);
        WRITE_1_REG( DIM_X_IND        , w * pixb );
	WRITE_1_REG( DIM_Y_IND        , h );
        fgp = (unsigned char *)&fg;
        pmp = (unsigned char *)&planemask;

        while( pixb-- )
        {
	    WAITFOR_WD();
	    WRITE_1_REG( FOREGR_IND       , *fgp );       fgp++;
	    WRITE_1_REG( PLANEMASK_IND    , *pmp );       pmp++;
	    WRITE_2_REG( DEST_IND         , dstAddrBlt);  dstAddrBlt++;

        }
}
