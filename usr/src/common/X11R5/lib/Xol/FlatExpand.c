#ifndef	NOIDENT
#ident	"@(#)flat:FlatExpand.c	1.43"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains various support and convenience routines
 *	for the flat widgets.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/FlatP.h>
#include <Xol/EventObj.h>
#include <Xol/Font.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private  Procedures 
 *		2. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void	nullProc OL_NO_ARGS();
static void	nullSize OL_ARGS((Screen *, OlgAttrs *, XtPointer,
				Dimension *, Dimension *));

					/* public procedures		*/

void	_OlDoGravity OL_ARGS((int, Dimension, Dimension, Dimension,
				Dimension, Position *, Position *));
void	OlFlatPreviewItem OL_ARGS((Widget, Cardinal, Widget, Cardinal));
void	OlFlatRefreshExpandedItem OL_ARGS((Widget, FlatItem, Boolean));
void	OlFlatRefreshItem OL_ARGS((Widget, Cardinal, Boolean));
OlFlatScreenCache * OlFlatScreenManager OL_ARGS((Widget, Cardinal, OlDefine));
void	OlFlatSetupLabelSize OL_ARGS((Widget, FlatItem, XtPointer *,
					void (**)()));
void	OlFlatSetupAttributes OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *,
				OlgAttrs **, XtPointer *, OlgLabelProc *));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define CVT_JUSTIFY(j)	((j) == OL_LEFT ? TL_LEFT_JUSTIFY :\
		((j) == OL_CENTER ? TL_CENTER_JUSTIFY : TL_RIGHT_JUSTIFY))

#define PIXELS(s,n)	OlScreenPointToPixel(OL_HORIZONTAL,n,s)

			/* Define some constants to reflect visual
			 * sizes for the default point size.
			 * This will have to be updated when scaling
			 * is supported					*/

#define MENU_MARK	8				/* in points	*/
#define OBLONG_RADIUS	10				/* in points	*/

#define FPART(w)	(((FlatWidget)(w))->flat)

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * nullProc - Do not draw a label; do not pass GO; do not collect $200.
 ****************************procedure*header*****************************
 */
static void
nullProc ()
{
} /* END OF nullProc() */

/*
 *************************************************************************
 * nullSize - Size procedure for null labels.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
nullSize OLARGLIST((scr, pAttrs, lbl, pWidth, pHeight))
    OLARG( Screen *,	scr)
    OLARG( OlgAttrs *,	pAttrs)
    OLARG( XtPointer,	lbl)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    *pWidth = *pHeight = 0;
} /* END OF nullSize() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlDoGravity - this routine calculates the offsets for a particular
 * container.  Offsets are determined using the gravity resource.
 * The following code never lets the offsets be less than zero.
 ****************************procedure*header*****************************
 */
void
_OlDoGravity OLARGLIST((gravity, c_width, c_height, width, height, x, y))
	OLARG( int,		gravity)	/* the desired gravity	*/
	OLARG( Dimension,	c_width)	/* Containing width	*/
	OLARG( Dimension,	c_height)	/* Containing height	*/
	OLARG( Dimension,	width)		/* interior width	*/
	OLARG( Dimension,	height)		/* interior height	*/
	OLARG( Position *,	x)		/* returned x offset	*/
	OLGRA( Position *,	y)		/* returned y offset	*/
{
	Dimension	dw;		/* difference in widths		*/
	Dimension	dh;		/* difference in heights	*/

	dw = (Dimension) (c_width > width ? c_width - width : 0);
	dh = (Dimension) (c_height > height ? c_height - height : 0);

	if (dh != 0 || dw != 0)
	{
		switch(gravity)
		{
		case EastGravity:
			dh /= 2;			/* (dw, dh/2)	*/
			break;
		case WestGravity:
			dw  = 0;			/* (0, dh/2)	*/
			dh /= 2;
			break;
		case CenterGravity:
			dw /= 2;			/* (dw/2, dh/2)	*/
			dh /= 2;
			break;
		case NorthGravity:
			dw /= 2;			/* (dw/2, 0)	*/
			dh  = 0;
			break;
		case NorthEastGravity:
			dh = 0;				/* (dw, 0)	*/
			break;
		case NorthWestGravity:
			dw = 0;				/* (0,0)	*/
			dh = 0;
			break;
		case SouthGravity:
			dw /= 2;			/* (dw/2, dh)	*/
			break;
		case SouthEastGravity:
			break;				/* (dw, dh)	*/
		case SouthWestGravity:
			dw = 0;				/* (0, dh)	*/
			break;
		default:
			OlVaDisplayWarningMsg((Display *)NULL,
				OleNinvalidParameters,
				OleTolDoGravity, OleCOlToolkitWarning,
				OleMinvalidParameters_olDoGravity, gravity);
			 break;
		}
	}

	*x = (Position) dw;		/* return the values	*/
	*y = (Position) dh;
} /* END OF _OlDoGravity() */

/*
 *************************************************************************
 * OlFlatPreviewItem - this routine previews a flat widget's sub-object
 * into another widget.  If the preview widget (i.e., the destination
 * widget) is a flat widget, the parameter preview_item is used to 
 * determine which sub-object is the destination sub-object for
 * previewing.
 ****************************procedure*header*****************************
 */
void
OlFlatPreviewItem OLARGLIST((w, item_index, preview, preview_item))
	OLARG( Widget,		w)		/* Flat widget id	*/
	OLARG( Cardinal,	item_index)	/* item to be previewed	*/
	OLARG( Widget,		preview)	/* destination widget	*/
	OLGRA( Cardinal,	preview_item)	/* destination item	*/
{
	if (item_index < FPART(w).num_items &&
	    preview != (Widget)NULL &&
	    XtIsRealized(preview) == (Boolean)True &&
	    OL_FLATCLASS(w).draw_item)
	{
		Display *	dpy;
		OlFlatDrawInfo	item_di;	/* item to be previewed	*/
		OlFlatDrawInfo	di;		/* preview draw info	*/
		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
		Boolean		is_gadget = _OlIsGadget(preview);

				/* Get the required width and height
				 * for the item to be previewed.	*/

		OlFlatGetItemGeometry(w, item_index, &item_di.x, &item_di.y,
					&item_di.width, &item_di.height);

				/* Obtain information about the
				 * destination.				*/

		if (preview_item != (Cardinal)OL_NO_ITEM &&
			 _OlIsFlat(preview) == True)
		{
			OlFlatGetItemGeometry(preview, preview_item, &di.x,
					&di.y, &di.width, &di.height);
		}
		else
		{
			di.x = (Position) (is_gadget == (Boolean )True ?
				(di.y = preview->core.y, preview->core.x) :
				(di.y = (Position)0, 0));

			di.width	= preview->core.width;
			di.height	= preview->core.height;
		}

				/* Set the other draw information
				 * parameters.				*/

		di.screen	= XtScreenOfObject(preview);
		dpy		= DisplayOfScreen(di.screen);
		di.drawable	= (Drawable) XtWindowOfObject(preview);
		di.background	= (is_gadget == (Boolean)True ?
				XtParent(preview)->core.background_pixel :
					preview->core.background_pixel);

					/* Clear the Destination area	*/

		(void) XClearArea(dpy, (Window)di.drawable,
					(int)di.x, (int)di.y,
					(unsigned int)di.width,
					(unsigned int)di.height, (Bool)False);

		OlFlatExpandItem(w, item_index, item);

		if (item->flat.managed == True &&
		    item->flat.mapped_when_managed == True)
		{
			(*OL_FLATCLASS(w).draw_item)(w, item, &di);
		}
		OL_FLAT_FREE_ITEM(item);
	}
} /* END OF OlFlatPreviewItem() */

/*
 *************************************************************************
 * OlFlatRefreshExpandedItem - optionally clears an item and then makes
 * an in-line request to draw it.
 *	This routine calls the class's refresh routine, if it has one.
 * If a class doesn't have a refresh routine, a default refresh is done.
 ****************************procedure*header*****************************
 */
void
OlFlatRefreshExpandedItem OLARGLIST((w, item, clear_area))
	OLARG( Widget,		w)		/* Flat Widget subclass	*/
	OLARG( FlatItem,	item)		/* item to draw		*/
	OLGRA( Boolean,		clear_area)	/* clear the area also?	*/
{

	if (XtIsRealized(w) == (Boolean)False ||
	   FPART(w).items_touched == True)
	{
		/* EMPTY */
		;
	}
	else if (OL_FLATCLASS(w).refresh_item)
	{
		(*OL_FLATCLASS(w).refresh_item)(w, item, clear_area);
	}
	else
	{
		Display *		dpy;
		OlFlatDrawInfo		di;

		OlFlatGetItemGeometry(w, item->flat.item_index, &di.x,
					&di.y, &di.width, &di.height);

		di.screen	= XtScreen(w);
		di.background	= w->core.background_pixel;
		di.drawable	= (Drawable) XtWindow(w);
		dpy		= DisplayOfScreen(di.screen);

		if (clear_area == True)
		{
			(void) XClearArea(dpy, (Window)di.drawable,
				(int)di.x, (int)di.y,
			(unsigned int)di.width, (unsigned int)di.height,
			(Bool)FALSE);
		}

		if (item->flat.managed == True &&
		    item->flat.mapped_when_managed == True)
		{
			(*OL_FLATCLASS(w).draw_item)(w, item, &di);
		}
	}
} /* END OF OlFlatRefreshExpandedItem() */

/*
 *************************************************************************
 * OlFlatRefreshItem - convenient interface to OlFlatRefreshExpandedItem
 ****************************procedure*header*****************************
 */
void
OlFlatRefreshItem OLARGLIST((w, item_index, clear_area))
	OLARG( Widget,		w)		/* Flat Widget subclass	*/
	OLARG( Cardinal,	item_index)	/* item to draw		*/
	OLGRA( Boolean,		clear_area)	/* clear the area also?	*/
{
	if (XtIsRealized(w) == (Boolean)True &&
	   FPART(w).items_touched == False)
	{
		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

		OlFlatExpandItem(w, item_index, item);
		OlFlatRefreshExpandedItem(w, item, clear_area);

		OL_FLAT_FREE_ITEM(item);
	}
} /* END OF OlFlatRefreshItem() */

/*
 *************************************************************************
 * ScreenManager - this routine caches information on a screen basis.
 * For each created widget instance, a reference is added to the count.
 * As an instance is destroyed, the reference count is decremented.
 * When the reference count is zero, the node is deleted.
 ****************************procedure*header*****************************
 */
OlFlatScreenCache *
OlFlatScreenManager OLARGLIST((w, point_size, flag))
	OLARG( Widget,	w)		/* widget referencing the cache	*/
	OLARG( Cardinal,point_size)	/* point size for this reference*/
	OLGRA( OlDefine,flag)		/* type of operation		*/
{
	static OlFlatScreenCacheList	screen_cache_head = NULL;
	OlFlatScreenCache *		self = screen_cache_head;
	Screen *			screen = XtScreen(w);
	Display *			dpy = DisplayOfScreen(screen);
	char				olmsgbuf[BUFSIZ];

	/* get localized "screenCache" string */

	(void)	OlGetMessage(	XtDisplay(w),
				olmsgbuf,
				BUFSIZ,
				OleNfileFlatExpand,
				OleTmsg1,
				OleCOlToolkitMessage,
				OleMfileFlatExpand_msg1,
				(XrmDatabase)NULL);

				/* If we didn't find a node, make one	*/

	if (flag == OL_ADD_REF || flag == OL_JUST_LOOKING)
	{
		while(self != (OlFlatScreenCache *)NULL &&
				self->screen != screen)
		{
			self = self->next;
		}

		if (self == (OlFlatScreenCacheList)NULL)
		{
			XGCValues	values;

			self			= XtNew(OlFlatScreenCache);
			self->next		= screen_cache_head;
			screen_cache_head	= self;
			self->screen		= screen;
			self->count		= 0;

			self->alt_bg		= WhitePixelOfScreen(screen);
			self->alt_fg		= BlackPixelOfScreen(screen);
			self->alt_attrs		= OlgCreateAttrs(screen,
							self->alt_fg,
							(OlgBG*)&(self->alt_bg),
							False, point_size);

	/*
	 * This field is no longer needed. A converter or the Primitive
	 * suplerclass will issue an error/warning if it can't convert
	 * a given font or superclass will load in a default font if a
	 * given font is NULL.
	 *		self->font		= _OlGetDefaultFont(w, NULL);
	 */
			self->stroke_width	= OlgGetHorizontalStroke(
							self->alt_attrs);
			if (self->stroke_width == 0)
			{
				self->stroke_width = 1;
			}

			self->oblong_radius	= PIXELS(screen,OBLONG_RADIUS);
			self->menu_mark		= PIXELS(screen,MENU_MARK);

			OlgSizeAbbrevMenuB(screen, self->alt_attrs,
				&self->abbrev_width, &self->abbrev_height);
			OlgSizeCheckBox(screen, self->alt_attrs,
				&self->check_width, &self->check_height);

						/* Create a scratch GC
						 * and one default one	*/

			values.graphics_exposures = (Bool)False;

			self->default_gc	= XCreateGC(dpy,
						RootWindowOfScreen(screen),
						(unsigned long)
						GCGraphicsExposures,
						&values);
			self->scratch_gc	= XCreateGC(dpy,
						RootWindowOfScreen(screen),
						(unsigned long)
						GCGraphicsExposures,
						&values);
		}
		if (flag == OL_ADD_REF)
		{
			++self->count;
		}
		else if (self->count == 0)
		{
		    OlVaDisplayWarningMsg(dpy,
					  OleNinternal,
					  OleTbadNodeReference,
					  OleCOlToolkitWarning,
					  OleMinternal_badNodeReference,
					  XtName(w),
					  OlWidgetToClassName(w),
					  olmsgbuf);
		}
	}
	else
	{
		OlFlatScreenCache * prev = (OlFlatScreenCache *)NULL;

		for(;self != (OlFlatScreenCacheList)NULL &&
			self->screen != screen; self = self->next)
		{
			prev = self;
		}

		if (self != (OlFlatScreenCacheList)NULL)
		{
			if (--self->count == 0)
			{
				if (prev != (OlFlatScreenCache *)NULL)
				{
					prev->next = self->next;
				}
				else
				{
					screen_cache_head = self->next;
				}

				(void) XFreeGC(dpy, self->scratch_gc);

				if (self->alt_attrs != (OlgAttrs *)NULL)
				{
					OlgDestroyAttrs(self->alt_attrs);
				}

				XtFree((char *) self);
			}
		}
		else
		{
		    OlVaDisplayWarningMsg(dpy,
					  OleNinternal,
					  OleTcorruptedList,
					  OleCOlToolkitWarning,
					  OleMinternal_corruptedList,
					  XtName(w),
					  OlWidgetToClassName(w),
					  olmsgbuf);
		}

		self = (OlFlatScreenCache *)NULL;
	}
	return(self);
} /* END OF OlFlatScreenManager() */

/*
 *************************************************************************
 * OlFlatSetupAttributes - this routine selects GCs, attributes and label
 * drawing functions for a flat item.  Note that there is only one
 * static label structure.  Subsequent calls to this function will destroy
 * previous contents.
 ****************************procedure*header*****************************
 */
void
OlFlatSetupAttributes OLARGLIST((w, item, di, ppAttrs, ppLbl, ppDrawProc))
    OLARG( Widget,		w)
    OLARG( FlatItem,		item)
    OLARG( OlFlatDrawInfo *,	di)
    OLARG( OlgAttrs **,		ppAttrs)
    OLARG( XtPointer *,		ppLbl)
    OLGRA( OlgLabelProc *,	ppDrawProc)
{
    FlatPart *		fp = &FPART(w);
    Cardinal		psize = OL_DEFAULT_POINT_SIZE;
    OlFlatScreenCache *	sc = OlFlatScreenManager (w, psize, OL_JUST_LOOKING);
    GC			label_GC, inverse_GC;
    void		(*sizeProc)();
    Display *		dpy = DisplayOfScreen(XtScreen(w));
    typedef union {
	OlgTextLbl	text;
	OlgPixmapLbl	pixmap;
    } MyLabel;
    MyLabel *		plbl;
    int			has_focus = (fp->focus_item == item->flat.item_index);
    int			isSensitive = item->flat.sensitive == True &&
					XtIsSensitive(w) == True;
    OlDefine		current_gui = OlGetGui();

		/* Let the label routine do most of the work for us.	*/

    OlFlatSetupLabelSize(w, item, ppLbl, &sizeProc);
    plbl = (MyLabel *)*ppLbl;

	    /* Now get attributes suitable for drawing the button	*/

    if (has_focus || item->flat.background_pixel != di->background)
    {
        Pixel	bg = ((has_focus && (current_gui == OL_OPENLOOK_GUI)) ?
		      item->flat.input_focus_color :
		      item->flat.background_pixel); 

			/* Try and use previously allocated attributes. */
	if (sc->alt_attrs == (OlgAttrs *)NULL ||
	    sc->alt_bg != bg || sc->alt_fg != item->flat.foreground)
	{
		if (sc->alt_attrs != (OlgAttrs *)NULL)
		{
			OlgDestroyAttrs(sc->alt_attrs);
		}
		sc->alt_bg = bg;
		sc->alt_fg = item->flat.foreground;
		sc->alt_attrs = OlgCreateAttrs (sc->screen, sc->alt_fg,
						(OlgBG *)&(sc->alt_bg),
						False, psize);
	}
	*ppAttrs = sc->alt_attrs;
    }
    else
    {
	*ppAttrs = fp->pAttrs;
    }

    /* Set up label GC */
    if (!has_focus &&
	(item->flat.font == OlFlatDefaultItem(w)->flat.font &&
	item->flat.font_color == OlFlatDefaultItem(w)->flat.font_color &&
	item->flat.background_pixel ==
			OlFlatDefaultItem(w)->flat.background_pixel &&
	(item->flat.label == (String) NULL || isSensitive)))
    {
	label_GC = fp->label_gc;
	inverse_GC = fp->inverse_gc;
    }
    else
    {
	XGCValues	values;
	Pixel		fg;		/* foreground for normal GC	*/
	Pixel		bg;		/* background for normal GC	*/
	unsigned long	mask = (GCForeground|GCBackground|GCFont|
				GCGraphicsExposures|GCFillStyle);

	label_GC			= sc->scratch_gc;
	values.graphics_exposures	= (Bool)False;
	values.fill_style		= FillSolid;
	values.font			= item->flat.font->fid;
	if (has_focus) {
		Pixel	ifc = item->flat.input_focus_color;

		if ((ifc == item->flat.font_color) &&
		    (current_gui == OL_OPENLOOK_GUI))
		{
			fg = item->flat.background_pixel;
			bg = item->flat.font_color;
		}
		else
		{
			fg = item->flat.font_color;
			bg = (current_gui == OL_OPENLOOK_GUI) ? ifc :
			  item->flat.background_pixel;
		}
	}
	else
	{
		fg = item->flat.font_color;
		bg = item->flat.background_pixel;
	}
	values.foreground	= fg;
	values.background	= bg;

	if (item->flat.label != (String) NULL)
	{
	    if (!isSensitive)
	    {
		values.stipple = OlgGetInactiveStipple (*ppAttrs);
		values.fill_style = FillStippled;
		mask |= GCStipple;
	    }
	}

	/* Update the scratch GC after copying default values. */

	XCopyGC(dpy, sc->default_gc, (unsigned long)~0, label_GC);
	XChangeGC(dpy, label_GC, mask, &values);

	/* Get an inverse GC if the label is a string */
	if (item->flat.label != (String) NULL)
	{
	    values.foreground	= bg;
	    values.background	= fg;
	    inverse_GC		= OlgGetScratchGC (*ppAttrs);

	    XCopyGC(dpy, sc->default_gc, (unsigned long)~0, inverse_GC);
	    XChangeGC(dpy, inverse_GC, mask, &values);
	}
    }

			/* Now write the GC information into the label	*/

    if (item->flat.label != (String) NULL)
    {
	plbl->text.normalGC	= label_GC;
	plbl->text.inverseGC	= inverse_GC;
	*ppDrawProc		= (OlgLabelProc)OlgDrawTextLabel;
    }
    else if (item->flat.label_image != (XImage *) NULL ||
	     (item->flat.background_pixmap != (Pixmap)None &&
	      item->flat.background_pixmap != (Pixmap)ParentRelative &&
	      item->flat.background_pixmap != XtUnspecifiedPixmap))
    {
	plbl->pixmap.normalGC	= label_GC;
	*ppDrawProc		= (OlgLabelProc)OlgDrawPixmapLabel;
    }
    else
    {
	*ppDrawProc = (OlgLabelProc)nullProc;
    }
} /* END OF OlFlatSetupAttributes() */

/*
 *************************************************************************
 * OlFlatSetupLabelSize - this routine populates a label structure with
 * the elements needed to calculate the size of the label.  It also selects
 * a function to calculate the size.  Note that this function destroys the
 * contents of the previous label.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
OlFlatSetupLabelSize OLARGLIST((w, item, ppLbl, ppSizeProc))
	OLARG( Widget,		w)
	OLARG( FlatItem,	item)
	OLARG( XtPointer *,	ppLbl)
	OLGRA( void,		(**ppSizeProc)())
{
	static union {
	    OlgTextLbl		text;
	    OlgPixmapLbl	pixmap;
	} lbl;
	unsigned char	justification =
				   CVT_JUSTIFY(item->flat.label_justify);
	int		isSensitive = item->flat.sensitive == True &&
					XtIsSensitive(w) == True;

	*ppLbl = (XtPointer) &lbl;

	if (item->flat.label != (String)NULL)
	{
		lbl.text.label		= item->flat.label;
		lbl.text.mnemonic	= item->flat.mnemonic;
		lbl.text.accelerator	= item->flat.accelerator_text;
		lbl.text.font		= item->flat.font;
		lbl.text.flags		= 0;
		lbl.text.justification	= justification;
		lbl.text.font_list	= ((PrimitiveWidget)w)->primitive.font_list;
		*ppSizeProc		= OlgSizeTextLabel;
	}
	else if (item->flat.label_image != (XImage *) NULL ||
	     (item->flat.background_pixmap != (Pixmap)None &&
	      item->flat.background_pixmap != (Pixmap)ParentRelative &&
	      item->flat.background_pixmap != XtUnspecifiedPixmap))
	{
		lbl.pixmap.flags		= 0;
		lbl.pixmap.justification	= justification;
		*ppSizeProc			= OlgSizePixmapLabel;

		if (item->flat.label_tile)
		{
			lbl.pixmap.flags |= PL_TILED;
		}

		if (!isSensitive)
		{
			lbl.pixmap.flags |= PL_INSENSITIVE;
			lbl.pixmap.stippleColor = item->flat.background_pixel;
		}

		if (item->flat.label_image != (XImage *)NULL)
		{
			lbl.pixmap.type		= PL_IMAGE;
			lbl.pixmap.label.image	= item->flat.label_image;
		}
		else
		{
			lbl.pixmap.type		= PL_PIXMAP;
			lbl.pixmap.label.pixmap	= item->flat.background_pixmap;
		}
	}
	else
	{
	        *ppSizeProc = nullSize;
	}
} /* END OF OlFlatSetupLabelSize */
