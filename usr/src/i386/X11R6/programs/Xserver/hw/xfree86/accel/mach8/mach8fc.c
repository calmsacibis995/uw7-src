/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach8/mach8fc.c,v 3.3 1995/01/28 17:00:29 dawes Exp $ */
/*
 * Copyright 1992 by Kevin E. Martin, Chapel Hill, North Carolina.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Kevin E. Martin not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Kevin E. Martin makes no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 * 
 * KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * 
 */

/*
 * Extract from s3fcach.c
 * Adapted to Mach8, Hans Nasten. (nasten@everyware.se).
 */
/* $XConsortium: mach8fc.c /main/3 1995/11/12 18:00:16 kaleb $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"cfb.h"
#include	"misc.h"
#include        "xf86.h"
#include	"windowstr.h"
#include	"gcstruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"xf86bcache.h"
#include	"xf86fcache.h"
#include	"mach8.h"
#include	"regmach8.h"


static unsigned long FontAge;
#define NEXT_FONT_AGE  ++FontAge

/*
 * When using the 8514 compatible read mask, the read mask
 * is rotated right 1 bit compared to the write mask.
 */

unsigned char mach8cachemask[ 8 ] = { 0x01, 0x02, 0x04, 0x08,
				      0x10, 0x20, 0x40, 0x80
};
unsigned char mach8cachemaskswapped[ 8 ] = { 0x02, 0x04, 0x08, 0x10,
				             0x20, 0x40, 0x80, 0x01
};

/*
 * Move the glyphs to the screen using the GE.
 */
static void
Domach8CPolyText8(x, y, count, chars, fentry, pGC, pBox)
     int   x, y, count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
     GCPtr pGC;
     BoxPtr pBox;
{
   int   gHeight;
   int   w = fentry->w;
   int blocki = 255;
   unsigned short height = 0;	/* Cache for height, width and		 */
   unsigned short width = 0;	/* readmask registers.			 */
   unsigned short pmsk = 0;	/* ( Out instructions _are_ expensive ). */

   for (;count > 0; count--, chars++) {
      CharInfoPtr pci;
      short xoff;

      pci = fentry->pci[*chars];
      if (pci != NULL) {
	 gHeight = GLYPHHEIGHTPIXELS(pci);
	 if (gHeight) {
	    if (*chars / 32 != blocki) {
	       bitMapBlockPtr block;

	       blocki = *chars / 32;
	       if( ( block = fentry->fblock[blocki] ) == NULL) {
		  /*
		   * Reset the GE context to a known state before
		   * calling the xf86loadfontblock function.
		   */
		  WaitQueue(8);
		  outw(MULTIFUNC_CNTL, SCISSORS_T | 0);
		  outw(MULTIFUNC_CNTL, SCISSORS_L | 0);
		  outw(MULTIFUNC_CNTL, SCISSORS_R | 1023);
		  outw(MULTIFUNC_CNTL, SCISSORS_B | 1023);
		  outw(RD_MASK, 0xff);
		  outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
		  outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
		  outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
		  xf86loadFontBlock(fentry, blocki);
		  block = fentry->fblock[blocki];
		  /*
		   * Restore the GE context.
		   */
		  WaitQueue(9);
		  outw(MULTIFUNC_CNTL, SCISSORS_L | (short)pBox->x1);
		  outw(MULTIFUNC_CNTL, SCISSORS_T | (short)pBox->y1);
		  outw(MULTIFUNC_CNTL, SCISSORS_R | (short)(pBox->x2 - 1));
		  outw(MULTIFUNC_CNTL, SCISSORS_B | (short)(pBox->y2 - 1));
		  outw(FRGD_COLOR, (short)pGC->fgPixel);
		  outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
		  outw(FRGD_MIX, FSS_FRGDCOL | mach8alu[pGC->alu]);
		  outw(BKGD_MIX, BSS_BKGDCOL | MIX_DST);
		  outw(WRT_MASK, (short)pGC->planemask);
		  height = width = pmsk = 0; /* Invalidate register caches. */
	       }
	       WaitQueue(2);
	       outw(CUR_Y, block->y);
	       /*
		* Is the readmask altered ?
		*/
	       if( mach8cachemaskswapped[block->id] != pmsk ) {
		 pmsk = mach8cachemaskswapped[block->id];
		 outw(RD_MASK, pmsk);
	       }
	       xoff = block->x;
	       block->lru = NEXT_FONT_AGE;
   	    }

	    WaitQueue(6);
	    outw(CUR_X, (short) (xoff + (*chars & 0x1f) * w));
	    outw(DESTX_DIASTP,
		  (short)(x + pci->metrics.leftSideBearing));
	    outw(DESTY_AXSTP, (short)(y - pci->metrics.ascent));
	    /*
	     * Need to update width register ?
	     */
	    if( (short)(GLYPHWIDTHPIXELS(pci)) != width) {
	      width = (short)(GLYPHWIDTHPIXELS(pci));
	      outw(MAJ_AXIS_PCNT, width - 1);
	    }
	    /*
	     * How about the height register ?
	     */
	    if( (short)(gHeight) != height ) {
	      height = (short)(gHeight);
	      outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | height - 1);
	    }
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
	 }
	 x += pci->metrics.characterWidth;
      }
   }

}

/*
 * Set the hardware scissors to match the clipping rectangles and
 * call the glyph output routine.
 */
void mach8GlyphWrite( x, y, count, chars, fentry, pGC, pBox, numRects )
int x, y, count, numRects;
unsigned char *chars;
CacheFont8Ptr fentry;
GCPtr pGC;
BoxPtr pBox;

{

   WaitQueue(5);
   outw(FRGD_COLOR, (short)pGC->fgPixel);
   outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
   outw(FRGD_MIX, FSS_FRGDCOL | mach8alu[pGC->alu]);
   outw(BKGD_MIX, BSS_BKGDCOL | MIX_DST);
   outw(WRT_MASK, (short)pGC->planemask);

   for (; --numRects >= 0; ++pBox) {
      WaitQueue(4);
      outw(MULTIFUNC_CNTL, SCISSORS_L | (short)pBox->x1);
      outw(MULTIFUNC_CNTL, SCISSORS_T | (short)pBox->y1);
      outw(MULTIFUNC_CNTL, SCISSORS_R | (short)(pBox->x2 - 1));
      outw(MULTIFUNC_CNTL, SCISSORS_B | (short)(pBox->y2 - 1));

      Domach8CPolyText8(x, y, count, chars, fentry, pGC, pBox);
   }

   WaitQueue(8);
   outw(MULTIFUNC_CNTL, SCISSORS_T | 0);
   outw(MULTIFUNC_CNTL, SCISSORS_L | 0);
   outw(MULTIFUNC_CNTL, SCISSORS_R | 1023);
   outw(MULTIFUNC_CNTL, SCISSORS_B | 1023);
   outw(RD_MASK, 0xff);
   outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);

}
