#ifndef NOIDENT
#ident	"@(#)olg:OlgShadow.c	1.7"
#endif

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/Olg.h>

#define DARK_GC(p)	(OlgIs3d() ? OlgGetBg3GC(p) : OlgGetBg2GC(p))
#define BRIGHT_GC(p)	(OlgIs3d() ? OlgGetBrightGC(p) : OlgGetBg2GC(p))

extern void	_OlgDrawBorderShadow OL_ARGS((Screen *, Window, OlgAttrs *,
	OlDefine, Dimension, Position, Position, Dimension,
				Dimension, GC, GC));

/* should we handle 2d case?	*/
extern void
OlgDrawBorderShadow OLARGLIST((scr, win, info, typ, thickness, x, y, w, h))
	OLARG( Screen *,	scr)
	OLARG( Window,		win)
	OLARG( OlgAttrs *,	info)
	OLARG( OlDefine,	typ)		/* shadow_type		*/
	OLARG( Dimension,	thickness)	/* shadow_thickness	*/
	OLARG( Position,	x)
	OLARG( Position,	y)
	OLARG( Dimension,	w)		/* width		*/
	OLGRA( Dimension,	h)		/* height		*/
{

	_OlgDrawBorderShadow(scr, win, info, typ, thickness, x, y, w, h,
		BRIGHT_GC(info), DARK_GC(info));
				
} /* end of OlgDrawBorderShadow */

#undef DARK_GC
#undef BRIGHT_GC

extern void
_OlgDrawBorderShadow OLARGLIST((scr, win, info, typ, thickness, x, y, w, h,
						lightGC, darkGC))
	OLARG( Screen *,	scr)
	OLARG( Window,		win)
	OLARG( OlgAttrs *,	info)
	OLARG( OlDefine,	typ)		/* shadow_type		*/
	OLARG( Dimension,	thickness)	/* shadow_thickness	*/
	OLARG( Position,	x)
	OLARG( Position,	y)
	OLARG( Dimension,	w)		/* width		*/
	OLARG( Dimension,	h)		/* height		*/
	OLARG( GC,		lightGC)	/* upper left GC	*/
	OLGRA( GC,		darkGC)		/* lower right GC	*/
{
	XPoint		darks[12], brights[12];
	Cardinal	num_sets = 0;
	Dimension	tw,	/* thickness (in pixel) in horit direction*/
			th,	/* thickness (in pixel) in verti direction*/
			tw2,	/* 2 * tw				  */
			th2;	/* 2 * th				  */

		/* may be a warning here...				*/
	if (thickness == 0 || scr == NULL || win == None)
		return;

	if (typ == OL_SHADOW_ETCHED_IN ||	/* darks, brights	*/
	    typ == OL_SHADOW_ETCHED_OUT)	/* brights, darks	*/
	{
		tw2 = th2 = thickness;
		tw  = tw2 / 2;
		th  = th2 / 2;
	}
	else if (typ == OL_SHADOW_IN ||		/* darks, brights	*/
		 typ == OL_SHADOW_OUT)		/* brights, darks	*/
	{
		tw = th = thickness;
		/* don't care about tw2 and th2 */
	}
	else
	{
			/* may be a warning here...			*/
		return;
	}

	num_sets = 1;

	darks[0].x = x;			darks[0].y = y;
	darks[1].x = x;			darks[1].y = y + h;
	darks[2].x = x + tw;		darks[2].y = y + h - th;
	darks[3].x = x + tw;		darks[3].y = y + th;
	darks[4].x = x + w - tw;	darks[4].y = y + th;
	darks[5].x = x + w;		darks[5].y = y;

	brights[0] = darks[4];
	brights[1] = darks[5];
	brights[2].x = x + w;		brights[2].y = y + h;
	brights[3] = darks[1];
	brights[4] = darks[2];
	brights[5].x = x + w - tw;	brights[5].y = y + h - th;

	if (typ == OL_SHADOW_ETCHED_IN ||	/* darks, brights	*/
	    typ == OL_SHADOW_ETCHED_OUT)	/* brights, darks	*/
	{
		num_sets = 2;

		darks[6] = darks[4];
		darks[7] = brights[5];
		darks[8] = darks[2];
		darks[9].x = x + tw2; 	   darks[9].y = y + h - th2;
		darks[10].x = x + w - tw2; darks[10].y = y + h - th2;
		darks[11].x = x + w - tw2; darks[11].y = y + th2;

		brights[6] = darks[3];
		brights[7] = darks[2];
		brights[8] = darks[9];
		brights[9].x = x + tw2;	   brights[9].y = y + th2;
		brights[10] = darks[11];
		brights[11] = darks[4];
	}
	XFillPolygon(
		DisplayOfScreen(scr), win,
		darkGC,
		(typ == OL_SHADOW_ETCHED_IN || typ == OL_SHADOW_IN) ?
				darks : brights,
		6, Nonconvex, CoordModeOrigin
	);
	XFillPolygon(
		DisplayOfScreen(scr), win,
		lightGC,
		(typ == OL_SHADOW_ETCHED_IN || typ == OL_SHADOW_IN) ?
				brights : darks,
		6, Nonconvex, CoordModeOrigin
	);
	if (num_sets == 2)
	{
		XFillPolygon(
			DisplayOfScreen(scr), win,
			darkGC,
			(typ == OL_SHADOW_ETCHED_IN) ?
					&darks[6] : &brights[6],
			6, Nonconvex, CoordModeOrigin
		);
		XFillPolygon(
			DisplayOfScreen(scr), win,
			lightGC,
			(typ == OL_SHADOW_ETCHED_IN) ?
					&brights[6] : &darks[6],
			6, Nonconvex, CoordModeOrigin
		);
	}

} /* end of OlgDrawBorderShadow */
