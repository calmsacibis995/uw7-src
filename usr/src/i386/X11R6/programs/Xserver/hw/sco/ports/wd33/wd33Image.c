/*
 *  @(#) wd33Image.c 11.1 97/10/22
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
 * wd33Image.c
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
 *	S002	Thu 26-Aug-1993		edb@sco.com
 *		Call genDrawMonoImage if w <=2 or h<=2 
 *		This should be revised , I think there is something wrong 
 *              with the calculation of nibbles and bytes
 *	S003	Thu 26-Aug-1993		edb@sco.com
 *		done
 */

#include <stdio.h>

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wdBitswap.h"
#include "wdBankMap.h"


/*
 * wd33ReadImage() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.
 *		pack 8-bit pixels one per byte.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
wd33ReadImage(pbox, image, stride, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	DrawablePtr pDraw;
{
        register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        register int pix_bytes = wdPriv->pixBytes;
        int w, h;
        int srcAddr;
	char *pSrc;

#ifdef DEBUG_PRINT
	ErrorF("wd33ReadImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d)\n", image, stride);
#endif

        w = ( pbox->x2 - pbox->x1) * pix_bytes;
        h = pbox->y2 - pbox->y1;
        assert( w > 0 && h > 0 );

        WAITFOR_DE();

        srcAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * pix_bytes;
	WD_MAP_AHEAD(wdPriv, srcAddr, w, pSrc);

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
 * wd33DrawImage() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
wd33DrawImage(pbox, image, stride, alu, planemask, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
        register wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        register int curBlock = wdPriv->curRegBlock;
        register int pix_bytes = wdPriv->pixBytes;
	unsigned char *image_pad;
        unsigned short *imagePtr;
        int w, h;
	int w32, left_pad, i, ii;

#ifdef DEBUG_PRINT
	ErrorF("wd33DrawImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d, alu=%d, planemask=0x%x)\n",
		image, stride, alu, planemask);
#endif

	planemask &= wdPriv->planeMaskMask;
        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        assert( w > 0 && h > 0 );

	if ((alu == GXcopy) && (planemask == wdPriv->planeMaskMask))
	{
	    char *pDst;
            int dstAddr =  pbox->y1 * wdPriv->fbStride + pbox->x1 * pix_bytes;

            WAITFOR_DE();

            w *= pix_bytes;
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
            /* transfer needs to start and end 32 bit aligned */
            if( pix_bytes == 1 ) {
                 left_pad  = pbox->x1 & 3;
                 w32 = (w + left_pad + 3 ) >> 2;
            }
            else {  /* pixBytes == 2 ) */
                 left_pad  = pbox->x1 & 1;
                 w32 = (w + left_pad + 1 ) >> 1;
            }
            image_pad = image - left_pad;

            WAITFOR_BUFF( 8 );
	    WRITE_REG( ENG_1, SOURCE_X      , left_pad );
	    WRITE_REG( ENG_1, SOURCE_Y      , 0        );
	    WRITE_REG( ENG_1, DEST_X        , pbox->x1 );
	    WRITE_REG( ENG_1, DEST_Y        , pbox->y1 );
	    WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask);
	    WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );

            WAITFOR_BUFF( 4 );
	    WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );   /* Don't ask about the -1 */
	    WRITE_REG( ENG_1, DIM_Y_IND     , h - 1 );   /* its one of these HW stupidities*/
	    WRITE_REG( ENG_1, CNTRL_2_IND   , wdPriv->bit11_10 | BIT_6_5 | COLOR_EXPAND_16 );
	    WRITE_REG( ENG_1, CNTRL_1_IND   , BITBLIT | SRC_IS_IO_PORT );

            WAITFOR_BUFF( 8 );   /* ref 12.14.2 Programming HBLT */
	    while( --h >= 0 )
	    {
/*		 repoutsd( BITBLT_IO_PORT_L0, image_pad, w32 );  */
                 i = w32;
                 imagePtr = (unsigned short *)image_pad;
                 while( i-- ) {
                       outw( BITBLT_IO_PORT_L0, *imagePtr++ );
                       outw( BITBLT_IO_PORT_L1, *imagePtr++ );
                 }
		 image_pad += stride;
	    }
        }
        wdPriv->curRegBlock = curBlock;
}


/*
 * wd33DrawMonoImage() - Draw color-expanded monochrome image;
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
wd33DrawMonoImage(pbox, image, startx, stride, fg, alu, planemask, pDraw)
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
        register int curBlock = wdPriv->curRegBlock;
        int w, h;
        int nibbles;

#ifdef DEBUG_PRINT
	ErrorF("wd33DrawMonoImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
     ErrorF("image=0x%x, startx=%d stride=%d, fg=%d, alu=%d, planemask=0x%x)\n",
		image, startx, stride, fg, alu, planemask);
#endif

        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        /*
         *  The wd90c33 does stupid things if w or h is too small
         *  Occasionally it seems to halt the cpu
         */
        if( wdPriv->pixBytes > 1 && (w <= 2 || h <= 2 )) {
            genDrawMonoImage(pbox, image, startx, stride, fg, alu, planemask, pDraw);
            return;
        }
        assert( w > 0 && h > 0 );

	nibbles = ((startx + w + 3) >> 2) - (startx >> 2);
	image += startx >> 3;

    /*
     * For color expand operations the number of sourcebits which can
     * be expanded in one source to destination transfer ( 16 bit I/O )
     * is limited.
     * The wd90c33 allows to specify the number of valid source bits per
     * per 16 bit I/O register write. The options are 2, 4, 8 or 16
     * The 8 or 16 bits options seem to be allowed for a certain VGA timing
     * only. Therefore we go with 4 bits as in wd90c31
     */

        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_REG( ENG_1, SOURCE_X      , startx & 3 );
	WRITE_REG( ENG_1, SOURCE_Y      , 0 );
	WRITE_REG( ENG_1, DEST_X        , pbox->x1 );
	WRITE_REG( ENG_1, DEST_Y        , pbox->y1 );

        WAITFOR_BUFF( 6 );
        WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );
        WRITE_REG( ENG_1, DIM_Y_IND     , h - 1 );
        WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
        WRITE_REG( ENG_1, CNTRL_2_IND , wdPriv->bit11_10 | MONO_TRANSP_ENAB | BIT_6_5 
                                      | COLOR_EXPAND_4 );
        WRITE_REG( ENG_1, CNTRL_1_IND , BITBLIT | SRC_IS_IO_PORT | SRC_IS_MONO );

        /* The WDC31 wants 4 bits units padded by 12 bits */

        WAITFOR_BUFF( 8 );
	do
	{
	    int c, n = nibbles;
            unsigned char *img = image;
	    if (startx & 4)
	    {
		c = BITSWAP(*img++);
		outw(BITBLT_IO_PORT_L0, c);
		--n;
	    }
	    while ((n -= 2) >= 0)
	    {
		c = BITSWAP(*img++);
		outw(BITBLT_IO_PORT_L0, c >> 4);
		outw(BITBLT_IO_PORT_L0, c);
	    }
	    if (n & 1)
	    {
		c = BITSWAP(*img++);
		outw(BITBLT_IO_PORT_L0, c >> 4);
	    }
	    image += stride;
	} while (--h > 0);
        wdPriv->curRegBlock = curBlock;
}

/*
 * wd33DrawOpaqueMonoImage() - Draw color-expanded monochrome image.
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
wd33DrawOpaqueMonoImage(pbox,image,startx,stride,fg,bg,alu,planemask,pDraw)
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
        register int curBlock = wdPriv->curRegBlock;
        int w, h;
        int nibbles;

#ifdef DEBUG_PRINT
	ErrorF("wd33DrawOpaqueMonoImage(box=(%d,%d)-(%d,%d), image=0x%x, \n",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2, image );
        ErrorF(" startx=%d stride=%d, fg=%d, bg=%d, alu=%d, planemask=0x%x)\n",
		startx, stride, fg, bg, alu, planemask);
#endif
        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        /*
         *  The wd90c33 does stupid things if w or h is too small
         *  Occasionally it seems to halt the cpu
         */
        if( wdPriv->pixBytes > 1 && (w <= 2 || h <= 2 )) {
            genDrawOpaqueMonoImage(pbox,image,startx,stride,fg,bg,alu,planemask,pDraw);
            return;
        }
        assert( w > 0 && h > 0 );

	nibbles = ((startx + w + 3) >> 2) - (startx >> 2);
	image += startx >> 3;

        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WRITE_2_REG( ENG_2, BACKGR_IND_0    , bg );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask );
	WRITE_REG( ENG_1, SOURCE_X      , startx & 3 );
	WRITE_REG( ENG_1, SOURCE_Y      , 0 );

        WAITFOR_BUFF( 8 );
	WRITE_REG( ENG_1, DEST_X        , pbox->x1 );
	WRITE_REG( ENG_1, DEST_Y        , pbox->y1 );
        WRITE_REG( ENG_1, DIM_X_IND     , w - 1 );
        WRITE_REG( ENG_1, DIM_Y_IND     , h - 1 );
        WRITE_REG( ENG_1, RASTEROP_IND  , alu << 8 );
        WRITE_REG( ENG_1, CNTRL_2_IND , wdPriv->bit11_10 | BIT_6_5 | COLOR_EXPAND_4 );
        WRITE_REG( ENG_1, CNTRL_1_IND , BITBLIT | SRC_IS_IO_PORT | SRC_IS_MONO );

    /*
     * For color expand operations the number of sourcebits which can
     * be expanded in one source to destination transfer ( 16 bit I/O )
     * is limited.
     * The wd90c33 allows to specify the number of valid source bits per
     * per 16 bit I/O register write. The options are 2, 4, 8 or 16
     * The 8 or 16 bits options seem to be allowed for a certain VGA timing
     * only. Therefore we go with 4 bits as in wd90c31
     */

        WAITFOR_BUFF( 8 );
	do
	{
	    int c, n = nibbles;
	    unsigned char *img = image;
	    if (startx & 4)
	    {
		c = BITSWAP(*img++);
		outw(BITBLT_IO_PORT_L0, c);
		--n;
	    }
	    while ((n -= 2) >= 0)
	    {
		c = BITSWAP(*img++);
		outw(BITBLT_IO_PORT_L0, c >> 4);
		outw(BITBLT_IO_PORT_L0, c);
	    }
	    if (n & 1)
	    {
		c = BITSWAP(*img++);
		outw(BITBLT_IO_PORT_L0, c >> 4);
	    }
	    image += stride;
	} while (--h > 0);
        wdPriv->curRegBlock = curBlock;
}


#ifdef DUMP_REGS

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

      READ_1_REG( word, ENG_1, CNTRL_1_IND );
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
