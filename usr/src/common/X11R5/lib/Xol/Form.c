/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)form:Form.c	1.38"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Form.c
 **
 **   Description: Contains code for the X Widget's Form manager.
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   
 *****************************************************************************
 *************************************<+>*************************************/


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <values.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/keysymdef.h>   
#include <Xol/OpenLookP.h>
#include <Xol/FormP.h>

#define ClassName Form
#include <Xol/NameDefs.h>

#define Strdup(S) strcpy(XtMalloc((unsigned)_OlStrlen(S) + 1), S)

#define DftMotifShadowThickness '2'		/* see ClassInitialize	*/

static char shadow_thickness[] = "0 points";	/* OL_OPENLOOK_GUI	*/

static XtResource resources[] = {
	/* uom: pixel, see ClassInitialize for OL_MOTIF_GUI	*/
   { XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
      XtOffset(FormWidget, manager.shadow_thickness),
      XtRString, (XtPointer)shadow_thickness
   },
};

/*  Constraint resource list for Form  */

static XtResource constraintResources[] = {
   {
      XtNxRefName, XtCXRefName, XtRString, sizeof(XtPointer),
      XtOffset(FormConstraints, x_ref_name), XtRString, (XtPointer) NULL
   },
   {
      XtNxRefWidget, XtCXRefWidget, XtRPointer, sizeof(XtPointer),
      XtOffset(FormConstraints, x_ref_widget), XtRPointer, NULL
   },
   {
      XtNxOffset, XtCXOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, x_offset), XtRImmediate, (XtPointer)0
   },
   {
      XtNxAddWidth, XtCXAddWidth, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_add_width), XtRImmediate, (XtPointer)False
   },
   {
      XtNxVaryOffset, XtCXVaryOffset, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_vary_offset), XtRImmediate, (XtPointer)False
   },
   {
      XtNxResizable, XtCXResizable, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_resizable), XtRImmediate, (XtPointer)False
   },
   {
      XtNxAttachRight, XtCXAttachRight, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_attach_right), XtRImmediate, (XtPointer)False
   },
   {
      XtNxAttachOffset, XtCXAttachOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, x_attach_offset), XtRImmediate, (XtPointer)0
   },
   {
      XtNyRefName, XtCYRefName, XtRString, sizeof(XtPointer),
      XtOffset(FormConstraints, y_ref_name), XtRString, (XtPointer) NULL
   },
   {
      XtNyRefWidget, XtCYRefWidget, XtRPointer, sizeof(XtPointer),
      XtOffset(FormConstraints, y_ref_widget), XtRPointer, NULL
   },
   {
      XtNyOffset, XtCYOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, y_offset), XtRImmediate, (XtPointer)0
   },
   {
      XtNyAddHeight, XtCYAddHeight, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_add_height), XtRImmediate, (XtPointer)False
   },
   {
      XtNyVaryOffset, XtCYVaryOffset, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_vary_offset), XtRImmediate, (XtPointer)False
   },
   {
      XtNyResizable, XtCYResizable, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_resizable), XtRImmediate, (XtPointer)False
   },
   {
      XtNyAttachBottom, XtCYAttachBottom, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_attach_bottom), XtRImmediate, (XtPointer)False
   },
   {
      XtNyAttachOffset, XtCYAttachOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, y_attach_offset), XtRImmediate, (XtPointer)0
   }
};


/*  Static routine definitions  */

static void	ClassInitialize OL_NO_ARGS();

static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));

static void	Realize OL_ARGS((Widget, XtValueMask *,
				 XSetWindowAttributes *));

static void	Resize OL_ARGS((Widget));

static void	Destroy OL_ARGS((Widget));

static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,
				   ArgList, Cardinal *));

static void	ChangeManaged OL_ARGS((Widget));

static XtGeometryResult
		GeometryManager OL_ARGS((Widget, XtWidgetGeometry *,
					 XtWidgetGeometry *));

static void	ConstraintInitialize OL_ARGS((Widget, Widget,
					      ArgList, Cardinal *));

static void	ConstraintDestroy OL_ARGS((Widget));

static Boolean	ConstraintSetValues OL_ARGS((Widget, Widget, Widget,
					     ArgList, Cardinal *));

static void	GetRefWidget OL_ARGS((Widget *, String *, Widget));

static Widget	XwFindWidget OL_ARGS((FormWidget, String));

static FormRef *XwGetFormRef OL_ARGS((Widget, Widget, Dimension, Boolean,
				      Boolean, Boolean, Boolean, Dimension,
				      Position, Dimension));

static Widget	XwFindValidRef OL_ARGS((Widget, int, FormRef *));

static FormRef *XwRefTreeSearch OL_ARGS((Widget, FormRef *));

static FormRef *XwParentRefTreeSearch OL_ARGS((Widget, FormRef *, FormRef *));

static void	XwMakeRefs OL_ARGS((Widget));

static void	XwDestroyRefs OL_ARGS((Widget));

static void	XwProcessRefs OL_ARGS((FormWidget, Boolean));

static void	XwAddRef OL_ARGS((FormRef *, FormRef *));

static void	XwRemoveRef OL_ARGS((FormRef *, FormRef *));

static void	XwFindDepthAndCount OL_ARGS((FormRef *, int));

static void	XwInitProcessList OL_ARGS((FormProcess **, FormRef *, int));

static void	XwConstrainList OL_ARGS((FormProcess **, int, int, Dimension *,
					 Boolean, int));

static void	XwFreeConstraintList OL_ARGS((FormProcess **, int));

/*  Static global variable definitions  */

static int depth, leaves, arrayIndex;


/*  The Form class record */

FormClassRec
formClassRec = {
   {
    (WidgetClass) &(managerClassRec),	/* superclass		 */	
      "Form",                           /* class_name	         */	
      sizeof(FormRec),			/* widget_size	         */	
      ClassInitialize,			/* class_initialize      */    
      NULL,                             /* class_part_initialize */    
      FALSE,                            /* class_inited          */	
      Initialize,          		/* initialize	         */	
      NULL,                             /* initialize_hook       */
      Realize,          		/* realize	         */	
      NULL,	                        /* actions               */	
      0,                                /* num_actions	         */	
      resources,	                /* resources	         */	
      XtNumber(resources),		/* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      TRUE,                             /* compress_motion       */	
      TRUE,                             /* compress_exposure     */	
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */	
      Destroy,           		/* destroy               */	
      Resize,            		/* resize                */
      XtInheritExpose,                  /* expose                */	
      SetValues,      			/* set_values	         */	
      NULL,                             /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      NULL,                             /* get_values_hook       */
      NULL,                             /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      XtInheritTranslations,            /* tm_table              */
      NULL,                             /* query_geometry        */
   },

   {                                         /*  composite class           */
      GeometryManager,   		     /*  geometry_manager          */
      ChangeManaged,          		     /*  change_managed            */
      XtInheritInsertChild,		     /* insert_child	           */
      XtInheritDeleteChild,                  /*  delete_child (inherited)  */
      NULL,				     /*  extension    	           */
   },

   {                                      /*  constraint class            */
      constraintResources,		  /*  constraint resource set     */
      XtNumber(constraintResources),      /*  num_resources               */
      sizeof(FormConstraintRec),	  /*  size of the constraint data */
      ConstraintInitialize,  		  /*  contraint initilize proc    */
      ConstraintDestroy,   	          /*  contraint destroy proc      */
      ConstraintSetValues,  		  /*  contraint set values proc */
      NULL,				  /*  extension                   */
   },

   {					/* manager class	*/
      NULL,				/* highlight_handler   	*/
      True,				/* focus_on_select	*/
      NULL,				/* traversal_handler	*/
      NULL,				/* activate		*/
      NULL,				/* event_procs		*/
      0,				/* num_event_procs	*/
      NULL,				/* register_focus	*/
      OlVersion,			/* version		*/
      NULL				/* extension		*/
   },

   {              			/*  form class  */
      0					/*  mumble      */               
   }	
};


WidgetClass formWidgetClass = (WidgetClass) &formClassRec;

/************************************************************************
 *
 *  ClassInitialize -
 ************************************************************************/

static void
ClassInitialize OL_NO_ARGS()
{
	if (OlGetGui() == OL_MOTIF_GUI)
	{
		shadow_thickness[0] = DftMotifShadowThickness;
	}
} /* end of ClassInitialize */

/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/

static void
Initialize OLARGLIST((req_w, new_w, args, num_args))
	OLARG( Widget,		req_w)
	OLARG( Widget,		new_w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
   FormWidget request = (FormWidget)req_w;
   FormWidget new = (FormWidget)new_w;

   /*  Initialize the tree fields to NULL  */

   new -> form.width_tree = 
      XwGetFormRef (	(Widget)new, (Widget)NULL, (Dimension)0, 
			False, False, True, False, 
			(Dimension)0, (Position)0, (Dimension)0);   
   new -> form.height_tree =
      XwGetFormRef (	(Widget)new, (Widget)NULL, (Dimension)0, 
			False, False, True, False, 
			(Dimension)0, (Position)0, (Dimension)0);   

   /*  Set up a geometry for the widget if it is currently 0.  */

   if (request -> core.width == (Dimension)0)
      new -> core.width += (Dimension)200;
   if (request -> core.height == (Dimension)0)
      new -> core.height += (Dimension)200;
} /* end of Initialize */

/************************************************************************
 *
 *  ConstraintInitialize
 *     The main widget instance constraint initialization routine.
 *
 ************************************************************************/

static void
ConstraintInitialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
   FormConstraintRec * constraintRec;


   constraintRec = (FormConstraintRec *) new -> core.constraints;


   /*  Initialize the contraint widget sizes for later processing  */

   constraintRec -> set_x = (Position)0;
   constraintRec -> set_y = (Position)0;
   constraintRec -> set_width = (Dimension)0;
   constraintRec -> set_height = (Dimension)0;

   constraintRec -> x = new -> core.x;
   constraintRec -> y = new -> core.y;
   constraintRec -> width = new -> core.width;
   constraintRec -> height = new -> core.height;

   constraintRec -> managed = False;


   /*
    * The refWidget and refName resources will be resolved at the last
    * possible moment, to allow forward referencing of yet-to-be-created
    * peers.
    */
   if (constraintRec -> x_ref_name)
      constraintRec -> x_ref_name = Strdup(constraintRec -> x_ref_name);
   if (constraintRec -> y_ref_name)
      constraintRec -> y_ref_name = Strdup(constraintRec -> y_ref_name);
} /* end of ConstraintInitialize  */

/************************************************************************
 *
 *  GetRefWidget
 *     Get and verify the reference widget given.
 *     NOTE: it is up to the caller to free these names.
 *
 ************************************************************************/

static void
GetRefWidget OLARGLIST((widget, name, w))
	OLARG( Widget *,	widget)
	OLARG( String *,	name)
	OLGRA( Widget,		w)
{
   if (*widget != NULL)
   {
      if (*name != NULL)
      {
         if (XrmStringToQuark(*name) != (*widget) -> core.xrm_name)
         {
		OlVaDisplayWarningMsg(	XtDisplay(w),
					OleNfileForm,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileForm_msg1,
              			 	w -> core.parent -> core.name,
               				XtName(*widget),
               				*name);

            XtFree (*name);
            *name = Strdup(XtName(*widget));
         }
      }
      else
         *name = Strdup(XtName(*widget));

      if ((*widget) != w -> core.parent &&
          (*widget) -> core.parent != w -> core.parent)
      {

		OlVaDisplayWarningMsg(	XtDisplay(w),
					OleNfileForm,
					OleTmsg2,
					OleCOlToolkitWarning,
					OleMfileForm_msg2,
              			 	w -> core.parent -> core.name,
               				*name);

         XtFree (*name);
         *name = Strdup(w -> core.parent -> core.name);
         *widget = w -> core.parent;
      }
   }

   else if (*name != NULL)
   {
      if ((*widget = XwFindWidget((FormWidget)(w -> core.parent), *name)) 
				== (Widget)NULL)
      {

		OlVaDisplayWarningMsg(	XtDisplay(w),
					OleNfileForm,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileForm_msg3,
              			 	w -> core.parent -> core.name,
               				*name);
         XtFree (*name);
         *name = Strdup(w -> core.parent -> core.name);
         *widget = w -> core.parent;
      }
   }
   else
   {
      *name = Strdup(w -> core.parent -> core.name);
      *widget = w -> core.parent;
   }
} /* end of GetRefWidget */

/************************************************************************
 *
 *  XwFindWidget
 *
 ************************************************************************/

static Widget
XwFindWidget OLARGLIST((w, name))
	OLARG( FormWidget,	w)
	OLGRA( String,		name)
{
   register int i;
   register Widget * list;
   int count;

   /*
    * Compare quarks, not strings. First, it should prove a little
    * faster, second, it works for gadget children (the string version
    * of the name isn't in core for gadgets).
    */
   XrmQuark xrm_name = XrmStringToQuark(name);

   if (xrm_name == w->core.xrm_name)
      return ((Widget) w);

   list = w -> composite.children;
   count = w -> composite.num_children;

   for (i = 0; i < count; i++)
   {
      if (xrm_name == (*list) -> core.xrm_name)
         return (*list);
      list++;
   }
   return (NULL);
} /* end of XwFindWidget */

/************************************************************************
 *
 *  Realize
 *	Create the widget window and create the gc's.
 *
 ************************************************************************/

static void
Realize OLARGLIST((w, valueMask, attributes))
	OLARG( Widget,			w)
	OLARG( XtValueMask *,		valueMask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
   FormWidget fw = (FormWidget)w;
   Mask newValueMask = *valueMask;
   XtCreateWindow (w, InputOutput, (Visual *) CopyFromParent,
		   newValueMask, attributes);

   XwProcessRefs (fw, False);
} /* end of Realize */

/************************************************************************
 *
 *  Resize
 *
 ************************************************************************/
		
static void
Resize OLARGLIST((w))
	OLGRA( Widget,	w)
{
   FormWidget fw = (FormWidget)w;

   if (XtIsRealized (w)) XwProcessRefs (fw, False);
} /* end of Resize */

/************************************************************************
 *
 *  Destroy
 *	Deallocate the head structures of the reference trees.
 *	The rest of the tree has already been deallocated.
 *
 ************************************************************************/

static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
   FormWidget fw = (FormWidget)w;

   XtFree ((char *)fw -> form.width_tree);
   XtFree ((char *)fw -> form.height_tree);
} /* end of Destroy */

/************************************************************************
 *
 *  ConstraintDestroy
 *	Deallocate the allocated referenence names.
 *
 ************************************************************************/

static void
ConstraintDestroy OLARGLIST((ww))
	OLGRA( Widget,	ww)
{
   FormWidget w = (FormWidget)ww;
   FormConstraintRec * constraint;

   constraint = (FormConstraintRec *) w -> core.constraints;

   if (constraint -> x_ref_name != NULL) XtFree (constraint -> x_ref_name);
   if (constraint -> y_ref_name != NULL) XtFree (constraint -> y_ref_name);
} /* end of ConstraintDestroy */

/************************************************************************
 *
 *  SetValues
 *	Currently nothing needs to be done.  The XtSetValues call 
 *	handles geometry requests and form does not define any
 *	new resources.
 *
 ************************************************************************/

static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
   return (False);
} /* end of SetValues */

/************************************************************************
 *
 *  ConstraintSetValues
 *	Process changes in the constraint set of a widget.
 *
 ************************************************************************/

static Boolean
ConstraintSetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
   FormConstraintRec * curConstraint;
   FormConstraintRec * newConstraint;
   FormConstraintRec * tempConstraint;


   curConstraint = (FormConstraintRec *) current -> core.constraints;
   newConstraint = (FormConstraintRec *) new -> core.constraints;


   /*  Check the geometrys to see if new's contraint record  */
   /*  saved geometry data needs to be updated.              */

   if (XtIsRealized (current))
   {
      FormWidget	fw = (FormWidget)new;

      if (new -> core.x != current -> core.x)
         newConstraint -> set_x = new -> core.x;
      if (new -> core.y != current -> core.y)
         newConstraint -> set_y = new -> core.y;
      if (new -> core.width != current -> core.width)
         newConstraint -> set_width = new -> core.width;
      if (new -> core.height != current -> core.height)
         newConstraint -> set_height = new -> core.height;
   }


   /*  If the reference widget or name has changed, set the  */
   /*  opposing member to NULL in order to get the proper    */
   /*  referencing.  For names, the string space will be     */
   /*  deallocated out of current later.                     */
   /*  However, if both reference widget and reference name  */
   /*  have changed, leave them alone so that mismatches can */
   /*  be caught.                                            */

   if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget
    && newConstraint -> x_ref_name != curConstraint -> x_ref_name)
      ;
   else if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget)
      newConstraint -> x_ref_name = NULL;
   else if (newConstraint -> x_ref_name != curConstraint -> x_ref_name)
      newConstraint -> x_ref_widget = NULL;

   if (newConstraint -> x_ref_name != curConstraint -> x_ref_name)
      XtFree (curConstraint -> x_ref_name);

   if (newConstraint -> y_ref_widget != curConstraint -> y_ref_widget
    && newConstraint -> y_ref_name != curConstraint -> y_ref_name)
      ;
   else if (newConstraint -> y_ref_widget != curConstraint -> y_ref_widget)
      newConstraint -> y_ref_name = NULL;
   else if (newConstraint -> y_ref_name != curConstraint -> y_ref_name)
      newConstraint -> y_ref_widget = NULL;

   if (newConstraint -> y_ref_name != curConstraint -> y_ref_name)
      XtFree (curConstraint -> y_ref_name);


   /*  See if any constraint data for the widget has changed.  */
   /*  (note that after the above checks it is sufficient to   */
   /*  compare just the reference widgets, below).             */
   /*  Is so, remove the old reference tree elements from the  */
   /*  forms constraint processing trees and build and insert  */
   /*  new reference tree elements.                            */
   /*                                                          */
   /*  Once this is finished, reprocess the constraint trees.  */

   if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget       ||
       newConstraint -> y_ref_widget != curConstraint -> y_ref_widget       ||

       newConstraint -> x_offset != curConstraint -> x_offset               ||
       newConstraint -> y_offset != curConstraint -> y_offset               ||

       newConstraint -> x_vary_offset != curConstraint -> x_vary_offset     ||
       newConstraint -> y_vary_offset != curConstraint -> y_vary_offset     ||

       newConstraint -> x_resizable != curConstraint -> x_resizable         ||
       newConstraint -> y_resizable != curConstraint -> y_resizable         ||

       newConstraint -> x_add_width != curConstraint -> x_add_width         ||
       newConstraint -> y_add_height != curConstraint -> y_add_height       ||

       newConstraint -> x_attach_right != curConstraint -> x_attach_right   ||
       newConstraint -> y_attach_bottom != curConstraint -> y_attach_bottom ||

       newConstraint -> x_attach_offset != curConstraint -> x_attach_offset ||
       newConstraint -> y_attach_offset != curConstraint -> y_attach_offset)
   {
      if (XtIsRealized (current) && current -> core.managed)
      {
         XwDestroyRefs (current->core.self); /*KSK*/

         tempConstraint = (FormConstraintRec *) current -> core.constraints;
         current -> core.constraints = new -> core.constraints;
	 XwMakeRefs (new->core.self); /*METH*/
         current -> core.constraints = (XtPointer) tempConstraint;
      }

      if (XtIsRealized (current))
	XwProcessRefs((FormWidget)(new -> core.parent), True);
   }

   return False;
} /* end of ConstraintSetValues */

/************************************************************************
 *
 *  GeometryManager
 *      Always accept the childs new size, set the childs constraint
 *      record size to the new size and process the constraints.
 *
 ************************************************************************/

static XtGeometryResult
GeometryManager OLARGLIST((w, request, reply))
	OLARG( Widget,			w)
	OLARG( XtWidgetGeometry *,	request)
	OLGRA( XtWidgetGeometry *,	reply)
{
   FormWidget fw = (FormWidget) w -> core.parent;
   FormConstraintRec * constraint;
   FormRef * xRef;
   FormRef * yRef;
   Dimension newBorder = w -> core.border_width;
   Boolean moveFlag = False;
   Boolean resizeFlag = False;

   constraint = (FormConstraintRec *) w -> core.constraints;

   if (request -> request_mode & CWX)
      constraint -> set_x = request -> x;

   if (request -> request_mode & CWY)
      constraint -> set_y = request -> y;

   if (request -> request_mode & CWWidth) {
      constraint -> set_width = request -> width;
	}

   if (request -> request_mode & CWHeight)
      constraint -> set_height = request -> height;

   if (request -> request_mode & CWBorderWidth)
      newBorder = request -> border_width;


   /*  If the x or the width has changed, find the horizontal  */
   /*  reference tree structure for this widget and update it  */

   xRef = yRef = (FormRef *) NULL;

   if  ((request->request_mode & CWWidth) || (request->request_mode & CWX)
	|| (request->request_mode & CWBorderWidth))
   {
      if ((xRef = XwRefTreeSearch (w, fw -> form.width_tree)) !=(FormRef *)NULL)
      {
         if  (request->request_mode & CWX)
            xRef -> set_loc = request -> x;
         if  (request->request_mode & CWWidth)
            xRef -> set_size = request -> width;
      }
   }


   /*  If the y or the height has changed, find the vertical   */
   /*  reference tree structure for this widget and update it  */

   if  ((request->request_mode & CWHeight) || (request->request_mode & CWY)
	|| (request->request_mode & CWBorderWidth))
   {
      if ((yRef = XwRefTreeSearch (w, fw -> form.height_tree))!=(FormRef *)NULL)
      {
         if  (request->request_mode & CWY)
            yRef -> set_loc = request -> y;
         if  (request->request_mode & CWHeight)
            yRef -> set_size = request -> height;
      }
   }


   /*  Process the constraints if either of the ref structs have changed */

   if (xRef != NULL || yRef != NULL)
   {
      if ((request->request_mode & CWX) || (request->request_mode & CWY))
      {
         moveFlag = True;
      }
      if ((request->request_mode & CWWidth) ||
          (request->request_mode & CWHeight)
	  || (request->request_mode & CWBorderWidth))
         resizeFlag = True;

      if (moveFlag && resizeFlag)
         XtConfigureWidget (w, constraint -> set_x + fw->manager.shadow_thickness,
			    constraint -> set_y + fw->manager.shadow_thickness,
                            constraint -> set_width, constraint -> set_height,
                            newBorder);
      else if (resizeFlag) {
         XtResizeWidget (w, constraint -> set_width, constraint -> set_height,
                            newBorder);
}
      else if (moveFlag)
         XtMoveWidget (w, constraint -> set_x, constraint -> set_y);


      XwProcessRefs ((FormWidget)(w -> core.parent), True);
   }


   /*  See if an almost condition should be returned  */

   if (((request->request_mode & CWX) && w->core.x != request->x) ||
       ((request->request_mode & CWY) && w->core.y != request->y) ||
       ((request->request_mode & CWWidth) && 
         w->core.width != request->width) ||
       ((request->request_mode & CWHeight) && 
         w->core.height != request->height))
   {
      reply->request_mode = request->request_mode;

      if (request->request_mode & CWX) reply->x = w->core.x;
      if (request->request_mode & CWY) reply->y = w->core.y;
      if (request->request_mode & CWWidth) reply->width = w->core.width;
      if (request->request_mode & CWHeight) reply->height = w->core.height;
      if (request->request_mode & CWBorderWidth)
         reply->border_width = request->border_width;
      if (request->request_mode & CWSibling)
         reply->sibling = request->sibling;
      if (request->request_mode & CWStackMode)
          reply->stack_mode = request->stack_mode;

      return (XtGeometryAlmost);
   }

   return (XtGeometryDone);
} /* end of GeometryManager */

/************************************************************************
 *
 *  ChangeManaged
 *
 ************************************************************************/

static void
ChangeManaged OLARGLIST((w))
	OLGRA( Widget,	w)
{
   FormWidget fw = (FormWidget)w;
   Widget child;
   FormConstraintRec * constraint;
   register int i;


   /*  If the widget is being managed, build up the reference     */
   /*  structures for it, adjust any references, and process the  */
   /*  reference set.  If unmanaged, remove its reference.        */
   
   /*
    * Resolve refWidget and refName values, if they have not
    * yet been resolved.
    */
   for (i = 0; i < fw -> composite.num_children; i++)
   {
      child = fw -> composite.children[i];
      constraint = (FormConstraintRec *) child -> core.constraints;
      GetRefWidget (&constraint -> x_ref_widget, &constraint -> x_ref_name, child);
      GetRefWidget (&constraint -> y_ref_widget, &constraint -> y_ref_name, child);
   }

   for (i = 0; i < fw -> composite.num_children; i++)
   {
      child = fw -> composite.children[i];
      constraint = (FormConstraintRec *) child -> core.constraints;

      if (constraint -> set_width == 0)
      {
         constraint -> set_x = child -> core.x;
         constraint -> set_y = child -> core.y;
         constraint -> set_width = child -> core.width;
         constraint -> set_height = child -> core.height;
      }

      if (child -> core.managed != constraint -> managed)
      {
	 if (child -> core.managed)
         {
            if (constraint->width_when_unmanaged != child->core.width)
               constraint->set_width = child->core.width;
            if (constraint->height_when_unmanaged != child->core.height)
               constraint->set_height = child->core.height;
            XwMakeRefs (child);
         }
         else
         {
            constraint -> width_when_unmanaged = child->core.width;
            constraint -> height_when_unmanaged = child->core.height;
            XwDestroyRefs (child);
         }
         constraint -> managed = child -> core.managed;
      }
   }

   XwProcessRefs (fw, True);
} /* end of ChangeManaged */

/************************************************************************
 *
 *  XwMakeRefs
 *	Build up and insert into the forms reference trees the reference
 *      structures needed for the widget w.
 *
 ************************************************************************/

static void
XwMakeRefs OLARGLIST((w))
	OLGRA( Widget,	w)
{
   Widget xRefWidget;
   Widget yRefWidget;
   FormWidget formWidget;
   FormConstraintRec * constraint;
   FormRef * xRefParent;
   FormRef * yRefParent;
   FormRef * xRef;
   FormRef * yRef;
   FormRef * checkRef;
   register int i;


   formWidget = (FormWidget) w -> core.parent;
   constraint = (FormConstraintRec *) w -> core.constraints;


   /*  The "true" reference widget may be unmanaged, so  */
   /*  we need to back up through the reference set      */
   /*  perhaps all the way to Form.                      */
      
      xRefWidget = XwFindValidRef (constraint -> x_ref_widget, OL_HORIZONTAL, 
                                formWidget -> form.width_tree);
      yRefWidget = XwFindValidRef (constraint -> y_ref_widget, OL_VERTICAL,
                                formWidget -> form.height_tree);


   /*  Search the referencing trees for the referencing widgets  */
   /*  The constraint reference struct will be added as a child  */
   /*  of this struct.                                           */

   if (xRefWidget != NULL)
      xRefParent = XwRefTreeSearch (xRefWidget, formWidget -> form.width_tree);

   if (yRefWidget != NULL)
      yRefParent = XwRefTreeSearch (yRefWidget, formWidget->form.height_tree);
	    

   /*  Allocate, initialize, and insert the reference structures  */

   if (xRefWidget != NULL)
   {
      xRef = XwGetFormRef (w, xRefWidget, constraint->x_offset,
 		           constraint->x_add_width, constraint->x_vary_offset,
		           constraint->x_resizable, constraint->x_attach_right,
		           constraint->x_attach_offset,
  			   constraint->set_x, constraint->set_width);
      XwAddRef (xRefParent, xRef);
   }

   if (yRefWidget != NULL)
   {
      yRef = XwGetFormRef(w, yRefWidget, constraint->y_offset, 
		          constraint->y_add_height, constraint->y_vary_offset,
		          constraint->y_resizable, constraint->y_attach_bottom,
		          constraint->y_attach_offset,
			  constraint->set_y, constraint->set_height);
      XwAddRef (yRefParent, yRef);
   }


   /*  Search through the parents reference set to get any child  */
   /*  references which need to be made child references of the   */
   /*  widget just added.                                         */

   if (xRefWidget != NULL)
   {
      for (i = 0; i < xRefParent -> ref_to_count; i++)
      {
         checkRef = xRefParent -> ref_to[i];
         constraint = (FormConstraintRec *) checkRef->this->core.constraints;
   	
         if (XwFindValidRef (constraint->x_ref_widget, OL_HORIZONTAL,
                             formWidget -> form.width_tree) != xRefWidget)
         {
    	 XwRemoveRef (xRefParent, checkRef);
            checkRef -> ref = xRef -> this;
   	 XwAddRef (xRef, checkRef);
         }
      }
   }

   if (yRefWidget != NULL)
   {
      for (i = 0; i < yRefParent -> ref_to_count; i++)
      {
         checkRef = yRefParent -> ref_to[i];
         constraint = (FormConstraintRec *) checkRef->this->core.constraints;
   	
         if (XwFindValidRef (constraint->y_ref_widget, OL_VERTICAL,
                             formWidget -> form.height_tree) != yRefWidget)
         {
    	 XwRemoveRef (yRefParent, checkRef);
            checkRef -> ref = yRef -> this;
   	 XwAddRef (yRef, checkRef);
         }
      }
   }
} /* end of XwMakeRefs */

/************************************************************************
 *
 *  XwDestroyRefs
 *	Remove and deallocate the reference structures for the widget w.
 *
 ************************************************************************/

static void
XwDestroyRefs OLARGLIST((w))
	OLGRA( Widget,	w)
{
   Widget xRefWidget;
   Widget yRefWidget;
   FormWidget formWidget;
   FormRef * xRefParent;
   FormRef * yRefParent;
   FormRef * xRef;
   FormRef * yRef;
   FormRef * tempRef;
   register int i;


   formWidget = (FormWidget) w -> core.parent;


   /*  Search through the reference trees to see if the widget  */
   /*  is within the tree.                                      */

   xRefWidget = w;
   yRefWidget = w;

   xRefParent = 
      XwParentRefTreeSearch (xRefWidget, formWidget -> form.width_tree,
                                         formWidget -> form.width_tree);
   yRefParent = 
      XwParentRefTreeSearch (yRefWidget, formWidget -> form.height_tree,
                                          formWidget -> form.height_tree);


   /*  For both the width and height references, if the ref parent was  */
   /*  not null, find the reference to be removed within the parents    
   /*  list, remove this reference.  Then, for any references attached  */
   /*  to the one just removed, reparent them to the parent reference.  */

   if (xRefParent != NULL)
   {
      for (i = 0; i < xRefParent -> ref_to_count; i++)
      {
         if (xRefParent -> ref_to[i] -> this == xRefWidget)
         {
             xRef = xRefParent -> ref_to[i];
	     break;
         }
      }

      XwRemoveRef (xRefParent, xRefParent -> ref_to[i]);

      while (xRef -> ref_to_count)
      {
         tempRef = xRef -> ref_to[0];
         tempRef -> ref = xRefParent -> this;
	 XwRemoveRef (xRef, tempRef);
	 XwAddRef (xRefParent, tempRef);
      }

      XtFree ((char *)xRef);   
   }

   if (yRefParent != NULL)
   {
      for (i = 0; i < yRefParent -> ref_to_count; i++)
      {
         if (yRefParent -> ref_to[i] -> this == yRefWidget)
         {
            yRef = yRefParent -> ref_to[i];
	    break;
	 }
      }

      XwRemoveRef (yRefParent, yRef);

      while (yRef -> ref_to_count)
      {
         tempRef = yRef -> ref_to[0];
         tempRef -> ref = yRefParent -> this;
	 XwRemoveRef (yRef, tempRef);
	 XwAddRef (yRefParent, tempRef);
      }

      XtFree ((char *)yRef);   
   }
} /* end of XwDestroyRefs */

/************************************************************************
 *
 *  XwGetFormRef
 *	Allocate and initialize a form constraint referencing structure.
 *
 ************************************************************************/

static FormRef *
XwGetFormRef OLARGLIST((this, ref, offset, add, vary, resizable, attach, attach_offset, loc, size))
	OLARG( Widget,		this)
	OLARG( Widget,		ref)
	OLARG( Dimension,	offset)
	OLARG( Boolean,		add)
	OLARG( Boolean,		vary)
	OLARG( Boolean,		resizable)
	OLARG( Boolean,		attach)
	OLARG( Dimension,	attach_offset)
	OLARG( Position,	loc)
	OLGRA( Dimension,	size)
{
   FormRef * formRef;

   formRef = (FormRef *) XtMalloc (sizeof (FormRef));
   formRef -> this = this;
   formRef -> ref = ref;
   formRef -> offset = offset;
   formRef -> add = add;
   formRef -> vary = vary;
   formRef -> resizable = resizable;
   formRef -> attach = attach;
   formRef -> attach_offset = attach_offset;

   formRef -> set_loc = loc;
   formRef -> set_size = size;

   formRef -> ref_to = NULL;
   formRef -> ref_to_count = 0;

   return (formRef);
} /* end of XwGetFormRef */

/************************************************************************
 *
 *  XwFindValidRef
 *	Given an initial reference widget to be used as a constraint,
 *	find a valid (managed) reference widget.  This is done by
 *	backtracking through the widget references listed in the
 *	constraint records.  If no valid constraint is found, "form"
 *	is returned indicating that this reference should be stuck
 *	immediately under the form reference structure.
 *
 ************************************************************************/

static Widget
XwFindValidRef OLARGLIST((refWidget, refType, formRef))
	OLARG( Widget,		refWidget)
	OLARG( int,		refType)
	OLGRA( FormRef *,	formRef)
{
   FormConstraintRec * constraint;

   if (refWidget == NULL) return (NULL);
   
   while (1)
   {
      if (XwRefTreeSearch (refWidget, formRef) != NULL) return (refWidget);

      constraint = (FormConstraintRec *) refWidget -> core.constraints;

      if (refType == OL_HORIZONTAL) refWidget = constraint -> x_ref_widget;
      else refWidget = constraint -> y_ref_widget;

      if (refWidget == NULL) return (refWidget -> core.parent);
   }
} /* end of XwFindValidRef */

/************************************************************************
 *
 *  XwRefTreeSearch
 *	Search the reference tree until the widget listed is found.
 *
 ************************************************************************/

static FormRef *
XwRefTreeSearch OLARGLIST((w, formRef))
	OLARG( Widget,		w)
	OLGRA( FormRef *,	formRef)
{
   register int i;
   FormRef * tempRef;


   if (formRef == NULL) return NULL;
   if (formRef -> this == w) return (formRef);

   for (i = 0; i < formRef -> ref_to_count; i++)
   {
      tempRef = XwRefTreeSearch (w, formRef -> ref_to[i]);
      if (tempRef != (FormRef *)NULL) return (tempRef);
   }

   return (NULL);
} /* end of XwRefTreeSearch */

/************************************************************************
 *
 *  XwParentRefTreeSearch
 *	Search the reference tree until the parent reference of the 
 *      widget listed is found.
 *
 ************************************************************************/

static FormRef *
XwParentRefTreeSearch OLARGLIST((w, wFormRef, parentFormRef))
	OLARG( Widget,		w)
	OLARG( FormRef *,	wFormRef)
	OLGRA( FormRef *,	parentFormRef)
{
   register int i;
   FormRef * tempRef;

   if (parentFormRef == NULL) return (NULL);
   if (wFormRef -> this == w) return (parentFormRef);

   for (i = 0; i < wFormRef -> ref_to_count; i++)
   {
      tempRef = 
         XwParentRefTreeSearch (w, wFormRef -> ref_to[i], wFormRef);
      if (tempRef != NULL) return (tempRef);
   }

   return (NULL);
} /* end of XwParentRefTreeSearch */

/************************************************************************
 *
 *  XwAddRef
 *	Add a reference structure into a parent reference structure.
 *
 ************************************************************************/
      
static void
XwAddRef OLARGLIST((refParent, ref))
	OLARG( FormRef *,	refParent)
	OLGRA( FormRef *,	ref)
{
   refParent -> ref_to =
      (FormRef **)
	 XtRealloc ((char *) refParent -> ref_to,
                    sizeof (FormRef *) * (refParent -> ref_to_count + 1));

   refParent -> ref_to[refParent -> ref_to_count] = ref;
   refParent -> ref_to_count += 1;
} /* end of XwAddRef  */

/************************************************************************
 *
 *  XwRemoveRef
 *	Remove a reference structure from a parent reference structure.
 *
 ************************************************************************/
      
static void
XwRemoveRef OLARGLIST((refParent, ref))
	OLARG( FormRef *,	refParent)
	OLGRA( FormRef *,	ref)
{
   register int i, j;

   for (i = 0; i < refParent -> ref_to_count; i++)
   {
      if (refParent -> ref_to[i] == ref)
      {
	 for (j = i; j < refParent -> ref_to_count - 1; j++)
	    refParent -> ref_to[j] = refParent -> ref_to[j + 1];
         break;
      }
   }

   if (refParent -> ref_to_count > 1)
   {
      refParent -> ref_to =
         (FormRef **)	 XtRealloc ((char *) refParent -> ref_to,
         sizeof (FormRef *) * (refParent -> ref_to_count - 1));

   }
   else
   {
      XtFree ((char *)refParent -> ref_to);
      refParent -> ref_to = NULL;
   }

   refParent -> ref_to_count -= 1;
} /* end of XwRemoveRef  */

/************************************************************************
 *
 *  XwProcessRefs
 *	Traverse throught the form's reference trees, calculate new
 *      child sizes and locations based on the constraints and adjust
 *      the children as is calculated.  The resizable flag indicates
 *      whether the form can be resized or not.
 *
 ************************************************************************/

static void
XwProcessRefs OLARGLIST((fw, formResizable))
	OLARG( FormWidget,	fw)
	OLGRA( Boolean,		formResizable)
{
   Dimension formWidth, formHeight;
   register int i, j;

   int horDepth, horLeaves;
   int vertDepth, vertLeaves;
   FormProcess ** horProcessList;
   FormProcess ** vertProcessList;

   XtGeometryResult geometryReturn;
   Dimension replyW, replyH;

   FormConstraintRec * constraintRec;
   Widget child;
   Boolean moveFlag, resizeFlag;

   /*  Initialize the form width and height variables  */

#if 0
   if (fw -> manager.layout == XwIGNORE) formResizable = False;
#endif

   if (formResizable) formWidth = formHeight = (Dimension)0;
   else
   {
      formWidth = fw -> core.width;
      formHeight = fw -> core.height;
   }
      

   /*  Traverse the reference trees to find the depth and leaf node count  */

   leaves = 0;
   depth = 0;
   XwFindDepthAndCount (fw -> form.width_tree, 1);
   horDepth = depth;
   horLeaves = leaves;

   leaves = 0;
   depth = 0;
   XwFindDepthAndCount (fw -> form.height_tree, 1);
   vertDepth = depth;
   vertLeaves = leaves;

   if (horDepth == 0 && vertDepth == 0)
      return;

   /*  Allocate and initialize the constraint array processing structures  */

   horProcessList =
       (FormProcess **) XtMalloc (sizeof (FormProcess **) * horLeaves);
   for (i = 0; i < horLeaves; i++)
   {
      horProcessList[i] =
         (FormProcess *) XtMalloc (sizeof (FormProcess) * horDepth);

      for (j = 0; j < horDepth; j++)
         horProcessList[i][j].ref = NULL;
   }


   vertProcessList =
      (FormProcess **) XtMalloc (sizeof (FormProcess **) * vertLeaves);
   for (i = 0; i < vertLeaves; i++)
   {
      vertProcessList[i] =
         (FormProcess *) XtMalloc (sizeof (FormProcess) * vertDepth);
            
      for (j = 0; j < vertDepth; j++)
         vertProcessList[i][j].ref = NULL;
   }


   /*  Initialize the process array placing each node of the tree into    */
   /*  the array such that it is listed only once and its first children  */
   /*  listed directly next within the array.                             */

   arrayIndex = 0;
   XwInitProcessList (horProcessList, fw -> form.width_tree, 0);
   arrayIndex = 0;
   XwInitProcessList (vertProcessList, fw -> form.height_tree, 0);

   /*  Process each array such that each row of the arrays contain  */
   /*  their required sizes and locations to match the constraints  */

   XwConstrainList (horProcessList, horLeaves,
		    horDepth, &formWidth, formResizable, OL_HORIZONTAL);
   XwConstrainList (vertProcessList, vertLeaves,
		    vertDepth, &formHeight, formResizable, OL_VERTICAL);

   if (formResizable == True)
   {
	   formWidth += 2 * fw->manager.shadow_thickness;
	   formHeight += 2 * fw->manager.shadow_thickness;
   }

   /*  If the form is resizable and the form width or height returned  */
   /*  is different from the current form width or height, then make   */
   /*  a geometry request to get the new form size.  If almost is      */
   /*  returned, use these sizes and reprocess the constrain lists     */
   
	/* if less then or equal to "0" after the substraction,		*/
	/* then just use oldW and/or oldH...				*/
#define TakeOutBorderShadow(oldW, oldH)			\
	 formWidth -= 2 * fw->manager.shadow_thickness;	\
	 formHeight -= 2 * fw->manager.shadow_thickness;\
	 if ((int)formWidth <= 0)			\
	 	formWidth = oldW;			\
	 if ((int)formHeight <= 0)			\
		formHeight = oldH

   if (formResizable &&
       ((Dimension)(formWidth - 2* fw->manager.shadow_thickness) != fw->core.width ||
	(Dimension)(formHeight -2* fw->manager.shadow_thickness) != fw->core.height))
   {
	if(formWidth==(Dimension)0)
		formWidth=(Dimension)1;

	if(formHeight==(Dimension)0)
		formHeight=(Dimension)1;

      geometryReturn =
	 XtMakeResizeRequest((Widget)fw, formWidth, formHeight, &replyW, &replyH);

      if (geometryReturn == XtGeometryAlmost)
      {
         formWidth = replyW;
         formHeight = replyH;

	 XtMakeResizeRequest((Widget)fw, formWidth, formHeight, NULL, NULL);

	 TakeOutBorderShadow(replyW, replyH);

         XwConstrainList (horProcessList, horLeaves,
      		          horDepth, &formWidth, False, OL_HORIZONTAL);
         XwConstrainList (vertProcessList, vertLeaves,
		          vertDepth, &formHeight, False, OL_VERTICAL);
      }	 

      else if (geometryReturn == XtGeometryNo)
      {
         formWidth = fw -> core.width;
         formHeight = fw -> core.height;

	 TakeOutBorderShadow(fw->core.width, fw->core.height);

         XwConstrainList (horProcessList, horLeaves,
      		          horDepth, &formWidth, False, OL_HORIZONTAL);
         XwConstrainList (vertProcessList, vertLeaves,
		          vertDepth, &formHeight, False, OL_VERTICAL);
      }
   }      
#undef TakeOutBorderShadow

   /*  Process the forms child list to compare the widget sizes and  */
   /*  locations with the widgets current values and if changed,     */
   /*  reposition, resize, or reconfigure the child.                 */

   for (i = 0; i < fw -> composite.num_children; i++)
   {
      child = (Widget) fw -> composite.children[i];

      if (child -> core.managed)
      {
	 constraintRec = (FormConstraintRec *) child -> core.constraints;

         moveFlag = resizeFlag = False;

	 if ((Dimension)(constraintRec -> x + fw->manager.shadow_thickness)
			!= child -> core.x ||
	     (Dimension)(constraintRec -> y + fw->manager.shadow_thickness)
			!= child -> core.y)
	    moveFlag = True;

	 if (constraintRec -> width != child -> core.width ||
	     constraintRec -> height != child -> core.height)
	    resizeFlag = True;

         if (moveFlag && resizeFlag)
	    XtConfigureWidget (child,
			       constraintRec->x + fw->manager.shadow_thickness,
			       constraintRec->y + fw->manager.shadow_thickness,
                               constraintRec->width, constraintRec->height,
                               child -> core.border_width);
         else if (moveFlag)
	    XtMoveWidget (child,
			  constraintRec->x + fw->manager.shadow_thickness,
			  constraintRec->y + fw->manager.shadow_thickness);
         else if (resizeFlag)
	    XtResizeWidget (child, constraintRec->width,
                            constraintRec->height, child->core.border_width);
      }
   }

   XwFreeConstraintList (horProcessList, horLeaves);
   XwFreeConstraintList (vertProcessList, vertLeaves);

} /* end of XwProcessRefs */

/************************************************************************
 *
 *  XwFreeConstraintList
 *	Free an allocated constraint list.
 *
 ************************************************************************/

static void
XwFreeConstraintList OLARGLIST((processList, leaves))
	OLARG( FormProcess **,	processList)
	OLGRA( int,		leaves)
{
   register int i;


   /*  Free each array attached to the list then free the list  */

   for (i = 0; i < leaves; i++)
      XtFree ((char *)processList[i]);

   XtFree ((char *)processList);
} /* end of XwFreeConstraintList */

/************************************************************************
 *
 *  XwFindDepthAndCount
 *	Search a constraint reference tree and find the maximum depth
 *      of the tree and the number of leaves in the tree.
 *
 ************************************************************************/

static void
XwFindDepthAndCount OLARGLIST((node, nodeLevel))
	OLARG( FormRef *,	node)
	OLGRA( int,		nodeLevel)
{
   register int i;

   if (node -> ref_to == NULL) leaves++;
   else
   {
      nodeLevel++;
      if (nodeLevel > depth) depth = nodeLevel;
      for (i = 0; i < node -> ref_to_count; i++)
	 XwFindDepthAndCount (node -> ref_to[i], nodeLevel);
   }
} /* end of XwFindDepthAndCount */

/************************************************************************
 *
 *  XwInitProcessList
 *	Search a constraint reference tree and find place the ref node
 *      pointers into the list.
 *
 ************************************************************************/

static void
XwInitProcessList OLARGLIST((processList, node, nodeLevel))
	OLARG( FormProcess **,	processList)
	OLARG( FormRef      *,	node)
	OLGRA( int,		nodeLevel)
{
   register int i;

   processList[arrayIndex][nodeLevel].ref = node;

   if (node -> ref_to == NULL)
   {
      processList[arrayIndex][nodeLevel].leaf = True;
      arrayIndex++;
   }
   else
   {
      processList[arrayIndex][nodeLevel].leaf = False;
      nodeLevel++;
      for (i = 0; i < node -> ref_to_count; i++)
	 XwInitProcessList (processList, node -> ref_to[i], nodeLevel);
   }
} /* end of XwInitProcessList */

/************************************************************************
 *
 *  XwConstrainList
 *	Process each array such that each row of the arrays contain
 *	their required sizes and locations to match the constraints
 *
 ************************************************************************/

static void
XwConstrainList OLARGLIST((processList, leaves, depth, formSize, varySize, orient))
	OLARG( FormProcess **,	processList)
	OLARG( int,		leaves)
	OLARG( int,		depth)
	OLARG( Dimension *,	formSize)
	OLARG( Boolean,		varySize)
	OLGRA( int,		orient)
{
   register int i, j;
   register FormRef  * ref;
   FormConstraintRec * constraint;
   FormConstraintRec * parentConstraint;
   Dimension heldSize= (Dimension)0;
   Dimension sizeDif;
   Dimension vary, resize;
   int varyCount, resizeCount;
   Dimension varyAmount, resizeAmount;
   Dimension constantSubtract;
   Dimension addAmount, subtractAmount;
   Dimension size=(Dimension)MAXSHORT, separation=(Dimension)MAXSHORT;
   Dimension Dim0=(Dimension)0,Dim1=(Dimension)1;
   Position Pos0=(Position)0;
   FormProcess *rptr; /* temporary pointer for current [i][j] */

   for (i = 0; i < leaves; i++)		/* Process all array lines  */
   {
      processList[i][0].size = Dim0;
      processList[i][0].loc = Pos0;


      for (j = 1; j < depth; j++)	/* Process array line */
      {
	rptr = &processList[i][j];
	ref = rptr->ref;

	 if (ref != NULL)
	 {
            rptr->size = ref -> set_size;

            if (ref -> ref == ref -> this -> core.parent)
            {
	       if (ref -> offset != Dim0)
                  rptr->loc = (Position) ref -> offset;
	       else
                  rptr->loc = ref -> set_loc;
            }
	    else
	    {
	       rptr->loc = 
                  processList[i][j - 1].loc + (Position) ref->offset;
	       if (ref -> add)
		  rptr->loc += (Position) (

		(int)(processList[i][j - 1].size) +
                ((int)(rptr->ref -> this -> core.border_width) * 2));
	    }
              
	 }
         else
	 {
	    rptr->ref = processList[i - 1][j].ref;
	    rptr->loc = processList[i - 1][j].loc;
	    rptr->size = processList[i - 1][j].size;
	    rptr->leaf = processList[i - 1][j].leaf;
	 }

         if (rptr->leaf)
         {
            if (processList[i][0].size < (Dimension) 

		((int)(rptr->size) 
				+
                ((int)(rptr->ref -> this -> core.border_width) * 2)
				+
                (int) (rptr->loc) + (int) (ref -> attach_offset)))
            {
	       processList[i][0].size = (Dimension)
			((int) (rptr->size) 
				+ 
               ((int)(rptr->ref -> this -> core.border_width) * 2)
				+
                  (int)(rptr->loc) + (int) (ref -> attach_offset));
            }

            if (rptr->leaf && processList[i][0].size > heldSize)
               heldSize = processList[i][0].size;

  	    break;
         }
      }
   }


   /*  Each array line has now been processed to optimal size.  Reprocess  */
   /*  each line to constrain it to formSize if not varySize or to         */
   /*  heldSize if varySize.                                               */

   if (varySize)
      *formSize = heldSize;


      
   for (i = 0; i < leaves; i++)
   {

      /*  For each array line if the 0th size (calculated form size needed  */
      /*  for this array line is less than the form size then increase the  */
      /*  seperations between widgets whose constaints allow it.            */

      if (processList[i][0].size < *formSize)
      {
	 sizeDif = *formSize - processList[i][0].size;
	 
         varyCount = 0;
         if (processList[i][1].leaf != True)
         {
            for (j = 2; j < depth; j++)	/*  Can't vary the first spacing  */
            {
               if (processList[i][j].ref -> vary) varyCount++;
               if (processList[i][j].leaf) break;
   	    }
         }
	       
         addAmount = Dim0;
         if (varyCount == 0) varyAmount = Dim0;
         else varyAmount = (Dimension)(((int)sizeDif) / varyCount);

         j = 1;

         while (j < depth)
         {
            if (j > 1 && processList[i][j].ref -> vary)
	       addAmount += varyAmount;
            processList[i][j].loc += (Position) addAmount;

            if (processList[i][j].leaf) break;

            j++;
	 }

         if (j > 1)
         {
		
	rptr= &processList[i][j];

            if (rptr->ref -> vary   &&  rptr->ref -> attach)
               rptr->loc = (Position) (*formSize - rptr->size -
                                       rptr->ref -> attach_offset);
            else if (rptr->ref -> vary == False &&
                     rptr->ref -> resizable     &&
                     rptr->ref -> attach)
            {
               rptr->size = *formSize
		- ((Dimension) rptr->loc
		+ (Dimension) ((int)(rptr->ref-> this-> core.border_width)*2)
		+ rptr->ref -> attach_offset);
            }
         } /* end of if(j>1) */
         else
         {
   	rptr= &processList[i][j];
            if (rptr->ref -> vary == False &&
                rptr->ref -> resizable     &&
                rptr->ref -> attach)
            {
               rptr->loc = (Position) rptr->ref -> offset;
               rptr->size = *formSize 
		- ((Dimension) rptr->loc 
		+ (Dimension) ((int)(rptr->ref->this->core.border_width) * 2)
		+ rptr->ref -> attach_offset);

            }
            else if (rptr->ref -> vary &&
                     rptr->ref -> attach)
            {
               rptr->loc =  (Position) (*formSize 
		- (rptr->size
		+ (Dimension)((int)(rptr->ref->this-> core.border_width)*2)
		+ rptr->ref -> attach_offset));
            }
            else if (rptr->ref -> vary &&
                     rptr->ref -> attach == False)
            {
               rptr->loc = (Position) rptr->ref -> offset;
            }
         } /* end of !if(j>1) */
      }

      /*  If the form size has gotten smaller, process the vary constraints */
      /*  until the needed size is correct or all seperations are 1 pixel.  */
      /*  If separations go to 1, then process the resizable widgets        */
      /*  until the needed size is correct or the sizes have gone to 1      */
      /*  pixel.  If the size is still not correct punt, cannot find a      */
      /*  usable size so clip it.                                           */

      if (processList[i][0].size > *formSize)
      {
	 sizeDif = processList[i][0].size - *formSize;
	 
         varyAmount = Dim0;
         varyCount = 0;

         j = 0;
         do
         {
            j++;

            if (j > 1 && processList[i][j].ref -> vary && 
                processList[i][j].ref -> offset)
            {
               varyAmount += processList[i][j].ref -> offset;
   	       varyCount++;
            }
  	 }
         while (processList[i][j].leaf == False);


	 resizeAmount = Dim0;
	 resizeCount = 0;
         for (j = 1; j < depth; j++)
         {
            if (processList[i][j].ref->resizable && processList[i][j].size 
				> Dim1)
	    {
               if (processList[i][j].leaf || processList[i][j+1].ref->add)
               {
                  resizeCount++;
                  resizeAmount += 
			(Dimension)((int)(processList[i][j].size) - 1);
               }
            }
            if (processList[i][j].leaf) break;
	 }


         /*  Do we have enough varience to match the constraints?  */

         if (( (Dimension) (varyAmount + resizeAmount)) > sizeDif) 
         {

            /*  first process out the vary amount  */

            if (varyCount)
            {	    
   	       do
               {
                  subtractAmount = Dim0;

                  for (j = 1; j < depth; j++)
                  {
                     if (j > 1 && processList[i][j].ref -> vary)
		     {
                        vary = (Dimension) 
                           (processList[i][j].loc - processList[i][j - 1].loc);

                        if (processList[i][j].ref -> add)
			   vary = (Dimension) (

				  (int)(vary - processList[i][j - 1].size) 
				- (1 
				+ (int)
		(processList[i][j-1].ref->this->core.border_width)*2));
                     }
                     else vary = Dim0;

                     if (vary > Dim0) subtractAmount++;

                     if (subtractAmount)
                     {
                        processList[i][j].loc -= (Position) subtractAmount;
                        sizeDif--;
                     }

                     if (processList[i][j].leaf) break;
		  }
               }
               while (subtractAmount != Dim0 && sizeDif > Dim0);
	    }

            /*  now process resize constraints if further constraint */
	    /*  processing is necessary.                             */
            if (sizeDif)
	    {
               if (resizeAmount > sizeDif) resizeAmount = sizeDif;
	       if (resizeCount) constantSubtract = (Dimension)
			(((int)(resizeAmount)) / resizeCount);
   	       while (resizeAmount > Dim0)
               {
                  subtractAmount = Dim0;

                  for (j = 1; j < depth; j++)
                  {
                     if (processList[i][j].ref -> add)
                        processList[i][j].loc -= (Position) subtractAmount;

                     if (processList[i][j].ref -> resizable)
                     {
                        if (processList[i][j].leaf || 
                            processList[i][j + 1].ref -> add)
                           resize = (Dimension)
				((int)(processList[i][j].size) - 1);
                        else
                           resize = Dim0;
		     
                        if (resize > Dim1)
                        {
                           if (constantSubtract < resize)
			      resize = constantSubtract;
                           subtractAmount += resize;
                           processList[i][j].size -= resize;
			}
		     }

                     if (processList[i][j].leaf) break;
		  }

                  {
		      Dimension old_resize_amount = resizeAmount;

		      resizeAmount -= subtractAmount;
		      if (resizeAmount >= old_resize_amount)
			  /* emergency break so we don't loop forever */
			  break;
		  }
                  constantSubtract = Dim1;
	       }
	    }
	 }
      }
   } /* end of i loop */



   /*  Now each array line is processed such that its line is properly  */
   /*  constrained to match the specified form size.  Since a single    */
   /*  widget reference structure can be referenced in multiple array   */
   /*  lines, the minumum constraint for each widget needs to be found. */
   /*  When found, the width and height will be placed into the widgets */
   /*  constraint structure.                                            */

   for (i = 1; i < depth; i++)
   {
      ref = NULL;

      for (j = 0; j < leaves + 1; j++)    /*  loop one too many - for exit */
      {
	rptr= &processList[j][i];

	 if (j == leaves || ref != rptr->ref)
	 {
            if (j == leaves || ref != NULL)
	    {
               if (ref != NULL)
               {
                  constraint = 
                     (FormConstraintRec *) ref -> this -> core.constraints;
                  parentConstraint =
                     (FormConstraintRec *) ref -> ref -> core.constraints;

                  if (orient == OL_HORIZONTAL) constraint -> width = size;
                  else constraint -> height = size;

/************************* CHECK THIS CODE  *********************/

  	          if (i > 1)
                  {
		     if (orient== OL_HORIZONTAL)
		       /* POSSIBLE BUGGGGG: Don't understand why */
		       /* this is done, will overflow and this */
		       /* value is never used. */
   		        constraint -> x = 
				parentConstraint -> x + (Position)separation;
                     else
		       /* POSSIBLE BUGGGGG: Don't understand why */
		       /* this is done, will overflow and this */
		       /* value is never used. */
		        constraint -> y = 
				parentConstraint -> y + (Position)separation;
	          }
	          else
	          {
		     if (orient == OL_HORIZONTAL) 
				constraint -> x = (Position) separation;
		     else 
				constraint -> y = (Position) separation;
	          }
/************************ CHECK THIS CODE */
               }

               if (j == leaves) break;	/* exit out of the inner loop  */
	    }
	    
            ref = rptr->ref;
            separation = (Dimension) MAXSHORT;
            size = (Dimension) MAXSHORT;
	 }

         if (ref != NULL)
         {
            if (size > rptr->size) size = rptr->size;
	 
            if (i > 1)
            {
               if (separation > (Dimension)(rptr->loc-processList[j][i-1].loc))
                  separation = (Dimension) rptr->loc-processList[j][i-1].loc;
            }
            else
               if (separation > (Dimension)rptr->loc)
	           separation = (Dimension)rptr->loc;
         }
      } /* end j loop */
   } /* end i loop */
} /* end of XwConstrainList */
