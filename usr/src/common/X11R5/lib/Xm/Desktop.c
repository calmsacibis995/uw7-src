#pragma ident	"@(#)m1.2libs:Xm/Desktop.c	1.3"
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
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
/* Make sure all wm properties can make it out of the resource manager */

#include <Xm/DesktopP.h>
#include <Xm/BaseClassP.h>
#include <Xm/ScreenP.h>
#include <Xm/DisplayP.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void DesktopClassPartInitialize() ;
static void ResParentDestroyed() ;
static void DesktopDestroy() ;
static void DesktopInsertChild() ;
static void DesktopDeleteChild() ;
static void DesktopInitialize() ;
static void DisplayDestroyCallback ();

#else

static void DesktopClassPartInitialize( 
                        WidgetClass widgetClass) ;
static void ResParentDestroyed( 
                        Widget resParent,
                        XtPointer closure,
                        XtPointer callData) ;
static void DesktopDestroy( 
                        Widget wid) ;
static void DesktopInsertChild( 
                        Widget wid) ;
static void DesktopDeleteChild( 
                        Widget wid) ;
static void DesktopInitialize( 
                        Widget requested_widget,
                        Widget new_widget,
                        ArgList args,
                        Cardinal *num_args) ;
static void DisplayDestroyCallback ( 
			Widget w, 
			XtPointer client_data, 
			XtPointer call_data );

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static XContext	actualClassContext = (XContext) NULL;


static XtResource desktopResources[] =
{
#ifdef notdef
/* this should be ok, but we're working around the XtNpersistent bug */
    {
	XmNlogicalParent,
	XmCLogicalParent, XmRWidget, sizeof (Widget),
	XtOffsetOf( struct _XmDesktopRec, ext.logicalParent),
	XmRImmediate, (XtPointer)NULL,
    },
#endif /* notdef */
    {
	XmNdesktopParent,
	XmCDesktopParent, XmRWidget, sizeof (Widget),
	XtOffsetOf( struct _XmDesktopRec, desktop.parent),
	XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNextensionType,
	XmCExtensionType, XmRExtensionType, sizeof (unsigned char),
	XtOffsetOf( struct _XmDesktopRec, ext.extensionType),
	XmRImmediate, (XtPointer)XmDESKTOP_EXTENSION,
    },
};


externaldef(xmdesktopclassrec)
XmDesktopClassRec xmDesktopClassRec = {
    {	
	(WidgetClass) &xmExtClassRec,	/* superclass	*/   
	"Desktop",			/* class_name 		*/   
	sizeof(XmDesktopRec), 		/* size 		*/   
	NULL,				/* Class Initializer 	*/   
	DesktopClassPartInitialize,	/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	DesktopInitialize,		/* initialize         	*/   
	NULL, 				/* initialize_notify    */ 
	NULL,	 			/* realize            	*/   
	NULL,	 			/* actions            	*/   
	0,				/* num_actions        	*/   
	desktopResources,		/* resources          	*/   
	XtNumber(desktopResources),	/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	DesktopDestroy,			/* destroy            	*/   
	NULL,		 		/* resize             	*/   
	NULL, 				/* expose             	*/   
	NULL,		 		/* set_values         	*/   
	NULL, 				/* set_values_hook      */ 
	NULL,			 	/* set_values_almost    */ 
	NULL,				/* get_values_hook      */ 
	NULL, 				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL, 				/* display_accelerator  */ 
	NULL, 				/* extension            */ 
    },	
    {					/* ext			*/
	NULL,				/* synthetic resources	*/
	0,				/* num syn resources	*/
	NULL,				/* extension		*/
    },
    {					/* desktop		*/
	NULL,				/* child_class		*/
	DesktopInsertChild,		/* insert_child		*/
	DesktopDeleteChild,		/* delete_child		*/
	NULL,				/* extension		*/
    },
};

externaldef(xmdesktopclass) WidgetClass 
      xmDesktopClass = (WidgetClass) &xmDesktopClassRec;
    

    
static void 
#ifdef _NO_PROTO
DesktopClassPartInitialize( widgetClass )
        WidgetClass widgetClass ;
#else
DesktopClassPartInitialize(
        WidgetClass widgetClass )
#endif /* _NO_PROTO */
{
    register XmDesktopClassPartPtr wcPtr;
    register XmDesktopClassPartPtr superPtr;
    
    wcPtr = (XmDesktopClassPartPtr)
      &(((XmDesktopObjectClass)widgetClass)->desktop_class);
    
    if (widgetClass != xmDesktopClass)
      /* don't compute possible bogus pointer */
      superPtr = (XmDesktopClassPartPtr)&(((XmDesktopObjectClass)widgetClass
					   ->core_class.superclass)->desktop_class);
    else
      superPtr = NULL;
    
    /* We don't need to check for null super since we'll get to xmDesktop
       eventually, and it had better define them!  */
    
#ifdef notdef
    if (wcPtr->create_child == XmInheritCreateChild) {
	wcPtr->create_child =
	  superPtr->create_child;
    }
#endif
    if (wcPtr->child_class == XmInheritClass) {
	wcPtr->child_class = 
	  superPtr->child_class;
    }
    if (wcPtr->insert_child == XtInheritInsertChild) {
	wcPtr->insert_child = superPtr->insert_child;
    }
    
    if (wcPtr->delete_child == XtInheritDeleteChild) {
	wcPtr->delete_child = superPtr->delete_child;
    }
    
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ResParentDestroyed( resParent, closure, callData )
        Widget resParent ;
        XtPointer closure ;
        XtPointer callData ;
#else
ResParentDestroyed(
        Widget resParent,
        XtPointer closure,
        XtPointer callData )
#endif /* _NO_PROTO */
{
    XmExtObject	me = (XmExtObject) closure ;
    if (!me->object.being_destroyed) {
      XtDestroyWidget((Widget) me);
    }
}

static void 
#ifdef _NO_PROTO
DesktopDestroy( wid )
    Widget wid ;
#else
DesktopDestroy(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmDesktopObject w = (XmDesktopObject) wid ;
    XmDesktopObject		deskObj = (XmDesktopObject)w;
    Widget			deskParent;
    Widget			resParent = w->ext.logicalParent;
    
    if ((deskParent = deskObj->desktop.parent) != NULL)
      {
	  if (XmIsScreen(deskParent)) {
	      XmScreenClass	deskParentClass;
	      deskParentClass = (XmScreenClass)
		XtClass(deskParent);
	      (*(deskParentClass->desktop_class.delete_child)) 
		((Widget) deskObj);
	  }
	  else {
	      XmDesktopObjectClass	deskParentClass;
	      deskParentClass = (XmDesktopObjectClass)
		XtClass(deskParent);
	      (*(deskParentClass->desktop_class.delete_child)) 
		((Widget) deskObj);
	  }
      }
    
    /*
     * if we were created as a sibling of our primary then we have a
     * destroy callback on them.
     */
    if (resParent && 
	!resParent->core.being_destroyed)
      XtRemoveCallback((Widget) resParent, 
		       XmNdestroyCallback,
		       ResParentDestroyed,
		       (XtPointer)w);
    XtFree((char *) w->desktop.children);
}

static void 
#ifdef _NO_PROTO
DesktopInsertChild( wid )
        Widget wid ;
#else
DesktopInsertChild(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmDesktopObject w = (XmDesktopObject) wid ;
    register Cardinal	     	position;
    register Cardinal        	i;
    register XmDesktopObject 	cw;
    register WidgetList      	children;
    
    cw = (XmDesktopObject) w->desktop.parent;
    children = cw->desktop.children;
    
    position = cw->desktop.num_children;
    
    if (cw->desktop.num_children == cw->desktop.num_slots) {
	/* Allocate more space */
	cw->desktop.num_slots +=  (cw->desktop.num_slots / 2) + 2;
	cw->desktop.children = children = 
	  (WidgetList) XtRealloc((char *) children,
				 (unsigned) (cw->desktop.num_slots) * sizeof(Widget));
    }
    /* Ripple children up one space from "position" */
    for (i = cw->desktop.num_children; i > position; i--) {
	children[i] = children[i-1];
    }
    children[position] = (Widget)w;
    cw->desktop.num_children++;
}

static void 
#ifdef _NO_PROTO
DesktopDeleteChild( wid )
        Widget wid ;
#else
DesktopDeleteChild(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmDesktopObject w = (XmDesktopObject) wid ;
    register Cardinal	     	position;
    register Cardinal	     	i;
    register XmDesktopObject 	cw;
    
    cw = (XmDesktopObject) w->desktop.parent;
    
    for (position = 0; position < cw->desktop.num_children; position++) {
        if (cw->desktop.children[position] == (Widget)w) {
	    break;
	}
    }
    if (position == cw->desktop.num_children) return;
    
    /* Ripple children down one space from "position" */
    cw->desktop.num_children--;
    for (i = position; i < cw->desktop.num_children; i++) {
        cw->desktop.children[i] = cw->desktop.children[i+1];
    }
}

/************************************************************************
 *
 *  DesktopInitialize
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DesktopInitialize( requested_widget, new_widget, args, num_args )
        Widget requested_widget ;
        Widget new_widget ;
        ArgList args ;
        Cardinal *num_args ;
#else
DesktopInitialize(
        Widget requested_widget,
        Widget new_widget,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmDesktopObject		deskObj = (XmDesktopObject)new_widget;
    Widget			deskParent;

    deskObj->desktop.num_children = 0;
    deskObj->desktop.children = NULL;
    deskObj->desktop.num_slots = 0;
    
    if ((deskParent = deskObj->desktop.parent) != NULL)
      {
	  if (XmIsScreen(deskParent)) {
	      XmScreenClass	deskParentClass;
	      deskParentClass = (XmScreenClass)
		XtClass(deskParent);
	      (*(deskParentClass->desktop_class.insert_child)) 
		((Widget) deskObj);
	  }
	  else {
	      XmDesktopObjectClass	deskParentClass;
	      deskParentClass = (XmDesktopObjectClass)
		XtClass(deskParent);
	      (*(deskParentClass->desktop_class.insert_child)) 
		((Widget) deskObj);
	  }
      }
/*
    if (resParent)
      XtAddCallback(resParent, 
		    XmNdestroyCallback, 
		    ResParentDestroyed, 
		    (XtPointer)deskObj);
*/
}


/************************************************************************
 *
 *  _XmGetActualClass
 *
 ************************************************************************/
/* ARGSUSED */
WidgetClass 
#ifdef _NO_PROTO
_XmGetActualClass( display, w_class )
	Display     *display;
        WidgetClass w_class ;
#else
_XmGetActualClass(
	Display     *display,
        WidgetClass w_class )
#endif /* _NO_PROTO */
{
	  WidgetClass		actualClass;

	  if (actualClassContext == (XContext) NULL)
	    actualClassContext = XUniqueContext();
	  
	  /*
	   * see if a non-default class has been specified for the
	   * class
	   */
	  if (XFindContext(display,
			   (Window) w_class,
			   actualClassContext,
			   (char **) &actualClass))
	    {
		return w_class;
	    }
	  else
	    return actualClass;
}

/************************************************************************
 *
 *  _XmSetActualClass
 *
 ************************************************************************/
/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmSetActualClass( display, w_class, actualClass )
	Display     *display;
        WidgetClass w_class ;
        WidgetClass actualClass ;
#else
_XmSetActualClass(
	Display     *display,
        WidgetClass w_class,
        WidgetClass actualClass )
#endif /* _NO_PROTO */
{
    XmDisplay   dd = (XmDisplay) XmGetXmDisplay(display);
    WidgetClass previous;
    WidgetClass oldActualClass;

    if (actualClassContext == (XContext) NULL)
      actualClassContext = XUniqueContext();
    
    /*
     * see if a non-default class has been specified for the
     * class
     */
    previous = _XmGetActualClass(display, w_class);
    XtRemoveCallback((Widget)dd, XtNdestroyCallback,
			DisplayDestroyCallback, (XtPointer) previous);


    /*
     * Save class data.
     * Delete old context if one exists.
     */
    if (XFindContext (display, (Window) w_class, actualClassContext,
			(char **)&oldActualClass)) {
	XSaveContext(display, (Window) w_class, actualClassContext,
			(char *) actualClass);
    }
    else if (oldActualClass != actualClass) {
	XDeleteContext (display, (Window) w_class, actualClassContext);
	XSaveContext(display, (Window) w_class, actualClassContext,
			(char *) actualClass);

    }

    XtAddCallback((Widget)dd, XtNdestroyCallback, 
			DisplayDestroyCallback, (XtPointer) w_class);
}

static void 
DisplayDestroyCallback 
#ifdef _NO_PROTO
	( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
	( Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	XDeleteContext(XtDisplay(w), (XID) client_data, actualClassContext);
}
