#pragma ident	"@(#)m1.2libs:Xm/DragC.c	1.5.1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
*/ 
/*
 * Motif Release 1.2.2
*/
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1991, 1992 HEWLETT-PACKARD COMPANY */

#define TESTING True

#ifndef DEFAULT_WM_TIMEOUT
#define DEFAULT_WM_TIMEOUT 5000
#endif

#include <Xm/DragCP.h>
#include <Xm/DragOverSP.h>
#include <Xm/ManagerP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/GadgetP.h>
#include "DragCI.h"
#include "DragICCI.h"
#include "DragBSI.h"
#include "TraversalI.h"
#include <Xm/DisplayP.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <Xm/AtomMgr.h>
#include <Xm/VendorSP.h>
#include "MessagesI.h"

#include <stdio.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif 


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


/* force the multi screen support on */
#define MULTI_SCREEN_DONE

#ifdef DEBUG
#define Warning(str) _XmWarning(NULL, str)
#else
#define Warning(str) ;
#endif

#define BIGSIZE ((Dimension)32767)

#define MESSAGE1 _XmMsgDragC_0001
#define MESSAGE2 _XmMsgDragC_0002
#define MESSAGE3 _XmMsgDragC_0003
#define MESSAGE4 _XmMsgDragC_0004
#define MESSAGE5 _XmMsgDragC_0005
#define MESSAGE6 _XmMsgDragC_0006

typedef struct _MotionEntryRec{
    int		type;
    Time	time;
    Window	window;
    Window	subwindow;
    Position	x, y;
    unsigned int state;
}MotionEntryRec, *MotionEntry;

#define STACKMOTIONBUFFERSIZE 	120

typedef struct _MotionBufferRec{
    XmDragReceiverInfo currReceiverInfo;
    Cardinal		count;
    MotionEntryRec      entries[STACKMOTIONBUFFERSIZE];
}MotionBufferRec, *MotionBuffer;

#define MOTIONFILTER		16

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void GetRefForeground() ;
static void CopyRefForeground() ;
static void GetRefBackground() ;
static void DragContextInitialize() ;
static Boolean DragContextSetValues() ;
static void DragContextDestroy() ;
static void DragContextClassInitialize() ;
static void DragContextClassPartInitialize() ;
static Window GetClientWindow() ;
static void ValidateDragOver() ;
static XmDragReceiverInfo FindReceiverInfo() ;
static void GetDestinationInfo() ;
static void GetScreenInfo() ;
static void SendDragMessage() ;
static void GenerateClientCallback() ;
static void DropLoseIncrSelection() ;
static void DropLoseSelection() ;
static void DragDropFinish() ;
static Boolean DropConvertIncrCallback() ;
static Boolean DropConvertCallback() ;
static void DragStartProto() ;
static void NewScreen() ;
static void LocalNotifyHandler() ;
static void ExternalNotifyHandler() ;
static void InitiatorMsgHandler() ;
static void SiteEnteredWithLocalSource() ;
static void SiteLeftWithLocalSource() ;
static void OperationChanged() ;
static void SiteMotionWithLocalSource() ;
static void DropStartConfirmed() ;
static Widget GetShell() ;
static void InitDropSiteManager() ;
static void TopWindowsReceived() ;
static void DragStart() ;
static void DragStartWithTracking() ;
static void UpdateMotionBuffer() ;
static void DragMotionProto() ;
static void ProcessMotionBuffer() ;
static void DragMotion() ;
static void DropStartTimeout() ;
static void FinishAction() ;
static void CheckModifiers() ;
static void IgnoreButtons() ;
static void CancelDrag() ;
static void HelpDrag() ;
static void FinishDrag() ;
static void InitiatorMainLoop() ;
static void DragCancel() ;

#else

static void GetRefForeground( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
static void CopyRefForeground( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
static void GetRefBackground( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
static void DragContextInitialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *numArgs) ;
static Boolean DragContextSetValues( 
                        Widget old,
                        Widget ref,
                        Widget new_w,
                        Arg *args,
                        Cardinal *numArgs) ;
static void DragContextDestroy( 
                        Widget w) ;
static void DragContextClassInitialize( void ) ;
static void DragContextClassPartInitialize( WidgetClass ) ;
static Window GetClientWindow( 
                        Display *dpy,
                        Window win,
                        Atom atom) ;
static void ValidateDragOver( 
                        XmDragContext dc,
#if NeedWidePrototypes
                        unsigned int oldStyle,
                        unsigned int newStyle) ;
#else
                        unsigned char oldStyle,
                        unsigned char newStyle) ;
#endif /* NeedWidePrototypes */
static XmDragReceiverInfo FindReceiverInfo( 
                        XmDragContext dc,
                        Window win) ;
static void GetDestinationInfo( 
                        XmDragContext dc,
                        Window root,
                        Window win) ;
static void GetScreenInfo( 
                        XmDragContext dc) ;
static void SendDragMessage( 
                        XmDragContext dc,
                        Window destination,
#if NeedWidePrototypes
                        unsigned int messageType) ;
#else
                        unsigned char messageType) ;
#endif /* NeedWidePrototypes */
static void GenerateClientCallback( 
                        XmDragContext dc,
#if NeedWidePrototypes
                        unsigned int reason) ;
#else
                        unsigned char reason) ;
#endif /* NeedWidePrototypes */
static void DropLoseIncrSelection( 
                        Widget w,
                        Atom *selection,
                        XtPointer clientData) ;
static void DropLoseSelection( 
                        Widget w,
                        Atom *selection) ;
static void DragDropFinish( 
                        XmDragContext dc) ;
static Boolean DropConvertIncrCallback( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *typeRtn,
                        XtPointer *valueRtn,
                        unsigned long *lengthRtn,
                        int *formatRtn,
                        unsigned long *maxLengthRtn,
                        XtPointer clientData,
                        XtRequestId *requestID) ;
static Boolean DropConvertCallback( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *typeRtn,
                        XtPointer *valueRtn,
                        unsigned long *lengthRtn,
                        int *formatRtn) ;
static void DragStartProto( 
                        XmDragContext dc) ;
static void NewScreen( 
                        XmDragContext dc,
                        Window newRoot) ;
static void LocalNotifyHandler( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void ExternalNotifyHandler( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void InitiatorMsgHandler( 
                        Widget w,
                        XtPointer clientData,
                        XEvent *event,
                        Boolean *dontSwallow) ;
static void SiteEnteredWithLocalSource( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void SiteLeftWithLocalSource( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void OperationChanged( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void SiteMotionWithLocalSource( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void DropStartConfirmed( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static Widget GetShell( 
                        Widget w) ;
static void InitDropSiteManager( 
                        XmDragContext dc) ;
static void TopWindowsReceived( 
                        Widget w,
                        XtPointer client_data,
                        Atom *selection,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static void DragStart( 
                        XmDragContext dc,
                        Widget src,
                        XEvent *event) ;
static void DragStartWithTracking( 
                        XmDragContext dc) ;
static void UpdateMotionBuffer( 
			XmDragContext dc,
                        MotionBuffer mb,
                        XEvent *event) ;
static void DragMotionProto( 
                        XmDragContext dc,
                        Window root,
                        Window subWindow) ;
static void ProcessMotionBuffer( 
                        XmDragContext dc,
                        MotionBuffer mb) ;
static void DragMotion( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
static void DropStartTimeout( 
                        XtPointer clientData,
                        XtIntervalId *id) ;
static void FinishAction( 
                        XmDragContext dc,
                        XEvent *ev) ;
static void CheckModifiers( 
                        XmDragContext dc,
                        unsigned int state) ;
static void IgnoreButtons( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
static void CancelDrag( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
static void HelpDrag( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
static void FinishDrag( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
static void InitiatorMainLoop( 
                        XtPointer clientData,
                        XtIntervalId *id) ;
static void DragCancel( 
                        XmDragContext dc) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static XtActionsRec	dragContextActions[] = 
{
    {"FinishDrag"	, FinishDrag	},
    {"CancelDrag"	, CancelDrag	},
    {"HelpDrag"		, HelpDrag	},
    {"DragMotion"	, DragMotion	},
    {"IgnoreButtons"	, IgnoreButtons	},
};

static char dragContextTranslations[] =         "\
<Btn2Up>		:FinishDrag()		\n\
<Btn1Up>		:FinishDrag()		\n\
<BtnUp>			:IgnoreButtons()	\n\
<BtnDown>		:IgnoreButtons()	\n\
<Key>osfCancel		:CancelDrag()		\n\
<Key>osfHelp		:HelpDrag()		\n\
Button2<Enter>		:DragMotion()		\n\
Button2<Leave>		:DragMotion()		\n\
Button2<Motion>		:DragMotion()		\n\
Button1<Enter>		:DragMotion()		\n\
Button1<Leave>		:DragMotion()		\n\
Button1<Motion>		:DragMotion()		";

static unsigned char	protocolMatrix[7][6] = {

    /*
     *
     * Rows are initiator styles, Columns are receiver styles.
     *
     * Receiver     NO   DO   PP   P    PD   D
     * Initiator  -------------------------------
     *      NO    | NO | NO | NO | NO | NO | NO |
     *      DO    | NO | DO | DO | DO | DO | DO |
     *      PP    | NO | DO | P  | P  | P  | D  |
     *      P     | NO | DO | P  | P  | P  | DO |
     *      PD    | NO | DO | D  | P  | D  | D  |
     *      D     | NO | DO | D  | DO | D  | D  |
     *      PR    | NO | DO | P  | P  | D  | D  |
     */


    { /* Initiator == XmDRAG_NONE ==   0 */
	XmDRAG_NONE,        XmDRAG_NONE,        XmDRAG_NONE,
	XmDRAG_NONE,        XmDRAG_NONE,        XmDRAG_NONE,
    },
    { /* Initiator == DROP_ONLY ==      1 */
	XmDRAG_NONE,        XmDRAG_DROP_ONLY,   XmDRAG_DROP_ONLY,
	XmDRAG_DROP_ONLY,   XmDRAG_DROP_ONLY,   XmDRAG_DROP_ONLY,
    },
    { /* Initiator == PREFER_PREREG ==  2 */
	XmDRAG_NONE,        XmDRAG_DROP_ONLY,   XmDRAG_PREREGISTER,
	XmDRAG_PREREGISTER, XmDRAG_PREREGISTER, XmDRAG_DYNAMIC,
    },
    { /* Initiator == PREREG ==         3 */
	 XmDRAG_NONE,        XmDRAG_DROP_ONLY,   XmDRAG_PREREGISTER,
	 XmDRAG_PREREGISTER, XmDRAG_PREREGISTER, XmDRAG_DROP_ONLY,
    },
    { /* Initiator == PREFER_DYNAMIC == 4 */
	 XmDRAG_NONE,        XmDRAG_DROP_ONLY,   XmDRAG_DYNAMIC,
	 XmDRAG_PREREGISTER, XmDRAG_DYNAMIC,     XmDRAG_DYNAMIC,
    },
    { /* Initiator == XmDRAG_DYNAMIC == 5 */
	 XmDRAG_NONE,        XmDRAG_DROP_ONLY,   XmDRAG_DYNAMIC,
	 XmDRAG_DROP_ONLY,   XmDRAG_DYNAMIC,     XmDRAG_DYNAMIC,
    },
    { /* Initiator == DRAG_RECEIVER ==  6 */
	 XmDRAG_NONE,        XmDRAG_DROP_ONLY,   XmDRAG_PREREGISTER,
	 XmDRAG_PREREGISTER, XmDRAG_DYNAMIC,     XmDRAG_DYNAMIC,
    },
};

/***************************************************************************
 *
 * Default values for resource lists
 *
 ***************************************************************************/

#define Offset(x)	(XtOffsetOf(XmDragContextRec, x))

static XtResource dragContextResources[] = 
{
    {
	XmNsourceWidget, XmCSourceWidget, XmRWidget,
	sizeof(Widget), Offset(drag.sourceWidget),
	XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNexportTargets, XmCExportTargets, XmRAtomList,
	sizeof(Atom *), Offset(drag.exportTargets),
	XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNnumExportTargets, XmCNumExportTargets, XmRInt,
	sizeof(Cardinal), Offset(drag.numExportTargets),
	XmRImmediate, (XtPointer)0,
    },
    {
	XmNconvertProc, XmCConvertProc, XmRFunction,
	sizeof(XmConvertSelectionRec), Offset(drag.convertProc),
	XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNclientData, XmCClientData, XmRPointer,
	sizeof(XtPointer), Offset(drag.clientData),
	XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNincremental, XmCIncremental, XmRBoolean,
	sizeof(Boolean), Offset(drag.incremental),
	XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNdragOperations, XmCDragOperations, XmRUnsignedChar,
	sizeof(unsigned char), Offset(drag.dragOperations),
	XmRImmediate, (XtPointer)(XmDROP_COPY | XmDROP_MOVE),
    },
    {
	XmNsourceCursorIcon, XmCSourceCursorIcon,
	XmRWidget, sizeof(Widget),
	Offset(drag.sourceCursorIcon), XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNsourcePixmapIcon, XmCSourcePixmapIcon,
	XmRWidget, sizeof(Widget),
	Offset(drag.sourcePixmapIcon), XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNstateCursorIcon, XmCStateCursorIcon,
	XmRWidget, sizeof(Widget),
	Offset(drag.stateCursorIcon), XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNoperationCursorIcon, XmCOperationCursorIcon,
	XmRWidget, sizeof(Widget),
	Offset(drag.operationCursorIcon), XmRImmediate, (XtPointer)NULL,
    },
    {	
	XmNcursorBackground, XmCCursorBackground, XmRPixel, 
	sizeof(Pixel), Offset(drag.cursorBackground), XmRCallProc,
	(XtPointer)GetRefBackground,
    },
    {	
	XmNcursorForeground, XmCCursorForeground, XmRPixel,
	sizeof(Pixel), Offset(drag.cursorForeground), XmRCallProc, (XtPointer)GetRefForeground,	
    },
    {
	XmNvalidCursorForeground, XmCValidCursorForeground, XmRPixel,
	sizeof(Pixel), Offset(drag.validCursorForeground), XmRCallProc, (XtPointer)CopyRefForeground,	
    },
    {	
	XmNinvalidCursorForeground, XmCInvalidCursorForeground,
	XmRPixel, sizeof(Pixel), Offset(drag.invalidCursorForeground),
	XmRCallProc, (XtPointer)CopyRefForeground,	
    },
    {	
	XmNnoneCursorForeground, XmCNoneCursorForeground, XmRPixel,
	sizeof(Pixel), Offset(drag.noneCursorForeground), 
	XmRCallProc, (XtPointer)CopyRefForeground,	
    },
    {
	XmNdropSiteEnterCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.siteEnterCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNdropSiteLeaveCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.siteLeaveCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNtopLevelEnterCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.topLevelEnterCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNdragMotionCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.dragMotionCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNtopLevelLeaveCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.topLevelLeaveCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNdropStartCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.dropStartCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNdragDropFinishCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.dragDropFinishCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNdropFinishCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.dropFinishCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNoperationChangedCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList), Offset(drag.operationChangedCallback),
        XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNblendModel, XmCBlendModel,
	XmRBlendModel,
        sizeof(unsigned char), Offset(drag.blendModel),
        XmRImmediate, (XtPointer)XmBLEND_ALL,
    },
    {
	XmNsourceIsExternal, XmCSourceIsExternal, XmRBoolean,
        sizeof(Boolean), Offset(drag.sourceIsExternal),
        XmRImmediate, (XtPointer)False,
    },
    {
	XmNsourceWindow, XmCSourceWindow, XmRWindow,
        sizeof(Window), Offset(drag.srcWindow),
        XmRImmediate, (XtPointer)None,
    },
    {
	XmNstartTime, XmCStartTime, XmRInt,
        sizeof(Time), Offset(drag.dragStartTime),
        XmRImmediate, (XtPointer)0,
    },
    {
	XmNiccHandle, XmCICCHandle, XmRAtom,
        sizeof(Atom), Offset(drag.iccHandle),
        XmRImmediate, (XtPointer)None,
    },
};


externaldef(xmdragcontextclassrec)
XmDragContextClassRec xmDragContextClassRec = {
    {	
	(WidgetClass) &coreClassRec,	/* superclass	*/   
	"XmDragContext",		/* class_name 		*/   
	sizeof(XmDragContextRec),	/* size 		*/   
	DragContextClassInitialize,	/* Class Initializer 	*/   
	DragContextClassPartInitialize,	/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	DragContextInitialize,		/* initialize         	*/   
	NULL, 				/* initialize_notify    */ 
	XtInheritRealize,		/* realize            	*/   
	dragContextActions,		/* actions		*/
	XtNumber(dragContextActions),	/* num_actions        	*/   
	dragContextResources,		/* resources          	*/   
	XtNumber(dragContextResources),/* resource_count    	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	DragContextDestroy,		/* destroy            	*/   
	NULL,		 		/* resize             	*/   
	NULL, 				/* expose             	*/   
	DragContextSetValues,		/* set_values         	*/   
	NULL, 				/* set_values_hook      */ 
	NULL,			 	/* set_values_almost    */ 
	NULL,				/* get_values_hook      */ 
	NULL, 				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	dragContextTranslations,	/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL, 				/* display_accelerator  */ 
	NULL, 				/* extension            */ 
    },	
    {					/* dragContext	*/
	DragStart,			/* start		*/
	DragCancel,			/* cancel		*/
	NULL,				/* extension record	*/
    },
};

externaldef(dragContextclass) WidgetClass 
      xmDragContextClass = (WidgetClass) &xmDragContextClassRec;

static XrmQuark		xrmFailure = NULLQUARK;
static XrmQuark		xrmSuccess = NULLQUARK;

/* Save the current root eventMask so that D&D works for MWM */
static long SaveEventMask = 0;

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
GetRefForeground( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
GetRefForeground(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)widget;
    Widget		sw = dc->drag.sourceWidget;
    static Pixel	pixel;

    pixel = BlackPixelOfScreen(XtScreen(widget));

    value->addr = (XPointer)(&pixel);
    value->size = sizeof(Pixel);

    if (sw) {
	if (XmIsGadget(sw))
	  pixel = ((XmManagerWidget)(XtParent(sw)))->manager.foreground;
	else if (XmIsPrimitive(sw))
	  pixel = ((XmPrimitiveWidget)sw)->primitive.foreground;
	else if (XmIsManager(sw))
	  pixel = ((XmManagerWidget)sw)->manager.foreground;
    }
    return;
} /* GetRefForeground */


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
CopyRefForeground( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
CopyRefForeground(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)widget;

    value->addr = (XPointer)(&dc->drag.cursorForeground);
    value->size = sizeof(Pixel);
    return;
} /* GetRefForeground */


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
GetRefBackground( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
GetRefBackground(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)widget;
    Widget		sw = dc->drag.sourceWidget;
    static Pixel 	pixel;

    pixel = WhitePixelOfScreen(XtScreen(dc));

    value->addr = (XPointer)(&pixel);
    value->size = sizeof(Pixel);

    if (sw) {
	if (XmIsGadget(sw))
	  pixel = ((XmManagerWidget)(XtParent(sw)))->core.background_pixel;
	else 
	  pixel = sw->core.background_pixel;
    }
    return;
} /* GetRefBackground */


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DragContextInitialize( req, new_w, args, numArgs )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *numArgs ;
#else
DragContextInitialize(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *numArgs )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)new_w;

    dc->drag.currWmRoot = 0;
    dc->drag.roundOffTime = 50;

    dc->drag.dragFinishTime =
      dc->drag.dropFinishTime = 0;

    dc->drag.inDropSite = False;
    dc->drag.dragTimerId = (XtIntervalId) NULL;
    dc->drag.activeBlendModel = dc->drag.blendModel;
    dc->drag.trackingMode = XmIsMotifWMRunning( XtParent( new_w)) ?
                                  XmDRAG_TRACK_WM_QUERY : XmDRAG_TRACK_MOTION ;
	dc->drag.curDragOver = dc->drag.origDragOver = NULL;

	dc->drag.startX = dc->drag.startY = 0;

    InitDropSiteManager(dc);

    if (dc->drag.exportTargets) {
	unsigned int 	size;
	size = sizeof(Atom) * dc->drag.numExportTargets;
	dc->drag.exportTargets = (Atom *)
	  _XmAllocAndCopy(dc->drag.exportTargets, size);
    }
    dc->core.x =
      dc->core.y = 0;
    dc->core.width =
      dc->core.height = 16;

    XtRealizeWidget((Widget)dc);

    dc->drag.currReceiverInfo = 
      dc->drag.receiverInfos = NULL;
    dc->drag.numReceiverInfos =
      dc->drag.maxReceiverInfos = 0;

}

/*ARGSUSED*/
static Boolean 
#ifdef _NO_PROTO
DragContextSetValues( old, ref, new_w, args, numArgs )
        Widget old ;
        Widget ref ;
        Widget new_w ;
        Arg *args ;
        Cardinal *numArgs ;
#else
DragContextSetValues(
        Widget old,
        Widget ref,
        Widget new_w,
        Arg *args,
        Cardinal *numArgs )
#endif /* _NO_PROTO */
{
    XmDragContext	oldDC = (XmDragContext)old;
    XmDragContext	newDC = (XmDragContext)new_w;
    XmDragOverShellWidget dos = newDC->drag.curDragOver;

    if (oldDC->drag.exportTargets != newDC->drag.exportTargets) {
	if (oldDC->drag.exportTargets) /* should have been freed */
	  XtFree( (char *)oldDC->drag.exportTargets);
	if (newDC->drag.exportTargets) {
	    unsigned int 	size;
	    size = sizeof(Atom) * newDC->drag.numExportTargets;
	    newDC->drag.exportTargets = (Atom *)
	      _XmAllocAndCopy(newDC->drag.exportTargets, size);
	}
    }
    if ( oldDC->drag.operationCursorIcon != newDC->drag.operationCursorIcon ||
      oldDC->drag.sourceCursorIcon != newDC->drag.sourceCursorIcon ||
      oldDC->drag.sourcePixmapIcon != newDC->drag.sourcePixmapIcon ||
      oldDC->drag.stateCursorIcon != newDC->drag.stateCursorIcon ||
      oldDC->drag.cursorBackground != newDC->drag.cursorBackground ||
      oldDC->drag.cursorForeground != newDC->drag.cursorForeground ||
      oldDC->drag.noneCursorForeground != newDC->drag.noneCursorForeground ||
      oldDC->drag.invalidCursorForeground !=
                               newDC->drag.invalidCursorForeground ||
      oldDC->drag.validCursorForeground !=
                               newDC->drag.validCursorForeground) {
       _XmDragOverChange((Widget)dos, dos->drag.cursorState);
    }
    return False;
}

static void 
#ifdef _NO_PROTO
DragContextDestroy( w )
        Widget w ;
#else
DragContextDestroy(
        Widget w )
#endif /* _NO_PROTO */
{
	XmDragContext	dc = (XmDragContext)w;
	Cardinal		i;

	/* Fix CR 5556:  Restore root event mask saved at DragStart time */
		/* We have to check `currWmRoot' because it's possible that
		 * a new drag is started when the last drop transfer (note
		 * that a drag_context object will be created if the
		 * drop is from external) has not been completed yet
		 * (either because waiting for selection-time-out
		 * (due to odd siutation) or because transfering large
		 * amount of data... */
	if (SaveEventMask != 0 && dc->drag.currWmRoot != 0) {
	  XSelectInput(XtDisplay(dc), dc->drag.currWmRoot, SaveEventMask);
          SaveEventMask = 0;
	}


	if (dc->drag.exportTargets)
		XtFree((char *)dc->drag.exportTargets);

	dc->drag.exportTargets = NULL;

	if (dc->drag.dragTimerId)
	{
		XtRemoveTimeOut(dc->drag.dragTimerId);
		dc->drag.dragTimerId = (XtIntervalId) NULL;
	}

	if (dc->drag.receiverInfos)
	{
#ifdef MULTI_SCREEN_DONE
		if (dc->drag.trackingMode != XmDRAG_TRACK_MOTION)
		{
			EventMask mask;
			XmDragReceiverInfo info;

			for (i = 1; i < dc->drag.numReceiverInfos; i++)
			{
				info = &(dc->drag.receiverInfos[i]);

				if (info->shell)
					mask = XtBuildEventMask(info->shell);
				else
					mask = 0;

				XSelectInput(XtDisplay(w), info->window, mask);
			}
		}
#endif /* MULTI_SCREEN_DONE */
		XtFree((char *)dc->drag.receiverInfos);
	}
}

static void 
#ifdef _NO_PROTO
DragContextClassInitialize()
#else
DragContextClassInitialize( void )
#endif /* _NO_PROTO */
{
/* This should be done by the applications */
#ifdef notdef
    XtRegisterGrabAction(StartDrag, 
			 True,
			 ButtonPressMask,
			 GrabModeSync,
			 GrabModeSync);
#endif /* notdef */
}   

static void
#ifdef _NO_PROTO
DragContextClassPartInitialize( wc )
     WidgetClass wc;
#else
DragContextClassPartInitialize( WidgetClass wc )
#endif /* _NO_PROTO */
{
  _XmFastSubclassInit (wc, XmDRAG_CONTEXT_BIT);
}

static Window 
#ifdef _NO_PROTO
GetClientWindow( dpy, win, atom )
        Display *dpy ;
        Window win ;
        Atom atom ;
#else
GetClientWindow(
        Display *dpy,
        Window win,
        Atom atom )
#endif /* _NO_PROTO */
{
    Window 		root, parent;
    Window 		*children;
    unsigned int 	nchildren;
    int		 	i;
    Atom 		type = None;
    int 		format;
    unsigned long 	nitems, after;
    unsigned char 	*data;
    Window 		inf = 0;

    XGetWindowProperty(dpy, win, atom, 0, 0, False, AnyPropertyType,
		       &type, &format, &nitems, &after, &data);
    if (type)
      return win;
    else {
	if (!XQueryTree(dpy, win, &root, &parent, &children, &nchildren) ||
	    (nchildren == 0))
	  return 0;
	for (i = nchildren - 1; i >= 0; i--) {
	    if ((inf = GetClientWindow(dpy, children[i], atom)) != NULL)
	      return inf;
	}
    }
    return 0;
}

static void 
#ifdef _NO_PROTO
ValidateDragOver( dc, oldStyle, newStyle )
        XmDragContext dc ;
        unsigned char oldStyle ;
        unsigned char newStyle ;
#else
ValidateDragOver(
        XmDragContext dc,
#if NeedWidePrototypes
        unsigned int oldStyle,
        unsigned int newStyle )
#else
        unsigned char oldStyle,
        unsigned char newStyle )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	Arg		args[1];
	XmDisplay		xmDisplay = (XmDisplay)XtParent(dc);
	unsigned char initiator = XmDRAG_NONE;

	initiator = xmDisplay->display.dragInitiatorProtocolStyle;

	if (newStyle != oldStyle) 
	{
		/*
		* If we're not still waiting to hear from the window manager,
		* and we're not running dynamic, then we can grab.
		*/
		if ((dc->drag.trackingMode != XmDRAG_TRACK_WM_QUERY_PENDING) &&
			(newStyle != XmDRAG_DYNAMIC) &&
			(initiator != XmDRAG_DYNAMIC) &&
			(initiator != XmDRAG_PREFER_DYNAMIC))
		{
			/* We can grab the server, but do we really need to? */
			if (!dc->drag.serverGrabbed)
			{
				XGrabServer(XtDisplay(dc));
				dc->drag.serverGrabbed = True;
				XtSetArg(args[0], XmNdragOverMode, XmPIXMAP);
				XtSetValues( (Widget)dc->drag.curDragOver, args, 1);
			}
		}
		else
		{
			if (dc->drag.serverGrabbed)
			{
				XUngrabServer(XtDisplay(dc));
				dc->drag.serverGrabbed = False;
				XtSetArg(args[0], XmNdragOverMode, XmCURSOR);
				XtSetValues( (Widget)dc->drag.curDragOver, args, 1);
			}
		}
	}
}


static XmDragReceiverInfo
#ifdef _NO_PROTO
FindReceiverInfo( dc, win )
        XmDragContext dc ;
        Window win ;
#else
FindReceiverInfo(
        XmDragContext dc,
        Window win )
#endif /* _NO_PROTO */
{
    Cardinal	i;
    XmDragReceiverInfo info = NULL;

    for (i = 0; i < dc->drag.numReceiverInfos; i++) {
	info = &dc->drag.receiverInfos[i];
	if ((info->frame == win) || (info->window == win))
	  break;
    }
    if (i < dc->drag.numReceiverInfos)
      return info;
    else 
      return NULL;
}

XmDragReceiverInfo
#ifdef _NO_PROTO
_XmAllocReceiverInfo( dc)
        XmDragContext dc ;
#else
_XmAllocReceiverInfo(
        XmDragContext dc )
#endif /* _NO_PROTO */
{
    Cardinal	offset = 0;

    if (dc->drag.currReceiverInfo) {
	offset = (Cardinal) (dc->drag.currReceiverInfo -
			     dc->drag.receiverInfos);
    }
    if (dc->drag.numReceiverInfos == dc->drag.maxReceiverInfos) {
	dc->drag.maxReceiverInfos = dc->drag.maxReceiverInfos*2 + 2;
	dc->drag.receiverInfos = (XmDragReceiverInfoStruct *)
	  XtRealloc((char*)dc->drag.receiverInfos,
		    dc->drag.maxReceiverInfos *
		    sizeof(XmDragReceiverInfoStruct));
    }
    if (offset)
      dc->drag.currReceiverInfo = &(dc->drag.receiverInfos[offset]);
    dc->drag.rootReceiverInfo = dc->drag.receiverInfos;
    return &(dc->drag.receiverInfos[dc->drag.numReceiverInfos++]);
}

/* Find a window with WM_STATE, else return win itself, as per ICCCM */
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
GetDestinationInfo( dc, root, win )
        XmDragContext dc ;
        Window root ;
        Window win ;
#else
GetDestinationInfo(
        XmDragContext dc,
	Window root,
        Window win )
#endif /* _NO_PROTO */
{
    Window	 	clientWin = win;
    Display		*dpy = XtDisplayOfObject((Widget) dc);
    Atom 		WM_STATE =  XmInternAtom(dpy, "WM_STATE", True);
    unsigned char 	oldStyle = dc->drag.activeProtocolStyle;
    XmDragReceiverInfo 	currReceiverInfo;

    dc->drag.crossingTime = dc->drag.lastChangeTime;

    currReceiverInfo = 
      dc->drag.currReceiverInfo = FindReceiverInfo(dc, win);
        
    /* 
     * check for bootstrap case 
     */
    if ((dc->drag.trackingMode == XmDRAG_TRACK_MOTION) &&
	(XtWindow(dc->drag.srcShell) == win) &&
	(!currReceiverInfo || (currReceiverInfo->frame == currReceiverInfo->window))) {
	Window		currRoot = dc->drag.currWmRoot;
	int 		root_x, root_y;
        Position	rel_x, rel_y;
	/* 
	 * set frame (win) to something reasonable
	 */
        rel_x = dc->drag.startX - dc->drag.srcShell->core.x;
        rel_y = dc->drag.startY - dc->drag.srcShell->core.y;

        if (rel_x < 0) rel_x = 0;
	if (rel_y < 0) rel_y = 0;

	(void)
	  XTranslateCoordinates(XtDisplayOfObject((Widget) dc), 
				win, currRoot,
				rel_x, rel_y,
				&root_x, &root_y,
				&win);
	if (currReceiverInfo)
	  currReceiverInfo->frame = win;
    }

    if (currReceiverInfo == NULL) {
	if (clientWin == win) {
	    if ((clientWin = GetClientWindow(dpy, win, WM_STATE)) == 0)
	      clientWin = win;
	}
	currReceiverInfo = 
	  dc->drag.currReceiverInfo = _XmAllocReceiverInfo(dc);
	currReceiverInfo->frame = win;
	currReceiverInfo->window = clientWin;
	currReceiverInfo->shell = XtWindowToWidget(dpy, clientWin);
	
    }

    /* 
     * we fetch the root info in NewScreen
     */
    if (currReceiverInfo != dc->drag.rootReceiverInfo /* is it the root ? */) {
	if (!currReceiverInfo->shell) {
	    if (_XmGetDragReceiverInfo(dpy,
				       currReceiverInfo->window,
				       currReceiverInfo))
	      {
		  switch (currReceiverInfo->dragProtocolStyle) {
		    case XmDRAG_PREREGISTER:
		    case XmDRAG_PREFER_PREREGISTER:
		    case XmDRAG_PREFER_DYNAMIC:
		      break;
		    case XmDRAG_DYNAMIC:
		    case XmDRAG_DROP_ONLY:
		    case XmDRAG_NONE:
		      /* free the data returned by the icc layer */
		      _XmFreeDragReceiverInfo(currReceiverInfo->iccInfo);
		      break;
		  }
	      }
	}
	else {
	    XmDisplay	xmDisplay = (XmDisplay)XtParent(dc);

		/*
		 * We only have a protocol style if we have drop sites.
		 */
		if (_XmDropSiteShell(dc->drag.currReceiverInfo->shell))
			currReceiverInfo->dragProtocolStyle =
			  xmDisplay->display.dragReceiverProtocolStyle;
		else
			currReceiverInfo->dragProtocolStyle = XmDRAG_NONE;

	    currReceiverInfo->xOrigin = dc->drag.currReceiverInfo->shell->core.x;
	    currReceiverInfo->yOrigin = dc->drag.currReceiverInfo->shell->core.y;
	    currReceiverInfo->width = dc->drag.currReceiverInfo->shell->core.width;
	    currReceiverInfo->height = dc->drag.currReceiverInfo->shell->core.height;
	    currReceiverInfo->depth = dc->drag.currReceiverInfo->shell->core.depth;
	    currReceiverInfo->iccInfo = NULL;
	}
    }

	/*
	 * If we're still waiting on the window manager, then don't mess
	 * with the active protocol style.
	 */
	if (dc->drag.trackingMode != XmDRAG_TRACK_WM_QUERY_PENDING)
	{
		dc->drag.activeProtocolStyle =
			_XmGetActiveProtocolStyle((Widget)dc);

		ValidateDragOver(dc, oldStyle, dc->drag.activeProtocolStyle);
	}
}


static void 
#ifdef _NO_PROTO
GetScreenInfo( dc )
        XmDragContext dc ;
#else
GetScreenInfo(
        XmDragContext dc )
#endif /* _NO_PROTO */
{
    Display	*dpy = XtDisplay(dc);
    Window	root = RootWindowOfScreen(XtScreen(dc->drag.curDragOver));
    XmDragReceiverInfo rootInfo;

    /* 
     * the rootInfo is the first entry in the receiverInfo
     * array
     */
    
    if (dc->drag.numReceiverInfos == 0) {
	dc->drag.rootReceiverInfo =
	  rootInfo = _XmAllocReceiverInfo(dc);
    }
    else {
	dc->drag.rootReceiverInfo =
	  rootInfo = dc->drag.receiverInfos;
    }

    rootInfo->frame = None;
    rootInfo->window = root;
    rootInfo->shell = XtWindowToWidget(dpy, root);
	rootInfo->xOrigin = rootInfo->yOrigin = 0;
	rootInfo->width = XWidthOfScreen(dc->drag.currScreen);
	rootInfo->height = XHeightOfScreen(dc->drag.currScreen);
	rootInfo->depth = DefaultDepthOfScreen(dc->drag.currScreen);
	rootInfo->iccInfo = NULL;

    if (_XmGetDragReceiverInfo(dpy,
			       root,
			       rootInfo))
      {
	  switch (rootInfo->dragProtocolStyle) {
	    case XmDRAG_PREREGISTER:
	    case XmDRAG_PREFER_PREREGISTER:
	    case XmDRAG_PREFER_DYNAMIC:
	      break;
	    case XmDRAG_DYNAMIC:
	    case XmDRAG_DROP_ONLY:
	    case XmDRAG_NONE:
	      /* free the data returned by the icc layer */
	      _XmFreeDragReceiverInfo(rootInfo->iccInfo);
	      break;
	  }
      }
} 


static void 
#ifdef _NO_PROTO
SendDragMessage( dc, destination, messageType )
        XmDragContext dc ;
        Window destination ;
        unsigned char messageType ;
#else
SendDragMessage(
        XmDragContext dc,
        Window destination,
#if NeedWidePrototypes
        unsigned int messageType )
#else
        unsigned char messageType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay)(XtParent(dc)));
    XmICCCallbackStruct	callbackRec;
    int			reason =
      _XmMessageTypeToReason(messageType);
    
    callbackRec.any.event = NULL;

    if (dc->drag.activeProtocolStyle == XmDRAG_DROP_ONLY)
	{
		XmDragOverShellWidget dos = dc->drag.curDragOver;

		if (reason == XmCR_TOP_LEVEL_ENTER)
		{
			_XmDragOverChange((Widget) dos, XmVALID_DROP_SITE);
			return;
		}
		if (reason == XmCR_TOP_LEVEL_LEAVE)
		{
			_XmDragOverChange((Widget) dos, XmINVALID_DROP_SITE);
			return;
		}
	}

    if ((dc->drag.activeProtocolStyle == XmDRAG_NONE)
	||
	((dc->drag.activeProtocolStyle == XmDRAG_DROP_ONLY) &&
	 (reason != XmCR_DROP_START)))
      return;

    switch(callbackRec.any.reason = reason) {
      case XmCR_TOP_LEVEL_ENTER:
	{
	    XmTopLevelEnterCallback	callback =
	      (XmTopLevelEnterCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->window = dc->drag.srcWindow;
	    callback->dragProtocolStyle =
	      dc->drag.activeProtocolStyle;
	    callback->screen = dc->drag.currScreen;
	    callback->x = dc->drag.currReceiverInfo->xOrigin;
	    callback->y = dc->drag.currReceiverInfo->yOrigin;
	    callback->iccHandle = dc->drag.iccHandle;
	}
	break;
      case XmCR_TOP_LEVEL_LEAVE:
	{
	    XmTopLevelLeaveCallback	callback =
	      (XmTopLevelLeaveCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->window = dc->drag.srcWindow;
	}
	break;
      case XmCR_DRAG_MOTION:
	{
	    XmDragMotionCallback	callback =
	      (XmDragMotionCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->x = dc->core.x;
	    callback->y = dc->core.y;
	    callback->operation = dc->drag.operation;
	    callback->operations = dc->drag.operations;

		/* Outgoing motion; be conservative */
		callback->dropSiteStatus = XmNO_DROP_SITE;
	}
	break;
      case XmCR_OPERATION_CHANGED:
	{
	    XmOperationChangedCallback	callback =
	      (XmOperationChangedCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->operation = dc->drag.operation;
	    callback->operations = dc->drag.operations;
	}
	break;
      case XmCR_DROP_START:
	{
	    XmDropStartCallback 	callback =
	      (XmDropStartCallback)&callbackRec;

	    callback->timeStamp = dc->drag.dragFinishTime;
	    callback->operation = dc->drag.operation;
	    callback->operations = dc->drag.operations;
	    callback->dropAction = dc->drag.dragCompletionStatus;
	    callback->x = dc->core.x;
	    callback->y = dc->core.y;
	    callback->iccHandle = dc->drag.iccHandle;
	    callback->window = XtWindow(dc->drag.srcShell);
	}
	break;
      default:
	break;
    }
    /* 	
     * if we're the initiator and the destination isn't us and either
     * its the drop message or the dynamic protocol send it to the wire
     */
    if ((!dc->drag.currReceiverInfo->shell) && 
	(!dc->drag.sourceIsExternal /* sanity check */) &&
	((dc->drag.activeProtocolStyle == XmDRAG_DYNAMIC) ||
	 (reason == XmCR_DROP_START)))
      {
	  _XmSendICCCallback(XtDisplayOfObject((Widget) dc), destination,
			     &callbackRec, XmICC_INITIATOR_EVENT);
      }
    else {
	XtPointer			data;
	XmDragTopLevelClientDataStruct	topLevelData;
	XmDragMotionClientDataStruct	motionData;

	if ((reason == XmCR_TOP_LEVEL_ENTER)   ||
	    (reason == XmCR_TOP_LEVEL_LEAVE)   || 
	    (reason == XmCR_DROP_START)){
	    
	    topLevelData.destShell = dc->drag.currReceiverInfo->shell;
	    topLevelData.sourceIsExternal = dc->drag.sourceIsExternal;
	    topLevelData.iccInfo = dc->drag.currReceiverInfo->iccInfo;
	    topLevelData.xOrigin =
			(Position)(dc->drag.currReceiverInfo->xOrigin);
	    topLevelData.yOrigin = (Position)
			(dc->drag.currReceiverInfo->yOrigin);
	    topLevelData.width = (Dimension)
			(dc->drag.currReceiverInfo->width);
	    topLevelData.height = (Dimension)
			(dc->drag.currReceiverInfo->height);
		topLevelData.window = dc->drag.currReceiverInfo->window;
		topLevelData.dragOver = (Widget)dc->drag.curDragOver;
	    data = (XtPointer)&topLevelData;
	}
	else if ((reason == XmCR_DRAG_MOTION) ||
	    (reason == XmCR_OPERATION_CHANGED)) {
	    motionData.window = dc->drag.currReceiverInfo->window;
	    motionData.dragOver = (Widget)dc->drag.curDragOver;
	    data = (XtPointer)&motionData;
	}
	else {
		data = NULL;
	}

	_XmDSMUpdate(dsm, 
		     (XtPointer)data,
		     (XtPointer)&callbackRec);
    }
}

static void 
#ifdef _NO_PROTO
GenerateClientCallback( dc, reason )
        XmDragContext dc ;
        unsigned char reason ;
#else
GenerateClientCallback(
        XmDragContext dc,
#if NeedWidePrototypes
        unsigned int reason )
#else
        unsigned char reason )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmICCCallbackStruct	callbackRec;
    XtCallbackList	callbackList = NULL;

    callbackRec.any.event = NULL;

    switch(callbackRec.any.reason = reason) {
      case XmCR_TOP_LEVEL_ENTER:
	if ((callbackList = dc->drag.topLevelEnterCallback) != NULL) {
	    XmTopLevelEnterCallback	callback =
	      (XmTopLevelEnterCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->window = dc->drag.currReceiverInfo->window;
	    callback->dragProtocolStyle =
	      dc->drag.activeProtocolStyle;
	    callback->screen = dc->drag.currScreen;
	    callback->iccHandle = dc->drag.iccHandle;
	    callback->x = dc->drag.currReceiverInfo->xOrigin;
	    callback->y = dc->drag.currReceiverInfo->yOrigin;
	}
	break;
      case XmCR_TOP_LEVEL_LEAVE:
	if ((callbackList = dc->drag.topLevelLeaveCallback) != NULL) {
	    XmTopLevelLeaveCallback	callback =
	      (XmTopLevelLeaveCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->screen = dc->drag.currScreen;
	    callback->window = dc->drag.currReceiverInfo->window;
	}
	break;
      case XmCR_DRAG_MOTION:
	if ((callbackList = dc->drag.dragMotionCallback) != NULL) {
	    XmDragMotionCallback	callback =
	      (XmDragMotionCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->x = dc->core.x;
	    callback->y = dc->core.y;
	    callback->operation = dc->drag.operation;
	    callback->operations = dc->drag.operations;

		/*
		 * If we're over DropOnly client, be optimistic.
		 */
		if (dc->drag.activeProtocolStyle == XmDRAG_DROP_ONLY)
		{
			callback->dropSiteStatus = XmVALID_DROP_SITE;
		}
		else
		{
			/*
			 * Otherwise, we're over the root (see
			 * DragMotionProto), and there's no drop site
			 * under us.
			 */
			callback->dropSiteStatus = XmNO_DROP_SITE;
		}
	}
	break;
      case XmCR_OPERATION_CHANGED:
	if ((callbackList = dc->drag.operationChangedCallback) != NULL) {
	    XmOperationChangedCallback	callback =
	      (XmOperationChangedCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	    callback->operation = dc->drag.operation;
	    callback->operations = dc->drag.operations;
            /*
               * If we're over DropOnly client, be optimistic.
               */
              if (dc->drag.activeProtocolStyle == XmDRAG_DROP_ONLY)
              {
                      callback->dropSiteStatus = XmVALID_DROP_SITE;
              }
              else
              {
                      /*
                       * Otherwise, we're over the root (see
                       * DragMotionProto), and there's no drop site
                       * under us.
                       */
                      callback->dropSiteStatus = XmNO_DROP_SITE;
              }
	}
	break;
      case XmCR_DROP_SITE_ENTER:
	{

#ifdef I18N_MSG
	    _XmWarning((Widget)dc, catgets(Xm_catd,MS_DragC,MSG_DRC_1, MESSAGE1));
#else
	    _XmWarning((Widget)dc, MESSAGE1);
#endif

	}
	break;
      case XmCR_DROP_SITE_LEAVE:
	{
	    XmDropSiteLeaveCallback	callback =
	      (XmDropSiteLeaveCallback)&callbackRec;
	    
	    callback->timeStamp = dc->drag.lastChangeTime;
	}
	break;
      default:
	break;
    }
    if (callbackList) 
    {
      XtCallCallbackList( (Widget)dc, callbackList, &callbackRec);

      if (callbackList == dc->drag.dragMotionCallback)
      {
         XmDragOverShellWidget dos = dc->drag.curDragOver;
         XmDragMotionCallback  callback =
                       (XmDragMotionCallback)&callbackRec;

         dc->drag.operation = callback->operation;
         dc->drag.operations = callback->operations;
         if (dos->drag.cursorState != callback->dropSiteStatus)
         {
            dos->drag.cursorState = callback->dropSiteStatus;
            _XmDragOverChange((Widget)dos, dos->drag.cursorState);
         }
      }
    }
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DropLoseIncrSelection( w, selection, clientData )
        Widget w ;
        Atom *selection ;
        XtPointer clientData ;
#else
DropLoseIncrSelection(
        Widget w,
        Atom *selection,
        XtPointer clientData )
#endif /* _NO_PROTO */
{
    DropLoseSelection( w, selection) ;
    }


static void 
#ifdef _NO_PROTO
DropLoseSelection( w, selection)
        Widget w ;
        Atom *selection ;
#else
DropLoseSelection(
        Widget w,
        Atom *selection)
#endif /* _NO_PROTO */
{   
            XmDragContext	dc ;

    if(!(dc = (XmDragContext) _XmGetDragContextFromHandle( w,
                                           *selection))    )

#ifdef I18N_MSG
    {   _XmWarning(w, catgets(Xm_catd,MS_DragC,MSG_DRC_2,
		MESSAGE2)) ;
        }
#else
    {   _XmWarning(w, MESSAGE2) ;
        }
#endif

    if(    dc->drag.dropFinishTime == 0    ) 
    {   

#ifdef I18N_MSG
        _XmWarning(w, catgets(Xm_catd,MS_DragC,MSG_DRC_3,
	        MESSAGE3)) ;
#else
        _XmWarning(w, MESSAGE3) ;
#endif

        }
} 

static void 
#ifdef _NO_PROTO
DragDropFinish( dc )
        XmDragContext dc ;
#else
DragDropFinish(
        XmDragContext dc )
#endif /* _NO_PROTO */
{
    XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay)(XtParent(dc)));

    /* Handle completion */
    if (dc->drag.dropFinishCallback) {
	XmDropFinishCallbackStruct cb;
	
	cb.reason = XmCR_DROP_FINISH;
	cb.event = NULL;
	cb.timeStamp = dc->drag.dropFinishTime;
	cb.operation = dc->drag.operation;
	cb.operations = dc->drag.operations;
	cb.dropSiteStatus = dsm->dropManager.curDropSiteStatus;
	cb.dropAction = dc->drag.dragCompletionStatus;
	cb.completionStatus = dc->drag.dragDropCompletionStatus;
	XtCallCallbackList((Widget) dc, dc->drag.dropFinishCallback, &cb);
	dc->drag.dragDropCompletionStatus = cb.completionStatus;
    }

    if (dc->drag.blendModel != XmBLEND_NONE) {
	_XmDragOverFinish((Widget)dc->drag.curDragOver, 
			  dc->drag.dragDropCompletionStatus);
    }

    if (dc->drag.dragDropFinishCallback) {
	XmDragDropFinishCallbackStruct cb;
	
	cb.reason = XmCR_DRAG_DROP_FINISH;
	cb.event = NULL;
	cb.timeStamp = dc->drag.dropFinishTime;
	XtCallCallbackList((Widget) dc, dc->drag.dragDropFinishCallback, &cb);
    }
    /*
     * we send this now so that the non-local receiver can clean up
     * its dc after everything is done
     */

    XtDisownSelection(dc->drag.srcShell, 
		      dc->drag.iccHandle,
		      dc->drag.dragFinishTime);

    _XmFreeMotifAtom((Widget)dc, dc->drag.iccHandle);

    XtRemoveEventHandler(dc->drag.srcShell, FocusChangeMask, True,
			 InitiatorMsgHandler, 
			 (XtPointer)dc);
    XtDestroyWidget((Widget) dc);
}



/*
 * This routine is passed as the frontend to the convertProc.
 */
/*VARARGS*/
static Boolean 
#ifdef _NO_PROTO
DropConvertIncrCallback( w, selection, target, typeRtn, valueRtn, lengthRtn, formatRtn, maxLengthRtn, clientData, requestID )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *typeRtn ;
        XtPointer *valueRtn ;
        unsigned long *lengthRtn ;
        int *formatRtn ;
        unsigned long *maxLengthRtn ;
        XtPointer clientData ;
        XtRequestId *requestID ;
#else
DropConvertIncrCallback(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *typeRtn,
        XtPointer *valueRtn,
        unsigned long *lengthRtn,
        int *formatRtn,
        unsigned long *maxLengthRtn,
        XtPointer clientData,
        XtRequestId *requestID )
#endif /* _NO_PROTO */
{
    XmDragContext	dc;
    XrmQuark			xrmTarget = NULLQUARK;
    String			targetName = NULL;
    Time			dropTime;
    Boolean 			returnVal = True;

    if (xrmFailure == NULLQUARK) {
	xrmSuccess = XrmStringToQuark("XmTRANSFER_SUCCESS");
	xrmFailure = XrmStringToQuark("XmTRANSFER_FAILURE");
    }
    dropTime = XtGetSelectionRequest(w, *selection, NULL)->time;

    if (!(dc = (XmDragContext)
	  _XmGetDragContextFromHandle(w, *selection))) {

#ifdef I18N_MSG
	_XmWarning(w, catgets(Xm_catd,MS_DragC,MSG_DRC_2,
                   MESSAGE2));
#else
	_XmWarning(w, MESSAGE2);
#endif

	return False;
    }
    if (((targetName = XmGetAtomName(XtDisplayOfObject((Widget) dc),
				 *target)) &&
	((xrmTarget = XrmStringToQuark(targetName)) == xrmFailure)) ||
	 (xrmTarget == xrmSuccess))
      {
	  if (xrmTarget == xrmFailure)
	    dc->drag.dragDropCompletionStatus = XmDROP_FAILURE;
	  else
	    dc->drag.dragDropCompletionStatus = XmDROP_SUCCESS;
	  *typeRtn = *target;
	  *lengthRtn = 0;
	  *formatRtn = 32;
	  *valueRtn = NULL;
	  *maxLengthRtn = 0;
	  /*
	   * the time is really of the start of the transfer but ...
	   */
	  dc->drag.dropFinishTime = dropTime;
	  DragDropFinish(dc);
      }
    else /* normal transfer */
      {
	  Atom		motifDrop;
	  
	  motifDrop = XmInternAtom(XtDisplay(dc), 
				   "_MOTIF_DROP",
				   False);
	  
	  returnVal =  (Boolean)((*(dc->drag.convertProc.sel_incr))
				 ((Widget)dc, 
				  &motifDrop,
				  target, 
				  typeRtn, 
				  valueRtn, 
				  lengthRtn, 
				  formatRtn,
				  maxLengthRtn,
				  clientData,
				  requestID));
      }
    if (targetName) XtFree(targetName);
    return returnVal;
}

/*
 * This routine is passed as the frontend to the convertProc.
 */
/*VARARGS*/
static Boolean 
#ifdef _NO_PROTO
DropConvertCallback( w, selection, target, typeRtn, valueRtn, lengthRtn, formatRtn )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *typeRtn ;
        XtPointer *valueRtn ;
        unsigned long *lengthRtn ;
        int *formatRtn ;
#else
DropConvertCallback(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *typeRtn,
        XtPointer *valueRtn,
        unsigned long *lengthRtn,
        int *formatRtn )
#endif /* _NO_PROTO */
{
    XmDragContext	dc;
    XrmQuark			xrmTarget = NULLQUARK;
    String			targetName = NULL;
    Time			dropTime;
    Boolean 			returnVal = True;

    if (xrmFailure == NULLQUARK) {
	xrmSuccess = XrmStringToQuark("XmTRANSFER_SUCCESS");
	xrmFailure = XrmStringToQuark("XmTRANSFER_FAILURE");
    }
    dropTime = XtGetSelectionRequest(w, *selection, NULL)->time;

    if (!(dc = (XmDragContext)
	  _XmGetDragContextFromHandle(w, *selection))) {

#ifdef I18N_MSG
	_XmWarning(w,catgets(Xm_catd,MS_DragC,MSG_DRC_2,
		MESSAGE2));
#else
	_XmWarning(w, MESSAGE2);
#endif

	return False;
    }
    if (((targetName = XmGetAtomName(XtDisplayOfObject((Widget) dc),
				 *target)) &&
	((xrmTarget = XrmStringToQuark(targetName)) == xrmFailure)) ||
	 (xrmTarget == xrmSuccess))
      {
	  if (xrmTarget == xrmFailure)
	    dc->drag.dragDropCompletionStatus = XmDROP_FAILURE;
	  else
	    dc->drag.dragDropCompletionStatus = XmDROP_SUCCESS;
	  *typeRtn = *target;
	  *lengthRtn = 0;
	  *formatRtn = 32;
	  *valueRtn = NULL;
	  /*
	   * the time is really of the start of the transfer but ...
	   */
	  dc->drag.dropFinishTime = dropTime;
	  DragDropFinish(dc);
      }
    else /* normal transfer */
      {
	  Atom		motifDrop;

	  motifDrop = XmInternAtom(XtDisplay(dc), 
					"_MOTIF_DROP",
					False);

	  returnVal =  (Boolean)
		       ((*(dc->drag.convertProc.sel))
				 ((Widget)dc, 
				  &motifDrop,
				  target, 
				  typeRtn, 
				  valueRtn, 
				  lengthRtn, 
				  formatRtn));
      }
    if (targetName) XtFree(targetName);
    return returnVal;
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DragStartProto( dc )
        XmDragContext dc ;
#else
DragStartProto(
        XmDragContext dc)
#endif /* _NO_PROTO */
{
    /*
     * bootstrap the top_level_window code
     */
    _XmWriteInitiatorInfo((Widget)dc);
    GetDestinationInfo(dc, 
		       RootWindowOfScreen(XtScreen(dc)),
		       XtWindow(dc->drag.srcShell));
    GenerateClientCallback(dc, XmCR_TOP_LEVEL_ENTER);
    SendDragMessage(dc, dc->drag.currReceiverInfo->window, XmTOP_LEVEL_ENTER);
    SendDragMessage(dc, dc->drag.currReceiverInfo->window, XmDRAG_MOTION);
}


static void 
#ifdef _NO_PROTO
NewScreen(dc, newRoot)
    XmDragContext	dc;
    Window		newRoot;
#else
NewScreen(
	  XmDragContext	dc,
	  Window	newRoot
	      )
#endif /* _NO_PROTO */
{
	Cardinal	i;
	Arg		args[8];
	Widget  old = (Widget) (dc->drag.curDragOver);

	/* Find the new screen number */
	for (i = 0; i < XScreenCount(XtDisplayOfObject((Widget) dc)); i++)
		if (RootWindow(XtDisplayOfObject((Widget) dc), i) == newRoot)
			break;

	dc->drag.currScreen =
		ScreenOfDisplay(XtDisplayOfObject((Widget) dc), i);
	dc->drag.currWmRoot = RootWindowOfScreen(dc->drag.currScreen);


	/* Build a new one */
	i = 0;
	/*
	 * If this is the first call, tracking mode will be querypending
	 * and we have to come up in cursor mode.  Otherwise, we come up
	 * in cursor for dynamic and pixmap for preregister.
	 */
	if ((dc->drag.trackingMode == XmDRAG_TRACK_WM_QUERY_PENDING) ||
		(dc->drag.activeProtocolStyle == XmDRAG_DYNAMIC))
	{
		XtSetArg(args[i], XmNdragOverMode, XmCURSOR); i++;
	}
	else
	{
		XtSetArg(args[i], XmNdragOverMode, XmPIXMAP); i++;
	}

	XtSetArg(args[i], XmNhotX, dc->core.x); i++;
	XtSetArg(args[i], XmNhotY, dc->core.y); i++;
	XtSetArg(args[i], XmNbackgroundPixmap, None); i++;
	XtSetArg(args[i], XmNscreen, dc->drag.currScreen);i++;
	XtSetArg(args[i], XmNdepth, 
		DefaultDepthOfScreen(dc->drag.currScreen));i++;
	XtSetArg(args[i], XmNcolormap, 
		DefaultColormapOfScreen(dc->drag.currScreen));i++;
	XtSetArg(args[i], XmNvisual, 
		DefaultVisualOfScreen(dc->drag.currScreen));i++;

	/*
	 * As popup child(ren) of the dc, the drag over(s) will 
	 * automatically destroyed by Xt when the dc is destroyed.
	 * Isn't that handy?
	 */
	dc->drag.curDragOver = (XmDragOverShellWidget)
		XtCreatePopupShell("dragOver", xmDragOverShellWidgetClass,
			(Widget) dc, args, i);
	
	if (dc->drag.currScreen == XtScreen(dc->drag.srcShell))
		_XmDragOverSetInitialPosition((Widget)dc->drag.curDragOver,
			dc->drag.startX, dc->drag.startY);
	
	if (old != NULL) {
		if (old != (Widget) (dc->drag.origDragOver))
			XtDestroyWidget(old);
		else
			_XmDragOverHide((Widget)dc->drag.origDragOver,
					0, 0, NULL);
	}

	GetScreenInfo(dc);

	if (dc->drag.origDragOver == NULL)
	{
		dc->drag.origDragOver = dc->drag.curDragOver;
	}

	if (dc->drag.trackingMode == XmDRAG_TRACK_MOTION)
	{
	    EventMask	mask = _XmDRAG_EVENT_MASK(dc);

	    if (XGrabPointer(XtDisplayOfObject((Widget) dc->drag.curDragOver),
			     RootWindowOfScreen(XtScreen(dc->drag.curDragOver)),
			     False,
			     mask,
			     GrabModeSync,
			     GrabModeAsync,
			     None,
	            _XmDragOverGetActiveCursor((Widget)dc->drag.curDragOver),
			     dc->drag.lastChangeTime) != GrabSuccess)

#ifdef I18N_MSG
			Warning(catgets(Xm_catd,MS_DragC,MSG_DRC_4,
			    MESSAGE4));
#else
			Warning(MESSAGE4);
#endif


	    XAllowEvents(XtDisplayOfObject((Widget) dc->drag.srcShell),
			 SyncPointer,
			 dc->drag.lastChangeTime);
	}
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
LocalNotifyHandler( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
LocalNotifyHandler(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDropSiteManagerObject 	dsm = (XmDropSiteManagerObject)w;
    XmDragContext	dc = (XmDragContext)client;

    switch(((XmAnyICCCallback)call)->reason) {
      case XmCR_DROP_SITE_ENTER:
	SiteEnteredWithLocalSource((Widget)dsm, 
				   (XtPointer)dc, 
				   (XtPointer)call);
	break;
      case XmCR_DROP_SITE_LEAVE:
	SiteLeftWithLocalSource((Widget) dsm, 
				(XtPointer) dc, 
				(XtPointer) call);
	break;
      case XmCR_DRAG_MOTION:
	SiteMotionWithLocalSource((Widget)dsm,
			      (XtPointer)dc,
			      (XtPointer)call);
	break;
      case XmCR_OPERATION_CHANGED:
	OperationChanged((Widget)dsm,
			 (XtPointer)dc,
			 (XtPointer)call);
	break;
      case XmCR_DROP_START:
	DropStartConfirmed((Widget)dsm,
			   (XtPointer)dc,
			   (XtPointer)call);
      default:
	break;
    }
}

/*
 * sends replies to drag messages 
 */
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ExternalNotifyHandler( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
ExternalNotifyHandler(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)client;
    XmAnyICCCallback		cb = (XmAnyICCCallback)call;

    switch(cb->reason) {
      case XmCR_DROP_SITE_ENTER:
      case XmCR_DROP_SITE_LEAVE:
      case XmCR_DRAG_MOTION:
      case XmCR_OPERATION_CHANGED:
      case XmCR_DROP_START:
	/*
	 * send a message to the external source 
	 */
	_XmSendICCCallback(XtDisplayOfObject((Widget) dc), 
			   dc->drag.srcWindow, 
			   (XmICCCallback)cb,
			   XmICC_RECEIVER_EVENT);
	
	break;
      default:

#ifdef I18N_MSG
	_XmWarning((Widget)dc, catgets(Xm_catd,MS_DragC,MSG_DRC_5,
	    MESSAGE5));
#else
	_XmWarning((Widget)dc, MESSAGE5);
#endif

	break;
    }
}


/*
 * catches replies on drag messages 
 */
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
InitiatorMsgHandler( w, clientData, event, dontSwallow )
        Widget w ;
        XtPointer clientData ;
        XEvent *event ;
        Boolean *dontSwallow ;
#else
InitiatorMsgHandler(
        Widget w,
        XtPointer clientData,
        XEvent *event,
        Boolean *dontSwallow )
#endif /* _NO_PROTO */
{
	XmDragContext	dc =(XmDragContext)clientData;
	XmICCCallbackStruct		callbackRec;

	if ((dc && (event->type != ClientMessage)) ||
		(!_XmICCEventToICCCallback((XClientMessageEvent *)event,
				&callbackRec, XmICC_RECEIVER_EVENT)) ||
		(dc->drag.dragStartTime > callbackRec.any.timeStamp) ||
		(dc->drag.crossingTime > callbackRec.any.timeStamp))
	return;

	LocalNotifyHandler(w, (XtPointer)dc, (XtPointer)&callbackRec);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
SiteEnteredWithLocalSource( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
SiteEnteredWithLocalSource(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)client;
    XmDropSiteEnterCallbackStruct  *cb = (XmDropSiteEnterCallbackStruct *)call;
    
    /* check against the current location of the pointer */

    if (dc->drag.siteEnterCallback) {
	XtCallCallbackList((Widget) dc, dc->drag.siteEnterCallback, cb);
    }
    dc->drag.operation = cb->operation;
    dc->drag.operations = cb->operations;
    dc->drag.inDropSite = True;
    _XmDragOverChange((Widget)dc->drag.curDragOver, cb->dropSiteStatus);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
SiteLeftWithLocalSource( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
SiteLeftWithLocalSource(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)client;
    XmDropSiteEnterPendingCallbackStruct  *cb =
			 (XmDropSiteEnterPendingCallbackStruct *)call;
    
	dc->drag.inDropSite = False;

    if (dc->drag.siteLeaveCallback) {
	XtCallCallbackList((Widget) dc, dc->drag.siteLeaveCallback,
			   (XmDropSiteLeaveCallbackStruct *)cb);
    }

    /*
     * Re-set to the initial settings of operation and operations
     */
    dc->drag.operations = dc->drag.dragOperations;
    if ((dc->drag.lastEventState & ShiftMask) &&
	(dc->drag.lastEventState & ControlMask)) {
	dc->drag.operations = 
	  dc->drag.operation =  (unsigned char)
	    (XmDROP_LINK & dc->drag.dragOperations);
    }
    else if (dc->drag.lastEventState & ShiftMask) {
	dc->drag.operations =
	  dc->drag.operation =  (unsigned char)
	    (XmDROP_MOVE & dc->drag.dragOperations);
    }
    else if (dc->drag.lastEventState & ControlMask) {
	dc->drag.operations = 
	  dc->drag.operation = (unsigned char)
	    (XmDROP_COPY & dc->drag.dragOperations);
    }
#ifdef USE_MOD1_FOR_INTERRUPT
    else if (dc->drag.lastEventState & Mod1Mask)
      /* do something for interrupt */
#endif
    else if (XmDROP_MOVE &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_MOVE;
    else if (XmDROP_COPY &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_COPY;
    else if (XmDROP_LINK &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_LINK;
    else {
	dc->drag.operations = 
	  dc->drag.operation = 0;
    }

    /*
     * dont forward laggard echo leaves to dragUnder
     */
    if (dc->drag.dragFinishTime == 0 && !cb->enter_pending)
      _XmDragOverChange((Widget)dc->drag.curDragOver, XmNO_DROP_SITE);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
OperationChanged( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
OperationChanged(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)client;
    XmOperationChangedCallbackStruct  *cb = (XmOperationChangedCallbackStruct *)call;
    
    if (dc->drag.operationChangedCallback) {
	XtCallCallbackList((Widget) dc, dc->drag.operationChangedCallback, cb);
    }
    dc->drag.operation = cb->operation;
    dc->drag.operations = cb->operations;
    _XmDragOverChange((Widget)dc->drag.curDragOver, cb->dropSiteStatus);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
SiteMotionWithLocalSource( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
SiteMotionWithLocalSource(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)client;
    XmDragMotionCallbackStruct  *cb = (XmDragMotionCallbackStruct *)call;

    if (dc->drag.dragMotionCallback) {
	XtCallCallbackList((Widget) dc, dc->drag.dragMotionCallback, cb);
    }
    dc->drag.operation = cb->operation;
    dc->drag.operations = cb->operations;
    _XmDragOverChange((Widget)dc->drag.curDragOver, cb->dropSiteStatus);
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DropStartConfirmed( w, client, call )
        Widget w ;
        XtPointer client ;
        XtPointer call ;
#else
DropStartConfirmed(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)client;
    XmDropStartCallbackStruct  *cb = (XmDropStartCallbackStruct *)call;

    if (dc->drag.dragTimerId) {
	XtRemoveTimeOut(dc->drag.dragTimerId);
	dc->drag.dragTimerId = (XtIntervalId) NULL;
    }
    if (dc->drag.dropStartCallback) {
	XtCallCallbackList( (Widget)dc, dc->drag.dropStartCallback, cb);
    }
    dc->drag.dragCompletionStatus = cb->dropAction;

    switch(dc->drag.dragCompletionStatus) {
      case XmDROP:
      case XmDROP_INTERRUPT:
      case XmDROP_HELP:
	break;
      case XmDROP_CANCEL: 
	break;
    }
}


static Widget 
#ifdef _NO_PROTO
GetShell( w )
        Widget w ;
#else
GetShell(
        Widget w )
#endif /* _NO_PROTO */
{
    while (w && !XtIsShell(w))
      w = XtParent(w);
    return w;
}

static void 
#ifdef _NO_PROTO
InitDropSiteManager( dc )
        XmDragContext dc ;
#else
InitDropSiteManager(
        XmDragContext dc )
#endif /* _NO_PROTO */
{
    XmDropSiteManagerObject	dsm;
    Arg				args[4];
    Cardinal			i = 0;

    dsm = _XmGetDropSiteManagerObject((XmDisplay)(XtParent(dc)));
    
    XtSetArg(args[i], XmNclientData, dc); i++;
    if (dc->drag.sourceIsExternal) {
	XtSetArg(args[i], XmNnotifyProc, ExternalNotifyHandler); i++;
    }
    else {
	XtSetArg(args[i], XmNnotifyProc, LocalNotifyHandler); i++;
    }
    XtSetValues((Widget)dsm, args, i);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
TopWindowsReceived( w, client_data, selection, type, value, length, format )
        Widget w ;
        XtPointer client_data ;
        Atom *selection ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
TopWindowsReceived(
        Widget w,
        XtPointer client_data,
        Atom *selection,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    XmDragContext dc = (XmDragContext)client_data;
	XmDisplay dd = (XmDisplay) w;
    Cardinal	i;
    XmDragReceiverInfo	currInfo, startInfo;
    Window	*clientWindows;
    unsigned char	oldStyle;

	if (dd->display.activeDC != dc)
	{
		/* Too late... */
		return;
	}


    /* CR 6337.  Must call DragOverChange if the blendModel
       changes.  This doesn't happen often,  only in certain
       cases involving PREREGISTER */

    if (dc -> drag.blendModel != dc -> drag.activeBlendModel) {
      dc->drag.blendModel = dc->drag.activeBlendModel;
      _XmDragOverChange((Widget)dc->drag.curDragOver, XmNO_DROP_SITE);
    }



#ifdef MULTI_SCREEN_DONE
    if ((*length != 0) && (*format == 32) && (*type == XA_WINDOW)) { 
	/* 
	 * we make a receiverInfo array one larger than the number of
	 * client windows since we keep the root info in array[0].
	 */
	if (dc->drag.numReceiverInfos >= 1)
	  startInfo = dc->drag.receiverInfos;
	else
	  startInfo = NULL;

	dc->drag.numReceiverInfos = 
	  dc->drag.maxReceiverInfos = *length + 1;
	dc->drag.receiverInfos = (XmDragReceiverInfo)
	  XtCalloc(dc->drag.maxReceiverInfos, sizeof(XmDragReceiverInfoStruct));
	clientWindows = (Window *)value;

	if (startInfo) {
	    memcpy((char *)dc->drag.receiverInfos, 
		   (char *)startInfo,
		   sizeof(XmDragReceiverInfoStruct));
	    dc->drag.rootReceiverInfo = dc->drag.receiverInfos;
	    XtFree((char *)startInfo);
	}
#ifdef DEBUG
	else
	  Warning("we don't have startInfo when we should\n");
#endif /* DEBUG */	
	for (i = 1; i < dc->drag.numReceiverInfos; i++) {
	    currInfo = &dc->drag.receiverInfos[i];
	    currInfo->window = clientWindows[i-1];
	    currInfo->shell = XtWindowToWidget(XtDisplay(dc), 
					       currInfo->window);
	    if (currInfo->shell == NULL) {
		XSelectInput(XtDisplay(dc),
			     currInfo->window,
			     EnterWindowMask | LeaveWindowMask);
	    }
			     
	}
	/*
	 * set the currReceiver to the srcShell since that's where
	 * we're confined to 
	 */
	dc->drag.currReceiverInfo = 
	  FindReceiverInfo(dc, XtWindow(dc->drag.srcShell));
	dc->drag.trackingMode = XmDRAG_TRACK_WM_QUERY;

	oldStyle = dc->drag.activeProtocolStyle;
	dc->drag.activeProtocolStyle =
		_XmGetActiveProtocolStyle((Widget)dc);
	ValidateDragOver(dc, oldStyle, dc->drag.activeProtocolStyle);
    }
    else
#endif /* MULTI_SCREEN_DONE */
    {
	EventMask	mask;
	Window		confineWindow;
	Cursor		cursor;

	dc->drag.trackingMode = XmDRAG_TRACK_MOTION;
	GetDestinationInfo(dc, 
			   dc->drag.currWmRoot,
			   dc->drag.currReceiverInfo->window);
#ifndef MULTI_SCREEN_DONE
	confineWindow = RootWindowOfScreen(XtScreen(dc));
#else
	confineWindow = None;
#endif /* MULTI_SCREEN_DONE */

	/*
	 * we need to regrab so that the confine window can be changed
	 * from the source window. If there was another value for
	 * trackingMode like XmDRAG_TRACK_WM_QUERY_FAILED we could do
	 * this in DragStartWithTracking
	 */

	mask = _XmDRAG_EVENT_MASK(dc);
	cursor = _XmDragOverGetActiveCursor((Widget)dc->drag.curDragOver);

	if (XGrabPointer(XtDisplayOfObject((Widget) dc),
			 RootWindowOfScreen(XtScreen(dc)),
			 False,
			 mask,
			 GrabModeSync,
			 GrabModeAsync,
			 confineWindow,
			 cursor,
			 dc->drag.lastChangeTime) != GrabSuccess)

#ifdef I18N_MSG
	  Warning(catgets(Xm_catd,MS_DragC,MSG_DRC_4, MESSAGE4));
#else
	  Warning(MESSAGE4);
#endif

    }
#ifdef MULTI_SCREEN_DONE
    if (value)
      XtFree((char *)value);
#endif /* MULTI_SCREEN_DONE */

    DragStartWithTracking(dc);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DragStart( dc, src, event )
        XmDragContext dc ;
        Widget src ;
        XEvent *event ;
#else
DragStart(
        XmDragContext dc,
        Widget src,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmDisplay			dd;
    unsigned char 	activeProtocolStyle;
    unsigned int		state = event->xbutton.state;
    EventMask			mask;
    Window			saveWindow;
    Window			confineWindow;
    Cursor		cursor = None;

    dd = (XmDisplay)XtParent(dc);
    dd->display.activeDC = dc;
    dd->display.userGrabbed = True;

    dc->drag.crossingTime = 
      dc->drag.dragStartTime = 
        dc->drag.lastChangeTime = event->xbutton.time;

	dc->drag.startX = dc->core.x = event->xbutton.x_root;
	dc->drag.startY = dc->core.y = event->xbutton.y_root;
    dc->drag.curDragOver = NULL;
    dc->drag.origDragOver = NULL;
    dc->drag.srcShell = GetShell(src);
    dc->drag.srcWindow = XtWindow(dc->drag.srcShell);
    dc->drag.iccHandle = _XmAllocMotifAtom((Widget)dc, dc->drag.dragStartTime);
    
    if (dc->drag.incremental) 
      XtOwnSelectionIncremental(dc->drag.srcShell, 
				dc->drag.iccHandle,
				dc->drag.dragStartTime,
				DropConvertIncrCallback,
				DropLoseIncrSelection,
				NULL, NULL, dc->drag.clientData);
    else
      XtOwnSelection(dc->drag.srcShell, 
		     dc->drag.iccHandle,
		     dc->drag.dragStartTime,
		     DropConvertCallback,
		     DropLoseSelection,
		     NULL);
    

     dc->drag.serverGrabbed = False;
     dc->drag.sourceIsExternal = False;

	dc->drag.activeProtocolStyle = activeProtocolStyle =
					_XmGetActiveProtocolStyle((Widget)dc);

	switch (dc->drag.activeProtocolStyle)
	{
		case XmDRAG_PREREGISTER:
			dc->drag.activeProtocolStyle = XmDRAG_DYNAMIC;
		case XmDRAG_DYNAMIC:
		break;
		case XmDRAG_DROP_ONLY:
			dc->drag.activeProtocolStyle = XmDRAG_NONE;
		case XmDRAG_NONE:
		break;
	}

	/* must be either DYNAMIC or NONE at this point */

    /*
     * start out with the default operations in effective operations
     */
    dc->drag.operations = dc->drag.dragOperations;
    dc->drag.lastEventState = state;
    if ((state & ShiftMask) && (state & ControlMask)) {
	dc->drag.operations = 
	  dc->drag.operation = (unsigned char) 
	    (XmDROP_LINK & dc->drag.dragOperations);
    }
    else if (state & ShiftMask) {
	dc->drag.operations =
	  dc->drag.operation =  (unsigned char)
	    (XmDROP_MOVE & dc->drag.dragOperations);
    }
    else if (state & ControlMask) {
	dc->drag.operations = 
	  dc->drag.operation = (unsigned char)
	    (XmDROP_COPY & dc->drag.dragOperations);
    }
#ifdef USE_MOD1_FOR_INTERRUPT
    else if (state & Mod1Mask)
      /* do something for interrupt */
#endif
    else if (XmDROP_MOVE &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_MOVE;
    else if (XmDROP_COPY &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_COPY;
    else if (XmDROP_LINK &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_LINK;
    else {
	dc->drag.operations = 
	  dc->drag.operation = 0;
    }

    dc->drag.sourceIsExternal = False;

    /*
     * if we're in query_pending mode then initialize the
     * currReceiverInfo to us
     */
    if (dc->drag.trackingMode == XmDRAG_TRACK_MOTION) {
	dc->drag.activeProtocolStyle = activeProtocolStyle;
#ifndef MULTI_SCREEN_DONE
      confineWindow = RootWindowOfScreen(XtScreen(dc));
#else
      confineWindow = None;
    } else { /* XmDRAG_TRACK_WM_QUERY */
	dc->drag.trackingMode = XmDRAG_TRACK_WM_QUERY_PENDING;
	confineWindow = XtWindow(dc->drag.srcShell);
    }
#endif /*  MULTI_SCREEN_DONE */

    if (dc->drag.trackingMode == XmDRAG_TRACK_WM_QUERY_PENDING &&
        activeProtocolStyle == XmDRAG_PREREGISTER) {
       dc->drag.blendModel = XmBLEND_NONE;
    }

    NewScreen(dc, RootWindowOfScreen(XtScreen(dc)));

    XtInsertEventHandler(dc->drag.srcShell, FocusChangeMask, True,
			 InitiatorMsgHandler, 
			 (XtPointer)dc,
			 XtListHead);

    mask = _XmDRAG_EVENT_MASK(dc);
    
#ifdef notdef
    /* 
     * need to use the actual dimensions of the grab cursor. The
     * warp will generate motion events (if selected) which could
     * confuse us. Luckily, we haven't selected for motion :-).
     * We'll get them in the next call.
     */
    XWarpPointer(XtDisplayOfObject((Widget) dc),
		 NULL,
		 RootWindowOfScreen(XtScreen(dc)),
		 0,0,0,0,
		 dc->core.x,
		 dc->core.y);
#endif /* notdef */    

    /*
     * we grab on the Xt pointer so that Xt doesn't interfere with us.
     * Also once we have the WM_QUERY capability this will work.
     * Otherwise we need to grab on the root so we can track the
     * changes in sub_window and infer the top_level crossings
     */
    /*
     * some more sleaze to get around the ungrab.  Since we can't rely
     * on the caller to have done an owner_events true (for Xt), we need to
     * guarantee that the release event goes to us. The grab window
     * must be viewable so we can't use the dc window
     */
	
       saveWindow = dc->core.window;

       cursor = _XmDragOverGetActiveCursor((Widget)dc->drag.curDragOver);

       dc->core.window = RootWindowOfScreen(XtScreen(dc));
       if ((XtGrabPointer((Widget)dc,
		       False,
		       (unsigned int) mask, 
		       GrabModeSync,
		       GrabModeAsync,
		       confineWindow,
		       cursor,
		       dc->drag.dragStartTime) != GrabSuccess) ||
	   (XGrabPointer(XtDisplayOfObject((Widget) dc),
		      RootWindowOfScreen(XtScreen(dc)),
		      False,
		      mask, 
		      GrabModeSync,
		      GrabModeAsync,
		      confineWindow,
		      cursor,
		      dc->drag.dragStartTime) != GrabSuccess)
	 ||
	   (XGrabKeyboard(XtDisplayOfObject((Widget) dc), 
		       RootWindowOfScreen(XtScreen(dc)),
		       False,
		       GrabModeSync,
		       GrabModeAsync,
		       dc->drag.dragStartTime) != GrabSuccess))

#ifdef I18N_MSG
	Warning(catgets(Xm_catd,MS_DragC,MSG_DRC_4, MESSAGE4));
#else
        Warning(MESSAGE4);
#endif

      _XmAddGrab((Widget)dc, True, False);
      dc->core.window = saveWindow;

    /* Begin fixing OSF CR 5556 */
    /*
     * Add ButtonMotionMask to the already set-up event mask for root window.
     * This gets reinstalled in DragContextDestroy()
     */
    {
      XWindowAttributes       xwa;

      XGetWindowAttributes(XtDisplay(dc), dc->drag.currWmRoot, &xwa);
      XSelectInput(XtDisplay(dc),
                   dc->drag.currWmRoot,
                   xwa.your_event_mask | ButtonMotionMask);
      SaveEventMask = xwa.your_event_mask;
    }
    /* End fixing OSF CR 5556 */

 
    XSelectInput(XtDisplay(dc), 
		 dc->drag.currWmRoot,
		 ButtonMotionMask);

    if (dc->drag.trackingMode == XmDRAG_TRACK_WM_QUERY_PENDING) {
	Atom	wmQuery;
	Atom	wmAllClients;

	wmQuery = XmInternAtom(XtDisplay(dc),
			       "_MOTIF_WM_QUERY",
			       False);
	wmAllClients = XmInternAtom(XtDisplay(dc),
				    "_MOTIF_WM_ALL_CLIENTS",
				    False);
	XtGetSelectionValue((Widget)dd,
			    wmQuery,
			    wmAllClients,
			    TopWindowsReceived, 
			    (XtPointer)dc,
			    dc->drag.dragStartTime);

	XAllowEvents(XtDisplayOfObject((Widget) dc->drag.srcShell),
		     SyncPointer,
		     dc->drag.dragStartTime);
    }
    else 
      DragStartWithTracking(dc);

    XSync(XtDisplay(dc), False);
 
    XtAppAddTimeOut( XtWidgetToApplicationContext( (Widget)dc),
		    0, InitiatorMainLoop,
		    (XtPointer)(&dd->display.activeDC));
    return;
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DragStartWithTracking( dc)
        XmDragContext dc ;
#else
DragStartWithTracking(
        XmDragContext dc)
#endif /* _NO_PROTO */
{
    if (dc->drag.dragFinishTime)
      return;

    if (dc->drag.trackingMode == XmDRAG_TRACK_WM_QUERY) {
	EventMask	mask = _XmDRAG_EVENT_MASK(dc);
	Window		confineWindow;
	Cursor		cursor;

#ifndef MULTI_SCREEN_DONE
	confineWindow = RootWindowOfScreen(XtScreen(dc));
#else
	confineWindow = None;
#endif
	cursor = _XmDragOverGetActiveCursor((Widget)dc->drag.curDragOver);

	/*
	 * set owner events to true so that the crossings and motion
	 * are reported relative to the destination windows. Don't
	 * tell Xt about it so we can continue to get everything
	 * reported to the dragContext via the spring-loaded/XtGrab
	 * interaction. Blegh
	 */
	if (XGrabPointer(XtDisplayOfObject((Widget) dc),
			 RootWindowOfScreen(XtScreen(dc)),
			 True,
			 mask,
			 GrabModeSync,
			 GrabModeAsync,
			 confineWindow,
			 cursor,
			 dc->drag.lastChangeTime) != GrabSuccess)

#ifdef I18N_MSG
	  Warning(catgets(Xm_catd,MS_DragC,MSG_DRC_4, MESSAGE4));
#else
	  Warning(MESSAGE4);
#endif

    }

    XAllowEvents(XtDisplayOfObject((Widget) dc->drag.srcShell),
		 SyncPointer,
		 dc->drag.lastChangeTime);
}

static void 
#ifdef _NO_PROTO
UpdateMotionBuffer( dc, mb, event)
        XmDragContext dc;
        MotionBuffer mb ;
        XEvent	     *event;
#else
UpdateMotionBuffer(
	XmDragContext dc,
        MotionBuffer mb,
	XEvent	     *event)
#endif /* _NO_PROTO */
{
    Time time ;
    unsigned int state;
    Window window, subwindow;
    Position x ;
    Position y ;

    if (dc->drag.currReceiverInfo == NULL)
      return;

    dc->drag.lastChangeTime = event->xmotion.time;

    /*
     * we munged the window and subwindow fields before calling
     * XtDispatchEvent so we could get the event thru to the
     * DragContext.  The subwindow field will hold the interesting
     * info. The window field is always set (by us) to the DC window.
     */
    switch(event->type) {
      case MotionNotify:
	if (mb->count && ((mb->count % (STACKMOTIONBUFFERSIZE)) == 0)) {
	    if (mb->count == (STACKMOTIONBUFFERSIZE)){
		MotionBuffer	oldMb = mb;
		Cardinal size;
		
		size = sizeof(MotionBufferRec) +
		  (STACKMOTIONBUFFERSIZE * sizeof(MotionEntryRec));
		mb = (MotionBuffer) XtMalloc(size);
		memcpy((char *)mb, (char *)oldMb, sizeof(MotionBufferRec));
	    }
	    else  {
		mb = (MotionBuffer)
		  XtRealloc((char *)mb, 
			    (sizeof(MotionBufferRec) +
			     (mb->count + STACKMOTIONBUFFERSIZE) *sizeof(MotionEntryRec)));
	    }
	}
	/*	
	 * for right now use the root although this wont work for
	 * pseudo-roots
	 */
	time = event->xmotion.time;
	state = event->xmotion.state;
	x = event->xmotion.x_root;
	y = event->xmotion.y_root;
	window = event->xmotion.root;
	if (dc->drag.trackingMode != XmDRAG_TRACK_MOTION) {
	    subwindow = mb->currReceiverInfo->window;
	}
	else {
	    subwindow = event->xmotion.subwindow;
	}
	mb->entries[mb->count].time = time;
	mb->entries[mb->count].window = window;
	mb->entries[mb->count].subwindow = subwindow;
	mb->entries[mb->count].state = state;
	mb->entries[mb->count].x = x;
	mb->entries[mb->count++].y = y;
	break;

      case EnterNotify:
	if ((event->xcrossing.mode == NotifyNormal) &&
	    (dc->drag.trackingMode != XmDRAG_TRACK_MOTION)) {
	    XmDragReceiverInfo	rInfo;
	    if ((rInfo = FindReceiverInfo(dc, event->xcrossing.subwindow))
                != NULL)
	      mb->currReceiverInfo = rInfo;
	}
	break;

      case LeaveNotify:
	if ((event->xcrossing.mode == NotifyNormal) &&
	    (dc->drag.trackingMode != XmDRAG_TRACK_MOTION)) {
	    XmDragReceiverInfo	rInfo;
	    if ((rInfo = FindReceiverInfo(dc, event->xcrossing.subwindow)) 
		!= NULL) {
		if (rInfo == mb->currReceiverInfo)
		  mb->currReceiverInfo = dc->drag.rootReceiverInfo;
	    }
	}
	break;
      default:
	break;
    }
}
   

static void 
#ifdef _NO_PROTO
DragMotionProto( dc, root, subWindow )
        XmDragContext dc ;
        Window root;
        Window	subWindow;
#else
DragMotionProto(
        XmDragContext dc,
        Window root,
        Window subWindow )
#endif /* _NO_PROTO */
{
	Boolean incrementTimestamp = False;

	/*
	* We've selected for motion on the root window. This allows us to
	* use the subwindow field to know whenever we have left or
	* entered a potential top-level window.
	*/
	if ((root != dc->drag.currWmRoot)
		||
		((((dc->drag.trackingMode == XmDRAG_TRACK_MOTION) &&
		(dc->drag.currReceiverInfo->frame != subWindow)))
			||
		((dc->drag.trackingMode == XmDRAG_TRACK_WM_QUERY) &&
		(dc->drag.currReceiverInfo->window != subWindow))))
	{
		if (dc->drag.currReceiverInfo->window)
		{
			if (dc->drag.activeProtocolStyle != XmDRAG_NONE)
			{
				/*
				* Assumes the root window doesn't contain drop-sites !!!
				* Send motion to old window.
				*
				* ** Old stuff for bootstrap
				* ** if (dc->drag.currReceiverInfo->frame) 
				* ** only send to non-initial windows 
				*/
				/* 
				* if the receiver is dynamic and we've been
				* informed that we're in a drop-site then
				* generate a dropsite leave to the initiator. If
				* we haven't yet been informed of a drop-site
				* enter (due to latency) but one is in the pipes,
				* it will be ignored once it gets in based on the
				* timestamp being stale.
				*
				* We'll make sure it's stale by incrementing the
				* timestamp by one millisecond.  This is a no-no but
				* it makes it easy to identify the echo events from
				* the receiver.  Its also relatively safe until we
				* get micro-second response times :-)
				*/
				if ((dc->drag.activeProtocolStyle == XmDRAG_DYNAMIC) &&
					(!dc->drag.currReceiverInfo->shell) &&
					(dc->drag.inDropSite))
				{
					GenerateClientCallback(dc, XmCR_DROP_SITE_LEAVE);
					dc->drag.inDropSite = False;
					incrementTimestamp = True;
				}
				SendDragMessage(dc, dc->drag.currReceiverInfo->window,
					XmDRAG_MOTION);
				SendDragMessage(dc, dc->drag.currReceiverInfo->window,
					XmTOP_LEVEL_LEAVE);
			}
			GenerateClientCallback(dc, XmCR_TOP_LEVEL_LEAVE);
		}

		if (root != dc->drag.currWmRoot)
			NewScreen(dc, root);

		GetDestinationInfo(dc, root, subWindow);

		/* we should special-case the root window */
		if (dc->drag.currReceiverInfo->window)
		{
			if (dc->drag.activeProtocolStyle != XmDRAG_NONE)
			{
				SendDragMessage(dc, dc->drag.currReceiverInfo->window,
					XmTOP_LEVEL_ENTER);
			}
			/* clear iccInfo for dsm's sanity */
			dc->drag.currReceiverInfo->iccInfo = NULL;
			GenerateClientCallback(dc, XmCR_TOP_LEVEL_ENTER);
		}
	}
	if (dc->drag.currReceiverInfo->window)
	{
		if (dc->drag.activeProtocolStyle != XmDRAG_NONE)
			SendDragMessage(dc, dc->drag.currReceiverInfo->window,
				XmDRAG_MOTION);
                else
		        GenerateClientCallback(dc, XmCR_DRAG_MOTION);
	}
	else
		GenerateClientCallback(dc, XmCR_DRAG_MOTION);
	if (incrementTimestamp)
		dc->drag.lastChangeTime++;
}

static void 
#ifdef _NO_PROTO
ProcessMotionBuffer( dc, mb )
        XmDragContext dc ;
        MotionBuffer mb ;
#else
ProcessMotionBuffer(
        XmDragContext dc,
        MotionBuffer mb )
#endif /* _NO_PROTO */
{
    Cardinal	incr, i, j, max;
    Window	protoWindow = None;

    incr = (mb->count / MOTIONFILTER);
    if (incr == 0) incr = 1;
    max = mb->count / incr;
    j = (mb->count + incr - 1) % incr;
    for (i = 0; i < max; i++, j += incr) {
	dc->core.x = mb->entries[j].x;
	dc->core.y = mb->entries[j].y;

	if (mb->entries[j].state != dc->drag.lastEventState)
	  CheckModifiers(dc, mb->entries[j].state);
	if (dc->drag.currWmRoot != mb->entries[j].window) {
	    /*
	     * cause the callbacks to get called so the client can
	     * change the dragOver visuals
	     */
	    DragMotionProto(dc, mb->entries[j].window, None);
	    protoWindow = None;
	}
	else 
	  protoWindow = mb->entries[j].subwindow;

    }
    _XmDragOverMove((Widget)dc->drag.curDragOver, dc->core.x, dc->core.y);
    /*
     * actually inform the receiver/initiator that movement has
     * occurred
     */
    DragMotionProto(dc, dc->drag.currWmRoot, protoWindow);

    if (mb->count > STACKMOTIONBUFFERSIZE)
      XtFree( (char *)mb);
}

#define IsGrabEvent(ev) \
  ((ev->type == ButtonPress) ||\
   (ev->type == ButtonRelease) ||\
   (ev->type == KeyPress) ||\
   (ev->type == KeyRelease))

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DragMotion( w, event, params, numParams )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *numParams ;
#else
DragMotion(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *numParams )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)w;
    Window		currRoot = dc->drag.currWmRoot;
    Window 		child;
    int 		root_x, root_y, win_x, win_y;
    unsigned int 	modMask;
    MotionBufferRec	stackBuffer;
    MotionBuffer	motionBuffer = &stackBuffer;
    Boolean		grabEvent = False;

    stackBuffer.count = 0;
    stackBuffer.currReceiverInfo = dc->drag.currReceiverInfo;
    UpdateMotionBuffer(dc, motionBuffer,event);
    /* 
     * get rid of any pending events
     */
    while (!grabEvent &&
	   XCheckMaskEvent(XtDisplayOfObject((Widget) w),
			   _XmDRAG_EVENT_MASK(dc),
			   event)) {
	grabEvent = IsGrabEvent(event);
	if (!grabEvent) {
	    if (dc->drag.trackingMode != XmDRAG_TRACK_MOTION)
	      event->xmotion.subwindow = event->xmotion.window;
	    UpdateMotionBuffer(dc, motionBuffer, event);
	}
	else
	  XPutBackEvent(XtDisplay(dc), event);
    }

    /* optimize the motion events */
    (void) XQueryPointer(XtDisplayOfObject((Widget) w), currRoot,
			 &currRoot, &child,
			 &root_x, &root_y,
			 &win_x, &win_y, &modMask);
    /* 
     * We need to process all outstanding events of interest
     * inline and flush out any stale motion events.  Need to
     * check for state change events like modifier or button press.
     */
    
    /*
     * get any that came in after
     */
    while (!grabEvent &&
	   XCheckMaskEvent(XtDisplayOfObject((Widget) w),
			   _XmDRAG_EVENT_MASK(dc),
			   event)) {
	grabEvent = IsGrabEvent(event);
	if (!grabEvent) {
	    if (dc->drag.trackingMode != XmDRAG_TRACK_MOTION)
	      event->xmotion.subwindow = event->xmotion.window;
	    UpdateMotionBuffer(dc, motionBuffer, event);
	}
	else
	  XPutBackEvent(XtDisplay(dc), event);
    }
    ProcessMotionBuffer(dc, motionBuffer);
    XFlush(XtDisplayOfObject((Widget) dc));
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DropStartTimeout( clientData, id )
        XtPointer clientData ;
        XtIntervalId *id ;
#else
DropStartTimeout(
        XtPointer clientData,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{

    XmDragContext	dc = (XmDragContext)clientData;
    XmDropStartCallbackStruct	callbackRec, *callback = &callbackRec;
    XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay)(XtParent(dc)));

    if (dc->drag.dropStartCallback) {
	callback->reason = XmCR_DROP_START;
	callback->event = NULL;
	callback->timeStamp = dc->drag.dragFinishTime;
	callback->operation = dc->drag.operation;
	callback->operations = dc->drag.operations;
	callback->dropAction = XmDROP_CANCEL;
	callback->dropSiteStatus = dsm->dropManager.curDropSiteStatus;
	callback->x = dc->core.x;
	callback->y = dc->core.y;
	callback->iccHandle = dc->drag.iccHandle;
	callback->window = XtWindow(dc->drag.srcShell);
	XtCallCallbackList((Widget)dc, dc->drag.dropStartCallback, callback);
	dc->drag.dragCompletionStatus = callback->dropAction;
        dsm->dropManager.curDropSiteStatus = callback->dropSiteStatus;
    }
    dc->drag.dragDropCompletionStatus = XmDROP_FAILURE;
    dc->drag.dropFinishTime = dc->drag.dragFinishTime;

    DragDropFinish(dc);
}    
 
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
FinishAction( dc, ev )
        XmDragContext dc ;
        XEvent *ev ;
#else
FinishAction(
        XmDragContext dc,
        XEvent *ev )
#endif /* _NO_PROTO */
{
    unsigned int	state = 0;
    Arg			args[4];
    Cardinal		i = 0;
    XmDisplay		dd = (XmDisplay) XmGetXmDisplay(XtDisplay(dc));

    dd->display.activeDC = NULL;
    dd->display.userGrabbed = False;

    if (ev) {
	switch(ev->type) {
	  case ButtonRelease:
	    state = ev->xbutton.state;
	    dc->drag.lastChangeTime = ev->xbutton.time;
	    dc->core.x = ev->xbutton.x_root;
	    dc->core.y = ev->xbutton.y_root;
	    break;
	  case KeyPress:
	    state = ev->xkey.state;
	    dc->drag.lastChangeTime = ev->xkey.time;
	    dc->core.x = ev->xkey.x_root;
	    dc->core.y = ev->xkey.y_root;
	    break;
	}
	
	/*
	 * start out with the default operations in effective operations
	 */
	dc->drag.operations = dc->drag.dragOperations;
	dc->drag.lastEventState = state;
	if ((state & ShiftMask) && (state & ControlMask)) {
	    dc->drag.operations = 
	      dc->drag.operation = (unsigned char) 
		(XmDROP_LINK & dc->drag.dragOperations);
	}
	else if (state & ShiftMask) {
	    dc->drag.operations =
	      dc->drag.operation =  (unsigned char)
		(XmDROP_MOVE & dc->drag.dragOperations);
	}
	else if (state & ControlMask) {
	    dc->drag.operations = 
	      dc->drag.operation = (unsigned char)
		(XmDROP_COPY & dc->drag.dragOperations);
	}
#ifdef USE_MOD1_FOR_INTERRUPT
	else if (state & Mod1Mask)
	  /* do something for interrupt */
#endif
	else if (XmDROP_MOVE &dc->drag.dragOperations)
	  dc->drag.operation = (unsigned char)XmDROP_MOVE;
	else if (XmDROP_COPY &dc->drag.dragOperations)
	  dc->drag.operation = (unsigned char)XmDROP_COPY;
	else if (XmDROP_LINK &dc->drag.dragOperations)
	  dc->drag.operation = (unsigned char)XmDROP_LINK;
	else {
	    dc->drag.operations = 
	      dc->drag.operation = 0;
	}
    }
      
    /*
     * change the dragOver to a window in so it can persist after the
     * ungrab 
     */
    if (dc->drag.curDragOver) {
       i = 0;
       XtSetArg(args[i], XmNhotX, dc->core.x); i++;
       XtSetArg(args[i], XmNhotY, dc->core.y); i++;
       XtSetArg(args[i], XmNdragOverMode, XmWINDOW); i++;
       XtSetValues((Widget) dc->drag.curDragOver, args, i);

       XUngrabPointer(XtDisplayOfObject((Widget) dc), dc->drag.lastChangeTime);
       XtUngrabPointer((Widget) dc, dc->drag.dragFinishTime);
       XUngrabKeyboard(XtDisplayOfObject((Widget) dc), dc->drag.lastChangeTime);

	_XmRemoveGrab((Widget)dc);
    }

    if (dc->drag.serverGrabbed)
      XUngrabServer(XtDisplay((Widget) dc));
    /*
     * if we're in pre-register with a non-local raeceiver then we need
     * to flush a top-level leave to the local dsm. 
     */
    dc->drag.dragFinishTime = dc->drag.lastChangeTime;
	    
    if (dc->drag.inDropSite) {
	GenerateClientCallback(dc, XmCR_DROP_SITE_LEAVE);
	dc->drag.inDropSite = False;
    }

    if (dc->drag.currReceiverInfo) {
       if (dc->drag.currReceiverInfo->window) {
	  SendDragMessage(dc, dc->drag.currReceiverInfo->window,
			  XmTOP_LEVEL_LEAVE);
	  GenerateClientCallback(dc, XmCR_TOP_LEVEL_LEAVE);

	  if ((dc->drag.activeProtocolStyle != XmDRAG_NONE) &&
	      ((dc->drag.dragCompletionStatus == XmDROP) ||
	       (dc->drag.dragCompletionStatus == XmDROP_HELP))) {
	    
	      XtAppContext appContext= XtWidgetToApplicationContext((Widget)dc);
	      /*
	       * we send the leave message in the dragDropFinish so
	       * that a non-local receiver can cleanup its dc
	       */
	      dc->drag.dragTimerId = 
	        XtAppAddTimeOut(appContext,
			        XtAppGetSelectionTimeout(appContext),
			        DropStartTimeout,
			        (XtPointer)dc);
	      SendDragMessage(dc, dc->drag.currReceiverInfo->window,
			      XmDROP_START);
	  }
	  else {
	      dc->drag.dragDropCompletionStatus = XmDROP_FAILURE;
	      dc->drag.dropFinishTime = dc->drag.dragFinishTime;
	      DropStartTimeout((XtPointer)dc, NULL);
	  }
       }
       dc->drag.currReceiverInfo->frame = 0;
    }
}



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CheckModifiers( dc, state)
    XmDragContext dc;
    unsigned int state;
#else
CheckModifiers(
	       XmDragContext	dc,
	       unsigned int	state)
#endif /* _NO_PROTO */
{
    unsigned char	oldOperation = dc->drag.operation;
    unsigned char	oldOperations = dc->drag.operations;

    dc->drag.lastEventState = state;
    dc->drag.operations = dc->drag.dragOperations;
    if ((state & ShiftMask) && (state & ControlMask)) {
	dc->drag.operations = 
	  dc->drag.operation =  (unsigned char)
	    (XmDROP_LINK & dc->drag.dragOperations);
    }
    else if (state & ShiftMask) {
	dc->drag.operations =
	  dc->drag.operation =  (unsigned char)
	    (XmDROP_MOVE & dc->drag.dragOperations);
    }
    else if (state & ControlMask) {
	dc->drag.operations = 
	  dc->drag.operation = (unsigned char)
	    (XmDROP_COPY & dc->drag.dragOperations);
    }
#ifdef USE_MOD1_FOR_INTERRUPT
    else if (state & Mod1Mask)
      /* do something for interrupt */
#endif
    else if (XmDROP_MOVE &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_MOVE;
    else if (XmDROP_COPY &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_COPY;
    else if (XmDROP_LINK &dc->drag.dragOperations)
      dc->drag.operation = (unsigned char)XmDROP_LINK;
    else {
	dc->drag.operations = 
	  dc->drag.operation = 0;
    }
    if ((oldOperations != dc->drag.operations) ||
	(oldOperation != dc->drag.operation)) {
	if ((dc->drag.currReceiverInfo->window) &&
	    (dc->drag.activeProtocolStyle != XmDRAG_NONE) &&
	    (dc->drag.activeProtocolStyle != XmDRAG_DROP_ONLY)) {
	    SendDragMessage(dc, 
			    dc->drag.currReceiverInfo->window,
			    XmOPERATION_CHANGED);
	}
	else {
	    GenerateClientCallback(dc, XmCR_OPERATION_CHANGED);
	}
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
IgnoreButtons( w, event, params, numParams )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *numParams ;
#else
IgnoreButtons(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *numParams )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)w;

    /*
     * the user has pressed some other buttons and caused the server
     * to synch up. Swallow and continue
     */

    XAllowEvents(XtDisplayOfObject((Widget) dc->drag.srcShell),
		 SyncPointer,
		 dc->drag.lastChangeTime);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CancelDrag( w, event, params, numParams )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *numParams ;
#else
CancelDrag(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *numParams )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)w;

    /* 
     * only cancel if drag has not yet completed
     */
    if (dc->drag.dragFinishTime == 0) {
	dc->drag.dragCompletionStatus = XmDROP_CANCEL;
	FinishAction(dc, event);
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HelpDrag( w, event, params, numParams )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *numParams ;
#else
HelpDrag(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *numParams )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)w;

    dc->drag.dragCompletionStatus = XmDROP_HELP;
    FinishAction(dc, event);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
FinishDrag( w, event, params, numParams )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *numParams ;
#else
FinishDrag(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *numParams )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)w;

    dc->drag.dragCompletionStatus = XmDROP;
    FinishAction(dc, event);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
InitiatorMainLoop( clientData, id )
        XtPointer clientData ;
        XtIntervalId *id ;
#else
InitiatorMainLoop(
        XtPointer clientData,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
    XmDragContext 	*activeDC = (XmDragContext *)clientData;
    XtAppContext	appContext;
    XEvent		event;
    Widget		focusWidget, shell;


    if (*activeDC) {
	appContext = XtWidgetToApplicationContext((Widget) *activeDC);
        shell = (*activeDC)->drag.srcShell;
        focusWidget = XmGetFocusWidget((Widget)((*activeDC)->drag.srcShell));
        if (_XmGetFocusPolicy(shell) == XmEXPLICIT) {
	   XtSetKeyboardFocus(shell, None);
        } else {
           XmFocusData         focusData = _XmGetFocusData(shell);
           focusData->needToFlush = False;
	   if (XmIsPrimitive(focusWidget)) {
              if (((XmPrimitiveWidgetClass) XtClass(focusWidget))
				        ->primitive_class.border_unhighlight)
		 (*(((XmPrimitiveWidgetClass) XtClass(focusWidget))
                            ->primitive_class.border_unhighlight))(focusWidget);
           } else if (XmIsGadget(focusWidget)) {
              if (((XmGadgetClass) XtClass(focusWidget))
					->gadget_class.border_unhighlight)
                 (*(((XmGadgetClass) XtClass(focusWidget))
			    ->gadget_class.border_unhighlight))(focusWidget);
           }
        }
	DragStartProto(*activeDC);
    }
    else return;
      
    while (*activeDC) {
	XmDragContext dc = *activeDC;
	XtAppNextEvent(appContext, &event);
	/*
	 * make sure evil Focus outs don't confuse Xt and cause the
	 * unhighlighting of the source hierarchy.
	 */
	switch(event.type) {
	  case FocusIn:
	  case FocusOut:
	    break;
	  case KeyPress:
	  case KeyRelease:
	  case ButtonPress:
	  case ButtonRelease:
	  case EnterNotify:
	  case LeaveNotify:
	  case MotionNotify:
	    {
	      /* dispatch it onto the dc */
		switch(dc->drag.trackingMode) {
		  case XmDRAG_TRACK_MOTION:
		    break;
#ifdef MULTI_SCREEN_DONE
		  case XmDRAG_TRACK_WM_QUERY:
		    event.xmotion.subwindow = event.xmotion.window;
		    break;
#endif /*  MULTI_SCREEN_DONE */
		  case XmDRAG_TRACK_WM_QUERY_PENDING:
		    event.xmotion.subwindow = event.xmotion.window;
		    break;
		} 
	    }
	    event.xmotion.window = XtWindow(dc);
	    break;
	}

        if ((event.type == MotionNotify ||
             event.type == LeaveNotify ||
             event.type == EnterNotify) &&
	     event.xmotion.state == dc->drag.lastEventState)
           DragMotion((Widget)dc, &event, NULL, 0);
        else
	   XtDispatchEvent(&event);
    }
    if (_XmGetFocusPolicy(shell) == XmEXPLICIT) {
       XtSetKeyboardFocus(shell, focusWidget);
    }
}



Widget
#ifdef _NO_PROTO
XmDragStart( w, event, args, numArgs )
        Widget w ;
        XEvent *event ;
        ArgList args ;
        Cardinal numArgs ;
#else
XmDragStart(
        Widget w,
        XEvent *event,
        ArgList args,
        Cardinal numArgs )
#endif /* _NO_PROTO */
{
    XmDragContext	dc;
    XmDisplay		dd = (XmDisplay) XmGetXmDisplay(XtDisplay(w));
    Arg			lclArgs[1];
    Arg			*mergedArgs;

	if (dd->display.dragInitiatorProtocolStyle == XmDRAG_NONE)
		return(NULL);

    if (event->type != ButtonPress ) {

#ifdef I18N_MSG
        _XmWarning(w, catgets(Xm_catd,MS_DragC,MSG_DRC_6,
                MESSAGE6));
#else
	_XmWarning(w, MESSAGE6);
#endif

	return NULL;
    }
    /*
     * check if the menu system is already active
     */
    if (dd->display.userGrabbed) {
#ifdef DEBUG
	Warning("can't drag; menu system active\n");
#endif	
	return NULL;
    }

    XtSetArg(lclArgs[0], XmNsourceWidget, w);

    if (numArgs) {
	mergedArgs = XtMergeArgLists(args, numArgs, lclArgs, 1);
    }
    else {
	mergedArgs = lclArgs;
    }
    dc = (XmDragContext)
      XtCreateWidget("dragContext", xmDragContextClass,
		     (Widget) dd, mergedArgs, numArgs + 1);
    _XmDragStart(dc, w, event);

    if (numArgs)
      XtFree( (char *)mergedArgs);
    return (Widget)dc;
}


static void 
#ifdef _NO_PROTO
DragCancel( dc )
        XmDragContext dc ;
#else
DragCancel(
        XmDragContext dc )
#endif /* _NO_PROTO */
{
    dc->drag.dragCompletionStatus = XmDROP_CANCEL;
    FinishAction(dc, NULL);
}

void 
#ifdef _NO_PROTO
XmDragCancel( dragContext )
        Widget dragContext ;
#else
XmDragCancel(
        Widget dragContext )
#endif /* _NO_PROTO */
{
    DragCancel((XmDragContext)dragContext);
}


/*ARGSUSED*/
Boolean 
#ifdef _NO_PROTO
XmTargetsAreCompatible( dpy, exportTargets, numExportTargets, importTargets, numImportTargets )
        Display *dpy ;
        Atom *exportTargets ;
        Cardinal numExportTargets ;
        Atom *importTargets ;
        Cardinal numImportTargets ;
#else
XmTargetsAreCompatible(
        Display *dpy,
        Atom *exportTargets,
        Cardinal numExportTargets,
        Atom *importTargets,
        Cardinal numImportTargets )
#endif /* _NO_PROTO */
{
    Cardinal		j, k;

    for (j = 0; j < numExportTargets; j++)
      for (k = 0; k < numImportTargets; k++)
	if (exportTargets[j] == importTargets[k])
	  return True;
    return False;
}


unsigned char 
#ifdef _NO_PROTO
_XmGetActiveProtocolStyle( w )
        Widget w ;
#else
_XmGetActiveProtocolStyle(
        Widget w )
#endif /* _NO_PROTO */
{
	unsigned char initiator = XmDRAG_NONE,
	receiver = XmDRAG_NONE,
	active = XmDRAG_NONE;
	XmDragContext	dc = (XmDragContext)w;
	XmDisplay		xmDisplay = (XmDisplay)XtParent(dc);

	initiator = xmDisplay->display.dragInitiatorProtocolStyle;
	receiver = xmDisplay->display.dragReceiverProtocolStyle;

	if (!dc->drag.sourceIsExternal)
	{
		/* We are the initiator.  Find the receiver. */
		if (dc->drag.currReceiverInfo)
		{
			receiver = dc->drag.currReceiverInfo->dragProtocolStyle;
		}
		else
		{
			/*
			 * This function is sometimes called before we have
			 * set up the receiver info struct on the dc.  In these
			 * cases, we are still in the initiating client, and so
			 * we will use the initiating client's receiver protocol
			 * style.  In order to do this we have to emulate the 
			 * protocol style decision made in GetDestinationInfo.
			 * But we can't, since we don't really know the shell
			 * widget--the particular shell widget may not have a dnd
			 * propterty attached to it.
			 *
			 * There are a number of different guesses that we could
			 * make.  None of them is substantially better than
			 * not guessing at all.  Not guessing only messes up if the
			 * shell really doesn't have any drop sites--we should be
			 * returning NONE, but we might return something else.
			 */
		}

		active = protocolMatrix[initiator][receiver];
	}
	else
	{
		/* We are the receiver, so we can't be preregister.  (Since
		 * the receiver doesn't hear about the drag during the drag,
		 * and so get ACTIVE protocol makes no sense.)
		 */
		switch(receiver)
		{
			case XmDRAG_NONE:
				active = XmDRAG_NONE;
			break;
			case XmDRAG_DROP_ONLY:
				/* this must be post drop emulation of drag */
			case XmDRAG_PREREGISTER:
			case XmDRAG_DYNAMIC:
			case XmDRAG_PREFER_DYNAMIC:
			case XmDRAG_PREFER_PREREGISTER:
			case XmDRAG_PREFER_RECEIVER:
				active = XmDRAG_DYNAMIC;
			break;
		}
	}
	return active;
}
