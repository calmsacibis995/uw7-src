#pragma ident	"@(#)m1.2libs:Xm/VendorSE.c	1.3"
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

#include <Xm/VendorSEP.h>
#include <Xm/BaseClassP.h>
#include <X11/ShellP.h>
#include "XmI.h"
#include "MessagesI.h"
#include <Xm/RepType.h>
#include <Xm/AtomMgr.h>

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include <string.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_VendorE,MSG_VE_2,_XmMsgVendorE_0000)
#define MSG6	catgets(Xm_catd,MS_VendorE,MSG_VE_3,_XmMsgVendorE_0005)
#else
#define MSG1	_XmMsgVendorE_0000
#define MSG6	_XmMsgVendorE_0005
#endif


#define DONT_CARE -1L
#define BIGSIZE ((Dimension)32767)

#define	done(value, type) \
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XmImportOperator ExtParentDimToHorizontalPixels() ;
static void ExtParentDimFromHorizontalPixels() ;
static XmImportOperator ExtParentDimToVerticalPixels() ;
static void ExtParentDimFromVerticalPixels() ;
static XmImportOperator ExtParentIntToHorizontalPixels() ;
static void ExtParentIntFromHorizontalPixels() ;
static XmImportOperator ExtParentIntToVerticalPixels() ;
static void ExtParentIntFromVerticalPixels() ;
static Boolean CvtStringToIconPixmap() ;
static Boolean CvtStringToHorizDim() ;
static Boolean CvtStringToHorizPos() ;
static Boolean CvtStringToVertDim() ;
static Boolean CvtStringToVertPos() ;
static Boolean CvtStringToHorizontalInt() ;
static Boolean CvtStringToVerticalInt() ;
static void FetchUnitType() ;
static void RegisterVendorConverters() ;
static void VendorClassInitialize() ;
static void VendorClassPartInitialize() ;
static void DeleteWindowHandler() ;
static void OffsetHandler() ;
static void InitializePrehook() ;
static void VendorDestroy() ;
static void GetMWMFunctionsFromProperty() ;

#else

static XmImportOperator ExtParentDimToHorizontalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static void ExtParentDimFromHorizontalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static XmImportOperator ExtParentDimToVerticalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static void ExtParentDimFromVerticalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static XmImportOperator ExtParentIntToHorizontalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static void ExtParentIntFromHorizontalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static XmImportOperator ExtParentIntToVerticalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static void ExtParentIntFromVerticalPixels( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static Boolean CvtStringToIconPixmap(
                        Display *dpy,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValuePtr fromVal,
                        XrmValuePtr toVal,
                        XtPointer *data) ;
static Boolean CvtStringToHorizDim( 
                        Display *dpy,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *toVal,
                        XtPointer *data) ;
static Boolean CvtStringToHorizPos( 
                        Display *dpy,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *toVal,
                        XtPointer *data) ;
static Boolean CvtStringToVertDim( 
                        Display *dpy,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *toVal,
                        XtPointer *data) ;
static Boolean CvtStringToVertPos( 
                        Display *dpy,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *toVal,
                        XtPointer *data) ;
static Boolean CvtStringToHorizontalInt( 
                        Display *dpy,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *toVal,
                        XtPointer *data) ;
static Boolean CvtStringToVerticalInt( 
                        Display *dpy,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *toVal,
                        XtPointer *data) ;
static void FetchUnitType( 
                        Widget widget,
                        Cardinal *size,
                        XrmValue *value) ;
static void RegisterVendorConverters( void ) ;
static void VendorClassInitialize( void ) ;
static void VendorClassPartInitialize( 
                        WidgetClass w) ;
static void DeleteWindowHandler( 
                        Widget wid,
                        XtPointer closure,
                        XtPointer call_data) ;
static void OffsetHandler( 
                        Widget shell,
                        XtPointer clientData,
                        XtPointer cd) ;
static void InitializePrehook( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void VendorDestroy( 
                        Widget wid) ;
static void GetMWMFunctionsFromProperty( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

static XtConvertArgRec iconArgs[] = {
    { XtWidgetBaseOffset,
        (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen),
        sizeof (Screen*)
    }
};

static XtConvertArgRec resIndConvertArgs[] = {
    { XtProcedureArg, 
      (XtPointer)FetchUnitType, 
      0
    },
    { XtWidgetBaseOffset,
        (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen),
        sizeof (Screen*)
    }
};

/* Fix for 1914 - Add static variable to hold the name of the icon pixmap */
static char * icon_pixmap_name = NULL;



/***************************************************************************
 *
 * Vendor shell class record
 *
 ***************************************************************************/

#define Offset(field) XtOffsetOf( struct _XmVendorShellExtRec, vendor.field)

static XtResource extResources[] =
{
    {
	XmNextensionType,
	XmCExtensionType, XmRExtensionType, sizeof (unsigned char),
	XtOffsetOf( struct _XmExtRec, ext.extensionType),
	XmRImmediate, (XtPointer)XmSHELL_EXTENSION,
    },
    {
	XmNdefaultFontList,
	XmCDefaultFontList, XmRFontList, sizeof (XmFontList),
	Offset (default_font_list),
	XmRImmediate, (XtPointer) NULL, 
    },
    {
        XmNbuttonFontList,
        XmCButtonFontList, XmRFontList, sizeof (XmFontList),
        Offset (button_font_list),
	XmRImmediate, (XtPointer) NULL, 
    },
    {
        XmNlabelFontList,
        XmCLabelFontList, XmRFontList, sizeof (XmFontList),
        Offset (label_font_list),
	XmRImmediate, (XtPointer) NULL, 
    },
    {
        XmNtextFontList,
        XmCTextFontList, XmRFontList, sizeof (XmFontList),
        Offset (text_font_list),
	XmRImmediate, (XtPointer) NULL, 
    },
    {
	XmNaudibleWarning, XmCAudibleWarning, XmRAudibleWarning,
	sizeof (unsigned char), Offset (audible_warning),
	XmRImmediate, (XtPointer) XmBELL,
    },    
    {
	XmNshellUnitType, XmCShellUnitType, XmRUnitType, 
	sizeof (unsigned char), Offset (unit_type),
	XmRImmediate, (XtPointer) XmPIXELS,
    },	
    {
	XmNdeleteResponse, XmCDeleteResponse, 
	XmRDeleteResponse, sizeof(unsigned char),
	Offset(delete_response), 
	XmRImmediate, (XtPointer) XmDESTROY,
    },
    {
	XmNkeyboardFocusPolicy, XmCKeyboardFocusPolicy, XmRKeyboardFocusPolicy, 
	sizeof(unsigned char),
	Offset(focus_policy), 
	XmRImmediate, (XtPointer)XmEXPLICIT,
    },
    { 
	XmNmwmDecorations, XmCMwmDecorations, XmRInt, 
	sizeof(long), Offset(mwm_hints.decorations), 
	XmRImmediate, (XtPointer) DONT_CARE,
    },
    { 
	XmNmwmFunctions, XmCMwmFunctions, XmRInt, 
	sizeof(long), Offset(mwm_hints.functions), 
	XmRImmediate, (XtPointer) DONT_CARE,
    },
    { 
	XmNmwmInputMode, XmCMwmInputMode, XmRInt, 
	sizeof(int), Offset(mwm_hints.input_mode), 
	XmRImmediate, (XtPointer) DONT_CARE,
    },
    { 
	XmNmwmMenu, XmCMwmMenu, XmRString, 
	sizeof(int), Offset(mwm_menu), 
	XmRImmediate, (XtPointer) NULL, 
    },
    { 
	XmNfocusMovedCallback, XmCCallback, XmRCallback, 
	sizeof(XtCallbackList), Offset(focus_moved_callback), 
	XmRImmediate, NULL,
    },
    { 
	XmNrealizeCallback, XmCCallback, XmRCallback, 
	sizeof(XtCallbackList), Offset(realize_callback), 
	XmRImmediate, NULL,
    },
    { 
	XmNinputMethod, XmCInputMethod, XmRString, 
	sizeof(String), Offset(input_method_string), 
	XmRImmediate, NULL,
    },
    { 
	XmNpreeditType, XmCPreeditType, XmRString, 
	sizeof(String), Offset(preedit_type_string), 
	XmRImmediate, "OverTheSpot,OffTheSpot,Root",
    },
    {
      XmNlightThreshold, XmCLightThreshold, XmRInt,
      sizeof(unsigned int), Offset(light_threshold),
      XmRImmediate, NULL,
    },
    {
      XmNdarkThreshold, XmCDarkThreshold, XmRInt,
      sizeof(unsigned int), Offset(dark_threshold),
      XmRImmediate, NULL,
    },
    {
      XmNforegroundThreshold, XmCForegroundThreshold, XmRInt,
      sizeof(unsigned int), Offset(foreground_threshold),
      XmRImmediate, NULL,
    },
};
#undef Offset

/*  Definition for resources that need special processing in get values  */

#define Offset(x) XtOffsetOf( VendorShellRec, x)

static XmSyntheticResource synResources[] =
{
    { 
	XmNx, sizeof (Position),
	Offset (core.x), 
	ExtParentDimFromHorizontalPixels,
	ExtParentDimToHorizontalPixels,
    },
    {
	XmNy, sizeof (Position),
	Offset (core.y), 
	ExtParentDimFromVerticalPixels,
	ExtParentDimToVerticalPixels,
    },
    {
	XmNwidth, sizeof (Dimension),
	Offset (core.width), 
	ExtParentDimFromHorizontalPixels,
	ExtParentDimToHorizontalPixels,
    },
    { 
	XmNheight, sizeof (Dimension),
	Offset (core.height), 
	ExtParentDimFromVerticalPixels,
	ExtParentDimToVerticalPixels,
    },
    {
	XmNborderWidth, sizeof (Dimension),
	Offset (core.border_width), 
	ExtParentDimFromHorizontalPixels,
	ExtParentDimToHorizontalPixels,
    },

/* size_hints minus things stored in core */

    { 
	XmNminWidth, sizeof(int),
	Offset(wm.size_hints.min_width), 
	ExtParentIntFromHorizontalPixels,
	ExtParentIntToHorizontalPixels,
    },	
    { 
	XmNminHeight, sizeof(int),
	Offset(wm.size_hints.min_height), 
	ExtParentIntFromVerticalPixels,
	ExtParentIntToVerticalPixels,
    },
    { 
	XmNmaxWidth, sizeof(int),
	Offset(wm.size_hints.max_width), 
	ExtParentIntFromHorizontalPixels,
	ExtParentIntToHorizontalPixels,
    },	
    { 	
	XmNmaxHeight,sizeof(int),
	Offset(wm.size_hints.max_height),
	ExtParentIntFromVerticalPixels,
	ExtParentIntToVerticalPixels,
    },

/* wm_hints */

    { 
	XmNiconX, sizeof(int),
	Offset(wm.wm_hints.icon_x), 
	ExtParentIntFromHorizontalPixels,
	ExtParentIntToHorizontalPixels,
    },
    { 
	XmNiconY, sizeof(int),
	Offset(wm.wm_hints.icon_y),  
	ExtParentIntFromVerticalPixels,
	ExtParentIntToVerticalPixels,
    },
    { 
	XmNmwmFunctions, sizeof(long),
	XtOffsetOf( struct _XmVendorShellExtRec, vendor.mwm_hints.functions),
	GetMWMFunctionsFromProperty,
	(XmImportProc)NULL,
    },
};

static XmBaseClassExtRec       myBaseClassExtRec = {
    NULL,                                     /* Next extension       */
    NULLQUARK,                                /* record type XmQmotif */
    XmBaseClassExtVersion,                    /* version              */
    sizeof(XmBaseClassExtRec),                /* size                 */
    InitializePrehook,		              /* initialize prehook   */
    XmInheritSetValuesPrehook,	              /* set_values prehook   */
    (XtInitProc)NULL,		              /* initialize posthook  */
    (XtSetValuesFunc)NULL,	              /* set_values posthook  */
    NULL,				      /* secondary class      */
    (XtInitProc)NULL,		              /* creation proc        */
    (XmGetSecResDataFunc)NULL,                 /* getSecRes data       */
    {0},                                      /* fast subclass        */
    XmInheritGetValuesPrehook,	              /* get_values prehook   */
    (XtArgsProc)NULL,		              /* get_values posthook  */
    XmInheritClassPartInitPrehook,	      /* class_part_prehook   */
    XmInheritClassPartInitPosthook,	      /* class_part_posthook  */
    NULL,	 			      /* compiled_ext_resources*/   
    NULL,	 			      /* ext_resources       	*/   
    0,					      /* resource_count     	*/   
    TRUE,				      /* use_sub_resources	*/
};

externaldef(xmvendorshellextclassrec)
XmVendorShellExtClassRec xmVendorShellExtClassRec = {
    {	
	(WidgetClass) &xmShellExtClassRec,/* superclass		*/   
	"VendorShell",			/* class_name 		*/   
	sizeof(XmVendorShellExtRec), 	/* size 		*/   
	VendorClassInitialize, 		/* Class Initializer 	*/   
	VendorClassPartInitialize,	/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	(XtInitProc)NULL,		/* initialize         	*/   
	(XtArgsProc)NULL, 		/* initialize_notify    */ 
	NULL,				/* realize            	*/   
	NULL,				/* actions            	*/   
	0,				/* num_actions        	*/   
	extResources, 			/* resources          	*/   
	XtNumber(extResources),		/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	VendorDestroy,			/* destroy            	*/   
	NULL,	 			/* resize             	*/   
	NULL, 				/* expose             	*/   
	(XtSetValuesFunc)NULL,		/* set_values         	*/   
	(XtArgsFunc)NULL, 		/* set_values_hook      */ 
	NULL,	 			/* set_values_almost    */ 
	(XtArgsProc)NULL,		/* get_values_hook      */ 
	NULL,				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL,				/* display_accelerator  */ 
	(XtPointer)&myBaseClassExtRec,	/* extension            */ 
    },	
    {					/* ext			*/
	synResources,			/* synthetic resources	*/
	XtNumber(synResources),		/* num syn resources	*/
	NULL,				/* extension		*/
    },
    {					/* desktop		*/
	NULL,				/* child_class		*/
	XtInheritInsertChild,		/* insert_child		*/
	XtInheritDeleteChild,		/* delete_child		*/
	NULL,				/* extension		*/
    },
    {					/* shell ext		*/
	XmInheritEventHandler,		/* structureNotify	*/
	NULL,				/* extension		*/
    },
    {					/* vendor ext		*/
	DeleteWindowHandler,            /* delete window handler*/
	OffsetHandler,	                /* offset handler	*/
	NULL,				/* extension		*/
    },
};

externaldef(xmVendorShellExtobjectclass) WidgetClass 
  xmVendorShellExtObjectClass = (WidgetClass) (&xmVendorShellExtClassRec);


static char *KeyboardFocusPolicyNames[] =
{   "explicit", "pointer"
    } ;
static char *DeleteResponseNames[] =
{   "destroy", "unmap", "do_nothing"
    } ;

#define NUM_NAMES( list )        (sizeof( list) / sizeof( char *))


static XmImportOperator 
#ifdef _NO_PROTO
ExtParentDimToHorizontalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentDimToHorizontalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;

    _XmToHorizontalPixels( (Widget) ve, offset, value);
    *((Dimension *) ((char *)(ve->ext.logicalParent) + offset)) = (Dimension)*value;
    return XmSYNTHETIC_NONE;
}

static void 
#ifdef _NO_PROTO
ExtParentDimFromHorizontalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentDimFromHorizontalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
     *value = (XtArgVal) *((Dimension *)(((char *)ve->ext.logicalParent) + offset));
     _XmFromHorizontalPixels( (Widget) ve, offset, value);
}

static XmImportOperator 
#ifdef _NO_PROTO
ExtParentDimToVerticalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentDimToVerticalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
    _XmToVerticalPixels( (Widget) ve, offset, value);
    *((Dimension *) ((char *)(ve->ext.logicalParent) + offset)) = (Dimension)*value;
    return XmSYNTHETIC_NONE;
}

static void 
#ifdef _NO_PROTO
ExtParentDimFromVerticalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentDimFromVerticalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
     *value = (XtArgVal) *((Dimension *)(((char *)ve->ext.logicalParent) + offset));
     _XmFromVerticalPixels( (Widget) ve, offset, value);
}

static XmImportOperator 
#ifdef _NO_PROTO
ExtParentIntToHorizontalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentIntToHorizontalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
    _XmToHorizontalPixels( (Widget) ve, offset, value);
    *((int *) ((char *)(ve->ext.logicalParent) + offset)) = (int)*value;
    return XmSYNTHETIC_NONE;
}

static void 
#ifdef _NO_PROTO
ExtParentIntFromHorizontalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentIntFromHorizontalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
     *value = (XtArgVal) *((int *)(((char *)ve->ext.logicalParent) + offset));
     _XmFromHorizontalPixels( (Widget) ve, offset, value);
}

static XmImportOperator 
#ifdef _NO_PROTO
ExtParentIntToVerticalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentIntToVerticalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
    _XmToVerticalPixels( (Widget) ve, offset, value);
    *((int *) ((char *)(ve->ext.logicalParent) + offset)) = (int)*value;
    return XmSYNTHETIC_NONE;
}

static void 
#ifdef _NO_PROTO
ExtParentIntFromVerticalPixels( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExtParentIntFromVerticalPixels(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
     *value = (XtArgVal) *((int *)(((char *)ve->ext.logicalParent) + offset));
     _XmFromVerticalPixels( (Widget) ve, offset, value);
}



/*
 * Fix for 1914 - Add a String to IconPixmap converter to allow user to
 *                enter icon pixmap for window manager from resource file.
 *                This method is based on that used to get the background
 *                pixmap.
 */
/************************************************************************
 *
 *  _XmGetIconPixmapName
 *      Return the icon pixmap name set by the string to icon resource
 *      converter.  This is used by the vendor shell.
 *
 ************************************************************************/
char*
#ifdef _NO_PROTO
_XmGetIconPixmapName()
#else
_XmGetIconPixmapName( void )
#endif /* _NO_PROTO */
{
   return (icon_pixmap_name);
}

/************************************************************************
 *
 *  _XmClearIconPixmapName
 *      Clear the icon pixmap name set by the string to icon resource
 *      converter.  This is used by the vendor shell.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmClearIconPixmapName()
#else
_XmClearIconPixmapName( void )
#endif /* _NO_PROTO */
{
   icon_pixmap_name = NULL;
}

    /*ARGSUSED*/
static Boolean
#ifdef _NO_PROTO
CvtStringToIconPixmap( dpy, args, num_args, fromVal, toVal, data )
        Display *dpy ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValuePtr fromVal ;
        XrmValuePtr toVal ;
        XtPointer *data ;
#else
CvtStringToIconPixmap(
        Display *dpy,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValuePtr fromVal,
        XrmValuePtr toVal,
        XtPointer *data )
#endif /* _NO_PROTO */
{

   Pixmap pixmap;
   char *image_name = (char *) (fromVal->addr);

   if (!_XmStringsAreEqual(image_name, "unspecified_pixmap"))
   {
       icon_pixmap_name = (char *) (fromVal->addr);
   }

/* Send back a NULL pixmap (will be created by VendorShell during Initialize */

   pixmap = 0;
   done (pixmap, Pixmap);
}

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
CvtStringToHorizDim( dpy, args, num_args, from_val, toVal, data )
        Display *dpy ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *toVal ;
        XtPointer *data ;
#else
CvtStringToHorizDim(
        Display *dpy,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *toVal,
        XtPointer *data )
#endif /* _NO_PROTO */
{
    unsigned char unitType = (unsigned char) *(args[0].addr);
    Screen * screen = * ((Screen **) args[1].addr);
    Dimension pixels;
    int intermediate;
    
    intermediate = (int) atoi(from_val->addr);
    
    pixels = (Dimension)
      _XmConvertUnits(screen,XmHORIZONTAL,(int) unitType,
		      intermediate,XmPIXELS);
    done(pixels, Dimension);
}

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
CvtStringToHorizPos( dpy, args, num_args, from_val, toVal, data )
        Display *dpy ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *toVal ;
        XtPointer *data ;
#else
CvtStringToHorizPos(
        Display *dpy,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *toVal,
        XtPointer *data )
#endif /* _NO_PROTO */
{
    unsigned char unitType = (unsigned char) *(args[0].addr);
    Screen * screen = * ((Screen **) args[1].addr);
    Position pixels;
    int intermediate;
    
    intermediate = (int) atoi(from_val->addr);
    
    pixels = (Position)
      _XmConvertUnits(screen,XmHORIZONTAL,(int) unitType,
		      intermediate,XmPIXELS);
    done(pixels, Position);

}
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
CvtStringToVertDim( dpy, args, num_args, from_val, toVal, data )
        Display *dpy ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *toVal ;
        XtPointer *data ;
#else
CvtStringToVertDim(
        Display *dpy,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *toVal,
        XtPointer *data )
#endif /* _NO_PROTO */
{
    unsigned char unitType = (unsigned char) *(args[0].addr);
    Screen * screen = * ((Screen **) args[1].addr);
    Dimension pixels;
    int intermediate;
    
    intermediate = (int) atoi(from_val->addr);
    
    pixels = (Dimension)
      _XmConvertUnits(screen,XmVERTICAL,(int) unitType,
		      intermediate,XmPIXELS);
    done(pixels, Dimension);
}

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
CvtStringToVertPos( dpy, args, num_args, from_val, toVal, data )
        Display *dpy ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *toVal ;
        XtPointer *data ;
#else
CvtStringToVertPos(
        Display *dpy,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *toVal,
        XtPointer *data )
#endif /* _NO_PROTO */
{
    unsigned char unitType = (unsigned char) *(args[0].addr);
    Screen * screen = * ((Screen **) args[1].addr);
    Position pixels;
    int intermediate;
    
    intermediate = (int) atoi(from_val->addr);
    
    pixels = (Position)
      _XmConvertUnits(screen,XmVERTICAL,(int) unitType,
		      intermediate,XmPIXELS);
    done(pixels, Position);
}

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
CvtStringToHorizontalInt( dpy, args, num_args, from_val, toVal, data )
        Display *dpy ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *toVal ;
        XtPointer *data ;
#else
CvtStringToHorizontalInt(
        Display *dpy,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *toVal,
        XtPointer *data )
#endif /* _NO_PROTO */
{
    unsigned char unitType = (unsigned char) *(args[0].addr);
    Screen * screen = * ((Screen **) args[1].addr);
    int pixels;
    int intermediate;
    
    intermediate = (int) atoi(from_val->addr);
    
    pixels = (int)
      _XmConvertUnits(screen,XmHORIZONTAL,(int) unitType,
		      intermediate,XmPIXELS);
    done(pixels, int);
}
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
CvtStringToVerticalInt( dpy, args, num_args, from_val, toVal, data )
        Display *dpy ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *toVal ;
        XtPointer *data ;
#else
CvtStringToVerticalInt(
        Display *dpy,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *toVal,
        XtPointer *data )
#endif /* _NO_PROTO */
{
    unsigned char unitType = (unsigned char) *(args[0].addr);
    Screen * screen = * ((Screen **) args[1].addr);
    int pixels;
    int intermediate;
    
    intermediate = (int) atoi(from_val->addr);
    
    pixels = (int)
      _XmConvertUnits(screen,XmVERTICAL,(int) unitType,
		      intermediate,XmPIXELS);
    done(pixels, int);
}

#undef done

static void 
#ifdef _NO_PROTO
FetchUnitType( widget, size, value )
        Widget widget ;
        Cardinal *size ;
        XrmValue *value ;
#else
FetchUnitType(
        Widget widget,
        Cardinal *size,
        XrmValue *value )
#endif /* _NO_PROTO */
{
    static unsigned char pixelType = XmPIXELS;

    if (widget == NULL) {
	XtErrorMsg("missingWidget", "fetchUnitType", "XtToolkitError",
                   MSG6,
                   (String*)NULL, (Cardinal*)NULL);
    }

    if (XmIsVendorShell(widget))
      {
	  XmWidgetExtData extData;
	  XmVendorShellExtObject ve;

	  if ((extData = _XmGetWidgetExtData(widget, XmSHELL_EXTENSION)) &&
	      (ve = (XmVendorShellExtObject)extData->widget))
	    value->addr = (XPointer)&(ve->vendor.unit_type);
	  else
	    value->addr = (XPointer)&(pixelType);
      }
    else {
	pixelType = _XmGetUnitType(widget);
	value->addr = (XPointer)&(pixelType);
    }

    value->size = sizeof(unsigned char);
}

/************************************************************************
 *
 *  RegisterVendorConverters
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
RegisterVendorConverters()
#else
RegisterVendorConverters( void )
#endif /* _NO_PROTO */
{
    static Boolean firstTime = TRUE;

    if (!firstTime)
      return;
    else
      firstTime = FALSE;
    
    XtSetTypeConverter(XmRString, 
		       XmRPixmap, 
		       CvtStringToIconPixmap, 
		       iconArgs, 
		       XtNumber(iconArgs),
		       XtCacheNone,
		       (XtDestructor)NULL);
    
    XtSetTypeConverter(XmRString, 
		       XmRShellHorizDim, 
		       CvtStringToHorizDim, 
		       resIndConvertArgs, 
		       XtNumber(resIndConvertArgs),
		       XtCacheNone, (XtDestructor)NULL);
    XtSetTypeConverter(XmRString, 
		       XmRShellVertDim,
		       CvtStringToVertDim,
		       resIndConvertArgs, 
		       XtNumber(resIndConvertArgs),
		       XtCacheNone, (XtDestructor)NULL);
    
    XtSetTypeConverter(XmRString, 
		       XmRShellHorizPos, 
		       CvtStringToHorizPos, 
		       resIndConvertArgs, 
		       XtNumber(resIndConvertArgs),
		       XtCacheNone, (XtDestructor)NULL);
    XtSetTypeConverter(XmRString, 
		       XmRShellVertPos,
		       CvtStringToVertPos,
		       resIndConvertArgs, 
		       XtNumber(resIndConvertArgs),
		       XtCacheNone, (XtDestructor)NULL);

    XtSetTypeConverter(XmRString, 
		       XmRHorizontalInt, 
		       CvtStringToHorizontalInt, 
		       resIndConvertArgs, 
		       XtNumber(resIndConvertArgs),
		       XtCacheNone, (XtDestructor)NULL);
    XtSetTypeConverter(XmRString, 
		       XmRVerticalInt, 
		       CvtStringToVerticalInt, 
		       resIndConvertArgs, 
		       XtNumber(resIndConvertArgs),
		       XtCacheNone, (XtDestructor)NULL);
    
    XmRepTypeRegister( XmRDeleteResponse, DeleteResponseNames, NULL,
                                             NUM_NAMES( DeleteResponseNames)) ;
    XmRepTypeRegister( XmRKeyboardFocusPolicy, KeyboardFocusPolicyNames, NULL,
                                        NUM_NAMES( KeyboardFocusPolicyNames)) ;
}

static void 
#ifdef _NO_PROTO
VendorClassInitialize()
#else
VendorClassInitialize( void )
#endif /* _NO_PROTO */
{
    RegisterVendorConverters();
    myBaseClassExtRec.record_type = XmQmotif;
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
VendorClassPartInitialize( w )
        WidgetClass w ;
#else
VendorClassPartInitialize(
        WidgetClass w )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObjectClass wc = (XmVendorShellExtObjectClass) w;
    XmVendorShellExtObjectClass sc =
      (XmVendorShellExtObjectClass) wc->object_class.superclass;
    
    if (wc == (XmVendorShellExtObjectClass)xmVendorShellExtObjectClass)
      return;

    if (wc->vendor_class.delete_window_handler == XmInheritProtocolHandler)
      wc->vendor_class.delete_window_handler = 
	sc->vendor_class.delete_window_handler;

    if (wc->vendor_class.offset_handler == XmInheritProtocolHandler)
      wc->vendor_class.offset_handler = 
	sc->vendor_class.offset_handler;
}



/************************************************************************
 *  DeleteWindowHandler
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
DeleteWindowHandler( wid, closure, call_data )
        Widget wid ;
        XtPointer closure ;
        XtPointer call_data ;
#else
DeleteWindowHandler(
        Widget wid,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
        VendorShellWidget w = (VendorShellWidget) wid ;
    XmVendorShellExtObject ve = (XmVendorShellExtObject) closure;

    switch(ve->vendor.delete_response)
      {
	case XmUNMAP:
	  if (w->shell.popped_up)
	    XtPopdown((Widget) w);
	  else
	    XtUnmapWidget((Widget) w);
	  break;
	  
	case XmDESTROY:
	  if (XtIsApplicationShell((Widget) w))
	    {
		XtDestroyApplicationContext
		  (XtWidgetToApplicationContext((Widget) w));
		exit(0);
	    }
	  else
	    XtDestroyWidget((Widget) w);
	  break;
	  
	case XmDO_NOTHING:
	default:
	  break;
      }
}    


/************************************************************************
 *
 *     OffsetHandler
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
OffsetHandler( shell, clientData, cd )
        Widget shell ;
        XtPointer clientData ;
        XtPointer cd ;
#else
OffsetHandler(
        Widget shell,
        XtPointer clientData,
        XtPointer cd )
#endif /* _NO_PROTO */
{
        XmAnyCallbackStruct *callData = (XmAnyCallbackStruct *) cd ;
    XClientMessageEvent		*offsetEvent;
    XmVendorShellExtObject	ve = (XmVendorShellExtObject)clientData;

    offsetEvent = (XClientMessageEvent *) callData->event;

    ve->vendor.lastOffsetSerial = offsetEvent->serial;
    ve->vendor.xOffset = (Position) offsetEvent->data.l[1];
    ve->vendor.yOffset = (Position) offsetEvent->data.l[2];
}


/************************************************************************
 *
 *     InitializePrehook
 *
 ************************************************************************/
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
    XmExtObjectClass		ec = (XmExtObjectClass) XtClass(new_w);
    XmBaseClassExt		*wcePtr;
    XmExtObject			ne = (XmExtObject) new_w;
    Widget			parent = ne->ext.logicalParent;
    XmExtObjectClass		pec = (XmExtObjectClass) XtClass(parent);
    XmBaseClassExt		*pcePtr;
    XmWidgetExtData		extData;

    wcePtr = _XmGetBaseClassExtPtr(ec, XmQmotif);
    pcePtr = _XmGetBaseClassExtPtr(pec, XmQmotif);

    if ((*wcePtr)->use_sub_resources)
      {
	  /*
	   * get a uncompiled resource list to use with
	   * XtGetSubresources. We can't do this in
	   * ClassPartInitPosthook because Xt doesn't set class_inited at
	   * the right place and thereby mishandles the
	   * XtGetResourceList call
	   */
	  if ((*wcePtr)->ext_resources == NULL)
	    {
		ec->object_class.resources =
		  (*wcePtr)->compiled_ext_resources;
		ec->object_class.num_resources =		
		  (*wcePtr)->num_ext_resources;

		XtGetResourceList((WidgetClass) ec,
				  &((*wcePtr)->ext_resources),
				  &((*wcePtr)->num_ext_resources));

	    }
	  if ((*pcePtr)->ext_resources == NULL)
	    {
		XtGetResourceList((WidgetClass) pec,
				  &((*pcePtr)->ext_resources),
				  &((*pcePtr)->num_ext_resources));
	    }
	  XtGetSubresources(parent,
			    (XtPointer)new_w,
			    NULL, NULL,
			    (*wcePtr)->ext_resources,
			    (*wcePtr)->num_ext_resources,
			    args, *num_args);

	  extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
	  _XmPushWidgetExtData(parent, extData, ne->ext.extensionType);
	  
	  extData->widget = new_w;
	  extData->reqWidget = (Widget)
	    XtMalloc(XtClass(new_w)->core_class.widget_size);
	  memcpy( extData->reqWidget, req,
		XtClass(new_w)->core_class.widget_size);
	  
	  /*  Convert the fields from unit values to pixel values  */

	  XtGetSubresources(parent,
			    (XtPointer)parent,
			    NULL, NULL,
			    (*pcePtr)->ext_resources,
			    (*pcePtr)->num_ext_resources,
			    args, *num_args);

	  _XmExtImportArgs(new_w, args, num_args);
      }
}


/************************************************************************
 *
 *  Destroy
 *
 *    This needs to be in the ext object because the extension gets
 *    blown away before the primary does since it's a child. Otherwise
 *    we'd have it in the primary.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
VendorDestroy( wid )
        Widget wid ;
#else
VendorDestroy(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
    if (ve->vendor.mwm_menu)
      XtFree(ve->vendor.mwm_menu);
    if (ve->vendor.input_method_string)
      XtFree(ve->vendor.input_method_string);
    if (ve->vendor.preedit_type_string)
      XtFree(ve->vendor.preedit_type_string);
    _XmDestroyFocusData(ve->vendor.focus_data);
}


/************************************************************************
 *
 *  _XmGetAudibleWarning
 *       This function is called by a widget to get the audibleWarning
 *   value. This is done by checking to see if any of the widgets, 
 *   in the widget's parent hierarchy is a subclass of VendorShell widget 
 *   class, and if it is, returning the  VendorShell resource value. 
 *   If no VendorShell is found, returns XmBELL, since it is the default
 *   value for this resource.
 *************************************************************************/
unsigned char
#ifdef _NO_PROTO
_XmGetAudibleWarning(w) Widget w ;
#else
_XmGetAudibleWarning(Widget w)
#endif /* _NO_PROTO */
{
    Arg al[1];
    unsigned char audib ;

    while (w && (w = XtParent(w))) {
	if (XmIsVendorShell (w)) {
	    XtSetArg(al[0], XmNaudibleWarning, &audib);
	    XtGetValues (w, al, 1);
	    return (audib);
            }
    }
   return (XmBELL);
} 

/****************************************************************/
static void 
#ifdef _NO_PROTO
GetMWMFunctionsFromProperty( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetMWMFunctionsFromProperty(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
  Atom actual_type;
  int actual_format;
  unsigned long num_items, bytes_after;
  PropMwmHints *prop = NULL ;
  XmVendorShellExtObject ve = (XmVendorShellExtObject) wid ;
  Widget shell = ve->ext.logicalParent ;
  Atom mwm_hints_atom ;

  if(    !XtIsRealized( shell)    )
    {   
      *value = (XtArgVal) ve->vendor.mwm_hints.functions ;
      return ;
    } 
  mwm_hints_atom = XmInternAtom( XtDisplay( shell), _XA_MWM_HINTS, FALSE);
  XGetWindowProperty( XtDisplay( shell), XtWindow( shell), mwm_hints_atom, 0,
		     (long) PROP_MWM_HINTS_ELEMENTS, FALSE, mwm_hints_atom,
		     &actual_type, &actual_format, &num_items, &bytes_after,
		     (unsigned char **) &prop);
  if(    (actual_type != mwm_hints_atom)
     ||  (actual_format != 32)
     ||  (num_items < PROP_MWM_HINTS_ELEMENTS)
     ||  (prop == NULL)    )
    {
      if(    prop != NULL    )
	{
	  XFree( (char *)prop) ;
	}
      *value = (XtArgVal) ve->vendor.mwm_hints.functions ;
      return ;
    }
  *value = (XtArgVal) prop->functions ;
  XFree( (char *) prop) ;
}

