#ifndef	NOIDENT
#ident	"@(#)textfield:TextField.c	2.19"
#endif

#include "string.h"
#include "stdio.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/TextFieldP.h"
#include "Xol/textbuff.h"

#define ClassName TextField
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&textFieldClassRec)
#define superClass	((WidgetClass)&textEditClassRec)
#define className	"TextField"

/*
 * Convenient macros:
 */

#define TEXTEDIT_P(W) ((TextEditWidget)(W))->textedit

#define TE(W) ((TextEditWidget)(W))

#define STREQU(A,B) (strcmp((A),(B)) == 0)

#define MaybeCall(I,ARGS)	if (indirect.I) (*indirect.I)ARGS; else
#define TFInitialize(R,N,A,NM)	MaybeCall(initialize,(R,N,A,NM))
#define TFSetValues(C,R,N,A,NM) \
	(indirect.set_values? (*indirect.set_values)(C,R,N,A,NM) : False)
#define TFResetMargin(W)	MaybeCall(reset_margin,(W))
#define TFDrawButton(W,R,D)	MaybeCall(draw_button,(W,R,D))
#define TFOverrideDefaultTextEditResources(W,D) \
	MaybeCall(override_default_textedit_resources,(W,D))

#define RectInRegion(REGION,RECT) \
	XRectInRegion(REGION, RECT->x, RECT->y, RECT->width, RECT->height)

#define CanStep(W) (TEXTFIELD_C(XtClass(W)).step != (OlTextFieldStepProc)0)
#define CanScroll(W) TEXTFIELD_P(W).can_scroll

#define LeftArrowActive(W) (CanScroll(W) && TEXTEDIT_P(W).xOffset != 0)

#define RightArrowActive(W) \
	(CanScroll(W)							\
	 && ((int)(TEXTEDIT_P(W).maxX + TEXTEDIT_P(W).xOffset)		\
					> (int)PAGE_R_MARGIN(TE(w))))

#define PointInRectangle(X,Y,R) \
	(   (R)->x <= X && X < (R)->x + (Position)(R)->width		\
         && (R)->y <= Y && Y < (R)->y + (Position)(R)->height  )

/*
 * Private routines:
 */

static void		ClassInitialize OL_ARGS((
	void
));
static void		ClassPartInitialize OL_ARGS((
	WidgetClass		wc
));
static void		Initialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Destroy OL_ARGS((
	Widget			w
));
static void		Resize OL_ARGS((
	Widget			w
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		GetValuesHook OL_ARGS((
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
));
static XtGeometryResult	QueryGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
));
static Boolean		Activate OL_ARGS((
	Widget			w,
	OlVirtualName		type,
	XtPointer		data		/*NOTUSED*/
));
static void		KeyHandler OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static void		ButtonPressHandler OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static void		ButtonReleaseHandler OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static Boolean 		Scroll OL_ARGS((
	Widget			w,
	Arrow			arrow
));
static Boolean		Step OL_ARGS((
	Widget			w,
	OlSteppedReason		reason,
	Cardinal		count
));
static void		ArrowDown OL_ARGS((
	Widget			w,
	Arrow			arrow,
	unsigned long		interval
));
static void		ArrowUp OL_ARGS((
	Widget			w
));
static void		PollMouse OL_ARGS((
	XtPointer		client_data,
	XtIntervalId *		id		/*NOTUSED*/
));
static void		SetTimer OL_ARGS((
	Widget			w,
	unsigned long		interval,
	XtTimerCallbackProc	cb
));
static void		RemoveTimer OL_ARGS((
	Widget			w
));
static void 		ModifyVerificationCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
));
static void		MarginCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
));
static void		RefreshArrow OL_ARGS((
	Widget			w,
	Arrow			arrow
));
static void		RefreshArea OL_ARGS((
	Widget			w,
	XRectangle *		rect
));
static void		DrawArrow OL_ARGS((
	Widget			w,
	Arrow			arrow,
	XRectangle *		dirty,
	XRectangle *		rect,
	Boolean			depressed,
	Boolean			force
));
static Arrow		PointToArrow OL_ARGS((
	Widget			w,
	Position		x,
	Position		y
));
static void		FetchDimensions OL_ARGS((
	Widget			w,
	Dimensions *		d
));
static void		CheckSize OL_ARGS((
	Widget			w,
	Dimension *		width,
	Dimension *		height,
	Boolean			query
));
static Boolean		RectInRect OL_ARGS((
	XRectangle *		ra,
	XRectangle *		rb
));
static void		GetCrayons OL_ARGS((
	Widget			w
));
static void		FreeCrayons OL_ARGS((
	Widget			w
));
static void		OverrideDefaultTextEditResources OL_ARGS((
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
));

/*
 * Private data:
 */

static Indirect		indirect;	/* See ClassInitialize */

#if	defined(DEBUG)
static TextFieldWidget	tfw = 0;
#endif

/*
 * Resources:
 */

#if	!defined(COLORED_LIKE_TEXT)

# define BACKGROUND \
    {	/* SGI */							\
	XtNbackground, XtCBackground,					\
	XtRPixel, sizeof(Pixel), offset(core.background_pixel),		\
	XtRString, XtDefaultBackground					\
    }

static _OlDynResource	dynamic_resources[] = {
# define offset(F) XtOffsetOf(TextFieldRec, F)
# define DYNFLAG   offset(textfield.dynamics)

	/*
	 * Need to provide our own dynamic registration of XtNbackground
	 * to override TextEdit's class name (XtCTextBackground).
	 */
  {	BACKGROUND,
	DYNFLAG, _TEXTFIELD_B_DYNAMIC_BACKGROUND, NULL
  },

# undef	offset
# undef	DYNFLAG
};

#endif	/* !COLORED_LIKE_TEXT */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(TextFieldRec,F)

    /*
     * This resource is in Primitive, but we need to customize the
     * default.
     *
     * THIS RESOURCE MUST BE FIRST IN THE LIST.
     */
    {
	XtNshadowThickness, XtCShadowThickness,
	XtRDimension, sizeof(Dimension), offset(primitive.shadow_thickness),
	XtRImmediate, (XtPointer)0	/*  (see ClassInitialize) */
    },

    /*
     * These resources are in the TextEdit superclass, but we don't want
     * the client changing them. We also need to initialize them before
     * TextEdit's initialize class procedure is called. Thus, we redefine
     * the class of the resources, and create a dummy resource that causes
     * a CallProc to be invoked to initialize the values.
     *
     * WARNING: This isn't foolproof. A client can use XtSetValues to
     * change the value of one of these resources and will succeed in
     * messing things up (e.g. the current buffer content). We will be
     * able to restore the values of these resources, but we currently
     * do not undo side-effects.
     *
     * MORE: Discover all the side-effects and find a way to undo them.
     *
     * See OverrideDefaultTextEditResources for more details.
     */
    {	/* G */
	XtNlinesVisible, XtCReadOnly,
	XtRInt, sizeof(int), offset(textedit.linesVisible),
	XtRImmediate, (XtPointer)1
    },
    {	/* G */
	XtNtopMargin, XtCReadOnly,
	XtRDimension, sizeof(Dimension), offset(textedit.topMargin),
	XtRImmediate, (XtPointer)0
    },
    {	/* G */
	XtNbottomMargin, XtCReadOnly,
	XtRDimension, sizeof(Dimension), offset(textedit.bottomMargin),
	XtRImmediate, (XtPointer)0
    },
    {	/* G */
	XtNwrapMode, XtCReadOnly,
	XtROlWrapMode, sizeof(OlWrapMode), offset(textedit.wrapMode),
	XtRImmediate, (XtPointer)OL_WRAP_OFF
    },
    {	/* G */
	XtNsourceType, XtCReadOnly,
	XtROlSourceType, sizeof(OlSourceType), offset(textedit.sourceType),
	XtRImmediate, (XtPointer)OL_STRING_SOURCE
    },
    {	/* G */
	XtNinsertReturn, XtCReadOnly,
	XtRBoolean, sizeof(Boolean), offset(textedit.insertReturn),
	XtRImmediate, (XtPointer)False
    },
    {	/* SGI */
	/*
	 * This has to be before bOgUs, so that it is set before
	 * the CallProc is called.
	 */
	XtNmaximumSize, XtCMaximumSize,
	XtRCardinal, sizeof(Cardinal), offset(textfield.maximum_size),
	XtRImmediate, (XtPointer)0
    },
    {	/* BOGUS */
	"bOgUs", "BoGuS",
	XtRBoolean, sizeof(Boolean), offset(textfield.bit_bucket),
	XtRCallProc, (XtPointer)OverrideDefaultTextEditResources
    },

    /*
     * These resources are in the TextEdit widget, but we want to give
     * different defaults.
     */
    {	/* SGI */
	XtNinsertTab, XtCInsertTab,
	XtRBoolean, sizeof(Boolean), offset(textedit.insertTab),
	XtRImmediate, (XtPointer)False
    },
    {	/* SGI */
	XtNpreselect, XtCPreselect,
	XtRBoolean, sizeof(Boolean), offset(textedit.preselect),
	XtRImmediate, (XtPointer)True
    },    

#if	!defined(COLORED_LIKE_TEXT)
    /*
     * This resource is in Core, but we want a different resource class.
     */
    BACKGROUND,
#endif

    /*
     * New name for a TextEdit resource: The TextField uses XtNstring
     * instead of XtNsource. Unfortunately, it is not easy to allow
     * either name to be used. What we do here hides XtNsource, preventing
     * clients from using it. (Reason: Xt overrides superclass resources
     * based on the offset, not resource name.)
     *
     * Normal XtGetValues won't work for this resource, though, because
     * (1) TextEdit mangles the field, changing it into a TextBuffer*,
     * (2) the client gets a malloc'd copy, not a pointer to our copy.
     * See GetValuesHook for the solution.
     *
     * MORE: Find a way to allow XtNsource to be used, too.
     */
    {	/* SGI */
	XtNstring, XtCString,
	XtRString, sizeof(String), offset(textedit.source),
	XtRImmediate, (XtPointer)0
    },

    /*
     * New resources:
     */
    {	/* SGI */
	XtNinitialDelay, XtCInitialDelay,
	XtRInt, sizeof(int), offset(textfield.initial_delay),
	XtRImmediate, (XtPointer)500
    },
    {	/* SGI */
	XtNrepeatRate, XtCRepeatRate,
	XtRInt, sizeof(int), offset(textfield.repeat_rate),
	XtRImmediate, (XtPointer)100
    },
    {	/* SI */
	XtNverification, XtCCallback,
	XtRCallback, sizeof(XtCallbackProc), offset(textfield.verification),
	XtRCallback, (XtPointer)0
    },
    {	/* SGI */
	XtNcanIncrement, XtCCanStep,
	XtRBoolean, sizeof(Boolean), offset(textfield.can_increment),
	XtRImmediate, (XtPointer)True
    },
    {	/* SGI */
	XtNcanDecrement, XtCCanStep,
	XtRBoolean, sizeof(Boolean), offset(textfield.can_decrement),
	XtRImmediate, (XtPointer)True
    },
    {	/* SGI */
	XtNcanScroll, XtCCanScroll,
	XtRBoolean, sizeof(Boolean), offset(textfield.can_scroll),
	XtRImmediate, (XtPointer)True
    },

    /*
     * This resource also is in the TextEdit superclass, but we need to
     * change its default. This resource is mapped to the textedit
     * resource so its value is evaluated before all textfield resources.
     */
    {	/* GI */
	XtNcharsVisible, XtCCharsVisible,
	XtRCardinal, sizeof(Cardinal), offset(textedit.charsVisible),
	XtRImmediate, (XtPointer)XtUnspecifiedCardinal
    },

#undef	offset
};

/*
 * Translations and actions:
 */

OLconst static OlEventHandlerRec event_procs[] = {
	{ KeyPress,      KeyHandler           },
	{ ButtonPress,   ButtonPressHandler   },
	{ ButtonRelease, ButtonReleaseHandler },
};

/*
 * Class record structure:
 *
 *	(I)	XtInherit'd field
 *	(D)	Chained downward (super-class to sub-class)
 *	(U)	Chained upward (sub-class to super-class)
 */

TextFieldClassRec		textFieldClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(TextFieldRec),
/* class_initialize     */                       ClassInitialize,
/* class_part_init   (D)*/                       ClassPartInitialize,
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
/* resize            (I)*/                       Resize,
/* expose            (I)*/                       XtInheritExpose,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/                       GetValuesHook,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       QueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Primitive class:
	 */
	{
/* focus_on_select      */                       False, /* textEdit class
                                                  already set this to True */
/* highlight_handler (I)*/                       XtInheritHighlightHandler,
/* traversal_handler (I)*/                       XtInheritTraversalHandler,
/* register_focus       */                       XtInheritRegisterFocus,
/* activate          (I)*/                       Activate,
/* event_procs          */ (OlEventHandlerRec *) event_procs,
/* num_event_procs      */                       XtNumber(event_procs),
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
#if	!defined(COLORED_LIKE_TEXT)
/* dyn_data             */ { dynamic_resources, XtNumber(dynamic_resources) },
#else
/* dyn_data             */ { (_OlDynResource *)0, (Cardinal)0 },
#endif
/* transparent_proc  (I)*/                       XtInheritTransparentProc,
	},
	/*
	 * TextEdit class:
	 */
	{
#if	defined(I18N)
/* im                   */ (OlIm *)              0,
/* status_info          */                       0,
/* im_key_index         */ (int)                 0
#else
	NULL
#endif
	},
	/*
	 * TextField class:
	 */
	{
/* scroll            (I)*/ (OlTextFieldScrollProc)0, /* ClassInitialize */
/* step              (I)*/ (OlTextFieldStepProc) 0,
/* extension            */ (XtPointer)           0,
	}
};

WidgetClass		textFieldWidgetClass = thisClass;

/**
 ** OlTextFieldCopyString
 **/

Cardinal
#if	OlNeedFunctionPrototypes
OlTextFieldCopyString (
	Widget			w,
	String			string
)
#else
OlTextFieldCopyString (w, string)
	Widget			w;
	String			string;
#endif
{
	Cardinal		length = 0;
	String			p;

	p = GetTextBufferLocation(TEXTEDIT_P(w).textBuffer, 0, (TextLocation *)0);
	if (p) {
		length = strlen(p);
		memcpy (string, p, length + 1);
	}
	return (length);
} /* OlTextFieldCopyString */

/**
 ** OlTextFieldGetString
 **/

String
#if	OlNeedFunctionPrototypes
OlTextFieldGetString (
	Widget			w,
	Cardinal *		size
)
#else
OlTextFieldGetString (w, size)
	Widget			w;
	Cardinal *		size;
#endif
{
	String			p = 0;

	if (OlTextEditCopyBuffer(w, &p)) {
		if (size)
			*size = strlen(p);
	}
	return (p);
} /* OlTextFieldGetString */

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
	void			(*class_initialize) OL_ARGS((
		Indirect *		indirect
	));

	OLRESOLVESTART
	OLRESOLVEEND (TFClassInitialize, class_initialize)

	(*class_initialize) (&indirect);

	return;
} /* ClassInitialize */

/**
 ** ClassPartInitialize()
 **/

static void
#if	OlNeedFunctionPrototypes
ClassPartInitialize (
	WidgetClass		wc
)
#else
ClassPartInitialize (wc)
	WidgetClass		wc;
#endif
{
#define INHERIT(WC,F,INH) \
	if (TEXTFIELD_C(WC).F == (INH))					\
		TEXTFIELD_C(WC).F = TEXTFIELD_C(SUPER_C(WC)).F;		\
	else

	INHERIT (wc, scroll, XtInheritScrollProc);
	INHERIT (wc, step, XtInheritStepProc);
#undef	INHERIT

	return;
} /* ClassPartInitialize */

/**
 ** Initialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
)
#else
Initialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
#if	defined(DEBUG)
fprintf (stderr, "OlTextFieldFooString returned length 1 too large, any of our clients care?\n");
fprintf (stderr, "Left/right arrows are wrong shape\n");
fprintf (stderr, "TextEdit (and other places?) should use _OlBeepDisplay\n");
fprintf (stderr, "Scrollbar (and other?) dimensions are wrong?\n");
fprintf (stderr, "Correct to use strchr in ModifyVerificationCB?\n");
fprintf (stderr, "Motif highlight ring doesn't dynamically change colors\n");
#endif
	/*
	 * See OverrideDefaultTextEditResources to find some of the
	 * fields initialized, including the OlgAttrs field.
	 */

	OlCheckReadOnlyResources (new, (Widget)0, args, *num_args);

	/*
	 * The TextEdit superclass has made sure that the width and height
	 * are set--we make sure they are set large enough.
	 */
	CheckSize (new, &CORE_P(new).width, &CORE_P(new).height, False);
	TFResetMargin (new);

	TFInitialize (request, new, args, num_args);

	XtAddCallback (new, XtNmodifyVerification, ModifyVerificationCB, (XtPointer)0);
	XtAddCallback (new, XtNmargin, MarginCB, (XtPointer)0);

	TEXTFIELD_P(new).polling = ArrowNone;
	TEXTFIELD_P(new).timer = 0;
	TEXTFIELD_P(new).button_down = False;

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
	RemoveTimer (w);
	FreeCrayons (w);
	return;
} /* Destroy */

/**
 ** Resize()
 **/

static void
#if	OlNeedFunctionPrototypes
Resize (
	Widget			w
)
#else
Resize (w)
	Widget			w;
#endif
{
	XtWidgetProc		resize = CORE_C(superClass).resize;

	TFResetMargin (w);
	(*resize) (w);
	return;
} /* Resize */

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
	ArgList			args,
	Cardinal *		num_args
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
	Boolean			redisplay	= False;

#define DIFFERENT(F) \
	(((TextFieldWidget)new)->F != ((TextFieldWidget)current)->F)


	OlCheckReadOnlyResources (new, current, args, *num_args);

	if (DIFFERENT(core.width) || DIFFERENT(core.height)) {
		CheckSize (new, &CORE_P(new).width, &CORE_P(new).height, False);
		if (DIFFERENT(core.height))
			TFResetMargin (new);
	}

	if (DIFFERENT(textedit.rightMargin) && CanStep(new))
		TEXTEDIT_P(new).rightMargin = TEXTEDIT_P(current).rightMargin;

	if (
		DIFFERENT(core.background_pixel)
	     || DIFFERENT(primitive.foreground)
	) {
		FreeCrayons (new);
		GetCrayons (new);
		redisplay = True;
	}
		
	if (
		DIFFERENT(textfield.can_increment)
	     || DIFFERENT(textfield.can_decrement)
	) {
		if (DIFFERENT(textfield.can_increment))
			if (TEXTFIELD_P(new).polling == ArrowIncrement)
				ArrowUp (new);
			else
				RefreshArrow (new, ArrowIncrement);
		if (DIFFERENT(textfield.can_decrement))
			if (TEXTFIELD_P(new).polling == ArrowDecrement)
				ArrowUp (new);
			else
				RefreshArrow (new, ArrowDecrement);
	}

	if (TFSetValues(current, request, new, args, num_args))
		redisplay = True;

#undef	DIFFERENT
	return (redisplay);
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
	Cardinal		i;

	/*
	 * We don't want to use XtGetSubvalues for XtNstring, because we
	 * malloc the value and expect the client to free it. If we use
	 * XtGetSubvalues we won't know if the client is asking for it.
	 * The only other resource is XtNtextEditWidget, so now that we're
	 * doing a loop we may as well handle it here instead of through
	 * XtGetSubvalues.
	 *
	 * Since we've hidden the TextEdit's XtNsource resource with
	 * XtNstring, normal XtGetValues can't fetch it (XtSetValues
	 * can't set it, either). We can at least help out the XtGetValues
	 * case.
	 */
	for (i = 0; i < *num_args; i++)
		if (STREQU(args[i].name, XtNstring))
			*(String *)args[i].value
				= OlTextFieldGetString(w, (Cardinal *)0);
		else if (STREQU(args[i].name, XtNsource))
			*(TextBuffer **)args[i].value
				= (TextBuffer *)TEXTEDIT_P(w).source;
		else if (STREQU(args[i].name, XtNtextEditWidget))
			*(Widget *)args[i].value = w;

	return;
} /* GetValuesHook() */

/**
 ** QueryGeometry()
 **/

static XtGeometryResult
#if	OlNeedFunctionPrototypes
QueryGeometry (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
)
#else
QueryGeometry (w, request, preferred)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	preferred;
#endif
{
	XtGeometryResult	result	= XtGeometryNo;


	/*
	 * For our convenience, make all size fields in request valid.
	 */
	if (!(request->request_mode & CWWidth))
		request->width = CORE_P(w).width;
	if (!(request->request_mode & CWHeight))
		request->height = CORE_P(w).height;
	if (!(request->request_mode & CWBorderWidth))
		request->border_width = CORE_P(w).border_width;

	preferred->request_mode = CWWidth|CWHeight|CWBorderWidth;
	preferred->width = request->width;
	preferred->height = request->height;
	preferred->border_width = 0;
	CheckSize (w, &preferred->width, &preferred->height, True);

#define CHECK(BIT,F) \
	if (!(request->request_mode & BIT) || request->F != preferred->F)\
		result = XtGeometryAlmost;				 \
	else

	CHECK (CWWidth, width);
	CHECK (CWHeight, height);
	CHECK (CWBorderWidth, border_width);
#undef	CHECK

	/*
	 * If the best we can do is our current size, returning anything
	 * but XtGeometryNo would be a waste of time.
	 */
#define SAME(F)	(request->F == CORE_P(w).F)
	if (
		result == XtGeometryAlmost
	     && SAME(width) && SAME(height) && SAME(border_width)
	)
		result = XtGeometryNo;
#undef	SAME

	return (result);
} /* QueryGeometry */

/**
 ** Activate()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
Activate (
	Widget			w,
	OlVirtualName		type,
	XtPointer		data		/*NOTUSED*/
)
#else
Activate (w, type, data)
	Widget			w;
	OlVirtualName		type;
	XtPointer		data;
#endif
{
	Boolean			consumed = False;

	TextBuffer *		tb;
	OlTextFieldVerify	verify;


	/*
	 * Just a little bit of code reuse here: When a valid keyboard
	 * equivalent is pressed we want to give visual feedback by
	 * showing the appropriate arrow button pressed. This feedback
	 * is identical (except for the arrow) for all keys, so we do it
	 * here. We also take this opportunity to set the consumed flag.
	 */
	switch (type) {
	case OL_PAGEUP:
	case OL_MULTIUP:
	case OL_SCROLLUP:
	case OL_MOVEUP:
	case OL_ROWUP:
	case OL_SCROLLTOP:
		if (!CanStep(w))
			goto Return;
		if (!TEXTFIELD_P(w).can_increment) {
			_OlBeepDisplay (w, 1);
			goto Return;
		}
		ArrowDown (w, ArrowIncrement, KEY_LAG);
		consumed = True;
		break;
	case OL_PAGEDOWN:
	case OL_MULTIDOWN:
	case OL_SCROLLDOWN:
	case OL_MOVEDOWN:
	case OL_ROWDOWN:
	case OL_SCROLLBOTTOM:
		if (!CanStep(w))
			goto Return;
		if (!TEXTFIELD_P(w).can_decrement) {
			_OlBeepDisplay (w, 1);
			goto Return;
		}
		ArrowDown (w, ArrowDecrement, KEY_LAG);
		consumed = True;
		break;
	}

	/*
	 * Now we do the action:
	 */
	switch (type) {

	case OL_PAGEDOWN:
	case OL_MULTIDOWN:
		(void)Step (w, OlSteppedDecrement, _OlGetMultiObjectCount(w));
		break;
	case OL_PAGEUP:
	case OL_MULTIUP:
		(void)Step (w, OlSteppedIncrement, _OlGetMultiObjectCount(w));
		break;
	case OL_SCROLLDOWN:
	case OL_MOVEDOWN:
	case OL_ROWDOWN:
		(void)Step (w, OlSteppedDecrement, 1);
		break;
	case OL_SCROLLUP:
	case OL_MOVEUP:
	case OL_ROWUP:
		(void)Step (w, OlSteppedIncrement, 1);
		break;
	case OL_SCROLLTOP:
		(void)Step (w, OlSteppedToMaximum, 0);
		break;
	case OL_SCROLLBOTTOM:
		(void)Step (w, OlSteppedToMinimum, 0);
		break;

	case OL_NEXT_FIELD:
		verify.reason = OlTextFieldNext;
		goto CallVerify;
	case OL_PREV_FIELD:
		verify.reason = OlTextFieldPrevious;
		goto CallVerify;
	case OL_RETURN:
		verify.reason = OlTextFieldReturn;
CallVerify:	tb = TEXTEDIT_P(w).textBuffer;
		verify.string = GetTextBufferLocation(tb, 0, NULL);
		verify.ok = True;
		XtCallCallbacks (w, XtNverification, &verify);
		/*
		 * We normally don't want to "consume" the event because
		 * we want OlAction to use it to traverse to the next
		 * widget. However, if the client refused the event, it
		 * means the client found an error in the field that the
		 * user needs to fix, so no traversal should occur and we
		 * should "consume" the event.
		 */
		consumed = !verify.ok;
		break;
	}

	/*
	 * If we didn't use this key, pass it to the TextEdut superclass
	 * for its possible consumption (i.e. envelope the superclass'
	 * activate method).
	 */
Return:	if (!consumed) {
		OlActivateFunc activate = PRIMITIVE_C(superClass).activate;
		if (activate)
			consumed = (*activate)(w, type, data);
	}
	return (consumed);
} /* Activate */

/**
 ** KeyHandler()
 **/

static void
#if	OlNeedFunctionPrototypes
KeyHandler (
	Widget			w,
	OlVirtualEvent		ve
)
#else
KeyHandler (w, ve)
	Widget			w;
	OlVirtualEvent		ve;
#endif
{
	/*
	 * The TextEdit superclass has an event_proc for KeyPress, and it
	 * will snarf some keys we want for stepping up/down. So we give
	 * our own event_proc to grab these keys first. Even so, our
	 * activate method does all the work.
	 */
	ve->consumed = OlActivateWidget(w, ve->virtual_name, (XtPointer)0);
	return;
} /* KeyHandler */

/**
 ** ButtonPressHandler()
 **/

static void
#if	OlNeedFunctionPrototypes
ButtonPressHandler (
	Widget			w,
	OlVirtualEvent		ve
)
#else
ButtonPressHandler (w, ve)
	Widget			w;
	OlVirtualEvent		ve;
#endif
{
	XButtonEvent *		xbutton	= (XButtonEvent *)ve->xevent;

	Arrow			arrow;

	Boolean			depress = False;
	Boolean			step = False;


	ve->consumed = False;

	arrow = PointToArrow(w, xbutton->x, xbutton->y);
	switch (arrow) {
	case ArrowLeft:
	case ArrowRight:
		if (!CanScroll(w)) /* will be so if not OPEN LOOK */
			return;
		break;
	case ArrowIncrement:
	case ArrowDecrement:
		if (!CanStep(w))
			return;
		break;
	default:
		return;
	}

	switch (ve->virtual_name) {
	case OL_SELECT:
	case OL_ADJUST:
		switch (arrow) {
		case ArrowLeft:
		case ArrowRight:
			/*
			 * On scrolling to the farthest edge, the button
			 * will disappear--no need to show it depressed!
			 */
			if (Scroll(w, arrow))
				depress = True;
			ve->consumed = True;
			break;
		case ArrowIncrement:
			if (!TEXTFIELD_P(w).can_increment)
				_OlBeepDisplay (w, 1);
			else
				step = True;
			ve->consumed = True;
			break;
		case ArrowDecrement:
			if (!TEXTFIELD_P(w).can_decrement)
				_OlBeepDisplay (w, 1);
			else
				step = True;
			ve->consumed = True;
			break;
		}

		if (step) {
			OlSteppedReason		reason;

			reason = arrow == ArrowIncrement?
				OlSteppedIncrement : OlSteppedDecrement;

			/*
			 * Even though the user may have reached the
			 * limit, show the button depressed anyway to
			 * give positive feedback.
			 */
			(void)Step (w, reason, 1);
			depress = True;
		}
		if (depress) {
			ArrowDown (w, arrow, TEXTFIELD_P(w).initial_delay);
			TEXTFIELD_P(w).button_down = True;
		}
		break;
	}

	return;
} /* ButtonPressHandler */

/**
 ** ButtonReleaseHandler()
 **/

static void
#if	OlNeedFunctionPrototypes
ButtonReleaseHandler (
	Widget			w,
	OlVirtualEvent		ve
)
#else
ButtonReleaseHandler (w, ve)
	Widget			w;
	OlVirtualEvent		ve;
#endif
{
	ve->consumed = False;

	switch (ve->virtual_name) {
	case OL_SELECT:
	case OL_ADJUST:
		if (TEXTFIELD_P(w).button_down) {
			ArrowUp (w);
			TEXTFIELD_P(w).button_down = False;
			ve->consumed = True;
		}
		break;
	}

	return;
} /* ButtonReleaseHandler */

/**
 ** Scroll()
 **/

static Boolean 
#if	OlNeedFunctionPrototypes
Scroll (
	Widget			w,
	Arrow			arrow
)
#else
Scroll (w, arrow)
	Widget			w;
	Arrow			arrow;
#endif
{
	OlTextFieldScrollProc	scroll = TEXTFIELD_C(XtClass(w)).scroll;
	return (scroll? (*scroll)(w, arrow) : False);
} /* Scroll */

/**
 ** Step()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
Step (
	Widget			w,
	OlSteppedReason		reason,
	Cardinal		count
)
#else
Step (w, reason, count)
	Widget			w;
	OlSteppedReason		reason;
	Cardinal		count;
#endif
{
	Boolean continue_to_step
		= (*TEXTFIELD_C(XtClass(w)).step)(w, reason, count);
	if (continue_to_step)
		switch (reason) {
		case OlSteppedIncrement:
		case OlSteppedToMaximum:
			continue_to_step = TEXTFIELD_P(w).can_increment;
			break;
		case OlSteppedDecrement:
		case OlSteppedToMinimum:
			continue_to_step = TEXTFIELD_P(w).can_decrement;
			break;
		}
	return (continue_to_step);
} /* Step */

/**
 ** ArrowDown()
 **/

static void
#if	OlNeedFunctionPrototypes
ArrowDown (
	Widget			w,
	Arrow			arrow,
	unsigned long		interval
)
#else
ArrowDown (w, arrow, interval)
	Widget			w;
	Arrow			arrow;
	unsigned long		interval;
#endif
{
	TEXTFIELD_P(w).polling = arrow;
	SetTimer (w, interval, PollMouse);
	RefreshArrow (w, arrow);
	return;
} /* ArrowDown */

/**
 ** ArrowUp()
 **/

static void
#if	OlNeedFunctionPrototypes
ArrowUp (
	Widget			w
)
#else
ArrowUp (w)
	Widget			w;
#endif
{
	Arrow			arrow = TEXTFIELD_P(w).polling;
	TEXTFIELD_P(w).polling = ArrowNone;
	RemoveTimer (w);
	RefreshArrow (w, arrow);
	return;
} /* ArrowUp */

/**
 ** PollMouse()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
PollMouse (
	XtPointer		client_data,
	XtIntervalId *		id		/*NOTUSED*/
)
#else
PollMouse (client_data, id)
	XtPointer		client_data;
	XtIntervalId *		id;
#endif
{
	Widget			w = (Widget)client_data;

	Boolean			continue_to_poll = False;


	/*
	 * The timer is automatically removed when it fires, so we need
	 * to clear our record of it here.
	 */
	TEXTFIELD_P(w).timer = 0;

	if (TEXTFIELD_P(w).button_down) {
		switch (TEXTFIELD_P(w).polling) {
		case ArrowLeft:
		case ArrowRight:
			/*
			 * Won't be here if not OPEN LOOK.
			 */
			continue_to_poll = Scroll(w, TEXTFIELD_P(w).polling);
			break;
		case ArrowIncrement:
			continue_to_poll = Step(w, OlSteppedIncrement, 1);
			break;
		case ArrowDecrement:
			continue_to_poll = Step(w, OlSteppedDecrement, 1);
			break;
		}
	}
	if (continue_to_poll)
		SetTimer (w, TEXTFIELD_P(w).repeat_rate, PollMouse);
	else
		ArrowUp (w);

	return;
} /* PollMouse */

/**
 ** SetTimer()
 **/

static void
#if	OlNeedFunctionPrototypes
SetTimer (
	Widget			w,
	unsigned long		interval,
	XtTimerCallbackProc	cb
)
#else
SetTimer (w, interval, cb)
	Widget			w;
	unsigned long		interval;
	XtTimerCallbackProc	cb;
#endif
{
	/*
	 * The timer can already be set if the user recently hit
	 * a keyboard equivalent and now hits another or presses
	 * the SELECT button.
	 */
	if (TEXTFIELD_P(w).timer)
		XtRemoveTimeOut (TEXTFIELD_P(w).timer);
	TEXTFIELD_P(w).timer = OlAddTimeOut(w, interval, cb, (XtPointer)w);
	return;
} /* SetTimer */

/**
 ** RemoveTimer()
 **/

static void
#if	OlNeedFunctionPrototypes
RemoveTimer (
	Widget			w
)
#else
RemoveTimer (w)
	Widget			w;
#endif
{
	if (TEXTFIELD_P(w).timer) {
		XtRemoveTimeOut (TEXTFIELD_P(w).timer);
		TEXTFIELD_P(w).timer = 0;
	}
	return;
} /* RemoveTimer */

/**
 ** ModifyVerificationCB
 **/

/*ARGSUSED*/
static void 
#if	OlNeedFunctionPrototypes
ModifyVerificationCB (
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
)
#else
ModifyVerificationCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlTextModifyCallData *	cd = (OlTextModifyCallData *)call_data;


	if (strchr(cd->text, '\n'))
		cd->ok = False;
	else if (TEXTFIELD_P(w).maximum_size) {
		Cardinal		potential_length;

		potential_length
		   = LengthOfTextBufferLine(TEXTEDIT_P(w).textBuffer, 0)-1
		   - (cd->select_end - cd->select_start)
		   + strlen(cd->text);
		if (potential_length > TEXTFIELD_P(w).maximum_size)
			cd->ok = False;
	}
	if (!cd->ok)
		_OlBeepDisplay (w, 1);

	return;
} /* ModifyVerificationCB */

/**
 ** MarginCB()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
MarginCB (
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
)
#else
MarginCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlTextMarginCallData *	cd = (OlTextMarginCallData *)call_data;

	/*
	 * The .hint field doesn't matter, e.g. OL_MARGIN_EXPOSED
	 * can be set for the obvious reason (expose event), and
	 * OL_MARGIN_CALCULATED can be set when the user inserts
	 * additional text.
	 */
	RefreshArea (w, cd->rect);

	return;
} /* MarginCB */

/**
 ** RefreshArrow()
 **/

static void
#if	OlNeedFunctionPrototypes
RefreshArrow (
	Widget			w,
	Arrow			arrow
)
#else
RefreshArrow (w, arrow)
	Widget			w;
	Arrow			arrow;
#endif
{
	Rectangles		r;

	_OlTFComputeRectangles (w, &r);
	if (OlGetGui() == OL_OPENLOOK_GUI) {
		switch (arrow) {
		case ArrowLeft:
			RefreshArea (w, &r.left);
			break;
		case ArrowRight:
			RefreshArea (w, &r.right);
			break;
		}
	}
	switch (arrow) {
	case ArrowIncrement:
		RefreshArea (w, &r.increment);
		break;
	case ArrowDecrement:
		RefreshArea (w, &r.decrement);
		break;
	}
	return;
} /* RefreshArrow */

/**
 ** RefreshArea()
 **/

static void
#if	OlNeedFunctionPrototypes
RefreshArea (
	Widget			w,
	XRectangle *		rect
)
#else
RefreshArea (w, rect)
	Widget			w;
	XRectangle *		rect;
#endif
{
	Display *		display	= XtDisplay(w);

	Screen *		screen	= XtScreen(w);

	Window			window	= XtWindow(w);

	Rectangles		r;


	_OlTFComputeRectangles (w, &r);

	if (OlGetGui() == OL_OPENLOOK_GUI) {
		/*
		 * Draw the text underline. Make sure we don't have a
		 * degenerate case (no room for the underline in a
		 * too-narrow widget). Since some area-clearing is
		 * involved, drawing the line first ensures that parts of
		 * the buttons aren't cleared.
		 */
		if (RectInRect(rect, &r.line)) {
			Dimensions	d;
			FetchDimensions (w, &d);
			XClearArea (
				display, window,
				r.line.x - d.gap, r.line.y, d.gap, r.line.height,
				False
			);
			XClearArea (
				display, window,
				r.line.x + r.line.width, r.line.y, d.gap, r.line.height,
				False
			);
			if (r.line.width)
				OlgDrawLine (
					screen, window, TEXTFIELD_P(w).attrs,
					r.line.x, r.line.y, r.line.width, r.line.height,
					False
				);
		}

		/*
		 * Draw the scroll left/right buttons, as needed.
		 */
		if (LeftArrowActive(w))
			DrawArrow (w, ArrowLeft, rect, &r.left, False, False);

		if (RightArrowActive(w))
			DrawArrow (w, ArrowRight, rect, &r.right, False, False);
	}

	/*
	 * Now draw the step up/down buttons, if needed.
	 */
	if (CanStep(w)) {
		/*
		 * Draw the right button after drawing the left button,
		 * so that in 2D the left button's shadow is covered by
		 * the right button. Note that, in 2D when just the left
		 * button is exposed, we have to draw the right button
		 * anyway so it overlaps the newly drawn left button.
		 */
		DrawArrow (
			w, ArrowIncrement, rect, &r.increment,
			!TEXTFIELD_P(w).can_increment,
			False
		);
		DrawArrow (
			w, ArrowDecrement, rect, &r.decrement,
			!TEXTFIELD_P(w).can_decrement,
			RectInRect(rect, &r.increment)
		);
	}

	return;
} /* RefreshArea */

/**
 ** DrawArrow()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawArrow (
	Widget			w,
	Arrow			arrow,
	XRectangle *		dirty,
	XRectangle *		rect,
	Boolean			dim,
	Boolean			force
)
#else
DrawArrow (w, arrow, dirty, rect, dim, force)
	Widget			w;
	Arrow			arrow;
	XRectangle *		dirty;
	XRectangle *		rect;
	Boolean			dim;
	Boolean			force;
#endif
{
	Display *		display	= XtDisplay(w);

	Screen *		screen	= XtScreen(w);

	Window			window	= XtWindow(w);

	Boolean			stipple_added = False;

	GC			gc;

	_OlgDevice *		pDev = TEXTFIELD_P(w).attrs->pDev;
	_OlgDesc *		_arrow;


	if (!RectInRect(dirty, rect) && !force)
		return;

	/*
	 * Clear the area under the left/right buttons--for the full
	 * height of the TextField widget--to clear any text that used
	 * to be there. Also, save a pointer to the arrow object to be
	 * drawn.
	 */
	if (OlGetGui() == OL_OPENLOOK_GUI) {
		switch (arrow) {
		case ArrowLeft:
			_arrow = &pDev->arrowLeft;
			goto Clear;
		case ArrowRight:
			_arrow = &pDev->arrowRight;
Clear:			XClearArea (
				display, window,
				rect->x, 0, rect->width, CORE_P(w).height,
				False
			);
			break;
		}
	}
	switch (arrow) {
	case ArrowIncrement:
		_arrow = &pDev->arrowUp;
		break;
	case ArrowDecrement:
		_arrow = &pDev->arrowDown;
		break;
	}

	TFDrawButton (w, rect, TEXTFIELD_P(w).polling == arrow);

	if (OlgIs3d())
		gc = OlgGetBg3GC(TEXTFIELD_P(w).attrs);
	else {
            if (TEXTFIELD_P(w).polling == arrow)  /* if depressed */
                gc = OlgGetBg1GC(TEXTFIELD_P(w).attrs);
            else
		gc = OlgGetFgGC(TEXTFIELD_P(w).attrs);
	}
	if (dim)
		if (gc != pDev->blackGC) {
			XSetStipple (display, gc, pDev->inactiveStipple);
			XSetFillStyle (display, gc, FillStippled);
			stipple_added = True;
		} else
			gc = pDev->dimGrayGC;

	OlgDrawObject (
		screen, window, gc, _arrow,
		rect->x + rect->width/2, rect->y + rect->height/2
	);

	if (stipple_added)
		XSetFillStyle (display, gc, FillSolid);

	return;
} /* DrawArrow */

/**
 ** _OlTFComputeRectangles()
 **/

void
#if	OlNeedFunctionPrototypes
_OlTFComputeRectangles (
	Widget			w,
	Rectangles *		r
)
#else
_OlTFComputeRectangles (w, r)
	Widget			w;
	Rectangles *		r;
#endif
{
	Dimensions		d;

	Position		baseline;
	Position		rightedge;

	XRectangle		proto;

	int			line_width;


	FetchDimensions (w, &d);

#if	defined(NORTH_GRAVITY)
	baseline = TEXTEDIT_P(w).topMargin
		 + TEXTEDIT_P(w).lineHeight
		 + d.line_thickness; /* will be 0 if not OPEN LOOK */
#else
	baseline = CORE_P(w).height - PRIMITIVE_P(w).shadow_thickness;
#endif
	rightedge = CORE_P(w).width - PRIMITIVE_P(w).shadow_thickness;

	proto.x = 0;
	proto.y = baseline - d.arrow_height;
	proto.width = d.arrow_width;
	proto.height = d.arrow_height;

	if (OlGetGui() == OL_OPENLOOK_GUI) {
		/*
		 * Position the scroll buttons to the extreme edges
		 * (ignore the left margin, in particular).
		 */
		r->left = proto;
		r->right = r->left;
		r->right.x = rightedge - d.arrow_width;

		/*
		 * See also OverrideDefaultTextEditResources.
		 */
		if (CanStep(w))
			r->right.x -= TEXTEDIT_P(w).rightMargin;
	}

	r->increment = proto;
	r->increment.x = rightedge - 2*d.arrow_width;
	if (!OlgIs3d())
		/*
		 * In 2D we have to cover the left button's shadow
		 * by overlapping the right button.
		 */
		r->increment.x += 2*OlgGetHorizontalStroke(TEXTFIELD_P(w).attrs);

	r->decrement = proto;
	r->decrement.x = rightedge - d.arrow_width;

	if (OlGetGui() == OL_OPENLOOK_GUI) {
		/*
		 * Avoid arithmetic problems with fields that aren't wide
		 * enough: Use an int type to calculate the line width,
		 * then bound it by 0.
		 */
		if (LeftArrowActive(w))
			r->line.x = d.gap + d.arrow_width;
		else
			r->line.x = TEXTEDIT_P(w).leftMargin;
		line_width = (int)rightedge - (int)r->line.x;

		if (RightArrowActive(w))
			line_width -= (int)(d.gap + d.arrow_width);
		if (CanStep(w))
			line_width -= (int)TEXTEDIT_P(w).rightMargin;
		r->line.width  = line_width > 0? line_width : 0;
		r->line.y = baseline - d.line_thickness;
		r->line.height = d.line_thickness;
	}

	return;
} /* _OlTFComputeRectangles */

/**
 ** PointToArrow()
 **/

static Arrow
#if	OlNeedFunctionPrototypes
PointToArrow (
	Widget			w,
	Position		x,
	Position		y
)
#else
PointToArrow (w, x, y)
	Widget			w;
	Position		x;
	Position		y;
#endif
{
	Rectangles		r;


	_OlTFComputeRectangles (w, &r);
	if (OlGetGui() == OL_OPENLOOK_GUI) {
		if (LeftArrowActive(w) && PointInRectangle(x, y, &r.left))
			return (ArrowLeft);
		if (RightArrowActive(w) && PointInRectangle(x, y, &r.right))
			return (ArrowRight);
	}
	if (CanStep(w) && PointInRectangle(x, y, &r.increment))
		return (ArrowIncrement);
	if (CanStep(w) && PointInRectangle(x, y, &r.decrement))
		return (ArrowDecrement);
	return (ArrowNone);
} /* PointToArrow */

/**
 ** FetchDimensions()
 **/

static void
#if	OlNeedFunctionPrototypes
FetchDimensions (
	Widget			w,
	Dimensions *		d
)
#else
FetchDimensions (w, d)
	Widget			w;
	Dimensions *		d;
#endif
{
	Screen *		screen	= XtScreen(w);

	static Screen *		last_screen = 0;

	static Dimensions	D;


	if (last_screen != screen) {
		/*
		 * MORE: Allow scales other than 12pt.
		 *
		 * NOTE: The TextField (old?) code uses the
		 * OlgSizeAbbrevMenuB() routine to figure the width
		 * and height. However, the Spec shows different sizes
		 * for the text scrolling/incrementing/decrementing
		 * buttons and the abbreviated buttons.
		 *
		 * MORE: The code in this widget is still drawing the
		 * increment/decrement buttons as separate buttons, not
		 * as a pair of joined buttons as in the Spec.
		 *
		 * NOTE: The Spec specifies a ``reference baseline''
		 * for the numeric text field (but not the regular text
		 * field). We punt on that, to avoid having the descenders
		 * overlapping the underline. Instead, we have the text
		 * descend to touch the line but no more.
		 */
#define DecipointsToPixels(DIR,DPTS) \
			OlScreenPointToPixel(DIR, DPTS, screen) / 10
#define HorizontalPixels(DPTS) DecipointsToPixels(OL_HORIZONTAL, DPTS)
#define VerticalPixels(DPTS) DecipointsToPixels(OL_VERTICAL, DPTS)

#if	defined(ACCORDING_TO_SPEC)
		if (OlgIs3d()) {
			D.arrow_width  = HorizontalPixels(140);
			D.arrow_height = VerticalPixels(140);
		} else {
			D.arrow_width  = HorizontalPixels(150);
			D.arrow_height = VerticalPixels(150);
		}
#else	/* SAME_AS_SCROLLBAR */
		if (OlgIs3d()) {
			D.arrow_width  = HorizontalPixels(160);
			D.arrow_height = VerticalPixels(160);
		} else {
			D.arrow_width  = HorizontalPixels(170);
			D.arrow_height = VerticalPixels(170);
		}
#endif
		D.gap = HorizontalPixels(20);

		if (OlGetGui() == OL_OPENLOOK_GUI) {
			D.line_thickness
				= OlgGetVerticalStroke(TEXTFIELD_P(w).attrs);
			if (OlgIs3d())
				D.line_thickness *= 2;
		} else
			D.line_thickness = 0;

		last_screen = screen;
#undef	DecipointsToPixels
#undef	HorizontalPixels
#undef	VerticalPixels
	}

	*d = D;

	return;
} /* FetchDimensions */

/**
 ** CheckSize()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckSize (
	Widget			w,
	Dimension *		width,
	Dimension *		height,
	Boolean			query
)
#else
CheckSize (w, width, height, query)
	Widget			w;
	Dimension *		width;
	Dimension *		height;
	Boolean			query;
#endif
{
	Dimension		base;
	Dimension		min;
	Dimensions		d;


	/*
	 * The minimum width is enough for the left and right margins
	 * plus a single character (remember that the right margin
	 * includes space for the step buttons, if any). The preferred
	 * width is anything at least this large.
	 */
	min = TEXTEDIT_P(w).leftMargin
	    + TEXTEDIT_P(w).charWidth
	    + TEXTEDIT_P(w).rightMargin;
	if ((int)*width < (int)min)
		*width = min;

	/*
	 * The minimum height is enough for the text and the underline
	 * or border shadows, or the left/right/up/down buttons (if any).
	 * The preferred height is exactly that much. If we have more
	 * height than we need, pad at the top (bottom if NORTH_GRAVITY).
	 *
	 * Note: We use bottomMargin instead of d.line_thickness here,
	 * so we're GUI independent.
	 */
	FetchDimensions (w, &d);
	min =
	base = TEXTEDIT_P(w).topMargin
	     + TEXTEDIT_P(w).lineHeight
	     + TEXTEDIT_P(w).bottomMargin;
	if (CanScroll(w) || CanStep(w)) {
		Dimension	min_arrow;
		min_arrow = d.arrow_height + 2*PRIMITIVE_P(w).shadow_thickness;
		if (min < min_arrow)
			min = min_arrow;
	}
	if (query || *height < min)
		*height = min;
	if (!query) {
#if	defined(NORTH_GRAVITY)
		TEXTEDIT_P(w).bottomMargin = *height - base;
#else
		TEXTEDIT_P(w).topMargin = *height - base;
#endif
	}

	return;
} /* CheckSize */

/**
 ** RectInRect()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
RectInRect (
	XRectangle *		ra,
	XRectangle *		rb
)
#else
RectInRect (ra, rb)
	XRectangle *		ra;
	XRectangle *		rb;
#endif
{
	static Region		scratch = 0;
	static Region		null;


	if (!scratch) {
		scratch = XCreateRegion();
		null = XCreateRegion();
	}

	XUnionRectWithRegion (rb, null, scratch);
	switch (RectInRegion(scratch, ra)) {
	case RectangleIn:
	case RectanglePart:
		return (True);
	default:
		return (False);
	}
} /* RectInRect */

/**
 ** GetCrayons()
 **/

static void
#if	OlNeedFunctionPrototypes
GetCrayons (
	Widget			w
)
#else
GetCrayons (w)
	Widget			w;
#endif
{
	if (!TEXTFIELD_P(w).attrs)
		TEXTFIELD_P(w).attrs = OlgCreateAttrs(
			XtScreen(w),
			PRIMITIVE_P(w).foreground,
			(OlgBG *)&CORE_P(w).background_pixel,
			False,
			OL_DEFAULT_POINT_SIZE
		);
	return;
} /* GetCrayons */

/**
 ** FreeCrayons()
 **/

static void
#if	OlNeedFunctionPrototypes
FreeCrayons (
	Widget			w
)
#else
FreeCrayons (w)
	Widget			w;
#endif
{
	if (TEXTFIELD_P(w).attrs) {
		OlgDestroyAttrs (TEXTFIELD_P(w).attrs);
		TEXTFIELD_P(w).attrs = 0;
	}
	return;
} /* FreeCrayons */

/**
 ** OverrideDefaultTextEditResources()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
OverrideDefaultTextEditResources (
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
)
#else
OverrideDefaultTextEditResources (w, offset, value)
	Widget			w;
	int			offset;
	XrmValue *		value;
#endif
{
	Dimensions		d;

	/*
	 * We need to initialize some of the TextEdit's resources to
	 * different defaults but:
	 * - we don't want to provide simple defaults, we also want to
	 *   keep the client from changing them, and
	 * - we need to set these defaults before the TextEdit initialize
	 *   method runs.
	 *
	 * Note: We don't set the left margin, nor do we set the
	 * right margin if the text field can't step. Instead, we let the
	 * TextEdit superclass figure the optimum values for these. The
	 * reason is that the insert-point cursor will spread beyond the
	 * nominal edge of the text when in the left-most or right-most
	 * position, and a non-zero left/right margin gives some space
	 * for this. Bottom and top margins we want to control, so that
	 * the text is properly spaced above the underline. Because we
	 * don't always try to set the right margin, we can't use the
	 * same means to prevent clients from changing it so we have to
	 * "manually" check in set_values.
	 */

	/* if charsVisible is the default then update it */
	if (TEXTEDIT_P(w).charsVisible == XtUnspecifiedCardinal) {
	    TEXTEDIT_P(w).charsVisible = TEXTFIELD_P(w).maximum_size;
	    if (TEXTEDIT_P(w).charsVisible == 0)
		TEXTEDIT_P(w).charsVisible = 20;	/* ugh */
	}
	TEXTEDIT_P(w).linesVisible = 1;
	TEXTEDIT_P(w).wrapMode = OL_WRAP_OFF;
	TEXTEDIT_P(w).sourceType = OL_STRING_SOURCE;
	TEXTEDIT_P(w).insertReturn = False;

	TEXTFIELD_P(w).attrs = 0;
	GetCrayons (w);
	FetchDimensions (w, &d);

	/*
	 * See also _OlTFComputeRectangles.
	 */
	if (CanStep(w)) {
		TEXTEDIT_P(w).rightMargin = d.gap + 2*d.arrow_width;
		if (!OlgIs3d())
			TEXTEDIT_P(w).rightMargin
			    -= 2*OlgGetHorizontalStroke(TEXTFIELD_P(w).attrs);
	}

	TFOverrideDefaultTextEditResources (w, &d);

	/*
	 * Can't return without setting the pointer to something valid.
	 */
	value->addr = (XtPointer)&TEXTFIELD_P(w).bit_bucket;
	return;
} /* OverrideDefaultTextEditResources */
