#ifndef NOIDENT
#ident	"@(#)primitive:PrimitiveP.h	1.24"
#endif

/***********************************************************************
 *
 * Primitive Widget Private Data
 *
 ***********************************************************************/

#ifndef _OlPrimitiveP_h
#define _OlPrimitiveP_h

#include <X11/CoreP.h>
#include <Xol/Primitive.h>
#include <Xol/Olg.h>

/***********************************************************************
 *
 * Class record
 *
 ***********************************************************************/

/* New fields for the PrimitiveWidget class record */
typedef struct {
    int				focus_on_select;
    OlHighlightProc		highlight_handler;
    OlTraversalFunc		traversal_handler;
    OlRegisterFocusFunc		register_focus;
    OlActivateFunc		activate;
    OlEventHandlerList		event_procs;
    Cardinal			num_event_procs;
    XtVersionType		version;
    XtPointer			extension;
    _OlDynData			dyn_data;
    OlTransparentProc		transparent_proc;
} PrimitiveClassPart;

/* Full class record declaration */
typedef struct _PrimitiveClassRec {
    CoreClassPart	core_class;
    PrimitiveClassPart	primitive_class;
} PrimitiveClassRec;

extern PrimitiveClassRec	primitiveClassRec;


/***********************************************************************
 *
 * Instance record
 *
 ***********************************************************************/

/* New fields for the PrimitiveWidget record */
typedef struct _PrimitivePart {
    /* Resource-related data */
    XtPointer		user_data;
    Widget     		reference_widget;
    String		accelerator;
    String		accelerator_text;
    String		reference_name;
    XtCallbackList	consume_event;
    XFontStruct		*font;
    Pixel		input_focus_color;
    Pixel		font_color;
    Pixel		foreground;
    OlMnemonic		mnemonic;
    Boolean		traversal_on;

    /* Resource-related data added for i18n, keep it last !! */
    OlFontList		*font_list;

    /* Non-resource-related data */
    Boolean		has_focus;
    unsigned char	dyn_flags;
#ifdef I18N
	 OlIc *	ic;					/* input context */
#endif

    /* MooLIT extension... */
    OlDefine		shadow_type;		/* XtNshadowType  */
    Dimension		shadow_thickness;	/* XtNshadowThickness  */
    OlgAttrs *          attrs;                  /* private field        */
    Dimension		highlight_thickness;	/* XtNhighlightThickness  */

} PrimitivePart;


/* Full instance record declaration */
typedef struct _PrimitiveRec {
    CorePart		core;
    PrimitivePart	primitive;
} PrimitiveRec;


/***********************************************************************
 *
 * Constants
 *
 ***********************************************************************/

#define _OlIsPrimitive(w) (XtIsSubclass((w), primitiveWidgetClass))
#define _OL_IS_PRIMITIVE _OlIsPrimitive

/* dynamic resources bit masks */
#define OL_B_PRIMITIVE_BG		(1 << 0)
#define OL_B_PRIMITIVE_FG		(1 << 1)
#define OL_B_PRIMITIVE_FONTCOLOR	(1 << 2)
#define OL_B_PRIMITIVE_FOCUSCOLOR	(1 << 3)
#define OL_B_PRIMITIVE_FONTGROUP	(1 << 4)
#define OL_B_PRIMITIVE_FONT		(1 << 5)

#if	!defined(OBJECT_C)
#define OBJECT_C(WC) ((ObjectClass)(WC))->object_class
#define OBJECT_P(W) ((Object)(W))->object
#define RECT_C(WC) ((RectObjClass)(WC))->rect_class
#define RECT_P(W) ((RectObj)(W))->rectangle
#define CORE_C(WC) ((WidgetClass)(WC))->core_class
#define CORE_P(W) ((Widget)(W))->core
#define SUPER_C(WC) CORE_C(WC).superclass
#define CLASS(WC) CORE_C(WC).class_name
#endif

#define PRIMITIVE_C(WC) ((PrimitiveWidgetClass)(WC))->primitive_class
#define PRIMITIVE_P(W) ((PrimitiveWidget)(W))->primitive

#endif /* _OlPrimitiveP_h */
