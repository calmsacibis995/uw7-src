#ifndef	NOIDENT
#ident	"@(#)handles:Handles.c	1.21"
#endif

#include "stdio.h"
#include "ctype.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/OlCursors.h"
#include "Xol/Error.h"
#include "Xol/HandlesP.h"
#include "Xol/HandlesExP.h"

#define ClassName Handles
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&handlesClassRec)
#define superClass	((WidgetClass)&eventObjClassRec)
#define className	"Handles"

#define HANDLES_DEBUG
#if	defined(HANDLES_DEBUG)
static Boolean		handles_debug;
#endif

/*
 * Private types:
 */

typedef enum {
	NormalLook,
	GrabbedLook,
	FocusedLook
}			HandleLook;

typedef struct HandleData {
	Cardinal		n;
	Cardinal		pane_n;
	OlDefine		position;
	XRectangle		geometry;
}			HandleData;

typedef struct BorderMargins {
	Dimension		width;
	Dimension		height;
}			BorderMargins;

typedef void		(*HandleOp) OL_ARGS((
	Widget			w,
	HandleData *		more,
	XtPointer		client_data
));

/*
 * Convenient macros:
 */

#define SWAP(A,B) (A ^= B, B ^= A, A ^= B)

#define ButtonEventsGrabbedFromParent \
	ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask

	/*
	 * Some loops that use this macro will change .num_handles,
	 * thus the need for the temporary _nh.
	 */
#define ForEachHandle(W,N,H) \
	if ((_nh = HANDLES_P(W).num_handles))				\
		for (N = 0; (H = HANDLES_P(W).handles + N), N < _nh; N++)

#define PaneOfHandle(W,HANDLE) HANDLES_P(W).panes[(HANDLE)->pane_n]

#define HandleAllowed(W,PANE,POSITION) \
	ComputeHandleGeometry(W, PANE, POSITION, (XRectangle *)0)

#define InitDestroyedAsNecessary(W) \
	if (!HANDLES_P(W).destroyed)					\
		HANDLES_P(W).destroyed = (Window *)XtMalloc(		\
			HANDLES_P(W).num_handles * sizeof(Window)	\
		);							\
	else

#define ClearDestroyed(W) \
	if (HANDLES_P(W).destroyed) {					\
		XtFree ((char *)HANDLES_P(W).destroyed);		\
		HANDLES_P(W).destroyed = 0;				\
	} else

#define PushDestroyed(W,WINDOW) \
	HANDLES_P(W).destroyed[HANDLES_P(W).num_destroyed++] = WINDOW

#define PopDestroyed(W) \
	(HANDLES_P(W).num_destroyed?					\
		  HANDLES_P(W).destroyed[--HANDLES_P(W).num_destroyed]	\
		: None)

#define RectInRegion(R,r) \
	(!R || XRectInRegion(R,r.x,r.y,r.width,r.height) != RectangleOut)

#define NewList(TYPE,LIST,N) \
	(TYPE *)memcpy(							\
		XtMalloc((N) * sizeof(TYPE)),				\
		(XtPointer)LIST, (N) * sizeof(TYPE)			\
	)

#define RightEdge(G) ((G).x + (int)(G).width)
#define BottomEdge(G) ((G).y + (int)(G).height)

#define PointInRect(X,Y,R) \
	(R.x <= X && X < RightEdge(R) && R.y <= Y && Y < BottomEdge(R))

#define SameGeometry(A,B) \
	(   (A).x == (B).x && (A).y == (B).y				\
	 && (A).width == (B).width && (A).height == (B).height   )

#define STREQU(A,B) (strcmp((A),(B)) == 0)

	/*
	 * Some typed numbers to keep our pal, the ANSI-C compiler, happy.
	 */
#if	defined(__STDC__)
# define u2	2U
#else
# define u2	2
#endif

/*
 * Local routines:
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
	Widget			w,		/*NOTUSED*/
	XtWidgetGeometry *	request,	/*NOTUSED*/
	XtWidgetGeometry *	preferred	/*NOTUSED*/
));
static void		HighlightHandler OL_ARGS((
	Widget			w,
	OlDefine		type
));
static Widget		TraversalHandler OL_ARGS((
	Widget			w,
	Widget			ignore,		/*NOTUSED*/
	OlVirtualName		dir,
	Time			time		/*NOTUSED*/
));
static Boolean		Activate OL_ARGS((
	Widget			w,
	OlVirtualName		type,
	XtPointer		data
));
static void		TransparentProc OL_ARGS((
	Widget			w,
	Pixel			pixel,		/*NOTUSED*/
	Pixmap			pixmap		/*NOTUSED*/
));
static void		Realize OL_ARGS((
	Widget			w
));
static void		Unrealize OL_ARGS((
	Widget			w
));
static void		Clear OL_ARGS((
	Widget			w
));
static void		Layout OL_ARGS((
	Widget			w
));
static void		SetSelection OL_ARGS((
	Widget			w,
	Widget *		panes,
	Cardinal		num_panes,
	Boolean			selected
));
static void		GetSelection OL_ARGS((
	Widget			w,
	Widget **		panes,
	Cardinal *		num_panes
));
static void		CheckForBadResources OL_ARGS((
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
));
static void		CreatePanesList OL_ARGS((
	Widget			w,
	Boolean			add_callbacks
));
static void		DestroyPanesList OL_ARGS((
	Widget			w,
	Boolean			remove_callbacks
));
static void		RemovePaneCB OL_ARGS((
	Widget			pane,
	XtPointer		client_data,
	XtPointer		call_data	/*NOTUSED*/
));
static void		ForEachHandleByPane OL_ARGS((
	Widget			w,
	HandleOp		op,
	XtPointer		client_data
));
static Boolean		ComputeHandleGeometry OL_ARGS((
	Widget			w,
	Widget			pane,
	OlDefine		position,
	XRectangle *		geometry
));
static Handle *		AddHandle OL_ARGS((
	Widget			w,
	HandleData *		more
));
static void		SaveCurrentGrabbed OL_ARGS((
	Widget			w
));
static void		RestoreCurrentGrabbed OL_ARGS((
	Widget			w
));
static Handle *		RestoreOne OL_ARGS((
	Widget			w,
	Handle *		handle
));
static void		ChangeHandle OL_ARGS((
	Widget			w,
	Handle *		handle
));
static void		DestroyHandles OL_ARGS((
	Widget			w
));
static void		DestroyHandle OL_ARGS((
	Widget			w,
	Handle *		handle
));
static void		ReallocateRemainingHandles OL_ARGS((
	Widget			w
));
static void		CommitDestroyedHandles OL_ARGS((
	Widget			w
));
static void		TransferHandles OL_ARGS((
	Widget			old,
	Widget			new
));
static void		AddHandleIfNewPane OL_ARGS((
	Widget			w,
	HandleData *		more,
	XtPointer		client_data
));
static int		SearchPaneList OL_ARGS((
	Widget			w,
	Widget			pane
));
static void		TransferPanes OL_ARGS((
	Widget			old,
	Widget			new
));
static void		LayoutHandle OL_ARGS((
	Widget			w,
	HandleData *		more,
	XtPointer		client_data	/*NOTUSED*/
));
static Handle *		NextHandle OL_ARGS((
	Widget			w,
	Handle *		handle,
	OlVirtualName		dir
));
static void		MoveFocus OL_ARGS((
	Widget			w,
	Handle *		handle,
	Boolean			change_look
));
static void		ParentButtonEH OL_ARGS((
	Widget			parent,
	XtPointer		client_data,
	XEvent *		event,
	Boolean *		continue_to_dispatch
));
static void		ParentExposeEH OL_ARGS((
	Widget			parent,		/*NOTUSED*/
	XtPointer		client_data,
	XEvent *		event,
	Boolean *		continue_to_dispatch
));
static void		ParentExpose OL_ARGS((
	Widget			w,
	Region			region
));
static void		GrabHandle OL_ARGS((
	Widget			w,
	Handle *		handle,
	Position		x,
	Position		y,
	Time *			time
));
static void		UngrabHandle OL_ARGS((
	Widget			w,
	Boolean			resize_pane
));
static void		CheckCursor OL_ARGS((
	Widget			w,
	Position		x,
	Position		y,
	Boolean			leaving
));
static int		PaneBorderUnderXY OL_ARGS((
	Widget			w,
	Position		x,
	Position		y
));
static void		SelectPaneByXY OL_ARGS((
	Widget			w,
	Position		x,
	Position		y,
	Boolean			toggle
));
static void		SelectPane OL_ARGS((
	Widget			w,
	Cardinal		n,
	Boolean			toggle
));
static void		CallCallbacks OL_ARGS((
	Widget			w,
	String			callback,
	XtPointer		call_data
));
static void		StartMove OL_ARGS((
	Widget			w,
	Position		x,
	Position		y
));
static void		EndMove OL_ARGS((
	Widget			w,
	Boolean			resize_pane
));
static void		Step OL_ARGS((
	Widget			w,
	Position		step
));
static void		Move OL_ARGS((
	Widget			w,
	Position		x,
	Position		y
));
static void		GetPaneGeometry OL_ARGS((
	Widget			w,
	Widget			pane,
	OlDefine		position,
	XtWidgetGeometry *	geometry
));
static Boolean		AcceptCompromise OL_ARGS((
	OlDefine		position,
	XtWidgetGeometry *	try,
	XtWidgetGeometry *	get
));
static void		XorRubberLines OL_ARGS((
	Widget			w,
	Widget			pane,
	OlDefine		position,
	XtWidgetGeometry *	geometry,
	Region			region
));
static void		DrawBorders OL_ARGS((
	Widget			w,
	Region			region
));
static void		DrawBorder OL_ARGS((
	Widget			w,
	Widget			pane,
	Boolean			selected,
	Region			region
));
static void		ClearBorder OL_ARGS((
	Widget			w,
	Widget			pane,
	Boolean			exposures
));
static void		CalculateBorderMargins OL_ARGS((
	Widget			w,
	BorderMargins *		margins
));
static void		CalculateBorderBounds OL_ARGS((
	Widget			w,
	Widget			pane,
	XRectangle *		r
));
static void		CalculateBorderRectangles OL_ARGS((
	Widget			w,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	Dimension		border_width,
	OlDefine		position,
	Boolean			selected,
	XRectangle *		r
));
static void		ColorHandles OL_ARGS((
	Widget			w
));
static void		GetCrayons OL_ARGS((
	Widget			w
));
static void		GetSomeCrayons OL_ARGS((
	Widget			w
));
static void		_GetSomeCrayons OL_ARGS((
	Widget			w,
	Pixel			foreground,
	Pixel			background_pixel,
	Pixmap			background_pixmap
));
static void		FreeCrayons OL_ARGS((
	Widget			w
));
static void		SetHandlePixmap OL_ARGS((
	Widget			w,
	OlDefine		position,
	HandleLook		look,
	XSetWindowAttributes *	attributes,
	XtValueMask *		mask
));

/*
 * Private data:
 */

static Region		_null_region;

static Handle		_current; /* temps that span SaveCurrentGrabbed */
static Handle		_grabbed; /* & RestoreCurrentGrabbed intervals  */

static Cardinal		_nh;      /* used by ForEachHandle              */

#if	defined(DEBUG)
static HandlesWidget	_hw;      /* useful when using debugger         */
#endif

/*
 * Resources:
 */

	/*
	 * A chiseled line is the OPEN LOOK default shadow type, while
	 * a raised edge is the Motif type. Both have the same default
	 * thickness.
	 */
static OlDefine		shadow_type = OL_SHADOW_ETCHED_IN;

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(HandlesRec,F)

	/*
	 * These are used to size the handle-windows. Width and
	 * height are for a horizontally oriented handle. Make
	 * sure the .orientation field is initialized accordingly
	 * (OL_HORIZONTAL).
	 */
    {	/* SGI */
	XtNheight, XtCHeight,
	XtRDimension, sizeof(Dimension), offset(rect.height),
	XtRString, (XtPointer)"6 points"
    },
    {	/* SGI */
	XtNwidth, XtCWidth,
	XtRDimension, sizeof(Dimension), offset(rect.width),
	XtRString, (XtPointer)"15 points"
    },
    {	/* SGI */
	XtNborderWidth, XtCReadOnly,
	XtRDimension, sizeof(Dimension), offset(rect.border_width),
	XtRImmediate, (XtPointer)0
    },

    {	/* SGI */
	XtNselect, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(handles.select),
	XtRCallback, (XtPointer)0
    },
    {	/* SGI */
	XtNunselect, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(handles.unselect),
	XtRCallback, (XtPointer)0
    },
    {	/* SGI */
	XtNpaneResized, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(handles.pane_resized),
	XtRCallback, (XtPointer)0
    },
    {	/* SGI */
	XtNpanes, XtCPanes,
	XtRWidgetList, sizeof(WidgetList), offset(handles.panes),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNnumPanes, XtCNumPanes,
	XtRCardinal, sizeof(Cardinal), offset(handles.num_panes),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNquery, XtCQuery,
	XtROlDefine, sizeof(OlDefine), offset(handles.query),
	XtRImmediate, (XtPointer)OL_BOTH
    },
    {	/* SGI */
	XtNshadowType, XtCShadowType,
	XtROlDefine, sizeof(OlDefine), offset(handles.shadow_type),
	XtROlDefine, (XtPointer)&shadow_type,
    },
    {	/* SGI */
	XtNshadowThickness, XtCShadowThickness,
	XtRDimension, sizeof(Dimension), offset(handles.shadow_thickness),
	XtRString, (XtPointer)"2 points"
    },
    {	/* SGI */
	XtNresizeAtEdges, XtCResizeAtEdges,
	XtRBoolean, sizeof(Boolean), offset(handles.resize_at_edges),
	XtRImmediate, (XtPointer)False
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

HandlesClassRec		handlesClassRec = {
	/*
	 * RectObj class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(HandlesRec),
/* class_initialize     */                       ClassInitialize,
/* class_part_init   (D)*/                       ClassPartInitialize,
/* class_inited         */                       False,
/* initialize        (D)*/                       Initialize,
/* initialize_hook   (D)*/ (XtArgsProc)          0, /* Obsolete */
/* (unused)             */ (XtProc)              0,
/* (unused)             */ (XtPointer)           0,
/* (unused)             */ (Cardinal)            0,
/* resources         (D)*/                       resources,
/* num_resources        */                       XtNumber(resources),
/* xrm_class            */                       NULLQUARK,
/* (unused)             */ (Boolean)             0,
/* (unused)             */ (XtEnum)              0,
/* (unused)             */ (Boolean)             0,
/* (unused)             */ (Boolean)             0,
/* destroy           (U)*/                       Destroy,
/* resize            (I)*/ (XtWidgetProc)        0,
/* expose            (I)*/ (XtExposeProc)        0,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/                       GetValuesHook,
/*
 * MORE: The current implementation of focus is wrong (according to the
 * OPEN LOOK spec.), but we can't fix it now. Until it is fixed, we need
 * to disable focus.
 */
/* accept_focus      (I)*/ (XtProc) /* sorry! */ 0,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* (unused)             */ (String)              0,
/* query_geometry    (I)*/                       QueryGeometry,
/* (unused)             */ (XtProc)              0,
/* extension            */ (XtPointer)           0
	},
	/*
	 * EventObj class:
	 */
	{
/* focus_on_select      */			 True,
/* highlight_handler (I)*/                       HighlightHandler,
/* traversal_handler (I)*/                       TraversalHandler,
/* register_focus    (I)*/                       XtInheritRegisterFocus,
/* activate          (I)*/                       Activate,
/* event_procs          */ (OlEventHandlerList)  0,
/* num_event_procs      */ (Cardinal)            0,
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { (_OlDynResource *)0, (Cardinal)0 },
/* transparent_proc  (I)*/                       TransparentProc,
	},
	/*
	 * Handles class:
	 */
	{
/* realize           (I)*/                       Realize,
/* unrealize         (I)*/                       Unrealize,
/* clear             (I)*/                       Clear,
/* layout            (I)*/                       Layout,
/* set_selection     (I)*/                       SetSelection,
/* get_selection     (I)*/                       GetSelection,
/* extension            */ (XtPointer)           0
	}
};

WidgetClass		handlesWidgetClass = thisClass;

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
#if	defined(DEBUG)
printf ("_OlResolveOlgGUISymbols called just from Primitive.\n");
printf ("Why not in/out/etc instead of shadowIn/shadowOut/etc?\n");
printf ("Why not OL_IN/OL_OUT/etc instead of OL_SHADOW_IN/etc?\n");
printf ("Why no shadow stuff in EventObj?\n");
printf ("0.5 in OlgShadow.c!\n");
#endif
	/*
	 * For XtNquery:
	 */
	_OlAddOlDefineType ("none",   OL_NONE);
	_OlAddOlDefineType ("child",  OL_CHILD);
	_OlAddOlDefineType ("parent", OL_PARENT);
	_OlAddOlDefineType ("both",   OL_BOTH);

	/*
	 * For XtNshadowType:
	 */
        _OlAddOlDefineType("shadowOut",       OL_SHADOW_OUT);
        _OlAddOlDefineType("shadowIn",        OL_SHADOW_IN);
        _OlAddOlDefineType("shadowEtchedOut", OL_SHADOW_ETCHED_OUT);
        _OlAddOlDefineType("shadowEtchedIn",  OL_SHADOW_ETCHED_IN);

	_null_region = XCreateRegion();

	if (OlGetGui() == OL_MOTIF_GUI)
		shadow_type = OL_SHADOW_OUT;

#if	defined(HANDLES_DEBUG)
	handles_debug = (getenv("HANDLES_DEBUG") != 0);
#endif
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
	if (HANDLES_C(WC).F == (INH))					\
		HANDLES_C(WC).F = HANDLES_C(SUPER_C(WC)).F

	INHERIT (wc, realize, XtInheritRealizeHandles);
	INHERIT (wc, unrealize, XtInheritUnrealizeHandles);
	INHERIT (wc, clear, XtInheritClearHandles);
	INHERIT (wc, layout, XtInheritLayoutHandles);
	INHERIT (wc, set_selection, XtInheritSetSelection);
	INHERIT (wc, get_selection, XtInheritGetSelection);
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
	HANDLES_P(new).states = 0;
	HANDLES_P(new).num_handles = 0;
	HANDLES_P(new).handles = 0;
	HANDLES_P(new).grabbed = 0;
	HANDLES_P(new).current = 0;
	HANDLES_P(new).destroyed = 0;
	HANDLES_P(new).num_destroyed = 0;
	HANDLES_P(new).invert_gc = 0;
	HANDLES_P(new).attrs = 0;
	HANDLES_P(new).tb_handle = 0;
	HANDLES_P(new).lr_handle = 0;
	HANDLES_P(new).tb_handle_grabbed = 0;
	HANDLES_P(new).lr_handle_grabbed = 0;
	HANDLES_P(new).tb_handle_focused = 0;
	HANDLES_P(new).lr_handle_focused = 0;
	HANDLES_P(new).orientation = OL_HORIZONTAL;
	HANDLES_P(new).expose_region = XCreateRegion();

	CheckForBadResources (new, args, num_args);

	/*
	 * Make a copy of the list of panes. We need to do this to allow
	 * the client to provide a new list of different panes using the
	 * same array pointer--in SetValues we need to examine both lists
	 * to see what old handles to destroy and new handles to create.
	 * Also, create a parallel array that contains auxiliary pane
	 * data. Also, add a destroy_callback to each pane, so we can
	 * clean up when it goes away without warning from the client.
	 */
	if (HANDLES_P(new).panes)
		CreatePanesList (new, True);

	/*
	 * Each handle is a pretty simple thing. We could burn widget
	 * space by making each a widget--that would be nice and object-
	 * oriented--but we think we're better than that :-) Instead, each
	 * handle is just a window. We can easily draw in these windows,
	 * but since they are not widgets we can't easily get events from
	 * them. So, we let the events percolate up to their parent
	 * window (our parent widget), then steal them from our parent.
	 * See the event handler for details.
	 *
	 * CAUTION: If you change the event masks (or anything else in
	 * this event registration), remember to change the corresponding
	 * information in the event removal (see Destroy).
	 */
	XtInsertEventHandler (
		XtParent(new),
		ButtonEventsGrabbedFromParent, False,
		ParentButtonEH, (XtPointer)new,
		XtListHead
	);
	XtAddEventHandler (
		XtParent(new),
		ExposureMask, False,
		ParentExposeEH, (XtPointer)new
	);

	/*
	 * Some of the drawing tools need a window, so we can't get them
	 * until later. But the OlgAttr we can get now and it is needed
	 * to answer requests for the border thickness.
	 */
	GetSomeCrayons (new);

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
	/*
	 * Destroy things that need the panes list.
	 */
	if (XtIsRealized(w))
		DestroyHandles (w);

	/*
	 * Now destroy the panes list and related lists.
	 */
	if (HANDLES_P(w).panes)
		DestroyPanesList (w, True);

	/*
	 * Destroy other stuff.
	 */
	XtRemoveEventHandler (
		XtParent(w),
		ButtonEventsGrabbedFromParent, False,
		ParentButtonEH, (XtPointer)w
	);
	XtRemoveEventHandler (
		XtParent(w),
		ExposureMask, False,
		ParentExposeEH, (XtPointer)w
	);
	XDestroyRegion (HANDLES_P(w).expose_region);
	FreeCrayons (w);

	return;
} /* Destroy */

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
	Boolean			relayout      = False;

#define DIFFERENT(F) \
	(((HandlesWidget)new)->F != ((HandlesWidget)current)->F)


	CheckForBadResources (new, args, num_args);

	if (
		DIFFERENT(rect.x) || DIFFERENT(rect.y)
	     || DIFFERENT(rect.width) || DIFFERENT(rect.height)
	     || DIFFERENT(rect.border_width)
	) {
		/*
		 * The main reason we don't want anyone changing these
		 * values is that Xt will clear the old and new areas,
		 * and this wrecks the XOR drawing of the floating handle.
		 */
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(new),
			"noGeometryChange", "setValues",
			OleCOlToolkitWarning,
			"Widget %s: geometry cannot be changed",
			XtName(new)
		);
		CORE_P(new).x = CORE_P(current).x;
		CORE_P(new).y = CORE_P(current).y;
		CORE_P(new).width = CORE_P(current).width;
		CORE_P(new).height = CORE_P(current).height;
		CORE_P(new).border_width = CORE_P(current).border_width;
	}

	if (!DIFFERENT(handles.panes) && DIFFERENT(handles.num_panes)) {
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(new),
			"numPanesNotPanes", "setValues",
			OleCOlToolkitWarning,
			"Widget %s: XtNnumPanes can't be changed without also changing XtNpanes",
			XtName(new)
		);
		HANDLES_P(new).num_panes = HANDLES_P(current).num_panes;
	}
	if (DIFFERENT(handles.panes)) {
		if (HANDLES_P(new).panes) {
			if (!HANDLES_P(current).panes)
				CreatePanesList (new, True);
			else {
				CreatePanesList (new, False);
				TransferPanes (current, new);
				if (XtIsRealized(XtParent(new)))
					TransferHandles (current, new);
			}
		}
		DestroyPanesList (current, False);
		relayout = True;
	}

	if (DIFFERENT(handles.resize_at_edges))
		relayout = True;

#undef	DIFFERENT

	if (relayout && XtIsRealized(new))
		OlLayoutHandles (new);

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
	static Boolean		compiled_yet = False;

	static XtResource	resources[] = {
#define offset(F) XtOffsetOf(BorderMargins, F)
	    {
		XtNpaneBorderWidth, "",
		XtRDimension, sizeof(Dimension), offset(width),
		"", (XtPointer)0
	    }, {
		XtNpaneBorderHeight, "",
		XtRDimension, sizeof(Dimension), offset(height),
		"", (XtPointer)0
	    }
#undef	offset
	};

	BorderMargins		m;


	/*
	 * The Intrinsics manual doesn't say it, but XtGetSubvalues
	 * assumes the resource list has been compiled. Calling
	 * XtGetSubresources takes care of this.
	 */
	if (!compiled_yet) {
		XtGetSubresources (
			w, &m, "junk", "Junk",
			resources, XtNumber(resources),
			(Arg *)0, (Cardinal)0
		);
		compiled_yet = True;
	}

	CalculateBorderMargins (w, &m);
	XtGetSubvalues (
	    (XtPointer)&m, resources, XtNumber(resources), args, *num_args
	);

	return;
} /* GetValuesHook() */

/**
 ** QueryGeometry()
 **/

/*ARGSUSED*/
static XtGeometryResult
#if	OlNeedFunctionPrototypes
QueryGeometry (
	Widget			w,		/*NOTUSED*/
	XtWidgetGeometry *	request,	/*NOTUSED*/
	XtWidgetGeometry *	preferred	/*NOTUSED*/
)
#else
QueryGeometry (w, request, preferred)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	preferred;
#endif
{
	/*
	 * Our real geometry is just the floating handle, which of fixed
	 * size (dictated by the OPEN LOOK spec.)
	 *
	 * MORE: Other geometry (esp. position) would be OK.
	 */
	return (XtGeometryNo);
} /* QueryGeometry */

/**
 ** HighlightHandler()
 **/

static void
#if	OlNeedFunctionPrototypes
HighlightHandler (
	Widget			w,
	OlDefine		type
)
#else
HighlightHandler (w, type)
	Widget			w;
	OlDefine		type;
#endif
{
	Boolean			change = True;

	Handle *		current = HANDLES_P(w).current;


	switch (type) {
	case OL_IN:
		if (!current)
			current = HANDLES_P(w).current = HANDLES_P(w).handles;
		break;
	case OL_OUT:
		HANDLES_P(w).current = 0;
		if (HANDLES_P(w).grabbed) {
			/*
			 * Activate will change the grabbed handle,
			 * so if that is also the current handle, don't
			 * do extra work.
			 */
			if (current == HANDLES_P(w).grabbed)
				change = False;
			Activate (w, OL_CANCEL, (XtPointer)HANDLES_P(w).grabbed);
		}
		break;
	}

	if (change && current)
		ChangeHandle (w, current);

	return;
} /* HighlightHandler */

/**
 ** TraversalHandler()
 **/

/*ARGSUSED*/
static Widget
#if	OlNeedFunctionPrototypes
TraversalHandler (
	Widget			w,
	Widget			ignore,		/*NOTUSED*/
	OlVirtualName		dir,
	Time			time		/*NOTUSED*/
)
#else
TraversalHandler (w, ignore, dir, time)
	Widget			w;
	Widget			ignore;
	OlVirtualName		dir;
	Time			time;
#endif
{
	/*
	 * If the user has already grabbed a handle, then these
	 * keys move the handle, not the focus.
	 */
	if (HANDLES_P(w).grabbed)
		Activate (w, dir, (XtPointer)HANDLES_P(w).grabbed);
	else
		MoveFocus (
			w, NextHandle(w, HANDLES_P(w).current, dir), True
		);

	return (w);
} /* TraversalHandler */

/**
 ** Activate()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
Activate (
	Widget			w,
	OlVirtualName		type,
	XtPointer		data
)
#else
Activate (w, type, data)
	Widget			w;
	OlVirtualName		type;
	XtPointer		data;
#endif
{
	Boolean			consumed = False;

	Handle *		handle   = (Handle *)data;


/*
... Arrow keys move between panes, too, so that they can be selected.
... OL_SELECTKEY and OL_ADJUSTKEY select/toggle-select a pane.
 */
	switch (type) {
	case OL_MOVELEFT:
	case OL_MOVERIGHT:
	case OL_MOVEUP:
	case OL_MOVEDOWN:
	case OL_MULTILEFT:
	case OL_MULTIRIGHT:
	case OL_MULTIUP:
	case OL_MULTIDOWN:
		if (!handle)
			handle = HANDLES_P(w).grabbed;
		if (handle) {
			consumed = True;
			switch (handle->position) {
			case OL_LEFT:
			case OL_RIGHT:
				switch (type) {
				case OL_MOVELEFT:
					Step (w, -1);
					break;
				case OL_MOVERIGHT:
					Step (w, 1);
					break;
				case OL_MULTILEFT:
					Step (w, -_OlGetMultiObjectCount(w));
					break;
				case OL_MULTIRIGHT:
					Step (w, _OlGetMultiObjectCount(w));
					break;
				}
				break;
			case OL_TOP:
			case OL_BOTTOM:
				switch (type) {
				case OL_MOVEUP:
					Step (w, -1);
					break;
				case OL_MOVEDOWN:
					Step (w, 1);
					break;
				case OL_MULTIUP:
					Step (w, -_OlGetMultiObjectCount(w));
					break;
				case OL_MULTIDOWN:
					Step (w, _OlGetMultiObjectCount(w));
					break;
				}
				break;
			}
		}
		break;

	case OL_SELECTKEY:
	case OL_DRAG:
		consumed = True;
		if (!handle)
			handle = HANDLES_P(w).current;
		if (handle)
			GrabHandle (w, handle, -1, -1, (Time *)0);
		break;

	case OL_UNDO:
		if (!HANDLES_P(w).grabbed) {
/*
... Undo previous completed move.
 */
			consumed = True;
			break;
		}
		/*FALLTHROUGH*/
	case OL_STOP:
	case OL_CANCEL:
		consumed = True;
		UngrabHandle (w, False);
		break;

	case OL_RETURN:
	case OL_DEFAULTACTION:
	case OL_DROP:
		consumed = True;
		UngrabHandle (w, True);
		break;
	}

	return (consumed);
} /* Activate */

/**
 ** TransparentProc()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
TransparentProc (
	Widget			w,
	Pixel			pixel,		/*NOTUSED*/
	Pixmap			pixmap		/*NOTUSED*/
)
#else
TransparentProc (w, pixel, pixmap)
	Widget			w;
	Pixel			pixel;
	Pixmap			pixmap;
#endif
{
	/*
	 * MORE: Maybe we should redraw the borders, but typically we are
	 * changing colors through dynamic resources, and the workspace
	 * manager helps us out by refreshing all windows--thus we'll
	 * redraw in response to an expose event.
	 */
	ColorHandles (w);
	return;
} /* TransparentProc */

/**
 ** Realize()
 **/

static void
#if	OlNeedFunctionPrototypes
Realize (
	Widget			w
)
#else
Realize (w)
	Widget			w;
#endif
{
	GetCrayons (w);
	OlLayoutHandles (w);
	return;
} /* Realize */

/**
 ** Unrealize()
 **/

static void
#if	OlNeedFunctionPrototypes
Unrealize (
	Widget			w
)
#else
Unrealize (w)
	Widget			w;
#endif
{
	FreeCrayons (w);
	DestroyHandles (w);
	return;
} /* Unrealize */

/**
 ** Clear()
 **/

static void
#if	OlNeedFunctionPrototypes
Clear (
	Widget			w
)
#else
Clear (w)
	Widget			w;
#endif
{
	Cardinal		n;

	for (n = 0; n < HANDLES_P(w).num_panes; n++)
		ClearBorder (w, HANDLES_P(w).panes[n], False);
	return;
} /* Clear */

/**
 ** Layout()
 **/

static void
#if	OlNeedFunctionPrototypes
Layout (
	Widget			w
)
#else
Layout (w)
	Widget			w;
#endif
{
	Handle *		handle;

	Cardinal		n;


	SaveCurrentGrabbed (w);

	/*
	 * Clear borders of panes that have changed managed state, to
	 * erase the borders of newly unmanaged panes and to generate
	 * exposures (so we can later draw the borders) for newly managed
	 * panes. Also, update the managed state, and clear the selected
	 * state for unmanaged panes.
	 *
	 * MORE: Need to clear the borders of more than just the panes
	 * that have changed managed state, also need to clear the borders
	 * of panes that have a different geometry. As of now we don't
	 * know the old sizes of the panes so we can't check. For now,
	 * just clear the whole parent; this is pretty heavy-handed and
	 * can cause a lot of unnecessary redrawing, but for a quick-fix
	 * it can't be helped. The clear causes an expose, so at least
	 * things will get fixed up properly.
	 */
	if (XtIsRealized(XtParent(w))) {
		Widget parent = XtParent(w);
		XClearArea (
			XtDisplay(parent), XtWindow(parent),
			0, 0, CORE_P(parent).width, CORE_P(parent).height,
			True
		);
	}
	for (n = 0; n < HANDLES_P(w).num_panes; n++) {
		Widget			pane  = HANDLES_P(w).panes[n];
		PaneState *		state = &HANDLES_P(w).states[n];

/*		if (XtIsManaged(pane) != state->managed)		*/
/*			ClearBorder (w, pane, True);			*/
		state->managed = XtIsManaged(pane);
		if (!state->managed)
			state->selected = False;
	}

	/*
	 * Delete defunct handles (ones that can no longer be made
	 * available because a pane is in the wrong position or is no
	 * longer selected.)
	 */
	ForEachHandle (w, n, handle) {
		Widget			pane = PaneOfHandle(w, handle);

		if (
			!HANDLES_P(w).states[handle->pane_n].selected
		     || !HandleAllowed(w, pane, handle->position)
		)
			DestroyHandle (w, handle);
	}
	ReallocateRemainingHandles (w);

	/*
	 * Now lay out the handles. LayoutHandle will detect where new
	 * handles should be created, and will create them.
	 * Don't do this if the Handles widget (i.e. the Handles widget's
	 * parent) isn't realized, since LayoutHandle attempts to create
	 * windows.
	 */
	if (XtIsRealized(w))
		ForEachHandleByPane (w, LayoutHandle, (XtPointer)0);

	RestoreCurrentGrabbed (w);
	CommitDestroyedHandles (w);

	return;
} /* Layout */

/**
 ** SetSelection()
 **/

static void
#if	OlNeedFunctionPrototypes
SetSelection (
	Widget			w,
	Widget *		panes,
	Cardinal		num_panes,
	Boolean			selected
)
#else
SetSelection (w, panes, num_panes, selected)
	Widget			w;
	Widget *		panes;
	Cardinal		num_panes;
	Boolean			selected;
#endif
{
	Widget *		p = HANDLES_P(w).panes;

	PaneState *		states = HANDLES_P(w).states;

	Cardinal		np = HANDLES_P(w).num_panes;
	Cardinal		n;
	Cardinal		k;


	for (n = 0; n < num_panes; n++)
		for (k = 0; k < np; k++)
			if (panes[n] == p[k] && states[k].selected != selected)
				SelectPane (w, k, True);

	return;
} /* SetSelection */

/**
 ** GetSelection()
 **/

static void
#if	OlNeedFunctionPrototypes
GetSelection (
	Widget			w,
	Widget **		panes,
	Cardinal *		num_panes
)
#else
GetSelection (w, panes, num_panes)
	Widget			w;
	Widget **		panes;
	Cardinal *		num_panes;
#endif
{
	Widget *		p = HANDLES_P(w).panes;

	PaneState *		states = HANDLES_P(w).states;

	Cardinal		np = HANDLES_P(w).num_panes;
	Cardinal		k;
	Cardinal		n;


	n = 0;
	for (k = 0; k < np; k++)
		if (states[k].selected)
			n++;
	if (!n) {
		*num_panes = 0;
		*panes = 0;
		return;
	}

	*num_panes = n;
	*panes = (Widget *)XtMalloc(n * sizeof(Widget));

	n = 0;
	for (k = 0; k < np; k++)
		if (states[k].selected)
			(*panes)[n++] = p[k];

	return;
} /* GetSelection */

/**
 ** CheckForBadResources()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckForBadResources (
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
)
#else
CheckForBadResources (w, args, num_args)
	Widget			w;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Arg *			arg;

	Cardinal		n;


	if (CORE_P(w).border_width) {
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(w),
			"borderWidth", "set",
			OleCOlToolkitWarning,
			"Widget %s: Cannot set XtNborderWidth on this widget class",
			XtName(w)
		);
		CORE_P(w).border_width = 0;
	}
	if (HANDLES_P(w).panes && !HANDLES_P(w).num_panes) {
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(w),
			"nullPanes", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtNpanes set but XtNnumPanes is zero",
			XtName(w)
		);
		HANDLES_P(w).panes = 0;
	}
	if (!HANDLES_P(w).panes && HANDLES_P(w).num_panes) {
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(w),
			"zeroNumPanes", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtNpanes is not set but XtNnumPanes is not zero",
			XtName(w)
		);
		HANDLES_P(w).num_panes = 0;
	}

	for (n = 0, arg = args; n < *num_args; n++, arg++)
		if (
			STREQU(arg->name, XtNpaneBorderWidth)
		     || STREQU(arg->name, XtNpaneBorderHeight)
		)
			OlVaDisplayWarningMsg (
				XtDisplayOfObject(w),
				"readOnly", "set",
				OleCOlToolkitWarning,
				"Widget %s: resource XtN%s is read-only",
				XtName(w), arg->name
			);

	return;
} /* CheckForBadResources */

/**
 ** CreatePanesList()
 **/

static void
#if	OlNeedFunctionPrototypes
CreatePanesList (
	Widget			w,
	Boolean			add_callbacks
)
#else
CreatePanesList (w, add_callbacks)
	Widget			w;
	Boolean			add_callbacks;
#endif
{
	if (add_callbacks) {
		Cardinal		n;

		for (n = 0; n < HANDLES_P(w).num_panes; n++)
			XtAddCallback (
				HANDLES_P(w).panes[n], XtNdestroyCallback,
				RemovePaneCB, (XtPointer)w
			);
	}
	HANDLES_P(w).panes = NewList(
		Widget, HANDLES_P(w).panes, HANDLES_P(w).num_panes
	);
	HANDLES_P(w).states = (PaneState *)XtCalloc(
		HANDLES_P(w).num_panes, sizeof(PaneState)
	);

	return;
} /* CreatePanesList */

/**
 ** DestroyPanesList()
 **/

static void
#if	OlNeedFunctionPrototypes
DestroyPanesList (
	Widget			w,
	Boolean			remove_callbacks
)
#else
DestroyPanesList (w, remove_callbacks)
	Widget			w;
	Boolean			remove_callbacks;
#endif
{
	/*
	 * The caller is responsible for destroying any handles
	 * associated with this pane.
	 */

	if (remove_callbacks) {
		Cardinal		n;

		for (n = 0; n < HANDLES_P(w).num_panes; n++)
			XtRemoveCallback (
				HANDLES_P(w).panes[n], XtNdestroyCallback,
				RemovePaneCB, (XtPointer)w
			);
	}
	if (HANDLES_P(w).panes) {
		XtFree ((char *)HANDLES_P(w).panes);
		HANDLES_P(w).panes = 0;
	}
	if (HANDLES_P(w).states) {
		XtFree ((char *)HANDLES_P(w).states);
		HANDLES_P(w).states = 0;
	}

	return;
} /* DestroyPanesList */

/**
 ** RemovePaneCB()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
RemovePaneCB (
	Widget			pane,
	XtPointer		client_data,
	XtPointer		call_data	/*NOTUSED*/
)
#else
RemovePaneCB (pane, client_data, call_data)
	Widget			pane;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Widget			w = (Widget)client_data;

	Cardinal		n;
	Cardinal		pane_n;

	Handle *		handle;


	if (HANDLES_P(w).handles)
		SaveCurrentGrabbed (w);

	ClearBorder (w, pane, True);

	/*
	 * Remove the handles for this pane.
	 */
	if (HANDLES_P(w).handles) {
		ForEachHandle (w, n, handle)
			if (PaneOfHandle(w, handle) == pane)
				DestroyHandle (w, handle);
		ReallocateRemainingHandles (w);
		CommitDestroyedHandles (w);
	}

	/*
	 * Remove the pane from the list. No urgent need to reallocate
	 * the list at this point (not that much space will be gained).
	 */
	for (pane_n = 0; pane_n < HANDLES_P(w).num_panes; pane_n++)
		if (pane == HANDLES_P(w).panes[pane_n]) {
			OlMemMove (
				Widget,
				&HANDLES_P(w).panes[pane_n],
				&HANDLES_P(w).panes[pane_n + 1],
				HANDLES_P(w).num_panes - pane_n - 1
			);
			OlMemMove (
				PaneState,
				&HANDLES_P(w).states[pane_n],
				&HANDLES_P(w).states[pane_n + 1],
				HANDLES_P(w).num_panes - pane_n - 1
			);
			HANDLES_P(w).num_panes--;
			break;
		}

	/*
	 * Update the pane references for other handles.
	 */
	ForEachHandle (w, n, handle)
		if (handle->pane_n > pane_n)
			handle->pane_n--;

	if (HANDLES_P(w).handles)
		RestoreCurrentGrabbed (w);

	/*
	 * This callback will be removed from the pane by Xt.
	 */
	return;
} /* RemovePaneCB */

/**
 ** ForEachHandleByPane()
 **/

static void
#if	OlNeedFunctionPrototypes
ForEachHandleByPane (
	Widget			w,
	HandleOp		op,
	XtPointer		client_data
)
#else
ForEachHandleByPane (w, op, client_data)
	Widget			w;
	HandleOp		op;
	XtPointer		client_data;
#endif
{
	Cardinal		j;
	Cardinal		n;
	Cardinal		nhandles = 0;

	Widget			pane;


	if (!XtIsManaged(w) || !XtIsRealized(w))
		return;

	for (n = 0; n < HANDLES_P(w).num_panes; n++) {
	    pane = HANDLES_P(w).panes[n];
	    if (HANDLES_P(w).states[n].selected) {

		/*
		 * WARNING: The order below dictates the order of
		 * keyboard traversal among the handles (see NextHandle).
		 */
		static OlDefine		positions[4] = {
			OL_LEFT, OL_TOP, OL_RIGHT, OL_BOTTOM
		};


		for (j = 0; j < XtNumber(positions); j++) {
			XRectangle		h;
			HandleData		d;

			/*
			 * Some positions can't have handles.
			 */
			if (ComputeHandleGeometry(w, pane, positions[j], &h)) {
				d.n = nhandles;
				d.pane_n = n;
				d.position = positions[j];
				d.geometry = h;
				(*op) (w, &d, client_data);
				nhandles++;
			}
		}
	    }
	}

	return;
} /* ForEachHandleByPane */

/**
 ** ComputeHandleGeometry()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
ComputeHandleGeometry (
	Widget			w,
	Widget			pane,
	OlDefine		position,
	XRectangle *		geometry
)
#else
ComputeHandleGeometry (w, pane, position, geometry)
	Widget			w;
	Widget			pane;
	OlDefine		position;
	XRectangle *		geometry;
#endif
{
	Widget			parent  = XtParent(pane);

	XRectangle		g;

	BorderMargins		margins;


	CalculateBorderMargins (w, &margins);

	g.x = CORE_P(pane).x;
	g.y = CORE_P(pane).y;
	g.width = _OlWidgetWidth(pane);
	g.height = _OlWidgetHeight(pane);

	/*
	 * Where the pane is up against an edge of the parent, maybe it
	 * can't have a handle.
	 */
	if (!HANDLES_P(w).resize_at_edges)
		switch (position) {
		case OL_LEFT:
			if (g.x - (int)margins.width <= 0)
				return (False);
			break;
		case OL_RIGHT:
			if (RightEdge(g) + (int)margins.width
						>= (int)CORE_P(parent).width)
				return (False);
			break;
		case OL_TOP:
			if (g.y - (int)margins.height <= 0)
				return (False);
			break;
		case OL_BOTTOM:
			if (BottomEdge(g) + (int)margins.height
						>= (int)CORE_P(parent).height)
				return (False);
			break;
		}

	if (!geometry)
		return (True);

	/*
	 * The HandlesWidget's geometry is the prototype for all
	 * handles, though it may have to be rotated to match each
	 * handle's orientation.
	 */
	geometry->width = _OlWidgetWidth(w);
	geometry->height = _OlWidgetHeight(w);
	switch (position) {
	case OL_LEFT:
	case OL_RIGHT:
		if (HANDLES_P(w).orientation != OL_VERTICAL)
			SWAP (geometry->width, geometry->height);
		break;
	case OL_TOP:
	case OL_BOTTOM:
		if (HANDLES_P(w).orientation != OL_HORIZONTAL)
			SWAP (geometry->width, geometry->height);
		break;
	}

#define NEAR(POS,DIM) \
	geometry->POS = g.POS - margins.DIM
#define FAR(POS,DIM) \
	geometry->POS = g.POS + (g.DIM - geometry->DIM) + margins.DIM
#define CENTER(POS,DIM)	\
	geometry->POS = g.POS + (g.DIM - geometry->DIM) / u2

	switch (position) {
	case OL_LEFT:
		NEAR (x, width);
		CENTER (y, height);
		break;
	case OL_RIGHT:
		FAR (x, width);
		CENTER (y, height);
		break;
	case OL_TOP:
		CENTER (x, width);
		NEAR (y, height);
		break;
	case OL_BOTTOM:
		CENTER (x, width);
		FAR (y, height);
		break;
	}
#undef	NEAR
#undef	CENTER
#undef	FAR

	return (True);
} /* ComputeHandleGeometry */

/**
 ** AddHandle()
 **/

static Handle *
#if	OlNeedFunctionPrototypes
AddHandle (
	Widget			w,
	HandleData *		more
)
#else
AddHandle (w, more)
	Widget			w;
	HandleData *		more;
#endif
{
	Handle *		handle;


	HANDLES_P(w).handles = (Handle *)XtRealloc(
		(char *)HANDLES_P(w).handles,
		(HANDLES_P(w).num_handles + 1) * sizeof(Handle)
	);
	if (more->n < HANDLES_P(w).num_handles)
		OlMemMove (
			Handle,
			&HANDLES_P(w).handles[more->n + 1],
			&HANDLES_P(w).handles[more->n],
			HANDLES_P(w).num_handles - more->n
		);

	handle = &HANDLES_P(w).handles[more->n];
	handle->pane_n = more->pane_n;
	handle->position = more->position;

	if ((handle->window = PopDestroyed(w)) != None)
		ChangeHandle (w, handle);
	else {
		Widget			parent = XtParent(w);
		XSetWindowAttributes	attributes;
		XtValueMask		mask;

		mask = 0;
		SetHandlePixmap (
			w, more->position, NormalLook, &attributes, &mask
		);
		attributes.cursor = OlGetStandardCursor(parent);
		attributes.save_under = True;
		mask |= (CWCursor|CWSaveUnder);

		handle->window = XCreateWindow(
			XtDisplay(parent), XtWindow(parent),
			more->geometry.x, more->geometry.y,
			more->geometry.width, more->geometry.height,
			0,
			CopyFromParent,	/*depth*/
			InputOutput,
			CopyFromParent,	/*visual*/
			mask, &attributes
		);
	}

	HANDLES_P(w).num_handles++;

	return (handle);
} /* AddHandle */

/**
 ** SaveCurrentGrabbed()
 **/

static void
#if	OlNeedFunctionPrototypes
SaveCurrentGrabbed (
	Widget			w
)
#else
SaveCurrentGrabbed (w)
	Widget			w;
#endif
{
	/*
	 * Save these "by value", in preparation for the handle list
	 * being rearranged/reallocated.
	 */
	if (HANDLES_P(w).grabbed)
		_grabbed = *HANDLES_P(w).grabbed;
	if (HANDLES_P(w).current)
		_current = *HANDLES_P(w).current;
	return;
} /* SaveCurrentGrabbed */

/**
 ** RestoreCurrentGrabbed()
 **/

static void
#if	OlNeedFunctionPrototypes
RestoreCurrentGrabbed (
	Widget			w
)
#else
RestoreCurrentGrabbed (w)
	Widget			w;
#endif
{
	if (HANDLES_P(w).grabbed)
		HANDLES_P(w).grabbed = RestoreOne(w, &_grabbed);
	if (HANDLES_P(w).current)
		HANDLES_P(w).current = RestoreOne(w, &_current);

	/*
	 * Restore focus to some handle if we lost the last handle that
	 * had focus.
	 */
	if (!HANDLES_P(w).current && OlHasFocus(w))
		HighlightHandler (w, OL_IN);

	return;
} /* RestoreCurrentGrabbed */

/**
 ** RestoreOne()
 **/

static Handle *
#if	OlNeedFunctionPrototypes
RestoreOne (
	Widget			w,
	Handle *		restore
)
#else
RestoreOne (w, restore)
	Widget			w;
	Handle *		restore;
#endif
{
	Cardinal		n;

	Handle *		handle;


	ForEachHandle (w, n, handle)
		if (restore->pane_n   == handle->pane_n
		 && restore->position == handle->position)
			return (handle);

	return (0);
} /* RestoreOne */

/**
 ** ChangeHandle()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeHandle (
	Widget			w,
	Handle *		handle
)
#else
ChangeHandle (w, handle)
	Widget			w;
	Handle *		handle;
#endif
{
	Display *		display = XtDisplayOfObject(w);

	XSetWindowAttributes	attributes;

	XtValueMask		mask;

	HandleLook		look;


	if (handle == HANDLES_P(w).grabbed)
		look = GrabbedLook;
	else if (handle == HANDLES_P(w).current && OlHasFocus(w))
		look = FocusedLook;
	else
		look = NormalLook;

	mask = 0;
	SetHandlePixmap (w, handle->position, look, &attributes, &mask);
	XChangeWindowAttributes (display, handle->window, mask, &attributes);
	XClearWindow (display, handle->window);

	return;
} /* ChangeHandle */

/**
 ** DestroyHandles()
 **/

static void
#if	OlNeedFunctionPrototypes
DestroyHandles (
	Widget			w
)
#else
DestroyHandles (w)
	Widget			w;
#endif
{
	Cardinal		n;

	Handle *		handle;


	ForEachHandle (w, n, handle)
		DestroyHandle (w, handle);
	CommitDestroyedHandles (w);

	XtFree ((char *)HANDLES_P(w).handles);
	HANDLES_P(w).handles = 0;

	return;
} /* DestroyHandles */

/**
 ** DestroyHandle()
 **/

static void
#if	OlNeedFunctionPrototypes
DestroyHandle (
	Widget			w,
	Handle *		handle
)
#else
DestroyHandle (w, handle)
	Widget			w;
	Handle *		handle;
#endif
{
	InitDestroyedAsNecessary (w);
	PushDestroyed (w, handle->window);
	handle->window = None;
	return;
} /* DestroyHandle */

/**
 ** ReallocateRemainingHandles()
 **/

static void
#if	OlNeedFunctionPrototypes
ReallocateRemainingHandles (
	Widget			w
)
#else
ReallocateRemainingHandles (w)
	Widget			w;
#endif
{
	Cardinal		n;
	Cardinal		j;

	Handle *		handle;


	/*
	 * Our job is to collapse "holes" in the handles list
	 * left by DestroyHandle, and then reallocate the list to
	 * free up unused space.
	 */

	j = 0;
	ForEachHandle (w, n, handle) {
		if (handle->window != None) {
			if (j != n)
				HANDLES_P(w).handles[j] = *handle;
			j++;
		}
	}

	if (HANDLES_P(w).num_handles != j)
		if (!(HANDLES_P(w).num_handles = j)) {
			XtFree ((char *)HANDLES_P(w).handles);
			HANDLES_P(w).handles = 0;
		} else
			HANDLES_P(w).handles = (Handle *)XtRealloc(
				(char *)HANDLES_P(w).handles, j * sizeof(Handle)
			);

	return;
} /* ReallocateRemainingHandles */

/**
 ** CommitDestroyedHandles()
 **/

static void
#if	OlNeedFunctionPrototypes
CommitDestroyedHandles (
	Widget			w
)
#else
CommitDestroyedHandles (w)
	Widget			w;
#endif
{
	Display *		display = XtDisplayOfObject(w);
	Window			window;

	while ((window = PopDestroyed(w)))
		XDestroyWindow (display, window);
	ClearDestroyed (w);

	return;
} /* CommitDestroyedHandles */

/**
 ** TransferHandles()
 **/

static void
#if	OlNeedFunctionPrototypes
TransferHandles (
	Widget			old,
	Widget			new
)
#else
TransferHandles (old, new)
	Widget			old;
	Widget			new;
#endif
{
	Cardinal		n;

	Handle *		handle;

	Widget			previous = 0;


	/*
	 * We have two different lists of panes, the old and the new.
	 * We need to compare the two and remove handles for defunct
	 * panes, add handles for new panes, and transfer the rest.
	 */

	/*
	 * Theoretically, the first step is to transfer the handle list
	 * from the old widget to the new. But we require the caller to do
	 * this (and, in fact, XtSetValues did it "automatically").
	 */
/*	HANDLES_P(new).handles = HANDLES_P(old).handles;		*/
/*	HANDLES_P(new).num_handles = HANDLES_P(old).num_handles;	*/

	/*
	 * The next step is to remember which handle has focus
	 * and which is currently grabbed by the user.
	 */
	SaveCurrentGrabbed (new);

	/*
	 * Destroy handles for panes no longer in the list. Destroy,
	 * also, each handle of a valid pane in the wrong position for
	 * the handle.
	 */
	ForEachHandle (new, n, handle) {
		Widget			pane;
		int			pane_n;

		pane = PaneOfHandle(old, handle);
		if (
			pane == previous
		     || (pane_n = SearchPaneList(new, pane)) == -1
		) {
			DestroyHandle (new, handle);
			previous = pane;
		} else if (!HandleAllowed(new, pane, handle->position))
			DestroyHandle (new, handle);
		else
			handle->pane_n = (Cardinal)pane_n;
	}
	ReallocateRemainingHandles (new);

	/*
	 * Create handles for panes now in the list.
	 */
	ForEachHandleByPane (new, AddHandleIfNewPane, (XtPointer)old);

	RestoreCurrentGrabbed (new);
	CommitDestroyedHandles (new);

	return;
} /* TransferHandles */

/**
 ** AddHandleIfNewPane()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
AddHandleIfNewPane (
	Widget			new,
	HandleData *		more,
	XtPointer		client_data
)
#else
AddHandleIfNewPane (new, handle, more, client_data)
	Widget			new;
	HandleData *		more;
	XtPointer		client_data;
#endif
{
	Widget			old  = (Widget)client_data;
	Widget			pane = HANDLES_P(new).panes[more->pane_n];

	if (XtIsManaged(pane) && SearchPaneList(old, pane) == -1)
		AddHandle (new, more);
	return;
} /* AddHandleIfNewPane */

/**
 ** SearchPaneList()
 **/

static int
#if	OlNeedFunctionPrototypes
SearchPaneList (
	Widget			w,
	Widget			pane
)
#else
SearchPaneList (w, pane)
	Widget			w;
	Widget			pane;
#endif
{
	int			j;

	for (j = 0; j < HANDLES_P(w).num_panes; j++)
		if (pane == HANDLES_P(w).panes[j])
			return (j);
	return (-1);
} /* SearchPaneList */

/**
 ** TransferPanes()
 **/

static void
#if	OlNeedFunctionPrototypes
TransferPanes (
	Widget			old,
	Widget			new
)
#else
TransferPanes (old, new)
	Widget			old;
	Widget			new;
#endif
{
	Cardinal		o;
	Cardinal		n;

	Widget			pane;


	/*
	 * For those panes that are still in the new list, transfer
	 * the pane state to the new list. For those panes that are
	 * no longer in the new list, clear their borders and remove
	 * our callbacks.
	 */
	for (o = 0; o < HANDLES_P(old).num_panes; o++) {
		pane = HANDLES_P(old).panes[o];
		if ((n = SearchPaneList(new, pane)) != -1)
			HANDLES_P(new).states[n] = HANDLES_P(old).states[o];
		else {
			ClearBorder (old, pane, True);
			XtRemoveCallback (
				pane, XtNdestroyCallback,
				RemovePaneCB, (XtPointer)new
			);                       /* yes, new */
		}
	}

	/*
	 * To those panes that are new, add our callbacks and clear the
	 * area where the border will go (thereby generating exposures
	 * that will cause us to draw the borders later).
	 */
	for (n = 0; n < HANDLES_P(new).num_panes; n++) {
		pane = HANDLES_P(new).panes[n];
		if (SearchPaneList(old, pane) == -1) {
			XtAddCallback (
				HANDLES_P(new).panes[n], XtNdestroyCallback,
				RemovePaneCB, (XtPointer)new
			);
			ClearBorder (new, pane, True);
		}
	}

	return;
} /* TransferPanes */

/**
 ** LayoutHandle()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
LayoutHandle (
	Widget			w,
	HandleData *		more,
	XtPointer		client_data	/*NOTUSED*/
)
#else
LayoutHandle (w, more, client_data)
	Widget			w;
	HandleData *		more;
	XtPointer		client_data;
#endif
{
	Display *		display = XtDisplayOfObject(w);

	Handle *		handle;


	/*
	 * We may have a mismatch here, if the handle coming in is
	 * one that hasn't been created yet. (We require that the caller
	 * has already weeded out handles that should no longer exist).
	 * These mismatches can occur if the panes have been rearranged or
	 * reselected since we last were here.
	 *
	 * Because ForEachHandleByPane steps through the handles in the
	 * same order every time, we can say the following:
	 *
	 * - more->n >= num_handles
	 *   ==> this is a new handle to append to the list
	 *
	 * - (more->pane, more->position) != nth (pane, position)
	 *   [Note: we're called with handle pointing to nth handle.]
	 *   ==> this is a new handle to insert before the nth, we
	 *       should see the nth (now (n+1)th) in a later call.
	 */
	handle = more->n < HANDLES_P(w).num_handles?
				&HANDLES_P(w).handles[more->n] : 0;
	if (
		!handle
	     || more->pane_n   != handle->pane_n
	     || more->position != handle->position
	) {
		/*
		 * AddHandle reallocates, so we need to reassign handle.
		 * We'll lay out the old handle (bumped up just now by
		 * AddHandle) in a subsequent visit.
		 */
		handle = AddHandle(w, more);
	}

	if (XtIsManaged(w) && XtIsManaged(PaneOfHandle(w, handle))) {
		Widget			sibling = PaneOfHandle(w, handle);
		XWindowChanges		geometry;

		geometry.x = more->geometry.x;
		geometry.y = more->geometry.y;
		geometry.width = more->geometry.width;
		geometry.height = more->geometry.height;
		if (!XtIsRealized(sibling))
			XtRealizeWidget (sibling);
		geometry.sibling = XtWindowOfObject(sibling);
		geometry.stack_mode = Above;
		XConfigureWindow (
			display, handle->window,
			CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode,
			&geometry
		);
		XMapWindow (display, handle->window);
	} else
		XUnmapWindow (display, handle->window);

	return;
} /* LayoutHandle */

/**
 ** NextHandle()
 **/

static Handle *
#if	OlNeedFunctionPrototypes
NextHandle (
	Widget			w,
	Handle *		handle,
	OlVirtualName		dir
)
#else
NextHandle (w, handle, dir)
	Widget			w;
	Handle *		handle;
	OlVirtualName		dir;
#endif
{
	int			start;
	int			next;	/* int to allow < 0 */

	Handle *		handles = HANDLES_P(w).handles;

	Cardinal		num_handles = HANDLES_P(w).num_handles;
	Cardinal		pane_n;


	if (!handle || !handles)
		return (0);

	/*
	 * The current internal traversal is:
	 *
	 * - The arrow keys move "focus" to the next handle in the
	 *   .handles list; this assumes that the list has some meaningful
	 *   order to it. (Currently the order is, by panes, then top,
	 *   right, bottom, left handles, where present, for each pane.)
	 *
	 * - The modified arrow keys ("multi-keys") move focus to the
	 *   first handle in the next pane. If there is no next pane,
	 *   then it moves focus to the next handle as above.
	 */
	start = handle - handles;
	switch (dir) {
	case OL_MOVELEFT:
	case OL_MOVEUP:
		next = start - 1;
		break;
	case OL_MOVERIGHT:
	case OL_MOVEDOWN:
		next = start + 1;
		break;
	case OL_MULTILEFT:
	case OL_MULTIUP:
		pane_n = handle->pane_n;
		for (next = start - 1; next != start; next--) {
			if (next < 0)
				next = num_handles - 1;
			if (handles[next].pane_n != pane_n) {
				/*
				 * Step back to the "first" handle on the
				 * pane.
				 */
				pane_n = handles[next].pane_n;
				while (
					next > 0
				     && handles[next-1].pane_n == pane_n
				)
					next--;
				break;
			}
		}
		if (next == start)
			next = start - 1;
		break;
	case OL_MULTIRIGHT:
	case OL_MULTIDOWN:
		pane_n = handle->pane_n;
		for (next = start + 1; next != start; next++) {
			if (next >= num_handles)
				next = 0;
			if (handles[next].pane_n != pane_n)
				break;
		}
		if (next == start)
			next = start + 1;
		break;
	}

	if (next < 0)
		next = num_handles - 1;
	else if (next >= num_handles)
		next = 0;

	return (&handles[next]);
} /* NextHandle */

/**
 ** MoveFocus()
 **/

static void
#if	OlNeedFunctionPrototypes
MoveFocus (
	Widget			w,
	Handle *		handle,
	Boolean			change_look
)
#else
MoveFocus (w, handle, change_look)
	Widget			w;
	Handle *		handle;
	Boolean			change_look;
#endif
{
	Handle *		old;


	if (HANDLES_P(w).current == handle)
		return;

	/*
	 * Restore the previous focus-handle to its normal look.
	 */
	if ((old = HANDLES_P(w).current)) {
		HANDLES_P(w).current = 0;
		ChangeHandle (w, old);
	}

	/*
	 * Change the new focus-handle (if there is one) to its
	 * highlighted look.
	 */
	if ((HANDLES_P(w).current = handle) && change_look)
		ChangeHandle (w, handle);

	return;
} /* MoveFocus */

/**
 ** ParentButtonEH()
 **/

static void
#if	OlNeedFunctionPrototypes
ParentButtonEH (
	Widget			parent,
	XtPointer		client_data,
	XEvent *		event,
	Boolean *		continue_to_dispatch
)
#else
ParentButtonEH (parent, client_data, event, continue_to_dispatch)
	Widget			parent;
	XtPointer		client_data;
	XEvent *		event;
	Boolean *		continue_to_dispatch;
#endif
{
	/*
	 * XButtonEvent == XMotionEvent == XCrossingEvent for the fields
	 * that interest us.
	 */
	XButtonEvent *		xe = (XButtonEvent *)event;

	Widget			w = (Widget)client_data;

	Boolean			consumed = False;

	OlVirtualEventRec	ve;

	Handle *		handle   = 0;


	/*
	 * Events on our handle windows percolate up to their parent
	 * window, since we have no event mask on the handles. For these
	 * events, subwindow will be the handle window.
	 */
	if (xe->subwindow != None) {
		Cardinal		n;
		Handle *		h;

		ForEachHandle (w, n, h)
			if (h->window == xe->subwindow) {
				handle = h;
				break;
			}
	}

	OlLookupInputEvent (parent, event, &ve, OL_DEFAULT_IE);
	switch (xe->type) {

	case ButtonPress:
		switch (ve.virtual_name) {
		case OL_SELECT:
			if (handle) {
				/*
				 * Pressing in a handle window moves the
				 * focus there, as well as starts a grab.
				 * Both MoveFocus and GrabHandle can
				 * change the look, so tell MoveFocus not
				 * to bother.
				 */
				MoveFocus (w, handle, False);
				GrabHandle (w, handle, xe->x, xe->y, &xe->time);
			}
			consumed = True;
			break;
		}
		break;
/*
... Here's a hard one: Selecting a pane in one window deselects any panes
... in another window. So says the OPEN LOOK spec.
 */
	case ButtonRelease:
		switch (ve.virtual_name) {
		case OL_SELECT:
			if (HANDLES_P(w).grabbed)
				UngrabHandle (w, True);
			else
				SelectPaneByXY (w, xe->x, xe->y, False);
			consumed = True;
			break;
		case OL_ADJUST:
			if (!handle)
				SelectPaneByXY (w, xe->x, xe->y, True);
			consumed = True;
			break;
		}
		break;

	case MotionNotify:
		if (ve.virtual_name == OL_SELECT && HANDLES_P(w).grabbed) {
			Move (w, xe->x, xe->y);
			consumed = True;
		} else
			CheckCursor (w, xe->x, xe->y, False);
		break;

	case EnterNotify:
		CheckCursor (w, xe->x, xe->y, False);
		break;
	case LeaveNotify:
		CheckCursor (w, xe->x, xe->y, True);
		break;
	}

	*continue_to_dispatch = !consumed;
	return;
} /* ParentButtonEH */

/**
 ** ParentExposeEH()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
ParentExposeEH (
	Widget			parent,		/*NOTUSED*/
	XtPointer		client_data,
	XEvent *		event,
	Boolean *		continue_to_dispatch
)
#else
ParentExposeEH (parent, client_data, event, continue_to_dispatch)
	Widget			parent;
	XtPointer		client_data;
	XEvent *		event;
	Boolean *		continue_to_dispatch;
#endif
{
	Widget			w = (Widget)client_data;


	/*
	 * Compress this series of exposures into a single region.
	 * This is similar to having on the parent a compress_exposure
	 * value of XtExposeCompressSeries.
	 */
	XtAddExposureToRegion (event, HANDLES_P(w).expose_region);
	if (!event->xexpose.count) {
		ParentExpose (w, HANDLES_P(w).expose_region);
		XIntersectRegion (
		    _null_region,
		    HANDLES_P(w).expose_region, HANDLES_P(w).expose_region
		);
	}

	*continue_to_dispatch = True;
	return;
} /* ParentExposeEH */

/**
 ** ParentExpose()
 **/

static void
#if	OlNeedFunctionPrototypes
ParentExpose (
	Widget			w,
	Region			region
)
#else
ParentExpose (w, region)
	Widget			w;
	Region			region;
#endif
{
	Handle *		grabbed = HANDLES_P(w).grabbed;

	XtWidgetGeometry	g;


	/*
	 * Draw the pane borders, and the rubber-banding lines for a
	 * border in the process of being resized.
	 */
	DrawBorders (w, region);
	if (grabbed) {
		Widget			pane = PaneOfHandle(w, grabbed);

		GetPaneGeometry (w, pane, grabbed->position, &g);
		XorRubberLines (w, pane, grabbed->position, &g, region);
	}

	/*
	 * The handles (fixed and floating) take care of themselves,
	 * for they are nothing but borders and background.
	 */
	return;
} /* ParentExpose */

/**
 ** GrabHandle()
 **/

static void
#if	OlNeedFunctionPrototypes
GrabHandle (
	Widget			w,
	Handle *		handle,
	Position		x,
	Position		y,
	Time *			time
)
#else
GrabHandle (w, handle, x, y, time)
	Widget			w;
	Handle *		handle;
	Position		x;
	Position		y;
	Time *			time;
#endif
{
	if (!HANDLES_P(w).grabbed) {
		Widget		parent = XtParent(w);
		Cursor		cursor = OlGetMoveCursor(parent);

		HANDLES_P(w).grabbed = handle;
		StartMove (w, x, y);
		ChangeHandle (w, handle);
		XDefineCursor (XtDisplay(parent), handle->window, cursor);

		/*
		 * If a mouse event caused us to grab a handle, then grab
		 * the pointer so that we can force a different cursor.
		 * We can tell how we got here by looking at the time
		 * pointer (null if not via pointer event).
		 *
		 * MORE: Not sure why owner_events should be False here,
		 * but then I'm not a grab expert.
		 */
		if (time)
			_OlGrabPointer (
			      parent, False, ButtonEventsGrabbedFromParent,
			      GrabModeAsync, GrabModeAsync,
			      None, cursor, *time
			);
	}
	return;
} /* GrabHandle */

/**
 ** UngrabHandle()
 **/

static void
#if	OlNeedFunctionPrototypes
UngrabHandle (
	Widget			w,
	Boolean			resize_pane
)
#else
UngrabHandle (w, resize_pane)
	Widget			w;
	Boolean			resize_pane;
#endif
{
	if (HANDLES_P(w).grabbed) {
		Widget		parent = XtParent(w);
		Handle *	handle;

		EndMove (w, resize_pane);
		/*
		 * The pointer to the grabbed handle, .grabbed, may have
		 * changed in EndMove via an indirect call that changed
		 * the geometry of the panes/handles. In particular, the
		 * pointer may point into reallocated space or may now be
		 * null!
		 */
		handle = HANDLES_P(w).grabbed;
		HANDLES_P(w).grabbed = 0;
		if (handle) {
			ChangeHandle (w, handle);
			XDefineCursor (
				XtDisplay(parent),
				handle->window, OlGetStandardCursor(parent)
			);
		}

		/*
		 * This is safe even if we didn't have the grab.
		 */
		_OlUngrabPointer (parent);
	}
	return;
} /* UngrabHandle */

/**
 ** CheckCursor()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckCursor (
	Widget			w,
	Position		x,
	Position		y,
	Boolean			leaving
)
#else
CheckCursor (w, x, y, leaving)
	Widget			w;
	Position		x;
	Position		y;
	Boolean			leaving;
#endif
{
	Widget			parent = XtParent(w);

	Cursor			cursor = None;


	if (leaving || PaneBorderUnderXY(w, x, y) == -1)
		cursor = OlGetStandardCursor(parent);
	else
		cursor = OlGetTargetCursor(parent);

	if (cursor != HANDLES_P(w).cursor) {
		XDefineCursor (
			XtDisplay(parent), XtWindow(parent), cursor
		);
		HANDLES_P(w).cursor = cursor;
	}

	return;
} /* CheckCursor */

/**
 ** PaneBorderUnderXY()
 **/

static int
#if	OlNeedFunctionPrototypes
PaneBorderUnderXY (
	Widget			w,
	Position		x,
	Position		y
)
#else
PaneBorderUnderXY (w, x, y)
	Widget			w;
	Position		x;
	Position		y;
#endif
{
	Cardinal		num_panes = HANDLES_P(w).num_panes;
	Cardinal		n;

	Widget *		panes     = HANDLES_P(w).panes;

	XRectangle		r;

	BorderMargins		margins;


	CalculateBorderMargins (w, &margins);
	for (n = 0; n < num_panes; n++) {
		/*
		 * If the point lies within the pane itself, no dice.
		 * If the point lies within a rectangle slightly larger
		 * than the pane (larger by the thickness of the border),
		 * we have a match. Making these two checks in this order
		 * ensures we get a match only when the point is right
		 * over the border.
		 */
		r.x = CORE_P(panes[n]).x;
		r.y = CORE_P(panes[n]).y;
		r.width = CORE_P(panes[n]).width;
		r.height = CORE_P(panes[n]).height;
		if (!PointInRect(x, y, r)) {
			r.x -= margins.width;
			r.y -= margins.height;
			r.width += 2 * margins.width;
			r.height += 2 * margins.height;
			if (PointInRect(x, y, r))
				return (n);
		}
	}

	return (-1);
} /* PaneBorderUnderXY */

/**
 ** SelectPaneByXY()
 **/

static void
#if	OlNeedFunctionPrototypes
SelectPaneByXY (
	Widget			w,
	Position		x,
	Position		y,
	Boolean			toggle
)
#else
SelectPaneByXY (w, x, y, toggle)
	Widget			w;
	Position		x;
	Position		y;
	Boolean			toggle;
#endif
{
	int n = PaneBorderUnderXY(w, x, y);
	if (n != -1)
		SelectPane (w, n, toggle);
	return;
} /* SelectPaneByXY */

/**
 ** SelectPane()
 **/

static void
#if	OlNeedFunctionPrototypes
SelectPane (
	Widget			w,
	Cardinal		n,
	Boolean			toggle
)
#else
SelectPane (w, n, toggle)
	Widget			w;
	Cardinal		n;
	Boolean			toggle;
#endif
{
	Cardinal		num_panes = HANDLES_P(w).num_panes;
	Cardinal		j;

	Widget *		panes     = HANDLES_P(w).panes;
	PaneState *		states    = HANDLES_P(w).states;


	if (states[n].selected && !toggle || !XtIsManaged(panes[n]))
		return;

	if (toggle)
		states[n].selected = !states[n].selected;
	else {
		/*
		 * Find any previously selected panes and deselect them.
		 * Instead of trying to redraw the borders right now,
		 * clear them and generate exposures. The ParentExpose
		 * procedure will take care of drawing them correctly.
		 */
		for (j = 0; j < num_panes; j++)
			if (states[j].selected) {
				states[j].selected = False;
				CallCallbacks (w, XtNunselect, (XtPointer)panes[j]);
				ClearBorder (w, panes[j], True);
			}
		states[n].selected = True;
	}

	if (states[n].selected)
		CallCallbacks (w, XtNselect, (XtPointer)panes[n]);
	else
		CallCallbacks (w, XtNunselect, (XtPointer)panes[n]);
	ClearBorder (w, panes[n], True);

	if (XtIsRealized(w))
		OlLayoutHandles (w);

	return;
} /* SelectPane */

/**
 ** CallCallbacks()
 **/

static void
#if	OlNeedFunctionPrototypes
CallCallbacks (
	Widget			w,
	String			callback,
	XtPointer		call_data
)
#else
CallCallbacks (w, callback, call_data)
	Widget			w;
	String			callback;
	XtPointer		call_data;
#endif
{
	if (XtHasCallbacks(w, callback) == XtCallbackHasSome)
		XtCallCallbacks (w, callback, call_data);
	return;
} /* CallCallbacks */

/**
 ** StartMove()
 **/

static void
#if	OlNeedFunctionPrototypes
StartMove (
	Widget			w,
	Position		x,
	Position		y
)
#else
StartMove (w, x, y)
	Widget			w;
	Position		x;
	Position		y;
#endif
{
	Widget			pane
				= PaneOfHandle(w, HANDLES_P(w).grabbed);

	OlDefine		position = HANDLES_P(w).grabbed->position;

	XRectangle		r;
	XtWidgetGeometry	g;


	/*
	 * Get the position of this handle.
	 */
	if (!ComputeHandleGeometry(w, pane, position, &r))
		return; /* huh? */

	/*
	 * The HandlesWidget's geometry corresponds to the "floating
	 * handle" that tracks the mouse. Note that the width and
	 * height are already correct, except they may have to be
	 * swapped.
	 *
	 * We're going to fiddle with the Handles widget's geometry
	 * without telling anyone, especially Xt. If we were to do
	 * the Correct Thing and use XtMakeGeometryRequest to ask if
	 * the gadget's geometry can change, Xt would clear the new
	 * and old areas on XtGeometryYes. This wrecks our XOR drawing
	 * of the floating handle. The Handles widget is just barely a
	 * widget (uh, gadget)--ghost of a widget is more like it.
	 * It draws with XOR, so it never permanently affects what it
	 * draws over (MORE: Is this true? Perhaps we need to grab the
	 * server?!) Moreover, there is no window structure to keep
	 * in sync. Thus I believe we can do what we want with the
	 * geometry values.
	 */
	switch (position) {
	case OL_LEFT:
	case OL_RIGHT:
		if (HANDLES_P(w).orientation != OL_VERTICAL) {
			SWAP (CORE_P(w).width, CORE_P(w).height);
			HANDLES_P(w).orientation = OL_VERTICAL;
		}
		break;		
	case OL_TOP:
	case OL_BOTTOM:
		if (HANDLES_P(w).orientation != OL_HORIZONTAL) {
			SWAP (CORE_P(w).width, CORE_P(w).height);
			HANDLES_P(w).orientation = OL_HORIZONTAL;
		}
		break;
	}
	g.x = CORE_P(w).x = r.x;
	g.y = CORE_P(w).y = r.y;
	g.width = CORE_P(w).width;
	g.height = CORE_P(w).height;

	/*
	 * We know the starting position (upper-left corner) of the
	 * handle grabbed by the user, we know the position of the
	 * "mouse" (ignoring keyboard input for the moment).
	 * The "floating handle" and the rubber-banding lines--and the
	 * edge of the pane--do not necessarily intersect the "mouse"
	 * position. But the difference is fixed, and now is the only
	 * time we know what it should be (the floating handle is
	 * coincident with the fixed handle). Save this offset so
	 * that we can convert mouse position to floating-handle
	 * position, etc., when the mouse moves.
	 *
	 * If we got here due to the keyboard, x/y will be -1/-1 and
	 * we need no offset.
	 */
	if (x == -1 && y == -1)
		HANDLES_P(w).offset = 0;
	else
		switch (position) {
		case OL_LEFT:
		case OL_RIGHT:
			HANDLES_P(w).offset = x - r.x;
			break;
		case OL_TOP:
		case OL_BOTTOM:
			HANDLES_P(w).offset = y - r.y;
			break;
		}

	/*
	 * Draw the first set of rubber-banding lines to lie along the
	 * edge of the floating handle.
	 */
	GetPaneGeometry (w, pane, position, &g);
	XorRubberLines (w, pane, position, &g, (Region )0);

	return;
} /* StartMove */

/**
 ** EndMove()
 **/

static void
#if	OlNeedFunctionPrototypes
EndMove (
	Widget			w,
	Boolean			resize_pane
)
#else
EndMove (w, resize_pane)
	Widget			w;
	Boolean			resize_pane;
#endif
{
	Widget			pane
				= PaneOfHandle(w, HANDLES_P(w).grabbed);

	OlDefine		position = HANDLES_P(w).grabbed->position;

	XtWidgetGeometry	was;
	XtWidgetGeometry	try;
	XtWidgetGeometry	got;


	/*
	 * Construct the pane's geometry from the last position to which
	 * the user moved the floating handle. Erase the rubber-banding
	 * lines.
	 */
	GetPaneGeometry (w, pane, position, &try);
	XorRubberLines (w, pane, position, &try, (Region )0);

	/*
	 * Resize the pane--if allowed. We use XtSetValues here instead
	 * of XtMakeGeometryRequest (which we used when querying the
	 * parent for potential resizes.) XtSetValues will call
	 * XtMakeGeometryRequest for us, and furthermore, will call the
	 * pane's resize procedure--calling XtMakeGeometryRequest alone
	 * would not do that.
	 */
	if (resize_pane) {
		OlHandlesPaneResizeData	cd;

		/*
		 * Clear the old border, and let the ParentExpose
		 * procedure redraw it.
		 */
		ClearBorder (w, pane, True);

		was.x = CORE_P(pane).x;
		was.y = CORE_P(pane).y;
		was.width = CORE_P(pane).width;
		was.height = CORE_P(pane).height;
#if	defined(HANDLES_DEBUG)
		if (handles_debug) {
			printf (
				"Set: (%d,%d;%d,%d) -> (%d,%d;%d,%d)\n",
				was.x, was.y, was.width, was.height,
				try.x, try.y, try.width, try.height
			);
		}
#endif
		XtVaSetValues (
			pane,
			XtNx,      (XtArgVal)try.x,
			XtNy,      (XtArgVal)try.y,
			XtNwidth,  (XtArgVal)try.width,
			XtNheight, (XtArgVal)try.height,
			(String)0
		);

		/*
		 * There is a chance that the resize won't be allowed
		 * (e.g. the window manager refused it--this is possible
		 * since the Layout extension doesn't pass a query to the
		 * window manager but always OK's it).
		 */
		got.x = CORE_P(pane).x;
		got.y = CORE_P(pane).y;
		got.width = CORE_P(pane).width;
		got.height = CORE_P(pane).height;
		if (
			got.x != was.x || got.y != was.y
		     || got.width != was.width || got.height != was.height
		) {
			cd.pane = pane;
			cd.reason = OlHandlesPaneResized;
			cd.geometry = &got;
			CallCallbacks (w, XtNpaneResized, (XtPointer)&cd);
		}

		/*
		 * We don't call OlLayoutHandles at this time, instead we
		 * rely on the parent calling it. We give the parent this
		 * responsibility because it has it in general--if a child
		 * were to ask for a resize on its own we would have no
		 * way of knowing about it unless the parent calls us.
		 * Since the parent can't distinguish between us asking
		 * for the resize and the child asking, it has to call us
		 * unconditionally.
		 */
	}

	return;
} /* EndMove */

/**
 ** Step()
 **/

static void
#if	OlNeedFunctionPrototypes
Step (
	Widget			w,
	Position		step
)
#else
Step (w, step)
	Widget			w;
	Position		step;
#endif
{
	/*
	 * Move will use only one of the coordinates and ignore the
	 * other, so we can safely just add the step to both previous
	 * coordinates.
	 */
	Move (w, CORE_P(w).x + step, CORE_P(w).y + step);
	return;
} /* Step */

/**
 ** Move()
 **/

static void
#if	OlNeedFunctionPrototypes
Move (
	Widget			w,
	Position		x,
	Position		y
)
#else
Move (w, x, y)
	Widget			w;
	Position		x;
	Position		y;
#endif
{
	Widget			pane
				= PaneOfHandle(w, HANDLES_P(w).grabbed);

	OlDefine		position = HANDLES_P(w).grabbed->position;

	XtWidgetGeometry	try;
	XtWidgetGeometry	get;

	BorderMargins		margins;

	OlHandlesPaneResizeData	cd;

	Cardinal		attempt;


	/*
	 * Erase the rubber-banding lines at their previous position.
	 */
	GetPaneGeometry (w, pane, position, &try);
	XorRubberLines (w, pane, position, &try, (Region )0);

	/*
	 * You're not seeing things, I really am changing the Handles
	 * widget's geometry directly. See the note in StartMove.
	 *
	 * Set the tentative position of the floating handle to match
	 * the x/y position passed in, and compute the new pane size
	 * and position.
	 */
	switch (position) {
	case OL_LEFT:
	case OL_RIGHT:
		CORE_P(w).x = x - HANDLES_P(w).offset;
		break;		
	case OL_TOP:
	case OL_BOTTOM:
		CORE_P(w).y = y - HANDLES_P(w).offset;
		break;
	}

	/*
	 * First ask the pane if it likes the new geometry that
	 * corresponds to where the user has moved the floating
	 * handle. Accept whatever size it says, including a change
	 * in the "other" position or width. This allows, for instance,
	 * a text pane that wraps words to adjust its height when
	 * the user makes it narrower. However, if the desired size is
	 * larger than what the user tried to set and the user is dragging
	 * the left or top edge, move the edge back to compensate for
	 * the new size, so that the right/bottom edge doesn't move.
	 *
	 * Second, tell the client that the user is resizing the pane,
	 * and let it adjust the geometry. NOTE: Because the geometry
	 * has not yet been screened by the geometry_manager of the
	 * parent, it may be "wrong". Clients should recognize this
	 * and not take action until they get the OlHandlesPanesResized
	 * reason (see EndMove).
	 *
	 * Finally, ask the parent's geometry_manager if the pane can have
	 * the new geometry. Accept whatever compromise size is returned,
	 * including a change in the "other" position or width, although
	 * adjust the left/top edge as in the query_geometry case. We call
	 * the geometry_manager last because it is the ultimate boss.
	 */

	GetPaneGeometry (w, pane, position, &try);

#if	defined(HANDLES_DEBUG)
	if (handles_debug) {
		printf (
			"Move %s/%s as follows\n",
			XtName(pane), CLASS(XtClass(pane))
		);
	}
#endif
	switch (HANDLES_P(w).query) {
	case OL_BOTH:
	case OL_CHILD:
#if	defined(HANDLES_DEBUG)
		if (handles_debug) {
# define P(SFIELD,FLAG,FIELD) \
			if (try.request_mode & FLAG)			\
				 printf (" %s/%d", SFIELD, try.FIELD)
			printf (" Before asking child: ");
			P("x", CWX, x);
			P("y", CWY, y);
			P("width", CWWidth, width);
			P("height", CWHeight, height);
			printf ("\n");
# undef	P
		}
#endif
		attempt = 1;
QueryAgain:	switch (XtQueryGeometry(pane, &try, &get)) {
		case XtGeometryYes:
			break;
		case XtGeometryAlmost:
			/*
			 * AcceptCompromise may change the position in
			 * the compromise geometry to avoid "pane drift".
			 * Thus we could get into an endless loop if we
			 * don't watch out.
			 */
			if (!AcceptCompromise(position, &try, &get))
				if (++attempt <= 2)
					goto QueryAgain;
			break;
		case XtGeometryNo:
			goto Return;
		}
		break;
	}

#if	defined(HANDLES_DEBUG)
	if (handles_debug) {
# define P(SFIELD,FLAG,FIELD) \
		if (try.request_mode & FLAG)				\
			 printf (" %s/%d", SFIELD, try.FIELD)
		printf (" Before asking client: ");
		P("x", CWX, x);
		P("y", CWY, y);
		P("width", CWWidth, width);
		P("height", CWHeight, height);
		printf ("\n");
# undef	P
	}
#endif
	cd.pane = pane;
	cd.reason = OlHandlesPaneResizing;
	cd.geometry = &try;
	CallCallbacks (w, XtNpaneResized, (XtPointer)&cd);

	switch (HANDLES_P(w).query) {
	case OL_BOTH:
	case OL_PARENT:
#if	defined(HANDLES_DEBUG)
		if (handles_debug) {
# define P(SFIELD,FLAG,FIELD) \
			if (try.request_mode & FLAG)			\
				 printf (" %s/%d", SFIELD, try.FIELD)
			printf (
				" Before asking parent %s/%s: ",
				XtName(XtParent(pane)),
				CLASS(XtClass(XtParent(pane)))
			);
			P("x", CWX, x);
			P("y", CWY, y);
			P("width", CWWidth, width);
			P("height", CWHeight, height);
			printf ("\n");
# undef	P
		}
#endif
		try.request_mode |= XtCWQueryOnly;
		attempt = 1;
RequestAgain:	switch (XtMakeGeometryRequest(pane, &try, &get)) {
		case XtGeometryYes:
			break;
		case XtGeometryAlmost:
			/*
			 * This is only a query, so if the geometry
			 * manager returns Almost we don't need to call it
			 * again, unless we can't accept the compromise.
			 * AcceptCompromise may change the position in
			 * the compromise geometry to avoid "pane drift".
			 * Thus we could get into an endless loop if we
			 * don't watch out.
			 */
			if (!AcceptCompromise(position, &try, &get))
				if (++attempt <= 2)
					goto RequestAgain;
			break;
		case XtGeometryNo:
			goto Return;
		}
		break;
	}
#if	defined(HANDLES_DEBUG)
	if (handles_debug) {
# define P(SFIELD,FLAG,FIELD) \
		if (try.request_mode & FLAG)				\
			 printf (" %s/%d", SFIELD, try.FIELD)
		printf (" Final: ");
		P("x", CWX, x);
		P("y", CWY, y);
		P("width", CWWidth, width);
		P("height", CWHeight, height);
		printf ("\n");
# undef	P
		}
#endif

	/*
	 * Calculate the position of the floating handle from what we've
	 * (possibly) negotiated with the parent and pane. Only one
	 * coordinate changes, the other remains the same since the
	 * floating handle is constrained to move only horizontally
	 * or vertically.
	 */
	CalculateBorderMargins (w, &margins);
	switch (position) {
	case OL_LEFT:
		CORE_P(w).x = try.x - margins.width;
		break;
	case OL_RIGHT:
		CORE_P(w).x = RightEdge(try) + 2*try.border_width
			    + margins.width - _OlWidgetWidth(w);
		break;
	case OL_TOP:
		CORE_P(w).y = try.y - margins.height;
		break;
	case OL_BOTTOM:
		CORE_P(w).y = BottomEdge(try) + 2*try.border_width
			    + margins.height - _OlWidgetHeight(w);
		break;
	}

	/*
	 * Finally, draw the new rubber-banding lines.
	 */
Return:	XorRubberLines (w, pane, position, &try, (Region )0);
	return;
} /* Move */

/**
 ** GetPaneGeometry()
 **/

static void
#if	OlNeedFunctionPrototypes
GetPaneGeometry (
	Widget			w,
	Widget			pane,
	OlDefine		position,
	XtWidgetGeometry *	geometry
)
#else
GetPaneGeometry (w, pane, position, geometry)
	Widget			w;
	Widget			pane;
	OlDefine		position;
	XtWidgetGeometry *	geometry;
#endif
{
	Position		x;
	Position		y;

	Widget			parent = XtParent(pane);

	BorderMargins		margins;


	/*
	 * Calculate the "safety margins"--the amount of space to leave
	 * between the panes and the edges of the parent. These margins
	 * are needed to show the borders we draw around the panes.
	 * These values also give the additional offset of the handles
	 * from the pane.
	 */
	CalculateBorderMargins (w, &margins);

	/*
	 * CORE_P(w) gives the position of the floating handle, which
	 * guides the calculation of the new pane size. The position
	 * is of the upper-left corner, but we need the coordinate
	 * of the (new) pane edge.
	 *
	 * Note: The handles align with the outside edge of the border,
	 * not the pane. After we check the parent-limits we'll
	 * shift x/y to the edge of the pane.
	 */
	x = CORE_P(w).x;
	y = CORE_P(w).y;
	switch (position) {
	case OL_RIGHT:
		x += _OlWidgetWidth(w);
		break;
	case OL_BOTTOM:
		y += _OlWidgetHeight(w);
		break;
	}

	/*
	 * Clip the movement to the parent's window. x/y already include
	 * the margin for the borders, so we can compare them directly
	 * to the parent's window.
	 *
	 * Note: x/y are relative to the parent's coordinate system,
	 * so we can ignore the x/y values of the parent.
	 */
#define PARENT CORE_P(parent)
#define CLIP(POS,DIM) \
	if (POS < (int)PARENT.border_width)				\
		POS = PARENT.border_width;				\
	else if (POS > (int)PARENT.DIM - 2*(int)PARENT.border_width)	\
		POS = PARENT.DIM - 2*PARENT.border_width

	switch (position) {
	case OL_LEFT:
	case OL_RIGHT:
		CLIP (x, width);
		break;
	case OL_TOP:
	case OL_BOTTOM:
		CLIP (y, height);
		break;
	}
#undef	CLIP
#undef	PARENT

	/*
	 * Now adjust the x/y values to bring them to the edge of the
	 * (new) pane geometry.
	 */
	switch (position) {
	case OL_LEFT:
		x += margins.width;
		break;
	case OL_RIGHT:
		x -= margins.width;
		break;
	case OL_TOP:
		y += margins.height;
		break;
	case OL_BOTTOM:
		y -= margins.height;
		break;
	}

	/*
	 * Set all geometry fields to their new values, but only set the
	 * request_mode flags for fields that have changed.
	 *
	 * Note that if the user is dragging the left or top edge of the
	 * pane, the following calculations preserve the position of the
	 * right or bottom edge (see NEAR calculations). This is important
	 * for AcceptCompromise.
	 */

	geometry->request_mode = 0;
	geometry->x = CORE_P(pane).x;
	geometry->y = CORE_P(pane).y;
	geometry->width = CORE_P(pane).width;
	geometry->height = CORE_P(pane).height;
	geometry->border_width = CORE_P(pane).border_width;

#define NEAR(POS,DIM,POS_FLAG,DIM_FLAG) \
	if (POS >= CORE_P(pane).POS + (int)CORE_P(pane).DIM) {		\
		geometry->POS = CORE_P(pane).POS + CORE_P(pane).DIM - 1;\
		geometry->DIM = 1;					\
	} else {							\
		geometry->POS = POS;					\
		geometry->DIM = CORE_P(pane).DIM + CORE_P(pane).POS - POS;\
	}								\
	if (geometry->POS != CORE_P(pane).POS)				\
		geometry->request_mode |= POS_FLAG;			\
	if (geometry->DIM != CORE_P(pane).DIM)				\
		geometry->request_mode |= DIM_FLAG

#define FAR(POS,DIM,DIM_FLAG) \
	if (POS <= CORE_P(pane).POS)					\
		geometry->DIM = 1;					\
	else								\
		geometry->DIM = POS - CORE_P(pane).POS			\
			      - 2 * CORE_P(pane).border_width;		\
	if (geometry->DIM != CORE_P(pane).DIM)				\
		geometry->request_mode |= DIM_FLAG

	switch (position) {
	case OL_LEFT:
		NEAR (x, width, CWX, CWWidth);
		break;
	case OL_RIGHT:
		FAR (x, width, CWWidth);
		break;
	case OL_TOP:
		NEAR (y, height, CWY, CWHeight);
		break;
	case OL_BOTTOM:
		FAR (y, height, CWHeight);
		break;
	}
#undef	NEAR
#undef	FAR

	return;
} /* GetPaneGeometry */

/**
 ** AcceptCompromise()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
AcceptCompromise (
	OlDefine		position,
	XtWidgetGeometry *	try,
	XtWidgetGeometry *	get
)
#else
AcceptCompromise (position, try, get)
	OlDefine		position;
	XtWidgetGeometry *	try;
	XtWidgetGeometry *	get;
#endif
{
	Boolean			accept = True;


#define ACCEPT(FIELD,FLAG) \
	if (get->request_mode & FLAG)					\
		try->FIELD = get->FIELD

	/*
	 * If the user is dragging the left or top edge and the suggested
	 * compromise is larger than the user tried to get, move the
	 * x or y position back to keep the right or bottom edge from
	 * drifting.
	 *
	 * Note: GetPaneGeometry ensured that the right/bottom edge of
	 * the pane didn't move for the OL_LEFT and OL_TOP cases. Thus,
	 * we have the following (considering OL_LEFT case as example):
	 *
	 * Desired new geometry: x, width
	 * Original right edge:  x + width  (thanks to GetPaneGeometry)
	 *
	 * Compromise width:     width'
	 * Shifted x coordinate: x'
	 *
	 * Keep right edge where it is:  x + width = x' + width'
	 *
	 * Shifted x coordinate:         x' = x + width - width'
	 *                               x' = x - (width' - width)
	 *
	 * (The client has a chance to mess this up in the XtNpaneResized
	 * callback, by changing x/y and width/height such that the right
	 * edge moves. C'est la vie.)
	 */
	switch (position) {
	case OL_LEFT:
		if (get->request_mode & CWX)
			if (get->width > try->width) {
				try->x -= get->width - try->width;
				accept = False;
			} else
				try->x = get->x;
		ACCEPT (y, CWY);
		break;
	case OL_TOP:
		if (get->request_mode & CWY)
			if (get->height > try->height) {
				try->y -= get->height - try->height;
				accept = False;
			} else
				try->y = get->y;
		ACCEPT (x, CWX);
		break;
	case OL_BOTTOM:
	case OL_RIGHT:
		ACCEPT (x, CWX);
		ACCEPT (y, CWY);
		break;
	}

	ACCEPT (width, CWWidth);
	ACCEPT (height, CWHeight);

#undef	ACCEPT
	return (accept);
} /* AcceptCompromise */

/**
 ** XorRubberLines()
 **/

static void
#if	OlNeedFunctionPrototypes
XorRubberLines (
	Widget			w,
	Widget			pane,
	OlDefine		position,
	XtWidgetGeometry *	geometry,
	Region			region
)
#else
XorRubberLines (w, pane, position, geometry, region)
	Widget			w;
	Widget			pane;
	OlDefine		position;
	XtWidgetGeometry *	geometry;
	Region			region;
#endif
{
	Widget			parent	= XtParent(w);

	Display *		display = XtDisplay(parent);

	Window			window  = XtWindow(parent);

	XRectangle		r[5];
	XRectangle		pane_r[5];

	Cardinal		n;

	Boolean			same_pane   = False;
	Boolean			same_parent = False;
	Boolean			same_region = False;

	static Region		scratch = 0;
	static Region		remove  = 0;
	static Region		clipset = 0;
	static Region		last_region = 0;

	static XRectangle	last_pane   = { 0, 0, 0, 0 };
	static XRectangle	last_parent = { 0, 0, 0, 0 };


	/*
	 * When the user starts resizing a pane, we'll come here quite
	 * often with the same pane and same region (typically null).
	 * Thus we will benefit from knowing when to avoid redundant
	 * computations.
	 */
	if (SameGeometry(last_pane, CORE_P(pane)))
		same_pane = True;
	else {
		last_pane.x = CORE_P(pane).x;
		last_pane.y = CORE_P(pane).y;
		last_pane.width = CORE_P(pane).width;
		last_pane.height = CORE_P(pane).height;
	}
	if (SameGeometry(last_parent, CORE_P(parent)))
		same_parent = True;
	else {
		last_parent.x = CORE_P(parent).x;
		last_parent.y = CORE_P(parent).y;
		last_parent.width = CORE_P(parent).width;
		last_parent.height = CORE_P(parent).height;
	}
	if (!region && !last_region)
		same_region = True;
	else
		last_region = region;

	/*
	 * Generate the rectangles that bound a selected border around
	 * the new pane, plus a floating handle; these define the rubber-
	 * banding lines. Generate a similar set of rectangles for the
	 * pane at its current size; these define a clip set where we need
	 * to *exlude* any attempt to draw, to avoid XOR-ing the existing
	 * border.
	 */
	CalculateBorderRectangles (
		w,
		geometry->x, geometry->y,
		geometry->width, geometry->height,
		geometry->border_width,
		position, True, r
	);
	if (!same_pane)
		CalculateBorderRectangles (
			w,
			CORE_P(pane).x, CORE_P(pane).y,
			CORE_P(pane).width, CORE_P(pane).height,
			CORE_P(pane).border_width,
			position, True, pane_r
		);

	/*
	 * Subtract the existing border's rectangles from the region
	 * at hand (or from the entire parent's window rectangle if
	 * no region was passed), to arrive at the resulting clip-set
	 * used to constrict the drawing.
	 */
	if (!scratch) {
		scratch = XCreateRegion();
		remove = XCreateRegion();
		clipset = XCreateRegion();
	}
	if (!region)
		if (same_parent)
			region = scratch;
		else {
			XRectangle		parent_r;

			parent_r.x = 0;
			parent_r.y = 0;
			parent_r.width = CORE_P(parent).width;
			parent_r.height = CORE_P(parent).height;
			XUnionRectWithRegion (&parent_r, _null_region, scratch);
			region = scratch;
		}
	if (!same_pane) {
		XUnionRectWithRegion (&pane_r[0], _null_region, remove);
		for (n = 1; n < 5; n++)
			XUnionRectWithRegion (&pane_r[n], remove, remove);
	}
	if (!same_pane || !same_parent || !same_region)
		XSubtractRegion (region, remove, clipset);

	/*
	 * We're sharing this GC with other modules, so we have to leave
	 * it as we find it, with a clip_mask of None.
	 */
	XSetRegion (display, HANDLES_P(w).invert_gc, clipset);
	XFillRectangles (display, window, HANDLES_P(w).invert_gc, r, 5);
	XSetClipMask (display, HANDLES_P(w).invert_gc, None);

	return;
} /* XorRubberLines */

/**
 ** DrawBorders()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawBorders (
	Widget			w,
	Region			region
)
#else
DrawBorders (w, region)
	Widget			w;
	Region			region;
#endif
{
	Cardinal		num_panes = HANDLES_P(w).num_panes;
	Cardinal		n;

	Widget *		panes     = HANDLES_P(w).panes;
	PaneState *		states    = HANDLES_P(w).states;


	for (n = 0; n < num_panes; n++) {
		if (XtIsManaged(panes[n]))
			DrawBorder (
				w, panes[n], states[n].selected, region
			);
	}

	return;
} /* DrawBorders */

/**
 ** DrawBorder()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawBorder (
	Widget			w,
	Widget			pane,
	Boolean			selected,
	Region			region
)
#else
DrawBorder (w, pane, selected, region)
	Widget			w;
	Widget			pane;
	Boolean			selected;
	Region			region;
#endif
{
	Widget			parent  = XtParent(w);

	Display *		display = XtDisplay(parent);

	Screen *		screen  = XtScreen(parent);

	Window			window  = XtWindow(parent);

	int			bw2     = 2 * CORE_P(pane).border_width;

	XRectangle		r;
	XRectangle		clip;


	/*
	 * DrawBorders can be called often, especially when multiple
	 * exposures occur. Thus, before trying to draw anything,
	 * we make two simple checks against the region:
	 * - does it intersect the rectangle made up of the outer
	 *   boundaries of the border?
	 * - does it extend outside the pane?
	 *
	 * MORE: Moving the handles will cause small exposures over
	 * the borders, but these cause us to redraw the entire border.
	 */
	CalculateBorderBounds (w, pane, &r);
	if (!RectInRegion(region, r))
		return;
	XClipBox (region, &clip);
	if (
		clip.x >= CORE_P(pane).x
	     && RightEdge(clip) <= RightEdge(CORE_P(pane)) + bw2
	     && clip.y >= CORE_P(pane).y
	     && BottomEdge(clip) <= BottomEdge(CORE_P(pane)) + bw2
	)
		return;

	if (selected || !OlgIs3d()) {
		XRectangle		R[4];

		CalculateBorderRectangles (
			w,
			CORE_P(pane).x, CORE_P(pane).y,
			CORE_P(pane).width, CORE_P(pane).height,
			CORE_P(pane).border_width,
			OL_NONE, selected, R
		);
		XFillRectangles (
			display, window,
			OlgGetDarkGC(HANDLES_P(w).attrs),
			R, 4
		);
	} else
		OlgDrawBorderShadow (
			screen, window,
			HANDLES_P(w).attrs,
			HANDLES_P(w).shadow_type,
			HANDLES_P(w).shadow_thickness,
			r.x, r.y, r.width, r.height
		);

	return;
} /* DrawBorder */

/**
 ** ClearBorder()
 **/

static void
#if	OlNeedFunctionPrototypes
ClearBorder (
	Widget			w,
	Widget			pane,
	Boolean			exposures
)
#else
ClearBorder (w, pane, exposures)
	Widget			w;
	Widget			pane;
	Boolean			exposures;
#endif
{
	Widget			parent    = XtParent(w);

	Display *		display   = XtDisplay(parent);

	Window			window    = XtWindow(parent);

	XRectangle		r[4];

	Cardinal		n;


	/*
	 * It is OK to get this far without being realized, as we want to
	 * allow OlHandlesSetSelection to actually set the selection.
	 * [Follow the flow of control to see this is so!]
	 */
	if (!window)
		return;

	/*
	 * For widget panes:
	 * Instead of generating the four rectangles that define the
	 * border and clearing each separately, just clear the area
	 * that bounds the border. This will "clear" a larger area,
	 * but the server will clip it with the obscuring pane.
	 * Furthermore, this single operation will generate a single
	 * exposure.
	 *
	 * For gadget panes:
	 * We have to clip "manually" and resign ourselves to four
	 * exposures.
	 */
	if (XtIsWidget(pane)) {
		CalculateBorderBounds (w, pane, &r[0]);
		n = 1;
	} else {
		CalculateBorderRectangles (
			w,
			CORE_P(pane).x, CORE_P(pane).y,
			CORE_P(pane).width, CORE_P(pane).height,
			CORE_P(pane).border_width,
			OL_NONE, True, r
		);
		n = 4;
	}
	while (n--)
		XClearArea (
			display, window,
			r[n].x, r[n].y, r[n].width, r[n].height,
			exposures
		);

	return;
} /* ClearBorder */

/**
 ** CalculateBorderBounds()
 **/

static void
#if	OlNeedFunctionPrototypes
CalculateBorderBounds (
	Widget			w,
	Widget			pane,
	XRectangle *		r
)
#else
CalculateBorderBounds (w, pane, r)
	Widget			w;
	Widget			pane;
	XRectangle *		r;
#endif
{
	Dimension		bw2       = 2 * CORE_P(pane).border_width;
	BorderMargins		margins;

	CalculateBorderMargins (w, &margins);
	r->x = CORE_P(pane).x - margins.width;
	r->y = CORE_P(pane).y - margins.height;
	r->width = CORE_P(pane).width + bw2 + 2 * margins.width;
	r->height = CORE_P(pane).height + bw2 + 2 * margins.height;
	return;
} /* CalculateBorderBounds */

/**
 ** CalculateBorderRectangles()
 **/

static void
#if	OlNeedFunctionPrototypes
CalculateBorderRectangles (
	Widget			w,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	Dimension		border_width,
	OlDefine		position,
	Boolean			selected,
	XRectangle *		r
)
#else
CalculateBorderRectangles (w, x, y, width, height, border_width, position, selected, r)
	Widget			w;
	Position		x;
	Position		y;
	Dimension		width;
	Dimension		height;
	Dimension		border_width;
	OlDefine		position;
	Boolean			selected;
	XRectangle *		r;
#endif
{
	Dimension		bw2 = 2 * border_width;
	Dimension		vchisel;
	Dimension		hchisel;


	vchisel = OlgGetHorizontalStroke(HANDLES_P(w).attrs);
	hchisel = OlgGetVerticalStroke(HANDLES_P(w).attrs);
	if (selected) {
		vchisel *= 2;
		hchisel *= 2;
	}

	/*
	 * At least one caller requires that the rectangles be computed
	 * in this order: top, right, bottom, left.
	 */

	/* TOP */
	r[0].x = x - vchisel;
	r[0].y = y - hchisel;
	r[0].width = width + bw2 + 2 * vchisel;
	r[0].height = hchisel;

	/* RIGHT */
	r[1].x = x + width + bw2;
	r[1].y = y;
	r[1].width = vchisel;
	r[1].height = height + bw2;

	/* BOTTOM */
	r[2].x = r[0].x;
	r[2].y = y + height + bw2;
	r[2].width = r[0].width;
	r[2].height = r[0].height;

	/* LEFT */
	r[3].x = x - vchisel;
	r[3].y = r[1].y;
	r[3].width = r[1].width;
	r[3].height = r[1].height;

	/*
	 * Add in a fifth rectangle for the handle on the side identified,
	 * if identified. This rectangle must be disjoint from the other
	 * rectangles, so we will clip this one accordingly (i.e. it won't
	 * be the full size of a handle).
	 */
	if (position != OL_NONE) {
		r[4].width = CORE_P(w).width;
		r[4].height = CORE_P(w).height;
		switch (position) {
		case OL_LEFT:
			r[4].x = r[3].x + r[3].width;
			r[4].y = r[3].y + (r[3].height - r[4].height) / u2;
			r[4].width -= r[3].width;
			break;
		case OL_RIGHT:
			r[4].x = r[1].x + r[1].width - r[4].width;
			r[4].y = r[1].y + (r[1].height - r[4].height) / u2;
			r[4].width -= r[1].width;
			break;
		case OL_TOP:
			r[4].x = r[0].x + (r[0].width - r[4].width) / u2;
			r[4].y = r[0].y + r[0].height;
			r[4].height -= r[0].height;
			break;
		case OL_BOTTOM:
			r[4].x = r[2].x + (r[2].width - r[4].width) / u2;
			r[4].y = r[2].y + r[2].height - r[4].height;
			r[4].height -= r[2].height;
			break;
		}
	}

	return;
} /* CalculateBorderRectangles */

/**
 ** CalculateBorderMargins()
 **/

static void
#if	OlNeedFunctionPrototypes
CalculateBorderMargins (
	Widget			w,
	BorderMargins *		margins
)
#else
CalculateBorderMargins (w, margins)
	Widget			w;
	BorderMargins *		margins;
#endif
{
	/*
	 * These margins give the maximum space around each pane for the
	 * borders--this is the space where a button event can fall and
	 * be considered related to the corresponding pane, for instance.
	 */
	margins->width = 2*OlgGetHorizontalStroke(HANDLES_P(w).attrs);
	margins->height = 2*OlgGetVerticalStroke(HANDLES_P(w).attrs);
	return;
} /* CalculateBorderMargins */

/**
 ** ColorHandles()
 **/

static void
#if	OlNeedFunctionPrototypes
ColorHandles (
	Widget			w
)
#else
ColorHandles (w)
	Widget			w;
#endif
{
	Cardinal		n;
	Handle *		handle;

	if (XtIsRealized(w)) {
		FreeCrayons (w);
		GetCrayons (w);
		ForEachHandle (w, n, handle)
			ChangeHandle (w, handle);
	}
	return;
} /* ColorHandles */

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
	Widget			parent  = XtParent(w);

	Display *		display = XtDisplay(parent);

	Screen *		screen  = XtScreen(parent);

	Window			window  = XtWindow(parent);

	Dimension		width   = _OlWidgetWidth(w);
	Dimension		height  = _OlWidgetHeight(w);

	unsigned int		depth   = CORE_P(parent).depth;

	Pixel			border_pixel;
	Pixel			foreground;
	Pixel			background_pixel;
	Pixel			input_focus_color;

	Pixmap			background_pixmap;

	XGCValues		v;

	OlgAttrs *		attrs;

	GC			bg1;
	GC			focus;


	XtVaGetValues (
		parent,
		XtNborderColor,      (XtArgVal)&border_pixel,
/*		XtNforeground,       (XtArgVal)&foreground,		*/
		XtNbackground,       (XtArgVal)&background_pixel,
		XtNbackgroundPixmap, (XtArgVal)&background_pixmap,
		XtNinputFocusColor,  (XtArgVal)&input_focus_color,
		(String)0
	);
	foreground = border_pixel;	/* Fix Manager!! */

	if (!(attrs = HANDLES_P(w).attrs)) {
		_GetSomeCrayons (
			w, foreground, background_pixel, background_pixmap
		);
		attrs = HANDLES_P(w).attrs;
	}

	/*
	 * A fairly standard technique for drawing rubber-bands is to
	 * invert those bit-planes of a pixel that differ between the
	 * background and the desired color of the rubber-bands.
	 *
	 * We need to apply a varying clip_mask, but I hate to create
	 * a private GC. Instead, we'll ask for a clip_mask of None,
	 * to get a GC that we can share with other modules that
	 * also want a clip_mask of None. Just before using the GC,
	 * we'll set the clip_mask as needed, then reset it to None
	 * right after using it.
	 */
	v.function = GXinvert;
	v.plane_mask = border_pixel ^ background_pixel;
	v.subwindow_mode = IncludeInferiors;
	v.clip_mask = None;
	HANDLES_P(w).invert_gc = XtGetGC(
		w, GCPlaneMask|GCFunction|GCSubwindowMode|GCClipMask , &v
	);

	bg1 = OlgGetBg1GC(attrs);
	focus = OlgGetScratchGC(attrs);
	XSetFillStyle (display, focus, FillSolid);
	XSetForeground (display, focus, input_focus_color);

	/*
	 * The HandlesWidget provides the prototype geometry of
	 * all other handles, except the orientation may be wrong.
	 */
	if (HANDLES_P(w).orientation != OL_HORIZONTAL)
		SWAP (width, height);

#define CREATE(PIXMAP,GC,WIDTH,HEIGHT,DEPRESSED) \
	PIXMAP = XCreatePixmap(display, window, WIDTH, HEIGHT, depth);   \
	XFillRectangle(display, PIXMAP, GC, 0, 0, WIDTH, HEIGHT);\
	OlgDrawBox (screen, PIXMAP, attrs, 0, 0, WIDTH, HEIGHT, DEPRESSED)

	CREATE (HANDLES_P(w).tb_handle, bg1, width, height, False);
	CREATE (HANDLES_P(w).lr_handle, bg1, height, width, False);
	CREATE (HANDLES_P(w).tb_handle_grabbed, bg1, width, height, True);
	CREATE (HANDLES_P(w).lr_handle_grabbed, bg1, height, width, True);
	CREATE (HANDLES_P(w).tb_handle_focused, focus, width, height, False);
	CREATE (HANDLES_P(w).lr_handle_focused, focus, height, width, False);
#undef	CREATE

	return;
} /* GetCrayons */

/**
 ** GetSomeCrayons()
 **/

static void
#if	OlNeedFunctionPrototypes
GetSomeCrayons (
	Widget			w
)
#else
GetSomeCrayons (w)
	Widget			w;
#endif
{
	Pixel			foreground;
	Pixel			background_pixel;

	Pixmap			background_pixmap;


	XtVaGetValues (
		XtParent(w),
/* Fix Manager!	XtNforeground,       (XtArgVal)&foreground,		*/
		XtNborderColor,      (XtArgVal)&foreground,
		XtNbackground,       (XtArgVal)&background_pixel,
		XtNbackgroundPixmap, (XtArgVal)&background_pixmap,
		(String)0
	);

	_GetSomeCrayons (w, foreground, background_pixel, background_pixmap);

	return;
} /* GetSomeCrayons */

/**
 ** _GetSomeCrayons()
 **/

static void
#if	OlNeedFunctionPrototypes
_GetSomeCrayons (
	Widget			w,
	Pixel			foreground,
	Pixel			background_pixel,
	Pixmap			background_pixmap
)
#else
_GetSomeCrayons (w, foreground, background_pixel, background_pixmap)
	Widget			w;
	Pixel			foreground;
	Pixel			background_pixel;
	Pixmap			background_pixmap;
#endif
{
	Screen *		screen = XtScreenOfObject(w);

	if (background_pixmap != XtUnspecifiedPixmap)
		HANDLES_P(w).attrs = OlgCreateAttrs(
			screen, foreground, (OlgBG *)&background_pixmap, True,
			OL_DEFAULT_POINT_SIZE
		);
	else		
		HANDLES_P(w).attrs = OlgCreateAttrs(
			screen, foreground, (OlgBG *)&background_pixel, False,
			OL_DEFAULT_POINT_SIZE
		);
	return;
} /* _GetSomeCrayons */

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
	Display *		display = XtDisplayOfObject(w);


	if (HANDLES_P(w).invert_gc) {
		XtReleaseGC (w, HANDLES_P(w).invert_gc);
		HANDLES_P(w).invert_gc = 0;
	}
	if (HANDLES_P(w).attrs) {
		OlgDestroyAttrs (HANDLES_P(w).attrs);
		HANDLES_P(w).attrs = 0;
	}

#define FREEP(PIXMAP) \
	if (PIXMAP) {							\
		XFreePixmap (display, PIXMAP);				\
		PIXMAP = 0;						\
	} else

	FREEP (HANDLES_P(w).tb_handle)
	FREEP (HANDLES_P(w).lr_handle)
	FREEP (HANDLES_P(w).tb_handle_grabbed)
	FREEP (HANDLES_P(w).lr_handle_grabbed)
	FREEP (HANDLES_P(w).tb_handle_focused)
	FREEP (HANDLES_P(w).lr_handle_focused)
#undef	FREEP

	return;
} /* FreeCrayons */

/**
 ** SetHandlePixmap()
 **/

static void
#if	OlNeedFunctionPrototypes
SetHandlePixmap (
	Widget			w,
	OlDefine		position,
	HandleLook		look,
	XSetWindowAttributes *	attributes,
	XtValueMask *		mask
)
#else
SetHandlePixmap (w, position, look, attributes, mask)
	Widget			w;
	OlDefine		position;
	HandleLook		look;
	XSetWindowAttributes *	attributes;
	XtValueMask *		mask;
#endif
{
	switch (position) {
	case OL_LEFT:
	case OL_RIGHT:
		switch (look) {
		case NormalLook:
			attributes->background_pixmap
					= HANDLES_P(w).lr_handle;
			break;
		case GrabbedLook:
			attributes->background_pixmap
					= HANDLES_P(w).lr_handle_grabbed;
			break;
		case FocusedLook:
			attributes->background_pixmap
					= HANDLES_P(w).lr_handle_focused;
			break;
		}
		break;
	case OL_TOP:
	case OL_BOTTOM:
		switch (look) {
		case NormalLook:
			attributes->background_pixmap
					= HANDLES_P(w).tb_handle;
			break;
		case GrabbedLook:
			attributes->background_pixmap
					= HANDLES_P(w).tb_handle_grabbed;
			break;
		case FocusedLook:
			attributes->background_pixmap
					= HANDLES_P(w).tb_handle_focused;
			break;
		}
		break;
	}

	*mask |= CWBackPixmap;

	return;
} /* SetHandlePixmap */
