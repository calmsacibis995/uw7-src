/*
 * @(#) mgaFillRct.c 11.1 97/10/22
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
 *	S000	Thu Jun  1 16:52:52 PDT 1995	brianm@sco.com
 *	- New code from Matrox.
 */

/*
 * GC rectangle ops for mga
 */
#include "X.h"
#include "Xmd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "../../nfb/nfbGCStr.h"
#include "../../nfb/nfbDefs.h"
#include "../../nfb/nfbWinStr.h"
#include "../../nfb/nfbScrStr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

void
mgaSolidFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	register BoxPtr pbox;
	register unsigned int nbox;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int pixop, mask, fgc;
	int h, alu;

#ifdef DEBUG_PRINT
	ErrorF("mgaSolidFillRects\n" );
#endif
	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

        alu = pGCPriv->rRop.alu;
        mask = pGCPriv->rRop.planemask;
        fgc = pGCPriv->rRop.fg;

	h = (pbox->y2 - pbox->y1);

	pixop = TITAN_OPCOD_TRAP | mgaALU[alu] | TITAN_AFOR_FCOL |
		TITAN_TRANSC_TRANSPARENT;

	/* set clipping, and replicate bits of mask, and foreground clolor */

	mgaNormalSetup(pMga, &mask, &fgc, NULL);

	MGA_WAIT(titan, 19); /*wait for 19 spots in fifo*/

	/* trapazoid setup for rectangle (virtical left and righe edges */
	titan->drawSetup.ar0	= 0;
	titan->drawSetup.ar6	= 0;
	titan->drawSetup.ar1	= 0;
	titan->drawSetup.ar4	= 0;
	titan->drawSetup.ar2	= 0;
	titan->drawSetup.ar5	= 0;
	titan->drawSetup.shift	= 0; /* no sift of source */
	titan->drawSetup.sgn	= 0; /* scan left to right */

	titan->drawSetup.dwgctl	= pixop;

	/* solid stipple pattern */
	titan->drawSetup.src0	= 0xffffffff;
	titan->drawSetup.src1	= 0xffffffff;
	titan->drawSetup.src2	= 0xffffffff;
	titan->drawSetup.src3	= 0xffffffff;

	titan->drawSetup.fcol	= fgc; /* replicated foreground color */
	titan->drawSetup.plnwt	= mask; /* replicated plane mask */

	/* trap functions require end of start + width */
	/* as opposed to blits which require start + (width - 1) */

	titan->drawSetup.fxleft = pbox->x1;	/* left start */
	titan->drawSetup.fxright = pbox->x2;	/* right end */
	titan->drawSetup.ydst = pbox->y1;	/* top start */
	titan->drawGo.len = h;			/* and start */

	while (--nbox > 0) {
		pbox++;				/* bump to next box */
		h = (pbox->y2 - pbox->y1);	/* calc height */

		MGA_WAIT(titan, 4);		/* wait for 4 slots in fifo */

		titan->drawSetup.fxleft = pbox->x1;
		titan->drawSetup.fxright = pbox->x2;
		titan->drawSetup.ydst = pbox->y1;
		titan->drawGo.len = h;
	}
}

/* load a stipple pattern into the pattern registers */

int mgaLoadStipple(VOLATILE register mgaTitanRegsPtr titan,int stipw, int stiph,
			      int stride, unsigned char *src)
{
	unsigned char pat[16];
	register unsigned short *p, *s;
	register unsigned short mask;
	unsigned long *lpat;
	int i, j;

#ifdef DEBUG_PRINT
	ErrorF("mgaLoadStipple()\n" );
#endif
	/* we can only stipple if width <= 16 and height <= 8 */
	/* and width and height are powers of 2 */
	/* and if the stride is correct for the width */

	if((stipw & (stipw - 1)) || (stipw > 16) ||
	   (stiph & (stiph - 1)) || (stiph > 8) ||
	   ((stipw == 16) && (stride != 2)) ||
	   ((stipw <= 8) && (stride != 1)))
		return 1;

	/* check to see if we need to replicate in either direction */

	if(stipw == 16)		/* no rep needed by x */
	{
	    if(stiph == 8)	/* no rep needed by y */
		lpat = (unsigned long *)src;	/* just use the bits */
	    else				/* else replicate by y */
	    {
		p = (unsigned short *)pat;
		s = (unsigned short *)src;

		for(i = 0; i < 8; ++i)
		    p[i] = s[i % stiph];

		lpat = (unsigned long *)pat;
	    }
	}
	else			/* rep needed by x */
	{
	    mask = (1 << stipw) - 1;
	    p = (unsigned short *)pat;
	    
	    for(i = 0; i < 8; ++i)	/* poss, rep by y at same time */
	    {
		p[i] = 0;
		for(j = 0; j < (16 / stipw); ++j)
		{
		    p[i] |= ((unsigned short)((src[i % stiph] & mask))
			    << (j * stipw));
		}
	    }
	    lpat = (unsigned long *)pat;
	}

	MGA_WAIT(titan, 4);	/* wait for 4 spots in fifo, then load */

	titan->drawSetup.src0	= lpat[0];
	titan->drawSetup.src1	= lpat[1];
	titan->drawSetup.src2	= lpat[2];
	titan->drawSetup.src3	= lpat[3];

	return 0;
}

/* function to do the work for stippled rectangles, either transparent or */
/* opaque */

void
mgaDoStippledFillRects(pGC, pDraw, pbox, nbox, trans)
	GCPtr pGC;
	DrawablePtr pDraw;
	register BoxPtr pbox;
	register unsigned int nbox;
	int trans;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int pixop, mask, fgc, bgc, alu;
	int xoff, yoff, width;
	int stipw, stiph;
	unsigned char *src;
	int i, stride;
	PixmapPtr pm;
	unsigned long *lpat;
	
#ifdef DEBUG_PRINT
	ErrorF("mgaDoStippleFillRects()\n" );
#endif
	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
	stride = pm->devKind;
	src = pm->devPrivate.ptr;

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

	/* use gen code if the load fails */

	if(mgaLoadStipple(titan, stipw, stiph, stride, src))
	{
	    if(trans)
		genStippledFillRects(pGC, pDraw, pbox, nbox);
	    else
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
	    return;
	}

	alu = pGCPriv->rRop.alu;
	mask = pGCPriv->rRop.planemask;
	fgc = pGCPriv->rRop.fg;
	bgc = pGCPriv->rRop.bg;
	pixop = TITAN_OPCOD_TRAP | mgaALU[alu] | TITAN_AFOR_FCOL;
	if(trans)
	    pixop |= TITAN_TRANSC_TRANSPARENT; /* if transparent set the bit */

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw); /* calc offsets */
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

	width = pbox->x2 - pbox->x1;

	/* set clipping and get replicated mask fg, and bg */

	mgaNormalSetup(pMga, &mask, &fgc, &bgc);

	MGA_WAIT(titan, 20); /*wait for 20 spots in fifo*/

	/* rectangle is a trapazoid with virtical sides */

	titan->drawSetup.ar0	= 0;
	titan->drawSetup.ar6	= 0;
	titan->drawSetup.ar1	= 0;
	titan->drawSetup.ar4	= 0;
	titan->drawSetup.ar2	= 0;
	titan->drawSetup.ar5	= 0;

	/* setup the shift to handle the offsets */
	titan->drawSetup.shift	= xoff | (yoff << 4);
	titan->drawSetup.sgn	= 0;			/* left to right */

	titan->drawSetup.dwgctl	= pixop;

	titan->drawSetup.fcol	= fgc;		/* replicated forground */
	titan->drawSetup.bcol	= bgc;		/* replicated background */
	titan->drawSetup.plnwt	= mask;		/* replicated mask */

	titan->drawSetup.fxleft = pbox->x1;	/* set start and end x */
	titan->drawSetup.fxright = pbox->x2;
	titan->drawSetup.ydst = pbox->y1;	/* and start y */
	titan->drawGo.len = pbox->y2 - pbox->y1; /* set height and start */

	while (--nbox > 0) {
		
		pbox++;

		MGA_WAIT(titan, 4);		/* wait for 4 spots in fifo */

		titan->drawSetup.fxleft = pbox->x1;
		titan->drawSetup.fxright = pbox->x2;
		titan->drawSetup.ydst = pbox->y1;
		titan->drawGo.len = pbox->y2 - pbox->y1;	/* and start */
	}
}

/* use above routine to do both opaque and transparent */

void
mgaStippledFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	register BoxPtr pbox;
	register unsigned int nbox;
{
	mgaDoStippledFillRects(pGC, pDraw, pbox, nbox, 1);
}

void
mgaOpStippledFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	register BoxPtr pbox;
	register unsigned int nbox;
{
	mgaDoStippledFillRects(pGC, pDraw, pbox, nbox, 0);
}

