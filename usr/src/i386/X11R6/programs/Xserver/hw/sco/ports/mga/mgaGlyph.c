/*
 * @(#) mgaGlyph.c 11.1 97/10/22
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
 * mgaGlyph.c
 *
 * mga glyph drawing ops.
 */

#include "X.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "../../nfb/nfbDefs.h"
#include "../../nfb/nfbWinStr.h"
#include "../../nfb/nfbScrStr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

/* expand 1 bit glyphs to true depth */
/* we do this by loading the glyp into offscreen memory, and then */
/* blitting them to the right place */

void
mgaDrawMonoGlyphs(glinfo, nglyphs, fg, alu, planemask, pPS, pDraw)
	register nfbGlyphInfo *glinfo;
	unsigned int nglyphs;
	unsigned long fg;
	unsigned char alu;
	unsigned long planemask;
	nfbFontPSPtr pPS;
	DrawablePtr pDraw;
{
	register mgaPrivatePtr pMga = MGA_PRIVATE_DATA(pDraw->pScreen);
	VOLATILE register mgaTitanRegsPtr titan = &pMga->regs->titan;
	int pixop, mask, fgc;
	int srcx, srcy, dsty, xleft, xright, sign, sypitch, linyaddr;
	int w, h, bytes, linstart, linend;
	unsigned char *pwrk;
	int doinit = 1;

#ifdef DEBUG_PRINT
	ErrorF("mgaDrawMonoGlyphs()\n" );
#endif
	titan->srcpage = (pMga->height * pMga->bstride) +
			 (pMga->ydstorg * pMga->bpp);

	pixop = TITAN_OPCOD_BITBLT | mgaALU[alu] | TITAN_BLTMOD_BMONO |
		TITAN_AFOR_FCOL | TITAN_TRANSC_TRANSPARENT;

	srcx = 0;
	srcy = pMga->height;
	pwrk = pMga->fbBase;

	/* blits require a linear offset to the source in pixels */
	/* and this depends on the source depth which in this case */
	/* is 1 bit per pixel. we load each glyph into offscreen mem */
	/* at the first offscreen location  so the linear source is */
	/* (the screen hight * the screen stride * the screen depth) + */
	/* (the screen ydstorg (256 if screen width is 1280) * */
	/* the screen depth) */

	linstart = (srcy * pMga->pstride * pMga->depth) +
		   (pMga->ydstorg * pMga->depth);

	while (nglyphs-- > 0)	/* while glyphs to do */
	{
		w = glinfo->box.x2 - glinfo->box.x1;
		h = glinfo->box.y2 - glinfo->box.y1;
		bytes = glinfo->stride * h;

		if (bytes > (7 * 1024))    /* if bytes is > a window size */
		{			   /* just use drawMonoImage */
			mgaDrawMonoImage(glinfo->box, glinfo->image, 0,
				glinfo->stride, fg, alu, planemask, pDraw);
			doinit = 1;
			continue;
		}

		sypitch = glinfo->stride * 8; /* the source pitch in pixels */
		xleft = glinfo->box.x1;
		xright = glinfo->box.x2 - 1;
		dsty = glinfo->box.y1;
		linend = linstart + (w - 1); /*linear end is start + width - 1*/

		WAIT_FOR_DONE(titan);		/* must wait for done on all */
						/* direct accesses */

		titan->drawSetup.plnwt = 0xffffffff; /* make sure mask is -1 */
		memcpy(pwrk, glinfo->image, bytes);  /* copy the bits */

		if (doinit)
		{
			doinit = 0;
			mask = planemask;
			fgc = fg;
			/* set clipping, and replicate bits in mask and fg */
			mgaNormalSetup(pMga, &mask, &fgc, NULL);
			titan->drawSetup.sgn	= 0; /* l to r, and t to b */
			titan->drawSetup.shift	= 0; /* no shift */
			titan->drawSetup.dwgctl	= pixop;
			titan->drawSetup.fcol	= fgc; /* reped fg */
		}

		titan->drawSetup.plnwt	= mask;	/* replicated mask */
		titan->drawSetup.fxleft  = xleft;
		titan->drawSetup.fxright  = xright;
		titan->drawSetup.ydst  = dsty;
		titan->drawSetup.ar5	= sypitch;
		titan->drawSetup.len	= h;
		titan->drawSetup.ar0	= linend;
		titan->drawGo.ar3	= linstart;	/* and start */

		++glinfo;
	}
}
