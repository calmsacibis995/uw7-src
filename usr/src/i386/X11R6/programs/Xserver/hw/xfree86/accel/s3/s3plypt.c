/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3plypt.c,v 3.4 1995/01/28 17:02:22 dawes Exp $ */
/************************************************************

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

Modified for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)

KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

********************************************************/


/*
 * Modified by Amancio Hasty and Jon Tombs
 * 
 */
/* $XConsortium: s3plypt.c /main/5 1995/11/12 19:07:05 kaleb $ */


#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "cfb.h"
#include "cfb16.h"
#include "cfb32.h"
#include "cfbmskbits.h"
#include "misc.h"
#include "xf86.h"
#include "s3.h"
#include "regs3.h"

#define isClipped(c,ul,lr)  ((((c) - (ul)) | ((lr) - (c))) & ClipMask)

void
s3PolyPoint(pDrawable, pGC, mode, npt, pptInit)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int   mode;
     int   npt;
     xPoint *pptInit;
{
   register long pt;
   register long c1, c2;
   register unsigned long ClipMask = 0x80008000;
   register long *ppt;
   RegionPtr cclip;
   int   nbox;
   register int i;
   register BoxPtr pbox;
   int   off;
   cfbPrivGCPtr devPriv;
   xPoint *pptPrev;


   if (!xf86VTSema)
   {
      switch (s3InfoRec.bitsPerPixel) {
      case 8:
	 cfbPolyPoint(pDrawable, pGC, mode, npt, pptInit);
         break;
      case 16:
	 cfb16PolyPoint(pDrawable, pGC, mode, npt, pptInit);
         break;
      case 32:
	 cfb32PolyPoint(pDrawable, pGC, mode, npt, pptInit);
	 break;
      }
      return;
   }

   devPriv = (cfbPrivGC *) (pGC->devPrivates[cfbGCPrivateIndex].ptr);
   if (pGC->alu == GXnoop)
      return;
   cclip = devPriv->pCompositeClip;
   if ((mode == CoordModePrevious) && (npt > 1)) {
      for (pptPrev = pptInit + 1, i = npt - 1; --i >= 0; pptPrev++) {
	 pptPrev->x += (pptPrev - 1)->x;
	 pptPrev->y += (pptPrev - 1)->y;
      }
   }
   off = *((int *)&pDrawable->x);
   off -= (off & 0x8000) << 1;

   BLOCK_CURSOR;
   WaitQueue16_32(4,6);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | s3alu[pGC->alu]);
   S3_OUTW32(WRT_MASK, pGC->planemask);
   S3_OUTW32(FRGD_COLOR, pGC->fgPixel);
   S3_OUTW(MAJ_AXIS_PCNT, 0);

   for (nbox = REGION_NUM_RECTS(cclip), pbox = REGION_RECTS(cclip);
	--nbox >= 0;
	pbox++) {
      c1 = *((long *)&pbox->x1) - off;
      c2 = *((long *)&pbox->x2) - off - 0x00010001;
      for (ppt = (long *)pptInit, i = npt; --i >= 0;) {
	 pt = *ppt++;
	 if (!isClipped(pt, c1, c2)) {
	    WaitQueue(3);
	    S3_OUTW(CUR_X, (short)(intToX(pt) + pDrawable->x));
	    S3_OUTW(CUR_Y, (short)(intToY(pt) + pDrawable->y));
	    S3_OUTW(CMD, CMD_LINE | DRAW | LINETYPE | PLANAR | WRTDATA);
	 }
      }
   }

   WaitQueue(2);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}
