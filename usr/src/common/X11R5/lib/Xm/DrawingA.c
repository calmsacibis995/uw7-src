#pragma ident	"@(#)m1.2libs:Xm/DrawingA.c	1.4"
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
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "XmI.h"
#include "RepTypeI.h"
#include "TraversalI.h"
#include "GMUtilsI.h"
#include <Xm/DrawingAP.h>
#include <Xm/TransltnsP.h>


#define	MARGIN_DEFAULT		10

#define defaultTranslations	_XmDrawingA_defaultTranslations
#define traversalTranslations	_XmDrawingA_traversalTranslations


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Redisplay() ;
static void Resize() ;
static XtGeometryResult GeometryManager() ;
static void ChangeManaged() ;
static Boolean SetValues() ;
static XtGeometryResult QueryGeometry() ;
static XmNavigability WidgetNavigable() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass w_class) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Resize( 
                        Widget wid) ;
static XtGeometryResult GeometryManager( 
                        Widget w,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static void ChangeManaged( 
                        Widget wid) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static XtGeometryResult QueryGeometry( 
                        Widget wid,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *desired) ;
static XmNavigability WidgetNavigable( 
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static XtActionsRec actionsList[] =
{
   { "DrawingAreaInput", _XmDrawingAreaInput },
};


/*  Resource definitions for DrawingArea
 */

static XmSyntheticResource syn_resources[] =
{
	{	XmNmarginWidth,
		sizeof (Dimension),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.margin_width),
		_XmFromHorizontalPixels,
		_XmToHorizontalPixels
	},

	{	XmNmarginHeight,
		sizeof (Dimension),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.margin_height),
		_XmFromVerticalPixels,
		_XmToVerticalPixels
	},
};


static XtResource resources[] =
{
	{	XmNmarginWidth,
		XmCMarginWidth, XmRHorizontalDimension, sizeof (Dimension),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.margin_width),
		XmRImmediate, (XtPointer) MARGIN_DEFAULT
	},

	{	XmNmarginHeight,
		XmCMarginHeight, XmRVerticalDimension, sizeof (Dimension),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.margin_height),
		XmRImmediate, (XtPointer) MARGIN_DEFAULT
	},

	{	XmNresizeCallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.resize_callback),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNexposeCallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.expose_callback),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNinputCallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.input_callback),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNresizePolicy,
		XmCResizePolicy, XmRResizePolicy, sizeof (unsigned char),
		XtOffsetOf( struct _XmDrawingAreaRec, drawing_area.resize_policy),
		XmRImmediate, (XtPointer) XmRESIZE_ANY
	},

};



/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

static XmBaseClassExtRec baseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    NULL,				/* InitializePrehook	*/
    NULL,				/* SetValuesPrehook	*/
    NULL,				/* InitializePosthook	*/
    NULL,				/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    NULL,				/* secondaryCreate	*/
    NULL,               		/* getSecRes data	*/
    { 0 },      			/* fastSubclass flags	*/
    NULL,				/* getValuesPrehook	*/
    NULL,				/* getValuesPosthook	*/
    NULL,                               /* classPartInitPrehook */
    NULL,                               /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    WidgetNavigable,                    /* widgetNavigable      */
    NULL                                /* focusChange          */
};

externaldef( xmdrawingareaclassrec) XmDrawingAreaClassRec
                                                        xmDrawingAreaClassRec =
{
   {			/* core_class fields      */
      (WidgetClass) &xmManagerClassRec,		/* superclass         */
      "XmDrawingArea",				/* class_name         */
      sizeof(XmDrawingAreaRec),			/* widget_size        */
      ClassInitialize,	        		/* class_initialize   */
      ClassPartInitialize,			/* class_part_init    */
      FALSE,					/* class_inited       */
      Initialize,       			/* initialize         */
      NULL,					/* initialize_hook    */
      XtInheritRealize,				/* realize            */
      actionsList,				/* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      TRUE,					/* compress_motion    */
      FALSE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      NULL,			                /* destroy            */
      Resize,           			/* resize             */
      Redisplay,	        		/* expose             */
      SetValues,                		/* set_values         */
      NULL,					/* set_values_hook    */
      XtInheritSetValuesAlmost,	        	/* set_values_almost  */
      NULL,					/* get_values_hook    */
      NULL,					/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      defaultTranslations,			/* tm_table           */
      QueryGeometry,                    	/* query_geometry     */
      NULL,             	                /* display_accelerator*/
      (XtPointer)&baseClassExtRec,              /* extension          */
   },
   {		/* composite_class fields */
      GeometryManager,    	                /* geometry_manager   */
      ChangeManaged,	                	/* change_managed     */
      XtInheritInsertChild,			/* insert_child       */
      XtInheritDeleteChild,     		/* delete_child       */
      NULL,                                     /* extension          */
   },

   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      0,					/* constraint size      */   
      NULL,					/* init proc            */   
      NULL,					/* destroy proc         */   
      NULL,					/* set values proc      */   
      NULL,                                     /* extension            */
   },

   {		/* manager_class fields */
      traversalTranslations,			/* translations           */
      syn_resources,				/* syn_resources      	  */
      XtNumber (syn_resources),			/* num_get_resources 	  */
      NULL,					/* syn_cont_resources     */
      0,					/* num_get_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension           */    
   },

   {		/* drawingArea class */     
      (XtPointer) NULL,				/* extension pointer */
   }	
};

externaldef( xmdrawingareawidgetclass) WidgetClass xmDrawingAreaWidgetClass
                                       = (WidgetClass) &xmDrawingAreaClassRec ;



/****************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{   
  baseClassExtRec.record_type = XmQmotif ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( w_class )
        WidgetClass w_class ;
#else
ClassPartInitialize(
        WidgetClass w_class )
#endif /* _NO_PROTO */
{   
/****************/

    _XmFastSubclassInit( w_class, XmDRAWING_AREA_BIT) ;

    return ;
    }

/****************************************************************
 * Let pass thru zero size, we'll catch them in Realize
 ****************/
static void 
#ifdef _NO_PROTO
Initialize( rw, nw, args, num_args )
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
            XmDrawingAreaWidget new_w = (XmDrawingAreaWidget) nw ;
/****************/
	if(    (new_w->drawing_area.resize_policy != XmRESIZE_SWINDOW)
            && !XmRepTypeValidValue( XmRID_RESIZE_POLICY,
                            new_w->drawing_area.resize_policy, (Widget) new_w)    )
        {   new_w->drawing_area.resize_policy = XmRESIZE_ANY ;
            } 

        return ;
}


/****************************************************************
 * General redisplay function called on exposure events.
 ****************/
static void 
#ifdef _NO_PROTO
Redisplay( wid, event, region )
        Widget wid ;
        XEvent *event ;
        Region region ;
#else
Redisplay(
        Widget wid,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
    XmDrawingAreaWidget da = (XmDrawingAreaWidget) wid ;
    XmDrawingAreaCallbackStruct cb;
/****************/
   
    cb.reason = XmCR_EXPOSE;
    cb.event = event;
    cb.window = XtWindow (da);

    XtCallCallbackList ((Widget) da, da->drawing_area.expose_callback, &cb);

    _XmRedisplayGadgets( (Widget) da, event, region);
    return ;
}
/****************************************************************
 * Invoke the application resize callbacks.
 ****************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmDrawingAreaWidget da = (XmDrawingAreaWidget) wid ;
    XmDrawingAreaCallbackStruct cb;
/****************/

    cb.reason = XmCR_RESIZE;
    cb.event = NULL;
    cb.window = XtWindow (da);

    XtCallCallbackList ((Widget) da, da->drawing_area.resize_callback, &cb);
    return ;
}

/****************************************************************
 * This function processes key and button presses and releases
 *   belonging to the DrawingArea.
 ****************/
void 
#ifdef _NO_PROTO
_XmDrawingAreaInput( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmDrawingAreaInput(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
            XmDrawingAreaWidget da = (XmDrawingAreaWidget) wid ;
            XmDrawingAreaCallbackStruct cb ;
            int x, y ;
            Boolean button_event, input_on_gadget, focus_explicit ;
/****************/

    if ((event->type == ButtonPress) || 
	(event->type == ButtonRelease)) {
	x = event->xbutton.x ;
	y = event->xbutton.y ;
	button_event = True ;
    } else 
    if (event->type == MotionNotify) {
	x = event->xmotion.x ;
	y = event->xmotion.y ;
	button_event = True ;
    } else
    if ((event->type == KeyPress) || 
	(event->type == KeyRelease)) {
	x = event->xkey.x ;
	y = event->xkey.y ;
	button_event = False ;
    } else return ; 
	    /* Unrecognized event (cannot determine x, y of pointer).*/
	
    input_on_gadget = (_XmInputForGadget((Widget)da, x, y) != NULL);
	    
    focus_explicit = ((_XmGetFocusPolicy((Widget)da) == XmEXPLICIT) &&
		      (da->composite.num_children != 0));

    if (!input_on_gadget) {
	if ((!focus_explicit) || (button_event)) {
	    cb.reason = XmCR_INPUT ;
	    cb.event = event ;
	    cb.window = XtWindow( da) ;
	    XtCallCallbackList ((Widget) da,
 				da->drawing_area.input_callback, &cb) ;

	}
    }
    return ;
}


/****************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
GeometryManager( w, request, reply )
        Widget w ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
GeometryManager(
        Widget w,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
            XmDrawingAreaWidget da;
/*            XtGeometryHandler manager ; */
/****************/

    da = (XmDrawingAreaWidget) w->core.parent;

    /* first treat the special case for scrolledwindow clipwindow. 
       If resize policy is xwresize_swindow then simply pass this request 
       to my parent and return its response to the requesting widget. */
    if (da->drawing_area.resize_policy == XmRESIZE_SWINDOW) {
/****************
 *
 * Ugh. Because of the structure of the DnD wrappers, and the heinous nature
 * of what we are about to do, we need to make a real geo request. Due to
 * the possible corner-case loss of data, we need to check to make sure that
 * the requesting child is really the workwindow. And it gets even worse. There 
 * is a corner case where the child can ask to become the exact size of the 
 * clipwindow. This generates a request that is the "same", so the Intrinsics 
 * helpfully bail out without making the request. So, we munge the da core fields
 * to force a call to occur. Oh well - it all works because of the way Scrolled
 * window calculates spacing.
 *
 ****************/

      Widget tmp = NULL;
      Arg args[1];
      XtGeometryResult res;

      XtSetArg(args[0], XmNworkWindow, &tmp);
      XtGetValues(da->core.parent, args, 1);
      if ((tmp != NULL) && (w == tmp))
      {
          if ((request->request_mode & CWWidth) &&
	      ( request->width == da->core.width))
	      da->core.width++;
	      
          if ((request->request_mode & CWHeight) &&
	      (request->height == da->core.height))
	      da->core.height++;

	  res = XtMakeGeometryRequest((Widget) da, request, reply);
	  return (res);
      }
      else
          return(XtGeometryNo);
    }

    /* function shared with Bulletin Board */
    return(_XmGMHandleGeometryManager((Widget)da, w, request, reply, 
                                    da->drawing_area.margin_width, 
                                    da->drawing_area.margin_height, 
                                    da->drawing_area.resize_policy,
                                    True));
    }

/****************************************************************
 * Re-layout children.
 ****************/
static void 
#ifdef _NO_PROTO
ChangeManaged( wid )
        Widget wid ;
#else
ChangeManaged(
        Widget wid )
#endif /* _NO_PROTO */
{
            XmDrawingAreaWidget da = (XmDrawingAreaWidget) wid ;
            XtWidgetProc manager ;
/****************/

    /* first treat the special case for the scrolledwindow clipwindow */

    if (da->drawing_area.resize_policy == XmRESIZE_SWINDOW)
       {
         manager = ((CompositeWidgetClass) (da->core.parent->core.widget_class))
                      ->composite_class.change_managed;
         (*manager)(da->core.parent);
         _XmNavigChangeManaged( (Widget) da) ;
         return;
       }

    /* function shared with Bulletin Board */
    _XmGMEnforceMargin((XmManagerWidget)da,
                     da->drawing_area.margin_width,
                     da->drawing_area.margin_height, 
                     False); /* use movewidget, not setvalue */

    /* The first time, reconfigure only if explicit size were not given */

    if (XtIsRealized((Widget)da) || (!XtWidth(da)) || (!XtHeight(da))) {

      /* function shared with Bulletin Board */
      (void)_XmGMDoLayout((XmManagerWidget)da,
                          da->drawing_area.margin_width,
                          da->drawing_area.margin_height,
                          da->drawing_area.resize_policy,
                          False);  /* queryonly not specified */
        }
  
    _XmNavigChangeManaged((Widget) da) ;

    return ;
    }

/****************************************************************/
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
    XmDrawingAreaWidget current = (XmDrawingAreaWidget) cw ;
    XmDrawingAreaWidget new_w = (XmDrawingAreaWidget) nw ;
/****************/

    if(    (new_w->drawing_area.resize_policy != XmRESIZE_SWINDOW)
       && !XmRepTypeValidValue( XmRID_RESIZE_POLICY,
			       new_w->drawing_area.resize_policy, 
			       (Widget) new_w)  ) {   
	new_w->drawing_area.resize_policy = 
	    current->drawing_area.resize_policy ;
    } 

    /* If new margins, re-enforce them using movewidget, 
       then update the width and height so that XtSetValues does
       the geometry request */
    if (XtIsRealized((Widget) new_w) &&
        (((new_w->drawing_area.margin_width != 
	  current->drawing_area.margin_width) ||
	 (new_w->drawing_area.margin_height !=
	  current->drawing_area.margin_height)))) {
	    
	/* move the child around if necessary */
	_XmGMEnforceMargin((XmManagerWidget)new_w,
			   new_w->drawing_area.margin_width,
			   new_w->drawing_area.margin_height,
			   False); /* use movewidget, no request */
	_XmGMCalcSize ((XmManagerWidget)new_w, 
		       new_w->drawing_area.margin_width, 
		       new_w->drawing_area.margin_height, 
		       &new_w->core.width, &new_w->core.height);
    }

    return( False) ;
}

   
/****************************************************************
 * Handle query geometry requests
 ****************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryGeometry( wid, intended, desired )
        Widget wid ;
        XtWidgetGeometry *intended ;
        XtWidgetGeometry *desired ;
#else
QueryGeometry(
        Widget wid,
        XtWidgetGeometry *intended,
        XtWidgetGeometry *desired )
#endif /* _NO_PROTO */
{
    XmDrawingAreaWidget da = (XmDrawingAreaWidget) wid ;
/****************/
    
    if (da->drawing_area.resize_policy == XmRESIZE_SWINDOW)
	return XtGeometryNo ;
	    
     /* function shared with Bulletin Board */
     return(_XmGMHandleQueryGeometry(wid, intended, desired, 
                                   da->drawing_area.margin_width, 
                                   da->drawing_area.margin_height, 
                                   da->drawing_area.resize_policy));
    }

static XmNavigability
#ifdef _NO_PROTO
WidgetNavigable( wid)
        Widget wid ;
#else
WidgetNavigable(
        Widget wid)
#endif /* _NO_PROTO */
{   
  /* If a Drawing Area has no navigable children, then it is being
   *  used as an X Window work area, so allow navigation to it.
   */
  if(    wid->core.sensitive
     &&  wid->core.ancestor_sensitive
     &&  ((XmManagerWidget) wid)->manager.traversal_on    )
    {   
      XmNavigationType nav_type = ((XmManagerWidget) wid)
	                                            ->manager.navigation_type ;
      Widget *children = ((XmManagerWidget) wid)->composite.children ;
      unsigned idx = 0 ;

      if(    ((XmDrawingAreaWidget) wid)->drawing_area.resize_policy
	                                               == XmRESIZE_SWINDOW    )
	{
	  if(    (nav_type == XmSTICKY_TAB_GROUP)
	     ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
	     ||  (    (nav_type == XmTAB_GROUP)
		  &&  !_XmShellIsExclusive( wid))    )
	    {
	      return XmDESCENDANTS_TAB_NAVIGABLE ;
	    }
	  return XmDESCENDANTS_NAVIGABLE ;
	}
      while(    idx < ((XmManagerWidget) wid)->composite.num_children    )
	{
	  if(    _XmGetNavigability( children[idx])    )
	    {
	      if(    (nav_type == XmSTICKY_TAB_GROUP)
		 ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
		 ||  (    (nav_type == XmTAB_GROUP)
		      &&  !_XmShellIsExclusive( wid))    )
		{
		  return XmDESCENDANTS_TAB_NAVIGABLE ;
		}
	      return XmDESCENDANTS_NAVIGABLE ;
	    }
	  ++idx ;
	}
      if(    (nav_type == XmSTICKY_TAB_GROUP)
	 ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
	 ||  (    (nav_type == XmTAB_GROUP)
	      &&  !_XmShellIsExclusive( wid))    )
	{
	  return XmTAB_NAVIGABLE ;
	}
      return XmCONTROL_NAVIGABLE ;
    }
  return XmNOT_NAVIGABLE ;
}

/****************************************************************
 * This function creates and returns a DrawingArea widget.
 ****************/
Widget 
#ifdef _NO_PROTO
XmCreateDrawingArea( p, name, args, n )
        Widget p ;
        String name ;
        ArgList args ;
        Cardinal n ;
#else
XmCreateDrawingArea(
        Widget p,
        String name,
        ArgList args,
        Cardinal n )
#endif /* _NO_PROTO */
{
/****************/

    return( XtCreateWidget( name, xmDrawingAreaWidgetClass, p, args, n)) ;
}
