#pragma ident	"@(#)m1.2libs:Xm/ResInd.c	1.3"
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

#include <string.h>
#include <Xm/VendorSP.h>
#include <Xm/VendorSEP.h>
#include <Xm/AtomMgr.h>
#include <Xm/ManagerP.h>
#include <Xm/GadgetP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ExtObjectP.h>
#include <Xm/ScreenP.h>

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void GetValuesHook() ;
static void ConstraintGetValuesHook() ;
static void ImportArgs() ;
static void ImportConstraintArgs() ;
static void FromPixels();
static XmImportOperator ToPixels();
#else

static void GetValuesHook( 
                        Widget w,
                        XtPointer base,
                        XmSyntheticResource *resources,
                        int num_resources,
                        ArgList args,
                        Cardinal num_args) ;
static void ConstraintGetValuesHook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
static void ImportArgs( 
                        Widget w,
                        XtPointer base,
                        XmSyntheticResource *resources,
                        int num_resources,
                        ArgList args,
                        Cardinal num_args) ;
static void ImportConstraintArgs( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;

static void FromPixels(
		       Widget widget,
		       int offset,
		       XtArgVal *value,
		       unsigned char orientation);
static XmImportOperator ToPixels(
				 Widget widget,
				 int offset,
				 XtArgVal *value,
				 unsigned char orientation);
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/




/**********************************************************************
 *
 *  _XmBuildResources
 *	Build up a new synthetic resource list based on a combination
 *	the the widget's class and super class resource list.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmBuildResources( wc_resources_ptr, wc_num_resources_ptr,
                                               sc_resources, sc_num_resources )
        XmSyntheticResource **wc_resources_ptr ;
        int *wc_num_resources_ptr ;
        XmSyntheticResource *sc_resources ;
        int sc_num_resources ;
#else
_XmBuildResources(
        XmSyntheticResource **wc_resources_ptr,
        int *wc_num_resources_ptr,
        XmSyntheticResource *sc_resources,
        int sc_num_resources )
#endif /* _NO_PROTO */
{
	XmSyntheticResource * wc_resources;
	int                  wc_num_resources;
	XmSyntheticResource * new_resources;
	int                  new_num_resources;
	register int i, j;
	Boolean override;


	wc_resources = (XmSyntheticResource *) *wc_resources_ptr;
	wc_num_resources = (int) *wc_num_resources_ptr;


	/*  If there are no new resources, just use the super class data  */

	if (wc_num_resources == 0)
	{
		*wc_resources_ptr = sc_resources;
		*wc_num_resources_ptr = sc_num_resources;
		return;
	}


	/*
	 * Allocate a new resource list to contain the combined set of
	 * resources from the class and super class.  This allocation
	 * may create to much space if there are overrides in the new
	 * resource list.  Copy sc's resources into the space.
	 */

	new_resources = (XmSyntheticResource *) 
		XtMalloc(sizeof (XmSyntheticResource) *
		(wc_num_resources + sc_num_resources));
	memcpy ((char *) new_resources, (char *) sc_resources,
		sc_num_resources * sizeof (XmSyntheticResource));


	/*
	 * Loop through the wc resources and copy 
	 * them into the new resources
	 */

	new_num_resources = sc_num_resources;

	for (i = 0; i < wc_num_resources; i++)
	{

		/*  First check to see if this is an override  */

		override = False;
		for (j = 0; j < sc_num_resources; j++)
		{
			/* ???
			 * We do name overrides while the
			 * intrinsics does offset overrides.
			 */
			if (new_resources[j].resource_name == 
				wc_resources[i].resource_name)
			{
				override = True;
				new_resources[j].export_proc = 
					wc_resources[i].export_proc;
				new_resources[j].import_proc = 
					wc_resources[i].import_proc;
				break;
			}
		}


		/*  If it wasn't an override stuff it into the list  */

		if (override == False)
			new_resources[new_num_resources++] = wc_resources[i];
	}


	/*  Replace the resource list and count will the new one.  */

	*wc_resources_ptr = new_resources;
	*wc_num_resources_ptr = new_num_resources;
}


/**********************************************************************
 *
 *  InitializeSyntheticResources
 *	Initialize a synthetic resource list.  This is called befor
 *	Primitive, Manager and Gadgets build resources to convert the
 *	resource names to quarks in the class' synthetic processing
 *	lists.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmInitializeSyntheticResources( resources, num_resources )
        XmSyntheticResource *resources ;
        int num_resources ;
#else
_XmInitializeSyntheticResources(
        XmSyntheticResource *resources,
        int num_resources )
#endif /* _NO_PROTO */
{
	register int i;

	for (i = 0; i < num_resources; i++)
		resources[i].resource_name = 
			(String) XrmStringToQuark (resources[i].resource_name);
}

/**********************************************************************
 *
 *  GetValuesHook
 *	This procedure is used as the synthetic hook in Primitive,
 *	Manager, and Gadget.  It uses the synthetic resource list
 *	attached to the class, comparing it to the input resource list,
 *	and for each match, calling the function to process the get
 *	value data.
 *
 **********************************************************************/
static void 
#ifdef _NO_PROTO
GetValuesHook( w, base, resources, num_resources, args, num_args )
        Widget w ;
        XtPointer base ;
        XmSyntheticResource *resources ;
        int num_resources ;
        ArgList args ;
        Cardinal num_args ;
#else
GetValuesHook(
        Widget w,
        XtPointer base,
        XmSyntheticResource *resources,
        int num_resources,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{
    register int i, j;
    register XrmQuark quark;
    XtArgVal value;
    XtArgVal orig_value;
    int value_size;
    XtPointer value_ptr;
    
    
    /*  Loop through each argument, quarkifing the name.  Then loop  */
    /*  through each synthetic resource to see if there is a match.  */
    
    for (i = 0; i < num_args; i++) {
	quark = XrmStringToQuark (args[i].name);
	
	for (j = 0; j < num_resources; j++) {
	    if ((resources[j].export_proc) &&
		(XrmQuark)(resources[j].resource_name) == quark) {
		value_size = resources[j].resource_size;
		value_ptr = (XtPointer) ((char *) base + 
					 resources[j].resource_offset);
		
    /* Note to fixers: on all 32-bit platforms, and at least one 64-bit,    */
    /* this set of tests (and the similar code below and in ImportArgs)     */
    /* contains some redundancy.  We chose to allow this rather than invent */
    /* a tangle of machine-dependent ifdefs.  Also note: it is possible to  */
    /* imagine an architecture where sizeof(int) == sizeof(long), but a     */
    /* long assignment does not accomplish an int assignment; however, we   */
    /* know of no such architecture.                                        */
		
		if (value_size == sizeof(long))
		    value = (XtArgVal)(*(long *)value_ptr);
		else if (value_size == sizeof(int))
		    value = (XtArgVal)(*(int *)value_ptr);
		else if (value_size == sizeof(short))
		    value = (XtArgVal)(*(short *)value_ptr);
		else if (value_size == sizeof(char))
		    value = (XtArgVal)(*(char *)value_ptr);
		else 
                    value = *(XtArgVal*)value_ptr;

		orig_value = value;
		
		(*(resources[j].export_proc))(w, 
				resources[j].resource_offset, &value);
		
		if ((orig_value == args[i].value) ||
		    (!XtIsRectObj(w) && !XmIsExtObject(w)))
		    {
			args[i].value = value;
		    }
		else
		    {
			value_ptr = (XtPointer) args[i].value;
			
			if (value_size == sizeof(long))
			    *(long *) value_ptr = (long) value;
			else if (value_size == sizeof(int))
			    *(int *) value_ptr = (int) value;
			else if (value_size == sizeof(short))
			    *(short *) value_ptr = (short) value;
			else if (value_size == sizeof(char))
			    *(char *) value_ptr = (char) value;
			else 
                            *(XtArgVal*) value_ptr= value;

		    }
		
		break;
	    }
	}
    }
}




/**********************************************************************
 *
 *  ConstraintGetValuesHook
 *	When a widget is a child of a constraint manager, this function
 *	processes the synthetic arg list with for any constraint based
 *	resource processing that needs to be done.
 *
 **********************************************************************/
static void 
#ifdef _NO_PROTO
ConstraintGetValuesHook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
ConstraintGetValuesHook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmManagerWidgetClass parent_wc = 
		(XmManagerWidgetClass) w->core.parent->core.widget_class;
	
	if (!XmIsManager(w->core.parent)) return;
	
	if (parent_wc->manager_class.num_syn_constraint_resources == 0)
		return;
	
	GetValuesHook (w, w->core.constraints,
		parent_wc->manager_class.syn_constraint_resources,
		parent_wc->manager_class.num_syn_constraint_resources,
		args, *num_args);
}




/**********************************************************************
 *
 *  _XmPrimitiveGetValuesHook
 *	Process the synthetic resources that need to be synthesized
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmPrimitiveGetValuesHook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmPrimitiveGetValuesHook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmPrimitiveWidgetClass wc = 
	(XmPrimitiveWidgetClass) w->core.widget_class;
	
    if (wc->primitive_class.num_syn_resources != 0)
	GetValuesHook (w, (XtPointer)w, wc->primitive_class.syn_resources,
		       wc->primitive_class.num_syn_resources, args, *num_args);
	
    if (w->core.constraints != NULL) 
	ConstraintGetValuesHook (w, args, num_args);
}




/**********************************************************************
 *
 *  _XmGadgetGetValuesHook
 *	Process the synthetic resources that need to be synthesized
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmGadgetGetValuesHook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmGadgetGetValuesHook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmGadgetClass wc = (XmGadgetClass) w->core.widget_class;
	
	if (wc->gadget_class.num_syn_resources != 0)
		GetValuesHook (w, (XtPointer) w, wc->gadget_class.syn_resources,
			wc->gadget_class.num_syn_resources, args, *num_args);
	
	if (w->core.constraints != NULL)
		ConstraintGetValuesHook (w, args, num_args);
}




/**********************************************************************
 *
 *  _XmManagerGetValuesHook
 *	Process the synthetic resources that need to be synthesized
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmManagerGetValuesHook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmManagerGetValuesHook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmManagerWidgetClass wc =
		(XmManagerWidgetClass) w->core.widget_class;
	
	if (wc->manager_class.num_syn_resources != 0)
		GetValuesHook (w, (XtPointer) w, wc->manager_class.syn_resources,
		wc->manager_class.num_syn_resources, args, *num_args);
	
	if (w->core.constraints != NULL)
		ConstraintGetValuesHook (w, args, num_args);
}


/**********************************************************************
 *
 *  _XmExtGetValuesHook
 *	Process the synthetic resources that need to be synthesized
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmExtGetValuesHook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmExtGetValuesHook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmExtObjectClass	 		wc;

    wc = (XmExtObjectClass)XtClass(w);
    
    if (wc->ext_class.num_syn_resources != 0)
      GetValuesHook(w, (XtPointer) w, 
		    wc->ext_class.syn_resources,
		    wc->ext_class.num_syn_resources, 
		    args, *num_args);
}



/**********************************************************************
 *
 * ImportArgs
 * Convert the value in the arg list from the application type to the 
 * appropriate internal representation by calling the import_proc 
 * specified for the given resource.
 *
 **********************************************************************/
static void 
#ifdef _NO_PROTO
ImportArgs( w, base, resources, num_resources, args, num_args )
        Widget w ;
        XtPointer base ;
        XmSyntheticResource *resources ;
        int num_resources ;
        ArgList args ;
        Cardinal num_args ;
#else
ImportArgs(
        Widget w,
        XtPointer base,
        XmSyntheticResource *resources,
        int num_resources,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{
    register int i, j;
    register XrmQuark quark;
    XtArgVal value;
    int value_size;
    XtPointer value_ptr;
    
    
    /*  Loop through each argument, quarkifing the name.  Then loop  */
    /*  through each synthetic resource to see if there is a match.  */
    
    for (i = 0; i < num_args; i++) {
	quark = XrmStringToQuark (args[i].name);
	
	for (j = 0; j < num_resources; j++) {
	    if ((resources[j].import_proc) &&
		(XrmQuark)(resources[j].resource_name) == quark) {
		value = args[i].value;
		
		if (((*(resources[j].import_proc))(w, 
				resources[j].resource_offset, &value) ==
		     XmSYNTHETIC_LOAD) &&
		    (base != NULL)) {
		    value_size = resources[j].resource_size;
		    value_ptr = (XtPointer) ((char *) base
					     + resources[j].resource_offset);
		    
		    /* Load the converted value into the structure */
		    
		    if (value_size == sizeof(long))
			*(long *) value_ptr = (long) value;
		    else if (value_size == sizeof(int))
			*(int *) value_ptr = (int) value;
		    else if (value_size == sizeof(short))
			*(short *) value_ptr = (short) value;
		    else if (value_size == sizeof(char))
			*(char *) value_ptr = (char) value;
		    else
			*(XtArgVal*) value_ptr= value;
		}
		else
		    args[i].value = value;
		
		break;
	    }
	}
    }
}



/**********************************************************************
 *
 * _XmExtImportArgs
 * Does arg importing for sub-classes of VendorExt.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmExtImportArgs( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmExtImportArgs(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmExtObjectClass	 		wc;

    wc = (XmExtObjectClass)XtClass(w);
    
    if (wc->ext_class.num_syn_resources != 0)
      ImportArgs (w, (XtPointer) w, 
		  wc->ext_class.syn_resources,
		  wc->ext_class.num_syn_resources, 
		  args, *num_args);
}




/**********************************************************************
 *
 *  ImportConstraintArgs
 *	When a widget is a child of a constraint manager, this function
 *	processes the synthetic arg list with for any constraint based
 *	resource processing that needs to be done.
 *
 **********************************************************************/
static void 
#ifdef _NO_PROTO
ImportConstraintArgs( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
ImportConstraintArgs(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmManagerWidgetClass parent_wc = 
		(XmManagerWidgetClass) w->core.parent->core.widget_class;
	
	if (!XmIsManager(w->core.parent)) return;
	
	if (parent_wc->manager_class.num_syn_constraint_resources == 0)
		return;
	
	ImportArgs (w, w->core.constraints,
		parent_wc->manager_class.syn_constraint_resources,
		parent_wc->manager_class.num_syn_constraint_resources,
		args, *num_args);
}




/**********************************************************************
 *
 * _XmPrimitiveImportArgs
 * Does arg importing for sub-classes of XmPrimitive.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmPrimitiveImportArgs( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmPrimitiveImportArgs(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmPrimitiveWidgetClass wc =
		(XmPrimitiveWidgetClass) w->core.widget_class;
	
	if (wc->primitive_class.num_syn_resources != 0)
		ImportArgs (w, (XtPointer) w, wc->primitive_class.syn_resources,
			wc->primitive_class.num_syn_resources, args, *num_args);
	
	if (w->core.constraints != NULL) 
		ImportConstraintArgs (w, args, num_args);
}



/**********************************************************************
 *
 *  _XmGadgetImportArgs
 * Does arg importing for sub-classes of XmGadget.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmGadgetImportArgs( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmGadgetImportArgs(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmGadgetClass wc = (XmGadgetClass) w->core.widget_class;
	
	/* Main object args */
	if (wc->gadget_class.num_syn_resources != 0)
		ImportArgs (w, (XtPointer) w, wc->gadget_class.syn_resources,
			wc->gadget_class.num_syn_resources, args, *num_args);
	
	if (w->core.constraints != NULL)
		ImportConstraintArgs (w, args, num_args);
}


/**********************************************************************
 *
 *  _XmGadgetImportSecondaryArgs
 * Does arg importing for sub-classes of XmGadget which have secondary
 * objects.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmGadgetImportSecondaryArgs( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmGadgetImportSecondaryArgs(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmGadgetClass wc = (XmGadgetClass) w->core.widget_class;
	XmBaseClassExt *classExtPtr;
	XmExtClassRec *secondaryObjClass;

	/* Secondary object args */
	classExtPtr = _XmGetBaseClassExtPtr(wc, XmQmotif);
	secondaryObjClass = (XmExtClassRec *) 
		((*classExtPtr)->secondaryObjectClass);

	if ((secondaryObjClass != NULL) &&
		(secondaryObjClass->ext_class.num_syn_resources != 0))
		ImportArgs (w, NULL,
			secondaryObjClass->ext_class.syn_resources,
			secondaryObjClass->ext_class.num_syn_resources,
			args, *num_args);
}




/**********************************************************************
 *
 *  _XmManagerImportArgs
 * Does arg importing for sub-classes of XmManager.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmManagerImportArgs( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmManagerImportArgs(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmManagerWidgetClass wc = 
		(XmManagerWidgetClass) w->core.widget_class;
	
	if (wc->manager_class.num_syn_resources != 0)
		ImportArgs (w, (XtPointer) w, wc->manager_class.syn_resources,
			wc->manager_class.num_syn_resources, args, *num_args);
	
	if (w->core.constraints != NULL)
		ImportConstraintArgs (w, args, num_args);
}



/**********************************************************************
 * Here begins the resolution conversion stuff, which is independent
 *   of the synthetic resource stuff. We need to create a new module.
 **********************************************************************/



/**********************************************************************
 *
 * _XmConvertUnits
 * Does the real work of conversion.
 * 
 **********************************************************************/
int 
#ifdef _NO_PROTO
_XmConvertUnits( screen, dimension, from_type, from_val, to_type )
        Screen *screen ;
        int dimension ;
        register int from_type ;
        register int from_val ;
        register int to_type ;
#else
_XmConvertUnits(
        Screen *screen,
        int dimension,
        register int from_type,
        register int from_val,
        register int to_type )
#endif /* _NO_PROTO */
{
    register int from_val_in_mm = 0;
    register int mm_per_pixel;
    int font_unit;
    
    
    /*  Do error checking  */
    
    if (dimension != XmHORIZONTAL && dimension != XmVERTICAL)
	return (0);
    
    if ((from_type != XmPIXELS) && 
	(from_type != Xm100TH_MILLIMETERS) &&
	(from_type != Xm1000TH_INCHES) && 
	(from_type != Xm100TH_POINTS) &&
	(from_type != Xm100TH_FONT_UNITS))
	return (0);
    
    if ((to_type != XmPIXELS) && 
	(to_type != Xm100TH_MILLIMETERS) &&
	(to_type != Xm1000TH_INCHES) && 
	(to_type != Xm100TH_POINTS) &&
	(to_type != Xm100TH_FONT_UNITS))
	return (0);
    
    /*  Check for type to same type conversions  */
    
    if (from_type == to_type)
	return (from_val);
    
    /*  Get the screen dimensional data  */
    
    if (dimension == XmHORIZONTAL)
	mm_per_pixel = 
	    (WidthMMOfScreen(screen) * 100) / WidthOfScreen(screen);
    else
	mm_per_pixel = 
	    (HeightMMOfScreen(screen) * 100) / HeightOfScreen(screen);
    
    if (from_type == XmPIXELS)
	from_val_in_mm = from_val * mm_per_pixel;
    else if (from_type == Xm100TH_POINTS)
	from_val_in_mm = (from_val * 353) / 1000;
    else if (from_type == Xm1000TH_INCHES)
	from_val_in_mm = (from_val * 254) / 100;
    else if (from_type == Xm100TH_MILLIMETERS)
	from_val_in_mm = from_val;
    else if (from_type == Xm100TH_FONT_UNITS)
	{
	    font_unit = _XmGetFontUnit (screen, dimension);
	    from_val_in_mm = from_val * font_unit * mm_per_pixel / 100;
	}
    
    
    if (to_type == XmPIXELS)
	return (from_val_in_mm / mm_per_pixel);
    else if (to_type == Xm100TH_POINTS)
	return ((from_val_in_mm * 1000) / 353);
    else if (to_type == Xm1000TH_INCHES)
	return ((from_val_in_mm * 100) / 254);
    else if (to_type == Xm100TH_MILLIMETERS)
	return (from_val_in_mm);
    else  /*  from_type is Xm100TH_FONT_UNITS  */
	{
	    font_unit = _XmGetFontUnit (screen, dimension);
	    return ((from_val_in_mm * 100) / (mm_per_pixel * font_unit));
	}
}




/**********************************************************************
 *
 *  XmConvertUnits
 *	Convert a value in from_type representation to a value in
 *	to_type representation using the screen to look up the screen
 *	resolution and the dimension to denote whether to use the
 *	horizontal or vertical resolution data.
 *
 **********************************************************************/
int 
#ifdef _NO_PROTO
XmConvertUnits( widget, dimension, from_type, from_val, to_type )
        Widget widget ;
        int dimension ;
        register int from_type ;
        register int from_val ;
        register int to_type ;
#else
XmConvertUnits(
        Widget widget,
        int dimension,
        register int from_type,
        register int from_val,
        register int to_type )
#endif /* _NO_PROTO */
{
	
	Screen *screen = XtScreen(widget);
	
	/*  Do error checking  */
	
	if (screen == NULL)
		return (0);
	if (dimension != XmHORIZONTAL && dimension != XmVERTICAL)
		return (0);
	
	return(_XmConvertUnits(screen, dimension,
		from_type, from_val, to_type));
}



/*********************************************************************
 *
 *  XmCvtToVerticalPixels
 *      Convert from a specified unit type to pixel type using
 *      the vertical resolution of the screen.
 *
 *********************************************************************/
int 
#ifdef _NO_PROTO
XmCvtToHorizontalPixels( screen, from_val, from_type )
        Screen *screen ;
        register int from_val ;
        register int from_type ;
#else
XmCvtToHorizontalPixels(
        Screen *screen,
        register int from_val,
        register int from_type )
#endif /* _NO_PROTO */
{
	 /*  Do error checking  */
	 if (screen == NULL)
		return (0);
	
	return(_XmConvertUnits(screen, XmHORIZONTAL,
		from_type, from_val, XmPIXELS));
}



/**********************************************************************
 *
 *  ToPixels
 *	Convert from a non-pixel unit type to pixels using the 
 *	horizontal orientation/resolution of the screen.
 *
 **********************************************************************/
static XmImportOperator 
#ifdef _NO_PROTO
ToPixels( widget, offset, value, orientation )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
        unsigned char orientation ;
#else
ToPixels(
        Widget widget,
        int offset,
        XtArgVal *value,
	unsigned char orientation )
#endif /* _NO_PROTO */
{
    int mm_per_pixel;
    Screen * screen = XtScreen (widget);
    register unsigned char unit_type;
    register int value_in_mm;
    int font_unit;

    
    /*  Get the unit type of the widget  */
    
    unit_type = _XmGetUnitType(widget) ;
    
    /*  Check for type to same type conversions  */
    
    if (unit_type == XmPIXELS) return XmSYNTHETIC_LOAD;
    
    /*  Pre-process FontUnit types  */
    
    if (unit_type == Xm100TH_FONT_UNITS)
	{
	    font_unit = _XmGetFontUnit (screen, orientation);
	    *value = (XtArgVal) (*((int*)value) * font_unit / 100);
	    return XmSYNTHETIC_LOAD;
	}
    
    
    /*  Get the screen dimensional data  */
    
    if (orientation == XmHORIZONTAL) {
	mm_per_pixel = 
	    (WidthMMOfScreen(screen) * 100) / WidthOfScreen(screen);
    } else {
	mm_per_pixel = 
	    (HeightMMOfScreen(screen) * 100) / HeightOfScreen(screen);
    }

    
    /*  Convert the data  */
    
    if (unit_type == Xm100TH_POINTS)
	value_in_mm = (*((int *)value) * 353) / 1000;
    else if (unit_type == Xm1000TH_INCHES)
	value_in_mm = (*((int *)value) * 254) / 100;
    else if (unit_type == Xm100TH_MILLIMETERS)
	value_in_mm = *((int *)value);
    else
	value_in_mm = 0;
    
    *value = (XtArgVal) (value_in_mm / mm_per_pixel);
    return XmSYNTHETIC_LOAD;
}


/**********************************************************************
 *
 *  _XmToHorizontalPixels
 *	Convert from a non-pixel unit type to pixels using the 
 *	horizontal resolution of the screen.  This function is
 *	accessed from a widget.
 *
 **********************************************************************/
XmImportOperator 
#ifdef _NO_PROTO
_XmToHorizontalPixels( widget, offset, value )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
#else
_XmToHorizontalPixels(
        Widget widget,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
   return ToPixels(widget, offset, value, XmHORIZONTAL) ;
}

/*********************************************************************
 *
 *  XmCvtToVerticalPixels
 *      Convert from a specified unit type to pixel type using
 *      the vertical resolution of the screen.
 *
 *********************************************************************/
int 
#ifdef _NO_PROTO
XmCvtToVerticalPixels( screen, from_val, from_type )
        Screen *screen ;
        register int from_val ;
        register int from_type ;
#else
XmCvtToVerticalPixels(
        Screen *screen,
        register int from_val,
        register int from_type )
#endif /* _NO_PROTO */
{
	 /*  Do error checking  */
	 if (screen == NULL)
		return (0);
	
	return(_XmConvertUnits(screen, XmVERTICAL,
		from_type, from_val, XmPIXELS));
}



/********************************************************************
 *
 *  _XmToVerticalPixels
 *	Convert from non-pixel unit type to pixels using the 
 *	vertical resolution of the screen.  This function is
 *	accessed from a widget.
 *
 **********************************************************************/
XmImportOperator 
#ifdef _NO_PROTO
_XmToVerticalPixels( widget, offset, value )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
#else
_XmToVerticalPixels(
        Widget widget,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
   return ToPixels(widget, offset, value, XmVERTICAL) ;
}


/*********************************************************************
*
*
*  XmCvtFromHorizontalPixels
*      Convert from a pixel unit type to specified type using
*      the horizontal resolution of the screen.
*
 **********************************************************************/
int 
#ifdef _NO_PROTO
XmCvtFromHorizontalPixels( screen, from_val, to_type )
        Screen *screen ;
        register int from_val ;
        register int to_type ;
#else
XmCvtFromHorizontalPixels(
        Screen *screen,
        register int from_val,
        register int to_type )
#endif /* _NO_PROTO */
{
												
	/*  Do error checking  */

	if (screen == NULL)
		return (0);
	
	return(_XmConvertUnits(screen, 
		XmHORIZONTAL, XmPIXELS, from_val, to_type));
}


/**********************************************************************
 *
 *  FromPixels
 *	Convert from a pixel unit type to a non-pixels using the 
 *	given orientation/resolution of the screen.
 *
 **********************************************************************/
static void 
#ifdef _NO_PROTO
FromPixels( widget, offset, value, orientation )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
        unsigned char orientation ;
#else
FromPixels(
        Widget widget,
        int offset,
        XtArgVal *value,
	unsigned char orientation)
#endif /* _NO_PROTO */
{
    Screen * screen = XtScreen (widget);
    int mm_per_pixel;
    register unsigned char unit_type ;
    register int value_in_mm;
    int font_unit;
    
    /*  Get the unit type of the widget  */
    
    unit_type = _XmGetUnitType(widget);
    
    /*  Check for type to same type conversions  */
    
    if (unit_type == XmPIXELS) return;
    
    /*  Pre-process FontUnit types  */
    
    if (unit_type == Xm100TH_FONT_UNITS)
	{
	    font_unit = _XmGetFontUnit (screen, orientation);
	    *value = (XtArgVal) (*((int *)value) * 100 / font_unit);
	    return ;
	}
    
    
    /*  Get the screen dimensional data  */
    
    if (orientation == XmHORIZONTAL) {
	mm_per_pixel = 
	    (WidthMMOfScreen(screen) * 100) / WidthOfScreen(screen);
    } else {
	mm_per_pixel = 
	    (HeightMMOfScreen(screen) * 100) / HeightOfScreen(screen);
    }

    /*  Convert the data  */
    
    value_in_mm = (int) (*value) * mm_per_pixel;
    
    if (unit_type == Xm100TH_POINTS)
	*value = (XtArgVal) ((value_in_mm * 1000) / 353);
    else if (unit_type == Xm1000TH_INCHES)
	*value = (XtArgVal) ((value_in_mm * 100) / 254);
    else if (unit_type == Xm100TH_MILLIMETERS)
	*value = (XtArgVal) (value_in_mm);
}


/**********************************************************************
 *
 *  _XmFromHorizontalPixels
 *	Convert from a pixel unit type to a non-pixels using the 
 *	horizontal resolution of the screen.  This function is
 *	accessed from a getvalues hook table.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmFromHorizontalPixels( widget, offset, value )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
#else
_XmFromHorizontalPixels(
        Widget widget,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
    FromPixels(widget, offset, value, XmHORIZONTAL);
}


/*********************************************************************
*
*
*  XmCvtFromVerticalPixels
*      Convert from a pixel unit type to specified type using
*      the horizontal resolution of the screen.
*
 **********************************************************************/
int 
#ifdef _NO_PROTO
XmCvtFromVerticalPixels( screen, from_val, to_type )
        Screen *screen ;
        register int from_val ;
        register int to_type ;
#else
XmCvtFromVerticalPixels(
        Screen *screen,
        register int from_val,
        register int to_type )
#endif /* _NO_PROTO */
{
												
	/*  Do error checking  */

	if (screen == NULL)
		return (0);
	
	return(_XmConvertUnits(screen, 
		XmVERTICAL, XmPIXELS, from_val, to_type));
}



/**********************************************************************
 *
 *  _XmFromVerticalPixels
 *	Convert from pixel unit type to non-pixels using the 
 *	vertical resolution of the screen.  This function is
 *	accessed from a getvalues hook table.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmFromVerticalPixels( widget, offset, value )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
#else
_XmFromVerticalPixels(
        Widget widget,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
    FromPixels(widget, offset, value, XmVERTICAL);
}



/**********************************************************************
 *
 * _XmSortResourceList
 * This procedure ensures the XmNunitType occurs first in the class 
 * resource list.  By doing this, the coreClass resource converters 
 * for x,y,width,height,and borderWidth are guaranteed to have a valid 
 * unitType.
 * WARNING: Doing that is not compliant with Xt, the internal format is
 *          not public. 
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmSortResourceList( list, len )
        XrmResource *list[] ;
        Cardinal len ;
#else
_XmSortResourceList(
        XrmResource *list[],
        Cardinal len )
#endif /* _NO_PROTO */
{
	static Boolean first_time = TRUE;
	static XrmQuark unitQ;
	int n,i;
	XrmResource *p = NULL;

	if (first_time)
	{
		unitQ = XrmStringToQuark(XmNunitType);
		first_time = FALSE;
	}

	for (n=0; n < len; n++)
		if (list[n]->xrm_name == unitQ)
		{
			p = list[n];
			break;
		}
	
	if (n == len)
		return; /* No unit type resource found in this list. */
	else
	{
		for (i=n; i > 0; i--)
			list[i] = list[i-1];
		
		list[0] = p;
	}
}




/**********************************************************************
 *
 * _XmUnitTypeDefault
 * This procedure is called as the resource default XtRCallProc
 * to default the unit type resource.  This procedure supports 
 * the propagation of unit type from parent to child.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmUnitTypeDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmUnitTypeDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
    static unsigned char unit_type;

    value->size = sizeof(unit_type);
    value->addr = (XPointer) &unit_type;

    if (XmIsManager(widget->core.parent))
	unit_type = 
	    ((XmManagerWidget)(widget->core.parent))->manager.unit_type;
    else
	unit_type = XmPIXELS;
}

/**********************************************************************
 *
 * _XmGetUnitType
 * This function takes care of the class of the widget being passed
 * and look in the appropriate field.
 *
 **********************************************************************/
unsigned char
#ifdef _NO_PROTO
_XmGetUnitType(widget)
        Widget widget ;
#else
_XmGetUnitType(
        Widget widget)
#endif /* _NO_PROTO */
{
    register unsigned char unit_type = XmPIXELS;

    if (XmIsGadget (widget))
	unit_type = ((XmGadget)(widget))->gadget.unit_type;
    else if (XmIsPrimitive (widget))
	unit_type = ((XmPrimitiveWidget)(widget))->primitive.unit_type;
    else if (XmIsManager (widget))
	unit_type = ((XmManagerWidget)(widget))->manager.unit_type;
    else if (XmIsExtObject(widget)) {
	XmExtObject	extObj = (XmExtObject)widget;
	Widget		parent = extObj->ext.logicalParent;
	    
	if (XmIsVendorShell(parent)) {
	    unit_type = ((XmVendorShellExtObject)widget) ->vendor.unit_type;
	}
	else if (XmIsGadget(parent)) {
	    unit_type = ((XmGadget)(parent))->gadget.unit_type;
	}
    }

    return unit_type ;    
}
