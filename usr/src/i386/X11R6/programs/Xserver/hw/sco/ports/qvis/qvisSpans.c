/*
 *	@(#) qvisSpans.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Fri Oct 16 17:21:09 PDT 1992	mikep@sco.com
 *	- Add bit swapping here.
 *	- Cache the GC
 *	S001	Thu Nov 05 16:40:56 PST 1992	mikep@sco.com
 *	- Change NFB_SERIAL_NUMBER macro.
 *	S002	Sat Dec 12 12:38:28 PST 1992	mikep@sco.com
 *	- GC Caching won't work unless you invalidate the current_gc
 *	everytime you change the hardware.  This needs a little more
 *	thought.
 *      S003    Tue Jul 13 12:50:49 PDT 1993    davidw@sco.com
 *      - compaq waltc AGA EFS 2.0 source handoff 07/02/93
 *      S004    Wed Sep 21 08:25:33 PDT 1994    davidw@sco.com
 *	- Correct compiler warnings.
 *      - Add VOLATILE keyword.
 *
 */

/**
 *
 * qvisSpans.c
 *
 * Copyright 1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mjk       04/07/92  Originated (see RCS log)
 * waltc     06/26/93  Substitute qvisPriv->pitch2 for hard-coded value.
 */

#include "xyz.h"
#include "X.h"
#include "Xmd.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "servermd.h"

#include "scoext.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

/*
 * Notice that we do rely on macros that are from the monochrome frame
 * buffer code.  These macros should be quite stable though.
 *
 * Used macros: maskbits & maskpartialbits
 *
 * See mfb/maskbits.h for descriptions of these macros.
 */
#include "../../mfb/maskbits.h"

#include "qvisHW.h"
#include "qvisMacros.h"
#include "qvisDefs.h"
#include "qvisProcs.h"

#if BITMAP_BIT_ORDER == LSBFirst		/* S000 vvv */ /* S004 */
#define SWAP_DWORD(dword) \
    { \
    register unsigned char *spw = (unsigned char *)&dword; \
    spw[0]=BIT_SWAP(spw[0]); \
    spw[1]=BIT_SWAP(spw[1]); \
    spw[2]=BIT_SWAP(spw[2]); \
    spw[3]=BIT_SWAP(spw[3]); \
    }
#else
#define SWAP_DWORD(dword)  /* as nothing */
#endif								/* S000 ^^^ */
	
/*
 * NOTE: supplying our own SolidFillSpans routine is a BIG win over using the
 * gen version of this routine.  We are talking 2-4x performance. -mjk
 */

/*
 * Since this is such a nice win in many of the x11perf tests, it would
 * be nice if a SolidFillSpans routine was written for the banked
 * mode so ISA implementation could benefit.
 */

#if 0				/* use the planar version instead! */

/*
 * qvisSolidFillSpans - packed version.
 */
void
qvisSolidFillSpans(
		     GCPtr pGC,
		     DrawablePtr pDraw,
		     DDXPointPtr ppt,
		     unsigned int *pwidth,
		     unsigned int n)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    nfbGCPrivPtr    gcPriv = NFB_GC_PRIV(pGC);
    int             x;
    int             y;
    int             width;
    unsigned int    fg;
    int             i;
    unsigned char  *pntplace;

    XYZ("qvisSolidFillSpans-entered");
    qvisSetCurrentScreen();
    XYZdelta("qvisSolidFillSpans-n", n);

    if (n == 0) {
	/*
	 * Bail out early if there is nothing to draw.  This is important
	 * because in the loop below we use a do-while loop instead of
	 * just a while loop.  The n==0 case is NOT handled by the
	 * do-while loop below (so don't remove this bail out).
	 */
	return;
    }
    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(ROPSELECT_ALL);
    qvisSetALU(gcPriv->hwRop.alu);
    qvisSetPlaneMask((unsigned char) gcPriv->hwRop.planemask);
    fg = gcPriv->hwRop.fg;
    fg = fg << 8 | fg;
    fg = fg << 16 | fg;

    do {
	x = ppt->x;
	y = ppt->y;
	width = *pwidth;
	pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
	while (width > 7) {
	    XYZ("qvisSolidFillSpans-8X");
	    *((unsigned int *) pntplace) = fg;
	    pntplace += sizeof(unsigned int);
	    *((unsigned int *) pntplace) = fg;
	    pntplace += sizeof(unsigned int);
	    width -= 8;
	}
	switch (width) {
	case 7:
	    XYZ("qvisSolidFillSpans-7");
	    *((unsigned int *) pntplace) = fg;
	    pntplace += sizeof(unsigned int);
	    *((unsigned short *) pntplace) = fg;
	    pntplace += sizeof(unsigned short);
	    *((unsigned char *) pntplace) = fg;
	    pntplace += sizeof(unsigned char);
	    break;
	case 6:
	    XYZ("qvisSolidFillSpans-6");
	    *((unsigned int *) pntplace) = fg;
	    pntplace += sizeof(unsigned int);
	    *((unsigned short *) pntplace) = fg;
	    pntplace += sizeof(unsigned short);
	    break;
	case 5:
	    XYZ("qvisSolidFillSpans-5");
	    *((unsigned int *) pntplace) = fg;
	    pntplace += sizeof(unsigned int);
	    *((unsigned char *) pntplace) = fg;
	    pntplace += sizeof(unsigned char);
	    break;
	case 4:
	    XYZ("qvisSolidFillSpans-4");
	    *((unsigned int *) pntplace) = fg;
	    pntplace += sizeof(unsigned int);
	    break;
	case 3:
	    XYZ("qvisSolidFillSpans-3");
	    *((unsigned short *) pntplace) = fg;
	    pntplace += sizeof(unsigned short);
	    *((unsigned char *) pntplace) = fg;
	    pntplace += sizeof(unsigned char);
	    break;
	case 2:
	    XYZ("qvisSolidFillSpans-2");
	    *((unsigned short *) pntplace) = fg;
	    pntplace += sizeof(unsigned short);
	    break;
	case 1:
	    XYZ("qvisSolidFillSpans-1");
	    *((unsigned char *) pntplace) = fg;
	    pntplace += sizeof(unsigned char);
	    break;
	}
	pwidth++;
	ppt++;
	n--;
    } while (n > 0);
}

#endif				/* never - use the planar version of
				 * qvisSolidFillSpans instead */

/*
 * qvisSolidFillSpans: uses planar mode do draw fast spans - planar mode is
 * much faster than packed mode, particularly for long spans like those drawn
 * by x11perf's 500 pixel solid circle test.
 */
void
qvisSolidFillSpans(
		     GCPtr pGC,
		     DrawablePtr pDraw,
		     DDXPointPtr ppt,
		     unsigned int *pwidth,
		     unsigned int n)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    nfbGCPrivPtr    gcPriv = NFB_GC_PRIV(pGC);
    unsigned int    x;
    unsigned int    y;
    unsigned int    width;
    unsigned int   *pntplace;
    unsigned int    nlw;
    unsigned int    startmask;
    unsigned int    endmask;

    XYZ("qvisSolidFillSpans-entered");
    qvisSetCurrentScreen();
    XYZdelta("qvisSolidFillSpans-n", n);

    if (n == 0) {
	/*
	 * Bail out early if there is nothing to draw.  This is important
	 * because in the loop below we use a do-while loop instead of
	 * just a while loop.  The n==0 case is NOT handled by the
	 * do-while loop below (so don't remove this bail out).
	 */
	return;
    }
    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    /*
     * Use planar mode so we can draw 32 pixels in a single access!
     */
    qvisSetPlanarMode(TRANSPARENT_WRITE);

    qvisSetForegroundColor((unsigned char) gcPriv->hwRop.fg);
    qvisSetALU(gcPriv->hwRop.alu);
    qvisSetPlaneMask((unsigned char) gcPriv->hwRop.planemask);

    do {
	x = ppt->x;
	y = ppt->y;
	width = *pwidth;
	/*
	 * determine aligned 32-bit word containing (x,y) pixel (in planar
	 * mode)
	 */
	pntplace = (unsigned int *) (fb_ptr + (y << (qvisPriv->pitch2 - 3)) + ((x >> 3) & ~3));
	if (((x & 0x1f) + width) < 32) {
	    XYZ("qvisSolidFillSpans-MaskPartialBits");
	    /* all pixels fit into a single 32-bit frame buffer word */
	    maskpartialbits(x, width, startmask);
	    SWAP_DWORD(startmask);				/* S000 */
	    *pntplace = startmask;
	} else {
	    XYZ("qvisSolidFillSpans-MaskBits");
	    maskbits(x, width, startmask, endmask, nlw);
	    if (startmask) {
		/* slam any initial pixels into place */
		SWAP_DWORD(startmask);				/* S000 */
		*pntplace++ = startmask;
	    }
	    /* slam "middle" pixels a word at a time */
	    while (nlw--) {
		*pntplace++ = ~0;
	    }
	    if (endmask) {
		/* slam any final pixels into place */
		SWAP_DWORD(endmask);				/* S000 */
		*pntplace = endmask;
	    }
	}
	pwidth++;
	ppt++;
	n--;
    } while (n > 0);
}
