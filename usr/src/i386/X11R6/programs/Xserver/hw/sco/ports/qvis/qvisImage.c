/**
 * @(#) qvisImage.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
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
 *	SCO MODIFICATION HISTORY
 *
 *      S000    Wed May 12 10:34:36 PDT 1993    davidw@sco.com
 *      - Removed width/height == 0 checks.  Now done in NFB.
 *      S001    Tue Jul 13 12:37:08 PDT 1993    davidw@sco.com
 *      - compaq waltc AGA EFS 2.0 source handoff 07/02/93
 *      S002    Wed Sep 21 08:40:39 PDT 1994    davidw@sco.com
 *      - Add VOLATILE keyword.
 */

/**
 * qvisImage.c
 *
 * Template for machine dependent ReadImage and DrawImage routines
 */

/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 * mikep@sco 07/27/92  Make current_bank a screen private.
 * waltc     06/26/93  Generalize address, bank calculations.
 *
 */

#include "xyz.h"
#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "qvisHW.h"
#include "qvisDefs.h"
#include "qvisMacros.h"

/* Set bank-switch deltas for 64K window */
int dy, dy2, dbank2, dbank_half;
#define SET_BANK_DELTAS { \
    if (qvisPriv->pitch == 2048) { \
        dy = 31;     /* 32 X 2K lines */ \
        dy2 = 5; \
    } \
    else if (qvisPriv->pitch == 1024) { \
        dy = 63;     /* 64 X 1K lines */ \
        dy2 = 6; \
    } \
    else { \
        dy = 127;    /* 128 X 512 lines */ \
        dy2 = 7; \
    } \
    if (qvisPriv->width == 1280) { \
        dbank2 = 2;  /* 4 X 16K pages */ \
        dbank_half = 2; \
    } \
    else { \
        dbank2 = 4;  /* 16 X 4K pages */ \
        dbank_half = 8; \
    } \
}

/**
 * qvisReadImage() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 1-bit pixels eight per byte;
 *		pack 2- to 8-bit pixels one per byte; pack 9- to 32-bit
 *		pixels one per 32-bit word.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void
qvisReadImage(pbox, image, stride, pDraw)
    BoxPtr          pbox;
    register unsigned char *image;
    unsigned int    stride;
    DrawablePtr     pDraw;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    int             width, height, rows, cols;
    register unsigned char *cur_fb_ptr;
    unsigned char  *temp_fb_ptr;
    unsigned char  *img_ptr = image;

    XYZ("qvisReadImage-entered");
    qvisSetCurrentScreen();
    width = pbox->x2 - pbox->x1;
    height = pbox->y2 - pbox->y1;

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(ROPSELECT_ALL);

    /* set ptr to first pixel */
    temp_fb_ptr = cur_fb_ptr = (unsigned char *) (fb_ptr +
                               (pbox->y1 << qvisPriv->pitch2) + pbox->x1);

    for (rows = 0; rows < height; rows++) {
	unsigned int   *i;
	unsigned int   *c;
	int             unaligned;
	int             start;

	img_ptr = image;

	unaligned = ((int) cur_fb_ptr) & 3;
	if (unaligned && (width > 3)) {
	    XYZ("qvisReadImage-UnalignedAndWideEnough");
	    /* not aligned on screen - try to adjust */
	    switch (unaligned) {
	    case 1:
		XYZ("qvisReadImage-Unaligned1");
		*img_ptr++ = *cur_fb_ptr++;
	    case 2:
		XYZ("qvisReadImage-Unaligned2");
		*img_ptr++ = *cur_fb_ptr++;
	    case 3:
		XYZ("qvisReadImage-Unaligned3");
		*img_ptr++ = *cur_fb_ptr++;
	    }
	    start = 4 - unaligned;
	    /* now we are aligned! */
	} else {
	    XYZ("qvisReadImage-AlignedOrNotWideEnough");
	    start = 0;
	}

	i = (unsigned int *) img_ptr;
	c = (unsigned int *) cur_fb_ptr;

	for (cols = start; cols < width - 11; cols += 12) {
	    XYZ("qvisReadImage-TripleWord");
	    *i++ = *c++;
	    *i++ = *c++;
	    *i++ = *c++;
	}

	switch (width - cols) {
	case 11:
	    XYZ("qvisReadImage-Final11");
	    *i++ = *c++;
	    *i++ = *c++;
	    *(((unsigned short *) i)++) = *(((unsigned short *) c)++);
	    *(((unsigned char *) i)++) = *(((unsigned char *) c)++);
	    break;
	case 10:
	    XYZ("qvisReadImage-Final10");
	    *i++ = *c++;
	    *i++ = *c++;
	    *(((unsigned short *) i)++) = *(((unsigned short *) c)++);
	    break;
	case 9:
	    XYZ("qvisReadImage-Final9");
	    *i++ = *c++;
	    *i++ = *c++;
	    *(((unsigned char *) i)++) = *(((unsigned char *) c)++);
	    break;
	case 8:
	    XYZ("qvisReadImage-Final8");
	    *i++ = *c++;
	    *i++ = *c++;
	    break;
	case 7:
	    XYZ("qvisReadImage-Final7");
	    *i++ = *c++;
	    *(((unsigned short *) i)++) = *(((unsigned short *) c)++);
	    *(((unsigned char *) i)++) = *(((unsigned char *) c)++);
	    break;
	case 6:
	    XYZ("qvisReadImage-Final6");
	    *i++ = *c++;
	    *(((unsigned short *) i)++) = *(((unsigned short *) c)++);
	    break;
	case 5:
	    XYZ("qvisReadImage-Final5");
	    *i++ = *c++;
	    *(((unsigned char *) i)++) = *(((unsigned char *) c)++);
	    break;
	case 4:
	    XYZ("qvisReadImage-Final4");
	    *i++ = *c++;
	    break;
	case 3:
	    XYZ("qvisReadImage-Final3");
	    *(((unsigned short *) i)++) = *(((unsigned short *) c)++);
	    *(((unsigned char *) i)++) = *(((unsigned char *) c)++);
	    break;
	case 2:
	    XYZ("qvisReadImage-Final2");
	    *(((unsigned short *) i)++) = *(((unsigned short *) c)++);
	    break;
	case 1:
	    XYZ("qvisReadImage-Final1");
	    *(((unsigned char *) i)++) = *(((unsigned char *) c)++);
	    break;
	}

	cur_fb_ptr = temp_fb_ptr + qvisPriv->pitch;
	temp_fb_ptr = cur_fb_ptr;
	image += stride;
    }
}

/*
 * qvisReadBankedImage: This routine is similar to the non-banked routine
 * above, except that it sets the page registers to    point to the
 * appropriate 64k window onto VRAM, and  operates with the address scaled to
 * that window.
 */
void
qvisReadBankedImage(pbox, image, stride, pDraw)
    BoxPtr          pbox;
    register unsigned char *image;
    unsigned int    stride;
    DrawablePtr     pDraw;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    int             width, height, rows, cols;
    register unsigned char *cur_fb_ptr;
    unsigned char  *temp_fb_ptr;
    unsigned char  *img_ptr = image;
    int             bank_number;
    int             current_y;

    XYZ("qvisReadBankedImage-entered");
    qvisSetCurrentScreen();
    current_y = pbox->y1;

    width = pbox->x2 - pbox->x1;
    height = pbox->y2 - current_y;

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(ROPSELECT_ALL);

    /* Set bank-switch deltas */
    SET_BANK_DELTAS;

    /* set ptr to first pixel */
    temp_fb_ptr = cur_fb_ptr = (unsigned char *) (fb_ptr +
                             ((current_y & dy) << qvisPriv->pitch2) + pbox->x1);

    for (rows = 0; rows < height; rows++) {
	img_ptr = image;

	if ((bank_number = (current_y >> dy2)) != qvisPriv->current_bank) {
 	    qvisOut8(GC_INDEX, PAGE_REG1);
	    qvisOut8(GC_DATA, bank_number << dbank2);
	    qvisOut8(GC_INDEX, PAGE_REG2);
	    qvisOut8(GC_DATA, (bank_number << dbank2) + dbank_half);
	    temp_fb_ptr = cur_fb_ptr = (unsigned char *) (fb_ptr +
                             ((current_y & dy) << qvisPriv->pitch2) + pbox->x1);
	    qvisPriv->current_bank = bank_number;
	}
	for (cols = 0; cols < width; cols++) {
	    *img_ptr++ = *cur_fb_ptr++;
	}

	cur_fb_ptr = temp_fb_ptr + qvisPriv->pitch;
	temp_fb_ptr = cur_fb_ptr;
	image += stride;
	current_y++;

    }
}

/**
 * qvisDrawImage() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void
qvisDrawImage(pbox, image, stride, alu, planemask, pDraw)
    BoxPtr          pbox;
    unsigned char  *image;
    unsigned int    stride;
    unsigned char   alu;
    unsigned long   planemask;
    DrawablePtr     pDraw;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    int             width, height, rows, cols;
    register unsigned char *cur_fb_ptr;
    unsigned char  *temp_fb_ptr;
    unsigned char  *img_ptr = image;

    XYZ("qvisDrawImage-entered");
    qvisSetCurrentScreen();
    width = pbox->x2 - pbox->x1;
    height = pbox->y2 - pbox->y1;

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(ROPSELECT_ALL);
    qvisSetPlaneMask(planemask);
    qvisSetALU(alu);

    /* set ptr to first pixel */
    cur_fb_ptr = qvisFrameBufferLoc(fb_ptr, pbox->x1, pbox->y1);

    if (width == 1) {
	XYZ("qvisDrawImage-width==1");
	img_ptr = image;
	for (rows = 0; rows < height; rows++) {

	    *cur_fb_ptr = *img_ptr;
	    cur_fb_ptr += qvisPriv->pitch;
	    img_ptr += stride;

	}
    } else {			/* width > 1 */

	for (rows = 0; rows < height; rows++) {
	    unsigned int   *i;
	    unsigned int   *c;
	    int             unaligned;
	    int             start;

	    img_ptr = image;

	    temp_fb_ptr = cur_fb_ptr;
	    unaligned = ((int) cur_fb_ptr) & 3;
	    if (unaligned && (width > 3)) {
		XYZ("qvisDrawImage-UnalignedAndWideEnough");
		/* not aligned on screen - try to adjust */
		switch (unaligned) {
		case 1:
		    XYZ("qvisDrawImage-Unaligned1");
		    *cur_fb_ptr++ = *img_ptr++;
		case 2:
		    XYZ("qvisDrawImage-Unaligned2");
		    *cur_fb_ptr++ = *img_ptr++;
		case 3:
		    XYZ("qvisDrawImage-Unaligned3");
		    *cur_fb_ptr++ = *img_ptr++;
		}
		start = 4 - unaligned;
		/* now we are aligned! */
	    } else {
		XYZ("qvisDrawImage-AlignedOrNotWideEnough");
		start = 0;
	    }

	    i = (unsigned int *) img_ptr;
	    c = (unsigned int *) cur_fb_ptr;

	    for (cols = start; cols < (width - 11); cols += 12) {
		XYZ("qvisDrawImage-TripleWord");
		*c++ = *i++;
		*c++ = *i++;
		*c++ = *i++;
	    }

	    switch (width - cols) {
	    case 11:
		XYZ("qvisDrawImage-Final11");
		*c++ = *i++;
		*c++ = *i++;
		*(((unsigned short *) c)++) = *(((unsigned short *) i)++);
		*(((unsigned char *) c)++) = *(((unsigned char *) i)++);
		break;
	    case 10:
		XYZ("qvisDrawImage-Final10");
		*c++ = *i++;
		*c++ = *i++;
		*(((unsigned short *) c)++) = *(((unsigned short *) i)++);
		break;
	    case 9:
		XYZ("qvisDrawImage-Final9");
		*c++ = *i++;
		*c++ = *i++;
		*(((unsigned char *) c)++) = *(((unsigned char *) i)++);
		break;
	    case 8:
		XYZ("qvisDrawImage-Final8");
		*c++ = *i++;
		*c++ = *i++;
		break;
	    case 7:
		XYZ("qvisDrawImage-Final7");
		*c++ = *i++;
		*(((unsigned short *) c)++) = *(((unsigned short *) i)++);
		*(((unsigned char *) c)++) = *(((unsigned char *) i)++);
		break;
	    case 6:
		XYZ("qvisDrawImage-Final6");
		*c++ = *i++;
		*(((unsigned short *) c)++) = *(((unsigned short *) i)++);
		break;
	    case 5:
		XYZ("qvisDrawImage-Final5");
		*c++ = *i++;
		*(((unsigned char *) c)++) = *(((unsigned char *) i)++);
		break;
	    case 4:
		XYZ("qvisDrawImage-Final4");
		*c++ = *i++;
		break;
	    case 3:
		XYZ("qvisDrawImage-Final3");
		*(((unsigned short *) c)++) = *(((unsigned short *) i)++);
		*(((unsigned char *) c)++) = *(((unsigned char *) i)++);
		break;
	    case 2:
		XYZ("qvisDrawImage-Final2");
		*(((unsigned short *) c)++) = *(((unsigned short *) i)++);
		break;
	    case 1:
		XYZ("qvisDrawImage-Final1");
		*(((unsigned char *) c)++) = *(((unsigned char *) i)++);
	    }

	    cur_fb_ptr = temp_fb_ptr + qvisPriv->pitch;
	    image += stride;
	}
    }
}

/*
 * qvisDrawBankedImage This routine is similar to the non-banked routine
 * above, except that it sets the page registers to    point to the
 * appropriate 64k window onto VRAM, and  operates with the address scaled to
 * that window.
 */
void
qvisDrawBankedImage(pbox, image, stride, alu, planemask, pDraw)
    BoxPtr          pbox;
    unsigned char  *image;
    unsigned int    stride;
    unsigned char   alu;
    unsigned long   planemask;
    DrawablePtr     pDraw;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    int             width, height, rows, cols;
    register unsigned char *cur_fb_ptr;
    unsigned char  *temp_fb_ptr;
    unsigned char  *img_ptr = image;
    int             bank_number;
    int             current_y;

    XYZ("qvisDrawBankedImage-entered");
    qvisSetCurrentScreen();
    current_y = pbox->y1;

    width = pbox->x2 - pbox->x1;
    height = pbox->y2 - current_y;

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(ROPSELECT_ALL);
    qvisSetPlaneMask(planemask);
    qvisSetALU(alu);

    /* Set bank-switch deltas */
    SET_BANK_DELTAS;

    /* set ptr to first pixel */
    temp_fb_ptr = cur_fb_ptr = (unsigned char *) (fb_ptr +
                             ((current_y & dy) << qvisPriv->pitch2) + pbox->x1);

    for (rows = 0; rows < height; rows++) {
	img_ptr = image;

	if ((bank_number = (current_y >> dy2)) != qvisPriv->current_bank) {
	    XYZ("qvisDrawBankedImage-SwitchedBank");
 	    qvisOut8(GC_INDEX, PAGE_REG1);
	    qvisOut8(GC_DATA, bank_number << dbank2);
	    qvisOut8(GC_INDEX, PAGE_REG2);
	    qvisOut8(GC_DATA, (bank_number << dbank2) + dbank_half);
	    temp_fb_ptr = cur_fb_ptr = (unsigned char *) (fb_ptr +
                             ((current_y & dy) << qvisPriv->pitch2) + pbox->x1);
	    qvisPriv->current_bank = bank_number;
	}
	for (cols = 0; cols < width; cols++) {

	    *cur_fb_ptr++ = *img_ptr++;
	}

	cur_fb_ptr = temp_fb_ptr + qvisPriv->pitch;
	temp_fb_ptr = cur_fb_ptr;
	image += stride;
	current_y++;

    }
}
