/*
 *  @(#) wdImage15.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
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
 * wdImage15.c
 */
/*
 *
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Mar 16 18:20:25 PST 1993	buckm@sco.com
 *		Created.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdBankMap.h"


/*
 * wdReadImage15() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.
 *		pack 15-bit pixels one per short.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
wdReadImage15(pbox, image, stride, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	DrawablePtr pDraw;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
	int w, h;
        int srcAddr;
	unsigned char *pSrc;

	w = (pbox->x2 - pbox->x1) * 2;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );

	srcAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 2;
	WD_MAP_AHEAD(wdPriv, srcAddr, w, pSrc);
        WAITFOR_WD();

	memcpy(image, pSrc, w);

	while (--h > 0)
	{
	    image += stride;
	    pSrc += wdPriv->fbStride;
	    WD_REMAP_AHEAD(wdPriv, pSrc, w);
	    memcpy(image, pSrc, w);
	}
}

/*
 * wdDrawImage15() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
wdDrawImage15(pbox, image, stride, alu, planemask, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
	int w, h;
	int dstAddr;

	planemask &= 0x7FFF;

	w = (pbox->x2 - pbox->x1) * 2;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );

	dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 2;

	if ((alu == GXcopy) && (planemask == 0x7FFF))
	{
	    char *pDst;

	    WD_MAP_AHEAD(wdPriv, dstAddr, w, pDst);
	    WAITFOR_WD();
	    memcpy(pDst, image, w);

	    while (--h > 0)
	    {
		image += stride;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, w);
		memcpy(pDst, image, w);
	    }
	}
	else if( ((planemask ^ (planemask >> 8)) & 0x7F) == 0 )
	{
	    int w16;

	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	    WRITE_2_REG( SOURCE_IND    , 0);
	    WRITE_2_REG( DEST_IND      , dstAddr);
	    WRITE_1_REG( DIM_X_IND     , w );
	    WRITE_1_REG( DIM_Y_IND     , h );
	    WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	    WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
	    WRITE_1_REG( CNTRL_1_IND   , START | PACKED_MODE | SRC_IS_IO_PORT );

	    w = ( w + 3 ) & ~3;
	    w16 = w >> 1;
	    if( w == stride )
		repoutsw( BITBLT_IO_PORT_L, image, w16 * h );
	    else
		while( --h >= 0 )
		{
		    repoutsw( BITBLT_IO_PORT_L, image, w16 );
		    image += stride;
		}
	}
	else
	{   /* draw 1-byte vertical stripes */
	    int pm;

	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	    WRITE_2_REG( SOURCE_IND    , 0 );
	    WRITE_1_REG( DIM_X_IND     , 1 );
	    WRITE_1_REG( DIM_Y_IND     , h );
	    WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	    WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | SRC_IS_IO_PORT );

	    do
	    {
		if (pm = planemask & 0xFF)
		{
		    int i = h;
		    unsigned char *p = image;
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		    do
		    {
			outw( BITBLT_IO_PORT_L, *p );
			outw( BITBLT_IO_PORT_L, 0 );
			p += stride;
		    } while (--h > 0);
		}
		++dstAddr; ++image;
		if (pm = planemask >> 8)
		{
		    int i = h;
		    unsigned char *p = image;
		    WRITE_1_REG( PLANEMASK_IND , pm );
		    WRITE_2_REG( DEST_IND      , dstAddr );
		    do
		    {
			outw( BITBLT_IO_PORT_L, *p );
			outw( BITBLT_IO_PORT_L, 0 );
			p += stride;
		    } while (--h > 0);
		}
		++dstAddr; ++image;
	    } while ((w -= 2) > 0);
	}
}
