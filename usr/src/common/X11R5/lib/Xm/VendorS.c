#pragma ident	"@(#)m1.2libs:Xm/VendorS.c	1.9"
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
/*
*  (c) Copyright 1987, 1988, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY */
/* Make sure all wm properties can make it out of the resource manager */

#include <Xm/VendorSP.h>
#include <Xm/VendorSEP.h>
#include <Xm/BaseClassP.h>
#include <Xm/DisplayP.h>
#include <Xm/ScreenP.h>
#include <Xm/WorldP.h>
#include <Xm/ProtocolsP.h>
#include <Xm/AtomMgr.h>
#include "MessagesI.h"
#include "CallbackI.h"

#ifdef I18N_MSG
#include "XmMsgI.h"

#if !defined(NL_CAT_LOCALE)
#define NL_CAT_LOCALE	0
#endif
#endif /* I18N_MSG */

extern void _XmImFreeData() ;


/* #define DEBUG_GRABS */


#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_Vendor,MSG_V_1,_XmMsgVendor_0000)
#else
#define MSG1	_XmMsgVendor_0000
#endif


#define DONT_CARE -1

#ifndef _XA_WM_DELETE_WINDOW
#define	_XA_WM_DELETE_WINDOW 	"WM_DELETE_WINDOW"
#endif /* _XA_WM_DELETE_WINDOW */
    
typedef struct {   
    XmVendorShellExtObject ve ;
    Widget shell ;
    } XmDestroyGrabRec, *XmDestroyGrabList ;


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XtPointer BaseProc() ;
static Cardinal GetSecResData() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void SetMwmStuff() ;
static void AddGrab() ;
static void RemoveGrab() ;
static void RemoveGrabCallback() ;
static void AddToGrabList() ;
static void AddCousinsToGrabList() ;
static Boolean IsPopupShell() ;
static void PopupCallback() ;
static void PopdownCallback() ;
static Widget GetNextShell() ;
static XmDesktopObject GetShellDesktopParent() ;
static void SecondaryObjectCreate() ;
static void InitializePrehook() ;
static void VendorExtInitialize() ;
static void Initialize() ;
static void InitializePosthook() ;
static Boolean SetValuesPrehook() ;
static Boolean VendorExtSetValues() ;
static Boolean SetValues() ;
static Boolean SetValuesPosthook() ;
static void GetValuesPrehook() ;
static void GetValuesHook() ;
static void GetValuesPosthook() ;
static void PendingTransientDestroyed() ;
static void SetTransientFor() ;
static void Resize() ;
static void ChangeManaged() ;
static void UpdateCoreGeometry() ;
static void Realize() ;
static XtGeometryResult GeometryManager() ;
static void SetMwmHints() ;
static void SetMwmMenu() ;
static void AddDLEntry() ;
static void RemoveDLEntry() ;
static void Destroy() ;
static void Redisplay() ;

#else

static XtPointer BaseProc( 
                        Widget widget,
                        XtPointer client_data) ;
static Cardinal GetSecResData( 
                        WidgetClass w_class,
                        XmSecondaryResourceData **secResDataRtn) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void SetMwmStuff( 
                        XmVendorShellExtObject ove,
                        XmVendorShellExtObject nve) ;
static void AddGrab( 
                        XmVendorShellExtObject ve,
                        Widget shell,
#if NeedWidePrototypes
                        int exclusive,
                        int springLoaded,
#else
                        Boolean exclusive,
                        Boolean springLoaded,
#endif /* NeedWidePrototypes */
                        XmVendorShellExtObject origKid) ;
static void RemoveGrab( 
                        XmVendorShellExtObject ve,
#if NeedWidePrototypes
                        int being_destroyed,
#else
                        Boolean being_destroyed,
#endif /* NeedWidePrototypes */
                        Widget shell) ;
static void RemoveGrabCallback( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void AddToGrabList( 
                        Widget parent,
                        Widget excludedKid,
                        Widget origKid) ;
static void AddCousinsToGrabList( 
                        Widget parent,
                        Widget excludedKid,
                        Widget origKid) ;
static Boolean IsPopupShell( 
                        Widget shell) ;
static void PopupCallback( 
                        Widget shellParent,
                        XtPointer closure,
                        XtPointer callData) ;
static void PopdownCallback( 
                        Widget shellParent,
                        XtPointer closure,
                        XtPointer callData) ;
static Widget GetNextShell( 
                        Widget vw) ;
static XmDesktopObject GetShellDesktopParent( 
                        VendorShellWidget vw,
                        ArgList args,
                        Cardinal *num_args) ;
static void SecondaryObjectCreate( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void InitializePrehook( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void VendorExtInitialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Initialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void InitializePosthook( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValuesPrehook( 
                        Widget old,
                        Widget ref,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean VendorExtSetValues( 
                        Widget old,
                        Widget ref,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValues( 
                        Widget current,
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValuesPosthook( 
                        Widget current,
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesPrehook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesHook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesPosthook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
static void PendingTransientDestroyed( 
                        Widget vw,
                        XtPointer cl_data,
                        XtPointer ca_data) ;
static void SetTransientFor( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void Resize( 
                        Widget w) ;
static void ChangeManaged( 
                        Widget wid) ;
static void UpdateCoreGeometry( 
                        VendorShellWidget vw,
                        XmVendorShellExtObject vendorExt) ;
static void Realize( 
                        Widget wid,
                        XtValueMask *vmask,
                        XSetWindowAttributes *attr) ;
static XtGeometryResult GeometryManager( 
                        Widget wid,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static void SetMwmHints( 
                        XmVendorShellExtObject ve) ;
static void SetMwmMenu( 
                        XmVendorShellExtObject ve) ;
static void AddDLEntry( 
                        XmVendorShellExtObject ve,
                        Widget shell) ;
static void RemoveDLEntry( 
                        unsigned pos) ;
static void Destroy( 
                        Widget wid) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


#ifdef I18N_MSG
nl_catd Xm_catd;	/* catalog descriptor for all libXm error messages */
#endif


#ifdef DEBUG_GRABS

#ifdef _NO_PROTO

static void PrintModal() ;
static void PrintXmGrabs() ;

#else

static void PrintModal(
                        XmModalData modal) ;
static void PrintXmGrabs(
                        Widget wid) ;

#endif /* _NO_PROTO */

#endif /* DEBUG_GRABS */

char _XmVersionString[] = XmVERSION_STRING ;

static XmDestroyGrabList destroy_list ;
static unsigned short destroy_list_size ;
static unsigned short destroy_list_cnt ;

static Display * _XmDisplayHandle = NULL ;


/***************************************************************************
 *
 * Vendor shell class record
 *
 ***************************************************************************/

#define Offset(field) (XtOffsetOf(WMShellRec, field))

static int default_unspecified_shell_int = XtUnspecifiedShellInt;
/*
 * Warning, casting XtUnspecifiedShellInt (which is -1) to an (XtPointer)
 * can result is loss of bits on some machines (i.e. crays)
 */

static XtResource resources[] =
{
    {
	XmNx, XmCPosition, XmRShellHorizPos, sizeof(Position),
	XtOffsetOf( struct _WidgetRec, core.x), XmRImmediate, (XtPointer) 0,
    },
    
    {
	XmNy, XmCPosition, XmRShellVertPos, sizeof(Position),
	XtOffsetOf( struct _WidgetRec, core.y), XmRImmediate, (XtPointer) 0,
    },
    
    {
	XmNwidth, XmCDimension, XmRShellHorizDim, sizeof(Dimension),
	XtOffsetOf( struct _WidgetRec, core.width), XmRImmediate, (XtPointer) 0,
    },
    
    {
	XmNheight, XmCDimension, XmRShellVertDim, sizeof(Dimension),
	XtOffsetOf( struct _WidgetRec, core.height), XmRImmediate, (XtPointer) 0,
    },
    
    {
	XmNborderWidth, XmCBorderWidth, XmRShellHorizDim, sizeof(Dimension),
	XtOffsetOf( struct _WidgetRec, core.border_width), XmRImmediate, (XtPointer) 0,
    },
    
    /* size_hints minus things stored in core */
    { XmNbaseWidth, XmCBaseWidth, XmRHorizontalInt, sizeof(int),
	Offset(wm.base_width),
	XmRHorizontalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNbaseHeight, XmCBaseHeight, XmRVerticalInt, sizeof(int),
	Offset(wm.base_height),
	XmRVerticalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNminWidth, XmCMinWidth, XmRHorizontalInt, sizeof(int),
	Offset(wm.size_hints.min_width),
	XmRHorizontalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNminHeight, XmCMinHeight, XmRVerticalInt, sizeof(int),
	Offset(wm.size_hints.min_height),
	XmRVerticalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNmaxWidth, XmCMaxWidth, XmRHorizontalInt, sizeof(int),
	Offset(wm.size_hints.max_width),
	XmRHorizontalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNmaxHeight, XmCMaxHeight, XmRVerticalInt, sizeof(int),
	Offset(wm.size_hints.max_height),
	XmRVerticalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNwidthInc, XmCWidthInc, XmRHorizontalInt, sizeof(int),
	Offset(wm.size_hints.width_inc),
	XmRHorizontalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNheightInc, XmCHeightInc, XmRVerticalInt, sizeof(int),
	Offset(wm.size_hints.height_inc),
	XmRVerticalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNminAspectX, XmCMinAspectX, XmRHorizontalInt, sizeof(int),
	Offset(wm.size_hints.min_aspect.x),
	XmRHorizontalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNminAspectY, XmCMinAspectY, XmRVerticalInt, sizeof(int),
	Offset(wm.size_hints.min_aspect.y),
	XmRVerticalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNmaxAspectX, XmCMaxAspectX, XmRHorizontalInt, sizeof(int),
	Offset(wm.size_hints.max_aspect.x),
	XmRHorizontalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNmaxAspectY, XmCMaxAspectY, XmRVerticalInt, sizeof(int),
	Offset(wm.size_hints.max_aspect.y),
	XmRVerticalInt, (XtPointer) &default_unspecified_shell_int},
    /* wm_hints */
    { XmNiconPixmap, XmCIconPixmap, XmRPixmap, sizeof(Pixmap),
	Offset(wm.wm_hints.icon_pixmap), XmRPixmap, NULL},
    { XmNiconX, XmCIconX, XmRHorizontalInt, sizeof(int),
	Offset(wm.wm_hints.icon_x),
	XmRHorizontalInt, (XtPointer) &default_unspecified_shell_int},
    { XmNiconY, XmCIconY, XmRVerticalInt, sizeof(int),
	Offset(wm.wm_hints.icon_y),
	XmRVerticalInt, (XtPointer) &default_unspecified_shell_int},

    { /* override dec default */
	XmNinput, XmCInput, XmRBool, 
	sizeof(Bool), Offset(wm.wm_hints.input), 
	XmRImmediate, (XtPointer) TRUE,
    },
    { /* override incorrect default in Shell.c */
	XmNwindowGroup, XmCWindowGroup, XmRWindow, sizeof(Window),
	Offset(wm.wm_hints.window_group),
	XmRImmediate, (XtPointer)XtUnspecifiedWindowGroup,
    },
};	
#undef Offset


static CompositeClassExtensionRec compositeClassExtRec = {
    NULL,
    NULLQUARK,
    XtCompositeExtensionVersion,
    sizeof(CompositeClassExtensionRec),
    TRUE,
};

static ShellClassExtensionRec shellClassExtRec = {
    NULL,
    NULLQUARK,
    XtShellExtensionVersion,
    sizeof(ShellClassExtensionRec),
    _XmRootGeometryManager,
};

static XmBaseClassExtRec baseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    InitializePrehook,				/* init prehook		*/
    SetValuesPrehook,				/* setval prehook	*/
    InitializePosthook,				/* init posthook	*/
    SetValuesPosthook,				/* setval posthook	*/
    (WidgetClass)&xmVendorShellExtClassRec,	/* secondary class	*/
    SecondaryObjectCreate,	        /* secondary create	*/
    GetSecResData,		                /* getSecRes data       */
    {0},					/* class flags		*/
    GetValuesPrehook,				/* get_values_prehook	*/
    GetValuesPosthook,				/* get_values_posthook  */
};

externaldef(vendorshellclassrec)
VendorShellClassRec vendorShellClassRec = {
    {	
	(WidgetClass) &wmShellClassRec, /* superclass 		*/   
	"VendorShell", 			/* class_name 		*/   
	sizeof(VendorShellRec), 	/* size 		*/   
	ClassInitialize, 		/* Class Initializer 	*/   
	ClassPartInitialize, 		/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	Initialize, 			/* initialize         	*/   
	NULL, 				/* initialize_hook	*/ 
	Realize, 			/* realize            	*/   
	NULL,	 			/* actions            	*/   
	0,				/* num_actions        	*/   
	resources, 			/* resources          	*/   
	XtNumber(resources), 		/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	TRUE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	Destroy,			/* destroy            	*/   
	Resize,		 		/* resize             	*/   
	NULL,				/* expose             	*/   
	SetValues,	 		/* set_values         	*/   
	NULL, 				/* set_values_hook      */ 
	XtInheritSetValuesAlmost, 	/* set_values_almost    */ 
	GetValuesHook,			/* get_values_hook      */ 
	NULL, 				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL, 				/* display_accelerator  */ 
	(XtPointer)&baseClassExtRec,	/* extension		*/ 
    },	
    { 					/* composite_class	*/
	GeometryManager,		/* geometry_manager 	*/   
	ChangeManaged,		 	/* change_managed 	*/   
	XtInheritInsertChild, 		/* insert_child 	*/   
	XtInheritDeleteChild, 		/* delete_child 	*/   
	(XtPointer)&compositeClassExtRec,/* extension            */ 
    },                           
    {                            	/* shell class		*/
	(XtPointer)&shellClassExtRec,	/* extension 		*/ 
    }, 
    {                            	/* wmshell class	*/
	NULL, 				/* extension            */ 
    }, 	
    {   				/* vendorshell class	*/
	NULL, 				/* extension            */ 
    }                            
};	   

externaldef(vendorshellwidgetclass) WidgetClass 
  vendorShellWidgetClass = (WidgetClass) (&vendorShellClassRec);



/************************************************************************
 *
 *  BaseProc
 *
 ************************************************************************/
static XtPointer 
#ifdef _NO_PROTO
BaseProc( widget, client_data )
        Widget widget ;
        XtPointer client_data ;
#else
BaseProc(
        Widget widget,
        XtPointer client_data )
#endif /* _NO_PROTO */
{   
    XmWidgetExtData	extData;
    Widget		secObj;

    if ((extData = _XmGetWidgetExtData(widget, XmSHELL_EXTENSION)) &&
	(secObj =extData->widget))
      return (XtPointer)secObj;
    else
      return NULL;
}

/************************************************************************
 *
 *  _XmFilterResources
 *
 ************************************************************************/
Cardinal 
#ifdef _NO_PROTO
_XmFilterResources( resources, numResources, filterClass, filteredResourcesRtn )
        XtResource *resources ;
        Cardinal numResources ;
        WidgetClass filterClass ;
        XtResource **filteredResourcesRtn ;
#else
_XmFilterResources(
        XtResource *resources,
        Cardinal numResources,
        WidgetClass filterClass,
        XtResource **filteredResourcesRtn )
#endif /* _NO_PROTO */
{
    XtResource		*filteredResources;
    Cardinal		copyIndexes[256];
    Cardinal		filterOffset;
    Cardinal		i, j;

    filterOffset = filterClass->core_class.widget_size;

    for (i = 0, j = 0; i < numResources; i++)
      {
	  if (resources[i].resource_offset >= filterOffset)
	    {
		copyIndexes[j++] = i;
	    }
      }

    filteredResources = (XtResource *) XtMalloc(j * sizeof(XtResource));

    for (i = 0; i < j; i++)
      {
	  filteredResources[i] = resources[copyIndexes[i]];
      }
    *filteredResourcesRtn = filteredResources;
    return j;
}

/************************************************************************
 *
 *  GetSecResData
 *
 ************************************************************************/
static Cardinal 
#ifdef _NO_PROTO
GetSecResData( w_class, secResDataRtn )
        WidgetClass w_class ;
        XmSecondaryResourceData **secResDataRtn ;
#else
GetSecResData(
        WidgetClass w_class,
        XmSecondaryResourceData **secResDataRtn )
#endif /* _NO_PROTO */
{
    XmBaseClassExt	*bcePtr;
    WidgetClass		secObjClass;
    int arrayCount;
    String  resource_class, resource_name;
    XtPointer  client_data;

    if ((bcePtr = _XmGetBaseClassExtPtr(w_class, XmQmotif)) &&
	(bcePtr) && (*bcePtr) &&
	(secObjClass = ((*bcePtr)->secondaryObjectClass)))
    {
      client_data = NULL;
      resource_class = NULL;
      resource_name = NULL;
      arrayCount =
        _XmSecondaryResourceData ( *bcePtr, secResDataRtn, client_data,  
                                   resource_name, resource_class,
			           BaseProc) ;

      return (arrayCount);
    }
    else
      return 0;
}

/************************************************************************
 *
 *  ClassInitialize
 *    Initialize the vendorShell class structure.  This is called only
 *    the first time a vendorShell widget is created.  It registers the
 *    resource type converters unique to this class.
 *
 ************************************************************************/
#ifdef BBA_DUMP_ON_EXIT
static void call_ba_dump() { }
static void cause_ba_dump()
{ 
    /* With the option "-DBBA_OPTBcall_ba_dump" on the cc command line,
     *   the BBA preprocessor will insert a call to _bA_dump() before
     *   calling the call_ba_dump() dummy function.
     */
    call_ba_dump() ;
}
#endif

static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
  Cardinal                    wc_num_res, sc_num_res;
  XtResource                  *merged_list;
  int                         i, j;
  XtResourceList              uncompiled;
  Cardinal                    num;

/**************************************************************************
   ShellExt's and VendorShellExt's resource lists are being merged into one
   and assigned to xmVendorShellExtObjectClassRec. This is for performance
   reasons, since, instead of two calls to XtGetSubResources() XtGetSubvaluse()
   and XtSetSubvalues() for both the superclass and the widget class, now
   we have just one call with a merged resource list.
   NOTE: At this point the resource lists for ShellExt and VendorShellExt do
         have unique entries, but if there are resources in the superclass
         that are being overwritten by the subclass then the merged_lists
         need to be created differently.
****************************************************************************/

#ifdef BBA_DUMP_ON_EXIT
    atexit( cause_ba_dump) ;
#endif

  wc_num_res = xmVendorShellExtClassRec.object_class.num_resources;

  sc_num_res = xmShellExtClassRec.object_class.num_resources;

  merged_list = (XtResource *)XtMalloc((sizeof(XtResource) * (wc_num_res +
                                                                 sc_num_res)));

  _XmTransformSubResources(xmShellExtClassRec.object_class.resources,
                           sc_num_res, &uncompiled, &num);

  for (i = 0; i < num; i++)
  {

  merged_list[i] = uncompiled[i];

  }

  for (i = 0, j = num; i < wc_num_res; i++, j++)
  {
   merged_list[j] =
        xmVendorShellExtClassRec.object_class.resources[i];
  }

  xmVendorShellExtClassRec.object_class.resources = merged_list;
  xmVendorShellExtClassRec.object_class.num_resources =
                wc_num_res + sc_num_res ;

  _XmRegisterConverters();
  _XmInitializeExtensions();

   xmVendorShellExtObjectClass->core_class.class_initialize();
   baseClassExtRec.record_type = XmQmotif;

   _XmBuildExtResources((WidgetClass) baseClassExtRec.secondaryObjectClass);

   if (((XmShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->desktop_class.insert_child ==
          XtInheritInsertChild)
       ((XmShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->desktop_class.insert_child =
        ((XmShellExtObjectClass) xmDesktopClass)->desktop_class.insert_child;

   if (((XmShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->desktop_class.delete_child ==
          XtInheritDeleteChild)
       ((XmShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->desktop_class.delete_child =
        ((XmShellExtObjectClass) xmDesktopClass)->desktop_class.delete_child;

   if (((XmShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->shell_class.structureNotifyHandler ==
          XmInheritEventHandler)
       ((XmShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->shell_class.structureNotifyHandler =
        ((XmShellExtObjectClass) xmShellExtObjectClass)->shell_class.structureNotifyHandler;

   if (((XmVendorShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->vendor_class.offset_handler ==
          XmInheritProtocolHandler)
       ((XmVendorShellExtObjectClass)baseClassExtRec.secondaryObjectClass)->vendor_class.offset_handler =
        ((XmVendorShellExtObjectClass) xmVendorShellExtObjectClass)->vendor_class.offset_handler;

   XtFree((char *)uncompiled);

#ifdef I18N_MSG
    Xm_catd = catopen(MF_XM, NL_CAT_LOCALE);
#endif

}


/************************************************************************
 *
 *  ClassPartInitialize
 *    Set up the inheritance mechanism for the routines exported by
 *    vendorShells class part.
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
    CompositeWidgetClass	compWc = (CompositeWidgetClass)wc;
    CompositeWidgetClass	superWc = (CompositeWidgetClass)wc->core_class.superclass;
    CompositeClassExtensionRec 	**compExtPtr;
    XmBaseClassExt		*wcePtr, *scePtr;
    XmVendorShellWidgetClass    vc = (XmVendorShellWidgetClass) wc; 

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif);
    scePtr = _XmGetBaseClassExtPtr(wc->core_class.superclass, XmQmotif);

    if ( (vc != (XmVendorShellWidgetClass)vendorShellWidgetClass) && scePtr && *scePtr && 
         (*wcePtr)->secondaryObjectClass != NULL &&
	((*scePtr)->secondaryObjectClass != (*wcePtr)->secondaryObjectClass))      {
/*
    if ( scePtr && *scePtr && 
	((*scePtr)->secondaryObjectClass != (*wcePtr)->secondaryObjectClass))      {
*/
      XmVendorShellExtObjectClass  wceClass, sceClass;

      wceClass = (XmVendorShellExtObjectClass)(*wcePtr)->secondaryObjectClass;
      sceClass = (XmVendorShellExtObjectClass)(*scePtr)->secondaryObjectClass;
      _XmBuildExtResources((WidgetClass) (*wcePtr)->secondaryObjectClass);
      
      if (wceClass->desktop_class.insert_child == XtInheritInsertChild)
	wceClass->desktop_class.insert_child = 
	  sceClass->desktop_class.insert_child;
      
      if (wceClass->desktop_class.delete_child == XtInheritDeleteChild)
	wceClass->desktop_class.delete_child =
	  sceClass->desktop_class.delete_child;
      
      if (wceClass->shell_class.structureNotifyHandler == XmInheritEventHandler)
	wceClass->shell_class.structureNotifyHandler =
	  sceClass->shell_class.structureNotifyHandler;
      
      if (wceClass->vendor_class.offset_handler == XmInheritProtocolHandler)
	wceClass->vendor_class.offset_handler = 
	  sceClass->vendor_class.offset_handler;
      }
    
    compExtPtr = (CompositeClassExtensionRec **) &(compWc->composite_class.extension);

    compExtPtr = (CompositeClassExtensionRec **)
      _XmGetClassExtensionPtr( (XmGenericClassExt *) compExtPtr, NULLQUARK);

    if (*compExtPtr == NULL) {
	CompositeClassExtensionRec 	**superExtPtr;

	superExtPtr = (CompositeClassExtensionRec **) &(superWc->composite_class.extension);
	superExtPtr = (CompositeClassExtensionRec **)
	  _XmGetClassExtensionPtr( (XmGenericClassExt *) superExtPtr, NULLQUARK);

	*compExtPtr = XtNew(CompositeClassExtensionRec);
	memcpy((char*)*compExtPtr, (char*)*superExtPtr, sizeof(CompositeClassExtensionRec));
    }

 /*
  _XmFastSubclassInit(wc)
  */

    /* Do this here because of bug in Xt */
    wc->core_class.expose = Redisplay;
   
}


/************************************************************************
 *
 *  SetMwmStuff
 *     ov will be null when called from Initialize
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetMwmStuff( ove, nve )
        XmVendorShellExtObject ove ;
        XmVendorShellExtObject nve ;
#else
SetMwmStuff(
        XmVendorShellExtObject ove,
        XmVendorShellExtObject nve )
#endif /* _NO_PROTO */
{
    Boolean		changed = FALSE;
    Widget		extParent = nve->ext.logicalParent;

    if (!ove || (ove->vendor.mwm_menu != nve->vendor.mwm_menu))
      {
	  /* make mwm_menu local */
	  if (ove && ove->vendor.mwm_menu) 
	    XtFree(ove->vendor.mwm_menu);
	  if (nve->vendor.mwm_menu)
	    nve->vendor.mwm_menu = XtNewString(nve->vendor.mwm_menu);
	  if (XtIsRealized(extParent))
	    SetMwmMenu(nve);
      }
    
    if (!ove || (ove->vendor.mwm_hints.functions != nve->vendor.mwm_hints.functions))
      {
	  if (nve->vendor.mwm_hints.functions == DONT_CARE)
	    nve->vendor.mwm_hints.flags &= ~MWM_HINTS_FUNCTIONS;
	  else
	    nve->vendor.mwm_hints.flags |= MWM_HINTS_FUNCTIONS;
	  changed |= TRUE;
      }
    
    if (!ove || (ove->vendor.mwm_hints.decorations != nve->vendor.mwm_hints.decorations))
      {
	  if (nve->vendor.mwm_hints.decorations == DONT_CARE)
	    nve->vendor.mwm_hints.flags &= ~MWM_HINTS_DECORATIONS;
	  else
	    nve->vendor.mwm_hints.flags |= MWM_HINTS_DECORATIONS;
	  changed |= TRUE;
      }
    
    if (!ove || (ove->vendor.mwm_hints.input_mode != nve->vendor.mwm_hints.input_mode))
      {
	  if (nve->vendor.mwm_hints.input_mode == DONT_CARE)
	    nve->vendor.mwm_hints.flags &= ~MWM_HINTS_INPUT_MODE;
	  else
	    nve->vendor.mwm_hints.flags |= MWM_HINTS_INPUT_MODE;
	  changed |= TRUE;
      }
    
    if (changed && XtIsRealized(extParent))
      SetMwmHints(nve);
}

/* The AddGrab and RemoveGrab routines manage a virtual Xt modal
 * cascade that allows us to remove entries in the list without
 * flushing out the grabs of all following entries. 
 */
void
#ifdef _NO_PROTO
_XmAddGrab( wid, exclusive, spring_loaded )
        Widget wid ;
        Boolean exclusive ;
        Boolean spring_loaded ;
#else
_XmAddGrab(
        Widget wid,
#if NeedWidePrototypes
        int exclusive,
        int spring_loaded)
#else
        Boolean exclusive,
        Boolean spring_loaded)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{   
  AddGrab( NULL, wid, exclusive, spring_loaded, NULL) ;
} 

void
#ifdef _NO_PROTO
_XmRemoveGrab( wid )
        Widget wid ; 
#else
_XmRemoveGrab(
        Widget wid)
#endif /* _NO_PROTO */
{   
  RemoveGrab( NULL, FALSE, wid) ;
} 

static void 
#ifdef _NO_PROTO
AddGrab( ve, shell, exclusive, springLoaded, origKid )
        XmVendorShellExtObject ve ;
        Widget shell ;          /* Doesn't really need to be a shell. */
        Boolean exclusive ;
        Boolean springLoaded ;
        XmVendorShellExtObject origKid ;
#else
AddGrab(
        XmVendorShellExtObject ve,
        Widget shell,
#if NeedWidePrototypes
        int exclusive,
        int springLoaded,
#else
        Boolean exclusive,
        Boolean springLoaded,
#endif /* NeedWidePrototypes */
        XmVendorShellExtObject origKid )
#endif /* _NO_PROTO */
{
    Cardinal		     	position;
    XmModalData			modals;
    XmDisplay			xmDisplay;

    if(    shell == NULL    )
      {
        shell = ve->ext.logicalParent ;
      } 
    xmDisplay = (XmDisplay) XmGetXmDisplay(XtDisplay(shell));

    modals = xmDisplay->display.modals;
    
    position = xmDisplay->display.numModals;
    
    if (xmDisplay->display.numModals == xmDisplay->display.maxModals) {
	/* Allocate more space */
	xmDisplay->display.maxModals +=  (xmDisplay->display.maxModals / 2) + 2;
	xmDisplay->display.modals = modals = (XmModalData) 
	  XtRealloc((char *) modals, (unsigned) 
		    ((xmDisplay->display.maxModals) * sizeof(XmModalDataRec)));
    }
    modals[position].wid = shell;
    modals[position].ve = ve;
    modals[position].grabber = origKid;
    modals[position].exclusive = exclusive;
    modals[position].springLoaded = springLoaded;
    xmDisplay->display.numModals++;
    XtAddGrab((Widget) shell,
	      exclusive,
	      springLoaded);
#ifdef DEBUG_GRABS
    printf( "AddGrab: XtAddGrab( %s, excl: %d, spring: %d); grabber: %s\n",
                  shell->core.name, exclusive, springLoaded,
                    origKid ? origKid->ext.logicalParent->core.name : "NULL") ;
#endif

    /* If the shell gets destroyed, we don't have to worry about removing
     * the Xt grab, but we do have to remove the ve from the list of modals.
     * Should the client_data be ve or origKid?
     */
    XtAddCallback((Widget) shell, XmNdestroyCallback, RemoveGrabCallback,(XtPointer)ve);
}

/*
 * we add a new argument here, being_destroyed.  If true, it means that
 * we are being called from a callback triggered by the destruction of a
 * shell.  If it is true, we should remove shells from the list of modals,
 * but we should not call XtRemoveGrab on them, because the intrinsics
 * already handle that.
 */
static void 
#ifdef _NO_PROTO
RemoveGrab( ve, being_destroyed, shell )
        XmVendorShellExtObject ve ;
	Boolean being_destroyed;
        Widget shell ;
#else
RemoveGrab(
        XmVendorShellExtObject ve,
#if NeedWidePrototypes
        int being_destroyed,
#else
	Boolean being_destroyed,
#endif /* NeedWidePrototypes */
	Widget shell)
#endif /* _NO_PROTO */
{
    XmDisplay			xmDisplay;
    Cardinal		     	incr, i, numRemoves, numModals;
    XmModalData			modals;

    if(    !being_destroyed    )
      {   
        /* The "shell" argument of this routine is required when the
         * "being_destroyed" argument is TRUE, since the vendor
         * extension record may be already de-allocated before the
         * RemoveGrabCallback is called, and thus cannot be de-referenced
         * to get the shell logical parent.  We will assume that the
         * shell argument is ignored by callers when the "being_destroyed"
         * argument is FALSE.
         */
        if(    shell == NULL    )
          {
            shell = ve->ext.logicalParent ;
          } 
        XtRemoveCallback( shell, XmNdestroyCallback, RemoveGrabCallback,
                                                              (XtPointer) ve) ;
      } 
#ifdef DEBUG_GRABS
    printf( "\n**** Entering RemoveGrab on %s ****\n", shell->core.name) ;
    PrintXmGrabs( (Widget) shell) ;
#endif

    xmDisplay = (XmDisplay) XmGetXmDisplay(XtDisplay(shell));
    modals = xmDisplay->display.modals;
    numModals = xmDisplay->display.numModals;

    for (i = 0, numRemoves = 0;
	 i < numModals; 
	 i++) 
      {
	  if(    (modals[i].wid == shell)  &&  (modals[i].ve == ve)    )
	    numRemoves++;
      }
#ifdef DEBUG_GRABS
    printf( "RemoveGrab: numRemoves: %d\n", numRemoves) ;
#endif
    if (numRemoves == 0) return;

    if (!being_destroyed)
       for (i = 0; i < numRemoves; i++)
         {   
	   XtRemoveGrab((Widget) shell);
#ifdef DEBUG_GRABS
           printf( "RemoveGrab: XtRemoveGrab( %s)\n", 
                                                  ((Widget)shell)->core.name) ;
#endif
         } 

    /* Add back all the grabs that were flushed by the removes */

    for (i = 0, incr = 0; 
	 (i + numRemoves) < numModals;
	 i++) 
      {
	  Widget incrWid;

	  /* We remove both the shell that's being pulled off the
	   * cascade and any raw mode shells that we've added. These
	   * should only be the app shells ?!
	   */
	  for (incrWid = modals[i+incr].wid;
	       ((i + incr) < numModals);
	       incr++, incrWid = modals[i+incr].wid)
	    {
		if (incrWid != shell)
		  {
		      if(    (ve != NULL) && (modals[i+incr].grabber == ve)   )
                        {   
                          /* Avoid re-adding to the grab list widgets
                           *   that were non-exclusives associated with
                           *   the shell grab being removed.
                           */
			  numRemoves++;
                        } 
		      else
			break;
		  }
	    }
	  if (incr && ((i+incr) < numModals))
	    {
              modals[i] = modals[i+incr];
              if(    !(modals[i].wid->core.being_destroyed)    )
                {
                  XtAddGrab( modals[i].wid, modals[i].exclusive,
                                                      modals[i].springLoaded) ;
#ifdef DEBUG_GRABS
                  printf( "RemoveGrab: XtAddGrab( %s, excl: %d, spring: %d)\n",
                        modals[i].wid->core.name,
			    modals[i].exclusive, modals[i].springLoaded) ;
#endif
                }
	    }
      }
    xmDisplay->display.numModals -= numRemoves ;

#ifdef DEBUG_GRABS
    printf( "\n**** Leaving RemoveGrab on %s ****\n", shell->core.name) ;
    PrintXmGrabs( (Widget) shell) ;
#endif
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
RemoveGrabCallback(w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
#else
RemoveGrabCallback(
	Widget w,
	XtPointer client_data,
	XtPointer call_data )
#endif
{
  if(    XmIsVendorShell( w)    )
    {   
      /* For VendorShells, defer the removal of the grab until after
       *   all destroy callbacks are completed, since the order of
       *   these callbacks can result in a situation where an
       *   Xt-invoked grab removal can strip "new" grabs which are
       *   installed in the RemoveGrab routine.
       */
      AddDLEntry( (XmVendorShellExtObject) client_data, w) ;
    } 
  else
    {   
      /* Nested calls to _XmAddGrab of non-VendorShell widgets
       *   are problematic, since the use of the Destroy method
       *   to ensure that the XtRemoveGrabs are completed before
       *   the Xm RemoveGrabs, cannot be employed for these widgets.
       */
      RemoveGrab( (XmVendorShellExtObject) client_data, TRUE, w) ;
    } 
}

static void 
#ifdef _NO_PROTO
AddToGrabList( parent, excludedKid, origKid )
        Widget parent ;
        Widget excludedKid ;
        Widget  origKid ;
#else
AddToGrabList(
        Widget  parent,
        Widget  excludedKid,
        Widget  origKid )
#endif /* _NO_PROTO */
{
    Widget		*currKid;
    Widget		*children;
    Cardinal		numChildren;
    Cardinal		i;

    if (!parent)
      return;
    else if (XmIsScreen(parent)) {
	XmScreen	xmScreen = (XmScreen)parent;
	children = xmScreen->desktop.children;
	numChildren = xmScreen->desktop.num_children;
    }
    else if (XmIsDisplay(parent)) {
	XmDisplay	xmDisplay = (XmDisplay)parent;
	children = (Widget *)xmDisplay->composite.children;
	numChildren = xmDisplay->composite.num_children;
    }
    else {
	XmDesktopObject	deskObj = (XmDesktopObject)parent;
	children = (Widget *)deskObj->desktop.children;
	numChildren = deskObj->desktop.num_children;
    }
    for (i = 0, currKid = children; i < numChildren; currKid++, i++) {	 
	  if (*currKid != excludedKid) {
	      if (!XmIsDisplay(parent)) {
		  ShellWidget shell;
		  shell = (ShellWidget)
		    ((XmDesktopObject)(*currKid))->ext.logicalParent;

		  if(    shell->shell.popped_up
                     || (    XtIsRealized( shell)
                          && !IsPopupShell( (Widget) shell))    )
		    AddGrab((XmVendorShellExtObject)*currKid, NULL,
			   (Boolean)False, (Boolean)False,
			   (XmVendorShellExtObject)origKid);
	      }
	      else if (!XmIsScreen(*currKid))
		  /*
		   * Don't traverse down non-Screen children of 
		   * XmDisplay widgets.
		   */
		  continue;

	      AddToGrabList(*currKid, NULL, origKid);
	  }
      }
}

static void 
#ifdef _NO_PROTO
AddCousinsToGrabList( parent, excludedKid, origKid )
        Widget parent ;
        Widget excludedKid ;
        Widget origKid ;
#else
AddCousinsToGrabList(
        Widget parent,
        Widget excludedKid,
        Widget origKid )
#endif /* _NO_PROTO */
{
    
    Widget	grandParent;
    
    if (!parent)
      return;
    else if (XmIsScreen(parent)) {
	XmScreen	xmScreen = (XmScreen)parent;
	grandParent = XtParent(xmScreen);
    }
    else if (XmIsDisplay(parent)) {
	grandParent = NULL;
    }
    else {
	XmDesktopObject	deskObj = (XmDesktopObject)parent;
	grandParent = deskObj->desktop.parent;
    }
    AddToGrabList(parent, excludedKid, origKid);
    AddCousinsToGrabList(grandParent, parent, origKid);
}

static Boolean
#ifdef _NO_PROTO
IsPopupShell( shell )
        Widget shell ;
#else
IsPopupShell(
        Widget shell)
#endif /* _NO_PROTO */
{   
  Widget parent = XtParent( shell) ;

  if(    parent != NULL    )
    {   
      Widget *ps_list = parent->core.popup_list ;
      unsigned n_psl = parent->core.num_popups ;

      while(    n_psl--    )
        {   
          if(    ps_list[n_psl] == shell    )
            {   
              return TRUE ;
            } 
        } 
    } 
  return FALSE ;
} 

/************************************************************************
 *
 *     PopupCallback
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
PopupCallback( shellParent, closure, callData )
        Widget shellParent ;
        XtPointer closure ;
        XtPointer callData ;
#else
PopupCallback(
        Widget shellParent,
        XtPointer closure,
        XtPointer callData )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject	ve = (XmVendorShellExtObject)closure;
    XtGrabKind			grab_kind = XtGrabNone; 
    Boolean			grabCousins = False;
    XmScreen			xmScreen;

    xmScreen = (XmScreen) XmGetXmScreen(XtScreen(shellParent));

    ve->vendor.xAtMap = shellParent->core.x;
    ve->vendor.yAtMap = shellParent->core.y;

    /*
     * work around broken Xt spec ordering for realize and popup callback
     */
    if (!XtIsRealized(shellParent))
      XtRealizeWidget(shellParent);
    
    /* 
     * get the request num + 1 Since it's a map raised. This will
     * only work when the hierarchy is already realized, i.e. after
     * first time
     */
    
    ve->vendor.lastMapRequest = NextRequest(XtDisplay(shellParent)) + 1;
    

#ifdef notdef
    /*
     * Set the map style to popup if it's not already been set to
     * manage. This allows the right handling of wm unmaps
     */
    if (ve->vendor.mapStyle == _XmRAW_MAP)
      ve->vendor.mapStyle = _XmPOPUP_MAP;
#endif /* notdef */

    switch (ve->vendor.mwm_hints.input_mode)
      {
	case DONT_CARE:
	case MWM_INPUT_MODELESS:
          grab_kind = XtGrabNonexclusive;
          break ;
	case MWM_INPUT_PRIMARY_APPLICATION_MODAL:
	  /*
	   * if we're not running mwm then this becomes full app modal
	   */
	  if (xmScreen->screen.mwmPresent)
	    grabCousins = True;
	  grab_kind = XtGrabExclusive;
	  break;
	case MWM_INPUT_SYSTEM_MODAL:
	case MWM_INPUT_FULL_APPLICATION_MODAL:
	  grab_kind = XtGrabExclusive;
	  break;
	default:
	  break;
      }
    if (grab_kind != XtGrabNone)
      AddGrab(ve, NULL,
	      (grab_kind == XtGrabExclusive), 
	      False, 
	      ve);

    ve->vendor.grab_kind = grab_kind;
    
    if (grabCousins)
      AddCousinsToGrabList((Widget)ve->desktop.parent,  
			   (Widget)ve,
			   (Widget)ve);
#ifdef DEBUG_GRABS
    printf( "\n**** After popup: ****\n") ;
    PrintXmGrabs( shellParent) ;
#endif
}

/************************************************************************
 *
 *     PopdownCallback
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
PopdownCallback( shellParent, closure, callData )
        Widget shellParent ;
        XtPointer closure ;
        XtPointer callData ;
#else
PopdownCallback(
        Widget shellParent,
        XtPointer closure,
        XtPointer callData )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject	ve = (XmVendorShellExtObject)closure;
    
    if (ve->vendor.grab_kind != XtGrabNone)
      RemoveGrab(ve, False, NULL);
#ifdef DEBUG_GRABS
    printf( "\n**** After popdown: ****\n" ) ;
    PrintXmGrabs( shellParent) ;
#endif
}

static Widget 
#ifdef _NO_PROTO
GetNextShell( vw )
        Widget vw ;
#else
GetNextShell(
        Widget vw )
#endif /* _NO_PROTO */
{
    Widget 	parent;
    
    for (parent = XtParent(vw);
	 parent && !XmIsVendorShell(parent);
	 parent = XtParent(parent)) ;

    return parent;
}
static XmDesktopObject 
#ifdef _NO_PROTO
GetShellDesktopParent( vw, args, num_args )
        VendorShellWidget vw ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetShellDesktopParent(
        VendorShellWidget vw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	Widget			transientParent = NULL;
	XmDesktopObject		desktopParent = NULL;


	if (vw->wm.transient)
	{
		if (XtIsSubclass((Widget) vw, transientShellWidgetClass))
		{
			TransientShellWidget tw = (TransientShellWidget)vw;

			if (!(transientParent = tw->transient.transient_for))
			{
				tw->transient.transient_for =
					transientParent = GetNextShell( (Widget) vw);
			}
		}
		else
		{
			transientParent = GetNextShell((Widget) vw);
		}
	}

	if (transientParent)
	{
		XmWidgetExtData	extData;

		if (XmIsVendorShell(transientParent))
		{
			extData = _XmGetWidgetExtData(transientParent,
				XmSHELL_EXTENSION);
			desktopParent = (XmDesktopObject)extData->widget;
		}
	}
	else if (!XmIsScreen((Widget)vw) && !XmIsDisplay((Widget)vw))
	{
		desktopParent =
			(XmDesktopObject) XmGetXmScreen(XtScreen((Widget)vw));
	}
	return desktopParent;
}

/************************************************************************
 *
 *  SecondaryObjectCreate
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SecondaryObjectCreate( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SecondaryObjectCreate(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmBaseClassExt		*pePtr;
    WidgetClass			vec;
    XmDesktopObject		desktopParent;
    Cardinal                    size;
    XtPointer                   newSec, reqSec;
    XmWidgetExtData             extData;

    desktopParent = GetShellDesktopParent( (VendorShellWidget) new_w,
                                                     args, num_args) ;
    if (desktopParent)
    {

	  /*
	   * if the secondary object is using sub_resources then
	   * create it as a child of the shell. Otherwise try to
	   * create it as a sibling in order to fake out resource path
	   */
	  pePtr = _XmGetBaseClassExtPtr(XtClass(new_w), XmQmotif);
	  vec = (*pePtr)->secondaryObjectClass;
          size = vec->core_class.widget_size;

          newSec = XtMalloc(size);
          reqSec = _XmExtObjAlloc(size);

    /*
     * fetch the resources in superclass to subclass order
     */

    XtGetSubresources(new_w,
                      newSec,
                      NULL, NULL,
                      vec->core_class.resources,
                      vec->core_class.num_resources,
                      args, *num_args );


    extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
    extData->widget = (Widget)newSec;
    extData->reqWidget = (Widget)reqSec;

    ((XmVendorShellExtObject)newSec)->ext.extensionType = XmSHELL_EXTENSION;
    ((XmVendorShellExtObject)newSec)->ext.logicalParent = new_w;
    ((XmVendorShellExtObject)newSec)->desktop.parent = (Widget) desktopParent;

    ((XmVendorShellExtObject)newSec)->object.widget_class = vec; 
    ((XmVendorShellExtObject)newSec)->object.parent = new_w;

    _XmPushWidgetExtData(new_w, extData,
                         ((XmVendorShellExtObject)newSec)->ext.extensionType);
    memcpy(reqSec, newSec, size);
   
    _XmExtImportArgs(newSec, args, num_args); 

    xmDesktopClass->core_class.initialize(reqSec, newSec, args, num_args);

      }
#ifdef DEBUG
    else
      _XmWarning(NULL, "No desktop parent");
#endif /* DEBUG */    

    return ;

}

/************************************************************************
 *
 *  InitializePrehook
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
InitializePrehook( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InitializePrehook(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmBaseClassExt		*cePtr;
    XtInitProc         		secondaryCreate;

    cePtr = _XmGetBaseClassExtPtr(XtClass(new_w), XmQmotif);

    if ((secondaryCreate = (*cePtr)->secondaryObjectCreate) != NULL)
	(*secondaryCreate)(req, new_w, args, num_args);
}


/************************************************************************
 *
 *     VendorExtInitialize
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
VendorExtInitialize( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
VendorExtInitialize(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmFontList                  defaultFont;
    XmVendorShellExtObject	ve;
    XmVendorShellExtObject	req_ve;
    XmVendorShellExtObjectClass	vec = (XmVendorShellExtObjectClass) XtClass(new_w);
    Atom			delete_atom;
    XtCallbackProc		delete_window_handler;
    Widget			extParent;
    Atom			offset_atom, mwm_messages;
    XmShellExtObjectClass	sec = (XmShellExtObjectClass) XtClass(new_w);
    XtEventHandler		handler;

    ve  = (XmVendorShellExtObject) new_w;
    req_ve  = (XmVendorShellExtObject) req;

    ve->shell.lastConfigureRequest = 0;
	
    extParent = ve->ext.logicalParent;

    /* add the handler for tracking whether the hierarchy has focus */

    XtInsertEventHandler(extParent, 
			 (EventMask)FocusChangeMask | EnterWindowMask | LeaveWindowMask,
			 FALSE,
			 _XmTrackShellFocus, 
			 (XtPointer)new_w,
			 XtListHead);

    handler = sec->shell_class.structureNotifyHandler;
    if (handler)
      {
	  XtInsertEventHandler(extParent, 
			       (EventMask) StructureNotifyMask,
			       TRUE, 
			       handler, 
			       (XtPointer)new_w,
			       XtListHead);
      }
#ifdef DEBUG
    else
      XtError("No structure notify handler for shell");
#endif /* DEBUG */

    ve->vendor.lastOffsetSerial =
      ve->vendor.lastMapRequest = 0;

    ve->vendor.xAtMap =
      ve->vendor.yAtMap =
	ve->vendor.xOffset =
	  ve->vendor.yOffset = 0;
/*
    XtAddCallback((Widget) ve, XmNrealizeCallback,
		  _XmVendorExtRealize,
		  NULL);
*/
    _XmAddCallback((InternalCallbackList *) &(ve->vendor.realize_callback),
                     _XmVendorExtRealize,
                     NULL);

    ve->vendor.externalReposition = False;
#ifdef notdef
    ve->vendor.mapStyle = _XmRAW_MAP;
#endif /* notdef */
    extParent = ve->ext.logicalParent;

    ve->vendor.focus_data = (XmFocusData) _XmCreateFocusData();

    switch (ve->vendor.delete_response){
      case XmUNMAP:
      case XmDESTROY:
      case XmDO_NOTHING:
	break;
      default:
	_XmWarning(NULL, MSG1);
	ve->vendor.delete_response = XmDESTROY;
    }

    XtAddCallback(extParent, XmNpopupCallback, PopupCallback,(XtPointer)new_w); 
    XtAddCallback(extParent, XmNpopdownCallback, PopdownCallback,(XtPointer)new_w); 

    offset_atom = XmInternAtom(XtDisplay(extParent), 
			       _XA_MOTIF_WM_OFFSET, 
			       FALSE);

    mwm_messages = XmInternAtom(XtDisplay(extParent), 
				_XA_MOTIF_WM_MESSAGES, 
				FALSE),

    delete_atom = XmInternAtom(XtDisplay(extParent), 
			       _XA_WM_DELETE_WINDOW,
			       FALSE);

    XmAddWMProtocols(extParent, &mwm_messages, 1);

    XmAddProtocols(extParent,
		   mwm_messages,
		   &offset_atom, 1);

    XmAddProtocolCallback( extParent,
			  mwm_messages, 
			  offset_atom,
			  vec->vendor_class.offset_handler,
			  (XtPointer) ve);
    
    /*
     * add deleteWindow stuff
     */
    XmAddWMProtocols(extParent, &delete_atom, 1);

    /* add a post hook for delete response */

    delete_window_handler = vec->vendor_class.delete_window_handler;

    XmSetWMProtocolHooks( extParent, 
			 delete_atom, NULL, NULL, 
			 delete_window_handler, (XtPointer) ve);


    /* initialize the old_managed field for focus change tracking */

    ve->vendor.old_managed = NULL;

    /* initialize the mwm_hints flags */
    ve->vendor.mwm_hints.flags = 0;

    SetMwmStuff( NULL, (XmVendorShellExtObject) new_w);

    if ((ve->vendor.focus_policy != XmEXPLICIT) &&
	(ve->vendor.focus_policy != XmPOINTER))
      {
	  ve->vendor.focus_policy = XmEXPLICIT;
      }
    
    /* initialize input manager resources */

    ve->vendor.input_method_string = 
			XtNewString(req_ve->vendor.input_method_string);
    ve->vendor.preedit_type_string = 
			XtNewString(req_ve->vendor.preedit_type_string);
    defaultFont =  ve->vendor.button_font_list;
    if ( !defaultFont )
      {
	defaultFont =  ve->vendor.default_font_list; /* backward compatibility */
	if ( !defaultFont )
	  defaultFont = _XmGetDefaultFontList( (Widget) extParent, XmBUTTON_FONTLIST);
      }
    ve->vendor.button_font_list = XmFontListCopy (defaultFont);
  
    defaultFont =  ve->vendor.label_font_list;
    if ( !defaultFont )
      {      
	defaultFont =  ve->vendor.default_font_list; /* backward compatibility */
	if ( !defaultFont )
	  defaultFont = _XmGetDefaultFontList( (Widget) extParent, XmLABEL_FONTLIST);
      }
    ve->vendor.label_font_list = XmFontListCopy (defaultFont);

    defaultFont =  ve->vendor.text_font_list;
    if ( !defaultFont )
      {       
	defaultFont =  ve->vendor.default_font_list; /* backward compatibility */
	if ( !defaultFont )
	  defaultFont = _XmGetDefaultFontList( (Widget) extParent, XmTEXT_FONTLIST);
      }
    ve->vendor.text_font_list = XmFontListCopy (defaultFont);
    ve->vendor.im_height = 0;
    ve->vendor.im_vs_height_set = False;
    ve->vendor.im_info = NULL;

}

/************************************************************************
 *
 *  Initialize
 *
 ************************************************************************/
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
    VendorShellWidget		vw = (VendorShellWidget)new_w;
    XmWidgetExtData		extData;
    Screen                      *screen;
    char                        *pixmap_name;

    if ((extData = _XmGetWidgetExtData((Widget) vw, XmSHELL_EXTENSION)) != NULL)
      {
	  VendorExtInitialize(extData->reqWidget,
			      extData->widget,
			      args,
			      num_args);
      }

    _XmDisplayHandle = XtDisplay(new_w);

    /* Keep count of the number of VendorShells. When it reaches zero
     * we will destroy the XmDisplay object in Destroy()
     */
    if (!XmIsDisplay(new_w)) {
	XmDisplay xmDisplay = (XmDisplay) XmGetXmDisplay (XtDisplay(new_w));

	xmDisplay->display.shellCount += 1;
	/*
	 * CR 1914 - Check to see if the user has supplied a 
	 * pixmap for the window manager to use.  If so, create the pixmap.
	 * This is done within this if statement to ensure that
	 * an infinite loop does not occur when making the XmDisplay.
	 */
        if ((pixmap_name = _XmGetIconPixmapName()) != NULL)
          {
              screen = vw->core.screen;
              vw->wm.wm_hints.icon_pixmap =
                  _XmGetPixmap (screen, pixmap_name, 1, 1, 0);
              _XmClearIconPixmapName();
	  }
	 /* End Fix 1914 */
    }
}

/************************************************************************
 *
 *  InitializePosthook
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
InitializePosthook( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InitializePosthook(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmWidgetExtData	ext;

    if ((ext = _XmGetWidgetExtData(new_w, XmSHELL_EXTENSION)) != NULL) {
	_XmExtObjFree((XtPointer) ext->reqWidget);
	ext->reqWidget = NULL;
	/* extData gets freed at destroy */
    }
}



/************************************************************************
 *
 *  SetValuesPrehook
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetValuesPrehook( old, ref, new_w, args, num_args )
        Widget old ;
        Widget ref ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesPrehook(
        Widget old,
        Widget ref,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmWidgetExtData	oldExtData, newExtData;
    XmBaseClassExt      *cePtr;
    WidgetClass         ec;
    Cardinal            extSize;

    cePtr = _XmGetBaseClassExtPtr(XtClass(new_w), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;
    extSize = ec->core_class.widget_size;

    oldExtData = _XmGetWidgetExtData(new_w, XmSHELL_EXTENSION);
    newExtData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
    
    if (oldExtData && newExtData) {
	_XmPushWidgetExtData(new_w, newExtData, XmSHELL_EXTENSION);

    newExtData->widget = oldExtData->widget;

    newExtData->oldWidget = (Widget) _XmExtObjAlloc(extSize);
    memcpy((char *)newExtData->oldWidget, (char *)oldExtData->widget, extSize);

    XtSetSubvalues(oldExtData->widget,
                   ec->core_class.resources,
                   ec->core_class.num_resources,
                   args, *num_args);

    newExtData->reqWidget = (Widget) _XmExtObjAlloc(extSize);
    memcpy((char *)newExtData->reqWidget, (char *)oldExtData->widget, extSize);

    /*  Convert the necessary fields from unit values to pixel values  */
     oldExtData->widget->core.widget_class = ec;
    _XmExtImportArgs(oldExtData->widget, args, num_args);

    }
    return FALSE;
}

/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
VendorExtSetValues( old, ref, new_w, args, num_args )
        Widget old ;
        Widget ref ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
VendorExtSetValues(
        Widget old,
        Widget ref,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmVendorShellExtPartPtr	ove, nve;
    XmVendorShellExtObject 	ov = (XmVendorShellExtObject) old;
    XmVendorShellExtObject 	nv = (XmVendorShellExtObject) new_w;
    XmFontList                  defaultFont;

    ove = (XmVendorShellExtPartPtr) &(ov->vendor);
    nve = (XmVendorShellExtPartPtr) &(nv->vendor);

    switch (nve->delete_response){
      case XmUNMAP:
      case XmDESTROY:
      case XmDO_NOTHING:
	break;
      default:

#ifdef I18N_MSG
	XtWarning(MSG1);
#else
	XtWarning("invalid value for delete response");
#endif

	nve->delete_response = XmDESTROY;
    }

    if ((nve->focus_policy != XmEXPLICIT) &&
	(nve->focus_policy != XmPOINTER))
      {
	  nve->focus_policy = 
	    ove->focus_policy;
      }
    
    if (nve->focus_policy != ove->focus_policy)
      {
  	  _XmFocusModelChanged( nv->ext.logicalParent, NULL, 
  			       (XtPointer)(unsigned long)nve->focus_policy );
      }
    
    SetMwmStuff(ov, nv);

    if (nve->input_method_string != ove->input_method_string)
    {
	XtFree(ove->input_method_string);
	nve->input_method_string = XtNewString(nve->input_method_string);
    }

    if (nve->preedit_type_string != ove->preedit_type_string)
    {
	XtFree(ove->preedit_type_string);
	nve->preedit_type_string = XtNewString(nve->preedit_type_string);
    }

	if (nve->button_font_list != ove->button_font_list)
	{
		XmFontListFree(ove->button_font_list);
		defaultFont = nve->button_font_list;
		if (!defaultFont)
			{
			defaultFont = nve->default_font_list;
			if (!defaultFont )
			   defaultFont = _XmGetDefaultFontList( (Widget) new_w,
                                            XmBUTTON_FONTLIST);
			}
	nve->button_font_list = XmFontListCopy (defaultFont);
	}

	if (nve->label_font_list != ove->label_font_list)
	{
		XmFontListFree(ove->label_font_list);
		defaultFont = nve->label_font_list;
		if (!defaultFont)
			{
			defaultFont = nve->default_font_list;
			if (!defaultFont )
			   defaultFont = _XmGetDefaultFontList( (Widget) new_w,
                                            XmLABEL_FONTLIST);
			}
	nve->label_font_list = XmFontListCopy (defaultFont);
	}

	if (nve->text_font_list != ove->text_font_list)
	{
		XmFontListFree(ove->text_font_list);
		defaultFont = nve->text_font_list;
		if (!defaultFont)
			{
			defaultFont = nve->default_font_list;
			if (!defaultFont )
			   defaultFont = _XmGetDefaultFontList( (Widget) new_w,
                                            XmTEXT_FONTLIST);
			}
	nve->text_font_list = XmFontListCopy (defaultFont);
	}

    return FALSE;
}

/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValues( current, req, new_w, args, num_args )
        Widget current ;
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget current,
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{

    VendorShellWidget		vw = (VendorShellWidget)new_w;
#ifdef notyet
    VendorShellWidget		ovw = (VendorShellWidget)current;
#endif /* notyet */
    XmWidgetExtData		extData;
    XmVendorShellExtObject	vse;
    Screen                      *screen;
    char                        *pixmap_name;

    /* This fixes a latency deficiency of setting iconPixmap via in
     * line resource conversion and XtVaSetValues().  We must pull
     * out the value (finally - the converter only socks away the name)
     * and notify the window manager.
     */
    if (pixmap_name = _XmGetIconPixmapName())
      {
	screen = vw->core.screen;
	vw->wm.wm_hints.icon_pixmap =
	  _XmGetPixmap(screen, pixmap_name, 1,
		       BlackPixelOfScreen(screen),
		       WhitePixelOfScreen(screen));

	vw->wm.wm_hints.flags |= IconPixmapHint;

        if(    XtIsRealized(vw) && !vw->shell.override_redirect    )
          {
            XSetWMHints(XtDisplay(vw), XtWindow(vw), &vw->wm.wm_hints);
          } 
	_XmClearIconPixmapName();
      }

    if ((extData = _XmGetWidgetExtData( (Widget) vw, XmSHELL_EXTENSION)) &&
	(vse = (XmVendorShellExtObject) extData->widget))
      {
	  VendorExtSetValues(extData->oldWidget,
			     extData->reqWidget,
			     extData->widget,
			     args,
			     num_args);

	    vse = (XmVendorShellExtObject) extData->widget;
	    if (req->core.height != current->core.height)
		vse->vendor.im_vs_height_set = True;

      }
#ifdef notyet
    if (ovw->transient.transient_for != vw->transient.transient_for)
      {
	  /* update the transient_for tree */
      }
#endif /* notyet */

	return(FALSE);
}


/************************************************************************
 *
 *  SetValuesPosthook
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValuesPosthook( current, req, new_w, args, num_args )
        Widget current ;
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesPosthook(
        Widget current,
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmWidgetExtData	ext;

    _XmPopWidgetExtData(new_w, &ext, XmSHELL_EXTENSION);

    if (ext) {
	_XmExtObjFree( (XtPointer) ext->reqWidget);
	_XmExtObjFree( (XtPointer) ext->oldWidget);
	XtFree((char *) ext);
    }
	return(FALSE);
}


/************************************************************************
 *
 *  GetValuesPrehook
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetValuesPrehook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesPrehook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmWidgetExtData	oldExtData, newExtData;

    if ((oldExtData = _XmGetWidgetExtData(w, XmSHELL_EXTENSION)) != NULL) {
	newExtData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
	newExtData->widget = oldExtData->widget;
	_XmPushWidgetExtData(w, newExtData, XmSHELL_EXTENSION);
    }
}

static void 
#ifdef _NO_PROTO
GetValuesHook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesHook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmWidgetExtData	ext;
    XmBaseClassExt      *cePtr;
    WidgetClass         ec;

    cePtr = _XmGetBaseClassExtPtr(XtClass(w), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;

    if ((ext = _XmGetWidgetExtData(w, XmSHELL_EXTENSION)) != NULL)
    {
      XtGetSubvalues(ext->widget,
                     ec->core_class.resources,
                     ec->core_class.num_resources,
                     args, *num_args);
      _XmExtGetValuesHook( ext->widget, args, num_args );
    }

}

static void 
#ifdef _NO_PROTO
GetValuesPosthook( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesPosthook(
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmWidgetExtData	ext = NULL;

    _XmPopWidgetExtData(w, &ext, XmSHELL_EXTENSION);
    if (ext)
      XtFree((char *) ext);
}

/*
 * This handles the case where the secondary shells is waiting for the
 * primary to get mapped and is destroyed in the interim.
 */
static void 
#ifdef _NO_PROTO
PendingTransientDestroyed( vw, cl_data, ca_data )
        Widget vw ;
        XtPointer cl_data ;
        XtPointer ca_data ;
#else
PendingTransientDestroyed(
        Widget vw,
        XtPointer cl_data,
        XtPointer ca_data )
#endif /* _NO_PROTO */
{
        XmExtObject ancestorExtObj = (XmExtObject) cl_data ;
	Widget ancestor = ancestorExtObj->ext.logicalParent ;

    if(    !ancestor->core.being_destroyed    )
    {   
/*
        XtRemoveCallback( (Widget) ancestorExtObj, XmNrealizeCallback, 
                                             SetTransientFor, (XtPointer) vw) ;
*/
;;        _XmRemoveCallback((InternalCallbackList *) &(((XmVendorShellExtObject)ancestorExtObj)->vendor.realize_callback),
                          SetTransientFor, (XtPointer) vw) ;
        } 
    return ;
    }

/*
 * Handle having the application shell realized after the secondary shells
 */
static void 
#ifdef _NO_PROTO
SetTransientFor( w, closure, call_data)
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
SetTransientFor(
        Widget w,
        XtPointer closure,
        XtPointer call_data)
#endif /* _NO_PROTO */
{   
        VendorShellWidget vw = (VendorShellWidget) closure ;
	Widget ancestor = ((XmExtObject) w)->ext.logicalParent ;
        Arg args[2] ;
        Cardinal i = 0 ;

    if(    !XtIsRealized( ancestor)    )
    {   
        XtRealizeWidget( ancestor) ;
        } 
    XtSetArg( args[i], XtNwindowGroup, XtWindow( ancestor)) ; i++ ;

    if(    XtIsTransientShell( (Widget) vw)    )
    {   
        /* because Shell.c is broken force the code */
        ((TransientShellWidget) vw)->transient.transient_for = NULL ;

        XtSetArg( args[i], XtNtransientFor, ancestor) ; i++ ;
        }
    XtSetValues( (Widget) vw, args, i) ;
/*
    XtRemoveCallback( w, XmNrealizeCallback, SetTransientFor, (XtPointer) vw) ;
*/

    _XmRemoveCallback((InternalCallbackList *) &(((XmVendorShellExtObject)w)->vendor.realize_callback),
                          SetTransientFor, (XtPointer) vw) ;

    XtRemoveCallback( (Widget) vw, XmNdestroyCallback, 
                                    PendingTransientDestroyed, (XtPointer) w) ;
    return ;
    }


/************************************************************************
 *
 *  Resize
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( w )
        Widget w ;
#else
Resize(
        Widget w )
#endif /* _NO_PROTO */
{
    register ShellWidget sw = (ShellWidget)w;    
    Widget childwid;
    int i;
    int y;
    XmVendorShellExtObject vendorExt;
    XmWidgetExtData	extData;

    extData = _XmGetWidgetExtData((Widget)sw, XmSHELL_EXTENSION);
    vendorExt = (XmVendorShellExtObject) extData->widget;

    _XmImResize((Widget)sw);
    y = sw->core.height - vendorExt->vendor.im_height;
    for(i = 0; i < sw->composite.num_children; i++) {
        if(XtIsManaged(sw->composite.children[i])) {
	    childwid = sw->composite.children[i];
	    XtResizeWidget(childwid, sw->core.width, y,
                           childwid->core.border_width);
        }
    }
}

/************************************************************************
 *
 *  ChangeManaged
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
    VendorShellWidget vw = (VendorShellWidget) wid ;
    WMShellWidgetClass	super = (WMShellWidgetClass)wmShellWidgetClass;
    Widget		firstManaged = NULL;
    Cardinal		i;
    XmVendorShellExtObject vendorExt;
    XmWidgetExtData	extData;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    vendorExt = (XmVendorShellExtObject) extData->widget;

    for (i= 0; i < vw->composite.num_children; i++)
      if (XtIsManaged(vw->composite.children[i]))
	firstManaged = vw->composite.children[i];

    /* Danger! Danger! Ugly Code Warning! Since the shell's
     * change Managed routine in the intrinsics will always
     * configure the child to be the size of the parent, if
     * there is any im_height we must subtract it here and
     * add it back in after this call.
     */

    vw->core.height -= vendorExt->vendor.im_height;
    (*(super->composite_class.change_managed)) ((Widget) vw);
    vw->core.height += vendorExt->vendor.im_height;

    /*
     * make sure that there is a reasonable initial focus path. This
     * is especially important for making sure the event handler is
     * there.
     */
    XtSetKeyboardFocus((Widget)vw, (Widget)firstManaged);
    
    _XmNavigChangeManaged((Widget)vw);
}
 
static void 
#ifdef _NO_PROTO
UpdateCoreGeometry( vw, vendorExt )
	VendorShellWidget vw ;
	XmVendorShellExtObject vendorExt ;
#else
UpdateCoreGeometry(
	VendorShellWidget vw,
	XmVendorShellExtObject vendorExt )
#endif /* _NO_PROTO */
{
	/* ||| check if geometry was user specified and convert if it was */
	if (vw->shell.geometry && vendorExt)
	{
		if (vendorExt->vendor.unit_type != XmPIXELS)
		{
			if (vw->wm.size_hints.flags & USPosition)
			{
				vw->core.x = (Position) XmCvtToHorizontalPixels(
					vw->core.screen, (int) vw->core.x,
					vendorExt->vendor.unit_type);
				vw->core.y = (Position) XmCvtToVerticalPixels(
					vw->core.screen, (int) vw->core.y,
					vendorExt->vendor.unit_type);
			}
			if (vw->wm.size_hints.flags & USSize)
			{
				vw->core.width = (Dimension) XmCvtToHorizontalPixels(
					vw->core.screen, (int) vw->core.width,
					vendorExt->vendor.unit_type);
				vw->core.height = (Dimension) XmCvtToVerticalPixels(
					vw->core.screen, (int) vw->core.height,
					vendorExt->vendor.unit_type);
			}
		}
	}
}

/************************************************************************
 *
 *  Realize
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Realize( wid, vmask, attr )
        Widget wid ;
        XtValueMask *vmask ;
        XSetWindowAttributes *attr ;
#else
Realize(
        Widget wid,
        XtValueMask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    VendorShellWidget vw = (VendorShellWidget) wid ;
    WMShellWidgetClass	super = (WMShellWidgetClass)wmShellWidgetClass;
    XmVendorShellExtObject vendorExt;
    XmWidgetExtData	extData;
    
    if ((extData = _XmGetWidgetExtData((Widget)vw,XmSHELL_EXTENSION)) != NULL) {
	vendorExt = (XmVendorShellExtObject) extData->widget;
	_XmImChangeManaged((Widget)vw);
    }
    else
      vendorExt = NULL;

#ifdef notdef
    /* ||| Call Shell's (via wmShell) changeManaged routine so that we
     * get the geometry string parsed and other misc stuff done. This
     * should not be in shell's changeManaged routine. We be confused
     * if child is not yet instantiated.  ||| 
     */
    (*(super->composite_class.change_managed)) (vw);
#endif
    
    UpdateCoreGeometry(vw, vendorExt);
    
    /*
     * Set nearest shell as transientFor so Mwm will be able to build tree.
     */
    if (vendorExt && XmIsShellExt(vendorExt->desktop.parent))
      {  
	  Widget ancestor = ((XmExtObject)(vendorExt->desktop.parent))
                                                          ->ext.logicalParent ;
	  /* try to have WMShell do the work */
	  if (XtIsRealized(ancestor))
	    vw->wm.wm_hints.window_group = XtWindow(ancestor);
	  else
            {
              XmWidgetExtData ancestorExtData = _XmGetWidgetExtData( ancestor,
                                                           XmSHELL_EXTENSION) ;
              if(    ancestorExtData && ancestorExtData->widget    )
                {   
                  _XmAddCallback((InternalCallbackList *) &(((XmVendorShellExtObject)ancestorExtData->widget)->vendor.realize_callback),
                   SetTransientFor,
                     (XtPointer) vw);

                  XtAddCallback( (Widget) vw, XmNdestroyCallback, 
                                       PendingTransientDestroyed,
                                         (XtPointer) ancestorExtData->widget) ;
                } 
	    }
      }

    /*	Make sure height and width are not zero, without warning, since
	we change the internal behavior of BB and DA and we don't want
	to change the external one.
    */
   if (!XtWidth(wid)) XtWidth(wid) = 1 ;
   if (!XtHeight(wid)) XtHeight(wid) = 1 ;
#ifdef NON_OSF_FIX
    /*  if the height is even with im_height, 1 pixel should be added,
	so that the height cannot be 0, when im_height is changed to 0.
    */
   else if (vendorExt && vendorExt->vendor.im_height &&
           (XtHeight(wid) == vendorExt->vendor.im_height))
      XtHeight(wid) += 1 ;
#endif /* NON_OSF_FIX */
    
    /* Make my superclass do all the dirty work */
    (*super->core_class.realize) ((Widget) vw, vmask, attr);
    if (vendorExt)
      _XmImRealize((Widget)vw);
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
    XtGeometryResult res ;

    if (!(extData = _XmGetWidgetExtData((Widget)shell,
					XmSHELL_EXTENSION)))
      return XtGeometryNo;

    ve = (XmVendorShellExtObject) extData->widget;

    if(!(shell->shell.allow_shell_resize) && XtIsRealized(wid) &&
       (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)))
      return(XtGeometryNo);

    my_request.request_mode = 0;
    /* %%% worry about XtCWQueryOnly */
    if (request->request_mode & XtCWQueryOnly)
      my_request.request_mode |= XtCWQueryOnly;

    if (request->request_mode & CWWidth) {
	my_request.width = request->width;
	my_request.request_mode |= CWWidth;
    }
    if (request->request_mode & CWHeight) {
	my_request.height = request->height + ve->vendor.im_height;
	my_request.request_mode |= CWHeight;
    }
    if (request->request_mode & CWBorderWidth) {
	my_request.border_width = request->border_width;
	my_request.request_mode |= CWBorderWidth;
    }

  if (request->request_mode & CWX) {
    my_request.x = request->x;
    my_request.request_mode |= CWX;
  }
  if (request->request_mode & CWY) {
    my_request.y = request->y;
    my_request.request_mode |= CWY;
  }

    res = XtMakeGeometryRequest((Widget)shell, &my_request, NULL) ;
    
    if (res == XtGeometryYes)
      {
	  _XmImResize((Widget)shell);
	  if (!(request->request_mode & XtCWQueryOnly))
	    {
		if (request->request_mode & CWWidth)
			wid->core.width = shell->core.width;
		if (request->request_mode & CWHeight)
			wid->core.height = shell->core.height -
						ve->vendor.im_height;
		  if (request->request_mode & CWX) 
			      wid->core.x = 0;
		  if (request->request_mode & CWY) 
			      wid->core.y = 0;
	    }
	  return XtGeometryYes;
      } 
    else 
      return XtGeometryNo;
}


/************************************************************************
 *
 *  _XmRootGeometryManager
 *
 ************************************************************************/
/*ARGSUSED*/
XtGeometryResult 
#ifdef _NO_PROTO
_XmRootGeometryManager( w, request, reply )
        Widget w ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
_XmRootGeometryManager(
        Widget w,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
    XmWidgetExtData	extData = _XmGetWidgetExtData(w, XmSHELL_EXTENSION);
    XmShellExtObject	se = (XmShellExtObject)extData->widget;
    XtGeometryHandler	wmGeoHandler;
    ShellWidgetClass	swc = (ShellWidgetClass)wmShellWidgetClass;
    ShellClassExtensionRec **scExtPtr;
    XtGeometryResult	returnVal = XtGeometryNo;
    WMShellWidget	wmShell = (WMShellWidget)w;

    if (se)
      {
	  se->shell.lastConfigureRequest = NextRequest(XtDisplay(w));
      }
#ifdef DEBUG
    else
      XtError("no extension object");
#endif /* DEBUG */
    
    scExtPtr = (ShellClassExtensionRec **)
      _XmGetClassExtensionPtr( (XmGenericClassExt *) &(swc->shell_class.extension),
			      NULLQUARK);
    if (request->request_mode & XtCWQueryOnly)
      {
	  if (!(wmShell->shell.allow_shell_resize) &&
	      (request->request_mode & 
	       (CWWidth | CWHeight | CWBorderWidth)))
	    return XtGeometryNo;
	  /*
	   * we should switch on useAsyncGeometry but we won't |||
	   */
	  else 
	    return XtGeometryYes;
      }

    if (se->shell.useAsyncGeometry)
      {
	  /* make wait_for_wm = FALSE to force desired behaviour */
	  wmShell->wm.wait_for_wm = FALSE;
	  /* FIX for 1684: remove the timeout = 0 line, not
	     needed and introduced a bug if not saved/restore together
	     with useAsync change - wait_for_wm will be later reset
	     by Shell in Xt */
      }
    if ((wmGeoHandler = (*scExtPtr)->root_geometry_manager) != NULL)
      {
	  returnVal =  (*wmGeoHandler)(w, request, reply);
	  if (se->shell.useAsyncGeometry) {
	      /* X configure was sent to the server, while this is happening,
		 let's everybody think it's a success (which is true
		 most of the time): set the shell size to what it wants
		 to be and return Yes */
	      if (request->request_mode & CWWidth)
		  w->core.width = request->width;
	      if (request->request_mode & CWHeight)
		  w->core.height = request->height ;
	      if (request->request_mode & CWBorderWidth)
		  w->core.border_width = request->border_width ;
	      if (request->request_mode & CWX) 
		  w->core.x = request->x;
	      if (request->request_mode & CWY) 
		  w->core.y = request->y;
	      
	      returnVal = XtGeometryYes;
	  }
      }
    return returnVal;
}

/************************************************************************
 *
 *  SetMwmHints
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetMwmHints( ve )
        XmVendorShellExtObject ve ;
#else
SetMwmHints(
        XmVendorShellExtObject ve )
#endif /* _NO_PROTO */
{
    PropMwmHints	prop;
    Atom		mwm_hints_atom;
    Widget		shell = ve->ext.logicalParent;

    mwm_hints_atom = XmInternAtom(XtDisplay(shell),
				  _XA_MWM_HINTS, 
				  FALSE);

#define SET(field) prop.field = ve->vendor.mwm_hints.field
    SET(flags);
    SET(functions);
    SET(decorations);
    prop.inputMode = ve->vendor.mwm_hints.input_mode;
#undef SET

    XChangeProperty (XtDisplay(shell), 
		     XtWindow(shell),
		     mwm_hints_atom,mwm_hints_atom, 
		     32, PropModeReplace, 
		     (unsigned char *) &prop, PROP_MWM_HINTS_ELEMENTS);
}	




/************************************************************************
 *
 *  SetMwmMenu
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetMwmMenu( ve )
        XmVendorShellExtObject ve ;
#else
SetMwmMenu(
        XmVendorShellExtObject ve )
#endif /* _NO_PROTO */
{
    Widget		shell = ve->ext.logicalParent;
    Atom		mwm_menu_atom;


    mwm_menu_atom = XmInternAtom(XtDisplay(shell),
				  _XA_MWM_MENU, 
				  FALSE);

    XChangeProperty (XtDisplay(shell), 
		     XtWindow(shell),
		     mwm_menu_atom, mwm_menu_atom, 
		     8, PropModeReplace, 
		     (unsigned char *) ve->vendor.mwm_menu, 
		     strlen(ve->vendor.mwm_menu) + 1);
}


void 
#ifdef _NO_PROTO
_XmVendorExtRealize( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
_XmVendorExtRealize(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject	ve = (XmVendorShellExtObject)w;
    VendorShellWidget		vw;

    vw = (VendorShellWidget)ve->ext.logicalParent;
    if (ve->vendor.mwm_hints.flags)
      SetMwmHints(ve);
    
    if (ve->vendor.mwm_menu)
      SetMwmMenu(ve);

    _XmInstallProtocols(ve->ext.logicalParent);

    if(    !IsPopupShell( (Widget) vw)    )
      {   
        /* Non-popup shells are allowed input as soon as
         * they are realized, since they don't use XtPopup.
         */
        AddGrab(ve, NULL, FALSE, FALSE, ve);
      } 
}

/************************************************************************
 *
 *  XmIsMotifWMRunning
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmIsMotifWMRunning( shell )
        Widget shell ;
#else
XmIsMotifWMRunning(
        Widget shell )
#endif /* _NO_PROTO */
{
    Atom	motif_wm_info_atom;
    Atom	actual_type;
    int		actual_format;
    unsigned long num_items, bytes_after;
    PropMotifWmInfo	*prop = 0;
    Window	root = RootWindowOfScreen(XtScreen(shell));

    motif_wm_info_atom = XmInternAtom(XtDisplay(shell),
				      _XA_MOTIF_WM_INFO,
				      FALSE);

    XGetWindowProperty (XtDisplay(shell), 
			root,
			motif_wm_info_atom,
			0, (long)PROP_MOTIF_WM_INFO_ELEMENTS,
			FALSE, motif_wm_info_atom,
			&actual_type, &actual_format,
			&num_items, &bytes_after,
			(unsigned char **) &prop);

    if ((actual_type != motif_wm_info_atom) ||
	(actual_format != 32) ||
	(num_items < PROP_MOTIF_WM_INFO_ELEMENTS))
      {
	  if (prop != 0) XFree((char *)prop);
	  return (FALSE);
      }
    else
      {
	  Window	wm_window = (Window) prop->wmWindow;
	  Window	top, parent, *children;
	  unsigned int	num_children;
	  Boolean	returnVal;
	  Cardinal	i;

	  if (XQueryTree(XtDisplay(shell),
			 root, &top, &parent,
			 &children, &num_children))
	    {
		for (i = 0; 
		     i < num_children && children[i] != wm_window;
		     i++);
		returnVal =  (i == num_children) ? FALSE : TRUE;
	    }
	  else
	    returnVal = FALSE;

	  if (prop) XFree((char *)prop);
	  if (children) XFree((char *)children);
	  return (returnVal);
      }
}
	
static void
#ifdef _NO_PROTO
AddDLEntry( ve, shell )
        XmVendorShellExtObject ve ;
        Widget shell ;
#else
AddDLEntry(
        XmVendorShellExtObject ve,
        Widget shell)
#endif
{
  unsigned short i = 0 ;
  while(    i < destroy_list_cnt    )
    {
      if(    destroy_list[i].shell == shell    )
        {   
          /* Already on list; once is enough.
          */
          return ;
        } 
      ++i ;
    } 
  if(    destroy_list_cnt == destroy_list_size    )
    {
      destroy_list_size += 2 ;
      destroy_list = (XmDestroyGrabList) XtRealloc( (char *) destroy_list,
                               destroy_list_size * sizeof( XmDestroyGrabRec)) ;
    } 
  destroy_list[i].shell = shell ;
  destroy_list[i].ve = ve ;
  ++destroy_list_cnt ;
} 

static void
#ifdef _NO_PROTO
RemoveDLEntry( pos )
        unsigned pos ;
#else
RemoveDLEntry(
        unsigned pos)
#endif
{
  while(    ++pos < destroy_list_cnt    )
    {
      destroy_list[pos-1].shell = destroy_list[pos].shell ;
      destroy_list[pos-1].ve = destroy_list[pos].ve ;
    }
  --destroy_list_cnt ;
} 

static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid)
#endif /* _NO_PROTO */
{
  XmWidgetExtData ext;
  XmVendorShellExtObject ve;
  unsigned short n = 0 ;

  while(    n < destroy_list_cnt    )
  {
    if(    destroy_list[n].shell == wid    )
      { 
        RemoveGrab( destroy_list[n].ve, TRUE, destroy_list[n].shell) ;
        RemoveDLEntry( n) ;
        break ;
      } 
    ++n ;
  } 

    _XmPopWidgetExtData(wid, &ext, XmSHELL_EXTENSION);
    if (ext != NULL) {
	if ((ve = (XmVendorShellExtObject) ext->widget) != NULL) {
	    if (ve->vendor.mwm_menu)
		XtFree(ve->vendor.mwm_menu);
	    if (ve->vendor.input_method_string)
		XtFree(ve->vendor.input_method_string);
            if (ve->vendor.im_info)
              {
                _XmImFreeData( ve->vendor.im_info) ;
              }
	    if (ve->vendor.preedit_type_string)
		XtFree(ve->vendor.preedit_type_string);
	    if (ve->vendor.button_font_list)
		XmFontListFree(ve->vendor.button_font_list);
	    if (ve->vendor.label_font_list)
		XmFontListFree(ve->vendor.label_font_list);
	    if (ve->vendor.text_font_list)
		XmFontListFree(ve->vendor.text_font_list);

	    _XmDestroyFocusData(ve->vendor.focus_data);

	    _XmRemoveAllCallbacks((InternalCallbackList *)
				&(ve->vendor.realize_callback));
	    _XmRemoveAllCallbacks((InternalCallbackList *)
				&(ve->vendor.focus_moved_callback));

	    xmDesktopClass->core_class.destroy((Widget) ve);
	    XtFree((char *) ve);
	}
	XtFree((char *) ext);
    }    

    /*
     * If all VendorShells have been destroyed, destroy the XmDisplay object
     * in order to reset any per-display and per-screen data. This is
     * necessary, since the application may be about to call XtCloseDisplay.
     */
    if (!XmIsDisplay (wid)) {
	XmDisplay xmDisplay = (XmDisplay) XmGetXmDisplay (XtDisplay(wid));

	xmDisplay->display.shellCount -= 1;
	if (xmDisplay->display.shellCount == 0)
	    XtDestroyWidget ((Widget)xmDisplay);
    }
}

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
    _XmImRedisplay(wid);
}


/************************************************************************
 *
 *  _XmGetDefaultDisplay
 *
 ************************************************************************/
Display *
#ifdef _NO_PROTO
_XmGetDefaultDisplay()
#else
_XmGetDefaultDisplay(void)
#endif /* _NO_PROTO */
{
    return _XmDisplayHandle ;
}

#ifdef DEBUG_GRABS

static void
#ifdef _NO_PROTO
PrintModal( modal )
        XmModalData modal ;
#else
PrintModal(
        XmModalData modal)
#endif
{   
  printf( "log_p: %s, grabber: %s, excl: %d, spring: %d\n",
      modal->wid->core.name,
      (modal->grabber
          ? modal->grabber->ext.logicalParent->core.name : "NULL"),
      modal->exclusive,
          modal->springLoaded) ;
}

static void
#ifdef _NO_PROTO
PrintXmGrabs( wid )
        Widget wid ;
#else
PrintXmGrabs(
        Widget wid)
#endif
{   
  XmDisplay disp = (XmDisplay) XmGetXmDisplay( XtDisplay( wid));
  XmModalData modals = disp->display.modals ;
  Cardinal nmodals = disp->display.numModals ;
  Cardinal cnt = 0 ;    

  while(    cnt < nmodals    )
    {   
      PrintModal( &(modals[cnt])) ;
      ++cnt ;
    } 
}

#endif /* DEBUG_GRABS */
