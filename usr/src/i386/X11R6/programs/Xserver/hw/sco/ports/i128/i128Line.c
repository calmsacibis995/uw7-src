/*
 * @(#) i128Line.c 11.1 97/10/22
 *
 * Copyright (C) 1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S000, 07-Jan-95, kylec
 *	create (large portions copied from p9000)
 */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "gen/genProcs.h"
#include "i128Defs.h"
#include "i128Procs.h"

void
i128PolyZeroPtPtSegs(
                     GC *gc,
                     DrawablePtr pDraw,
                     BoxRec *line,
                     int nlines)
{
     nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(gc);
     i128Private *i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
     unsigned long cmd;
     BoxRec prev;

     cmd = (I128_OPCODE_LINE |
            i128Priv->rop[pGCPriv->rRop.alu] |
            I128_CMD_CLIP_IN | 
            I128_CMD_SOLID |
            I128_CMD_NO_LAST);

     if (gc->capStyle != CapNotLast)
     {
         cmd &= ~I128_CMD_NO_LAST;
     }

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->cmd = cmd;
     i128Priv->engine->foreground = pGCPriv->rRop.fg;
     i128Priv->engine->plane_mask = 
          I128_CONVERT(I128_mode.pixelsize, pGCPriv->rRop.planemask);

     while (nlines--)
     {
          I128_WAIT_UNTIL_READY(i128Priv->engine);
          i128Priv->engine->xy0 = I128_XY(line->x1, line->y1);
          i128Priv->engine->xy1 = I128_XY(line->x2, line->y2);
          line++;
     }

}


void
i128SolidZeroSeg(
                 GCPtr pGC,
                 DrawablePtr pDraw,
                 int signdx,
                 int signdy,
                 int axis,
                 int x1,
                 int y1,
                 int e,
                 int e1,
                 int e2,
                 int len)
{

     VOLATILE i128Private *i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
     nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
     unsigned long alu, planemask, fg, raster, status;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->foreground = pGC->fgPixel;
     i128Priv->engine->plane_mask =
          I128_CONVERT(i128Priv->mode.pixelsize, pGC->planemask);

#if USE_LINE
     i128Priv->engine->cmd = (I128_OPCODE_LINE |
                              i128Priv->rop[pGC->alu] |
                              I128_CMD_CLIP_IN | 
                              I128_CMD_SOLID);
     
     while (len--)
     {
          I128_WAIT_UNTIL_READY(i128Priv->engine);
          i128Priv->engine->xy0 =  I128_XY (x1, y1);
          i128Priv->engine->xy1 =  I128_XY (x1, y1);

#else
     i128Priv->engine->xy3 = I128_DIR_LR_TB;
     i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
     i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                              i128Priv->rop[pGC->alu] |
                              I128_CMD_CLIP_IN | 
                              I128_CMD_SOLID);
     
     while (len--)
     {
          I128_WAIT_UNTIL_READY(i128Priv->engine);
          I128_BLIT(0, 0, x1, y1, 1, 1, I128_DIR_LR_TB);          
#endif

          if (e > 0)
          {
               x1 += signdx;
               y1 += signdy;
               e += e2;
          }
          else
          {
               if (axis == X_AXIS)
                    x1 += signdx;
               else
                    y1 += signdy;
               e += e1;
          }
     }

}
