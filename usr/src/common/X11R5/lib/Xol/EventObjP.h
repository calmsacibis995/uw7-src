#ifndef NOIDENT
#ident	"@(#)eventobj:EventObjP.h	1.16"
#endif

#ifndef _OlEventObjP_h
#define _OlEventObjP_h

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the private definitions for the
 *	OPEN LOOK(tm) EventObj (Meta) Class.
 *
 ************************************************************
 */

#include <X11/RectObjP.h>
#include <Xol/PrimitiveP.h>	/* for Primitive functionality */
#include <Xol/EventObj.h>

extern void _OlAddEventHandler();
extern void _OlRemoveEventHandler();

/* Macros */

#define _OlXTrans(w, x_val) \
    ((_OlIsGadget(w)) ? ((RectObj)(w))->rectangle.x + (x_val) : (x_val))

#define _OlYTrans(w, y_val) \
    ((_OlIsGadget(w)) ? ((RectObj)(w))->rectangle.y + (y_val) : (y_val))

/* dynamic resources bit masks */
#define OL_B_EVENT_BG		(1 << 0)
#define OL_B_EVENT_FG		(1 << 1)
#define OL_B_EVENT_FONTCOLOR	(1 << 2)
#define OL_B_EVENT_FOCUSCOLOR	(1 << 3)

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

#define EVENTOBJ_C(WC) ((EventObjClass)(WC))->event_class
#define EVENTOBJ_P(W) ((EventObj)(W))->event

/*
 *  Event Object Class Data Structures
 */
typedef struct _EventObjClassPart {
    /* fields for Primitive Class equivalence */
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
} EventObjClassPart;

typedef struct _EventObjClassRec {
    RectObjClassPart	rect_class;
    EventObjClassPart	event_class;
} EventObjClassRec;

/*
 *  eventObjClassRec is defined in EventObj.c
 */
extern EventObjClassRec eventObjClassRec;

/*
 *  Event Object Instance Data Structures
 */
typedef struct {
    /* Position dependent: the following fields coincide with CorePart */
    XtEventTable	event_table;

    /* Fields for Primitive Instance equivalence */
    XtPointer		user_data;
    Boolean		traversal_on;
    OlMnemonic		mnemonic;
    String		accelerator;
    String		accelerator_text;
    String		reference_name;
    Widget		reference_widget;
    XtCallbackList	consume_event;
    XFontStruct		*font;
    Pixel		input_focus_color;
    Pixel		font_color;
    Pixel		foreground;

    /* Resource-related data added for i18n, keep it last !! */
    OlFontList    *font_list;

    /* non-resource related fields */
    Boolean		has_focus;
    unsigned char	dyn_flags;

} EventObjPart;

typedef struct _EventObjRec {
    ObjectPart		object;
    RectObjPart		rectangle;
    EventObjPart	event;
} EventObjRec;

#endif /* _OlEventObjP_h */
