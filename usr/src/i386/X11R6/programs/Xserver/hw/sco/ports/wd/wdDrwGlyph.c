/*   @(#) wdDrwGlyph.c 11.1 97/10/22
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
 *   wdDrwGlyph.c      Load glyph in cache and draw
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Tue 25-May-1993	edb@sco.com
 *              - Moved wdLoadGlyph and wdDrawGlyph from wdGlyph.c
 *      S001    Thu 27-May-1993 edb@sco.com
 *		- attribute caching removed ( makes it easier for the 16 bit )
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
 * wdLoadGlyph8() - Convert the monochrome image into an 8 bit deep
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
wdLoadGlyph8(w, h, image, stride, dstAddr, dstPlane, pDraw)
	int w, h;
	unsigned char *image;
	unsigned int stride;
	int dstAddr;
	int dstPlane;
	DrawablePtr pDraw;
{
	int pad, nibbles;

#ifdef DEBUG_PRINT
	ErrorF("wdLoadGlyph8(w=%d, h=%d, image=0x%x, stride=%d, ",
		w, h, image, stride);
	ErrorF("dstAddr=%d, dstPlane=%d)\n",
		dstAddr, dstPlane);
#endif

	pad = stride - ((w + 7) >> 3);
	nibbles = (w + 3) >> 2;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	WRITE_2_REG( SOURCE_IND    , 0 );
	WRITE_2_REG( DEST_IND      , dstAddr );
	WRITE_1_REG( DIM_X_IND     , w );
	WRITE_1_REG( DIM_Y_IND     , h );
	WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
	WRITE_1_REG( FOREGR_IND    , 0xFF );
	WRITE_1_REG( BACKGR_IND    , 0x00 );
	WRITE_1_REG( PLANEMASK_IND , 1 << dstPlane );

	WRITE_1_REG( CNTRL_1_IND   , START | PACKED_MODE | DST_LINEAR |
				     SRC_IS_MONO | SRC_IS_IO_PORT );

	do
	{
		int c, n = nibbles;
		while ((n -= 2) >= 0)
		{
			c = BITSWAP(*image++);
			outw(BITBLT_IO_PORT_L, c >> 4);
			outw(BITBLT_IO_PORT_L, c);
		}
		if (n & 1)
		{
			c = BITSWAP(*image++);
			outw(BITBLT_IO_PORT_L, c >> 4);
		}
		image += pad;
	} while (--h > 0);
}

/*
 * wdDrawGlyph8() - Assume glyph is already cached and starts at cacheAddr
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
wdDrawGlyph8(
	BoxPtr pbox,
	int cacheAddr,
	int cachePlane,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
	int w, h;
	int  dstAddrBlt;

#ifdef DEBUG_PRINT
	ErrorF("wdDrawGlyph(box=(%d,%d)-(%d,%d), cacheAddr=%d \n",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2, cacheAddr );
	ErrorF(" fg=0x%x, alu=%d, planemask=0x%x)\n",
		fg, alu, planemask);
#endif

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	dstAddrBlt =  pbox->y1 * wdPriv->fbStride + pbox->x1;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND      , MONO_TRANSP_ENAB );
	WRITE_1_REG( RASTEROP_IND     , alu << 8 );
	WRITE_1_REG( FOREGR_IND       , fg );
	WRITE_1_REG( TRANSPCOL_IND    , 0xFF );
	WRITE_1_REG( TRANSPMASK_IND   , 1 << cachePlane );
	WRITE_1_REG( PLANEMASK_IND    , planemask & 0xFF );
	WRITE_2_REG( SOURCE_IND       , cacheAddr);
	WRITE_2_REG( DEST_IND         , dstAddrBlt);
	WRITE_1_REG( DIM_X_IND        , w );
	WRITE_1_REG( DIM_Y_IND        , h );

	WRITE_1_REG( CNTRL_1_IND ,
		     START | PACKED_MODE | SRC_IS_MONO_COMP | SRC_LINEAR );
}
