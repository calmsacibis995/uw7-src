#ifndef NOIDENT
#ident	"@(#)olg:OlgAbbrev.c	1.9"
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

/* Determine the size of an abbreviated menu mark
 *
 * Calculate the correct size of an abbreviated menu mark for the
 * given scale and screen.  This routine is currently correct only
 * for the 12 point scale.
 */
/* ARGSUSED */
void
OlgSizeAbbrevMenuB OLARGLIST((scr, pInfo, pWidth, pHeight))
    OLARG(Screen *,	scr)
    OLARG(OlgAttrs *,	pInfo)
    OLARG(Dimension *,	pWidth)
    OLGRA(Dimension *,	pHeight)
{
    if (OlgIs3d())
    {
	*pWidth = OlScreenPointToPixel(OL_HORIZONTAL, 16, scr);
	*pHeight = OlScreenPointToPixel(OL_VERTICAL, 15, scr);
    }
    else
    {
	*pWidth = OlScreenPointToPixel(OL_HORIZONTAL, 17, scr);
	*pHeight = OlScreenPointToPixel(OL_VERTICAL, 16, scr);
    }
}

void
OlgDrawWindowMark OLARGLIST ((scr, win, pInfo, x, y, width, height, flags))
  OLARG( Screen *,      scr )
  OLARG( Drawable,      win )
  OLARG( OlgAttrs *,    pInfo )
  OLARG( Position,      x )
  OLARG( Position,      y )
  OLARG( Dimension,     width )
  OLARG( Dimension,     height )
  OLGRA( OlBitMask,      flags )
{
	GC dotsGC;
	XRectangle dots[3];
	Dimension _2points = OlScreenPointToPixel(OL_HORIZONTAL, 2, scr);

	if (!OlgIs3d())  {
		if (flags & MM_INVERT)
			dotsGC = OlgGetBg1GC(pInfo);
		else
			dotsGC = OlgGetFgGC(pInfo);
	}
	else
		dotsGC = OlgGetBg3GC(pInfo);

	/*  Use rectangles because Arcs don't work reliably for such small
	    circles.  */
	dots[0].x = (short) (x + (width/2) - (_2points*2) - (_2points/2));
	dots[0].y = (short) (y + height - (Dimension)
			OlScreenPointToPixel(OL_VERTICAL, 5, scr));
	dots[0].width = (unsigned short) _2points;
	dots[0].height = (unsigned short) OlScreenPointToPixel(OL_VERTICAL, 2, scr);

	dots[2] = dots[1] = dots[0];

	dots[1].x = dots[0].x + _2points * 2;
	dots[2].x = dots[1].x + _2points * 2;

	XFillRectangles(XDisplayOfScreen(scr), win, dotsGC, dots, 3);
}  /* end of OlgDrawWindowMark() */


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
OlgDrawAbbrevMenuB OLARGLIST((scr, win, pInfo, x, y, type))
    OLARG(Screen *,	scr)
    OLARG(Drawable,	win)
    OLARG(OlgAttrs *,	pInfo)
    OLARG(Position,	x)
    OLARG(Position,	y)
    OLGRA(OlBitMask,	type)
{
	(*_olmOlgDrawAbbrevMenuB) (scr, win, pInfo, x, y, type);
}  /* end of OlgDrawAbbrevMenuB() */
