/*
 * @(#) mgaRectOps.c 11.1 97/10/22
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
 *	SCO Modifications:
 *
 *	S003	Tue Jun 20 10:19:08 PDT 1995	brianm@sco.com
 *		made a copy into offscreen memory a little
 *		less efficient.  Solved a bug in the xbench bitmapcopy400
 *		and seems to have made the driver a bit faster. (?)
 *	S002	Thu Jun  1 16:56:14 PDT 1995	brianm@sco.com
 *		Merged in new code from Matrox.
 *	S001	Tue May 24 16:01:38 PDT 1994	hiramc@sco.COM
 *		Slight correction to the offscreen blit in PutMonoImage
 */

/*
 * mgaRectOps.c
 *
 * mga rectangular drawing ops.
 */

#include "X.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

extern unsigned long mgaALU[16];

/* this function sets the clipping rectangle, and will replicate bits	*/
/* in up to 3 arguments based on the curren depth.			*/
/* these are usually the mask, the forground color, and the background color */
/* NULL pointers are ignored */

mgaNormalSetup(mgaPrivatePtr pMga, int *arg1, int *arg2, int *arg3)
{
	VOLATILE register mgaTitanRegsPtr titan = &pMga->regs->titan;
	int *p[3];
	register int ap, i;
	
	/* fill in the array so we can loop */

	p[0] = arg1;
	p[1] = arg2;
	p[2] = arg3;

	/* for each non NULL pointer replicate the significant bits */

	if(pMga->depth != 32)	/* no replication on 32 bit depth */
	{
	  for(i = 0; i < 3; ++i)
	  {
	    if(p[i] == NULL) continue;

	    if(pMga->depth == 8) /* replicate the lower 8 bits 3 times */
	    {
		ap = *p[i] & 0xff;
		*p[i] = ap | (ap << 8) | (ap << 16) | (ap << 24);
	    }
	    else if(pMga->depth == 16) /* replicate the lower 16 bits once */
	    {
		ap = *p[i] & 0xffff;
		*p[i] = ap | (ap << 16);
	    }
	  }
	}

	/*	set clipping here	*/

	MGA_WAIT(titan, 4);	/* wait for 4 spots in the fifo */

	titan->drawSetup.cxleft	= pMga->clipXL;
	titan->drawSetup.cxright= pMga->clipXR;
	titan->drawSetup.ytop = pMga->clipYT;
	titan->drawSetup.ybot = pMga->clipYB;
}

/*
 * mgaCopyRect() - Copy a rectangle from screen to screen.
 *	pdstBox - the destination rectangle.
 *	psrc - the source point.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void 
mgaCopyRect(pdstBox, psrc, alu, planemask, pDraw)
	BoxPtr pdstBox;
	DDXPointPtr psrc;
	unsigned char alu;
	unsigned int planemask;
	DrawablePtr pDraw;
{
	mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	int srcx, srcy, dsty, xleft, xright;
	int w, h, mask;
	unsigned long pixop, sign;
	long sypitch, linstart, linend;

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

	mask = planemask;

	/* blit + alu + bfcol + afor */

	pixop = TITAN_OPCOD_BITBLT | mgaALU[alu] | TITAN_BLTMOD_BFCOL |
		TITAN_AFOR_FCOL;

	sypitch = pMga->pstride;   /* stride in pixels (top to bottom)*/
	sign = 0;		/* scan right to left and top to bottom; */

	w = (pdstBox->x2 - pdstBox->x1);
	h = (pdstBox->y2 - pdstBox->y1);

	srcx = psrc->x;
	srcy = psrc->y;
	xleft = pdstBox->x1;
	xright = pdstBox->x2 - 1;
	dsty = pdstBox->y1;

	if (srcy < dsty) {
		srcy += (h - 1);
		dsty += (h - 1);
		sypitch = -sypitch;	 /* scan bottom to top */
		sign = 0x4;		/* diddo */
	}

	linstart = (srcy * pMga->pstride) + srcx + pMga->ydstorg;

	if (srcx < xleft) {
		linend = linstart;
		linstart += (w - 1);
		sign |= 1;	/* scan left */
	}
	else
	    linend = linstart + (w - 1 );

	/* set clipping, and get replicated mask */
	mgaNormalSetup(pMga, &mask, NULL, NULL);

	MGA_WAIT(titan, 11);			/* wait for 11 locations */

	titan->drawSetup.sgn	= sign;		/* sign figured out above */
	titan->drawSetup.shift	= 0;		/*  no shift */
	titan->drawSetup.dwgctl	= pixop;
	titan->drawSetup.plnwt	= mask;		/* replicated mask */
	titan->drawSetup.len	= h;		/* number of scanlines */
	titan->drawSetup.ar5	= sypitch;	/* source pitch */
	titan->drawSetup.ydst  = dsty;		/* starting line */
	titan->drawSetup.fxleft  = xleft;	/* leftmost pixel */
	titan->drawSetup.fxright  = xright;	/* rightmost pixel */
	titan->drawSetup.ar0	= linend;	/* linear end address */
	titan->drawGo.ar3	= linstart;	/* linear start and go */
}

/*
 * mgaDrawSolidRects() - Draw solid-color rectangles.
 *	pbox - array of rectangles to draw.
 *	nbox - number of rectangles.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
mgaDrawSolidRects(pbox, nbox, fg, alu, planemask, pDraw)
	register BoxPtr pbox;
	unsigned int nbox;
	unsigned long fg;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	int pixop, mask, fgc;
	int h;

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

	pixop = TITAN_OPCOD_TRAP | mgaALU[alu] | TITAN_AFOR_FCOL |
		TITAN_TRANSC_TRANSPARENT;

	mask = planemask;
	fgc = fg;
	h = (pbox->y2 - pbox->y1);

	/* set clipping, and get replicated mask, and fg */
	mgaNormalSetup(pMga, &mask, &fgc, NULL);

	MGA_WAIT(titan, 19); /*wait for 19 spots in fifo*/

	titan->drawSetup.ar0	= 0;	/* rectangle is trap with virt sides */
	titan->drawSetup.ar6	= 0;
	titan->drawSetup.ar1	= 0;
	titan->drawSetup.ar4	= 0;
	titan->drawSetup.ar2	= 0;
	titan->drawSetup.ar5	= 0;
	titan->drawSetup.shift	= 0;	/* no shift */
	titan->drawSetup.sgn	= 0;	/* l to r, and t to b */

	titan->drawSetup.dwgctl	= pixop;
	titan->drawSetup.src0	= 0xffffffff;	/* no pattern */
	titan->drawSetup.src1	= 0xffffffff;
	titan->drawSetup.src2	= 0xffffffff;
	titan->drawSetup.src3	= 0xffffffff;

	titan->drawSetup.fcol	= fgc;		/* replicated fg */
	titan->drawSetup.plnwt	= mask;		/* replicated plane mask */

	titan->drawSetup.fxleft = pbox->x1;	/* left pixel */
	titan->drawSetup.fxright = pbox->x2;	/* note one passed right pixel*/
	titan->drawSetup.ydst = pbox->y1;	/* top y */
	titan->drawGo.len = h;			/* no. of lines and go */

	/* while more boxes to do */

	while (--nbox > 0) {
		pbox++;
		h = (pbox->y2 - pbox->y1);

		MGA_WAIT(titan, 4);

		titan->drawSetup.fxleft = pbox->x1;
		titan->drawSetup.fxright = pbox->x2;
		titan->drawSetup.ydst = pbox->y1;
		titan->drawGo.len = h;
	}
}

/*
 * mgaDrawPoints() - Draw points.
 *	ppt - array of points to draw.
 *	npts - number of points.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
mgaDrawPoints(ppt, npts, fg, alu, planemask, pDraw)
	register DDXPointPtr ppt;
	unsigned int npts;
	unsigned long fg;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	int pixop, mask, fgc;

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

	/* use trapazoid function to draw points */

	pixop = TITAN_OPCOD_TRAP | mgaALU[alu] | TITAN_AFOR_FCOL |
		TITAN_TRANSC_TRANSPARENT;

	mask = planemask;
	fgc = fg;

	/* set clipping, and get replicated mask and fg */
	mgaNormalSetup(pMga, &mask, &fgc, NULL);

	MGA_WAIT(titan, 19); /*wait for 19 spots in fifo*/

	titan->drawSetup.ar0	= 0;	/* rect is trap with virt sides */
	titan->drawSetup.ar6	= 0;
	titan->drawSetup.ar1	= 0;
	titan->drawSetup.ar4	= 0;
	titan->drawSetup.ar2	= 0;
	titan->drawSetup.ar5	= 0;
	titan->drawSetup.shift	= 0;	/* no shift */
	titan->drawSetup.sgn	= 0;	/* l to r, and t to b */

	titan->drawSetup.dwgctl	= pixop;
	titan->drawSetup.src0	= 0xffffffff;	/* no pattern */
	titan->drawSetup.src1	= 0xffffffff;
	titan->drawSetup.src2	= 0xffffffff;
	titan->drawSetup.src3	= 0xffffffff;

	titan->drawSetup.fcol	= fgc;	/* replicated fg */
	titan->drawSetup.plnwt	= mask;	/* replicated planemask */

	titan->drawSetup.fxleft = ppt->x;
	titan->drawSetup.fxright = ppt->x + 1;	/* note difference from blit */
	titan->drawSetup.ydst = ppt->y;
	titan->drawGo.len = 1;			/* and start */

	while (--npts > 0) {
		ppt++;

		MGA_WAIT(titan, 4);
		titan->drawSetup.fxleft = ppt->x;
		titan->drawSetup.fxright = ppt->x + 1;
		titan->drawSetup.ydst = ppt->y;
		titan->drawGo.len = 1;			/* and start */
	}
}

/*
 * mgaPutMonoImage() - Draw color-expanded monochrome image;
 *	pbox - destination rectangle to draw into.
 *	image - the bits to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	startx - number of bits to skip at start of each line of image.
 *	fg - the color to make the '1' bits.
 *	bg - the color to make the '0' bits if not transparent.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 *	Trans - flag for transparent 1 = don't draw 0 bits, 0 = do draw them.
 */
static void
mgaPutMonoImage(pbox, image, startx, stride, fg, bg, alu, planemask, pDraw,
		 Trans)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int startx;
	unsigned int stride;
	unsigned long fg;
	unsigned long bg;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
	int Trans;
{

	mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	int w, h, th, bytes, rowsperblit, rowsperslice;
	unsigned char *pwrk, *ti;
	int srcx, srcy, dsty, xleft, xright;
	int mask;
	unsigned long pixop, sign;
	long sypitch, linstart, linend;
	int linyaddr;

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	bytes = (w + startx + 7) >> 3;

	/* if not enough room in offscreen mem, let gen do it */
	if (bytes > pMga->offscreenSize) {
		genDrawMonoImage(pbox, image, startx, stride,
				 fg, alu, planemask, pDraw);
		return;
	}

	mask = planemask;

	/* blit + alu + bmono + afor */

	pixop = TITAN_OPCOD_BITBLT | mgaALU[alu] | TITAN_BLTMOD_BMONO |
		TITAN_AFOR_FCOL;

	if(Trans)
	    pixop |= TITAN_TRANSC_TRANSPARENT; /* if transparent set the bit */

	sypitch = stride * 8;	   /* stride in pixels (top to bottom) */
	sign = 0;		/* scan right to left and top to bottom; */

	srcx = startx;
	srcy = pMga->height;
	xleft = pbox->x1;
	xright = pbox->x2 - 1;
	dsty = pbox->y1;

	/* linear start address in 1bit pixels */

	linstart = (srcy * pMga->pstride * pMga->depth) + startx +
		   (pMga->ydstorg * pMga->depth);

	linend = linstart + (w - 1 );

	/* figure out rows per blit and rows ber slice */
	/* rows per blit is the number of rows of offscreen memory */
	/* rows per slice is the number of rows in a 7k video aperture */

	if (stride > pMga->offscreenSize)
	{
		rowsperblit = rowsperslice = 1;
	}
	else
	{
		bytes = stride;

		rowsperslice = (7 * 1024) / bytes;

		/* make sure rowsperblit is an even multiple of rowsperslice */

		rowsperblit = ((pMga->offscreenSize / bytes) / rowsperslice)
				* rowsperslice;
		if(rowsperblit > h)
		    rowsperblit = h;

		if(rowsperslice > rowsperblit)
		    rowsperslice = rowsperblit;

		/* S003 Start match to window */
		if(rowsperblit > rowsperslice)	
			rowsperblit = rowsperslice;
		/* S003 End */
	}

	pwrk = pMga->fbBase;
	linyaddr = (pMga->height * pMga->bstride) + (pMga->ydstorg * pMga->bpp);

	/* wait for idle drawing engine */

	WAIT_FOR_DONE(titan);

	/* make sure all bits are writeable */
	titan->drawSetup.plnwt = 0xffffffff;

	ti = (unsigned char *)image;

	/* copy as much of the image as we can into offscreen mem */

	for(th = rowsperblit; th >= rowsperslice; th -= rowsperslice) /* S001 */
	{
	    titan->srcpage = linyaddr;
	    memcpy(pwrk, ti, stride * rowsperslice);
	    linyaddr += (rowsperslice * pMga->bstride);
	    ti += (rowsperslice * stride);
	}
	/* set clip, and get replicated mask, fg, and bg */

	mgaNormalSetup(pMga, &mask, (int *)&fg, (int *)&bg);

	/* no need to wait, there is room since we were done befor memcpy */
	/* load stuff that wont change between blits */

	titan->drawSetup.sgn	= sign;
	titan->drawSetup.shift	= 0;
	titan->drawSetup.dwgctl	= pixop;
	titan->drawSetup.fcol	= fg;
	titan->drawSetup.bcol	= bg;
	titan->drawSetup.ar5	= sypitch;
	titan->drawSetup.fxleft  = xleft;
	titan->drawSetup.fxright  = xright;

	/* now do the first (maybe only) blit */

	titan->drawSetup.len	= rowsperblit;
	titan->drawSetup.plnwt	= mask;
	titan->drawSetup.ydst  = dsty;
	titan->drawSetup.ar0	= linend;
	titan->drawGo.ar3	= linstart;	/* and start */

	/* do the rest of the image */

	while ((h -= rowsperblit) > 0) {
		dsty += rowsperblit;
		image += (stride * rowsperblit);

		if (rowsperblit > h)
			rowsperblit = h;
		if (rowsperslice > rowsperblit)
			rowsperslice = rowsperblit;

		pwrk = pMga->fbBase;
		linyaddr = (pMga->height * pMga->bstride) +
			   (pMga->ydstorg * pMga->bpp);	 /* reset linyaddr */
		ti = (unsigned char *)image;

		/* wait for drawing engine done */

		WAIT_FOR_DONE(titan);

		titan->drawSetup.plnwt = 0xffffffff;

		/* copy as much of the image as we can into offscreen mem */
		for(th = rowsperblit; th; th -= rowsperslice)
		{
		    titan->srcpage = linyaddr;
		    memcpy(pwrk, ti, stride * rowsperslice);
		    linyaddr += (rowsperslice * pMga->bstride);
		    ti += (rowsperslice * stride);
		}

		/* now blit it */

		titan->drawSetup.plnwt	= mask;
		titan->drawSetup.len	= rowsperblit;
		titan->drawSetup.ydst  = dsty;
		titan->drawSetup.ar0	= linend;
		titan->drawGo.ar3	= linstart;	/* and start */
	}

}

/*
 * mgaDrawMonoImage() - Draw color-expanded monochrome image;
 *			don't draw '0' bits.
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
mgaDrawMonoImage(pbox, image, startx, stride, fg, alu, planemask, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int startx;
	unsigned int stride;
	unsigned long fg;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
	mgaPutMonoImage(pbox, image, startx, stride, fg, 0, alu, planemask,
			pDraw, 1);
}

/*
 * mgaDrawOpaqueMonoImage() - Draw color-expanded monochrome image.
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
mgaDrawOpaqueMonoImage(pbox,image,startx,stride,fg,bg,alu,planemask,pDraw)
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
	mgaPutMonoImage(pbox, image, startx, stride, fg, bg, alu, planemask,
			pDraw, 0);
}
