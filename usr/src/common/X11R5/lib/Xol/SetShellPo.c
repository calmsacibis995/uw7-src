/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olmisc:SetShellPo.c	1.2"
#endif


#include "string.h"

#include "X11/Intrinsic.h"
#include "X11/StringDefs.h"
#include "X11/Shell.h"

#include "Xol/OpenLook.h"


#define BIGGEST_NUMBER_S	"4294967295"	/* largest ulong */

/**
 ** OlSetShellPosition()
 **/

static void		FreeGeometryCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));

void
#if	OlNeedFunctionPrototypes
OlSetShellPosition (
	Widget			w,
	Position		default_x,
	Position		default_y
)
#else
OlSetShellPosition (w, default_x, default_y)
	Widget			w;
	Position		default_x;
	Position		default_y;
#endif
{
	String			geometry;
	String			p;

	char			buf[
			/* width  */	sizeof(BIGGEST_NUMBER_S)
			/* x      */  + 1
			/* height */  + sizeof(BIGGEST_NUMBER_S)
			/* {+-}   */  + 1
			/* X      */  + sizeof(BIGGEST_NUMBER_S)
			/* {+-}   */  + 1
			/* Y      */  + sizeof(BIGGEST_NUMBER_S)
			/* null   */  + 1
				];

	int			x	= default_x;
	int			y	= default_y;
	int			flags	= (XValue|(x < 0? XNegative : 0))
					| (YValue|(y < 0? YNegative : 0));

	unsigned int		width	= 0;
	unsigned int		height	= 0;


	XtVaGetValues (w, XtNgeometry, (XtArgVal)&geometry, (String)0);
	if (geometry) {
		flags = XParseGeometry(geometry, &x, &y, &width, &height);

		/*
		 * The user can override the program, so if the user
		 * gave both x and y, there's nothing for us to do.
		 */
		if (flags & XValue && flags & YValue)
			return;
	}

#if	defined(__STDC__)
#define SIGN(Z) (flags & Z ## Negative? "" : "+")
#else
#define SIGN(Z) (flags & Z/**/Negative? "" : "+")
#endif

	*(p = buf) = 0;
	if (flags & (WidthValue|HeightValue))
		p += sprintf(p, "%ldx%ld", width, height);
	if (flags & (XValue|YValue))
		p += sprintf(p, "%s%ld%s%ld", SIGN(X), x, SIGN(Y), y);

	geometry = XtNewString(buf);
	XtVaSetValues (w, XtNgeometry, (XtArgVal)geometry, (String)0);

	XtAddCallback (w, XtNdestroyCallback, FreeGeometryCB, geometry);

	return;
}

static void
#if	OlNeedFunctionPrototypes
FreeGeometryCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
FreeGeometryCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	XtFree ((String)client_data);
	return;
}
