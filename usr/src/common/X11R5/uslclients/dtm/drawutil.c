#pragma ident	"@(#)dtm:drawutil.c	1.14"

/******************************file*header********************************

    Description:
	This file contains convenience routines for drawing icons.
*/
						/* #includes go here	*/
#include <stdio.h>
#include <Xm/Xm.h>
#include <FIconBoxP.h>

#include "Dtm.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void	DrawIcon(Widget w, ExmFlatItem item,
			 ExmFlatDrawInfo *di, DmViewFormatType type);

					/* public procedures		*/
void		DmDrawLongIcon(Widget, XtPointer, XtPointer);
void		DmDrawNameIcon(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    DrawIcon- handles long and short views

    ORIENTATION: The icon and label are oriented horizontally, 
    separated by the horizontal pad of the flatIconBox.
    SELECTED ITEMS: If the item is selected, the icon and label are 
    enclosed in a single shadow box.
    INPUT FOCUS: If the item has input focus, the icon and label are enclosed
    in a single highlight box (which surrounds the shadow box, when shown).
    Geometry is always left for the location cursor and the shadow box.


     FLH MORE: This can be done more efficiently.  If we are
       always vertically centering the icon and the label,
       we need not adjust the y_offset and height for highlight and
       shadow because these adjustments offset each other.
       y_offset can always be set to (height - glyph_height)/2.
       x_offset must be calculated, though.  But we need
       to calculate these values to draw the shadow and
       highlight.  What values don't need to be recalculated?



*/
static void
DrawIcon(Widget w, ExmFlatItem item, ExmFlatDrawInfo *di, DmViewFormatType type)
{
    Display *		dpy = XtDisplay(w);
    DmGlyphPtr		gp;
    unsigned int 	width, height;
    int			x_offset = 0;
    int 		y_offset = 0;
    DmObjectPtr		op = (DmObjectPtr)IITEM(item).object_data;


    width = di->width;
    height = di->height;

    /* First Draw highlight and shadow, as needed */
    if (PPART(w).highlight_thickness)
    {
	if (di->item_has_focus)
	    _XmDrawSimpleHighlight(dpy, di->drawable,
				   PPART(w).highlight_GC, 
				   di->x, di->y, di->width, di->height,
				   PPART(w).highlight_thickness);
	
	/* Adjust to draw within highlight, if any */
	x_offset += PPART(w).highlight_thickness;
	y_offset += PPART(w).highlight_thickness;
	/* adjust width/height for shadow box below */
	width -= 2 * PPART(w).highlight_thickness;
	height -= 2 * PPART(w).highlight_thickness;
    }

    if (PPART(w).shadow_thickness)
    {
	if (FITEM(item).selected)
	    _XmDrawShadows (dpy, di->drawable,
			    PPART(w).bottom_shadow_GC, PPART(w).top_shadow_GC,
			    di->x + x_offset, di->y + y_offset, width, height, 
			    PPART(w).shadow_thickness, XmSHADOW_OUT);
	
	/* Adjust to draw within shadow, if any */
	/* FLH MORE: are the y_offset and height adjustments needed here?
	   If we remove both, the end result should be the same.
	   */
	x_offset += PPART(w).shadow_thickness;
	y_offset += PPART(w).shadow_thickness;
	width -= 2 * PPART(w).shadow_thickness;
	height -= 2 * PPART(w).shadow_thickness;
    }

    gp = DmFtypeToFmodeKey(op->ftype)->small_icon;

    /* Use offset to center glyph vertically */
    y_offset += ((int)height - (int)(gp->height)) / 2;

    DmDrawIconGlyph(w, item, di, gp, di->x + x_offset, di->y + y_offset);


    if (op->attrs & DM_B_SYMLINK) {
	DmGlyphPtr sgp;		/* shortcut glyph */
	int        link_x, link_y, link_width;

	sgp = DESKTOP_SHORTCUT(Desktop);
	
	/* FLH MORE: remove ICON_PADDING */
	link_y = di->y + y_offset + gp->height + ICON_PADDING;
	link_width = (gp->width / sgp->width) * sgp->width;
	link_x = di->x + x_offset;
	
	/* FLH MORE: do we need to copy the GC here?
	   Should this be XSetTile/FillTiled ?
	 */
	
	XSetStipple(dpy, di->gc, sgp->pix);
	XSetFillStyle(dpy, di->gc, FillStippled);
	XSetTSOrigin(dpy, di->gc, link_x, link_y);
	XFillRectangle(dpy, di->drawable, di->gc, link_x, link_y, link_width, 
		       sgp->height);
	XSetFillStyle(dpy, di->gc, FillSolid);
	XSetTSOrigin(dpy, di->gc, 0, 0);
    }

    /* Draw the label to the right of the icon */

    if (!_XmStringEmpty(FITEM(item).label))
    {
	int		pad = IPART(w).hpad;
	Dimension	lbl_w, lbl_h;

	_XmStringExtent(FPART(w).font, FITEM(item).label, &lbl_w, &lbl_h);

	/* Use offset to center label vertically 
	   FLH_MORE: should we worry about shadow/hightlight or
	   just assume that the label will fall within them.
	 */
	
	y_offset = ((int)di->height - (int)lbl_h) / 2;

	/* Text offset is to the right of highlight, shadow, glyph, and pad.
	   Width includes all of space for text because drawing is done
	   with DrawImageString.
	 */
	
	x_offset += gp->width + pad;
	width -= gp->width + pad;	

	DmDrawIconLabel(w, item, di, di->x+x_offset, di->y+y_offset, width);
    }	
}				/* end of DrawIcon */

/***************************private*procedures****************************

    Private Procedures
*/
/****************************procedure*header*****************************
    DmDrawLongIcon-
*/
void 
DmDrawLongIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
    ExmFlatItem item = (ExmFlatItem) client_data;
    ExmFlatDrawInfo *di = (ExmFlatDrawInfo *)call_data;

    DrawIcon(w, item, di, DM_NAME);
}

/****************************procedure*header*****************************
    DmDrawNameIcon-
*/
void
DmDrawNameIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
    ExmFlatItem item = (ExmFlatItem) client_data;
    ExmFlatDrawInfo *di = (ExmFlatDrawInfo *)call_data;

    DrawIcon(w, item, di, DM_NAME);
}


