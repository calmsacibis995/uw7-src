/*
 * @(#) mgaLine.c 11.1 97/10/22
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
 *	S002	Thu Jun  1 16:55:55 PDT 1995	brianm@sco.com
 *		Merged in new code from Matrox.
 *	S001	Wed May 25 16:51:48 PDT 1994	hiramc@sco.COM
 *		Must have a single ZeroSeg routine for Tbird,
 *		for everest, we can use the ZeroSegs
 */

#include "X.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "colormapst.h"

#include "../../nfb/nfbGCStr.h"
#include "../../nfb/nfbDefs.h"
#include "../../nfb/nfbWinStr.h"
#include "../../nfb/nfbScrStr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

#ifdef agaII

mgaSolidZeroSeg( pGC, pDraw, signdx, signdy, axis, x1, y1, e, e1, e2, len )
	GCPtr pGC;
	DrawablePtr pDraw;
	int signdx;
	int signdy;
	int axis;
	int x1;
	int y1;
	int e;
	int e1;
	int e2;
	int len;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	nfbGCPrivPtr pGCPriv;
	int pixop, mask, fgc, alu;
	int sign;

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	pGCPriv = NFB_GC_PRIV(pGC);
	titan = &pMga->regs->titan;

	/* get alu, mask, and fg from GC */

	alu = pGCPriv->rRop.alu;
	mask = pGCPriv->rRop.planemask;
	fgc = pGCPriv->rRop.fg;

	pixop = TITAN_OPCOD_LINE_OPEN | mgaALU[alu] | TITAN_AFOR_FCOL;

	/* set clip and get replicated mask and fg */

	mgaNormalSetup(pMga, &mask, &fgc, NULL);

	MGA_WAIT(titan, 8); /*wait for 8 spots in fifo*/

	titan->drawSetup.shift	= 0; /* no shift */
	titan->drawSetup.dwgctl	= pixop;
	titan->drawSetup.src0	= 0xffffffff; /* no pattern */
	titan->drawSetup.src1	= 0xffffffff;
	titan->drawSetup.src2	= 0xffffffff;
	titan->drawSetup.src3	= 0xffffffff;
	titan->drawSetup.fcol	= fgc;		/* replicated fg */
	titan->drawSetup.plnwt	= mask;		/* replicated plane mask */

	if(axis == X_AXIS)
		sign = 1;	/* sdydx1 */
	else
		sign = 0;

	if(signdx < 0)
		sign |= 2;	/* sdx1 */

	if(signdy < 0)
		sign |= 4;  /* sdy */

	MGA_WAIT(titan, 7);			/*wait for 7 spots in fifo*/

	titan->drawSetup.sgn	= sign;	/* set sign figured out above */
	titan->drawSetup.ar1	= e; /* set error terms */
	titan->drawSetup.ar0	= e1;
	titan->drawSetup.ar2	= e2;
	titan->drawSetup.xdst	= x1; /* starting x */
	titan->drawSetup.ydst	= y1; /* starting y */
	titan->drawGo.len		= len;	/* length and start */

}

#else	/*	agaII	*/

mgaSolidZeroSegs(pGC, pDraw, plines, nlines)
	GCPtr pGC;
	DrawablePtr pDraw;
	BresLinePtr plines;
	int nlines;
{
	register mgaPrivatePtr pMga;
	VOLATILE register mgaTitanRegsPtr titan;
	nfbGCPrivPtr pGCPriv;
	int pixop, mask, fgc, alu;
	int sign;

	pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	pGCPriv = NFB_GC_PRIV(pGC);
	titan = &pMga->regs->titan;

	/* get alu, mask, and fg from GC */

	alu = pGCPriv->rRop.alu;
	mask = pGCPriv->rRop.planemask;
	fgc = pGCPriv->rRop.fg;

	pixop = TITAN_OPCOD_LINE_OPEN | mgaALU[alu] | TITAN_AFOR_FCOL;

	/* set clip and get replicated mask and fg */

	mgaNormalSetup(pMga, &mask, &fgc, NULL);

	MGA_WAIT(titan, 8); /*wait for 8 spots in fifo*/

	titan->drawSetup.shift	= 0; /* no shift */
	titan->drawSetup.dwgctl	= pixop;
	titan->drawSetup.src0	= 0xffffffff; /* no pattern */
	titan->drawSetup.src1	= 0xffffffff;
	titan->drawSetup.src2	= 0xffffffff;
	titan->drawSetup.src3	= 0xffffffff;
	titan->drawSetup.fcol	= fgc;		/* replicated fg */
	titan->drawSetup.plnwt	= mask;		/* replicated plane mask */

	while (nlines--)
	{
	    if(plines->axis == X_AXIS)
		sign = 1;	/* sdydx1 */
	    else
		sign = 0;

	    if(plines->signdx < 0)
		sign |= 2;	/* sdx1 */

	    if(plines->signdy < 0)
		sign |= 4;  /* sdy */

	    MGA_WAIT(titan, 7);			/*wait for 7 spots in fifo*/

	    titan->drawSetup.sgn	= sign;	/* set sign figured out above */
	    titan->drawSetup.ar1	= plines->e; /* set error terms */
	    titan->drawSetup.ar0	= plines->e1;
	    titan->drawSetup.ar2	= plines->e2;
	    titan->drawSetup.xdst	= plines->x1; /* starting x */
	    titan->drawSetup.ydst	= plines->y1; /* starting y */
	    titan->drawGo.len		= plines->len;	/* length and start */
	    plines++;
	}
}
#endif	/*	agaII	*/
