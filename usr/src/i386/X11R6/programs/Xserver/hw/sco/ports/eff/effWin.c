/*
 *	@(#) effWin.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *
 */
/*
 * effWin.c
 *
 * Template for machine dependent ValidateWindowPriv() routine
 */

#include "X.h"
#include "Xproto.h"
#include "pixmap.h"
#include "gcstruct.h"
#include "colormap.h"
#include "windowstr.h"
/*
#include "nfb.h"
*/
#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"
#include "nfbProcs.h"

/*
 * effValidateWindowPriv() - set win ops structure based on depth/visual
 *
 * This routine sets the win ops pointer in the nfb window priv.
 * If you are supporting multiple depths/visuals you have to
 * test here and set ops to the correct version.
 */
extern nfbWinOps effWinOps;

void
effValidateWindowPriv(pWin)
	WindowPtr pWin;
{
	(NFB_WINDOW_PRIV(pWin))->ops = &effWinOps;
/*
 * If you are supporting multiple depths/visuals then check out the
 * code below.
 */
#ifdef NOTDEF
	if ( pWin->drawable.depth == 8 )
		(NFB_WINDOW_PRIV(pWin))->ops = &effWinOps8;
	else
		(NFB_WINDOW_PRIV(pWin))->ops = &effWinOps24;
#endif
}
