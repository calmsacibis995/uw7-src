#pragma ident	"@(#)m1.2libs:Xm/MenuShell.c	1.6"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#include <Xm/MenuShellP.h>
#include "XmI.h"
#include <Xm/BaseClassP.h>
#include <X11/ShellP.h>
#include <X11/VendorP.h>
#include <Xm/VendorSP.h>
#include <Xm/MenuUtilP.h>
#include <Xm/RowColumnP.h>
#include <Xm/TearOffP.h>
#include <Xm/LabelP.h>
#include <Xm/CascadeBP.h>
#include <Xm/LabelGP.h>
#include <Xm/CascadeBGP.h>
#include "MessagesI.h"
#include <Xm/TransltnsP.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define Events ((unsigned int) (ButtonPressMask | ButtonReleaseMask | \
		EnterWindowMask | LeaveWindowMask))

/* Warning messages */

#ifdef I18N_MSG
#define ChildMsg	catgets(Xm_catd,MS_MShell,MSG_MS_1,_XmMsgMenuShell_0000)
#define ManageMsg	catgets(Xm_catd,MS_MShell,MSG_MS_2,_XmMsgMenuShell_0001)
#else
#define ChildMsg	_XmMsgMenuShell_0000
#define ManageMsg	_XmMsgMenuShell_0001
#endif


#define default_translations	_XmMenuShell_translations


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void _XmFastExpose() ;
static void _XmFastPopdown() ;
static void PostMenuShell() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void StructureNotifyHandler() ;
static Boolean SetValues() ;
static void Resize() ;
static void DeleteChild() ;
static void InsertChild() ;
static void ForceMenuPaneOnScreen() ;
static void PopupSharedMenuShell() ;
static XtGeometryResult GeometryManager() ;
static void ChangeManaged() ;
static void Popdown() ;
static void PopdownKids() ;
static int SkipPopdown() ;
static void PopdownOne() ;
static void PopdownEveryone() ;
static void PopdownDone() ;
static void ClearTraversalInternal() ;
static void Destroy() ;
static Widget _XmFindPopup() ;
static void _XmMenuPopupAction() ;
static void _XmMenuPopdownAction() ;

#else

static void _XmFastExpose( 
                        register XmManagerWidget rowcol) ;
static void _XmFastPopdown( 
                        XmMenuShellWidget shell) ;
static void PostMenuShell( 
                        XmMenuShellWidget menuShell,
                        XtGrabKind grab_kind,
#if NeedWidePrototypes
                        int spring_loaded) ;
#else
                        Boolean spring_loaded) ;
#endif /* NeedWidePrototypes */
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void StructureNotifyHandler( 
                        Widget wid,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *continue_to_dispatch) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Resize( 
                        Widget wid) ;
static void DeleteChild( 
                        Widget widget) ;
static void InsertChild( 
                        Widget widget) ;
static void ForceMenuPaneOnScreen( 
                        register XmRowColumnWidget rowcol,
                        register Position *x,
                        register Position *y) ;
static void PopupSharedMenuShell( 
                        Widget cbwid,
                        Widget smwid,
                        XEvent *event) ;
static XtGeometryResult GeometryManager( 
                        Widget wid,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static void ChangeManaged( 
                        Widget w) ;
static void Popdown( 
                        XmMenuShellWidget menushell,
                        XEvent *event) ;
static void PopdownKids( 
                        XmMenuShellWidget menushell,
                        XEvent *event) ;
static int SkipPopdown( 
                        XmCascadeButtonWidget cascade) ;
static void PopdownOne( 
                        Widget widget,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PopdownEveryone( 
                        Widget widget,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PopdownDone( 
                        Widget widget,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ClearTraversalInternal( 
                        XmMenuShellWidget menushell,
                        XEvent *event) ;
static void Destroy( 
                        Widget wid) ;
static Widget _XmFindPopup( 
                        Widget widget,
                        String name) ;
static void _XmMenuPopupAction( 
                        Widget widget,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void _XmMenuPopdownAction( 
                        Widget widget,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

static char tk_error[] = "XtToolkitError" ;

static XtActionsRec actionsList[] = 
{
    {"MenuShellPopdownOne", PopdownOne},
    {"MenuShellPopdownDone", PopdownDone},
    {"XtMenuPopup", _XmMenuPopupAction},
    {"XtMenuPopdown", _XmMenuPopdownAction},
    {"ClearTraversal",       _XmClearTraversal}
};
static XtResource resources[] =
{
   {     XmNdefaultFontList,
         XmCDefaultFontList,
         XmRFontList,
         sizeof(XmFontList),
         XtOffsetOf( struct _XmMenuShellRec, menu_shell.default_font_list),
         XmRString,
         (XtPointer) NULL
   },

   {      XmNlabelFontList,
          XmCLabelFontList,
          XmRFontList,
          sizeof(XmFontList),
          XtOffsetOf( struct _XmMenuShellRec, menu_shell.label_font_list),
          XmRFontList,
          (XtPointer) NULL
   },

   {      XmNbuttonFontList,
          XmCButtonFontList,
          XmRFontList,
          sizeof(XmFontList),
          XtOffsetOf( struct _XmMenuShellRec, menu_shell.button_font_list),
          XmRFontList,
          (XtPointer) NULL
   },
};


static XmBaseClassExtRec baseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    (XtInitProc)NULL,			/* InitializePrehook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPrehook	*/
    (XtInitProc)NULL,			/* InitializePosthook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    (XtInitProc)NULL,			/* secondaryCreate	*/
    (XmGetSecResDataFunc)NULL,	        /* getSecRes data	*/
    { 0 },				/* fastSubclass flags	*/
    (XtArgsProc)NULL,			/* get_values_prehook	*/
    (XtArgsProc)NULL,			/* get_values_posthook	*/

	/* ... */
};



externaldef(xmmenushellclassrec) XmMenuShellClassRec xmMenuShellClassRec = 
{
    {
        (WidgetClass) & overrideShellClassRec,  /* superclass */
        "XmMenuShell",                  /* class_name */
        sizeof (XmMenuShellWidgetRec),  /* widget size */
        ClassInitialize,                /* Class Initializer */
        ClassPartInitialize,            /* chained class init */
        FALSE,
        Initialize,                     /* instance init proc */
        (XtArgsProc)NULL,               /* init_hook proc */
        XtInheritRealize,               /* instance realize proc */
        actionsList,                    /* actions */
        XtNumber(actionsList),          /* num_actions */
        resources,                      /* resource list */
        XtNumber(resources),            /* resource_count */
        NULLQUARK,                      /* xrm_class */
        TRUE,                           /* do compress_motion */
        XtExposeCompressMaximal,	/* do compress_exposure */
        TRUE,                           /* do compress enter-leave */
        FALSE,                          /* do have visible_interest */
        Destroy,                           /* instance widget destroyb */
        Resize,                         /* resize */
        XtInheritExpose,                /* expose */
        SetValues,                      /* set values proc */
        (XtArgsFunc)NULL,               /* set values hook proc */
        XtInheritSetValuesAlmost,       /* set_values_almost proc */
        (XtArgsProc)NULL,               /* get_values_hook */
        (XtAcceptFocusProc)NULL,        /* accept_focus proc */
        XtVersion,                      /* current version */
        NULL,                           /* callback offset    */
        default_translations,		/* translation table */
        (XtGeometryHandler)NULL,        /* query geo proc */
        (XtStringProc)NULL,             /* disp accelerator */
	(XtPointer)&baseClassExtRec,    /* extension */
    },
    {                       /* composite class record */
/* BEGIN OSF fix pir 2131 */
        GeometryManager,       		/* geo mgr */
/* END OSF fix pir 2131 */
        ChangeManaged,            /* have our own */
        InsertChild,                    /* and this too */
        DeleteChild,
	NULL,                           /* Extension */
    },
    {                       /* shell class record */
        NULL,                           /* extension */
    },
    {                       /* override shell class record */
        NULL,                           /* extension */
    },
    {                       /* menushell class record */
        PopdownOne,
	PopdownEveryone,
        PopdownDone,
        PopupSharedMenuShell,
        NULL,                           /* extension */
    },
};

externaldef(xmmenushellwidgetclass) WidgetClass xmMenuShellWidgetClass = 
   (WidgetClass) &xmMenuShellClassRec;



/*
 * When using an override redirect window, it is safe to draw to the
 * window as soon as you have mapped it; you need not wait for exposure
 * events to arrive.  So ... to force menupanes to post quickly, we will
 * redraw all of the items now, and ignore the exposure events we receive
 * later.
 */
static void 
#ifdef _NO_PROTO
_XmFastExpose( rowcol )
        register XmManagerWidget rowcol ;
#else
_XmFastExpose(
        register XmManagerWidget rowcol )
#endif /* _NO_PROTO */
{
   register int i;
   register Widget child;

   /* Process the menupane */
   RC_SetExpose(rowcol, True);
   (*(XtClass(rowcol)->core_class.expose))((Widget) rowcol, NULL, NULL);

   /* Process each windowed child */
   for (i = 0; i < rowcol->composite.num_children; i++)
   {
      child = rowcol->composite.children[i];

      if (XtIsWidget(child) && 
          XtIsManaged(child))
      {
         (*(XtClass(child)->core_class.expose))(child, NULL, NULL);
      }
   }

   XFlush(XtDisplay(rowcol));

   /* Set one-shot, so menupane will ignore next expose */
   RC_SetExpose(rowcol, False);
}



/*
 * When unposting a group of cascading menupanes, we will first unmap all
 * of the MenuShell widgets, and then take care of the other work needed
 * to get the job done.  We do it this way, so that we can give the user
 * the fastest possible feedback.
 */
static void 
#ifdef _NO_PROTO
_XmFastPopdown( shell )
        XmMenuShellWidget shell ;
#else
_XmFastPopdown(
        XmMenuShellWidget shell )
#endif /* _NO_PROTO */
{
   if (RC_PopupPosted(shell->composite.children[0]))
       _XmFastPopdown((XmMenuShellWidget) RC_PopupPosted(shell->composite.children[0]));
   
   XtUnmapWidget(shell);
}



static void
#ifdef _NO_PROTO
_XmPopupI(widget, grab_kind, spring_loaded)
    Widget      widget;
    XtGrabKind  grab_kind;
    Boolean     spring_loaded;
#else
_XmPopupI(
    Widget      widget,
    XtGrabKind  grab_kind,
    Boolean  spring_loaded
    )
#endif
{
    register ShellWidget shell_widget = (ShellWidget) widget;

    if (! XtIsShell(widget)) {
	XtAppErrorMsg(XtWidgetToApplicationContext(widget),
		"invalidClass","xmPopup",tk_error,
                "XmPopup requires a subclass of shellWidgetClass",
                  (String *)NULL, (Cardinal *)NULL);
    }

    if (! shell_widget->shell.popped_up) {
	XtGrabKind call_data = grab_kind;
	XtCallCallbacks(widget, XtNpopupCallback, (XtPointer)&call_data);
	shell_widget->shell.popped_up = TRUE;
	shell_widget->shell.grab_kind = grab_kind;
	shell_widget->shell.spring_loaded = spring_loaded;
	if (shell_widget->shell.create_popup_child_proc != NULL) {
	    (*(shell_widget->shell.create_popup_child_proc))(widget);
	}
	if (grab_kind == XtGrabExclusive) {
	    _XmAddGrab(widget, TRUE, spring_loaded);
	} else if (grab_kind == XtGrabNonexclusive) {
	    _XmAddGrab(widget, FALSE, spring_loaded);
	}
	XtRealizeWidget(widget);
	XMapRaised(XtDisplay(widget), XtWindow(widget));
    } else
	XRaiseWindow(XtDisplay(widget), XtWindow(widget));

} /* _XmPopupI */

void
#ifdef _NO_PROTO
_XmPopupSpringLoaded( shell )
        Widget shell ;
#else
_XmPopupSpringLoaded(
        Widget shell)
#endif /* _NO_PROTO */
{   
  _XmPopupI( shell, XtGrabExclusive, True) ;
} 

void
#ifdef _NO_PROTO
_XmPopup( shell, grab_kind )
        Widget shell ;
        XtGrabKind grab_kind;
#else
_XmPopup(
        Widget shell,
        XtGrabKind grab_kind)
#endif /* _NO_PROTO */
{   
  _XmPopupI( shell, grab_kind, FALSE) ;
} 

/*
 * Post the requested menu shell widget,
 */
static void 
#ifdef _NO_PROTO
PostMenuShell( menuShell, grab_kind, spring_loaded )
        XmMenuShellWidget menuShell ;
        XtGrabKind grab_kind ;
        Boolean spring_loaded ;
#else
PostMenuShell(
        XmMenuShellWidget menuShell,
        XtGrabKind grab_kind,
#if NeedWidePrototypes
        int spring_loaded )
#else
        Boolean spring_loaded )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XEvent event;

   /* Remember the next event from the queue
      so we don't popdown on the replay */
   event.xbutton.type = ButtonPress;
   event.xbutton.serial = 
     XLastKnownRequestProcessed(XtDisplay((Widget) menuShell));
   event.xbutton.send_event = 0;
   event.xbutton.time = 
     XtLastTimestampProcessed(XtDisplay((Widget) menuShell));
   event.xbutton.display = XtDisplay((Widget)menuShell);
   _XmRecordEvent((XEvent *) &event);

   if (spring_loaded)
     {   
       /* XmPopupSpringLoaded ((Widget) menuShell); */
       /* Since the modality implementation requires the use of
        *   _XmAddGrab() instead of XtAddGrab(), XtPopupSpringLoaded()
        *   must be replaced with _XmPopupSpringLoaded(), which uses
        *   the appropriate AddGrab routine.
        */
       _XmPopupSpringLoaded( (Widget) menuShell) ;
     } 
   else
       _XmPopup ((Widget) menuShell, grab_kind);

   /* mark the row column as NOT popping down */
   RC_SetPoppingDown(menuShell->composite.children[0], False);
}

/*
 * Class Initialize.  Set up fast subclassing.
 */
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
   _XmInitializeExtensions();
   baseClassExtRec.record_type = XmQmotif;
}

static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
   _XmBaseClassPartInitialize(wc);
   _XmFastSubclassInit (wc, XmMENU_SHELL_BIT);
}


/*
 * Initialize routine
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Initialize( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmFontList defaultFont;
    XmMenuShellWidget ms = (XmMenuShellWidget) new_w;

    new_w->core.background_pixmap = None;

    XtBorderWidth (new_w) = 0;            /* Style Guide says so */

    XtRealizeWidget (new_w);

    ms->menu_shell.focus_data = (XmFocusData)_XmCreateFocusData();

    ms->menu_shell.focus_policy = XmEXPLICIT;

    ms->shell.allow_shell_resize = TRUE;

    /* Assume, for now, that the application created this shell */
    ms->menu_shell.private_shell = False;

    defaultFont =  ms->menu_shell.button_font_list;

    if ( !defaultFont )
    {
	/* backward compatibility */
	defaultFont =  ms->menu_shell.default_font_list; 

	if ( !defaultFont )
	    defaultFont = _XmGetDefaultFontList( (Widget) ms, 
					    XmBUTTON_FONTLIST);
    }

    ms->menu_shell.button_font_list = XmFontListCopy (defaultFont);

    defaultFont =  ms->menu_shell.label_font_list;

    if ( !defaultFont )
    {
	/* backward compatibility */
	defaultFont =  ms->menu_shell.default_font_list; 
	if ( !defaultFont )
	    defaultFont = _XmGetDefaultFontList( (Widget) ms,
						XmLABEL_FONTLIST);
    }

    ms->menu_shell.label_font_list = XmFontListCopy (defaultFont);   

    _XmSetSwallowEventHandler((Widget) ms, True);

    /*
     * Take over EventHandler in Shell, to dump the configure notify
     * events since it is not smart enough to ignore stale events.
     */
    XtInsertEventHandler((Widget) ms, (EventMask) StructureNotifyMask, True,
			 StructureNotifyHandler, (XtPointer) NULL,
			 XtListHead);

}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
StructureNotifyHandler( wid, closure, event, continue_to_dispatch )
        Widget wid ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *continue_to_dispatch ;
#else
StructureNotifyHandler(
        Widget wid,
        XtPointer closure,
        XEvent *event,
        Boolean *continue_to_dispatch )
#endif /* _NO_PROTO */
{
    /*
     * This event handler is inserted at the head of the list so that
     * the configure notify events will be ignored.  This is because the
     * configures are done directly when needed, not when the event handler
     * is called.
     */
    if (event->type ==  ConfigureNotify)
    {
	/* all the configure notify events should be ignored */
	*continue_to_dispatch = False;
    }
}

/*
 * set values
 *
 * Don't allow the allowShellResize flag to be set false
 */
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args, num_args )
        Widget cw ;
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
   XmMenuShellWidget new_w = (XmMenuShellWidget) nw ;
   XmMenuShellWidget old_w = (XmMenuShellWidget) cw ;
	XmFontList defaultFont;

	if (new_w->menu_shell.button_font_list != 
		old_w->menu_shell.button_font_list)
	{
		XmFontListFree(old_w->menu_shell.button_font_list);
		defaultFont = new_w->menu_shell.button_font_list;
		if (!defaultFont)
			{
			defaultFont = new_w->menu_shell.default_font_list;
			if (!defaultFont )
			   defaultFont = _XmGetDefaultFontList( (Widget) new_w,
                                            XmBUTTON_FONTLIST);
			}
	new_w->menu_shell.button_font_list = XmFontListCopy (defaultFont);
	}

	if (new_w->menu_shell.label_font_list != 
		old_w->menu_shell.label_font_list)
	{
		XmFontListFree(old_w->menu_shell.label_font_list);
		defaultFont = new_w->menu_shell.label_font_list;
		if (!defaultFont)
			{
			defaultFont = new_w->menu_shell.default_font_list;
			if (!defaultFont )
			   defaultFont = _XmGetDefaultFontList( (Widget) new_w,
                                            XmLABEL_FONTLIST);
			}
	new_w->menu_shell.label_font_list = XmFontListCopy (defaultFont);
	}

   new_w->shell.allow_shell_resize = TRUE;
   return (TRUE);
}


/*
 * Resize our first child
 *
 * The statement pertaining to shared menupanes is commented out because
 * if the user is rapidly moving thru the menubar, an old configure notify
 * event may come in, cause the shell to resize the wrong child.
 */
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
   XmMenuShellWidget ms = (XmMenuShellWidget) wid ;

   if (((ms->composite.num_children == 1) && 
        (XtIsManaged(ms->composite.children[0]))))
/*
     || ((ms->composite.num_children > 1) && (ms->shell.popped_up)))
*/
   {
      Widget child = ms->composite.children[0];

      XtConfigureWidget(child, -ms->core.border_width, -ms->core.border_width,
	 ms->core.width, ms->core.height, ms->core.border_width);
   }
}


/*
 * DeleteChild routine
 */
static void 
#ifdef _NO_PROTO
DeleteChild( widget )
        Widget widget ;
#else
DeleteChild(
        Widget widget )
#endif /* _NO_PROTO */
{
   XmMenuShellWidget parent = (XmMenuShellWidget)XtParent(widget);

   /* Remove the child as our tab group */
   XmRemoveTabGroup(widget);

   /* Let composite class finish the work */
   (*(((CompositeWidgetClass)compositeWidgetClass)->composite_class.
          delete_child))(widget);

   /*
    * If we had multiple children, but we've now dropped down to 1 child,
    * then we need to unmanage the child.
    */
   if (parent->composite.num_children == 1)
   { 
       if (!parent->shell.popped_up)
           (parent->composite.children[0])->core.managed = False;
       XMapWindow(XtDisplay(parent), XtWindow(parent->composite.children[0]));
   }
   else if ((parent->composite.num_children == 0) && 
            (parent->menu_shell.private_shell) &&
            (!parent->core.being_destroyed))
   {
       /* Destroy private shells, when they have no more children */
       XtDestroyWidget((Widget) parent);
   }
}

/*
 * InsertChild routine
 */
static void 
#ifdef _NO_PROTO
InsertChild( widget )
        Widget widget ;
#else
InsertChild(
        Widget widget )
#endif /* _NO_PROTO */
{
    CompositeWidget parent = (CompositeWidget) widget->core.parent;

    if (XmIsRowColumn (widget))
    {
       if ((RC_Type(widget) == XmMENU_PULLDOWN) ||
	   (RC_Type(widget) == XmMENU_POPUP))
       {
	  XtAddEventHandler (widget, EnterWindowMask, FALSE,
			     _XmEnterRowColumn, FALSE);
       }

       /*
        * add to composite list
        */

       (*((CompositeWidgetClass)compositeWidgetClass)->composite_class.
              insert_child) (widget);

       XtRealizeWidget(widget);

       /*
        * If only one child, then we will manage and unmanage it when we
        * want it to post or unpost.  If there are more than one child, then
        * we will have them always managed, and we will use a different
        * technique for posting and unposting.
        */
       if (parent->composite.num_children == 1)
       {
          XtSetKeyboardFocus(( Widget) parent, widget);
       }
       else if (parent->composite.num_children == 2)
       {
          XtManageChildren(parent->composite.children, 2);
       }
       else
       {
          XtManageChild(widget);
       }
       if (parent->composite.num_children == 1)
       {
	  XMapWindow(XtDisplay(widget), XtWindow(widget));
       }
    }
    else
       _XmWarning(widget, ChildMsg);
}


static void 
#ifdef _NO_PROTO
ForceMenuPaneOnScreen( rowcol, x, y )
        register XmRowColumnWidget rowcol ;
        register Position *x ;
        register Position *y ;
#else
ForceMenuPaneOnScreen(
        register XmRowColumnWidget rowcol,
        register Position *x,
        register Position *y )
#endif /* _NO_PROTO */
{
   Position rightEdgeOfMenu, bottomEdgeOfMenu;
   Dimension dispWidth, dispHeight;
   Position old_x;
   Widget pulldown_button = RC_CascadeBtn(rowcol);
   Dimension RowColBorderWidth = rowcol->core.border_width << 1;
   Dimension CascadeBorderWidth = 0;

   if (pulldown_button)
      CascadeBorderWidth = pulldown_button->core.border_width << 1;

   /* Force the rowcol to be completely visible */

   old_x = *x;
   rightEdgeOfMenu = *x + RowColBorderWidth + rowcol->core.width;
   bottomEdgeOfMenu = *y + RowColBorderWidth + rowcol->core.height;
   dispWidth = WidthOfScreen (XtScreen(rowcol));
   dispHeight = HeightOfScreen (XtScreen(rowcol));

   /*
    * For OPTION menus, if the submenu is [partially] offscreen, offset it
    * off the button.
    */
   if (pulldown_button && XtParent(pulldown_button) &&
	XmIsRowColumn(XtParent(pulldown_button)) &&
       (RC_Type(XtParent(pulldown_button)) == XmMENU_OPTION))
   {
      if (bottomEdgeOfMenu >= (Position)dispHeight)
      {
          *y = dispHeight - rowcol->core.height - RowColBorderWidth - 1;
          *x = old_x + pulldown_button->core.width + CascadeBorderWidth;
	  rightEdgeOfMenu = *x + RowColBorderWidth + rowcol->core.width;
          bottomEdgeOfMenu = *y + RowColBorderWidth + rowcol->core.height;
      }

      if (*y < 0)
      {
          *y = 0;

	  /* Consider CascadeBtn as well as RowCol width to allow multi
	   * column RowColumn
	   */
          *x = old_x + pulldown_button->core.width + CascadeBorderWidth;
	  rightEdgeOfMenu = *x + RowColBorderWidth + rowcol->core.width;
          bottomEdgeOfMenu = *y + RowColBorderWidth + rowcol->core.height;
      }

      if (rightEdgeOfMenu >= (Position)dispWidth)
      {
	  *x = old_x - rowcol->core.width + RowColBorderWidth;
	  rightEdgeOfMenu = *x + RowColBorderWidth + rowcol->core.width;
      }

      if (*x < 0)
      {
          *x = old_x + pulldown_button->core.width + CascadeBorderWidth;
	  rightEdgeOfMenu = *x + RowColBorderWidth + rowcol->core.width;
      }
   }

   /*
    * If the submenu is offscreen force it completely on.
    */
   if (rightEdgeOfMenu >= (Position)dispWidth)
       *x -= (rightEdgeOfMenu - dispWidth + 1);

   if (bottomEdgeOfMenu >= (Position)dispHeight)
   {
      if (pulldown_button && XtParent(pulldown_button) &&
	  (RC_Type(XtParent(pulldown_button)) == XmMENU_BAR))
      {
	 Position y_temp = *y;

	 /* this menu pane is being pulled down from a bar */
	 /* it doesn't fit, so we have to place it above   */
	 /* the bar -- if it will fit                      */

	 y_temp -= (CascadeBorderWidth
		    + pulldown_button->core.height
		    + RowColBorderWidth
		    + rowcol->core.height + 1);
			
	 if (y_temp > 0)
	     *y = y_temp;
      }
      else
	  *y -= (bottomEdgeOfMenu - dispHeight + 1);
   }

   /* Make sure that the top left corner os on screen! */
   if (*x < 0)
       *x = 0;

   if (*y < 0)
       *y = 0;
}

/*
 * Method for posting a multi-paned menushell
 */
static void 
#ifdef _NO_PROTO
PopupSharedMenuShell( cbwid, smwid, event )
        Widget cbwid ;
        Widget smwid ;
        XEvent *event ;
#else
PopupSharedMenuShell(
        Widget cbwid,
        Widget smwid,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmCascadeButtonWidget cascadebtn = (XmCascadeButtonWidget) cbwid ;
   XmRowColumnWidget submenu = (XmRowColumnWidget) smwid ;
   register XmMenuShellWidget popup = (XmMenuShellWidget) XtParent(submenu);
   XmRowColumnWidget parent_menu;
   Position x, y;
   Dimension height, width;
   int _index = 0;
   register int i;
   Boolean popped_up = popup->shell.popped_up;
   XmRowColumnWidget old_rowcol = NULL;
   XmCascadeButtonWidget old_cascadebtn = NULL;
   XmMenuState mst = _XmGetMenuState((Widget)submenu);
   Time _time = __XmGetDefaultTime(cbwid, event);

   /* Find out which child is trying to get posted */
   for (i = 0; i < popup->composite.num_children; i++)
   {
      if (popup->composite.children[i] == (Widget)submenu)
      {
	 _index = i;
	 break;
      }
   }

   /* Swap only if the submenu is not already child[0] */
   if (_index != 0) 
   {
      /* Swap places in the children list */
      old_rowcol = (XmRowColumnWidget) popup->composite.children[0];
      old_cascadebtn = (XmCascadeButtonWidget)RC_CascadeBtn(old_rowcol);

      RC_SetPoppingDown(old_rowcol, True);
      popup->composite.children[_index] = (Widget)old_rowcol;
      popup->composite.children[0] = (Widget)submenu;
      RC_SetPoppingDown(submenu, False);

      if (RC_TornOff(old_rowcol))
	 _XmRestoreTearOffToToplevelShell((Widget)old_rowcol, event);
      else
	 XUnmapWindow(XtDisplay(old_rowcol), XtWindow(old_rowcol));
   } else
      if (cascadebtn != (XmCascadeButtonWidget)
	      RC_CascadeBtn(popup->composite.children[0]))
      {
	 /* If the 2 different cascade buttons popped up this submenu,
	  * unmap it in preparation for the 'move'.  Also save the old
	  * cascade so it can be unhighlighted.
	  */
         old_cascadebtn = (XmCascadeButtonWidget)RC_CascadeBtn(submenu);
	 XUnmapWindow(XtDisplay(submenu), XtWindow(submenu));
      }

   if (popped_up ||
       (old_cascadebtn && RC_TornOff(XtParent(old_cascadebtn)) && 
	!XmIsMenuShell(XtParent(XtParent(old_cascadebtn)))))
   {
      XmCascadeButtonHighlight( (Widget) old_cascadebtn, False);
   }

   if (submenu->core.being_destroyed)
       return;

   /* It seems that menupanes that share their menushell are always managed.
    * When reposting a tear off, the restoration code unmanages the pane.
    * So set it here!
    */
   submenu->core.managed = TRUE;

   if (XmIsCascadeButtonGadget(cascadebtn))
       (* xmLabelGadgetClassRec.label_class.menuProcs)
	   (XmMENU_CASCADING, (Widget) cascadebtn, NULL, submenu, event);
   else
       (* xmLabelClassRec.label_class.menuProcs)
	   (XmMENU_CASCADING, (Widget) cascadebtn, NULL, submenu, event);

   /*
    * Set us as the active tab group for our parent
    */
   _XmSetActiveTabGroup(popup->menu_shell.focus_data, (Widget)submenu);

   /*
    * get the shell widget in sync with the menu widget
    * We keep the menu at 0,0 and shell at the menu's position.
    * the menu's position is just used as a place to keep shell's loc
    *
    * NOTE: the menu may be moved down slightly to take into account the
    * tear off control.
    */

   /* Always adjust to the size of the menupane */
   width = XtWidth(submenu);
   height = XtHeight(submenu);

   if (RC_WidgetHasMoved (submenu))     /* but make sure it didn't */
   {                                    /* just move to where it */
                                        /* is now.  Happens with */
                                        /* pulldowns a lot */
      x = XtX(submenu);
      y = XtY(submenu);
      
      ForceMenuPaneOnScreen(submenu,&x,&y);
      
      XtX (submenu) = XtY (submenu) = (-1 * XtBorderWidth(submenu));

      if (RC_WindowHasMoved(submenu))
      {
	 XMoveWindow (XtDisplay(submenu), XtWindow(submenu), 
		      XtX(submenu), XtY(submenu));
	 RC_SetWindowMoved (submenu, FALSE);
      }

      RC_SetWidgetMoved (submenu, FALSE);
   }
   else
   {
      x = XtX(popup);
      y = XtY(popup);
   }
   
   XtConfigureWidget((Widget) popup, x, y, width, height, popup->core.border_width);

   XMapWindow(XtDisplay(submenu), XtWindow(submenu));

   if (popped_up && 
       !((old_rowcol == submenu) && (cascadebtn == old_cascadebtn)))
      _XmCallRowColumnUnmapCallback((Widget)old_rowcol, event);

   _XmCallRowColumnMapCallback((Widget)submenu, event);

   /*
    * if there is a tear off control, set the initial focus to the first
    * real child.  If none of the children is traversible, let the
    * traversal code set it to the toc.
    */
   if (RC_TearOffControl(submenu) && XtIsManaged(RC_TearOffControl(submenu)))
   {
       for (i=0; i < submenu->composite.num_children; i++)
       {
	   if (XmIsTraversable(submenu->composite.children[i]))
	   {
	       _XmSetInitialOfTabGroup( (Widget) submenu,
				       submenu->composite.children[i]);
	       break;
	   }
       }
   }

   (*(((XmRowColumnClassRec *)XtClass(submenu))->row_column_class.
      menuProcedures)) (XmMENU_ARM, (Widget) submenu);

   if (popped_up)
      _XmFastExpose((XmManagerWidget) submenu);
   else
   {
      /*  
       * we are the root of the chain and better do the exclusive grab
       */
      parent_menu = (XmRowColumnWidget) XtParent (cascadebtn);

      if (RC_Type(parent_menu) == XmMENU_OPTION)
      {
	 /* Remember the time in case this is just a BSelect Click */
	 if ((event->type == ButtonPress) || (event->type == ButtonRelease))
	    mst->MS_LastManagedMenuTime = event->xbutton.time;

         PostMenuShell(popup, XtGrabExclusive, True);
         _XmFastExpose((XmManagerWidget) submenu);

	 _XmMenuFocus(XtParent(submenu), XmMENU_BEGIN, _time);

	 _XmGrabPointer((Widget) submenu, True, Events,
	    GrabModeSync, GrabModeAsync, None, 
	    XmGetMenuCursor(XtDisplay(submenu)), _time);
      }
      else
      {
	 PostMenuShell(popup, XtGrabNonexclusive, False);
	 _XmFastExpose((XmManagerWidget) submenu);

	 if ((RC_Type(parent_menu) == XmMENU_BAR) &&
	     (RC_BeingArmed(parent_menu)))
	 {
	    _XmGrabPointer((Widget) parent_menu, True, Events,
	       GrabModeSync, GrabModeAsync, None, 
	       XmGetMenuCursor(XtDisplay(parent_menu)), _time);
									      
	    RC_SetBeingArmed(parent_menu, False);
	 } 

	 /* We always must move the focuse for the leaf menu pane */
	 _XmMenuFocus(XtParent(submenu), XmMENU_MIDDLE, _time);
      }
   }

   /*
    * now highlight the pulldown entry widget that
    * pulled down the menu
    */
   XmCascadeButtonHighlight ((Widget) cascadebtn, TRUE);

   if (popped_up)
   {
      XmGadget activeChild;
      
      activeChild = (XmGadget)old_rowcol->manager.active_child;
      
      /*
       * If the active child was a gadget, then we need to 
       * handle the sending of the focus out ourselves.
       */
      if (activeChild && XmIsGadget(activeChild))
      {
	 _XmDispatchGadgetInput((Widget) activeChild, NULL, 
	    XmFOCUS_OUT_EVENT);
      }
   }
}                       

/* BEGIN OSF fix pir 2131 */
/* This is ripped off from Xt/Shell.c and modified to also deal with X
and Y
 * requests.
 */
/*
 * This is gross, I can't wait to see if the change happened so I will
ask
 * the window manager to change my size and do the appropriate X work.
 * I will then tell the requester that he can.  Care must be taken
because
 * it is possible that some time in the future the request will be
 * asynchronusly denied and the window reverted to it's old size/shape.
*/

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryManager( wid, request, reply )
	Widget wid;
	XtWidgetGeometry *request;
	XtWidgetGeometry *reply;
#else
GeometryManager( 
	Widget wid,
	XtWidgetGeometry *request,
	XtWidgetGeometry *reply )
#endif
{
  ShellWidget shell = (ShellWidget)(wid->core.parent);
  XtWidgetGeometry my_request;

  if(shell->shell.allow_shell_resize == FALSE && XtIsRealized(wid))
    return(XtGeometryNo);

  if(!XtIsRealized((Widget)shell)){
    if (request->request_mode & (CWX | CWY)) {
      return(XtGeometryNo);
    }
    *reply = *request;
    if(request->request_mode & CWWidth)
      wid->core.width = shell->core.width = request->width;
    if(request->request_mode & CWHeight)
      wid->core.height = shell->core.height = request->height;
    if(request->request_mode & CWBorderWidth)
      wid->core.border_width = shell->core.border_width =
        request->border_width;
    return(XtGeometryYes);
  }

  /* %%% worry about XtCWQueryOnly */
  my_request.request_mode = 0;
  if (request->request_mode & CWX) {
    my_request.x = request->x;
    my_request.request_mode |= CWX;
  }
  if (request->request_mode & CWY) {
    my_request.y = request->y;
    my_request.request_mode |= CWY;
  }
  if (request->request_mode & CWWidth) {
    my_request.width = request->width;
    my_request.request_mode |= CWWidth;
  }
  if (request->request_mode & CWHeight) {
    my_request.height = request->height;
    my_request.request_mode |= CWHeight;
  }
  if (request->request_mode & CWBorderWidth) {
    my_request.border_width = request->border_width;
    my_request.request_mode |= CWBorderWidth;
  }
  /*
   * Don't make geometry request if the shared menupane has a
   * visible pulldown, which isn't the current pulldown widget. This prevents
   * another pulldown menu to resize the menu shell's width and height, thus
   * dynamically resizing the visible pulldown.
   */
  if ((wid != shell->composite.children[0])
      || ((wid == shell->composite.children[0])
	  &&  (XtMakeGeometryRequest((Widget)shell, &my_request, NULL)
	       == XtGeometryYes))) {
    if (request->request_mode & CWX) {
      wid->core.x = 0;
    }
    if (request->request_mode & CWY) {
      wid->core.y = 0;
    }
    if (request->request_mode & CWWidth) {
      wid->core.width = request->width;
    }
    if (request->request_mode & CWHeight) {
      wid->core.height = request->height;
    }
    if (request->request_mode & CWBorderWidth) {
      wid->core.border_width = request->border_width;
      wid->core.x = wid->core.y = -request->border_width;
    }
    return XtGeometryYes;
  } else return XtGeometryNo;
}
/* END OSF fix pir 2131 */


/*
 * ChangeManaged
 */
static void 
#ifdef _NO_PROTO
ChangeManaged( w )
        Widget w ;
#else
ChangeManaged(
        Widget w )
#endif /* _NO_PROTO */
{
   register XmMenuShellWidget popup = (XmMenuShellWidget) w;
   XmRowColumnWidget parent_menu;
   Position x, y;
   Dimension height, width;
   XmCascadeButtonWidget cascadebtn;
   register Widget child;
   register XmRowColumnWidget rowcol = 
       (XmRowColumnWidget)popup->composite.children[0];
   int i;
   XmMenuState mst = _XmGetMenuState((Widget)w);
   /* Time _time = XtLastTimestampProcessed(XtDisplay(w)); */
   Time _time = CurrentTime;

   mst->RC_ButtonEventStatus.waiting_to_be_managed = FALSE;

   /* Don't handle multi-paned shells here */
   if (popup->composite.num_children > 1)
       return;

   if (rowcol->core.being_destroyed)
       return;

   if (XtIsManaged (rowcol)) 
   {
      if  ((RC_Type(rowcol) == XmMENU_PULLDOWN) &&
	   (! RC_CascadeBtn(rowcol)))
      {
	 /*
	  * this pulldown does not have a complete path to
	  * a toplevel pane, so it can not be managed
	  */
	 _XmWarning(w, ManageMsg);
	 XtUnmanageChild ((Widget) rowcol);
	 return;
      } 
      else  
         if (RC_Type(rowcol) == XmMENU_POPUP) 
         {
	    /* Verify MenuPost/WhichButton */
#ifdef BETTER_BUT_NOT_1_1_COMPATABLE
	    if ((mst->RC_ButtonEventStatus.time !=
		    XtLastTimestampProcessed(XtDisplay(rowcol))) ||
		!mst->RC_ButtonEventStatus.verified)
#else
            if ((mst->RC_ButtonEventStatus.time ==
                    XtLastTimestampProcessed(XtDisplay(rowcol))) &&
                 !mst->RC_ButtonEventStatus.verified)
#endif
	    {
	       mst->RC_ButtonEventStatus.verified = False;
	       XtUnmanageChild ((Widget) rowcol);
	       return;
	    }
	    /* Remember the time in case this is just a BMenu Click */
	    mst->MS_LastManagedMenuTime = mst->RC_ButtonEventStatus.time;
	 }

      /* Set us as the active tab group for our parent */
      _XmSetActiveTabGroup(popup->menu_shell.focus_data, (Widget)rowcol);
	 
      /* make the callback now to give the application a crack at
       * repositioning the pane.
       */
      _XmCallRowColumnMapCallback((Widget)rowcol, 
	 (XEvent *)&mst->RC_ButtonEventStatus.event);
      
      /*
       * get the shell widget in sync with the menu widget
       * We keep the menu at 0,0 and shell at the menu's position.
       * the menu's position is just used as a place to keep shell's loc
       *
       * If a tear off control is relevant, move the pane down to allow for
       * its height.
       */

      width = XtWidth(rowcol);
      height = XtHeight(rowcol);
      
      if (RC_WidgetHasMoved (rowcol))   
      {                                 
	 x = XtX(rowcol);
	 y = XtY(rowcol);
	      
         ForceMenuPaneOnScreen(rowcol,&x,&y);
	 
	 XtX (rowcol) = XtY (rowcol) = (-1 * XtBorderWidth(rowcol));

	 if (RC_WindowHasMoved(rowcol))
	 {
	    XMoveWindow (XtDisplay(rowcol), XtWindow(rowcol), 
			 XtX(rowcol), XtY(rowcol));
	    RC_SetWindowMoved (rowcol, FALSE);
	 }
	      
	 RC_SetWidgetMoved (rowcol, FALSE);
      }
      else
      {
	 x = XtX(popup);
	 y = XtY(popup);
      }
	 
      XtConfigureWidget((Widget) popup, x, y, width, height,
			popup->core.border_width);
	 
      /*
       * if there is a tear off control, set the initial focus to the first
       * real child.  If none of the children is traversible, let the
       * traversal code set it to the toc.
       */
      if (RC_TearOffControl(rowcol) && XtIsManaged(RC_TearOffControl(rowcol)))
      {
	  for (i=0; i < rowcol->composite.num_children; i++)
	  {
	      if (XmIsTraversable(rowcol->composite.children[i]))
	      {
		  _XmSetInitialOfTabGroup( (Widget) rowcol,
					  rowcol->composite.children[i]);
		  break;
	      }
	  }
      }
      
      (*(((XmRowColumnClassRec *)XtClass(rowcol))->row_column_class.
	 menuProcedures)) (XmMENU_ARM, (Widget) rowcol);

      /*
       * now figure out exactly how to pop it up
       */
	 
      switch (RC_Type(rowcol))
      {
       case XmMENU_POPUP:
	 PostMenuShell((XmMenuShellWidget) w, XtGrabExclusive, True);
	 _XmFastExpose((XmManagerWidget) rowcol);

	 _XmMenuFocus(XtParent(rowcol), XmMENU_BEGIN, _time);

	 /* For popups, put pointer grab on root menu pane */
         _XmGrabPointer((Widget) rowcol, True, Events,
            GrabModeSync, GrabModeAsync, None, 
	    XmGetMenuCursor(XtDisplay(rowcol)), _time);

	 /* To support menu replay, keep the pointer in sync mode */
	 XAllowEvents(XtDisplay(rowcol), SyncPointer, _time);

         (*(((XmRowColumnClassRec *)XtClass(rowcol))->row_column_class.
            menuProcedures)) (XmMENU_TRAVERSAL, (Widget) rowcol, FALSE);

	 break;

       case XmMENU_PULLDOWN:
	 cascadebtn = (XmCascadeButtonWidget) RC_CascadeBtn(rowcol);
	 parent_menu = (XmRowColumnWidget) XtParent (cascadebtn);

	 /* Make sure the correct item gets the focus */
	 if (!_XmGetInDragMode((Widget) rowcol))
	 {
	    if (RC_MemWidget(rowcol) && 
		(RC_Type(parent_menu) == XmMENU_OPTION))
	    {
	       if (XtParent(RC_MemWidget(rowcol)) == (Widget)rowcol)
		  _XmSetInitialOfTabGroup( (Widget) rowcol,
					  RC_MemWidget(rowcol));

	       else
	       {
		  /* Find our link to the memory widget */
		  child = (Widget)RC_MemWidget(rowcol);
		  while (child)
		  {
		     if (XtParent(child) == (Widget)rowcol)
			 break;
		     
		     child = (Widget)RC_CascadeBtn(XtParent(child));
		  }
		  rowcol->manager.active_child =  child;
	       }
	    }
	    else
		rowcol->manager.active_child = NULL;
	    
	    if ((parent_menu->manager.active_child != (Widget)cascadebtn) &&
		((RC_Type(parent_menu) == XmMENU_POPUP) ||
		 (RC_Type(parent_menu) == XmMENU_PULLDOWN)))
	    {
	       /* Move the focus to the child */
	       _XmGrabTheFocus( (Widget) cascadebtn, NULL);
	    }
	 }
	 
	 if (RC_Type(parent_menu) == XmMENU_OPTION)
	 {
	    /* Remember the time in case this is just a BSelect Click */
	    mst->MS_LastManagedMenuTime = mst->RC_ButtonEventStatus.time;

	    PostMenuShell((XmMenuShellWidget) w, XtGrabExclusive, True);
	    _XmFastExpose((XmManagerWidget) rowcol);
	      
	    _XmMenuFocus(XtParent(rowcol), XmMENU_BEGIN, _time);

	    /* For popups, put pointer grab on root menu pane */
            _XmGrabPointer((Widget) rowcol, True, Events,
               GrabModeSync, GrabModeAsync, None, 
	       XmGetMenuCursor(XtDisplay(rowcol)), _time);

	    /* To support menu replay, keep the pointer in sync mode */
	    XAllowEvents(XtDisplay(rowcol), SyncPointer, _time);
	 }
	 else
	 {
	    PostMenuShell((XmMenuShellWidget) w, XtGrabNonexclusive, False);
	    _XmFastExpose((XmManagerWidget) rowcol);

	    /* For a menubar, put the pointer grab on the menubar. */
	    if ((RC_Type(parent_menu) == XmMENU_BAR) &&
		(RC_BeingArmed(parent_menu)))
	    {
               _XmGrabPointer((Widget) parent_menu, True, Events,
                  GrabModeSync, GrabModeAsync, None, 
		  XmGetMenuCursor(XtDisplay(parent_menu)), _time);

	       RC_SetBeingArmed(parent_menu, False);
	    }

	    /* We always must grab the keyboard for the leaf menu pane */
	    _XmMenuFocus(XtParent(rowcol), XmMENU_MIDDLE, _time);
	    XtSetKeyboardFocus(XtParent(rowcol), (Widget) rowcol);
	    
	    /*
	     * now highlight the pulldown entry widget that
	     * pulled down the menu
	     */
	    XmCascadeButtonHighlight ((Widget) cascadebtn, TRUE);
	    break;
	 }
      }
   }
   else
   {
      /* popdown anything that is still up */
      _XmMenuFocus(w, XmMENU_END, _time);
      
      (* (((XmMenuShellClassRec *)(popup->
	 core.widget_class))->menu_shell_class.popdownEveryone))
	    ((Widget) popup, NULL, NULL, NULL);
      
      if (RC_Type(rowcol) == XmMENU_POPUP)
      {
	 XtUngrabPointer(w, _time);
      }
   }
   mst->RC_ButtonEventStatus.verified = False;
}                       


void
#ifdef _NO_PROTO
_XmPopdown( widget )
        Widget widget ;
#else
_XmPopdown(
        Widget widget)
#endif /* _NO_PROTO */
{   
  register ShellWidget shell_widget = (ShellWidget) widget;

  if (! XtIsShell(widget)) {
    XtAppErrorMsg(XtWidgetToApplicationContext(widget),
		  "invalidClass","xmPopdown",tk_error,
		  "XmPopdown requires a subclass of shellWidgetClass",
		  (String *)NULL, (Cardinal *)NULL);
  }

  if (shell_widget->shell.popped_up) {
    XtGrabKind grab_kind = shell_widget->shell.grab_kind ;
    XWithdrawWindow( XtDisplay( shell_widget), XtWindow( shell_widget),
		    XScreenNumberOfScreen( XtScreen( shell_widget))) ;
    if(    grab_kind != XtGrabNone    )
      {
	_XmRemoveGrab((Widget) shell_widget) ;
      }
    shell_widget->shell.popped_up = FALSE ;
    XtCallCallbacks((Widget) shell_widget, XtNpopdownCallback, (XtPointer) &grab_kind) ;
  } 
}

/*
 * actually popdown a menuShell widget, also flag as unmanaged
 * its menu child and unhighlight the cascadebutton that popped up this
 * widget
 */
static void 
#ifdef _NO_PROTO
Popdown( menushell, event )
        XmMenuShellWidget menushell ;
        XEvent *event ;
#else
Popdown(
        XmMenuShellWidget menushell,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmRowColumnWidget  rowcol = (XmRowColumnWidget) 
                                 menushell->composite.children[0];

    if (menushell->shell.popped_up)
    {
        /* mark the row column as popping down */
        RC_SetPoppingDown (rowcol, True);

        /* XmPopdown ((Widget) menushell); */
        /* Since the MenuShell may be popped up spring-loaded, 
         *   and since XmPopdown will remove the grab using XtRemoveGrab
         *   if the shell is spring loaded, and since we must use
         *   _XmRemoveGrab because of the modality implementation, we
         *   must use our own version of _XmPopdown to pop down the
         *   MenuShell.
         */
        _XmPopdown( (Widget) menushell) ;

	(*(((XmRowColumnClassRec *)XtClass(rowcol))->row_column_class.
	   menuProcedures)) (XmMENU_DISARM, (Widget) rowcol); 

        ClearTraversalInternal(menushell, event);

        /* Don't unmanage if we're sharing the menu shell */
        if (menushell->composite.num_children == 1)
           rowcol->core.managed = FALSE;

	_XmCallRowColumnUnmapCallback((Widget)rowcol, event);

	/* Restore tear offs - except for Popups 'cause the ungrabs are 
	 * yet to occur
	 */
	if (RC_Type(rowcol) != XmMENU_POPUP)
	   _XmRestoreTearOffToToplevelShell((Widget)rowcol, event);
    }
}


/*
 * Popdown all the popup kids of this widget, do bottom to top popdown.
 */
static void 
#ifdef _NO_PROTO
PopdownKids( menushell, event )
        XmMenuShellWidget menushell ;
        XEvent *event ;
#else
PopdownKids(
        XmMenuShellWidget menushell,
        XEvent *event )
#endif /* _NO_PROTO */
{
   ShellWidget subShell;

   if ((subShell = (ShellWidget)
	RC_PopupPosted(menushell->composite.children[0])) != NULL)
   {
      Widget rowcol = subShell->composite.children[0];

      /* mark this row colum as popping down */
      RC_SetPoppingDown (rowcol, True);
    
      PopdownKids ((XmMenuShellWidget) subShell, event);
      Popdown ((XmMenuShellWidget) subShell, event);
   }
}


/*
 * This routine determines if there is an enter event pending which will
 * just pop back up the row column that is about to be popped down.
 */
static int 
#ifdef _NO_PROTO
SkipPopdown( cascade )
        XmCascadeButtonWidget cascade ;
#else
SkipPopdown(
        XmCascadeButtonWidget cascade )
#endif /* _NO_PROTO */
{
    XEvent  event;

    /*
     * check if an enter event is pending.  
     */
    if (XPending (XtDisplay (cascade)))
    {
        XPeekEvent (XtDisplay (cascade), &event);

        if (event.type == EnterNotify)
        {
            XEnterWindowEvent * enterevent = (XEnterWindowEvent *) &event;

            if (XtWindow (cascade) == enterevent->window)
                     return (TRUE);
        }
    }

    return (FALSE);
}


/* 
 * event handler for entering on a row column widget.  The widget must
 * be either a pulldown or popup menu.
 */
/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmEnterRowColumn( widget, closure, event, cont )
        Widget widget ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
_XmEnterRowColumn(
        Widget widget,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XmRowColumnWidget cascade;
    Widget cascadebtn;
    Time _time = __XmGetDefaultTime(widget, event);
    
    XmRowColumnWidget rowcol = (XmRowColumnWidget) widget;
    XmMenuShellWidget menushell = (XmMenuShellWidget) XtParent(rowcol);
    XEnterWindowEvent *enterevent = (XEnterWindowEvent *) event;

    /* 
     * Ignore popdown requests if traversal is on, or we're no longer
     * visible, or we don't have any submenus up.
     */
    if (!_XmGetInDragMode(widget)         ||
	(!menushell->shell.popped_up)     ||
	(! RC_PopupPosted(rowcol)))
	return;

    cascade = (XmRowColumnWidget)
	((CompositeWidget) RC_PopupPosted(rowcol))->composite.children[0];
    
    cascadebtn = RC_CascadeBtn (cascade);

    /*
     * Make sure that popdown should not be skipped.  It should be skipped
     * if the mouse has moved into the cascadebutton (widget or gadget)
     * that will just pop it back up again.
     */
    if (XmIsCascadeButtonGadget(cascadebtn) &&
        (cascadebtn == (Widget) _XmInputInGadget( (Widget) rowcol,
                                                enterevent->x, enterevent->y)))
    {
       return;
    }
#ifdef PRER5SERVER
     ** When the menu is posted, the pointer is grabbed.  The server generates
     ** an enterevent for the grabbing window.  For <= R4 servers, the
     ** subwindow field was filled out with the child window that the pointer
     ** was contained within.  This is no longer in R5.  So we must do the
     ** calculation ourselves.  The R5 deficiency has been reported to MIT.
     ** The code is ifdef in case a future decision is made to fix this
     ** compatibility defect in >= R5 servers.
    else if ((XmIsCascadeButton(cascadebtn)) &&
	     ((enterevent->subwindow == XtWindow(cascadebtn)) || 
	      (SkipPopdown ((XmCascadeButtonWidget) cascadebtn))))
    {
        return;
    }
#else
    else if (XmIsCascadeButton(cascadebtn))
    {
       if (SkipPopdown ((XmCascadeButtonWidget) cascadebtn))
	  return;
       else
       {
	  Position x, y;

	  XtTranslateCoords(cascadebtn, 0, 0, &x, &y);
	  if ( (enterevent->x_root >= x)  && 
	       (enterevent->x_root < (x + XtWidth(cascadebtn))) &&
	       (enterevent->y_root >= y)  && 
	       (enterevent->y_root < (y + XtHeight(cascadebtn))))
	  {
	     return;
	  }
       }
    }
#endif

    _XmMenuFocus(XtParent(rowcol), XmMENU_MIDDLE, _time);
    PopdownKids (menushell, event);
}


static void 
#ifdef _NO_PROTO
PopdownOne( widget, event, params, num_params )
        Widget widget ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PopdownOne(
        Widget widget,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmMenuShellWidget ms = (XmMenuShellWidget)widget;
    XmMenuShellWidget tms;
    XmRowColumnWidget rowcol;

    /* Find a menu shell, this may be a menushell, rowcol or button */
    /* OR (!) a top level shell! */
    while (ms && !XtIsShell(ms))
         ms = (XmMenuShellWidget)XtParent(ms);

    if (ms && !XmIsMenuShell(ms))
    {
       _XmDismissTearOff((Widget) ms, (XtPointer) event, (XtPointer) NULL);
       return;
    }

    if (ms == NULL)
        return;

    /* Find the toplevel MenuShell 
     */
    _XmGetActiveTopLevelMenu (ms->composite.children[0], (Widget *) &rowcol);
    tms = ms;
    if (RC_Type(rowcol) == XmMENU_BAR)
        tms = (XmMenuShellWidget) RC_PopupPosted(rowcol);
    else if ((RC_Type(rowcol) == XmMENU_POPUP) ||
	     ((RC_Type(rowcol) == XmMENU_PULLDOWN) &&
	      !XmIsMenuShell(XtParent( rowcol))))
    {
        tms = (XmMenuShellWidget) XtParent(rowcol);
	if (!XmIsMenuShell(tms))	/* torn! */
	{
	   /* if the next pane up is the torn popup, bring down entire menu */
	   if (rowcol == (XmRowColumnWidget)XtParent(
	     RC_CascadeBtn(ms->composite.children[0])))
	      ms = tms = (XmMenuShellWidget)RC_ParentShell(rowcol);
	}
    }

    if (ms == tms)
       (* (((XmMenuShellClassRec *)(ms->core.widget_class))->
		menu_shell_class.popdownDone))
		   (widget, event, params, num_params);
    else
    {
          (* (((XmMenuShellClassRec *)(ms->core.widget_class))->
		menu_shell_class.popdownEveryone))
			((Widget) ms, event, params, num_params);
    }

    if (event)
       _XmRecordEvent(event);
}
    

/*
 * Class function used to unpost all menupanes
 */

/*
 * popdown this all the popup children, and then me, ie. bottom to top
 *
 * called from ChangeManaged and PopdownDone
 */
static void 
#ifdef _NO_PROTO
PopdownEveryone( widget, event, params, num_params )
        Widget widget ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PopdownEveryone(
        Widget widget,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    Widget rowcol;
    XmMenuShellWidget shell = (XmMenuShellWidget) widget;

    /* Ignore this event, if already processed */
    if ( event && !_XmIsEventUnique(event))
       return;
    /* 
     * If only a portion of the hierarchy is being popped down, then
     * make sure the keyboard grab gets reset to the right window.
     */
    rowcol = shell->composite.children[0];

    /* Only reset the focus if the this is not a tear off or the cascaded
     * submenu's parent is not a tear off.
     */
    if ((RC_Type(rowcol) == XmMENU_PULLDOWN) &&
        XmIsMenuShell(XtParent(rowcol)) &&
        RC_CascadeBtn(rowcol) &&
        XmIsMenuShell(XtParent(XtParent(RC_CascadeBtn(rowcol)))) )
    {
       _XmMenuFocus(XtParent(XtParent(RC_CascadeBtn(rowcol))),
		    XmMENU_MIDDLE, __XmGetDefaultTime(widget, event));
    }

    /* mark this row colum as popping down */
    RC_SetPoppingDown (rowcol, True);

    if (shell->shell.popped_up) 
    {
       if (XmIsMenuShell(shell))
       {
	  _XmFastPopdown((XmMenuShellWidget) widget);
	  XFlush(XtDisplay(widget));
	  PopdownKids ((XmMenuShellWidget) widget, event); /* do our kids */
	  Popdown ((XmMenuShellWidget) widget ,event);     /* do us */
       }
       else	/* torn - don't popdown the popup! */
       {
	  PopdownKids ((XmMenuShellWidget) widget, event); /* do our kids */
       }
    }
}


/*
 * Class function used to unpost all menupanes.  
 * Note that to catch BSelect in a gadget and popdown correctly, widget param
 * must be the LEAF node menushell, rowcol, or button.
 */
static void 
#ifdef _NO_PROTO
PopdownDone( widget, event, params, num_params )
        Widget widget ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PopdownDone(
        Widget widget,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmMenuShellWidget ms = (XmMenuShellWidget)widget;
    XmRowColumnWidget  rowcol;
    XmMenuState mst = _XmGetMenuState((Widget)widget);
    Time _time = __XmGetDefaultTime(widget, event);

    /* Ignore this event, if already processed */
    if (event && !_XmIsEventUnique(event))
       return;

    /* Find a menu shell, this may be a menushell, rowcol or button */
    while (ms && !XmIsMenuShell(ms))
         ms = (XmMenuShellWidget)XtParent(ms);

    if (ms == NULL)
	return;

    _XmGetActiveTopLevelMenu (ms->composite.children[0], (Widget *) &rowcol);

    if (event && ((event->xbutton.type==ButtonPress) || 
	 (event->xbutton.type==ButtonRelease)))
    {
       if ( !(_XmMatchBtnEvent( event, XmIGNORE_EVENTTYPE,
                RC_PostButton(rowcol), RC_PostModifiers(rowcol)) ||
	      _XmMatchBSelectEvent(ms->composite.children[0], event )) )
       {
	  /* To support menu replay, keep the pointer in sync mode */
	  XAllowEvents(XtDisplay(rowcol), SyncPointer, _time);
          return;
       }
    }

    /* Take menus out of drag mode in either case of click into traversal or
     * unpost, unless the menu is a torn-off pane.
     */
    if ((RC_Type(rowcol) == XmMENU_BAR) ||
	(RC_Type(rowcol) == XmMENU_OPTION) ||
	XmIsMenuShell(XtParent(rowcol)))
    {
	_XmSetInDragMode((Widget) rowcol, False);
    }

    /* This is to enable Option and Popup Menus to post on BMenu/BSelect Click.
     * By this time we know that the button is valid.  We need to check
     * if the post event occured within a short time span.  Next we force 
     * the menu into traversal mode.  And, of course, we return immediately
     * without popping down the pane.
     */
     if (RC_popupMenuClick(rowcol) &&
	 event &&
	 ((event->type == ButtonPress) || (event->type == ButtonRelease)) &&
	 ((event->xbutton.time - mst->MS_LastManagedMenuTime) < 
	  XtGetMultiClickTime(XtDisplay(ms))))	/* or 150 ms? */
     {
	if (RC_Type(rowcol) == XmMENU_OPTION)
	{
	   if (!XmProcessTraversal(RC_MemWidget(rowcol), XmTRAVERSE_CURRENT))
	      XmProcessTraversal(RC_OptionSubMenu(rowcol), XmTRAVERSE_CURRENT);

	   /* To support menu replay, keep the pointer in sync mode */
	   XAllowEvents(XtDisplay(rowcol), SyncPointer, _time);
	   return;
	}
	else
	   if (!rowcol->manager.highlighted_widget)
	   {
              XmProcessTraversal((Widget) rowcol, XmTRAVERSE_CURRENT);

	      /* To support menu replay, keep the pointer in sync mode */
	      XAllowEvents(XtDisplay(rowcol), SyncPointer, _time);
	      return;
	   }
     }

    /* 
     * If in a menubar or popup, get the toplevel menushell 
     * CHECK OPTION MENU esp w/ cascading submenus, JAY!
     */
    if (RC_Type(rowcol) == XmMENU_BAR)
    {
	/* It's possible that we got here via an accelerator and the menu
	 * isn't popped up - so ms could be NULL!
	 */
	if ( (ms = (XmMenuShellWidget) RC_PopupPosted(rowcol)) == NULL )
	   return;
    } 
    else if ((RC_Type(rowcol) == XmMENU_POPUP) || RC_TornOff(rowcol))
	ms = (XmMenuShellWidget) XtParent(rowcol);

    _XmMenuFocus( (Widget) ms, XmMENU_END, _time);
    
    if (XmIsMenuShell(ms))
    {
       (* (((XmMenuShellClassRec *)(ms->
	  core.widget_class))->menu_shell_class.popdownEveryone))
	     ((Widget) ms, event, params, num_params);
    }
    else
    {
       (* (((XmMenuShellClassRec *)(RC_ParentShell(rowcol)->
	  core.widget_class))->menu_shell_class.popdownEveryone))
	     ((Widget) ms, event, params, num_params);
    }

    /* if its a menubar, clean it up and disarm it */
    if (RC_Type(rowcol) == XmMENU_BAR)
    {
        (*(((XmRowColumnClassRec *)XtClass(rowcol))->row_column_class.
            menuProcedures)) (XmMENU_BAR_CLEANUP, (Widget) rowcol);
    } 
    else if ((RC_Type(rowcol) == XmMENU_POPUP) && RC_TornOff(rowcol))
	_XmRestoreTearOffToToplevelShell((Widget)rowcol, event);

    (*(((XmRowColumnClassRec *)XtClass(rowcol))->row_column_class.
       menuProcedures)) (XmMENU_DISARM, (Widget) rowcol); 

    /* Cleanup grabs */
    XtUngrabPointer((Widget) ms, _time);
}

static void
#ifdef _NO_PROTO
ClearTraversalInternal(menushell, event)
        XmMenuShellWidget menushell;
        XEvent *event;
#else
ClearTraversalInternal(
        XmMenuShellWidget menushell,
        XEvent *event)
#endif /* _NO_PROTO */
{
   Widget activeChild;
   XmRowColumnWidget rowcol;

   rowcol = (XmRowColumnWidget)menushell->composite.children[0];

   /* Clean up traversal */
   activeChild = rowcol->manager.active_child;
   if (activeChild)
   {
      if (XmIsGadget(activeChild))
      {
	 (*((XmLabelGadgetClass)XtClass(activeChild))->
	    gadget_class.border_unhighlight) (activeChild);
      } 
      else
      {
	 (*((XmLabelWidgetClass)XtClass(activeChild))->
	    primitive_class.border_unhighlight) (activeChild);
      }
   }

   /*
    * (Internal state update) Clear focus from unposting submenu.
    */
   _XmClearFocusPath((Widget) rowcol);

   if ((RC_Type(rowcol) != XmMENU_POPUP) && RC_CascadeBtn(rowcol))
   {
/*
We can't discern whether this is in traversal mode (or in traversal mode going
into drag mode) so don't do the unhighlighting here.  Push it up a level to
PushB and ToggleB and ???
*/
      /* This unhighlight is a dragmode only requirement.  */
      if (_XmGetInDragMode((Widget)rowcol))
         XmCascadeButtonHighlight (RC_CascadeBtn(rowcol), FALSE);
      RC_PopupPosted (XtParent(RC_CascadeBtn(rowcol))) = FALSE;
   }
}

/*
 * Clear traversal in the associated menu hierarchy
 */
void 
#ifdef _NO_PROTO
_XmClearTraversal( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmClearTraversal(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmMenuShellWidget ms = (XmMenuShellWidget) wid ;
    XmRowColumnWidget rowcol;
    XmMenuState mst = _XmGetMenuState((Widget)wid);
   
    if (!_XmIsEventUnique(event))
	return;

    /* if a valid event */
    if ((ms->composite.num_children != 0) &&
	(rowcol = (XmRowColumnWidget)ms->composite.children[0]) &&
	(_XmMatchBtnEvent( event, RC_PostEventType(rowcol),
			  RC_PostButton(rowcol),
			  RC_PostModifiers(rowcol)) ||
	(_XmMatchBSelectEvent((Widget)rowcol, event ))))
    {
	XButtonPressedEvent *xbutton = (XButtonPressedEvent *) event;

	/*
	 * if this is the Popup's posting event, ignore it.  It is just
	 * being sent to the spring loaded widget by Xt's dispatch
	 * after the post.  This may not have been recorded.
	 *
	 * This is also reached through an Mwm edge case. Be sure to 
	 * thaw the queue or pointer deadlock occurs.
	 */
	if ((RC_Type(rowcol) == XmMENU_POPUP) &&
	     (mst->MS_LastManagedMenuTime == xbutton->time))
	{
	    XAllowEvents(XtDisplay(ms), SyncPointer, 
			 __XmGetDefaultTime(wid, event));
	    return;
	}

	_XmHandleMenuButtonPress (ms->composite.children[0], event);
    }
    else
       XAllowEvents(XtDisplay(ms), SyncPointer, 
		    __XmGetDefaultTime(wid, event));
}

void
#ifdef _NO_PROTO
_XmSetLastManagedMenuTime (wid, newTime)
	Widget wid;
	Time newTime;
#else
_XmSetLastManagedMenuTime (
	Widget wid,
	Time newTime )
#endif /* _NO_PROTO */
{
   _XmGetMenuState((Widget)wid)->MS_LastManagedMenuTime = newTime;
}

/*
 * Public Routine
 */
Widget 
#ifdef _NO_PROTO
XmCreateMenuShell( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateMenuShell(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
   return (XtCreatePopupShell (name, xmMenuShellWidgetClass, parent, al, ac));
}

static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmMenuShellWidget ms = (XmMenuShellWidget) wid ;
    _XmDestroyFocusData(ms->menu_shell.focus_data);
    if (ms->menu_shell.default_font_list != NULL) 
       XmFontListFree (ms->menu_shell.default_font_list);

    if (ms->menu_shell.button_font_list != NULL) 
       XmFontListFree (ms->menu_shell.button_font_list);

    if (ms->menu_shell.label_font_list != NULL) 
       XmFontListFree (ms->menu_shell.label_font_list);    
}

static Widget
#ifdef _NO_PROTO
_XmFindPopup(widget, name)
    Widget widget;
    String name;
#else
_XmFindPopup(
    Widget widget,
    String name)
#endif /* _NO_PROTO */
{
    register Cardinal i;
    register XrmQuark q;
    register Widget w;

    q = XrmStringToQuark(name);

    for (w=widget; w != NULL; w=w->core.parent)
	for (i=0; i<w->core.num_popups; i++)
	    if (w->core.popup_list[i]->core.xrm_name == q)
		return w->core.popup_list[i];

    return NULL;
}

static void
#ifdef _NO_PROTO
_XmMenuPopupAction(widget, event, params, num_params)
    Widget widget;
    XEvent *event;
    String *params;
    Cardinal *num_params;
#else
_XmMenuPopupAction(
    Widget widget,
    XEvent *event,
    String *params,
    Cardinal *num_params)
#endif /* _NO_PROTO */
{
    Boolean spring_loaded;
    register Widget popup_shell;

    if (*num_params != 1) {
	XtAppWarningMsg(XtWidgetToApplicationContext(widget),
		      "invalidParameters","xtMenuPopupAction",tk_error,
			"MenuPopup wants exactly one argument",
			(String *)NULL, (Cardinal *)NULL);
	return;
    }

    if (event->type == ButtonPress)
	spring_loaded = True;
    else if (event->type == KeyPress || event->type == EnterNotify)
	spring_loaded = False;
    else {
	XtAppWarningMsg(XtWidgetToApplicationContext(widget),
		"invalidPopup","unsupportedOperation",tk_error,
		"Pop-up menu creation is only supported on ButtonPress, KeyPress or EnterNotify events.",
                (String *)NULL, (Cardinal *)NULL);
	spring_loaded = False;
      }

    popup_shell = _XmFindPopup(widget, params[0]);
    if (popup_shell == NULL) {
	XtAppWarningMsg(XtWidgetToApplicationContext(widget),
			"invalidPopup","xtMenuPopup",tk_error,
			"Can't find popup widget \"%s\" in XtMenuPopup",
			params, num_params);
	return;
    }

    if (spring_loaded) _XmPopupI(popup_shell, XtGrabExclusive, TRUE);
    else _XmPopupI(popup_shell, XtGrabNonexclusive, FALSE);
}

static void
#ifdef _NO_PROTO
_XmMenuPopdownAction(widget, event, params, num_params)
     Widget widget;
     XEvent *event;
     String *params;
     Cardinal *num_params;
#else
_XmMenuPopdownAction(
    Widget widget,
    XEvent *event,
    String *params,
    Cardinal *num_params)
#endif
{
    Widget popup_shell;

    if (*num_params == 0) {
	_XmPopdown(widget);
    } else if (*num_params == 1) {
	popup_shell = _XmFindPopup(widget, params[0]);
	if (popup_shell == NULL) {
            XtAppWarningMsg(XtWidgetToApplicationContext(widget),
			    "invalidPopup","xtMenuPopdown",tk_error,
			    "Can't find popup widget \"%s\" in XtMenuPopdown",
			    params, num_params);
	    return;
	}
	_XmPopdown(popup_shell);
    } else {
	XtAppWarningMsg(XtWidgetToApplicationContext(widget),
			"invalidParameters","xtMenuPopdown", tk_error,
			"XtMenuPopdown called with num_params != 0 or 1",
			(String *)NULL, (Cardinal *)NULL);
    }
}

