#ifndef NOIDENT
#ident	"@(#)olg:OlgAbbrevM.c	1.5"
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
_OlmOlgDrawAbbrevMenuB OLARGLIST((scr, win, pInfo, x, y, type))
    OLARG(Screen *,	scr)
    OLARG(Drawable,	win)
    OLARG(OlgAttrs *	,pInfo)
    OLARG(Position,	x)
    OLARG(Position,	y)
    OLGRA(OlBitMask,	type)
{
    register _OlgDevice	*pDev;
    Dimension		width, height;
    Dimension	thickness;

    /* Get the size of the button */
    OlgSizeAbbrevMenuB (scr, pInfo, &width, &height);

    pDev = pInfo->pDev;

    /* Draw the outer box */
    thickness = OlScreenPointToPixel(OL_HORIZONTAL, 2, scr);
    OlgDrawBorderShadow (scr, win, pInfo,
                       (type & AM_SELECTED) ? OL_SHADOW_IN : OL_SHADOW_OUT,
			thickness,
			x, y, width, height);

    /* If the button is selected and 2-D, then fill the interior with the
     * foreground color.
     */
    if (!OlgIs3d()) {
	Position	insetX, insetY;
	Dimension	insetWidth, insetHeight;
	Dimension	_3points = OlScreenPointToPixel(OL_HORIZONTAL, 3, scr);
	Dimension	_3Vpoints = OlScreenPointToPixel(OL_VERTICAL, 3, scr);
	GC		gc;

	insetX = x + _3points;
	insetY = y + _3Vpoints;
	insetWidth = width - (_3points*2);
	insetHeight = height - (_3Vpoints*2);

	if (type & AM_SELECTED)
		gc = OlgGetFgGC(pInfo);
	else
		gc = OlgGetBg1GC(pInfo);

	XFillRectangle (DisplayOfScreen (scr), win, gc,
			insetX, insetY, insetWidth, insetHeight);
    }

    if (type & AM_WINDOW)  {
    /* Draw the ... in the box */
	(void) OlgDrawWindowMark(scr, win, pInfo, x, y, width, height,
			    (type & AM_SELECTED) ? MM_INVERT : 0);
    }
    else  {
    /* Draw the triangle in the box */
        (void) OlgDrawMenuMark (scr, win, pInfo, x, y, width, height,
			    MM_DOWN | MM_CENTER | ((type & AM_SELECTED) ?
			        MM_INVERT : 0));
    }
}  /* end of _OlmOlgDrawAbbrevMenuB() */
