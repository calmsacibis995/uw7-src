/*
 *  @(#) wd33DrwGlyph.c 11.1 97/10/22
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
 *   wd33DrwGlyph.c      Load glyph in cache and draw
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Tue 17-Aug-1993	edb@sco.com
 *              Add support for DrawFontText8
 *      S003    Thu 18-Aug-1993 edb@sco.com
 *		Input buffer timing was not right
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

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wd33Procs.h"
#include "wdBitswap.h"


/*
 * wd33LoadGlyph() - Draw the monochrome image into one plane of cache
 *                   
 *	w, h  - the width and height of the glyph
 *	image - the bits to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 */
void
wd33LoadGlyph( wdPriv, w, h, image, stride, dstX, dstY, dstPlane)
	wdScrnPrivPtr wdPriv;
	int w, h;
	unsigned char *image;
	unsigned int stride;
	int dstX, dstY;
	int dstPlane;
{
        register int curBlock = wdPriv->curRegBlock;
	int pad, nibbles;

#ifdef DEBUG_PRINT
	ErrorF("wd33LoadGlyph(w=%d, h=%d, image=0x%x, stride=%d, ",
		w, h, image, stride);
	ErrorF("dstX=%d, dstY=%d, dstPlane=%d)\n",
		dstX, dstY, dstPlane);
#endif

	pad = stride - ((w + 7) >> 3);
	nibbles = (w + 3) >> 2;

        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , wdPriv->planeMaskMask );
        WRITE_2_REG( ENG_2, BACKGR_IND_0    ,  0 );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , 1 << dstPlane );

        WAITFOR_BUFF( 8 );
        WRITE_REG( ENG_1, SOURCE_X      , 0 );
        WRITE_REG( ENG_1, SOURCE_Y      , 0 );
        WRITE_REG( ENG_1, DEST_X        , dstX );
        WRITE_REG( ENG_1, DEST_Y        , dstY );
        WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );
        WRITE_REG( ENG_1, DIM_Y_IND     , h - 1 );
        WRITE_REG( ENG_1, RASTEROP_IND  , GXcopy << 8 );

        WAITFOR_BUFF( 2 );
        WRITE_REG( ENG_1, CNTRL_2_IND , wdPriv->bit11_10 |BIT_6_5|COLOR_EXPAND_4);
        WRITE_REG( ENG_1, CNTRL_1_IND , BITBLIT | SRC_IS_IO_PORT | SRC_IS_MONO );

        /* The WDC31 wants 4 bits units padded by 12 bits */

        WAITFOR_BUFF( 8 );
        do
	{
		int c, n = nibbles;
		while ((n -= 2) >= 0)
		{
			c = BITSWAP(*image++);
			outw(BITBLT_IO_PORT_L0, c >> 4);
			outw(BITBLT_IO_PORT_L0, c);
		}
		if (n & 1)
		{
			c = BITSWAP(*image++);
			outw(BITBLT_IO_PORT_L0, c >> 4);
		}
		image += pad;
	} while (--h > 0);
        wdPriv->curRegBlock = curBlock;
}

/*
 * wd33DrawGlyph() - Assume glyph is already cached and is located cacheX,cacheY
 *                   in plane cachePlane.
 *                   Copy glyph from cache to desired position pbox
 *                   Convert foreground color , use alu and planemask 
 *                   
 *	pbox       - Destination rectangle to draw into.
 *	cacheX,cacheY - glyph coordinates in cache memory
 *	cachePlane - plane of glyph in cache
 *	fg         - desired foreground color
 *	alu        - the raster op to use when drawing.
 *	planemask  - the window planes to be affected.
 *	pDraw      - the window to draw on.
 */

void
wd33DrawGlyph(
	BoxPtr pbox,
	int cacheX,
        int cacheY,
	int cachePlane,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        register int curBlock= wdPriv->curRegBlock;
	int w, h;

#ifdef DEBUG_PRINT
	ErrorF("wd33DrawGlyph(box=(%d,%d)-(%d,%d),cacheXY=(%d,%d),cachePlane=%d \n",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2, cacheX,cacheY,cachePlane);
	ErrorF(" fg=0x%x, alu=%d, planemask=0x%x)\n",
		fg, alu, planemask);
#endif

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask);
	WRITE_2_REG( ENG_2, TRANSPMASK_IND_0, 1 << cachePlane );
	WRITE_2_REG( ENG_2, TRANSPCOL_IND_0 , wdPriv->planeMaskMask );

        WAITFOR_BUFF( 8 );
        WRITE_REG( ENG_1, SOURCE_X      , cacheX );
        WRITE_REG( ENG_1, SOURCE_Y      , cacheY );
        WRITE_REG( ENG_1, DEST_X        , pbox->x1 );
        WRITE_REG( ENG_1, DEST_Y        , pbox->y1 );
        WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );
        WRITE_REG( ENG_1, DIM_Y_IND     , h - 1 );
        WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );

        WAITFOR_BUFF( 2 );
        WRITE_REG( ENG_1, CNTRL_2_IND , wdPriv->bit11_10 | BIT_6_5 |
                                             MONO_TRANSP_ENAB );
        WRITE_REG( ENG_1, CNTRL_1_IND , BITBLIT | SRC_IS_MONO_COMP );
        wdPriv->curRegBlock = curBlock;
}
