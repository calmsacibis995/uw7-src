#ifndef	NOIDENT
#ident	"@(#)olmisc:References.c	1.2"
#endif

#include "X11/IntrinsicP.h"

#include "Xol/OpenLook.h"
#include "Xol/Error.h"

#define CORE_P(w) ((Widget)(w))->core

/**
 ** OlResolveReference()
 **/

void
#if	OlNeedFunctionPrototypes
OlResolveReference (
	Widget			w,
	Widget *		ref_widget,
	String *		ref_name,
	OlReferenceScope	scope,
	Widget			(*find_widget) OL_ARGS((
		Widget			root,
		OLconst char *		name
	)),
	Widget			root
)
#else
OlResolveReference (w, ref_widget, ref_name, scope, find_widget, root)
	Widget			w;
	Widget *		ref_widget;
	String *		ref_name;
	OlReferenceScope	scope;
	Widget			(*find_widget)();
	Widget			root;
#endif 
{   
	Widget			parent = XtParent(w);


	/*
	 * Resolve the reference widget's ID if we don't have it yet,
	 * unless the widget is not managed.
	 */
	if (*ref_widget)
		return;
	if ((scope & OlReferenceManaged) && !XtIsManaged(w))
		return;

	if (!root)
		root = parent;
	if (!find_widget)
		find_widget = XtNameToWidget;

	if (*ref_name) {
		*ref_widget = (*find_widget)(root, *ref_name);
		if (!*ref_widget) {
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				"badReference", "unknownWidget",
				OleCOlToolkitWarning,
				"Widget %s: refName (%s) doesn't name a known widget",
				XtName(w), *ref_name
			);
		}
		switch (scope) {
		case OlReferenceSibling:
		case OlReferenceManagedSibling:
			if (*ref_widget == parent) {
				OlVaDisplayWarningMsg (
					XtDisplay(w),
					"badReference", "isParent",
					OleCOlToolkitWarning,
					"Widget %s: refWidget (%s) cannot be parent",
					XtName(w), *ref_name
				);
				*ref_widget = 0;
			}
			break;
		}
		if (!*ref_widget) {
			XtFree (*ref_name);
			*ref_name = 0;
		}
	}

	/*
	 * Provide a default if we know a good one.
	 */
	if (!*ref_widget)
		switch (scope) {
		case OlReferenceParentOrSibling:
		case OlReferenceManagedParentOrSibling:
			*ref_widget = parent;
			break;
		}

	return;
} /* OlResolveReference */

/**
 ** OlCheckReference()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckReference (
	Widget			w,
	Widget *		ref_widget,
	String *		ref_name,
	OlReferenceScope	scope,
	Widget			(*find_widget) OL_ARGS((
		Widget			root,
		OLconst char *		name
	)),
	Widget			root
)
#else
OlCheckReference (w, ref_widget, ref_name, scope, find_widget, root)
	Widget			w;
	Widget *		ref_widget;
	String *		ref_name;
	OlReferenceScope	scope;
	Widget			(*find_widget)();
	Widget			root;
#endif 
{   
	Widget			parent = XtParent(w);


	/*
	 * Here we check for two possible errors: (1) Both widget ID
	 * and widget name are given, but they are inconsistent.
	 * (2) A given widget ID'd isn't within the scope.
	 * Other checks are deferred until OlResolveReference,
	 * to allow the client to create the widgets in an order
	 * different from their reference chain.
	 */

	if (*ref_widget && *ref_name) {
		if (!root)
			root = parent;
		if (!find_widget)
			find_widget = XtNameToWidget;
		if (*ref_widget != (*find_widget)(root, *ref_name)) {
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				"badReference", "inconsistent",
				OleCOlToolkitWarning,
				"Widget %s: refName (%s) differs from refWidget (%s)",
				XtName(w), *ref_name, XtName(*ref_widget)
			);
			XtFree (*ref_name);
			*ref_name = 0;
		}
	}

	if (*ref_widget) {
		switch (scope) {
		case OlReferenceSibling:
		case OlReferenceManagedSibling:
			if (*ref_widget == parent) {
				OlVaDisplayWarningMsg (
					XtDisplay(w),
					"badReference", "isParent",
					OleCOlToolkitWarning,
					"Widget %s: refWidget (%s) cannot be parent",
					XtName(w), XtName(*ref_widget)
				);
				*ref_widget = 0;
			}
			if (XtParent(*ref_widget) != parent) {
				OlVaDisplayWarningMsg (
					XtDisplay(w),
					"badReference", "notSibling",
					"Widget %s: refWidget (%s) must be sibling",
					XtName(w), XtName(*ref_widget)
				);
				*ref_widget = 0;
			}
			break;
		case OlReferenceParentOrSibling:
		case OlReferenceManagedParentOrSibling:
			if (*ref_widget != parent && XtParent(*ref_widget) != parent) {
				OlVaDisplayWarningMsg (
					XtDisplay(w),
					"badReference", "notInFamily",
					OleCOlToolkitWarning,
					"Widget %s: refWidget (%s) is neither sibling nor parent",

					XtName(w), XtName(*ref_widget)
				);
				*ref_widget = 0;
			}
			break;
		}
		if (!*ref_widget)
			if (*ref_name) {
				XtFree (*ref_name);
				*ref_name = 0;
			}
	}

	return;
} /* OlCheckReference */
