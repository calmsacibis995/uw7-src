#ifndef NOIDENT
#ident	"@(#)olg:OlgCheck.c	1.14"
#endif

/* Check Box Functions */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

/* Determine the size of the box portion of a checkbox.
 *
 * The bitmap that contains the check is larger than the actual box.
 * Calculate the size of the box from the bitmap.  The method used is
 * to find the first pixel from the top that is set in the left column.
 * The distance from this pixel to the bottom define the interior height
 * of the box.  The right-most pixel set of the bottom line is similarly
 * used to find the interior width of the box.
 */

void
_OlgCBsizeBox (pDev)
    _OlgDevice	*pDev;
{
    XImage	*image;
    Dimension	width, height;
    int		i;

    width = pDev->widthChecks / 2;
    height = pDev->heightChecks;

    image = XGetImage (DisplayOfScreen (pDev->scr), pDev->checks, width, 0,
		       width, height, 1, PixmapType);

    for (i=0; i<(int)width; i++)
    {
	if (XGetPixel (image, 0, i))
	    break;
    }
    pDev->heightCheckBox = height - i + pDev->verticalStroke * 2;

    for (i=width-1; i>=0; i--)
    {
	if (XGetPixel (image, i, height - 1))
	    break;
    }
    pDev->widthCheckBox = i + pDev->horizontalStroke * 2 + 1;

    if (OlGetGui() == OL_MOTIF_GUI)  {
		/* size of Motif toggle box is 2 points smaller
		 * than O/L checkbox in 12 point scale. Reduce, scottn...
		 */
	pDev->widthCheckBox -= 2 * pDev->horizontalStroke;
	pDev->heightCheckBox -= 2 * pDev->verticalStroke;
    }

    XDestroyImage (image);
}

/* Size a check box.
 *
 * Determine the size of a check box for the current scale.  All types of
 * check boxes have the same size.
 */

void
OlgSizeCheckBox OLARGLIST((scr, pInfo, pWidth, pHeight))
  OLARG ( Screen *,	scr )
  OLARG ( OlgAttrs *,	pInfo )
  OLARG ( Dimension *,	pWidth )
  OLGRA ( Dimension *,	pHeight )
{
    register _OlgDevice	*pDev;

    pDev = pInfo->pDev;

    /* Load the bitmaps, if we haven't already. */
    if (!pDev->checks)
    {
	_OlgGetBitmaps (scr, pInfo, OLG_CHECKS);
	_OlgCBsizeBox (pDev);
    }

    if (OlGetGui() == OL_OPENLOOK_GUI)
    {
	*pWidth = pDev->widthChecks / 2 + OlgGetHorizontalStroke (pInfo);
	*pHeight = pDev->heightChecks + OlgGetVerticalStroke (pInfo);
    }
    else
    {
	*pWidth = pDev->widthCheckBox / 2 + OlgGetHorizontalStroke (pInfo);
	*pHeight = pDev->heightCheckBox + OlgGetVerticalStroke (pInfo);
    }
}

void
OlgDrawCheckBox OLARGLIST((scr, win, pInfo, x, y, flags))
  OLARG(Screen *,	scr)
  OLARG(Drawable,	win)
  OLARG(OlgAttrs *,	pInfo)
  OLARG(Position,	x)
  OLARG(Position,	y)
  OLGRA(OlBitMask,	flags)
{
  (*_olmOlgDrawCheckBox) (scr, win, pInfo, x, y, flags);
}
