/*
 *	@(#) qvisRectOps.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Wed Oct 07 13:43:12 PDT 1992	mikep@sco.com
 *	- Move Mono routines into their own file
 *	- Wrote qvisDrawBankedPoints() for new Bres lines.
 *	S001	Wed Oct 14 14:08:36 PDT 1992	mikep@sco.com
 *	- Remove dead code.
 *	S002	Fri Oct 16 18:37:15 PDT 1992	mikep@sco.com
 *	- Add GC Caching to draw points.
 *	S003	Wed Oct 28 17:38:31 PST 1992	mikep@sco.com
 *	- Remove S002, GC caching can't be done off the drawable.
 *      S004    Wed May 12 10:12:36 PDT 1993    davidw@sco.com
 *      - Remove width/height checks from qvisCopyRect and
 *	qvisDrawSolidRects. Now done in NFB.  Also changed #define
 *	to USE_INLINE_CODE.
 *      S005    Thu Oct 07 12:50:52 PDT 1993    davidw@sco.com
 *      - Integrated Compaq source handoff
 *      S006    Tue Sep 20 14:53:30 PDT 1994    davidw@sco.com
 *      - Add VOLATILE keyword.
 *
 */

/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 * waltc     06/26/93  Generalize address, bank calculations.
 * waltc     10/05/93  Fix V35-3 ssblit bug.
 *
 */

#include "xyz.h"
#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"		/* for BITMAP_BIT_ORDER */

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"

#include "qvisHW.h"
#include "qvisDefs.h"
#include "qvisMacros.h"
#include "qvisProcs.h"

/*
 * qvisCopyRect
 */
void
qvisCopyRect(
	       BoxPtr pdstBox,
	       DDXPointPtr psrc,
	       unsigned char alu,
	       unsigned long planemask,
	       DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    int             width = pdstBox->x2 - pdstBox->x1;
    int             height = pdstBox->y2 - pdstBox->y1;
    int             src_page, dst_page;

    XYZ("qvisCopyRect-entered");
    qvisSetCurrentScreen();
    /* Check both engines */
    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    if (qvisPriv->bad_blit_state) {
	/*
	 * Reset the blit engine AND flush the shadowed values.  I don't know
	 * why this has to be done but the blit engine gets in a bad state
	 * when the screen-to-screen blit for the glyph caching is done and
	 * needs to be reset.
	 * 
	 * NOTE: bad_blit_state gets set by qvisFlushGlyphQueue.
	 */
	qvisOut8(GC_INDEX, 0x10);
	qvisOut8(GC_DATA, 0x40);
	qvisOut8(GC_DATA, 0x28);
#ifdef QVIS_SHADOWED
	/*
	 * More annoyance: when you reset the blit, we need to invalidate our
	 * shadowed registers.  (Ouch, that hurts.)
	 */
	qvisInvalidateShadows(qvisPriv);
#endif				/* QVIS_SHADOWED */
	qvisPriv->bad_blit_state = FALSE;
    }

    /* Force read-modify-write for V35-3 ssblits within 2K RAM page */
    if (qvisPriv->v35_blit_bug == TRUE && alu == 3) {
        src_page = ((psrc->y << qvisPriv->pitch2) + psrc->x) >> 11;
        dst_page = ((pdstBox->y1 << qvisPriv->pitch2) + pdstBox->x1) >> 11;
        if (src_page == dst_page) {
            qvisSetPackedMode(ROPSELECT_NO_ROPS | SRC_IS_SCRN_LATCHES);
            qvisSetALU(6);
        }
        else {
            qvisSetPackedMode(ROPSELECT_ALL | SRC_IS_SCRN_LATCHES);
            qvisSetALU(alu);
        }
    }
    else {
        qvisSetPackedMode(ROPSELECT_ALL | SRC_IS_SCRN_LATCHES);
        qvisSetALU(alu);
    }
    qvisSetPlaneMask((unsigned char) planemask);

    /* Width register */
    qvisOut16(WIDTH_REG, width);
    /* Height register */
    qvisOut16(HEIGHT_REG, height);

    qvisOut8(BLT_CMD1, 0xc0);	/* Src/Dst address is x-y format,  */

    if ((psrc->y < pdstBox->y1) ||
	(psrc->y == pdstBox->y1 &&
	 psrc->x < pdstBox->x1)) {
	/* blit in reverse!  */
	XYZ("qvisCopyRect-BlitInReverse");
	/* Specify src  */
	qvisOut16(X0_BREG, (short) (psrc->x + width - 1));
	qvisOut16(Y0_BREG, (short) (psrc->y + height - 1));
	/* Specify dest */
	qvisOut16(X1_BREG, (short) (pdstBox->x1 + width - 1));
	qvisOut16(Y1_BREG, (short) (pdstBox->y1 + height - 1));
	/* blt reverse,start   */
	qvisOut8(BLT_CMD0, 0x41);
    } else {
	XYZ("qvisCopyRect-BlitForward");
	/* Specify src   */
	qvisOut16(X0_BREG, (short) psrc->x);
	qvisOut16(Y0_BREG, (short) psrc->y);
	/* Specify dest */
	qvisOut16(X1_BREG, (short) pdstBox->x1);
	qvisOut16(Y1_BREG, (short) pdstBox->y1);
	/* blt forward,start   */
	qvisOut8(BLT_CMD0, 0x01);
    }
    qvisPriv->engine_used = TRUE;
}



/*
 * qvisDrawPoints - for FLAT model server ONLY.  Banked should use gen*.
 */
void
qvisDrawPoints(
		 DDXPointPtr ppt,
		 unsigned int npts,
		 unsigned long fg,
		 unsigned char alu,
		 unsigned long planemask,
		 DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    register unsigned char *ptplace;

    XYZ("qvisDrawPoints-entered");
    qvisSetCurrentScreen();

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }

    qvisSetPackedMode(ROPSELECT_ALL);
    qvisSetALU(alu);
    qvisSetPlaneMask((unsigned char) planemask);

    XYZ("qvisDrawPoints-npts");
    /* unrolled 4X loop */
    while (npts > 3) {
	ptplace = (unsigned char *) (fb_ptr + (ppt->y << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
	ppt++;

	ptplace = (unsigned char *) (fb_ptr + (ppt->y << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
	ppt++;

	ptplace = (unsigned char *) (fb_ptr + (ppt->y << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
	ppt++;

	ptplace = (unsigned char *) (fb_ptr + (ppt->y << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
	ppt++;

	npts -= 4;
    }
    switch (npts) {
    case 3:
	ptplace = (unsigned char *) (fb_ptr + (ppt->y << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
	ppt++;
    case 2:
	ptplace = (unsigned char *) (fb_ptr + (ppt->y << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
	ppt++;
    case 1:
	ptplace = (unsigned char *) (fb_ptr + (ppt->y << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
    }
}



/*
 * qvisDrawBankedPoints - for line speed this has to be done
 */
void
qvisDrawBankedPoints(
		 DDXPointPtr ppt,
		 unsigned int npts,
		 unsigned long fg,
		 unsigned char alu,
		 unsigned long planemask,
		 DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    register unsigned char *ptplace;
    int bank_number;
    int dy, dy2, dbank2, dbank_half;

    qvisSetCurrentScreen();

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(ROPSELECT_ALL);

    qvisSetALU(alu);
    qvisSetPlaneMask((unsigned char) planemask);

    /* Set bank-switch deltas for 64K window */
    if (qvisPriv->pitch == 2048) {
        dy = 31;     /* 32 X 2K lines */
        dy2 = 5;
    }
    else if (qvisPriv->pitch == 1024) {
        dy = 63;     /* 64 X 1K lines */
        dy2 = 6;
    }
    else {
        dy = 127;    /* 128 X 512 lines */
        dy2 = 7;
    }
    if (qvisPriv->width == 1280) {
        dbank2 = 2;  /* 4 X 16K pages */
        dbank_half = 2;
    }
    else {
        dbank2 = 4;  /* 16 X 4K pages */
        dbank_half = 8;
    }

    while (npts--) {
	if ((bank_number = (ppt->y >> dy2)) != qvisPriv->current_bank) {
	    XYZ("qvisDrawBankedImage-SwitchedBank");
	    qvisOut8(GC_INDEX, PAGE_REG1);
	    qvisOut8(GC_DATA, bank_number << dbank2);
	    qvisOut8(GC_INDEX, PAGE_REG2);
	    qvisOut8(GC_DATA, (bank_number << dbank2) + dbank_half);
	    qvisPriv->current_bank = bank_number;
	}
	ptplace = (unsigned char *) (fb_ptr + ((ppt->y & dy) << qvisPriv->pitch2) + ppt->x);
	*ptplace = (unsigned char) fg;
	ppt++;
    }
}


/*
 * qvisDrawSolidRects
 */
void
qvisDrawSolidRects(
		     BoxPtr pBox,
		     unsigned int nBox,
		     unsigned long color,
		     unsigned char rop,
		     unsigned long planemask,
		     DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    short           width;
    short           height;

    XYZ("qvisDrawSolidRects-entered");
    qvisSetCurrentScreen();

    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPlanarMode(ROPSELECT_ALL | SRC_IS_PATTERN_REGS);
    qvisSetALU(rop);
    qvisSetPlaneMask((unsigned char) planemask);

    qvisSetForegroundColor(color);
    qvisSetBackgroundColor(color);

    XYZ("qvisDrawSolidRects-nBox");
    while (nBox--) {

	width = pBox->x2 - pBox->x1;
	height = pBox->y2 - pBox->y1;

	/* width/height <= 0 checks are now done in NFB */
#ifdef XYZEXT
	switch (width) {
	case 1:
	    XYZ("qvisDrawSolidRects-width==1");
	    break;
	case 2:
	    XYZ("qvisDrawSolidRects-width==2");
	    break;
	case 3:
	    XYZ("qvisDrawSolidRects-width==3");
	    break;
	case 4:
	    XYZ("qvisDrawSolidRects-width==4");
	    break;
	case 5:
	    XYZ("qvisDrawSolidRects-width==5");
	    break;
	case 6:
	    XYZ("qvisDrawSolidRects-width==6");
	    break;
	case 7:
	    XYZ("qvisDrawSolidRects-width==7");
	    break;
	case 8:
	    XYZ("qvisDrawSolidRects-width==8");
	    break;
	default:
	    XYZ("qvisDrawSolidRects-width>8");
	    break;
	}
	switch (height) {
	case 1:
	    XYZ("qvisDrawSolidRects-height==1");
	    break;
	case 2:
	    XYZ("qvisDrawSolidRects-height==2");
	    break;
	case 3:
	    XYZ("qvisDrawSolidRects-height==3");
	    break;
	case 4:
	    XYZ("qvisDrawSolidRects-height==4");
	    break;
	case 5:
	    XYZ("qvisDrawSolidRects-height==5");
	    break;
	case 6:
	    XYZ("qvisDrawSolidRects-height==6");
	    break;
	case 7:
	    XYZ("qvisDrawSolidRects-height==7");
	    break;
	case 8:
	    XYZ("qvisDrawSolidRects-height==8");
	    break;
	default:
	    XYZ("qvisDrawSolidRects-height>8");
	    break;
	}
#endif

	/* program the engine here... */

#ifdef USE_INLINE_CODE
	/* Width register */
	qvisOut16(WIDTH_REG, width);
	/* Height register */
	qvisOut16(HEIGHT_REG, height);
	/* Specify src */
	qvisOut16(X0_BREG, (short) pBox->x1);
	qvisOut16(Y0_BREG, (short) pBox->y1);
	/* Specify dest */
	qvisOut16(X1_BREG, (short) pBox->x1);
	qvisOut16(Y1_BREG, (short) pBox->y1);
#else
	qvisZip(width, height, pBox->x1, pBox->y1);
#endif

	/* Src/Dst address is x-y format,  */
	qvisOut8(BLT_CMD1, 0xc0);

	/* blt forward,start   */
	qvisOut8(BLT_CMD0, 0x01);

	/* BUFFERING */
	qvisWaitForBufferNotBusy();
	pBox++;
    }
    qvisPriv->engine_used = TRUE;
}
