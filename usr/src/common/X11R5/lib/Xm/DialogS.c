#pragma ident	"@(#)m1.2libs:Xm/DialogS.c	1.7"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.3
else
 * Motif Release 1.2.4
endif
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
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#include "XmI.h"
#include <Xm/DialogSP.h>
#include <Xm/DialogSEP.h>
#include <Xm/BaseClassP.h>
#include <Xm/BulletinBP.h>
#include "MessagesI.h"


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_DShell,MSG_DS_1,_XmMsgDialogS_0000)
#else
#define MSG1	_XmMsgDialogS_0000
#endif


#define MAGIC_VAL ((Position)~0L)

#define HALFDIFF(a, b) ((((Position)a) - ((Position)b))/2)

#define TotalWidth(w)   (XtWidth  (w) + (2 * (XtBorderWidth (w))))
#define TotalHeight(w)  (XtHeight (w) + (2 *(XtBorderWidth (w))))

#define CALLBACK(w,which,why,evnt)		\
{						\
 if (XmIsBulletinBoard(w))	\
   {						\
 XmAnyCallbackStruct temp;		\
   temp.reason = why;			\
     temp.event  = evnt;			\
       XtCallCallbacks (w, which, &temp);	\
     }						\
   }


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInit() ;
static Widget GetRectObjKid() ;
static void Initialize() ;
static Boolean SetValues() ;
static void InsertChild() ;
static void GetDefaultPosition() ;
static void ChangeManaged() ;
static XtGeometryResult GeometryManager() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInit( 
                        WidgetClass wc) ;
static Widget GetRectObjKid( 
                        CompositeWidget p) ;
static void Initialize( 
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValues( 
                        Widget current,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void InsertChild( 
                        Widget w) ;
static void GetDefaultPosition( 
                        XmBulletinBoardWidget child,
                        Widget parent,
                        Position *xRtn,
                        Position *yRtn) ;
static void ChangeManaged( 
                        Widget wid) ;
static XtGeometryResult GeometryManager( 
                        Widget wid,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


#ifdef FULL_EXT
static XmBaseClassExtRec	myBaseClassExtRec = {
    NULL,				/* Next extension	*/
    NULLQUARK,				/* record type XmQmotif	*/
    XmBaseClassExtVersion,		/* version		*/
    sizeof(XmBaseClassExtRec),		/* size			*/
    XmInheritInitializeSetup,		/* initialize setup	*/
    NULL,				/* initialize prehook	*/
    NULL,				/* initialize posthook	*/
    XmInheritInitializeCleanup,		/* initialize cleanup	*/
    XmInheritSetValuesSetup,		/* setValues setup	*/
    NULL,				/* setValues prehook	*/
    NULL,				/* setValues posthook	*/
    XmInheritSetValuesCleanup,		/* setValues cleanup	*/
    XmInheritGetValuesSetup,		/* getValues setup	*/
    NULL,				/* getValues prehook	*/
    NULL,				/* getValues posthook	*/
    XmInheritGetValuesCleanup,		/* getValues cleanup	*/
    (WidgetClass)&xmDialogShellExtClassRec,/* secondary class	*/
    XmInheritSecObjectCreate,		/* secondary create	*/
    {0},				/* fast subclass	*/
};
#else
static XmBaseClassExtRec	myBaseClassExtRec = {
    NULL,				/* Next extension	*/
    NULLQUARK,				/* record type XmQmotif	*/
    XmBaseClassExtVersion,		/* version		*/
    sizeof(XmBaseClassExtRec),		/* size			*/
    XmInheritInitializePrehook,		/* initialize prehook	*/
    XmInheritSetValuesPrehook,		/* set_values prehook	*/
    XmInheritInitializePosthook,	/* initialize posthook	*/
    XmInheritSetValuesPosthook,		/* set_values posthook	*/
    (WidgetClass)&xmDialogShellExtClassRec,/* secondary class	*/
    XmInheritSecObjectCreate,		/* secondary create	*/
    NULL,				/* getSecRes data	*/
    {0}				/* fast subclass	*/
};
#endif


externaldef(xmdialogshellclassrec)
XmDialogShellClassRec xmDialogShellClassRec = {
    {					    /* core class record */
	
	(WidgetClass) & transientShellClassRec,	/* superclass */
	"XmDialogShell", 		/* class_name */
	sizeof(XmDialogShellWidgetRec), /* widget_size */
	ClassInitialize,		/* class_initialize proc */
	ClassPartInit,			/* class_part_initialize proc */
	FALSE, 				/* class_inited flag */
	Initialize, 			/* instance initialize proc */
	NULL, 				/* init_hook proc */
	XtInheritRealize,		/* realize widget proc */
	NULL, 				/* action table for class */
	0, 				/* num_actions */
	NULL,	 			/* resource list of class */
	0,		 		/* num_resources in list */
	NULLQUARK, 			/* xrm_class ? */
	FALSE, 				/* don't compress_motion */
	TRUE, 				/* do compress_exposure */
	FALSE, 				/* do compress enter-leave */
	FALSE, 				/* do have visible_interest */
	NULL, 				/* destroy widget proc */
	XtInheritResize, 		/* resize widget proc */
	NULL, 				/* expose proc */
	SetValues, 			/* set_values proc */
	NULL, 				/* set_values_hook proc */
	XtInheritSetValuesAlmost, 	/* set_values_almost proc */
	NULL, 				/* get_values_hook */
	NULL, 				/* accept_focus proc */
	XtVersion, 			/* current version */
	NULL, 				/* callback offset    */
	XtInheritTranslations, 		/* default translation table */
	XtInheritQueryGeometry, 	/* query geometry widget proc */
	NULL, 				/* display accelerator    */
	(XtPointer)&myBaseClassExtRec,	/* extension record      */
    },
    { 					/* composite class record */
	GeometryManager,                /* geometry_manager */
	ChangeManaged, 			/* change_managed		*/
	InsertChild,			/* insert_child			*/
	XtInheritDeleteChild, 		/* from the shell */
	NULL, 				/* extension record      */
    },
    { 					/* shell class record */
	NULL, 				/* extension record      */
    },
    { 					/* wm shell class record */
	NULL, 				/* extension record      */
    },
    { 					/* vendor shell class record */
	NULL,				/* extension record      */
    },
    { 					/* transient class record */
	NULL, 				/* extension record      */
    },
    { 					/* our class record */
	NULL, 				/* extension record      */
    },
};


/*
 * now make a public symbol that points to this class record
 */

externaldef(xmdialogshellwidgetclass)
    WidgetClass xmDialogShellWidgetClass = (WidgetClass)&xmDialogShellClassRec;
    

static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
  Cardinal                    wc_num_res, sc_num_res, wc_unique_res;
  XtResource                  *merged_list;
  int                         i, j, k;
  XtResourceList              uncompiled, res_list;
  Cardinal                    num;

/**************************************************************************
   VendorExt and  DialogExt resource lists are being merged into one
   and assigned to xmDialogShellExtClassRec. This is for performance
   reasons, since, instead of two calls to XtGetSubResources() XtGetSubvaluse()
   and XtSetSubvalues() for both the superclass and the widget class, now
   we have just one call with a merged resource list.

****************************************************************************/

  wc_num_res = xmDialogShellExtClassRec.object_class.num_resources ;

  wc_unique_res = wc_num_res - 1; /* XmNdeleteResponse has been defined */
                                  /* in VendorSE  */

  sc_num_res = xmVendorShellExtClassRec.object_class.num_resources;

  merged_list = (XtResource *)XtMalloc((sizeof(XtResource) * (wc_unique_res +
                                                                 sc_num_res)));

  _XmTransformSubResources(xmVendorShellExtClassRec.object_class.resources,
                           sc_num_res, &uncompiled, &num);

  for (i = 0; i < num; i++)
  {

  merged_list[i] = uncompiled[i];

  }

  XtFree((char *)uncompiled);

  res_list = xmDialogShellExtClassRec.object_class.resources;

  for (i = 0, j = num; i < wc_num_res; i++)
  {

   for (k = 0; 
        ((k < sc_num_res) &&  (strcmp(merged_list[k].resource_name,
                              res_list[i].resource_name) != 0)); k++)
   {
    ;
   }
   if ( (k < sc_num_res) && (strcmp(merged_list[k].resource_name, res_list[i].resource_name) == 0))
     merged_list[k] = res_list[i];
   else
   {
     merged_list[j] =
        xmDialogShellExtClassRec.object_class.resources[i];
     j++;
   }
  }

  xmDialogShellExtClassRec.object_class.resources = merged_list;
  xmDialogShellExtClassRec.object_class.num_resources =
                wc_unique_res + sc_num_res ;

  xmDialogShellExtObjectClass->core_class.class_initialize();

  myBaseClassExtRec.record_type = XmQmotif;
}

/************************************************************************
 *
 *  ClassPartInit
 *    Set up the fast subclassing for the widget.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInit( wc )
        WidgetClass wc ;
#else
ClassPartInit(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
   _XmFastSubclassInit(wc, XmDIALOG_SHELL_BIT);
}

static Widget 
#ifdef _NO_PROTO
GetRectObjKid( p )
        CompositeWidget p ;
#else
GetRectObjKid(
        CompositeWidget p )
#endif /* _NO_PROTO */
{
    Cardinal	i;
    Widget	*currKid;
    
    for (i = 0, currKid = p->composite.children;
	 i < p->composite.num_children;
	 i++, currKid++)
      {
	  if(    XtIsRectObj( *currKid)
              /* The Input Method child is a CoreClass object; ignore it. */
              && ((*currKid)->core.widget_class != coreWidgetClass)    )
          {   
              return (*currKid);
              } 
      }
    return NULL;
}


/************************************************************************
 *
 *  Initialize
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Initialize( request, new_w, args, num_args )
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    if (XtWidth  (new_w) <= 0)  XtWidth  (new_w) = 5;
    if (XtHeight (new_w) <= 0)  XtHeight (new_w) = 5;
}




/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValues( current, request, new_w, args, num_args )
        Widget current ;
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget current,
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
        Widget child ;

    if(    !current->core.mapped_when_managed
        && new_w->core.mapped_when_managed    )
    {   
        if(    (child = GetRectObjKid( (CompositeWidget) new_w))
            && !child->core.being_destroyed    )
        {   
            CALLBACK( (Widget) child, XmNmapCallback, XmCR_MAP, NULL) ;
            XtPopup( new_w, XtGrabNone) ;
            } 
        } 
    return (FALSE);
    }

static void 
#ifdef _NO_PROTO
InsertChild( w )
        Widget w ;
#else
InsertChild(
        Widget w )
#endif /* _NO_PROTO */
{
    CompositeWidget p = (CompositeWidget) XtParent (w);
   

    /*
     * Make sure we only have a rectObj, a VendorObject, and
     *   maybe an Input Method (CoreClass) object as children.
     */
    if (!XtIsRectObj(w))
      return;
    else
	{
	    if(    (w->core.widget_class != coreWidgetClass)
                /* The Input Method child is a CoreClass object. */
                && GetRectObjKid( p)    )
	      {
		/* we need _XmError() too! */
		  XtError(MSG1);
	      }
	    else
	      {   /*
		   * make sure we're realized so people won't core dump when 
		   *   doing incorrect managing prior to realize
		   */
		  XtRealizeWidget((Widget) p);
	      }
	}
    (*((CompositeWidgetClass) compositeWidgetClass)
                                          ->composite_class.insert_child)( w) ;
    return ;
}

static void 
#ifdef _NO_PROTO
GetDefaultPosition( child, parent, xRtn, yRtn )
        XmBulletinBoardWidget child ;
        Widget parent ;
        Position *xRtn ;
        Position *yRtn ;
#else
GetDefaultPosition(
        XmBulletinBoardWidget child,
        Widget parent,
        Position *xRtn,
        Position *yRtn )
#endif /* _NO_PROTO */
{
    Display 	*disp;
    int 	max_w, max_h;
    Position 	x, y;

    x = HALFDIFF(XtWidth(parent), XtWidth(child));
    y = HALFDIFF(XtHeight(parent), XtHeight(child));
    
    /* 
     * find root co-ords of the parent's center
     */
    if (XtIsRealized (parent))
      XtTranslateCoords(parent, x, y, &x, &y);
    
    /*
     * try to keep the popup from dribbling off the display
     */
    disp = XtDisplay (child);
    max_w = DisplayWidth  (disp, DefaultScreen (disp));
    max_h = DisplayHeight (disp, DefaultScreen (disp));
    
    if ((x + (int)TotalWidth  (child)) > max_w) 
      x = max_w - TotalWidth  (child);
    if ((y + (int)TotalHeight (child)) > max_h) 
      y = max_h - TotalHeight (child);
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    *xRtn = x;
    *yRtn = y;
}
#undef HALFDIFF







/*
 * border width and size and location are ty...
 *
 * 1. We allow the border width of a XmDialogShell child to change
 *    size arbitrarily.
 *
 * 2. The border width of the shell widget tracks the child's
 *    at all times, exactly.
 *
 * 3. The width of the shell is kept exactly the same as the
 *    width of the child at all times.
 *
 * 4. The child is always positioned at the location
 *    (- child_border, - child_border).
 *
 * the net result is the child has a border width which is always
 * what the user asked for;  but none of it is ever seen, it's all
 * clipped by the shell (parent).  The user sees the border
 * of the shell which is the size he set the child's border to.
 *
 * In the DEC window manager world the window manager does
 * exactly the same thing with the window it puts around the shell.
 * Hence the shell and child have a border width just as the user
 * set but the window manager overrides that and only a single
 * pixel border is displayed.  In a non-wm environment the child 
 * appears to have a border width, in reality this is the shell
 * widget border.  You wanted to know...
 */
static void 
#ifdef _NO_PROTO
ChangeManaged( wid )
        Widget wid ;
#else
ChangeManaged(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmDialogShellWidget shell = (XmDialogShellWidget) wid ;
    /*
     *  If the child went to unmanaged, call XtPopdown.
     *  If the child went to managed, call XtPopup.
     */
    
    XmBulletinBoardWidget	 child;
    XmWidgetExtData		extData = _XmGetWidgetExtData((Widget) shell, XmSHELL_EXTENSION);
    XmVendorShellExtObject	ve = (XmVendorShellExtObject)extData->widget;
    Boolean			childIsBB;

    if (((child = (XmBulletinBoardWidget) GetRectObjKid((CompositeWidget) shell)) == NULL) ||
	(child->core.being_destroyed))
      return;
    
    childIsBB = XmIsBulletinBoard(child);
    
    if (child->core.managed) 
      {
	  XtWidgetGeometry	request;
	  Position		kidX, kidY;
	  Dimension		kidBW;
	  Boolean		defaultPosition = True;

	  /*
	   * temporary workaround for setkeyboard focus |||
	   */
	  if (((Widget)child != ve->vendor.old_managed)
#ifdef notdef
	      &&(_XmGetFocusPolicy(child) == XmEXPLICIT)
#endif /* notdef */
	      )
	    {
		XtSetKeyboardFocus((Widget)shell, (Widget)child);
		ve->vendor.old_managed = (Widget)child;
	    }

	  /* 
	   * if the child isn't realized, then we need to realize it
	   * so we have a valid size. It will get created as a result
	   * so we  zero out it's position info so it'll
	   * be okay and then restore it.
	   */
	  if (!XtIsRealized(child))
	    {
		kidX = XtX(child);
		kidY = XtY(child);
		kidBW = XtBorderWidth(child);
		
		XtX(child) = 0;
		XtY(child) = 0;
		XtBorderWidth(child) = 0;
		
		if (XtHeight(shell) != XtHeight(child))
                   _XmImChangeManaged(shell);

		XtRealizeWidget((Widget) child);
		
		XtX(child) = kidX;
		XtY(child) = kidY;
		XtBorderWidth(child) = kidBW;
	    }
	  
	  else if (childIsBB)
	    {
		/*  
		 *  Move the window to 0,0
		 *  but don't tell the widget.  It thinks it's where
		 *  the shell is...
		 */
		if ((XtX(child) != 0) || (XtY(child) != 0))
		  XMoveWindow (XtDisplay(child), 
			       XtWindow(child), 
			       0, 0);
	    }
	  /*
	   * TRY TO FIX 1.0 BUG ALERT!
	   *
	   * map callback should occur BEFORE bulletinBoard class default positioning
	   * otherwise, widgets such as fileselection using map callback for
	   * correct sizing have default positioning done before the widget 
	   * grows to its correct dimensions
	   */
          if(    shell->core.mapped_when_managed    )
	  {   CALLBACK ((Widget) child, XmNmapCallback, XmCR_MAP, NULL);	
              } 
	  /* 
	   * Make sure that the shell has the same common parameters as 
	   * its child.  Then move the child so that the shell will 
	   * correctly surround it.
	   */
	  request.request_mode = 0;
	  
	  if (childIsBB)
	    {
		defaultPosition =
		  child->bulletin_board.default_position;
		if (defaultPosition && (ve->vendor.externalReposition))
		  defaultPosition = 
		    child->bulletin_board.default_position = 
		      False;
	    }
	  if (XtX(child) && childIsBB)
	    {
		kidX = XtX(child);
		XtX(child) = 0;
	    }
	  else
	    kidX = XtX(shell);
	  
	  if (XtY(child) && childIsBB)
	    {
		kidY = XtY(child);
		XtY(child) = 0;
	    }
	  else
	    kidY = XtY(shell);
	  if (XtBorderWidth(child) && childIsBB)
	    {
		kidBW = XtBorderWidth(child);
		XtBorderWidth(child) = 0;
	    }
	  else
	    kidBW = XtBorderWidth(shell);
	  
	  if (XtWidth (child) != XtWidth (shell))
	    {
		request.request_mode |= CWWidth;
		request.width = XtWidth(child);
	    }
   	  if (XtHeight (child) + ve->vendor.im_height != XtHeight (shell))
    	    {
		request.request_mode |= CWHeight;
		request.height = XtHeight(child) + ve->vendor.im_height;
	    }
	  
	  if (childIsBB)
	    {
		if (defaultPosition)
		  {
		      GetDefaultPosition(child,
					 XtParent(shell),
					 &request.x,
					 &request.y);
		      if (request.x != kidX)
			request.request_mode |= CWX;
		      if (request.y != kidY)
			request.request_mode |= CWY;
		  }
		else
		  {
		      if (kidX != XtX(shell))
			{
			    request.request_mode |= CWX;
			    if (kidX == MAGIC_VAL)
			      request.x = 0;
			    else
			      request.x = kidX;
			}
		      if (kidY != XtY(shell))
			{
			    request.request_mode |= CWY;
			    if (kidY == MAGIC_VAL)
			      request.y = 0;
			    else
			      request.y = kidY;
			}
		  }
	    }
	  else
	    {
		if (kidX != XtX(shell))
		  {
		      request.request_mode |= CWX;
		      request.x = kidX;
		  }
		if (kidY != XtY(shell))
		  {
		      request.request_mode |= CWY;
		      request.y = kidY;
		  }
		if (kidBW != XtBorderWidth(shell))
		  {
		      request.request_mode |= CWBorderWidth;
		      request.border_width = kidBW;
		  }
	    }
	  if (request.request_mode)
	  {
	    unsigned int old_height = ve->vendor.im_height;
	    XtMakeGeometryRequest((Widget) shell, &request, &request);
	    _XmImResize((Widget)shell);
 	    if (ve->vendor.im_height != old_height)
     	    {
 		request.request_mode = CWHeight;
 		request.height = XtHeight(child) + ve->vendor.im_height;
 		XtMakeGeometryRequest((Widget) shell, &request, &request);
 		_XmImResize((Widget)shell);
 	    }
	  }

#ifdef notdef
	  /*
	   * Set the mapStyle to manage so that if we are externally
	   * unmapped by the wm we will be able to recover on reciept
	   * of the unmap notify and unmanage ourselves
	   */
	  ve->vendor.mapStyle = _XmMANAGE_MAP;
#endif /* notdef */

	  /*
	   * the grab_kind is handled in the popup_callback
	   */
          if(    shell->core.mapped_when_managed    )
	  {   XtPopup  ((Widget) shell, XtGrabNone);
              } 
      }
    /*
     * CHILD BEING UNMANAGED
     */
    else
      {
              Position	x, y;

           XtTranslateCoords(( Widget)shell,
                         -((Position) shell->core.border_width),
                              -((Position) shell->core.border_width), &x, &y) ;
#ifdef notdef
	  if (XmIsBulletinBoard(child))
	    {
		XtX(child) = x;
		XtY(child) = y;
	    }
	  /*
	   * update normal_hints even though we shouldn't need to
	   */
	  SetWMOffset(shell);
#endif
 
   	  /*
  	   * Fix for CR5043 and CR5758 -
   	   * For nested Dialog Shells, it is necessary to unmanage
   	   * dialog shell popups of the child of this dialog shell.
   	   */
  	   {
  	     int i, j;
  	     for (i = 0; i < child->core.num_popups; i++)
   	     {
  	       if (XmIsDialogShell(child->core.popup_list[i]))
   		 {
   		   XmDialogShellWidget next_shell = 
  		     (XmDialogShellWidget)(child->core.popup_list[i]);
		   for (j=0; j< next_shell->composite.num_children; j++)
		   {
			Widget next_bb = next_shell->composite.children[j];
			if (next_bb && XtIsWidget(next_bb))
				XtUnmanageChild(next_bb);
		   }
   		 }
   	     }
  	  }
  	  /* End Fix CR5043 and CR5758*/
   
	  /*
	   * take it down and then tell user
	   */
	  
	  XtPopdown((Widget) shell);
	  
	  CALLBACK ((Widget) child, XmNunmapCallback, XmCR_UNMAP, NULL);	
      }
    _XmNavigChangeManaged((Widget) shell);
}                       


/************************************************************************
 *
 *  GeometryManager
 *
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
GeometryManager( wid, request, reply )
        Widget wid ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
GeometryManager(
        Widget wid,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
    ShellWidget 	shell = (ShellWidget)(wid->core.parent);
    XtWidgetGeometry 	my_request;
    XmVendorShellExtObject ve;
    XmWidgetExtData   extData;

    extData = _XmGetWidgetExtData((Widget)shell, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;

    if(!(shell->shell.allow_shell_resize) && XtIsRealized(wid) &&
       (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)))
      return(XtGeometryNo);
    /*
     * because of our klutzy API we mimic position requests on the
     * dialog to ourselves
     */
    my_request.request_mode = 0;

    /* %%% worry about XtCWQueryOnly */
    if (request->request_mode & XtCWQueryOnly)
      my_request.request_mode |= XtCWQueryOnly;

    if (request->request_mode & CWX) {
	if (request->x == MAGIC_VAL)
	  my_request.x = 0;
	else
	  my_request.x = request->x;
	my_request.request_mode |= CWX;
    }
    if (request->request_mode & CWY) {
	if (request->y == MAGIC_VAL)
	  my_request.y = 0;
	else
	  my_request.y = request->y;
	my_request.request_mode |= CWY;
    }
    if (request->request_mode & CWWidth) {
	my_request.width = request->width;
	my_request.request_mode |= CWWidth;
    }
    if (request->request_mode & CWHeight) {
	if (!ve->vendor.im_height)
	  _XmImResize((Widget)shell); /* updates im_height */
	my_request.height = request->height + ve->vendor.im_height;
	my_request.request_mode |= CWHeight;
    }
    if (request->request_mode & CWBorderWidth) {
	my_request.border_width = request->border_width;
	my_request.request_mode |= CWBorderWidth;
    }

    if (XtMakeGeometryRequest((Widget)shell, &my_request, NULL)
	== XtGeometryYes) {
          if (!(request->request_mode & XtCWQueryOnly)) {
	      /* just report the size changes to the kid, not
		 the dialog position itself */
	      if (my_request.request_mode & CWWidth)
		  wid->core.width = my_request.width ;
	      _XmImResize((Widget)shell);
	      if (my_request.request_mode & CWHeight)
		  wid->core.height = my_request.height - ve->vendor.im_height;
	  }
	  return XtGeometryYes;
      } else 
	  return XtGeometryNo;
}


/*
 *************************************************************************
 *
 * Public creation entry points
 *
 *************************************************************************
 */
/*
 * low level create entry points
 */
Widget 
#ifdef _NO_PROTO
XmCreateDialogShell( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateDialogShell(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (XtCreatePopupShell(name, xmDialogShellWidgetClass, p, al, ac));
}

