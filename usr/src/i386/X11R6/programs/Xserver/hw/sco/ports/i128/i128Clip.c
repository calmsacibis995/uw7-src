/*
 *	@(#) i128Clip.c 11.1 97/10/22
 *
 *      Copyright (C) The Santa Cruz Operation, 1991-1994.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 *
 * Modification History
 *
 * S000, 14-Jan-95, kylec
 * 	created
 */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "i128Defs.h"
#include "i128Procs.h"

void
i128SetClipRegions(
                   BoxRec *pbox,
                   int nbox,
                   DrawablePtr pDraw)
{
     VOLATILE i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
     unsigned long xy1, xy2;

     if (nbox > 0)
     {
          xy1 = I128_XY(pbox->x1, pbox->y1);
          xy2 = I128_XY(pbox->x2 - 1, (pbox->y2 - 1));
          i128Priv->clip = *pbox;
     }
     else
     {
          xy1 = I128_XY(0, 0);
          xy2 = I128_XY(i128Priv->mode.bitmap_width, i128Priv->mode.bitmap_height);
          i128Priv->clip.x1 = i128Priv->clip.y1 = 0;
          i128Priv->clip.x2 = i128Priv->mode.bitmap_width;
          i128Priv->clip.y2 = i128Priv->mode.bitmap_height;
     }
     
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->clip_top_left = xy1;
     i128Priv->engine->clip_bottom_right = xy2;

}
