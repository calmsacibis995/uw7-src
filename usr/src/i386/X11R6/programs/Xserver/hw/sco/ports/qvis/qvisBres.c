/*
 *	@(#) qvisBres.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Oct 06 18:20:30 PDT 1992	mikep@sco.com
 *	- Rewrite qvisSolidZeroSeg() to be protocol complient.  Using the
 *	  Bresenham capabilities of the card may make it go faster.  xbench
 *	  is still unaffected.
 *	- Try a little GC caching
 *	S001	Sun Oct 11 16:16:03 PDT 1992	mikep@sco.com
 *	- Allow old version to be used if use does not specify -ppp
 *	S002	Thu Oct 15 15:21:03 PDT 1992	mikep@sco.com
 *	- Move the setting of engine_used
 *	S003	Thu Nov 05 16:40:56 PST 1992	mikep@sco.com
 *	- Change NFB_SERIAL_NUMBER macro.
 *	S004	Sat Dec 12 12:38:28 PST 1992	mikep@sco.com
 *	- GC Caching won't work unless you invalidate the current_gc
 *	everytime you change the hardware.  This needs a little more
 *	thought.
 *	S005	Thu Feb 11 10:31:23 PST 1993	mikep@sco.com
 *	- Incorperate Compaq fixes.  New line code.
 *      S006    Tue May 04 14:13:58 PDT 1993    davidw@sco.com
 *      - Fix stupid check for the line1 bug.  Point case was happening
 *	everytime.
 *      S007    Wed Sep 21 08:54:23 PDT 1994    davidw@sco.com
 *      - Correct compiler warnings.
 *
 */

/**
 * Copyright 1991, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 * waltc     02/04/93  Add Bresenham code, remove qvisSolidZeroSegPtToPt
 *
 */

/* #define USE_BRES */

#include "xyz.h"
#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "scrnintstr.h"

#include "mfb/mfb.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "qvisHW.h"
#include "qvisDefs.h"
#include "qvisMacros.h"

/*
 *  This routine is much slower under x11perf, but pixelates the same way
 *  as CFB.  Though it draws diagnol and very short lines faster than the
 *  one below.
 */  
void
qvisSolidZeroSeg(
		   GCPtr pGC,
		   DrawablePtr pDraw,
		   int signdx,
		   int signdy,
		   int axis,
		   int x,
		   int y,
		   int e,	/* XXX unreference formal parameter */
		   int e1,
		   int e2,
		   int len)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr    pGCPriv = NFB_GC_PRIV(pGC);
    void (* DrawPoints)(					/* S007 */
		DDXPointPtr ppt,
		unsigned int npts,
		unsigned long fg,
		unsigned char alu,
		unsigned long planemask,
		DrawablePtr pDrawable);
    DDXPointRec *pt, *tpt;
    unsigned char signcode = 0;
    int	npts;

    XYZ("qvisSolidZeroSeg-entered");
    qvisSetCurrentScreen();
    if (len <= 0) {
	XYZ("qvisSolidZeroSeg-ZeroLengthLine");
	return;
    }
    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }

    qvisSetPlanarMode(DOUBLE_DASH_LINE);

    qvisSetForegroundColor((unsigned char) (pGCPriv->rRop.fg));
    qvisSetPlaneMask((unsigned char) (pGCPriv->rRop.planemask));
    qvisSetALU(pGCPriv->rRop.alu);

/*
 *  Draw short line point by point
 */

    if (len < 5 || 
	e >= 0x800 || e < -0x800 || e2 >= 0x800 || e2 < -0x800 || e1 >= 0x800 ){
        DrawPoints = (NFB_WINDOW_PRIV(pDraw))->ops->DrawPoints;
        tpt = pt = (DDXPointRec *)ALLOCATE_LOCAL(len * (sizeof(DDXPointRec)));
        npts = len;

        while(len--) {
	    tpt->x = x;
	    tpt->y = y;
	    tpt++;
	    if( e > 0 ) {              /* make diagonal step */
	          x += signdx;
	          y += signdy;
	          e += e2;
	    }
	    else {                     /* make linear step   */
	        if( axis == X_AXIS ) 
		    x += signdx;
	        else
		    y += signdy;
	        e += e1;
	    }
       }
       (* DrawPoints)(pt, npts, pGCPriv->rRop.fg, pGCPriv->rRop.alu, 
  				       pGCPriv->rRop.planemask, pDraw);
       DEALLOCATE_LOCAL(pt);
    }

/*
 *  Draw 90 degree accelerated line
 */

    else if (e2 == 0) {
        /* Retain Pat Ptr, Keep X0/Y0, Calc and Draw */
        qvisOut8(GC_INDEX, LINE_COMMAND_REG);
        qvisOut8(GC_DATA, 0x18);
        len--;
#ifdef USE_INLINE_CODE
	qvisOut16(X0_LREG, (short) x);
	qvisOut16(Y0_LREG, (short) y);
	qvisOut16(X1_LREG, (short) x + (len * signdx));
	/* This I/O will start engine */
	qvisOut16(Y1_LREG, (short) y + (len * signdy));
#else
	/*
	 * Using this macro results in a 5-7% speedup in line drawing
	 * performance using this routine (measured by x11perf -range
	 * line1,line100). -mjk
	 */
	qvisSpit(x, y, x + (len * signdx), y + (len * signdy));
#endif
	qvisPriv->engine_used = TRUE;				/* S002 */
	return;
    }

/*
 *  Draw other sloped
 */

    else {
        /* Axial_When_0, Retain_Pattern_Pointer, Keep_X0_Y0, Last_Pixel_Null, Calc_Only */
        qvisOut8(GC_INDEX, LINE_COMMAND_REG);
        qvisOut8(GC_DATA, 0x5E);

        qvisOut16(X0_LREG, (short) x);
        qvisOut16(Y0_LREG, (short) y);
        qvisOut8(GC_INDEX, LINE_ERROR_TERM); 
        qvisOut8(GC_DATA, e & 0x00ff);
        qvisOut8(GC_INDEX, LINE_ERROR_TERM + 1); 
        qvisOut8(GC_DATA, (e & 0xff00) >> 8);
        qvisOut8(GC_INDEX, LINE_K1_CONST);
        qvisOut8(GC_DATA, e1 & 0x00ff);
        qvisOut8(GC_INDEX, LINE_K1_CONST + 1);
        qvisOut8(GC_DATA, (e1 & 0xff00) >> 8);
        qvisOut8(GC_INDEX, LINE_K2_CONST);
        qvisOut8(GC_DATA, e2 & 0x00ff);
        qvisOut8(GC_INDEX, LINE_K2_CONST + 1);
        qvisOut8(GC_DATA, (e2 & 0xff00) >> 8);
        qvisOut8(GC_INDEX, LINE_SIGN_CODES);
        if (signdx < 0)
            signcode = LINE_SIGN_DX;
        if (signdy < 0)
            signcode |= LINE_SIGN_DY;
        if (axis == Y_AXIS)
            signcode |= LINE_MAJ_AXIS;
        qvisOut8(GC_DATA, signcode);
        qvisOut8(GC_INDEX, LINE_PEL_CNT);
        qvisOut8(GC_DATA, len & 0x00ff);
        qvisOut8(GC_INDEX, LINE_PEL_CNT + 1);
        qvisOut8(GC_DATA, (len & 0xff00) >> 8);

        /* Start line draw */
        qvisOut8(GC_INDEX, LINE_COMMAND_REG);
        qvisOut8(GC_DATA, 0x5F);

        qvisPriv->engine_used = TRUE;
    }
}
