/*
 *	@(#) ifbWin.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * ifbWin.c
 *
 * ifb ValidateWindowPriv() routine
 */

#include "X.h"
#include "Xproto.h"
#include "pixmap.h"
#include "gcstruct.h"
#include "colormap.h"
#include "windowstr.h"

#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"
#include "nfbProcs.h"

/*
 * ifbValidateWindowPriv() - set win ops structure based on depth/visual
 */
extern nfbWinOps ifbWinOps1;
extern nfbWinOps ifbWinOps8;
extern nfbWinOps ifbWinOps16;
extern nfbWinOps ifbWinOps32;

void
ifbValidateWindowPriv(pWin)
	WindowPtr pWin;
{
	register nfbWindowPrivPtr pNfb = NFB_WINDOW_PRIV(pWin);

	switch (pWin->drawable.bitsPerPixel) {
	    case 1:	pNfb->ops = &ifbWinOps1;  break;
	    case 8:	pNfb->ops = &ifbWinOps8;  break;
	    case 16:	pNfb->ops = &ifbWinOps16; break;
	    case 32:	pNfb->ops = &ifbWinOps32; break;
	}
}
