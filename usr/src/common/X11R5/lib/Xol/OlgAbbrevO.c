#ifndef NOIDENT
#ident	"@(#)olg:OlgAbbrevO.c	1.4"
#endif

/* Abbreviated Menu Button Functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

/* Draw an abbreviated menu button.
 *
 * Draw an abbreviated menu button in either the normal or selected modes.
 * The type of button drawn is specified by the flag:
 *
 *	AM_NORMAL
 *	AM_SELECTED
 *	AM_WINDOW - abbreviated window button
 */

void
_OloOlgDrawAbbrevMenuB OLARGLIST((scr, win, pInfo, x, y, type))
    OLARG(Screen *,	scr)
    OLARG(Drawable,	win)
    OLARG(OlgAttrs *	,pInfo)
    OLARG(Position,	x)
    OLARG(Position,	y)
    OLGRA(OlBitMask,	type)
{
    register _OlgDevice	*pDev;
    _OlgDesc		*ul, *ur, *ll, *lr;
    Dimension		width, height;

    /* Get the size of the button */
    OlgSizeAbbrevMenuB (scr, pInfo, &width, &height);

    /* Draw outer box */
    pDev = pInfo->pDev;
    if (OlgIs3d())
    {
	ul = &pDev->rect3UL;
	ur = &pDev->rect3UR;
	ll = &pDev->rect3LL;
	lr = &pDev->rect3LR;
    }
    else
    {
	ul = &pDev->rect2UL;
	ur = &pDev->rect2UR;
	ll = &pDev->rect2LL;
	lr = &pDev->rect2LR;
    }

    OlgDrawFilledRBox (scr, win, pInfo, x, y, width, height, ul, ur, ll, lr,
		       (type & AM_NORMAL || !OlgIs3d()) ? FB_UP : 0);
    OlgDrawRBox (scr, win, pInfo, x, y, width, height, ul, ur, ll, lr,
		 (type & AM_NORMAL) ? RB_UP : 0);

    /* If the button is selected and 2-D, then fill the interior with the
     * foreground color.
     */
    if (!OlgIs3d() && (type & AM_SELECTED))
    {
	Position	insetX, insetY;
	Dimension	insetWidth, insetHeight;

	insetX = x + pDev->rect2OrigX;
	insetY = y + pDev->rect2OrigY;
	insetWidth = width - pDev->rect2CornerX - pDev->rect2OrigX;
	insetHeight = height - pDev->rect2CornerY - pDev->rect2OrigY;

	XFillRectangle (DisplayOfScreen (scr), win, OlgGetFgGC (pInfo),
			insetX, insetY, insetWidth, insetHeight);
    }

    /* Draw the triangle in the box */
    if (type & AM_WINDOW)  {
	(void) OlgDrawWindowMark(scr, win, pInfo, x, y, width, height,
			    (type & AM_SELECTED) ? MM_INVERT : 0);
    }
    else  {
        (void) OlgDrawMenuMark (scr, win, pInfo, x, y, width, height,
			    MM_DOWN | MM_CENTER | ((type & AM_SELECTED) ?
			        MM_INVERT : 0));
    }
}  /* end of _OloOlgDrawAbbrevMenuB() */
