/*
 *  @(#) wdImage24.c 11.1 97/10/22
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
 * wdImage24.c
 */
/*
 *
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Thu Feb 11 17:46:45 PST 1993	buckm@sco.com
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


/*
 * wdReadImage24() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.
 *		pack 24-bit pixels one per dword.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
wdReadImage24(pbox, image, stride, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	DrawablePtr pDraw;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
	int w, h;
        int srcAddr;
	register unsigned int r, *p;
	register int n;

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );

	srcAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * 3;

	WAITFOR_WD();

	WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	WRITE_2_REG( SOURCE_IND    , srcAddr );
	WRITE_2_REG( DEST_IND      , 0 );
	WRITE_1_REG( DIM_X_IND     , w * 3 );
	WRITE_1_REG( DIM_Y_IND     , h );
	WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
	WRITE_1_REG( PLANEMASK_IND , 0xFF );

	WRITE_1_REG( CNTRL_1_IND , START | PACKED_MODE | DST_IS_IO_PORT );

	while( --h >= 0 )
	{
	    p = (unsigned int *) image;
	    image += stride;
	    n = w >> 2;
	    while( --n >= 0 )
	    {
		r = inw( BITBLT_IO_PORT_L ) & 0xFFFF;
		r |= inw( BITBLT_IO_PORT_L ) << 16;
		*p++ = r;

		r >>= 24;
		r |= inw( BITBLT_IO_PORT_L ) << 8;
		*p++ = r;
		r = inw( BITBLT_IO_PORT_L ) & 0xFFFF;

		r |= inw( BITBLT_IO_PORT_L ) << 16;
		*p++ = r;
		r >>= 24;
		r |= inw( BITBLT_IO_PORT_L ) << 8;

		*p++ = r;
	    }
	    switch( w & 3 )
	    {
	    case 1:
		r = inw( BITBLT_IO_PORT_L ) & 0xFFFF;
		r |= inw( BITBLT_IO_PORT_L ) << 16;
		*p = r;
		break;

	    case 2:
		r = inw( BITBLT_IO_PORT_L ) & 0xFFFF;
		r |= inw( BITBLT_IO_PORT_L ) << 16;
		*p++ = r;

		r >>= 24;
		r |= inw( BITBLT_IO_PORT_L ) << 8;
		*p = r;
		inw( BITBLT_IO_PORT_L );
		break;

	    case 3:
		r = inw( BITBLT_IO_PORT_L ) & 0xFFFF;
		r |= inw( BITBLT_IO_PORT_L ) << 16;
		*p++ = r;

		r >>= 24;
		r |= inw( BITBLT_IO_PORT_L ) << 8;
		*p++ = r;
		r = inw( BITBLT_IO_PORT_L ) & 0xFFFF;

		r |= inw( BITBLT_IO_PORT_L ) << 16;
		*p = r;
		inw( BITBLT_IO_PORT_L );
		break;
	    }
	}
}

/*
 * wdDrawImage24() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
wdDrawImage24(pbox, image, stride, alu, planemask, pDraw)
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

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	assert( w > 0 && h > 0 );

	dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1 * 3;

	/* is planemask identical in each byte ? */
	if( ((planemask ^ (planemask >> 8)) & 0xFFFF) == 0 )
	{
	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	    WRITE_2_REG( SOURCE_IND    , 0 );
	    WRITE_2_REG( DEST_IND      , dstAddr );
	    WRITE_1_REG( DIM_X_IND     , w * 3 );
	    WRITE_1_REG( DIM_Y_IND     , h );
	    WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	    WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );
	    WRITE_1_REG( CNTRL_1_IND   , START | PACKED_MODE | SRC_IS_IO_PORT );

	    while( --h >= 0 )
	    {
		unsigned int r;
		unsigned int *p = (unsigned int *) image;
		int n = w >> 2;

		image += stride;
		while( --n >= 0 )
		{
		    r = *p++ & 0xFFFFFF;
		    outw( BITBLT_IO_PORT_L, r );
		    r >>= 16;
		    r |= *p++ << 8;
		    outw( BITBLT_IO_PORT_L, r );

		    r >>= 16;
		    outw( BITBLT_IO_PORT_L, r );
		    r = *p++ & 0xFFFFFF;
		    outw( BITBLT_IO_PORT_L, r );

		    r >>= 16;
		    r |= *p++ << 8;
		    outw( BITBLT_IO_PORT_L, r );
		    r >>= 16;
		    outw( BITBLT_IO_PORT_L, r );
		}
		switch ( w & 3 )
		{
		case 1:
		    r = *p;
		    outw( BITBLT_IO_PORT_L, r );
		    r >>= 16;
		    outw( BITBLT_IO_PORT_L, r );
		    break;

		case 2:
		    r = *p++ & 0xFFFFFF;
		    outw( BITBLT_IO_PORT_L, r );
		    r >>= 16;
		    r |= *p << 8;
		    outw( BITBLT_IO_PORT_L, r );

		    r >>= 16;
		    outw( BITBLT_IO_PORT_L, r );
		    outw( BITBLT_IO_PORT_L, r );
		    break;

		case 3:
		    r = *p++ & 0xFFFFFF;
		    outw( BITBLT_IO_PORT_L, r );
		    r >>= 16;
		    r |= *p++ << 8;
		    outw( BITBLT_IO_PORT_L, r );

		    r >>= 16;
		    outw( BITBLT_IO_PORT_L, r );
		    r = *p;
		    outw( BITBLT_IO_PORT_L, r );

		    r >>= 16;
		    outw( BITBLT_IO_PORT_L, r );
		    outw( BITBLT_IO_PORT_L, r );
		    break;
		}
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
		if (pm = (planemask >> 8) & 0xFF)
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
		if (pm = (planemask >> 16) & 0xFF)
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
		++dstAddr; image += 2;
	    } while (--w > 0);
	}
}
