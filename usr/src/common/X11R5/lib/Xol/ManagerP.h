#ifndef NOIDENT
#ident	"@(#)manager:ManagerP.h	1.18"
#endif

/***********************************************************************
 *
 * Manager Widget Private Data
 *
 ***********************************************************************/

#ifndef _OlManagerP_h
#define _OlManagerP_h


#include <X11/ConstrainP.h>
#include <Xol/Manager.h>

#include <Xol/Olg.h>

/***********************************************************************
 *
 * Class record
 *
 ***********************************************************************/

/* New fields for the ManagerWidget class record */
typedef struct {
    OlHighlightProc	highlight_handler;
    int			focus_on_select;
    OlTraversalFunc	traversal_handler;
    OlActivateFunc	activate;
    OlEventHandlerList	event_procs;
    Cardinal		num_event_procs;
    OlRegisterFocusFunc	register_focus;
    XtVersionType	version;
    XtPointer		extension;
    _OlDynData		dyn_data;
    OlTransparentProc	transparent_proc;
} ManagerClassPart;

/* Full class record declaration */
typedef struct _ManagerClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
} ManagerClassRec;

extern ManagerClassRec	managerClassRec;

/***********************************************************************
 *
 * Instance record
 *
 ***********************************************************************/

/* New fields for the ManagerWidget record */
typedef struct _ManagerPart {
    /* Resource-related data */
    XtPointer		user_data;
    XtCallbackList	consume_event;
    Pixel		input_focus_color;
    String		reference_name;
    Widget		reference_widget;
    Boolean		traversal_on;

    /* Non-resource-related data */
    Boolean		has_focus;
    unsigned char	dyn_flags;

    /* mooLIT extension...	*/
    OlDefine		shadow_type;		/* XtNshadowType	*/
    Dimension		shadow_thickness;	/* XtNshadowThickness	*/
    OlgAttrs *		attrs;			/* private field	*/
} ManagerPart;


/* Full instance record declaration */
typedef struct _ManagerRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
} ManagerRec;


/***********************************************************************
 *
 * Constants
 *
 ***********************************************************************/

#define MGRPART(w)		( &(((ManagerWidget)(w))->manager) )
#define MGRCLASSPART(wc)	( &(((ManagerWidgetClass)(wc))->manager_class) )
#define _OlIsManager(w)		XtIsSubclass((w), managerWidgetClass)
#define _OL_IS_MANAGER		_OlIsManager

/* dynamic resources bit values */
#define OL_B_MANAGER_BG			(1 << 0)
#define OL_B_MANAGER_FOCUSCOLOR		(1 << 1)
#define OL_B_MANAGER_BORDERCOLOR	(1 << 2)

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

#define COMPOSITE_C(WC) ((CompositeWidgetClass)(WC))->composite_class
#define CONSTRAINT_C(WC) ((ConstraintWidgetClass)(WC))->constraint_class
#define MANAGER_C(WC) ((ManagerWidgetClass)(WC))->manager_class

#define COMPOSITE_P(W) ((CompositeWidget)(W))->composite
#define MANAGER_P(W) ((ManagerWidget)(W))->manager

#define FOR_EACH_CHILD(W,CHILD,N) \
	for (N = 0; N < COMPOSITE_P(W).num_children			\
		 && (CHILD = COMPOSITE_P(W).children[N]); N++)

#define FOR_EACH_MANAGED_CHILD(W,CHILD,N) \
	FOR_EACH_CHILD (W, CHILD, N)					\
		if (XtIsManaged(CHILD))

#endif /* _OlManagerP_h */
