/* $XConsortium: i128Cursor.c /main/1 1995/12/09 15:31:28 kaleb $ */
/*
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robin Cutshaw not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Robin Cutshaw makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ROBIN CUTSHAW DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ROBIN CUTSHAW BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/i128/i128Cursor.c,v 3.0 1995/12/07 07:24:02 dawes Exp $ */

#include "i128.h"
#include "i128reg.h"
#include "i128Cursor.h"

static Bool i128UnrealizeCursor();
static void i128SetCursor();
extern Bool i128TiRealizeCursor();
extern void i128TiCursorOn();
extern void i128TiCursorOff();
extern void i128TiLoadCursor();
extern void i128TiMoveCursor();

static miPointerSpriteFuncRec i128TiPointerSpriteFuncs =
{
   i128TiRealizeCursor,
   i128UnrealizeCursor,
   i128SetCursor,
   i128TiMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int i128CursGeneration = -1;
static CursorPtr i128SaveCursors[MAXSCREENS];
static Bool useSWCursor = FALSE;

extern int i128hotX, i128hotY;


Bool
i128CursorInit(pm, pScr)
     char *pm;
     ScreenPtr pScr;
{
   i128hotX = 0;
   i128hotY = 0;
   i128BlockCursor = FALSE;
   i128ReloadCursor = FALSE;
   
   if (i128CursGeneration != serverGeneration) {
      if (!(miPointerInitialize(pScr, &i128TiPointerSpriteFuncs,
				&xf86PointerScreenFuncs, FALSE)))
         return FALSE;
      i128CursGeneration = serverGeneration;
   }

   return TRUE;
}

void
i128ShowCursor()
{
   if (useSWCursor) 
      return;

   i128TiCursorOn();
}

void
i128HideCursor()
{
   if (useSWCursor) 
      return;

   i128TiCursorOff();
}

static Bool
i128UnrealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   pointer priv;

   if (pCurs->bits->refcnt <= 1 &&
       (priv = pCurs->bits->devPriv[pScr->myNum]))
      xfree(priv);
   return TRUE;
}


static void
i128SetCursor(pScr, pCurs, x, y, generateEvent)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int   x, y;
     Bool  generateEvent;

{
   int index = pScr->myNum;

   if (!pCurs)
      return;

   if (useSWCursor) 
      return;

   i128hotX = pCurs->bits->xhot;
   i128hotY = pCurs->bits->yhot;
   i128SaveCursors[index] = pCurs;

   if (!i128BlockCursor)
      i128TiLoadCursor(pScr, pCurs, x, y);
   else
      i128ReloadCursor = TRUE;
}

void
i128RestoreCursor(pScr)
     ScreenPtr pScr;
{
   int index = pScr->myNum;
   int x, y;

   if (useSWCursor) 
      return;

   i128ReloadCursor = FALSE;
   miPointerPosition(&x, &y);
   i128TiLoadCursor(pScr, i128SaveCursors[index], x, y);
}

void
i128RepositionCursor(pScr)
     ScreenPtr pScr;
{
   int x, y;

   if (useSWCursor) 
      return;

   miPointerPosition(&x, &y);
   i128TiMoveCursor(pScr, x, y);
}

void
i128WarpCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   if (xf86VTSema) {
      /* Wait for vertical retrace */
#ifdef WORKWORKWORK
      VerticalRetraceWait();
#endif
   }
   miPointerWarpCursor(pScr, x, y);
   xf86Info.currentScreen = pScr;
}

void 
i128QueryBestSize(class, pwidth, pheight, pScreen)
     int class;
     unsigned short *pwidth;
     unsigned short *pheight;
     ScreenPtr pScreen;
{
   if (*pwidth > 0) {
      switch (class) {
         case CursorShape:
	    if (*pwidth > 64)
	       *pwidth = 64;
	    if (*pheight > 64)
	       *pheight = 64;
	    break;
         default:
	    mfbQueryBestSize(class, pwidth, pheight, pScreen);
	    break;
      }
   }
}
