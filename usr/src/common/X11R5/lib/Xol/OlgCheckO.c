#ifndef NOIDENT
#ident	"@(#)olg:OlgCheckO.c	1.6"
#endif

/* Check Box Functions */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

/* Draw a check box.
 *
 * Draw a check box of the type indicated.  Valid types are:
 *
 *	CB_CHECKED
 *	CB_DIM
 *
 * The area under the box is assumed to be cleared to the background.
 */

void
_OloOlgDrawCheckBox OLARGLIST((scr, win, pInfo, x, y, flags))
  OLARG(Screen *,	scr)
  OLARG(Drawable,	win)
  OLARG(OlgAttrs *,	pInfo)
  OLARG(Position,	x)
  OLARG(Position,	y)
  OLGRA(OlBitMask,	flags)
{
    _OlgDevice	*pDev = pInfo->pDev;
    Position	xBox, yBox;
    Dimension	width, height;
    GC		gc;

    /* Load the bitmaps, if we haven't already. */
    if (!pDev->checks)
    {
	_OlgGetBitmaps (scr, pInfo, OLG_CHECKS);
	_OlgCBsizeBox (pDev);
    }

    /* The checks bitmap contains two subimages.  The left one contains the
     * actual check; the right one contains the mask where the check isn't.
     * Both are the same size.
     */
    width = pDev->widthChecks / 2;
    height = pDev->heightChecks;

    /* If the check box is dim, we have to muck with the GCs to add a
     * stipple.
     */
    if (flags & CB_DIM)
    {
	if (OlgIs3d())
	{
	    gc = OlgGetBrightGC (pInfo);
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);

	    gc = OlgGetBg3GC (pInfo);
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	}
	else
	{
	    gc = OlgGetFgGC (pInfo);
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	}
    }

    /* Draw the box */
    yBox = y + pDev->heightChecks + OlgGetVerticalStroke (pInfo) -
	pDev->heightCheckBox;

    if (OlgIs3d())
    {
	OlgDrawRBox (scr, win, pInfo, x, yBox, pDev->widthCheckBox,
		     pDev->heightCheckBox, &pDev->rect3UL, &pDev->rect3UR,
		     &pDev->rect3LL, &pDev->rect3LR, RB_UP);
    }
    else
    {
	OlgDrawBox (scr, win, pInfo, x, yBox, pDev->widthCheckBox,
		    pDev->heightCheckBox, False);
    }

    /* Draw the check, if present */
    if (flags & CB_CHECKED)
    {
	/* Determine upper left of interior of box */
	xBox = x + OlgGetHorizontalStroke (pInfo);

	/* If the thing is also dim, then make the check be a clip mask.
	 * The performance of this approach is perhaps sub-optimal, but it
	 * seems better than creating a temporary pixmap, copying the check,
	 * ANDing in the stipple pattern, copying the result to the window,
	 * and freeing the temporary pixmap.  Besides, it should be a
	 * relatively infrequent operation.
	 */
	gc = OlgIs3d() ? OlgGetBg3GC (pInfo) : OlgGetFgGC (pInfo);
	if (flags & CB_DIM)
	{
	    XSetClipMask (DisplayOfScreen (scr), gc, pDev->checks);
	    XSetClipOrigin (DisplayOfScreen (scr), gc, xBox, y);
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->inactiveStipple);

	    XFillRectangle (DisplayOfScreen (scr), win, gc, xBox, y,
			    width, height);

	    XSetClipMask (DisplayOfScreen (scr), gc, None);
	    XSetClipOrigin (DisplayOfScreen (scr), gc, 0, 0);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillSolid);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			   FillSolid);
	}
	else
	{
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->checks);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	    XSetTSOrigin (DisplayOfScreen (scr), gc, xBox, y);

	    XFillRectangle (DisplayOfScreen (scr), win, gc, xBox, y,
			    width, height);

	    XSetFillStyle (DisplayOfScreen (scr), gc, FillSolid);
	    XSetTSOrigin (DisplayOfScreen (scr), gc, 0, 0);
	}

	/* Fill the interior of the button where the check isn't.  More
	 * nonsense if the background is a pixmap.
	 */
	gc = OlgGetBg1GC (pInfo);
	if (pInfo->flags & OLG_BGPIXMAP)
	{
	    XSetClipMask (DisplayOfScreen (scr), gc, pDev->checks);
	    XSetClipOrigin (DisplayOfScreen(scr), gc, xBox - width, y);

	    XFillRectangle (DisplayOfScreen (scr), win, gc, xBox, y,
			    width, height);

	    XSetClipMask (DisplayOfScreen (scr), gc, None);
	    XSetClipOrigin (DisplayOfScreen (scr), gc, 0, 0);
	}
	else
	{
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->checks);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	    XSetTSOrigin (DisplayOfScreen (scr), gc, xBox - width, y);

	    XFillRectangle (DisplayOfScreen (scr), win, gc, xBox, y,
			    width, height);

	    XSetFillStyle (DisplayOfScreen (scr), gc, FillSolid);
	    XSetTSOrigin (DisplayOfScreen (scr), gc, 0, 0);
	}
    }
    else
    {
	Dimension	hStroke, vStroke;

	/* The box is not checked.  Fill the interior */
	gc = OlgGetBg1GC (pInfo);
	hStroke = OlgGetHorizontalStroke (pInfo);
	vStroke = OlgGetVerticalStroke (pInfo);
	XFillRectangle (DisplayOfScreen (scr), win, gc,
			x + hStroke, yBox + vStroke,
			pDev->widthCheckBox - 2 * hStroke,
			pDev->heightCheckBox - 2 * vStroke);

	/* Clean up any GCs that were polluted */
	if (flags & CB_DIM)
	    if (OlgIs3d())
	    {
		XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			       FillSolid);
		XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			       FillSolid);
	    }
	    else
		XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			       FillSolid);
    }
}

