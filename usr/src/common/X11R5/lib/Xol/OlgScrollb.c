#ifndef NOIDENT
#ident	"@(#)olg:OlgScrollb.c	1.11"
#endif

/* Scrollbar functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>

void
OlgDrawAnchor OLARGLIST((scr, win, pInfo, x, y, width, height, isPressed))
    OLARG(Screen *,	scr)
    OLARG(Drawable,	win)
    OLARG(OlgAttrs *,	pInfo)
    OLARG(Position,	x)
    OLARG(Position,	y)
    OLARG(Dimension,	width)
    OLARG(Dimension,	height)
    OLGRA(Boolean,	isPressed)
{
    XFillRectangle (DisplayOfScreen (scr), win, (OlgIs3d() && isPressed) ?
		        OlgGetBg2GC (pInfo) : OlgGetBg1GC (pInfo),
		    x, y, width, height);
    OlgDrawBox (scr, win, pInfo, x, y, width, height, isPressed);

    if (!OlgIs3d() && isPressed)
    {
	unsigned	hInset, vInset;

	hInset = pInfo->pDev->horizontalStroke * 2;
	vInset = pInfo->pDev->verticalStroke * 2;

	XFillRectangle (DisplayOfScreen (scr), win, OlgGetBg2GC (pInfo),
			x + hInset, y + vInset,
			width - (hInset << 1), height - (vInset << 1));
    }
}

void
OlgDrawScrollbar OLARGLIST((w, pInfo))
    OLARG( register Widget,	w)
    OLGRA( OlgAttrs *,		pInfo)
{
	(*_olmOlgDrawScrollbar)(w, pInfo);
}

void
OlgUpdateScrollbar OLARGLIST((w, pInfo, flags))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLGRA( OlBitMask,	flags)
{
	(*_olmOlgUpdateScrollbar)(w, pInfo, flags);
}

void
OlgSizeScrollbarAnchor OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
	(*_olmOlgSizeScrollbarAnchor)(w, pInfo, pWidth, pHeight);
}

void
OlgSizeScrollbarElevator OLARGLIST((w, pInfo, type, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( OlDefine,	type)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
	(*_olmOlgSizeScrollbarElevator)(w, pInfo, type, pWidth, pHeight);
}
