#ifndef	NOIDENT
#ident	"@(#)textfield:IntegerFie.c	1.12"
#endif

#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/IntegerFiP.h"
#include "Xol/Error.h"

#define ClassName IntegerField
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&integerFieldClassRec)
#define superClass	((WidgetClass)&textFieldClassRec)
#define className	"IntegerField"

/*
 * Private types:
 */

typedef enum ComplaintType {
	_Complain,
	_Beep,
	_DoNothing
}			ComplaintType;

/*
 * Convenient macros:
 */

#define TEXTEDIT_P(w) ((TextEditWidget)(w))->textedit

#define STREQU(A,B) (strcmp((A),(B)) == 0)

#define NumberOfDigits(TYPE) \
	(sizeof(TYPE) == 2? 5 : (sizeof(TYPE) == 4? 10 : 20))

#define GetString(W) \
	GetTextBufferLocation(TEXTEDIT_P(W).textBuffer, 0, (TextLocation *)0)

/*
 * Private routines:
 */

static void		Initialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
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
static Boolean		Step OL_ARGS((
	Widget			w,
	OlSteppedReason		reason,
	Cardinal		count
));
static void 		ModifyVerificationCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
));
static void 		VerificationCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
));
static void 		FormatStringValue OL_ARGS((
	Widget			w
));
static Boolean		KeepValueInRange OL_ARGS((
	Widget			w,
	ComplaintType		complaint
));
static void 		CheckRange OL_ARGS((
	Widget			w
));
static void		CallValueChanged OL_ARGS((
	Widget			w,
	OlTextVerifyReason	reason
));
static void		SetDefaultValueResource OL_ARGS((
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
));

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(IntegerFieldRec,F)

    /*
     * The OPEN LOOK spec doesn't say whether numeric fields can scroll.
     * I think they shouldn't (too confusing for the user).
     */
    {	/* SGI */
	XtNcanScroll, XtCCanScroll,
	XtRBoolean, sizeof(Boolean), offset(textfield.can_scroll),
	XtRImmediate, (XtPointer)False
    },

    {	/* SGI */
	XtNvalue, XtCValue,
	XtRInt, sizeof(int), offset(integerfield.value),
	XtRCallProc, (XtPointer)SetDefaultValueResource
    },
    {	/* SGI */
	XtNvalueMin, XtCValueMin,
	XtRInt, sizeof(int), offset(integerfield.value_min),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNvalueMax, XtCValueMax,
	XtRInt, sizeof(int), offset(integerfield.value_max),
	XtRImmediate, (XtPointer)100
    },
    {	/* SGI */
	XtNvalueGranularity, XtCValueGranularity,
	XtRInt, sizeof(int), offset(integerfield.value_granularity),
	XtRImmediate, (XtPointer)1
    },
    {	/* SI */
	XtNvalueChanged, XtCCallback,
	XtRCallback, sizeof(XtCallbackProc), offset(integerfield.value_changed),
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

IntegerFieldClassRec		integerFieldClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(IntegerFieldRec),
/* class_initialize     */ (XtProc)              0,
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
/* destroy           (U)*/ (XtWidgetProc)        0,
/* resize            (I)*/                       XtInheritResize,
/* expose            (I)*/                       XtInheritExpose,
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
/* extension            */ (XtPointer)           0
	},
	/*
	 * Primitive class:
	 */
	{
/* focus_on_select      */			 False, /* textEdit class
                                                  already set this to True */
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
	},
	/*
	 * IntegerField class:
	 */
	{
/* extension            */ (XtPointer)           0,
	}
};

WidgetClass		integerFieldWidgetClass = thisClass;

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
	ArgList			args;		/*NOTUSED*/
	Cardinal *		num_args;	/*NOTUSED*/
#endif
{
	CheckRange (new);
	(void)KeepValueInRange (new, _Complain);
	FormatStringValue (new);
	INTEGER_P(new).prev_value = INTEGER_P(new).value;

	XtAddCallback (new, XtNmodifyVerification, ModifyVerificationCB, (XtPointer)0);
	XtAddCallback (new, XtNverification, VerificationCB, (XtPointer)0);

	return;
} /* Initialize */

/**
 ** SetValues()
 **/

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
	Boolean			redisplay	= False;

#define DIFFERENT(F) (INTEGER_P(new).F != INTEGER_P(current).F)

	/* notice we compare the 'request' against the 'current' */
	if (TEXTEDIT_P(request).source != TEXTEDIT_P(current).source)
	    INTEGER_P(new).value = atoi(TEXTEDIT_P(request).source);

	if (DIFFERENT(value_min) || DIFFERENT(value_max))
		CheckRange (new);

	if (DIFFERENT(value) || DIFFERENT(value_min) || DIFFERENT(value_max)) {
		(void)KeepValueInRange (new, _Complain);
		FormatStringValue (new);
	}

	INTEGER_P(new).prev_value = INTEGER_P(new).value;

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

    for (i = 0; i < *num_args; i++)
	if (STREQU(args[i].name, XtNvalue)) {
	    INTEGER_P(w).value = atoi(GetString(w));
	    (void)KeepValueInRange (w, _DoNothing);
	    FormatStringValue (w);
	    *(int *)(args[i].value)= INTEGER_P(w).value;
	    break;
	}
} /* GetValuesHook() */

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
	Boolean			continue_to_step = True;

	Cardinal		n;

	int			original_value;

#define IN_RANGE(w) \
  (INTEGER_P(w).value_min <= INTEGER_P(w).value				\
                          && INTEGER_P(w).value <= INTEGER_P(w).value_max)

#if	defined(__STDC__)
# define STEP(W,COUNT,OP) \
	for (n = 0; IN_RANGE(W) && n < COUNT; n++)			\
		INTEGER_P(W).value OP##= INTEGER_P(W).value_granularity
#else
# define STEP(W,COUNT,OP) \
	for (n = 0; IN_RANGE(W) && n < COUNT; n++)			\
		INTEGER_P(W).value OP/**/= INTEGER_P(W).value_granularity
#endif

	/*
	 * The user may have started typing a new number before switching
	 * to the up/down buttons, so fetch it and check its range.
	 */
	original_value = INTEGER_P(w).value = atoi(GetString(w));
	(void)KeepValueInRange (w, _Beep);

	/*
	 * Update the value.
	 */
	switch (reason) {
	case OlSteppedIncrement:
		STEP (w, count, +);
		break;
	case OlSteppedDecrement:
		STEP (w, count, -);
		break;
	case OlSteppedToMaximum:
		INTEGER_P(w).value = INTEGER_P(w).value_max;
		break;
	case OlSteppedToMinimum:
		INTEGER_P(w).value = INTEGER_P(w).value_min;
		break;
	}
	continue_to_step = KeepValueInRange(w, _DoNothing);

	if (original_value != INTEGER_P(w).value) {
		FormatStringValue (w);
		CallValueChanged (w, OlTextFieldStep);
	}
	/*
	 * The value will be in range--if the client wants to change
	 * the value, it has to call our set_values to do so and it
	 * will keep the value in range. But we do the following anyway,
	 * to see if we're at a limit.
	 */
	continue_to_step = KeepValueInRange(w, _DoNothing);

	return (continue_to_step);
} /* Step */

/**
 ** ModifyVerificationCB
 **/

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

	Cardinal		n;


	if (cd->ok)
		for (n = 0; n < cd->text_length; n++) {
			char c = cd->text[n];
			if (!isdigit(c) && !isspace(c) && c != '-') {
				_OlBeepDisplay (w, 1);
				cd->ok = False;
				break;
			}
		}

	return;
} /* ModifyVerificationCB */

/**
 ** VerificationCB
 **/

static void 
#if	OlNeedFunctionPrototypes
VerificationCB (
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
)
#else
VerificationCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlTextFieldVerify *	cd = (OlTextFieldVerify *)call_data;

	INTEGER_P(w).value = atoi(GetString(w));
	(void)KeepValueInRange (w, _Beep);
	FormatStringValue (w);

	/*
	 * Tell the client, even though the value may not have changed.
	 * We want clients to use the XtNvalueChanged callback, so that we
	 * can ensure that the value is legal before calling the client.
	 * However, a reasonable client behavior is for it to act whenever
	 * the user presses the Return key. If we don't call the
	 * XtNvalueChanged callback every time we get here, reasonable
	 * clients may be forced to use the TextField's XtNverification
	 * callback.
	 */
	CallValueChanged (w, cd->reason);

	return;
} /* VerificationCB */

/**
 ** FormatStringValue()
 **/

static void 
#if	OlNeedFunctionPrototypes
FormatStringValue (
	Widget			w
)
#else
FormatStringValue (w)
	Widget			w;
#endif
{
	char			buf[NumberOfDigits(int) + 1];


#if	defined(DO_THIS)
	sprintf (buf, "%*d", NumberOfDigits(int), INTEGER_P(w).value);
#else
	sprintf (buf, "%d", INTEGER_P(w).value);
#endif

	/*
	 * Avoid a call to set_values methods, since we may have come
	 * here via a set_values.
	 */
	OlTextEditUpdate (w, False);
/*
	OlTextEditSetCursorPosition (
		w, 0, LastTextBufferPosition(TEXTEDIT_P(w).textBuffer), 0
	);
*/
	TEXTEDIT_P(w).cursorPosition =
	TEXTEDIT_P(w).selectStart = 0;
	TEXTEDIT_P(w).selectEnd = LastTextBufferPosition(TEXTEDIT_P(w).textBuffer);
	OlTextEditInsert (w, buf, strlen(buf));
	OlTextEditUpdate (w, True);

	return;
} /* FormatStringValue */

/**
 ** KeepValueInRange()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
KeepValueInRange (
	Widget			w,
	ComplaintType		complaint
)
#else
KeepValueInRange (w, complaint)
	Widget			w;
	ComplaintType		complaint;
#endif
{
	Boolean			out = False;


	if (INTEGER_P(w).value < INTEGER_P(w).value_min) {
		INTEGER_P(w).value = INTEGER_P(w).value_min;
		out = True;
	}
	if (INTEGER_P(w).value > INTEGER_P(w).value_max) {
		INTEGER_P(w).value = INTEGER_P(w).value_max;
		out = True;
	}
	if (out)
		switch (complaint) {
		case _Complain:
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				"badValue", "set",
				OleCOlToolkitWarning,
				"Widget %s: %s resource out of range",
				XtName(w), XtNvalue
			);
			break;
		case _Beep:
			_OlBeepDisplay (w, 1);
			break;
		}

	TEXTFIELD_P(w).can_decrement =
			INTEGER_P(w).value != INTEGER_P(w).value_min;
	TEXTFIELD_P(w).can_increment =
			INTEGER_P(w).value != INTEGER_P(w).value_max;

	return (!out);
} /* KeepValueInRange */

/**
 ** CheckRange()
 **/

static void 
#if	OlNeedFunctionPrototypes
CheckRange (
	Widget			w
)
#else
CheckRange (w)
	Widget			w;
#endif
{
	if (INTEGER_P(w).value_max < INTEGER_P(w).value_min) {
		int i = INTEGER_P(w).value_min;
		INTEGER_P(w).value_min = INTEGER_P(w).value_max;
		INTEGER_P(w).value_max = i;
		OlVaDisplayWarningMsg (
			XtDisplay(w),
			"badRange", "set",
			OleCOlToolkitWarning,
			"Widget %s: %s and %s resources inverted",
			XtName(w), XtNvalueMin, XtNvalueMax
		);
	}
	return;
} /* CheckRange */

/**
 ** CallValueChanged()
 **/

static void
#if	OlNeedFunctionPrototypes
CallValueChanged (
	Widget			w,
	OlTextVerifyReason	reason
)
#else
CallValueChanged (w, reason)
	Widget			w;
	OlTextVerifyReason	reason;
#endif
{
	OlIntegerFieldChanged	changed;

	/*
	 * Tell the client about the new value. If the client wants to
	 * change the value, it must do so via set_values.
	 */
	changed.value = INTEGER_P(w).value;
	changed.changed = (INTEGER_P(w).prev_value != INTEGER_P(w).value);
	changed.reason = reason;
	INTEGER_P(w).prev_value = INTEGER_P(w).value;
	XtCallCallbacks (w, XtNvalueChanged, &changed);

	return;
} /* CallValueChanged */

/**
 ** SetDefaultValueResource()
 **/

static void
#if	OlNeedFunctionPrototypes
SetDefaultValueResource (
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
)
#else
SetDefaultValueResource (w, offset, value)
	Widget			w;
	int			offset;
	XrmValue *		value;
#endif
{
	static int		_value;

	/*
	 * When we get here, TextEdit's initialize method has not yet been
	 * called, thus the source field is still interpretable as type
	 * String (after initialize does its work the field has to be
	 * interpreted as type pointer-to-TextBuffer).
	 */
	_value = TEXTEDIT_P(w).source? atoi(TEXTEDIT_P(w).source) : 0;
	value->addr = (XtPointer)&_value;
	return;
} /* SetDefaultValueResource */
