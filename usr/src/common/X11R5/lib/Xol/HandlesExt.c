#ifndef	NOIDENT
#ident	"@(#)handles:HandlesExt.c	1.13"
#endif

#include "string.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/ManagerP.h"
#include "Xol/HandlesP.h"
#include "Xol/HandlesExP.h"
#include "Xol/Error.h"

#define ClassName HandlesExt
#include <Xol/NameDefs.h>

/*
 * Convenient macros:
 */

#define GetHandlesExtension(WC) \
	GetHandlesExtension_QInherited(WC, (Boolean*)0)

#define INIT(pVar) \
	_OlArrayInitialize(pVar, 5, 25, HandlesWidgetExtensionCompare)

#define STREQU(A,B) (strcmp((A),(B)) == 0)

	/*
	 * Stolen from Xt/IntrinsicI.h:
	 */
#define XtStackAlloc(size, stack_cache_array) \
	(size <= sizeof(stack_cache_array)?				\
		  (XtPointer)stack_cache_array : XtMalloc((unsigned)size))
#define XtStackFree(pointer, stack_cache_array) \
	(void)((XtPointer)(pointer) != (XtPointer)stack_cache_array?	\
			XtFree((XtPointer)pointer),0 : 0)

/*
 * Private types:
 */

typedef struct ExtensionResourceRec {
	XtEnum			selected; /* allow indeterminate state */
}			ExtensionResourceRec;

/*
 * Locale routines:
 */

static void		ClassPartInitialize OL_ARGS((
	WidgetClass		wc
));
static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Destroy OL_ARGS((
	Widget			w
));
static void		CheckSelected OL_ARGS((
	Widget			w,
	ArgList			args,
	Cardinal *		num_args,
	Boolean			initialize
));
static Widget		GetHandlesWidget OL_ARGS((
	Widget			parent
));
static void		RealizeHandles OL_ARGS((
	Widget			w,
	Widget			handles
));
static void		UnrealizeCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data	/*NOTUSED*/
));
static void		GetHandlesExtensionPointers OL_ARGS((
	Widget			w,
	HandlesCoreClassExtension * E,
	HandlesWidgetExtension * e
));
static HandlesCoreClassExtension GetHandlesExtension_QInherited OL_ARGS((
	WidgetClass		wc,
	Boolean *		inherited
));
static HandlesWidgetExtension GetHandlesWidgetExtension OL_ARGS((
	HandlesCoreClassExtension E,
	Widget			w
));
static int		FindWidget OL_ARGS((
	HandlesCoreClassExtension E,
	Widget			w
));
static int		HandlesWidgetExtensionCompare OL_ARGS((
	XtPointer		pA,
	XtPointer		pB
));

/*
 * Resources:
 */

static XtResource	ExtensionResources[] = {
#define offset(F) XtOffsetOf(ExtensionResourceRec, F)

    {	/* SGI */
	XtNselected, XtCSelected,
	XtRBoolean, sizeof(Boolean), offset(selected),
	XtRImmediate, (XtPointer)XtUnspecifiedBoolean
    }

#undef	offset
};

/*
 * Private data:
 */

static struct save {
	XtWidgetClassProc	class_part_initialize;
	XtInitProc		initialize;
	XtWidgetProc		destroy;
}			save;

/*
 * Public data:
 */

XrmQuark		XtQHandlesCoreClassExtension = 0;

/**
 ** _OlInitializeHandlesCoreClassExtension()
 **/

void
#if	OlNeedFunctionPrototypes
_OlInitializeHandlesCoreClassExtension (
	void
)
#else
_OlInitializeHandlesCoreClassExtension ()
#endif
{
	/*
	 * We need to be "informed" when every class is initialized,
	 * when every widget and gadget is created, and when every widget
	 * and gadget is destroyed. We can get this by "enveloping"
	 * RectObj's class_part_initialize, initialize, and destroy
	 * methods.
	 */
#define ENVELOPE(METHOD,NEW) \
	save.METHOD = RECT_C(rectObjClass).METHOD;			\
	RECT_C(rectObjClass).METHOD = NEW

	ENVELOPE (class_part_initialize, ClassPartInitialize);
	ENVELOPE (initialize, Initialize);
	ENVELOPE (destroy, Destroy);
#undef	ENVELOPE

	XtQHandlesCoreClassExtension
			= XrmStringToQuark(XtNHandlesCoreClassExtension);

	return;
} /* _OlInitializeHandlesCoreClassExtension */

/**
 ** _OlDefaultRealize()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDefaultRealize (
	Widget			w,
	XtValueMask *		value_mask,
	XSetWindowAttributes *	attributes
)
#else
_OlDefaultRealize (w, value_mask, attributes)
	Widget			w;
	XtValueMask *		value_mask;
	XSetWindowAttributes *	attributes;
#endif
{
	XtRealizeProc		realize = CORE_C(SUPER_C(_OlClass(w))).realize;

	if (realize)
		(*realize) (w, value_mask, attributes);
	OlCheckRealize (w);
	return;
} /* _OlDefaultRealize */

/**
 ** _OlDefaultInsertChild()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDefaultInsertChild (
	Widget			w
)
#else
_OlDefaultInsertChild (w)
	Widget			w;
#endif
{
	XtWidgetProc		insert_child
		= COMPOSITE_C(SUPER_C(_OlClass(XtParent(w)))).insert_child;

	OlCheckInsertedChild (w);
	if (insert_child)
		(*insert_child) (w);
	return;
} /* _OlDefaultInsertChild */

/**
 ** _OlDefaultDeleteChild()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDefaultDeleteChild (
	Widget			w
)
#else
_OlDefaultDeleteChild (w)
	Widget			w;
#endif
{
	XtWidgetProc		delete_child
		= COMPOSITE_C(SUPER_C(_OlClass(XtParent(w)))).delete_child;

	OlCheckDeletedChild (w);
	if (delete_child)
		(*delete_child) (w);
	return;
} /* _OlDefaultDeleteChild */

/**
 ** _OlDefaultConstraintInitialize()
 **/

/*ARGSUSED*/
void
#if	OlNeedFunctionPrototypes
_OlDefaultConstraintInitialize (
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
)
#else
_OlDefaultConstraintInitialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	CheckSelected (new, args, num_args, True);
	return;
} /* _OlDefaultConstraintInitialize */

/**
 ** _OlDefaultConstraintSetValues()
 **/

/*ARGSUSED*/
Boolean
#if	OlNeedFunctionPrototypes
_OlDefaultConstraintSetValues (
	Widget			current,	/*NOTUSED*/
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
)
#else
_OlDefaultConstraintSetValues (current, request, new, args, num_args)
	Widget			current;
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	CheckSelected (new, args, num_args, False);
	return (False);
} /* _OlDefaultConstraintSetValues */

/**
 ** _OlDefaultConstraintGetValuesHook()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDefaultConstraintGetValuesHook (
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
)
#else
_OlDefaultConstraintGetValuesHook (w, args, num_args)
	Widget			w;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Widget			parent = XtParent(w);
	Widget *		p = 0;

	Cardinal		k = 0;

	HandlesCoreClassExtension E;
	HandlesWidgetExtension	e;

	ExtensionResourceRec	data;


	GetHandlesExtensionPointers (parent, &E, &e);
	if (!E || e->handles == w)
		return;

	/*
	 * MORE: Need to merge these two lists.
	 */
	if (e->handles)
		OlHandlesGetSelection (e->handles, &p, &k);
	else if (e->set_list) {
		p = &_OlArrayElement(e->set_list, 0);
		k = _OlArraySize(e->set_list);
	}

	if (p && k) {
		data.selected = False;
		while (k--)
			if (p[k] == w) {
				data.selected = True;
				break;
			}
		XtGetSubvalues (
			(XtPointer)&data,
			ExtensionResources, XtNumber(ExtensionResources),
			args, *num_args
		);
	}

	return;
} /* _OlDefaultConstraintGetValuesHook */

/**
 ** _OlDefaultUpdateHandles()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDefaultUpdateHandles (
	Widget			w,
	Widget			handles
)
#else
_OlDefaultUpdateHandles (w, handles)
	Widget			w;
	Widget			handles;
#endif
{
	Widget			stack[20];
	Widget			child;
	Widget *		panes;

	Cardinal		n;
	Cardinal		k = 0;


	panes = (Widget *)XtStackAlloc(
		COMPOSITE_P(w).num_children * sizeof(Widget), stack
	);
	FOR_EACH_MANAGED_CHILD (w, child, n)
		if (child != handles)
			panes[k++] = child;
	XtVaSetValues (
		handles,
		XtNpanes,    (XtArgVal)panes,
		XtNnumPanes, (XtArgVal)k,
		(String)0
	);

	/*
	 * The Handles widget copies the panes list, so we
	 * can free our copy.
	 */
	XtStackFree (panes, stack);

	return;
} /* _OlDefaultUpdateHandles */

/**
 ** OlUpdateHandles()
 **/

void
#if	OlNeedFunctionPrototypes
OlUpdateHandles (
	Widget			w
)
#else
OlUpdateHandles (w)
	Widget			w;
#endif
{
	HandlesCoreClassExtension E;
	HandlesWidgetExtension	e;

	GetHandlesExtensionPointers (w, &E, &e);
	if (E && E->update_handles && e->handles)
		(*E->update_handles) (w, e->handles);
	return;
} /* OlUpdateHandles */

/**
 ** OlCheckRealize()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckRealize (
	Widget			w
)
#else
OlCheckRealize (w)
	Widget			w;
#endif
{
	HandlesCoreClassExtension E;
	HandlesWidgetExtension	e;


	GetHandlesExtensionPointers (w, &E, &e);
	if (E && e->handles)
		RealizeHandles (w, e->handles);

	return;
} /* OlCheckRealize */

/**
 ** OlCheckInsertedChild()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckInsertedChild (
	Widget			w
)
#else
OlCheckInsertedChild (w)
	Widget			w;
#endif
{
	if (OlIsHandles(w)) {
		Widget			parent = XtParent(w);
		HandlesCoreClassExtension E;
		HandlesWidgetExtension	e;

		GetHandlesExtensionPointers (parent, &E, &e);
		if (E)
			e->handles = w;
		if (XtIsRealized(parent))
			RealizeHandles (parent, w);
	}
	return;
} /* OlCheckInsertedChild */

/**
 ** OlCheckDeletedChild()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckDeletedChild (
	Widget			w
)
#else
OlCheckDeletedChild (w)
	Widget			w;
#endif
{
	if (OlIsHandles(w)) {
		Widget			parent = XtParent(w);
		HandlesCoreClassExtension E;
		HandlesWidgetExtension	e;

		GetHandlesExtensionPointers (parent, &E, &e);
		if (E)
			e->handles = 0;
		/*
		 * Don't have to "unrealize" the Handles widget, because
		 * its destroy method will do the equivalent.
		 */
	}
	return;
} /* OlCheckDeletedChild */

/**
 ** OlIsHandles()
 **/

Boolean
#if	OlNeedFunctionPrototypes
OlIsHandles (
	Widget			w
)
#else
OlIsHandles (w)
	Widget			w;
#endif
{
	return (STREQU(CLASS(CORE_P(w).widget_class), "Handles"));
} /* OlIsHandles */

/**
 ** OlIsHandlesWidget()
 **/

Boolean
#if	OlNeedFunctionPrototypes
OlIsHandlesWidget (
	Widget			parent,
	Widget			w
)
#else
OlIsHandlesWidget (parent, w)
	Widget			parent;
	Widget			w;
#endif
{
	return (GetHandlesWidget(parent) == w);
} /* OlIsHandlesWidget */

/**
 ** OlHandlesBorderThickness()
 **/

Dimension
#if	OlNeedFunctionPrototypes
OlHandlesBorderThickness (
	Widget			child,
	OlDefine		orientation
)
#else
OlHandlesBorderThickness (child, orientation)
	Widget			child;
	OlDefine		orientation;
#endif
{
	Widget			w = XtParent(child);

	HandlesCoreClassExtension E;
	HandlesWidgetExtension	e;

	Dimension *		thickness;

	String			resource;


	GetHandlesExtensionPointers (w, &E, &e);
	if (!E || !e->handles)
		return (0);

	if (orientation == OL_HORIZONTAL) {
		thickness = &e->handles_border.horz;
		resource = XtNpaneBorderWidth;
	} else {
		thickness = &e->handles_border.vert;
		resource = XtNpaneBorderHeight;
	}
	if (!*thickness)
		XtVaGetValues (
			e->handles,
			resource, (XtArgVal)thickness,
			(String)0
		);
	return (*thickness);
} /* OlHandlesBorderThickness */

/**
 ** OlRealizeHandles()
 **/

void
#if	OlNeedFunctionPrototypes
OlRealizeHandles (
	Widget			w
)
#else
OlRealizeHandles (w)
	Widget			w;
#endif
{
	XtWidgetProc		realize;

	if (!OlIsHandles(w))
		w = GetHandlesWidget(w);
	if (w) {
		realize = HANDLES_C(XtClass(w)).realize;
		if (XtIsRealized(XtParent(w)) && realize)
			(*realize) (w);
	}
	return;
} /* OlRealizeHandles */

/**
 ** OlUnrealizeHandles()
 **/

void
#if	OlNeedFunctionPrototypes
OlUnrealizeHandles (
	Widget			w
)
#else
OlUnrealizeHandles (w)
	Widget			w;
#endif
{
	XtWidgetProc		unrealize;


	if (!OlIsHandles(w))
		w = GetHandlesWidget(w);
	if (w) {
		unrealize = HANDLES_C(XtClass(w)).unrealize;
		if (unrealize)
			(*unrealize) (w);
	}
	return;
} /* OlUnrealizeHandles */

/**
 ** OlClearHandles()
 **/

void
#if	OlNeedFunctionPrototypes
OlClearHandles (
	Widget			w
)
#else
OlClearHandles (w)
	Widget			w;
#endif
{
	XtWidgetProc		clear;

	if (!OlIsHandles(w))
		w = GetHandlesWidget(w);
	if (w) {
		clear = HANDLES_C(XtClass(w)).clear;
		if (XtIsRealized(w) && clear)
			(*clear) (w);
	}
	return;
} /* OlClearHandles */

/**
 ** OlLayoutHandles()
 **/

void
#if	OlNeedFunctionPrototypes
OlLayoutHandles (
	Widget			w
)
#else
OlLayoutHandles (w)
	Widget			w;
#endif
{
	static Boolean		recursion = False;

	XtWidgetProc		layout;


	/*
	 * The Handles widget will call OlLayoutHandles when given a new
	 * set of selected panes, so we need to protect against endless
	 * looping
	 */
	if (recursion)
		return;
	recursion = True;

	if (!OlIsHandles(w))
		w = GetHandlesWidget(w);
	if (w) {
		HandlesCoreClassExtension E;
		HandlesWidgetExtension	e;

		/*
		 * We've delayed setting the selection states as long
		 * as possible, to allow the panes to be created and
		 * identified to the Handles widget.
		 */
		GetHandlesExtensionPointers (XtParent(w), &E, &e);
		if (E && e->set_list) {
			OlHandlesSetSelection (
				w,
				&_OlArrayElement(e->set_list, 0),
				_OlArraySize(e->set_list),
				True
			);
			_OlArrayFree (e->set_list);
			e->set_list = 0;
		}

		layout = HANDLES_C(XtClass(w)).layout;
		if (XtIsRealized(w) && layout)
			(*layout) (w);
	}

	recursion = False;
	return;
} /* OlLayoutHandles */

/**
 ** OlHandlesSetSelection()
 **/

void
#if	OlNeedFunctionPrototypes
OlHandlesSetSelection (
	Widget			w,
	Widget *		panes,
	Cardinal		num_panes,
	Boolean			selected
)
#else
OlHandlesSetSelection (w, panes, num_panes, selected)
	Widget			w;
	Widget *		panes;
	Cardinal		num_panes;
	Boolean			selected;
#endif
{
	OlHandlesSetSelectionProc set_selection;

	if (!OlIsHandles(w))
		w = GetHandlesWidget(w);
	if (w) {
		set_selection = HANDLES_C(XtClass(w)).set_selection;
		if (set_selection)
			(*set_selection) (w, panes, num_panes, selected);
	}
	return;
} /* OlHandlesSetSelection */

/**
 ** OlHandlesGetSelection()
 **/

void
#if	OlNeedFunctionPrototypes
OlHandlesGetSelection (
	Widget			w,
	Widget **		panes,
	Cardinal *		num_panes
)
#else
OlHandlesGetSelection (w, panes, num_panes, selected)
	Widget			w;
	Widget **		panes;
	Cardinal *		num_panes;
#endif
{
	OlHandlesGetSelectionProc get_selection;

	if (!OlIsHandles(w))
		w = GetHandlesWidget(w);
	if (w) {
		get_selection = HANDLES_C(XtClass(w)).get_selection;
		if (XtIsRealized(w) && get_selection)
			(*get_selection) (w, panes, num_panes);
	}
	return;
} /* OlHandlesGetSelection */

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
	Boolean				inherited;

	HandlesCoreClassExtension	E;


	if (save.class_part_initialize)
		(*save.class_part_initialize)(wc);

	/*
	 * If the extension was inherited from a superclass, don't
	 * initialize the extension (it's already been initialized!)
	 */
	E = GetHandlesExtension_QInherited(wc, &inherited);
	if (E && !inherited)
		INIT (&E->widgets);

	return;
} /* ClassPartInitialize */

/**
 ** Initialize()
 **/

static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,
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
	HandlesCoreClassExtension	E = GetHandlesExtension(XtClass(new));
	HandlesWidgetExtensionRec e;

	if (save.initialize)
		(*save.initialize)(request, new, args, num_args);
	if (E) {
		e.self = new;
		e.handles = 0;
		e.set_list = 0;
		e.handles_border.horz = 0;
		e.handles_border.vert = 0;
		_OlArrayOrderedInsert (&E->widgets, e);
	}

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
	HandlesCoreClassExtension E = GetHandlesExtension(XtClass(w));

	if (save.destroy)
		(*save.destroy)(w);
	if (E) {
		/*
		 * Careful! _OlArrayDelete is a macro that evaluates its
		 * arguments more than once.
		 */
		int i = FindWidget(E, w);
		_OlArrayDelete (&E->widgets, i);
		if (!_OlArraySize(&E->widgets)) {
			_OlArrayFree (&E->widgets);
			INIT (&E->widgets);
		}
	}
	return;
} /* Destroy */

/**
 ** CheckSelected()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckSelected (
	Widget			w,
	ArgList			args,
	Cardinal *		num_args,
	Boolean			initialize
)
#else
CheckSelected (w, args, num_args, initialize)
	Widget			w;
	ArgList			args;
	Cardinal *		num_args;
	Boolean			initialize;
#endif
{
	Widget			parent = XtParent(w);

	HandlesCoreClassExtension E;
	HandlesWidgetExtension	e;

	ExtensionResourceRec	data;


	GetHandlesExtensionPointers (parent, &E, &e);
	if (!E || e->handles == w)
		return;

	data.selected = XtUnspecifiedBoolean;
	if (initialize)
		XtGetSubresources (
			w,
			(XtPointer)&data,
			(String)0, (String)0,	/* no name/class */
			ExtensionResources, XtNumber(ExtensionResources),
			args, *num_args
		);
	else
		XtSetSubvalues (
			(XtPointer)&data,
			ExtensionResources, XtNumber(ExtensionResources),
			args, *num_args
		);

	switch (data.selected) {
	case True:
		if (!e->set_list)
			_OlArrayAllocate (WidgetArray, e->set_list, 10, 10);
		_OlArrayUniqueAppend (e->set_list, w);
		break;
	case False:
		if (e->set_list) {
			int i = _OlArrayFind(e->set_list, w);
			if (i != _OL_NULL_ARRAY_INDEX)
				_OlArrayDelete (e->set_list, i);
		}
		break;
	case XtUnspecifiedBoolean:
		break;
	}

	return;
} /* CheckSelected */

/**
 ** GetHandlesWidget()
 **/

static Widget
#if	OlNeedFunctionPrototypes
GetHandlesWidget (
	Widget			parent
)
#else
GetHandlesWidget (parent)
	Widget			parent;
#endif
{
	HandlesCoreClassExtension E;
	HandlesWidgetExtension	e;

	GetHandlesExtensionPointers (parent, &E, &e);
	return (E? e->handles : 0);
} /* GetHandlesWidget */

/**
 ** RealizeHandles()
 **/

static void
#if	OlNeedFunctionPrototypes
RealizeHandles (
	Widget			w,
	Widget			handles
)
#else
RealizeHandles (w, handles)
	Widget			w;
	Widget			handles;
#endif
{
	OlRealizeHandles (handles);
	if (XtHasCallbacks(w, XtNunrealizeCallback) != XtCallbackNoList)
		XtAddCallback (
			w, XtNunrealizeCallback, UnrealizeCB, (XtPointer)0
		);
	return;
} /* RealizeHandles */

/**
 ** UnrealizeCB()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
UnrealizeCB (
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data	/*NOTUSED*/
)
#else
UnrealizeCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	HandlesCoreClassExtension E;
	HandlesWidgetExtension	e;


	/*
	 * No need to check if e is valid since we wouldn't be here
	 * otherwise. See _OlDefaultRealize.
	 */
	GetHandlesExtensionPointers (w, &E, &e);
	XtRemoveCallback (w, XtNunrealizeCallback, UnrealizeCB, (XtPointer)0);
	OlUnrealizeHandles (e->handles);

	return;
} /* UnrealizeCB */

/**
 ** GetHandlesExtensionPointers()
 **/

static void
#if	OlNeedFunctionPrototypes
GetHandlesExtensionPointers (
	Widget			w,
	HandlesCoreClassExtension * E,
	HandlesWidgetExtension * e
)
#else
GetHandlesExtensionPointers (w, E, e)
	Widget			w;
	HandlesCoreClassExtension * E;
	HandlesWidgetExtension * e;
#endif
{
	*E = GetHandlesExtension(XtClass(w));
	*e = (*E? GetHandlesWidgetExtension(*E, w) : 0);
	return;
} /* GetHandlesExtensionPointers */

/**
 ** GetHandlesExtension_QInherited()
 **/

static HandlesCoreClassExtension
#if	OlNeedFunctionPrototypes
GetHandlesExtension_QInherited (
	WidgetClass		wc,
	Boolean *		inherited
)
#else
GetHandlesExtension_QInherited (wc, inherited)
	WidgetClass		wc;
	Boolean *		inherited;
#endif
{
	HandlesCoreClassExtension E;


	/*
	 * Start off assuming the extension will not be inhertied.
	 * WARNING: Shortly we will recursively call this routine--don't
	 * pass inherited again!
	 */
	if (inherited)
		*inherited = False;

	E = (HandlesCoreClassExtension)_OlGetClassExtension(
		(OlClassExtension)CORE_C(wc).extension,
		XtQHandlesCoreClassExtension,
		OlHandlesCoreClassExtensionVersion
	);

	/*
	 * We interpret a missing extension to mean this class wants
	 * to inherit its superclass' extension.
	 */
	if ((!E || E->update_handles == XtInheritUpdateHandles) && SUPER_C(wc)) {
		if (inherited)
			*inherited = True;
		E = GetHandlesExtension(SUPER_C(wc));
	}

	return (E && E->update_handles != XtInheritUpdateHandles? E : 0);
} /* GetHandlesExtension_QInherited */

/**
 ** GetHandlesWidgetExtension()
 **/

static HandlesWidgetExtension
#if	OlNeedFunctionPrototypes
GetHandlesWidgetExtension (
	HandlesCoreClassExtension E,
	Widget			w
)
#else
GetHandlesWidgetExtension (E, w)
	HandlesCoreClassExtension E;
	Widget			w;
#endif
{
	int i = FindWidget(E, w);
	return (&_OlArrayElement(&E->widgets, i));
} /* GetHandlesWidgetExtension */

/**
 ** FindWidget()
 **/

static int
#if	OlNeedFunctionPrototypes
FindWidget (
	HandlesCoreClassExtension E,
	Widget			w
)
#else
FindWidget (E, w)
	HandlesCoreClassExtension E;
	Widget			w;
#endif
{
	HandlesWidgetExtensionRec find;
	int			i;

	find.self = w;
	i = _OlArrayFind(&E->widgets, find);
	if (i == _OL_NULL_ARRAY_INDEX)
		OlVaDisplayErrorMsg (
			XtDisplayOfObject(w),
			"internalError", "noHandlesExtension",
			OleCOlToolkitError,
			"Widget %s: No Handles Extension for this widget",
			XtName(w)
		);
		/*NOTREACHED*/

	return (i);
} /* FindWidget */

/**
 ** HandlesWidgetExtensionCompare()
 **/

static int
#if	OlNeedFunctionPrototypes
HandlesWidgetExtensionCompare (
	XtPointer		pA,
	XtPointer		pB
)
#else
HandlesWidgetExtensionCompare (pA, pB)
	XtPointer		pA;
	XtPointer		pB;
#endif
{
#define A ((HandlesWidgetExtension)pA)
#define B ((HandlesWidgetExtension)pB)

	return ((int)(A->self - B->self));

#undef	A
#undef	B
} /* HandlesWidgetExtensionCompare */
