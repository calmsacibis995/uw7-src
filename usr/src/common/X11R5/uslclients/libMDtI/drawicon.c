#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:drawicon.c	1.11"
#endif

#include <stdio.h>
#include "FIconBoxP.h"	/* for ExmFlatDrawInfo, ExmFlatItem */
#include "DtI.h"

/****************************procedure*header*****************************
 *   DmDrawIconGlyph- Draw the glyph for an item in the FlatIconBox
 *  
 *	INPUTS: FIconBox widget
 *		ExmFlatItem from drawProc CB
 *		ExmFlatDrawInfo from drawProc CB
 *		glyph
 *		x,y position for glyph
 *		FLH MORE: x and y are used directly, no shadow
 *		or hightlight are drawn.  If the item is not sensitive
 *		it is drawn with a stippled mask.
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DmDrawIconGlyph(Widget w, ExmFlatItem item, ExmFlatDrawInfo *di,
		DmGlyphPtr glyph, WidePosition x, WidePosition y)

{
    Display	*dpy = XtDisplay(w);
    Boolean	is_sensitive = (FITEM(item).sensitive && XtIsSensitive(w));



    /************ Draw the glyph ************/

    /* Set tile for insensitive item (for glyph and label). */
    ExmFIconDrawIcon(w, glyph, di->drawable, di->gc, is_sensitive, NULL, x, y);

}   /* end of DmDrawIconGlyph */

void
DmDrawIconLabel(Widget w, ExmFlatItem item, ExmFlatDrawInfo *di,
		WidePosition x, WidePosition y, Dimension width)
{
    Display *dpy = XtDisplay(w);

    _XmStringDrawImage(dpy, di->drawable, FPART(w).font, FITEM(item).label,
		       di->gc, (int)x, (int)y, width,
		       XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
}	/* end of DmDrawIconLabel */

/****************************procedure*header*****************************
    DmDrawIcon - draws an icon visual if given an icon glyph and string.
*/
void
DmDrawIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
    ExmFIconDrawProc(w, (ExmFlatItem) client_data, (ExmFlatDrawInfo *) call_data);

} /* DmDrawIcon() */

/****************************procedure*header*****************************
    DmMaskPixmap - masks the pixmap in glyph using the background color
    and the mask in glyph.
*/
Pixmap
DmMaskPixmap(Widget w, DmGlyphPtr glyph)
{

	Arg		arg[10];
	Pixel		p;
	Display	*	dpy = XtDisplay(w);
	XGCValues	values;
	Pixmap		mask;
	GC		gc;

	/* Create the mask that will contain the new clipped mask */
	mask = XCreatePixmap(
		dpy, RootWindowOfScreen(XtScreen(w)),
		(unsigned int)glyph->width, (unsigned int)glyph->height, 1
	);

	/* Invert the mask */
	values.function = GXcopyInverted;
	gc = XCreateGC(dpy, glyph->mask, GCFunction, &values);
	XCopyArea(
		dpy, glyph->mask, mask, gc,
		0, 0, glyph->width, glyph->height, 0, 0
	);

	XFreeGC(dpy, gc);
	/* Get a gc where the foreground color is the background */
	/* color of the widget.  This makes the fill work */
	/* correctly. */
	XtVaGetValues(w, XmNbackground, &p, NULL);
	values.foreground = p;
	gc = XCreateGC(dpy, glyph->pix, GCForeground, &values);

	XSetClipMask(dpy, gc, mask);	/* Set mask and origin */
	XSetClipOrigin(dpy, gc, 0, 0);

	/* Fill everything but the image */
	XFillRectangle(
		dpy, glyph->pix, gc, 0, 0,
		(unsigned int)glyph->width, (unsigned int)glyph->height
	);

	XFreePixmap(dpy, mask);
	XFreeGC(dpy, gc);

	return glyph->pix;
}

