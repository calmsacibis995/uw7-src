#ifndef NOIDENT
#ident	"@(#)category:Category.c	2.16"
#endif

#include "string.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/CategoryP.h"
#include "Xol/AbbrevButt.h"
#include "Xol/PopupMenu.h"
#include "Xol/FButtons.h"
#include "Xol/ControlAre.h"
#include "Xol/Footer.h"
#include "Xol/ChangeBar.h"

#define ClassName Category
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&categoryClassRec)
#define superClass	((WidgetClass)&managerClassRec)
#define className	"Category"

/*
 * Private types:
 */

typedef struct PositionPair {
	Position		x;
	Position		y;
}			PositionPair;

typedef struct Geometry {
	Position		x;
	Position		y;
	Position		y_baseline;
	Dimension		width;
	Dimension		height;
}			Geometry;

/*
 * Macros:
 */

#if	defined(AVAILABLE_WHEN_UNMANAGED)

static Boolean
#if	OlNeedFunctionPrototypes
IsManaged ( Widget w )
#else
IsManaged (w) Widget w;
#endif
{ return (XtIsManaged(w)); }

# if	defined(XtIsManaged)
#  undef	XtIsManaged
# endif
# define XtIsManaged(W) \
	(IsManaged(W) || CATEGORY_CP(W).available_when_unmanaged)

#endif

#define ContrastBackground(W) \
		OlContrastingColor(W, CORE_P(W).background_pixel, 25)

	/*
	 * The private children of this composite are inserted up
	 * front in the .children list, in the following order.
	 */
#define _AMB	0	/* abbreviated menu button     */
#define _LCA	1	/* lower control area          */
#define _FTR	2	/* footer                      */
#define _1ST	3	/* index of first client child */

#define HaveFooter(W) \
	(   COMPOSITE_P(W).num_children > _FTR				\
	 && XtIsManaged(COMPOSITE_P(W).children[_FTR])  )

#define Shadow(W) MANAGER_P(W).shadow_thickness

#define WidthFromPoints(P) (int)OlScreenPointToPixel(OL_HORIZONTAL,(P),screen)
#define HeightFromPoints(P) (int)OlScreenPointToPixel(OL_VERTICAL,(P),screen)

#define ComputeChangeBarPosition(W,P) \
	ComputeTopSize((W), (XtWidgetGeometry *)0, (P), (Geometry *)0, (PositionPair *)0, (Geometry *)0)
#define ComputeCategoryLabelSize(W,P) \
	ComputeTopSize((W), (XtWidgetGeometry *)0, (Geometry *)0, (P), (PositionPair *)0, (Geometry *)0)
#define ComputePageLabelSize(W,P) \
	ComputeTopSize((W), (XtWidgetGeometry *)0, (Geometry *)0, (Geometry *)0, (PositionPair *)0, (P))

#define FOR_EACH_PAGE(W,CHILD,N) \
	for (N = _1ST; N < COMPOSITE_P(W).num_children			\
		       && (CHILD = COMPOSITE_P(W).children[N]); N++)

#define FOR_EACH_MANAGED_PAGE(W,CHILD,N) \
	FOR_EACH_PAGE (W, CHILD, N)					\
		if (XtIsManaged(CHILD))

#define ISFLAG(W,F)	(CATEGORY_P(W).flags & (F))
#define SETFLAG(W,F)	CATEGORY_P(W).flags |= (F)
#define CLRFLAG(W,F)	CATEGORY_P(W).flags &= ~(F)

#define STREQU(A,B)	(strcmp((A),(B)) == 0)

	/*
	 * Some typed numbers to keep our pal, the ANSI-C compiler, happy.
	 */
#if	defined(__STDC__)
# define u0	0U
# define u2	2U
#else
# define u0	0
# define u2	2
#endif

/*
 * Global data:
 */

	/*
	 * These are here to support backwards compatibility.
	 */
char		XtNshowFooter             [] = "showFooter";
char		XtCShowFooter             [] = "ShowFooter";
#if	defined(AVAILABLE_WHEN_UNMANAGED)
char		XtNavailableWhenUnmanaged [] = "availableWhenUnmanaged";
char		XtCAvailableWhenUnmanaged [] = "AvailableWhenUnmanaged";
#endif

/*
 * Local routines:
 */

static void		ClassInitialize OL_ARGS((
	void
));
static void		Initialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		Destroy OL_ARGS((
	Widget			w
));
static void		ExposeProc OL_ARGS((
	Widget			w,
	XEvent *		pe,		/*NOTUSED*/
	Region			region
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
static void		InsertChild OL_ARGS((
	Widget			w
));
static void		ConstraintInitialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		ConstraintDestroy OL_ARGS((
	Widget			w
));
static Boolean		ConstraintSetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		Layout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint, /*NOTUSED*/
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static Cardinal		InsertPosition OL_ARGS((
	Widget			w
));
static void		SetWindowGravity OL_ARGS((
	Widget			w
));
static void		FixMenuWidth OL_ARGS((
	Widget			w
));
static Boolean		NeedChangeBar OL_ARGS((
	Widget			w,
	Cardinal		current_page
));
static void		CheckCancelDismiss OL_ARGS((
	Widget			w
));
static void		DrawText OL_ARGS((
	Widget			w,
	Region			region,
	GC			gc,
	Geometry *		g,
	String			text
));
static void		DrawBorder OL_ARGS((
	Widget			w,
	Region			region,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height
));
static void		ClearChangeBar OL_ARGS((
	Widget			w
));
static void		ClearCategoryLabel OL_ARGS((
	Widget			w
));
static void		ClearPageLabel OL_ARGS((
	Widget			w
));
static void		ComputeTopSize OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	p_top,
	Geometry *		p_change_bar,
	Geometry *		p_category_label,
	PositionPair *		p_abbrev,
	Geometry *		p_page_label
));
static void		ComputeTextSize OL_ARGS((
	XFontStruct *		f,
	OlFontList *		font_list,
	String			text,
	Dimension *		p_width,
	Dimension *		p_ascent,
	Dimension *		p_descent
));
static Boolean		InRegion OL_ARGS((
	Region			region,
	Geometry *		g
));
static void		SetPageCB OL_ARGS((
	Widget			w,		/*NOTUSED*/
	XtPointer		client_data,
	XtPointer		call_data
));
static void		NextPageCB OL_ARGS((
	Widget			w,		/*NOTUSED*/
	XtPointer		client_data,
	XtPointer		call_data	/*NOTUSED*/
));
static void		SetPage OL_ARGS((
	Widget			w,
	Cardinal		new_page,
	Boolean			call_callback
));
static int		FindChild OL_ARGS((
	Widget			w,
	Widget			child
));
static void		GetGCs OL_ARGS((
	Widget			w
));
static void		FreeGCs OL_ARGS((
	Widget			w
));
static void		GetValue OL_ARGS((
	Widget			w,
	String			name,
	XtArgVal		value
));
static void		SetValue OL_ARGS((
	Widget			w,
	String			name,
	XtArgVal		value
));
static Boolean		IsInternalChild OL_ARGS((
	Widget			w,
	Widget			child
));

/*
 * Local data:
 */

static String		MenuFields[]	= {
	XtNlabel,
	XtNset,
	XtNuserData
};

static MenuItem		FakeMenuItems[]	= {
	{ (XtArgVal)"(no active pages)" }
};

static char		internal;

/*
 * Instance resource list:
 */

	/*
	 * An chiseled line is the OPEN LOOK default shadow type, while
	 * a raised edge is the Motif type. Both have the same default
	 * thickness.
	 */
static OlDefine		shadow_type = OL_SHADOW_ETCHED_IN;

#define FONT \
    {	/* SGI */							\
	XtNfont, XtCFont,						\
	XtRFontStruct, sizeof(XFontStruct *), offset(category.font),	\
	XtRString, OlDefaultFont					\
    }
#define CATEGORY_FONT \
    {	/* SGI */							\
	XtNcategoryFont, XtCFont,					\
	XtRFontStruct, sizeof(XFontStruct *), offset(category.category_font),\
	XtRString, OlDefaultBoldFont					\
    }
#define FONT_COLOR \
    {	/* SGI */							\
	XtNfontColor, XtCFontColor,					\
	XtRPixel, sizeof(Pixel), offset(category.font_color),		\
	XtRString, (XtPointer)XtDefaultForeground			\
    }

static _OlDynResource	dyn_resources[] = {
#define offset(F) XtOffsetOf(CategoryRec, F)
#define DYNFLAG   offset(category.dynamics)

  {	FONT,
	DYNFLAG, _CATEGORY_B_DYNAMIC_FONT, NULL
  },
  {	CATEGORY_FONT,
	DYNFLAG, _CATEGORY_B_DYNAMIC_CATEGORY_FONT, NULL
  },
  {	FONT_COLOR,
	DYNFLAG, _CATEGORY_B_DYNAMIC_FONTCOLOR, NULL
  },

#undef	offset
#undef	DYNFLAG
};

static XtResource resources[] = {
#define offset(F) XtOffsetOf(CategoryRec, F)

    /*
     * This resource is from Composite, but we wish to map it to a
     * different field. See Initialize and InsertPosition.
     */
    {
	XtNinsertPosition, XtCInsertPosition,
	XtRFunction, sizeof(XtOrderProc), offset(category.insert_position),
	XtRImmediate, (XtPointer)0
    },

    /*
     * These resources are from Manager, but we need to change their
     * defaults.
     */
    {	/* SGI */
	XtNshadowType, XtCShadowType,
	XtROlDefine, sizeof(OlDefine), offset(manager.shadow_type),
	XtROlDefine, (XtPointer)&shadow_type
    },
    {	/* SGI */
	XtNshadowThickness, XtCShadowThickness,
	XtRDimension, sizeof(Dimension), offset(manager.shadow_thickness),
	XtRString, (XtPointer)"2 points"
    },

    /*
     * New resources:
     */
    {	/* SGI */
	XtNlayoutWidth, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(category.layout.width),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },
    {	/* SGI */
	XtNlayoutHeight, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(category.layout.height),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },
    {	/* SGI */
	XtNpageWidth, XtCPageWidth,
	XtRDimension, sizeof(Dimension), offset(category.page.width), 
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNpageHeight, XtCPageHeight,
	XtRDimension, sizeof(Dimension), offset(category.page.height), 
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNcategoryLabel, XtCCategoryLabel,
	XtRString, sizeof(String), offset(category.category_label),
	XtRImmediate, (XtPointer)"CATEGORY:"
    },
    {	/* SGI */
	XtNleftFoot, XtCFoot,
	XtRString, sizeof(String), offset(category.left_foot),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNrightFoot, XtCFoot,
	XtRString, sizeof(String), offset(category.right_foot),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNshowFooter, XtCShowFooter,
	XtRBoolean, sizeof(Boolean), offset(category.show_footer),
	XtRImmediate, (XtPointer)False
    },
    {	/* SI */
	XtNnewPage, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(category.new_page),
	XtRCallback, (XtPointer)0
    },
    {	/* SGI */
	XtNfontGroup, XtCFontGroup,
	XtROlFontList, sizeof(OlFontList *), offset(category.font_list),
	XtRString, (XtPointer)0
    },
    FONT,
    CATEGORY_FONT,
    FONT_COLOR,

#undef offset
};

/*
 * Constraint resource list:
 */

static XtResource	constraints[] = {
#define offset(F) XtOffsetOf(CategoryConstraintRec, F)

    {	/* SGI */
	XtNpageLabel, XtCPageLabel,
	XtRString, sizeof(String), offset(category.page_label),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNpaneGravity, XtCGravity,
	XtRGravity, sizeof(int), offset(category.gravity),
	XtRImmediate, (XtPointer)CenterGravity
    },
    {	/* GI */
	XtNdefault, XtCDefault,
	XtRBoolean, sizeof(Boolean), offset(category._default),
	XtRImmediate, (XtPointer)False
    },
    {	/* SGI */
	XtNchanged, XtCChanged,
	XtRBoolean, sizeof(Boolean), offset(category.changed),
	XtRImmediate, (XtPointer)False
    },

#if	defined(AVAILABLE_WHEN_UNMANAGED)
    /*
     * The following is obsolete, here just to support backwards
     * compatibility for a while.
     */
    {	/* SGI */
	XtNavailableWhenUnmanaged, XtCAvailableWhenUnmanaged,
	XtRBoolean, sizeof(Boolean), offset(category.available_when_unmanaged),
	XtRImmediate, (XtPointer)False
    },
#endif

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
/* query_alignment   (I)*/ (XtGeometryHandler)0,
};

CategoryClassRec	categoryClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(CategoryRec),
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
/* tm_table          (I)*/ (String)              XtInheritTranslations,
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
/* insert_child      (I)*/                       InsertChild,
/* delete_child      (I)*/                       XtInheritDeleteChild,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Constraint class:
	 */
	{
/* resources            */                       constraints,
/* num_resources        */                       XtNumber(constraints),
/* constraint_size      */                       sizeof(CategoryConstraintRec),
/* initialize        (D)*/                       ConstraintInitialize,
/* destroy           (U)*/                       ConstraintDestroy,
/* set_values        (D)*/                       ConstraintSetValues,
/* extension            */ (XtPointer)           0
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
/* activate          (I)*/                       XtInheritActivateFunc,
/* event_procs          */ (OlEventHandlerList)  0,
/* num_event_procs      */ (Cardinal)            0,
/* register_focus    (I)*/                       XtInheritRegisterFocus,
#if	defined(OL_VERSION) && OL_VERSION < 5
/* reserved             */ (XtPointer)           0,
#endif
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { dyn_resources, XtNumber(dyn_resources) },
/* transparent_proc  (I)*/                       XtInheritTransparentProc,
	},
	/*
	 * Category class:
	 */
	{
/* extension            */ (XtPointer)           0,
	}                          
};

WidgetClass		categoryWidgetClass = thisClass;

/**
 ** OlCategorySetPage()
 **/

Boolean
#if	OlNeedFunctionPrototypes
OlCategorySetPage (
	Widget			w,
	Widget			child
)
#else
OlCategorySetPage (w, child)
	Widget			w;
	Widget			child;
#endif
{
	if (!XtIsRealized(w))
		/*
		 * If the category widget has not yet been realized,
		 * then its change_managed procedure won't have been
		 * called yet so there'll be no page list yet. We
		 * thus delay the setting of the page until we have
		 * the list.
		 */
		CATEGORY_P(w).delayed_set_page_child = child;
	else {
		int i = FindChild(w, child);
		if (i >= 0)
			SetPage (w, (Cardinal)i, False);
		else
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				OleNfileCategory, OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileCategory_msg1,
				XtName(w), XtName(child)
			);
	}

	return (NeedChangeBar(w, CATEGORY_P(w).current_page));
} /* OlCategorySetPage */

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
	_OlAddOlDefineType ("maximize", OL_MAXIMIZE);
	_OlAddOlDefineType ("minimize", OL_MINIMIZE);
	_OlAddOlDefineType ("ignore",   OL_IGNORE);

	layoutCoreClassExtension.record_type = XtQLayoutCoreClassExtension;

	if (OlGetGui() == OL_MOTIF_GUI)
		shadow_type = OL_SHADOW_OUT;

	return;
} /* ClassInitialize */

/**
 ** Initialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,	/*NOTUSED*/
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
	Widget			menu_pane;

	XtWidgetGeometry	query;
	XtWidgetGeometry	response;


	OlCheckLayoutResources (
		new, &CATEGORY_P(new).layout, (OlLayoutResources *)0
	);

	CATEGORY_P(new).page_list              = 0;
	CATEGORY_P(new).page_list_size         = 0;
	CATEGORY_P(new).current_page           = 0;
	CATEGORY_P(new).delayed_set_page_child = 0;

	CATEGORY_P(new).cb = OlCreateChangeBar(
		new, ContrastBackground(new)
	);

	/*
	 * See Primitive Initialize() for explanation...
	 */
	_OlLoadDefaultFont (new, CATEGORY_P(new).category_font);
	_OlLoadDefaultFont (new, CATEGORY_P(new).font);

	/*
	 * We want internal children to be in a particular order, thus we
	 * provide an insert_position procedure. We also let the client
	 * provide its version of the procedure. See InsertPosition and
	 * the resource list.
	 */
	COMPOSITE_P(new).insert_position = InsertPosition;

	if (!CATEGORY_P(new).category_label)
		CATEGORY_P(new).category_label = XtName(new);
	CATEGORY_P(new).category_label
		= XtNewString(CATEGORY_P(new).category_label);

	GetGCs (new);

	/*
	 * Don't manage any internal widgets until we have everything
	 * ready for the "change_managed" procedure.
	 */
	CATEGORY_P(new).flags = 0;
	SETFLAG (new, _CATEGORY_INTERNAL_CHILD);

	menu_pane = XtVaCreatePopupShell(
		"pane", popupMenuShellWidgetClass, new,
		XtNoptionMenu,	(XtArgVal)True,
		(String)0
	);

	/*
	 * _AMB (abbreviated menu button):
	 */
	XtVaCreateManagedWidget (
		"abbrevMenu", abbreviatedButtonWidgetClass, new,
		XtNpopupWidget, (XtArgVal)menu_pane,
		(String)0
	);
	
	/*
	 * Unless we give a non-empty list of items, the flat exclusives
	 * widget will complain. Since we don't have any page-children
	 * yet, we'll use a fake list.
	 */
	CATEGORY_P(new).page_choice = XtVaCreateManagedWidget(
		"pageChoice",
		flatButtonsWidgetClass,
		menu_pane,
		XtNnumItems,      (XtArgVal)1,
		XtNitems,         (XtArgVal)FakeMenuItems,
		XtNnumItemFields, (XtArgVal)XtNumber(MenuFields),
		XtNitemFields,    (XtArgVal)MenuFields,
		XtNselectProc,    (XtArgVal)SetPageCB,
		XtNclientData,    (XtArgVal)new,
		XtNlayoutType,    (XtArgVal)OL_FIXEDCOLS,
		XtNlayoutWidth,   (XtArgVal)OL_MINIMIZE,
		XtNlabelJustify,  (XtArgVal)OL_LEFT,
		XtNfont,          (XtArgVal)CATEGORY_P(new).font,
		XtNexclusives,    (XtArgVal)True,
		XtNbuttonType,    (XtArgVal)OL_RECT_BTN,
		(String)0
	);

	/*
	 * Barf, retch.
	 */
      	{
	      	static String	fields[] = { XtNlabel };

	      	CATEGORY_P(new).next_page = XtVaCreateManagedWidget(
	      		"olCategoryNextPage",
	      		flatButtonsWidgetClass,
	      		menu_pane,
	      		XtNitemFields,    (XtArgVal)fields,
	      		XtNnumItemFields, (XtArgVal)XtNumber(fields),
	      		XtNselectProc,    (XtArgVal)NextPageCB,
	      		XtNclientData,    (XtArgVal)new,
	      		(String)0
		);
		OlVaFlatSetValues (
			CATEGORY_P(new).next_page, 0,
			XtNdefault, (XtArgVal)True,
			(String)0
		);
      	}

	/*
	 * _LCA (lower control area):
	 */
	XtVaCreateManagedWidget (
		"lowerControlArea", controlAreaWidgetClass, new,
		XtNshadowThickness, (XtArgVal)0,
		(String)0
	);

	/*
	 * _FTR (Footer):
	 */
	XtVaCreateWidget (
		"footer", footerWidgetClass, new,
		XtNpopupWidget, (XtArgVal)menu_pane,
		XtNleftFoot,    (XtArgVal)CATEGORY_P(new).left_foot,
		XtNrightFoot,   (XtArgVal)CATEGORY_P(new).right_foot,
		(String)0
	);
	if (CATEGORY_P(new).show_footer)
		XtManageChild (COMPOSITE_P(new).children[_FTR]);
	/*
	 * Set the values to something the client can never use, so that
	 * we can detect a change in SetValues.
	 */
	CATEGORY_P(new).left_foot  = &internal;
	CATEGORY_P(new).right_foot = &internal;
	
	CLRFLAG (new, _CATEGORY_INTERNAL_CHILD);

	/*
	 * Call Layout to determine our initial size.
	 */
	query.width = 0;
	query.height = 0;
	OlLayoutWidget (new, True, True, False, new, &query, &response);
	OlInitializeGeometry (
	     new, &CATEGORY_P(new).layout, response.width, response.height
	);

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
	XtFree ((char *)CATEGORY_P(w).category_label);
	if (CATEGORY_P(w).page_list)
		XtFree ((char *)CATEGORY_P(w).page_list);
	FreeGCs (w);
	if (CATEGORY_P(w).cb)
		OlDestroyChangeBar (w, CATEGORY_P(w).cb);
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
	XEvent *		pe,		/*NOTUSED*/
	Region			region
)
#else
ExposeProc (w, pe, region)
	Widget			w;
	XEvent *		pe;
	Region			region;
#endif
{
	Geometry		change_bar;
	Geometry		category_label;
	Geometry		page_label;

	XtWidgetGeometry	top;

	Dimension		footer;

	int			h;


	/*
	 * CATEGORY label, page label:
	 */

	ComputeTopSize (
		w,
		&top, &change_bar, &category_label,
		(PositionPair *)0, &page_label
	);

	if (NeedChangeBar(w, CATEGORY_P(w).current_page))
		OlDrawChangeBar (
			w,
			CATEGORY_P(w).cb,
			OL_NORMAL, False,
			change_bar.x, change_bar.y,
			region
		);

	DrawText (
		w,
		region, CATEGORY_P(w).category_font_GC,
		&category_label, CATEGORY_P(w).category_label
	);
	if (CATEGORY_P(w).page_list)
		DrawText (
			w,
			region, CATEGORY_P(w).font_GC,
			&page_label,
			(String)CATEGORY_P(w).page_list[CATEGORY_P(w).current_page].label
		);

	/*
	 * Border:
	 */
	footer = HaveFooter(w)?
			CORE_P(COMPOSITE_P(w).children[_FTR]).height : 0;
	if (OlGetGui() == OL_MOTIF_GUI) {
		DrawBorder (
			w,
			region,
			0, 0, CORE_P(w).width, top.height
		);
		if (footer) {
			footer += 2*Shadow(w);
			DrawBorder (
				w,
				region,
				0, (Position)((int)CORE_P(w).height - footer),
				CORE_P(w).width, (Dimension)footer
			);
		}
	}
	h = top.height + footer;
	h = (int)CORE_P(w).height > h? (int)CORE_P(w).height - h : 0;
	DrawBorder (
	    w, region, 0, top.height, CORE_P(w).width, h
	);

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
	Boolean			do_layout = False;
	Boolean			redisplay = False;
	Boolean			get_GCs = False;

	Arg			a[2];

	Cardinal		n;


#define DIFFERENT(F) \
	(((CategoryWidget)new)->F != ((CategoryWidget)current)->F)


	OlCheckLayoutResources (
		new, &CATEGORY_P(new).layout, &CATEGORY_P(current).layout
	);

	/*
	 * We play a subtle trick here: We aren't supposed to do any
	 * (re)displaying in this routine, but should just return a
	 * Boolean that indicates whether the Intrinsics should force
	 * a redisplay. But the Intrinsics does this by "calling the
	 * Xlib XClearArea() function on the [parent] widget's window."
	 * This would cause the entire widget to need a redisplay!
	 * Thus, instead of returning True here, we instead call
	 * XClearArea (with exposures) on just the area of the label
	 * and return False.
	 */
	if (
		DIFFERENT(category.category_label)
	     || DIFFERENT(category.category_font)
	     || DIFFERENT(category.font_color)
	) {
		ClearCategoryLabel (current);
		do_layout = True;
	}
	if (DIFFERENT(category.category_font))
		_OlLoadDefaultFont (new, CATEGORY_P(new).category_font);
	if (DIFFERENT(category.category_label)) {
		XtFree ((char *)CATEGORY_P(current).category_label);
		if (!CATEGORY_P(new).category_label)
			CATEGORY_P(new).category_label = XtName(new);
		CATEGORY_P(new).category_label
			= XtNewString(CATEGORY_P(new).category_label);
	}
	if (
		DIFFERENT(category.font)
	     || DIFFERENT(category.font_color)
	) {
		ClearPageLabel (current);
		do_layout = True;
	}
	if (DIFFERENT(category.font))
		_OlLoadDefaultFont (new, CATEGORY_P(new).font);

	if (DIFFERENT(core.background_pixel)) {
		OlChangeBarSetValues (
			new, ContrastBackground(new), CATEGORY_P(new).cb
		);
		ClearChangeBar (new);
		get_GCs = True;
	}

	if (
		DIFFERENT(category.font_color)
	     || DIFFERENT(category.category_font->fid)
	     || DIFFERENT(category.font->fid)
	)
		get_GCs = True;

	if (get_GCs) {
		FreeGCs (new);
		GetGCs (new);
	}

	if (DIFFERENT(category.show_footer)) {
		/*
		 * Avoid going through ChangeManaged just to get to
		 * Layout; this will also delay the layout until we can
		 * do it just once.
		 */
		SETFLAG (new, _CATEGORY_INTERNAL_CHILD);
		if (CATEGORY_P(new).show_footer)
			XtManageChild (COMPOSITE_P(new).children[_FTR]);
		else
			XtUnmanageChild (COMPOSITE_P(new).children[_FTR]);
		CLRFLAG (new, _CATEGORY_INTERNAL_CHILD);
		do_layout = True;
	}

	n = 0;
	if (DIFFERENT(category.left_foot)) {
		XtSetArg (a[n], XtNleftFoot, CATEGORY_P(new).left_foot);
		n++;
		CATEGORY_P(new).left_foot  = &internal;
	}
	if (DIFFERENT(category.right_foot)) {
		XtSetArg (a[n], XtNrightFoot, CATEGORY_P(new).right_foot);
		n++;
		CATEGORY_P(new).right_foot = &internal;
	}
	if (n)
		XtSetValues (COMPOSITE_P(new).children[_FTR], a, n);

	if (
		DIFFERENT(category.layout.width)
	     || DIFFERENT(category.layout.height)
	     || DIFFERENT(category.page.width)
	     || DIFFERENT(category.page.height)
	)
		do_layout = True;

	OlLayoutWidgetIfLastClass (new, thisClass, do_layout, False);

#undef DIFFERENT
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
	Cardinal		n;

	for (n = 0; n < *num_args; n++)
		if (STREQU(args[n].name, XtNlowerControlArea))
			*(Widget *)args[n].value
				= COMPOSITE_P(w).children[_LCA];
		else if (STREQU(args[n].name, XtNfooter))
			*(Widget *)args[n].value
				= COMPOSITE_P(w).children[_FTR];
		else if (
			STREQU(args[n].name, XtNleftFoot)
		     || STREQU(args[n].name, XtNrightFoot)
		)
			GetValue (
				COMPOSITE_P(w).children[_FTR],
				args[n].name, args[n].value
			);

	return;
} /* GetValuesHook() */

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
	Cardinal		size = 0;
	Cardinal		i;
	Cardinal		n;

	Widget			current = 0;
	Widget			child;

	MenuItem *		list = CATEGORY_P(w).page_list;


	/*
	 * Don't waste time with special (internal) children.
	 */
	if (ISFLAG(w, _CATEGORY_INTERNAL_CHILD))
		return;

	/*
	 * Save the current page's widget for later. If we have
	 * a delayed OlCategorySetPage() call pending, use the
	 * page widget we set aside, instead. However, if the delayed
	 * page is not available, it can't be made the current page.
	 */
	if (CATEGORY_P(w).delayed_set_page_child) {
		current = CATEGORY_P(w).delayed_set_page_child;
		CATEGORY_P(w).delayed_set_page_child = 0;
		if (!XtIsManaged(current))
			current = 0;
	}
	if (!current && list)
		current = (Widget)list[CATEGORY_P(w).current_page].user_data;

	/*
	 * If we don't yet have a current page, or if the current page is
	 * no longer available, find an available page that has XtNdefault
	 * TRUE.
	 *
	 * Checking to see if more than one child is added with
	 * XtNdefault TRUE is not a good idea. As it is now, an
	 * application can change the default to a widget being
	 * added in one shot. Insisting that only one child
	 * has XtNdefault TRUE would force an application to
	 * clear the flag on an old widget before setting it
	 * for a new widget. We just use reasonable defaults:
	 *
	 *	- if no children have XtNdefault TRUE, use the first
	 *	  child
	 *
	 *	- if more then one child has XtNdefault TRUE, use the
	 *	  last child added (or set)
	 */
	if (!current || !XtIsManaged(current))
		FOR_EACH_MANAGED_PAGE (w, child, n)
			if (CATEGORY_CP(child)._default)
				current = child;
	if (!current || !XtIsManaged(current))
		FOR_EACH_MANAGED_PAGE (w, child, n) {
			current = child;
			break;
		}

	/*
	 * If the current page has changed, switch to it.
	 */
	if (list && current != (Widget)list[CATEGORY_P(w).current_page].user_data) 
		if (current) {
			int cp = FindChild(w, current);
			if (cp != -1)
				SetPage (w, (Cardinal)cp, True);
		}

	/*
	 * Count the number of pages.
	 * While we're here, set the window gravity if it hasn't
	 * been set yet.
	 */
	FOR_EACH_MANAGED_PAGE (w, child, n) {
		size++;
		SetWindowGravity (child);
	}

	/*
	 * (Re)allocate space for the menu list.
	 */
	if (size != CATEGORY_P(w).page_list_size) {
		if (size)
			list = (MenuItem *)XtRealloc(
				(char *)list, size * sizeof(MenuItem)
			);
		else {
			XtFree ((char *)list);
			list = 0;
		}
		CATEGORY_P(w).page_list = list;
		CATEGORY_P(w).page_list_size = size;
	}

	/*
	 * Fill the (new) array, and find the new index of the
	 * current page. Clear all mapped-when-managed flags here,
	 * except for the current page.
	 */
	CATEGORY_P(w).current_page = 0;
	i = 0;
	FOR_EACH_MANAGED_PAGE (w, child, n) {
		list[i].label = (XtArgVal)CATEGORY_CP(child).page_label;
		list[i].user_data = (XtArgVal)child;
		if (child == current) {
			CATEGORY_P(w).current_page = i;
			CORE_P(child).mapped_when_managed = True;
			list[i].set = (XtArgVal)True;
		} else {
			CORE_P(child).mapped_when_managed = False;
			list[i].set = (XtArgVal)False;
		}
		i++;
	}

	/*
	 * If the page-list is (now) empty, we may have to
	 * replace the flat exclusives menu with a fake menu,
	 * to avoid problems with an empty list.
	 */
	SETFLAG (w, _CATEGORY_INTERNAL_CHILD);
	if (size) {
		XtVaSetValues (
			CATEGORY_P(w).page_choice,
			XtNnumItems,    (XtArgVal)size,
			XtNitems,       (XtArgVal)list,
			XtNlayoutWidth, (XtArgVal)OL_MINIMIZE,
			(String)0
		);
	} else {
		XtVaSetValues (
			CATEGORY_P(w).page_choice,
			XtNnumItems,    (XtArgVal)1,
			XtNitems,       (XtArgVal)FakeMenuItems,
			(String)0
		);
	}
	CLRFLAG (w, _CATEGORY_INTERNAL_CHILD);
	FixMenuWidth (w);

	/*
	 * Let the superclass take care of the balance of change_managed
	 * duties.
	 */
	(*COMPOSITE_C(superClass).change_managed) (w);

	return;
} /* ChangeManaged */

/**
 ** InsertChild()
 **/

static void
#if	OlNeedFunctionPrototypes
InsertChild (
	Widget			w
)
#else
InsertChild (w)
	Widget			w;
#endif
{
	SetValue (w, XtNshadowThickness, (XtArgVal)0);
	(*COMPOSITE_C(superClass).insert_child) (w);
	return;
} /* InsertChild */

/**
 ** ConstraintInitialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
ConstraintInitialize (
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
ConstraintInitialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Widget			parent = XtParent(new);


	if (IsInternalChild(parent, new))
		return;

	if (!CATEGORY_CP(new).page_label)
		CATEGORY_CP(new).page_label = XtName(new);
	CATEGORY_CP(new).page_label
			= XtNewString(CATEGORY_CP(new).page_label);

	CATEGORY_CP(new).window_gravity_set = False;

	/*
	 * The XtNdefault resource is checked in the change_managed
	 * procedure.
	 */
	return;
} /* ConstraintInitialize */

/**
 ** ConstraintDestroy()
 **/

static void
#if	OlNeedFunctionPrototypes
ConstraintDestroy (
	Widget			w
)
#else
ConstraintDestroy (w)
	Widget			w;
#endif
{
	if (!IsInternalChild(XtParent(w), w)) {
		XtFree ((char *)CATEGORY_CP(w).page_label);
		/*
		 * The change_managed procedure has already been called
		 * and this page-child has been removed from the menu.
		 */
	}
	return;
} /* ConstraintDestroy */

/**
 ** ConstraintSetValues()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
ConstraintSetValues (
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
ConstraintSetValues (current, request, new, args, num_args)
	Widget			current;
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Widget			parent = XtParent(new);

	Boolean			do_layout = False;
	Boolean			redisplay = False;


	if (IsInternalChild(parent, new))
		return (False);

#define DIFFERENT(F) \
	( ((CategoryConstraintRec *)(new->core.constraints))->F !=	\
	  ((CategoryConstraintRec *)(current->core.constraints))->F )

	if (DIFFERENT(category.page_label)) {
		int		i = FindChild(parent, new);

		XtFree ((char *)CATEGORY_CP(current).page_label);
		if (!CATEGORY_CP(new).page_label)
			CATEGORY_CP(new).page_label = XtName(new);
		CATEGORY_CP(new).page_label
			= XtNewString(CATEGORY_CP(new).page_label);

		if (i >= 0) {
			CATEGORY_P(parent).page_list[i].label
				= (XtArgVal)CATEGORY_CP(new).page_label;
			XtVaSetValues (
				CATEGORY_P(parent).page_choice,
				XtNitemsTouched, (XtArgVal)True,
				XtNlayoutWidth,  (XtArgVal)OL_MINIMIZE,
				(String)0
			);
			FixMenuWidth (parent);
		}

		/*
		 * We play a subtle trick here: We aren't supposed to do
		 * any (re)displaying in this routine, but should just
		 * return a Boolean that indicates whether the Intrinsics
		 * should force a redisplay. But the Intrinsics does this
		 * by "calling the Xlib XClearArea() function on the
		 * [parent] widget's window." This would cause the entire
		 * widget to need a redisplay! Thus, instead of returning
		 * True here, we instead call XClearArea ourselves on just
		 * the area of the label and return False.
		 *
		 * The XClearArea() (done inside ClearPageLabel()) will
		 * generate a much smaller expose event.
		 */
		if (CATEGORY_P(parent).current_page == i)
			ClearPageLabel (parent);

		do_layout = True;
	}

	if (DIFFERENT(category.changed)) {
		/*
		 * Same trick as above.
		 */
		ClearChangeBar (parent);
		CheckCancelDismiss (parent);
	}

	if (DIFFERENT(category.gravity)) {
		CATEGORY_CP(new).window_gravity_set = False;
		SetWindowGravity (new);
		do_layout = True;
	}

	OlLayoutWidgetIfLastClass (new, thisClass, do_layout, False);

#undef	DIFFERENT
	return (redisplay);
} /* ConstraintSetValues */

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
	Boolean			cached_best_fit_ok_hint, /*NOTUSED*/
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
	Widget			amb = COMPOSITE_P(w).children[_AMB];
	Widget			lca = COMPOSITE_P(w).children[_LCA];
	Widget			ftr = COMPOSITE_P(w).children[_FTR];
	Widget			cur = 0;
	Widget			child;

	Cardinal		n;

	Dimension		footer_height;
	Dimension		shadow = Shadow(w);

	XtWidgetGeometry	top;
	XtWidgetGeometry	page;
	XtWidgetGeometry	best_fit;
	XtWidgetGeometry	adjusted;
	XtWidgetGeometry	available;

	int			x;
	int			y;
	int			width;
	int			height;

	PositionPair		abbrev;


	if (CATEGORY_P(w).page_list)
		cur = (Widget)(CATEGORY_P(w).page_list[CATEGORY_P(w).current_page].user_data);
	if (cur && !XtIsManaged(cur))
		cur = 0;

	/*
	 * If a page is asking but the child isn't the current page,
	 * give it what it wants. This could be a lie, but we'll correct
	 * ourselves when the page is finally made current. This lie
	 * prevents a jerky view as each little subwidget of the
	 * asking page is created.
	 */
	if (who_asking) {
		/*
		 * Note: If who_asking is a child it must be a managed
		 * child--since who_asking being set to a child means we
		 * got here from the geometry_manager--therefore FindChild
		 * is sufficient.
		 */
		int i = FindChild(w, who_asking);
		if (i >= 0 && who_asking != cur) {
			if (response)
				*response = *request;
			return;
		}
	}

	/*
	 * MORE:
	 * If one of the internal children is querying, we need to use
	 * its requested geometry instead of CORE, in the following
	 * routine and elsewhere below.
	 */

	/*
	 * Fetch the sizes of things. Note that top and footer_height
	 * include the space for the Motif shadows around the top and
	 * footer, if any.
	 */
	ComputeTopSize (w, &top, (Geometry *)0, (Geometry *)0, &abbrev, (Geometry *)0);
	if (HaveFooter(w)) {
		footer_height = CORE_P(ftr).height;
		if (OlGetGui() == OL_MOTIF_GUI)
			footer_height += 2*shadow;
	} else
		footer_height = 0;

	/*
	 * Find size of largest page.
	 */
	page.width = 0;
	page.height = 0;
	FOR_EACH_MANAGED_PAGE (w, child, n) {
		Dimension	w;
		Dimension	h;
		if (who_asking == child) {
			w = request->width + 2*request->border_width;
			h = request->height + 2*request->border_width;
		} else {
			w = _OlWidgetWidth(child);
			h = _OlWidgetHeight(child);
		}
		page.width = _OlMax(page.width, w);
		page.height = _OlMax(page.height, h);
	}

	/*
	 * If the layout is OL_IGNORE and we were given a page size, use
	 * it in place of the maximum page size.
	 * If the layout is OL_MINIMIZE, use the size of the current page
	 * instead of the maximum page size.
	 */
#define DECISION(DIM) \
	switch (CATEGORY_P(w).layout.DIM) {				\
	case OL_IGNORE:							\
		if (CATEGORY_P(w).page.DIM)				\
			page.DIM = CATEGORY_P(w).page.DIM;		\
		break;							\
	case OL_MINIMIZE:						\
		if (cur)						\
			if (who_asking == cur)				\
				page.DIM = request->DIM			\
					 + 2*request->border_width;	\
			else if (CORE_P(cur).DIM)			\
				page.DIM = CORE_P(cur).DIM;		\
		break;							\
	}

	DECISION (width)
	DECISION (height)
#undef	DECISION

	/*
	 * The page "includes" the button box across the bottom.
	 * (Not really, but we draw it to appear so.)
	 */
	if (page.width < CORE_P(lca).width)
		page.width = CORE_P(lca).width;
	page.height += CORE_P(lca).height;

	/*
	 * Add space for the border around the page. This border is
	 * present in both OPEN LOOK and Motif modes.
	 */
	page.width += 2*shadow;
	page.height += 2*shadow;

	/*
	 * This is the "just fits" size. Note that the footer
	 * width doesn't count (it is truncated to fit, if too wide).
	 */
	best_fit.width  = _OlMax(top.width, page.width);
	best_fit.height = top.height + page.height + footer_height;

	/*
	 * If this is a query of the composite best size, return now.
	 * The response must be no smaller than best_size.
	 */
	if (who_asking == w) {
		response->x = CORE_P(w).x;
		response->y = CORE_P(w).y;
		response->width = _OlMax(request->width, best_fit.width);
		response->height = _OlMax(request->height, best_fit.height);
		response->border_width = request->border_width;
		return;
	}

	/*
	 * Adjust the best_fit to account for the XtNlayoutWidth and
	 * XtNlayoutHeight "constraints".
	 */
	OlAdjustGeometry (w, &CATEGORY_P(w).layout, &best_fit, &adjusted);

	/*
	 * If we can resize, try to resolve any delta between our
	 * current size and the best-fit size by asking our parent
	 * for the new size.
	 */
	OlAvailableGeometry (
		w, resizable, query_only, who_asking, request,
		&adjusted, &available
	);
	/*
	 * At this point, available contains the size available for
	 * managing the children. Note that it may not be the same
	 * as the (composite's) core geometry (a query situation).
	 */

	/*
	 * Place the abbreviated menu button in its correct
	 * place; the category label and the page label will
	 * be taken care of when we get an expose event.
	 */
	OlConfigureChild (
		amb,
		abbrev.x, abbrev.y,
		CORE_P(amb).width, CORE_P(amb).height,
		CORE_P(amb).border_width,
		query_only, who_asking, response
	);

	/*
	 * Try to center the lower control area above the footer,
	 * but if there's not enough space, maintain a NorthWest
	 * gravity.
	 */
	x = ((int)available.width - (int)CORE_P(lca).width) / 2;
	y = (int)available.height
	  - (int)footer_height
	  - (int)shadow  /* for bottom-edge of page shadow */
	  - (int)CORE_P(lca).height;
	OlConfigureChild (
		lca,
		x, y, CORE_P(lca).width, CORE_P(lca).height,
		CORE_P(lca).border_width,
		query_only, who_asking, response
	);

	/*
	 * Position the footer at the bottom of the window above and
	 * right-of its shadow, if any.
	 */
	if (HaveFooter(w)) {
		Dimension sh = OlGetGui() == OL_MOTIF_GUI? shadow : 0;
		OlConfigureChild (
			ftr,
			sh, available.height - footer_height + sh,
			available.width - 2*sh, CORE_P(ftr).height,
			CORE_P(ftr).border_width,
			query_only, who_asking, response
		);
	}

	/*
	 * This is the largest the page can be. Don't worry about
	 * negative values (!), as a check inside the following
	 * loop prevents us trying to set child sizes <= 0.
	 */
	width  = (int)available.width
	       - 2*(int)shadow;	/* shadow around page */
	height = (int)available.height
	       - (int)top.height
	       - (int)2*shadow	/* shadow around page */
	       - (int)CORE_P(lca).height
	       - (int)footer_height;

	/*
	 * Configure the current page.
	 *
	 * We don't configure any other pages here, just to avoid doing
	 * needless work. When the user switches to a new page, Layout
	 * will be called again for that page.
	 */
	if (cur) {
		Dimension		bw = CORE_P(cur).border_width;

		XtWidgetGeometry	max;
		XtWidgetGeometry	pre;


		/*
		 * Set up the maximum available geometry.
		 */
		max.x = shadow;			/* inset the page    */
		max.y = top.height + shadow;	/* inside its shadow */
		if (width > (int)(2*bw))
			max.width = width - (int)(2*bw);
		else
			max.width = 1;
		if (height > (int)(2*bw))
			max.height = height - (int)(2*bw);
		else
			max.height = 1;
		max.border_width = bw;

		if (cur == who_asking) {
			pre.request_mode = CWWidth|CWHeight;
			pre.width = request->width;
			pre.height = request->height;
		} else
			pre.request_mode = 0;
		max.request_mode = CWWidth|CWHeight; /* see OlResolveGravity */
		OlResolveGravity (
			cur, CATEGORY_CP(cur).gravity,
			&max, &max, &max, &pre, (XtWidgetGeometry *)0,
			XtQueryGeometry
		);
		OlConfigureChild (
			cur,
			pre.x, pre.y, pre.width, pre.height, bw,
			query_only, who_asking, response
		);
	}

	return;
} /* Layout */

/**
 ** InsertPosition()
 **/

static Cardinal
#if	OlNeedFunctionPrototypes
InsertPosition (
	Widget			w
)
#else
InsertPosition (w)
	Widget			w;
#endif
{
	Widget			parent = XtParent(w);

	Cardinal		pos;


	if (
		!ISFLAG(parent, _CATEGORY_INTERNAL_CHILD)
	     && CATEGORY_P(parent).insert_position
	) {
		pos = (*CATEGORY_P(parent).insert_position)(w);
		if (pos < _1ST)
			pos = _1ST;
	} else
		pos = COMPOSITE_P(parent).num_children;

	return (pos);
} /* InsertPosition */

/**
 ** SetWindowGravity
 **/

static void
#if	OlNeedFunctionPrototypes
SetWindowGravity (
	Widget			w
)
#else
SetWindowGravity (w)
	Widget			w;
#endif
{
	XSetWindowAttributes	attributes;


	if (
		!XtIsWidget(w)
	     || !XtIsRealized(w)
	     || CATEGORY_CP(w).window_gravity_set
	)
		return;

	switch (CATEGORY_CP(w).gravity) {
	case AllGravity:
	case NorthSouthEastWestGravity:
	case SouthEastWestGravity:
	case NorthEastWestGravity:
	case NorthSouthWestGravity:
	case NorthSouthEastGravity:
	case EastWestGravity:
	case NorthSouthGravity:
	default:
		break;
	case CenterGravity:
	case NorthGravity:
	case SouthGravity:
	case EastGravity:
	case WestGravity:
	case NorthWestGravity:
	case NorthEastGravity:
	case SouthWestGravity:
	case SouthEastGravity:
		attributes.bit_gravity = CATEGORY_CP(w).gravity;
		XChangeWindowAttributes (
		      XtDisplay(w), XtWindow(w), CWBitGravity, &attributes
		);
		CATEGORY_CP(w).window_gravity_set = True;
		break;
	}

	return;
} /* SetWindowGravity */

/**
 ** FixMenuWidth()
 **/

static void
#if	OlNeedFunctionPrototypes
FixMenuWidth (
	Widget			w
)
#else
FixMenuWidth (w)
	Widget			w;
#endif
{
	Dimension		width;


	/*
	 * The menu looks a bit silly when the exclusives are
	 * narrower than the next-page button.
	 */
	GetValue (CATEGORY_P(w).next_page, XtNwidth, (XtArgVal)&width);
	SetValue (CATEGORY_P(w).page_choice, XtNitemMinWidth, (XtArgVal)width);

	return;
} /* FixMenuWidth */

/**
 ** NeedChangeBar()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
NeedChangeBar (
	Widget			w,
	Cardinal		current_page
)
#else
NeedChangeBar (w, current_page)
	Widget			w;
	Cardinal		current_page;
#endif
{
	Widget			child;
	Widget			current;

	Cardinal		n;


	/*
	 * WARNING: We assume the caller is passing a valid current_page.
	 */
	if (CATEGORY_P(w).page_list) {
		current = (Widget)(CATEGORY_P(w).page_list[current_page].user_data);
		FOR_EACH_MANAGED_PAGE (w, child, n)
			if (CATEGORY_CP(child).changed && child != current)
				return (True);
	}
	return (False);
} /* NeedChangeBar */

/**
 ** CheckCancelDismiss()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckCancelDismiss (
	Widget			w
)
#else
CheckCancelDismiss (w)
	Widget			w;
#endif
{
	Widget			child;
	Widget			shell = XtParent(w);

	Cardinal		n;

	OlDefine		new_menu_type = OL_MENU_LIMITED;
	OlDefine		menu_type;


	if (!XtIsShell(shell))
		return;

	FOR_EACH_MANAGED_PAGE (w, child, n)
		if (CATEGORY_CP(child).changed) {
			new_menu_type = OL_MENU_CANCEL;
			break;
		}

	GetValue (shell, XtNmenuType, (XtArgVal)&menu_type);
	if (new_menu_type != menu_type)
		SetValue (shell, XtNmenuType, (XtArgVal)new_menu_type);

	return;
} /* CheckCancelDismiss */

/**
 ** DrawText()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawText (
	Widget			w,
	Region			region,
	GC			gc,
	Geometry *		g,
	String			text
)
#else
DrawText (w, region, gc, g, text)
	Widget			w;
	Region			region;
	GC			gc;
	Geometry *		g;
	String			text;
#endif
{
	Cardinal		len	= _OlStrlen(text);


	if (XtIsRealized(w) && len) {
		if (InRegion(region, g)) {
			if (!CATEGORY_P(w).font_list)
				XDrawImageString (
					XtDisplay(w), XtWindow(w),
					gc,
					g->x, g->y_baseline,
					text, len
				);
			else
				OlDrawImageString (
					XtDisplay(w), XtWindow(w),
					CATEGORY_P(w).font_list,
					gc,
					g->x, g->y_baseline,
					(unsigned char *)text, len
				);
			XFlush (XtDisplay(w));
		}
	}
	return;
} /* DrawText */

/**
 ** DrawBorder()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawBorder (
	Widget			w,
	Region			region,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height
)
#else
DrawBorder (w, region, x, y, width, height)
	Widget			w;
	Region			region;
	Position		x;
	Position		y;
	Dimension		width;
	Dimension		height;
#endif
{
	if (XtIsRealized(w)) {
		Geometry		g;

		/*
		 * Left edge in region?
		 */
		g.x = x;
		g.y = y;
		g.width = Shadow(w);
		g.height = height;
		if (InRegion(region, &g))
			goto DrawIt;
		/*
		 * Right edge in region?
		 */
		g.x = x + (width - Shadow(w));
		g.y = y;
		g.width = Shadow(w);
		g.height = height;
		if (InRegion(region, &g))
			goto DrawIt;
		/*
		 * Top edge in region?
		 */
		g.x = x;
		g.y = y;
		g.width = width;
		g.height = Shadow(w);
		if (InRegion(region, &g))
			goto DrawIt;
		/*
		 * Bottom edge in region?
		 */
		g.x = x;
		g.y = y + (height - Shadow(w));
		g.width = width;
		g.height = Shadow(w);
		if (InRegion(region, &g)) {
DrawIt:			OlgDrawBorderShadow (
				XtScreen(w), XtWindow(w),
				MANAGER_P(w).attrs,
				MANAGER_P(w).shadow_type, Shadow(w),
				x, y, width, height
			);
		}
	}
	return;
} /* DrawBorder */

/**
 ** ClearChangeBar()
 **/

static void
#if	OlNeedFunctionPrototypes
ClearChangeBar (
	Widget			w
)
#else
ClearChangeBar (w)
	Widget			w;
#endif
{
	if (XtIsRealized(w)) {
		Geometry		change_bar;

		/*
		 * Note: We don't check to see if we need a change bar,
		 * because we rely on the expose event that the server
		 * generates after this. On getting the expose event,
		 * we will see if we need to draw a change bar.
		 */
		ComputeChangeBarPosition (w, &change_bar);
		OlDrawChangeBar (
			w,
			CATEGORY_P(w).cb,
			OL_NONE, True,
			change_bar.x, change_bar.y,
			(Region)0
		);
	}
	return;
} /* ClearChangeBar */

/**
 ** ClearCategoryLabel()
 **/

static void
#if	OlNeedFunctionPrototypes
ClearCategoryLabel (
	Widget			w
)
#else
ClearCategoryLabel (w)
	Widget			w;
#endif
{
	if (XtIsRealized(w)) {
		Geometry		category_label;

		ComputeCategoryLabelSize (w, &category_label);
		XClearArea (
			XtDisplay(w), XtWindow(w),
			category_label.x, category_label.y,
			category_label.width, category_label.height,
			True
		);
	}
	return;
} /* ClearCategoryLabel */

/**
 ** ClearPageLabel()
 **/

static void
#if	OlNeedFunctionPrototypes
ClearPageLabel (
	Widget			w
)
#else
ClearPageLabel (w)
	Widget			w;
#endif
{
	if (XtIsRealized(w)) {
		Geometry		page_label;

		if (CATEGORY_P(w).page_list) {
			ComputePageLabelSize (w, &page_label);
			XClearArea (
				XtDisplay(w), XtWindow(w),
				page_label.x, page_label.y,
				page_label.width, page_label.height,
				True
			);
		}
	}
	return;
} /* ClearPageLabel */

/**
 ** ComputeTopSize()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputeTopSize (
	Widget			w,
	XtWidgetGeometry *	p_top,
	Geometry *		p_change_bar,
	Geometry *		p_category_label,
	PositionPair *		p_abbrev,
	Geometry *		p_page_label
)
#else
ComputeTopSize (w, p_top, p_change_bar, p_category_label, p_abbrev, p_page_label)
	Widget			w;
	XtWidgetGeometry *	p_top;
	Geometry *		p_change_bar;
	Geometry *		p_category_label;
	PositionPair *		p_abbrev;
	Geometry *		p_page_label;
#endif
{
	Screen *		screen = XtScreen(w);

	Widget			amb = COMPOSITE_P(w).children[_AMB];

	XtWidgetGeometry	top;

	PositionPair		abbrev;

	Geometry		change_bar;
	Geometry		category_label;
	Geometry		page_label;

	Dimension		top_margin = HeightFromPoints(CATEGORY_TOP_MARGIN);
	Dimension		width;
	Dimension		ascent;
	Dimension		descent;
	Dimension		max_ascent = 0;
	Dimension		max_descent = 0;
	Dimension		shadow;
	Dimension		h_pad;


	/*
	 * We don't have good guidelines for the vertical spacing of an
	 * abbreviated button and the labels to either side. Close
	 * inspection of the pictures in the OPEN LOOK specification
	 * suggest that the label to the right should be centered
	 * vertically in its ascent with the button, and the label
	 * to the left should have the same baseline as the label
	 * to the right. Not all pictures show this, but it looks OK.
	 */


	shadow = OlGetGui() == OL_MOTIF_GUI? Shadow(w) : 0;
	top.width = shadow;

	/*
	 * Change bar:
	 */
	width = CATEGORY_P(w).cb->width + CATEGORY_P(w).cb->pad;
	h_pad = WidthFromPoints(CATEGORY_CHANGE_BAR_SPACE) + width;
	top.width += h_pad;
	change_bar.x = top.width - width;
/*	change_bar.y = see BELOW */
	change_bar.width = CATEGORY_P(w).cb->width;
	change_bar.height = CATEGORY_P(w).cb->height;

	/*
	 * CATEGORY label:
	 */
	ComputeTextSize (
		CATEGORY_P(w).category_font,
		CATEGORY_P(w).font_list,
		CATEGORY_P(w).category_label,
		&width, &ascent, &descent
	);
	top.width += width;
	category_label.x = top.width - width;
	category_label.y = -ascent; /* see BELOW */
	category_label.width = width;
	category_label.height = ascent + descent;

	if (ascent > max_ascent)
		max_ascent = ascent;
	if (descent > max_descent)
		max_descent = descent;

	/*
	 * Abbreviated menu button (Part I):
	 */
	top.width += WidthFromPoints(CATEGORY_SPACE1) + CORE_P(amb).width;
	abbrev.x = top.width - CORE_P(amb).width;

	/*
	 * Page label:
	 */
	if (CATEGORY_P(w).page_list) {
		ComputeTextSize (
			CATEGORY_P(w).font,
			CATEGORY_P(w).font_list,
			(String)CATEGORY_P(w).page_list[CATEGORY_P(w).current_page].label,
			&width, &ascent, &descent
		);

		top.width += WidthFromPoints(CATEGORY_SPACE2) + width;
		page_label.x = top.width - width;
		page_label.y = -ascent; /* see BELOW */
		page_label.width = width;
		page_label.height = ascent + descent;

		if (ascent > max_ascent)
			max_ascent = ascent;
		if (descent > max_descent)
			max_descent = descent;
	}

	/*
	 * Space to the right of the page label. This is the same as
	 * the space to the left of the category label (though that
	 * space is used by the change bar).
	 */
	top.width += h_pad + shadow;

	/*
	 * Abbreviated menu button (Part II):
	 *
	 * At this point, "ascent" is the ascent for the label to the
	 * right (page label), if we have one, otherwise, it is the
	 * ascent for the label to the left (category label). Center
	 * the abbreviated menu button vertically with this distance.
	 */
	ascent = CORE_P(amb).height - (CORE_P(amb).height - ascent) / u2;
	descent = _OlMin(CORE_P(amb).height - ascent, u0);
	if (ascent > max_ascent)
		max_ascent = ascent;
	if (descent > max_descent)
		max_descent = descent;

	top.height = shadow
		   + top_margin
		   + max_ascent
		   + max_descent
		   + (Dimension)HeightFromPoints(CATEGORY_BOTTOM_MARGIN)
		   + shadow;

	/*
	 * [If you're looking for "BELOW", here it is!]
	 *
	 * Note: .y already has the negative of the ascent for the
	 * corresponding font, so adding the .y_baseline value
	 * gives the coordinate of the top edge of the bounding box.
	 */
	category_label.y_baseline  = shadow + top_margin + max_ascent;
	category_label.y          += category_label.y_baseline;
	page_label.y_baseline      = shadow + top_margin + max_ascent;
	page_label.y              += page_label.y_baseline;
	change_bar.y               = category_label.y;

	/*
	 * At this point, ascent is the "ascent" for the abbreviated
	 * menu button.
	 */
	abbrev.y = shadow + top_margin + max_ascent - ascent;


	if (p_top)		*p_top            = top;
	if (p_change_bar)	*p_change_bar     = change_bar;
	if (p_category_label)	*p_category_label = category_label;
	if (p_abbrev)		*p_abbrev         = abbrev;
	if (p_page_label)	*p_page_label     = page_label;

	return;
} /* ComputeTopSize */

/**
 ** ComputeTextSize()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputeTextSize (
	XFontStruct *		f,
	OlFontList *		font_list,
	String			text,
	Dimension *		p_width,
	Dimension *		p_ascent,
	Dimension *		p_descent
)
#else
ComputeTextSize (f, font_list, text, p_width, p_ascent, p_descent)
	XFontStruct *		f;
	OlFontList *		font_list;
	String			text;
	Dimension *		p_width;
	Dimension *		p_ascent;
	Dimension *		p_descent;
#endif
{
	XCharStruct  max;
	if (text && *text) {
		if (!font_list)
			*p_width = XTextWidth(f, text, _OlStrlen(text));
		else
			*p_width = OlTextWidth(
				font_list,
				(unsigned char *)text, _OlStrlen(text)
			);
		max = OlFontMaxBounds(f, font_list);
		
		*p_ascent = max.ascent;
		*p_descent = max.descent;
	}
	return;
} /* ComputeTextSize */

/**
 ** InRegion()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
InRegion (
	Region			region,
	Geometry *		g
)
#else
InRegion (region, g)
	Region			region;
	Geometry *		g;
#endif
{
	switch (XRectInRegion(region, g->x, g->y, g->width, g->height)) {
	case RectanglePart:
	case RectangleIn:
		return (True);
	case RectangleOut:
	default:
		return (False);
	}
} /* InRegion */

/**
 ** SetPageCB()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
SetPageCB (
	Widget			w,		/*NOTUSED*/
	XtPointer		client_data,
	XtPointer		call_data
)
#else
SetPageCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	SetPage (
		(Widget)client_data,
		((OlFlatCallData *)call_data)->item_index,
		True
	);
	return;
} /* SetPageCB */

/**
 ** NextPageCB()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
NextPageCB (
	Widget			w,		/*NOTUSED*/
	XtPointer		client_data,
	XtPointer		call_data	/*NOTUSED*/
)
#else
NextPageCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Widget			cw = (Widget)client_data;

	Cardinal		new_page;


	new_page = CATEGORY_P(cw).current_page + 1;
	if (new_page >= CATEGORY_P(cw).page_list_size)
		new_page = 0;
	SetPage (cw, new_page, True);

	return;
} /* NextPageCB */

/**
 ** SetPage()
 **/

static void
#if	OlNeedFunctionPrototypes
SetPage (
	Widget			w,
	Cardinal		new_page,
	Boolean			call_callbacks
)
#else
SetPage (w, new_page, call_callbacks)
	Widget			w;
	Cardinal		new_page;
	Boolean			call_callbacks;
#endif
{
	MenuItem *		list = CATEGORY_P(w).page_list;
	MenuItem *		new;
	MenuItem *		old;

	Widget			new_w;
	Widget			old_w;


	if (
		CATEGORY_P(w).current_page == new_page
	     || !list
	     || new_page >= CATEGORY_P(w).page_list_size
	)
		return;

	old = &(list[CATEGORY_P(w).current_page]);
	new = &(list[new_page]);

	old_w = (Widget)old->user_data;
	new_w = (Widget)new->user_data;

	if (
		call_callbacks
	     && XtHasCallbacks(w, XtNnewPage) == XtCallbackHasSome
	) {
		OlCategoryNewPage	np;

		np.old_page       = old_w;
		np.new_page       = new_w;
		np.apply_all = NeedChangeBar(w, new_page);
		XtCallCallbacks (w, XtNnewPage, &np);
	}

	/*
	 * Erase the current page label; this will cause an exposure
	 * event later which will in turn cause us to draw the new page
	 * label. Likewise, erase the change bar.
	 */
	ClearPageLabel (w);
	ClearChangeBar (w);

	/*
	 * Mark the new page as "current" after issuing the callbacks
	 * above, so that Layout will ignore any geometry requests for
	 * the new page while in the callback.
	 */
	CATEGORY_P(w).current_page = new_page;
	old->set = False;
	new->set = True;
	SetValue (CATEGORY_P(w).page_choice, XtNitemsTouched, (XtArgVal)True);

	/*
	 * Call Layout to fix the geometry for the new page. Need to
	 * unmap the old page before calling Layout and map the new page
	 * after calling Layout to minimize the visual rearrangements.
	 * Layout must be called now, since we never lay out a page until
	 * it becomes visible.
	 */
	XtSetMappedWhenManaged (old_w, False);
	OlSimpleLayoutWidget (w, True, False);
	XtSetMappedWhenManaged (new_w, True);

	return;
} /* SetPage */

/**
 ** FindChild()
 **/

static int
#if	OlNeedFunctionPrototypes
FindChild (
	Widget			w,
	Widget			child
)
#else
FindChild (w, child)
	Widget			w;
	Widget			child;
#endif
{
	int			i;

	for (i = 0; i < CATEGORY_P(w).page_list_size; i++)
		if (child == (Widget)(CATEGORY_P(w).page_list[i].user_data))
			return (i);
	return (-1);
} /* FindChild */

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


	v.foreground = CATEGORY_P(w).font_color;
	v.font       = CATEGORY_P(w).category_font->fid;
	v.background = CORE_P(w).background_pixel;
	CATEGORY_P(w).category_font_GC = XtGetGC(
		w, GCForeground | GCFont | GCBackground, &v
	);

	v.foreground = CATEGORY_P(w).font_color;
	v.font       = CATEGORY_P(w).font->fid;
	v.background = CORE_P(w).background_pixel;
	CATEGORY_P(w).font_GC = XtGetGC(
		w, GCForeground | GCFont | GCBackground, &v
	);

	return;
} /* GetGCs */

/**
 ** FreeGCs()
 **/

static void
#if	OlNeedFunctionPrototypes
FreeGCs (
	Widget			w
)
#else
FreeGCs (w)
	Widget			w;
#endif
{
	XtReleaseGC (w, CATEGORY_P(w).category_font_GC);
	XtReleaseGC (w, CATEGORY_P(w).font_GC);
	return;
} /* FreeGCs */

/**
 ** GetValue()
 **/

static void
#if	OlNeedFunctionPrototypes
GetValue (
	Widget			w,
	String			name,
	XtArgVal		value
)
#else
GetValue (w, name, value)
	Widget			w;
	String			name;
	XtArgVal		value;
#endif
{
	Arg			arg;

	arg.name = name;
	arg.value = value;
	XtGetValues (w, &arg, 1);
	return;
} /* GetValue */

/**
 ** SetValue()
 **/

static void
#if	OlNeedFunctionPrototypes
SetValue (
	Widget			w,
	String			name,
	XtArgVal		value
)
#else
SetValue (w, name, value)
	Widget			w;
	String			name;
	XtArgVal		value;
#endif
{
	Arg			arg;

	arg.name = name;
	arg.value = value;
	XtSetValues (w, &arg, 1);
	return;
} /* SetValue */

/**
 ** IsInternalChild()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
IsInternalChild (
	Widget			w,
	Widget			child
)
#else
IsInternalChild (w, child)
	Widget			w;
	Widget			child;
#endif
{
	/*
	 * If we don't yet have enough children to fill the _1ST child
	 * slot, then by definition this must be an internal child.
	 * Remember: insert_child isn't called until after constraint
	 * initialize is called, so the very first time here num_children
	 * will be zero.
	 */
	return (
		COMPOSITE_P(w).num_children < _1ST
	     || child == COMPOSITE_P(w).children[_AMB]
	     || child == COMPOSITE_P(w).children[_LCA]
	     || child == COMPOSITE_P(w).children[_FTR]
	);
} /* IsInternalChild */
