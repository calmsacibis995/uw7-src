#ifndef	NOIDENT
#ident	"@(#)textfield:TextFieldM.c	2.3"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/TextFieldP.h"

#define ClassName TextFieldM
#include <Xol/NameDefs.h>

/*
 * Convenient macros:
 */

#define TEXTEDIT_P(w) ((TextFieldWidget)(w))->textedit

/*
 * Private routines:
 */

static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		ResetMargin OL_ARGS((
	Widget			w
));
static void		DrawButton OL_ARGS((
	Widget			w,
	XRectangle *		rect,
	Boolean			depressed
));
static void		OverrideDefaultTextEditResources OL_ARGS((
	Widget			w,
	Dimensions *		d		/*NOTUSED*/
));

/**
 ** _OlmTFClassInitialize()
 **/

void
#if	OlNeedFunctionPrototypes
_OlmTFClassInitialize (
	Indirect *		indirect
)
#else
_OlmTFClassInitialize (indirect)
	Indirect *		indirect;
#endif
{
	TEXTFIELD_C(textFieldWidgetClass).scroll = 0;

	indirect->initialize = 0;
	indirect->set_values = SetValues;
	indirect->reset_margin = ResetMargin;
	indirect->draw_button = DrawButton;
	indirect->override_default_textedit_resources
				= OverrideDefaultTextEditResources;

	CORE_C(textFieldWidgetClass).resources[0].default_type = XtRString;
	CORE_C(textFieldWidgetClass).resources[0].default_addr = "2 points";

	return;
} /* _OlmTFClassInitialize */

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
#define DIFFERENT(F) \
	(((TextFieldWidget)new)->F != ((TextFieldWidget)current)->F)


	/*
	 * Assume that the client doesn't want to worry about including
	 * the shadow thickness when it changes the left/right margins.
	 * This assumption allows it to act GUI-independently.
	 */
	if (DIFFERENT(textedit.leftMargin))
		TEXTEDIT_P(new).leftMargin += PRIMITIVE_P(new).shadow_thickness;
	if (DIFFERENT(textedit.rightMargin))
		TEXTEDIT_P(new).rightMargin += PRIMITIVE_P(new).shadow_thickness;

#undef	DIFFERENT
	return (False);
} /* SetValues */

/**
 ** ResetMargin()
 **/

static void
#if	OlNeedFunctionPrototypes
ResetMargin (
	Widget			w
)
#else
ResetMargin (w)
	Widget			w;
#endif
{
	Dimension		shadow = PRIMITIVE_P(w).shadow_thickness;
	int			min;

#define RESET(W,ATTRACT,REPEL) \
Again:	min = TEXTEDIT_P(w).ATTRACT + TEXTEDIT_P(w).lineHeight + shadow;\
	if ((int)CORE_P(w).height > min)				\
		TEXTEDIT_P(w).REPEL = CORE_P(w).height - min + shadow;	\
	else if (TEXTEDIT_P(w).ATTRACT > shadow) {			\
		TEXTEDIT_P(w).ATTRACT = shadow;				\
		goto Again;						\
	} else								\
		TEXTEDIT_P(w).REPEL = 0

#if	defined(NORTH_GRAVITY)
	RESET (w, topMargin, bottomMargin);
#else
	RESET (w, bottomMargin, topMargin);
#endif
	return;
} /* ResetMargin */

/**
 ** DrawButton()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawButton (
	Widget			w,
	XRectangle *		rect,
	Boolean			depressed
)
#else
DrawButton (w, rect, depressed)
	Widget			w;
	XRectangle *		rect;
	Boolean			depressed;
#endif
{
	Screen *		screen	= XtScreen(w);

	Window			window	= XtWindow(w);
	Dimension		thickness;


	/*
	 * This code was derived from OlgDrawAbbrevMenuB.
	 */

	thickness = OlScreenPointToPixel(OL_HORIZONTAL, 2, screen);
	OlgDrawBorderShadow (
		screen, window, TEXTFIELD_P(w).attrs,
		depressed? OL_SHADOW_IN : OL_SHADOW_OUT,
		thickness,
		rect->x, rect->y, rect->width, rect->height
	);

	if (!OlgIs3d() && depressed) {
		Dimension h_inset = OlScreenPointToPixel(OL_HORIZONTAL, 3, screen);
		Dimension v_inset = OlScreenPointToPixel(OL_VERTICAL, 3, screen);
		XFillRectangle (
			DisplayOfScreen(screen), window,
			OlgGetFgGC(TEXTFIELD_P(w).attrs),
			rect->x + h_inset, rect->y + v_inset,
			rect->width - 2*h_inset, rect->height - 2*v_inset
		);
	}

	return;
} /* DrawButton */

/**
 ** OverrideDefaultTextEditResources()
 **/

static void
#if	OlNeedFunctionPrototypes
OverrideDefaultTextEditResources (
	Widget			w,
	Dimensions *		d		/*NOTUSED*/
)
#else
OverrideDefaultTextEditResources (w, d)
	Widget			w;
	Dimensions *		d;
#endif
{
	/*
	 * Leave a minimum gap at top and bottom inside the shadow border,
	 * so that the text doesn't touch. Assume TextEdit takes care of
	 * left and right.
	 */
	TEXTEDIT_P(w).topMargin =
	TEXTEDIT_P(w).bottomMargin = 1 + PRIMITIVE_P(w).shadow_thickness;
	return;
} /* OverrideDefaultTextEditResources */
