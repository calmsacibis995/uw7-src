/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#ident	"@(#)menu:MenuShellO.c	1.9"
#endif

/*
 * MenuShellO.c
 *
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/PopupMenuP.h>
#include <Xol/VendorI.h>
#include <Xol/FButtonsP.h>

#ifdef DEBUG
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x)
#endif

/*  These values come from the Open Look Specification for the 12 pt.
    visuals.  */
#define TOP_MARGIN	9  /* calculated from the OL spec. */
#define SIDE_MARGIN	9  /* from the OL spec.  */
#define BOTTOM_MARGIN	9  /* calculated from the OL spec. */
#define HEADER_SPACE	6  /* space between header and 1st child  */
#define SEPERATOR_GAP	4  /* space between title seperator and border */
#define TITLE_SPACE	22 /* space from top border to seperator */
#define TITLE_BASELINE	7  /* distance from seperator to title's baseline */
#define MIN_BASELINE	3  /* minimum distance from seperator to descent */
#define PUSHPIN_SPACE	22 /* space from top border to pushpin's baseline */
#define PUSHPIN_BASELINE 4 /* distance from bottom of pushpin to pushpin base */
#define PUSHPIN_GAP	9  /* distance between pushpin and title */
#define SEPERATOR_HEIGHT 2 /* thickness of etched seperator line */
#define CHILD_SPACE     2  /* space between menu items */

#define Width(W,P) OlScreenPointToPixel(OL_HORIZONTAL,(P), XtScreen(W))
#define Height(W,P) OlScreenPointToPixel(OL_VERTICAL,(P), XtScreen(W))

#define NPART(nw,field) ((nw)-> popup_menu_shell.field)
#define HAS_PUSHPIN(nw) ((_OlGetVendorPartExtension((Widget)nw))->pushpin != OL_NONE)
#define HAS_TITLE(nw)   (((PopupMenuShellWidget)(nw))-> wm.title != NULL && strcmp(((PopupMenuShellWidget)nw)->wm.title, " "))

static Dimension SpecHeightOrBigger OL_ARGS((
	Dimension	spec_height,
	Dimension	spec_baseline,
	Dimension	ascent,
	Dimension	descent,
	Dimension	default_top,
	Dimension	default_bottom,
	Dimension *	calc_baseline
));

/*
 * _OloMSHandlePushpin
 *
 */
void
_OloMSHandlePushpin OLARGLIST((nw, nPart, pin_state, size_or_draw))
	OLARG( PopupMenuShellWidget,	nw)
	OLARG( PopupMenuShellPart *,	nPart)
	OLARG( OlDefine, 		pin_state)
	OLGRA( char,			size_or_draw)
{
  XRectangle * pin = &(nPart->pin);

  if (HAS_PUSHPIN(nw)) {
     if (size_or_draw == 'd') {
		/*  Drawing the pushpin uses the pin rectangle that is
		    sized in this function and positioned in Layout.  */

	unsigned	type = PP_IN;	/* OL_BUSY || OL_IN		*/

	if (!nw->shell.override_redirect)
		return;

		/* if I see OL_NONE here then we have trouble somewhere	*/
	if (pin_state == OL_OUT && nPart-> pushpinDefault) {
		type = PP_DEFAULT;
	}
	else if (pin_state == OL_OUT) {
		type = PP_OUT;
	}
	if (XtIsRealized((Widget)nw))  {
		/*  When popping the menu up from default key, the window
		    may not be realized, yet. */
		if (pin->width && pin->height)  {
        		XClearArea(XtDisplay(nw), XtWindow(nw), 
	   			pin->x, pin->y, pin->width, pin->height, False);

        		OlgDrawPushPin(XtScreen(nw), XtWindow(nw), nPart-> attrs,
	   			pin->x, pin->y, pin->width, pin->height, type);
		}
	}
        }
     else {
	/*  Set the size to be the natural size */
        OlgSizePushPin(XtScreen(nw), nPart-> attrs, 
           &pin->width, &pin->height);
        }
     }
} /* end of _OloMSHandlePushpin */

/*
 * _OloMSHandleTitle
 *
 */
void
_OloMSHandleTitle OLARGLIST((nw, nPart, size_or_draw))
	OLARG( PopupMenuShellWidget,	nw)
	OLARG( PopupMenuShellPart *,	nPart)
	OLGRA( char,			size_or_draw)
{
  OlgTextLbl labeldata;
  XRectangle * title = &(nPart->title);

  if (HAS_TITLE(nw)) {
     labeldata.label         = nw-> wm.title;
     labeldata.normalGC      =
     labeldata.inverseGC     = nPart-> gc;
     labeldata.font          = nPart-> font;
     labeldata.accelerator   = NULL;
     labeldata.mnemonic      = '\0';
     labeldata.justification = TL_CENTER_JUSTIFY;
     labeldata.flags         = 0;
     labeldata.font_list     = nPart->font_list;

	if (size_or_draw == 'd') {
		/*  The title rectangle has the position and size of the
		    title, but the size of the title rectangle is always the
		    natural size of the title.  When drawing it, we use the
		    core size to calculate the width, and use the x,y, and
		    height from the title rectangle.  */
		Dimension side = Width(nw, SIDE_MARGIN);
		Dimension seperator = Width(nw, SEPERATOR_GAP);
		Position x = side;

		if (nPart->pin.width)
			x += nPart->pin.width + (Position)Width(nw, PUSHPIN_GAP);

        	OlgDrawTextLabel(XtScreen(nw), XtWindow(nw), nPart->attrs, 
			title->x, title->y, 
			(Dimension) _OlMin((int) title->width,
				(int)(nw->core.width - (x + side))), 
			title->height, &labeldata);

		/*  Draw the etched out chiseled seperator line. */
		(void) OlgDrawLine(XtScreen(nw), XtWindow(nw), nPart->attrs,
			(Position) seperator,
			(Position) (title->height + title->y +
				(Position) Height(nw, MIN_BASELINE)),
			(Dimension) (nw->core.width - 2*seperator),
			(unsigned) Height(nw, SEPERATOR_HEIGHT),
			(unsigned) False);
	}
	else
		/*  Sizing the title sets only the width and height to the
		    natural size of the string. */
		OlgSizeTextLabel(XtScreen(nw), nPart->attrs, &labeldata,
			&nPart->title.width, &nPart->title.height);
    }
} /* end of _OloMSHandleTitle */


/* ARGSUSED */
void
_OloMSCreateButton OLARGLIST((w))
	OLGRA(Widget, w)
{
	/* no-op for Open Look */
}  /*  end of _OloMSCreateButton() */

/**
 ** _OloMSLayoutPreferred()
 **
 ** Compute the preferred size of the menu shell based on the title,
 ** pushpin, spec margins, and preferred size of the managed children.
 **/

extern void
_OloMSLayoutPreferred OLARGLIST((_w, who_asking, request, width, height, xpos, ypos, child_space))
	OLARG(Widget,			_w)
	OLARG(Widget,			who_asking)
	OLARG(XtWidgetGeometry *,	request)
	OLARG(Dimension *,		width)
	OLARG(Dimension *,		height)
	OLARG(Position *,		xpos)
	OLARG(Position *,		ypos)
	OLGRA(Dimension *,		child_space)
{
	PopupMenuShellWidget	w = (PopupMenuShellWidget)_w;

	Cardinal		nchildren    = w->composite.num_children;
	Cardinal		n;

	Widget *		pchild	     = w->composite.children;
	Widget *		p;

	XtWidgetGeometry	prefer;
	Dimension		max_child = 0;
	Dimension		side_margin = Width(w, SIDE_MARGIN);
	Boolean			added_space;


	/*  Compute the optimum fit based on the size of the title,
	    pushpin, spec. compliant margins, and width of children.
	*/

	/*  Initialize the width and height to the spec's margins.  */

	/*  The minimum width starts with a margin on each side of the items. */
	*width = 2 * side_margin;

	/*  When overried_redirect is False the window manager accounts for
	    the title and pushpin.  */
	if (HAS_TITLE(w) && w->shell.override_redirect)  {
		Dimension title_space = Height(w, TITLE_SPACE);
		Dimension baseline = Height(w, TITLE_BASELINE);

		/*  Menus with titles have the standard header dimensions.  */

		/*  Initialize the position of the title.  */
		NPART(w, title.x) = side_margin;

		/*  The Pushpin can be larger than the title, so we chain the
		    SpecHeight calculations together by feeding the pushpin's
		    SpecHeightOrBigger as the title's spec height.  */
		if (HAS_PUSHPIN(w))  {
			title_space = SpecHeightOrBigger(
				(Dimension) Height(w, PUSHPIN_SPACE),
				(Dimension) Height(w, PUSHPIN_BASELINE),
				(Dimension) (NPART(w, pin.height) - Height(w,PUSHPIN_BASELINE)),
				(Dimension) Height(w, PUSHPIN_BASELINE),
				(Dimension) Height(w, TOP_MARGIN),
				(Dimension) 0,
				(Dimension *) &baseline);
			/*  Set the position of the pushpin. */
			NPART(w, pin.x) = side_margin;
			NPART(w, pin.y) = (title_space - baseline) -
							NPART(w, pin.height);

			/*  The spec's Title baseline  is different from the
			    pushpin's, so account for that fixed difference. */
			baseline += TITLE_BASELINE - PUSHPIN_BASELINE;

			/*  Add on the space between the title and pusinpin
			    and the pushpin width. */
			NPART(w, title.x) += Width(w, PUSHPIN_GAP) +
							(Position)NPART(w, pin.width);
			*width += (Dimension)Width(w, PUSHPIN_GAP) + (Dimension)NPART(w, pin.width);
		}

		*height = SpecHeightOrBigger(
				(Dimension) title_space,
				(Dimension) baseline,
				(Dimension) OlFontAscent(NPART(w, font), NPART(w, font_list)),
				(Dimension) OlFontDescent(NPART(w, font), NPART(w, font_list)),
				(Dimension) Height(w, TOP_MARGIN),
				(Dimension) Height(w, MIN_BASELINE),
				(Dimension *) &baseline);
		/*  Use this height and baseline to set the y position of the
		    title.  The x position is a little later... */
		NPART(w, title.y) = (*height - baseline) -
				NPART(w, font)->max_bounds.ascent;

		/*  All menus with titles also have a etched seperator line
		    and a fixed amount of space between the seperator and the
		    first menu item.  */
		*height += (Dimension)Height(w, SEPERATOR_HEIGHT) + (Dimension)Height(w, HEADER_SPACE);

		/*  Add on the width of the title.  */
		*width += NPART(w, title.width);
	}
	else if (HAS_PUSHPIN(w) && w->shell.override_redirect)  {
		/*  with no title... */
		Dimension baseline;
		/*  The spec's pushpin header space may not be enough if the
		    Olg routines are being tricked into using larger images,
		    so we take some precautions...  */
		*height = SpecHeightOrBigger(
				(Dimension) Height(w, PUSHPIN_SPACE),
				(Dimension) Height(w, PUSHPIN_BASELINE),
				(Dimension) (NPART(w, pin.height) - Height(w,PUSHPIN_BASELINE)),
				(Dimension) Height(w, PUSHPIN_BASELINE),
				(Dimension) Height(w, TOP_MARGIN),
				(Dimension) 0,
				(Dimension *) &baseline);

		/*  Set the position of the pushpin. */
		NPART(w, pin.x) = side_margin;
		NPART(w, pin.y) = (*height - baseline) - NPART(w, pin.height);

		/*  Add on to the pushpin the fixed amount of space between the
		    header and the first item.  */
/*		*height += Height(w, HEADER_SPACE);  */

		/*  Add on the width of the pushpin.  */
		*width += NPART(w, pin.width);
	}
	else  {	/* no title, no pushpin, so give the standard margin.  */
		*height = Height(w, TOP_MARGIN);
	}

	/*  Later, when the children are configured, this is the position
	    of the first child.  */
	*ypos = *height;

	/*  Add on to the height the space after the last item */
	*height += (Dimension)Height(w, BOTTOM_MARGIN);

	/*  The width and height now have the prefered size based on the
	    standard Open Look decorations and margins.  Now we ask each
	    managed child its prefered size.  When keep track of the max
	    width so that it can be compared to the header's width.  The
	    height is accumulated as we ask each child.  */
	*child_space = Height(w, CHILD_SPACE);
	added_space = False;

	for (p = pchild, n = 0; n < nchildren; n++, p++)  {
		if (XtIsManaged(*p))  {
			if (who_asking == *p)  {
				/*  Allow the child's request.  */
				*height += request->height;
				max_child = _OlMax((int) max_child,
							(int) request->width);
			}
			else  {
				/*  Don't care about the answer because prefer
				    will always get the preferred or current
				    geometry.  */
				XtQueryGeometry(*p, NULL, &prefer);

				/*  To avoid caching the heights or querying
				    twice, I assume that the core.height is
				    adequate and use it when the widget is
				    configrued as well.  */
				*height += (*p)->core.height;

				max_child = _OlMax((int) max_child,
							(int) prefer.width);
			}
			added_space = True;
			*height += *child_space;
		}
	}

	/*  subtract the space added after the last child - it is only for
	    space BETWEEN items.  */
	if (added_space)
		*height -= *child_space;

	/*  Compare the header width with the maximum child's width.  */
	if ((int)*width < (int)(max_child + (side_margin * 2)))  {
		*width = max_child + (side_margin * 2);
	}

	/*  Position the title in the wider header.  It is centered
	    in the total width unless it is doesn't maintain a
	    fixed distance from the pushpin.  */
	if (HAS_TITLE(w))  {
		NPART(w, title.x) = _OlMax((int) NPART(w, title.x),
			(int)((int)(*width - NPART(w, title.width)) / 2));
	}

	/*  Set the x position to the SIDE_MARGIN. */
	*xpos = side_margin;
	return;
} /* _OloMSLayoutPreferred */

/*
 *  SpecHeightOrBigger -  a routine to calculate the height of various Open Look
 *	specification items that may not behave properly given a different
 *	font or different size image.  This function requires many arguments
 *	to do this calculation properly.
 *
 *	spec_height - the total height of the area (including baseline)
 *	spec_baseline - the distance from the bottom to the baseline
 *	ascent - the height of the REAL object above its baseline
 *	descent - the height of the REAL object below its baseline
 *	default_top - the top margin (usually calculated from the spec)
 *	default_bottom - the bottom margin (usually calculated from the spec)
 *	calc_baseline - returns the distance from the bottom of the RETURNED
 *		height to the calculated baseline
 *
 *	The return value is the calculated height.
 */

static Dimension 
SpecHeightOrBigger OLARGLIST((spec_height, spec_baseline, ascent, descent, default_top, default_bottom, calc_baseline))
	OLARG(Dimension, spec_height)
	OLARG(Dimension, spec_baseline)
	OLARG(Dimension, ascent)
	OLARG(Dimension, descent)
	OLARG(Dimension, default_top)
	OLARG(Dimension, default_bottom)
	OLGRA(Dimension *, calc_baseline)
{
	Dimension height = spec_height;
	Dimension b;

	/*  First check that the descent and the minimum amount of
	    bottom margin will fit in the space specified by the spec.
	*/
	if ((int)(descent + default_bottom) > (int)spec_baseline)
		b = descent + default_bottom;
	else
		b = spec_baseline;

	/*  Return the calculated base line if an address was passed in.  */
	if (calc_baseline)
		*calc_baseline = b;

	/*  The total height is increased if the calculated baseline along
	    with the height of the ascent and the minimum height of the top
	    margin is greater than the specification's height.  */
	if ((int)(ascent + b + default_top) > (int)spec_height)
		height = ascent + b + default_top;

	return(height);

}  /* SpecHeightOrBigger()  */
