/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)eventobj:EventObj.c	1.75"
#endif
    
/******************************file*header********************************
 *
 * Description:
 *	Source code for the OPEN LOOK (Tm - AT&T) EventObj (Meta) Class.
 *
 */
    
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <Xt/EventI.h> /* Xt (sic) so that we don't have to install it in X11 */
#include <Xol/OpenLookP.h>
#include <Xol/EventObjP.h>
#include <Xol/Accelerate.h>
#include <Xol/LayoutExtP.h>

#define ClassName EventObj
#include <Xol/NameDefs.h>

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 */

typedef enum {
	eRemoveGrab, eAddGrab, eQueryGrabs, eQuerySiblingGrabs
} GrabCode;
typedef struct {
	Widget	object;
	Boolean	exclusive;
} GrabRec, *GrabList;

#define ON_GRAB_LIST(w)	HandleGrabs(eQueryGrabs, w, False, False)

#define CREATE		True
#define DONT_CREATE	False
#define OleTdestroy	"destroy"

#define HASH_MASK	0x7F				/* 2^n - 1 */
#define TABLE_SIZE	(HASH_MASK + 1)
#define HASH(entry)	((((int)(entry)) >> 3) & HASH_MASK)

typedef struct  _GadgetData {
    WidgetList  gadget_children;/* array of gadget children	*/
    Cardinal    num_gadgets;	/* total number of gadget children */
    Cardinal    num_slots;	/* number of slots in gadget array */
    int         last_used;	/* index into the gadget_children */
/*  Widget      last_focus;	/* gadget that last had input focus */
/*  Position	bounding_x;	/* origin of bounding box */
/*  Position	bounding_y;	/* origin of bounding box */
/*  Dimension	bounding_width;	/* dimension of bounding box */
/*  Dimension	bounding_height;/* dimension of bounding box */
    struct _GadgetData * next;	/* next GadgetData in hash table */
} GadgetData;

typedef enum _GrabType {pass, ignore, remap} GrabType;
    
#define PointInRect(rect_x, rect_y, rect_width, rect_height, x, y) \
    ( \
     (int)(rect_x) <= (int)(x) && \
     (int)((rect_x) + (rect_width)) >= (int)(x) && \
     (int)(rect_y) <= (int)(y) && \
     (int)((rect_y) + (rect_height)) >= (int)(y) \
     ) 

#define NonMaskableMask ((EventMask)0x80000000L)

#define BYTE_OFFSET	XtOffsetOf(EventObjRec, event.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_EVENT_BG, NULL },
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_EVENT_FG, NULL },
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_EVENT_FONTCOLOR, NULL },
{ { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, "Red" }, BYTE_OFFSET, OL_B_EVENT_FOCUSCOLOR, NULL },
};
#undef BYTE_OFFSET

/**************************forward*declarations***************************
 *
 * Forward Procedure declarations listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 */
						/* private procedures */
static Boolean		AcceptFocus OL_ARGS((Widget, Time *));
static Boolean		AllowGadgetGrabs OL_ARGS((Widget));
static EventMask	BuildParentMask();
static void		ConvertTypeToMask();
static void		DispatchEvent();
static void		DispatchGadgetEvent();
static void		FreeGadgetData();
static GadgetData *	GetGadgetData();
static GadgetData **	GetHashTblEntry();
static void		GrabDestroyCallback OL_ARGS((Widget,
					XtPointer, XtPointer));
static Boolean		HandleGrabs OL_ARGS((GrabCode,Widget,Boolean,Boolean));
static Boolean		IntersectRect();
static int		WidgetToGadget();
static Cardinal		WidgetToGadgets();
						/* class procedures */
static void ClassInitialize();
static void ClassPartInitialize();
static void Destroy();
static void GetValuesHook();
static void Initialize();
static Boolean SetValues();
						/* action procedures */
						/* public procedures */
void _OlAddEventHandler();
void _OlAddGrab OL_ARGS((Widget, Boolean, Boolean));
void _OlRemoveGrab OL_ARGS((Widget));
void _OlRemoveEventHandler();
Widget _OlWidgetToGadget OL_ARGS((Widget, Position, Position));

/***********************widget*translations*actions***********************
 *
 * Define Translations and Actions
 *
 */

/* There are no Translations or Action Tables for the eventObj widget */

/****************************widget*resources*****************************
 *
 * Define Resource list associated with the Widget Instance
 *
 */
#define OFFSET(field) XtOffsetOf(EventObjRec, event.field)

static XtResource resources[] =
{
  { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffset(EventObj, rectangle.border_width), XtRImmediate, (XtPointer)0
  },
  { XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
    OFFSET(accelerator), XtRString, (XtPointer) NULL
  },
  { XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
    OFFSET(accelerator_text), XtRString, (XtPointer) NULL
  },
  { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(consume_event), XtRCallback, (XtPointer)NULL
  },
  { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
    OFFSET(font), XtRString, OlDefaultFont
  },
  { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
    OFFSET(font_color), XtRString, XtDefaultForeground
  },
  { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(foreground), XtRString, XtDefaultForeground
  },
  { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
    OFFSET(input_focus_color), XtRString, "Red"
  },
  { XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
    OFFSET(mnemonic), XtRImmediate, (XtPointer) '\0'
  },
  { XtNreferenceName, XtCReferenceName, XtRString, sizeof(String),
    OFFSET(reference_name), XtRString, (XtPointer)NULL
  },
  { XtNreferenceWidget, XtCReferenceWidget, XtRWidget, sizeof(Widget),
    OFFSET(reference_widget), XtRWidget, (XtPointer)NULL
  },
  { XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
    OFFSET(traversal_on), XtRImmediate, (XtPointer)True
  },
  { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
    OFFSET(user_data), XtRPointer, (XtPointer)NULL
  },
  { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *),
    OFFSET(font_list), XtRString, (XtPointer)NULL
  },
};

/***************************widget*class*record***************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 */

/*
 * Class record structure:
 *
 *	(I)	XtInherit'd field
 *	(D)	Chained downward (super-class to sub-class)
 *	(U)	Chained upward (sub-class to super-class)
 */

EventObjClassRec	eventObjClassRec = {
	/*
	 * RectObj class:
	 */
	{
/* superclass           */ (WidgetClass)         &rectObjClassRec,
/* class_name           */                       "Event", /* [sic] */
/* widget_size          */                       sizeof(EventObjRec),
/* class_initialize     */                       ClassInitialize,
/* class_part_init   (D)*/                       ClassPartInitialize,
/* class_inited         */                       False,
/* initialize        (D)*/                       Initialize,
/* initialize_hook   (D)*/ (XtArgsProc)          0, /* Obsolete */
/* (unused)             */ (XtProc)              0,
/* (unused)             */ (XtPointer)           0,
/* (unused)             */ (Cardinal)            0,
/* resources         (D)*/                       resources,
/* num_resources        */                       XtNumber(resources),
/* xrm_class            */                       NULLQUARK,
/* (unused)             */ (Boolean)             0,
/* (unused)             */ (XtEnum)              0,
/* (unused)             */ (Boolean)             0,
/* (unused)             */ (Boolean)             0,
/* destroy           (U)*/                       Destroy,
/* resize            (I)*/                       _OlDefaultResize,
/* expose            (I)*/ (XtExposeProc)        0,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/                       GetValuesHook,
/* accept_focus (!Xt)(I)*/ (XtProc)              AcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* (unused)             */ (String)              0,
/* query_geometry    (I)*/                       _OlDefaultQueryGeometry,
/* (unused)             */ (XtProc)              0,
/* extension            */ (XtPointer)           0
	},
	/*
	 * EventObj class:
	 */
	{
/* focus_on_select	*/ 			 True,
/* highlight_handler (I)*/ (OlHighlightProc)     0,
/* traversal_handler (I)*/ (OlTraversalFunc)     0,
/* register_focus    (I)*/ (OlRegisterFocusFunc) 0,
/* activate          (I)*/ (OlActivateFunc)      0,
/* event_procs          */ (OlEventHandlerList)  0,
/* num_event_procs      */ (Cardinal)            0,
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { dyn_res, XtNumber(dyn_res) },
/* transparent_proc  (I)*/ (OlTransparentProc)   0,
	},
}; 

/*************************public*class*definition*************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 */

WidgetClass eventObjClass = (WidgetClass) &eventObjClassRec;
    
/***************************private*procedures****************************
 *
 * Private Procedures
 *
 */
    
/***************************function*header*******************************
  AcceptFocus- this function is pointed to by accept_focus field in Core.
	Xt does not make use of this field (since objects cannot take
	focus) but Xol makes use of it (CallAcceptFocus).  The assumption
	is that Xt will simply leave the field alone and not NULLify it,
	for instance.
 */

static Boolean
AcceptFocus OLARGLIST((w, time))
    OLARG( Widget,	w)
    OLGRA( Time *,	time)
{

    if (OlCanAcceptFocus(w, *time))
    {
     	return(OlSetInputFocus(w, RevertToNone, *time));
    }

    return (False);

} /* AcceptFocus() */

static EventMask
BuildParentMask(w, nonmaskable)
    Widget	w;
    Boolean *	nonmaskable;
{
    GadgetData *	gd = GetGadgetData(XtParent(w), DONT_CREATE);
    Cardinal		num_gadgets;
    EventMask		mask;
    
    mask = (EventMask) 0L;
    *nonmaskable = FALSE;
    
    for (num_gadgets = 0; num_gadgets < gd->num_gadgets; num_gadgets++)
    {
	Widget widget = gd->gadget_children[num_gadgets];
	
	/* taken from XtBuildEventMask (Event.c).  XtBuildEventMask does
	 * not help to get 'nonmaskable', however.
	 */
	register XtEventTable ev;
	for (ev = widget->core.event_table; ev != NULL; ev = ev->next) {
	    if (ev->select) mask |= ev->mask;
#if defined(XtSpecificationRelease) && XtSpecificationRelease < 5
	    *nonmaskable = *nonmaskable || ev->non_filter;
#endif
	}
	if (widget->core.widget_class->core_class.expose != NULL)
	    mask |= ExposureMask;
	if (widget->core.widget_class->core_class.visible_interest)
	    mask |= VisibilityChangeMask;
    }
    
    /* if any child wants Enter/Leaves, must have parent get Motion */
    if (mask & (EnterWindowMask | LeaveWindowMask))
	mask |= ButtonMotionMask;

    return(mask);
}

static void
ConvertTypeToMask (eventType, mask, grabType)
    int		eventType;
    EventMask   *mask;
    GrabType    *grabType;
{
    
    static struct {
	EventMask   mask;
	GrabType    grabType;
    } masks[] = {
	{0,				pass}, /* shouldn't see 0  */
	{0,				pass}, /* shouldn't see 1  */
	{KeyPressMask,		remap},	/* KeyPress		*/
	{KeyReleaseMask,		remap},	/* KeyRelease       */
	{ButtonPressMask,		remap},	/* ButtonPress      */
	{ButtonReleaseMask,		remap},	/* ButtonRelease    */
	{PointerMotionMask
	  | Button1MotionMask
	  | Button2MotionMask
	  | Button3MotionMask
	  | Button4MotionMask
	  | Button5MotionMask
	  | ButtonMotionMask,	ignore}, /* MotionNotify	*/
        {EnterWindowMask,		ignore}, /* EnterNotify	*/
	{LeaveWindowMask,		ignore}, /* LeaveNotify	*/
	{FocusChangeMask,		pass}, /* FocusIn		*/
	{FocusChangeMask,		pass}, /* FocusOut		*/
	{KeymapStateMask,		pass}, /* KeymapNotify	*/
	{ExposureMask,		pass}, /* Expose		*/
	{0,				pass}, /* GraphicsExpose   */
	{0,				pass}, /* NoExpose		*/
	{VisibilityChangeMask,      pass}, /* VisibilityNotify */
	{SubstructureNotifyMask,    pass}, /* CreateNotify	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* DestroyNotify	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* UnmapNotify	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* MapNotify	*/
	{SubstructureRedirectMask,  pass}, /* MapRequest	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* ReparentNotify   */
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* ConfigureNotify  */
	{SubstructureRedirectMask,  pass}, /* ConfigureRequest */
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* GravityNotify	*/
	{ResizeRedirectMask,	pass}, /* ResizeRequest	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* CirculateNotify	*/
	{SubstructureRedirectMask,  pass}, /* CirculateRequest */
	{PropertyChangeMask,	pass}, /* PropertyNotify   */
	{0,				pass}, /* SelectionClear   */
	{0,				pass}, /* SelectionRequest */
	{0,				pass}, /* SelectionNotify  */
	{ColormapChangeMask,	pass}, /* ColormapNotify   */
	{0,				pass}, /* ClientMessage	*/
	{0,				pass}, /* MappingNotify ???*/
    };
    
    eventType &= 0x7f;	/* Events sent with XSendEvent have high bit set. */
    (*mask)      = masks[eventType].mask;
    (*grabType)  = masks[eventType].grabType;
}

/* keep this SMALL to avoid blowing stack cache! */
/* because some compilers allocate all local locals on procedure entry */
#define EHSIZE 4

static void
DispatchEvent(w, event)
    Widget	w;			/* really the gadget */
    XEvent *	event;
{
    register XtEventRec * p;
    EventMask		mask;
    GrabType		grabType;
    int			numprocs, i;
    XtEventHandler	proc[100];
    XtPointer		closure[100];
    
    /* Have to copy the procs into an array, because calling one of them */
    /* might call XtRemoveEventHandler, which would break our linked list.*/
    
    ConvertTypeToMask(event->type, &mask, &grabType);
    numprocs = 0;
    
    if (grabType != pass && ON_GRAB_LIST(w) == False)
	return;

    p=w->core.event_table;
    if (p) {
        if (p->next) {
		XtEventHandler proc[EHSIZE];
		XtPointer closure[EHSIZE];
		int numprocs = 0;

            /* Have to copy the procs into an array, because one of them might
             * call XtRemoveEventHandler, which would break our linked list. */

            for (; p; p = p->next) {
                if (mask & p->mask
#if defined(XtSpecificationRelease) && XtSpecificationRelease < 5
			|| (mask == 0 && p->non_filter)
#endif
				) {
                    if (numprocs >= EHSIZE)
                        break;
                    proc[numprocs] = p->proc;
                    closure[numprocs] = p->closure;
                    numprocs++;
                }
            }
            if (numprocs) {
                    int i;
                    Boolean cont_to_disp = True;
                    for (i = 0; i < numprocs && cont_to_disp; i++)
                        (*(proc[i]))(w, closure[i], event, &cont_to_disp);
            }
        } else if (mask & p->mask
#if defined(XtSpecificationRelease) && XtSpecificationRelease < 5
			|| (mask == 0 && p->non_filter)
#endif
				) {
	    Boolean was_dispatched = True;
            (*p->proc)(w, p->closure, event, &was_dispatched);
        }
    }
}				/*  DispatchEvent  */

/* ARGSUSED */
static void
DispatchGadgetEvent(w, client_data, event, continue_to_dispatch)
    Widget	w;
    XtPointer	client_data;
    XEvent *	event;
    Boolean *	continue_to_dispatch;
{
    GadgetData *	gadget_data = GetGadgetData(w, DONT_CREATE);
    int			last_index;
    WidgetList		child;
    Widget		gadget;
    int			i;
    int			current_index = 0;
    
    /* No gadget_data or no gadget children */
    if ((gadget_data == NULL) || (gadget_data->num_gadgets == 0))
	return;
    
    last_index = gadget_data->last_used;
    child = gadget_data->gadget_children;

    /* This code does motion, enter/leave compression if specified. */
    /* For other types of events, it distributes them to one or more */
    /* gadget.  The switch must handle any type of X event that a */
    /* gadget can register an event handler for. */

    switch (event->type)  {
    case ButtonPress:
    case ButtonRelease:
    case KeyPress:
    case KeyRelease:
	/* get index of gadget over which this event occurred */
	current_index = WidgetToGadget(event->xkey.x,
				      event->xkey.y, gadget_data);
	if (current_index != -1) {
	    /* get gadget child */
	    gadget = child[current_index];

	    if (XtIsSensitive(gadget)) /* if sensitive, Dispatch event */
		DispatchEvent(gadget, event);

	    gadget_data->last_used = current_index;
	}
	break;
	
    case EnterNotify:
	/* look for a border gadget & generate Enter for it */
	current_index =
	    WidgetToGadget(event->xkey.x, event->xkey.y, gadget_data);

	if (current_index != last_index && current_index != -1) {
	    gadget = child[current_index];

	    /* Dispatch event only if gadget is Sensitive */
            if (XtIsSensitive(gadget) == True)
		DispatchEvent(gadget, event);

	    gadget_data->last_used = current_index;
	}
	break;
	
    case LeaveNotify:
	/* Generate Leave on gadget (if one) */
	if (last_index != -1) {
	    gadget = child[last_index];
	    DispatchEvent(gadget, event);
	    gadget_data->last_used = -1;
	}
	break;

    case MotionNotify:
	/* get gadget over which motion occurred */
	current_index =
	    WidgetToGadget(event->xkey.x, event->xkey.y, gadget_data);

	/* consider delivering Enter/Leaves */
	if (current_index != last_index) {
	    XCrossingEvent new_event;
	    
	    new_event.serial		= event->xmotion.serial;
	    new_event.send_event	= TRUE;
	    new_event.display		= event->xmotion.display;
	    new_event.window		= event->xmotion.window;
	    new_event.root		= event->xmotion.root;
	    new_event.subwindow		= event->xmotion.subwindow;
	    new_event.time		= event->xmotion.time;
	    new_event.x			= event->xmotion.x;
	    new_event.y			= event->xmotion.y;
	    new_event.x_root		= event->xmotion.x_root;
	    new_event.y_root		= event->xmotion.y_root;
	    new_event.mode		= NotifyNormal;
	    new_event.detail		= NotifyAncestor;
	    new_event.same_screen	= event->xmotion.same_screen;
	    new_event.focus		= FALSE;
	    new_event.state		= event->xmotion.state;
	    
	    /* generate Leave event on last gadget (if any) */
	    if (last_index != -1) {
		Widget last_w = child[last_index];
		new_event.type = LeaveNotify;
		DispatchEvent(last_w, (XEvent *)&new_event);
		gadget_data->last_used = -1;
	    }

	    /* now generate Enter on gadget we're over (if any) */
	    if (current_index != -1)  {
		Widget current_w = child[current_index];
		if (XtIsSensitive(current_w) == True) {
                    new_event.type = EnterNotify;
                    DispatchEvent(current_w, (XEvent *)&new_event);
		    gadget_data->last_used = current_index;
		}
            }
	}

	/* Now decide if Motion event itself should be delivered */
	if (current_index != -1) {

	    gadget = child[current_index];
#if 0
	    /*  Check if compression should be done  */
	    if (XtClass(gadget)->core_class.compress_motion) {
		/*  compress motion through this gadget  */
	    }
	    if (XtClass(gadget)->core_class.compress_enterleave) {
		/*  compress enter/leaves in this gadget  */
	    }
#endif

	    /* Dispatch event only if gadget is Sensitive */
	    if (XtIsSensitive(gadget) == True)
		DispatchEvent(gadget, event);
	}
	break;

    case Expose:
    case GraphicsExpose:
	/*  Can generate events to many gadgets  */
	{
	    WidgetList	gadget_list;

	    i = WidgetToGadgets(event->xexpose.x, event->xexpose.y,
			    event->xexpose.width, event->xexpose.height,
			    gadget_data, &gadget_list);

	    while(current_index < i)
	    {
		    DispatchEvent(*gadget_list++, event);
		    ++current_index;
	    }
	}
	break;
	
    case ClientMessage:
    case SelectionClear:
    case SelectionNotify:
    case SelectionRequest:
	/*  Send these events to all gadgets  */
	for (i = 0; i < gadget_data->num_gadgets; i++)
	    DispatchEvent(child[i], event);
	break;
	
    case CirculateNotify:
    case ColormapNotify:
    case ConfigureNotify:
    case CreateNotify:
    case DestroyNotify:
    case FocusIn:
    case FocusOut:
    case GravityNotify:
    case KeymapNotify:
    case MapNotify:
    case MappingNotify:
    case NoExpose:
    case PropertyNotify:
    case ReparentNotify:
    case ResizeRequest:
    case UnmapNotify:
    case VisibilityNotify:
	/*  These events do not apply to gadgets  */
	break;
    }
}				/*  DispatchGadgetEvent  */

static void
FreeGadgetData(w)
    Widget w;
{
    GadgetData ** gd = GetHashTblEntry(w);
    
    if (*gd != NULL) {
	GadgetData * tmp = *gd;			/* save pointer to entry */

	*gd = (*gd)->next;		/* point to next; may be NULL */

	if (tmp->gadget_children != NULL)
	    XtFree((char *)tmp->gadget_children); /* free list of children */
    
	XtFree((char *)tmp);			/* free gadget data */
    }
}

static GadgetData **
GetHashTblEntry(w)
    Widget w;			/* composite/parent of gadgets */
{

    static GadgetData * HashTbl[TABLE_SIZE];
    register GadgetData ** ptr;

    for (ptr = &HashTbl[ HASH(w) ]; *ptr != NULL; ptr = &((*ptr)->next))
	if (XtParent((*ptr)->gadget_children[0]) == w)
	    break;

    return (ptr);
}


static GadgetData *
GetGadgetData(w, create)
    Widget	w;		/* composite/parent of gadgets */
    Boolean	create;		/* make one if not pre-existing */
{
    GadgetData ** gd = GetHashTblEntry(w);

    if (*gd == NULL && create == CREATE) {

	/* GadgetData was not found for this composite so alloc one. */
	/* 'gd' points to correct link in list */

	*gd = (GadgetData *) XtMalloc(sizeof(GadgetData));
	(*gd)->gadget_children	= (WidgetList) NULL;
	(*gd)->num_gadgets	= 0;
	(*gd)->num_slots	= 0;
	(*gd)->last_used	= -1;
	(*gd)->next		= NULL;
    }
    return (*gd);
}

static Boolean
IntersectRect(x1, y1, rect_width, rect_height, x3, y3, width, height)
    Position	x1, y1;
    Dimension	rect_width, rect_height;
    Position	x3, y3;
    int		width, height;
{
    int partIn = FALSE;
    int x2 = x1 + rect_width;
    int y2 = y1 + rect_height;
    int x4 = x3 + width;
    int y4 = y3 + height;
    
    if (x1 > x3)  {
	if (x4 >= x1)  {
	    partIn = TRUE;
	}
    } else {
	if (x2 >= x3)  {
	    partIn = TRUE;
	}
    }
    
    if (partIn)  {
	if (y1 > y3)  {
	    if (y4 >= y1)  {
		return(TRUE);
	    }
	} else  {
	    if (y2 >= y3)  {
		return(TRUE);
	    }
	}
    }
    return(FALSE);	
}				/*  IntersectRect  */

static int
WidgetToGadget(x, y, gadget_data)
    int			x, y;
    GadgetData *	gadget_data;
{
    Cardinal		num_gadgets;
    Cardinal		current;
    WidgetList		child;
    
    /* If the position of the event does not intersect the
       bounding box of all gadgets, then give up
       if (!PointInRect(gadget_data->bounding_x,
       gadget_data->bounding_y,
       gadget_data->bounding_width,
       gadget_data->bounding_height, x, y))
       return(-1);
       */
    
    current	= (gadget_data->last_used == -1) ? 0 : gadget_data->last_used;
    child	= gadget_data->gadget_children;

    for (num_gadgets = gadget_data->num_gadgets;
	 num_gadgets != 0; current++, num_gadgets--) {
	if (current == gadget_data->num_gadgets)
	    current = 0;

	if (XtIsManaged(child[current])) /* only consider managed gadgets */
	{
	    Widget gadget = child[current];

	    if (PointInRect(gadget->core.x, gadget->core.y,
			gadget->core.width, gadget->core.height, x, y))
		return(current);
	}
    }
    return(-1);
}				/*  WidgetToGadget  */

static Cardinal
WidgetToGadgets(x, y, width, height, gadget_data, list_return)
    int			x, y, width, height;
    GadgetData *	gadget_data;
    WidgetList *	list_return;
{
#define MORESLOTS	4

    register Cardinal	num_gadgets, i;
    register WidgetList	child;

    static Widget *	gadget_list = NULL;
    static int		gadget_list_slots_left = 0;
    static int		gadget_list_alloced = 0;
    
    /*  If the position of the event does not intersect the
	bounding box of all gadgets, then give up
	if (!IntersectRect(gadget_data->bounding_x,
	gadget_data->bounding_y,
	gadget_data->bounding_width,
	gadget_data->bounding_height,
	x, y, width, height))
	return;
	*/

    gadget_list_slots_left = gadget_list_alloced;
    child		   = gadget_data->gadget_children;
    
    for (num_gadgets = gadget_data->num_gadgets, i = 0;
	 num_gadgets != 0; num_gadgets--, child++)
    {
	if (!XtIsManaged(*child))	/* only consider managed gadgets */
	    continue;

	if (IntersectRect((*child)->core.x, (*child)->core.y,
			  (*child)->core.width, (*child)->core.height,
			  x, y, width, height))
	{
	    if (gadget_list_slots_left == 0)
	    {
	       gadget_list_alloced += MORESLOTS;
	       gadget_list_slots_left += MORESLOTS;
	       gadget_list = (Widget *) XtRealloc((char *)gadget_list,
						gadget_list_alloced *
						sizeof (Widget));
	    }
	    gadget_list[i++] = *child;
	    gadget_list_slots_left--;
	}
    }

    *list_return = (i == (Cardinal)0 ? NULL : gadget_list);
    return(i);
#undef MORESLOTS
}				/*  WidgetToGadgets  */

/****************************class*procedures*****************************
 *
 * Class Procedures
 *
 */

/*****************************procedure*header*****************************
 * ClassInitialize- 
 * 
 */

static void
ClassInitialize()
{
	EVENTOBJ_C(eventObjClass).event_procs
			= (OlEventHandlerList)_OlGenericEventHandlerList;
	EVENTOBJ_C(eventObjClass).num_event_procs
			= _OlGenericEventHandlerListSize;
	return;
}

/*************************************************************************
 * ClassPartInitialize - Provides for inheritance of the class procedures
 ***************************function*header*******************************/

static void
ClassPartInitialize(wc)
	WidgetClass	wc;
{
	/*
	 * Warn if the subclass' version isn't up to date with this code.
	 */
	if (EVENTOBJ_C(wc).version != OlVersion && EVENTOBJ_C(wc).version < 5000) {
		OlVaDisplayWarningMsg((Display *)NULL,
			OleNinternal, OleTbadVersion,
			OleCOlToolkitWarning, OleMinternal_badVersion,
			RECT_C(wc).class_name,
			EVENTOBJ_C(wc).version, OlVersion);
	}

#define INHERIT(WC,F,INH) \
	if (EVENTOBJ_C(WC).F == (INH))					\
		EVENTOBJ_C(WC).F = EVENTOBJ_C(SUPER_C(WC)).F

	INHERIT (wc, highlight_handler, XtInheritHighlightHandler);
	INHERIT (wc, register_focus,    XtInheritRegisterFocus);
	INHERIT (wc, traversal_handler, XtInheritTraversalHandler);
	INHERIT (wc, activate,          XtInheritActivateFunc);
	INHERIT (wc, transparent_proc,  XtInheritTransparentProc);
#undef	INHERIT

	/*
	 * Since Xt does not define accept_focus for gadgets, Core.c will
	 * not do inheritance so it must be done here.
	 */
	if (CORE_C(wc).accept_focus == XtInheritAcceptFocus)
		CORE_C(wc).accept_focus = CORE_C(SUPER_C(wc)).accept_focus;

	if (wc == eventObjClass)
		return;

	if (EVENTOBJ_C(wc).dyn_data.num_resources == 0) {
		/* give the superclass's resource list to this class */
		EVENTOBJ_C(wc).dyn_data = EVENTOBJ_C(SUPER_C(wc)).dyn_data;
	}
	else {
		/* merge the two lists */
		_OlMergeDynResources(&(EVENTOBJ_C(wc).dyn_data),
				     &(EVENTOBJ_C(SUPER_C(wc)).dyn_data));
	}

	return;
} /* ClassPartInitialize() */

/*****************************procedure*header*****************************
 * Destroy- 
 *   1	decrement number of gadget children in GadgetData.
 *   2	remove gadget pointer from gadget_children by moving down
 *	those that follow. 
 *   3	if this is the last gadget...
 * 
 * This is taken from CompositeDeleteChild in Composite.c
 */

static void
Destroy(w)
    Widget w;
{
    register Cardinal		position;
    register Cardinal		i;
    Widget			parent = XtParent(w);
    register GadgetData *	gd = GetGadgetData(parent, DONT_CREATE);
    register WidgetList		child;
    
    /* none of the following (taken from CompositeDeleteChild in */
    /* Composite.c) really needs to be done if the parent is being */
    /* destroyed.  There is a check, however, in CompositeDeleteChild */
    /* for a child not found (this has been maintained in the code */
    /* below). */

    if (gd == NULL)
	return;

    child = gd->gadget_children;

    for (position = 0; position < gd->num_gadgets; position++)
	if (child[position] == w)
	    break;
    
    if (position == gd->num_gadgets) {	/* not found ! */

		OlVaDisplayWarningMsg(	XtDisplayOfObject(w),
					OleNfileEventObj,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileEventObj_msg1);
	return;
    }

    gd->num_gadgets--;		/* one less gadget child */

    /* Ripple children down one space from "position" */
    for (i = position; i < gd->num_gadgets; i++)
	child[i] = child[i+1];

    if (gd->num_gadgets == 0)	/* last gadget child! */
	FreeGadgetData(parent);
    
    /* from WindObjDestroy in WindowObj.c */
    _XtFreeEventTable(&w->core.event_table);

    _OlDestroyKeyboardHooks(w);

    if (EVENTOBJ_P(w).accelerator)
	XtFree(EVENTOBJ_P(w).accelerator);
    if (EVENTOBJ_P(w).accelerator_text)
	XtFree(EVENTOBJ_P(w).accelerator_text);
    if (EVENTOBJ_P(w).reference_name)
	XtFree(EVENTOBJ_P(w).reference_name);
}

/*****************************procedure*header*****************************
 * GetValuesHook - check for XtNreferenceWidget and XtNreferenceName
 *		and return info according to the traversal list
 */
static void
GetValuesHook(w, args, num_args)
	Widget		w;
	ArgList		args;
	Cardinal *	num_args;
{
	_OlGetRefNameOrWidget(w, args, num_args);
}

/*****************************procedure*header*****************************
 * Initialize- 
 *   *	install dispatcher on parent
 *   *	add destroy callback to parent so GadgetData can be deallocated
 *	when parent is destroyed
 *   *  allocate GadgetData for parent/composite & attach to heap
 *   *	add this child to gadget data
 */

/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
    Widget request, new;
    ArgList	args;
    Cardinal *	num_args;
{
    GadgetData *	gd = GetGadgetData(XtParent(new), CREATE);
    
    /* initialize the event_table field  */
    new->core.event_table = NULL;
    
    /* add this child to gadget data (from CompositeInsertChild) */
    if (gd->num_gadgets == gd->num_slots) {
	/* Allocate more space */
	gd->num_slots += (gd->num_slots / 2) + 2;
	gd->gadget_children = (WidgetList)
	    XtRealloc((char *)gd->gadget_children,
			gd->num_slots * sizeof(Widget));
    }
    gd->gadget_children[gd->num_gadgets] = new;
    gd->num_gadgets++;

    /* DEBUG */
    if (gd->num_gadgets > 1) {
	Widget w = gd->gadget_children[gd->num_gadgets - 2];
	if (XtParent(w) != XtParent(new))

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNfileEventObj,
					OleTmsg2,
					OleCOlToolkitWarning,
					OleMfileEventObj_msg2,
					XtParent(new)->core.name,
					XtParent(w)->core.name); 
    }

    /* Initialize non-resource fields */

    EVENTOBJ_P(new).has_focus = False;

	/* See Primitive:Initialize() for explanation...	*/
    _OlLoadDefaultFont(new, EVENTOBJ_P(new).font);

    if (EVENTOBJ_P(new).mnemonic != NULL)
	if (_OlAddMnemonic(new, 0, EVENTOBJ_P(new).mnemonic) != OL_SUCCESS)
	    EVENTOBJ_P(new).mnemonic = NULL;

    if (EVENTOBJ_P(new).accelerator != NULL)
	if (_OlAddAccelerator(new, 0, EVENTOBJ_P(new).accelerator) != OL_SUCCESS)
	    EVENTOBJ_P(new).accelerator = NULL;

    if (EVENTOBJ_P(new).accelerator != NULL)
    {
	EVENTOBJ_P(new).accelerator = XtNewString(EVENTOBJ_P(new).accelerator);
	EVENTOBJ_P(new).accelerator_text =
	    (EVENTOBJ_P(new).accelerator_text == NULL) ?
		_OlMakeAcceleratorText(new, EVENTOBJ_P(new).accelerator) :
		XtNewString(EVENTOBJ_P(new).accelerator_text);
    } else if (EVENTOBJ_P(new).accelerator_text)
	EVENTOBJ_P(new).accelerator_text = XtNewString(EVENTOBJ_P(new).accelerator_text);

		/* add me to the traversal list */
    if (EVENTOBJ_P(new).reference_name)
	EVENTOBJ_P(new).reference_name = XtNewString(EVENTOBJ_P(new).reference_name);

    _OlUpdateTraversalWidget(new, EVENTOBJ_P(new).reference_name,
			     EVENTOBJ_P(new).reference_widget, True);

    _OlInitDynResources(new, &(((EventObjClass)(new->core.widget_class))->
		event_class.dyn_data));
    _OlCheckDynResources(new, &(((EventObjClass)(new->core.widget_class))->
		event_class.dyn_data), args, *num_args);
}

/*************************************************************************
 * SetValues - Sets up internal values based on changes made to external
 *	       ones
 ***************************function*header*******************************/

/* ARGSUSED */
static Boolean
SetValues (current, request, new, args, num_args)
	Widget	current;
	Widget	request;
	Widget	new;
	ArgList		args;
	Cardinal *	num_args;
{
	Boolean		redisplay = False;


#define CHANGED(field)	(EVENTOBJ_P(new).field != EVENTOBJ_P(current).field)

		/* See Primitive:Initialize() for explanation...	*/
	if (CHANGED(font))
	{
		_OlLoadDefaultFont(new, EVENTOBJ_P(new).font);
	}

	if (CHANGED(font))
		redisplay = True;
		
	if (CHANGED(reference_name))	/* this has higher preference	*/
	{
		if (EVENTOBJ_P(new).reference_name)
		{
			EVENTOBJ_P(new).reference_name = XtNewString(
					EVENTOBJ_P(new).reference_name);

			_OlUpdateTraversalWidget(new, EVENTOBJ_P(new).reference_name,
					 NULL, False);
		}
		if (EVENTOBJ_P(current).reference_name != NULL)
		{
			XtFree(EVENTOBJ_P(current).reference_name);
			EVENTOBJ_P(current).reference_name = NULL;
		}
	}
    	else if (CHANGED(reference_widget))
    	{
			/* no need to keep this around */
		if (EVENTOBJ_P(current).reference_name != NULL)
		{
			XtFree(EVENTOBJ_P(current).reference_name);
			EVENTOBJ_P(current).reference_name = NULL;
		}

		_OlUpdateTraversalWidget(new, NULL,
				 EVENTOBJ_P(new).reference_widget, False);
    	}

    if (CHANGED(mnemonic))
    {
	if (EVENTOBJ_P(current).mnemonic != NULL)
	    _OlRemoveMnemonic(new, 0, False, EVENTOBJ_P(current).mnemonic);

	if ((EVENTOBJ_P(new).mnemonic != NULL) &&
	  (_OlAddMnemonic(new, 0, EVENTOBJ_P(new).mnemonic) != OL_SUCCESS))
	    EVENTOBJ_P(new).mnemonic = NULL;
	redisplay = True;
    }

    {
	Boolean changed_accelerator = CHANGED(accelerator);
	Boolean changed_accelerator_text = CHANGED(accelerator_text);

    if (changed_accelerator)
    {
	if (EVENTOBJ_P(current).accelerator)
        {
	    _OlRemoveAccelerator(new, 0, False, EVENTOBJ_P(current).accelerator);
	    XtFree(EVENTOBJ_P(current).accelerator);
	    EVENTOBJ_P(current).accelerator = (String)0;
        }

	if (EVENTOBJ_P(new).accelerator != NULL) {
	    if (_OlAddAccelerator(new, 0, EVENTOBJ_P(new).accelerator) == OL_SUCCESS)
		EVENTOBJ_P(new).accelerator = XtNewString(EVENTOBJ_P(new).accelerator);
	    else
		EVENTOBJ_P(new).accelerator = (String) 0;
        }
	redisplay = TRUE;
    }

    if (changed_accelerator || changed_accelerator_text) {
           if (EVENTOBJ_P(current).accelerator_text) {
			XtFree(EVENTOBJ_P(current).accelerator_text);
			EVENTOBJ_P(current).accelerator_text = (String) 0;
	   }

	   EVENTOBJ_P(new).accelerator_text = (changed_accelerator_text ?
			XtNewString(EVENTOBJ_P(new).accelerator_text) :
			_OlMakeAcceleratorText(new, EVENTOBJ_P(new).accelerator));
	    redisplay = True;
    }
    }

	if (!XtIsSensitive(new) &&
	    (OlGetCurrentFocusWidget(new) == new)) {
		/*
		 * When it becomes insensitive, need to move focus elsewhere,
		 * if this widget currently has focus.
		 */
		OlMoveFocus(new, OL_IMMEDIATE, CurrentTime);
	}

#undef CHANGED

    _OlCheckDynResources(new, &(((EventObjClass)(new->core.widget_class))->
		event_class.dyn_data), args, *num_args);

    return (redisplay);
}	/* SetValues() */

/****************************action*procedures****************************
 *
 * Action Procedures
 *
 */

/****************************public*procedures****************************
 *
 * Public Procedures
 *
 */

void
_OlAddEventHandler(widget, eventMask, other, proc, closure)
    Widget		widget;
    EventMask		eventMask;
    Boolean		other;
    XtEventHandler	proc;
    XtPointer		closure;
{
    register XtEventRec	*p,**pp;
    EventMask		mask;
    Boolean		nonmaskable;
    
    if (!_OlIsGadget(widget)) {
	XtAddEventHandler(widget, eventMask, other, proc, closure);
	return;
    }
    
    if (eventMask == 0 && other == FALSE) return;
    
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
    eventMask &= ~NonMaskableMask;
    if (other)
        eventMask |= NonMaskableMask;
    if (!eventMask) return;
#endif

    pp = & widget->core.event_table;
    
    pp = & widget->core.event_table;
    while ((p = *pp) &&
           (p->proc != proc || p->closure != closure))
        pp = &p->next;

    if (!p) {				 /* New proc to add to list */
	p = XtNew(XtEventRec);
	p->proc = proc;
	p->closure = closure;
	p->mask = eventMask;
	p->select = TRUE;
#if defined(XtSpecificationRelease) && XtSpecificationRelease < 5
	p->non_filter = other;
	p->raw = FALSE;
#endif
	
	p->next = widget->core.event_table;
	widget->core.event_table = p;
	
    } else {				/* update existing proc */
	p->mask |= eventMask;
#if defined(XtSpecificationRelease) && XtSpecificationRelease < 5
	p->non_filter = p->non_filter;
	p->select |= TRUE;
#endif
    }
    
    /* build dispatching mask for parent from all gadget children */
    mask = BuildParentMask(widget, &nonmaskable);
    
    /* update entry for Dispatcher on parent */
    XtAddEventHandler(XtParent(widget), mask,
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
			eventMask,
#else
			nonmaskable,
#endif
		      DispatchGadgetEvent, NULL);
    
} /*  _OlAddEventHandler  */

void
_OlRemoveEventHandler(widget, eventMask, other, proc, closure)
    Widget		widget;
    EventMask		eventMask;
    Boolean		other;
    XtEventHandler	proc;
    XtPointer		closure;
{
    register XtEventRec *p,**pp;
    
    if (!_OlIsGadget(widget)) {
	XtRemoveEventHandler(widget, eventMask, other, proc, closure);
	return;
    }
    pp = &widget->core.event_table;
    
    /* find it */
    while ((p = *pp) &&
           (p->proc != proc || p->closure != closure))
        pp = &p->next;
    if (!p) return;

    /* un-register it */
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
    eventMask &= ~NonMaskableMask;
    if (other)
        eventMask |= NonMaskableMask;
    p->mask &= ~eventMask;
#else
    p->mask &= ~eventMask;
    if (other) p->non_filter = FALSE;
#endif
    
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
    if (!p->mask)  {
#else
    if (p->mask == 0 && !p->non_filter) {
#endif
	/* delete it entirely */
	*pp = p->next;
	XtFree((char *)p);
    }
    
    /* update entry for Dispatcher on parent */
    XtRemoveEventHandler(XtParent(widget), eventMask, other,
			 DispatchGadgetEvent, NULL);
}

Widget
_OlWidgetToGadget OLARGLIST((w, x, y))
    OLARG(Widget,	w)
    OLARG(Position,	x)
    OLGRA(Position,	y)
{
    int			child_index;
    GadgetData *	gadget_data = GetGadgetData(w, DONT_CREATE);

    if (gadget_data == NULL)
	return(NULL);

    child_index = WidgetToGadget(x, y, gadget_data);

    if (child_index == -1)
	return(NULL);

    return(gadget_data->gadget_children[child_index]);
}

/* GrabDestroyCallback - removes the object from the grab list
 */
/* ARGSUSED */
static void
GrabDestroyCallback OLARGLIST((w, client_data, call_data))
	OLARG( Widget,		w)
	OLARG( XtPointer, client_data)
	OLGRA( XtPointer, call_data)
{
	(void)HandleGrabs(eRemoveGrab, w, False, False);
} /* END OF GrabDestroyCallback() */

/* AllowGadgetGrabs -
 * flag that an application can set to permit gadgets to be used on
 * grab lists.  By Default, gadgets are not allowed to have grabs to
 * maintain backwards functionality.  If this value is set to TRUE,
 * the user will be able to drag the MENU button over menuButton gadgets
 * in a control area to preview the submenus.  When this is false,
 * once one submenu is displayed the user must dismiss the submenu before
 * being able to post another one.
 */
static Boolean
AllowGadgetGrabs OLARGLIST((w))
	OLGRA( Widget,	w)
{
	static Boolean	first_time = True;
	static Boolean	allow_gadget_grabs;

	if (first_time == True)
	{
		XtResource	rsc;

		first_time = False;

		rsc.resource_name	= "allowGadgetGrabs";
		rsc.resource_class	= "AllowGadgetGrabs";
		rsc.resource_type	= XtRBoolean;
		rsc.resource_size	= sizeof(Boolean);
		rsc.resource_offset	= (Cardinal)0;
		rsc.default_type	= XtRImmediate;
		rsc.default_addr	= (XtPointer)False;

		XtGetApplicationResources(w, (XtPointer)&allow_gadget_grabs,
				&rsc, 1, NULL, (Cardinal)0);
	}
	return(allow_gadget_grabs);
} /* END OF AllowGadgetGrabs() */

/* HandleGrabs - 
 * Shadow the Intrinsic's grabList.  This routine uses a stack
 * where new elements are put onto the end of the stack.
 */
static Boolean
HandleGrabs OLARGLIST((opcode, w, exclusive, spring_loaded))
	OLARG( GrabCode,	opcode)
	OLARG( Widget,		w)
	OLARG( Boolean,		exclusive)
	OLGRA( Boolean,		spring_loaded)
{
	static Cardinal	slots = 0;
	static Cardinal	num_grabs = 0;
	static GrabList	grablist = (GrabList)NULL;

	if (AllowGadgetGrabs(w) == False)
	{
		if (_OlIsGadget(w) == False)
		{
			switch(opcode) {
			case eAddGrab:
				XtAddGrab(w, exclusive, spring_loaded);
				break;
			case eRemoveGrab:
				XtRemoveGrab(w);
				break;
			}
		}
		return (True);
	}

	if (opcode == eQueryGrabs ||
	    opcode == eQuerySiblingGrabs)
	{
		Cardinal	i;
		Widget		orig = w;

		if (num_grabs == 0)
		{
			return(True);
		}

		while (w != (Widget)NULL)
		{
			i = num_grabs;
			while (i)
			{
				--i;
				if (grablist[i].object == w)
					return True;
				else if (grablist[i].exclusive == True)
					break;
			}
			w = XtParent(w);
		}


		/* If we reach here, the requested object is not on
		 * the grabList.  However, since the Intrinsics maintains
		 * the real grab list we should check to see if the
		 * parent of the original object (if it was a gadget)
		 * is on the grab list or if any gadget siblings 
		 * are on the grablist.  If neither of these is true,
		 * chances are that the application has created another
		 * XtGrab (e.g., using XtPopupWidget with XtGrabExclusive)
		 * or with XtAddGrab explicitly and we can't detect that.
		 * We can be pretty sure of this since the event would
		 * not have gotten here in the first place if the Instrinsics
		 * didn't think the gadget's parent wasn't on the grablist.
		 * So we'll consider this a degenerate case and return TRUE.
		 *
		 * However, check to see if we're here because we're checking
		 * for a sibling grab.  If so, return FALSE.
		 */

		if (opcode == eQuerySiblingGrabs ||
		    _OlIsGadget(orig) == FALSE)
		{
			return (FALSE);
		}
		else
		{
			/* Check for siblings on the list.  If one's on
			 * the list return FALSE since this means the
			 * original gadget is not permitted to receive
			 * the event, else, check for the parent being on
			 * the list.
			 */

		    Cardinal		i;
		    GadgetData *	gd = GetGadgetData(XtParent(orig),
							DONT_CREATE);

		    for (i = 0; i < gd->num_gadgets; ++i)
		    {
			w = gd->gadget_children[i];

			if (w != orig &&
			    HandleGrabs(eQuerySiblingGrabs, w, False, False)
				== True)
			{
				return(FALSE);
			}
		    }

		    return ((ON_GRAB_LIST(XtParent(orig)) == FALSE ?
				TRUE : FALSE));
		}
	}
	else if (opcode == eAddGrab)
	{
		XtAddCallback(w, XtNdestroyCallback, GrabDestroyCallback,
					(XtPointer)NULL);

		if (num_grabs == slots)
		{
			slots += 4;
			grablist = (GrabList)XtRealloc((char *)grablist,
						slots * sizeof(GrabRec));
		}
		grablist[num_grabs].object	= w;
		grablist[num_grabs].exclusive	= exclusive;
		++num_grabs;
	}
	else /* eRemoveGrab */
	{
		Cardinal	i = num_grabs;

		while (i)
		{
			--i;
			if (grablist[i].object == w)
			{
				Cardinal	old_num = num_grabs;

				num_grabs = i;

				for(; i< old_num; ++i)
				{
					XtRemoveCallback(grablist[i].object,
							XtNdestroyCallback,
							GrabDestroyCallback,
							(XtPointer)NULL);
				}
				break;
			}
		}
	}

	if (_OlClass(w) == eventObjClass)
	{
		w = XtParent(w);
	}

	if (opcode == eAddGrab)
	{
		XtAddGrab(w, exclusive, spring_loaded);
	}
	else
	{
		/* Don't bother removing the XtGrab when this object is
		 * being destroyed since Xt will do it for us.
		 */
		if (w->core.being_destroyed == False)
			XtRemoveGrab(w);
	}
	return True;
} /* END OF HandleGrabs() */

/*
 * _OlAddGrab - must use our own grab routine since XtAddGrab doesn't work
 * for Gadgets.
 */
void
_OlAddGrab OLARGLIST((w, exclusive, spring_loaded))
	OLARG( Widget,	w)
	OLARG( Boolean,	exclusive)
	OLGRA( Boolean,	spring_loaded)
{
	(void)HandleGrabs(eAddGrab, w, exclusive, spring_loaded);
} /* END OF _OlAddGrab() */

void
_OlRemoveGrab OLARGLIST((w))
	OLGRA( Widget,	w)
{
	(void)HandleGrabs(eRemoveGrab, w, False, False);
} /* END OF _OlRemoveGrab() */
