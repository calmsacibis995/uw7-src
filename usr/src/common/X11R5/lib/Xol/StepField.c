#ifndef	NOIDENT
#ident	"@(#)textfield:StepField.c	1.5"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/StepFieldP.h"

/*
 * Private routines:
 */

static Boolean		Step OL_ARGS((
	Widget			w,
	OlSteppedReason		reason,
	Cardinal		count
));

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(StepFieldRec,F)

    /*
     * The OPEN LOOK spec doesn't say whether numeric fields can scroll.
     * I think they shouldn't (too confusing for the user).
     */
    {	/* SGI */
	XtNcanScroll, XtCCanScroll,
	XtRBoolean, sizeof(Boolean), offset(textfield.can_scroll),
	XtRImmediate, (XtPointer)False
    },

    {	/* SI */
	XtNstepped, XtCCallback,
	XtRCallback, sizeof(XtCallbackProc), offset(stepfield.stepped),
	XtRCallback, (XtPointer)0
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

StepFieldClassRec	stepFieldClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */ (WidgetClass)         &textFieldClassRec,
/* class_name           */                       "StepField",
/* widget_size          */                       sizeof(StepFieldRec),
/* class_initialize     */ (XtProc)              0,
/* class_part_init   (D)*/ (XtWidgetClassProc)   0,
/* class_inited         */                       False,
/* initialize        (D)*/ (XtInitProc)          0,
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
/* destroy           (U)*/ (XtWidgetProc)        0,
/* resize            (I)*/                       XtInheritResize,
/* expose            (I)*/                       XtInheritExpose,
/* set_values        (D)*/ (XtSetValuesFunc)     0,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/ (XtArgsProc)          0,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       XtInheritQueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Primitive class:
	 */
	{
/* focus_on_select      */			 False, /* textEdit class
                                                  already has this to True */
/* highlight_handler (I)*/                       XtInheritHighlightHandler,
/* traversal_handler (I)*/                       XtInheritTraversalHandler,
/* register_focus       */                       XtInheritRegisterFocus,
/* activate          (I)*/                       XtInheritActivateFunc,
/* event_procs          */ (OlEventHandlerList)  0,
/* num_event_procs      */ (Cardinal)            0,
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { (_OlDynResource *)0, (Cardinal)0 },
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
/* scroll            (I)*/                       XtInheritScrollProc,
/* step              (I)*/                       Step,
/* extension            */ (XtPointer)           0,
	}
};

WidgetClass	stepFieldWidgetClass = (WidgetClass)&stepFieldClassRec;

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
	if (XtHasCallbacks(w, XtNstepped) == XtCallbackHasSome) {
		OlTextFieldStepped	stepped;

		stepped.reason = reason;
		stepped.count = count;
		XtCallCallbacks (w, XtNstepped, &stepped);
	}
	/*
	 * Check the callback list again, in case the client removed it.
	 */
	return (XtHasCallbacks(w, XtNstepped) == XtCallbackHasSome);
} /* Step */
