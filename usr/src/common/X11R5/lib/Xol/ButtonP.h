#ifndef	NOIDENT
#ident	"@(#)button:ButtonP.h	1.29"
#endif

#ifndef _Ol_ButtonP_h_
#define _Ol_ButtonP_h_

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the private definitions for the
 *	OPEN LOOK(tm) Button widget and gadget.
 *
 ************************************************************
 */

#include <Xol/OlgP.h>

#include <Xol/PrimitiveP.h>	/* include widget's superclass header */
#include <Xol/EventObjP.h>	/* include gadget's superclass header */
#include <Xol/Button.h>

#define find_button_part(w) (_OlIsGadget((Widget)w) ?		\
			     &((ButtonGadget)(w))->button :	\
			     &((ButtonWidget)(w))->button)
#define find_font_list(w) (_OlIsGadget((Widget)w) ? \
              ((ButtonGadget)(w))->event.font_list : \
              ((ButtonWidget)(w))->primitive.font_list)
#define XtCeil(n)	((int) (n + .5))
#define isOblongType(t)	((int) (t == OL_OBLONG || t == OL_BUTTONSTACK || t == OL_HALFSTACK) ? TRUE : FALSE)

#define NORMAL 1
#define HIGHLIGHTED 0

/*
 *  There are no new fields for the ButtonClassPart
 */
typedef struct _ButtonClass {
    char no_class_fields;		/* Makes compiler happy */
} ButtonClassPart;

/*
 *  declare the ButtonClassRec as a subclass of core
 */
typedef struct _ButtonClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	ButtonClassPart		button_class;
} ButtonClassRec;

/*
 *  declare the ButtonGadgetClassRec as a subclass of EventObj
 */
typedef struct _ButtonGadgetClassRec {
	RectObjClassPart rect_class;
	EventObjClassPart event_class;
	ButtonClassPart button_class;
} ButtonGadgetClassRec;

/*
 *  buttonClassRec and buttonGadgetClassRec are defined in Button.c
 */
extern ButtonClassRec buttonClassRec;
extern ButtonGadgetClassRec buttonGadgetClassRec;

/*
 *  declaration of the Button widget fields
 */
typedef struct {
	/*
	 *  Public resources
	 */
	Pixel		background_pixel;
	Pixel		corner_color;
	char *		label;
	XImage *	label_image;
	OlDefine	label_type;
	OlDefine	button_type;
	OlDefine	menumark;
	OlDefine	label_justify;
	int		scale;
	int		shell_behavior;
	Boolean		label_tile;
	Boolean		is_default;
	Boolean		set;
	Boolean		dim;
	Boolean		busy;
	Boolean		internal_busy;
	Boolean		recompute_size;
	XtCallbackList	select;
	XtCallbackList	unselect;
	Widget		preview;
	XtCallbackList	post_select;
	XtCallbackList	label_proc;

	/*
	 *  Private fields
	 */
	Dimension	normal_height;
	Dimension	normal_width;
	GC		normal_GC;
	GC		inverse_text_GC;
	OlgAttrs *	pAttrs;
	OlgAttrs *	pHighlightAttrs;
} ButtonPart;

typedef struct _ButtonRec {
	CorePart	core;
	PrimitivePart	primitive;
	ButtonPart	button;
} ButtonRec;

typedef struct _ButtonGadgetRec {
	ObjectPart object;
	RectObjPart rect;
	EventObjPart event;
	ButtonPart button;
} ButtonGadgetRec;

						/* Add some externs	*/
extern void
_OlButtonPreview();

extern void 
_OlDrawHighlightButton();

extern void 
_OlDrawNormalButton();

#endif /* _Ol_ButtonP_h_ */
