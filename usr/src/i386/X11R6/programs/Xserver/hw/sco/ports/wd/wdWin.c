/*
 *  @(#) wdWin.c 11.1 97/10/22
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
 * wdWin.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *	S001	Tue 09-Feb-1993	buckm@sco.com
 *              add 15,16,24 bit ops.
 */

#include "X.h"
#include "Xproto.h"
#include "pixmap.h"
#include "gcstruct.h"
#include "colormap.h"
#include "windowstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbProcs.h"

/*
 * wdValidateWindowPriv() - set win ops structure based on depth/visual
 *
 * This routine sets the win ops pointer in the nfb window priv.
 * If you are supporting multiple depths/visuals you have to
 * test here and set ops to the correct version.
 */
extern nfbWinOps wdWinOps, wdWinOps15, wdWinOps16, wdWinOps24;

void
wdValidateWindowPriv(pWin)
	WindowPtr pWin;
{
	register nfbWindowPrivPtr pNfb = NFB_WINDOW_PRIV(pWin);

	switch (pWin->drawable.depth) {
	    case 8:	pNfb->ops = &wdWinOps;   break;
	    case 15:	pNfb->ops = &wdWinOps15; break;
	    case 16:	pNfb->ops = &wdWinOps16; break;
	    case 24:	pNfb->ops = &wdWinOps24; break;
	}
}
