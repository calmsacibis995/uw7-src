/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)flat:FColors.c	1.12"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/Olg.h>
#include <Xol/ColorChip.h>
#include <Xol/FColorsP.h>

#define ClassName FlatColors
#include <Xol/NameDefs.h>

/*
 * Convenient macros:
 */

#define FLAT_P(w)	 ((FlatColorsWidget)(w))->flat
#define EXCLUSIVES_P(w)  ((FlatColorsWidget)(w))->exclusives
#define ROWCOLUMN_P(w)	 ((FlatColorsWidget)(w))->row_column
#define EXCLUSIVES_IP(i) ((FlatColorsItem)(i))->exclusives
#define FLAT_IP(i)       ((FlatColorsItem)(i))->flat

/*
 * Resources:
 */

static XtResource	resources[] = {

#define offset(F)       XtOffsetOf(FlatColorsRec,F)

	/*
	 * Override some superclass resources:
	 */
    {
	XtNbuttonType, XtCButtonType,
	XtROlDefine, sizeof(OlDefine), offset(exclusives.button_type),
	XtRImmediate, (XtPointer)OL_RECT_BTN
    },
	/* Note that XtNnoneSet is False by default in flatButtons	*/
    {
	XtNexclusives, XtCExclusives,
	XtRBoolean, sizeof(Boolean), offset(exclusives.exclusive_settings),
	XtRImmediate,(XtPointer)True
    },
    {
	XtNsameWidth, XtCSameWidth,
	XtROlDefine, sizeof(OlDefine), offset(row_column.same_width),
	XtRImmediate, (XtPointer)OL_ALL
    },
    {
	XtNsameHeight, XtCSameHeight,
	XtROlDefine, sizeof(OlDefine), offset(row_column.same_height),
	XtRImmediate, (XtPointer)OL_ALL
    },
    {
	XtNitemMinHeight, XtCItemMinHeight,
	XtRDimension, sizeof(Dimension), offset(row_column.item_min_height),
	XtRString, (XtPointer)"23 vertical points"
    },
    {
	XtNitemMinWidth, XtCItemMinWidth,
	XtRDimension, sizeof(Dimension), offset(row_column.item_min_width),
	XtRString, (XtPointer)"18 horizontal points"
    },

#undef	offset
};

/*
 * Translations and actions:
 */

/*
 * Note: since the 'augment' and 'override' directives don't work for
 * class translations, we have to copy the generic translations and then
 * append what we need.  See the ClassInitialize Procedure.
 */
#if	Xt_augment_works_right
OLconst static char	translations[] = "#augment\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction() \n\
";
#else
OLconst static char	translations[] = "\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction() \n\
";
#endif

/*
 * Local routines:
 */

static void		ClassInitialize OL_NO_ARGS();

static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Destroy OL_ARGS((
	Widget			w
));
static void		DrawItem OL_ARGS((
	Widget			w,
	FlatItem		item,
	OlFlatDrawInfo *	di
));
static void		ItemDimensions OL_ARGS((
	Widget			w,
	FlatItem		item,
	register Dimension *	p_width,
	register Dimension *	p_height
));
static Boolean ItemLocCursorDims OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));

/*
 * Class record structure:
 */

FlatColorsClassRec	flatColorsClassRec = {
    /*
     * Core class:
     */
    {
    /* superclass          */	(WidgetClass)&flatButtonsClassRec,
    /* class_name          */	"FlatColors",
    /* widget_size         */	sizeof(FlatColorsRec),
    /* class_initialize    */	ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* class_inited        */	FALSE,
    /* initialize          */	NULL,
    /* initialize_hook     */	NULL,
    /* realize             */	XtInheritRealize,
    /* actions             */	NULL,
    /* num_actions         */	0,
    /* resources           */	resources,
    /* num_resources       */	XtNumber(resources),
    /* xrm_class           */	NULLQUARK,
    /* compress_motion     */	TRUE,
    /* compress_exposure   */	TRUE,
    /* compress_enterleave */	TRUE,
    /* visible_interest    */	FALSE,
    /* destroy             */	Destroy,
    /* resize              */	XtInheritResize,
    /* expose              */	XtInheritExpose,
    /* set_values          */	NULL,
    /* set_values_hook     */	NULL,
    /* set_values_almost   */	XtInheritSetValuesAlmost,
    /* get_values_hook     */	NULL,
    /* accept_focus        */	XtInheritAcceptFocus,
    /* version             */	XtVersion,
    /* callback_offsets    */	NULL,
#if	Xt_augment_works_right
    /* tm_table            */	translations,
#else
    /* tm_table            */	NULL,
#endif
    /* query_geometry      */	XtInheritQueryGeometry,
    /* display_accelerator */	XtInheritDisplayAccelerator,
    /* extension           */	NULL
    }, /* End of Core Class Part Initialization */
    {
    /* focus_on_select	   */	True,
    /* highlight_handler   */	XtInheritHighlightHandler,
    /* traversal_handler   */	XtInheritTraversalHandler,
    /* register_focus      */	NULL,
    /* activate            */	XtInheritActivateFunc,
    /* event_procs         */	NULL,
    /* num_event_procs     */	0,
    /* version             */	OlVersion,
    /* extension           */	NULL
    }, /* End of Primitive Class Part Initialization */
    {
    /* extension	   */	(XtPointer) NULL,
    /* transparent_bg      */	True,
    /* default_offset	   */	XtOffsetOf(FlatColorsRec, default_item),
    /* rec_size		   */	sizeof(FlatColorsItemRec),
    /* item_resources      */	NULL,
    /* num_item_resources  */	0,
    /* required_resources  */	NULL,
    /* num_required_resources*/	0,

		/*
		 *  See ClassInitialize for procedures
		 */

    }, /* End of Flat Class Part Initialization */
    {
    /* unused              */	0,
    }, /* End of FlatRowColumn Class Part Initialization */
    {
    /* unused              */	0,
    }, /* End of FlatButtons (Exclusives) Class Part Initialization */
    {
    /* unused              */	0,
    }, /* End of FlatColors Class Part Initialization */
};

WidgetClass	flatColorsWidgetClass = (WidgetClass)&flatColorsClassRec;

/**
 ** ClassInitialize()
 **/

static void
ClassInitialize OL_NO_ARGS()
{
#ifndef	Xt_augment_works_right
	char *			t;

	t = XtMalloc(
	    strlen(translations) + strlen(_OlGenericTranslationTable) + 1
	);

	sprintf (t, "%s%s", translations, _OlGenericTranslationTable);
	flatColorsWidgetClass->core_class.tm_table = t;

#endif

		/* Inherit all superclass procedures, but	*/
		/* override a few as well provide some chained	*/
		/* procedures. This scheme saves us from	*/
		/* worrying about putting function pointers	*/
		/* in the wrong class slot if they were 	*/
		/* statically declared. It also allows us to	*/
		/* inherit new functions simply be recompiling,	*/
		/* i.e., we don't have to stick XtInheritFoo	*/
		/* into the class slot.				*/
	OlFlatInheritAll(flatColorsWidgetClass);

#undef F
#define F	flatColorsClassRec.flat_class

	F.initialize		= Initialize;
	F.draw_item		= DrawItem;
	F.item_dimensions	= ItemDimensions;

	if (OlGetGui() == OL_MOTIF_GUI)
	{
		F.item_location_cursor_dimensions = ItemLocCursorDims;
	}

#undef F
} /* ClassInitialize */

/**
 ** Initialize()
 **/

static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	_OlCreateColorChip (new);
	return;
} /* Initialize */

/**
 ** Destroy()
 **/

static void
Destroy OLARGLIST((w))
	OLGRA( Widget,		w)
{
	_OlDestroyColorChip (w);
	return;
} /* Destroy */

/**
 ** DrawItem()
 **/

static void
DrawItem OLARGLIST((w, item, dii))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlFlatDrawInfo *,	dii)
{
	Cardinal		item_index = item->flat.item_index;

	Boolean			draw_selected;

	unsigned int		flags      = 0;

	OlgAttrs *		item_attrs;

	OlgLabelProc		ignore_draw_proc;

	union {
		OlgTextLbl		text;
		OlgPixmapLbl		pixmap;
	}			ignore_label;

	_OlColorChipLabel	color;

	Dimension		margin = (EXCLUSIVES_P(w).menu_descendant? 1:0);

	OlFlatDrawInfo		DI;
	OlFlatDrawInfo *	di = &DI;
	Boolean			motif_gui = (OlGetGui() == OL_MOTIF_GUI);


	if (!item->flat.mapped_when_managed)
		return;

	DI = *dii;

	if ( motif_gui && !EXCLUSIVES_P(w).exclusive_settings )
	{
#define THICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness
#define SUPERCLASS1 \
	  ((FlatColorsClassRec *)flatColorsClassRec.core_class.superclass)
#define SUPERCLASS \
	  ((FlatButtonsClassRec *)SUPERCLASS1->core_class.superclass)

		Dimension	h2 = 2 * THICKNESS;

		if ( FLAT_P(w).focus_item == item_index )
			(*SUPERCLASS->flat_class.draw_item)(w, item, di);

		di->x += THICKNESS;
		di->y += THICKNESS;
		di->width -= h2;
		di->height -= h2;

#undef THICKNESS
	}

	if (EXCLUSIVES_P(w).exclusive_settings) {
		draw_selected = (Boolean)
			((item_index == EXCLUSIVES_P(w).current_item &&
			!(EXCLUSIVES_P(w).none_set == True &&
			EXCLUSIVES_P(w).current_item == EXCLUSIVES_P(w).set_item))
					||
			(EXCLUSIVES_P(w).current_item == (Cardinal) OL_NO_ITEM &&
		     	EXCLUSIVES_IP(item).set == True) ? True : False);

	} else {
		draw_selected = (Boolean)
			((item_index == EXCLUSIVES_P(w).current_item &&
			  EXCLUSIVES_IP(item).set == False)
					||
	     		(EXCLUSIVES_P(w).current_item != item_index &&
			EXCLUSIVES_IP(item).set == True) ? True : False);
	}

	if (EXCLUSIVES_P(w).preview)
		draw_selected = (draw_selected == True ? False : True);

	if (draw_selected)
	        flags |= RB_SELECTED;

	if (
		EXCLUSIVES_P(w).default_item == item_index
	     || EXCLUSIVES_IP(item).is_default
	)
	        flags |= RB_DEFAULT;

	if (EXCLUSIVES_P(w).dim == True)
	        flags |= RB_DIM;

	if (!item->flat.sensitive || !XtIsSensitive(w)) {
	        flags |= RB_INSENSITIVE;
		color.insensitive = True;
	} else
		color.insensitive = False;

	OlFlatSetupAttributes (
		w, item, di, &item_attrs,
		(XtPointer *)&ignore_label, &ignore_draw_proc
	);

	color.pixel = FLAT_IP(item).foreground;
	_OlgDrawRectButton (
		di->screen,
		di->drawable,
		item_attrs,
		di->x,     di->y,
		di->width, di->height,
		&color,
		_OlDrawColorChip,
		flags,
		margin, margin
	);

	if ( motif_gui && EXCLUSIVES_P(w).exclusive_settings &&
	     FLAT_P(w).focus_item == item_index )
	{
		(*SUPERCLASS->flat_class.draw_item)(w, item, di);

#undef SUPERCLASS1
#undef SUPERCLASS
	}

	return;
} /* DrawItem */

/**
 ** ItemDimensions()
 **/

static void
ItemDimensions OLARGLIST((w, item, p_width, p_height))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLARG( register Dimension *,	p_width)
	OLGRA( register Dimension *,	p_height)
{
	if (p_width)
		*p_width = ROWCOLUMN_P(w).item_min_width;
	if (p_height)
		*p_height = ROWCOLUMN_P(w).item_min_height;

		/* Add space for Location Cursor */
	if (OlGetGui() == OL_MOTIF_GUI)
	{
#define THICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness

		Dimension	h2 = 2 * THICKNESS;

		*p_width += h2;
		*p_height += h2;

#undef THICKNESS
	}

	return;
} /* ItemDimensions */

/*
 * ItemLocCursorDims - figure out the place to draw location cursor.
 */
static Boolean
ItemLocCursorDims OLARGLIST((w, item, di))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlFlatDrawInfo *,	di)
{
	OlgAttrs *	attrs = FLAT_P(w).pAttrs;
	Dimension	h_sp = OlgGetHorizontalStroke(attrs),
			v_sp = OlgGetVerticalStroke(attrs);

	if (EXCLUSIVES_P(w).exclusive_settings)
	{
		di->x += h_sp;
		di->y += v_sp;
		di->width -= (h_sp + h_sp);
		di->height -= (v_sp + v_sp);
	}

	return(True);
} /* end of ItemLocCursorDims */
