/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_cursor.c,v 3.10 1995/04/09 13:53:36 dawes Exp $ */
/*
 *
 * Copyright 1993-94 by Simon P. Cooper, New Brunswick, New Jersey, USA.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Simon P. Cooper not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Simon P. Cooper makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * SIMON P. COOPER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SIMON P. COOPER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Simon P. Cooper, <scooper@vizlab.rutgers.edu>
 *
/* $XConsortium: cir_cursor.c /main/8 1995/11/13 08:20:54 kaleb $ */

#define CIRRUS_DEBUG_CURSOR

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "xf86.h"
#include "mipointer.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"
#include "cir_driver.h"
#include "cirBlitter.h"

static Bool cirrusRealizeCursor();
static Bool cirrusUnrealizeCursor();
static void cirrusSetCursor();
static void cirrusMoveCursor();
static void cirrusRecolorCursor();

static miPointerSpriteFuncRec cirrusPointerSpriteFuncs =
{
   cirrusRealizeCursor,
   cirrusUnrealizeCursor,
   cirrusSetCursor,
   cirrusMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int cirrusCursGeneration = -1;

cirrusCurRec cirrusCur;

Bool
cirrusCursorInit(pm, pScr)
     char *pm;
     ScreenPtr pScr;
{
  cirrusCur.hotX = 0;
  cirrusCur.hotY = 0;

  if (cirrusCursGeneration != serverGeneration)
    {
      if (!(miPointerInitialize(pScr, &cirrusPointerSpriteFuncs,
				&xf86PointerScreenFuncs, FALSE)))
	return FALSE;
    }
  cirrusCursGeneration = serverGeneration;

  return TRUE;
}

void
cirrusShowCursor()
{
  /* turn the cursor on */
  outw (0x3C4, ((cirrusCur.cur_size<<2)+1)<<8 | 0x12);
}

void
cirrusHideCursor()
{
  /* turn the cursor off */
  outw (0x3C4, (0x0)<<8 | 0x12);
}

static Bool
cirrusRealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;

{
   register int i, j;
   unsigned short *pServMsk;
   unsigned short *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   int	 wsrc, wdst, h, off;
   unsigned short *ram, *curp;
   CursorBitsPtr bits = pCurs->bits;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   ram = (unsigned short *)xalloc(1024);

   memset (ram, 0, (cirrusCur.width>>2)*cirrusCur.height);

   curp = (unsigned short *)ram;
   off  = 64;

   *pPriv = (pointer) ram;

   if (!ram)
      return FALSE;

   pServSrc = (unsigned short *)bits->source;
   pServMsk = (unsigned short *)bits->mask;

   h = bits->height;
   if (h > cirrusCur.height)
      h = cirrusCur.height;

   wsrc = ((bits->width + 0x1f) >> 4) & ~0x1;		/* words per line */
   wdst = (cirrusCur.width >> 4);

   for (i = 0; i < h; i++)
     {
       for (j = 0; j < wdst; j++)
	 {
	   unsigned short m, s;

	   if (i < h && j < wsrc)
	     {
	       m = *pServMsk++;
	       s = *pServSrc++;

	       ((char *)&m)[0] = byte_reversed[((unsigned char *)&m)[0]];
	       ((char *)&m)[1] = byte_reversed[((unsigned char *)&m)[1]];

	       ((char *)&s)[0] = byte_reversed[((unsigned char *)&s)[0]];
	       ((char *)&s)[1] = byte_reversed[((unsigned char *)&s)[1]];

	       *curp	   = s & m;
	       *(curp+off) = m;
	     }
	   else
	     {
	       *curp       = 0x0;
	       *(curp+off) = 0x0;
	     }
	   curp++;
	 }
     }

   return TRUE;
}

static Bool
cirrusUnrealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   pointer priv;

   if (pCurs->bits->refcnt <= 1 &&
       (priv = pCurs->bits->devPriv[pScr->myNum]))
     {
       xfree(priv);
       pCurs->bits->devPriv[pScr->myNum] = 0x0;
     }
   return TRUE;
}

cirrusLoadCursorToCard (pScr, pCurs, x, y)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int x, y;
{
  int   index = pScr->myNum;
  int   l, w, winw, count, rshift, lshift;
  unsigned long *pDstM, *pSrcM;
  unsigned long dAddr;

  if (!xf86VTSema)
      return;

  /* check for blitter operation: must not meddle with ram when blitter is
     running...
   */

  if (cirrusBLTisBusy)
    WAITUNTILFINISHED ();
      
  /* Calculate the number of words to transfer to the board */
  count = ((cirrusCur.cur_size == 0) ? 256 : 1024) >> 2;

  /* Onboard address to start writing the cursor backwards */
  dAddr = cirrusCur.cur_addr;
    
  CIRRUSSETSINGLE(dAddr);

  pDstM = (unsigned long *)(CIRRUSSINGLEBASE() + dAddr);
  pDstM += count - 1;

  pSrcM = (unsigned long *)pCurs->bits->devPriv[index];
  pSrcM += count - 1;

  if (x > 0) x = 0;
  if (y > 0) y = 0;
  
  if (x == 0 && y == 0)
    {
      for (l=count;l;l--)
	{
	  *pDstM-- = *pSrcM--;
	}
      
    }
  else
    {
      unsigned long *pDstS = pDstM - 32;
      unsigned long *pSrcS = pSrcM - 32;
      int t = -y;
      
      winw = cirrusCur.width >> 5;
      
      if (x == 0)
	{
	  for (l=0; l < cirrusCur.height; l++)
	    {
	      for (w=0; w < winw; w++)
		{
		  if (l<t)
		    {
		      *pDstM-- = 0;
		      *pDstS-- = 0;
		    }
		  else
		    {
		      *pDstM-- = *pSrcM--;
		      *pDstS-- = *pSrcS--;
		    }
		}		      
	    }
	}
      else
	{
	  unsigned char *bpDstS = (unsigned char *)pDstS + 3;
	  unsigned char *bpDstM = (unsigned char *)pDstM + 3;
	  unsigned char *bpSrcS = (unsigned char *)pSrcS + 3;
	  unsigned char *bpSrcM = (unsigned char *)pSrcM + 3;
	  int bskip = (-x) >> 3;
	  
	  lshift = (-x) & 0x7;
	  rshift = 8 - lshift;
	  winw <<= 2;
	  
	  for (l=0; l < cirrusCur.height; l++)
	    {
	      unsigned char leftoverM = 0;
	      unsigned char leftoverS = 0;
	      unsigned char temp;
       
	      if (l<t)
		{
		  for (w=0; w < winw; w++)
		    {
		      *bpDstM-- = 0;
		      *bpDstS-- = 0;
		    }
		}
	      else
		{
		  for (w=0; w < winw; w++)
		    {
		      if (w < bskip)
			{
			  *bpDstM-- = 0;
			  *bpDstS-- = 0;
			}
		      else
			{
			  temp = *bpSrcM--;
			  *bpDstM-- = (temp << lshift) | leftoverM;
			  leftoverM = temp >> rshift;
			  
			  temp = *bpSrcS--;
			  *bpDstS-- = (temp << lshift) | leftoverS;
			  leftoverS = temp >> rshift;
			}
		    }
		  bpSrcM -= bskip;
		  bpSrcS -= bskip;
		}
	      
	    }
	}
    }
}

static void
cirrusLoadCursor(pScr, pCurs, x, y)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int x, y;
{
   int   index = pScr->myNum;
   int   i, count;
   unsigned long *pDst, *pSrc;
   unsigned long dAddr;

   if (!xf86VTSema)
      return;

   if (!pCurs)
      return;

   /* Remember the cursor currently loaded into this cursor slot */
   cirrusCur.pCurs = pCurs;

   cirrusHideCursor ();

   /* Select the cursor index */
   outw (0x3C4, (cirrusCur.cur_select << 8) | 0x13);

   cirrusLoadCursorToCard (pScr, pCurs, x, y);

   cirrusRecolorCursor (pScr, pCurs, 1);

   /* position cursor */
   cirrusMoveCursor (pScr, x, y);

   /* Turn it on */
   cirrusShowCursor ();
}

static void
cirrusSetCursor(pScr, pCurs, x, y, generateEvent)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int   x, y;
     Bool  generateEvent;

{
  if (!pCurs)
    return;

  cirrusCur.hotX = pCurs->bits->xhot;
  cirrusCur.hotY = pCurs->bits->yhot;

  cirrusLoadCursor(pScr, pCurs, x, y);
}

void
cirrusRestoreCursor(pScr)
     ScreenPtr pScr;
{
   int index = pScr->myNum;
   int x, y;

   miPointerPosition(&x, &y);

   cirrusLoadCursor(pScr, cirrusCur.pCurs, x, y);
}

static void
cirrusMoveCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{

  if (!xf86VTSema)
      return;

  x -= vga256InfoRec.frameX0 + cirrusCur.hotX;
  y -= vga256InfoRec.frameY0 + cirrusCur.hotY;

  if (x < 0 || y < 0)
    {
      cirrusLoadCursorToCard (pScr, cirrusCur.pCurs, x, y);
      cirrusCur.skewed = 1;
    }
  else if (cirrusCur.skewed)
    {
      cirrusLoadCursorToCard (pScr, cirrusCur.pCurs, 0, 0);
      cirrusCur.skewed = 0;
    }
  
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  if (XF86SCRNINFO(pScr)->modes->Flags & V_DBLSCAN)
      y *= 2;

  if (XF86SCRNINFO(pScr)->modes->CrtcHAdjusted) {
      /* 5434 palette-clock doubling mode; cursor is squashed but */
      /* get at least the position right. */
      x /= 2;
      if (XF86SCRNINFO(pScr)->modes->CrtcVAdjusted)
          y /= 2;
  }

  /* Your eyes do not deceive you - the low order bits form part of the
   * the INDEX
   */

  outw (0x3C4, (x << 5) | 0x10);
  outw (0x3C4, (y << 5) | 0x11);
}

static void
cirrusRecolorCursor(pScr, pCurs, displayed)
     ScreenPtr pScr;
     CursorPtr pCurs;
     Bool displayed;
{
   unsigned short red, green, blue;
   int shift;
   int i;
   VisualPtr pVisual;
   unsigned char sr12;

   if (!xf86VTSema)
       return;

   /* Find the PseudoColour or TrueColor visual for the colour mapping
    * function
    */

   for (i = 0, pVisual = pScr->visuals; i < pScr->numVisuals; i++, pVisual++)
     {
       if ((pVisual->class == PseudoColor) || (pVisual->class == TrueColor))
	 break;
     }

   if (i == pScr->numVisuals)
     {
       ErrorF ("CIRRUS: Failed to find a visual for mapping hardware cursor colours\n");
       return;
     }

   shift = 16 - pVisual->bitsPerRGBValue;

   outb (0x3c4, 0x12);          /* SR12 allows access to DAC extended colors */
   sr12 = inb (0x3c5);
                                /* Disable the cursor and allow access to
                                   the hidden DAC registers */
   outb (0x3c5, (sr12 & 0xfe) | 0x02);

   red   = pCurs->backRed;
   green = pCurs->backGreen;
   blue  = pCurs->backBlue;

   pScr->ResolveColor (&red, &green, &blue, pVisual);

   outb (0x3c8, 0x00);          /* DAC color 256 */
   outb (0x3c9, (red>>shift));
   outb (0x3c9, (green>>shift));
   outb (0x3c9, (blue>>shift));

   red   = pCurs->foreRed;
   green = pCurs->foreGreen;
   blue  = pCurs->foreBlue;

   pScr->ResolveColor (&red, &green, &blue, pVisual);

   outb (0x3c8, 0x0f);          /* DAC color 257 */
   outb (0x3c9, (red>>shift));
   outb (0x3c9, (green>>shift));
   outb (0x3c9, (blue>>shift));

                               /* Restore the state of SR12 */
   outw (0x3c4, (sr12 <<8) | 0x12);
}

void
cirrusWarpCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
  miPointerWarpCursor(pScr, x, y);
  xf86Info.currentScreen = pScr;
}

void
cirrusQueryBestSize(class, pwidth, pheight)
     int class;
     short *pwidth;
     short *pheight;
{
  if (*pwidth > 0)
    {
      switch (class)
	{
	case CursorShape:
	  if (*pwidth > cirrusCur.width)
	    *pwidth  = cirrusCur.width;
	  if (*pheight > cirrusCur.height)
	    *pheight = cirrusCur.height;
	  break;

	default:
	  (void) mfbQueryBestSize(class, pwidth, pheight);
	  break;
	}
    }
}
