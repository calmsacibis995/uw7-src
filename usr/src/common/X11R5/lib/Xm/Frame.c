#pragma ident	"@(#)m1.2libs:Xm/Frame.c	1.3"
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

#include <Xm/FrameP.h>
#include <Xm/GadgetP.h>
#include "XmI.h"
#include "RepTypeI.h"
#include <Xm/DrawP.h>
#include "GMUtilsI.h"

#define GetFrameConstraint(w) \
        (&((XmFrameConstraintPtr) (w)->core.constraints)->frame)


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void DrawShadow() ;
static void ClearShadow();
static void ConfigureChildren() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Resize() ;
static void Redisplay() ;
static XtGeometryResult GeometryManager() ;
static void ChangeManaged() ;
static Boolean SetValues() ;
static XtGeometryResult QueryGeometry() ;
static void CalcFrameSize() ;
static void CalcWorkAreaSize() ;
static void CalcTitleExtent();
static void ConstraintInitialize();
static Boolean ConstraintSetValues();
static void ConstraintDestroy();

#else

static void DrawShadow(
                        XmFrameWidget fw) ;
static void ClearShadow(
                        XmFrameWidget fw) ;
static void ConfigureChildren(
                        XmFrameWidget fw,
			Widget instigator,
			XtWidgetGeometry *inst_desired) ;

static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Resize( 
                        Widget wid) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
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
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *ret) ;
static void CalcFrameSize( 
                        XmFrameWidget fw,
#if NeedWidePrototypes
                        int titleWidth,
                        int titleHeight,
                        int titleBorder,
                        int workWidth,
                        int workHeight,
                        int workBorder,
#else
                        Dimension titleWidth,
                        Dimension titleHeight,
                        Dimension titleBorder,
                        Dimension workWidth,
                        Dimension workHeight,
                        Dimension workBorder,
#endif /* NeedWidePrototypes */
                        Dimension *fwWidth,
                        Dimension *fwHeight) ;
static void CalcWorkAreaSize( 
                        XmFrameWidget fw,
                        Dimension *workWidth,
                        Dimension *workHeight,
#if NeedWidePrototypes
                        int workBorder,
                        int fwWidth,
                        int fwHeight) ;
#else
                        Dimension workBorder,
                        Dimension fwWidth,
                        Dimension fwHeight) ;
#endif /* NeedWidePrototypes */
static void CalcTitleExtent(
                        XmFrameWidget fw,
#if NeedWidePrototypes
                        int titleHeight,
                        int titleBorder,
#else
                        Dimension titleHeight,
                        Dimension titleBorder,
#endif /* NeedWidePrototypes */
			Dimension *titleExtent,
			Position *titleY,
			Dimension *shadowWidth,
			Position *shadowY) ;
static void ConstraintInitialize(
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean ConstraintSetValues(
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void ConstraintDestroy(
			Widget w) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/* Default translation table */

#define defaultTranslations[]	_XmFrame_defaultTranslations


static XmSyntheticResource syn_resources[] = 
{
   {  XmNmarginWidth, 
      sizeof (Dimension),
      XtOffsetOf( struct _XmFrameRec, frame.margin_width),
      _XmFromHorizontalPixels,
      _XmToHorizontalPixels },

   {  XmNmarginHeight, 
      sizeof (Dimension),
      XtOffsetOf( struct _XmFrameRec, frame.margin_height),
      _XmFromVerticalPixels,
      _XmToVerticalPixels, }
};

static XtResource resources[] =
{
   {  XmNmarginWidth, 
      XmCMarginWidth, 
      XmRHorizontalDimension, 
      sizeof (Dimension),
      XtOffsetOf( struct _XmFrameRec, frame.margin_width),
      XmRImmediate, (XtPointer) 0 },

   {  XmNmarginHeight, 
      XmCMarginHeight, 
      XmRVerticalDimension,
      sizeof (Dimension),
      XtOffsetOf( struct _XmFrameRec, frame.margin_height),
      XmRImmediate, (XtPointer) 0 },

   {  XmNshadowType,
      XmCShadowType,
      XmRShadowType,
      sizeof (unsigned char),
      XtOffsetOf( struct _XmFrameRec, frame.shadow_type),
      XmRImmediate, (XtPointer) XmINVALID_DIMENSION },

   {  XmNshadowThickness,
      XmCShadowThickness,
      XmRHorizontalDimension,
      sizeof (Dimension),
      XtOffsetOf( struct _XmFrameRec, manager.shadow_thickness),
      XmRImmediate, (XtPointer) XmINVALID_DIMENSION }
};

static XmSyntheticResource syn_constraints[] =
{
   {  XmNchildHorizontalSpacing, sizeof (Dimension),
      XtOffsetOf( struct _XmFrameConstraintRec, frame.child_h_spacing),
      _XmFromHorizontalPixels, _XmToHorizontalPixels }
};

static XtResource constraints[] =
{
   {  XmNchildType, XmCChildType, XmRChildType,
      sizeof (unsigned char),
      XtOffsetOf( struct _XmFrameConstraintRec, frame.child_type),
      XmRImmediate, (XtPointer) XmFRAME_WORKAREA_CHILD },

   {  XmNchildHorizontalAlignment, XmCChildHorizontalAlignment,
      XmRChildHorizontalAlignment,
      sizeof (unsigned char),
      XtOffsetOf( struct _XmFrameConstraintRec, frame.child_h_alignment),
      XmRImmediate, (XtPointer) XmALIGNMENT_BEGINNING },

   {  XmNchildVerticalAlignment, XmCChildVerticalAlignment,
      XmRChildVerticalAlignment,
      sizeof (unsigned char),
      XtOffsetOf( struct _XmFrameConstraintRec, frame.child_v_alignment),
      XmRImmediate, (XtPointer) XmALIGNMENT_CENTER   },

   {  XmNchildHorizontalSpacing, XmCChildHorizontalSpacing,
      XmRHorizontalDimension, sizeof (Dimension),
      XtOffsetOf( struct _XmFrameConstraintRec, frame.child_h_spacing),
      XmRImmediate, (XtPointer) XmINVALID_DIMENSION}
};

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

externaldef(xmframeclassrec) XmFrameClassRec xmFrameClassRec = 
{
   {			/* core_class fields      */
      (WidgetClass) &xmManagerClassRec,		/* superclass         */
      "XmFrame",				/* class_name         */
      sizeof(XmFrameRec),			/* widget_size        */
      NULL,					/* class_initialize   */
      ClassPartInitialize,			/* class_part_init    */
      FALSE,					/* class_inited       */
      Initialize,       			/* initialize         */
      NULL,					/* initialize_hook    */
      XtInheritRealize,				/* realize            */
      NULL,					/* actions	      */
      0,					/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      TRUE,					/* compress_motion    */
      XtExposeCompressMaximal,	                /* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      NULL,					/* destroy            */
      Resize,			                /* resize             */
      Redisplay,		    	        /* expose             */
      SetValues,                		/* set_values         */
      NULL,					/* set_values_hook    */
      XtInheritSetValuesAlmost,			/* set_values_almost  */
      NULL,					/* get_values_hook    */
      NULL,					/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      XtInheritTranslations,			/* tm_table           */
      QueryGeometry,	                        /* query_geometry     */
      NULL,                                     /* display_accelerator   */
      NULL,                                     /* extension             */
   },

   {		/* composite_class fields */
      GeometryManager,    	                /* geometry_manager   */
      ChangeManaged,		                /* change_managed     */
      XtInheritInsertChild,		        /* insert_child       */
      XtInheritDeleteChild,			/* delete_child       */
      NULL,                                     /* extension          */
   },

   {		/* constraint_class fields */
      constraints,				/* resource list        */   
      XtNumber(constraints),			/* num resources        */   
      sizeof (XmFrameConstraintRec),            /* constraint size      */   
      ConstraintInitialize,			/* init proc            */   
      ConstraintDestroy,			/* destroy proc         */   
      ConstraintSetValues,			/* set values proc      */   
      NULL,                                     /* extension            */
   },

   {						/* manager class     */
      XtInheritTranslations,			/* translations      */
      syn_resources,				/* syn resources      	  */
      XtNumber(syn_resources),			/* num syn_resources 	  */
      syn_constraints,				/* get_cont_resources     */
      XtNumber(syn_constraints),		/* num_get_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension         */    
   },

   {						/* frame class       */
      NULL,					/* extension         */    
   }	
};

externaldef(xmframewidgetclass) WidgetClass xmFrameWidgetClass =
			                        (WidgetClass) &xmFrameClassRec;




/************************************************************************
 *
 *  DrawShadow
 *	Draw the Frame shadow
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
DrawShadow( fw )
        XmFrameWidget fw ;
#else
DrawShadow(
        XmFrameWidget fw )
#endif /* _NO_PROTO */
{
   if (XtIsRealized(fw)) {
       _XmDrawShadows(XtDisplay (fw), XtWindow (fw),
		fw->manager.top_shadow_GC, fw->manager.bottom_shadow_GC,
		fw->frame.old_shadow_x, fw->frame.old_shadow_y,
		fw->frame.old_width, fw->frame.old_height,
		fw->frame.old_shadow_thickness, fw->frame.shadow_type);
   }
}




/************************************************************************
 *
 *  ClearShadow
 *	Erase the Frame shadow
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
ClearShadow( fw )
        XmFrameWidget fw ;
#else
ClearShadow(
        XmFrameWidget fw )
#endif /* _NO_PROTO */
{
   if (XtIsRealized(fw)) {
	_XmClearBorder (XtDisplay(fw), XtWindow(fw),
		      fw->frame.old_shadow_x,
		      fw->frame.old_shadow_y, 
		      fw->frame.old_width, 
		      fw->frame.old_height,
		      fw->frame.old_shadow_thickness);

    }
}




/************************************************************************
 *
 *  ConfigureChildren
 *	Configure the title and work area if they aren't instigaror
 *      of the request (Yes policy). Compute the shadow location.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ConfigureChildren( fw, instigator, inst_geometry)
	XmFrameWidget fw;	
        Widget instigator;
        XtWidgetGeometry * inst_geometry ;
#else
ConfigureChildren(
	XmFrameWidget fw,
	Widget instigator,
	XtWidgetGeometry * inst_geometry)
#endif /* _NO_PROTO */
{
    Widget child;
    XmFrameConstraint fc;
    Position childX = 0;
    Position childY;
    Dimension childWidth;
    Dimension childHeight;
    Dimension childBW;
    Dimension shadowThickness = fw->manager.shadow_thickness;
    Dimension titleExtent = shadowThickness;
    Dimension shadowHeight = fw->core.height;
    Position shadowY = 0;
    XtWidgetGeometry title_reply;
    Dimension spacing;

    if (fw->frame.title_area && XtIsManaged(fw->frame.title_area)) {
	child = fw->frame.title_area;
	fc = GetFrameConstraint(child);
	spacing = shadowThickness + fc->child_h_spacing;

	/* asking the preferred geometry without constraint */
	XtQueryGeometry (child, NULL, &title_reply);
	childWidth = (title_reply.request_mode & CWWidth) ?
			title_reply.width : child->core.width;
	childHeight = (title_reply.request_mode & CWHeight) ?
			title_reply.height : child->core.height;
	childBW = child->core.border_width;
	if (child == instigator) {
	    childWidth = (inst_geometry->request_mode & CWWidth) ?
			inst_geometry->width : childWidth;
	    childHeight = (inst_geometry->request_mode & CWHeight) ?
			inst_geometry->height : childHeight;
	    childBW = (inst_geometry->request_mode & CWBorderWidth) ?
			inst_geometry->border_width : childBW;
	}
	if (childWidth + 2 * (spacing + childBW) > fw->core.width) {
	    if (fw->core.width > 2 * (spacing + childBW))
		childWidth = fw->core.width - 2 * (spacing + childBW);
	    else
		childWidth = 1;
	}
	switch (fc->child_h_alignment) {
	    case(XmALIGNMENT_BEGINNING):
		if (fw->manager.string_direction == XmSTRING_DIRECTION_L_TO_R)
		    childX = spacing;
		else
		    childX = fw->core.width - spacing -
				childWidth - 2 * childBW;
		break;
	    case(XmALIGNMENT_CENTER):
		childX = fw->core.width/2 - childWidth/2 - childBW;
	        break;
	    case(XmALIGNMENT_END):
	    default:
		if (fw->manager.string_direction == XmSTRING_DIRECTION_L_TO_R)
		    childX = fw->core.width - spacing -
				childWidth - 2 * childBW;
		else
		    childX = spacing;
		break;
	}
	CalcTitleExtent (fw, childHeight, childBW,
				&titleExtent, &childY, &shadowHeight, &shadowY);

	if (child != instigator) {
	    _XmConfigureObject (child, childX, childY, childWidth,
				childHeight, childBW);
	}
	else {
	    /* Do not resize the instigator, just return GeometryYes */

	    inst_geometry->request_mode = CWX | CWY | CWWidth | CWHeight |
						CWBorderWidth;
	    child->core.x = childX; 
	    child->core.y = childY; 
	    child->core.width = childWidth; 
	    child->core.height = childHeight; 
	    child->core.border_width = childBW;
	}
    }

    if (fw->frame.work_area  && XtIsManaged(fw->frame.work_area)) {
	child = fw -> frame.work_area;
	if (child != instigator)
	    childBW = child->core.border_width;
	else
	    childBW = (inst_geometry->request_mode & CWBorderWidth) ?
		    inst_geometry->border_width : child->core.border_width;
	    
	CalcWorkAreaSize (fw, &childWidth, &childHeight, childBW,
			      fw->core.width, fw->core.height);
	childX = shadowThickness + fw->frame.margin_width;
	childY = titleExtent + fw->frame.margin_height;
	if (child != instigator) {
	    _XmConfigureObject (child, childX, childY, childWidth, childHeight,
				childBW);
	}
	else {
	    /* Do not resize the instigator, just return GeometryYes */

	    inst_geometry->request_mode = CWX | CWY | CWWidth | CWHeight |
						CWBorderWidth;
	    child->core.x = childX; 
	    child->core.y = childY; 
	    child->core.width = childWidth; 
	    child->core.height = childHeight; 
	    child->core.border_width = childBW;
	}   
    }

    fw->frame.old_shadow_x = 0;
    fw->frame.old_shadow_y = shadowY;
    fw->frame.old_width = fw->core.width;
    fw->frame.old_height = shadowHeight;
    fw->frame.old_shadow_thickness = shadowThickness;

}




/************************************************************************
 *
 *  ClassPartInitialize
 *     Set up the fast subclassing for the widget
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
   _XmFastSubclassInit (wc, XmFRAME_BIT);
}

      


/************************************************************************
 *
 *  Initialize
 *	Ensure that the width and height are not 0.
 *
 *************************************<->***********************************/
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
   XmFrameWidget request = (XmFrameWidget) rw ;
   XmFrameWidget new_w = (XmFrameWidget) nw;

   new_w->frame.title_area = NULL;
   new_w->frame.work_area = NULL;
   new_w->frame.processing_constraints = False;

   if (new_w->frame.shadow_type == (unsigned char) XmINVALID_DIMENSION)
      if (XtIsShell (XtParent(new_w)))
         new_w->frame.shadow_type = XmSHADOW_OUT;
      else
         new_w->frame.shadow_type = XmSHADOW_ETCHED_IN;

   if (!XmRepTypeValidValue( XmRID_SHADOW_TYPE, new_w->frame.shadow_type,
			(Widget) new_w))
   {
      new_w->frame.shadow_type = XmSHADOW_ETCHED_IN;
   }

   if (request->manager.shadow_thickness == XmINVALID_DIMENSION)
      if (XtIsShell (XtParent(new_w)))
         new_w->manager.shadow_thickness = 1;
      else
         new_w->manager.shadow_thickness = 2;

   new_w->frame.old_shadow_x = 0;
   new_w->frame.old_shadow_y = 0;
   new_w->frame.old_width = new_w->core.width;
   new_w->frame.old_height = new_w->core.height;
   new_w->frame.old_shadow_thickness = new_w->manager.shadow_thickness;
}




/************************************************************************
 *
 *  Resize 
 *  	Calculate the size of the children and resize.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
   XmFrameWidget fw = (XmFrameWidget) wid ;

   ClearShadow(fw);

   ConfigureChildren(fw, NULL, NULL);

   DrawShadow(fw);
}




/************************************************************************
 *
 *  Redisplay
 *     General redisplay function called on exposure events.
 *
 ************************************************************************/
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
   XmFrameWidget fw = (XmFrameWidget) wid;
   Widget title_area = fw->frame.title_area;

   DrawShadow(fw);

   /* since the shadow may have screw up the gadget title, while this
      one won't get refresh, we have to redraw it manually */

   if (title_area && XmIsGadget(title_area) && XtIsManaged(title_area))
   {
      XClearArea (XtDisplay(fw), XtWindow(fw),
		  title_area->core.x, title_area->core.y,
		  title_area->core.width, title_area->core.height,
		  False);
      if (region && !XRectInRegion (region, title_area->core.x,
	title_area->core.y, title_area->core.width, title_area->core.height))
      {
         if (title_area->core.widget_class->core_class.expose)
	    (*(title_area->core.widget_class->core_class.expose))(
		title_area, event, NULL);
      }
   }

   _XmRedisplayGadgets( (Widget) fw, event, region);
}




/************************************************************************
 *
 *  Geometry Manager
 *	Take the requested geometry, calculate the needed size for
 *	the frame and make a request to the frames parent.
 *      Requests to change x, y position are always denied.
 *
 ************************************************************************/
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
   XmFrameWidget fw = (XmFrameWidget) XtParent(w);
   Widget title_area = fw->frame.title_area;
   Widget work_area = fw->frame.work_area;
   Dimension req_width, req_height, req_bw;
   Boolean query_only = False;
   Boolean almost = False;
   Dimension title_width = (title_area)?title_area->core.width:0;
   Dimension title_height = (title_area)?title_area->core.height:0;
   Dimension title_bw = (title_area)?title_area->core.border_width:0;
   Dimension work_width = (work_area)?work_area->core.width:0;
   Dimension work_height = (work_area)?work_area->core.height:0;
   Dimension work_bw = (work_area)?work_area->core.border_width:0;
   Dimension frame_width, frame_height;
   XtWidgetGeometry parent_request;
   XtWidgetGeometry parent_reply;
   Dimension almost_width;
   Dimension almost_height;

   if (fw->frame.processing_constraints)
   {
      fw->frame.processing_constraints = False;
      request -> border_width -= 1;
   }

   /*  Set up the calculation variables according to the  */
   /*  contents of the requested geometry.                */

   if (request -> request_mode & XtCWQueryOnly)
      query_only = True;

   if ((request -> request_mode & CWX) || (request -> request_mode & CWY))
      almost = True;

   if (request -> request_mode & CWWidth) req_width = request -> width;
   else req_width = w -> core.width;

   if (request -> request_mode & CWHeight) req_height = request -> height;
   else req_height = w -> core.height;

   if (request -> request_mode & CWBorderWidth)
       req_bw = request -> border_width;
   else req_bw = w -> core.border_width;

   if (w == title_area)
   {
      title_width = req_width;
      title_height = req_height;
      title_bw = req_bw;
   }
   if (w == work_area)
   {
      work_width = req_width;
      work_height = req_height;
      work_bw = req_bw;
   }

   /* find the frame size based on the children preferred geometry */

   CalcFrameSize (fw, title_width, title_height, title_bw,
		  work_width, work_height, work_bw,
		  &frame_width, &frame_height);

   parent_request.request_mode = CWWidth | CWHeight;
   if (almost || query_only) parent_request.request_mode |= XtCWQueryOnly;
   parent_request.width = frame_width;
   parent_request.height = frame_height;

   switch (XtMakeGeometryRequest ((Widget)fw, 
				  &parent_request, &parent_reply)) {
   case XtGeometryYes:
       if (!almost) {
	   if (!query_only) {
	       ClearShadow(fw);
	       ConfigureChildren(fw, w, request);
	       DrawShadow(fw);
	   }
	   return (XtGeometryYes);
         } else {
	     almost_width = request->width;
	     almost_height = request->height;
	 }
       break;
   case XtGeometryNo:
	 if (w == title_area) { 
	     /* we got a No, try to honor the title request anyway,
	        by resizing the work_area child */
	     if (!almost) {
		 if (!query_only) {
		     ClearShadow(fw);
		     ConfigureChildren(fw, w, request);
		     DrawShadow(fw);
		 }
		 return (XtGeometryYes);
	     } else {
		 almost_width = request->width;
		 almost_height = request->height;
	     }
	 } else return (XtGeometryNo);
         break;

   case XtGeometryAlmost:
	 if (w == title_area) {
	     /* we got an Almost, try to honor the title request anyway,
	        by accepting the deal and resizing the work_area child */
	     if (!almost) {
		 if (!query_only) {
		     ClearShadow(fw);
		     XtMakeResizeRequest((Widget)fw, parent_reply.width,
					 parent_reply.height, NULL, NULL); 
		     ConfigureChildren(fw, w, request);
		 }
		 return (XtGeometryYes);
	     } else {
		 almost_width = request->width;
		 almost_height = request->height;
	     }
	 } else {
	     /* we got an Almost, accept the deal and 
		compute the work_area size */
	     CalcWorkAreaSize (fw, &almost_width, &almost_height,
			       req_bw, parent_reply.width, 
			       parent_reply.height);
	 }
         break;
   default:
	 break;
   }


   /*  Fallen through to an almost condition.  Clear the x and y  */
   /*  and set the width, height, and border.                     */


   if (reply != NULL) {
      reply -> request_mode = request -> request_mode & ~(CWX | CWY);
      reply -> width = almost_width;
      reply -> height = almost_height;
      reply -> border_width = req_bw;
      if (request -> request_mode & CWSibling)
               reply -> sibling = request -> sibling;
      if (request -> request_mode & CWStackMode)
               reply -> stack_mode = request -> stack_mode;
      return (XtGeometryAlmost);
   }

   return (XtGeometryNo);
}




/************************************************************************
 *
 *  ChangeManaged
 *	Process a changed in managed state of the child.  If its
 *	size is out of sync with the frame, make a resize request
 *	to change the size of the frame.
 *	Note: called before ConstraintDestroy.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ChangeManaged( wid )
        Widget wid ;
#else
ChangeManaged(
        Widget wid )
#endif /* _NO_PROTO */
{
   XmFrameWidget fw = (XmFrameWidget) wid ;

   Widget title_area = (fw->frame.title_area &&
			XtIsManaged(fw->frame.title_area)) ?
			fw->frame.title_area : (Widget) NULL;
   Dimension t_w = (title_area) ? title_area->core.width : 0;
   Dimension t_h = (title_area) ? title_area->core.height : 0;
   Dimension t_bw = (title_area) ? title_area->core.border_width : 0;

   Widget work_area = (fw->frame.work_area &&
			XtIsManaged(fw->frame.work_area)) ?
			fw->frame.work_area : (Widget) NULL;

   Dimension w_w = (work_area) ? work_area->core.width : 0;
   Dimension w_h = (work_area) ? work_area->core.height : 0;
   Dimension w_bw = (work_area) ? work_area->core.border_width : 0;
   Dimension fwWidth, fwHeight;

   /* need to  check on initial sizing (not null) */
   if (XtIsRealized(fw) || (XtWidth(fw) == 0) || (XtHeight(fw) == 0)) {
       CalcFrameSize (fw, t_w, t_h, t_bw, w_w, w_h, w_bw,
		      &fwWidth, &fwHeight);

       while (XtMakeResizeRequest ((Widget) fw, 
				   fwWidth, fwHeight,
				   &fwWidth, &fwHeight) 
	      == XtGeometryAlmost) ;
       ClearShadow(fw);
   }
 
   ConfigureChildren(fw, NULL, NULL);

   DrawShadow(fw);

   _XmNavigChangeManaged((Widget) fw);
}





/************************************************************************
 *
 *  Set Values
 *	Adjust the size of the manager based on shadow thickness
 *	changes.
 *
 ************************************************************************/
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
    XmFrameWidget current = (XmFrameWidget) cw ;
    XmFrameWidget new_w = (XmFrameWidget) nw ;
    Boolean redisplay = False;
    Widget title_area = (new_w->frame.title_area &&
			 XtIsManaged(new_w->frame.title_area)) ?
			     new_w->frame.title_area : NULL;
    Dimension t_w = (title_area) ? title_area->core.width : 0;
    Dimension t_h = (title_area) ? title_area->core.height : 0;
    Dimension t_bw = (title_area) ? title_area->core.border_width : 0;

    Widget work_area = (new_w->frame.work_area &&
			XtIsManaged(new_w->frame.work_area)) ?
			    new_w->frame.work_area : (Widget) NULL;
    Dimension w_w = (work_area) ? work_area->core.width : 0;
    Dimension w_h = (work_area) ? work_area->core.height : 0;
    Dimension w_bw = (work_area) ? work_area->core.border_width : 0;


    if (!XmRepTypeValidValue( XmRID_SHADOW_TYPE, new_w->frame.shadow_type,
			     (Widget) new_w))
	{
	    new_w->frame.shadow_type = current->frame.shadow_type;
	}

    if (!XtIsRealized(new_w)) return False ;

    if (new_w->frame.margin_width != current->frame.margin_width ||
	new_w->frame.margin_height != current->frame.margin_height ||
	new_w->manager.shadow_thickness != current->manager.shadow_thickness)
	{
	    CalcFrameSize (new_w, t_w, t_h, t_bw, w_w, w_h, w_bw,
			   &new_w->core.width, &new_w->core.height);
	}
    
    if (new_w -> frame.shadow_type != current -> frame.shadow_type ||
	new_w->frame.margin_width != current->frame.margin_width ||
	new_w->frame.margin_height != current->frame.margin_height ||
	new_w->manager.shadow_thickness != current->manager.shadow_thickness)
	{
	    redisplay = True;
	}

    return (redisplay);
}




/************************************************************************
 *
 *  QueryGeometry
 *  	return width X height based on the children preferred sizes
 *
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryGeometry( widget, intended, desired )
        Widget widget ;
        XtWidgetGeometry *intended ;
        XtWidgetGeometry *desired ;
#else
QueryGeometry(
        Widget widget,
        XtWidgetGeometry *intended,
        XtWidgetGeometry *desired )
#endif /* _NO_PROTO */
{
    Dimension work_width = 0, work_height = 0, work_bw = 0 ;
    Dimension title_width = 0, title_height = 0, title_bw = 0 ;
    XtWidgetGeometry child_pref ;
    XmFrameWidget fw = (XmFrameWidget) widget ;

    /* first determine what is the desired size, using the
       preferred sizes of the title and the work_area, or the
       current setting if no preference are given */
    if (fw->frame.work_area) {
	XtQueryGeometry (fw->frame.work_area, NULL, &child_pref);    
	if (IsWidth(&child_pref)) work_width = child_pref.width ;
	else work_width = XtWidth(fw->frame.work_area);
	if (IsHeight(&child_pref)) work_height = child_pref.height ;
	else work_height = XtHeight(fw->frame.work_area);
	if (IsBorder(&child_pref)) work_bw = child_pref.border_width ;
	else work_bw = XtBorderWidth(fw->frame.work_area);
    } 
    if (fw->frame.title_area) {
	XtQueryGeometry (fw->frame.title_area, NULL, &child_pref);    
	if (IsWidth(&child_pref)) title_width = child_pref.width ;
	else title_width = XtWidth(fw->frame.title_area);
	if (IsHeight(&child_pref)) title_height = child_pref.height ;
	else title_height = XtHeight(fw->frame.title_area);
	if (IsBorder(&child_pref)) title_bw = child_pref.border_width ;
	else title_bw = XtBorderWidth(fw->frame.title_area);
    }

    CalcFrameSize (fw, 
		   title_width, title_height, title_bw,
		   work_width, work_height, work_bw,
		   &desired->width, &desired->height);

    /* deal with user initial size setting */
    if (!XtIsRealized(widget))  {
	if (XtWidth(widget) != 0) desired->width = XtWidth(widget) ;
	if (XtHeight(widget) != 0) desired->height = XtHeight(widget) ;
    }	    

    return _XmGMReplyToQueryGeometry(widget, intended, desired) ;
}


/************************************************************************
 *
 *  CalcFrameSize
 *	Calculate the manager size based on the supplied width 
 *	and height.
 *  Note: all of the dimensions passed in may be 0, in which case the return
 *  value is based on the frame's own visuals.
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CalcFrameSize( fw, titleWidth, titleHeight, titleBorder, workWidth, workHeight,
		workBorder, fwWidth, fwHeight )
        XmFrameWidget fw ;
        Dimension titleWidth ;
        Dimension titleHeight ;
        Dimension titleBorder ;
        Dimension workWidth ;
        Dimension workHeight ;
        Dimension workBorder ;
        Dimension *fwWidth ;
        Dimension *fwHeight ;
#else
CalcFrameSize(
        XmFrameWidget fw,
#if NeedWidePrototypes
        int titleWidth,
        int titleHeight,
        int titleBorder,
        int workWidth,
        int workHeight,
        int workBorder,
#else
        Dimension titleWidth,
        Dimension titleHeight,
        Dimension titleBorder,
        Dimension workWidth,
        Dimension workHeight,
        Dimension workBorder,
#endif /* NeedWidePrototypes */
        Dimension *fwWidth,
        Dimension *fwHeight )
#endif /* _NO_PROTO */
{
   Dimension shadowThickness = fw->manager.shadow_thickness;
   Dimension titleExtent = shadowThickness;
   Dimension workMax = 0;
   Dimension titleMax = 0;

   if (fw->frame.title_area && XtIsManaged(fw->frame.title_area)) {
      XmFrameConstraint fc = GetFrameConstraint(fw->frame.title_area);

      CalcTitleExtent (fw, titleHeight, titleBorder, &titleExtent, NULL,
		NULL, NULL);
      titleMax = 2 * (shadowThickness + titleBorder + fc->child_h_spacing) +
		titleWidth;
   }

   workMax = 2 * (workBorder + shadowThickness + fw->frame.margin_width) +
		workWidth;

   *fwWidth =  Max (workMax, titleMax);
   if (*fwWidth == 0) *fwWidth = 1;

   *fwHeight = workHeight + 2 * (workBorder + fw->frame.margin_height) +
       shadowThickness + titleExtent;
   if (*fwHeight == 0) *fwHeight = 1;
}




/************************************************************************
 *
 *  CalcWorkAreaSize
 *	Calculate the work area size based on the supplied width 
 *	and height.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CalcWorkAreaSize( fw, workWidth, workHeight, workBorder, fwWidth, fwHeight )
        XmFrameWidget fw ;
        Dimension *workWidth ;
        Dimension *workHeight ;
        Dimension workBorder ;
        Dimension fwWidth ;
        Dimension fwHeight ;
#else
CalcWorkAreaSize(
        XmFrameWidget fw,
        Dimension *workWidth,
        Dimension *workHeight,
#if NeedWidePrototypes
        int workBorder,
        int fwWidth,
        int fwHeight )
#else
        Dimension workBorder,
        Dimension fwWidth,
        Dimension fwHeight )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   Widget title = fw->frame.title_area;
   Dimension shadowThickness = fw->manager.shadow_thickness;
   Dimension titleExtent = shadowThickness;
   int temp;

   if (title && XtIsManaged(title))
      CalcTitleExtent (fw, title->core.height, title->core.border_width,
			&titleExtent, NULL, NULL, NULL);

   temp = (int) fwWidth - 
          (int) (2 * (workBorder + shadowThickness + fw->frame.margin_width));

   if (temp <= 0) *workWidth = 1;
   else *workWidth = (Dimension) temp;

   temp = (int) fwHeight - 
          (int) (2 * (workBorder + fw->frame.margin_height) +
                shadowThickness + titleExtent);

   if (temp <= 0) *workHeight = 1;
   else *workHeight = (Dimension) temp;
}




/************************************************************************
 *
 *  CalcTitleExtent
 *	Calculate layout parameters which depend on the title.
 *
 *	titleExtent:	vertical space above the work area.
 *	titleY:		y position of the title.
 *	shadowHeight:	vertical size of the shadow rectangle.
 *	shadowY:	y position of the shadow rectangle.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CalcTitleExtent( fw, titleHeight, titleBorder, titleExtent, titleY, shadowHeight, shadowY )
        XmFrameWidget fw ;
        Dimension titleHeight ;
        Dimension titleBorder ;
        Dimension *titleExtent ;
        Position *titleY ;
        Dimension *shadowHeight ;
        Position *shadowY ;
#else
CalcTitleExtent(
        XmFrameWidget fw,
#if NeedWidePrototypes
        int titleHeight,
        int titleBorder,
#else
        Dimension titleHeight,
        Dimension titleBorder,
#endif /* NeedWidePrototypes */
        Dimension *titleExtent,
        Position *titleY,
        Dimension *shadowHeight,
        Position *shadowY)
#endif /* _NO_PROTO */
{
    XmFrameConstraint fc = GetFrameConstraint(fw->frame.title_area);
    Dimension shadowThickness = fw->manager.shadow_thickness;
    Dimension extent;
    Position ty;
    Dimension sh;
    Position sy;
    Dimension base;
    Dimension *lines;
    int nlines;
    Dimension total = titleHeight + 2 * titleBorder;

    switch (fc->child_v_alignment) {
	case(XmALIGNMENT_BASELINE_TOP):
	case(XmALIGNMENT_BASELINE_BOTTOM):
	    if (XmWidgetGetBaselines (fw->frame.title_area, &lines, &nlines)) {
		if (fc->child_v_alignment == XmALIGNMENT_BASELINE_TOP)
		    base = lines[0];
		else
		    base = lines[nlines - 1];
		XtFree((char *)lines);
	    }
	    else {
		base = total/2;
	    }
	    ty = (base + titleBorder > shadowThickness/2) ?
		0 : shadowThickness/2 - (base + titleBorder);
	    extent = ty + Max(total, titleBorder + base + shadowThickness/2);
	    sy = (base + titleBorder > shadowThickness/2) ?
		base + titleBorder - shadowThickness/2 : 0;
	    sh = ((Dimension) sy >= fw->core.height) ? 
		1 : fw->core.height - sy;
	    break;
	case(XmALIGNMENT_CENTER):
	    ty = (total > shadowThickness) ? 0 : (shadowThickness - total)/2;
	    extent = Max(shadowThickness, total);
            sy = (shadowThickness > total) ? 0 : total/2 - shadowThickness/2;
	    sh = ((Dimension) sy >= fw->core.height) ? 
		1 : fw->core.height - sy;
	    break;
	case(XmALIGNMENT_WIDGET_TOP):
	    ty = 0;
	    extent = shadowThickness + total;
	    sy = total;
	    sh = ((Dimension) sy >= fw->core.height) ? 
		1 : fw->core.height - sy;
	    break;
	case(XmALIGNMENT_WIDGET_BOTTOM):
	default:
	    ty = shadowThickness;
	    extent = shadowThickness + total;
	    sy = 0;
	    sh = fw->core.height;
	    break;
    }

    if (titleExtent) *titleExtent = extent;
    if (titleY) *titleY = ty;
    if (shadowHeight) *shadowHeight = sh;
    if (shadowY) *shadowY = sy;
}



/************************************************************************
 *
 *  Constraint Initialize
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
ConstraintInitialize( rw, nw, args, num_args )
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
ConstraintInitialize(
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
   XmFrameConstraint nc;
   XmFrameConstraint bc;
   XmFrameWidget fw;

   if (!XtIsRectObj (nw)) return;

   nc = GetFrameConstraint(nw);
   fw = (XmFrameWidget)XtParent(nw);

   if (!XmRepTypeValidValue( XmRID_CHILD_TYPE, nc->child_type, nw))
   {
      nc->child_type = XmFRAME_GENERIC_CHILD;
   }
   else
   {
      switch (nc->child_type)
      {
	 case (XmFRAME_TITLE_CHILD) :
	    if (fw->frame.title_area != NULL) {
		bc = GetFrameConstraint(fw->frame.title_area);
		bc->child_type = XmFRAME_GENERIC_CHILD;
	    }
	    fw->frame.title_area = nw;
            if (nc->child_h_spacing == (Dimension) XmINVALID_DIMENSION)
                nc->child_h_spacing = Max(10,fw->frame.margin_width);
            break;
         case (XmFRAME_WORKAREA_CHILD) :
            if (fw->frame.work_area != NULL) {
                bc = GetFrameConstraint(fw->frame.work_area);
                bc->child_type = XmFRAME_GENERIC_CHILD;
	    }
            fw->frame.work_area = nw;
            break;
         case (XmFRAME_GENERIC_CHILD) :
            break;
      }
   }

   if (!XmRepTypeValidValue( XmRID_CHILD_HORIZONTAL_ALIGNMENT,
			nc->child_h_alignment, nw))
   {
      nc->child_h_alignment = XmALIGNMENT_BEGINNING;
   }

   if (!XmRepTypeValidValue( XmRID_CHILD_VERTICAL_ALIGNMENT,
			nc->child_v_alignment, nw))
   {
      nc->child_v_alignment = XmALIGNMENT_CENTER;
   }
}




/************************************************************************
 *
 *  Constraint SetValues
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
ConstraintSetValues( cw, rw, nw, args, num_args )
        Widget cw ;
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
ConstraintSetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
   XmFrameConstraint nc;
   XmFrameConstraint cc;
   XmFrameConstraint bc;
   XmFrameWidget fw;
   Boolean reconfigure = False;

   if (!XtIsRectObj (nw)) return (False);

   nc = GetFrameConstraint(nw);
   cc = GetFrameConstraint(cw);
   fw = (XmFrameWidget)XtParent(nw);

   if (nc->child_type != cc->child_type)
   {
      if(    !XmRepTypeValidValue( XmRID_CHILD_TYPE, nc->child_type, nw)    )
      {
         nc->child_type = cc->child_type;
      }
      else
      {
        switch(nc->child_type)
        {
         case (XmFRAME_TITLE_CHILD) :
	    if (fw->frame.title_area != NULL) {
		bc = GetFrameConstraint(fw->frame.title_area);
		bc->child_type = XmFRAME_GENERIC_CHILD;
	    }
	    fw->frame.title_area = nw;
            if (nc->child_h_spacing == (Dimension) XmINVALID_DIMENSION)
                nc->child_h_spacing = Max(10,fw->frame.margin_width);
            break;
         case (XmFRAME_WORKAREA_CHILD) :
            if (fw->frame.work_area != NULL) {
                bc = GetFrameConstraint(fw->frame.work_area);
                bc->child_type = XmFRAME_GENERIC_CHILD;
	    }
            fw->frame.work_area = nw;
            break;
         case (XmFRAME_GENERIC_CHILD) :
            if (nw == fw->frame.title_area)
               fw->frame.title_area = NULL;
            else if (nw == fw->frame.work_area)
               fw->frame.work_area = NULL;
            break;
        }
      }
   }

   if (nc->child_h_alignment != cc->child_h_alignment &&
      !XmRepTypeValidValue( XmRID_CHILD_HORIZONTAL_ALIGNMENT,
			nc->child_h_alignment, nw))
   {
      nc->child_h_alignment = cc->child_h_alignment;
   }
   if (nc->child_v_alignment != cc->child_v_alignment &&
      !XmRepTypeValidValue( XmRID_CHILD_VERTICAL_ALIGNMENT,
			nc->child_v_alignment, nw))
   {
      nc->child_v_alignment = cc->child_v_alignment;
   }

   if (nc->child_type == XmFRAME_TITLE_CHILD &&
      (nc->child_h_alignment != cc->child_h_alignment ||
       nc->child_h_spacing != cc->child_h_spacing ||
       nc->child_v_alignment != cc->child_v_alignment))
   {
      reconfigure = True;
   }

   if (nc->child_type != cc->child_type)
   {
      reconfigure = True;
   }

   if (reconfigure && XtIsManaged (nw) && XtIsRealized (nw))
   {
      fw->frame.processing_constraints = True;
      nw->core.border_width+=1; /* force call to GM */
      return (True);
  }

   return (False);
}




/************************************************************************
 *
 *  Constraint Destroy
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
ConstraintDestroy( w )
        Widget w ;
#else
ConstraintDestroy(
        Widget w )
#endif /* _NO_PROTO */
{
   XmFrameWidget fw;

   if (!XtIsRectObj (w)) return;

   fw = (XmFrameWidget)XtParent(w);

   if (w == fw->frame.title_area)
      fw->frame.title_area = NULL;
   else  if (w == fw->frame.work_area)
      fw->frame.work_area = NULL;
}




/************************************************************************
 *
 *  XmCreateFrame
 *	Create an instance of a frame widget and return the widget id.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateFrame( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateFrame(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmFrameWidgetClass, 
                           parent, arglist, argcount));
}
