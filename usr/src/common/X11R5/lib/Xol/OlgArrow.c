#ifndef NOIDENT
#ident	"@(#)olg:OlgArrow.c	1.4"
#endif

/* Draw Shadow arrows */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

void
OlgDrawArrow OLARGLIST((scr, win, pInfo, x, y, width, height, flags))
    OLARG(Screen *,	scr)
    OLARG(Drawable,	win)
    OLARG(OlgAttrs *,	pInfo)
    OLARG(Position,	x)
    OLARG(Position,	y)
    OLARG(Dimension,	width)
    OLARG(Dimension,	height)
    OLGRA(OlBitMask,	flags)
{
	register _OlgDevice *pDev;
	GC centerGC, darkGC, brightGC;
	Position relX, relY;

	pDev = pInfo->pDev;

	/*  Set up the GCs according whether the arrow is pressed or not. */
	if (flags & AR_PRESSED)  {
		if (OlgIs3d())  {
			centerGC = OlgGetBg2GC(pInfo);
			darkGC = OlgGetBrightGC(pInfo);
			brightGC = OlgGetBg3GC(pInfo);
		}
		else  {
			centerGC = OlgGetFgGC(pInfo);
			darkGC = OlgGetFgGC(pInfo);
			brightGC = OlgGetFgGC(pInfo);
		}
	}
	else  {
		if (OlgIs3d())  {
			centerGC = OlgGetBg1GC(pInfo);
			darkGC = OlgGetBg3GC(pInfo);
			brightGC = OlgGetBrightGC(pInfo);
		}
		else  {
			centerGC = OlgGetBg1GC(pInfo);
			darkGC = OlgGetFgGC(pInfo);
			brightGC = OlgGetFgGC(pInfo);
		}
	}

	/*  Calculate the relative position of the arrow. The OlgDrawObject
		centers the object around the point given, so here we
		calculate the center of the rectangle in the window that
		has been given.  */
	relX = x + width / 2;
	relY = y + height / 2;

	if (flags & AR_UP)  {
		OlgDrawObject(scr, win, darkGC,
			&pDev->arrowUpDark, relX, relY);
		OlgDrawObject(scr, win, brightGC,
			&pDev->arrowUpBright, relX, relY);
		OlgDrawObject(scr, win, centerGC,
			&pDev->arrowUpCenter, relX, relY);
	}
	else if (flags & AR_DOWN)  {
		OlgDrawObject(scr, win, darkGC,
			&pDev->arrowDownDark, relX, relY);
		OlgDrawObject(scr, win, brightGC,
			&pDev->arrowDownBright, relX, relY);
		OlgDrawObject(scr, win, centerGC,
			&pDev->arrowDownCenter, relX, relY);
	}
	else if (flags & AR_RIGHT)  {
		OlgDrawObject(scr, win, darkGC,
			&pDev->arrowRightDark, relX, relY);
		OlgDrawObject(scr, win, brightGC,
			&pDev->arrowRightBright, relX, relY);
		OlgDrawObject(scr, win, brightGC,
			&pDev->arrowRightVert, relX, relY);
		OlgDrawObject(scr, win, centerGC,
			&pDev->arrowRightCenter, relX, relY);
	}
	else if (flags & AR_LEFT)  {
		OlgDrawObject(scr, win, darkGC,
			&pDev->arrowLeftDark, relX, relY);
		OlgDrawObject(scr, win, darkGC,
			&pDev->arrowLeftVert, relX, relY);
		OlgDrawObject(scr, win, brightGC,
			&pDev->arrowLeftBright, relX, relY);
		OlgDrawObject(scr, win, centerGC,
			&pDev->arrowLeftCenter, relX, relY);
	} 
	else  {
		/* error - direction flag set */
	}

	return;
}  /* end of OlgDrawArrow() */
