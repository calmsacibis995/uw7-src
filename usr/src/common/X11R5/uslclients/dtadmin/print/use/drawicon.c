/* ident	"@(#)dtadmin:print/use/drawicon.c	1.6" */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "DtI.h"

/* same as OL_DEFAULT_POINT_SIZE, Unfortunately, it is defined in 
   OpenLookP.h and this is not enough reason to include that file.
   Ideally, XGetFontProperty() on XA_POINT_SIZE should be done.
   FIconBox is too flexible in providing font resource for each sub-item.
   But.., efficiency wins. 
*/
#define	POINT_SIZE	12	

/****************************procedure*header*****************************
    DrawIcon - draws an icon visual if given an icon glyph and string.
*/
void
DrawIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFIconDrawPtr draw_info = (OlFIconDrawPtr)call_data;
	int x_offset;
	DmGlyphPtr gp = (DmGlyphPtr) draw_info->op;
	OlgAttrs *bg_attrs;
	OlgAttrs *attrs;
	Pixel pixel;
	GC gc = draw_info->label_gc;
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);
	unsigned flags;

	flags = (draw_info->select) ? RB_SELECTED : RB_NOFRAME;
	if (draw_info->busy == True)
		flags |= RB_DIM;

	bg_attrs = OlgCreateAttrs(XtScreen(w), draw_info->fg_color,
				  (OlgBG *)&(draw_info->bg_color), False,
				  POINT_SIZE);
	if (draw_info->focus)
		attrs = OlgCreateAttrs(XtScreen(w), draw_info->fg_color,
				       (OlgBG *)&(draw_info->focus_color),
				       False, POINT_SIZE);
	else
		attrs = bg_attrs;

	/* Use offset to center glyph horizontally */
	x_offset = ((int)draw_info->width - (int)(gp->width)) / 2;

	DmDrawIconGlyph(w, gc, gp, bg_attrs, draw_info->x + x_offset,
			draw_info->y + ICON_PADDING, flags);

	if (draw_info->label != (String)NULL) {
		Dimension	text_width;
		int		y_offset;

		text_width = draw_info->font_list ? 
				OlTextWidth(draw_info->font_list,
					(unsigned char *)(draw_info->label),
					strlen(draw_info->label)) :
				XTextWidth(draw_info->font,
					draw_info->label,
					strlen(draw_info->label));

		/* Use offset to center text horizontally */
		x_offset = ((int)draw_info->width - (int)text_width) / 2;

		/* Text offset is below glyph */
		y_offset = gp->height + ICON_PADDING * 5 + ICON_PADDING / 2;

		if (draw_info->focus)
			XSetBackground(dpy, gc, draw_info->focus_color);
		DmDrawIconLabel(w, gc, draw_info->label,
				draw_info->font_list,
				draw_info->font,
				attrs,
				draw_info->x + x_offset,
				draw_info->y + y_offset,
				flags);
		if (draw_info->focus)
			XSetBackground(dpy, gc, draw_info->bg_color);
	}
} /* DmDrawIcon() */

