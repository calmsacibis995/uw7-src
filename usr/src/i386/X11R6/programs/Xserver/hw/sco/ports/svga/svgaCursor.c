/*
 * @(#)svgaCursor.c 11.1
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
 * svgaCursor.c
 *
 * Template for hardware cursor routines.  If you are using the
 * software cursor, ignore these routines.   
 */

/*
 * These are the basic routines needed to implement a hardware cursor.
 * For software cursors see midispcur.c or just call scoSWCursorInit() 
 * from svgaInit.c
 *
 * Note miReColorCursor works by calling UnRealize, Realize and Display.
 * Hence, your DisplayCursor routine must be able change the colors.
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"
#include "input.h"

#include "mi/mipointer.h"
#include "mi/misprite.h"

static Bool svgaRealizeCursor();
static Bool svgaUnrealizeCursor();
static void svgaSetCursor();
static void svgaMoveCursor();

miPointerSpriteFuncRec svgaPointerFuncs = {
    svgaRealizeCursor,
    svgaUnrealizeCursor,
    svgaSetCursor,
    svgaMoveCursor,
};


/* 
 * Initialize the cursor and register movement routines.
 */
void
svgaCursorInitialize(pScreen)
    ScreenPtr               pScreen;
{
    if(scoPointerInitialize(pScreen, &svgaPointerFuncs, NULL, TRUE) == 0)
	FatalError("Cannot initialize Hardware Cursor\n");
}

/*
 * Realize the Cursor Image.   This routine must remove the cursor from
 * the screen if pCursor == NullCursor.
 */
Bool
svgaRealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
#ifdef DEBUG_PRINT
	ErrorF("svgaRealizeCursor(%x,%x)\n",pScreen,pCursor);
#endif /* DEBUG_PRINT */
}

/*
 * Free anything allocated above
 */
Bool
svgaUnrealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor )
{
#ifdef DEBUG_PRINT
	ErrorF("svgaUnrealizeCursor(%x,%x)\n",pScreen,pCursor);
#endif /* DEBUG_PRINT */
}

/*
 * Move and possibly change current sprite.   This routine must remove 
 * the cursor from the screen if pCursor == NullCursor.
 */
static void
svgaSetCursor(pScreen, pCursor, x, y)
    ScreenPtr   pScreen;
    CursorPtr   pCursor;
{
#ifdef DEBUG_PRINT
	ErrorF("svgaSetCursor(%x,%x,%d,%d)\n",pScreen,pCursor,x,y);
#endif /* DEBUG_PRINT */
}

/*
 *  Just move current sprite
 */
static void
svgaMoveCursor (pScreen, x, y)
    ScreenPtr   pScreen;
    int         x, y;
{
#ifdef DEBUG_PRINT
	ErrorF("svgaMoveCursor(%x,%d,%d)\n",pScreen,x,y);
#endif /* DEBUG_PRINT */
}
