/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)flat:ColorChip.c	1.3"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>
#include <Xol/ColorChip.h>

/*
 * These routines are used to draw simple rectangular ``color chips''.
 *
 * WARNING: Only one GC is used, so all chips must have the same depth
 * and display.
 *
 * MORE: Remove this severe restriction.
 */

/*
 * Local data:
 */

	/*
	 * We use a single, private GC because it changes so often.
	 */
static XGCValues	ColorGCV         = { 0 };
static GC		ColorGC          = 0;
static Cardinal		ColorGC_RefCount = 0;

/**
 ** _OlCreateColorChip()
 **/

void
#if	OlNeedFunctionPrototypes
_OlCreateColorChip (
	Widget			w
)
#else
_OlCreateColorChip (w)
	Widget			w;
#endif
{
	ColorGC_RefCount++;
	return;
} /* _OlCreateColorChip */

/**
 ** _OlDestroyColorChip()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDestroyColorChip (
	Widget			w
)
#else
_OlDestroyColorChip (w)
	Widget			w;
#endif
{
	if (!ColorGC_RefCount)

		OlVaDisplayErrorMsg(	XtDisplay(w),
					OleNfileColorChip,
					OleTmsg1,
					OleCOlToolkitError,
					OleMfileColorChip_msg1);
		/*NOTREACHED*/

	if (!--ColorGC_RefCount)
		if (ColorGC) {
			XFreeGC (XtDisplayOfObject(w), ColorGC);
			ColorGC = 0;
		}
	return;
} /* _OlDestroyColorChip */

/**
 ** _OlDrawColorChip()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDrawColorChip (
	Screen *		screen,
	Window			window,
	OlgAttrs *		attrs,	/*NOTUSED*/
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	XtPointer		label
)
#else
_OlDrawColorChip (screen, window, attrs, x, y, width, height, label)
	Screen *		screen;
	Window			window;
	OlgAttrs *		attrs;
	Position		x;
	Position		y;
	Dimension		width;
	Dimension		height;
	XtPointer		label;
#endif
{
	Display *		display	= DisplayOfScreen(screen);

	_OlColorChipLabel *	color	= (_OlColorChipLabel *)label;

	unsigned long		gcvm	= 0;


	if (!ColorGC || ColorGCV.foreground != color->pixel) {
		ColorGCV.foreground = color->pixel;
		gcvm |= GCForeground;
	}
	if (color->insensitive) {
		if (!ColorGC || ColorGCV.fill_style != FillStippled) {
			ColorGCV.fill_style = FillStippled;
			gcvm |= GCFillStyle;
		}
	} else {
		if (!ColorGC || ColorGCV.fill_style != FillSolid) {
			ColorGCV.fill_style = FillSolid;
			gcvm |= GCFillStyle;
		}
	}
	if (!ColorGC) {
		ColorGCV.stipple = attrs->pDev->inactiveStipple;
		gcvm |= GCStipple;
	}

	if (!ColorGC)
		ColorGC = XCreateGC(display, window, gcvm, &ColorGCV);
	else if (gcvm)
		XChangeGC (display, ColorGC, gcvm, &ColorGCV);

	XFillRectangle (display, window, ColorGC, x, y, width, height);

	return;
} /* _OlDrawColorChip */
