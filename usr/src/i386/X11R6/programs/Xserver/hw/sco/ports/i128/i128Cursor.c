/*
 * @(#) i128Cursor.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1996 The Santa Cruz Operation, Inc.
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
 * Modification History
 *
 * S000, Tue Mar 26 10:44:04 PST 1996, kylec@sco.com
 * 	Use software cursor with ibm528 dac
 *
 */


#include "X.h"
#include "Xproto.h"
#include "servermd.h"
#include "misc.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"

#ifdef usl
#include "mi/mi.h"
#endif

#include "mi/mipointer.h"
#include "mi/misprite.h"

#include "i128Defs.h"
#include "ti3025.h"
#include "ibm528.h"

static Bool i128RealizeCursor();
static Bool i128UnrealizeCursor();
static void i128SetCursor();
static void i128MoveCursor();

#define I128_HI_BYTE(b) (((b) >> 8) && 0xFF)
#define I128_LO_BYTE(b) ((b) && 0xFF)

miPointerSpriteFuncRec i128PointerFuncs = {
    i128RealizeCursor,
    i128UnrealizeCursor,
    i128SetCursor,
    i128MoveCursor,
};


static Bool i128ibm528RealizeCursor();
static Bool i128ibm528UnrealizeCursor();
static void i128ibm528SetCursor();
static void i128ibm528MoveCursor();

miPointerSpriteFuncRec i128ibm528PointerFuncs = {
    i128ibm528RealizeCursor,
    i128ibm528UnrealizeCursor,
    i128ibm528SetCursor,
    i128ibm528MoveCursor,
};

#define I128_CURSOR_MOVE(dac, x, y) { \
	dac[0x18] = 0; dac[0x1C] = x; \
	dac[0x18] = 1; dac[0x1C] = x >> 8; \
	dac[0x18] = 2; dac[0x1C] = y; \
	dac[0x18] = 3; dac[0x1C] = y >> 8; }

#define I128_CURSOR_ON(dac) { dac[0x18] = 6; dac[0x1C] |= 0x50; }
#define I128_CURSOR_OFF(dac) { \
	I128_CURSOR_MOVE(dac, -64, -64); \
        dac[0x18] = 6; dac[0x1C] &= ~0x50; }

#define I128_CURSOR_TRANSPARENT	0x00

/* 
 * Initialize the cursor and register movement routines.
 */
void
i128CursorInitialize(pScreen)
    ScreenPtr               pScreen;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
    
    switch (i128Priv->hardware.DAC_type)
    {
      case I128_DACTYPE_TIVPT:
        if(scoPointerInitialize(pScreen,
                                &i128PointerFuncs, NULL, TRUE) == 0)
        {
            FatalError("i128: cannot initialize Hardware Cursor\n");
        }
        break;
        
      case I128_DACTYPE_IBM528: /* S000 */
        if (scoPointerInitialize(pScreen,
                                 &i128ibm528PointerFuncs,
                                 NULL, TRUE) == 0)
        {
            FatalError("i128: cannot initialize Hardware Cursor\n");
        }
        break;
        
      default:
        FatalError("i128: DAC not supported.\n");
        break;
    }
}

/*
 * Realize the Cursor Image.   This routine must remove the cursor from
 * the screen if pCursor == NullCursor.
 */
Bool
i128RealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
    return TRUE;
}


/*
 * Free anything allocated above
 */
Bool
i128UnrealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
    return TRUE;
}

/*
 * Move and possibly change current sprite.   This routine must remove 
 * the cursor from the screen if pCursor == NullCursor.
 */
static void
i128SetCursor(pScreen, pCursor, x, y)
    ScreenPtr   pScreen;
    CursorPtr   pCursor;
{

     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     volatile char *dac = i128Priv->info.global->dac_regs;
     unsigned char *src;
     unsigned char *msk;
     int stride;
     int w, row, column;

#ifdef DEBUG_PRINT
     ErrorF("i128SetCursor(%x)\n", pCursor);
#endif

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     I128_CURSOR_OFF(dac);
     if (pCursor == NullCursor)
          return;

     /* Sprite origin Y */
     dac[0x18] = 4;
     dac[0x1C] = pCursor->bits->xhot;           /* Y origin */
     
     /* Sprite origin X */
     dac[0x18] = 5;
     dac[0x1C] = pCursor->bits->yhot;           /* X origin */

     /* Cursor Color0 */
     dac[0x18] = 0x23;
     dac[0x1C] = pCursor->backRed >> 8;   

     dac[0x18] = 0x24;
     dac[0x1C] = pCursor->backGreen >> 8; 

     dac[0x18] = 0x25;
     dac[0x1C] = pCursor->backBlue >> 8;

     /* Cursor Color1 */
     dac[0x18] = 0x26;
     dac[0x1C] = pCursor->foreRed >> 8;

     dac[0x18] = 0x27;
     dac[0x1C] = pCursor->foreGreen >> 8;

     dac[0x18] = 0x28;
     dac[0x1C] = pCursor->foreBlue >> 8;

     /* ? */
     dac[0x18] = 0x8;
     dac[0x1C] = 0x0;
     dac[0x18] = 0x9;
     dac[0x1C] = 0x0;

     /* Write image data 64x64 */
     dac[0x18] = 0xA;           /* Cursor RAM */
     src = pCursor->bits->source;
     msk = pCursor->bits->mask;
     stride = PixmapBytePad(pCursor->bits->width, 1);
     
     for (row = 0; row < 64; row++)
     {
          if (row >= pCursor->bits->height)
          {
               for (column = 0; column < 64; column += 8)
               {
                    dac[0x1C] = I128_CURSOR_TRANSPARENT;
                    dac[0x1C] = I128_CURSOR_TRANSPARENT;
               }
               continue;
          }
          else for (column = 0; column < 64; I128_NOOP)
          {
               int i, j;
               unsigned char s, m;
               unsigned cursor;

               s = src[column >> 3];
               m = msk[column >> 3];
               
               /* merge src and msk */
               for (i=0; i<2; i++)
               {
                    cursor = 0;
                    for (j=0; j<4; j++, column++)
                    {
                         cursor <<= 2;
                         if (column < pCursor->bits->width)
                         {
                              cursor |= (m & 0x1) << 1 | s & 0x1;
                              m >>= 1;
                              s >>= 1;
                         }
                    }
                    dac[0x1C] = cursor;
               }
          }
          msk += stride;
          src += stride;
     }
     I128_CURSOR_ON(dac);
     I128_CURSOR_MOVE(dac, x, y);
}

/*
 *  Just move current sprite
 */
static void
i128MoveCursor (pScreen, x, y)
    ScreenPtr   pScreen;
    int         x, y;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     volatile char *dac = i128Priv->info.global->dac_regs;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     I128_CURSOR_MOVE(dac, x, y);
}


/* IBM 528 DAC */

#define I128_IBM528_CURSOR_ON(dac) \
    dac[I128_DAC_IBM528_IDXHI]  = 0; \
    dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_CTRL; \
    dac[I128_DAC_IBM528_DATA]   = 0xFF;
        
#define I128_IBM528_CURSOR_OFF(dac) \
    dac[I128_DAC_IBM528_IDXHI]  = 0; \
    dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_CTRL; \
    dac[I128_DAC_IBM528_DATA]   = 0x00;
        
#define I128_IBM528_HIDE_CURSOR(dac) \
{ \
    int i;\
    dac[I128_DAC_IBM528_IDXHI] = IBM528_CURSOR_ARRAY >> 8;  \
    dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_ARRAY; \
    for (i=0; i<64*16; i++) \
        dac[I128_DAC_IBM528_DATA] = I128_CURSOR_TRANSPARENT; \
}

#define I128_IBM528_MOVE_CURSOR(dac, x, y) \
    dac[I128_DAC_IBM528_IDXHI]  = 0; \
    dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_X_LOW; \
    dac[I128_DAC_IBM528_DATA]   = x; \
    dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_X_HIGH; \
    dac[I128_DAC_IBM528_DATA]   = x >> 8; \
    dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_Y_LOW; \
    dac[I128_DAC_IBM528_DATA]   = y; \
    dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_Y_HIGH; \
    dac[I128_DAC_IBM528_DATA]   = y >> 8; 
        

/*
 * Realize the Cursor Image.   This routine must remove the cursor from
 * the screen if pCursor == NullCursor.
 */
Bool
i128ibm528RealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
    return TRUE;
}


/*
 * Free anything allocated above
 */
Bool
i128ibm528UnrealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
    return TRUE;
}

/*
 * Move and possibly change current sprite.   This routine must remove 
 * the cursor from the screen if pCursor == NullCursor.
 */
static void
i128ibm528SetCursor(pScreen, pCursor, x, y)
    ScreenPtr   pScreen;
    CursorPtr   pCursor;
{

     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     volatile char *dac = i128Priv->info.global->dac_regs;
     unsigned char *src;
     unsigned char *msk;
     int stride;
     int w, row, column;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     I128_IBM528_HIDE_CURSOR(dac);
     if (pCursor == NullCursor)
          return;

     /* Sprite origin X */
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_X_HOT; 
     dac[I128_DAC_IBM528_IDXHI] = 0; 
     dac[I128_DAC_IBM528_DATA] = pCursor->bits->xhot;
     
     /* Sprite origin Y */
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_Y_HOT;
     dac[I128_DAC_IBM528_DATA] = pCursor->bits->yhot;

     /* Cursor Color0 */
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURS_1_RED;
     dac[I128_DAC_IBM528_IDXHI] = 0; 
     dac[I128_DAC_IBM528_DATA]   = pCursor->backRed >> 8;   

     dac[I128_DAC_IBM528_IDXHI] = 0;
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURS_1_GREEN; 
     dac[I128_DAC_IBM528_DATA]   = pCursor->backGreen >> 8; 

     dac[I128_DAC_IBM528_IDXHI] = 0;
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURS_1_BLUE;
     dac[I128_DAC_IBM528_DATA]   =  pCursor->backBlue >> 8;

     /* Cursor Color1 */
     dac[I128_DAC_IBM528_IDXHI] = 0;
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURS_2_RED;
     dac[I128_DAC_IBM528_DATA]   = pCursor->foreRed >> 8;

     dac[I128_DAC_IBM528_IDXHI] = 0;
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURS_2_GREEN;
     dac[I128_DAC_IBM528_DATA]   = pCursor->foreGreen >> 8;

     dac[I128_DAC_IBM528_IDXHI] = 0;
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURS_2_BLUE;
     dac[I128_DAC_IBM528_DATA]   = pCursor->foreBlue >> 8;

     /* Write image data 64x64 */
     dac[I128_DAC_IBM528_IDXHI] = IBM528_CURSOR_ARRAY >> 8; 
     dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_ARRAY; 
     src = pCursor->bits->source;
     msk = pCursor->bits->mask;
     stride = PixmapBytePad(pCursor->bits->width, 1);

     for (row = 0; row < 64; row++)
     {
          if (row >= pCursor->bits->height)
          {
               for (column = 0; column < 64; column += 8)
               {
                    dac[I128_DAC_IBM528_DATA] = I128_CURSOR_TRANSPARENT;
                    dac[I128_DAC_IBM528_DATA] = I128_CURSOR_TRANSPARENT;
               }
               continue;
          }
          else for (column = 0; column < 64; I128_NOOP)
          {
               int i, j;
               unsigned char s, m;
               unsigned cursor;

               s = src[column >> 3];
               m = msk[column >> 3];

               /* merge src and msk */
               for (i=0; i<2; i++)
               {
                    cursor = 0;
                    for (j=0; j<4; j++, column++)
                    {
                         cursor <<= 2;
                         if (column < pCursor->bits->width)
                         {
                              cursor |= (m & 0x1) << 1 | s & 0x1;
                              m >>= 1;
                              s >>= 1;
                         }
                    }
                    dac[I128_DAC_IBM528_DATA] = cursor;
               }
          }
          msk += stride;
          src += stride;
     }
     I128_IBM528_MOVE_CURSOR(dac, x, y);
}

/*
 *  Just move current sprite
 */
static void
i128ibm528MoveCursor (pScreen, x, y)
    ScreenPtr   pScreen;
    int         x, y;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     volatile char *dac = i128Priv->info.global->dac_regs;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     I128_IBM528_MOVE_CURSOR(dac, x, y);
}
