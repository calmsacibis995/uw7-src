#pragma ident	"@(#)m1.2libs:Xm/BulletinB.c	1.3"
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

/* Include files:
*/
#include "XmI.h"
#include "RepTypeI.h"
#include "GMUtilsI.h"
#include <Xm/BulletinBP.h>
#include <Xm/BaseClassP.h>
#include <Xm/VendorSEP.h>
#include <Xm/VendorS.h>
#include <Xm/DialogS.h>

#include <Xm/AtomMgr.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushBGP.h>
#include <Xm/PushBP.h>
#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include "MessagesI.h"
#include "CallbackI.h"
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


/****************************************************************/
/* Local defines:
*/

#define STRING_CHARSET          "ISO8859-1"
#define	MARGIN_DEFAULT		10

#define DONT_CARE               -1L
#define COMMON_FUNCS     (MWM_FUNC_CLOSE | MWM_FUNC_MOVE | MWM_FUNC_RESIZE)
#define DIALOG_FUNCS     COMMON_FUNCS
#define CLIENT_FUNCS     (COMMON_FUNCS | MWM_FUNC_MAXIMIZE | MWM_FUNC_MINIMIZE)


#ifdef I18N_MSG
#define WARN_DIALOG_STYLE	catgets(Xm_catd,MS_BBoard,MSG_BB_2,\
_XmMsgBulletinB_0001)
#else
#define WARN_DIALOG_STYLE	_XmMsgBulletinB_0001
#endif



/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassPartInitialize() ;
static void DialogStyleDefault() ;
static void Initialize() ;
static void Destroy() ;
static Boolean SetValues() ;
static Boolean SetValuesHook() ;
static void HandleChangeManaged() ;
static void HandleResize() ;
static XtGeometryResult HandleGeometryManager() ;
static void ChangeManaged() ;
static void UnmanageCallback() ;
static void InsertChild() ;
static void DeleteChild() ;
static XtGeometryResult GeometryManager() ;
static XtGeometryResult QueryGeometry() ;
static void Resize() ;
static void Redisplay() ;
static Boolean BulletinBoardParentProcess() ;
static void GetDialogTitle() ;
static Widget GetBBWithDB() ;

#else

static void ClassPartInitialize( 
                        WidgetClass w_class) ;
static void DialogStyleDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
static void Initialize( 
                        Widget wid_req,
                        Widget wid_new,
                        ArgList args,
                        Cardinal *numArgs) ;
static void Destroy( 
                        Widget wid) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValuesHook( 
                        Widget wid,
                        ArgList args,
                        Cardinal *num_args) ;
static void HandleChangeManaged( 
                        XmBulletinBoardWidget bbWid,
                        XmGeoCreateProc geoMatrixCreate) ;
static void HandleResize( 
                        XmBulletinBoardWidget bbWid,
                        XmGeoCreateProc geoMatrixCreate) ;
static XtGeometryResult HandleGeometryManager( 
                        Widget instigator,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed,
                        XmGeoCreateProc geoMatrixCreate) ;
static void ChangeManaged( 
                        Widget wid) ;
static void UnmanageCallback( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void InsertChild( 
                        Widget child) ;
static void DeleteChild( 
                        Widget child) ;
static XtGeometryResult GeometryManager( 
                        Widget w,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static XtGeometryResult QueryGeometry( 
                        Widget wid,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *desired) ;
static void Resize( 
                        Widget wid) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static Boolean BulletinBoardParentProcess( 
                        Widget wid,
                        XmParentProcessData event) ;
static void GetDialogTitle( 
                        Widget bb,
                        int resource,
                        XtArgVal *value) ;
static Widget GetBBWithDB( 
                        Widget wid) ;
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*  default translation table and action list  */

#define defaultTranslations	_XmBulletinB_defaultTranslations

static XtActionsRec actionsList[] =
{
	{ "Return",	_XmBulletinBoardReturn },       /*Motif 1.0 BC. */
	{ "BulletinBoardReturn", _XmBulletinBoardReturn },
        { "BulletinBoardCancel", _XmBulletinBoardCancel },
	{ "BulletinBoardMap",    _XmBulletinBoardMap },
};



/*  Resource definitions for BulletinBoard
 */

static XmSyntheticResource syn_resources[] =
{
	{	XmNdialogTitle,
		sizeof (XmString),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.dialog_title),
		GetDialogTitle,
		NULL
	},
	{	XmNmarginWidth,
		sizeof (Dimension),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.margin_width),
		_XmFromHorizontalPixels,
		_XmToHorizontalPixels
	},

	{	XmNmarginHeight,
		sizeof (Dimension),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.margin_height),
		_XmFromVerticalPixels,
		_XmToVerticalPixels
	},
};



static XtResource resources[] =
{
	{	XmNshadowType,
		XmCShadowType, XmRShadowType, sizeof (unsigned char),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.shadow_type),
		XmRImmediate, (XtPointer) XmSHADOW_OUT
	},

	{
		XmNshadowThickness,
		XmCShadowThickness, XmRHorizontalDimension, sizeof (Dimension),
		XtOffsetOf( struct _XmBulletinBoardRec, manager.shadow_thickness),
		XmRImmediate, (XtPointer) 0
	},

	{	XmNmarginWidth,
		XmCMarginWidth, XmRHorizontalDimension, sizeof (Dimension),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.margin_width),
		XmRImmediate, (XtPointer) MARGIN_DEFAULT
	},

	{	XmNmarginHeight,
		XmCMarginHeight, XmRVerticalDimension, sizeof (Dimension),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.margin_height),
		XmRImmediate, (XtPointer) MARGIN_DEFAULT
	},

	{	XmNdefaultButton,
		XmCWidget, XmRWidget, sizeof (Widget), 
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.default_button),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNcancelButton,
		XmCWidget, XmRWidget, sizeof (Widget),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.cancel_button),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNfocusCallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.focus_callback),
		XmRImmediate, (XtPointer) NULL
	},
#ifdef BB_HAS_LOSING_FOCUS_CB
	{	XmNlosingFocusCallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		XtOffsetOf( struct _XmBulletinBoardRec, 
                                         bulletin_board.losing_focus_callback),
		XmRImmediate, (XtPointer) NULL
	},
#endif
	{	XmNmapCallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.map_callback),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNunmapCallback,
		XmCCallback, XmRCallback, sizeof (XtCallbackList),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.unmap_callback),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNbuttonFontList,
		XmCButtonFontList, XmRFontList, sizeof (XmFontList),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.button_font_list),
		XmRFontList, (XtPointer) NULL
	},

	{	XmNlabelFontList,
		XmCLabelFontList, XmRFontList, sizeof (XmFontList),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.label_font_list),
		XmRFontList, (XtPointer) NULL
	},

	{	XmNtextFontList,
		XmCTextFontList, XmRFontList, sizeof (XmFontList),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.text_font_list),
		XmRFontList, (XtPointer) NULL
	},

	{	XmNtextTranslations,
		XmCTranslations, XmRTranslationTable, sizeof (XtTranslations),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.text_translations),
		XmRImmediate, (XtPointer) NULL
	},

	{	XmNallowOverlap,
		XmCAllowOverlap, XmRBoolean, sizeof (Boolean),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.allow_overlap),
		XmRImmediate, (XtPointer) True
	},

	{	XmNautoUnmanage,
		XmCAutoUnmanage, XmRBoolean, sizeof (Boolean),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.auto_unmanage),
		XmRImmediate, (XtPointer) True
	},

	{	XmNdefaultPosition,
		XmCDefaultPosition, XmRBoolean, sizeof (Boolean),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.default_position),
		XmRImmediate, (XtPointer) True
	},

	{	XmNresizePolicy,
		XmCResizePolicy, XmRResizePolicy, sizeof (unsigned char),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.resize_policy),
		XmRImmediate, (XtPointer) XmRESIZE_ANY
	},

	{	XmNnoResize,
		XmCNoResize, XmRBoolean, sizeof (Boolean),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.no_resize),
		XmRImmediate, (XtPointer) False
	},

	{	XmNdialogStyle,
		XmCDialogStyle, XmRDialogStyle, sizeof (unsigned char),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.dialog_style),
		XmRCallProc, (XtPointer) DialogStyleDefault
	},

	{	XmNdialogTitle,
		XmCDialogTitle, XmRXmString, sizeof (XmString),
		XtOffsetOf( struct _XmBulletinBoardRec, bulletin_board.dialog_title),
		XmRString, (XtPointer) NULL
	},
                  
};



/****************************************************************
 *
 *	BulletinBoard class record
 *
 ****************************************************************/

externaldef( xmbulletinboardclassrec) XmBulletinBoardClassRec
                                                      xmBulletinBoardClassRec =
{
   {			/* core_class fields      */
      (WidgetClass) &xmManagerClassRec,		/* superclass         */
      "XmBulletinBoard",			/* class_name         */
      sizeof(XmBulletinBoardRec),		/* widget_size        */
      NULL,					/* class_initialize   */
      ClassPartInitialize,			/* class_part_init    */
      FALSE,					/* class_inited       */
      Initialize,				/* initialize         */
      NULL,					/* initialize_hook    */
      XtInheritRealize,                    	/* realize            */
      actionsList,		                /* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,	                	/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      TRUE,					/* compress_motion    */
      XtExposeCompressMaximal,			/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      Destroy,                                  /* destroy            */
      Resize,					/* resize             */
      Redisplay,        			/* expose             */
      SetValues,	                	/* set_values         */
      SetValuesHook,				/* set_values_hook    */
      XtInheritSetValuesAlmost, 		/* set_values_almost  */
      NULL,					/* get_values_hook    */
      NULL,					/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      defaultTranslations,                      /* tm_table           */
      QueryGeometry,	                        /* query_geometry     */
      NULL,             	                /* display_accelerator*/
      NULL,		                        /* extension          */
   },

   {		/* composite_class fields */
      GeometryManager,    	                /* geometry_manager   */
      ChangeManaged,		                /* change_managed     */
      InsertChild,				/* insert_child       */
      DeleteChild,				/* delete_child       */
      NULL,                                     /* extension          */
   },

   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      0,					/* constraint size      */   
      NULL,					/* init proc            */   
      NULL,             			/* destroy proc         */   
      NULL,					/* set values proc      */   
      NULL,                                     /* extension            */
   },

   {		/* manager_class fields */
      XtInheritTranslations,			/* default translations   */
      syn_resources,				/* syn_resources      	  */
      XtNumber (syn_resources),			/* num_syn_resources 	  */
      NULL,					/* syn_cont_resources     */
      0,					/* num_syn_cont_resources */
      BulletinBoardParentProcess,               /* parent_process         */
      NULL,					/* extension		  */
   },

   {		/* bulletinBoard class fields */     
      FALSE,                                    /*always_install_accelerators*/
      NULL,                                     /* geo_matrix_create */
      _XmBulletinBoardFocusMoved,               /* focus_moved_proc */
      NULL					/* extension */
   }
};

externaldef( xmbulletinboardwidgetclass) WidgetClass xmBulletinBoardWidgetClass
                                     = (WidgetClass) &xmBulletinBoardClassRec ;


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
            XmBulletinBoardWidgetClass bbClass
                                         = (XmBulletinBoardWidgetClass) w_class ;
            XmBulletinBoardWidgetClass bbSuper
                  = (XmBulletinBoardWidgetClass) w_class->core_class.superclass ;
/****************/

    _XmFastSubclassInit (w_class, XmBULLETIN_BOARD_BIT);

    if(    bbClass->bulletin_board_class.geo_matrix_create
                                               == XmInheritGeoMatrixCreate    )
    {   bbClass->bulletin_board_class.geo_matrix_create
                            = bbSuper->bulletin_board_class.geo_matrix_create ;
        } 
    if(    bbClass->bulletin_board_class.focus_moved_proc
                                                == XmInheritFocusMovedProc    )
    {   bbClass->bulletin_board_class.focus_moved_proc
                             = bbSuper->bulletin_board_class.focus_moved_proc ;
        } 
    return ;
    }

/****************************************************************
 * Set the default style to modeless if the parent is a 
 *   DialogShell, otherwise set to workarea.
 ****************/
static void 
#ifdef _NO_PROTO
DialogStyleDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
DialogStyleDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{   
    static unsigned char    style ;
/****************/

    style = XmDIALOG_MODELESS ;

    value->addr = (XPointer) (&style) ;

    return ;
    }

/****************************************************************
 * Initialize a BulletinBoard instance.
 ****************/
static void 
#ifdef _NO_PROTO
Initialize( wid_req, wid_new, args, numArgs )
        Widget wid_req ;
        Widget wid_new ;
        ArgList args ;
        Cardinal *numArgs ;
#else
Initialize(
        Widget wid_req,
        Widget wid_new,
        ArgList args,
        Cardinal *numArgs )
#endif /* _NO_PROTO */
{   
            XmBulletinBoardWidget request = (XmBulletinBoardWidget) wid_req ;
            XmBulletinBoardWidget new_w = (XmBulletinBoardWidget) wid_new ;
            Arg             al[5] ;
    register Cardinal       ac ;
            long            mwm_functions ;
            char *          text_value ;
            XmFontList      defaultFL ;
            int             mwmStyle ;
            Widget          ancestor ;
            XmWidgetExtData extData ;
            XmBulletinBoardWidgetClass bbClass ;
            XmVendorShellExtObject vendorExt;
/****************/

    new_w->bulletin_board.in_set_values = False ;
    new_w->bulletin_board.geo_cache = (XmGeoMatrix) NULL ;
    new_w->bulletin_board.initial_focus = TRUE ;

    /*	Copy font list data.
    */
    defaultFL = BB_ButtonFontList( new_w) ;
    if(    !defaultFL    )
    {   defaultFL = _XmGetDefaultFontList( (Widget) new_w, XmBUTTON_FONTLIST) ;
        } 
    BB_ButtonFontList( new_w) = XmFontListCopy( defaultFL) ;

    defaultFL = BB_LabelFontList( new_w) ;
    if(    !defaultFL    )
    {   defaultFL = _XmGetDefaultFontList( (Widget) new_w, XmLABEL_FONTLIST) ;
        } 
    BB_LabelFontList( new_w) = XmFontListCopy( defaultFL) ;

    defaultFL = BB_TextFontList( new_w) ;
    if(    !defaultFL    )
    {   defaultFL = _XmGetDefaultFontList( (Widget) new_w, XmTEXT_FONTLIST) ;
        } 
    BB_TextFontList( new_w) = XmFontListCopy( defaultFL) ;

    if(    (request->manager.shadow_thickness == 0)
        && XtIsShell( XtParent( request))    )
    {   new_w->manager.shadow_thickness = 1 ;
        }
    /* Default and Cancel buttons are Set/Get resources only.  The
    *   DefaultButton field is used as a marker for the state of buttons
    *   in the child list.  As such, it is essential that it is NULL at
    *   the end of Initialize.
    */
    BB_DefaultButton( new_w) = NULL ;
    BB_CancelButton( new_w) = NULL ;

    /* The DynamicButton fields are managed and will be set appropriately
    *   in the focus moved callback routine.
    */
    BB_DynamicDefaultButton( new_w) = NULL ;
    BB_DynamicCancelButton( new_w) = NULL ;

    new_w->bulletin_board.old_shadow_thickness = 0 ;

    if(    request->bulletin_board.dialog_title    )
    {   
        new_w->bulletin_board.dialog_title = XmStringCopy( 
                                        request->bulletin_board.dialog_title) ;
        _XmStringUpdateWMShellTitle(new_w->bulletin_board.dialog_title,
				    XtParent(new_w)) ;
        }
    /*	Set parent attributes.
    */
    ac = 0 ;
    text_value = NULL ;

    /*	Turn off window manager resize.
    */
    if(    request->bulletin_board.no_resize
        && XmIsVendorShell( XtParent( new_w))    )
    {   
        long old_mwm_functions ;

        XtSetArg( al[0], XmNmwmFunctions, &old_mwm_functions) ;
	XtGetValues( XtParent( new_w), al, 1) ;

	mwm_functions = XmIsDialogShell( XtParent( new_w))
                                                ? DIALOG_FUNCS : CLIENT_FUNCS ;
	if(    old_mwm_functions != DONT_CARE    )
	  {
            mwm_functions |= old_mwm_functions ;
	  }
        mwm_functions &= ~MWM_FUNC_RESIZE ;
        XtSetArg( al[ac], XmNmwmFunctions, mwm_functions) ;  ac++ ;
        }
          
    /* If parent is DialogShell, set dialog attributes and realize.
    */
    if(    XmIsDialogShell (XtParent (request))    )
    {   
        new_w->bulletin_board.shell = XtParent( request) ;

        switch(    request->bulletin_board.dialog_style    )
        {   
            case XmDIALOG_PRIMARY_APPLICATION_MODAL:
            {   mwmStyle = MWM_INPUT_PRIMARY_APPLICATION_MODAL ;
                break ;
                } 
            case XmDIALOG_FULL_APPLICATION_MODAL:
            {   mwmStyle = MWM_INPUT_FULL_APPLICATION_MODAL ;
                break; 
                } 
            case XmDIALOG_SYSTEM_MODAL:
            {   mwmStyle = MWM_INPUT_SYSTEM_MODAL ;
                break ;
                } 
            case XmDIALOG_MODELESS:
            default:
            {   mwmStyle = MWM_INPUT_MODELESS ;
                break ;
                } 
            } 
        XtSetArg( al[ac], XmNmwmInputMode, mwmStyle) ;  ac++ ;
        XtSetValues( new_w->bulletin_board.shell, al, ac) ;
        XtRealizeWidget( new_w->bulletin_board.shell) ;
        } 
    else
    {   new_w->bulletin_board.shell = NULL ;
        if(    ac    )
        {   XtSetValues( XtParent (request), al, (Cardinal) ac) ;
            } 
        }
    XtFree( text_value) ;

    if(    !XmRepTypeValidValue( XmRID_SHADOW_TYPE,
                            new_w->bulletin_board.shadow_type, (Widget) new_w)    )
    {   new_w->bulletin_board.shadow_type = XmSHADOW_OUT ;
        }

    if(    !XmRepTypeValidValue( XmRID_RESIZE_POLICY,
                          new_w->bulletin_board.resize_policy, (Widget) new_w)    )
    {   new_w->bulletin_board.resize_policy = XmRESIZE_ANY ;
        }

    if(    new_w->bulletin_board.shell    )
    {   
        if(    !XmRepTypeValidValue( XmRID_DIALOG_STYLE,
                           new_w->bulletin_board.dialog_style, (Widget) new_w)    )
        {   new_w->bulletin_board.dialog_style = XmDIALOG_MODELESS ;
            } 
        } 
    else
    {   if(    new_w->bulletin_board.dialog_style != XmDIALOG_MODELESS    )
        {   
            _XmWarning( (Widget) new_w, WARN_DIALOG_STYLE) ;
            new_w->bulletin_board.dialog_style = XmDIALOG_MODELESS ;
            } 
        }
    /* Set source widget for accelerators used 
    *   by Manager in ConstraintInitialize.
    */
    if(    new_w->core.accelerators    )
    {   new_w->manager.accelerator_widget = (Widget) new_w ;
        } 

    bbClass = (XmBulletinBoardWidgetClass) new_w->core.widget_class ;

    if(    bbClass->bulletin_board_class.focus_moved_proc    )
    {   
        /* Walk up hierarchy to find vendor shell.  Then setup moved focus
        *   callback so default button can be maintained.
        */
        ancestor = XtParent( new_w) ;
        while(    ancestor  &&  !XmIsVendorShell( ancestor)    )
        {   ancestor = XtParent( ancestor) ;
            } 
        if(    ancestor
	   && (extData = _XmGetWidgetExtData( ancestor, XmSHELL_EXTENSION)) 
	   && (extData->widget)  )
        {   
             vendorExt = (XmVendorShellExtObject) extData->widget;

            _XmAddCallback((InternalCallbackList *) &(vendorExt->vendor.focus_moved_callback),
                            (XtCallbackProc) bbClass->bulletin_board_class.focus_moved_proc,
                            (XtPointer) new_w) ;
        } 
    } 
    new_w->bulletin_board.old_width = new_w->core.width;  /* may be 0 */
    new_w->bulletin_board.old_height = new_w->core.height;        /* may be 0 */

    return ;
}


/****************************************************************
 * Free fontlists, etc.
 ****************/
static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid )
#endif /* _NO_PROTO */
{   
    XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
    Widget          ancestor ;
    XmWidgetExtData extData ;
    XmBulletinBoardWidgetClass bbClass ;
    XmVendorShellExtObject vendorExt;
    int num_children, i;
    Widget *childList;

 /*
  * Fix for 5209 - Check all ancestors to see if they are also bulletin
  *                boards.  If they are, check the values in the ancestor's
  *                default_button, cancel_button, and dynamic buttons to
  *                see if they match those of the bb being destroyed.  If
  *                they do, set those fields on the ancestor to NULL to
  *                avoid accessing freed memory in the future.
  */
    ancestor = XtParent( bb) ;
    while(    ancestor  &&  !XmIsVendorShell( ancestor)    )
       {   
           if (XmIsBulletinBoard(ancestor))
           {
             num_children = bb->composite.num_children;
             childList = bb->composite.children;
             for (i = 0; i < num_children; i++)
             {
               if (BB_CancelButton(ancestor) == childList[i])
                 BB_CancelButton(ancestor) = NULL; 
               if (BB_DynamicCancelButton(ancestor) == childList[i])
                 BB_DynamicCancelButton(ancestor) = NULL; 
               if (BB_DefaultButton(ancestor) == childList[i])
                 BB_DefaultButton(ancestor) = NULL; 
               if (BB_DynamicDefaultButton(ancestor) == childList[i])
                 BB_DynamicDefaultButton(ancestor) = NULL; 
             }
           }
           ancestor = XtParent( ancestor) ;
       }
  /* End 5209 */
  

    XmStringFree( bb->bulletin_board.dialog_title) ;

    /*  Free geometry cache.
    */
    if(    bb->bulletin_board.geo_cache    )
    {   _XmGeoMatrixFree( bb->bulletin_board.geo_cache) ;
        } 
    /*	Free fontlists.
    */
    if(    bb->bulletin_board.button_font_list    )
    {   XmFontListFree( bb->bulletin_board.button_font_list) ;
        } 
    if(    bb->bulletin_board.label_font_list    )
    {   XmFontListFree( bb->bulletin_board.label_font_list) ;
        } 
    if(    bb->bulletin_board.text_font_list    )
    {   XmFontListFree( bb->bulletin_board.text_font_list) ;
        } 

    bbClass = (XmBulletinBoardWidgetClass) bb->core.widget_class ;

    if(    bbClass->bulletin_board_class.focus_moved_proc    )
    {   
        /* Walk up hierarchy to find vendor shell.  Then remove focus moved
        *   callback.
        */
        ancestor = XtParent( bb) ;
        while(    ancestor  &&  !XmIsVendorShell( ancestor)    )
        {   ancestor = XtParent( ancestor) ;
            } 
        if(    ancestor
            && !ancestor->core.being_destroyed
            && (extData = _XmGetWidgetExtData( ancestor, XmSHELL_EXTENSION))  )
        {   
             vendorExt = (XmVendorShellExtObject) extData->widget;

            _XmRemoveCallback((InternalCallbackList *) &(vendorExt->vendor.focus_moved_callback),
                             (XtCallbackProc) bbClass->bulletin_board_class.focus_moved_proc,
                             (XtPointer) bb) ;
        } 
    } 
}


/****************************************************************
 * Modify attributes of a BulletinBoard instance.
 ****************/
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
    XmBulletinBoardWidget current = (XmBulletinBoardWidget) cw ;
    XmBulletinBoardWidget request = (XmBulletinBoardWidget) rw ;
    XmBulletinBoardWidget new_w = (XmBulletinBoardWidget) nw ;
    Arg		    al[10] ;
    register Cardinal       ac ;
    long            mwm_functions ;
    int             mwmStyle ;
    unsigned int    numChildren ;
    unsigned int    childIndex ;
    Widget *        childList ;
    Boolean flag = False ;

/****************/

    current->bulletin_board.in_set_values = True ;


    if(new_w->bulletin_board.shadow_type
       != current->bulletin_board.shadow_type) {
	if (XmRepTypeValidValue( XmRID_SHADOW_TYPE,
				new_w->bulletin_board.shadow_type, 
				(Widget) new_w) )
	    flag = True ;
	else 
	    new_w->bulletin_board.shadow_type = 
		current->bulletin_board.shadow_type ;
    } 
    
    if(    (new_w->bulletin_board.resize_policy 
                                      != current->bulletin_board.resize_policy)
        && !XmRepTypeValidValue( XmRID_RESIZE_POLICY,
                          new_w->bulletin_board.resize_policy, (Widget) new_w)    )
	{   new_w->bulletin_board.resize_policy 
		= current->bulletin_board.resize_policy ;
        }

    if(    new_w->bulletin_board.dialog_style
                                   != current->bulletin_board.dialog_style    )
    {   if(    new_w->bulletin_board.shell    )
        {   
            if(    !XmRepTypeValidValue( XmRID_DIALOG_STYLE, 
                           new_w->bulletin_board.dialog_style, (Widget) new_w)    )
            {   new_w->bulletin_board.dialog_style
                                       = current->bulletin_board.dialog_style ;
                } 
            } 
        else
        {   if(    new_w->bulletin_board.dialog_style != XmDIALOG_MODELESS    )
            {   
                _XmWarning( (Widget) new_w, WARN_DIALOG_STYLE) ;
                new_w->bulletin_board.dialog_style
                                       = current->bulletin_board.dialog_style ;
                } 
            }
        } 
    /*	Update shell attributes.
    */
    if(    new_w->bulletin_board.dialog_title !=
                                      current->bulletin_board.dialog_title    )
    {   /* Update window manager title.
        */
        XmStringFree( current->bulletin_board.dialog_title) ;
        new_w->bulletin_board.dialog_title = XmStringCopy(
                                        request->bulletin_board.dialog_title) ;
        _XmStringUpdateWMShellTitle(new_w->bulletin_board.dialog_title,
				    XtParent(new_w)) ;
        }

/*
 * Fix for 2940 - If the parent of the BulletinBoard is a VendorShell or
 *                related to VendorShell, check the no_resize and 
 *		  change it if necessary
 */
    if (XmIsVendorShell(XtParent(new_w)))
    {   
        ac = 0 ;
        /*	Turn window manager resize on or off.
        */
        if(    new_w->bulletin_board.no_resize != current->bulletin_board.no_resize    )
        {   
            XtSetArg( al[0], XmNmwmFunctions, &mwm_functions) ;
	    XtGetValues( XtParent( new_w), al, 1) ;

	    if(    mwm_functions == DONT_CARE    )
	      {
		mwm_functions = XmIsDialogShell( XtParent( new_w))
                                                ? DIALOG_FUNCS : CLIENT_FUNCS ;
	      }
            if(    new_w->bulletin_board.no_resize    )
            {   mwm_functions &= ~MWM_FUNC_RESIZE ;
                } 
            else
            {   mwm_functions |= MWM_FUNC_RESIZE ;
                } 
            XtSetArg( al[ac], XmNmwmFunctions, mwm_functions) ;  ac++ ;
            }
        if(    new_w->bulletin_board.shell    )
        {
          if(    new_w->bulletin_board.dialog_style
                                   != current->bulletin_board.dialog_style    )
          {   if(    !XmRepTypeValidValue( XmRID_DIALOG_STYLE,
                             new_w->bulletin_board.dialog_style, (Widget) new_w)    )
              {   
                  new_w->bulletin_board.dialog_style 
                                       = current->bulletin_board.dialog_style ;
                  } 
              else
              {   switch(    new_w->bulletin_board.dialog_style    )
                  {   
                      case XmDIALOG_PRIMARY_APPLICATION_MODAL:
                      {   mwmStyle = MWM_INPUT_PRIMARY_APPLICATION_MODAL ;
                          break ;
                          } 
                      case XmDIALOG_FULL_APPLICATION_MODAL:
                      {   mwmStyle = MWM_INPUT_FULL_APPLICATION_MODAL ;
                          break ; 
                          } 
                      case XmDIALOG_SYSTEM_MODAL:
                      {   mwmStyle = MWM_INPUT_SYSTEM_MODAL ;
                          break ;
                          } 
                      case XmDIALOG_MODELESS:
	              default:
                      {   mwmStyle = MWM_INPUT_MODELESS ;
                          break ;
                          } 
                      } 
                  XtSetArg( al[ac], XmNmwmInputMode, mwmStyle) ; ac++ ;
                  }
              }
          }
        if(    ac    )
        {   
            XtSetValues( XtParent (new_w), al, ac) ;
            }
        }
    /*	Copy new font list data, free previous.
    */
    if(    request->bulletin_board.button_font_list
                               != current->bulletin_board.button_font_list    )
    {   if(    current->bulletin_board.button_font_list    )
        {   XmFontListFree( current->bulletin_board.button_font_list) ;
            } 
        if(    new_w->bulletin_board.button_font_list    )
        {   new_w->bulletin_board.button_font_list = XmFontListCopy( 
                                    request->bulletin_board.button_font_list) ;
            } 
        if(    !new_w->bulletin_board.button_font_list    )
        {   new_w->bulletin_board.button_font_list = XmFontListCopy( 
                     _XmGetDefaultFontList( (Widget) new_w, XmBUTTON_FONTLIST)) ;
            } 
        }
    if(    request->bulletin_board.label_font_list
                                != current->bulletin_board.label_font_list    )
    {   if(    current->bulletin_board.label_font_list    )
        {   XmFontListFree( current->bulletin_board.label_font_list) ;
            } 
        if(    new_w->bulletin_board.label_font_list    )
        {   new_w->bulletin_board.label_font_list = XmFontListCopy( 
                                     request->bulletin_board.label_font_list) ;
            } 
        if(    !new_w->bulletin_board.label_font_list    )
        {   new_w->bulletin_board.label_font_list = XmFontListCopy( 
                      _XmGetDefaultFontList( (Widget) new_w, XmLABEL_FONTLIST)) ;
            } 
        }
    if(    request->bulletin_board.text_font_list
                                 != current->bulletin_board.text_font_list    )
    {   if(    current->bulletin_board.text_font_list    )
        {   XmFontListFree( current->bulletin_board.text_font_list) ;
            } 
        if(    new_w->bulletin_board.text_font_list    )
        {   new_w->bulletin_board.text_font_list = XmFontListCopy( 
                                      request->bulletin_board.text_font_list) ;
            } 
        if(    !new_w->bulletin_board.text_font_list    )
        {   new_w->bulletin_board.text_font_list = XmFontListCopy( 
                       _XmGetDefaultFontList( (Widget) new_w, XmTEXT_FONTLIST)) ;
            } 
        }
    if(    BB_DefaultButton( new_w) != BB_DefaultButton( current)    )
    {   
        if(    !BB_DefaultButton( current)    )
        {   
            /* This is the a new default button.  To avoid children generating
            *   geometry requests every time they get the default button
            *   shadow, be sure that the compatibility bit is turned off.
            *   This may generate geometry requests at this time, but they
            *   will never occur subsequently because the insert child
            *   routine will hereafter assure that the compatibility bit is
            *   turned off.
            */
            numChildren = new_w->composite.num_children ;
            childList = new_w->composite.children ;
            childIndex = 0 ;
            while(    childIndex < numChildren    )
            {   
                if(    XmIsPushButtonGadget( childList[childIndex])
                    || XmIsPushButton( childList[childIndex])    )
                {   
                    _XmBulletinBoardSetDefaultShadow( childList[childIndex]) ;
                    } 
                ++childIndex ;
                } 
            }
           _XmBBUpdateDynDefaultButton( (Widget) new_w) ;
        } 
    if(    !(new_w->manager.initial_focus) && BB_DefaultButton( new_w)    )
      {
	_XmSetInitialOfTabGroup( (Widget) new_w, BB_DefaultButton( new_w)) ;
      }

    if(new_w->manager.shadow_thickness
       != current->manager.shadow_thickness) {
	flag = True ;
	new_w->bulletin_board.old_shadow_thickness =
	    new_w->manager.shadow_thickness ;
    } 

    if(XtClass( new_w) == xmBulletinBoardWidgetClass) {   

     /* do the margin enforcement only for pure BB, not subclasses,
         which have their own layout */

	if (((new_w->bulletin_board.margin_width != 
	      current->bulletin_board.margin_width) ||
	     (new_w->bulletin_board.margin_height !=
	      current->bulletin_board.margin_height))) {
	    
	    /* move the child around if necessary */
	    _XmGMEnforceMargin((XmManagerWidget)new_w,
			       new_w->bulletin_board.margin_width,
			       new_w->bulletin_board.margin_height,
			       False); /* use movewidget, no request */
	    _XmGMCalcSize ((XmManagerWidget)new_w, 
			   new_w->bulletin_board.margin_width, 
			   new_w->bulletin_board.margin_height, 
			   &new_w->core.width, &new_w->core.height);
	}
	
    } 

    current->bulletin_board.in_set_values = False ;

    return(flag) ;
}

/****************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValuesHook( wid, args, num_args )
        Widget wid ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesHook(
        Widget wid,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
        XmBulletinBoardWidget   bb = (XmBulletinBoardWidget) wid ;
	int			i;
	Widget			shell = bb->bulletin_board.shell;
/****************/

/* shared with DialogS.c */    
#define MAGIC_VAL ((Position)~0L)

    if (!shell)
      return (False);
    
    for (i=0; i<*num_args; i++)
      {
	  if (strcmp (args[i].name, XmNx) == 0)
	    {
		if ((args[i].value == 0) && (XtX (bb) == 0))
		  {
		      XtX (bb) = MAGIC_VAL;
		  }
	    } 
	  if (strcmp (args[i].name, XmNy) == 0)
	    {
		if ((args[i].value == 0) && (XtY (bb) == 0))
		  {
		      XtY (bb) = MAGIC_VAL;
		  }
	    }
      }
    return (False);
}


/****************************************************************/
Widget 
#ifdef _NO_PROTO
_XmBB_CreateButtonG( bb, l_string, name )
        Widget bb ;
        XmString l_string ;
        char *name ;
#else
_XmBB_CreateButtonG(
        Widget bb,
        XmString l_string,
        char *name )
#endif /* _NO_PROTO */
{
            Arg		    al[10] ;
    register Cardinal       ac = 0 ;
            Widget          button ;
/****************/

    if(    l_string    )
    {
	XtSetArg( al[ac], XmNlabelString, l_string) ; ac++ ;
        }
    XtSetArg( al[ac], XmNstringDirection, BB_StringDirection( bb)) ; ac++ ;

    button = XmCreatePushButtonGadget( (Widget) bb, name, al, ac) ;

    _XmBulletinBoardSetDefaultShadow( button) ;

    return( button) ;
    }

/****************************************************************/
Widget 
#ifdef _NO_PROTO
_XmBB_CreateLabelG( bb, l_string, name )
        Widget bb ;
        XmString l_string ;
        char *name ;
#else
_XmBB_CreateLabelG(
        Widget bb,
        XmString l_string,
        char *name )
#endif /* _NO_PROTO */
{
            Arg		    al[10] ;
    register int            ac = 0 ;
/****************/

    if(    l_string    )
    {
	XtSetArg( al[ac], XmNlabelString, l_string) ; ac++ ;
        }
    XtSetArg( al[ac], XmNstringDirection, BB_StringDirection( bb)) ; ac++ ;
    XtSetArg( al[ac], XmNhighlightThickness, 0) ; ac++ ;
    XtSetArg( al[ac], XmNtraversalOn, False) ; ac++ ;
    XtSetArg( al[ac], XmNalignment, XmALIGNMENT_BEGINNING) ; ac++ ;

    return( XmCreateLabelGadget( bb, name, al, ac)) ;
    }

/****************************************************************
 * Arrange widgets and make geometry request if necessary.
 ****************/
static void 
#ifdef _NO_PROTO
HandleChangeManaged( bbWid, geoMatrixCreate )
        XmBulletinBoardWidget bbWid ;
        XmGeoCreateProc geoMatrixCreate ;
#else
HandleChangeManaged(
        XmBulletinBoardWidget bbWid,
        XmGeoCreateProc geoMatrixCreate)
#endif /* _NO_PROTO */
{
            Dimension       desired_w = 0 ;
            Dimension       desired_h = 0 ;
            Dimension       allowed_w ;
            Dimension       allowed_h ;
            XmGeoMatrix     geoSpec ;
/****************/

	/* honor initial sizes if given and resize_none sizes */
    if (!XtIsRealized( bbWid)) {
	if (XtWidth(bbWid)) desired_w = XtWidth(bbWid) ;
	if (XtHeight(bbWid)) desired_h = XtHeight(bbWid) ;
    } else {
	if(BB_ResizePolicy(bbWid) == XmRESIZE_NONE) {
	    desired_w = XtWidth(bbWid) ;
	    desired_h = XtHeight(bbWid) ;
	}
    }

    geoSpec = (*geoMatrixCreate)( (Widget) bbWid, NULL, NULL) ;

    _XmGeoMatrixGet( geoSpec, XmGET_PREFERRED_SIZE) ;

    _XmGeoArrangeBoxes( geoSpec, (Position) 0, (Position) 0,
                                                      &desired_w, &desired_h) ;
    /*	Adjust desired width and height if resize_grow.
    */
    if(    BB_ResizePolicy( bbWid) == XmRESIZE_GROW    )
    {
	Boolean		reset_w = False ;
        Boolean         reset_h = False ;

        if(    desired_w < XtWidth( bbWid)    )
	{
	    desired_w = XtWidth( bbWid) ;
	    reset_w = True ;
	    }
	if(    desired_h < XtHeight( bbWid)    )
	{
	    desired_h = XtHeight( bbWid) ;
	    reset_h = True ;
	    }
	if(    reset_w  ||  reset_h    )
	{   
            _XmGeoArrangeBoxes( geoSpec, (Position) 0, (Position) 0,
                     &desired_w, &desired_h) ;
            }
	}

    /*	Request new geometry from parent.
    */
    if(    (desired_h != XtHeight( bbWid))
        || (desired_w != XtWidth( bbWid))    )
    {   
        switch(    XtMakeResizeRequest( (Widget) bbWid, desired_w, desired_h, 
                                     &allowed_w, &allowed_h)    )
        {   
            case XtGeometryYes:
            {   break ;
                } 
            case XtGeometryAlmost:
            {   
                XtMakeResizeRequest( (Widget) bbWid, allowed_w, allowed_h, 
                                  NULL, NULL) ;
                _XmGeoArrangeBoxes( geoSpec, (Position) 0, (Position) 0, 
                                 &allowed_w, &allowed_h) ;
                break ;
                } 
            case XtGeometryNo:
	    default:
            {   allowed_w = XtWidth( bbWid) ;
                allowed_h = XtHeight( bbWid) ;
                _XmGeoArrangeBoxes( geoSpec, (Position) 0, (Position) 0, 
                                 &allowed_w, &allowed_h) ;
                break ;
                } 
            }
        /*	Clear shadow if necessary.
        */
        if(    bbWid->bulletin_board.old_shadow_thickness
            && (   (allowed_w > bbWid->bulletin_board.old_width)
                || (allowed_h > bbWid->bulletin_board.old_height))    )
        {   
            _XmClearShadowType( (Widget) bbWid,
			       bbWid->bulletin_board.old_width,
			       bbWid->bulletin_board.old_height,
			       bbWid->bulletin_board.old_shadow_thickness, 
			       (Dimension) 0) ;
            bbWid->bulletin_board.old_shadow_thickness = 0 ;
            }
        }
    /*	Update children.
    */
    _XmGeoMatrixSet( geoSpec) ;

    _XmGeoMatrixFree( geoSpec) ;

    if(    bbWid->manager.shadow_thickness
        && (XtWidth( bbWid) <= bbWid->bulletin_board.old_width)
        && (XtHeight( bbWid) <= bbWid->bulletin_board.old_height)    )
    {   
        _XmDrawShadows(XtDisplay((Widget) bbWid), 
			XtWindow((Widget) bbWid),
			bbWid->manager.top_shadow_GC,
			bbWid->manager.bottom_shadow_GC,
			0, 0,
			bbWid->core.width, bbWid->core.height,
			bbWid->manager.shadow_thickness, 
			bbWid->bulletin_board.shadow_type) ;
        bbWid->bulletin_board.old_shadow_thickness 
                                            = bbWid->manager.shadow_thickness ;
        }
    bbWid->bulletin_board.old_width = bbWid->core.width ;
    bbWid->bulletin_board.old_height = bbWid->core.height ;

    _XmNavigChangeManaged( (Widget) bbWid) ;

    return ;
    }
/****************************************************************
 * Arrange widgets based on new size.
 ****************/
static void 
#ifdef _NO_PROTO
HandleResize( bbWid, geoMatrixCreate )
        XmBulletinBoardWidget bbWid ;
        XmGeoCreateProc geoMatrixCreate ;
#else
HandleResize(
        XmBulletinBoardWidget bbWid,
        XmGeoCreateProc geoMatrixCreate)
#endif /* _NO_PROTO */
{
            Dimension       new_w = XtWidth( bbWid) ;
            Dimension       new_h = XtHeight( bbWid) ;
            XmGeoMatrix     geoSpec ;
/*********** draw_shadow is not used **************
            Boolean         draw_shadow = False ;
****************/
    
    /*	Clear shadow.
    */
    if(    bbWid->bulletin_board.old_shadow_thickness
        && (   (bbWid->bulletin_board.old_width != bbWid->core.width)
            || (bbWid->bulletin_board.old_height != bbWid->core.height))    )
    {
	_XmClearShadowType( (Widget) bbWid, bbWid->bulletin_board.old_width,
                              bbWid->bulletin_board.old_height, 
                               bbWid->bulletin_board.old_shadow_thickness, 
                                                               (Dimension) 0) ;
        bbWid->bulletin_board.old_shadow_thickness = 0 ;
	}

    geoSpec = (*geoMatrixCreate)( (Widget) bbWid, NULL, NULL) ;

    _XmGeoMatrixGet( geoSpec, XmGET_PREFERRED_SIZE) ;

    _XmGeoArrangeBoxes( geoSpec, (Position) 0, (Position) 0, &new_w, &new_h) ;

    _XmGeoMatrixSet( geoSpec) ;

    if(    bbWid->manager.shadow_thickness
        && (XtWidth( bbWid) <= bbWid->bulletin_board.old_width)
        && (XtHeight( bbWid) <= bbWid->bulletin_board.old_height)    )
    {   
        _XmDrawShadows(XtDisplay((Widget) bbWid), 
			XtWindow((Widget) bbWid),
			bbWid->manager.top_shadow_GC,
			bbWid->manager.bottom_shadow_GC,
			0, 0,
			bbWid->core.width, bbWid->core.height,
			bbWid->manager.shadow_thickness, 
			bbWid->bulletin_board.shadow_type) ;
        bbWid->bulletin_board.old_shadow_thickness 
                                            = bbWid->manager.shadow_thickness ;
        } 
    bbWid->bulletin_board.old_width = bbWid->core.width ;
    bbWid->bulletin_board.old_height = bbWid->core.height ;

    _XmGeoMatrixFree( geoSpec) ;
    return ;
    }
/****************************************************************
 * Handle geometry requests from children.
 ****************/
static XtGeometryResult 
#ifdef _NO_PROTO
HandleGeometryManager( instigator, desired, allowed, geoMatrixCreate )
        Widget instigator ;
        XtWidgetGeometry *desired ;
        XtWidgetGeometry *allowed ;
        XmGeoCreateProc geoMatrixCreate ;
#else
HandleGeometryManager(
        Widget instigator,
        XtWidgetGeometry *desired,
        XtWidgetGeometry *allowed,
        XmGeoCreateProc geoMatrixCreate)
#endif /* _NO_PROTO */
{
            XmBulletinBoardWidget bbWid ;
            XtGeometryResult result ;
/****************/

    bbWid = (XmBulletinBoardWidget) XtParent( instigator) ;

    if(    !(desired->request_mode & (CWWidth | CWHeight))    )
    {   return (XtGeometryYes) ;
        } 

    /*	Clear shadow if necessary.
    */
    if(    bbWid->bulletin_board.old_shadow_thickness
        && (BB_ResizePolicy( bbWid) != XmRESIZE_NONE)    )
    {
        _XmClearShadowType( (Widget) bbWid, bbWid->bulletin_board.old_width, 
                              bbWid->bulletin_board.old_height, 
                               bbWid->bulletin_board.old_shadow_thickness, 0) ;
        bbWid->bulletin_board.old_shadow_thickness = 0 ;
	}

    result = _XmHandleGeometryManager( (Widget) bbWid,
                         instigator, desired, allowed, BB_ResizePolicy( bbWid),
                           &bbWid->bulletin_board.geo_cache, geoMatrixCreate) ;

    if(    !bbWid->bulletin_board.in_set_values
        && bbWid->manager.shadow_thickness
        && (XtWidth( bbWid) <= bbWid->bulletin_board.old_width)
        && (XtHeight( bbWid) <= bbWid->bulletin_board.old_height)    )
    {   
        _XmDrawShadows(XtDisplay((Widget) bbWid), 
			XtWindow((Widget) bbWid),
			bbWid->manager.top_shadow_GC,
			bbWid->manager.bottom_shadow_GC,
			0, 0,
			bbWid->core.width, bbWid->core.height,
			bbWid->manager.shadow_thickness, 
			bbWid->bulletin_board.shadow_type) ;
        bbWid->bulletin_board.old_shadow_thickness
                                            = bbWid->manager.shadow_thickness ;
        } 
    bbWid->bulletin_board.old_width = bbWid->core.width ;
    bbWid->bulletin_board.old_height = bbWid->core.height ;

    return( result) ;
    }

/****************************************************************/
void 
#ifdef _NO_PROTO
_XmBulletinBoardSizeUpdate( wid )
        Widget wid ;
#else
_XmBulletinBoardSizeUpdate(
        Widget wid )
#endif /* _NO_PROTO */
{
            XmBulletinBoardWidget bbWid = (XmBulletinBoardWidget) wid ;
            XmBulletinBoardWidgetClass classPtr = 
                        (XmBulletinBoardWidgetClass) bbWid->core.widget_class ;
/****************/

    if(    !XtIsRealized( bbWid)    )
    {   return ;
        } 

    if(    classPtr->bulletin_board_class.geo_matrix_create    )
    {   
        /*	Clear shadow if necessary.
        */
        if(    bbWid->bulletin_board.old_shadow_thickness
            && (BB_ResizePolicy( bbWid) != XmRESIZE_NONE)    )
        {
            _XmClearShadowType( (Widget) bbWid,
                         bbWid->bulletin_board.old_width,
                            bbWid->bulletin_board.old_height,
                               bbWid->bulletin_board.old_shadow_thickness, 0) ;
            bbWid->bulletin_board.old_shadow_thickness = 0 ;
            }

        _XmHandleSizeUpdate( (Widget) bbWid, BB_ResizePolicy( bbWid),
                            classPtr->bulletin_board_class.geo_matrix_create) ;

        if(    bbWid->manager.shadow_thickness
            && (XtWidth( bbWid) <= bbWid->bulletin_board.old_width)
            && (XtHeight( bbWid) <= bbWid->bulletin_board.old_height)    )
        {   
            _XmDrawShadows(XtDisplay((Widget) bbWid), 
			    XtWindow((Widget) bbWid),
			    bbWid->manager.top_shadow_GC,
			    bbWid->manager.bottom_shadow_GC,
			    0, 0,
			    bbWid->core.width, bbWid->core.height,
			    bbWid->manager.shadow_thickness, 
			    bbWid->bulletin_board.shadow_type) ;
	    bbWid->bulletin_board.old_shadow_thickness
		= bbWid->manager.shadow_thickness ;
            }
        } 
    bbWid->bulletin_board.old_width = bbWid->core.width ;
    bbWid->bulletin_board.old_height = bbWid->core.height ;

    return ;
    }

/****************************************************************
 * Layout children of the BulletinBoard.
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
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
            XmBulletinBoardWidgetClass classPtr = 
                           (XmBulletinBoardWidgetClass) bb->core.widget_class ;
/****************/

    if(    classPtr->bulletin_board_class.geo_matrix_create    )
    {   HandleChangeManaged( bb, 
                            classPtr->bulletin_board_class.geo_matrix_create) ;
        return ;
        } 

    /* function shared with Bulletin Board */
    _XmGMEnforceMargin((XmManagerWidget)bb,
                     bb->bulletin_board.margin_width,
                     bb->bulletin_board.margin_height, 
                     False); /* use movewidget, not setvalue */

    /*	Clear shadow if necessary.
    */
    if(    bb->bulletin_board.old_shadow_thickness    )
    {   
        _XmClearShadowType( (Widget) bb, bb->bulletin_board.old_width, 
				bb->bulletin_board.old_height, 
				bb->bulletin_board.old_shadow_thickness, 0) ;
        bb->bulletin_board.old_shadow_thickness = 0 ;
        }
  
    /* The first time, reconfigure only if explicit size were not given */

    if (XtIsRealized((Widget)bb) || (!XtWidth(bb)) || (!XtHeight(bb))) {
      /* function shared with DrawingArea */
      (void)_XmGMDoLayout((XmManagerWidget)bb,
                          bb->bulletin_board.margin_width,
                          bb->bulletin_board.margin_height,
                          bb->bulletin_board.resize_policy,
                          False);  /* queryonly not specified */
    }

    if(    bb->manager.shadow_thickness
        && (XtWidth( bb) <= bb->bulletin_board.old_width)
        && (XtHeight( bb) <= bb->bulletin_board.old_height)    )
    {   
        _XmDrawShadows(XtDisplay((Widget) bb), 
			XtWindow((Widget) bb),
			bb->manager.top_shadow_GC,
			bb->manager.bottom_shadow_GC,
			0, 0,
			bb->core.width, bb->core.height,
			bb->manager.shadow_thickness, 
			bb->bulletin_board.shadow_type) ;
        bb->bulletin_board.old_shadow_thickness 
                                               = bb->manager.shadow_thickness ;
        }
    bb->bulletin_board.old_width = bb->core.width ;
    bb->bulletin_board.old_height = bb->core.height ;

    _XmNavigChangeManaged( (Widget) bb) ;

    return ;
    }
   
/****************************************************************
 * Unmanage BulletinBoard after button is activated.
 ****************/
static void 
#ifdef _NO_PROTO
UnmanageCallback( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
UnmanageCallback(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
            Widget          bb = (Widget) client_data ;
/****************/

    XtUnmanageChild( bb) ;

    return ;
    }

/****************************************************************
 * Add a child to the BulletinBoard.
 ****************/
static void 
#ifdef _NO_PROTO
InsertChild( child )
        Widget child ;
#else
InsertChild(
        Widget child )
#endif /* _NO_PROTO */
{   
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget)
                                                             XtParent( child) ;
            Boolean         is_button = False ;
/****************/

    /* call manager InsertChild to update BB child info
    */
    (*((XmManagerWidgetClass) xmManagerWidgetClass)
                                      ->composite_class.insert_child)( child) ;
    if(    !XtIsRectObj( child)    )
    {   return ;
        }

    if(    XmIsPushButtonGadget( child)  ||  XmIsPushButton( child)    )
    {   is_button = TRUE ;

        if(    BB_DefaultButton( bb)    )
        {   
            /* Turn off compatibility mode behavior of Default PushButtons and
            *   set the default button shadow thickness.
            */
            _XmBulletinBoardSetDefaultShadow( child) ;
            } 
        } 
    if(    XmIsDrawnButton( child)    )
    {   is_button = True ;
        } 
    if(    is_button && bb->bulletin_board.shell
                                       && bb->bulletin_board.auto_unmanage    )
    {   
        XtAddCallback( child, XmNactivateCallback,
                                            UnmanageCallback, (XtPointer) bb) ;
        }
    if(    XmIsText (child)
        || XmIsTextField (child)    )
    {   
        if(    bb->bulletin_board.text_translations    )
        {   XtOverrideTranslations( child, 
				bb->bulletin_board.text_translations) ;
            } 
        }
    return ;
    }

/****************************************************************
 * Clear widget id in instance record
 ****************/
static void 
#ifdef _NO_PROTO
DeleteChild( child )
        Widget child ;
#else
DeleteChild(
        Widget child )
#endif /* _NO_PROTO */
{   
  XmBulletinBoardWidget bb ;
/****************/

  if(    XtIsRectObj( child)    )
    {   
      bb = (XmBulletinBoardWidget) XtParent( child) ;

      /* To fix CR #4882, unnest the following to allow
	 for one button to be more than one of the defined buttons */
      if(    child == BB_DefaultButton( bb)    )
	BB_DefaultButton( bb) = NULL ;
      if(    child == BB_DynamicDefaultButton( bb)    )
	BB_DynamicDefaultButton( bb) = NULL ;
      if(    child == BB_CancelButton( bb)    )
	BB_CancelButton( bb) = NULL ;
      if(    child == BB_DynamicCancelButton( bb)    )
	BB_DynamicCancelButton( bb) = NULL ;
    }
  (*((XmManagerWidgetClass) xmManagerWidgetClass)
           ->composite_class.delete_child)( child) ;
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
            XmBulletinBoardWidget bb ;
            XmBulletinBoardWidgetClass classPtr ;
/****************/

    bb = (XmBulletinBoardWidget) w->core.parent ;
    classPtr = (XmBulletinBoardWidgetClass) bb->core.widget_class ;

    if(    classPtr->bulletin_board_class.geo_matrix_create    )
    {   return( HandleGeometryManager( w, request, reply,
                           classPtr->bulletin_board_class.geo_matrix_create)) ;
        } 

    /* function shared with DrawingArea */
    return(_XmGMHandleGeometryManager((Widget)bb, w, request, reply, 
                                    bb->bulletin_board.margin_width, 
                                    bb->bulletin_board.margin_height, 
                                    bb->bulletin_board.resize_policy,
                                    bb->bulletin_board.allow_overlap));
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
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
            XmBulletinBoardWidgetClass classPtr = 
                           (XmBulletinBoardWidgetClass) bb->core.widget_class ;
/****************/

    if(    classPtr->bulletin_board_class.geo_matrix_create    )
    {   
        return( _XmHandleQueryGeometry( (Widget) bb,
                        intended, desired, BB_ResizePolicy( bb),
                           classPtr->bulletin_board_class.geo_matrix_create)) ;
        } 
    /* function shared with DrawingArea */
    return(_XmGMHandleQueryGeometry(wid, intended, desired, 
                                  bb->bulletin_board.margin_width, 
                                  bb->bulletin_board.margin_height, 
                                  bb->bulletin_board.resize_policy));
    }

/****************************************************************
 * Conform to new size.
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
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
            XmBulletinBoardWidgetClass classPtr = 
                           (XmBulletinBoardWidgetClass) bb->core.widget_class ;
/****************/

    if(    classPtr->bulletin_board_class.geo_matrix_create    )
    {   
        HandleResize( bb, classPtr->bulletin_board_class.geo_matrix_create) ;
        return ;
        } 
    /*	Clear shadow.
    */
    if(    bb->bulletin_board.old_shadow_thickness    )
    {   
        _XmClearShadowType( (Widget) bb, bb->bulletin_board.old_width, 
				bb->bulletin_board.old_height, 
				bb->bulletin_board.old_shadow_thickness, 0) ;
        bb->bulletin_board.old_shadow_thickness = 0 ;
        }
    if(    bb->manager.shadow_thickness
        && (XtWidth( bb) <= bb->bulletin_board.old_width)
        && (XtHeight( bb) <= bb->bulletin_board.old_height)    )
    {   
        _XmDrawShadows(XtDisplay((Widget) bb), 
			XtWindow((Widget) bb),
			bb->manager.top_shadow_GC,
			bb->manager.bottom_shadow_GC,
			0, 0,
			bb->core.width, bb->core.height,
			bb->manager.shadow_thickness, 
			bb->bulletin_board.shadow_type) ;
        bb->bulletin_board.old_shadow_thickness 
                                               = bb->manager.shadow_thickness ;
        }
    bb->bulletin_board.old_width = bb->core.width ;
    bb->bulletin_board.old_height = bb->core.height ;

    return ;
    }

/****************************************************************
 * Redisplay gadgets and draw shadow.
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
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
/****************/

    /*	Redisplay gadgets.
    */
    _XmRedisplayGadgets( wid, event, region) ;

    /*	Draw shadow.
    */
    if(    bb->manager.shadow_thickness    )
    {   
        _XmDrawShadows(XtDisplay((Widget) bb), 
			XtWindow((Widget) bb),
			bb->manager.top_shadow_GC,
			bb->manager.bottom_shadow_GC,
			0, 0,
			bb->core.width, bb->core.height,
			bb->manager.shadow_thickness, 
			bb->bulletin_board.shadow_type) ;
        bb->bulletin_board.old_shadow_thickness 
                                               = bb->manager.shadow_thickness ;
        } 
    bb->bulletin_board.old_width = bb->core.width ;
    bb->bulletin_board.old_height = bb->core.height ;

    return ;
    }

/****************************************************************/
void 
#ifdef _NO_PROTO
_XmBulletinBoardFocusMoved( wid, client_data, data )
        Widget wid ;
        XtPointer client_data ;
        XtPointer data ;
#else
_XmBulletinBoardFocusMoved(
        Widget wid,
        XtPointer client_data,
        XtPointer data )
#endif /* _NO_PROTO */
{   
            XmFocusMovedCallbackStruct * call_data 
                                        = (XmFocusMovedCallbackStruct *) data ;
            Widget          ancestor ;
	    XmBulletinBoardWidget bb = (XmBulletinBoardWidget) client_data ;
            XmAnyCallbackStruct	cb ;
            Boolean         BBHasFocus = FALSE ;
            Boolean         BBHadFocus = FALSE ;
            Widget          dbutton = NULL ;
            Widget          cbutton = NULL ;
/****************/

    if(    !call_data->cont    )
    {   /* Preceding callback routine wants focus-moved processing
        *   to be discontinued.
        */
        return ;
        } 

    /* Walk the heirarchy above the widget that is getting the focus to
    *   determine the correct default and cancel buttons for the current 
    *   context.
    *  Note that no changes are made to the Bulletin Board instance record
    *   until after the possibility of a forced traversal is evaluated.
    */
    ancestor = call_data->new_focus ;
    while(    ancestor  &&  !XtIsShell( ancestor)    )
    {   
        if(    ancestor == (Widget) bb    )
        {   BBHasFocus = TRUE ;
            break ;
            } 
        if(    XmIsBulletinBoard( ancestor)    )
        {   
            if(    !dbutton    )
            {   dbutton = BB_DefaultButton( ancestor) ;
                } 
            if(    !cbutton    )
            {   cbutton = BB_CancelButton( ancestor) ;
                } 
            } 
        ancestor = XtParent( ancestor) ;
        } 
    ancestor = call_data->old_focus ;
    while(    ancestor  &&  !XtIsShell( ancestor)    )
    {   
        if(    ancestor == (Widget) bb    )
        {   BBHadFocus = TRUE ;
            break ;
            } 
        ancestor = XtParent( ancestor) ;
        }

    if(    BBHasFocus    )
    {   
        /* The widget getting the input focus is a descendent of
        *   or is this Bulletin Board.
        * If there were no descendent Bulletin Boards with default or cancel
        *   buttons, use our own.
        */
        if(    dbutton == NULL   )
        {   dbutton = BB_DefaultButton( bb) ;
            } 
        if(    cbutton == NULL   )
        {   cbutton = BB_CancelButton( bb) ;
            } 

        if(    dbutton == NULL   )
        {   
            /* No descendant has active default button, so be sure
             *   that the dynamic_default_button field for this
             *   ancestor is NULL.
             */
            BB_DynamicDefaultButton( bb) = NULL ;
            } 
        else
        {   
            if(    XmIsPushButton( call_data->new_focus)
                || XmIsPushButtonGadget( call_data->new_focus)    )
            {   
                /* Any pushbutton which gets the focus
                *   gets the default visuals and behavior.
                */
                _XmBulletinBoardSetDynDefaultButton( (Widget) bb,
                                                        call_data->new_focus) ;
                } 
            else /* call_data->new_focus is not a push button */
            {   
                if(    (call_data->focus_policy == XmEXPLICIT)
                    || !XmIsManager( call_data->new_focus)
                    || !call_data->old_focus
                    || (    !XmIsPushButtonGadget( call_data->old_focus)
                         && !XmIsPushButton( call_data->old_focus))    )
                {   
                    /* Avoid setting the default button when in pointer mode,
                    *   leaving a button, and entering the background area.
                    *   The appropriate default button will be set when the
                    *   next focus moved event occurs.
                    *   This will avoid unnecessary flashing of default
                    *   button shadow when moving pointer between buttons.
                    */
                    if(    XtIsManaged( dbutton)    )
                    {   
                        _XmBulletinBoardSetDynDefaultButton( (Widget) bb,
                                                                     dbutton) ;
                        } 
                    } 
                } 
          }
        BB_DynamicCancelButton( bb) = cbutton ;

        if(    !BBHadFocus    )
        {   
            cb.reason = XmCR_FOCUS ;
            cb.event = NULL ;
            XtCallCallbackList( (Widget) bb,
                                      bb->bulletin_board.focus_callback, &cb) ;
            } 
        }
    else /* Bulletin Board does not have focus */
    {   
        if(    BBHadFocus    )
        {   
 	  if(    call_data->new_focus != NULL    )
 	    {
 	      _XmBulletinBoardSetDynDefaultButton( (Widget) bb, NULL) ;
 	    }
#ifdef BB_HAS_LOSING_FOCUS_CB
            cb.reason = XmCR_LOSING_FOCUS ;
            cb.event = NULL ;
            XtCallCallbackList( (Widget) bb,
                               bb->bulletin_board.losing_focus_callback, &cb) ;
#endif
            } 
    }
    BB_InitialFocus( bb) = FALSE ;

    return ;
    }

/****************************************************************/
static Boolean 
#ifdef _NO_PROTO
BulletinBoardParentProcess( wid, event )
        Widget wid ;
        XmParentProcessData event ;
#else
BulletinBoardParentProcess(
        Widget wid,
        XmParentProcessData event )
#endif /* _NO_PROTO */
{
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
/****************/

    if(    (event->any.process_type == XmINPUT_ACTION)
        && (   (    (event->input_action.action == XmPARENT_ACTIVATE)
                 && BB_DynamicDefaultButton( bb))
            || (    (event->input_action.action == XmPARENT_CANCEL)
                 && BB_DynamicCancelButton( bb)))    )
    {   
        if(    event->input_action.action == XmPARENT_ACTIVATE    )
        {   
            _XmBulletinBoardReturn( (Widget)bb, event->input_action.event,
                                      event->input_action.params,
                                         event->input_action.num_params) ;
            } 
        else
        {   _XmBulletinBoardCancel( (Widget)bb, event->input_action.event,
                                      event->input_action.params,
                                         event->input_action.num_params) ;
            } 
        return( TRUE) ;
        } 

    return( _XmParentProcess( XtParent( bb), event)) ;
    }

/****************************************************************
 * Process Return and Enter key events in the BulletinBoard.
 *   If there is a default button, call its Activate callbacks.
 ****************/
void 
#ifdef _NO_PROTO
_XmBulletinBoardReturn( wid, event, params, numParams )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *numParams ;
#else
_XmBulletinBoardReturn(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *numParams )
#endif /* _NO_PROTO */
{   
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
            XmGadgetClass   gadget_class ;
            XmPrimitiveWidgetClass primitive_class ;
            Widget          dbutton = BB_DynamicDefaultButton( bb) ;
/****************/

    if(    !dbutton    )
    {   
        XmParentInputActionRec parentEvent ;

        parentEvent.process_type = XmINPUT_ACTION ;
        parentEvent.action = XmPARENT_ACTIVATE ;
        parentEvent.event = event ;
        parentEvent.params = params ;
        parentEvent.num_params = numParams ;

        _XmParentProcess( XtParent( bb), (XmParentProcessData) &parentEvent) ;
        }
    else
    {   if(    XmIsGadget( dbutton) && XtIsManaged( dbutton)    )
        {   
            gadget_class = (XmGadgetClass) XtClass( dbutton) ;
            if(    gadget_class->gadget_class.arm_and_activate
                && XtIsSensitive( dbutton)    )
            {   (*(gadget_class->gadget_class.arm_and_activate))( dbutton,
                                                    event, params, numParams) ;
                } 
            }
        else
        {   if(    XmIsPrimitive( dbutton) && XtIsManaged( dbutton)    )
            {   
                primitive_class = (XmPrimitiveWidgetClass) XtClass( dbutton) ;
                if(    primitive_class->primitive_class.arm_and_activate
                    && XtIsSensitive( dbutton)    )
                {   (*(primitive_class->primitive_class.arm_and_activate))(
                                           dbutton, event, params, numParams) ;
                    } 
                }
            else
            {   if(    XtIsSensitive( dbutton)
                    && (XtHasCallbacks( dbutton, XmNactivateCallback)
                                                     == XtCallbackHasSome)    )
                {   XmAnyCallbackStruct cb ;

		    cb.reason = XmCR_ACTIVATE ;
                    cb.event = event ;
                    XtCallCallbacks( dbutton, XmNactivateCallback, 
                                                             (XtPointer) &cb) ;
                    } 
                }
            }
        } 
    return ;
    }

/****************************************************************
 * Process Cancel key events in the BulletinBoard.
 * If there is a cancel button, call its Activate callbacks.
 ****************/
void 
#ifdef _NO_PROTO
_XmBulletinBoardCancel( wid, event, params, numParams )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *numParams ;
#else
_XmBulletinBoardCancel(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *numParams )
#endif /* _NO_PROTO */
{   
            XmBulletinBoardWidget bb = (XmBulletinBoardWidget) wid ;
            XmGadgetClass   gadget_class ;
            XmPrimitiveWidgetClass primitive_class ;
            Widget          cbutton = BB_DynamicCancelButton( bb) ;
/****************/

    if(    !cbutton    )
    {   
        XmParentInputActionRec parentEvent ;

        parentEvent.process_type = XmINPUT_ACTION ;
        parentEvent.action = XmPARENT_CANCEL ;
        parentEvent.event = event ;
        parentEvent.params = params ;
        parentEvent.num_params = numParams ;

        _XmParentProcess( XtParent( bb), (XmParentProcessData) &parentEvent) ;
        } 
    else
    {   if(    XmIsGadget( cbutton) && XtIsManaged( cbutton)    )
        {   
            gadget_class = (XmGadgetClass) XtClass( cbutton) ;
            if(    gadget_class->gadget_class.arm_and_activate
                && XtIsSensitive( cbutton)    )
            {   (*(gadget_class->gadget_class.arm_and_activate))( cbutton,
                                                    event, params, numParams) ;
                } 
            }
        else
        {   if(    XmIsPrimitive( cbutton) && XtIsManaged( cbutton)    )
            {   
                primitive_class = (XmPrimitiveWidgetClass) XtClass( cbutton) ;
                if(    primitive_class->primitive_class.arm_and_activate
                    && XtIsSensitive( cbutton)    )
                {   (*(primitive_class->primitive_class.arm_and_activate))( 
                                           cbutton, event, params, numParams) ;
                    } 
                }
            else
            {   if(    XtIsSensitive( cbutton)
                    && (XtHasCallbacks( cbutton, XmNactivateCallback)
                                                     == XtCallbackHasSome)    )
                {   XmAnyCallbackStruct cb ;

		    cb.reason = XmCR_ACTIVATE ;
                    cb.event = event ;
                    XtCallCallbacks( cbutton, XmNactivateCallback,
                                                             (XtPointer) &cb) ;
                    } 
                }
            }
        }
    return ;
    }

 
void 
#ifdef _NO_PROTO
_XmBulletinBoardMap( wid, event, params, numParams )
         Widget wid ;
         XEvent *event ;
         String *params ;
         Cardinal *numParams ;
#else
_XmBulletinBoardMap(
         Widget wid,
         XEvent *event,
         String *params,
         Cardinal *numParams )
#endif /* _NO_PROTO */
 {   
   if(    BB_DefaultButton( wid)    )
     {
       Widget focus_hier = _XmGetFirstFocus( wid) ;
 
       while(    focus_hier
 	    &&  !XtIsShell( focus_hier)    )
 	{
 	  if(    focus_hier == wid    )
 	    {
 	      _XmBulletinBoardSetDynDefaultButton( wid,
 						      BB_DefaultButton( wid)) ;
 	      break ;
 	    }
 	  if(    XmIsBulletinBoard( focus_hier)
 	     &&  BB_DefaultButton( focus_hier)    )
 	    {
 	      break ;
 	    }
 	  focus_hier = XtParent( focus_hier) ;
 	}
     }
 }
 
/*
 * Copy the XmString in XmNdialogTitle before returning it to the user.
 */
static void
#ifdef _NO_PROTO
GetDialogTitle( bb, resource, value)
        Widget bb ;
        int resource ;
        XtArgVal *value ;
#else
GetDialogTitle(
        Widget bb,
        int resource,
        XtArgVal *value)
#endif
{
    XmString      data ;
/****************/

    data = XmStringCopy((XmString) ((XmBulletinBoardWidget) bb)
                                                ->bulletin_board.dialog_title);
    *value = (XtArgVal) data ;

    return ;
    }


/****************************************************************/
void 
#ifdef _NO_PROTO
_XmBulletinBoardSetDefaultShadow( button )
        Widget button ;
#else
_XmBulletinBoardSetDefaultShadow(
        Widget button )
#endif /* _NO_PROTO */
{   
            Arg             argv[2] ;
            Cardinal        argc ;
            Dimension       dbShadowTh ;
            Dimension       shadowTh ;
/****************/

    if(    XmIsPushButtonGadget( button)    )
    {   _XmClearBGCompatibility( button) ;
        } 
    else
    {   if(    XmIsPushButton( button)    )
        {   _XmClearBCompatibility( button) ;
            } 
        } 
    argc = 0 ;
    XtSetArg( argv[argc], XmNshadowThickness, &shadowTh) ; ++argc ;
    XtSetArg( argv[argc], XmNdefaultButtonShadowThickness, 
                                                        &dbShadowTh) ; ++argc ;
    XtGetValues( button, argv, argc) ;
    
    if(    !dbShadowTh    )
    {   if(    shadowTh > 1    )
        {   dbShadowTh = shadowTh >> 1 ;
            }
        else
        {   dbShadowTh = shadowTh ;
            } 
        argc = 0 ;
        XtSetArg( argv[argc], XmNdefaultButtonShadowThickness, 
                                                         dbShadowTh) ; ++argc ;
        XtSetValues( button, argv, argc) ;
        } 
    return ;
    }

/****************************************************************
 * If the default button of the bulletin board widget (wid) is not equal to
 *   the specified default button (newDefaultButton), then SetValues is called
 *   to turn off/on the appropriate default borders and the default button
 *   field of the bulletin board instance record is updated appropriately.
  ****************/
void 
#ifdef _NO_PROTO
_XmBulletinBoardSetDynDefaultButton( wid, newDefaultButton )
        Widget wid ;
        Widget newDefaultButton ;
#else
_XmBulletinBoardSetDynDefaultButton(
        Widget wid,
        Widget newDefaultButton )
#endif /* _NO_PROTO */
{
            XmBulletinBoardWidget bbWid = (XmBulletinBoardWidget) wid ;
            Arg             args[1] ;
/****************/

    if(    newDefaultButton != BB_DynamicDefaultButton( bbWid)    )
    {   
        if(BB_DynamicDefaultButton( bbWid) && 
	   !(BB_DynamicDefaultButton(bbWid)->core.being_destroyed))
        {   XtSetArg( args[0], XmNshowAsDefault, FALSE) ;
            XtSetValues( BB_DynamicDefaultButton( bbWid), args, 1) ;
            } 
        BB_DynamicDefaultButton( bbWid) = newDefaultButton ;

        if(    newDefaultButton    )
        {   
            if(    XtParent( newDefaultButton) != (Widget) bbWid    )
            {   
                if(    XmIsPushButtonGadget( newDefaultButton)    )
                {   _XmClearBGCompatibility( newDefaultButton) ;
                    } 
                else
                {   if(    XmIsPushButton( newDefaultButton)    )
                    {   _XmClearBCompatibility( newDefaultButton) ;
                        } 
                    } 
                } 
            XtSetArg( args[0], XmNshowAsDefault, TRUE) ;
            XtSetValues( newDefaultButton, args, 1) ;
            } 
        } 
    return ;
    }

 
void
#ifdef _NO_PROTO
_XmBBUpdateDynDefaultButton( bb)
	Widget bb ;
#else
_XmBBUpdateDynDefaultButton(
 	Widget bb)
#endif /* _NO_PROTO */
 {
   Widget bbwdb = GetBBWithDB( bb) ;
 
   if(    bbwdb == NULL    )
     {
       if(    ((XmBulletinBoardWidget) bb)
 	                           ->bulletin_board.dynamic_default_button    )
 	{
 	  _XmBulletinBoardSetDynDefaultButton( bb, NULL) ;
 	}
     }
   else
     {
       if(    bbwdb == bb    )
 	{
 	  _XmBulletinBoardSetDynDefaultButton( bb, BB_DefaultButton( bb)) ;
 	}
     }
 }
 
static Widget
#ifdef _NO_PROTO
GetBBWithDB( wid)
 	Widget wid ;
#else
GetBBWithDB(
 	Widget wid)
#endif /* _NO_PROTO */
 {
   Widget focus ;
 
   if(    (_XmGetFocusPolicy( wid) == XmEXPLICIT)
      &&  (    (focus = XmGetFocusWidget( wid))
 	  ||  (focus = _XmGetFirstFocus( wid)))    )
     {
       while(    focus
 	    &&  !XtIsShell( focus)    )
 	{
 	  if(    XmIsBulletinBoard( focus)
 	     &&  BB_DefaultButton( focus)    )
 	    {
 	      return focus ;
 	    }
 	  focus = XtParent( focus) ;
 	}
 
     }
   return NULL ;
 }


/****************************************************************
 * This function creates and returns a BulletinBoard widget.
 ****************/
Widget 
#ifdef _NO_PROTO
XmCreateBulletinBoard( p, name, args, n )
        Widget p ;
        String name ;
        ArgList args ;
        Cardinal n ;
#else
XmCreateBulletinBoard(
        Widget p,
        String name,
        ArgList args,
        Cardinal n )
#endif /* _NO_PROTO */
{   
/****************/

    return( XtCreateWidget( name, xmBulletinBoardWidgetClass, p, args, n)) ;
    }
/****************************************************************
 * This convenience function creates a DialogShell and a BulletinBoard
 *   child of the shell; returns the BulletinBoard widget.
 ****************/
Widget 
#ifdef _NO_PROTO
XmCreateBulletinBoardDialog( ds_p, name, bb_args, bb_n )
        Widget ds_p ;
        String name ;
        ArgList bb_args ;
        Cardinal bb_n ;
#else
XmCreateBulletinBoardDialog(
        Widget ds_p,
        String name,
        ArgList bb_args,
        Cardinal bb_n )
#endif /* _NO_PROTO */
{   
            Widget		bb ;		/*  BulletinBoard	*/
            Widget		ds ;		/*  DialogShell		*/
            ArgList		ds_args ;	/*  arglist for shell	*/
            char *              ds_name ;
/****************/

    /*	Create DialogShell parent.
    */
    ds_name = XtMalloc( (strlen(name)+XmDIALOG_SUFFIX_SIZE+1) * sizeof(char)) ;
    strcpy( ds_name, name) ;
    strcat( ds_name, XmDIALOG_SUFFIX) ;

    ds_args = (ArgList) XtMalloc( sizeof( Arg) * (bb_n + 1)) ;
    memcpy( ds_args, bb_args, (sizeof( Arg) * bb_n)) ;
    XtSetArg( ds_args[bb_n], XmNallowShellResize, True) ; 
    ds = XmCreateDialogShell( ds_p, ds_name, ds_args, bb_n + 1) ;

    XtFree( (char *) ds_args);
    XtFree( ds_name) ;
    /*	Create BulletinBoard.
    */
    bb = XtCreateWidget( name, xmBulletinBoardWidgetClass, ds, bb_args, bb_n) ;
    /*	Add callback to destroy DialogShell parent.
    */
    XtAddCallback( bb, XmNdestroyCallback, _XmDestroyParentCallback, 
                                                            (XtPointer) NULL) ;
    /*	Return BulletinBoard.
    */
    return( bb) ;
    }
/****************************************************************/
