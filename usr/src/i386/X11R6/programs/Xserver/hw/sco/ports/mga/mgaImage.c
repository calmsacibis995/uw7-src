/*
 * @(#) mgaImage.c 11.1 97/10/22
 *
 * Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
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
 *	S001	Fri Jul 21 14:58:42 PDT 1995	brianm@sco.com
 *	- Fixed a problem in mgaDrawImage where we weren't waiting for
 *	  enough slots when using something other than GXCopy.
 *	S000	Thu Jun  1 16:52:52 PDT 1995	brianm@sco.com
 *	- New code from Matrox.
 */

/*
 * mgaImage.c
 *
 * Template for machine dependent ReadImage and DrawImage routines
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

extern unsigned long mgaALU[16];
extern void mgaNormalSetup(mgaPrivatePtr, int*, int*, int*);

/*
 * mgaReadImage() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 1-bit pixels eight per byte;
 *		pack 2- to 8-bit pixels one per byte;
 *		pack 9- to 16-bit pixels one per 16-bit short;
 *		pack 17- to 32-bit pixels one per 32-bit word.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
mgaReadImage(pbox, image, stride, pDraw)
	BoxPtr pbox;
	void *image;
	unsigned int stride;
	DrawablePtr pDraw;
{
	mgaPrivatePtr privp = MGA_PRIVATE_DATA(pDraw->pScreen);
	VOLATILE register mgaTitanRegsPtr titan = &privp->regs->titan;

	int height, width;
	register int linyadr, count;
	register unsigned char *dst;
	register unsigned char *src;
	unsigned char *adst;

#ifdef DEBUG_PRINT
	ErrorF("mgaReadImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d)\n", image, stride);
#endif


	/* width and height in pixels */
	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;


	/* caluclate the linear source address */

	linyadr = (pbox->y1 * privp->bstride) + (privp->ydstorg * privp->bpp);
	adst = (unsigned char *) image;

	/* the source is allways within the first raster of the window */
	src = (unsigned char *)(privp->fbBase + (pbox->x1 * privp->bpp));
	dst = adst;
	count = width * privp->bpp;		/* one scan line */

	/* wait for processor not busy */

	WAIT_FOR_DONE(titan);

	while(height--)
	{
	    titan->srcpage = linyadr;
	    memcpy(dst, src, count);
	    dst += stride;
	    linyadr += privp->bstride; /* bumps us to next raster */
	}
}

/*
 * mgaDrawImage() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
mgaDrawImage(pbox, image, stride, alu, planemask, pDraw)
	BoxPtr pbox;
	void *image;
	unsigned int stride;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
	mgaPrivatePtr privp = MGA_PRIVATE_DATA(pDraw->pScreen);
	VOLATILE register mgaTitanRegsPtr titan = &privp->regs->titan;

	int height, width, tmask;
	register int linyadr, count, pixop, dsty;
	register unsigned char *dst;
	register unsigned char *src;
	unsigned char *asrc;


#ifdef DEBUG_PRINT
	ErrorF("mgaDrawImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d, alu=%d, planemask=0x%x)\n",
		image, stride, alu, planemask);
#endif

	/* width and height in pixels */
	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

        /* blit + alu + bfcol + afor */

        pixop = 8 | mgaALU[alu] | (2 << 25) | (1 << 27);
 
	asrc = (unsigned char *) image;
	dsty = pbox->y1;

	tmask = planemask;
	count = width * privp->bpp;
	src = asrc;

	mgaNormalSetup(privp, &tmask, NULL, NULL);

	/* poss. setup static drawing reg's */


	if(alu == 3)		/* alu is src so direct copy is poss. */
	{
	  WAIT_FOR_DONE(titan);

	  titan->drawSetup.plnwt		= tmask;
	  linyadr = (pbox->y1 * privp->bstride) + (privp->ydstorg * privp->bpp);
	  dst = (unsigned char *)(privp->fbBase + (pbox->x1 * privp->bpp));
	  while(height--)
	  {
	    titan->srcpage = linyadr;
	    memcpy(dst, src, count);
	    src += stride;
	    linyadr += privp->bstride;
	  }
	}
	else	/* copy to off screen mem, then blit to do rop */
	{


	  /*1st off screen line */
	  titan->srcpage = (privp->height * privp->bstride) +
			   (privp->ydstorg * privp->bpp);
	  dst = privp->fbBase;
	  while(height--)
	  {					/* S001 Begin */
	    WAIT_FOR_DONE(titan);
	    if((alu != 0) && (alu != 15))	/* if not set or clear */
	    {
	        titan->drawSetup.plnwt    = 0xffffffff;
		memcpy(dst, src, count);
	    }

	    /* wait for enough room in fifo */
	    /*	    ok, now blit with rop	*/
	    MGA_WAIT(titan, 11);

	    titan->drawSetup.sgn	= 0; /* l to r and t to b */
	    titan->drawSetup.shift	= 0; /* no shifting */
	    titan->drawSetup.dwgctl	= pixop;
	    titan->drawSetup.len	= 1; /* allways 1 raster */
	    titan->drawSetup.ar5	= privp->width;
	    titan->drawSetup.ar3	= (privp->pstride * privp->height);
	    titan->drawSetup.ar0	= (privp->pstride * privp->height)
				  + (width - 1);

	    titan->drawSetup.plnwt    = tmask;
	    titan->drawSetup.fxleft  = pbox->x1;
	    titan->drawSetup.fxright  = pbox->x2 - 1;
	    titan->drawGo.ydst  = dsty++;
	    src += stride;
	  }					/* S001 End */
	}
}
