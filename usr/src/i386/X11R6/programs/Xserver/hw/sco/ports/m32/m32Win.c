/*
 * @(#) m32Win.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 27-Jul-93, buckm
 *	Created.
 */
/*
 * m32Win.c
 *
 * Mach-32 ValidateWindowPriv() routine.
 */

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
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
 * m32ValidateWindowPriv() - set win ops structure based on depth/visual
 */
extern nfbWinOps m32WinOps8;
extern nfbWinOps m32WinOps16;
#ifdef NOTYET
extern nfbWinOps m32WinOps24;
#endif

void
m32ValidateWindowPriv(pWin)
	WindowPtr pWin;
{
	nfbWindowPrivPtr pNfb = NFB_WINDOW_PRIV(pWin);

	switch (pWin->drawable.depth) {
	    case 8:	pNfb->ops = &m32WinOps8;  break;
	    case 16:	pNfb->ops = &m32WinOps16; break;
#ifdef NOTYET
	    case 24:	pNfb->ops = &m32WinOps24; break;
#endif
	}
}
