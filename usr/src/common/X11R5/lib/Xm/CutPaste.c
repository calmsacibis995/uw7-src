#pragma ident	"@(#)m1.2libs:Xm/CutPaste.c	1.2"
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

#define CUTPASTE

#include <Xm/CutPasteP.h>
#include <Xm/AtomMgr.h>
#include "MessagesI.h"
#include <string.h>
#include <stdio.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifndef MIN
#define MIN(x,y)	((x) < (y) ? (x) : (y))
#endif /* MIN */

#ifndef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif /* MAX */

#define XMERROR(key, message)                                            \
    XtErrorMsg (key, "xmClipboardError", "XmToolkitError", message, NULL, NULL)

#define CLIPBOARD ( XmInternAtom( display, "CLIPBOARD", False ))
#define CLIP_TEMP ( XmInternAtom( display, "CLIP_TEMPORARY", False ))
#define CLIP_INCR ( XmInternAtom( display, "INCR", False ))
#define TARGETS   ( XmInternAtom( display, "TARGETS", False ))
#define LENGTH    ( XmInternAtom( display, "LENGTH", False ))
#define TIMESTAMP ( XmInternAtom( display, "TIMESTAMP", False ))
#define MULTIPLE ( XmInternAtom( display, "MULTIPLE", False ))

#define XMRETRY 3
#define XM_APPEND 0
#define XM_REPLACE 1
#define XM_PREPEND 2

#define XM_HEADER_ID 0
#define XM_NEXT_ID   1
#define XM_LOCK_ID   2

#define XM_FIRST_FREE_ID 1000	/* First Item Id allocated */
#define XM_ITEM_ID_INC 1000	/* Increase in Item Id between each copy */
#define XM_ITEM_ID_MAX 5000	/* 'Safe' threshold for resetting Item Id */

#define XM_FORMAT_HEADER_TYPE 1
#define XM_DATA_ITEM_RECORD_TYPE 2
#define XM_HEADER_RECORD_TYPE 3

#define XM_UNDELETE 0
#define XM_DELETE 1

#define XM_DATA_REQUEST_MESSAGE 0
#define XM_DATA_DELETE_MESSAGE  1


#ifdef I18N_MSG
#define XM_CLIPBOARD_MESSAGE1	catgets(Xm_catd,MS_CPaste,MSG_CP_1,\
					_XmMsgCutPaste_0000)
#define XM_CLIPBOARD_MESSAGE2	catgets(Xm_catd,MS_CPaste,MSG_CP_2,\
					_XmMsgCutPaste_0001)
#define XM_CLIPBOARD_MESSAGE3	catgets(Xm_catd,MS_CPaste,MSG_CP_3,\
					_XmMsgCutPaste_0002)
#else
#define XM_CLIPBOARD_MESSAGE1	_XmMsgCutPaste_0000
#define XM_CLIPBOARD_MESSAGE2	_XmMsgCutPaste_0001
#define XM_CLIPBOARD_MESSAGE3	_XmMsgCutPaste_0002
#endif

#define CLIPBOARD_BAD_DATA_TYPE	_XmMsgCutPaste_0003
#define BAD_DATA_TYPE		_XmMsgCutPaste_0004
#define CLIPBOARD_CORRUPT	_XmMsgCutPaste_0005
#define CORRUPT_DATA_STRUCTURE	_XmMsgCutPaste_0006
#define CLIPBOARD_BAD_FORMAT	_XmMsgCutPaste_0007
#define BAD_FORMAT		_XmMsgCutPaste_0008
#define BAD_FORMAT_NON_NULL	_XmMsgCutPaste_0009



#define XM_STRING	 8
#define XM_COMPOUND_TEXT 8
#define XM_ATOM	32
#define XM_ATOM_PAIR	32 
#define XM_BITMAP	32
#define XM_PIXMAP	32
#define XM_DRAWABLE	32
#define XM_SPAN	32
#define XM_INTEGER	32
#define XM_WINDOW	32
#define XM_PIXEL	32
#define XM_COLORMAP	32
#define XM_TEXT	 8

#define MAX_SELECTION_INCR(dpy) (((65536 < XMaxRequestSize(dpy)) ? \
      (65536 << 2)  : (XMaxRequestSize(dpy) << 2))-100)

#define BYTELENGTH( length, format ) \
	((format ==8) ? length : \
	((format == 16) ? length * sizeof(unsigned short) : \
	length * sizeof(unsigned long))) 

#define CONVERT_32_FACTOR (sizeof(unsigned long) / 4)

#define CleanupHeader(display) XDeleteProperty (display, \
				XDefaultRootWindow (display), \
				XmInternAtom (display, "_MOTIF_CLIP_HEADER", \
				False));
typedef long itemId;

typedef struct {
   Atom target;
   Atom property;
 } IndirectPair;

/*-----------------------------------------------------*/
/*   Define the clipboard selection information record */
/*-----------------------------------------------------*/

typedef struct _ClipboardSelectionInfo {

    Display *display;
    Window window;
    Window selection_window;
    Time time;
    char *format;
    unsigned long count;
    char *data;
    Atom type;
    Boolean incremental;
    Boolean selection_notify_received;

} ClipboardSelectionInfoRec, *ClipboardSelectionInfo;

/*------------------------------------------------------------*/
/*   Define the clipboard property destroy information record */
/*------------------------------------------------------------*/

typedef struct _ClipboardDestroyInfo {

    Display *display;
    Window  window;
    Atom property;

} ClipboardDestroyInfoRec, *ClipboardDestroyInfo;

/*-------------------------------------------------------*/
/*   Define the clipboard cut by name information record */
/*-------------------------------------------------------*/

typedef struct _ClipboardCutByNameInfo {

    Window window;
    itemId formatitemid;

} ClipboardCutByNameInfoRec, *ClipboardCutByNameInfo;

/*---------------------------------------------------*/
/*   Define the clipboard format registration record */
/*---------------------------------------------------*/

typedef struct _ClipboardFormatRegRec {

    int formatLength;

} ClipboardFormatRegRec, *ClipboardFormatRegPtr;

/*-------------------------------------------*/
/*   Define the clipboard lock record        */
/*-------------------------------------------*/

typedef struct _ClipboardLockRec {

    Window windowId;
    int lockLevel;

} ClipboardLockRec, *ClipboardLockPtr;

/* All items in the next three structures should have the same size as	*/
/* an unsigned long.							*/

/*---------------------------------------------------*/
/*   Define the clipboard format item record         */
/*---------------------------------------------------*/

typedef struct _ClipboardFormatItemRec {

    long recordType;
    itemId parentItemId;  /* this is the data item that owns the format */
    Display *displayId;	/* display id of application owning data */
    Window windowId;	/* window id for cut by name */
    Widget cutByNameWidget;    /* window id for cut by name */
    Window cutByNameWindow;    /* window id for cut by name */
    XmCutPasteProc cutByNameCallback;   /* address of callback routine for */
    					/* cut by name data */

    long itemLength;	/* length of this format item data */
    itemId formatDataId;	/* id for format item data, */
    			        /* 0 if passed by name */
    Atom formatNameAtom; /* format name atom */
    unsigned long formatNameLength;

    unsigned long cancelledFlag;  /* format was cancelled by poster */
    unsigned long cutByNameFlag;  /* data has not yet been provided */

    itemId thisFormatId;  /* id given application for identifying format item */
    itemId itemPrivateId; /* id provide by application for identifying item */

    unsigned long copiedLength;  /* amount already copied incrementally */

} ClipboardFormatItemRec, *ClipboardFormatItem;

/*-------------------------------------------------*/
/*   Define the clipboard data item record         */
/*-------------------------------------------------*/

typedef struct _ClipboardDataItemRec {

    long recordType;
    itemId adjunctData; /* for future compatibility */
    Display *displayId;	/* display id of application owning data */
    Window windowId;	/* window id for cut by name */

    itemId thisItemId;  /* item id of this data item */

    unsigned long dataItemLabelId; /* id of label (comp) string */
    unsigned long formatIdList;	 /* offset of beginning of format id list */
    long formatCount;		/* number of formats stored for this item */
    long cancelledFormatCount;	/* number of cut by name formats cancelled */

    unsigned long cutByNameFlag;  /* data has not yet been provided */
    unsigned long deletePendingFlag;	/* is item marked for deletion? */
    unsigned long permanentItemFlag;	/* is item permanent or temporary? */

    XmCutPasteProc cutByNameCallback;   /* address of callback routine for */
    					/* cut by name data */
    Widget cutByNameWidget;   /* widget receiving messages concerning */
    				      /* cut by name */
    Window cutByNameWindow;

} ClipboardDataItemRec, *ClipboardDataItem;

/*----------------------------------------------*/
/*   Define the clipboard header record         */
/*----------------------------------------------*/

typedef struct _ClipboardHeaderRec {

    long recordType;
    itemId adjunctHeader;	/* for future compatibility */
    unsigned long maxItems;	/* maximum number of clipboard items */
    				/* including those marked for delete */
    unsigned long dataItemList;	    /* offset of data item id list */

    itemId nextPasteItemId;    	/* data id of next item id to paste */
    itemId oldNextPasteItemId; 	/* data id of old next paste item id */
    itemId deletedByCopyId;	/* item marked deleted by last copy, if any */
    itemId lastCopyItemId;  /* item id of last item put on clipboard */
    itemId recopyId;        /* item id of item requested for recopy */
      
    unsigned long currItems;   /* current number of clipboard items */
    			       /* including those marked for delete */
    Time   selectionTimestamp; /* for ICCCM clipboard selection compatability */
    Time   copyFromTimestamp; /* time of event causing inquire or copy from */
    unsigned long foreignCopiedLength; /* amount copied so far in incr copy from */ 
			      /* selection */
    Window ownSelection;
    unsigned long incrementalCopyFrom;  /* requested in increments */

    unsigned long startCopyCalled;  /* to ensure that start copy is called
					before copy or endcopy */

} ClipboardHeaderRec, *ClipboardHeader;

/*---------------------------------------------*/

typedef union {
    ClipboardFormatItemRec format;
    ClipboardDataItemRec   item;
    ClipboardHeaderRec     header;
} ClipboardUnionRec, *ClipboardPointer;

/*---------------------------------------------*/
     
/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XtPointer AddAddresses() ;
static Time _XmClipboardGetCurrentTime() ;
static void _XmClipboardSetNextItemId() ;
static Boolean _XmWeOwnSelection() ;
static void _XmAssertClipboardSelection() ;
static Boolean _XmWaitForPropertyDelete() ;
static Boolean _XmGetConversion() ;
static void _XmHandleSelectionEvents() ;
static Boolean _XmSelectionRequestHandler() ;
static Window _XmInitializeSelection() ;
static int _XmRegIfMatch() ;
static int _XmRegisterFormat() ;
static void _XmClipboardError() ;
static void _XmClipboardEventHandler() ;
static int _XmClipboardFindItem() ;
static int _XmGetWindowProperty() ;
static int _XmClipboardRetrieveItem() ;
static void _XmClipboardReplaceItem() ;
static Atom _XmClipboardGetAtomFromId() ;
static Atom _XmClipboardGetAtomFromFormat() ;
static int _XmClipboardGetLenFromFormat() ;
static char * _XmClipboardAlloc() ;
static void _XmClipboardFreeAlloc() ;
static ClipboardHeader _XmClipboardOpen() ;
static void _XmClipboardClose() ;
static void _XmClipboardDeleteId() ;
static ClipboardFormatItem _XmClipboardFindFormat() ;
static void _XmClipboardDeleteFormat() ;
static void _XmClipboardDeleteFormats() ;
static void _XmClipboardDeleteItemLabel() ;
static unsigned long _XmClipboardIsMarkedForDelete() ;
static void _XmClipboardDeleteItem() ;
static void _XmClipboardDeleteMarked() ;
static void _XmClipboardMarkItem() ;
static int _XmClipboardSendMessage() ;
static int _XmClipboardDataIsReady() ;
static int _XmClipboardSelectionIsReady() ;
static int _XmClipboardRequestorIsReady() ;
static int _XmClipboardGetSelection() ;
static int _XmClipboardRequestDataAndWait() ;
static itemId _XmClipboardGetNewItemId() ;
static void _XmClipboardSetAccess() ;
static int _XmClipboardLock() ;
static int _XmClipboardUnlock() ;
static int _XmClipboardSearchForWindow() ;
static int _XmClipboardWindowExists() ;

#else

static XtPointer AddAddresses( 
                        XtPointer base,
                        long offset) ;
static Time _XmClipboardGetCurrentTime( 
                        Display *dpy) ;
static void _XmClipboardSetNextItemId( 
                        Display *display,
                        long itemid) ;
static Boolean _XmWeOwnSelection( 
                        Display *display,
                        ClipboardHeader header) ;
static void _XmAssertClipboardSelection( 
                        Display *display,
                        Window window,
                        ClipboardHeader header,
                        Time time) ;
static Boolean _XmWaitForPropertyDelete( 
                        Display *display,
                        Window window,
                        Atom property) ;
static Boolean _XmGetConversion( 
                        Atom selection,
                        Atom target,
                        Atom property,
                        Widget widget,
                        Window window,
                        Boolean *incremental,
#if NeedWidePrototypes
                        int multiple,
#else
                        Boolean multiple,
#endif /* NeedWidePrototypes */
                        Boolean *abort,
                        XEvent *ev) ;
static void _XmHandleSelectionEvents( 
                        Widget widget,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static Boolean _XmSelectionRequestHandler( 
                        Widget widget,
                        Atom target,
                        XtPointer *value,
                        unsigned long *length,
                        int *format) ;
static Window _XmInitializeSelection( 
                        Display *display,
                        ClipboardHeader header,
                        Window window,
                        Time time) ;
static int _XmRegIfMatch( 
                        Display *display,
                        char *format_name,
                        char *match_name,
                        int format_length) ;
static int _XmRegisterFormat( 
                        Display *display,
                        char *format_name,
                        int format_length) ;
static void _XmClipboardError( 
                        char *key,
                        char *message) ;
static void _XmClipboardEventHandler( 
                        Widget widget,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static int _XmClipboardFindItem( 
                        Display *display,
                        itemId itemid,
                        XtPointer *outpointer,
                        unsigned long *outlength,
                        int *format,
                        int rec_type) ;
static int _XmGetWindowProperty( 
                        Display *display,
                        Window window,
                        Atom property_atom,
                        XtPointer *outpointer,
                        unsigned long *outlength,
                        Atom *type,
                        int *format,
#if NeedWidePrototypes
                        int delete_flag) ;
#else
                        Boolean delete_flag) ;
#endif /* NeedWidePrototypes */
static int _XmClipboardRetrieveItem( 
                        Display *display,
                        itemId itemid,
                        int add_length,
                        int def_length,
                        XtPointer *outpointer,
                        unsigned long *outlength,
                        int *format,
                        int rec_type,
                        unsigned long discard) ;
static void _XmClipboardReplaceItem( 
                        Display *display,
                        itemId itemid,
                        XtPointer pointer,
                        unsigned long length,
                        int mode,
                        int format,
#if NeedWidePrototypes
                        int free_flag) ;
#else
                        Boolean free_flag) ;
#endif /* NeedWidePrototypes */
static Atom _XmClipboardGetAtomFromId( 
                        Display *display,
                        itemId itemid) ;
static Atom _XmClipboardGetAtomFromFormat( 
                        Display *display,
                        char *format_name) ;
static int _XmClipboardGetLenFromFormat( 
                        Display *display,
                        char *format_name,
                        int *format_length) ;
static char * _XmClipboardAlloc( 
                        size_t size) ;
static void _XmClipboardFreeAlloc( 
                        char *addr) ;
static ClipboardHeader _XmClipboardOpen( 
                        Display *display,
                        int add_length) ;
static void _XmClipboardClose( 
                        Display *display,
                        ClipboardHeader root_clipboard_header) ;
static void _XmClipboardDeleteId( 
                        Display *display,
                        itemId itemid) ;
static ClipboardFormatItem _XmClipboardFindFormat( 
                        Display *display,
                        ClipboardHeader header,
                        char *format,
                        itemId itemid,
                        int n,
                        unsigned long *maxnamelength,
                        int *count,
                        unsigned long *matchlength) ;
static void _XmClipboardDeleteFormat( 
                        Display *display,
                        itemId formatitemid) ;
static void _XmClipboardDeleteFormats( 
                        Display *display,
                        Window window,
                        itemId dataitemid) ;
static void _XmClipboardDeleteItemLabel( 
                        Display *display,
                        Window window,
                        itemId dataitemid) ;
static unsigned long _XmClipboardIsMarkedForDelete( 
                        Display *display,
                        ClipboardHeader header,
                        itemId itemid) ;
static void _XmClipboardDeleteItem( 
                        Display *display,
                        Window window,
                        ClipboardHeader header,
                        itemId deleteid) ;
static void _XmClipboardDeleteMarked( 
                        Display *display,
                        Window window,
                        ClipboardHeader header) ;
static void _XmClipboardMarkItem( 
                        Display *display,
                        ClipboardHeader header,
                        itemId dataitemid,
                        unsigned long state) ;
static int _XmClipboardSendMessage( 
                        Display *display,
                        Window window,
                        ClipboardFormatItem formatptr,
                        int messagetype) ;
static int _XmClipboardDataIsReady( 
                        Display *display,
                        XEvent *event,
                        char *private_info) ;
static int _XmClipboardSelectionIsReady( 
                        Display *display,
                        XEvent *event,
                        char *private_info) ;
static int _XmClipboardRequestorIsReady( 
                        Display *display,
                        XEvent *event,
                        char *private_info) ;
static int _XmClipboardGetSelection( 
                        Display *display,
                        Window window,
                        char *format,
                        ClipboardHeader header,
                        XtPointer *format_data,
                        unsigned long *data_length) ;
static int _XmClipboardRequestDataAndWait( 
                        Display *display,
                        Window window,
                        ClipboardFormatItem formatptr) ;
static itemId _XmClipboardGetNewItemId( 
                        Display *display) ;
static void _XmClipboardSetAccess( 
                        Display *display,
                        Window window) ;
static int _XmClipboardLock( 
                        Display *display,
                        Window window) ;
static int _XmClipboardUnlock( 
                        Display *display,
                        Window window,
#if NeedWidePrototypes
                        int all_levels) ;
#else
                        Boolean all_levels) ;
#endif /* NeedWidePrototypes */
static int _XmClipboardSearchForWindow( 
                        Display *display,
                        Window parentwindow,
                        Window window) ;
static int _XmClipboardWindowExists( 
                        Display *display,
                        Window window) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*---------------------------------------------*/
/* internal routines			       */
/*---------------------------------------------*/
static XtPointer
#ifdef _NO_PROTO
AddAddresses( base, offset )
        XtPointer base ;
        long offset ;
#else
AddAddresses(
        XtPointer base,
        long offset )
#endif /* _NO_PROTO */
{
    char* ptr;

    ptr = (char*)(base);

    ptr = ptr + offset;

    return((XtPointer) ptr) ;
}

static Time 
#ifdef _NO_PROTO
_XmClipboardGetCurrentTime( dpy )
        Display *dpy ;
#else
_XmClipboardGetCurrentTime(
        Display *dpy )
#endif /* _NO_PROTO */
{
    XEvent event;

    XSelectInput(dpy,DefaultRootWindow(dpy),PropertyChangeMask);
    XChangeProperty(dpy,DefaultRootWindow(dpy),
                  XmInternAtom(dpy,"_MOTIF_CLIP_TIME",False),
                  XmInternAtom(dpy,"_MOTIF_CLIP_TIME",False),
                  8,PropModeAppend,NULL,0);
    XWindowEvent(dpy,DefaultRootWindow(dpy),PropertyChangeMask,&event);
    return(event.xproperty.time);
}
static void 
#ifdef _NO_PROTO
_XmClipboardSetNextItemId( display, itemid )
        Display *display ;
        long itemid ;
#else
_XmClipboardSetNextItemId(
        Display *display,
        long itemid )
#endif /* _NO_PROTO */
{
    itemId base;
    itemId nextItem;
    XtPointer int_ptr;
    unsigned long length;
    ClipboardHeader header;
    itemId current_item;
    itemId last_item;

    header = _XmClipboardOpen( display, 0 );
    current_item = header->nextPasteItemId;
    last_item = header->oldNextPasteItemId;
    _XmClipboardClose( display, header );

    nextItem = itemid;
    do {
	base = nextItem - (nextItem % XM_ITEM_ID_INC);
	if (base >= XM_ITEM_ID_MAX) {
	    nextItem = XM_FIRST_FREE_ID;
	} else {
	    nextItem = base + XM_ITEM_ID_INC;
	}
    } while (nextItem == current_item - 1 || nextItem == last_item - 1);

    _XmClipboardFindItem( display,
			XM_NEXT_ID,
			&int_ptr,
			&length,
			0,
			0 );
    *(int *) int_ptr = (int) nextItem;
    length = sizeof( int );
    _XmClipboardReplaceItem( display,
			XM_NEXT_ID,
			int_ptr,
			length,
			PropModeReplace,
			32,
			True );
}


static Boolean 
#ifdef _NO_PROTO
_XmWeOwnSelection( display, header )
        Display *display ;
        ClipboardHeader header ;
#else
_XmWeOwnSelection(
        Display *display,
        ClipboardHeader header )
#endif /* _NO_PROTO */
{
    Window selectionwindow;

    selectionwindow = XGetSelectionOwner( display, CLIPBOARD );

    return ( selectionwindow == header->ownSelection );
}

static void 
#ifdef _NO_PROTO
_XmAssertClipboardSelection( display, window, header, time )
        Display *display ;
        Window window ;
        ClipboardHeader header ;
        Time time ;
#else
_XmAssertClipboardSelection(
        Display *display,
        Window window,
        ClipboardHeader header,
        Time time )
#endif /* _NO_PROTO */
{
    Widget widget;

    header->ownSelection = None;
    header->selectionTimestamp = 0;

    widget = XtWindowToWidget (display, window);

    /* Need a valid Widget to add an event handler. */

    if (widget == NULL)
    {
	return;
    }

    /* Assert ownership of CLIPBOARD selection only if there is valid data. */

    if (header->nextPasteItemId == 0)
    {
	return;
    }

    header->ownSelection = window;
    header->selectionTimestamp = time;

    XSetSelectionOwner (display, CLIPBOARD, window, time);
    XtAddEventHandler (widget, 0, TRUE, _XmHandleSelectionEvents, NULL);
    return;
}



static Boolean 
#ifdef _NO_PROTO
_XmWaitForPropertyDelete( display, window, property )
        Display *display ;
        Window window ;
        Atom property ;
#else
_XmWaitForPropertyDelete(
        Display *display,
        Window window,
        Atom property )
#endif /* _NO_PROTO */
{
    XEvent event_return;
    ClipboardDestroyInfoRec info;

    info.window = window;
    info.property = property;


    /*
     * look for match event and discard all prior configures
     */
    if (XCheckIfEvent( display, &event_return,
                       _XmClipboardRequestorIsReady, (char *)&info )) {
	if ( info.window == 0 )
	    return FALSE;
	else
	    return TRUE;
    }

    XIfEvent( display, &event_return, _XmClipboardRequestorIsReady,
					(char *)&info );
    if ( info.window == 0 )
	return FALSE;
    else
	return TRUE;
}


/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
_XmGetConversion( selection, target, property, widget, window, incremental, multiple, abort, ev )
        Atom selection ;
        Atom target ;
        Atom property ;
        Widget widget ;
        Window window ;
        Boolean *incremental ;
        Boolean multiple ;
        Boolean *abort ;
        XEvent *ev ;
#else
_XmGetConversion(
        Atom selection,
        Atom target,
        Atom property,
        Widget widget,
        Window window,
        Boolean *incremental,
#if NeedWidePrototypes
        int multiple,
#else
        Boolean multiple,
#endif /* NeedWidePrototypes */
        Boolean *abort,
        XEvent *ev )
#endif /* _NO_PROTO */
{
    XWindowAttributes window_attributes;
    XtPointer value;
    Atom loc_target;
    unsigned long length, next_length, length_sent;
    int format;
    Display *display;
    XSelectionEvent *event;

    event = (XSelectionEvent*)ev;

    display = XtDisplay( widget );

    *abort = FALSE;

    /* get the data from the clipboard, regardless of how long it is
    */
    if ( _XmSelectionRequestHandler( widget,
                                      target,
                                      &value,
                                      &length,
                                      &format ) == FALSE )
    {
        return FALSE;
    }

    if ( length > MAX_SELECTION_INCR( display ) )
    {
        /* have to do an incremental
        */
        *incremental = TRUE;

        /* if this is a multiple request then don't actually send data now
        */
        if ( multiple ) return TRUE;

        /* get the current window event mask
        */
        XGetWindowAttributes( display, window, &window_attributes );

        /* select for property notify as well as current mask
        */
        XSelectInput( display,
                      window,
                      PropertyChangeMask  |
                      StructureNotifyMask | window_attributes.your_event_mask );

        loc_target = CLIP_INCR;

        /* inform the requestor
        */
        XChangeProperty( display,
                         window,
                         property,
                         loc_target,
                         32,
                         PropModeReplace,
                         (unsigned char *) &length,
                         1 );

        (void) XSendEvent( display,
                           event->requestor,
                           False,
                           (unsigned long)NULL,
                           ev );

        /* now wait for the property delete
        */
        if (!_XmWaitForPropertyDelete( display, window, property))
        {
            /* this means the window got destroyed before the property was */
            /* deleted
            */
            _XmClipboardFreeAlloc((char *)value);
	    *abort = TRUE;
            return FALSE;
        }

        next_length = MAX_SELECTION_INCR( display );

        /* send the data in increments
        */
        for( length_sent = 0;
             next_length > 0;
             length_sent = length_sent + next_length )
        {
            next_length = MIN( length - length_sent,
                               next_length );

            /* last property changed will be length 0
            */
            XChangeProperty( display,
                             window,
                             property,
                             target,
                             format,
                             PropModeReplace,
                             (unsigned char *) value + length_sent,
                             (int) (next_length * 8) / format );

            /* wait for the property delete, unless it has length zero
            */
            if ( next_length > 0 )
            {
                if ( !_XmWaitForPropertyDelete( display, window, property ) )
                {
                   /* this means the window got destroyed before the property */
                   /* was deleted
                   */
                   _XmClipboardFreeAlloc((char *)value);
                   *abort = TRUE;
                   return FALSE;
                }
            }
        }

        /* put mask back the way it was
        */
        XSelectInput( display,
                      window,
                      window_attributes.your_event_mask );

    }else{
        /* not incremental
        */
        *incremental = FALSE;

        /* write the data to the property
        */
        XChangeProperty( display,
                         window,
                         property,
                         target,
                         format,
                         PropModeReplace,
                         (unsigned char *) value,
                         (int) (length * 8) / format );

    }

    _XmClipboardFreeAlloc((char *)value);

    return TRUE;
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmHandleSelectionEvents( widget, closure, event, cont )
        Widget widget ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
_XmHandleSelectionEvents(
        Widget widget,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XSelectionEvent ev;
    Boolean incremental;
    unsigned long bytesafter, length;
    IndirectPair *p;
    char *value;
    int format;
    Atom target;
    unsigned long count;
    Boolean writeback;

    switch( event->type )
    {
        case SelectionClear:
            if (event->xselectionclear.selection != XmInternAtom(
                event->xselectionclear.display, "CLIPBOARD", False))
                      return;
            break;

        case SelectionRequest:
        {
            Display *display;
            Window window;
            ClipboardHeader header;
            Time data_time;
            Boolean notify_sent = FALSE;
	    Boolean abort = FALSE;

            if (event->xselectionrequest.selection != XmInternAtom(
                event->xselectionrequest.display, "CLIPBOARD", False))
                        return;

	    writeback = FALSE;

            display = XtDisplay( widget );
            window  = XtWindow(  widget );

            data_time = CurrentTime;

            if ( _XmClipboardLock( display, window ) == ClipboardSuccess )
            {
                /* get the clipboard header
                */
                header = _XmClipboardOpen( display, 0 );

                data_time = header->selectionTimestamp;

                _XmClipboardClose( display, header );
                _XmClipboardUnlock( display, window, False );
            }

            ev.type = SelectionNotify;
            ev.display = event->xselectionrequest.display;
            ev.requestor = event->xselectionrequest.requestor;
            ev.selection = event->xselectionrequest.selection;
            ev.time = event->xselectionrequest.time;
            ev.target = event->xselectionrequest.target;
            /* if obsolete requestor, then use the target as property */
            if( event->xselectionrequest.property == None )
                event->xselectionrequest.property =
					event->xselectionrequest.target;
            ev.property = event->xselectionrequest.property;

            if (( event->xselectionrequest.time != CurrentTime ) &&
                ( event->xselectionrequest.time < data_time ))
            {
                ev.property = None;
            }else{
                if ( ev.target == MULTIPLE )
                {
                    (void) XGetWindowProperty(
                                event->xselectionrequest.display,
                                event->xselectionrequest.requestor,
                                event->xselectionrequest.property,
                                0L,
                                1000000,
                                False,
                                AnyPropertyType,
                                &target,
                                &format,
                                &length,
                                &bytesafter,
                                (unsigned char **)&value );

                    count = BYTELENGTH( length, format )
                                / sizeof( IndirectPair );

                    for ( p = (IndirectPair *)value; count; p++, count-- )
                    {
                        if ( _XmGetConversion( ev.selection,
                                            p->target,
                                            p->property,
                                            widget,
                                            event->xselectionrequest.requestor,
                                            &incremental,
                                            TRUE,
					    &abort,
                                            (XEvent *) &ev ))
                        {
                            if ( incremental )
                            {
                                p->target = CLIP_INCR;
                                writeback = TRUE;
                            }
                        }else{
                            p->property = None;
                            writeback = TRUE;
                        }
                    }

                    if ( writeback )
                    {
                        XChangeProperty( ev.display,
                                         ev.requestor,
                                         event->xselectionrequest.property,
                                         target,
                                         format,
                                         PropModeReplace,
                                         (unsigned char *) value,
                                         (int) length);
                    }

                    if (value != NULL)
                        XFree(value);
                 }else{
                    /* not multiple
                    */
                    if ( _XmGetConversion( ev.selection,
                                        event->xselectionrequest.target,
                                        event->xselectionrequest.property,
                                        widget,
                                        event->xselectionrequest.requestor,
                                        &incremental,
                                        FALSE,
					&abort,
                                        (XEvent *) &ev ))
                    {
                        if ( incremental )
                            notify_sent = TRUE;
                    }else{
                        ev.property = None;
                    }
                }
            } /* end time okay */

            if ( !notify_sent  && !abort )
            {
               (void) XSendEvent( display,
                                  ev.requestor,
                                  False,
                                  (unsigned long)NULL,
                                  (XEvent *) &ev );
            }
            break;

        } /* end selection request */

    } /* end switch on event type */
}

static Boolean 
#ifdef _NO_PROTO
_XmSelectionRequestHandler( widget, target, value, length, format )
        Widget widget ;
        Atom target ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
#else
_XmSelectionRequestHandler(
        Widget widget,
        Atom target,
        XtPointer *value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    Display *display;
    Window window;
    ClipboardHeader header;
    Boolean ret_value;
    int i;

    display = XtDisplay( widget );
    window  = XtWindow(  widget );

    if ( _XmClipboardLock( display, window ) != ClipboardSuccess )
	    return False;

    ret_value = True;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );

    /* fake loop
    */
    for( i = 0; i < 1; i++ )
    {
	
    if ( !_XmWeOwnSelection( display, header ) ) 
    {
	/* we don't own the selection, something's wrong
	*/
	ret_value = False;
	break;
    }

    if ( target == TARGETS )
    {
	Atom *ptr, *save_ptr;
	ClipboardFormatItem nextitem;
	int n, count;
	unsigned long dummy;
	int bytes_per_target;

	*length = 0;

	*format = XM_ATOM;  /* i.e. 32 */

	bytes_per_target = XM_ATOM / 8;

	n = 1;

	/* find the first format for the next paste item, if any remain
	*/
	nextitem = _XmClipboardFindFormat( display, 
					    header, 
					    0, 
					    (itemId) NULL, 
					    n,
					    &dummy, 
					    &count,
					    &dummy );

	/* allocate storage for list of target atoms 
	*/
	ptr = (Atom *)_XmClipboardAlloc( count * bytes_per_target );
	save_ptr = ptr;

        while ( nextitem != NULL )
	{
	    /* add format to list 
	    */
	    *ptr = nextitem->formatNameAtom;

	    n = n + 1;

	    _XmClipboardFreeAlloc( (char *) nextitem );

	    /* find the nth format for the next paste item
	    */
	    nextitem = _XmClipboardFindFormat( display, 
						header, 
						0, 
						(itemId) NULL, 
						n,
						&dummy, 
						&count,
						&dummy );
	    if (nextitem != NULL) ptr++;
		
	}
	*value = (char *) save_ptr;

	/* n - 1 is number of targets, length is 4 times that
	*/
	*length = (n - 1) * bytes_per_target;

    }else{
    if ( target == TIMESTAMP )
    {
	Time *timestamp;

	timestamp = (Time *)_XmClipboardAlloc(sizeof(Time));
        *timestamp  = header->selectionTimestamp;

	*value = (char *) timestamp;

	*length = sizeof(Time);

	*format = XM_INTEGER;

    }else{
	/* some named format */

	char *format_name;
	long private_id;
	unsigned long outlength;

	/* convert atom to format name
	*/
	format_name = XmGetAtomName( display, target );

        _XmClipboardGetLenFromFormat( display, format_name, format) ;

	if (XmClipboardInquireLength( display, window, format_name, length )
	     != ClipboardSuccess)
	{
	    ret_value = False;
	    break;
	}

	if ( *length == 0 )
	{
	    ret_value = False;
	    break;
	}

	*value = _XmClipboardAlloc( (int) *length );

        /* do incremental copy if it's too long
        */

	if ( XmClipboardRetrieve( display, 
				   window, 
				   format_name, 
				   (XtPointer) *value, 
				   *length, 
				   &outlength, 
				   &private_id ) != ClipboardSuccess )
	{
	    ret_value = False;
	    break;
	}
    }
    } 
    } /* end fake loop */

    _XmClipboardClose( display, header );
    _XmClipboardUnlock( display, window, False );

    return ret_value;
}


static Window 
#ifdef _NO_PROTO
_XmInitializeSelection( display, header, window, time )
        Display *display ;
        ClipboardHeader header ;
        Window window ;
        Time time ;
#else
_XmInitializeSelection(
        Display *display,
        ClipboardHeader header,
        Window window,
        Time time )
#endif /* _NO_PROTO */
{
    Window selectionwindow;    

    /* If there is no CLIPBOARD owner, and we have clipboard
     * data, then assert ownership, and use that data.
     */

    selectionwindow = XGetSelectionOwner (display, CLIPBOARD);


    /* if the header is corrupted, give up. */

    if (selectionwindow == window && header->ownSelection == None)
    {
	selectionwindow = None;
	XSetSelectionOwner (display, CLIPBOARD, None, time);
    }

    if (selectionwindow != None) /* someone owns CLIPBOARD already */
    {
	return selectionwindow;
    }

    /* assert ownership of the clipboard selection */

    _XmAssertClipboardSelection (display, window, header, time);

    selectionwindow = XGetSelectionOwner (display, CLIPBOARD);

    return (selectionwindow);
}


static int 
#ifdef _NO_PROTO
_XmRegIfMatch( display, format_name, match_name, format_length )
        Display *display ;
        char *format_name ;
        char *match_name ;
        int format_length ;
#else
_XmRegIfMatch(
        Display *display,
        char *format_name,
        char *match_name,
        int format_length )
#endif /* _NO_PROTO */
{
	if ( strcmp( format_name, match_name ) == 0 )
	{
	    _XmRegisterFormat( display, format_name, format_length );
	    return 1;
	}
    return 0;
}

static int 
#ifdef _NO_PROTO
_XmRegisterFormat( display, format_name, format_length )
        Display *display ;
        char *format_name ;
        int format_length ;
#else
_XmRegisterFormat(
        Display *display,       /* Display id of application passing data */
        char *format_name,      /* Name string for data format */
        int format_length )     /* Format length  8-16-32 */
#endif /* _NO_PROTO */
{
    Window rootwindow;
    Atom formatatom;
    int stored_len;

    /* get the atom for the format_name
    */
    formatatom = _XmClipboardGetAtomFromFormat( display, 
    					         format_name );

    rootwindow = XDefaultRootWindow( display );

    if ( _XmClipboardGetLenFromFormat( display, 
				        format_name,
					&stored_len ) == ClipboardSuccess )
    {
	if ( stored_len == format_length )
	    return ClipboardSuccess;

	/* it is already registered, don't allow override
	*/
	return ClipboardFail;
    }

    XChangeProperty( display, 
		     rootwindow, 
		     formatatom,
		     formatatom,
		     32,
		     PropModeReplace,
		     (unsigned char*)&format_length,
		     1 );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardError( key, message )
        char *key ;
        char *message ;
#else
_XmClipboardError(
        char *key,
        char *message )
#endif /* _NO_PROTO */
{
    XMERROR(key, message );
}

/*---------------------------------------------*/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmClipboardEventHandler( widget, closure, event, cont )
        Widget widget ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
_XmClipboardEventHandler(
        Widget widget,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XClientMessageEvent *event_rcvd;
    Display *display;
    itemId formatitemid;
    ClipboardFormatItem formatitem; 
    unsigned long formatlength;
    long privateitemid;
    XmCutPasteProc callbackroutine;
    int reason, ret_value;

    event_rcvd = (XClientMessageEvent*)event;

    if ( (event_rcvd->type & 127) != ClientMessage )
    	return ;

    display = event_rcvd->display;

    if ( event_rcvd->message_type != XmInternAtom( display, 
    					    "_MOTIF_CLIP_MESSAGE", False ) )
    	return ;

    formatitemid  = event_rcvd->data.l[1];
    privateitemid = event_rcvd->data.l[2];

    /* get the callback routine
    */
    ret_value = _XmClipboardFindItem( display, 
    				       formatitemid,
    			   	       (XtPointer *) &formatitem,
    			   	       &formatlength,
				       0,
    			   	       XM_FORMAT_HEADER_TYPE );

    if ( ret_value != ClipboardSuccess ) 
    	return ;

    callbackroutine = formatitem->cutByNameCallback;

    _XmClipboardFreeAlloc( (char *) formatitem );

    if ( callbackroutine == 0 )
    	return ;

    reason = 0;

    if ( event_rcvd->data.l[0] == XmInternAtom( display, 
    	    			       	       "_MOTIF_CLIP_DATA_REQUEST", 
   	  					False ) )
    	reason = XmCR_CLIPBOARD_DATA_REQUEST;

    if ( event_rcvd->data.l[0] == XmInternAtom( display, 
    	    			    	        "_MOTIF_CLIP_DATA_DELETE", 
   	  					False ) )
    	reason = XmCR_CLIPBOARD_DATA_DELETE;

    if ( reason == 0 )
    	return ;

    /* call the callback routine
    */
    (*callbackroutine)( widget, 
    		        (long *) &formatitemid,
    		        &privateitemid,
    			&reason );

    /* if this was a data request, reset the recopy id
    */
    if (reason == XmCR_CLIPBOARD_DATA_REQUEST)
    {
	unsigned long hlength;
	ClipboardHeader header;

	_XmClipboardFindItem (display, XM_HEADER_ID, (XtPointer *)&header,
				&hlength, 0, 0);

	header->recopyId = 0;

	_XmClipboardReplaceItem (display, XM_HEADER_ID, header, hlength,
				PropModeReplace, 32, True);
    }

    return ;
}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardFindItem( display, itemid, outpointer, outlength, format, rec_type )
        Display *display ;
        itemId itemid ;
        XtPointer *outpointer ;
        unsigned long *outlength ;
        int *format ;
        int rec_type ;
#else
_XmClipboardFindItem(
        Display *display,
        itemId itemid,
        XtPointer *outpointer,
        unsigned long *outlength,
        int *format,
        int rec_type )
#endif /* _NO_PROTO */
{

    Window rootwindow;
    int ret_value;
    Atom itematom;
    ClipboardPointer ptr;

    rootwindow = XDefaultRootWindow( display );

    /* convert the id into an atom
    */
    itematom = _XmClipboardGetAtomFromId( display, itemid );

    ret_value = _XmGetWindowProperty( display,
                                       rootwindow,
                                       itematom,
                                       outpointer,
                                       outlength,
                                       0,
                                       format,
                                       FALSE );

    if ( ret_value != ClipboardSuccess ) return ret_value;

    ptr = (ClipboardPointer)(*outpointer);

    if ( rec_type != 0 && ptr->header.recordType != rec_type )
    {
    	_XmClipboardFreeAlloc( (char *) *outpointer );
	CleanupHeader (display);
    	_XmClipboardError( CLIPBOARD_BAD_DATA_TYPE, BAD_DATA_TYPE );
    	return ClipboardFail;
    }

    return ClipboardSuccess;
}


/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmGetWindowProperty( display, window, property_atom, outpointer, outlength, type, format, delete_flag )
        Display *display ;
        Window window ;
        Atom property_atom ;
        XtPointer *outpointer ;
        unsigned long *outlength ;
        Atom *type ;
        int *format ;
        Boolean delete_flag ;
#else
_XmGetWindowProperty(
        Display *display,
        Window window,
        Atom property_atom,
        XtPointer *outpointer,
        unsigned long *outlength,
        Atom *type,
        int *format,
#if NeedWidePrototypes
        int delete_flag )
#else
        Boolean delete_flag )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

    int ret_value;
    Atom loc_type;
    unsigned long bytes_left;
    unsigned long cur_length;
    unsigned char *loc_pointer;
    unsigned long this_length;
    char *cur_pointer;
    int loc_format;
    long request_size;
    long offset;
    int byte_length;


    bytes_left = 1;
    offset = 0;
    cur_length = 0;
    cur_pointer = NULL;

    *outpointer = 0;
    *outlength = 0;

    request_size = MAX_SELECTION_INCR( display );

    while ( bytes_left > 0 )
    {
	/* retrieve the item from the root
	*/
	ret_value = XGetWindowProperty( display, 
					window, 
					property_atom,
					offset, /*offset*/
					request_size, /*length*/
					FALSE,
					AnyPropertyType,
					&loc_type,
					&loc_format,			    
					&this_length,
					&bytes_left,
					&loc_pointer );

        if ( ret_value != 0 ) 
            return ClipboardFail;

        if ( loc_pointer == 0 || this_length == 0 )
        {
            if ( delete_flag )
            {
                XDeleteProperty( display, window, property_atom );
	    }
	    	if (loc_pointer != NULL)
	    XFree ((char *)loc_pointer);
            return ClipboardFail;
	  }

        /* convert length according to format
        */
        byte_length = BYTELENGTH( this_length, loc_format );

	if ( cur_length == 0 )
	{
	    cur_pointer = _XmClipboardAlloc((size_t)(byte_length + bytes_left));
	    /* Size arg. is truncated if sizeof( long) > sizeof( size_t) */

	    *outpointer = cur_pointer;
	}

	memcpy(cur_pointer, loc_pointer, (size_t) byte_length );
	cur_pointer = cur_pointer + byte_length;
	cur_length  = cur_length  + byte_length;
	offset += loc_format * this_length / 32;

	if (loc_pointer != NULL)
		XFree ((char *)loc_pointer);
    }

    if ( delete_flag )
    {
        XDeleteProperty( display, window, property_atom );
    }

    if ( format != 0 ) 
    {
	*format = loc_format;
    }

    if ( type != 0 )
    {
        *type = loc_type;
    }

    *outlength = cur_length;

    return ClipboardSuccess;
}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardRetrieveItem( display, itemid, add_length, def_length, outpointer, outlength, format, rec_type, discard )
        Display *display ;
        itemId itemid ;
        int add_length ;
        int def_length ;
        XtPointer *outpointer ;
        unsigned long *outlength ;
        int *format ;
        int rec_type ;
        unsigned long discard ;
#else
_XmClipboardRetrieveItem(
        Display *display,
        itemId itemid,
        int add_length,             /* allocate this add'l */
        int def_length,             /* if item non-exist */
        XtPointer *outpointer,
        unsigned long *outlength,
        int *format,
        int rec_type,
        unsigned long discard )     /* ignore old data */
#endif /* _NO_PROTO */
{

    int ret_value;
    int loc_format;
    unsigned long loclength;
    ClipboardPointer clipboard_pointer;
    XtPointer pointer;

     /* retrieve the item from the root
    */
    ret_value = _XmClipboardFindItem( display, 
    				       itemid,
    				       &pointer,
    				       &loclength,
				       &loc_format,
    				       rec_type );

    if (loclength == 0 || ret_value != ClipboardSuccess)
    {
    	*outlength = def_length;
    }else{    
	if ( discard == 1 ) loclength = 0;

	*outlength = loclength + add_length;
    }

    /* get local memory for the item
    */
    clipboard_pointer = (ClipboardPointer)_XmClipboardAlloc( (size_t) *outlength );
	    /* Size arg. is truncated if sizeof( long) > sizeof( size_t) */

    if (ret_value == ClipboardSuccess)
    {
	/* copy the item into the local memory
	*/
	memcpy(clipboard_pointer, pointer, (size_t) loclength );
    }

    *outpointer = (char*)clipboard_pointer;

    /* free memory pointed to by pointer 
    */
    _XmClipboardFreeAlloc( (char *) pointer );

    if ( format != 0 )
    {
	*format = loc_format;
    }

    /* return a pointer to the item
    */
    return ret_value;

}

/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardReplaceItem( display, itemid, pointer, length, mode, format, free_flag )
        Display *display ;
        itemId itemid ;
        XtPointer pointer ;
        unsigned long length ;
        int mode ;
        int format ;
        Boolean free_flag ;
#else
_XmClipboardReplaceItem(
        Display *display,
        itemId itemid,
        XtPointer pointer,
        unsigned long length,
        int mode,
        int format,
#if NeedWidePrototypes
        int free_flag )
#else
        Boolean free_flag )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Window rootwindow;
    Atom itematom;
    XtPointer loc_pointer;
    unsigned long loc_length;
    int loc_mode;
    unsigned int max_req_size;
    int factor;

    loc_pointer = pointer;
    loc_mode = mode;

    rootwindow = XDefaultRootWindow( display );

    /* convert the id into an atom
    */
    itematom = _XmClipboardGetAtomFromId( display, itemid );

    /* lengths are passed in bytes, but need to specify in format units */
    /* for ChangeProperty.  This code is portable to the architectures	*/
    /* with 64-bit longs (currently Cray and Alpha).  Note that these	*/
    /* architectures transmit format-32 properties by removing the top	*/
    /* 32 bits of each 64-bit quantity.					*/

    factor = (format == 8) ? 1 :
		(format == 16) ? sizeof(unsigned short) :
				 sizeof(unsigned long);

    loc_length = length / factor;

    max_req_size = ( MAX_SELECTION_INCR( display ) ) / factor;

    do
    {
	unsigned long next_length;

	if ( loc_length > max_req_size )
	{
	    next_length = max_req_size;
	}else{
	    next_length = loc_length;
	}
	
	/* put the new values in the root 
	*/
	XChangeProperty( display, 
			 rootwindow, 
			 itematom,
			 itematom,
			 format,
			 loc_mode,
			 (unsigned char*)loc_pointer,
			 (int) next_length ); /* Truncation of next_length.*/

	loc_mode = PropModeAppend;
	loc_length = loc_length - next_length;
	loc_pointer = (char *) loc_pointer + next_length;
    }
    while( loc_length > 0 );

    if ( free_flag == True )
    {
	/* note:  you have to depend on the free flag, even if the length */
	/* is zero, the pointer may point to non-zero-length allocated data 
	*/
        _XmClipboardFreeAlloc( (char *) pointer );
    }
}

/*---------------------------------------------*/
static Atom 
#ifdef _NO_PROTO
_XmClipboardGetAtomFromId( display, itemid )
        Display *display ;
        itemId itemid ;
#else
_XmClipboardGetAtomFromId(
        Display *display,
        itemId itemid )
#endif /* _NO_PROTO */
{
    char *item;
    char atomname[ 100 ];

    switch ( (int)itemid )
    {	
	case 0:	item = "_MOTIF_CLIP_HEADER";
	    break;
	case 1: item = "_MOTIF_CLIP_NEXT_ID";
	    break;
	default:
    	    sprintf( atomname, "_MOTIF_CLIP_ITEM_%d", itemid );
    	    item = atomname;
	    break;	
    }

    return XmInternAtom( display, item, False );
}
/*---------------------------------------------*/
static Atom 
#ifdef _NO_PROTO
_XmClipboardGetAtomFromFormat( display, format_name )
        Display *display ;
        char *format_name ;
#else
_XmClipboardGetAtomFromFormat(
        Display *display,
        char *format_name )
#endif /* _NO_PROTO */
{
    char *item;
    Atom ret_value;

    item = _XmClipboardAlloc( strlen( format_name ) + 20 );

    sprintf( item, "_MOTIF_CLIP_FORMAT_%s", format_name );

    ret_value = XmInternAtom( display, item, False );

    _XmClipboardFreeAlloc( (char *) item );

    return ret_value;
}
/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardGetLenFromFormat( display, format_name, format_length )
        Display *display ;
        char *format_name ;
        int *format_length ;
#else
_XmClipboardGetLenFromFormat(
        Display *display,
        char *format_name,
        int *format_length )
#endif /* _NO_PROTO */
{
    Atom format_atom;
    int ret_value;
    Window rootwindow;
    unsigned long outlength;
    unsigned char *outpointer;
    Atom type;
    int format;
    unsigned long bytes_left;

    format_atom = _XmClipboardGetAtomFromFormat( display, format_name );

    rootwindow = XDefaultRootWindow( display );

    /* get the format record 
    */
    ret_value = XGetWindowProperty( display, 
				    rootwindow, 
				    format_atom,
				    0, /*offset*/
				    10000000, /*length*/
				    False,
				    AnyPropertyType,
				    &type,
				    &format,
				    &outlength,
				    &bytes_left,
				    &outpointer );

    if ( outpointer == 0 || outlength == 0 || ret_value !=0 )
    {
	/* if not successful, return warning that format is not registered
	*/
	ret_value = ClipboardFail;

	*format_length = 8;

    }else{
	ret_value = ClipboardSuccess;

	/* return the length of the format
	*/
	*format_length = *((int *)outpointer);
    }

    if (outpointer != NULL)
      XFree((char*)outpointer);

    return ret_value;
}

/*---------------------------------------------*/
static char * 
#ifdef _NO_PROTO
_XmClipboardAlloc( size )
        size_t size ;
#else
_XmClipboardAlloc(
        size_t size )
#endif /* _NO_PROTO */
{
        return XtMalloc( size );
}
/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardFreeAlloc( addr )
        char *addr ;
#else
_XmClipboardFreeAlloc(
        char *addr )
#endif /* _NO_PROTO */
{
     if ( addr != 0 )*addr = (char)255;
        XtFree( addr );
}

/*---------------------------------------------*/
static ClipboardHeader 
#ifdef _NO_PROTO
_XmClipboardOpen( display, add_length )
        Display *display ;
        int add_length ;
#else
_XmClipboardOpen(
        Display *display,
        int add_length )
#endif /* _NO_PROTO */
{
    int ret_value;
    unsigned long headerlength;
    ClipboardHeader root_clipboard_header;
    int number ;
    unsigned long length;
    XtPointer int_ptr;

    ret_value = ClipboardSuccess;

    if ( add_length == 0 )
    {
	/* get the clipboard header
	*/
	ret_value = _XmClipboardFindItem( display, 
					   XM_HEADER_ID,
					   (XtPointer *) &root_clipboard_header,
					   &headerlength,
					   0,
    					   0 );
    }

    if ( add_length != 0 || ret_value != ClipboardSuccess )
    {
	/* get the clipboard header (this will allocate memory if doesn't exist)
	*/
	ret_value = _XmClipboardRetrieveItem( display, 
					       XM_HEADER_ID,
					       add_length,
					       sizeof( ClipboardHeaderRec ),
					       (XtPointer *) &root_clipboard_header,
					       &headerlength,
					       0,
    					       0,
					       0 ); /* don't discard old data */
    }

    /* means clipboard header had not been initialized
    */
    if ( ret_value != ClipboardSuccess ) 
    {
    	root_clipboard_header->recordType = XM_HEADER_RECORD_TYPE;
    	root_clipboard_header->adjunctHeader = 0;
    	root_clipboard_header->maxItems = 1;
    	root_clipboard_header->currItems = 0;
    	root_clipboard_header->dataItemList =
	  sizeof( ClipboardHeaderRec ) / CONVERT_32_FACTOR;
    	root_clipboard_header->nextPasteItemId = 0;
    	root_clipboard_header->lastCopyItemId = 0;
    	root_clipboard_header->recopyId = 0;
    	root_clipboard_header->oldNextPasteItemId = 0;
    	root_clipboard_header->deletedByCopyId = 0;
	root_clipboard_header->ownSelection = 0;
	root_clipboard_header->selectionTimestamp = CurrentTime;
	root_clipboard_header->copyFromTimestamp  = CurrentTime;
    	root_clipboard_header->foreignCopiedLength = 0;
    	root_clipboard_header->incrementalCopyFrom = 0;
	root_clipboard_header->startCopyCalled = (unsigned long) False;
    }

    /* make sure "next free id" property has been initialized
    */
    ret_value = _XmClipboardFindItem( display, 
				       XM_NEXT_ID,
				       &int_ptr,
				       &length,
    				       0,
    				       0 );

    if ( ret_value != ClipboardSuccess ) 
    {
	number = XM_FIRST_FREE_ID;
    	int_ptr = (XtPointer) &number;
    	length = sizeof( int );
    	
    	/* initialize the next id property
    	*/
	_XmClipboardReplaceItem( display,
				  XM_NEXT_ID,
				  int_ptr,
				  length,
    				  PropModeReplace,
				  32,
    				  False );
    }
    else
    {
    	_XmClipboardFreeAlloc( (char*)int_ptr );
    }

    return root_clipboard_header;
}

/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardClose( display, root_clipboard_header )
        Display *display ;
        ClipboardHeader root_clipboard_header ;
#else
_XmClipboardClose(
        Display *display,
        ClipboardHeader root_clipboard_header )
#endif /* _NO_PROTO */
{
    unsigned long headerlength;

    headerlength = sizeof( ClipboardHeaderRec ) + 
    		(root_clipboard_header -> currItems) * sizeof( itemId );

    /* replace the clipboard header
    */
    _XmClipboardReplaceItem( display, 
    			      XM_HEADER_ID,
    			      (XtPointer)root_clipboard_header,
    			      headerlength,
    			      PropModeReplace,
			      32,
    			      True );
}

/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardDeleteId( display, itemid )
        Display *display ;
        itemId itemid ;
#else
_XmClipboardDeleteId(
        Display *display,
        itemId itemid )
#endif /* _NO_PROTO */
{
    Window rootwindow;
    Atom itematom;

    rootwindow = XDefaultRootWindow( display );

    itematom = _XmClipboardGetAtomFromId( display, itemid ); 

    XDeleteProperty( display, rootwindow, itematom );

}


/*---------------------------------------------*/
static ClipboardFormatItem 
#ifdef _NO_PROTO
_XmClipboardFindFormat( display, header, format, itemid, n, maxnamelength, count, matchlength )
        Display *display ;
        ClipboardHeader header ;
        char *format ;
        itemId itemid ;
        int n ;
        unsigned long *maxnamelength ;
        int *count ;
        unsigned long *matchlength ;
#else
_XmClipboardFindFormat(
        Display *display,       /* Display id of application wanting data */
        ClipboardHeader header,
        char *format,
        itemId itemid,
        int n,                  /* if looking for nth format */
        unsigned long *maxnamelength,/* receives max format name length */
        int *count,             /* receives next paste format count */
        unsigned long *matchlength )      /* receives length of matching format */
#endif /* _NO_PROTO */
{
    ClipboardDataItem queryitem;
    ClipboardFormatItem currformat, matchformat;
    unsigned long reclength ;
    int i, free_flag, index;
    itemId currformatid, queryitemid, *idptr;
    Atom formatatom;

    *count = 0;
    *maxnamelength = 0;

    if ( itemid < 0 ) return 0;

    /* if passed an item id then use that, otherwise use next paste item
    */
    if ( itemid != 0 )
    {
    	queryitemid = itemid;

    }else{

    	if ( header->currItems == 0 ) return 0;

        queryitemid = header->nextPasteItemId;
    }

    if ( queryitemid == 0 ) return 0;

    /* get the query item
    */
    if ( _XmClipboardFindItem( display, 
    				queryitemid,
    				(XtPointer *) &queryitem,
    				&reclength,
				0,
    				XM_DATA_ITEM_RECORD_TYPE ) == ClipboardFail ) 
    	return 0;

    if ( queryitem == 0 )
    {
	CleanupHeader (display);
	_XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
	return 0;
    }

    *count = queryitem->formatCount - queryitem->cancelledFormatCount;

    if ( *count < 0 ) *count = 0;

    /* point to the first format id in the list
    */
    idptr = (itemId*) AddAddresses( (XtPointer) queryitem,
				queryitem->formatIdList * CONVERT_32_FACTOR);
    currformatid = *idptr;

    matchformat = 0;
    *matchlength = 0;
    index = 1;
    formatatom = XmInternAtom( display, format, False );

    /* run through all the formats for the query item looking */
    /* for a name match with the input format name
    */
    for ( i = 0; i < queryitem->formatCount; i++ )
    {
    	/* free the allocation unless it is the matching format
    	*/
    	free_flag = 1;

	/* get the next format
	*/
	_XmClipboardFindItem( display, 
			       currformatid,
			       (XtPointer *) &currformat,
			       &reclength,
			       0,
    			       XM_FORMAT_HEADER_TYPE );

    	if ( currformat == 0 )
    	{
	    CleanupHeader (display);
	    _XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
    	    return 0;
    	}

    	if ( currformat->cancelledFlag == 0 )
    	{
	    /* format has not been cancelled
	    */
	    *maxnamelength = MAX( *maxnamelength, 
				  currformat->formatNameLength );

	    if (format != NULL)
    	    {
		if ( currformat->formatNameAtom == formatatom )
		{
		    matchformat = currformat;
    		    free_flag = 0;
    		    *matchlength = reclength;
    		}

    	    }else{
		/* we're looking for the n'th format 
		*/
    		if ( index == n )
    		{
		    matchformat = currformat;
    		    free_flag = 0;
    		    *matchlength = reclength;
    		}

		index = index + 1;
    	    }
    	}

    	if (free_flag == 1 )
    	{
    	    _XmClipboardFreeAlloc( (char *) currformat );
    	}

    	idptr = idptr + 1;
	
	currformatid = *idptr; 
    }

    _XmClipboardFreeAlloc( (char *) queryitem );

    return matchformat;
}

/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardDeleteFormat( display, formatitemid )
        Display *display ;
        itemId formatitemid ;
#else
_XmClipboardDeleteFormat(
        Display *display,
        itemId formatitemid )
#endif /* _NO_PROTO */
{
    itemId  dataitemid;
    ClipboardDataItem dataitem;
    ClipboardFormatItem formatitem;
    unsigned long length ;
    unsigned long formatlength;

    /* first get the format item out of the root 
    */
    _XmClipboardFindItem( display, 
    			   formatitemid,
    			   (XtPointer *) &formatitem,
    			   &formatlength,
			   0,
    			   XM_FORMAT_HEADER_TYPE );

    if ( formatitem == 0 )
    {
	CleanupHeader (display);
	_XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
	return;
    }

    if ( ( formatitem->cutByNameFlag == 0 ) ||
    	 ( formatitem->cancelledFlag != 0 ) )
    {
    	/* nothing to do, data not passed by name or already cancelled
    	*/
    	_XmClipboardFreeAlloc( (char *) formatitem );
    	return;
    }

    dataitemid = formatitem->parentItemId;

    /* now get the data item out of the root 
    */
    _XmClipboardFindItem( display, 
    			   dataitemid,
    			   (XtPointer *) &dataitem,
    			   &length,
			   0,
    			   XM_DATA_ITEM_RECORD_TYPE );

    if ( dataitem == 0 )
    {
	CleanupHeader (display);
	_XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
	return;
    }

    dataitem->cancelledFormatCount = dataitem->cancelledFormatCount + 1; 

    if ( dataitem->cancelledFormatCount == dataitem->formatCount ) 
    {
    	/* no formats left, mark the item for delete
    	*/
        dataitem->deletePendingFlag = 1;
    }

    /* set the cancel flag on
    */
    formatitem->cancelledFlag = 1;

    /* return the property on the root window for the item
    */
    _XmClipboardReplaceItem( display, 
    			      formatitemid,
    			      (XtPointer)formatitem,
    			      formatlength,
    			      PropModeReplace,
			      32,
			      True );

    _XmClipboardReplaceItem( display, 
    			      dataitemid,
    			      (XtPointer)dataitem,
    			      length,
    			      PropModeReplace,
			      32,
    			      True );

}

/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardDeleteFormats( display, window, dataitemid )
        Display *display ;
        Window window ;
        itemId dataitemid ;
#else
_XmClipboardDeleteFormats(
        Display *display,
        Window window,
        itemId dataitemid )
#endif /* _NO_PROTO */
{
    itemId *deleteptr;
    ClipboardDataItem datalist;
    ClipboardFormatItem formatdata;
    unsigned long length ;
    int i;

    /* first get the data item out of the root 
    */
    _XmClipboardFindItem( display, 
    			   dataitemid,
    			   (XtPointer *) &datalist,
    			   &length,
			   0,
    			   XM_DATA_ITEM_RECORD_TYPE );

    if ( datalist == 0 )
    {
	CleanupHeader (display);
	_XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
	return;
    }

    deleteptr = (itemId*)AddAddresses( (XtPointer) datalist,
				datalist->formatIdList * CONVERT_32_FACTOR); 

    for ( i = 0; i < datalist->formatCount; i++ )
    {
    	/* first delete the format data 
    	*/
	_XmClipboardFindItem( display, 
			       *deleteptr,
			       (XtPointer *) &formatdata,
			       &length,
			       0,
    			       XM_FORMAT_HEADER_TYPE );

    	if ( formatdata == 0 )
    	{
	    CleanupHeader (display);
	    _XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
    	    return;
    	}

    	if ( formatdata->cutByNameFlag == 1 )
    	{
    	    /* format was cut by name
    	    */
	    _XmClipboardSendMessage( display, 
    				      window,
    				      formatdata,
				      XM_DATA_DELETE_MESSAGE );
    	}

    	_XmClipboardDeleteId( display, formatdata->formatDataId );

        _XmClipboardFreeAlloc( (char *) formatdata );

    	/* then delete the format header
    	*/
    	_XmClipboardDeleteId( display, *deleteptr );

    	*deleteptr = 0;

    	deleteptr = deleteptr + 1;
    }

    _XmClipboardFreeAlloc( (char *) datalist );
}

/*---------------------------------------------*/
/* ARGSUSED */
 static void 
#ifdef _NO_PROTO
_XmClipboardDeleteItemLabel( display, window, dataitemid )
        Display *display ;
        Window window ;
        itemId dataitemid ;
#else
_XmClipboardDeleteItemLabel(
        Display *display,
        Window window,
        itemId dataitemid )
#endif /* _NO_PROTO */
{
     ClipboardDataItem datalist;
     unsigned long length;

     /* first get the data item out of the root
     */
     _XmClipboardFindItem( display,
                          dataitemid,
                          (XtPointer *) &datalist,
                          &length,
                          0,
                          XM_DATA_ITEM_RECORD_TYPE );

     if ( datalist == 0 )
     {
	CleanupHeader (display);
	_XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
	return;
     }
     /* delete item label
     */
     _XmClipboardDeleteId (display, datalist->dataItemLabelId);

     _XmClipboardFreeAlloc( (char *) datalist );
 }

/*---------------------------------------------*/
/* ARGSUSED */
static unsigned long 
#ifdef _NO_PROTO
_XmClipboardIsMarkedForDelete( display, header, itemid )
        Display *display ;
        ClipboardHeader header ;
        itemId itemid ;
#else
_XmClipboardIsMarkedForDelete(
        Display *display,
        ClipboardHeader header,
        itemId itemid )
#endif /* _NO_PROTO */
{
    ClipboardDataItem curritem;
    unsigned long return_value, reclength;

    if ( itemid == 0 ) 
    {
	CleanupHeader (display);
	_XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
    	return 0;
    }

    /* get the next format
    */
    _XmClipboardFindItem( display, 
			   itemid,
			   (XtPointer *) &curritem,
			   &reclength,
			   0,
    			   XM_DATA_ITEM_RECORD_TYPE );

    return_value = curritem->deletePendingFlag;

    _XmClipboardFreeAlloc( (char *) curritem );

    return return_value;
}

/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardDeleteItem( display, window, header, deleteid )
        Display *display ;
        Window window ;
        ClipboardHeader header ;
        itemId deleteid ;
#else
_XmClipboardDeleteItem(
        Display *display,
        Window window,
        ClipboardHeader header,
        itemId deleteid )
#endif /* _NO_PROTO */
{
    int i;
    itemId *listptr,*thisid, *nextid, nextpasteid;
    int nextpasteindex;
    int lastflag = 0;

    /* find the delete id in the header item list 
    */
    listptr = (itemId*)AddAddresses( (XtPointer) header,
				header->dataItemList * CONVERT_32_FACTOR ); 

    i = 0;

    nextpasteindex = 0;
    nextpasteid    = 0;

    nextid = listptr;
    thisid = nextid;

    /* redo the item list
    */
    if(    !header->currItems    )
    {   return ;
        } 
    while ( i < header->currItems )
    {
    	i++ ;

	if (*nextid == deleteid ) 
    	{
    	    nextid++;

    	    nextpasteindex = i - 2;

    	    /* if this flag doesn't get reset, then delete item was last item
    	    */
    	    lastflag = 1;

    	    continue;
    	}

    	lastflag = 0;

    	*thisid = *nextid;

    	thisid = thisid + 1;
	nextid = nextid + 1;
    }

    *thisid = 0;

    header->currItems = header->currItems - 1;

    /* if we are deleting the next paste item, then we need to find */
    /* a new one
    */
    if ( header->nextPasteItemId == deleteid )
    {

	if ( lastflag == 1 )
	{
	    nextpasteindex = nextpasteindex - 1; 
	}

	/* store this value temporarily
	*/
	i = nextpasteindex;

	/* now find the next paste candidate */
	/* first try to find next older item to make next paste
	*/
	while ( nextpasteindex >= 0 )
	{
	    thisid = listptr + nextpasteindex;

	    if ( !_XmClipboardIsMarkedForDelete( display, header, *thisid ) )
	    { 
		nextpasteid = *thisid;
		break;
	    }

	    nextpasteindex = nextpasteindex - 1;
	}

	/* if didn't find a next older item, find next newer item
	*/
	if ( nextpasteid == 0 )
	{
	    /* restore this value
	    */
	    nextpasteindex = i;

	    while ( nextpasteindex < header->currItems )
	    {
		thisid = listptr + nextpasteindex;

		if ( !_XmClipboardIsMarkedForDelete( display, header, *thisid ) )
		{ 
		    nextpasteid = *thisid;
		    break;
		}

		nextpasteindex = nextpasteindex + 1;
	    }
	}

        header->nextPasteItemId = nextpasteid;
        header->oldNextPasteItemId = 0;
    }
     /* delete the item label
     */
     _XmClipboardDeleteItemLabel( display, window, deleteid);

    /* delete all the formats belonging to the data item
    */
    _XmClipboardDeleteFormats( display, window, deleteid );

    /* now delete the item itself
    */
    _XmClipboardDeleteId( display, deleteid );
    	
}


/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardDeleteMarked( display, window, header )
        Display *display ;
        Window window ;
        ClipboardHeader header ;
#else
_XmClipboardDeleteMarked(
        Display *display,
        Window window,
        ClipboardHeader header )
#endif /* _NO_PROTO */
{
    itemId *nextIdPtr;
    unsigned long endi, i;

    /* find the header item list 
    */
    nextIdPtr = (itemId*)AddAddresses((XtPointer) header,
				header->dataItemList * CONVERT_32_FACTOR); 

    i = 0;
    endi = header->currItems;

    /* run through the item list looking for things to delete
    */
    while( 1 )
    { 
    	if ( i >= endi ) break;

    	i = i + 1;

	if ( _XmClipboardIsMarkedForDelete( display, header, *nextIdPtr ) )
    	{
    	    _XmClipboardDeleteItem( display, window, header, *nextIdPtr );

    	}else{
    	    nextIdPtr = nextIdPtr + 1;
    	}    
    }
}
/*---------------------------------------------*/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmClipboardMarkItem( display, header, dataitemid, state )
        Display *display ;
        ClipboardHeader header ;
        itemId dataitemid ;
        unsigned long state ;
#else
_XmClipboardMarkItem(
        Display *display,
        ClipboardHeader header,
        itemId dataitemid,
        unsigned long state )
#endif /* _NO_PROTO */
{
    ClipboardDataItem itemheader;
    unsigned long itemlength;

    if ( dataitemid == 0 ) return;

    /* get a pointer to the item
    */
    _XmClipboardFindItem( display, 
    			   dataitemid,
    			   (XtPointer *) &itemheader,
    			   &itemlength,
			   0,
    			   XM_DATA_ITEM_RECORD_TYPE );

    if ( itemheader == 0 ) 
    {
	CleanupHeader (display);
	_XmClipboardError( CLIPBOARD_CORRUPT, CORRUPT_DATA_STRUCTURE );
    	return;
    }

    /* mark the delete pending flag
    */
    itemheader->deletePendingFlag = state;

    /* return the item to the root window 
    */
    _XmClipboardReplaceItem( display, 
    			      dataitemid,
    			      (XtPointer)itemheader,
    			      itemlength,
    			      PropModeReplace,
			      32,
    			      True );

}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardSendMessage( display, window, formatptr, messagetype )
        Display *display ;
        Window window ;
        ClipboardFormatItem formatptr ;
        int messagetype ;
#else
_XmClipboardSendMessage(
        Display *display,
        Window window,
        ClipboardFormatItem formatptr,
        int messagetype )
#endif /* _NO_PROTO */
{
    Window widgetwindow;
    XClientMessageEvent event_sent;
    long event_mask = 0;
    Display *widgetdisplay;
    unsigned long headerlength;
    ClipboardHeader root_clipboard_header;
    Boolean dummy;

    widgetdisplay = formatptr->displayId;
    widgetwindow = formatptr->cutByNameWindow;

    if ( widgetwindow == 0 ) return 0;

    event_sent.type         = ClientMessage;
    event_sent.display      = widgetdisplay;
    event_sent.window       = widgetwindow;
    event_sent.message_type = XmInternAtom( display, 
    					    "_MOTIF_CLIP_MESSAGE", False );
    event_sent.format = 32;

    switch ( messagetype )
    {	
	case XM_DATA_REQUEST_MESSAGE:	

	    /* get the clipboard header
	    */
	    _XmClipboardFindItem( display, 
				   XM_HEADER_ID,
				   (XtPointer *) &root_clipboard_header,
				   &headerlength,
				   0,
				   0 );

	    /* set the recopy item id in the header (so locking can be circumvented)
	    */
	    root_clipboard_header->recopyId = formatptr->thisFormatId;

	    /* replace the clipboard header
	    */
	    _XmClipboardReplaceItem( display, 
				      XM_HEADER_ID,
				      (XtPointer)root_clipboard_header,
				      headerlength,
				      PropModeReplace,
				      32,
				      True );

            event_sent.data.l[0] = XmInternAtom( display, 
    	    			    	        "_MOTIF_CLIP_DATA_REQUEST",
   	  					False );
	    break;

	case XM_DATA_DELETE_MESSAGE:	
            event_sent.data.l[0] = XmInternAtom( display, 
    	    			    		"_MOTIF_CLIP_DATA_DELETE", 
   	  					False );
	    break;
    }

    event_sent.data.l[1] = formatptr->thisFormatId;
    event_sent.data.l[2] = formatptr->itemPrivateId;

    /* is this the same application that stored the data?
    */
    if ( formatptr->windowId == window && formatptr->displayId == display )
    {
    	/* call the event handler directly to avoid blocking 
    	*/
    	_XmClipboardEventHandler( formatptr->cutByNameWidget,
    				   0,
    				   (XEvent *) &event_sent,
                                   &dummy );
    }else{

	/* if we aren't in same application that stored the data, then */
	/* make sure the window still exists
	*/
        if ( !_XmClipboardWindowExists( display, widgetwindow )) return 0;

	/* send a client message to the window supplied by the user
	*/
	XSendEvent( display, widgetwindow, True, event_mask, 
		    (XEvent*)&event_sent ); 
    }

    return 1;
}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardDataIsReady( display, event, private_info )
        Display *display ;
        XEvent *event ;
        char *private_info ;
#else
_XmClipboardDataIsReady(
        Display *display,
        XEvent *event,
        char *private_info )
#endif /* _NO_PROTO */
{
     XDestroyWindowEvent *destroy_event;
     ClipboardCutByNameInfo cutbynameinfo;
     ClipboardFormatItem formatitem;
     unsigned long formatlength;
     int okay;
 
     cutbynameinfo = ( ClipboardCutByNameInfo )private_info;

     if ( (event->type & 127) == DestroyNotify )
     {
        destroy_event = (XDestroyWindowEvent*)event;

        if ( destroy_event->window == cutbynameinfo->window )
        {
            cutbynameinfo->window = 0;
            return 1;
        }
     }

     if ( (event->type & 127) != PropertyNotify )
    	return 0;


     /* get the format item
     */
     _XmClipboardFindItem( display,
 			   cutbynameinfo->formatitemid,
 			   (XtPointer *) &formatitem,
 			   &formatlength,
			   0,
    			   XM_FORMAT_HEADER_TYPE );
 
 
     okay = (int)( formatitem->cutByNameFlag == 0 );
     	    
     /* release the allocation
     */
     _XmClipboardFreeAlloc( (char *) formatitem );
     
     return okay;
     
}


/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardSelectionIsReady( display, event, private_info )
        Display *display ;
        XEvent *event ;
        char *private_info ;
#else
_XmClipboardSelectionIsReady(
        Display *display,
        XEvent *event,
        char *private_info )
#endif /* _NO_PROTO */
{
    XPropertyEvent *property_event;
    XSelectionEvent *selection_event;
    XDestroyWindowEvent *destroy_event;
    XtPointer outpointer, temp_ptr;
    unsigned long outlength ;
    int loc_format, ret_value;
    Atom loc_type;
    ClipboardSelectionInfo info;
    Boolean get_the_property;

    info = ( ClipboardSelectionInfo )private_info;

    get_the_property = FALSE;

    if ( (event->type & 127) == DestroyNotify )
    {
        destroy_event = (XDestroyWindowEvent*)event;

        if ( destroy_event->window == info->selection_window )
        {
            info->selection_window = 0;
            return 1;
        }
    }

    if ( (event->type & 127) == SelectionNotify )
    {
        selection_event = (XSelectionEvent*)event;

	if (selection_event->property == None)
        {
            return 1;
        }

	if (selection_event->property == CLIP_TEMP)
	{
	    info->selection_notify_received = TRUE;
	    get_the_property = TRUE;
	}
    }

    if ( (event->type & 127) == PropertyNotify )
    {
        property_event = (XPropertyEvent*)event;

        /* make sure we have right property and are ready
        */
        if ( property_event->atom == CLIP_TEMP
                            &&
             property_event->state == PropertyNewValue
                            &&
             info->selection_notify_received == TRUE )
        {
            get_the_property = TRUE;
        }
    }

    if ( get_the_property )
    {
        /* get the property from the window, deleting it as we do
        */
        ret_value = _XmGetWindowProperty( info->display,
                                           info->window,
                                           CLIP_TEMP,
                                           &outpointer,
                                           &outlength,
                                           &loc_type,
                                           &loc_format,
                                           TRUE );

        /* if zero length then done
        */
        if ( outpointer == 0 || outlength == 0
                             || ret_value != ClipboardSuccess )
        {
            return 1;
        }

        /* is the target INCR?
        */
        if ( loc_type == CLIP_INCR )
        {
            info->incremental = TRUE;

            return 0;
        }

        info->type = loc_type;

        /* length returned is in bytes */
        /* allocate storage for the data
        */
        temp_ptr = (char *)_XmClipboardAlloc( (size_t) (info->count + outlength));

        /* copy over what we have so far
        */
        memcpy( temp_ptr, info->data, (size_t) info->count );

        /* free up old allocation
        */
        _XmClipboardFreeAlloc( (char *) info->data );

        info->data = (char *)temp_ptr;

        /* append the new stuff from the property
        */
        memcpy((char *)temp_ptr + info->count, outpointer, (size_t) outlength );

        _XmClipboardFreeAlloc(outpointer) ;

        info->count = info->count + outlength;

        /* if we aren't in incremental receive mode, then we're done
        */
        if ( !info->incremental )
        {
            return 1;
        }
    }

    return 0;

 }


/*---------------------------------------------*/
/* ARGSUSED */
static int 
#ifdef _NO_PROTO
_XmClipboardRequestorIsReady( display, event, private_info )
        Display *display ;
        XEvent *event ;
        char *private_info ;
#else
_XmClipboardRequestorIsReady(
        Display *display,
        XEvent *event,
        char *private_info )
#endif /* _NO_PROTO */
{
    XPropertyEvent *property_event;
    XDestroyWindowEvent *destroy_event;
    ClipboardDestroyInfo info;

    info = ( ClipboardDestroyInfo )private_info;

    if ( (event->type & 127) == DestroyNotify )
    {
        destroy_event = (XDestroyWindowEvent*)event;

        if ( destroy_event->window == info->window )
        {
            info->window = 0;
            return 1;
        }
    }

    if ( (event->type & 127) == PropertyNotify )
    {
        property_event = (XPropertyEvent*)event;

        /* make sure we have right property and are ready
        */
        if ( property_event->atom == info->property
                            &&
             property_event->state == PropertyDelete )
        {
            return 1;
        }
    }

    return 0;
 }


/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardGetSelection( display, window, format, header, format_data, data_length )
        Display *display ;
        Window window ;
        char *format ;
        ClipboardHeader header ;
        XtPointer *format_data ;
        unsigned long *data_length ;
#else
_XmClipboardGetSelection(
        Display *display,
        Window window,
        char *format,
        ClipboardHeader header,
        XtPointer *format_data,
        unsigned long *data_length )
#endif /* _NO_PROTO */
{
    XEvent event_return;
    ClipboardSelectionInfoRec info;
    int dataisready;
    Window selectionwindow;
    XWindowAttributes window_attributes, sel_window_attributes;
    Atom format_atom;

    format_atom = XmInternAtom( display, format, FALSE );

    /* will return null if window no longer exists that last had selection
    */
    selectionwindow = XGetSelectionOwner( display, CLIPBOARD );

    if ( selectionwindow == None )
    {
        return FALSE;
    }

    /* get the selection owner window event mask
    */
    XGetWindowAttributes( display, selectionwindow, &sel_window_attributes );

    /* select for structure notify as well as current mask
    */
    XSelectInput( display,
                  selectionwindow,
                  StructureNotifyMask | sel_window_attributes.your_event_mask );

    /* get the current window event mask
    */
    XGetWindowAttributes( display, window, &window_attributes );

    /* select for property notify as well as current mask
    */
    XSelectInput( display,
                  window,
                  PropertyChangeMask | window_attributes.your_event_mask );

    /* ask for the data in the specified format
    */
    XConvertSelection( display,
                       CLIPBOARD,
                       format_atom,
                       CLIP_TEMP,
                       window,
                       header->copyFromTimestamp );

    /* initialize the fields in the info record passed to the */
    /* predicate function
    */
    info.display = display;
    info.format  = format;
    info.window  = window;
    info.selection_window  = selectionwindow;
    info.time    = header->copyFromTimestamp;
    info.incremental          = FALSE;
    info.selection_notify_received = FALSE;
    info.data = 0;
    info.count = 0;

    /* check to see if have property change or selection notify events */
    /* this doesn't block
    */
    dataisready = XCheckIfEvent( display,
                                 &event_return,
                                 _XmClipboardSelectionIsReady,
                                 (char*)&info );

    if ( info.selection_window == 0 )
    {
        /* this means the window was destroyed before selection ready
        */
        /* put mask back the way it was
        */
        XSelectInput( display,
                      window,
                      window_attributes.your_event_mask );

        return FALSE;
    }

    if ( !dataisready )
    {
        /* check to see if have property change or selection notify events */
        /* this blocks waiting for event
        */
        XIfEvent( display,
                  &event_return,
                  _XmClipboardSelectionIsReady,
                  (char*)&info );
    }

    /* put mask back the way it was
    */
    XSelectInput( display,
                  window,
                  window_attributes.your_event_mask );


    if ( info.selection_window == 0 )
    {
        /* this means the window was destroyed before selection ready
        */
        return FALSE;
    }


    /* put mask back the way it was
    */
    XSelectInput( display,
                  selectionwindow,
                  sel_window_attributes.your_event_mask );

    *format_data = info.data;
    *data_length = info.count;

    if (*format_data==NULL || *data_length==0)
	return FALSE;

    return TRUE;
}


/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardRequestDataAndWait( display, window, formatptr )
        Display *display ;
        Window window ;
        ClipboardFormatItem formatptr ;
#else
_XmClipboardRequestDataAndWait(
        Display *display,
        Window window,
        ClipboardFormatItem formatptr )
#endif /* _NO_PROTO */
{
    XEvent event_return;
    int dataisready;
    XWindowAttributes rootattributes;
    Window rootwindow;
    ClipboardCutByNameInfoRec cutbynameinfo;

    rootwindow = XDefaultRootWindow( display );

    /* get the current root window event mask
    */
    XGetWindowAttributes( display, rootwindow, &rootattributes );

    /* select for property notify as well as current mask
    */
    XSelectInput( display, 
		  rootwindow, 
		  PropertyChangeMask  |
		  StructureNotifyMask | rootattributes.your_event_mask );

    if ( _XmClipboardSendMessage( display, 
    			           window,
    			           formatptr,
    			           XM_DATA_REQUEST_MESSAGE ) == 0 )
    {
	/* put mask back the way it was
	*/
	XSelectInput( display, 
		      rootwindow, 
		      rootattributes.your_event_mask );
    	return 0;
    }

    cutbynameinfo.formatitemid = formatptr->thisFormatId;
    cutbynameinfo.window = window;

    dataisready = XCheckIfEvent( display, 
    				 &event_return, 
    				 _XmClipboardDataIsReady, 
                                 (char*)&cutbynameinfo );

    if ( cutbynameinfo.window == 0 )
    {
        /* this means the cut by name window had been destroyed
        */
        return 0;
    }

    if ( !dataisready )
    {

        XIfEvent( display, 
    		  &event_return, 
    		  _XmClipboardDataIsReady, 
                  (char*)&cutbynameinfo );
    }

    if ( cutbynameinfo.window == 0 )
    {
        /* this means the cut by name window had been destroyed
        */
        return 0;
    }

    /* put mask back the way it was
    */
    XSelectInput( display, 
		  rootwindow, 
		  rootattributes.your_event_mask );

    return 1;
}

/*---------------------------------------------*/
static itemId
#ifdef _NO_PROTO
_XmClipboardGetNewItemId( display )
                        Display *display ;
#else
_XmClipboardGetNewItemId( 
                        Display *display )
#endif /* _NO_PROTO */
{
    XtPointer propertynumber;
    int *integer_ptr;
    unsigned long length;
    itemId loc_id;

    _XmClipboardFindItem( display,
			   XM_NEXT_ID,
			   &propertynumber,
			   &length,
			   0,
    			   0 );

    integer_ptr = (int *)propertynumber;

    *integer_ptr = *integer_ptr + 1;

    loc_id = (itemId)*integer_ptr;

    _XmClipboardReplaceItem( display,
			      XM_NEXT_ID,
			      propertynumber,
			      length,
    			      PropModeReplace,
			      32,
    			      True );

    return loc_id;
}


/*---------------------------------------------*/
static void 
#ifdef _NO_PROTO
_XmClipboardSetAccess( display, window )
        Display *display ;
        Window window ;
#else
_XmClipboardSetAccess(
        Display *display,
        Window window )
#endif /* _NO_PROTO */
{
    Atom itematom;

    itematom = XmInternAtom( display, 
			    "_MOTIF_CLIP_LOCK_ACCESS_VALID", False );

    /* put the clipboard lock access valid property on window
    */
    XChangeProperty( display, 
		     window, 
		     itematom,
		     itematom,
		     8,
		     PropModeReplace,
		     (unsigned char*)"yes",
		     3 );
}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardLock( display, window )
        Display *display ;
        Window window ;
#else
_XmClipboardLock(
        Display *display,
        Window window )
#endif /* _NO_PROTO */
{
    ClipboardLockPtr lockptr;
    unsigned long length;
    Atom _MOTIF_CLIP_LOCK = XmInternAtom (display, "_MOTIF_CLIP_LOCK", False);
    Window lock_owner = XGetSelectionOwner (display, _MOTIF_CLIP_LOCK);
    Boolean take_lock = False;

    if (lock_owner != window && lock_owner != None)
	return (ClipboardLocked);

    _XmClipboardFindItem (display, XM_LOCK_ID, (XtPointer *)&lockptr,
                          &length, 0, 0);

    if (length == 0)	/* create new lock property */
    {
	lockptr = (ClipboardLockPtr)_XmClipboardAlloc(sizeof(ClipboardLockRec));
	lockptr->lockLevel = 0;
    }

    if (lockptr->lockLevel == 0) /* new or invalid V1.0 lock. Take the lock */
    {
	lockptr->windowId = window;
	lockptr->lockLevel = 1;
	take_lock = True;
    }
    else if (lockptr->windowId == window) /* already have the lock */
    {
	lockptr->lockLevel += 1;
    }
    else	/* another client has the lock */
    {
	if (_XmClipboardWindowExists (display, lockptr->windowId))
	{
	    _XmClipboardFreeAlloc ((char *)lockptr);
	    return (ClipboardLocked);
	}
	else /* locking client has gone away, clipboard may be corrupted */
	{
	    ClipboardHeader header;
	    Window owner = XGetSelectionOwner (display, CLIPBOARD);
	    Time timestamp = _XmClipboardGetCurrentTime(display);

	    /* Drop the selection if a Motif client owns it */

            header = _XmClipboardOpen (display, 0);
	    if (header->ownSelection == owner)
	    {
		XSetSelectionOwner (display, CLIPBOARD, None, timestamp);
	    }
            _XmClipboardClose (display, header);

	    /* Reset the header property */

	    CleanupHeader (display);
	    header = _XmClipboardOpen (display, 0);
	    _XmClipboardClose (display, header);

	    /* Take the lock */

	    lockptr->windowId = window;
	    lockptr->lockLevel = 1;
	    take_lock = True;
	}	    
    }

    if (take_lock == True) {
	if (XGetSelectionOwner (display, _MOTIF_CLIP_LOCK) == None) {
	    XSetSelectionOwner (display, _MOTIF_CLIP_LOCK, window, 
				_XmClipboardGetCurrentTime(display));
	    if (XGetSelectionOwner (display, _MOTIF_CLIP_LOCK) != window) {
		_XmClipboardFreeAlloc ((char *)lockptr);
		return (ClipboardLocked);
	    }
	}
	else {
	    _XmClipboardFreeAlloc ((char *)lockptr);
	    return (ClipboardLocked);
	}
    }

    _XmClipboardReplaceItem (display, XM_LOCK_ID, (XtPointer)lockptr,
		sizeof(ClipboardLockRec), PropModeReplace, 32, False );

    _XmClipboardSetAccess (display, window);

    _XmClipboardFreeAlloc ((char *)lockptr);

    return (ClipboardSuccess);
}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardUnlock( display, window, all_levels )
        Display *display ;
        Window window ;
        Boolean all_levels ;
#else
_XmClipboardUnlock(
        Display *display,
        Window window,
#if NeedWidePrototypes
        int all_levels )
#else
        Boolean all_levels )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    unsigned long length;
    ClipboardLockPtr lockptr;
    Atom _MOTIF_CLIP_LOCK = XmInternAtom (display, "_MOTIF_CLIP_LOCK", False);
    Window lock_owner = XGetSelectionOwner (display, _MOTIF_CLIP_LOCK);
    Boolean release_lock = False;

    if (lock_owner != window && lock_owner != None)
	return (ClipboardFail);

    _XmClipboardFindItem (display, XM_LOCK_ID, (XtPointer *)&lockptr,
                          &length, 0, 0);

    if (length == 0) /* There is no lock property */
    {
    	return (ClipboardFail);
    }

    if ( lockptr->windowId != window ) /* Someone else has the lock */
    {
    	_XmClipboardFreeAlloc ((char *)lockptr);
    	return (ClipboardFail);
    }

    /* do the unlock */

    if (all_levels == 0)
    {
        lockptr->lockLevel -= 1;
    }else{
        lockptr->lockLevel = 0;
    }

    if (lockptr->lockLevel <= 0)
    {
	length = 0;
	release_lock = True;
    }else{
	length = sizeof(ClipboardLockRec);
    }

    _XmClipboardReplaceItem (display, XM_LOCK_ID, (XtPointer)lockptr, length,
			PropModeReplace, 32, False);
    _XmClipboardFreeAlloc ((char *)lockptr);


    if (release_lock == True) {
	XSetSelectionOwner (display, _MOTIF_CLIP_LOCK, None,
			    _XmClipboardGetCurrentTime(display));
    }

    return (ClipboardSuccess);
}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardSearchForWindow( display, parentwindow, window )
        Display *display ;
        Window parentwindow ;
        Window window ;
#else
_XmClipboardSearchForWindow(
        Display *display,
        Window parentwindow,
        Window window )
#endif /* _NO_PROTO */
{
    /* recursively search the roots descendant tree for the given window */

    Window rootwindow, p_window, *children;
    unsigned int numchildren;
    int found, i;
    Window *windowptr;

    if (XQueryTree( display, parentwindow, &rootwindow, &p_window,
		     &children, &numchildren ) == 0)
    {
	return (0);
    }

    found = 0;
    windowptr = children;

    /* now search through the list for the window */
    for ( i = 0; i < numchildren; i++ )
    {
	if ( *windowptr == window )
    	{
    	    found = 1;
    	}else{
    	    found = _XmClipboardSearchForWindow( display, *windowptr, window); 
   	}
    	if ( found == 1 ) break;
	windowptr = windowptr + 1;
    }

    _XmClipboardFreeAlloc( (char*)children );

    return found;
}

/*---------------------------------------------*/
static int 
#ifdef _NO_PROTO
_XmClipboardWindowExists( display, window )
        Display *display ;
        Window window ;
#else
_XmClipboardWindowExists(
        Display *display,
        Window window )
#endif /* _NO_PROTO */
{
    Window rootwindow;
    Atom itematom;
    int exists;
    unsigned long outlength;
    unsigned char *outpointer;
    Atom type;
    int format;
    unsigned long bytes_left;

    rootwindow = XDefaultRootWindow( display );

    exists = _XmClipboardSearchForWindow( display, rootwindow, window );

    if ( exists == 1 )
    {
	/* see if the window has the lock activity property, for if */
	/* it doesn't then this is a new assignment of the window id */
	/* and the lock is bogus due to a crash of the application */
	/* with the original locking window   
	*/
       itematom = XmInternAtom(display, "_MOTIF_CLIP_LOCK_ACCESS_VALID", False);

       XGetWindowProperty( display, 
			window, 
			itematom,
			0, /*offset*/
			10000000, /*length*/
			False,
			AnyPropertyType,
			&type,
			&format,
			&outlength,
			&bytes_left,
			&outpointer );

	if ( outpointer == 0 || outlength == 0 )
    	{
    	    /* not the same window that locked the clipboard 
    	    */
	    exists = 0;
    	}

    if(outpointer != NULL)
	   XFree((char*)outpointer);
    }    

    return exists;
}
/*---------------------------------------------*/
/* external routines			       */
/*---------------------------------------------*/


/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardBeginCopy( display, window, label, widget, callback, itemid )
        Display *display ;
        Window window ;
        XmString label ;
        Widget widget ;
        VoidProc callback ;
        long *itemid ;
#else
XmClipboardBeginCopy(
        Display *display,   /* display id for application passing data */
        Window window,  /* window to receive request for cut by name data */
        XmString label,     /* label to be associated with data item   */
        Widget widget,      /* only for cut by name */
        VoidProc callback,/* addr of callback routine for cut by name */
        long *itemid )      /* received id to identify this data item */
#endif /* _NO_PROTO */
{
    return(XmClipboardStartCopy( display, window, label, CurrentTime, 
				widget, (XmCutPasteProc)callback, itemid ));
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardStartCopy( display, window, label, timestamp, widget, callback, itemid )
        Display *display ;
        Window window ;
        XmString label ;
        Time timestamp ;
        Widget widget ;
        XmCutPasteProc callback ;
        long *itemid ;
#else
XmClipboardStartCopy(
        Display *display,   /* display id for application passing data */
        Window window,  /* window to receive request for cut by name data */
        XmString label, /* label to be associated with data item   */
        Time timestamp, /* timestamp of event triggering copy to clipboard */
        Widget widget,  /* only for cut by name */
        XmCutPasteProc callback,/* addr of callback routine for cut by name */
        long *itemid )  /* received id to identify this data item */
#endif /* _NO_PROTO */
{
    ClipboardHeader header;
    ClipboardDataItem itemheader;
    unsigned long itemlength;
    itemId loc_itemid;
    int status;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    /* get the clipboard header, make sure clipboard is initialized
    */
    header = _XmClipboardOpen( display, 0 );

    header->selectionTimestamp = timestamp;
    header->startCopyCalled = (unsigned long) True;

    /* allocate storage for the data item  
    */ 
    itemlength = sizeof( ClipboardDataItemRec );

    itemheader = (ClipboardDataItem)_XmClipboardAlloc( (size_t) itemlength );

    loc_itemid = _XmClipboardGetNewItemId( display ); 

    /* initialize fields in the data item
    */
    itemheader->thisItemId = loc_itemid;
    itemheader->adjunctData = 0;
    itemheader->recordType = XM_DATA_ITEM_RECORD_TYPE;
    itemheader->displayId = display;
    itemheader->windowId  = window;
    itemheader->dataItemLabelId = _XmClipboardGetNewItemId( display ); 
    itemheader->formatIdList = itemlength / CONVERT_32_FACTOR; /* offset */
    itemheader->formatCount = 0;
    itemheader->cancelledFormatCount = 0;
    itemheader->deletePendingFlag = 0;
    itemheader->permanentItemFlag = 0;
    itemheader->cutByNameFlag = 0;
    itemheader->cutByNameCallback = 0;
    itemheader->cutByNameWidget = 0;
    itemheader->cutByNameWindow = 0;

    if ( callback != 0 && widget != 0 )
    {
    	/* set up client message handling if widget passed
    	*/
	itemheader->cutByNameCallback = callback;
	itemheader->cutByNameWidget = widget;
	itemheader->cutByNameWindow = XtWindow( widget );
	_XmClipboardSetAccess( display, itemheader->cutByNameWindow );
    }
    
    /* store the label 
    */
    _XmClipboardReplaceItem( display, 
    			      itemheader->dataItemLabelId,
    			      (XtPointer)label,
    			      XmStringLength( label ),
    			      PropModeReplace,
			      8,
    			      False );

    /* return the item to the root window 
    */
    _XmClipboardReplaceItem( display, 
    			      loc_itemid,
    			      (XtPointer)itemheader,
    			      itemlength,
    			      PropModeReplace,
			      32,
    			      True );

    if ( itemid != 0 )
    {
        *itemid = (long)loc_itemid;
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardCopy( display, window, itemid, format, buffer, length, private_id, dataid )
        Display *display ;
        Window window ;
        long itemid ;
        char *format ;
        XtPointer buffer ;
        unsigned long length ;
        long private_id ;
        long *dataid ;
#else
XmClipboardCopy(
        Display *display,   /* Display id of application passing data */
        Window window,
        long itemid,        /* id returned from begin copy */
        char *format,       /* Name string for data format */
        XtPointer buffer,   /* Address of buffer holding data in this format */
        unsigned long length,   /* Length of the data */
        long private_id,     /* Private id provide by application */
        long *dataid )       /* Data id returned by clipboard */
#endif /* _NO_PROTO */
{
    ClipboardDataItem itemheader;
    ClipboardHeader header;
    ClipboardFormatItem formatptr;
    char *formatdataptr;
    itemId formatid, formatdataid, *idptr;
    char *to_ptr;
    int status, count, format_len ;
    unsigned long maxname, formatlength, itemlength, formatdatalength;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );
 
    if (!header->startCopyCalled) {
	_XmWarning(NULL, XM_CLIPBOARD_MESSAGE1);
        _XmClipboardUnlock( display, window, 0 );
        return ClipboardFail;
    } 

    /* first check to see if the format already exists for this item
    */
    formatptr = _XmClipboardFindFormat( display, header, format, 
    					   (itemId) itemid, 0,
    					   &maxname, &count, &formatlength );

    /* if format doesn't exist, then have to access the data item */
    /* record
    */
    if ( formatptr == 0 )
    {
	/* get a pointer to the item
	*/
	status = _XmClipboardRetrieveItem( display, 
				   (itemId)itemid,
				   sizeof( itemId ),
				   0,
				   (XtPointer *) &itemheader,
				   &itemlength,
				   0,
				   XM_DATA_ITEM_RECORD_TYPE,
				   0 );
	if (status != ClipboardSuccess)
	{
	    return (status);
	}

	itemheader->formatCount = itemheader->formatCount + 1;

	if ((itemheader->formatCount * 2 + 2) >= XM_ITEM_ID_INC) {
	    _XmWarning(NULL, XM_CLIPBOARD_MESSAGE3);
	    _XmClipboardFreeAlloc( (char *)itemheader);
	    _XmClipboardUnlock( display, window, 0 );
	    return ClipboardFail;
	}

	formatlength = sizeof( ClipboardFormatItemRec );

	/* allocate local storage for the data format record 
	*/ 
	formatptr     = (ClipboardFormatItem)_XmClipboardAlloc( (size_t) formatlength );

	formatid     = _XmClipboardGetNewItemId( display ); 
	formatdataid = _XmClipboardGetNewItemId( display ); 

	/* put format record id in data item's format id list
	*/
	idptr = (itemId*)AddAddresses( (XtPointer)itemheader, 
				       itemlength - sizeof( itemId ) ); 

	*idptr = formatid; 

	/* initialize the fields in the format record
	*/
	formatptr->recordType = XM_FORMAT_HEADER_TYPE;
	formatptr->formatNameAtom = XmInternAtom( display, format, False );
	formatptr->itemLength = 0;
	formatptr->formatNameLength = strlen( format );
	formatptr->formatDataId = formatdataid;
	formatptr->thisFormatId = formatid;
	formatptr->itemPrivateId = private_id;
	formatptr->cancelledFlag = 0;
	formatptr->copiedLength = 0;
	formatptr->parentItemId = itemid;
	formatptr->cutByNameWidget = itemheader->cutByNameWidget;
	formatptr->cutByNameWindow = itemheader->cutByNameWindow;
	formatptr->cutByNameCallback = itemheader->cutByNameCallback;
	formatptr->windowId = itemheader->windowId;
	formatptr->displayId = itemheader->displayId;

	/* if buffer is null then it is a pass by name
	*/
	if ( buffer != 0 )
	{
	    formatptr->cutByNameFlag = 0;
	    formatdatalength = length;
	}else{
	    itemheader->cutByNameFlag = 1;
	    formatptr->cutByNameFlag = 1;
	    formatdatalength = 4;  /* we want a property stored regardless */
	}    	

	/* return the property on the root window for the item
	*/
	_XmClipboardReplaceItem( display, 
				  itemid,
				  (XtPointer)itemheader,
				  itemlength,
				  PropModeReplace,
				  32,
				  True );


	if( _XmClipboardGetLenFromFormat( display, format, &format_len ) 
			== ClipboardFail)
	{
	    /* if it's one of the predefined formats, register it for second try
	    */
	    XmClipboardRegisterFormat( display, 
					format, 
					0 );

	    _XmClipboardGetLenFromFormat( display, format, &format_len );
	}

 
	formatdataptr = _XmClipboardAlloc( (size_t) formatdatalength );

        to_ptr = formatdataptr; 

    }else{
    	formatid = formatptr->thisFormatId;
    	formatdataid = formatptr->formatDataId;
    	
    	/* the format already existed so get the data and append
    	*/
    	_XmClipboardRetrieveItem( display, 
    			           formatdataid,
    				   (int) length,/* BAD NEWS: truncation here.*/
    				   0,
    			           (XtPointer *) &formatdataptr,
    			           &formatdatalength,
				   &format_len,
    				   0,
				   0 );

        to_ptr = (char *) AddAddresses(formatdataptr, formatdatalength-length ); 
    }

    if ( buffer != 0 )
    {
	/* copy the format data over to acquired storage
	*/
	memcpy( to_ptr, buffer, (size_t) length );
    }

    formatptr->itemLength = formatptr->itemLength + (int) length;

    /* replace the property on the root window for the format data
    */
    _XmClipboardReplaceItem( display, 
			      formatdataid,
			      formatdataptr,
			      formatdatalength,
    			      PropModeReplace,
			      format_len,  /* 8, 16, or 32 */
    			      True );
    
    /* replace the property on the root window for the format 
    */
    _XmClipboardReplaceItem( display, 
    			      formatid,
    			      (XtPointer)formatptr,
    			      formatlength,
    			      PropModeReplace,
			      32,
    			      True );

    if ( dataid != 0 )
    {
        *dataid = formatid;
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardEndCopy( display, window, itemid )
        Display *display ;
        Window window ;
        long itemid ;
#else
XmClipboardEndCopy(
        Display *display,
        Window window,
        long itemid )
#endif /* _NO_PROTO */
{
    ClipboardDataItem itemheader;
    ClipboardHeader header;
    itemId *itemlist;
    unsigned long itemlength ;
    long newitemoffset;
    itemId *newitemaddr;
    int status;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, sizeof( itemId ) );

    if (!header->startCopyCalled) {
	_XmWarning(NULL, XM_CLIPBOARD_MESSAGE2);
        _XmClipboardUnlock( display, window, 0 );
        return ClipboardFail;
    } 

    _XmClipboardDeleteMarked( display, window, header );

    if (header->currItems >= header->maxItems) 
    {
    	itemlist = (itemId*)AddAddresses((XtPointer) header,
				header->dataItemList * CONVERT_32_FACTOR);

    	/* mark least recent item for deletion and delete previously marked
    	*/
        _XmClipboardMarkItem( display, header, *itemlist, XM_DELETE );

    	header->deletedByCopyId = *itemlist;
    } else {
	header->deletedByCopyId = 0;
    }

    newitemoffset = header->dataItemList + 
    		    (header->currItems * sizeof( itemId ) / CONVERT_32_FACTOR);

    /* stick new item at the bottom of the list 
    */
    newitemaddr = (itemId*)AddAddresses((XtPointer) header,
					newitemoffset * CONVERT_32_FACTOR);

    *newitemaddr = (itemId)itemid;

    /* new items always become next paste item 
    */
    header->oldNextPasteItemId  = header->nextPasteItemId;
    header->nextPasteItemId = (itemId)itemid;
    header->lastCopyItemId = (itemId)itemid;

    header->currItems = header->currItems + 1;
    header->startCopyCalled = False;

    /* if there was a cut by name format, then set up event handling
    */
    _XmClipboardFindItem( display,
			   itemid,
			   (XtPointer *) &itemheader,
			   &itemlength,
			   0,
    			   XM_DATA_ITEM_RECORD_TYPE );

    if ( itemheader->cutByNameWidget != 0 )
    {
        EventMask event_mask = 0;

    	XtAddEventHandler( itemheader->cutByNameWidget, event_mask, TRUE, 
    			   _XmClipboardEventHandler, 0 );
    }

    _XmClipboardFreeAlloc( (char *) itemheader ); 

    _XmAssertClipboardSelection( display, window, header, 
				  header->selectionTimestamp );

    _XmClipboardSetNextItemId(display, itemid);

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardCancelCopy( display, window, itemid )
        Display *display ;
        Window window ;
        long itemid ;
#else
XmClipboardCancelCopy(
        Display *display,
        Window window,
        long itemid )   /* id returned by begin copy */
#endif /* _NO_PROTO */
{
    itemId deleteitemid;
    itemId previous;
    XtPointer int_ptr;
    unsigned long length;
    ClipboardHeader header;

    if ( _XmClipboardLock( display, window) == ClipboardLocked )
        return(ClipboardLocked);

    deleteitemid = (itemId)itemid;

/*
 * first, free up the properties set by the StartCopy and Copy calls
 */
     /* delete the item label
     */
     _XmClipboardDeleteItemLabel( display, window, deleteitemid);

    /* delete all the formats belonging to the data item
    */
    _XmClipboardDeleteFormats( display, window, deleteitemid );

    /* now delete the item itself
    */
    _XmClipboardDeleteId( display, deleteitemid );

 /*
  * reset the startCopyCalled flag and reset the XM_NEXT_ID property
  * it's value prior to StartCopy.
  */

    _XmClipboardFindItem( display,
			XM_NEXT_ID,
			&int_ptr,
			&length,
			0,
			0 );
    previous = itemid - 1;
    *(int *) int_ptr = (int) previous;
    length = sizeof( int );
    _XmClipboardReplaceItem( display,
			XM_NEXT_ID,
			int_ptr,
			length,
			PropModeReplace,
			32,
			True );

    header = _XmClipboardOpen( display, 0);
    header->startCopyCalled = False;
    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

	return(ClipboardSuccess);
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardWithdrawFormat( display, window, data )
        Display *display ;
        Window window ;
        long data ;
#else
XmClipboardWithdrawFormat(
        Display *display,
        Window window,
        long data )  /* data id of format no longer provided by application */
#endif /* _NO_PROTO */
{
    int status;

    status = _XmClipboardLock( display, window  );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    _XmClipboardDeleteFormat( display, data );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardCopyByName( display, window, data, buffer, length, private_id )
        Display *display ;
        Window window ;
        long data ;
        XtPointer buffer ;
        unsigned long length ;  
        long private_id ;
#else
XmClipboardCopyByName(
        Display *display,   /* Display id of application passing data */
        Window window,
        long data,          /* Data id returned previously by clipboard */
        XtPointer buffer,   /* Address of buffer holding data in this format */
        unsigned long length,   /* Length of the data */
        long private_id )    /* Private id provide by application */
#endif /* _NO_PROTO */
{
    ClipboardFormatItem formatheader;
    int format;
    char *formatdataptr;
    unsigned long formatlength, formatdatalength;
    char *to_ptr;
    int status, locked;
    unsigned long headerlength;
    ClipboardHeader root_clipboard_header;

    /* get the clipboard header
    */
    _XmClipboardFindItem( display, 
			   XM_HEADER_ID,
			   (XtPointer *) &root_clipboard_header,
			   &headerlength,
			   0,
    			   0 );

    locked = 0;

    /* if this is a recopy as the result of a callback, then circumvent */
    /* any existing lock
    */
    if ( root_clipboard_header->recopyId != data )
    {        
	status = _XmClipboardLock( display, window );
	if ( status == ClipboardLocked ) return ClipboardLocked;
    	locked = 1;

    }else{
	root_clipboard_header->recopyId = 0;

	/* replace the clipboard header 
	*/
	_XmClipboardReplaceItem( display, 
				  XM_HEADER_ID,
				  (XtPointer)root_clipboard_header,
				  headerlength,
				  PropModeReplace,
				  32,
				  False );
    }

    /* get a pointer to the format
    */
    if ( _XmClipboardFindItem( display, 
    				data,
    			 	(XtPointer *) &formatheader,
    				&formatlength,
				0,
    				XM_FORMAT_HEADER_TYPE ) == ClipboardSuccess )
    {
	formatheader->itemPrivateId = private_id;

	if (formatheader->cutByNameFlag)
	    formatheader->itemLength = (int) length;
	else
	    formatheader->itemLength = formatheader->itemLength + (int) length;

    	_XmClipboardRetrieveItem( display, 
    			           formatheader->formatDataId,
    				   (int) length,/* BAD NEWS: truncation here.*/
    				   0,
    			           (XtPointer *) &formatdataptr,
    			           &formatdatalength,
				   &format,
				   0,
    				   formatheader->cutByNameFlag ); 
					    /* if cut by  */
					    /* name, discard any old data */

    	formatheader->cutByNameFlag = 0;
    	
        to_ptr = (char *) AddAddresses(formatdataptr, formatdatalength-length ); 

	/* copy the format data over to acquired storage
	*/
	memcpy( to_ptr, buffer, (size_t) length );

	/* create the property on the root window for the format data
	*/
	_XmClipboardReplaceItem( display, 
				  formatheader->formatDataId,
				  formatdataptr,
				  formatdatalength,
    				  PropModeReplace,
				  format,
    				  True );

	/* change the property on the root window for the format item
	*/
	_XmClipboardReplaceItem( display, 
				  data,
				  (XtPointer)formatheader,
				  formatlength,
    				  PropModeReplace,
				  32,
    				  True );
    }

    if ( locked )
    {
        _XmClipboardUnlock( display, window, 0 );
    }

    _XmClipboardFreeAlloc( (char *) root_clipboard_header );

    return ClipboardSuccess;
}


/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardUndoCopy( display, window )
        Display *display ;
        Window window ;
#else
XmClipboardUndoCopy(
        Display *display,
        Window window )
#endif /* _NO_PROTO */
{
    ClipboardHeader header;
    ClipboardDataItem itemheader;
    unsigned long itemlength;
    itemId itemid;
    int status, undo_okay;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    /* note: second call to undo, undoes the first call */

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );

    itemid = header->lastCopyItemId;

    undo_okay = 0;

    if ( itemid == 0 )
    {
    	undo_okay = 1;

    }else{
	/* get the item 
	*/
	_XmClipboardFindItem( display,
			       itemid,
			       (XtPointer *) &itemheader,
			       &itemlength,
			       0,
    			       XM_DATA_ITEM_RECORD_TYPE );

	/* if no last copy item */
	/* or if the item's window or display don't match, then can't undo
	*/
	if ( itemheader->windowId == window  &&
	     itemheader->displayId == display ) 
	{
    	    undo_okay = 1;

	    /* mark last item for delete
	    */
    	    _XmClipboardMarkItem( display, header, itemid, XM_DELETE );
    	}

    	_XmClipboardFreeAlloc( (char*)itemheader );
    }

    if ( undo_okay )
    {
	/* fetch the item marked deleted by the last copy, if any
	*/
	itemid = header->deletedByCopyId;

	/* mark it undeleted
	*/
	_XmClipboardMarkItem( display, header, itemid, XM_UNDELETE );

	/* switch item marked deleted 
	*/
	header->deletedByCopyId = header->lastCopyItemId;
	header->lastCopyItemId = itemid;

	/* switch next paste and old next paste
	*/
	itemid = header->oldNextPasteItemId;
	header->oldNextPasteItemId = header->nextPasteItemId;
	header->nextPasteItemId = itemid;
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardLock( display, window )
        Display *display ;
        Window window ;
#else
XmClipboardLock(
        Display *display,
        Window window )     /* identifies application owning lock */
#endif /* _NO_PROTO */
{
    return _XmClipboardLock( display, window );
}


/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardUnlock( display, window, all_levels )
        Display *display ;
        Window window ;
        Boolean all_levels ;
#else
XmClipboardUnlock(
        Display *display,
        Window window,  /* specifies window owning lock, must match window */
#if NeedWidePrototypes  /* passed to clipboardlock */
        int all_levels )
#else
        Boolean all_levels )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    return _XmClipboardUnlock( display, window, all_levels );
}


/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardStartRetrieve( display, window, timestamp )
        Display *display ;
        Window window ;
        Time timestamp ;
#else
XmClipboardStartRetrieve(
        Display *display,       /* Display id of application wanting data */
        Window window,
        Time timestamp )
#endif /* _NO_PROTO */
{
    ClipboardHeader header;
    int status;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    header = _XmClipboardOpen( display, 0 );

    header->incrementalCopyFrom = True;
    header->copyFromTimestamp = timestamp;
    header->foreignCopiedLength = 0;

    _XmClipboardClose( display, header );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardEndRetrieve( display, window )
        Display *display ;
        Window window ;
#else
XmClipboardEndRetrieve(
        Display *display,   /* Display id of application wanting data */
        Window window )
#endif /* _NO_PROTO */
{
    ClipboardHeader header;

    header = _XmClipboardOpen( display, 0 );

    header->incrementalCopyFrom = False;
    header->copyFromTimestamp   = CurrentTime;

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardRetrieve( display, window, format, buffer, length, outlength, private_id )
        Display *display ;
        Window window ;
        char *format ;
        XtPointer buffer ;
        unsigned long length ;
        unsigned long *outlength ;
        long *private_id ;
#else
XmClipboardRetrieve(
        Display *display,        /* Display id of application wanting data */
        Window window,
        char *format,   /* Name string for data format */
        XtPointer buffer, /* Address of buffer to receive data in this format */
        unsigned long length,    /* Length of the data buffer */
        unsigned long *outlength,/* Length of the data transferred to buffer */
        long *private_id )   /* Private id provide by application */
#endif /* _NO_PROTO */
{
    ClipboardHeader header;
    ClipboardFormatItem matchformat;
    char *formatdata;
    unsigned long formatdatalength ;
    unsigned long matchformatlength;
    int truncate, count;
    unsigned long maxname ;
    itemId matchid;
    char *ptr;
    int status, dataok;
    unsigned long loc_outlength;
    itemId loc_private;
    unsigned long copiedlength, remaininglength;
    Time timestamp;
    
    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );
    timestamp = header->copyFromTimestamp;

    loc_outlength = 0;
    loc_private = 0;
    truncate = 0;
    dataok = 0;
    ptr = NULL;

    /* check to see if we need to reclaim the selection
    */
    _XmInitializeSelection( display, header, window, timestamp );

    /* get the data from clipboard or selection owner
    */
    if ( _XmWeOwnSelection( display, header ) )
    {
	/* we own the selection
	*/
	/* find the matching format for the next paste item
	*/
	matchformat = _XmClipboardFindFormat(display, header, format,
					     (itemId) NULL, 0,
					     &maxname, &count, 
					     &matchformatlength );
	if (matchformat != 0)
	{
	    dataok = 1;

	    matchid = matchformat->thisFormatId;

	    if ( matchformat->cutByNameFlag == 1 )
	    {
		/* passed by name 
		*/
		dataok = _XmClipboardRequestDataAndWait( display, 
							  window,
							  matchformat );
		if ( dataok )
		{
		    /* re-check out matchformat since it may have changed
		    */
		    _XmClipboardFreeAlloc( (char *) matchformat );

		    _XmClipboardFindItem( display,
					   matchid,
					   (XtPointer *) &matchformat,
					   &matchformatlength,
					   0,
					   XM_FORMAT_HEADER_TYPE );
		}
	    }

	    if ( dataok )
	    {
		_XmClipboardFindItem( display,
				       matchformat->formatDataId,
				       (XtPointer *) &formatdata,
				       &formatdatalength,
				       0,
				       0 );

		copiedlength = matchformat->copiedLength;
		ptr = formatdata + copiedlength;

		remaininglength = formatdatalength - copiedlength;

		if ( length < remaininglength )
		{
		    loc_outlength = length;
		    truncate = 1;
		}else{
		    loc_outlength = remaininglength;
		}

		if ( header->incrementalCopyFrom )
		{
		    /* update the copied length
		    */
		    if ( loc_outlength == remaininglength )
		    {
			/* we've copied everything, so reset
			*/
			matchformat->copiedLength = 0;
		    }else{
			matchformat->copiedLength = matchformat->copiedLength
						  + loc_outlength;
		    }
		}

		loc_private = matchformat->itemPrivateId;
	    }

	    _XmClipboardReplaceItem( display, 
				      matchid,
				      (XtPointer)matchformat,
				      matchformatlength,
				      PropModeReplace,
				      32,
				      True );
	}

    }else{

	/* we don't own the selection, get the data from selection owner
	*/
        if ( _XmClipboardGetSelection( display, window, format, header,
                                  (XtPointer *) &formatdata, &loc_outlength ) )
	{
	    /* we're okay
	    */
		dataok = 1;

		copiedlength = header->foreignCopiedLength;

		ptr = formatdata + copiedlength;

		remaininglength = loc_outlength - copiedlength;

		if ( length < remaininglength )
		{
		    loc_outlength = length;
		    truncate = 1;
		}else{
		    loc_outlength = remaininglength;
		}

		if ( header->incrementalCopyFrom )
		{
		    /* update the copied length
		    */
		    if ( loc_outlength == remaininglength )
		    {
			/* we've copied everything, so reset
			*/
			header->foreignCopiedLength = 0;

		    }else{

			header->foreignCopiedLength = 
						header->foreignCopiedLength
						  + loc_outlength;
		    }
		}
	    }
    }

    if ( dataok )
    {
	/* copy the data to the user buffer
	*/
	memcpy( buffer, ptr, (size_t) loc_outlength );

	_XmClipboardFreeAlloc( (char *) formatdata );
    }

    /* try to prevent access violation even if outlength is mandatory 
    */
    if ( outlength != 0 )
    {
    	*outlength = loc_outlength;
    }

    if ( private_id != 0 )
    {    	
	*private_id = loc_private;
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    if (truncate == 1) return ClipboardTruncate;
    if (dataok == 0)   return ClipboardNoData;    

    return ClipboardSuccess;
}

/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardInquireCount( display, window, count, maxlength )
        Display *display ;
        Window window ;
        int *count ;
        unsigned long *maxlength ;
#else
XmClipboardInquireCount(
        Display *display,
        Window window,
        int *count,     /* receives number of formats in next paste item */
        unsigned long *maxlength )/* receives max length of format names */
#endif /* _NO_PROTO */
{
    ClipboardHeader header;
    char *alloc_to_free;
    unsigned long loc_maxlength, loc_matchlength;
    unsigned long loc_count_len ;
    int status;
    int loc_count ;
    Time timestamp;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );

    /* If StartRetrieve wasn't called use latest Timestamp from server */

    if ( header->copyFromTimestamp == CurrentTime )
    {
        timestamp = _XmClipboardGetCurrentTime(display);
    }else{
        timestamp = header->copyFromTimestamp;
    }

    /* check to see if we need to reclaim the selection
    */
    _XmInitializeSelection( display, header, window, timestamp );
    loc_maxlength = 0;
    loc_count = 0;

    /* do we own the selection?
    */
    if ( _XmWeOwnSelection( display, header ) )
    {
	/* yes, find the next paste item, only looking for maxlength and count
	*/
	alloc_to_free = (char*)_XmClipboardFindFormat( display, header, 0, 
							(itemId) NULL, 0,
							&loc_maxlength, 
							&loc_count,
							&loc_matchlength );
    }else{
	/* we don't own the selection, get the data from selection owner
	*/
        if ( !_XmClipboardGetSelection( display, window, "TARGETS", header,
                               (XtPointer *) &alloc_to_free, &loc_count_len ) )
        {
	    _XmClipboardClose( display, header );
	    _XmClipboardUnlock( display, window, 0 );
            return ClipboardNoData;

        }else{

            /* we obtained a TARGETS type selection conversion
            */
            Atom *atomptr;
            int i;

            atomptr   = (Atom*)alloc_to_free;

            /* returned count is in bytes, targets are atoms of length 4 bytes
            */
            loc_count = (int) loc_count_len / 4;

	    /* max the lengths of all the atom names
	    */
	    for( i = 0; i < loc_count; i++ )
	    {
		    int temp;

		    if ((*atomptr) != (Atom)0)
		    {
			temp = strlen( XmGetAtomName( display, *atomptr ));

			if ( temp > loc_maxlength) 
			{
			    loc_maxlength = temp;
			}
		    }
		    atomptr++;
	     }
	}
    }	

    if ( maxlength != 0 )
    {
	/* user asked for max length of available format names
	*/
	*maxlength = loc_maxlength;
    }

    if ( count != 0 )
    {
	*count = loc_count;
    }

    if ( alloc_to_free != 0 )
    {
	_XmClipboardFreeAlloc( (char *) alloc_to_free ); 
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}


/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardInquireFormat( display, window, n, buffer, bufferlength, outlength )
        Display *display ;
        Window window ;
        int n ;
        XtPointer buffer ;
        unsigned long bufferlength ;
        unsigned long *outlength ;
#else
XmClipboardInquireFormat(
        Display *display,       /* Display id of application inquiring */
        Window window,
        int n,                  /* Which format for this data item? */
        XtPointer buffer,       /* Address of buffer to receive format name */
        unsigned long bufferlength, /* Length of the buffer */
        unsigned long *outlength )  /* Receives length copied to name buffer */
#endif /* _NO_PROTO */
{
    ClipboardHeader header;
    ClipboardFormatItem matchformat;
    char *alloc_to_free;
    char *ptr;
    int count;
    unsigned long loc_matchlength, maxname, loc_outlength ;
    int status;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    status = ClipboardSuccess;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );

    /* check to see if we need to reclaim the selection
    */
    _XmInitializeSelection( display, header, window, 
			     header->copyFromTimestamp );

    ptr = 0;
    loc_outlength = 0;

    /* do we own the selection?
    */
    if ( _XmWeOwnSelection( display, header ) )
    {
	/* retrieve the matching format 
	*/
	matchformat = _XmClipboardFindFormat(display, header, 0,
					     (itemId) NULL, n,
					     &maxname, &count, 
					     &loc_matchlength );

	if ( matchformat != 0 )
	{
	    ptr = XmGetAtomName( display, matchformat->formatNameAtom ); 

            _XmClipboardFreeAlloc( (char *) matchformat );
	}

    }else{
        /* we don't own the selection, get the data from selection owner
        */
        if ( !_XmClipboardGetSelection( display, window, "TARGETS", header,
                             (XtPointer *) &alloc_to_free, &loc_matchlength ) )
        {
            *outlength = 0;
	    _XmClipboardClose( display, header );
	    _XmClipboardUnlock( display, window, 0 );
            return ClipboardNoData;

        }else{
            /* we obtained a TARGETS type selection conversion
            */
            Atom *nth_atom;

            nth_atom = (Atom*)alloc_to_free;

            /* returned count is in bytes, targets are atoms of length 4 bytes
            */
            loc_matchlength = loc_matchlength / 4;

            if ( loc_matchlength >= n )
            {
                nth_atom = nth_atom + n - 1;

                ptr = XmGetAtomName( display, *nth_atom );

                _XmClipboardFreeAlloc( (char *) alloc_to_free );
            }
	}
    }

    if ( ptr != 0 )
    {
	loc_outlength = strlen( ptr );

	if ( loc_outlength > bufferlength )
	{
	    status = ClipboardTruncate;

	    loc_outlength = bufferlength;
	}
	strncpy( (char *) buffer, ptr, (unsigned) loc_outlength );
	/* loc_outlenght is truncated above.*/

	_XmClipboardFreeAlloc( (char *) ptr );
    }

    if ( outlength != 0 )
    {
    	*outlength = loc_outlength;
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return status;
}


/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardInquireLength( display, window, format, length )
        Display *display ;
        Window window ;
        char *format ;
        unsigned long *length ;
#else
XmClipboardInquireLength(
        Display *display,   /* Display id of application inquiring */
        Window window,
        char *format,       /* Name string for data format */
        unsigned long *length )/* Receives length of the data in that format */
#endif /* _NO_PROTO */
{

    ClipboardHeader header;
    ClipboardFormatItem matchformat;
    char *alloc_to_free;
    int count ;
    unsigned long loc_length, maxname, loc_matchlength;
    int status;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );

    /* check to see if we need to reclaim the selection
    */
    _XmInitializeSelection( display, header, window, 
			     header->copyFromTimestamp );

    loc_length = 0;

    /* do we own the selection?
    */
    if ( _XmWeOwnSelection( display, header ) )
    {
	/* retrieve the next paste item
	*/
	matchformat = _XmClipboardFindFormat(display, header, format,
					     (itemId) NULL, 0, &maxname,
					     &count, &loc_matchlength);

	/* return the length 
	*/
	if ( matchformat != 0 )
	{
	    loc_length = matchformat->itemLength;
	    _XmClipboardFreeAlloc( (char *) matchformat );
	}
    }else{
        /* we don't own the selection, get the data from selection owner
        */
        if ( !_XmClipboardGetSelection( display, window, format, header,
                                  (XtPointer *) &alloc_to_free, &loc_length ) )
        {
	    _XmClipboardClose( display, header );
	    _XmClipboardUnlock( display, window, 0 );
            return ClipboardNoData;

        }else{

            _XmClipboardFreeAlloc( (char *) alloc_to_free );
	}
    }

    if ( length != 0 )
    {
    	*length = loc_length;
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    return ClipboardSuccess;
}


/*---------------------------------------------*/
int 
#ifdef _NO_PROTO
XmClipboardInquirePendingItems( display, window, format, list, count )
        Display *display ;
        Window window ;
        char *format ;
        XmClipboardPendingList *list ;
        unsigned long *count ;
#else
XmClipboardInquirePendingItems(
        Display *display,       /* Display id of application passing data */
        Window window,
        char *format,           /* Name string for data format */
        XmClipboardPendingList *list,
        unsigned long *count )  /* Number of items in returned list */
#endif /* _NO_PROTO */
{
    ClipboardHeader header;
    ClipboardFormatItem matchformat;
    XmClipboardPendingList itemlist, nextlistptr;
    itemId *id_ptr;
    int loc_count, i;
    unsigned long maxname, loc_matchlength;
    int status;

    status = _XmClipboardLock( display, window );
    if ( status == ClipboardLocked ) return ClipboardLocked;

    if ( list == 0 )
    {
    	/* just get out to avoid access violation
    	*/
	_XmClipboardUnlock( display, window, 0 );
        return ClipboardSuccess;
    }

    *list = 0;
    loc_count = 0;

    /* get the clipboard header
    */
    header = _XmClipboardOpen( display, 0 );

    id_ptr = (itemId*)AddAddresses((XtPointer) header,
				   header->dataItemList * CONVERT_32_FACTOR);

    itemlist = (XmClipboardPendingList)_XmClipboardAlloc( (size_t)
    			(header->currItems * sizeof( XmClipboardPendingRec)));

    nextlistptr = itemlist;

    /* run through all the items in the clipboard looking for matching formats 
    */
    for ( i = 0; i < header->currItems; i++ )
    {
    	/* if it is marked for delete, skip it
    	*/
    	if ( _XmClipboardIsMarkedForDelete( display, header, *id_ptr ) )
    	{
    	    matchformat = 0;
    	}else{
    	    int dummy;

	    /* see if there is a matching format
	    */
	    matchformat = _XmClipboardFindFormat( display, header, 
						   format, *id_ptr, 
						   0, &maxname, &dummy,
    						   &loc_matchlength );
    	}

    	if ( matchformat != 0 )
    	{
    	    /* found matching format
    	    */
    	    if ( matchformat->cutByNameFlag == 1 )
    	    {
    		/* it was passed by name so is pending
    		*/
    	    	nextlistptr->DataId = matchformat->thisFormatId;    	    
    	    	nextlistptr->PrivateId = matchformat->itemPrivateId;

    		nextlistptr = nextlistptr + 1;
    		loc_count = loc_count + 1;
    	    }

    	    _XmClipboardFreeAlloc( (char *) matchformat );
    	}

    	id_ptr = id_ptr + 1;
    }

    _XmClipboardClose( display, header );

    _XmClipboardUnlock( display, window, 0 );

    if ( count != 0 )
    {
        *count = loc_count;
    }

    *list  = itemlist;

    return ClipboardSuccess;
}


/*---------------------------------------------*/
int
#ifdef _NO_PROTO
XmClipboardRegisterFormat( display, format_name, format_length )
        Display *display ;
        char *format_name ;
        int format_length ;
#else
XmClipboardRegisterFormat(
        Display *display,       /* Display id of application passing data */
        char *format_name,      /* Name string for data format            */
        int format_length )     /* Format length  8-16-32         */
#endif /* _NO_PROTO */
{
    if ( format_length != 0 && 
	 format_length != 8 && format_length != 16 && format_length != 32 )
    {
    	_XmClipboardError( CLIPBOARD_BAD_FORMAT, BAD_FORMAT );
	return ClipboardBadFormat;
    }

    if ( format_name == 0  ||  strlen( format_name ) == 0 )
    {
    	_XmClipboardError( CLIPBOARD_BAD_FORMAT, BAD_FORMAT_NON_NULL );
    }

    /* make sure predefined formats are registered */
    /* use dummy format as a test, if not found then register the rest
    */
    if ( format_length != 0 )
    {
	return _XmRegisterFormat( display, format_name, format_length );

    }else{
      /* caller asking to look through predefines for format name 
      */
      if (
	_XmRegIfMatch( display, format_name, "TARGETS",	XM_ATOM )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "MULTIPLE",	XM_ATOM_PAIR )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "TIMESTAMP",	XM_INTEGER )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "STRING",		XM_STRING )
	 )  return ClipboardSuccess;
      if (
        _XmRegIfMatch( display, format_name, "COMPOUND_TEXT", XM_COMPOUND_TEXT)
         )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "LIST_LENGTH",	XM_INTEGER )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "PIXMAP",		XM_DRAWABLE )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "DRAWABLE",	XM_DRAWABLE )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "BITMAP",		XM_BITMAP )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "FOREGROUND",	XM_PIXEL )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "BACKGROUND",	XM_PIXEL )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "COLORMAP",	XM_COLORMAP )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "ODIF",		XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "OWNER_OS",	XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "FILE_NAME",	XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "HOST_NAME",	XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "CHARACTER_POSITION", XM_SPAN )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "LINE_NUMBER",	XM_SPAN )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "COLUMN_NUMBER",	XM_SPAN )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "LENGTH",		XM_INTEGER )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "USER",		XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "PROCEDURE",	XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "MODULE",		XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "PROCESS",	XM_INTEGER )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "TASK",		XM_INTEGER )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "CLASS",		XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "NAME",		XM_TEXT )
	 )  return ClipboardSuccess;
      if (
	_XmRegIfMatch( display, format_name, "CLIENT_WINDOW",	XM_WINDOW )
	 )  return ClipboardSuccess;
    }

    return ClipboardFail;
}

