/*
 * @(#) mgaFillSp.c 11.1 97/10/22
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
 *	SCO Modifications
 *
 *	S002 Thu Jun  1 16:54:34 PDT 1995	brianm@sco.com
 *		added in new code from Matrox.
 *	S001 Tue Nov  1 15:14:02 PST 1994	brianm@sco.com
 *		non-closed comment field
 */

#include "X.h"
#include "Xmd.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"

#include "../../nfb/nfbGCStr.h"
#include "../../nfb/nfbDefs.h"
#include "../../nfb/nfbWinStr.h"
#include "../../nfb/nfbScrStr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

void
mgaSolidFS(pGC, pDraw, ppt, pwidth, n)
	GCPtr pGC;
	DrawablePtr pDraw;
	register DDXPointPtr ppt;
	register unsigned int *pwidth;
	unsigned int n;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int pixop, mask, fgc, alu;
#ifdef DEBUG_PRINT
	ErrorF("mgaSolidFS()\n" );
#endif

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

	alu = pGCPriv->rRop.alu;
	mask = pGCPriv->rRop.planemask;
	fgc = pGCPriv->rRop.fg;

	pixop = TITAN_OPCOD_TRAP | mgaALU[alu] | TITAN_AFOR_FCOL |
		TITAN_TRANSC_TRANSPARENT;

	/* set clipping, and get replicated mask and fg color */

	mgaNormalSetup(pMga, &mask, &fgc, NULL);

	MGA_WAIT(titan, 19); /* wait for 19 spots in fifo */

	/* only one horizontal line, so set trap for virtical sides */
	titan->drawSetup.ar0	= 0;
	titan->drawSetup.ar6	= 0;
	titan->drawSetup.ar1	= 0;
	titan->drawSetup.ar4	= 0;
	titan->drawSetup.ar2	= 0;
	titan->drawSetup.ar5	= 0;
	titan->drawSetup.shift	= 0; /* no shift */
	titan->drawSetup.sgn	= 0; /* left to right */

	titan->drawSetup.dwgctl	= pixop;

	/* solid fill pattern */
	titan->drawSetup.src0	= 0xffffffff;
	titan->drawSetup.src1	= 0xffffffff;
	titan->drawSetup.src2	= 0xffffffff;
	titan->drawSetup.src3	= 0xffffffff;

	titan->drawSetup.fcol	= fgc;	/* replicated forground color */
	titan->drawSetup.plnwt	= mask;	/* replicated plane mask */

	titan->drawSetup.fxleft = ppt->x;	/* left and right x */
	titan->drawSetup.fxright = ppt->x + *pwidth;
	titan->drawSetup.ydst = ppt->y;		/* top y */
	titan->drawGo.len = 1;			/* height and start */

	while (--n > 0) {
		
		ppt++;
		pwidth++;

		MGA_WAIT(titan, 4);	/* wait for 4 spots in fifo */

		titan->drawSetup.fxleft = ppt->x;
		titan->drawSetup.fxright = ppt->x + *pwidth;
		titan->drawSetup.ydst = ppt->y;
		titan->drawGo.len = 1;
	}
}

static void
mgaDoStippledFS(pGC, pDraw, ppt, pwidth, n, trans)
	GCPtr pGC;
	DrawablePtr pDraw;
	register DDXPointPtr ppt;
	register unsigned int *pwidth;
	unsigned int n;
	int trans;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int pixop, mask, fgc, bgc, alu;
	int xoff, yoff;
	int stipw, stiph;
	unsigned char *src;
	int i, stride;
	PixmapPtr pm;
#ifdef DEBUG_PRINT
	ErrorF("mgaDoStippleFS()\n" );
#endif
	
	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	titan = &pMga->regs->titan;

	pm = pGC->stipple;

	stipw = pm->drawable.width;
	stiph = pm->drawable.height;
	stride = pm->devKind;
	src = pm->devPrivate.ptr;


	/* use gen code if load fails */

	if(mgaLoadStipple(titan, stipw, stiph, stride, src))
	{
	    if(trans)
		genStippledFS(pGC, pDraw, ppt, pwidth, n);
	    else
		genOpStippledFS(pGC, pDraw, ppt, pwidth, n);
	    return;
	}

	alu = pGCPriv->rRop.alu;
	mask = pGCPriv->rRop.planemask;
	fgc = pGCPriv->rRop.fg;
	bgc = pGCPriv->rRop.bg;

	pixop = TITAN_OPCOD_TRAP | mgaALU[alu] | TITAN_AFOR_FCOL;
	if(trans)
	    pixop |= TITAN_TRANSC_TRANSPARENT; /* set bit if transparent */

	xoff = stipw - (pGCPriv->screenPatOrg.x % stipw);
	yoff = stiph - (pGCPriv->screenPatOrg.y % stiph);

	/* set clip, and get replicated mask, fg, and bg */
	mgaNormalSetup(pMga, &mask, &fgc, &bgc);

	MGA_WAIT(titan, 16); /*wait for 16 spots in fifo*/

	/* trap with virtical sides */
	titan->drawSetup.ar0	= 0;
	titan->drawSetup.ar6	= 0;
	titan->drawSetup.ar1	= 0;
	titan->drawSetup.ar4	= 0;
	titan->drawSetup.ar2	= 0;
	titan->drawSetup.ar5	= 0;

	titan->drawSetup.shift	= xoff | (yoff << 4); /* set offsets */
	titan->drawSetup.sgn	= 0;	/* left to right and top to bottom */

	titan->drawSetup.dwgctl	= pixop;

	titan->drawSetup.fcol	= fgc;	/* replicated forground */
	titan->drawSetup.bcol	= bgc;	/* replicated background */
	titan->drawSetup.plnwt	= mask;	/* replicated plane mask */

	titan->drawSetup.fxleft = ppt->x; /* left and right x S001 */
	titan->drawSetup.fxright = ppt->x + *pwidth;
	titan->drawSetup.ydst = ppt->y;	/* top y */
	titan->drawGo.len = 1;			/* one line and start */

	while (--n > 0) {
		
		ppt++;
		pwidth++;

		MGA_WAIT(titan, 4);

		titan->drawSetup.fxleft = ppt->x;
		titan->drawSetup.fxright = ppt->x + *pwidth;
		titan->drawSetup.ydst = ppt->y;
		titan->drawGo.len = 1;
	}
}

/* use above routine for both opaque and transparent */

void
mgaStippledFS(pGC, pDraw, ppt, pwidth, n)
	GCPtr pGC;
	DrawablePtr pDraw;
	register DDXPointPtr ppt;
	register unsigned int *pwidth;
	unsigned int n;
{
	mgaDoStippledFS(pGC, pDraw, ppt, pwidth, n, 1);
}

void
mgaOpStippledFS(pGC, pDraw, ppt, pwidth, n)
	GCPtr pGC;
	DrawablePtr pDraw;
	register DDXPointPtr ppt;
	register unsigned int *pwidth;
	unsigned int n;
{
	mgaDoStippledFS(pGC, pDraw, ppt, pwidth, n, 0);
}

