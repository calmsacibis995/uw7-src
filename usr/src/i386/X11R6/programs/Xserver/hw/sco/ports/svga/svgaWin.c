/*
 * @(#)svgaWin.c 11.1
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
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
 * svgaWin.c
 *
 * Template for machine dependent ValidateWindowPriv() routine
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
 * svgaValidateWindowPriv() - set win ops structure based on depth/visual
 *
 * This routine sets the win ops pointer in the nfb window priv.
 * If you are supporting multiple depths/visuals you have to
 * test here and set ops to the correct version.
 */
extern nfbWinOps svgaWinOps, svgaWinOps16, svgaWinOps24;

void
svgaValidateWindowPriv(pWin)
     WindowPtr pWin;
{
#ifdef NOTDEF
  (NFB_WINDOW_PRIV(pWin))->ops = &svgaWinOps;
#else
  /*
   * If you are supporting multiple depths/visuals then check out the
   * code below.
   */
  switch (pWin->drawable.depth)
    {
    case 8:
      (NFB_WINDOW_PRIV(pWin))->ops = &svgaWinOps;
      break;
    case 16:
      (NFB_WINDOW_PRIV(pWin))->ops = &svgaWinOps16;
      break;
    case 24:
      (NFB_WINDOW_PRIV(pWin))->ops = &svgaWinOps24;
      break;
    }
#endif
}
