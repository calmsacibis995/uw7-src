/*
 *	@(#)ctWin.c	11.1	10/22/97	12:35:07
 *	@(#) ctWin.c 59.1 96/11/04 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 * ctWin.c
 *
 * Template for machine dependent ValidateWindowPriv() routine
 */

#ident "@(#) $Id: ctWin.c 59.1 96/11/04 "

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

#include "ctDefs.h"

/*
 * CT(ValidateWindowPriv)() - set win ops structure based on depth/visual
 *
 * This routine sets the win ops pointer in the nfb window priv.
 * If you are supporting multiple depths/visuals you have to
 * test here and set ops to the correct version.
 */
extern nfbWinOps CT(WinOps);

void
CT(ValidateWindowPriv)(pWin)
	WindowPtr pWin;
{
	(NFB_WINDOW_PRIV(pWin))->ops = &CT(WinOps);
/*
 * If you are supporting multiple depths/visuals then check out the
 * code below.
 */
#ifdef NOTDEF
	if ( pWin->drawable.depth == 8 )
		{
		(NFB_WINDOW_PRIV(pWin))->ops = &CT(WinOps8);
		}
	else
		{
		(NFB_WINDOW_PRIV(pWin))->ops = &CT(WinOps24);
		}
#endif
}
