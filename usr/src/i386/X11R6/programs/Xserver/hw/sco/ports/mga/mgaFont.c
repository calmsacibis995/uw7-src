/*
 * @(#) mgaFont.c 11.1 97/10/22
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

#include "X.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

/* download a font into offscreen memory */
/* we can handle two downloaded fonts at a time */
/* index tells us which to load */

void
mgaDownloadFont8(
	unsigned char **bits, 
	int count, 
	int width, 
	int height, 
	int stride,
	int index,
	ScreenRec *pScreen)
{
	mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pScreen);
	VOLATILE register mgaTitanRegsPtr titan = &mgaPriv->regs->titan;
	int linyaddr = mgaPriv->font[index].loc;
	int charsperpage = (7 * 1024) / (4 * 32); /* page size / cell size */
	register int i;
	register unsigned char *dst;
#ifdef DEBUG_PRINT
	ErrorF("mgaDownLoadFont8()\n" );
#endif

	mgaPriv->font[index].stride = stride;

	WAIT_FOR_DONE(titan);	/* must wait for done on direct accesses */

	titan->drawSetup.plnwt = 0xffffffff; /* make sure mask is -1 */
	
	for (i = 0; i < count; ++i)
	{
	    if((i % charsperpage) == 0)	/* begining of 7k page */
	    {
		titan->srcpage = linyaddr; /* set window */
		linyaddr += (7 * 1024); /* bump for next time */
		dst = mgaPriv->fbBase; /* set dst to start of window */
	    }

	    memcpy(dst, bits[i], stride * height); /* move bits to cell */

	    dst += (4 * 32);		/* bump to next cell */
	}
}

/* draw a bunch of chars using one of the two downloaded fonts */
/* index tells us which one */

void
mgaDrawFontText(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	unsigned short glyph_width,
	int index,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw)
{
	int ch, dst_xl, dst_xr, dst_y, glyph_height;
	unsigned long pixop;
	mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pDraw->pScreen);
	VOLATILE register mgaTitanRegsPtr titan = &mgaPriv->regs->titan;
	int linstart, linend;

#ifdef DEBUG_PRINT
	ErrorF("mgaDrawFontText()\n" );
#endif
	pixop = TITAN_OPCOD_BITBLT | mgaALU[alu] | TITAN_BLTMOD_BMONO |
		TITAN_AFOR_FCOL;

	if (transparent)
            pixop |= TITAN_TRANSC_TRANSPARENT; /* set bit if transparent */

	dst_xl = pbox->x1;
	dst_xr = dst_xl + (glyph_width - 1); /* note difference from traps */
	dst_y = pbox->y1;
	glyph_height = pbox->y2 - dst_y;

	/* set clipping, and get replicated mask, fg and bg */
	mgaNormalSetup(mgaPriv, &planemask, &fg, &bg);

	MGA_WAIT(titan, 7);	/* wait for 7 spots in fifo */

	titan->drawSetup.sgn    = 0; /* left to right and top to bottom */
	titan->drawSetup.shift  = 0; /* no shifting of source */
	titan->drawSetup.dwgctl = pixop;
	titan->drawSetup.fcol   = fg; /* replicated fg */
	titan->drawSetup.bcol   = bg; /* replicated bg */
	titan->drawSetup.plnwt  = planemask; /* replicated plane mask */

	/* width of chars */
	titan->drawSetup.ar5    = mgaPriv->font[index].stride * 8;

	/* blits require a linear offset to the source in pixels */
	/* and this depends on the source depth which in this case */
	/* is 1 bit per pixel.  the font locations are stored as a */
	/* byte offset so mult them by 8, then add the char * 32 * 32 */
	/* to find the char start offset in one bit pixels */
	/* the linear end offset is mearly the start + the glyph width - 1 */
	/* blits are different than traps in that the ends, must be */
	/* the start + width - 1 */

	while (count--)	/* while chars to do */
	{
	    ch = *chars++;	/* get the char */

	    /* calc linear start and end */
	    linstart = (mgaPriv->font[index].loc * 8) + (ch * 32 * 32);
	    linend = linstart + (glyph_width - 1);

	    MGA_WAIT(titan, 6);		/* wait for 6 spots in fifo */

	    titan->drawSetup.fxleft  = dst_xl;	/* set left, right, and top */
	    titan->drawSetup.fxright  = dst_xr;
	    titan->drawSetup.ydst  = dst_y;
	    titan->drawSetup.len    = glyph_height;	/* set height */
	    titan->drawSetup.ar0    = linend;		/* source end */
	    titan->drawGo.ar3       = linstart;     /* and start */

	    dst_xl += glyph_width;	/* bump to next dest */
	    dst_xr += glyph_width;
	}
}
