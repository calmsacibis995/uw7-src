/*
 *	@(#) qvisGlyph.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S003    Wed Sep 21 08:10:52 PDT 1994    davidw@sco.com
 *      - Add VOLATILE keyword.
 *      S002    Wed May 12 10:38:09 PDT 1993    davidw@sco.com
 *      - Removed width/height/planemask/alu == 0 checks. Now done in NFB.
 *	S001	Mon Jan 18 13:25:33 PST 1993	hiramc@sco.COM
 *	- Missed one byte to swap in the case 3, two lines of code missing
 *	- This showed up as errors in fonts with three bytes as stride
 *	S000	Tue Oct 06 21:57:20 PDT 1992	mikep@sco.com
 *	- Add BIT_SWAP macros
 *
 */
/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  Originated (see RCS log)
 *
 */

#include "xyz.h"
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "misc.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"

#include "qvisHW.h"
#include "qvisDefs.h"
#include "qvisMacros.h"
#include "qvisProcs.h"

void
qvisUncachedDrawMonoGlyphs(
			     nfbGlyphInfo * glyph_info,
			     unsigned int nglyphs,
			     unsigned long fg,
			     unsigned char alu,
			     unsigned long planemask,
			     unsigned long font_private,	/* unused parameter */
			     DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    BoxPtr          pbox;
    unsigned char  *image;
    unsigned int    stride;
    register int    width;
    register int    height;
    int             widthinbytes;
    int             x1, x2;
    int             y1, y2;
    int             last_height = -1;	/* bogus value of shadow for
					 * HEIGHT_REG */
    int             last_width = -1;	/* bogus value of shadow for
					 * WIDTH_REG */
    int             last_y = -1;/* bogus value of shadow for BREG_Y */
    unsigned int    i;

    XYZ("qvisUncachedDrawMonoGlyphs-entered");
    qvisSetCurrentScreen();

    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPlanarMode(TRANSPARENT_WRITE);
    qvisSetALU(alu);
    qvisSetPlaneMask((unsigned char) planemask);
    qvisSetForegroundColor((unsigned char) fg);

    /* this reg should always be zero for this routine */
    qvisOut16(X0_BREG, 0x00);
    /* set DAF to X-Y */
    qvisOut8(BLT_CMD1, 0x80);

    XYZdelta("qvisUncachedDrawMonoGlyphs-nglyphs", nglyphs);
    for (i = 0; i < nglyphs; ++i, ++glyph_info) {

	pbox = &glyph_info->box;
	y1 = pbox->y1;
	y2 = pbox->y2;
	x1 = pbox->x1;
	x2 = pbox->x2;
	height = pbox->y2 - pbox->y1;
	width = pbox->x2 - pbox->x1;

	image = glyph_info->image;
	stride = glyph_info->stride;

#ifdef XYZEXT
	    if (stride == 1)
		XYZ("qvisUncachedDrawMonoGlyphs-stride==1");
	    if (stride == 2)
		XYZ("qvisUncachedDrawMonoGlyphs-stride==2");
	    if (stride == 3)
		XYZ("qvisUncachedDrawMonoGlyphs-stride==3");	/* unlikely! */
	    if (stride == 4)
		XYZ("qvisUncachedDrawMonoGlyphs-stride==4");
	    if (stride > 4)
		XYZ("qvisUncachedDrawMonoGlyphs-stride>4");
    #endif

	    /* Check engines */
	    qvisWaitForGlobalNotBusy();

	    /*
	     * NOTE: in the code below, we only set HEIGHT_REG,
	     * WIDTH_REG, and Y1_BREG if it is different from
	     * the last thing we set it to.  This tries to eliminate
	     * unnecessary outw's.
	     *
	     * It is not done for X1_BREG since that will clearly
	     * be changing as characters are written across
	     * a horizontal row!
	     */

	    /* load up height and width of CPU-to-screen blit */
	    if (height != last_height) {
		XYZ("qvisUncachedDrawMonoGlyphs-OutHeight");
		qvisOut16(HEIGHT_REG, (short) height);
		last_height = height;
	    } else {
		XYZ("qvisUncachedDrawMonoGlyphs-ShadowedHeightPaysOff");
	    }
	    if (width != last_width) {
		XYZ("qvisUncachedDrawMonoGlyphs-OutWidth");
		qvisOut16(WIDTH_REG, (short) width);
		last_width = width;
	    } else {
		XYZ("qvisUncachedDrawMonoGlyphs-ShadowedWidthPaysOff");
	    }

	    /* load up x and y of CPU-to-screen blit */
	    qvisOut16(X1_BREG, (short) x1); /* not remembered */
	    if (y1 != last_y) {
		XYZ("qvisUncachedDrawMonoGlyphs-Out_Y");
		qvisOut16(Y1_BREG, (short) y1);
		last_y = y1;
	    } else {
		XYZ("qvisUncachedDrawMonoGlyphs-Shadowed_Y_PaysOff");
	    }

	    qvisOut8(BLT_CMD0, 0x01);	/* turn on the engine */

	    /*
	     * We special case out strides of 1 and 2 bytes so we can do the
	     * blit by just iterating over height - instead of both height
	     * and width.  This gives a 25-50% speedup according to x11perf.
	     * -mjk
	     */
	    switch (stride) {
	case 1:
	    XYZ("qvisUncachedDrawMonoGlyphs-QuickStride1");
	    do {
		*fb_ptr = BIT_SWAP(*image);			/* S000 */
		image++;
		height--;
	    } while (height > 0);
	    break;
	case 2:
	    XYZ("qvisUncachedDrawMonoGlyphs-QuickStride2");
	    do {
#if BITMAP_BIT_ORDER == MSBFirst				/* S000 */
		*((unsigned short *) fb_ptr) = *((unsigned short *) image);
		image += 2;
#else								/* S000 */
		*fb_ptr = BIT_SWAP(*image++);		/* S000 */
		*fb_ptr = BIT_SWAP(*image++);		/* S000 */
#endif								/* S000 */
		height--;
	    } while (height > 0);
	    break;
	case 3:
	    XYZ("qvisUncachedDrawMonoGlyphs-QuickStride3");
	    do {
#if BITMAP_BIT_ORDER == MSBFirst				/* S000 */
		*((unsigned short *) fb_ptr) = *((unsigned short *) image);
		image += 2;
		*fb_ptr = *image;				/* S001 */
#else								/* S000 */
		*fb_ptr = BIT_SWAP(*image++);		/* S000 */
		*fb_ptr = BIT_SWAP(*image++);		/* S000 */
		*fb_ptr = BIT_SWAP(*image);			/* S001 */
#endif								/* S000 */
		image++;
		height--;
	    } while (height > 0);
	    break;
	default:
	    XYZ("qvisUncachedDrawMonGlyphs-SlowStride");

	    /* calculate number of bytes for given width */
	    widthinbytes = ((width - 1) >> 3) + 1;

	    /**
	     * Another way to compute widthinbytes would be as follows:
	     *
	     * widthinbytes = width >> 3;
	     * if (width & 7) {
	     *    widthinbytes++;
	     *  }
	     *
	     * My testing shows this method is 18% slower than just
	     * using the above formula.  markl used the conditional
	     * version before; I changed it. -mjk
	     */

	    do {		/* most happen at least once since height > 0 */
		unsigned char  *pimage = image;
		int             bytesleft = widthinbytes;

		/*
		 * This while loop probably could be a do loop. -mjk
		 */
		bytesleft = widthinbytes;
		do {
		    *fb_ptr = BIT_SWAP(*pimage);		/* S000 */
		    pimage++;
		    bytesleft--;
		} while (bytesleft > 0);
		image += stride;
		height--;
	    } while (height > 0);
	    break;
	}
    }				/* end for each character */

    qvisPriv->engine_used = TRUE;
}

#ifdef TEST_CASE

/*
 * XXX qvisSSBlitDrawMonoGlyphs is a DrawMonoGlyphs replacement routine
 * which exposes a hardware bug (I think -mjk) in the Q-Vision Blit
 * engine.  What this routine does is draws the glyph to be draw
 * off-screen so it can use a linear to X-Y screen to screen blit
 * to draw the character on the screen (the same way qvisCachedDrawMonoGlyphs
 * does).  After that blit, it does a X-Y to X-Y screen to screen
 * blit of the character just drawn to (10,10) on the screen.
 * This blit will be drawn with a "panhandle" on the right side
 * on the top scan line.
 *
 * It appears the blit use to draw the screen originally put the
 * blit engine in a bad state.  The next X-Y to X-Y screen to screen
 * blit gets messed up.
 *
 * SEE: qvisPriv->bad_blit_state
 */
void
qvisSSBlitDrawMonoGlyphs(
			   nfbGlyphInfo * glyph_info,
			   unsigned int nglyphs,
			   unsigned long fg,
			   unsigned char alu,
			   unsigned long planemask,
			   unsigned long font_private,	/* unused parameter */
			   DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    unsigned char  *fb_base = qvisPriv->fb_base;
    unsigned char  *fb_line;
    unsigned char  *fb_ptr;
    BoxPtr          pbox;
    int             x1, y1, x2, y2;
    int             height, width;
    unsigned int    stride;
    unsigned int    widthinbytes;
    unsigned char  *image;
    unsigned char  *pline;
    unsigned char  *pimage;
    int             i, j, k;

    XYZ("qvisSSBlitDrawMonoGlyphs-entered");
    qvisSetCurrentScreen();
    /*
     * XXX This routine does follow "waiting" rules in qvisMacros.h - it waits
     * TOO MUCH just to make sure!
     */
    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    qvisSetForegroundColor((unsigned char) fg);
    qvisSetALU(alu);
    qvisSetPlaneMask((unsigned char) planemask);

    for (i = 0; i < nglyphs; i++, glyph_info++) {

	qvisSetPackedMode(0x4);
	image = glyph_info->image;
	stride = glyph_info->stride;
	pbox = &glyph_info->box;
	y1 = pbox->y1;
	y2 = pbox->y2;
	x1 = pbox->x1;
	x2 = pbox->x2;
	height = y2 - y1;
	width = x2 - x1;
	widthinbytes = ((width - 1) >> 3) + 1;

	fb_line = fb_base + (768 * 1024);
	pline = glyph_info->image;
	for (j = 0; j < height; j++, pline += stride, fb_line += 4) {
	    pimage = pline;
	    fb_ptr = fb_line;
	    for (k = 0; k < widthinbytes; k++) {
		*fb_ptr++ = *pimage++;
	    }
	}

	qvisWaitForGlobalNotBusy();

	qvisSetPlanarMode(0x51);
	qvisOut8(BLT_CMD1, 0x80);

	qvisOut16(Y0_BREG, (768 << 13) >> 16);
	qvisOut16(X0_BREG, (768 << 13) & 0xffff);

	qvisOut16(HEIGHT_REG, height);
	qvisOut16(WIDTH_REG, width);
	qvisOut16(X1_BREG, x1);
	qvisOut16(Y1_BREG, y1);
	qvisOut16(SOURCE_PITCH_REG, 1);

	/* turn on the engine */
	qvisOut8(BLT_CMD0, 0x1);

	qvisWaitForGlobalNotBusy();

#define DEBUG_BLIT_OFF_SCREEN_CHAR_TO_SCREEN
#ifdef DEBUG_BLIT_OFF_SCREEN_CHAR_TO_SCREEN

#ifdef DEBUG_RESET_BLIT_ENGINE
	/*
	 * Reset the state of the blit engine so the next screen-to-screen
	 * blit will work.
	 */
	qvisOut8(GC_INDEX, 0x10);
	qvisOut8(GC_DATA, 0x40);
	qvisOut8(GC_DATA, 0x28);
	qvisInvalidateShadows(qvisPriv);
	qvisWaitForGlobalNotBusy();
#endif				/* DEBUG_RESET_BLIT_ENGINE */

	/*
	 * Copy the character we just drew on-screen using a screen to screen
	 * blit to location (10,10) on the screen.
	 */
	qvisSetPackedMode(0x05);
	qvisOut16(HEIGHT_REG, height);
	qvisOut16(WIDTH_REG, width);
	qvisOut16(X0_BREG, x1);
	qvisOut16(Y0_BREG, y1);
	qvisOut16(X1_BREG, 10);
	qvisOut16(Y1_BREG, 10);
	qvisOut8(BLT_CMD1, 0xc0);
	qvisOut8(BLT_CMD0, 0x1);

	qvisWaitForGlobalNotBusy();
#endif				/* DEBUG_BLIT_OFF_SCREEN_CHAR_TO_SCREEN */
    }

    /*
     * Reset the blit engine AND flush the shadowed values.  I don't know why
     * this has to be done but the blit engine will do the next packed
     * screen-to-screen blit wrong if this reset is not done.  Hardware bug?
     * -mjk
     */
    qvisOut8(GC_INDEX, 0x10);
    qvisOut8(GC_DATA, 0x40);
    qvisOut8(GC_DATA, 0x28);
#ifdef QVIS_SHADOWED
    qvisInvalidateShadows(qvisPriv);
#endif

    qvisPriv->engine_used = TRUE;
}

#endif				/* TEST_CASE */
