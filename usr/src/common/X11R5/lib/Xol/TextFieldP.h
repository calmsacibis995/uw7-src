#ifndef	NOIDENT
#ident	"@(#)textfield:TextFieldP.h	2.4"
#endif

#ifndef _TEXTFIELDP_H
#define _TEXTFIELDP_H

#include "Xol/TextEditP.h"
#include "Xol/TextField.h"
#include "Xol/OlgP.h"

/*
 * Semi-private constants:
 */

#define KEY_LAG	500	/* how long to depress arrow on keyboard equiv. */

/*
 * TextField class types:
 */

typedef enum Arrow {
	ArrowLeft,
	ArrowRight,
	ArrowIncrement,
	ArrowDecrement,
	ArrowNone
}			Arrow;

/*
 * Class structure:
 */

typedef Boolean		(*OlTextFieldScrollProc) OL_ARGS((
	Widget			w,
	Arrow			arrow
));
typedef Boolean		(*OlTextFieldStepProc) OL_ARGS((
	Widget			w,
	OlSteppedReason		reason,
	Cardinal		count
));

typedef struct _TextFieldClassPart {
	OlTextFieldScrollProc	scroll;
	OlTextFieldStepProc	step;
	XtPointer		extension;
}			TextFieldClassPart;

typedef struct _TextFieldClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	TextEditClassPart	textedit_class;
	TextFieldClassPart	textfield_class;
}			TextFieldClassRec;

extern TextFieldClassRec	textFieldClassRec;

#define XtInheritScrollProc	((OlTextFieldScrollProc)_XtInherit)
#define XtInheritStepProc	((OlTextFieldStepProc)_XtInherit)

#define TEXTFIELD_C(WC) ((TextFieldWidgetClass)(WC))->textfield_class

/*
 * Instance structure:
 */

typedef struct _TextFieldPart {
	/*
	 * Public:
	 */
	int			initial_delay;
	int			repeat_rate;
	int			maximum_size;
	XtCallbackList		verification;
	Boolean			can_increment;
	Boolean			can_decrement;
	Boolean			can_scroll;

	/*
	 * Private:
	 */
	Boolean			button_down;
	OlgAttrs *		attrs;
	Arrow			polling;
	XtIntervalId		timer;
	XtArgVal		bit_bucket;
	unsigned char		dynamics;
}			TextFieldPart;

#define _TEXTFIELD_B_DYNAMIC_BACKGROUND	0x001

typedef struct _TextFieldRec {
	CorePart		core;
	PrimitivePart		primitive;
	TextEditPart		textedit;
	TextFieldPart		textfield;
}			TextFieldRec;

#define TEXTFIELD_P(W) ((TextFieldWidget)(W))->textfield

/*
 * Private types:
 */

typedef struct Rectangles {
	XRectangle		left;
	XRectangle		right;
	XRectangle		increment;
	XRectangle		decrement;
	XRectangle		line;
}			Rectangles;

typedef struct Dimensions {
	Dimension		arrow_width;
	Dimension		arrow_height;
	Dimension		gap;
	Dimension		line_thickness;
}			Dimensions;

typedef struct Indirect {
	XtInitProc	initialize;
	XtSetValuesFunc	set_values;
	void		(*reset_margin) OL_ARGS((
		Widget		w
	));
	void		(*draw_button) OL_ARGS((
		Widget		w,
		XRectangle *	rect,
		Boolean		depressed
	));
	void		(*override_default_textedit_resources) OL_ARGS((
		Widget		w,
		Dimensions *	d
	));
}			Indirect;

/*
 * Private functions:
 */

OLBeginFunctionPrototypeBlock

extern void
_OloTFClassInitialize OL_ARGS((
	Indirect *		indirect
));
extern void
_OlmTFClassInitialize OL_ARGS((
	Indirect *		indirect
));
extern void
_OlTFComputeRectangles OL_ARGS((
	Widget			w,
	Rectangles *		r
));

OLEndFunctionPrototypeBlock

#endif
