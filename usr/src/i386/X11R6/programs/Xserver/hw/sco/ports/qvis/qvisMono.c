/*
 *	@(#) qvisMono.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Oct 06 18:24:34 PDT 1992	mikep@sco.com
 *	- Created file and ported to R5
 *	S001	Thu Oct 15 14:11:02 PDT 1992	mikep@sco.com
 *	- Fix two stupid bitswapping bugs
 *      S002    Wed May 12 10:22:25 PDT 1993    davidw@sco.com
 *      - Removed width/height == 0 checks from qvisDrawMonoImage
 *	and qvisDrawOpaqueMonoImage. Now done in NFB.
 *      S003    Tue Sep 20 14:57:06 PDT 1994    davidw@sco.com
 *      - Corrected compiler warnings.
 *	- Add VOLATILE keyword.
 *
 */

#include "xyz.h"
#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"		/* for BITMAP_BIT_ORDER */
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"


#include "qvisHW.h"
#include "qvisDefs.h"
#include "qvisMacros.h"
#include "qvisProcs.h"

/*
 * qvisDrawMonoImage
 */
void
qvisDrawMonoImage(
		    BoxPtr pbox,
		    void *void_image,				/* S003 */
		    unsigned int startx,
		    unsigned int stride,
		    unsigned long fg,
		    unsigned char alu,
		    unsigned long planemask,
		    DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    register int    width, height;
    int             widthinbytes, innrbytes;
    unsigned char  *pimage;
    int             errorflag = 0;
    unsigned char *image = (unsigned char *)void_image;		/* S003 */

    XYZ("qvisDrawMonoImage-entered");
    qvisSetCurrentScreen();
    height = pbox->y2 - pbox->y1;
    width = pbox->x2 - pbox->x1;

#ifdef XYZEXT
    switch (width) {
    case 1:
	XYZ("qvisDrawMonoImage-width==1");
	break;
    case 2:
	XYZ("qvisDrawMonoImage-width==2");
	break;
    case 3:
	XYZ("qvisDrawMonoImage-width==3");
	break;
    case 4:
	XYZ("qvisDrawMonoImage-width==4");
	break;
    case 5:
	XYZ("qvisDrawMonoImage-width==5");
	break;
    case 6:
	XYZ("qvisDrawMonoImage-width==6");
	break;
    case 7:
	XYZ("qvisDrawMonoImage-width==7");
	break;
    case 8:
	XYZ("qvisDrawMonoImage-width==8");
	break;
    default:
	XYZ("qvisDrawMonoImage-width>8");
	break;
    }
    switch (height) {
    case 1:
	XYZ("qvisDrawMonoImage-height==1");
	break;
    case 2:
	XYZ("qvisDrawMonoImage-height==2");
	break;
    case 3:
	XYZ("qvisDrawMonoImage-height==3");
	break;
    case 4:
	XYZ("qvisDrawMonoImage-height==4");
	break;
    case 5:
	XYZ("qvisDrawMonoImage-height==5");
	break;
    case 6:
	XYZ("qvisDrawMonoImage-height==6");
	break;
    case 7:
	XYZ("qvisDrawMonoImage-height==7");
	break;
    case 8:
	XYZ("qvisDrawMonoImage-height==8");
	break;
    default:
	XYZ("qvisDrawMonoImage-height>8");
	break;
    }
    if (height * width <= 10)
	XYZ("qvisDrawMonoImage-pixels<=10");
    if (height * width > 10 && height * width <= 50)
	XYZ("qvisDrawMonoImage-pixels>10,<=50");
    if (height * width > 50 && height * width <= 100)
	XYZ("qvisDrawMonoImage-pixels>50,<=100");
    if (height * width > 100 && height * width <= 1000)
	XYZ("qvisDrawMonoImage-pixels>100,<=1000");
    if (height * width > 1000)
	XYZ("qvisDrawMonoImage-pixels>1000");
#endif				/* XYZEXT */

    /* Check both engines */
    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPlanarMode(TRANSPARENT_WRITE);
    qvisSetALU(alu);
    qvisSetPlaneMask((unsigned char) planemask);
    qvisSetForegroundColor((unsigned char) fg);

    if (startx >= 8) {
	image += startx >> 3;
	startx &= 7;
    }
    /* program up bitblt eng */
    qvisOut16(X0_BREG, (short) startx);

    qvisOut16(WIDTH_REG, (short) width);
    qvisOut16(HEIGHT_REG, (short) height);

    widthinbytes = (startx + width) >> 3;

    qvisOut16(X1_BREG, (short) pbox->x1);
    qvisOut16(Y1_BREG, (short) pbox->y1);

    if ((startx + width) & 7)
	widthinbytes += 1;

#ifdef XYZEXT
    switch (widthinbytes) {
    case 1:
	XYZ("qvisDrawMonoImage-widthinbytes==1");
	break;
    case 2:
	XYZ("qvisDrawMonoImage-widthinbytes==2");
	break;
    case 3:
	XYZ("qvisDrawMonoImage-widthinbytes==3");
	break;
    case 4:
	XYZ("qvisDrawMonoImage-widthinbytes==4");
	break;
    default:
	XYZ("qvisDrawMonoImage-widthinbytes>4");
	break;
    }
    switch (stride) {
    case 1:
	XYZ("qvisDrawMonoImage-stride==1");
	break;
    case 2:
	XYZ("qvisDrawMonoImage-stride==2");
	break;
    case 3:
	XYZ("qvisDrawMonoImage-stride==3");
	break;
    case 4:
	XYZ("qvisDrawMonoImage-stride==4");
	break;
    case 5:
	XYZ("qvisDrawMonoImage-stride==5");
	break;
    case 6:
	XYZ("qvisDrawMonoImage-stride==6");
	break;
    default:
	XYZ("qvisDrawMonoImage-stride>6");
	break;
    }
#endif

    qvisOut8(BLT_CMD1, 0x80);	/* set DAF to X-Y */
    qvisOut8(BLT_CMD0, 0x01);	/* turn on the engine */

    /*
     * MISC NOTE: xdvi will make this routine get called for every character
     * it displays.  For small characters < 32 pixels wide, the stride is
     * ALWAYS 4.  We would like to take advantage of this fact to speed up
     * the slowness of Q-Vision xdvi but the width varies from 1-4 bytes for
     * the strides of 4.  The switch on widthinbytes should help because 1
     * and 2 bytes are the most common. -mjk
     */

    /*
     * CAREFUL: it seems reasonable to switch on stride to elminate
     * incrementing image by a variable amount (we would increment by a
     * constant 1 or 2).  In the stride=2 case though, the width could be
     * less than two! -mjk
     */
    switch (widthinbytes) {
    case 1:
	/*
	 * use a single loop over height (instead of looping over width to
	 * like the default case does). -mjk
	 */
	XYZ("qvisDrawMonoImage-FastStride1");
	do {
	    *fb_ptr = BIT_SWAP(*image);				/* S000 */
	    image += stride;
	    height--;
	} while (height > 0);
	break;
    case 2:
	/*
	 * use a single loop over height (instead of looping over width to
	 * like the default case does).  AND stuff two bytes at a time! -mjk
	 */
	XYZ("qvisDrawMonoImage-FastStride2");
	do {
#if BITMAP_BIT_ORDER == LSBFirst				/* S000 */
	    *fb_ptr = BIT_SWAP(*image);				/* S000 */
	    *fb_ptr = BIT_SWAP(*(image+1));			/* S001 */
#else								/* S000 */
	    *((unsigned short *) fb_ptr) = *((unsigned short *) image);
#endif
	    image += stride;
	    height--;
	} while (height > 0);
	break;
    default:
	XYZ("qvisDrawMonoImage-SlowStride");
	do {			/* we know height is at least 1 so invert
				 * loop -mjk */
	    pimage = image;
	    innrbytes = widthinbytes;
	    while (innrbytes >= 4) {
		XYZ("qvisDrawMonoImage-4");
		*fb_ptr = BIT_SWAP(*pimage++);	/* anywhere in frame buffer */
		*fb_ptr = BIT_SWAP(*pimage++);	/* anywhere in frame buffer */
		*fb_ptr = BIT_SWAP(*pimage++);	/* anywhere in frame buffer */
		*fb_ptr = BIT_SWAP(*pimage++);	/* anywhere in frame buffer */
		innrbytes -= 4;
	    }
	    switch (innrbytes) {
	    case 3:
		XYZ("qvisDrawMonoImage-3");
		*fb_ptr = BIT_SWAP(*pimage++);   /* anywhere in frame buffer */
	    case 2:
		XYZ("qvisDrawMonoImage-2");
		*fb_ptr = BIT_SWAP(*pimage++);   /* anywhere in frame buffer */
	    case 1:
		XYZ("qvisDrawMonoImage-1");
		/* NOTE: unnecessary to decrement pimage this last time */
		*fb_ptr = BIT_SWAP(*pimage); /* anywhere in frame buffer S001 */
	    }
	    image += stride;
	    height--;
	} while (height > 0);
	break;
    }				/* end of switch(stride) */

    qvisPriv->engine_used = TRUE;
}

/*
 * qvisDrawOpaqueMonoImage
 */
void
qvisDrawOpaqueMonoImage(
			  BoxPtr pbox,
			  void *void_image,			/* S003 */
			  unsigned int startx,
			  unsigned int stride,
			  unsigned long fg,
			  unsigned long bg,
			  unsigned char alu,
			  unsigned long planemask,
			  DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base;
    register int    width, height;
    unsigned char  *pline;
    int             widthinbytes, innrbytes;
    unsigned char  *pimage;
    unsigned char *image = (unsigned char *)void_image;		/* S003 */

    XYZ("qvisDrawOpaqueMonoImage-entered");
    qvisSetCurrentScreen();
    height = pbox->y2 - pbox->y1;
    width = pbox->x2 - pbox->x1;

    /* Check both engines */
    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPlanarMode(COLOR_EXPAND_WRITE);
    qvisSetALU(alu);
    qvisSetPlaneMask((unsigned char) planemask);
    qvisSetForegroundColor((unsigned char) fg);
    qvisSetBackgroundColor((unsigned char) bg);

    if (startx >= 8) {
	image += startx >> 3;
	startx &= 7;
    }
    /* program up bitblt eng */
    qvisOut16(X0_BREG, (short) startx);

    qvisOut16(WIDTH_REG, (short) width);
    qvisOut16(HEIGHT_REG, (short) height);

    widthinbytes = (startx + width) >> 3;

    qvisOut16(X1_BREG, (short) pbox->x1);
    qvisOut16(Y1_BREG, (short) pbox->y1);

    if ((startx + width) & 7)
	widthinbytes += 1;

    /* set DAF to X-Y */
    qvisOut8(BLT_CMD1, 0x80);
    /* turn on the engine */
    qvisOut8(BLT_CMD0, 0x01);

    /*
     * XXX These loops could be unrolled but I don't think this
     * code is performance critical.  Unroll it just like 
     * qvisDrawMonoImage does.
     */

    /* rows loop */
    for (pline = image; height--; pline += stride) {
	pimage = pline;

	/* columns loop */
	for (innrbytes = 0; innrbytes < widthinbytes; innrbytes++) {
	    /* doc says anywhere in frame buffer */
	    *fb_ptr = BIT_SWAP(*pimage++);			/* S000 */
	}
    }
    qvisPriv->engine_used = TRUE;
}



