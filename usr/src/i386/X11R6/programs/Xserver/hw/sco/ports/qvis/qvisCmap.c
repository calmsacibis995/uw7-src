
/**
 * @(#) qvisCmap.c 11.1 97/10/22
 *
 * Copyright (C) 1991 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right
 * to use, modify, and incorporate this code into other products for purposes
 * authorized by the license agreement provided they include this notice
 * and the associated copyright notice with any such product.  The
 * information in this file is provided "AS IS" without warranty.
 *
 */

/**
 * qvisCmap.c
 *
 * Template for machine dependent colormap routines
 */

/**
 * Copyright 1991-1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 *
 */

#include "xyz.h"
#include "X.h"
#include "misc.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "colormapst.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "qvisMacros.h"
#include "qvisHW.h"
#include "qvisDefs.h"

/**
 * qvisSetColor() - set an entry in a PseudoColor colormap
 *	cmap - machine dependent colormap number
 *	index - offset into colormap
 *	r,g,b - the rgb tripple to enter in this colormap entry
 *	pScreen - pointer to X's screen struct for this screen.  Simple
 *		implementations can initially ignore this.
 *
 * NOTE: r,g,b are 16 bits.  If you have a device that uses 8 bits you
 *	should use the MOST SIGNIFICANT 8 bits.
 */
void
qvisSetColor(cmap, index, r, g, b, pScreen)
    unsigned int    cmap;	/* unused (Warning C4100) */
    unsigned int    index;
    unsigned short  r;
    unsigned short  g;
    unsigned short  b;
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);

    XYZ("qvisSetColor-entered");
    qvisSetCurrentScreen();
    if (index > (unsigned int) pScreen->visuals[0].ColormapEntries) {
	XYZ("qvisSetColor-IndexTooLarge");
	return;
    }
    qvisOut8(0x3C8, index);
    qvisIn8(0x84);
    qvisOut8(0x3C9, r >> 8);	/* 8 because qvisVisual.bitsPerRGBValue == 8 */
    qvisIn8(0x84);
    qvisOut8(0x3C9, g >> 8);	/* 8 because qvisVisual.bitsPerRGBValue == 8 */
    qvisIn8(0x84);
    qvisOut8(0x3C9, b >> 8);	/* 8 because qvisVisual.bitsPerRGBValue == 8 */
}

void
qvisRestoreColormap(
		      ScreenPtr pScreen)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    nfbScrnPriv    *devPriv = NFB_SCREEN_PRIV(pScreen);
    ColormapPtr     pmap = devPriv->installedCmap;

    XYZ("qvisRestoreColormap");
    qvisSetCurrentScreen();
    (*devPriv->LoadColormap) (pmap);
}

/*
 * XXX The 8514 code has its own LoadColormap routine written.  Should
 * we do something like this?
 *
 * It would be faster but colormap switching probably is not performance
 * critical.  We will just use nfbLoadColormap.
 */

