#ifndef	NOIDENT
#ident	"@(#)caption:Caption.c	2.16"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/CaptionP.h"
#include "Xol/Error.h"

#define ClassName Caption
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&captionClassRec)
#define superClass	((WidgetClass)&managerClassRec)
#define className	"Caption"

/*
 * Define the following if you want to be able to turn on debugging
 * information in the binary product.
 */
#define	CAPTION_DEBUG
#if	defined(CAPTION_DEBUG)
#include "stdio.h"
static Boolean		caption_debug = False;
#endif

/*
 * Convenient macros:
 */

#define ClearArea(W,RECT,EXPOSURES) \
	XClearArea (							\
		XtDisplay(W), XtWindow(W),				\
		(RECT)->x, (RECT)->y, (RECT)->width, (RECT)->height,	\
		EXPOSURES						\
	)

	/*
	 * Some typed numbers to keep our pal, the ANSI-C compiler, happy.
	 */
#if	defined(__STDC__)
# define u2	2U
#else
# define u2	2
#endif

#define STREQU(A,B) (strcmp((A),(B)) == 0)

/*
 * Local routines:
 */

static void		ClassInitialize OL_ARGS((
	void
));
static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		Destroy OL_ARGS((
	Widget			w
));
static void		ExposeProc OL_ARGS((
	Widget			w,
	XEvent *		xevent,		/*NOTUSED*/
	Region			region		/*NOTUSED*/
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		GetValuesHook OL_ARGS((
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
));
static void		ChangeManaged OL_ARGS((
	Widget			w
));
static Boolean		Activate OL_ARGS((
	Widget			w,
	OlVirtualName		type,		/*NOTUSED*/
	XtPointer		data		/*NOTUSED*/
));
static void		Layout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,/*NOTUSED*/
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static XtGeometryResult	QueryAlignment OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static void		ComputeGeometries OL_ARGS((
	Widget			w,
	Widget			child,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	label_geometry,
	XtWidgetGeometry *	child_geometry,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate
));
static void		ComputeAggregateAndAdjust OL_ARGS((
	Cardinal		pass,
	Cardinal		para_dim_offset,  /* parallel */
	Cardinal		perp_dim_offset,  /* perpendicular */
	Dimension		space,
	XtWidgetGeometry *	label_geometry,
	XtWidgetGeometry *	child_geometry,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	available
));
static void		ComputePerpendicularPosition OL_ARGS((
	OlDefine		alignment,
	Cardinal		pos,
	Cardinal		dim,
	XtWidgetGeometry *	label_geometry,
	XtWidgetGeometry *	child_geometry
));
static void		SetTextLabel OL_ARGS((
	Widget			w,
	OlgTextLbl *		label
));
static void		GeometryToRect OL_ARGS((
	XtWidgetGeometry *	geometry,
	XRectangle *		rect
));
static Widget		FetchChild OL_ARGS((
	Widget			w,
	Boolean *		more
));
static void		CheckResources OL_ARGS((
	Widget			new,
	Widget			current
));
static void		SetDefaultLabelResource OL_ARGS((
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
));
static void		GetGCs OL_ARGS((
	Widget			w
));
static void		FreeGCs OL_ARGS((
	Widget			w
));

/*
 * Resources:
 */

#define FONT_COLOR \
    {	/* SGI */							\
	XtNfontColor, XtCFontColor,					\
	XtRPixel, sizeof(Pixel), offset(caption.font_color),		\
	XtRString, (XtPointer)XtDefaultForeground			\
    }
#define FONT \
    {	/* SGI */							\
	XtNfont, XtCFont,						\
	XtRFontStruct, sizeof(XFontStruct *), offset(caption.font),	\
	XtRString, OlDefaultBoldFont					\
    }
#define FONT_GROUP \
    {	/* SGI */							\
	XtNfontGroup, XtCFontGroup,					\
	XtROlFontList, sizeof(OlFontList *), offset(caption.font_list),	\
	XtRString, (XtPointer)NULL					\
    }

static _OlDynResource	dynamic_resources[] = {
#define offset(F) XtOffsetOf(CaptionRec, F)
#define DYNFLAG   offset(caption.dynamics)

  {	FONT_COLOR,
	DYNFLAG, _CAPTION_B_DYNAMIC_FONTCOLOR, NULL
  },
  {	FONT,
	DYNFLAG, _CAPTION_B_DYNAMIC_FONT, NULL
  },
  {	FONT_GROUP,
	DYNFLAG, _CAPTION_B_DYNAMIC_FONTGROUP, NULL
  },

#undef	offset
#undef	DYNFLAG
};

static OlDefine		DefaultShadowType = OL_SHADOW_OUT;

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(CaptionRec,F)

    /*
     * This resource is from Manager, but we need to change the default
     * when in Motif mode.
     */
    {	/* SGI */
	XtNshadowType, XtCShadowType,
	XtROlDefine, sizeof(OlDefine), offset(manager.shadow_type),
	XtROlDefine, (XtPointer)&DefaultShadowType
    },

    /*
     * New resources:
     */
    FONT_COLOR,
    FONT,
#if	defined(I18N)
    FONT_GROUP,
#endif
    {	/* SGI */
	XtNlabel, XtCLabel,
	XtRString, sizeof(String), offset(caption.label),
	XtRCallProc, (XtPointer)SetDefaultLabelResource
    },
    {	/* SGI */
	XtNlabelPixmap, XtCLabelPixmap,
	XtRPixmap, sizeof(Pixmap), offset(caption.label_pixmap),
	XtRImmediate, (XtPointer)XtUnspecifiedPixmap
    },
    {	/* SGI */
	XtNlabelImage, XtCLabelImage,
	XtRPointer, sizeof(XImage *), offset(caption.label_image),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNposition, XtCPosition,
	XtROlDefine, sizeof(OlDefine), offset(caption.position),
	XtRImmediate, (XtPointer)OL_LEFT
    },
    {	/* SGI */
	XtNalignment, XtCAlignment,
	XtROlDefine, sizeof(OlDefine), offset(caption.alignment),
	XtRImmediate, (XtPointer)OL_CENTER
    },
    {	/* SGI */
	XtNspace, XtCSpace,
	XtRDimension, sizeof(Dimension), offset(caption.space),
	XtRImmediate, (XtPointer)4
    },
    {	/* SGI */
	XtNmnemonic, XtCMnemonic,
	OlRChar, sizeof(char), offset(caption.mnemonic),
	XtRImmediate, (XtPointer)'\0'
    },
    {	/* SGI */
	XtNlayoutWidth, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(caption.layout.width),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },
    {	/* SGI */
	XtNlayoutHeight, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(caption.layout.height),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },

#undef	offset
};

/*
 * Class record structure:
 *
 *	(I)	XtInherit'd field
 *	(D)	Chained downward (super-class to sub-class)
 *	(U)	Chained upward (sub-class to super-class)
 */

static
LayoutCoreClassExtensionRec	layoutCoreClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */ (XrmQuark) 0, /* see ClassInitialize */
/* version              */            OlLayoutCoreClassExtensionVersion,
/* record_size          */            sizeof(LayoutCoreClassExtensionRec),
/* layout            (I)*/            Layout,
/* query_alignment   (I)*/            QueryAlignment,
};

CaptionClassRec		captionClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(CaptionRec),
/* class_initialize     */                       ClassInitialize,
/* class_part_init   (D)*/ (XtWidgetClassProc)   0,
/* class_inited         */                       False,
/* initialize        (D)*/                       Initialize,
/* initialize_hook   (D)*/ (XtArgsProc)          0, /* Obsolete */
/* realize           (I)*/                       XtInheritRealize,
/* actions           (U)*/ (XtActionList)        0,
/* num_actions          */ (Cardinal)            0,
/* resources         (D)*/                       resources,
/* num_resources        */                       XtNumber(resources),
/* xrm_class            */                       NULLQUARK,
/* compress_motion      */                       True,
/* compress_exposure    */                       XtExposeCompressSeries,
/* compress_enterleave  */                       True,
/* visible_interest     */                       False,
/* destroy           (U)*/                       Destroy,
/* resize            (I)*/                       XtInheritResize,
/* expose            (I)*/                       ExposeProc,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/                       GetValuesHook,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       XtInheritQueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           &layoutCoreClassExtension
	},
	/*
	 * Composite class
	 */
	{
/* geometry_manager  (I)*/                       XtInheritGeometryManager,
/* change_managed    (I)*/                       ChangeManaged,
/* insert_child      (I)*/                       XtInheritInsertChild,
/* delete_child      (I)*/                       XtInheritDeleteChild,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Constraint class:
	 */
	{
/* resources            */ (XtResourceList)      0,
/* num_resources        */ (Cardinal)            0,
/* constraint_size      */ (Cardinal)            0,
/* initialize        (D)*/ (XtInitProc)          0,
/* destroy           (U)*/ (XtWidgetProc)        0,
/* set_values        (D)*/ (XtSetValuesFunc)     0,
/* extension            */ (XtPointer)           0,
	},
	/*
	 * Manager class:
	 */
	{
/* highlight_handler (I)*/                       XtInheritHighlightHandler,
/* focus_on_select      */			 True,
#if	defined(OL_VERSION) && OL_VERSION < 5
/* reserved             */ (XtPointer)           0,
#endif
/* traversal_handler (I)*/                       XtInheritTraversalHandler,
/* activate          (I)*/                       Activate,
/* event_procs          */ (OlEventHandlerList)  0,
/* num_event_procs      */ (Cardinal)            0,
/* register_focus    (I)*/                       XtInheritRegisterFocus,
#if	defined(OL_VERSION) && OL_VERSION < 5
/* reserved             */ (XtPointer)           0,
#endif
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { dynamic_resources, XtNumber(dynamic_resources) },
/* transparent_proc  (I)*/                       XtInheritTransparentProc,
	},
	/*
	 * Caption class:
	 */
	{
/* extension            */ (XtPointer)           0
	}
};

WidgetClass		captionWidgetClass = thisClass;

/**
 ** ClassInitialize()
 **/

static void
#if	OlNeedFunctionPrototypes
ClassInitialize (
	void
)
#else
ClassInitialize ()
#endif
{
	/*
	 * For XtNposition and XtNalignment:
	 */
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("center", OL_CENTER);
	_OlAddOlDefineType ("right",  OL_RIGHT);
	_OlAddOlDefineType ("top",    OL_TOP);
	_OlAddOlDefineType ("bottom", OL_BOTTOM);

	/*
	 * For XtNlayoutWidth and XtNlayoutHeight:
	 */
	_OlAddOlDefineType ("maximize", OL_MAXIMIZE);
	_OlAddOlDefineType ("minimize", OL_MINIMIZE);
	_OlAddOlDefineType ("ignore",   OL_IGNORE);

	layoutCoreClassExtension.record_type = XtQLayoutCoreClassExtension;

	if (OlGetGui() == OL_MOTIF_GUI)
		DefaultShadowType = OL_SHADOW_IN;

	return;
} /* ClassInitialize */

/**
 ** Initialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
Initialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	XtWidgetGeometry	pref;


	OlCheckLayoutResources (
		new, &CAPTION_P(new).layout, (OlLayoutResources *)0
	);
	CheckResources (new, (Widget)0);

	if (CAPTION_P(new).label)
		CAPTION_P(new).label = XtNewString(CAPTION_P(new).label);

	if (CAPTION_P(new).mnemonic)
		if (_OlAddMnemonic(new, (XtPointer)0, CAPTION_P(new).mnemonic) != OL_SUCCESS)
			CAPTION_P(new).mnemonic = 0;

	GetGCs (new);

	CAPTION_P(new).label_geometry.x = 0;
	CAPTION_P(new).label_geometry.y = 0;
	CAPTION_P(new).label_geometry.width = 0;
	CAPTION_P(new).label_geometry.height = 0;

	/*
	 * For the purpose of geometry management we treat the label
	 * as a RectObj "child" (it isn't, of course). This means we
	 * keep track of the label's geometry and, when it changes,
	 * we expose the old and new geometries, etc., just like Xt
	 * does for RectObj's. Since this "child" is always here and
	 * always managed, we can compute the current layout. Doing
	 * this will ensure we have a non-zero geometry.
	 *
	 * However, the client (or user) may have set an initial size
	 * that we must obey. What's worse, perhaps only one of width and
	 * height have been set. In this worst case, Layout gets called
	 * twice, first to find the preferred size of the unspecified
	 * dimension, then to set both initial dimensions.
	 *
	 * The following is equivalent to OlInitializeGeometry, except
	 * that an initial query is made (if necessary) and XtResizeWidget
	 * is called instead of poking the core fields directly; this is
	 * because we need to properly size and lay out the caption.
	 *
	 * Note: We check request, so that we can determine if the client
	 * (or user) set a dimension; but we size with new, so that a
	 * superclass can set up, e.g., a minimum size.
	 */
	if (!CORE_P(request).width || !CORE_P(request).height)
		OlQueryChildGeometry (new, (XtWidgetGeometry *)0, &pref);
	CAPTION_P(new).layout.flags
			&= ~(OlLayoutWidthNotSet|OlLayoutHeightNotSet);
#define SET(FLAG,FIELD) \
	if (CORE_P(request).FIELD)					\
		pref.FIELD = CORE_P(new).FIELD;				\
	else								\
		CAPTION_P(new).layout.flags |= FLAG

	SET (OlLayoutWidthNotSet, width);
	SET (OlLayoutHeightNotSet, height);
#undef	SET
	XtResizeWidget (
		new, pref.width, pref.height, CORE_P(new).border_width
	);
	
#if	defined(CAPTION_DEBUG)
	caption_debug = (getenv("CAPTION_DEBUG") != 0);
#endif
	return;
} /* Initialize */

/**
 ** Destroy()
 **/

static void
#if	OlNeedFunctionPrototypes
Destroy (
	Widget			w
)
#else
Destroy (w)
	Widget			w;
#endif
{
	if (CAPTION_P(w).label)
		XtFree (CAPTION_P(w).label);
	FreeGCs (w);
	return;
} /* Destroy */

/**
 ** ExposeProc()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
ExposeProc (
	Widget			w,
	XEvent *		xevent,		/*NOTUSED*/
	Region			region		/*NOTUSED*/
)
#else
ExposeProc (w, xevent, region)
	Widget			w;
	XEvent *		xevent;
	Region			region;
#endif
{
	Display *		display = XtDisplay(w);

	Screen *		screen  = XtScreen(w);

	Dimension		width;		/*SET,NOTUSED*/
	Dimension		height;

	OlgTextLbl		label;


	/*
	 * If we have a region for the exposure, see if it overlaps the
	 * label's geometry. If so, we'll continue with drawing the
	 * label, but we'll use a clip mask to limit the server's work.
	 * Note: OlgDrawTextLabel would set the clip mask if the height
	 * passed to it is less than the height needed for the label.
	 * This would clobber the clip mask we set here, so we pass a
	 * height large enough for the label. But in turn, we have to
	 * augment the region passed to include the clipping necessary
	 * due to the limited geometry at hand. This keeps the label from
	 * clobbering the shadow.
	 */
	if (region) {
		static Region		scratch = 0;
		Region			r;

		switch (XRectInRegion(
			region,
			CAPTION_P(w).label_geometry.x,
			CAPTION_P(w).label_geometry.y,
			CAPTION_P(w).label_geometry.width,
			CAPTION_P(w).label_geometry.height
		)) {
		case RectangleIn:
			r = region;
			break;
		case RectanglePart:
			if (!scratch)
				scratch = XCreateRegion();
			OlIntersectRectWithRegion (
			     &CAPTION_P(w).label_geometry, region, scratch
			);
			r = scratch;
			break;
		case RectangleOut:
		default:
			goto Skip;
		}

		XSetRegion (display, CAPTION_P(w).normal_gc, r);
		XSetRegion (display, CAPTION_P(w).inverse_gc, r);
	}
	SetTextLabel (w, &label);
	OlgSizeTextLabel (
		screen, CAPTION_P(w).attrs, &label, &width, &height
	);
	OlgDrawTextLabel (
		XtScreen(w), XtWindow(w),
		CAPTION_P(w).attrs,
		CAPTION_P(w).label_geometry.x,
		CAPTION_P(w).label_geometry.y,
		CAPTION_P(w).label_geometry.width,
		height,
		&label
	);

	/*
	 * We share these GCs, so we have to restore the clip mask.
	 */
	XSetClipMask (display, CAPTION_P(w).normal_gc, None);
	XSetClipMask (display, CAPTION_P(w).inverse_gc, None);

	/*
	 * Now envelope the superclass expose procedure to get the shadow.
	 */
Skip:	if (CORE_C(superClass).expose)
		(*CORE_C(superClass).expose) (w, xevent, region);

	return;
} /* ExposeProc */

/**
 ** SetValues()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
SetValues (
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
SetValues (current, request, new, args, num_args)
	Widget			current;
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Boolean			new_label = False;
	Boolean			redisplay = False;
	Boolean			do_layout = False;

#define DIFFERENT(F) \
	(((CaptionWidget)new)->F != ((CaptionWidget)current)->F)


	OlCheckLayoutResources (
		new, &CAPTION_P(new).layout, &CAPTION_P(current).layout
	);
	CheckResources (new, current);

	if (DIFFERENT(caption.label)) {
		if (CAPTION_P(current).label)
			XtFree (CAPTION_P(current).label);
		if (CAPTION_P(new).label)
			CAPTION_P(new).label = XtNewString(CAPTION_P(new).label);
		new_label = True;
	}

	if (DIFFERENT(caption.mnemonic)) {
		if (CAPTION_P(current).mnemonic)
			_OlRemoveMnemonic (new, (XtPointer)0, False, CAPTION_P(current).mnemonic);
		if (CAPTION_P(new).mnemonic)
			if (_OlAddMnemonic(new, (XtPointer)0, CAPTION_P(new).mnemonic) != OL_SUCCESS)
				CAPTION_P(new).mnemonic = 0;
		new_label = True;
	}

	if (
		DIFFERENT(caption.font)
#if	defined(I18N)
	     || DIFFERENT(caption.font_list)
#endif
	)
		new_label = True;

	if (
		DIFFERENT(core.sensitive)
	     || DIFFERENT(core.background_pixel)
	     || DIFFERENT(caption.font_color)
	)
		redisplay = True;

	if (
		DIFFERENT(caption.position)
	     || DIFFERENT(caption.alignment)
	     || DIFFERENT(caption.space)
	     || DIFFERENT(caption.layout.width)
	     || DIFFERENT(caption.layout.height)
	     || DIFFERENT(manager.shadow_thickness)
	)
		do_layout = True;

	if (
		DIFFERENT(caption.font->fid)
	     || DIFFERENT(core.background_pixel)
	     || DIFFERENT(caption.font_color)
	) {
		FreeGCs (new);
		GetGCs (new);
	}

	/*
	 * MORE: If the client changes the width/height at the same time
	 * as it changes other resources, then do_layout and/or redisplay
	 * should be ignored? This may require a set_values_hook?
	 */

	if (new_label)
		do_layout = True;

	if ((new_label || redisplay && !do_layout) && XtIsRealized(new))
		/*
		 * If the only thing(s) that changed require a simple
		 * redrawing of the label, or if the entire label is now
		 * suspect, clear the label area. Generate exposures
		 * only for the simple case where a redisplay is needed
		 * but no relayout.
		 */
		ClearArea (
			new, &CAPTION_P(new).label_geometry,
			(redisplay && !do_layout)
		);

	if (new_label) {
		/*
		 * With a new label, clobber the old label geometry since
		 * it no longer matches. The XClearArea above was enough
		 * to clear out the old label, Layout will take care of
		 * the new label.
		 */
		CAPTION_P(new).label_geometry.width = 0;
		CAPTION_P(new).label_geometry.height = 0;
	}

	OlLayoutWidgetIfLastClass (
		new, captionWidgetClass, do_layout, True
	);

#undef	DIFFERENT
	return (False);
} /* SetValues */

/**
 ** GetValuesHook()
 **/

static void
#if	OlNeedFunctionPrototypes
GetValuesHook (
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
)
#else
GetValuesHook (w, args, num_args)
	Widget			w;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Arg *			arg;

	Cardinal		n;


	for (n = 0, arg = args; n < *num_args; n++, arg++)
		if (STREQU(arg->name, XtNcaptionWidth))
			*(Dimension *)(arg->value)
				= CAPTION_P(w).label_geometry.width;

	return;
} /* GetValuesHook */

/**
 ** ChangeManaged()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeManaged (
	Widget			w
)
#else
ChangeManaged (w)
	Widget			w;
#endif
{
	XtWidgetProc		change_managed =
				COMPOSITE_C(superClass).change_managed;

	Boolean			more;


	/*
	 * Complain if we have more than one managed child.
	 */
	(void)FetchChild (w, &more);	
	if (more)
		OlVaDisplayWarningMsg (
			XtDisplay(w),
			"tooManyChildren", "changeManaged",
			OleCOlToolkitWarning,
			"Widget %s: Only one child at a time may be managed",
			XtName(w)
		);

	if (change_managed)
		(*change_managed) (w);

	return;
} /* ChangeManaged */

/**
 ** Activate()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
Activate (
	Widget			w,
	OlVirtualName		type,		/*NOTUSED*/
	XtPointer		data		/*NOTUSED*/
)
#else
Activate (w, type, data)
	Widget			w;
	OlVirtualName		type;
	XtPointer		data;
#endif
{
	Widget			child = FetchChild(w, (Boolean *)0);	

	Time			time = CurrentTime;


	/*
	 * MORE: It is a bit bogus to do any focus moving here, what if
	 * we add some other activation reason in the future? E.g. what
	 * if we support accelerators? Should move focus in some other
	 * routine.
	 */
	if (!child)
		return (False);
	if (!XtCallAcceptFocus(child, &time))
		OlMoveFocus (w, OL_NEXTFIELD, CurrentTime);
	return (OlActivateWidget(child, OL_SELECTKEY, (XtPointer)0));
} /* Activate */

/**
 ** Layout()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Layout (
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,/*NOTUSED*/
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
Layout (w, resizable, query_only, cached_best_fit_ok_hint, who_asking, request, response)
	Widget			w;
	Boolean			resizable;
	Boolean			query_only;
	Boolean			cached_best_fit_ok_hint;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	XtWidgetGeometry	best_fit;
	XtWidgetGeometry	adjusted;
	XtWidgetGeometry	available;
	XtWidgetGeometry	label_geometry;	/*SET,NOTUSED*/
	XtWidgetGeometry	child_geometry;

	XRectangle		save_label_geometry;

	Widget			child;


#if	defined(CAPTION_DEBUG)
	if (caption_debug) {
		printf (
			"Layout: %s/%s/%x resizable %d query_only %d",
			XtName(w), CLASS(XtClass(w)), w,
			resizable, query_only
		);
		if (who_asking == w)
			printf (
				" XtQuery (%s%d,%s%d)\n",
				request->request_mode & CWWidth? "":"!",
				request->width,
				request->request_mode & CWHeight? "":"!",
				request->height
			);
		else if (who_asking)
			printf (
				" XtGeometry (%s%d,%s%d)\n",
				request->request_mode & CWWidth? "":"!",
				request->width,
				request->request_mode & CWHeight? "":"!",
				request->height
			);
		else
			printf ("\n");
	}
#endif

	child = FetchChild(w, (Boolean *)0);	/* may return null */

	/*
	 * Compute the geometry that exactly contains the full
	 * label and an unclipped child.
	 */
	ComputeGeometries (
		w, child,
		child && who_asking == child? request : (XtWidgetGeometry *)0,
		&label_geometry, &child_geometry,
		(XtWidgetGeometry *)0, &best_fit
	);
	best_fit.width += 2*MANAGER_P(w).shadow_thickness;
	best_fit.height += 2*MANAGER_P(w).shadow_thickness;
#if	defined(CAPTION_DEBUG)
	if (caption_debug) {
		printf (
			"ComputeGeometries #1: label(%d,%d;%d,%d) child(%d,%d;%d,%d) best(%d,%d)\n",
			label_geometry.x, label_geometry.y,
			label_geometry.width, label_geometry.height,
			child_geometry.x, child_geometry.y,
			child_geometry.width, child_geometry.height,
			best_fit.width, best_fit.height
		);
	}
#endif

	/*
	 * Make it easy on the poor slob who insists on creating
	 * us with no child and no label.
	 */
	if (!best_fit.width)
		best_fit.width = 1;
	if (!best_fit.height)
		best_fit.height = 1;

	/*
	 * Adjust the best_fit to account for the XtNlayoutWidth and
	 * XtNlayoutHeight "constraints".
	 */
	OlAdjustGeometry (
		w, &CAPTION_P(w).layout, &best_fit, &adjusted
	);

	/*
	 * If we are not forced or requested to fit a particular size,
	 * calculate the best-fit size and see if the parent allows it.
	 * Otherwise, use the requested or current size.
	 *
	 * Since OlAvailableGeometry may make a geometry request of the
	 * parent, and since the parent may ask for the alignment of the
	 * caption, we need to be prepared to answer with an alignment
	 * consistent with the geometry needs. But the current layout
	 * request may not be applied, so we need to save/restore the
	 * label geometry.
	 */
	save_label_geometry = CAPTION_P(w).label_geometry;
	GeometryToRect (&label_geometry, &CAPTION_P(w).label_geometry);
	CAPTION_P(w).label_geometry.x += MANAGER_P(w).shadow_thickness;
	CAPTION_P(w).label_geometry.y += MANAGER_P(w).shadow_thickness;
	OlAvailableGeometry (
		w, resizable, query_only, who_asking,
		request, &adjusted, &available
	);
	CAPTION_P(w).label_geometry = save_label_geometry;
#if	defined(CAPTION_DEBUG)
	if (caption_debug) {
		printf (
			"Layout: %s/%s/%x adjusted (%d,%d) available (%d,%d)\n",
			XtName(w), CLASS(XtClass(w)), w,
			adjusted.width, adjusted.height,
			available.width, available.height
		);
	}
#endif

	/*
	 * At this point, available contains the size available for
	 * managing the children. Note that it may not be the same
	 * as the (composite's) core geometry (a query situation).
	 *
	 * Compute the label and child geometries given the
	 * constraints of available. best_fit is changed to be the
	 * best fit under those constraints--it won't be larger than
	 * available but it might be smaller.
	 */
	if (
		available.width != best_fit.width
	     || available.height != best_fit.height
	) {
		ComputeGeometries (
			w, child,
			child && who_asking == child? request : (XtWidgetGeometry *)0,
			&label_geometry, &child_geometry,
			&available, &best_fit
		);
		best_fit.width += 2*MANAGER_P(w).shadow_thickness;
		best_fit.height += 2*MANAGER_P(w).shadow_thickness;
#if	defined(CAPTION_DEBUG)
		if (caption_debug) {
			printf (
				"ComputeGeometries #2: label(%d,%d;%d,%d) child(%d,%d;%d,%d) best(%d,%d)\n",
				label_geometry.x, label_geometry.y,
				label_geometry.width, label_geometry.height,
				child_geometry.x, child_geometry.y,
				child_geometry.width, child_geometry.height,
				best_fit.width, best_fit.height
			);
		}
#endif
	}

	/*
	 * If the Caption widget is just asking, return best_fit.
	 */
	if (who_asking == w) {
		if (response) {
			response->x            = CORE_P(w).x;
			response->y            = CORE_P(w).y;
			response->width        = best_fit.width;
			response->height       = best_fit.height;
			response->border_width = CORE_P(w).border_width;
		}
		return;
	}

	child_geometry.x += MANAGER_P(w).shadow_thickness;
	child_geometry.y += MANAGER_P(w).shadow_thickness;
	label_geometry.x += MANAGER_P(w).shadow_thickness;
	label_geometry.y += MANAGER_P(w).shadow_thickness;

	/*
	 * Configure the child or return the child's best geometry,
	 * as appropriate.
	 */
	if (child)
		OlConfigureChild (
			child,
			child_geometry.x, child_geometry.y,
			child_geometry.width, child_geometry.height,
			child_geometry.border_width,
			query_only, who_asking, response
		);

	/*
	 * If this isn't a query and the label geometry has changed,
	 * clear the areas for the old and new labels. Clearing the
	 * area for the new label should generate an exposure to get
	 * the label drawn.  If the widget is not realized, then we've
	 * been called through ChangeManaged, and the label_geometry field
	 * is initialized here.
	 */
#define SAME(F) (CAPTION_P(w).label_geometry.F == label_geometry.F)
	if (
		!query_only
	     && !(SAME(x) && SAME(y) && SAME(width) && SAME(height))
	) {
		if (XtIsRealized(w))
			ClearArea (w, &CAPTION_P(w).label_geometry, False);
		GeometryToRect (&label_geometry, &CAPTION_P(w).label_geometry);
		if (XtIsRealized(w))
			ClearArea (w, &CAPTION_P(w).label_geometry, True);
	}
#undef	SAME

	return;
} /* Layout */

/**
 ** QueryAlignment()
 **/

static XtGeometryResult
#if	OlNeedFunctionPrototypes
QueryAlignment (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
QueryAlignment (w, request, response)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	if (request->request_mode & CWX) {
		/*
		 * MORE: Support a width query (request_mode & CWWidth).
		 */
		switch (CAPTION_P(w).position) {
		case OL_TOP:
		case OL_BOTTOM:
		case OL_LEFT:
			response->x = CAPTION_P(w).label_geometry.x
				    + CAPTION_P(w).label_geometry.width
				    + CAPTION_P(w).space;
			break;
		case OL_RIGHT:
			response->x = CAPTION_P(w).label_geometry.x
				    - CAPTION_P(w).space;
			break;
		}
	}
	if (request->request_mode & CWY) {
		/*
		 * MORE: Support baseline alignment.
		 */
		response->y = 0;
	}
	return (XtGeometryYes);
} /* QueryAlignment */

/**
 ** ComputeGeometries()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputeGeometries (
	Widget			w,
	Widget			child,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	label_geometry,
	XtWidgetGeometry *	child_geometry,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate
)
#else
ComputeGeometries (w, child, request, label_geometry, child_geometry, available, aggregate)
	Widget			w;
	Widget			child;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	label_geometry;
	XtWidgetGeometry *	child_geometry;
	XtWidgetGeometry *	available;
	XtWidgetGeometry *	aggregate;
#endif
{
	Dimension		space    = CAPTION_P(w).space;
	Dimension		child_width;
	Dimension		child_height;

	OlDefine		position = CAPTION_P(w).position;

	OlgTextLbl		label;

	Cardinal		pass;

	XtWidgetGeometry	preferred;

#define SUBTRACT(A,B) if ((int)(A) > (int)(B)) A -= B; else A = 0


	label_geometry->width =
	label_geometry->height = 0;
	if (CAPTION_P(w).label) {
		SetTextLabel (w, &label);
		OlgSizeTextLabel (
			XtScreen(w), CAPTION_P(w).attrs, &label,
			&label_geometry->width, &label_geometry->height
		);
	}

	if (child)
		if (request) {
			child_geometry->width = request->width;
			child_geometry->height = request->height;
			child_geometry->border_width = request->border_width;
		} else {
			child_geometry->width = CORE_P(child).width;
			child_geometry->height = CORE_P(child).height;
			child_geometry->border_width = CORE_P(child).border_width;
		}
	else
		child_geometry->width =
		child_geometry->height =
		child_geometry->border_width = 0;

	/*
	 * Throughout most of the following calculations, child_geometry
	 * will include the border width.
	 */
	child_geometry->width += 2*child_geometry->border_width;
	child_geometry->height += 2*child_geometry->border_width;

	/*
	 * It may take two passes to get the best geometry for the child
	 * and the label. The first time, we will adjust the child's
	 * geometry to accomodate any mismatch between the current
	 * geometry and the available geometry. Then we'll query the
	 * child to see if it will accept this adjustment, and if it
	 * can't accept the entire change we'll take a second pass
	 * and adjust the label to make up the difference.
	 */

	pass = 1;
TryAgain:
	child_width = child_geometry->width;
	child_height = child_geometry->height;
	switch (position) {
	case OL_LEFT:
	case OL_RIGHT:
		ComputeAggregateAndAdjust (
			pass,
			XtOffsetOf(XtWidgetGeometry, width),
			XtOffsetOf(XtWidgetGeometry, height),
			space,
			label_geometry, child_geometry,
			aggregate, available
		);
		break;
	case OL_TOP:
	case OL_BOTTOM:
		ComputeAggregateAndAdjust (
			pass,
			XtOffsetOf(XtWidgetGeometry, height),
			XtOffsetOf(XtWidgetGeometry, width),
			space,
			label_geometry, child_geometry,
			aggregate, available
		);
		break;
	}

	if (
		child
	     && !request
	     && (child_width != child_geometry->width
	      || child_height != child_geometry->height)
	) {

		SUBTRACT (child_geometry->width, 2*child_geometry->border_width);
		SUBTRACT (child_geometry->height, 2*child_geometry->border_width);

		child_geometry->request_mode = CWWidth|CWHeight;
#if	defined(CAPTION_DEBUG)
		if (caption_debug) {
			printf (
				"Caption XtQuery: %s/%s/%x (%d,%d)\n",
				XtName(w), CLASS(XtClass(w)), w,
				child_geometry->width, child_geometry->height
			);
		}
#endif
		switch (XtQueryGeometry(child, child_geometry, &preferred)) {

		case XtGeometryYes:
#if	defined(CAPTION_DEBUG)
			if (caption_debug) printf ("Yes");
#endif
			/*
			 * The child accepts this resize, so we may be
			 * done. However, if we have a degenerate size,
			 * don't be fooled--force a minimum size and
			 * try the next pass.
			 */
			if (child_geometry->width && child_geometry->height)
				pass++;	/* skip second pass */
			else {
				if (!child_geometry->width)
					child_geometry->width = 1;
				if (!child_geometry->height)
					child_geometry->height = 1;
			}
			break;

		case XtGeometryAlmost:
#if	defined(CAPTION_DEBUG)
			if (caption_debug) printf ("Almost");
#endif
#define CHECK(BIT,F) \
			if ((preferred.request_mode & BIT))		\
				child_geometry->F = preferred.F

			CHECK (CWWidth, width);
			CHECK (CWHeight, height);
#undef	CHECK
			break;

		case XtGeometryNo:
#if	defined(CAPTION_DEBUG)
			if (caption_debug) printf ("No");
#endif
			child_geometry->width = CORE_P(child).width;
			child_geometry->height = CORE_P(child).height;
			break;
		}
#if	defined(CAPTION_DEBUG)
		if (caption_debug) {
			printf (
				" -> (%d,%d)\n",
				child_geometry->width, child_geometry->height
			);
		}
#endif

		child_geometry->width += 2*child_geometry->border_width;
		child_geometry->height += 2*child_geometry->border_width;

		if (pass++ == 1)
			goto TryAgain;
	}

	/*
	 * Now that we have the best size, compute the positions
	 * of the label and child.
	 */

	switch (position) {
	case OL_LEFT:
		label_geometry->x = 0;
		child_geometry->x = label_geometry->width + space;
		break;
	case OL_RIGHT:
		label_geometry->x = child_geometry->width + space;
		child_geometry->x = 0;
		break;
	case OL_TOP:
		label_geometry->y = 0;
		child_geometry->y = label_geometry->height + space;
		break;
	case OL_BOTTOM:
		label_geometry->y = child_geometry->height + space;
		child_geometry->y = 0;
		break;
	}

	switch (position) {
	case OL_LEFT:
	case OL_RIGHT:
		ComputePerpendicularPosition (
			CAPTION_P(w).alignment,
			XtOffsetOf(XtWidgetGeometry, y),
			XtOffsetOf(XtWidgetGeometry, height),
			label_geometry, child_geometry
		);
		break;
	case OL_TOP:
	case OL_BOTTOM:
		ComputePerpendicularPosition (
			CAPTION_P(w).alignment,
			XtOffsetOf(XtWidgetGeometry, x),
			XtOffsetOf(XtWidgetGeometry, width),
			label_geometry, child_geometry
		);
		break;
	}

	/*
	 * Now we can remove the border-width component from the size.
	 */
	SUBTRACT (child_geometry->width, 2*child_geometry->border_width);
	SUBTRACT (child_geometry->height, 2*child_geometry->border_width);

#undef	SUBTRACT
	return;
} /* ComputeGeometries */

/**
 ** ComputeAggregateAndAdjust()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputeAggregateAndAdjust (
	Cardinal		pass,
	Cardinal		para_dim_offset,  /* parallel */
	Cardinal		perp_dim_offset,  /* perpendicular */
	Dimension		space,
	XtWidgetGeometry *	label_geometry,
	XtWidgetGeometry *	child_geometry,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	available
)
#else
ComputeAggregateAndAdjust (pass, para_dim_offset, perp_dim_offset, space, label_geometry, child_geometry, aggregate, available)
	Cardinal		pass;
	Cardinal		para_dim_offset;
	Cardinal		perp_dim_offset;
	Dimension		space;
	XtWidgetGeometry *	label_geometry;
	XtWidgetGeometry *	child_geometry;
	XtWidgetGeometry *	aggregate;
	XtWidgetGeometry *	available;
#endif
{
#define PARA(P) *(Dimension *)((char *)(P) + para_dim_offset)
#define PERP(P) *(Dimension *)((char *)(P) + perp_dim_offset)

	/*
	 * In the direction parallel to the caption-child orientation,
	 * the aggregate dimension is the sum of the caption's and child's
	 * dimension and the space between them. If the aggregate differs
	 * from available, we take the delta from (or give it to) the
	 * child (pass 1) else the label (pass 2). Note, though, that
	 * certain conditions may exist:
	 *  - no child (child_geometry is zero)
	 *  - delta is too large for child
	 * These conditions require us to apply some or all of the delta
	 * to the label, regardless of the pass.
	 */
	PARA(aggregate) = PARA(label_geometry) + space + PARA(child_geometry);
	if (available && PARA(aggregate) != PARA(available)) {
		int		delta;	/* int to allow < 0 */

		/*
		 * Negative delta means extra space we can give to the
		 * child (or the label).
		 */
		delta = PARA(aggregate) - PARA(available);
		if (pass == 1) {
			if (delta < 0 || (int)PARA(child_geometry) >= delta) {
				PARA(child_geometry) -= delta;
				delta = 0;
			} else {
				delta -= PARA(child_geometry);
				PARA(child_geometry) = 0;
			}
		}
		if (delta < 0 || (int)PARA(label_geometry) >= delta)
			PARA(label_geometry) -= delta;
		else
			PARA(label_geometry) = 0;
		PARA(aggregate) = PARA(label_geometry) + space + PARA(child_geometry);
	}

	/*
	 * In the direction perpendicular to the caption-child orientation
	 * the aggregate dimension is the larger of the child or label.
	 * However, we attempt to make the child's perpendicular dimension
	 * equal available, and we prevent the label's dimension from
	 * exceeding available.
	 */
	if (available) {
		PERP(child_geometry) = PERP(available);
		if (PERP(label_geometry) > PERP(available))
			PERP(label_geometry) = PERP(available);
	}
	if (PERP(child_geometry) > PERP(label_geometry))
		PERP(aggregate) = PERP(child_geometry);
	else
		PERP(aggregate) = PERP(label_geometry);

#undef	PARA
#undef	PERP
	return;
} /* ComputeAggregateAndAdjust */

/**
 ** ComputePerpendicularPosition()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputePerpendicularPosition (
	OlDefine		alignment,
	Cardinal		pos,
	Cardinal		dim,
	XtWidgetGeometry *	label_geometry,
	XtWidgetGeometry *	child_geometry
)
#else
ComputePerpendicularPosition (alignment, pos, dim, label_geometry, child_geometry)
	OlDefine		alignment;
	Cardinal		pos;
	Cardinal		dim;
	XtWidgetGeometry *	label_geometry;
	XtWidgetGeometry *	child_geometry;
#endif
{
#define POS(P) *(Position *)((char *)(P) + pos)
#define DIM(P) *(Dimension *)((char *)(P) + dim)

	switch (alignment) {
	case OL_LEFT:
	case OL_TOP:
		POS(label_geometry) = POS(child_geometry) = 0;
		break;
	case OL_CENTER:
		if (DIM(child_geometry) > DIM(label_geometry)) {
			POS(child_geometry) = 0;
			POS(label_geometry)
				= (DIM(child_geometry) - DIM(label_geometry)) / u2;
		} else {
			POS(label_geometry) = 0;
			POS(child_geometry)
				= (DIM(label_geometry) - DIM(child_geometry)) / u2;
		}
		break;
	case OL_RIGHT:
	case OL_BOTTOM:
		if (DIM(child_geometry) > DIM(label_geometry)) {
			POS(child_geometry) = 0;
			POS(label_geometry)
				= DIM(child_geometry) - DIM(label_geometry);
		} else {
			POS(label_geometry) = 0;
			POS(child_geometry)
				= DIM(label_geometry) - DIM(child_geometry);
		}
		break;
	}

#undef	POS
#undef	DIM
	return;
} /* ComputePerpendicularPosition */

/**
 ** SetTextLabel()
 **/

static void
#if	OlNeedFunctionPrototypes
SetTextLabel (
	Widget			w,
	OlgTextLbl *		label
)
#else
SetTextLabel (w, label)
	Widget			w;
	OlgTextLbl *		label;
#endif
{
	label->label = CAPTION_P(w).label;
	label->normalGC = CAPTION_P(w).normal_gc;
	label->inverseGC = CAPTION_P(w).inverse_gc;
	label->font = CAPTION_P(w).font;
	label->accelerator = 0;
	label->mnemonic = CAPTION_P(w).mnemonic;
	label->justification = 0;
	if (!XtIsSensitive(w)) {
		label->flags = TL_INSENSITIVE;
		label->stippleColor = CORE_P(w).background_pixel;
	} else
		label->flags = 0;
#if	defined(I18N)
	label->font_list = CAPTION_P(w).font_list;
#endif

	return;
} /* SetTextLabel */

/**
 ** GeometryToRect()
 **/

static void
#if	OlNeedFunctionPrototypes
GeometryToRect (
	XtWidgetGeometry *	geometry,
	XRectangle *		rect
)
#else
GeometryToRect (geometry, rect)
	XtWidgetGeometry *	geometry;
	XRectangle *		rect;
#endif
{
	rect->x = geometry->x;
	rect->y = geometry->y;
	rect->width = geometry->width;
	rect->height = geometry->height;
	return;
} /* GeometryToRect */

/**
 ** FetchChild()
 **/

static Widget
#if	OlNeedFunctionPrototypes
FetchChild (
	Widget			w,
	Boolean *		more
)
#else
FetchChild (w, more)
	Widget			w;
	Boolean *		more;
#endif
{
	Widget			returned_child = 0;
	Widget			child;

	Cardinal		n;


	if (more)
		*more = False;
	FOR_EACH_MANAGED_CHILD (w, child, n)
		if (!returned_child)
			returned_child = child;
		else {
			if (more)
				*more = True;
			break;
		}

	return (returned_child);
} /* FetchChild */

/**
 ** CheckResources()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckResources (
	Widget			new,
	Widget			current
)
#else
CheckResources (new, current)
	Widget			new;
	Widget			current;
#endif
{
#define SET_OR_RESET(NEW,CURRENT,FIELD,DEFAULT) \
	if (CURRENT)							\
		CAPTION_P(NEW).FIELD = CAPTION_P(CURRENT).FIELD;	\
	else								\
		CAPTION_P(NEW).FIELD = DEFAULT

	switch (CAPTION_P(new).alignment) {
	case OL_LEFT:
	case OL_RIGHT:
	case OL_TOP:
	case OL_BOTTOM:
	case OL_CENTER:
		break;
	default:
		OlVaDisplayWarningMsg (
			XtDisplay(new),
			"illegalResource", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName(new), XtNalignment
		);
		SET_OR_RESET (new, current, alignment, OL_CENTER);
		break;
	}

	switch (CAPTION_P(new).position) {
	case OL_LEFT:
	case OL_RIGHT:
		switch (CAPTION_P(new).alignment) {
		case OL_TOP:
		case OL_CENTER:
		case OL_BOTTOM:
			break;
		default:
			goto Complain;
		}
		break;
	case OL_TOP:
	case OL_BOTTOM:
		switch (CAPTION_P(new).alignment) {
		case OL_LEFT:
		case OL_CENTER:
		case OL_RIGHT:
			break;
		default:
Complain:		OlVaDisplayWarningMsg (
				XtDisplay(new),
				"inconsistentResource", "set",
				OleCOlToolkitWarning,
				"Widget %s: XtN%s and XtN%s resources are inconsistent",
				XtName(new), XtNposition, XtNalignment
			);
			SET_OR_RESET (new, current, alignment, OL_CENTER);
			break;
		}
		break;
	default:
		OlVaDisplayWarningMsg (
			XtDisplay(new),
			"illegalResource", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName(new), XtNposition
		);
		SET_OR_RESET (new, current, position, OL_LEFT);
		/*
		 * We could go back and recheck the alignment now, but
		 * we take the hard-line position that if the client/user
		 * screws up the value of position, they have to suffer
		 * with a reset alignment. This makes sense from the
		 * point of view that these two resources go hand-in-hand.
		 */
		SET_OR_RESET (new, current, alignment, OL_CENTER);
		break;
	}

	return;
} /* CheckResources */

/**
 ** SetDefaultLabelResource()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
SetDefaultLabelResource (
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
)
#else
SetDefaultLabelResource (w, offset, value)
	Widget			w;
	int			offset;
	XrmValue *		value;
#endif
{
	/*
	 * Caption is not a gadget, so .name is cool.
	 */
	value->addr = (XtPointer)CORE_P(w).name;
	return;
} /* SetDefaultLabelResource */

/**
 ** GetGCs()
 **/

static void
#if	OlNeedFunctionPrototypes
GetGCs (
	Widget			w
)
#else
GetGCs (w)
	Widget			w;
#endif
{
	XGCValues		v;


	/*
	 * WARNING:
	 * Never call this routine if the GC is already in the widget
	 * structure. Call FreeGCs() first if a new GC is needed,
	 * so that the old GC is released.
	 */

	v.foreground = CAPTION_P(w).font_color;
	v.font       = CAPTION_P(w).font->fid;
	v.background = CORE_P(w).background_pixel;
	CAPTION_P(w).normal_gc = XtGetGC(
		w, GCForeground | GCFont | GCBackground, &v
	);

	v.foreground = CORE_P(w).background_pixel;
	v.background = CAPTION_P(w).font_color;
	CAPTION_P(w).inverse_gc = XtGetGC(
		w, GCForeground | GCFont | GCBackground, &v
	);

	CAPTION_P(w).attrs = OlgCreateAttrs(
		XtScreen(w),
		CAPTION_P(w).font_color,
		(OlgBG *)&(CORE_P(w).background_pixel),
		False,
		OL_DEFAULT_POINT_SIZE
	);

	return;
} /* GetGCs */

/**
 ** FreeGCs()
 **/

static void
#if	OlNeedFunctionPrototypes
FreeGCs (
	Widget		w
)
#else
FreeGCs (w)
	Widget		w;
#endif
{
	if (CAPTION_P(w).normal_gc) {
		XtReleaseGC (w, CAPTION_P(w).normal_gc);
		CAPTION_P(w).normal_gc = 0;
	}
	if (CAPTION_P(w).inverse_gc) {
		XtReleaseGC (w, CAPTION_P(w).inverse_gc);
		CAPTION_P(w).inverse_gc = 0;
	}
	if (CAPTION_P(w).attrs)
		OlgDestroyAttrs (CAPTION_P(w).attrs);
	return;
} /* FreeGCs */
