#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:FIconBoxI.c	1.11"
#endif

/******************************file*header********************************
 *
 * Description:
 *	This file contains the source code for the flat iconBox
 *	widget.
 */

						/* #includes go here	*/
#include <Xm/DrawP.h>
#include "DtI.h"
#include "FIconBoxI.h"

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

/**************************forward*declarations***************************
 *
 * Forward Procedure declarations.
 */
					/* private procedures		*/

/***************************private*procedures****************************
 *
 * Private Procedures
 */

/****************************public*procedures****************************
 *
 * Public Procedures
 */

/****************************procedure*header*****************************
 * ExmFIconDrawIcon-
 */
void
ExmFIconDrawIcon(Widget w, DmGlyphPtr glyph, Drawable drawable, GC gc1,
		 Boolean is_sensitive, Pixmap stipple, int x, int y)
{
    Display	*dpy = XtDisplay(w);
    Pixel	bpix;
    Pixel	fpix;

    XSetClipMask(dpy, gc1, glyph->mask);	/* Set mask and origin */
    XSetClipOrigin(dpy, gc1, x, y);
    if (glyph->depth == 1)
	    XCopyPlane(dpy, glyph->pix, drawable, gc1, 0, 0,
		    glyph->width, glyph->height, x, y, (unsigned long)1);
    else
	    XCopyArea(dpy, glyph->pix, drawable, gc1,
			    0, 0, glyph->width, glyph->height, x, y);

    if (!is_sensitive)
    {
	XSetStipple(dpy, gc1, stipple!=NULL?stipple:FPART(w).insens_pixmap);
	XSetFillStyle(dpy, gc1, FillStippled);
        XtVaGetValues(w, XmNbackground, &bpix, NULL);
	XSetForeground(dpy, gc1, bpix);
    	XFillRectangle(dpy, drawable, gc1, x, y, glyph->width, glyph->height);
	XSetFillStyle(dpy, gc1, FillSolid);
	XSetForeground(dpy, gc1, PPART(w).foreground);
    }
    XSetClipMask(dpy, gc1, None);	/* Reset mask */
}					/* end of ExmFIconDrawIcon */

/****************************procedure*header*****************************
 * ExmFIconDrawProc-
 */
void
ExmFIconDrawProc(Widget w, ExmFlatItem item, ExmFlatDrawInfo * di)
{
    Display *	dpy = XtDisplay(w);
    DmGlyphPtr	glyph = ((DmObjectPtr)IITEM(item).object_data)->fcp->glyph;
    Boolean	is_sensitive = (FITEM(item).sensitive && XtIsSensitive(w));
    int		pad = IPART(w).vpad;
    int		x, y;
    unsigned int width, height;
    int		delta;
    Dimension	lbl_w, lbl_h;
    Pixel	bpix;
    XGCValues	gcval;

    /************	Draw the glyph		************/

    /* glyph is centered horiz including highlight and shadow, if any */
    width = glyph->width +
	2 * (PPART(w).highlight_thickness + PPART(w).shadow_thickness);
    height = glyph->height +
	2 * (PPART(w).highlight_thickness + PPART(w).shadow_thickness);

    /* Compute x & y to draw highlight (if any) */
    x = ( (delta = di->width - width) > 1 ) ? di->x + (delta / 2) : di->x; 
    y = di->y;

    if (PPART(w).highlight_thickness)
    {
	if (di->item_has_focus)
	    _XmDrawSimpleHighlight(dpy, di->drawable,
				   PPART(w).highlight_GC, 
				   x, y, width, height,
				   PPART(w).highlight_thickness);
	
	/* Adjust to draw within highlight, if any */
	x += PPART(w).highlight_thickness;
	y += PPART(w).highlight_thickness;
	width -= 2 * PPART(w).highlight_thickness;
	height -= 2 * PPART(w).highlight_thickness;
    }

    if (PPART(w).shadow_thickness)
    {
	if (FITEM(item).selected)
	    _XmDrawShadows (dpy, di->drawable,
			    PPART(w).bottom_shadow_GC, PPART(w).top_shadow_GC,
			    x, y, width, height, 
			    PPART(w).shadow_thickness, XmSHADOW_OUT);
	
	/* Adjust to draw within shadow, if any */
	x += PPART(w).shadow_thickness;
	y += PPART(w).shadow_thickness;
    }

    /* Draw the icon either sensitive or insensitive. */
    ExmFIconDrawIcon(w, glyph, di->drawable, di->gc, is_sensitive, NULL, x, y);

    /************	Draw the label		************/

    if (_XmStringEmpty(FITEM(item).label))
	return;

    _XmStringExtent(FPART(w).font, FITEM(item).label, &lbl_w, &lbl_h);

    /* label is centered horiz including shadow, if any */
    width = lbl_w + 2 * PPART(w).shadow_thickness;

    /* Compute x & y to draw shadow (if any) */
    x = ( (delta = di->width - width) > 1 ) ? di->x + (delta / 2) : di->x; 
    y += glyph->height + PPART(w).shadow_thickness +
	PPART(w).highlight_thickness + pad;
    width = lbl_w + 2 * PPART(w).shadow_thickness;
    height = lbl_h + 2 * PPART(w).shadow_thickness;

    if (PPART(w).shadow_thickness)
    {
	if (FITEM(item).selected)
	    _XmDrawShadows (dpy, di->drawable,
			    PPART(w).bottom_shadow_GC, PPART(w).top_shadow_GC,
			    x, y, width, height, 
			    PPART(w).shadow_thickness, XmSHADOW_OUT);
	
	/* Adjust to draw within shadow, if any */
	x += PPART(w).shadow_thickness;
	y += PPART(w).shadow_thickness;
	width -= 2 * PPART(w).shadow_thickness;
	height -= 2 * PPART(w).shadow_thickness;
    }

    XGetGCValues(dpy, di->gc, GCForeground | GCBackground, &gcval);
    XSetForeground(dpy, di->gc, gcval.background);
    XFillRectangle(dpy, di->drawable, di->gc, x, y, width, height);
    XSetForeground(dpy, di->gc, gcval.foreground);

    _XmStringDraw(dpy, di->drawable, FPART(w).font, FITEM(item).label,
		       di->gc, x, y, width,
		       XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, NULL);
/*		       XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL); */

}					/* end of DrawItemProc */
