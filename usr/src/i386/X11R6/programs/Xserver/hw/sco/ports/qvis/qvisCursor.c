/*
 *	@(#) qvisCursor.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Oct 06 21:51:26 PDT 1992	mikep@sco.com
 *	- Add BIT_SWAP code
 *	- A NullCursor in SetCursor means to turn it off
 *      S001    Thu Oct 07 12:49:51 PDT 1993    davidw@sco.com
 *      - Integrated Compaq source handoff
 *      S002    Tue Mar 22 04:14:40 PST 1994	davidw@sco.com
 *      - Integrated Compaq source handoff AHS 3.3.0.
 *      S003    Wed Sep 21 09:15:59 PDT 1994    davidw@sco.com
 *      - Correct compiler warnings.
 *
 */

/**
 * qvisCursor.c
 *
 */

/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 * mikep     10/05/92  Shift cursor color values
 * waltc     06/26/93  Add 64 X 64 cursor support.
 * waltc     10/05/93  Fix cursor src/mask indexing.
 * waltc     03/20/94  Reset 2 MSB's of 64 X 64 cursor RAM address.
 *
 */

#include "xyz.h"
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"
#include "servermd.h"

#ifdef usl
#include "mi/mi.h"
#endif

#include "mi/mipointer.h"
#include "mi/misprite.h"

#include "scoext.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "qvisHW.h"
#include "qvisMacros.h"
#include "qvisDefs.h"

Bool            qvisRealizeCursor();
Bool            qvisUnrealizeCursor();
void            qvisSetCursor();
void            qvisMoveCursor();
void            qvisCursorOn();

miPointerSpriteFuncRec qvisPointerFuncs =
{
    qvisRealizeCursor,
    qvisUnrealizeCursor,
    qvisSetCursor,
    qvisMoveCursor,
};

#define SQUARE(x) (x) * (x)

/*
 * Initialize the cursor and register movement routines.
 */
void
qvisCursorInitialize(pScreen)
    ScreenPtr       pScreen;
{
    XYZ("qvisCursorInitialize-entered");
    if (scoPointerInitialize(pScreen, &qvisPointerFuncs, NULL, TRUE) == 0) {

	FatalError("qvis: Cannot initialize Hardware Cursor\n");
    }
}

/*
 * Realize the Cursor Image
 */
static          Bool
qvisRealizeCursor(pScr, pCurs)
    ScreenPtr       pScr;
    CursorPtr       pCurs;	/* a SERVER-DEPENDENT cursor */
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScr);
    unsigned char  *a, *b;	/* vaxstar-defined */
    unsigned char  *mask;	/* server-defined */
    unsigned char  *src;	/* server-defined */
    unsigned int    j;
    int             i;
    int             maxbytes, goodbytes, stride;
    int             cursor_bytes = SQUARE(qvisPriv->max_cursor) / 4;
    int             cursor_chars = cursor_bytes / sizeof(char);
    int             lastRow = (((int)pCurs->bits->height < qvisPriv->max_cursor) ? (int)pCurs->bits->height : qvisPriv->max_cursor); /* S003 */
    /*
     * used to mask off beyond the edge of the real mask and source bits
     */

    XYZ("qvisRealizeCursor-entered");
    pCurs->devPriv[pScr->myNum] = (pointer) xalloc(cursor_bytes);
    if (pCurs->devPriv[pScr->myNum] == NULL) {
	return FALSE;
    }
    bzero((char *) pCurs->devPriv[pScr->myNum], cursor_bytes);

    /**
     * Transform the SERVER-DEPENDENT, device-independent cursor bits into
     * what the device wants, which is 256 or 1024 contiguous BYTES.
     *
     * Also, plane0 data is expected to be followed by plane1 data, in
     * their respective entirities.  Or, all 128 bytes of the Source, and
     * THEN all 128 bytes of the mask.  This'll give us a 32X32 pixel
     * cursor. A 64X64 pixel cursor will require 512 + 512 bytes.
     *
     * cursor hardware has "A" and "B" bitmaps
     * logic table is:
     *
     *          A       B       cursor
     *
     *          0       0       Pixel Data
     *          1       0       Pixel Data
     *          0       1       Color1
     *          1       1       Color2
     */

    /**
     * "a" bitmap = image
     * "b" bitmap = "mask".
     */

    maxbytes = qvisPriv->max_cursor / 8;
    if ((int)pCurs->bits->width <= qvisPriv->max_cursor) {	/* get # of bytes w/cursor data */ /* S003 */
	if (!(pCurs->bits->width % 8)) {	/* get # of bytes w/cursor
						 * data */
	    goodbytes = pCurs->bits->width / 8;
	} else {
	    goodbytes = ((pCurs->bits->width / 8) + 1);
	}
    } else {
	XYZ("qvisRealizeCursor-ForceToMaxBits");
	goodbytes = maxbytes;			/* force to max bytes */
    }

    a = (unsigned char *) pCurs->devPriv[pScr->myNum];
    b = ((unsigned char *) pCurs->devPriv[pScr->myNum]) + cursor_chars / 2;
    src = pCurs->bits->source;
    mask = pCurs->bits->mask;

    stride = PixmapBytePad((unsigned int)pCurs->bits->width, 1);/* S003 */

    for (j = 0; j < pCurs->bits->height && j < qvisPriv->max_cursor; ++j) {
	for (i = 0; i < goodbytes; i++) {
	    *a++ = BIT_SWAP(*src++);				/* S000 */
	    *b++ = BIT_SWAP(*mask++);				/* S000 */
	}
	a += (maxbytes - goodbytes);	/* get a & b to beginning of next row */
	b += (maxbytes - goodbytes);
	src += (stride - goodbytes);  /* and src & mask as well (padded to L) */
	mask += (stride - goodbytes);
    }
    return TRUE;
}

/*
 * Free anything allocated above
 */
Bool
qvisUnrealizeCursor(pScr, pCurs)
    ScreenPtr       pScr;
    CursorPtr       pCurs;
{
    XYZ("qvisUnrealizeCursor-entered");
    xfree(pCurs->devPriv[pScr->myNum]);
    return TRUE;
}

/*
 * Move and possibly change current sprite
 */
static void
qvisSetCursor(pScr, pCurs, x, y)
    ScreenPtr       pScr;
    CursorPtr       pCurs;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScr);
    int cursor_chars = SQUARE(qvisPriv->max_cursor) / (4 * sizeof(char));
    int             i;
    unsigned int    actual_x, actual_y;
    unsigned char  *data_to_go;

    if (pCurs == NullCursor)					/* S000 vvv */
	{
	qvisCursorOn(FALSE, pScr);
	return;
	}							/* S000 ^^^ */

    XYZ("qvisSetCursor-entered");
    qvisSetCurrentScreen();
    /* point to csr data */
    data_to_go = (unsigned char *) pCurs->devPriv[pScr->myNum];

    /*
     * load the cursor
     */

    qvisOut8(0x13c9, (qvisIn8(0x13c9) & 0xfc));	/* disable cursor */

    qvisOut8(0x83c8, 0x00);	/* set write adrs */
    if (qvisPriv->max_cursor == 64)
        qvisOut8(0x13c6, (qvisIn8(0x13c6) & 0xfc));

    for (i = 0; i < cursor_chars; ++i)
	qvisOut8(0x13c7, *data_to_go++);	/* write out the data */

    qvisOut8(0x83c8, 0x02);	/* write fg color  */
    qvisOut8(0x83c9, (unsigned char) (pCurs->foreRed >> 8));
    qvisOut8(0x83c9, (unsigned char) (pCurs->foreGreen >> 8));
    qvisOut8(0x83c9, (unsigned char) (pCurs->foreBlue >> 8));

    qvisOut8(0x83c8, 0x01);	/* write bg colr */
    qvisOut8(0x83c9, (unsigned char) (pCurs->backRed >> 8));
    qvisOut8(0x83c9, (unsigned char) (pCurs->backGreen >> 8));
    qvisOut8(0x83c9, (unsigned char) (pCurs->backBlue >> 8));

    qvisOut8(0x13c9, (qvisIn8(0x13c9) | 0x03));	/* enable cursor */

    qvisPriv->Xhot = qvisPriv->max_cursor - pCurs->bits->xhot;
    qvisPriv->Yhot = qvisPriv->max_cursor - pCurs->bits->yhot;

    /* adjust for hotspot  */
    actual_x = x + qvisPriv->Xhot;
    actual_y = y + qvisPriv->Yhot;

    /* write x coord to hardware */
    qvisOut8(0x93c8, (unsigned char) actual_x);
    qvisOut8(0x93c9, (unsigned char) (actual_x >> 8));

    /* write y coord to hardware */
    /* DAC triggers on write to 93c7, y_hi */
    qvisOut8(0x93c6, (unsigned char) actual_y);
    qvisOut8(0x93c7, (unsigned char) (actual_y >> 8));
}

/*
 * Just move current sprite
 */
static void
qvisMoveCursor(pScreen, x, y)
    ScreenPtr       pScreen;
    int             x, y;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    unsigned int    actual_x, actual_y;

    XYZ("qvisMoveCursor-entered");
    qvisSetCurrentScreen();
    /* adjust for hotspot  */
    actual_x = x + qvisPriv->Xhot;
    actual_y = y + qvisPriv->Yhot;

    /* write x coord to hardware */
    qvisOut8(0x93c8, (unsigned char) actual_x);
    qvisOut8(0x93c9, (unsigned char) (actual_x >> 8));

    /* write y coord to hardware */
    /* DAC triggers on write to 93c7, y_hi */
    qvisOut8(0x93c6, (unsigned char) actual_y);
    qvisOut8(0x93c7, (unsigned char) (actual_y >> 8));
}


/**
 * qvisCursorOn() - turns the cursor on and off
 *	on - a boolean, turn cursor on if true, off if false
 *	pScreen - which screen's cursor to turn on or off
 *
 *	This routine will be called by the multi-head code
 *	to turn the cursor off when the pointer moves to
 *	another screen.
 */
void
qvisCursorOn(on, pScreen)
    int             on;
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);

    XYZ("qvisCursorOn-entered");
    qvisSetCurrentScreen();
    if (on) {
	XYZ("qvisCursorOn-ON");
	qvisOut8(0x13c9, (qvisIn8(0x13c9) | 0x03));	/* set mode 3 (X) csr */
    } else {
	XYZ("qvisCursorOn-OFF");
	qvisOut8(0x13c9, (qvisIn8(0x13c9) & 0xfc));	/* disable cursor */
    }
}
