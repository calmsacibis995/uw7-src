#ident	"@(#)R5Xt:Destroy.c	1.3"
/* $XConsortium: Destroy.c,v 1.44 92/01/03 18:03:59 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "IntrinsicI.h"

struct _DestroyRec {
    int dispatch_level;
    Widget widget;
};

static void Recursive(widget, proc)
    Widget       widget;
    XtWidgetProc proc;
{
    register int    i;
    CompositePart   *cwp;

    /* Recurse down normal children */
    if (XtIsComposite(widget)) {
	cwp = &(((CompositeWidget) widget)->composite);
	for (i = 0; i < cwp->num_children; i++) {
	    Recursive(cwp->children[i], proc);
	}
    }

    /* Recurse down popup children */
    if (XtIsWidget(widget)) {
	for (i = 0; i < widget->core.num_popups; i++) {
	    Recursive(widget->core.popup_list[i], proc);
	}
    }

    /* Finally, apply procedure to this widget */
    (*proc) (widget);  
} /* Recursive */

static void Phase1Destroy (widget)
    Widget    widget;
{
    widget->core.being_destroyed = TRUE;
} /* Phase1Destroy */

static void Phase2Callbacks(widget)
    Widget    widget;
{
    if (widget->core.destroy_callbacks != NULL) {
	XtCallCallbackList(widget,
			   widget->core.destroy_callbacks, (XtPointer) NULL);
    }
} /* Phase2Callbacks */

static void Phase2Destroy(widget)
    register Widget widget;
{
    register WidgetClass	    class;
    register ConstraintWidgetClass  cwClass;

    /* Call constraint destroy procedures */
    /* assert: !XtIsShell(w) => (XtParent(w) != NULL) */
    if (!XtIsShell(widget) && XtIsConstraint(XtParent(widget))) {
	cwClass = (ConstraintWidgetClass)XtParent(widget)->core.widget_class;
	for (;;) {
	    if (cwClass->constraint_class.destroy != NULL)
		(*(cwClass->constraint_class.destroy)) (widget);
            if (cwClass == (ConstraintWidgetClass)constraintWidgetClass) break;
            cwClass = (ConstraintWidgetClass) cwClass->core_class.superclass;
	}
    }

    /* Call widget destroy procedures */
    for (class = widget->core.widget_class;
	 class != NULL; 
	 class = class->core_class.superclass) {
	if ((class->core_class.destroy) != NULL)
	    (*(class->core_class.destroy))(widget);
    }
} /* Phase2Destroy */

static Boolean IsDescendant(widget, root)
    register Widget widget, root;
{
    while ((widget = XtParent(widget)) != root) {
	if (widget == NULL) return False;
    }
    return True;
}

static void XtPhase2Destroy (widget)
    register Widget widget;
{
    Display	    *display;
    Window	    window;
    Widget          parent;
    XtAppContext    app = XtWidgetToApplicationContext(widget);
    Widget	    outerInPhase2Destroy = app->in_phase2_destroy;
    int		    starting_count = app->destroy_count;
    Boolean	    isPopup = False;

    /* invalidate focus trace cache for this display */
    _XtGetPerDisplay(XtDisplayOfObject(widget))->pdi.traceDepth = 0;

    parent = widget->core.parent;

    if (parent && XtIsWidget(parent) && parent->core.num_popups) {
	int i;
	for (i = 0; i < parent->core.num_popups; i++) {
	    if (parent->core.popup_list[i] == widget) {
		isPopup = True;
		break;
	    }
	}
    }

    if (!isPopup && parent && XtIsComposite(parent)) {
	XtWidgetProc delete_child =
	    ((CompositeWidgetClass) parent->core.widget_class)->
		composite_class.delete_child;
        if (XtIsRectObj(widget)) {
       	    XtUnmanageChild(widget);
        }
	if (delete_child == NULL) {
	    String param = parent->core.widget_class->core_class.class_name;
	    Cardinal num_params = 1;
	    XtAppWarningMsg(XtWidgetToApplicationContext(widget),
		"invalidProcedure","deleteChild",XtCXtToolkitError,
		"null delete_child procedure for class %s in XtDestroy",
		&param, &num_params);
	} else {
	    (*delete_child) (widget);
	}
    }

    /* widget is freed in Phase2Destroy, so retrieve window now.
     * Shells destroy their own windows, to prevent window leaks in
     * popups; this test is practical only when XtIsShell() is cheap.
     */
    if (XtIsShell(widget) || !XtIsWidget(widget)) {
	window = 0;
#ifdef lint
	display = 0;
#endif
    }
    else {
	display = XtDisplay(widget);
	window = widget->core.window;
    }

    Recursive(widget, Phase2Callbacks);
    if (app->destroy_count > starting_count) {
	int i = starting_count;
	while (i < app->destroy_count) {

	    DestroyRec * dr = app->destroy_list + i;
	    if (IsDescendant(dr->widget, widget)) {
		Widget descendant = dr->widget;
		register int j;
		app->destroy_count--;
		for (j = app->destroy_count - i; --j >= 0; dr++)
		    *dr = *(dr + 1);
		XtPhase2Destroy(descendant);
	    }
	    else i++;
	}
    }

    app->in_phase2_destroy = widget;
    Recursive(widget, Phase2Destroy);
    app->in_phase2_destroy = outerInPhase2Destroy;

    if (isPopup) {
	int i;
	for (i = 0; i < parent->core.num_popups; i++)
	    if (parent->core.popup_list[i] == widget) {
		parent->core.num_popups--;
		while (i < parent->core.num_popups) {
		    parent->core.popup_list[i] = parent->core.popup_list[i+1];
		    i++;
		}
		break;
	    }
    }

    /* %%% the following parent test hides a more serious problem,
       but it avoids breaking those who depended on the old bug
       until we have time to fix it properly. */

    if (window && (parent == NULL || !parent->core.being_destroyed))
	XDestroyWindow(display, window);
} /* XtPhase2Destroy */


void _XtDoPhase2Destroy(app, dispatch_level)
    XtAppContext app;
    int dispatch_level;
{
    /* Phase 2 must occur in fifo order.  List is not necessarily
     * contiguous in dispatch_level.
     */

    int i = 0;
    while (i < app->destroy_count) {

	/* XtPhase2Destroy can result in calls to XtDestroyWidget,
	 * and these could cause app->destroy_list to be reallocated.
	 */

	DestroyRec* dr = app->destroy_list + i;
	if (dr->dispatch_level >= dispatch_level)  {
	    Widget w = dr->widget;
	    register int j;
	    app->destroy_count--;
	    for (j = app->destroy_count - i; --j >=0; dr++)
		*dr = *(dr + 1);
	    XtPhase2Destroy(w);
	}
	else i++;
    }
}


void XtDestroyWidget (widget)
    Widget    widget;
{
    XtAppContext app = XtWidgetToApplicationContext(widget);
    register DestroyRec *dr, *dr2;

    if (widget->core.being_destroyed) return;

    Recursive(widget, Phase1Destroy);

    if (app->in_phase2_destroy &&
	IsDescendant(widget, app->in_phase2_destroy))
    {
	XtPhase2Destroy(widget);
	return;
    }

    if (app->destroy_count == app->destroy_list_size) {
	app->destroy_list_size += 10;
	app->destroy_list = (DestroyRec*)
	    XtRealloc( (char*)app->destroy_list,
		       (unsigned)sizeof(DestroyRec)*app->destroy_list_size
		      );
    }
    dr = app->destroy_list + app->destroy_count++;
    dr->dispatch_level = app->dispatch_level;
    dr->widget = widget;

    if (app->dispatch_level > 1) {
	int i;
	for (i = app->destroy_count - 1; i;) {
	    /* this handles only one case of nesting difficulties */
 	    dr = app->destroy_list + (--i);
 	    if (dr->dispatch_level < app->dispatch_level &&
 		IsDescendant(dr->widget, widget)) {
 	        dr2 = app->destroy_list + (app->destroy_count-1);
 		dr2->dispatch_level = dr->dispatch_level;
  		break;
  	    }
  	}
    }

    if (_XtSafeToDestroy(app)) {
	app->dispatch_level = 1; /* avoid nested _XtDoPhase2Destroy */
	_XtDoPhase2Destroy(app, 0);
	app->dispatch_level = 0;
    }
	
} /* XtDestroyWidget */
