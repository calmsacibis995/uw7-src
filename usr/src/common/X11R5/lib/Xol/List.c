/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)scrollinglist:List.c	1.65"
#endif
/*
 List.c (C source file)
	Acc: 596865446 Tue Nov 29 22:57:26 1988
	Mod: 596865446 Tue Nov 29 22:57:26 1988
	Sta: 596865446 Tue Nov 29 22:57:26 1988
	Owner: 4777
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/*******************************file*header*******************************
 * Date:	Oct-88
 * File:	List.c
 *
 * Description:
 *	List.c - OPEN LOOK(TM) List
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrollingP.h>		/* my private header file */
#include <Xol/ListPaneP.h>		/* fixed pane */
#include <Xol/Scrollbar.h>		/* fixed scrollbar */

#define ClassName List
#include <Xol/NameDefs.h>

/**************************forward*declarations***************************
 *
 * Forward function definitions listed by category:
 *		1. Private functions
 *		2. Class   functions
 *		3. Action  functions
 *		4. Public  functions
 *
 */
						/* private procedures */
						/* class procedures */
static Boolean	AcceptFocus OL_ARGS((Widget, Time *));
static void	Destroy();
static void	GetValuesHook();
static void	Initialize();
static Widget	RegisterFocus();
static Boolean	SetValues();
static Widget	TraversalHandler OL_ARGS((Widget, Widget, OlVirtualName, Time));
						/* action procedures */
static void	ConsumeEventCB OL_ARGS((Widget, XtPointer, XtPointer));

						/* public procedures */

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

#define LPART(w)	( &(((ListWidget)(w))->list) )

	/* Spacing for List layout.  Values are in points */
#define PANE_BW		1	/* in pixels */

/* these resources actually belong to the ListPane.  Used in */
/* Initialize, SetValues, GetValuesHook */

static MaskArg PaneArgs[] = {
    { XtNapplAddItem, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplDeleteItem, NULL, OL_SOURCE_PAIR },	/* GetValues only */
    { XtNapplEditClose, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplEditOpen, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplTouchItem, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplUpdateView, NULL, OL_SOURCE_PAIR },	/* GetValues only */
    { XtNapplViewItem, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNbackground, NULL, OL_SOURCE_PAIR },
    { XtNbackgroundPixmap, NULL, OL_SOURCE_PAIR },
    { XtNborderColor, NULL, OL_SOURCE_PAIR },
    { XtNborderPixmap, NULL, OL_SOURCE_PAIR },
    { XtNborderWidth, NULL, OL_SOURCE_PAIR },
    { XtNfont, NULL, OL_SOURCE_PAIR },			/* and for textField */
    { XtNfontColor, NULL, OL_SOURCE_PAIR },		/* and for textField */
    { XtNforeground, NULL, OL_SOURCE_PAIR },		/* and for textField */
    { XtNmaximumSize, NULL, OL_SOURCE_PAIR },		/* for textField */
    { XtNrecomputeWidth, NULL, OL_SOURCE_PAIR },
    { XtNselectable, NULL, OL_SOURCE_PAIR },
    { XtNstring, NULL, OL_SOURCE_PAIR },		/* for textField */
    { XtNtextField, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNtraversalOn, NULL, OL_SOURCE_PAIR },
    { XtNverification, NULL, OL_SOURCE_PAIR },		/* for textField */
    { XtNviewHeight, NULL, OL_SOURCE_PAIR },
    };

#define BYTE_OFFSET	XtOffsetOf(ListRec, list.dyn_flags)
static _OlDynResource dyn_res[] = {
  { { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0, XtRString,
	XtDefaultForeground }, BYTE_OFFSET, OL_B_LIST_FG, NULL },

  { { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0, XtRString,
	XtDefaultForeground }, BYTE_OFFSET, OL_B_LIST_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/***********************widget*translations*actions***********************
 *
 * Translations and Actions
 *
 */
/* None */

/****************************widget*resources*****************************
 *
 * List Resources
 */

#define LOFFSET(member)	XtOffsetOf(ListRec, list.member)

static XtResource resources[] = {
    { XtNuserDeleteItems, XtCCallback, XtRCallback, sizeof(XtPointer),
	LOFFSET(userDeleteItems), XtRCallback, (XtPointer)NULL
    },
    { XtNuserMakeCurrent, XtCCallback, XtRCallback, sizeof(XtPointer),
	LOFFSET(userMakeCurrent), XtRCallback, (XtPointer)NULL
    },
};

#undef LOFFSET

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

ListClassRec listClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &formClassRec,
    /* class_name		*/	"List",
    /* widget_size		*/	sizeof(ListRec),
    /* class_initialize		*/	NULL,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	False,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	True,
    /* compress_exposure	*/	True,
    /* compress_enterleave	*/	True,
    /* visible_interest		*/	False,
    /* destroy			*/	Destroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	NULL,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	GetValuesHook,
    /* accept_focus		*/	AcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query geometry		*/	NULL
  },
  { /* composite class		*/
    /* geometry_manager		*/	XtInheritGeometryManager,
    /* change_managed		*/	XtInheritChangeManaged,
    /* insert_child		*/	XtInheritInsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension		*/	NULL
  },
  { /* constraint class		*/
    /* resources		*/	NULL,
    /* num_resources		*/	0,
    /* constraint_size		*/	sizeof(ListConstraintRec),
    /* initialize		*/	NULL,
    /* destroy			*/      NULL,
    /* set_values		*/	NULL,
    /* extension		*/	NULL,
  },
  { /* manager_class fields	*/
    /* highlight_handler  	*/	NULL,
    /* focus_on_select		*/	True,
    /* traversal_handler	*/	TraversalHandler,
    /* activate			*/	NULL,
    /* event_procs		*/	NULL,
    /* num_event_proc		*/	0,
    /* register_focus		*/	RegisterFocus,
    /* version			*/	OlVersion,
    /* extension		*/	NULL,
    /* dyn_data			*/	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc		*/	_OlDefaultTransparentProc,
  },
  { /* form class		*/
					0,
  },
  { /* list class		*/
					0,
  },

};

WidgetClass scrollingListWidgetClass	= (WidgetClass)&listClassRec;


/***************************private*procedures****************************
 *
 * Private Functions
 *
 */


/*************************************************************************
 *
 * Class Procedures
 *
 */

/******************************function*header****************************
 * AcceptFocus - pass along request to ListPane
 */
static Boolean
AcceptFocus OLARGLIST((w, time))
	OLARG( Widget,	w)		/* List Widget	*/
	OLGRA( Time *,	time)
{
	return( XtCallAcceptFocus(_OlListPane(w), time) );
} /* END OF AcceptFocus() */

/******************************function*header****************************
 * Destroy():  free private storage.
 */

static void
Destroy(w)
    Widget w;
{
    XtRemoveAllCallbacks(w, XtNuserDeleteItems);
    XtRemoveAllCallbacks(w, XtNuserMakeCurrent);
}

/******************************function*header****************************
 * GetValuesHook(): get resource values aimed at List but belong to Pane
 */

static void
GetValuesHook(w, args, num_args)
    Widget	w;
    ArgList	args;
    Cardinal *	num_args;
{
    ArgList	mergedArgs;
    Cardinal	mergedCnt;

    _OlComposeArgList(args, *num_args, PaneArgs, XtNumber(PaneArgs),
		      &mergedArgs, &mergedCnt);

    if (mergedCnt != 0) {
	XtGetValues(_OlListPane(w), mergedArgs, mergedCnt);
	XtFree((char *)mergedArgs);
    }
}

/******************************function*header****************************
 * Initialize()
 */

static void
Initialize(request, new, args, num_args)
    Widget	request;
    Widget	new;
    ArgList	args;
    Cardinal *	num_args;
{
    Arg		zArgs[10];
    ArgList	mergedArgs;
    Cardinal	mergedCnt;
    Cardinal	cnt;

    /***************************************************************
		create sub-widgets

	Order of child creation is important.  See macros _OlListSBar,
	_OlListTextF & PANE.
	List pane gets created below after scrollbar.
    */
    /* create scrollbar */
    (void) XtCreateManagedWidget("scrollbar", scrollbarWidgetClass,
					new, NULL, 0);

    /* Pane must be created after SBar so pane can add callbacks to SBar.  */
    _OlComposeArgList(args, *num_args, PaneArgs, XtNumber(PaneArgs),
					&mergedArgs, &mergedCnt);

    (void) XtCreateManagedWidget("pane", listPaneWidgetClass,
					new, mergedArgs, mergedCnt);
    XtFree((char *)mergedArgs);

    _OlDeleteDescendant(_OlListPane(new));/* delete pane from traversal list */
					  /*  and add myself instead */
    _OlUpdateTraversalWidget(new, MGRPART(new)->reference_name,
			     MGRPART(new)->reference_widget, True);

    /* Add callback for consumeEvent so they can be forwared to user */
    XtAddCallback(_OlListPane(new), XtNconsumeEvent, ConsumeEventCB, (XtPointer)new);

    /* now go back and make scrollbar position relative to pane */
    cnt = 0;
    XtSetArg(zArgs[cnt], XtNyAttachOffset,
		PANE_BW); cnt++;		/* WORKAROUND Form bug */
    XtSetArg(zArgs[cnt], XtNxRefWidget, _OlListPane(new)); cnt++;
    XtSetArg(zArgs[cnt], XtNxAddWidth, True); cnt++;
    XtSetArg(zArgs[cnt], XtNxOffset,
		OlScreenPointToPixel(OL_HORIZONTAL, 2, XtScreen(new))); cnt++;
    XtSetArg(zArgs[cnt], XtNyRefWidget, _OlListPane(new)); cnt++;
    XtSetArg(zArgs[cnt], XtNyAddHeight, False); cnt++;	/* default */
    XtSetArg(zArgs[cnt], XtNyAttachBottom, True); cnt++;
    XtSetArg(zArgs[cnt], XtNyOffset, PANE_BW); cnt++;
    XtSetValues(_OlListSBar(new), zArgs, cnt);

					/* associate pane with container */
    OlAssociateWidget(new, _OlListPane(new), False);
					/* associate sbar with pane */
    OlAssociateWidget(_OlListPane(new), _OlListSBar(new), True);
}

/******************************function*header****************************
 * RegisterFocus():  return widget to register on Shell
 */

static Widget
RegisterFocus(w)
    Widget w;
{
    return (w);
}
/******************************function*header****************************
 * SetValues():  pass on SetValues aimed at the List but destined
 * for a subwidget.
 */

static Boolean
SetValues(current, request, new, args, num_args)
    Widget	current;
    Widget	request;
    Widget	new;
    ArgList	args;
    Cardinal *	num_args;
{
    ArgList	mergedArgs;
    Cardinal	mergedCnt;

    _OlComposeArgList(args, *num_args, PaneArgs, XtNumber(PaneArgs),
		      &mergedArgs, &mergedCnt);

    if (mergedCnt != 0) {
	XtSetValues(_OlListPane(new), mergedArgs, mergedCnt);
	XtFree((char *)mergedArgs);
    }

    new->core.border_width = 0;		/* always */

    return False;
}

/******************************function*header****************************
   TraversalHandler- If 'w' is ScrollingList, defer to ListPane's
	TraversalHandler, else (w is Scrollbar), return NULL
*/
static Widget
TraversalHandler OLARGLIST((mgr, w, direction, time))
    OLARG(Widget,	mgr)		/* traversal manager (me) */
    OLARG(Widget,	w)		/* starting widget */
    OLARG(OlVirtualName,direction)
    OLGRA(Time,		time)
{
	/* macro makes things more palettable */
#define PrimClass(w) \
	    ( ((PrimitiveWidgetClass)w->core.widget_class)->primitive_class )

    return ( (w == mgr) ?
	(*PrimClass(_OlListPane(mgr)).traversal_handler)
		(_OlListPane(mgr), _OlListPane(mgr), direction, time) : NULL);

#undef PrimClass
}

/*************************************************************************
 *
 * Action Procedures
 *
 */

/******************************function*header****************************
    ConsumeEventCB-
*/
static void
ConsumeEventCB OLARGLIST((w, client_data, call_data))
    OLARG( Widget,	w )
    OLARG( XtPointer,	client_data )
    OLGRA( XtPointer,	call_data )
{
#define lw ( (Widget)client_data )

    if (XtHasCallbacks(lw, XtNconsumeEvent) == XtCallbackHasSome)
	XtCallCallbacks(lw, XtNconsumeEvent, call_data);

#undef lw
}

/*************************************************************************
 *
 * Public Procedures
 *
 */
