/*
 *  @(#) wdCursor.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
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
 * wdCursor.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *      S001    Wdn 14-Oct-1992 edb@sco.com
 *              WD90C31 cursor
 *      S002    Wdn 11-Nov-1992 edb@sco.com
 *              Add wdSetSpriteColor and wdRecolorCursor
 *      S003    Wdn 03-Feb-1993 buckm@sco.com
 *              Shouldn't do any SetColor's in wdSetSpriteColor;
 *		you just have to use what FakeAllocColor gives you.
 *      S004    Fri 12-Feb-1993 buckm@sco.com
 *              Use frame buffer access to load cursor.
 *      S005    Wdn 01-Sep-1993 edb@sco.com
 *		Fix bug SCO-9-271 ( sprite drawing corrupt near bottom)
 *		The 64 bit version of the hardware cursor does not
 *		have this problem
 */

/*
 * These are the basic routines needed to implement a hardware cursor.
 * For software cursors see midispcur.c or just call scoSWCursorInit() 
 * from wdInit.c
 *
 * Note the SetCursor routine must be able to color the cursor.
 * A simpler interface has yet to be developed here.
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"
#include "servermd.h"

#ifdef usl
#include "mi/mi.h"
#endif

#include "mi/mipointer.h"
#include "mi/misprite.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wdScrStr.h"
#include "wdDefs.h"
#include "wdBitswap.h"
#include "wdBankMap.h"


static Bool wdRealizeCursor();
static Bool wdUnrealizeCursor();
       void wdSetCursor();
static void wdMoveCursor();
static void wdRecolorCursor();
static wdCursor * wdConvertCursorBits();
static void wdLoadCursorBitmap();
static void wdSetSpriteColor();

miPointerSpriteFuncRec wdPointerFuncs = {
    wdRealizeCursor,
    wdUnrealizeCursor,
    wdSetCursor,
    wdMoveCursor,
};

/* 
 * Initialize the cursor and register movement routines.
 */
void
wdCursorInitialize(pScreen)
    ScreenPtr               pScreen;
{
    if(scoPointerInitialize(pScreen, &wdPointerFuncs, TRUE) == 0)
	FatalError("Cannot initialize Hardware Cursor\n");

    pScreen->RecolorCursor = wdRecolorCursor;
}

/*
 * Realize the Cursor Image.
 */
static Bool
wdRealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
        wdCursor *wd_cursor;

#ifdef DEBUG_PRINT
	ErrorF("wdRealizeCursor(%x,%x)\n",pScreen,pCursor);
#endif /* DEBUG_PRINT */

        wd_cursor = wdConvertCursorBits( pCursor->bits );
        pCursor->devPriv[pScreen->myNum] = (pointer)wd_cursor;

        wd_cursor->primColor = pScreen->blackPixel;
        wd_cursor->secColor  = pScreen->whitePixel;
}

/*
 * Free anything allocated above
 */
static Bool
wdUnrealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
        wdCursor *wd_cursor = (wdCursor *)pCursor->devPriv[pScreen->myNum];

#ifdef DEBUG_PRINT
	ErrorF("wdUnrealizeCursor(%x,%x)\n",pScreen,pCursor);
#endif /* DEBUG_PRINT */

        xfree( wd_cursor->patAddr );
        xfree( wd_cursor );
        pCursor->devPriv[pScreen->myNum] = NULL;
        
}

/*
 * Move and possibly change current sprite.   This routine must remove 
 * the cursor from the screen if pCursor == NullCursor.
 */
 void
wdSetCursor(pScreen, pCursor, x, y)
    ScreenPtr   pScreen;
    CursorPtr   pCursor;
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
        wdCursor *wd_cursor;
#ifdef DEBUG_PRINT
	ErrorF("wdSetCursor(%x,%x,%d,%d)\n",pScreen,pCursor,x,y);
#endif /* DEBUG_PRINT */

        SELECT_CURSOR_REG_BLOCK();
        WRITE_1_REG( CURS_CNTRL, CURS_DISAB );
        if( pCursor != NULL )
        {
            wd_cursor = (wdCursor *)pCursor->devPriv[pScreen->myNum];
            wdSetSpriteColor( pScreen, wd_cursor,
                       pCursor->foreRed,pCursor->foreGreen,pCursor->foreBlue,
                       pCursor->backRed,pCursor->backGreen,pCursor->backBlue);
            if( wd_cursor != wdPriv->curCursor )
            {
		SELECT_BITBLT_REG_BLOCK();
                wdLoadCursorBitmap( wdPriv, wd_cursor );
		SELECT_CURSOR_REG_BLOCK();

                WRITE_2_REG( CURS_PATTERN       ,(wd_cursor->memAddr >>2) );
                /*                                                  -----   */
                /* One of the mysteries in the WD chip. 0 - read page 8.6.1 */
                wdPriv->curCursor = wd_cursor;
            }
            WRITE_1_REG( CURS_ORIGIN        ,wd_cursor->origin  );
            WRITE_1_REG( CURS_AUX_COLOR     , 0                 );
            WRITE_1_REG( CURS_PRIM_COLOR    ,wd_cursor->primColor);
            WRITE_1_REG( CURS_SEC_COLOR     ,wd_cursor->secColor);
            WRITE_1_REG( CURS_POS_X         , x & 0x7FF         );
            WRITE_1_REG( CURS_POS_Y         , y & 0x3FF         );
            WRITE_1_REG( CURS_CNTRL,CURS_ENAB | wd_cursor->patType |TWO_COLOR);
       }
       else
            wdPriv->curCursor = NULL;

       SELECT_BITBLT_REG_BLOCK();
}

/*
 *  Just move current sprite
 */
static void
wdMoveCursor (pScreen, x, y)
    ScreenPtr   pScreen;
    int         x, y;
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
        wdCursor *wd_cursor  = wdPriv->curCursor;
#ifdef DEBUG_PRINT
	ErrorF("wdMoveCursor(%x,%d,%d)\n",pScreen,x,y);
#endif /* DEBUG_PRINT */

       if( wd_cursor != NULL )
       {
           SELECT_CURSOR_REG_BLOCK();
           WRITE_1_REG( CURS_POS_X         , x & 0x7FF         );
           WRITE_1_REG( CURS_POS_Y         , y & 0x3FF         );
           SELECT_BITBLT_REG_BLOCK();
       }
}

/*
 * wdRecolorCursor() - set the foreground and background cursor colors.
 *                      We do not have to re-realize the cursor.
 */
static void
wdRecolorCursor(pScreen, pCursor, displayed)
        ScreenPtr pScreen;
        register CursorPtr pCursor;
        Bool displayed;
{
        wdCursor *wd_cursor = (wdCursor *)pCursor->devPriv[pScreen->myNum];
#ifdef DEBUG_PRINT
	ErrorF("wdRecolorCursor(%x,%x,%d)\n",pScreen,pCursor, displayed);
#endif /* DEBUG_PRINT */

        if( displayed )
            wdSetSpriteColor(pScreen, wd_cursor,
                    pCursor->foreRed, pCursor->foreGreen, pCursor->foreBlue,
                    pCursor->backRed, pCursor->backGreen, pCursor->backBlue );
}

static void
wdSetSpriteColor(pScreen, wd_cursor, fr,fg,fb, br,bg,bb)
        ScreenPtr pScreen;
        wdCursor *wd_cursor;
        unsigned short fr, fg, fb, br, bg, bb;
{
        xColorItem		foreGColor, backGColor;
        nfbScrnPrivPtr nfbPriv = NFB_SCREEN_PRIV(pScreen);
        ColormapPtr pmap;

#ifdef DEBUG_PRINT
	ErrorF("wdSetSpriteColor(%x,%x,%d,%d,%d,%d,%d,%d)\n",
                pScreen,wd_cursor,fr,fg,fb, br,bg,bb);
#endif /* DEBUG_PRINT */

        pmap = nfbPriv->installedCmap;
        
        foreGColor.red   = fr;
        foreGColor.green = fg;
        foreGColor.blue  = fb;
        FakeAllocColor (pmap, &foreGColor);
        backGColor.red   = br;
        backGColor.green = bg;
        backGColor.blue  = bb;
        FakeAllocColor (pmap, &backGColor);
        wd_cursor->primColor = foreGColor.pixel;
        wd_cursor->secColor  = backGColor.pixel;
        /* "free" the pixels right away, they stay loaded in device */
        FakeFreeColor(pmap, foreGColor.pixel);
        FakeFreeColor(pmap, backGColor.pixel);
}
/*
 *   Convert _CursorBits into WD 2bit deep Cursor pattern 
 *   if width > 64 or height > 64  truncate to 64 relative to 
 *   letf upper corner
 */

static wdCursor *
wdConvertCursorBits( bits )
    CursorBitsPtr bits;
{
    wdCursor * wd_cursor;
    unsigned char *srcRow, *srcPtr;
    unsigned char *mskRow, *mskPtr;
    unsigned char *patRow, *patPtr;
    unsigned char srcByte, mskByte;
    int width,height, ind, wd_stride, stride;
    short xorigin, yorigin;
    int x,y;

    wd_cursor = ( wdCursor *)xalloc( sizeof(wdCursor));
/*
 *    The 32 bit hardware cursor has a problem,        S005 vvv
 *    therefore we use the 64 bit cursor only.
 *    There is no noticable performance difference
 */
    wd_cursor->width   = 64;
    wd_cursor->height  = 64;
    wd_cursor->patType = PIX64;
    wd_cursor->patSize = 1024;
    wd_stride          = 16;                         /* S005 ^^^ */

    wd_cursor->patAddr = (unsigned char *)xalloc( wd_cursor->patSize );
    xorigin = bits->xhot;
    yorigin = bits->yhot;
    if( xorigin > wd_cursor->width ) xorigin = wd_cursor->width;
    if( yorigin > wd_cursor->height) yorigin = wd_cursor->height;
    wd_cursor->origin  = xorigin  | ( yorigin << 6 );

    width = (bits->width > wd_cursor->width) ? wd_cursor->width : bits->width;
    height= (bits->height> wd_cursor->height)? wd_cursor->height: bits->height;
    stride = PixmapBytePad( bits->width, 1);

    /*  1st byte hold transparency bits, 2nd the pattern */

    patPtr = wd_cursor->patAddr;
    for( ind = 0; ind < wd_cursor->patSize; ind+=2)
    {
        *patPtr++ = 0xFF;    /* bug in WD Doc 8.6.2.2 */
        *patPtr++ = 0x00; 
    }

    srcRow = bits->source;
    mskRow = bits->mask;
    patRow = wd_cursor->patAddr;
    for( y = 0; y < height; y++)
    {
         srcPtr = srcRow;
         mskPtr = mskRow;
         patPtr = patRow;
         for( x = 0; x < width; x+=8  )
         {
              mskByte   = BITSWAP( *mskPtr++ ); 
              srcByte   = BITSWAP( *srcPtr++ );
          /*
           *  We want source bits 1 only when the corresponding mask bit is 1 !
           */
              srcByte   &=  mskByte;
             *patPtr++  =  ~mskByte; /*The WD90C31 needs a 1 for transparency*/
             *patPtr++  =   srcByte;
         }
         srcRow +=  stride;
         mskRow +=  stride;
         patRow +=  wd_stride;
    }
    return wd_cursor;
}

static void
wdLoadCursorBitmap( wdPriv, wd_cursor )
wdScrnPrivPtr wdPriv;
wdCursor * wd_cursor;
{
	char *pDst;

	WD_MAP_AHEAD(wdPriv, wdPriv->cursorCache, wd_cursor->patSize, pDst);

        WAITFOR_WD();

	memcpy(pDst, wd_cursor->patAddr, wd_cursor->patSize);

        wd_cursor->memAddr = wdPriv->cursorCache;
}
