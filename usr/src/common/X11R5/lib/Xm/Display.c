#pragma ident	"@(#)m1.2libs:Xm/Display.c	1.8"
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
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <stdio.h>
#include <Xm/DisplayP.h>
#include <Xm/ScreenP.h>
#include <Xm/WorldP.h>
#include <Xm/DropTransP.h>
#include "DisplayI.h"
#include "DragCI.h"
#include "DragICCI.h"
#include <Xm/AtomMgr.h>
#include <X11/Xatom.h>
#include "MessagesI.h"
#ifdef USE_COLOR_OBJECT
#include <Xm/ColorObjP.h>
#endif /* USE_COLOR_OBJECT */
#ifdef USE_FONT_OBJECT
#include <Xm/FontObjP.h>
#endif /* USE_FONT_OBJECT */

#define MESSAGE1 _XmMsgDisplay_0001
#define MESSAGE2 _XmMsgDisplay_0002
#define MESSAGE3 _XmMsgDisplay_0003

#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define TheDisplay(dd) (XtDisplayOfObject((Widget)dd))
#define TheScreen(dd) (XtScreen((Widget)dd))

#define Offset(x) (XtOffsetOf( struct _XmDisplayRec, x))

#define CHECK_TIME(dc, time) \
  ((dc->drag.dragStartTime <= time) && \
   ((dc->drag.dragFinishTime == 0) || (time <= dc->drag.dragFinishTime)))

#ifdef CDE_RESOURCES
enum {XmEXTERNAL_HIGHLIGHT, XmINTERNAL_HIGHLIGHT};
static char *DefaultButtonEmphasisNames[] =
{   "external_highlight", "internal_highlight"
    } ;
static unsigned char EnableBtn1TransferValues[] = 
{ False, True, 2};
static char *EnableBtn1TransferNames[] =
{   "false", "true", "button2_transfer"
    } ;
#define NUM_NAMES( list )        (sizeof( list) / sizeof( char *))

typedef struct _CDEDisplay_InstanceExtRec
    {
        XtEnum          enable_btn1_transfer ;
        Boolean         enable_button_tab ;
        Boolean         enable_multi_key_bindings ;
	Boolean		enable_etched_in_menu;
	Boolean		default_button_emphasis;
	Boolean		enable_toggle_color;
	Boolean		enable_toggle_visual;
	Boolean		enable_drag_icon;
	Boolean		enable_unselectable_drag;
#ifdef USE_FONT_OBJECT
	Boolean		use_font_object;
#endif /* USE_FONT_OBJECT */
    } CDEDisplay_InstanceExtRec, *CDEDisplay_InstanceExt ;
#endif /* CDE_RESOURCES */

#if defined(CDE_BIND) || defined(CDE_NO_DRAG_FROM_LABELS)
static _XmConst char _CDEBaseTranslationsOverride[] = "\
*XmArrowButton.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	PrimitiveParentCancel()\\n\
	<Key>Escape:	PrimitiveParentCancel()\n\
*XmBulletinBoard.baseTranslations:\
    #override\
	<Key>F1:	ManagerGadgetHelp()\\n\
	c<Key>s:	ManagerParentCancel()\\n\
	<Key>Escape:	ManagerParentCancel()\n\
*XmCascadeButton.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	CleanupMenuBar()\\n\
	<Key>Escape:	CleanupMenuBar()\n\
*XmDragContext.baseTranslations:\
    #override\
	<Key>F1:	HelpDrag()\\n\
	c<Key>s:	CancelDrag()\\n\
	<Key>Escape:	CancelDrag()\n\
*XmDrawingArea.baseTranslations:\
    #override\
	 <Key>F1:	DrawingAreaInput() ManagerGadgetHelp()\\n\
	c<Key>s:	DrawingAreaInput() ManagerParentCancel()\\n\
	<Key>Escape:	DrawingAreaInput() ManagerParentCancel()\n\
*XmDrawnButton.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	PrimitiveParentCancel()\\n\
	<Key>Escape:	PrimitiveParentCancel()\n\
*XmFrame.baseTranslations:\
    #override\
	c<Key>s:	ManagerParentCancel()\\n\
	<Key>Escape:	ManagerParentCancel()\n\
*XmLabel.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	PrimitiveParentCancel()\\n\
	<Key>Escape:	PrimitiveParentCancel()\\n\
	<Btn2Down>:	\n\
*XmList.baseTranslations:\
    #override\
	<Key>F1:	PrimitiveHelp()\\n\
	c<Key>s:	ListKbdCancel()\\n\
	<Key>Escape:	ListKbdCancel()\n\
*XmManager.baseTranslations:\
    #override\
	<Key>F1:	ManagerGadgetHelp()\\n\
	c<Key>s:	ManagerParentCancel()\\n\
	<Key>Escape:	ManagerParentCancel()\n\
*XmPrimitive.baseTranslations:\
    #override\
	<Key>F1:	PrimitiveHelp()\\n\
	c<Key>s:	PrimitiveParentCancel()\\n\
	<Key>Escape:	PrimitiveParentCancel()\n\
*XmPushButton.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	PrimitiveParentCancel()\\n\
	<Key>Escape:	PrimitiveParentCancel()\\n\
	<Btn2Down>:	\n\
*XmRowColumn.baseTranslations:\
    #override\
	<Key>F1:	MenuHelp()\\n\
	c<Key>s:	ManagerParentCancel()\\n\
	<Key>Escape:	ManagerParentCancel()\n\
*XmSash.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	PrimitiveParentCancel()\\n\
	<Key>Escape:	PrimitiveParentCancel()\n\
*XmScrollBar.baseTranslations:\
    #override\
	<Key>F1:	PrimitiveHelp()\\n\
	c<Key>s:	CancelDrag()\\n\
	<Key>Escape:	CancelDrag()\n\
*XmScrolledWindow.baseTranslations:\
    #override\
	<Key>F1:	ManagerGadgetHelp()\\n\
	c<Key>s:	ManagerParentCancel()\\n\
	<Key>Escape:	ManagerParentCancel()\n\
*XmTextField.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	process-cancel()\\n\
	<Key>Escape:	process-cancel()\\n\
	c<Key>x:	cut-clipboard()\\n\
	c<Key>c:	copy-clipboard()\\n\
	c<Key>v:	paste-clipboard()\\n\
	s<Key>Delete:	cut-clipboard()\\n\
	c<Key>Insert:	copy-clipboard()\\n\
	s<Key>Insert:	paste-clipboard()\n\
*XmText.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	process-cancel()\\n\
	<Key>Escape:	process-cancel()\\n\
	c<Key>x:	cut-clipboard()\\n\
	c<Key>c:	copy-clipboard()\\n\
	c<Key>v:	paste-clipboard()\\n\
	s<Key>Delete:	cut-clipboard()\\n\
	c<Key>Insert:	copy-clipboard()\\n\
	s<Key>Insert:	paste-clipboard()\n\
*XmToggleButton.baseTranslations:\
    #override\
	<Key>F1:	Help()\\n\
	c<Key>s:	PrimitiveParentCancel()\\n\
	<Key>Escape:	PrimitiveParentCancel()\\n\
	<Btn2Down>:	\n\
";
#endif /* defined(CDE_BIND) || defined(CDE_NO_DRAG_FROM_LABELS) */

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void DisplayClassPartInitialize() ;
static void DisplayClassInitialize() ;
static void DisplayClassInitializeHook() ;
static void SetDragReceiverInfo() ;
static void TreeUpdateHandler() ;
static void DisplayInitialize() ;
static void DisplayInsertChild() ;
static void DisplayDeleteChild() ;
static void DisplayDestroy() ;
static XmDragContext FindDC() ;
static int isMine() ;
static void ReceiverShellExternalSourceHandler() ;
static Widget GetDisplay() ;

#ifdef CDE_RESOURCES
static void NewDisplayInstanceExt() ;
static void FreeDisplayInstanceExt() ;
static CDEDisplay_InstanceExt CDEGetDisplayExtRecord() ;
static void DisplayGetValuesHook();
static Boolean _CDEGetEnableMultiKeyBindings() ;
#endif /* CDE_RESOURCES */
#else

static void DisplayClassPartInitialize( 
                        WidgetClass wc) ;
static void DisplayClassInitialize( void ) ;
static void DisplayClassInitializeHook( Widget, ArgList, Cardinal * ) ;
static void SetDragReceiverInfo( 
                        Widget w,
                        XtPointer client_data,
                        XEvent *event,
                        Boolean *dontSwallow) ;
static void TreeUpdateHandler( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void DisplayInitialize( 
                        Widget requested_widget,
                        Widget new_widget,
                        ArgList args,
                        Cardinal *num_args) ;
static void DisplayInsertChild( 
                        Widget w) ;
static void DisplayDeleteChild( 
                        Widget w) ;
static void DisplayDestroy( 
                        Widget w) ;
static XmDragContext FindDC( 
                        XmDisplay xmDisplay,
                        Time time,
#if NeedWidePrototypes
                        int sourceIsExternal) ;
#else
                        Boolean sourceIsExternal) ;
#endif /* NeedWidePrototypes */
static int isMine( 
                        Display *dpy,
                        register XEvent *event,
                        char *arg) ;
static void ReceiverShellExternalSourceHandler( 
                        Widget w,
                        XtPointer client_data,
                        XEvent *event,
                        Boolean *dontSwallow) ;
static Widget GetDisplay( 
                        Display *display) ;

#ifdef CDE_RESOURCES
static void NewDisplayInstanceExt(
        Widget display_object,
        ArgList args,
        Cardinal nargs);
static void FreeDisplayInstanceExt(
        Widget display_object);
static CDEDisplay_InstanceExt CDEGetDisplayExtRecord(
        Widget w);
static void DisplayGetValuesHook(
	Widget w,
	ArgList args,
	Cardinal *num_args);
static Boolean _CDEGetEnableMultiKeyBindings(
        Widget w);
#endif /* CDE_RESOURCES */
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

externaldef(displaymsg) String	_Xm_MOTIF_DRAG_AND_DROP_MESSAGE;

static XContext	displayContext = 0;
static WidgetClass curDisplayClass = NULL;

static XtResource resources[] = {
    {
	XmNdropSiteManagerClass, XmCDropSiteManagerClass, XmRWidgetClass,
	sizeof(WidgetClass), Offset(display.dropSiteManagerClass), 
	XmRImmediate, (XtPointer)&xmDropSiteManagerClassRec,
    },
    {
	XmNdropTransferClass, XmCDropTransferClass, XmRWidgetClass,
	sizeof(WidgetClass), Offset(display.dropTransferClass), 
	XmRImmediate, (XtPointer)&xmDropTransferClassRec,
    },
    {
	XmNdragContextClass, XmCDragContextClass, XmRWidgetClass,	
	sizeof(WidgetClass), Offset(display.dragContextClass), 
	XmRImmediate, (XtPointer)&xmDragContextClassRec,
    },
    {
	XmNdragInitiatorProtocolStyle, XmCDragInitiatorProtocolStyle,
	XmRDragInitiatorProtocolStyle, sizeof(unsigned char), 
	Offset(display.dragInitiatorProtocolStyle), 
	XmRImmediate, (XtPointer)XmDRAG_PREFER_RECEIVER,
    },
    {
	XmNdragReceiverProtocolStyle, XmCDragReceiverProtocolStyle,
	XmRDragReceiverProtocolStyle, sizeof(unsigned char), 
	Offset(display.dragReceiverProtocolStyle), 
	XmRImmediate, (XtPointer)XmDRAG_PREFER_PREREGISTER,
    },
    {
	"defaultVirtualBindings", "DefaultVirtualBindings",
	XmRString, sizeof(String),
	Offset(display.bindingsString),
	XmRImmediate, (XtPointer)NULL,
    },
};

#undef Offset

#ifdef CDE_RESOURCES
static XtResource cde_resources[] = {
  { "enableBtn1Transfer", "EnableBtn1Transfer", "EnableBtn1Transfer", sizeof(XtEnum),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_btn1_transfer),
    XmRImmediate, (XtPointer) False
  },
  { "enableButtonTab", "EnableButtonTab", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_button_tab),
    XmRImmediate, (XtPointer) False
  },
  { "enableMultiKeyBindings", "EnableMultiKeyBindings", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_multi_key_bindings),
    XmRImmediate, (XtPointer) False
  },
  { "enableEtchedInMenu", "EnableEtchedInMenu", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_etched_in_menu),
    XmRImmediate, (XtPointer) False
  },
  { "defaultButtonEmphasis", "DefaultButtonEmphasis", "DefaultButtonEmphasis",
    sizeof(XtEnum),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, default_button_emphasis),
    XmRImmediate, (XtPointer) XmEXTERNAL_HIGHLIGHT
  },
  { "enableToggleColor", "EnableToggleColor", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_toggle_color),
    XmRImmediate, (XtPointer) False
  },
  { "enableToggleVisual", "EnableToggleVisual", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_toggle_visual),
    XmRImmediate, (XtPointer) False
  },
  { "enableDragIcon", "EnableDragIcon", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_drag_icon),
    XmRImmediate, (XtPointer) False
  },
  { "enableUnselectableDrag", "EnableUnselectableDrag", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, enable_unselectable_drag),
    XmRImmediate, (XtPointer) True
  },
#ifdef USE_FONT_OBJECT
  { "useFontObject", "UseFontObject", XmRBoolean, sizeof(Boolean),
    XtOffsetOf(struct _CDEDisplay_InstanceExtRec, use_font_object),
    XmRImmediate, (XtPointer) True
  },
#endif /* USE_FONT_OBJECT */
};

static XContext cde_display_rec_context;

#endif /* CDE_RESOURCES */

static XmBaseClassExtRec baseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    (XtInitProc)NULL,			/* InitializePrehook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPrehook	*/
    (XtInitProc)NULL,			/* InitializePosthook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    (XtInitProc)NULL,			/* secondaryCreate	*/
    (XmGetSecResDataFunc)NULL,        	/* getSecRes data	*/
    { 0 },     				/* fastSubclass flags	*/
    (XtArgsProc)NULL,			/* getValuesPrehook	*/
    (XtArgsProc)NULL,			/* getValuesPosthook	*/
    (XtWidgetClassProc)NULL,               /* classPartInitPrehook */
    (XtWidgetClassProc)NULL,               /* classPartInitPosthook*/
    NULL,               /* ext_resources        */
    NULL,               /* compiled_ext_resources*/
    0,                  /* num_ext_resources    */
    FALSE,              /* use_sub_resources    */
    (XmWidgetNavigableProc)NULL,               /* widgetNavigable      */
    (XmFocusChangeProc)NULL,               /* focusChange          */
    (XmWrapperData)NULL	/* wrapperData		*/
};


externaldef(xmdisplayclassrec)
XmDisplayClassRec xmDisplayClassRec = {
    {	
	(WidgetClass) &applicationShellClassRec,	/* superclass		*/   
	"XmDisplay",			/* class_name 		*/   
	sizeof(XmDisplayRec),	 	/* size 		*/   
	DisplayClassInitialize,		/* Class Initializer 	*/   
	DisplayClassPartInitialize,	/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	DisplayInitialize,		/* initialize         	*/   
	DisplayClassInitializeHook,	/* initialize_notify    */ 
	XtInheritRealize,		/* realize            	*/   
	NULL,	 			/* actions            	*/   
	0,				/* num_actions        	*/   
	resources,			/* resources          	*/   
	XtNumber(resources),		/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	DisplayDestroy,			/* destroy            	*/   
	(XtWidgetProc)NULL, 		/* resize             	*/   
	(XtExposeProc)NULL, 		/* expose             	*/   
	(XtSetValuesFunc)NULL, 		/* set_values         	*/   
	(XtArgsFunc)NULL, 		/* set_values_hook      */ 
	(XtAlmostProc)NULL,	 	/* set_values_almost    */ 
#ifdef CDE_RESOURCES
	(XtArgsProc)DisplayGetValuesHook,/* get_values_hook     */ 
#else
	(XtArgsProc)NULL,		/* get_values_hook      */ 
#endif /* CDE_RESOURCES */
	(XtAcceptFocusProc)NULL, 	/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	(XtGeometryHandler)NULL, 	/* query_geometry       */ 
	(XtStringProc)NULL, 		/* display_accelerator  */ 
	(XtPointer)&baseClassExtRec, 	/* extension            */ 
    },	
    { 					/* composite class record */
	(XtGeometryHandler)NULL,	/* geometry_manager 	*/
	(XtWidgetProc)NULL,		/* change_managed	*/
	DisplayInsertChild,		/* insert_child		*/
	DisplayDeleteChild, 		/* from the shell 	*/
	NULL, 				/* extension record     */
    },
    { 					/* shell class record 	*/
	NULL, 				/* extension record     */
    },
    { 					/* wm shell class record */
	NULL, 				/* extension record     */
    },
    { 					/* vendor shell class record */
	NULL,				/* extension record     */
    },
    { 					/* toplevelclass record */
	NULL, 				/* extension record     */
    },
    { 					/* appShell record 	*/
	NULL, 				/* extension record     */
    },
    {					/* Display class	*/
	GetDisplay,			/* GetDisplay		*/
	NULL,				/* extension		*/
    },
};

externaldef(xmdisplayclass) WidgetClass 
      xmDisplayClass = (WidgetClass) (&xmDisplayClassRec);



static void 
#ifdef _NO_PROTO
DisplayClassPartInitialize(wc)
	WidgetClass wc;
#else
DisplayClassPartInitialize(
	WidgetClass wc )
#endif /* _NO_PROTO */
{
	_XmFastSubclassInit(wc, XmDISPLAY_BIT);
}

static void 
#ifdef _NO_PROTO
DisplayClassInitialize()
#else
DisplayClassInitialize( void )
#endif /* _NO_PROTO */
{
	baseClassExtRec.record_type = XmQmotif;
    _Xm_MOTIF_DRAG_AND_DROP_MESSAGE =
		XmMakeCanonicalString("_MOTIF_DRAG_AND_DROP_MESSAGE");
#ifdef CDE_RESOURCES
    XmRepTypeRegister( "DefaultButtonEmphasis", DefaultButtonEmphasisNames,
			NULL, NUM_NAMES(DefaultButtonEmphasisNames));
    XmRepTypeRegister( "EnableBtn1Transfer", EnableBtn1TransferNames,
			EnableBtn1TransferValues,
			NUM_NAMES(EnableBtn1TransferNames));
#endif /* CDE_RESOURCES */
}    

static void
#ifdef _NO_PROTO
DisplayClassInitializeHook(w, args, num_args)
	Widget w;
	ArgList args;
	Cardinal * num_args;
#else
DisplayClassInitializeHook(
	Widget w,
	ArgList args,
	Cardinal * num_args)
#endif /* _NO_PROTO */
{
#ifdef USE_COLOR_OBJECT
	_XmColorObjCreate(w, args, num_args);
#endif /* USE_COLOR_OBJECT */
#ifdef USE_FONT_OBJECT
	{
	CDEDisplay_InstanceExt rec_cache = CDEGetDisplayExtRecord(w);

	if (rec_cache && rec_cache->use_font_object)
		_XmFontObjectCreate(w, args, num_args);
	}
#endif /* USE_FONT_OBJECT */
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
SetDragReceiverInfo( w, client_data, event, dontSwallow )
        Widget w ;
        XtPointer client_data ;
        XEvent *event ;
        Boolean *dontSwallow ;
#else
SetDragReceiverInfo(
        Widget w,
        XtPointer client_data,
        XEvent *event,
        Boolean *dontSwallow )
#endif /* _NO_PROTO */
{
    XmDisplay	dd = (XmDisplay) XmGetXmDisplay(XtDisplay(w));

    if (XtIsRealized(w)) {
	_XmSetDragReceiverInfo(dd, (Widget)client_data);
	XtRemoveEventHandler(w, StructureNotifyMask, False,
			     SetDragReceiverInfo,
			     client_data);
    }
}

/*
 * this routine is registered on the XmNtreeUpdateProc resource of the
 * dropSiteManager.  It is called whenever the tree is changed.
 */
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
TreeUpdateHandler( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
TreeUpdateHandler(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmAnyCallbackStruct	    	*anyCB = (XmAnyCallbackStruct *)call;
    XmDisplay	  	dd = (XmDisplay) XmGetXmDisplay(XtDisplay(w));

    if (dd->display.dragReceiverProtocolStyle == XmDRAG_NONE)
		return;

    switch(anyCB->reason) {
      case XmCR_DROP_SITE_TREE_ADD:
	{
	    XmDropSiteTreeAddCallback cb =
	      (XmDropSiteTreeAddCallback)anyCB;

	    if (XtIsRealized(cb->rootShell)) {
		_XmSetDragReceiverInfo(dd, cb->rootShell);
	    }
	    else {
		XtAddEventHandler(cb->rootShell, 
				  StructureNotifyMask, False,
				  SetDragReceiverInfo,
				  (XtPointer)cb->rootShell);
	    }
	    /*
	     * ClientMessages are not maskable so all we have to
	     * do is indicate interest in non-maskable events.
	     */
	    XtAddEventHandler(cb->rootShell, NoEventMask, True,
			      ReceiverShellExternalSourceHandler,
			      (XtPointer)dd);
	}
	break;
      case XmCR_DROP_SITE_TREE_REMOVE:
	{
	    XmDropSiteTreeRemoveCallback cb =
	      (XmDropSiteTreeRemoveCallback)anyCB;
	    XtRemoveEventHandler(cb->rootShell, NoEventMask, True,
				 ReceiverShellExternalSourceHandler,
				 (XtPointer)dd);
	    if (XtIsRealized(cb->rootShell))
	      _XmClearDragReceiverInfo(cb->rootShell);
	}
	break;
      default:
	break;
    }
}

/************************************************************************
 *
 *  DisplayInitialize
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DisplayInitialize( requested_widget, new_widget, args, num_args )
        Widget requested_widget ;
        Widget new_widget ;
        ArgList args ;
        Cardinal *num_args ;
#else
DisplayInitialize(
        Widget requested_widget,
        Widget new_widget,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmDisplay	xmDisplay = (XmDisplay)new_widget;

    xmDisplay->display.shellCount = 0;

    xmDisplay->display.numModals = 0;
    xmDisplay->display.modals = NULL;
    xmDisplay->display.maxModals = 0;
    xmDisplay->display.userGrabbed = False;
    xmDisplay->display.activeDC = NULL;
    xmDisplay->display.dsm = (XmDropSiteManagerObject) NULL ;

	xmDisplay->display.proxyWindow =
		_XmGetDragProxyWindow(XtDisplay(xmDisplay));

    _XmInitByteOrderChar();
    xmDisplay->display.xmim_info = NULL;

    xmDisplay->display.displayInfo = (XtPointer) XtNew(XmDisplayInfo);
    ((XmDisplayInfo *)(xmDisplay->display.displayInfo))->SashCursor = 0L;
    ((XmDisplayInfo *)(xmDisplay->display.displayInfo))->TearOffCursor = 0L;
    ((XmDisplayInfo *)(xmDisplay->display.displayInfo))->UniqueStamp = 0L;
    ((XmDisplayInfo *)(xmDisplay->display.displayInfo))->destinationWidget= 
	(Widget)NULL;

    _XmVirtKeysInitialize (new_widget);

    if (displayContext == 0)
      displayContext = XUniqueContext();
	
	if (! XFindContext(XtDisplay(xmDisplay), None, displayContext,
		(char **) &xmDisplay))
	{
		/*
		 * There's one already created for this display.
		 * What should we do?  If we destroy the previous one, we may
		 * wreak havoc with shell modality and screen objects.  BUT,
		 * Xt doesn't really give us a way to abort a create.  We'll
		 * just let the new one dangle.
		 */


#ifdef I18N_MSG
		_XmWarning((Widget) xmDisplay, catgets(Xm_catd,MS_Display,
MSG_DSP_1, MESSAGE1)); 
#else
		_XmWarning((Widget) xmDisplay, MESSAGE1);
#endif

	}
	else
	{
		XSaveContext(XtDisplayOfObject((Widget)xmDisplay),
			 None,
			 displayContext,
			 (char *)xmDisplay);
	}
#ifdef CDE_RESOURCES
	NewDisplayInstanceExt(new_widget, args, *num_args);
#endif /* CDE_RESOURCES */

#if defined(CDE_BIND) || defined(CDE_NO_DRAG_FROM_LABELS)
    if (_CDEGetEnableMultiKeyBindings(new_widget)) {
	Display * display = XtDisplay(new_widget);
	int i, num_screens = ScreenCount(display);
	XrmDatabase new_db = XrmGetStringDatabase(_CDEBaseTranslationsOverride);

	for (i = 0; i < num_screens; i++)  {
	    Screen * screen = ScreenOfDisplay(display, i);
	    XrmDatabase db = XtScreenDatabase(screen);
	    XrmCombineDatabase(new_db, &db, False);
	}
    }
#endif /* defined(CDE_BIND) || defined(CDE_NO_DRAG_FROM_LABELS) */
}


/************************************************************************
 *
 *  DisplayInsertChild
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DisplayInsertChild( w )
        Widget w ;
#else
DisplayInsertChild(
        Widget w )
#endif /* _NO_PROTO */
{
	if (XtIsRectObj(w))
		(* ((CompositeWidgetClass)compositeWidgetClass)
			->composite_class.insert_child) (w);
}


/************************************************************************
 *
 *  DisplayDeleteChild
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DisplayDeleteChild( w )
        Widget w ;
#else
DisplayDeleteChild(
        Widget w )
#endif /* _NO_PROTO */
{
	if (XtIsRectObj(w))
		(* ((CompositeWidgetClass)compositeWidgetClass)
			->composite_class.delete_child) (w);
}

/************************************************************************
 *
 *  DisplayDestroy
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DisplayDestroy( w )
        Widget w ;
#else
DisplayDestroy(
        Widget w )
#endif /* _NO_PROTO */
{
    XmDisplay dd = (XmDisplay) w ;

    XtFree((char *) dd->display.modals);
    XtFree((char *) dd->display.displayInfo);

    _XmVirtKeysDestroy (w);

    XDeleteContext( XtDisplay( w), None, displayContext) ;
#ifdef CDE_RESOURCES
    FreeDisplayInstanceExt(w);
#endif /* CDE_RESOURCES */
}

/*ARGSUSED*/
XmDropSiteManagerObject 
#ifdef _NO_PROTO
_XmGetDropSiteManagerObject( xmDisplay )
        XmDisplay xmDisplay ;
#else
_XmGetDropSiteManagerObject(
        XmDisplay xmDisplay )
#endif /* _NO_PROTO */
{
  /* Defer the creation of the XmDisplayObject's DropSiteManager until
   *   it is referenced, since the converters of some DSM resources
   *   (animationPixmap, for instance) depend on the presence of the
   *   Display/Screen objects, and a circular recursive creation loop
   *   results if the DSM is created during DisplayInitialize.
   */
  if(    xmDisplay->display.dsm == NULL    )
    {   
      Arg lclArgs[1] ;

      XtSetArg( lclArgs[0], XmNtreeUpdateProc, TreeUpdateHandler) ;
      xmDisplay->display.dsm = (XmDropSiteManagerObject) XtCreateWidget( "dsm",
                                       xmDisplay->display.dropSiteManagerClass,
                                              (Widget) xmDisplay, lclArgs, 1) ;
    } 
  return( xmDisplay->display.dsm);
}


unsigned char 
#ifdef _NO_PROTO
_XmGetDragProtocolStyle( w )
        Widget w ;
#else
_XmGetDragProtocolStyle(
        Widget w )
#endif /* _NO_PROTO */
{
    XmDisplay		xmDisplay;
    unsigned char	style;

    xmDisplay = (XmDisplay) XmGetXmDisplay(XtDisplay(w));

    switch(xmDisplay->display.dragReceiverProtocolStyle) {
	  case XmDRAG_NONE:
	  case XmDRAG_DROP_ONLY:
	    style = XmDRAG_NONE;
	    break;
	  case XmDRAG_DYNAMIC:
	    style = XmDRAG_DYNAMIC;
	    break;
	  case XmDRAG_PREFER_DYNAMIC:
	  case XmDRAG_PREFER_PREREGISTER:
	  case XmDRAG_PREREGISTER:
	    style = XmDRAG_PREREGISTER;
	    break;
	  default:
	    style = XmDRAG_NONE;
	    break;
	}
    return style;
}

Widget 
#ifdef _NO_PROTO
XmGetDragContext( w, time )
        Widget w ;
        Time time ;
#else
XmGetDragContext(
        Widget w,
        Time time )
#endif /* _NO_PROTO */
{
	XmDisplay		xmDisplay;
	XmDragContext	matchedDC = NULL, dc = NULL;
	Cardinal		i;

	xmDisplay = (XmDisplay)XmGetXmDisplay(XtDisplay(w));
	for(i = 0; i < xmDisplay->composite.num_children; i++)
	{
		dc = (XmDragContext)(xmDisplay->composite.children[i]);
		if ((XmIsDragContext((Widget) dc)) && (CHECK_TIME(dc, time)) &&
			((!matchedDC) ||
				(matchedDC->drag.dragStartTime
					< dc->drag.dragStartTime)) &&
			!dc->core.being_destroyed)
			matchedDC = dc;
	}
	return((Widget)matchedDC);
}

Widget 
#ifdef _NO_PROTO
_XmGetDragContextFromHandle( w, iccHandle )
        Widget w ;
        Atom iccHandle ;
#else
_XmGetDragContextFromHandle(
        Widget w,
        Atom iccHandle )
#endif /* _NO_PROTO */
{
	XmDisplay		xmDisplay;
	XmDragContext	dc;
	Cardinal		i;

	xmDisplay = (XmDisplay)XmGetXmDisplay(XtDisplay(w));

	for(i = 0; i < xmDisplay->composite.num_children; i++)
	{
		dc = (XmDragContext)(xmDisplay->composite.children[i]);
		if ((XmIsDragContext((Widget) dc)) && 
			(dc->drag.iccHandle == iccHandle) &&
			!dc->core.being_destroyed)
			return((Widget)dc);
	}
	return(NULL);
}




static XmDragContext 
#ifdef _NO_PROTO
FindDC( xmDisplay, time, sourceIsExternal )
        XmDisplay xmDisplay ;
        Time time ;
        Boolean sourceIsExternal ;
#else
FindDC(
        XmDisplay xmDisplay,
        Time time,
#if NeedWidePrototypes
        int sourceIsExternal )
#else
        Boolean sourceIsExternal )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	XmDragContext	dc = NULL;
	Cardinal			i;

	for(i = 0; i < xmDisplay->composite.num_children; i++)
	{
		dc = (XmDragContext)(xmDisplay->composite.children[i]);
		if ((XmIsDragContext((Widget) dc)) &&
			(CHECK_TIME(dc, time)) &&
			(dc->drag.sourceIsExternal == sourceIsExternal) &&
			!dc->core.being_destroyed)
			return(dc);
	}
	return(NULL);
}

/*ARGSUSED*/
static int 
#ifdef _NO_PROTO
isMine( dpy, event, arg )
        Display *dpy ;
        register XEvent *event ;
        char *arg ;
#else
isMine(
        Display *dpy,
        register XEvent *event,
        char *arg )
#endif /* _NO_PROTO */
{
	XmDisplayEventQueryStruct 	*q = (XmDisplayEventQueryStruct *) arg;
	XmICCCallbackStruct		callback, *cb = &callback;

	/* Once we have a drop start we must stop looking at the queue */
	if (q->hasDropStart)
		return(False);

	if (!_XmICCEventToICCCallback((XClientMessageEvent *)event, 
			cb, XmICC_INITIATOR_EVENT))
		return(False);

	if ((cb->any.reason == XmCR_DROP_SITE_ENTER) ||
		(cb->any.reason == XmCR_DROP_SITE_LEAVE))
	{
		/*
		 * We must assume this to be a dangling set of messages, so
		 * we will quietly consume it in the interest of hygene.
		 */
		return(True);
	}


	if (!q->dc)
		q->dc = FindDC(q->dd, cb->any.timeStamp, True);
	/*
	 * if we can find a dc then we have already processed
	 * an enter for this shell.
	 */

	switch(cb->any.reason)
	{
		case XmCR_TOP_LEVEL_ENTER:
			q->hasLeave = False;
			if (q->dc == NULL)
			{
				*(q->enterCB) = *(XmTopLevelEnterCallbackStruct *)cb;
				q->hasEnter = True;
			}
		break;
		case XmCR_TOP_LEVEL_LEAVE:
			q->hasEnter = False;
			if (q->dc != NULL)
			{
				*(q->leaveCB) = *(XmTopLevelLeaveCallbackStruct *)cb;
				q->hasLeave = True;
				q->hasMotion = False;
			}
			else
			{

#ifdef I18N_MSG
			_XmWarning((Widget) q->dd,
				catgets(Xm_catd,MS_Display,MSG_DSP_2, 
				MESSAGE2));
#else
			_XmWarning((Widget) q->dd, MESSAGE2);
#endif

			}
		break;
		case XmCR_DRAG_MOTION:
			*(q->motionCB) = *(XmDragMotionCallbackStruct *)cb;
			q->hasMotion = True;
		break;
		case XmCR_DROP_START:
			*(q->dropStartCB) = *(XmDropStartCallbackStruct *)cb;
			q->hasDropStart = True;
		break;
		default:
		break;
	}
	return True;
}

/*
 * this handler is used to catch messages from external drag
 * contexts that want to map motion or drop events
 */
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ReceiverShellExternalSourceHandler( w, client_data, event, dontSwallow )
        Widget w ;
        XtPointer client_data ;
        XEvent *event ;
        Boolean *dontSwallow ;
#else
ReceiverShellExternalSourceHandler(
        Widget w,
        XtPointer client_data,
        XEvent *event,
        Boolean *dontSwallow )
#endif /* _NO_PROTO */
{
    XmDragTopLevelClientDataStruct 	topClientData;
    XmTopLevelEnterCallbackStruct	enterCB;
    XmTopLevelLeaveCallbackStruct	leaveCB;
    XmDropStartCallbackStruct		dropStartCB;
    XmDragMotionCallbackStruct		motionCB;
    XmDisplayEventQueryStruct		 		q;	
    Widget	  			shell = w;
    XmDisplay			dd = (XmDisplay) XmGetXmDisplay(XtDisplay(w));
    XmDragContext			dc;
    XmDropSiteManagerObject		dsm = _XmGetDropSiteManagerObject( dd);

    /*
     * If dd has an active dc then we are the initiator.  We shouldn't
     * do receiver processing as the initiator, so bail out.
     */
    if (dd->display.activeDC != NULL)
	return;

    q.dd = dd;
    q.dc = (XmDragContext)NULL;
    q.enterCB = &enterCB;
    q.leaveCB = &leaveCB;
    q.motionCB = &motionCB;
    q.dropStartCB = &dropStartCB;
    q.hasEnter =
      q.hasDropStart = 
	q.hasLeave = 
	  q.hasMotion = False;
    /*
     * Since this event handler gets called for all non-maskable events,
     * we want to bail out now with don't swallow if we don't want this
     * event.  Otherwise we'll swallow it.
     */
    
     /*
	  * process the event that fired this event handler.
	  * If it's not a Receiver DND event, bail out.
	  */
    if (!isMine(XtDisplayOfObject(w), event, (char*)&q))
		return;

    /*
     * swallow all the pending messages inline except the last motion
     */
    while (XCheckIfEvent( XtDisplayOfObject(w), event, isMine, (char*)&q)) {
    }

    dc = q.dc;

    if (q.hasEnter || q.hasMotion || q.hasDropStart || q.hasLeave) {
	/*
	 * handle a dangling leave first
	 */
	if (q.hasLeave) 
	  {
	      XmTopLevelLeaveCallback	callback = &leaveCB;
	      
	      topClientData.destShell = shell;
	      topClientData.xOrigin = shell->core.x;
	      topClientData.yOrigin = shell->core.y;
	      topClientData.sourceIsExternal = True;
	      topClientData.iccInfo = NULL;
	      topClientData.window = XtWindow(w);
	      topClientData.dragOver = NULL;
	      
	      _XmDSMUpdate(dsm, 
			   (XtPointer)&topClientData, 
			   (XtPointer)callback);
	      /* destroy it if no drop. otherwise done in dropTransfer */
	      if (!q.hasDropStart)
	      {
		XtDestroyWidget((Widget)dc);
		q.dc = dc = NULL;
	      }
	  }
	/*
	 * check for a dropStart from a preregister client or an enter
	 * either of which require a dc to be alloced
	 */
	if (q.hasEnter || q.hasDropStart) {
	    if (!q.dc) {
		Arg		args[4];
		Cardinal	i = 0;
		Time		timeStamp;
		Window		window;
		Atom		iccHandle;

		if (q.hasDropStart) {
		    XmDropStartCallback	dsCallback = &dropStartCB;
		    
		    timeStamp = dsCallback->timeStamp;
		    window = dsCallback->window;
		    iccHandle = dsCallback->iccHandle;
		}
		else {
		    XmTopLevelEnterCallback teCallback = &enterCB;	    
		    
		    timeStamp = teCallback->timeStamp;
		    window = teCallback->window;
		    iccHandle = teCallback->iccHandle;
		}
		XtSetArg(args[i], XmNsourceWindow, window);i++;
		XtSetArg(args[i], XmNsourceIsExternal,True);i++;
		XtSetArg(args[i], XmNstartTime, timeStamp);i++;
		XtSetArg(args[i], XmNiccHandle, iccHandle);i++;
		dc = (XmDragContext) XtCreateWidget("dragContext", 
			dd->display.dragContextClass, (Widget)dd, args, i);
		_XmReadInitiatorInfo((Widget)dc);
		/*
		 * force in value for dropTransfer to use in selection
		 * calls.
		 */
		dc->drag.currReceiverInfo =
		  _XmAllocReceiverInfo(dc);
		dc->drag.currReceiverInfo->shell = shell;
                dc->drag.currReceiverInfo->dragProtocolStyle = 
			                  dd->display.dragReceiverProtocolStyle;
	    }
	    topClientData.destShell = shell;
	    topClientData.xOrigin = XtX(shell);
	    topClientData.yOrigin = XtY(shell);
	    topClientData.width = XtWidth(shell);
	    topClientData.height = XtHeight(shell);
	    topClientData.sourceIsExternal = True;
	    topClientData.iccInfo = NULL;
	}

	if (!dc) return;

	if (q.hasDropStart) {
	    dc->drag.dragFinishTime = dropStartCB.timeStamp;
	    _XmDSMUpdate(dsm,
			 (XtPointer)&topClientData, 
			 (XtPointer)&dropStartCB);
	}
	/* 
	 * we only see enters if they're not matched with a leave
	 */
	if (q.hasEnter) {
	    _XmDSMUpdate(dsm,
			 (XtPointer)&topClientData, 
			 (XtPointer)&enterCB);
	}
	if (q.hasMotion) {
	    XmDragMotionCallback	callback = &motionCB;
	    XmDragMotionClientDataStruct	motionData;
	    
	    motionData.window = XtWindow(w);
	    motionData.dragOver = NULL;
	    _XmDSMUpdate(dsm, (XtPointer)&motionData, (XtPointer)callback);
	}
    }
}

static Widget 
#ifdef _NO_PROTO
GetDisplay( display )
        Display *display ;
#else
GetDisplay(
        Display *display )
#endif /* _NO_PROTO */
{
	XmDisplay	xmDisplay = NULL;
	Arg args[3];
	int n;

	if ((displayContext == 0) ||
		(XFindContext(display, None, displayContext,
			(char **) &xmDisplay)))
	{
		String	name, w_class;

		XtGetApplicationNameAndClass(display, &name, &w_class);

		n = 0;
		XtSetArg(args[n], XmNmappedWhenManaged, False); n++;
		XtSetArg(args[n], XmNwidth, 1); n++;
		XtSetArg(args[n], XmNheight, 1); n++;
		xmDisplay = (XmDisplay) XtAppCreateShell(name, w_class,
			xmDisplayClass, display, args, n);
	}

	/* We need a window to be useful */
	if (!XtIsRealized((Widget)xmDisplay))
		XtRealizeWidget((Widget)xmDisplay);

	return ((Widget)xmDisplay);
}

Widget
#ifdef _NO_PROTO
XmGetXmDisplay( display )
        Display *display ;
#else
XmGetXmDisplay(
        Display *display )
#endif /* _NO_PROTO */
{
	XmDisplayClass dC;

	/*
	 * We have a chicken and egg problem here; we'd like to get
	 * the display via a class function, but we don't know which
	 * class to use.  Hence the magic functions _XmGetXmDisplayClass
	 * and _XmSetXmDisplayClass.
	 */
	
	dC = (XmDisplayClass) _XmGetXmDisplayClass();

	return((*(dC->display_class.GetDisplay))(display));
}

/*
 * It would be nice if the next two functions were methods, but
 * for obvious reasons they're not.
 */

WidgetClass
#ifdef _NO_PROTO
_XmGetXmDisplayClass()
#else
_XmGetXmDisplayClass( void )
#endif /* _NO_PROTO */
{
	if (curDisplayClass == NULL)
		curDisplayClass = xmDisplayClass;
	return(curDisplayClass);
}

WidgetClass
#ifdef _NO_PROTO
_XmSetXmDisplayClass(wc)
	WidgetClass wc;
#else
_XmSetXmDisplayClass(
	WidgetClass wc )
#endif /* _NO_PROTO */
{
	WidgetClass oldDisplayClass = curDisplayClass;
	WidgetClass sc = wc;

	/*
	 * We aren't going to let folks just set any old class in as the
	 * display class.  They will have to use subclasses of xmDisplay.
	 */
	while ((sc != NULL) && (sc != xmDisplayClass))
		sc = sc->core_class.superclass;

	if (sc != NULL)
		curDisplayClass = wc;
	else

#ifdef I18N_MSG
		_XmWarning(NULL, catgets(Xm_catd,MS_Display,MSG_DSP_3,
                           MESSAGE3));
#else
		_XmWarning(NULL, MESSAGE3);
#endif

	return(oldDisplayClass);
}

#ifdef CDE_RESOURCES
static void
#ifdef _NO_PROTO
NewDisplayInstanceExt(display_object, args, nargs)
        Widget display_object;
        ArgList args;
        Cardinal nargs;
#else
NewDisplayInstanceExt(
        Widget display_object,
        ArgList args,
        Cardinal nargs)
#endif /* _NO_PROTO */
{
  CDEDisplay_InstanceExt rec_cache;

  /* The following is a static global */
  if (!cde_display_rec_context)
      cde_display_rec_context = XUniqueContext();

  rec_cache = (CDEDisplay_InstanceExt)
              XtCalloc(1, sizeof(CDEDisplay_InstanceExtRec));
  XtGetSubresources( display_object, (XtPointer) rec_cache, NULL, NULL,
                     cde_resources, XtNumber(cde_resources), args, nargs) ;
  XSaveContext( XtDisplay( display_object), None, cde_display_rec_context,
                (XPointer) rec_cache) ;
}

static void
#ifdef _NO_PROTO
FreeDisplayInstanceExt(display_object)
        Widget display_object;
#else
FreeDisplayInstanceExt(
        Widget display_object)
#endif /* _NO_PROTO */
{
  CDEDisplay_InstanceExt rec_cache;

  rec_cache = CDEGetDisplayExtRecord(display_object);
  if (rec_cache)
      XtFree((char *)rec_cache);
  XDeleteContext( XtDisplay(display_object), None, cde_display_rec_context) ;
}

static CDEDisplay_InstanceExt
#ifdef _NO_PROTO
CDEGetDisplayExtRecord(w)
        Widget w;
#else
CDEGetDisplayExtRecord(
        Widget w)
#endif /* _NO_PROTO */
{
  CDEDisplay_InstanceExt rec_cache;

  if (XFindContext(XtDisplay(w), None, cde_display_rec_context,
      (char **) &rec_cache))
      /*  The context was not found */
      return(NULL);
  return(rec_cache);
}

static Boolean
#ifdef _NO_PROTO
_CDEGetEnableMultiKeyBindings(w)
        Widget w;
#else
_CDEGetEnableMultiKeyBindings(
        Widget w)
#endif /* _NO_PROTO */
{
  CDEDisplay_InstanceExt rec_cache = CDEGetDisplayExtRecord(w);

  if (rec_cache)
      return(rec_cache->enable_multi_key_bindings);
}

static void
#ifdef _NO_PROTO
DisplayGetValuesHook(w, args, num_args)
	Widget w;
	ArgList args;
	Cardinal *num_args;
#else
DisplayGetValuesHook(
	Widget w,
	ArgList args,
	Cardinal *num_args)
#endif
{
	Cardinal i;
	CDEDisplay_InstanceExt rec_cache = CDEGetDisplayExtRecord(w);
	
	if (!rec_cache)
		return;

	for (i = 0; i < *num_args; i++)  {
		if (strcmp(args[i].name, "enableDragIcon") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_drag_icon;
		}
		if (strcmp(args[i].name, "enableToggleVisual") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_toggle_visual;
		}
		if (strcmp(args[i].name, "enableToggleColor") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_toggle_color;
		}
		if (strcmp(args[i].name, "enableBtn1Transfer") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_btn1_transfer;
		}
		if (strcmp(args[i].name, "enableButtonTab") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_button_tab;
		}
		if (strcmp(args[i].name, "enableMultiKeyBindings") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_multi_key_bindings;
		}
		if (strcmp(args[i].name, "enableEtchedInMenu") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_etched_in_menu;
		}
		if (strcmp(args[i].name, "defaultButtonEmphasis") == 0) {
			*(Boolean *)args[i].value = rec_cache->default_button_emphasis;
		}
		if (strcmp(args[i].name, "enableUnselectableDrag") == 0) {
			*(Boolean *)args[i].value = rec_cache->enable_unselectable_drag;
		}
	}
}  /* end of DisplayGetValuesHook() */
#endif /* CDE_RESOURCES */
