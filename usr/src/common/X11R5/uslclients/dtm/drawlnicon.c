#ifndef NOIDENT
#ident	"@(#)dtm:drawlnicon.c	1.6"
#endif

#include <Xm/Xm.h>
#include <FIconBoxP.h>
#include "Dtm.h"
#include "extern.h"

/*
 *************************************************************************
 * DmDrawLinkIcon - draws an icon visual given an icon glyph and string.
 ****************************procedure*header*****************************
 */
void 
DmDrawLinkIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
    Display        *dpy = XtDisplay(w);
    ExmFlatItem    item = (ExmFlatItem) client_data;
    ExmFlatDrawInfo *di = (ExmFlatDrawInfo *)call_data;
    DmObjectPtr op	= (DmObjectPtr)IITEM(item).object_data;

	/* draw the standard icon first */
    DmDrawIcon(w, item, di);

    if (op->attrs & DM_B_SYMLINK) {

	DmGlyphPtr sgp = DESKTOP_SHORTCUT(Desktop); /* shortcut glyph */
	DmGlyphPtr gp = op->fcp->glyph;
	int x, y, width;
	Dimension	pad = IPART(w).vpad;


	/* The link stipple is centered between the glyph
	 *  and the label.  From the top of the item, we
	 *  add heights of:
	 *	item glyph, highlight, shadow, half of item pad
	 */
	
	y = di->y + gp->height + 2 * PPART(w).highlight_thickness +
	    2 * PPART(w).shadow_thickness + pad/2;
	width = (gp->width / sgp->width) * sgp->width;
	x = di->x + ((int)di->width - width) / 2;
	
	XSetStipple(dpy, di->gc, sgp->pix);
	XSetFillStyle(dpy, di->gc, FillStippled);
	XSetTSOrigin(dpy, di->	gc, x, y);
	XFillRectangle(dpy, XtWindow(w), di->gc, x, y, width, sgp->height);
	XSetFillStyle(dpy, di->gc, FillSolid);
	XSetTSOrigin(dpy, di->gc, 0, 0);
    }
} /* DmDrawLinkIcon() */

