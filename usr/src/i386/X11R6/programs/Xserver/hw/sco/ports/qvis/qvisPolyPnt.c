/*
 *	@(#) qvisPolyPnt.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S000    Tue Jul 13 12:43:06 PDT 1993    davidw@sco.com
 *      - compaq waltc AGA EFS 2.0 source handoff 07/02/93
 *
 */

/**
 *
 * qvisPolyPnt.c
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
#include "Xprotostr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "regionstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#if 0				/* XXX can't include without problem with
				 * xCharInfo */
#include "nfb/nfbProcs.h"
#else				/* all we really want is the nfbPolyPoint
				 * declaration so past it in */
void
nfbPolyPoint(
		struct _Drawable * pDraw,
		struct _GC * pGC,
		int mode,	/* Origin or Previous */
		unsigned int npt,
		struct _xPoint * pptInit);
#endif

#include "qvisHW.h"
#include "qvisDefs.h"
#include "qvisMacros.h"

/*
 * qvisPolyPoint - fast poly point drawing for Q-Vision; works by drawing
 * points only if the clipping region is a single box (like x11perf and
 * xbench do - snicker, snicker); otherwise punts to nfbPolyPoint -mjk
 *
 * This routine should ONLY be installed when HYPER_LINK_KIT is set
 * to YES since this violates what the SCO Link Kit allows.
 */
void
qvisPolyPoint(pDraw, pGC, mode, npt, ppt)
    DrawablePtr     pDraw;
    GCPtr           pGC;
    int             mode;	/* Origin or Previous */
    int             npt;
    xPoint         *ppt;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    unsigned char  *fb_ptr = qvisPriv->fb_base;
    BoxPtr          pbox;
    register int    x;
    register int    y;
    int             x1, y1, x2, y2;
    int             nbox;
    int             winx, winy;
    nfbGCPrivPtr    gcPriv = NFB_GC_PRIV(pGC);
    RegionPtr       pClip;
    unsigned char  *pntplace;
    register unsigned char fg;

    XYZ("qvisPolyPoint-entered");
    qvisSetCurrentScreen();
    if (npt == 0) {
	XYZ("qvisPolyPoint-NoPoints");
	return;
    }
    XYZdelta("qvisPolyPoint-npt", npt);

    pClip = gcPriv->pCompositeClip;

    pbox = REGION_RECTS(pClip);

    if ((nbox = REGION_NUM_RECTS(pClip)) != 1) {
	if (nbox > 0) {
	    /*
	     * PUNT back to the routine that can handle ALL cases!
	     */
	    XYZ("qvisPolyPoint-fallingBackToNfbPolyPoint");
	    nfbPolyPoint(pDraw, pGC, mode, npt, ppt);
	}
	return;
    }
    x1 = pbox->x1;
    x2 = pbox->x2;
    y1 = pbox->y1;
    y2 = pbox->y2;

    winx = pDraw->x;
    winy = pDraw->y;

    x = ppt->x + winx;
    y = ppt->y + winy;

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(ROPSELECT_ALL);
    qvisSetALU(gcPriv->hwRop.alu);
    qvisSetPlaneMask((unsigned char) gcPriv->hwRop.planemask);
    fg = (unsigned char) (gcPriv->hwRop.fg & 0xff);

    /* draw clipped point */
    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
	pntplace = (unsigned char *) (fb_ptr + (y << qvisPriv->pitch2) + x);
	*pntplace = (unsigned char) fg;
    }
    /* advance to second point */
    ppt++;
    npt--;

    if (mode == CoordModePrevious) {
	XYZ("qvisPolyPoint-mode==CoordModePrevious");
	/*
	 * This mode is rare (I think) and it's harder to unroll
	 * since each point is offset from the last and stuff.
	 */
	while (npt--) {
	    x += ppt->x;
	    y += ppt->y;

	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	    /* advance to next point! */
	    ppt++;
	}
    } else {			/* mode == CoordModeOrigin */
	XYZ("qvisPolyPoint-mode==CoordModeOrigin");
	/* loop unrolled 4X */
	while (npt > 3) {
	    XYZ("qvisPolyPoint-Origin-4X");
	    x = ppt->x + winx;
	    y = ppt->y + winy;
	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	    /* advance to next point! */
	    ppt++;
	    npt--;

	    x = ppt->x + winx;
	    y = ppt->y + winy;
	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	    /* advance to next point! */
	    ppt++;
	    npt--;

	    x = ppt->x + winx;
	    y = ppt->y + winy;
	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	    /* advance to next point! */
	    ppt++;
	    npt--;

	    x = ppt->x + winx;
	    y = ppt->y + winy;
	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	    /* advance to next point! */
	    ppt++;
	    npt--;
	}
	switch (npt) {
	case 3:
	    XYZ("qvisPolyPoint-Origin-3");
	    x = ppt->x + winx;
	    y = ppt->y + winy;
	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	    /* advance to next point! */
	    ppt++;
	case 2:
	    XYZ("qvisPolyPoint-Origin-2");
	    x = ppt->x + winx;
	    y = ppt->y + winy;
	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	    /* advance to next point! */
	    ppt++;
	case 1:
	    XYZ("qvisPolyPoint-Origin-1");
	    x = ppt->x + winx;
	    y = ppt->y + winy;
	    /* draw clipped point */
	    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
		pntplace = qvisFrameBufferLoc(fb_ptr, x, y);
		*pntplace = (unsigned char) fg;
	    }
	}
    }
}
