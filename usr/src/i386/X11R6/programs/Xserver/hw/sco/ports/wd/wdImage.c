/*
 *  @(#) wdImage.c 11.1 97/10/22
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
 * wdImage.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver and modified to accept
 *              image bitorder LSBfirst
 *	S001	Fri 09-Oct-1992	buckm@sco.com
 *		get rid of SourceValidate calls,
 *		  this work-around is no longer needed.
 *      S002    Wdn 14-Oct-1992 edb@sco.com
 *              changes to implement WD90C31 cursor
 *	S003	Thu  29-Oct-1992	edb@sco.com
 *              GC caching by checking of serial # changes
 *	S004	Thu  05-Nov-1992	edb@sco.com
 *              undo S003, does'nt work for window ops
 *	S005	Thu  07-Jan-1993	buckm@sco.com
 *              work around mono transparency chip bug.
 *      S006    Tue 09-Feb-1993 buckm@sco.com
 *              change parameter checks into assert()'s.
 *		get rid of GC caching.
 *		use frame buffer access in {Read,Draw}Image.
 *	S007	Wed 25-Jan-1995 brianm@sco.com
 *		added in the wdDrawDumbImage which avoids using the
 *		blitter.  Temporarily used in wd90c24 laptops.
 *		Also cleared up some compiler warnings char -> unsigned char
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"						/* S005 */

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdBitswap.h"
#include "wdBankMap.h"


/*
 * wdReadImage() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.
 *		pack 8-bit pixels one per byte.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
wdReadImage(pbox, image, stride, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	DrawablePtr pDraw;
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int w, h;
        int srcAddr;
	unsigned char *pSrc;

#ifdef DEBUG_PRINT
	ErrorF("wdReadImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d)\n", image, stride);
#endif

        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        assert( w > 0 && h > 0 );

        srcAddr = pbox->y1 * wdPriv->fbStride + pbox->x1;
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
 * wdDrawImage() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
wdDrawImage(pbox, image, stride, alu, planemask, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int w, h;
	int w16;
        int dstAddr;

#ifdef DEBUG_PRINT
	ErrorF("wdDrawImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d, alu=%d, planemask=0x%x)\n",
		image, stride, alu, planemask);
#endif

	planemask &= 0xFF;

        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        assert( w > 0 && h > 0 );

        dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1;

        WAITFOR_WD();

	if ((alu == GXcopy) && (planemask == 0xFF))
	{
	    unsigned char *pDst;

	    WD_MAP_AHEAD(wdPriv, dstAddr, w, pDst);
	    memcpy(pDst, image, w);

	    while (--h > 0)
	    {
		image += stride;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, w);
		memcpy(pDst, image, w);
	    }
	}
	else
	{
	    WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	    WRITE_2_REG( SOURCE_IND    , 0);
	    WRITE_2_REG( DEST_IND      , dstAddr);
	    WRITE_1_REG( DIM_X_IND     , w );
	    WRITE_1_REG( DIM_Y_IND     , h );
	    WRITE_1_REG( RASTEROP_IND  , alu << 8 );
	    WRITE_1_REG( PLANEMASK_IND , planemask );

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
}


/*
 * wdDrawMonoImage() - Draw color-expanded monochrome image;
 *			don't draw '0' bits.
 *                     adjust to bitorder for R5  ( LSBfirst )
 *	pbox - destination rectangle to draw into.
 *	image - the bits to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	startx - number of bits to skip at start of each line of image.
 *	fg - the color to make the '1' bits.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawMonoImage(pbox, image, startx, stride, fg, alu, planemask, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int startx;
	unsigned int stride;
	unsigned long fg;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;

{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int w, h;
        int bytes, nibbles, pad, dstAddr;

#ifdef DEBUG_PRINT
	ErrorF("wdDrawMonoImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
     ErrorF("image=0x%x, startx=%d stride=%d, fg=%d, alu=%d, planemask=0x%x)\n",
		image, startx, stride, fg, alu, planemask);
#endif

        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        assert( w > 0 && h > 0 );

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

        dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1;

	bytes   = ((startx + w + 7) >> 3) - (startx >> 3);
	nibbles = ((startx + w + 3) >> 2) - (startx >> 2);
	pad = stride - bytes;
	image += startx >> 3;

        WAITFOR_WD();

    /* the source register allows only a 4 pixels (2 bits)alignement field   */
    /* if startx is larger we have to skip 4 pixels in the first source byte */

        WRITE_1_REG( CNTRL_2_IND   , MONO_TRANSP_ENAB );
        WRITE_2_REG( SOURCE_IND    , startx & 0x03 );
        WRITE_2_REG( DEST_IND      , dstAddr );
        WRITE_1_REG( DIM_X_IND     , w );
        WRITE_1_REG( DIM_Y_IND     , h );
        WRITE_1_REG( RASTEROP_IND  , alu << 8 );
        WRITE_1_REG( FOREGR_IND    , fg );
        WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );

        WRITE_1_REG( CNTRL_1_IND ,
		     START | PACKED_MODE | SRC_IS_IO_PORT | SRC_IS_MONO );

        /* The WDC31 wants 4 bits units padded by 12 bits */

	do
	{
	    int c, n = nibbles;
	    if (startx & 4)
	    {
		c = BITSWAP(*image++);
		outw(BITBLT_IO_PORT_L, c);
		--n;
	    }
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
 * wdDrawOpaqueMonoImage() - Draw color-expanded monochrome image.
 *	pbox - destination rectangle to draw into.
 *	image - the bits to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	startx - number of bits to skip at start of each line of image.
 *	fg - the color to make the '1' bits.
 *	bg - the color to make the '0' bits.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
wdDrawOpaqueMonoImage(pbox,image,startx,stride,fg,bg,alu,planemask,pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int startx;
	unsigned int stride;
	unsigned long fg;
	unsigned long bg;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int w, h;
        int bytes, nibbles, pad, dstAddr;

#ifdef DEBUG_PRINT
	ErrorF("wdDrawOpaqueMonoImage(box=(%d,%d)-(%d,%d), image=0x%x, \n",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2, image );
        ErrorF(" startx=%d stride=%d, fg=%d, bg=%d, alu=%d, planemask=0x%x)\n",
		startx, stride, fg, bg, alu, planemask);
#endif
        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        assert( w > 0 && h > 0 );

        dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1;

	bytes   = ((startx + w + 7) >> 3) - (startx >> 3);
	nibbles = ((startx + w + 3) >> 2) - (startx >> 2);
	pad = stride - bytes;
	image += startx >> 3;

        WAITFOR_WD();

        WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
        WRITE_2_REG( SOURCE_IND    , startx & 3 );
        WRITE_2_REG( DEST_IND      , dstAddr );
        WRITE_1_REG( DIM_X_IND     , w );
        WRITE_1_REG( DIM_Y_IND     , h );
        WRITE_1_REG( RASTEROP_IND  , alu << 8 );
        WRITE_1_REG( FOREGR_IND    , fg );
        WRITE_1_REG( BACKGR_IND    , bg );
        WRITE_1_REG( PLANEMASK_IND , planemask & 0xFF );

        WRITE_1_REG( CNTRL_1_IND ,
		     START | PACKED_MODE | SRC_IS_IO_PORT | SRC_IS_MONO );

        /* The WDC31 wants 4 bits units padded by 12 bits */

	do
	{
	    int c, n = nibbles;
	    if (startx & 4)
	    {
		c = BITSWAP(*image++);
		outw(BITBLT_IO_PORT_L, c);
		--n;
	    }
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


#ifdef DEBUG_PRINT

dumpRegs()
{
      unsigned short word;
      int dword1, dword2, index;
      static char *bit11[2]  = { "STOP","START" };
      static char *bit10[2] =  {"DIR=l->r","DIR=l<-r" };
      static char *bit9_8[4]={"MODE=planar","MODE=packed","MODE=??","MODE=??"};
      static char *bit7[2]   = { "DST=rect","DST=lin " };
      static char *bit6[2]   = { "SRC=rect ","SRC=lin " };
      static char *bit5_4[4] = { "DST=scrn","DST=??","DST=I/O ","DST=??"};
      static char *bit3_2[4] = { "SRC=color","SRC=comp","SRC=fill","SRC=mono"};
      static char *bit1_0[4] = { "SRC=scrn","SRC=??","SRC=I/O","SRC=??"};

      static char *port[15] = {
                      "Control 1 ","Control 2 ",
                      "Sourc addr"," ","Dest addr "," ",
                      "Dim X     ","Dim Y     ",
                      "Row pitch ","Rasterop  ",
                      "Foregr col","Backgr col",
                      "Transp col","Transp mask","Plane mask" };

      ErrorF("BITBLT Registers\n");

      READ_1_REG( word, CNTRL_1_IND );
   ErrorF("Cntrl 1:  %s  %s  %s  %s  %s\n                %s  %s  %s\n",
          bit11 [ ( word & 0x0800 ) >> 11 ]  ,
          bit10 [ ( word & 0x0400 ) >> 10 ]  ,
          bit9_8[ ( word & 0x0300 ) >>  8 ]  ,
          bit7  [ ( word & 0x0080 ) >>  7 ]  ,
          bit6  [ ( word & 0x0040 ) >>  6 ]  ,
          bit5_4[ ( word & 0x0030 ) >>  4 ]  ,
          bit3_2[ ( word & 0x000C ) >>  2 ]  ,
          bit1_0[ ( word & 0x0003 )       ]  );

          READ_2_REG( dword1, 2 );
          READ_2_REG( dword2, 4 );
          ErrorF("  %s: %6d,  %s: %6d\n",
                  port[2], dword1, port[4], dword2 );
          for( index=6; index <= 0xE; index++)
          {
               READ_1_REG( word, index ); 
               ErrorF("  %s: 0x%4x,", port[index], word );
               if((index % 2) ==1) ErrorF("\n");
          }
          ErrorF("\n");
}

#endif /* DEBUG_PRINT */

/*
 * S007
 * Temporary Routine, should try to get rid of this for WD90C24 and use blitter
 *
 * wdDrawDumbImage() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
wdDrawDumbImage(pbox, image, stride, alu, planemask, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int w, h;
	int w16;
        int dstAddr;
	register int tmp;
	register unsigned char *src, *dst;

#ifdef DEBUG_PRINT
	ErrorF("wdDrawDumbImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d, alu=%d, planemask=0x%x)\n",
		image, stride, alu, planemask);
#endif

	planemask &= 0xFF;

        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        assert( w > 0 && h > 0 );

        dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1;

        WAITFOR_WD();

	if (planemask == 0xFF)
	{
	    unsigned char *pDst;

            if (alu == GXnoop)
		return;

	    WD_MAP_AHEAD(wdPriv, dstAddr, w, pDst);

	    while (--h >= 0)
	    {
		tmp = w;
		src = image;
		dst = pDst;

		if (alu == GXcopy)
			memcpy( dst, src, w );
		else
			switch (alu)
			{
				case GXclear:
				while (--tmp >= 0)
				{
					*dst = fnCLEAR( *src, *dst );
					dst ++;
					src ++;
				}
				break;
				case GXand:
				while (--tmp >= 0)
				{
					*dst = fnAND( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXandReverse:
				while (--tmp >= 0)
				{
					*dst = fnANDREVERSE( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXandInverted:
				while (--tmp >= 0)
				{
					*dst = fnANDINVERTED( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXxor:
				while (--tmp >= 0)
				{
					*dst = fnXOR( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXor:
				while (--tmp >= 0)
				{
					*dst = fnOR( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXnor:
				while (--tmp >= 0)
				{
					*dst = fnNOR( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXequiv:
				while (--tmp >= 0)
				{
					*dst = fnEQUIV( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXinvert:
				while (--tmp >= 0)
				{
					*dst = fnINVERT( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXorReverse:
				while (--tmp >= 0)
				{
					*dst = fnORREVERSE( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXcopyInverted:
				while (--tmp >= 0)
				{
					*dst = fnCOPYINVERTED( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXorInverted:
				while (--tmp >= 0)
				{
					*dst = fnORINVERTED( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXnand:
				while (--tmp >= 0)
				{
					*dst = fnNAND( *src, *dst );
					dst++;
					src++;
				}
				break;
				case GXset:
				while (--tmp >= 0)
				{
					*dst = fnSET( *src, *dst );
					dst++;
					src++;
				}
				break;
			}

		image += stride;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, w);
	    }
	} else {
	    unsigned char *pDst;

            if (alu == GXnoop)
		return;

	    WD_MAP_AHEAD(wdPriv, dstAddr, w, pDst);

	    while (--h >= 0)
	    {
		tmp = w;
		src = image;
		dst = pDst;

		switch (alu)
		{
			case GXcopy:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnCLEAR( *src, *dst ),
					*dst, planemask );
				dst ++;
				src ++;
			}
			break;
			case GXclear:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnCLEAR( *src, *dst ),
					*dst, planemask );
				dst ++;
				src ++;
			}
			break;
			case GXand:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnAND( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXandReverse:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnANDREVERSE( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXandInverted:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnANDINVERTED( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXxor:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnXOR( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXor:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnOR( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXnor:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnNOR( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXequiv:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnEQUIV( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXinvert:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnINVERT( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXorReverse:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnORREVERSE( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXcopyInverted:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnCOPYINVERTED( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXorInverted:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnORINVERTED( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXnand:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnNAND( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
			case GXset:
			while (--tmp >= 0)
			{
				*dst = RopWithMask(fnSET( *src, *dst ),
					*dst, planemask );
				dst++;
				src++;
			}
			break;
		}

		image += stride;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, w);
	    }
	}
}
