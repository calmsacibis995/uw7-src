#pragma ident	"@(#)m1.2libs:Xm/MessageB.c	1.3"
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

#include "XmI.h"
#include "RepTypeI.h"
#include <Xm/MessageBP.h>

#include <Xm/RowColumnP.h>
#include <Xm/LabelG.h>
#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/DrawnB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/DialogS.h>
#include <Xm/SeparatoG.h>
#include "MessagesI.h"
#include <string.h>
#include <Xm/VendorSP.h>  /* for the default display */


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


/* convenience macros */
#define Double(d)        ((d) << 1)
#define TotalWidth(w)    ((w)->core.width + Double ((w)->core.border_width))
#define TotalHeight(w)   ((w)->core.height + Double ((w)->core.border_width))
#define BottomEdge(w)    ((w)->core.y + TotalHeight (w))

#define IsButton(w) ( \
      XmIsPushButton(w) || XmIsPushButtonGadget(w) || \
      XmIsToggleButton(w) || XmIsToggleButtonGadget(w) || \
      XmIsArrowButton(w) || XmIsArrowButtonGadget(w) || \
      XmIsDrawnButton(w))
      
#define IsAutoChild(mb, w) ( \
      (w) == mb->message_box.symbol_wid || \
      (w) == mb->message_box.message_wid || \
      (w) == mb->message_box.separator || \
      (w) == mb->message_box.ok_button || \
      (w) == mb->bulletin_board.cancel_button || \
      (w) == mb->message_box.help_button)

#define ARG_LIST_CNT    25

#define PIXMAP_LIST_MOD 6

/* defines for warning message */

#ifdef I18N_MSG
#define WARNING4	catgets(Xm_catd,MS_MBox,MSG_MB_4,_XmMsgMessageB_0003)
#define WARNING5	catgets(Xm_catd,MS_MBox,MSG_MB_5,_XmMsgMessageB_0004)
#else
#define WARNING4	_XmMsgMessageB_0003
#define WARNING5	_XmMsgMessageB_0004
#endif



/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XImage * CreateDefaultImage() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Destroy() ;
static void DeleteChild() ;
static void MessageCallback() ;
static Boolean SetValues() ;
static void SetUpSymbol() ;
static void SetUpMessage() ;
static Widget CreateShell() ;
static void CreateWidgets() ;
static Widget _XmCreateDialog() ;
static void GetMessageString() ;
static void GetSymbolPixmap() ;
static void GetOkLabelString() ;
static void GetCancelLabelString() ;
static void GetHelpLabelString() ;
static void GetMsgBoxPixmap() ;

#else

static XImage * CreateDefaultImage( 
                        char *bits,
                        unsigned int width,
                        unsigned int height) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget wid) ;
static void DeleteChild( 
                        Widget child) ;
static void MessageCallback( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void SetUpSymbol( 
                        XmMessageBoxWidget w) ;
static void SetUpMessage( 
                        XmMessageBoxWidget w) ;
static Widget CreateShell( 
                        Widget parent,
                        char *name,
                        ArgList args,
                        Cardinal ac) ;
static void CreateWidgets( 
                        XmMessageBoxWidget w) ;
static Widget _XmCreateDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        int ac,
#if NeedWidePrototypes
                        unsigned int type) ;
#else
                        unsigned char type) ;
#endif /* NeedWidePrototypes */
static void GetMessageString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void GetSymbolPixmap( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void GetOkLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void GetCancelLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void GetHelpLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void GetMsgBoxPixmap( 
                        XmMessageBoxWidget mBox) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*  Resource definitions, "get resources" first */

static XmSyntheticResource syn_resources[] = {

    {   XmNmessageString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.message_string),
        GetMessageString,
		NULL},

    {   XmNsymbolPixmap,
        sizeof (Pixmap),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.symbol_pixmap),
        GetSymbolPixmap,
		NULL},

    {   XmNokLabelString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.ok_label_string),
        GetOkLabelString,
		NULL},

    {   XmNcancelLabelString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.cancel_label_string),
        GetCancelLabelString,
		NULL},

    {   XmNhelpLabelString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.help_label_string),
        GetHelpLabelString,
		NULL},
};


static XtResource resources[] = {

    {   XmNdialogType,
        XmCDialogType,
        XmRDialogType,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.dialog_type),
        XmRImmediate,
        (XtPointer) XmDIALOG_MESSAGE},

    {   XmNminimizeButtons,
        XmCMinimizeButtons,
        XmRBoolean,
        sizeof(Boolean),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.minimize_buttons),
        XmRImmediate,
        (XtPointer) False},

    {   XmNdefaultButtonType,
        XmCDefaultButtonType,
        XmRDefaultButtonType,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.default_type),
        XmRImmediate,
        (XtPointer) XmDIALOG_OK_BUTTON},

    {   XmNmessageString,
        XmCXmString,
        XmRXmString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.message_string),
        XmRXmString,
        NULL},

    {   XmNmessageAlignment,
        XmCAlignment,
        XmRAlignment,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.message_alignment),
        XmRImmediate,
        (XtPointer) XmALIGNMENT_BEGINNING},

    {   XmNsymbolPixmap,
        XmCPixmap,
        XmRManForegroundPixmap,
        sizeof (Pixmap),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.symbol_pixmap),
        XmRImmediate,
        (XtPointer) XmUNSPECIFIED_PIXMAP},

    {   XmNokLabelString,
        XmCXmString,
        XmRXmString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.ok_label_string),
        XmRXmString,
        NULL},             /* "OK" default dynamically set from label name */

    {   XmNokCallback,
        XmCCallback,
        XmRCallback,
        sizeof (XtCallbackList),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.ok_callback),
        XmRCallback,
        NULL},

    {   XmNcancelLabelString,
        XmCXmString,
        XmRXmString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.cancel_label_string),
        XmRXmString,
        NULL},            /* "Cancel" default dynamically set from label name */

    {   XmNcancelCallback,
        XmCCallback,
        XmRCallback,
        sizeof (XtCallbackList),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.cancel_callback),
        XmRCallback,
        NULL},

    {   XmNhelpLabelString,
        XmCXmString,
        XmRXmString,
        sizeof (XmString),
        XtOffsetOf( struct _XmMessageBoxRec, message_box.help_label_string),
        XmRXmString,
        NULL},             /* "Help" default dynamically set from label name */

};



/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

externaldef( xmmessageboxclassrec) XmMessageBoxClassRec xmMessageBoxClassRec =
{
   {                                            /* core_class fields  */
      (WidgetClass) &xmBulletinBoardClassRec,   /* superclass         */
      "XmMessageBox",                           /* class_name         */
      sizeof(XmMessageBoxRec),                  /* widget_size        */
      ClassInitialize,                          /* class_initialize   */
      ClassPartInitialize,                      /* class_part_init    */
      FALSE,                                    /* class_inited       */
      Initialize,                               /* initialize         */
      NULL,                                     /* initialize_hook    */
      XtInheritRealize,                         /* realize            */
      NULL,                                     /* actions            */
      0,                                        /* num_actions        */
      resources,                                /* resources          */
      XtNumber(resources),                      /* num_resources      */
      NULLQUARK,                                /* xrm_class          */
      TRUE,                                     /* compress_motion    */
      XtExposeCompressMaximal,                  /* compress_exposure  */
      FALSE,                                    /* compress_enterlv   */
      FALSE,                                    /* visible_interest   */
      Destroy,                                  /* destroy            */
      XtInheritResize,                          /* resize             */
      XtInheritExpose,                          /* expose             */
      SetValues,                                /* set_values         */
      NULL,                                     /* set_values_hook    */
      XtInheritSetValuesAlmost,                 /* set_values_almost  */
      NULL,                                     /* get_values_hook    */
      XtInheritAcceptFocus,                     /* enter_focus        */
      XtVersion,                                /* version            */
      NULL,                                     /* callback_private   */
      XtInheritTranslations,                    /* tm_table           */
      XtInheritQueryGeometry,                   /* query_geometry     */
      NULL,                                     /* display_accelerator*/
      NULL,                                     /* extension          */
   },

   {                                            /* composite_class fields */
      XtInheritGeometryManager,                 /* geometry_manager   */
      XtInheritChangeManaged,                   /* change_managed     */
      XtInheritInsertChild,                     /* insert_child       */
      DeleteChild,                              /* delete_child       */
      NULL,                                     /* extension          */
   },

   {                                            /* constraint_class fields */
      NULL,                                     /* resource list        */   
      0,                                        /* num resources        */   
      0,                                        /* constraint size      */   
      NULL,                                     /* init proc            */   
      NULL,                                     /* destroy proc         */   
      NULL,                                     /* set values proc      */   
      NULL,                                     /* extension            */
   },

   {                                            /* manager_class fields   */
      XmInheritTranslations,                    /* translations           */
      syn_resources,                            /* syn_resources          */
      XtNumber(syn_resources),                  /* num_syn_resources      */
      NULL,                                     /* syn_cont_resources     */
      0,                                        /* num_syn_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,                                     /* extension              */
   },

   {                                            /* bulletinBoard class  */
      TRUE,                                     /*always_install_accelerators*/
      _XmMessageBoxGeoMatrixCreate,             /* geo__matrix_create */
      XmInheritFocusMovedProc,                  /* focus_moved_proc */
      NULL                                      /* extension */
   },   

   {                                            /* messageBox class */
      (XtPointer) NULL                          /* extension */
   }    
};

externaldef( xmmessageboxwidgetclass) WidgetClass xmMessageBoxWidgetClass
                                        = (WidgetClass) &xmMessageBoxClassRec ;


/************************************************************************
 *  Bitmap Data for Default Symbol
 **********************************<->***********************************/

static unsigned char errorBits[] = {
   0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0xf0, 0x3a, 0x00, 0x58, 0x55, 0x00,
   0x2c, 0xa0, 0x00, 0x56, 0x40, 0x01, 0xaa, 0x80, 0x02, 0x46, 0x81, 0x01,
   0x8a, 0x82, 0x02, 0x06, 0x85, 0x01, 0x0a, 0x8a, 0x02, 0x06, 0x94, 0x01,
   0x0a, 0xe8, 0x02, 0x14, 0x50, 0x01, 0x28, 0xb0, 0x00, 0xd0, 0x5f, 0x00,
   0xa0, 0x2a, 0x00, 0x40, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static unsigned char infoBits[] = {
   0x00, 0x00, 0x78, 0x00, 0x54, 0x00, 0x2c, 0x00, 0x54, 0x00, 0x28, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x2a, 0x00, 0x5c, 0x00, 0x28, 0x00,
   0x58, 0x00, 0x28, 0x00, 0x58, 0x00, 0x28, 0x00, 0x58, 0x00, 0x28, 0x00,
   0x58, 0x00, 0xae, 0x01, 0x56, 0x01, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00};

static unsigned char questionBits[] = {
   0xf0, 0x3f, 0x00, 0x58, 0x55, 0x00, 0xac, 0xaa, 0x00, 0xd6, 0x5f, 0x01,
   0xea, 0xbf, 0x02, 0xf6, 0x7f, 0x01, 0xea, 0xba, 0x02, 0xf6, 0x7d, 0x05,
   0xea, 0xba, 0x0a, 0x56, 0x7d, 0x15, 0xaa, 0xbe, 0x1e, 0x56, 0x5f, 0x01,
   0xac, 0xaf, 0x02, 0x58, 0x57, 0x01, 0xb0, 0xaf, 0x00, 0x60, 0x55, 0x01,
   0xa0, 0xaa, 0x00, 0x60, 0x17, 0x00, 0xa0, 0x2f, 0x00, 0x60, 0x17, 0x00,
   0xb0, 0x2a, 0x00, 0x50, 0x55, 0x00};

static unsigned char warningBits[] = {
   0x00, 0x00, 0x18, 0x00, 0x2c, 0x00, 0x56, 0x00, 0x2a, 0x00, 0x56, 0x00,
   0x2a, 0x00, 0x56, 0x00, 0x2c, 0x00, 0x14, 0x00, 0x2c, 0x00, 0x14, 0x00,
   0x2c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x14, 0x00,
   0x2c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00};

static unsigned char workingBits[] = {
   0x00, 0x00, 0x00, 0xfe, 0xff, 0x0f, 0xaa, 0xaa, 0x0a, 0x44, 0x55, 0x06,
   0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06, 0xcc, 0x2a, 0x02, 0x84, 0x15, 0x06,
   0x8c, 0x2a, 0x02, 0x04, 0x15, 0x06, 0x0c, 0x0a, 0x02, 0x04, 0x06, 0x06,
   0x0c, 0x0b, 0x02, 0x84, 0x15, 0x06, 0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06,
   0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06, 0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06,
   0xfe, 0xff, 0x0f, 0x56, 0x55, 0x05, 0x00, 0x00, 0x00};



/****************************************************************
 * Create a default images for symbol... used in ClassInitialize.
 ****************/
static XImage * 
#ifdef _NO_PROTO
CreateDefaultImage( bits, width, height )
        char *bits ;
        unsigned int width ;
        unsigned int height ;
#else
CreateDefaultImage(
        char *bits,
        unsigned int width,
        unsigned int height )
#endif /* _NO_PROTO */
{
    XImage *        image ;
    Display * display = _XmGetDefaultDisplay() ; /* we don't have one here */

    _XmCreateImage(image, display, bits, width, height, LSBFirst);

    return( image) ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
    XImage *image;
/****************/

    /* create and install the default images for the symbol */

    image = CreateDefaultImage ((char *)errorBits, 20, 20);
    XmInstallImage (image, "default_xm_error");

    image = CreateDefaultImage ((char *)infoBits, 11, 24);
    XmInstallImage (image, "default_xm_information");

    image = CreateDefaultImage ((char *)questionBits, 22, 22);
    XmInstallImage (image, "default_xm_question");

    image = CreateDefaultImage ((char *)warningBits, 9, 22);
    XmInstallImage (image, "default_xm_warning");

    image = CreateDefaultImage ((char *)workingBits, 21, 23);
    XmInstallImage (image, "default_xm_working");

    return ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
/****************/

    _XmFastSubclassInit (wc, XmMESSAGE_BOX_BIT);

    return ;
}
/****************************************************************
 * MessageBox widget specific initialization
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
            XmMessageBoxWidget new_w = (XmMessageBoxWidget) nw ;
            Widget          defaultButton ;
/****************/

    new_w->message_box.message_wid = NULL;
    new_w->message_box.symbol_wid = NULL;
    new_w->message_box.separator = NULL;
    new_w->message_box.ok_button = NULL;
    new_w->bulletin_board.cancel_button = NULL;
    new_w->message_box.help_button = NULL;
    new_w->bulletin_board.default_button = NULL;
    new_w->message_box.internal_pixmap = FALSE ;

    if(    !XmRepTypeValidValue( XmRID_DIALOG_TYPE,
                               new_w->message_box.dialog_type, (Widget) new_w)    )
    {   new_w->message_box.dialog_type = XmDIALOG_MESSAGE ;
        } 

    if(    !XmRepTypeValidValue( XmRID_ALIGNMENT,
                         new_w->message_box.message_alignment, (Widget) new_w)    )
    {   new_w->message_box.message_alignment = XmALIGNMENT_BEGINNING ;
        } 

    CreateWidgets( new_w) ;

    if(    !XmRepTypeValidValue( XmRID_DEFAULT_BUTTON_TYPE,
                              new_w->message_box.default_type, (Widget) new_w)    )
    {   new_w->message_box.default_type = XmDIALOG_OK_BUTTON ;
        } 
    switch(    new_w->message_box.default_type    )
    {   
        case XmDIALOG_CANCEL_BUTTON:
        {   defaultButton = new_w->bulletin_board.cancel_button ;
            break ;
            } 
        case XmDIALOG_HELP_BUTTON:
        {   defaultButton = new_w->message_box.help_button ;
            break ;
            } 
        case XmDIALOG_OK_BUTTON:
        {   defaultButton = new_w->message_box.ok_button ;
            break ;
            } 
        default:
	{   defaultButton = NULL ;
	    }
        } 
      if(    defaultButton    )
      {
          BB_DefaultButton( new_w) = defaultButton ;
          new_w->manager.initial_focus = defaultButton ;
          _XmBulletinBoardSetDynDefaultButton( (Widget) new_w, defaultButton) ;
      }
    return ;
    }
/****************************************************************
 * Destroy the widget specific data structs
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
            XmMessageBoxWidget d = (XmMessageBoxWidget) wid ;
/****************/

    if(    (d->message_box.symbol_pixmap != XmUNSPECIFIED_PIXMAP)
        && d->message_box.internal_pixmap    )
    {   
        XmDestroyPixmap( d->core.screen, d->message_box.symbol_pixmap) ;
        } 
    return ;
    }

/****************************************************************
 * Set the widget id to NULL for the child widget being destroyed.
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
            XmMessageBoxWidget mbox ;
/****************/

    if(    XtIsRectObj( child)    )
    {   
        mbox = (XmMessageBoxWidget) XtParent( child) ;

        /* Check for which child is getting destroyed and set to NULL.
        */
        if(    child == mbox->message_box.message_wid    )
        {   mbox->message_box.message_wid = NULL ;
            } 
        else
        {   if(    child == mbox->message_box.symbol_wid    )
            {   mbox->message_box.symbol_wid = NULL ;
                } 
            else
            {   if(    child == mbox->message_box.ok_button    )
                {   mbox->message_box.ok_button = NULL ;
                    } 
                else
                {   if(    child == mbox->message_box.help_button    )
                    {   mbox->message_box.help_button = NULL ;
                        } 
                    else
                    {   if(    child == mbox->message_box.separator    )
                        {   mbox->message_box.separator = NULL ;
                            } 
                        } 
                    } 
                } 
            }
        }
    (*((XmBulletinBoardWidgetClass) xmBulletinBoardWidgetClass)
                                      ->composite_class.delete_child)( child) ;
    return ;
    }

/****************************************************************/
static void 
#ifdef _NO_PROTO
MessageCallback( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
MessageCallback(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
            XmMessageBoxWidget tag = (XmMessageBoxWidget) closure ;

    XmAnyCallbackStruct temp;
/****************/

    if (call_data) {
    	temp.reason  = ((XmAnyCallbackStruct *) call_data)->reason;
    	temp.event   = ((XmAnyCallbackStruct *) call_data)->event;
	}
    else {
    	temp.reason  = 0;
    	temp.event   = NULL;
    }

    if (tag->message_box.ok_button == w) {
        temp.reason = XmCR_OK;
        XtCallCallbackList ((Widget) tag, tag->message_box.ok_callback, &temp);
    }
    else if (tag->bulletin_board.cancel_button == w) {
        temp.reason = XmCR_CANCEL;
        XtCallCallbackList ((Widget) tag, tag->message_box.cancel_callback, &temp);
    }
    else if (tag->message_box.help_button == w) {
        temp.reason  = XmCR_HELP;
        XtCallCallbackList ((Widget) tag, tag->manager.help_callback, &temp);
    }
    return ;
}

/****************************************************************
 * Set attributes of a message widget
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
            XmMessageBoxWidget current = (XmMessageBoxWidget) cw ;
            XmMessageBoxWidget new_w = (XmMessageBoxWidget) nw ;
            Arg             al[ARG_LIST_CNT] ;  /* arg list       */
            Cardinal        ac ;                /* arg list count */
            Boolean         need_layout = FALSE ;
            Boolean         newPixmap = FALSE ;
            Widget          defaultButton ;
/****************/

    /* "in_set_values" means the GeometryManager won't try to resize
    *    and/or re-layout subwidgets.
    */
    BB_InSetValues( new_w) = True;

	    /* fix for CR 5895 */
    if(    !new_w->message_box.symbol_pixmap    )
    {   new_w->message_box.symbol_pixmap = XmUNSPECIFIED_PIXMAP ;
        } 

    /* Over-ride BBoard to disallow a direct change of button id's
    */
    if ( current->bulletin_board.cancel_button
                                        != new_w->bulletin_board.cancel_button) {
        new_w->bulletin_board.cancel_button 
                                       = current->bulletin_board.cancel_button;
        _XmWarning( (Widget) new_w, WARNING5);
    }

    if(    (current->message_box.dialog_type != new_w->message_box.dialog_type)
        && !XmRepTypeValidValue( XmRID_DIALOG_TYPE,
                               new_w->message_box.dialog_type, (Widget) new_w)    )
    {   
        new_w->message_box.dialog_type = current->message_box.dialog_type ;
        }
    if (new_w->message_box.symbol_pixmap == (Pixmap)0)
    {
       new_w->message_box.symbol_pixmap = XmUNSPECIFIED_PIXMAP;
    }
    if(    new_w->message_box.symbol_pixmap
                                     != current->message_box.symbol_pixmap    )
    {   newPixmap = TRUE ;
        new_w->message_box.internal_pixmap = FALSE ;

        if(    (current->message_box.symbol_pixmap != XmUNSPECIFIED_PIXMAP)
            && current->message_box.internal_pixmap    )
        {   
            XmDestroyPixmap( current->core.screen,
                                          current->message_box.symbol_pixmap) ;
            } 
        } 
    else
    {   /* If symbol pixmap is unchanged and a new dialog type is specified,
        *   then set to default pixmap.
        */
        if(    new_w->message_box.dialog_type
                                       != current->message_box.dialog_type    )
        {   newPixmap = TRUE ;

            GetMsgBoxPixmap( new_w) ;

            if(    (current->message_box.symbol_pixmap != XmUNSPECIFIED_PIXMAP)
                && current->message_box.internal_pixmap    )
            {   
                XmDestroyPixmap( current->core.screen, 
                                          current->message_box.symbol_pixmap) ;
                } 
            }
        } 
    if(    newPixmap    )
    {   
        need_layout = TRUE ;

        if(    new_w->message_box.symbol_wid    ) 
        {   
            ac = 0 ;
            XtSetArg( al[ac], XmNlabelPixmap, 
                                       new_w->message_box.symbol_pixmap) ; ++ac ;
            XtSetValues( new_w->message_box.symbol_wid, al, ac) ;
            }
        } 
    /* Check the buttons and labels
    */
    ac = 0 ;
    if(    new_w->message_box.message_string    )
    {   
        XtSetArg( al[ac], XmNlabelString, 
                                      new_w->message_box.message_string) ; ++ac ;
        XtSetArg( al[ac], XmNstringDirection,
                                             BB_StringDirection( new_w)) ; ++ac ;
        new_w->message_box.message_string = NULL ;
        need_layout = TRUE ;
        }
    if(    current->message_box.message_alignment
                                     != new_w->message_box.message_alignment    )
    {   if(    !XmRepTypeValidValue( XmRID_ALIGNMENT, 
                         new_w->message_box.message_alignment, (Widget) new_w)    )
        {   new_w->message_box.message_alignment
                                     = current->message_box.message_alignment ;
            } 
        else
        {   XtSetArg( al[ac], XmNalignment, 
                                   new_w->message_box.message_alignment) ; ++ac ;
            need_layout = TRUE ;
            } 
        }
    if(    ac  &&  new_w->message_box.message_wid    )
    {   XtSetValues( new_w->message_box.message_wid, al, ac) ;
        } 

    if(    new_w->message_box.ok_label_string    )
    {   
        if(    new_w->message_box.ok_button    )
        {   ac = 0 ;
            XtSetArg( al[ac], XmNlabelString, 
                                     new_w->message_box.ok_label_string) ; ++ac ;
            XtSetArg( al[ac], XmNstringDirection,
                                             BB_StringDirection( new_w)) ; ++ac ;
            XtSetValues( new_w->message_box.ok_button, al, ac) ;
            } 
        new_w->message_box.ok_label_string = NULL ;
        need_layout = TRUE ;
        } 
    if(    new_w->message_box.cancel_label_string    )
    {   
        if(    new_w->bulletin_board.cancel_button    )
        {   ac = 0 ;
            XtSetArg( al[ac], XmNlabelString, 
                                 new_w->message_box.cancel_label_string) ; ++ac ;
            XtSetArg( al[ac], XmNstringDirection,
                                             BB_StringDirection( new_w)) ; ++ac ;
            XtSetValues( new_w->bulletin_board.cancel_button, al, ac) ;
            } 
        new_w->message_box.cancel_label_string = NULL ;
        need_layout = TRUE ;
        } 
    if(    new_w->message_box.help_label_string    )
    {   
        if(    new_w->message_box.help_button    )
        {   ac = 0 ;
            XtSetArg( al[ac], XmNlabelString, 
                                   new_w->message_box.help_label_string) ; ++ac ;
            XtSetArg( al[ac], XmNstringDirection,
                                             BB_StringDirection( new_w)) ; ++ac ;
            XtSetValues( new_w->message_box.help_button, al, ac) ;
            } 
        new_w->message_box.help_label_string = NULL ;
        need_layout = TRUE ;
        } 

    /* If Default Pushbutton changes, reset showAsDefault.
    */
    if(    current->message_box.default_type
                                        != new_w->message_box.default_type    )
    {   if(    !XmRepTypeValidValue( XmRID_DEFAULT_BUTTON_TYPE,
                          new_w->message_box.default_type, (Widget) new_w)    )
        {   new_w->message_box.default_type
	                                  = current->message_box.default_type ;
            } 
        else
        {   switch(    new_w->message_box.default_type    )
            {   
                case XmDIALOG_CANCEL_BUTTON:
                {   defaultButton = new_w->bulletin_board.cancel_button ;
                    break ;
                    } 
                case XmDIALOG_HELP_BUTTON:
                {   defaultButton = new_w->message_box.help_button ;
                    break ;
                    } 
                case XmDIALOG_OK_BUTTON:
                {   defaultButton = new_w->message_box.ok_button ;
                    break ;
                    }
                default:
		{   defaultButton = NULL ;
		    }
        }
		BB_DefaultButton( new_w) = defaultButton ;
		_XmBulletinBoardSetDynDefaultButton( (Widget) new_w, defaultButton);

	    if(    (current->manager.initial_focus
	                                     == BB_DefaultButton( current))
	       &&  (current->manager.initial_focus
		                          == new_w->manager.initial_focus)    )
	    {   new_w->manager.initial_focus = defaultButton ;
		_XmSetInitialOfTabGroup( (Widget) new_w, defaultButton) ;
	        }
	    }
        }
    BB_InSetValues( new_w) = False;

    /* Re-layout the sub-widgets if necessary
    */
    if(    need_layout
        && (XtClass( new_w) == xmMessageBoxWidgetClass)    )
    {
        _XmBulletinBoardSizeUpdate( (Widget) new_w) ;
        }
    return( FALSE) ;
    }

/****************************************************************
 * Set up the icon (pixmap label widget) and the label widget itself.
 ****************/
static void 
#ifdef _NO_PROTO
SetUpSymbol( w )
        XmMessageBoxWidget w ;
#else
SetUpSymbol(
        XmMessageBoxWidget w )
#endif /* _NO_PROTO */
{
    Arg al[ARG_LIST_CNT];
    int ac;
/****************/

    /* If no pixmap specified, try to get from bitmap file or default
    */
    if(    w->message_box.symbol_pixmap == XmUNSPECIFIED_PIXMAP    )
    {   GetMsgBoxPixmap( w) ;
        }

    if(    !w->message_box.symbol_pixmap    )
    {   w->message_box.symbol_pixmap = XmUNSPECIFIED_PIXMAP ;
        } 

    /* Create symbol label even if no pixmap specified; allows SetValues
    *   on dialogType to make pixmap appear in what starts as message box.
    */
    ac = 0;
    XtSetArg (al[ac], XmNwidth, 0);                                    ac++;
    XtSetArg (al[ac], XmNheight, 0);                                   ac++;
    XtSetArg (al[ac], XmNlabelType, XmPIXMAP);                         ac++; 
    XtSetArg (al[ac], XmNlabelPixmap, w->message_box.symbol_pixmap);   ac++;
    XtSetArg (al[ac], XmNtraversalOn, False);                          ac++;

    w->message_box.symbol_wid = XmCreateLabelGadget( (Widget) w, "Symbol",
                                                                       al, ac);
    return ;
    }

/****************************************************************
 * Set up the message label (assumes none set yet).
 ****************/
static void 
#ifdef _NO_PROTO
SetUpMessage( w )
        XmMessageBoxWidget w ;
#else
SetUpMessage(
        XmMessageBoxWidget w )
#endif /* _NO_PROTO */
{
    Arg al[ARG_LIST_CNT];
    int ac;
    XmString empty_string = NULL ;
/****************/

    /* set up y value dependent on symbol_wid */

    ac = 0;
    XtSetArg (al[ac], XmNalignment, w->message_box.message_alignment);  ac++;
    XtSetArg (al[ac], XmNborderWidth, 0);                               ac++;
    XtSetArg (al[ac], XmNtraversalOn, False);                           ac++;
    if (w->message_box.message_string) {
       XtSetArg(al[ac], XmNlabelString, w->message_box.message_string); ac++;
       w->message_box.message_string = NULL;
    } else {
	/* we don't want "Message", the name of the label, to become
	   the label string, it would break the AES */
/* BEGIN OSF Fix CR 4847 */
	empty_string = XmStringCreateLocalized(XmS);
/* END OSF Fix CR 4847 */
	XtSetArg(al[ac], XmNlabelString, empty_string); ac++;
    }
    XtSetArg (al[ac], XmNstringDirection, BB_StringDirection( w)) ;
    ac++;

    w->message_box.message_wid = XmCreateLabelGadget( (Widget) w, "Message",
						     al, ac);
/* BEGIN OSF Fix CR 4847 */
    if (empty_string != NULL) XmStringFree(empty_string);
/* END OSF Fix CR 4847 */

    return ;
}

/****************************************************************/
XmGeoMatrix 
#ifdef _NO_PROTO
_XmMessageBoxGeoMatrixCreate( wid, instigator, desired )
        Widget wid ;
        Widget instigator ;
        XtWidgetGeometry *desired ;
#else
_XmMessageBoxGeoMatrixCreate(
        Widget wid,
        Widget instigator,
        XtWidgetGeometry *desired )
#endif /* _NO_PROTO */
{
            XmMessageBoxWidget mb = (XmMessageBoxWidget) wid ;
            XmGeoMatrix     geoSpec ;
    register XmGeoRowLayout  layoutPtr ;
    register XmKidGeometry   boxPtr ;
            XmKidGeometry   firstBoxInRow ;
            Widget menubar = NULL;
            Widget workarea = NULL;
            Boolean has_buttons = False;
            Boolean has_message = False;
            Boolean menubar_adjust = False;
            int nrows = 2;
            int nchildren = mb->composite.num_children;
            int i;

    /*
     * Layout MessageBox XmGeoMatrix.
     * Each row is terminated by leaving an empty XmKidGeometry and
     * moving to the next XmGeoRowLayout.
     */

    /* identify menu bar and work area children. */

    for (i=0; i < nchildren; i++)
    {   register Widget w = mb->composite.children[i];
  
        if( menubar == NULL   
            && XmIsRowColumn(w)
            && ((XmRowColumnWidget)w)->row_column.type == XmMENU_BAR)
        {   menubar = w;
            nrows += 1;
            }
        else
        {    if (IsButton(w))
             {   has_buttons = True;
                }
             else
             {   if (workarea == NULL && !IsAutoChild(mb,w))
                {   workarea = w;
                    nrows += 1;
                    } 
                }
             }
         }
    if (has_buttons)
	nrows += 1;

    geoSpec = _XmGeoMatrixAlloc( nrows, nchildren, 0) ;
    geoSpec->composite = (Widget) mb ;
    geoSpec->instigator = (Widget) instigator ;
    if (desired)
	geoSpec->instig_request = *desired ;
    geoSpec->margin_w = BB_MarginWidth( mb) + mb->manager.shadow_thickness ;
    geoSpec->margin_h = BB_MarginHeight( mb) + mb->manager.shadow_thickness ;
    geoSpec->no_geo_request = _XmMessageBoxNoGeoRequest ;

    layoutPtr = &(geoSpec->layouts->row) ;
    boxPtr = geoSpec->boxes ;

    /* menu bar */
 
    if(    menubar && _XmGeoSetupKid( boxPtr, menubar)    )
    {   layoutPtr->fix_up = _XmMenuBarFix ;
        menubar_adjust = True ;
        boxPtr += 2;
        ++layoutPtr;
        }

    /* symbol pixmap and message string */

    firstBoxInRow = boxPtr ;
    if(    (mb->message_box.symbol_pixmap != XmUNSPECIFIED_PIXMAP)
        && _XmGeoSetupKid( boxPtr, mb->message_box.symbol_wid)    )
    {   ++boxPtr ;
        } 
    if(    _XmGeoSetupKid( boxPtr, mb->message_box.message_wid)    )
    {   has_message = True ;
        ++boxPtr ;
        } 
    if(    boxPtr != firstBoxInRow && (workarea == NULL || has_message)    )
    {   if(    menubar_adjust    )
        {   menubar_adjust = False ;
            } 
        else
        {   layoutPtr->space_above = BB_MarginHeight( mb) ;
            } 
        layoutPtr->space_between = BB_MarginWidth( mb) ;
        ++boxPtr ;
        ++layoutPtr ;
        firstBoxInRow = boxPtr ;
        } 

    /* work area */

    if(    workarea && _XmGeoSetupKid( boxPtr, workarea)    )
    {   ++boxPtr ;
        } 
    if(    boxPtr != firstBoxInRow    )
    {   layoutPtr->fill_mode = XmGEO_EXPAND;
        layoutPtr->fit_mode = XmGEO_PROPORTIONAL;
        if(    menubar_adjust    )
        {   menubar_adjust = False;
            }
        else
        {   layoutPtr->space_above = BB_MarginHeight( mb) ;
            }
        layoutPtr->space_between = BB_MarginWidth(mb);
        layoutPtr->stretch_height = True;
        layoutPtr->even_height = 1;
        boxPtr++;
        ++layoutPtr ;
        }

    /* separator */

    if(    _XmGeoSetupKid( boxPtr, mb->message_box.separator)    )
    {   layoutPtr->fix_up = _XmSeparatorFix ;
        layoutPtr->space_above = BB_MarginHeight( mb) ;
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* buttons */

    if(    _XmGeoSetupKid( boxPtr, mb->message_box.ok_button)    )
    {   ++boxPtr ;
        } 
    for (i = 0; i < nchildren; i++)
    {
        register Widget w = mb->composite.children[i];
 
        if(   !IsAutoChild(mb,w) && IsButton(w) && _XmGeoSetupKid(boxPtr, w)  )
        {   ++boxPtr;
            }
        }
    if(    _XmGeoSetupKid( boxPtr, mb->bulletin_board.cancel_button)    )
    {   ++boxPtr ;
        } 
    if(    _XmGeoSetupKid( boxPtr, mb->message_box.help_button)    )
    {   ++boxPtr ;
        } 

    if(    has_buttons    )
    {   layoutPtr->fill_mode = XmGEO_CENTER ;
        layoutPtr->fit_mode = XmGEO_WRAP ;
        if(    !mb->message_box.minimize_buttons    )
        {   layoutPtr->even_width = 1 ;
            } 
        layoutPtr->even_height = 1 ;
        layoutPtr->space_above = BB_MarginHeight( mb) ;
	++layoutPtr ;
        } 

    /* end */

    layoutPtr->space_above = BB_MarginHeight( mb) ;
    layoutPtr->end = TRUE ;
    return( geoSpec) ;
    }
/****************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmMessageBoxNoGeoRequest( geoSpec )
        XmGeoMatrix geoSpec ;
#else
_XmMessageBoxNoGeoRequest(
        XmGeoMatrix geoSpec )
#endif /* _NO_PROTO */
{
/****************/

    if(    BB_InSetValues( geoSpec->composite)
        && (XtClass( geoSpec->composite) == xmMessageBoxWidgetClass)    )
    {   
        return( TRUE) ;
        } 
    return( FALSE) ;
    }

/****************************************************************/
static Widget 
#ifdef _NO_PROTO
CreateShell( parent, name, args, ac )
        Widget parent ;
        char *name ;
        ArgList args ;
        Cardinal ac ;
#else
CreateShell(
        Widget parent,
        char *name,
        ArgList args,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    ArgList argsNew ;
    Widget retWid;
    char *ds_name;
/****************/

    /* BEGIN fix for PIR 3698 */
    if (!name) name = "";
    /* END fix for PIR 3698 */
    ds_name = XtMalloc((strlen(name)+XmDIALOG_SUFFIX_SIZE+1) * sizeof(char)) ;

    strcpy(ds_name,name);
    strcat(ds_name,XmDIALOG_SUFFIX);

    argsNew = (ArgList) XtMalloc( sizeof( Arg) * (ac + 1)) ;
    memcpy( argsNew, args, (sizeof( Arg) * ac)) ;
    XtSetArg (argsNew[ac], XmNallowShellResize, TRUE); ++ac ;
    retWid = XmCreateDialogShell (parent, ds_name, argsNew, ac);

    XtFree( (char *) argsNew) ;
    XtFree(ds_name);

    return (retWid);
}
/****************************************************************
 * Construct the required captive widgets for the box.  Don't worry about
 *   positioning since a layout will happen later on.
 ****************/
static void 
#ifdef _NO_PROTO
CreateWidgets( w )
        XmMessageBoxWidget w ;
#else
CreateWidgets(
        XmMessageBoxWidget w )
#endif /* _NO_PROTO */
{
    Arg  al[2];

#ifdef I18N_MSG
    XmString default_ok_label_string     = NULL;
    XmString default_help_label_string   = NULL;
    XmString default_cancel_label_string = NULL; 
#endif

/****************/

    /* create the symbol label */
    if (!(w->message_box.dialog_type == XmDIALOG_TEMPLATE &&
	w->message_box.symbol_pixmap == XmUNSPECIFIED_PIXMAP))
	SetUpSymbol (w);

    /* create the message label, after symbol is created */
    if (!(w->message_box.dialog_type == XmDIALOG_TEMPLATE &&
	w->message_box.message_string == NULL))
	SetUpMessage (w);

    /* create the separator */
    XtSetArg (al[0], XmNhighlightThickness, 0);
    w->message_box.separator = XmCreateSeparatorGadget( (Widget) w,
                                                           "Separator", al, 1);

#ifdef I18N_MSG
    /* look up default label strings for pushbuttons if not already set */

    if (w->message_box.ok_label_string == NULL && w->message_box.dialog_type != XmDIALOG_TEMPLATE) {
        default_ok_label_string = XmStringCreateLocalized(
	    catgets(Xm_catd,MS_RESOURCES,MSG_Res_1,"OK"));
	w->message_box.ok_label_string = default_ok_label_string;
    }

    if (w->message_box.cancel_label_string == NULL && w->message_box.dialog_type != XmDIALOG_TEMPLATE) {
        default_cancel_label_string = XmStringCreateLocalized(
	    catgets(Xm_catd,MS_RESOURCES,MSG_Res_2,"Cancel"));
	w->message_box.cancel_label_string = default_cancel_label_string;
    }

    if (w->message_box.help_label_string == NULL && w->message_box.dialog_type != XmDIALOG_TEMPLATE) {
        default_help_label_string = XmStringCreateLocalized(
	    catgets(Xm_catd,MS_RESOURCES,MSG_Res_5,"Help"));
	w->message_box.help_label_string = default_help_label_string;
    }
#endif

    /* create all pushbuttons, user can unmanage if they don't want them */

    /* "Ok" button... if no label, use default "OK" name for label */
    if (!(w->message_box.dialog_type == XmDIALOG_TEMPLATE &&
	w->message_box.ok_label_string == NULL &&
	w->message_box.ok_callback == NULL))
    {
	w->message_box.ok_button = _XmBB_CreateButtonG( (Widget) w,
                                        w->message_box.ok_label_string, "OK") ;
	w->message_box.ok_label_string = NULL;
	XtAddCallback( w->message_box.ok_button, XmNactivateCallback, 
                                              MessageCallback, (XtPointer) w) ;
    }
    /* "Cancel" button... if no label, use default "Cancel" name for label */
    if (!(w->message_box.dialog_type == XmDIALOG_TEMPLATE &&
	w->message_box.cancel_label_string == NULL &&
	w->message_box.cancel_callback == NULL))
    {

	w->bulletin_board.cancel_button = _XmBB_CreateButtonG( (Widget) w,
                                w->message_box.cancel_label_string, "Cancel") ;
	w->message_box.cancel_label_string = NULL;
	XtAddCallback( w->bulletin_board.cancel_button, XmNactivateCallback, 
                                              MessageCallback, (XtPointer) w) ;
    }
    /* "Help" button... if no label, use default "Help" name for label */
    if (!(w->message_box.dialog_type == XmDIALOG_TEMPLATE &&
	w->message_box.help_label_string == NULL &&
	w->manager.help_callback == NULL))
    {
	w->message_box.help_button = _XmBB_CreateButtonG( (Widget) w,
                                    w->message_box.help_label_string, "Help") ;
	w->message_box.help_label_string = NULL;

	/* Remove BulletinBoard Unmanage callback from help button.
	*/
	XtRemoveAllCallbacks( w->message_box.help_button, XmNactivateCallback) ;
	XtAddCallback( w->message_box.help_button, XmNactivateCallback, 
                                              MessageCallback, (XtPointer) w) ;
    }

#ifdef I18N_MSG
    /* free any default label strings we allocated*/
    if (default_ok_label_string)     XmStringFree(default_ok_label_string);
    if (default_help_label_string)   XmStringFree(default_help_label_string);
    if (default_cancel_label_string) XmStringFree(default_cancel_label_string);
#endif

    /* Now manage all my children.
    */
    XtManageChildren (w->composite.children, w->composite.num_children);
    return ;
}

/****************************************************************
 * Common create routine for message dialogs...
 *   it will create the shell and widgets, and set the dialog_type to
 *   whatever has been passed in...
 ****************/
static Widget 
#ifdef _NO_PROTO
_XmCreateDialog( parent, name, al, ac, type )
        Widget parent ;
        char *name ;
        ArgList al ;
        int ac ;
        unsigned char type ;
#else
_XmCreateDialog(
        Widget parent,
        char *name,
        ArgList al,
        int ac,
#if NeedWidePrototypes
        unsigned int type )
#else
        unsigned char type )        /* type of dialog being created */
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Widget shell;
    Widget w;
    ArgList  argsNew;
/****************/

    shell = CreateShell (parent, name, al, ac); 

    /* add dialogType to arglist and force to type passed in... */

    /*  allocate arglist, copy args, add dialog type arg */
    argsNew = (ArgList) XtMalloc (sizeof(Arg) * (ac + 1));
    memcpy( argsNew, al, sizeof(Arg) * ac);
    XtSetArg (argsNew[ac], XmNdialogType, type);  ac++;

    /*  create MessageBox, add shell destroy callback, free argsNew, return */
    w = XtCreateWidget (name, xmMessageBoxWidgetClass, shell, argsNew, ac);
    XtAddCallback (w, XmNdestroyCallback, _XmDestroyParentCallback, NULL);
    XtFree( (char *) argsNew);

    return (w);
}

/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateMessageBox( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateMessageBox(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = XtCreateWidget (name, xmMessageBoxWidgetClass, parent, al, ac);

    return w;
}
/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateMessageDialog( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateMessageDialog(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = _XmCreateDialog (parent, name, al, ac, XmDIALOG_MESSAGE);
    return w;
}
/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateErrorDialog( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateErrorDialog(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = _XmCreateDialog (parent, name, al, ac, XmDIALOG_ERROR);
    return w;
}
/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateInformationDialog( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateInformationDialog(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = _XmCreateDialog (parent, name, al, ac, XmDIALOG_INFORMATION);
    return w;
}
/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateQuestionDialog( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateQuestionDialog(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = _XmCreateDialog (parent, name, al, ac, XmDIALOG_QUESTION);
    return w;
}
/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateWarningDialog( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateWarningDialog(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = _XmCreateDialog (parent, name, al, ac, XmDIALOG_WARNING);
    return w;
}
/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateWorkingDialog( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateWorkingDialog(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = _XmCreateDialog (parent, name, al, ac, XmDIALOG_WORKING);
    return w;
}

/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateTemplateDialog( parent, name, al, ac )
        Widget parent ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateTemplateDialog(
        Widget parent,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    Widget w;
/****************/

    w = _XmCreateDialog (parent, name, al, ac, XmDIALOG_TEMPLATE);
    return w;
}

/****************************************************************/
Widget 
#ifdef _NO_PROTO
XmMessageBoxGetChild( widget, child )
        Widget widget ;
        unsigned char child ;
#else
XmMessageBoxGetChild(
        Widget widget,
#if NeedWidePrototypes
        unsigned int child )
#else
        unsigned char child )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmMessageBoxWidget  w = (XmMessageBoxWidget)widget;
/****************/

    switch (child) {
        case XmDIALOG_DEFAULT_BUTTON: return (w->bulletin_board.default_button);
        case XmDIALOG_SYMBOL_LABEL:   return (w->message_box.symbol_wid);
        case XmDIALOG_MESSAGE_LABEL:  return (w->message_box.message_wid);
        case XmDIALOG_OK_BUTTON:      return (w->message_box.ok_button);
        case XmDIALOG_CANCEL_BUTTON:  return (w->bulletin_board.cancel_button);
        case XmDIALOG_HELP_BUTTON:    return (w->message_box.help_button);
        case XmDIALOG_SEPARATOR:      return (w->message_box.separator);
        default: _XmWarning( (Widget) w, WARNING4); break ;
    }
    return NULL ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
GetMessageString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetMessageString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmMessageBoxWidget w = (XmMessageBoxWidget) wid ;
    XmString  data;
    Arg       al[1];
/****************/

    if (w->message_box.message_wid) {
        XtSetArg (al[0], XmNlabelString, &data);
        XtGetValues (w->message_box.message_wid, al, 1);
        *value = (XtArgVal) data;
    }
    else *value = (XtArgVal) NULL;
    return ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
GetSymbolPixmap( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetSymbolPixmap(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmMessageBoxWidget w = (XmMessageBoxWidget) wid ;
    Pixmap  data;
    Arg     al[1];
/****************/

    if (w->message_box.symbol_wid) {
        XtSetArg (al[0], XmNlabelPixmap, &data);
        XtGetValues (w->message_box.symbol_wid, al, 1);
        *value = (XtArgVal) data;
    }
    else *value = (XtArgVal) NULL;
    return ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
GetOkLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetOkLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmMessageBoxWidget w = (XmMessageBoxWidget) wid ;
    XmString  data;
    Arg       al[1];
/****************/

    if (w->message_box.ok_button) {
        XtSetArg (al[0], XmNlabelString, &data);
        XtGetValues (w->message_box.ok_button, al, 1);
        *value = (XtArgVal) data;
    }
    else *value = (XtArgVal) NULL;
    return ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
GetCancelLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetCancelLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmMessageBoxWidget w = (XmMessageBoxWidget) wid ;
    XmString  data;
    Arg       al[1];
/****************/

    if (w->bulletin_board.cancel_button) {
        XtSetArg (al[0], XmNlabelString, &data);
        XtGetValues (w->bulletin_board.cancel_button, al, 1);
        *value = (XtArgVal) data;
    }
    else *value = (XtArgVal) NULL;
    return ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
GetHelpLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetHelpLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmMessageBoxWidget w = (XmMessageBoxWidget) wid ;
    XmString  data;
    Arg       al[1];
/****************/

    if (w->message_box.help_button) {
        XtSetArg (al[0], XmNlabelString, &data);
        XtGetValues (w->message_box.help_button, al, 1);
        *value = (XtArgVal) data;
    }
    else *value = (XtArgVal) NULL;
    return ;
}
/****************************************************************/
static void 
#ifdef _NO_PROTO
GetMsgBoxPixmap( mBox )
        XmMessageBoxWidget mBox ;
#else
GetMsgBoxPixmap(
        XmMessageBoxWidget mBox )
#endif /* _NO_PROTO */
{
            Pixmap          tmpPix = XmUNSPECIFIED_PIXMAP ;
            char *          fileName ;
            char *          defaultName ;
/****************/

    /* Try to get pixmap from bitmap file or default.
    */
    switch(    mBox->message_box.dialog_type    ) 
    {   
        case XmDIALOG_ERROR: 
        {   fileName = "xm_error" ; 
            defaultName = "default_xm_error" ;
            break ;
            } 
        case XmDIALOG_INFORMATION:
        {   fileName = "xm_information" ;
            defaultName = "default_xm_information" ;
            break ;
            }
        case XmDIALOG_QUESTION:
        {   fileName = "xm_question" ;
            defaultName = "default_xm_question" ;
            break ;
            }
        case XmDIALOG_WARNING:
        {   fileName = "xm_warning" ;
            defaultName = "default_xm_warning" ;
            break ;
            }
        case XmDIALOG_WORKING:
        {   fileName = "xm_working" ;
            defaultName = "default_xm_working" ;
            break ;
            }
        default: 
        {   fileName = NULL ;
            defaultName = NULL ;
            break ;
            }
        }
    if(    fileName    )
    {
	tmpPix = XmGetPixmapByDepth( mBox->core.screen, fileName, 
				    mBox->manager.foreground, 
				    mBox->core.background_pixel,
				    mBox->core.depth) ;
 	if(    tmpPix == XmUNSPECIFIED_PIXMAP    )
         {   tmpPix = XmGetPixmapByDepth( mBox->core.screen, defaultName,
					 mBox->manager.foreground, 
					 mBox->core.background_pixel,
					 mBox->core.depth) ;
            } 
    }
    mBox->message_box.symbol_pixmap = tmpPix ;
    mBox->message_box.internal_pixmap = TRUE ;

    return ;
    }
/****************************************************************/
