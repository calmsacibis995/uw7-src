#pragma ident	"@(#)m1.2libs:Xm/DragICC.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "DragICCI.h"
#include "MessagesI.h"
#include <Xm/DragC.h>
#include <Xm/DisplayP.h>
#include <X11/Xatom.h>
#include <Xm/AtomMgr.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MESSAGE1	catgets(Xm_catd,MS_DragICC,MSG_DRI_1,_XmMsgDragICC_0000)
#define MESSAGE2	catgets(Xm_catd,MS_DragBS, MSG_DRB_2,_XmMsgDragICC_0001)
#else
#define MESSAGE1	_XmMsgDragICC_0000
#define MESSAGE2	_XmMsgDragICC_0001
#endif


#define BUFFER_DATA	0
#define BUFFER_HEAP	1

#define NUM_HEADER_ARGS 0
#define MAXSTACK	1000

/*
 * We assume that there are only 64 possible messageTypes for each
 * clientMessage type that we use.  This allows us to only eat up a
 * single byte in the data area of the event.  The following tables
 * and routines are used to map back and forth between the client
 * message and the callback reasons
 */
typedef struct _ReasonTable {
    int			reason;	
    unsigned char	messageType;
} ReasonTable;

/***************************************************************

For improving response time in a Drag operation, OSF/Motif allows clients
to cache frequently needed data on window properties to reduce roundtrip
X server requests.

The data is stored on window properties of the motifDragWindow, a
persistent, override-redirect, InputOnly child window of the display's
default root window.  A client looks for the motifDragWindow id on the
"_MOTIF_DRAG_WINDOW" property of the display's default root window.
If the window id is NULL or invalid, a client may create the motifDragWindow.
The motifDragWindow is mapped but is not visible on the screen.


Three sets of data are stored on motifDragWindow properties:

        1. a list of atom names/value pairs,
        2. an atom table, and
        3. a targets list table.

The "_MOTIF_DRAG_ATOM_PAIRS" property contains a list of atoms
name/value pairs from the "_MOTIF_DRAG_ATOM_PAIRS" property on
motifDragWindow. If the property does not exist on motifDragWindow, a
client may create the list of atom name/value pairs and store it on the
property. The list created by the OSF/Motif toolkit includes most of the
atoms used by Xm.

If a client finds the "_MOTIF_DRAG_ATOM_PAIRS" property, it may read
that property and cache the atoms, avoiding multiple roundtrip
server requests to intern the atoms itself.

The "_MOTIF_DRAG_ATOMS" property on motifDragWindow contains an atoms
table, consisting of pairs of atoms and timestamps.  The atoms have been
interned once and are available for clients to use without repeated
roundtrip server requests to intern them.  A timestamp of 0 indicates
that the atom is available.  A nonzero timestamp indicates when the
atom was last allocated to a client.  The OSF/Motif toolkit initializes
this property to contains only atom "_MOTIF_ATOM_0" with timestamp 0.

A client requests an atom by trying to find an available atom in the
table.  If it succeeds it sets the atom's timestamp to the value
specified and returns the atom.  If no atom is available, a client
may add an atom to the table with the specified timestamp, updates the
"_MOTIF_DRAG_ATOMS" property on motifDragWindow, and use the new
atom.  These new atoms are named "_MOTIF_ATOM_n" where n is 1, 2, 3, etc.

The "_MOTIF_DRAG_TARGETS" property on motifDragWindow contains a
targets table, consisting of a sequence of target lists to be shared
among clients.  In other words, this property is a list of lists.
These target lists are sorted into ascending order to avoid
permutations.  By sharing the targets table, clients may pass target
lists between themselves by exchanging only the corresponding target
list index in the protocol messages.

The targets table on the "_MOTIF_DRAG_TARGETS" property initially
contain only two lists:

                { 0,            }, and
                { XA_STRING,    }

To add a target list to the table, a client must first sorts the
target list into ascending order, then search the targets table for a
match. If it finds a match, it may use the index of the matching
targets table entry in the drag protocol.  Otherwise, it adds the
sorted target list to the table, updates the "_MOTIF_DRAG_TARGETS"
property on motifDragWindow, and uses the index of the new targets
table entry.


******************************************************************/

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XmICCEventType GetMessageData() ;
static void SwapMessageData() ;

#else

static XmICCEventType GetMessageData( 
                        Display *display,
                        xmICCMessageStruct *xmessage,
                        XmICCCallbackStruct *callback) ;
static void SwapMessageData( 
                        xmICCMessageStruct *xmessage) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

externaldef(dragicc)
unsigned char _XmByteOrderChar = (char) 0;

static ReasonTable	reasonTable[] = {
    {	XmCR_TOP_LEVEL_ENTER,		XmTOP_LEVEL_ENTER	},
    {	XmCR_TOP_LEVEL_LEAVE,		XmTOP_LEVEL_LEAVE	},
    {	XmCR_DRAG_MOTION,		XmDRAG_MOTION		},
    {	XmCR_DROP_SITE_ENTER,		XmDROP_SITE_ENTER	},
    {	XmCR_DROP_SITE_LEAVE,		XmDROP_SITE_LEAVE	},
    {	XmCR_DROP_START,		XmDROP_START		},
    {	XmCR_DROP_FINISH,		XmDROP_FINISH		},
    {	XmCR_DRAG_DROP_FINISH,		XmDRAG_DROP_FINISH	},
    {	XmCR_OPERATION_CHANGED,		XmOPERATION_CHANGED	},
};

static int	messageTable[] = {
    XmCR_TOP_LEVEL_ENTER,	/* XmTOP_LEVEL_ENTER 	0 */
    XmCR_TOP_LEVEL_LEAVE,	/* XmTOP_LEVEL_LEAVE 	1 */
    XmCR_DRAG_MOTION,		/* XmDRAG_MOTION	2 */
    XmCR_DROP_SITE_ENTER,	/* XmDROP_SITE_ENTER	3 */
    XmCR_DROP_SITE_LEAVE,	/* XmDROP_SITE_LEAVE	4 */
    XmCR_DROP_START,		/* XmDROP_START		5 */
    XmCR_DROP_FINISH,		/* XmDROP_FINISH	6 */
    XmCR_DRAG_DROP_FINISH,	/* XmDRAG_DROP_FINISH	7 */
    XmCR_OPERATION_CHANGED,	/* XmOPERATION_CHANGED	8 */
};

/************************************************************************
 *
 *  _XmReasonToMessageType()
 *
 ***********************************************************************/

unsigned char 
#ifdef _NO_PROTO
_XmReasonToMessageType( reason )
        int reason ;
#else
_XmReasonToMessageType(
        int reason )
#endif /* _NO_PROTO */
{
    int	i;
    
    for (i = 0; i < XtNumber(reasonTable); i++)
      if (reasonTable[i].reason == reason)
	return((unsigned char)i);
    return 0xFF;
}

/************************************************************************
 *
 *  _XmMessageTypeToReason()
 *
 ***********************************************************************/

unsigned int 
#ifdef _NO_PROTO
_XmMessageTypeToReason( messageType )
        unsigned char messageType ;
#else
_XmMessageTypeToReason(
#if NeedWidePrototypes
        unsigned int messageType )
#else
        unsigned char messageType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    return(messageTable[messageType]);
}

/************************************************************************
 *
 *  _XmICCCallbackToICCEvent()
 *
 *  Reformat a motif callback into a client message
 ***********************************************************************/

void 
#ifdef _NO_PROTO
_XmICCCallbackToICCEvent( display, window, callback, cmev, type )
        Display *display ;
        Window window ;
        XmICCCallback callback ;
        XClientMessageEvent *cmev ;
	XmICCEventType type ;
#else
_XmICCCallbackToICCEvent(
        Display *display,
        Window window,
        XmICCCallback callback,
        XClientMessageEvent *cmev,
	XmICCEventType type )
#endif /* _NO_PROTO */
{
    xmICCMessageStruct	*xmessage = (xmICCMessage)&cmev->data.b[0];

    cmev->display = display;
    cmev->type = ClientMessage;
    cmev->serial = LastKnownRequestProcessed(display);
    cmev->send_event = True;
    cmev->window = window;
    cmev->format = 8;
    cmev->message_type = XmInternAtom(display,
		_Xm_MOTIF_DRAG_AND_DROP_MESSAGE, False);

    
    xmessage->any.byte_order = (BYTE) _XmByteOrderChar;
    xmessage->any.message_type = (BYTE)
      _XmReasonToMessageType(callback->any.reason);
    
    switch (callback->any.reason) {
      case XmCR_TOP_LEVEL_ENTER:
	/*
	 * this message goes to receiver
	 */
	{
	    register XmTopLevelEnterCallback cb =
	      (XmTopLevelEnterCallback)callback;
	    
	    xmessage->topLevelEnter.flags = (CARD16) 0;
	    xmessage->topLevelEnter.time  = (CARD32) cb->timeStamp;
	    xmessage->topLevelEnter.src_window = (CARD32) cb->window;
	    xmessage->topLevelEnter.icc_handle = (CARD32) cb->iccHandle;
	    /* load has_drop_sites */
	}
	break;
      case XmCR_TOP_LEVEL_LEAVE:
	/*
	 * this message goes to receiver
	 */
	{
	    register XmTopLevelLeaveCallback cb =
	      (XmTopLevelLeaveCallback)callback;
	    
	    xmessage->topLevelLeave.flags = (CARD16) 0;
	    xmessage->topLevelLeave.time  = (CARD32) cb->timeStamp;
	    xmessage->topLevelLeave.src_window = (CARD32) cb->window;
	}
	break;
      case XmCR_DRAG_MOTION:
	/*
	 * this message goes both ways
	 */
	{
	    register XmDragMotionCallback	cb =
	      (XmDragMotionCallback)callback;
	    
	    xmessage->dragMotion.flags = 0;
	    xmessage->dragMotion.flags |= PUT_SITE_STATUS(cb->dropSiteStatus);
	    xmessage->dragMotion.flags |= PUT_OPERATION(cb->operation);
	    xmessage->dragMotion.flags |= PUT_MULTIOPS(cb->operations);
	    xmessage->dragMotion.time   = (CARD32) cb->timeStamp;
	    xmessage->dragMotion.x	= (INT16) cb->x;
	    xmessage->dragMotion.y	= (INT16) cb->y;
	}
	break;
      case XmCR_OPERATION_CHANGED:
	/*
	 * this message goes both ways
	 */
	{
	    register XmOperationChangedCallback	cb =
	      (XmOperationChangedCallback)callback;
	    
	    xmessage->operationChanged.flags = 0;
	    xmessage->operationChanged.flags |=
			PUT_OPERATION(cb->operation);
	    xmessage->operationChanged.flags |=
			PUT_SITE_STATUS(cb->dropSiteStatus);
	    xmessage->operationChanged.flags |=
			PUT_MULTIOPS(cb->operations);
	    xmessage->operationChanged.time   = (CARD32) cb->timeStamp;
	}
	break;
      case XmCR_DROP_SITE_ENTER:
	/*
	 * this message goes to initiator
	 */
	{
	    register XmDropSiteEnterCallback	cb =
	      (XmDropSiteEnterCallback)callback;

	    /* invalid flags stuff ||| */
	    xmessage->dropSiteEnter.flags = 0;
	    xmessage->dropSiteEnter.flags |= PUT_OPERATION(cb->operation);
	    xmessage->dropSiteEnter.flags |= PUT_MULTIOPS(cb->operations);
	    xmessage->dropSiteEnter.flags |=
		PUT_SITE_STATUS(cb->dropSiteStatus);
	    xmessage->dropSiteEnter.time  = (CARD32) cb->timeStamp;
	    xmessage->dropSiteEnter.x = (INT16) cb->x;
	    xmessage->dropSiteEnter.y = (INT16) cb->y;
	}
	break;
      case XmCR_DROP_SITE_LEAVE:
	/*
	 * this message goes to initiator
	 */
	{
	    register XmDropSiteLeaveCallback	cb =
	      (XmDropSiteLeaveCallback)callback;

	    /* invalid flags stuff ||| */
	    xmessage->dropSiteLeave.flags = 0;
	    xmessage->dropSiteLeave.time  = (CARD32) cb->timeStamp;
	}
	break;
      case XmCR_DROP_START:
	/*
	 * this message goes to receiver
	 */
	{
	    register XmDropStartCallback	cb =
	      (XmDropStartCallback)callback;
	    
	    xmessage->drop.flags = 0;
#ifdef NON_OSF_FIX	/* OSF contact # 18398 */
            xmessage->drop.flags |= PUT_SITE_STATUS(cb->dropSiteStatus);
#endif
	    xmessage->drop.flags |= PUT_COMPLETION(cb->dropAction);
	    xmessage->drop.flags |= PUT_OPERATION(cb->operation);
	    xmessage->drop.flags |= PUT_MULTIOPS(cb->operations);
	    xmessage->drop.time  = (CARD32) cb->timeStamp;
	    xmessage->drop.x     = (INT16) cb->x;
	    xmessage->drop.y     = (INT16) cb->y;
	    xmessage->drop.icc_handle = (CARD32) cb->iccHandle;
	    xmessage->drop.src_window = (CARD32) cb->window;
	}
	break;
      default:
	break;
    }

    xmessage->any.message_type |= PUT_ICC_EVENT_TYPE(type);
}

/************************************************************************
 *
 *  _XmSendICCCallback()
 *
 ***********************************************************************/

void 
#ifdef _NO_PROTO
_XmSendICCCallback( display, window, callback, type )
        Display *display ;
        Window window ;
        XmICCCallback callback ;
	XmICCEventType type ;
#else
_XmSendICCCallback(
        Display *display,
        Window window,
        XmICCCallback callback,
	XmICCEventType type )
#endif /* _NO_PROTO */
{
	XClientMessageEvent	msgEvent;
	Window receiverWindow;
	XmDisplay dd = (XmDisplay) XmGetXmDisplay(display);

	_XmICCCallbackToICCEvent(display, window, callback,
		&msgEvent, type);

#ifdef NON_OSF_FIX	/* OSF contact # 18825 */
	if (((receiverWindow = dd->display.proxyWindow) == None)
	    || (type == XmICC_RECEIVER_EVENT)) /* always ACK to the src win */
#else
	if ((receiverWindow = dd->display.proxyWindow) == None)
#endif
		receiverWindow = window;

	XSendEvent(display, receiverWindow, False, 0,
		(XEvent *) &msgEvent); 
}

/************************************************************************
 *
 *  GetMessageData()
 *
 *  Message data has already been byte-swapped, if necessary.
 ***********************************************************************/

static XmICCEventType
#ifdef _NO_PROTO
GetMessageData( display, xmessage, callback )
        Display *display ;
        xmICCMessageStruct *xmessage ;
        XmICCCallbackStruct *callback ;
#else
GetMessageData(
        Display *display,
        xmICCMessageStruct *xmessage,
        XmICCCallbackStruct *callback )
#endif /* _NO_PROTO */
{
    XmICCEventType type;
	unsigned char message_type;

	message_type = xmessage->any.message_type;
    type = GET_ICC_EVENT_TYPE(message_type);
	message_type &= CLEAR_ICC_EVENT_TYPE;

    /* Load up the callback */
    callback->any.reason = (int)
      _XmMessageTypeToReason(message_type);
    callback->any.event = NULL;
    callback->any.timeStamp = (Time) xmessage->any.time;

    switch(message_type) {
      case XmTOP_LEVEL_ENTER:
	{
	    register XmTopLevelEnterCallback	cb =
	      (XmTopLevelEnterCallback)callback;
	    
	    cb->window = (Window) xmessage->topLevelEnter.src_window;
	    cb->iccHandle = (Atom) xmessage->topLevelEnter.icc_handle;
	}
	break;

      case XmTOP_LEVEL_LEAVE:
	{
	    register XmTopLevelLeaveCallback	cb =
	      (XmTopLevelLeaveCallback)callback;
	    
	    cb->window = (Window) xmessage->topLevelLeave.src_window;
	}
	break;

      case XmDRAG_MOTION:
	{
	    register XmDragMotionCallback	cb =
	      (XmDragMotionCallback)callback;
	    
	    cb->x = (Position) cvtINT16toShort(xmessage->dragMotion.x);
	    cb->y = (Position) cvtINT16toShort(xmessage->dragMotion.y);
	    cb->operation = (unsigned char)
		GET_OPERATION(xmessage->dragMotion.flags);
	    cb->operations = (unsigned char)
		GET_MULTIOPS(xmessage->dragMotion.flags);
	    cb->dropSiteStatus = (unsigned char)
		GET_SITE_STATUS(xmessage->dragMotion.flags);
	}
	break;
      case XmOPERATION_CHANGED:
	{
	    register XmOperationChangedCallback	cb =
	      (XmOperationChangedCallback)callback;

	    cb->operation = (unsigned char)
		GET_OPERATION(xmessage->dragMotion.flags);
	    cb->operations = (unsigned char)
		GET_MULTIOPS(xmessage->dragMotion.flags);
		cb->dropSiteStatus = (unsigned char)
		GET_SITE_STATUS(xmessage->dragMotion.flags);
	}
	break;

      case XmCR_DROP_SITE_ENTER:
	/*
	 * this message goes to initiator
	 */
	{
	    register XmDropSiteEnterCallback	cb =
	      (XmDropSiteEnterCallback)callback;
	    
	    cb->x = (Position) cvtINT16toShort(xmessage->dropSiteEnter.x);
	    cb->y = (Position) cvtINT16toShort(xmessage->dropSiteEnter.y);

	    cb->operation = (unsigned char)
		GET_OPERATION(xmessage->dropSiteEnter.flags);
	    cb->operations = (unsigned char)
		GET_MULTIOPS(xmessage->dropSiteEnter.flags);
	    cb->dropSiteStatus = (unsigned char)
		GET_SITE_STATUS(xmessage->dropSiteEnter.flags);
	}
	break;

      case XmCR_DROP_SITE_LEAVE:
	/*
	 * this message goes to initiator
	 */
	break;

      case XmCR_DROP_START:
	/*
	 * this message goes to receiver
	 */
	{
	    register XmDropStartCallback	cb =
	      (XmDropStartCallback)callback;
#ifdef NON_OSF_FIX	/* OSF contact # 18398 */
            cb->dropSiteStatus = (unsigned char) 
                GET_SITE_STATUS(xmessage->drop.flags);
#endif
	    cb->operation = (unsigned char)
		GET_OPERATION(xmessage->drop.flags);
	    cb->operations = (unsigned char)
		GET_MULTIOPS(xmessage->drop.flags);
	    cb->dropAction = (unsigned char)
		GET_COMPLETION(xmessage->drop.flags);
	    cb->x = (Position) cvtINT16toShort(xmessage->drop.x);
	    cb->y = (Position) cvtINT16toShort(xmessage->drop.y);
	    cb->iccHandle = (Atom) xmessage->drop.icc_handle;
	    cb->window = (Window) xmessage->drop.src_window;
	}
	break;

      default:
	_XmWarning ((Widget) XmGetXmDisplay (display), MESSAGE1);
	break;
    }

    return(type);
}

/************************************************************************
 *
 *  SwapMessageData()
 *
 ***********************************************************************/

static void 
#ifdef _NO_PROTO
SwapMessageData( xmessage )
        xmICCMessageStruct *xmessage ;
#else
SwapMessageData(
        xmICCMessageStruct *xmessage )
#endif /* _NO_PROTO */
{
    swap2bytes(xmessage->any.flags);
    swap4bytes(xmessage->any.time);

    switch(xmessage->any.message_type) {
      case XmTOP_LEVEL_ENTER:
	{
	    swap4bytes(xmessage->topLevelEnter.src_window);
	    swap4bytes(xmessage->topLevelEnter.icc_handle);
	}
	break;

      case XmTOP_LEVEL_LEAVE:
	{
	    swap4bytes(xmessage->topLevelLeave.src_window);
	}
	break;

      case XmDRAG_MOTION:
	{
	    swap2bytes(xmessage->dragMotion.x);
	    swap2bytes(xmessage->dragMotion.y);
	}
	break;

      case XmDROP_SITE_ENTER:
	{
	    swap2bytes(xmessage->dropSiteEnter.x);
	    swap2bytes(xmessage->dropSiteEnter.y);
	}
	break;

      case XmDROP_START:
	{
	    swap2bytes(xmessage->drop.x);
	    swap2bytes(xmessage->drop.y);
	    swap4bytes(xmessage->drop.icc_handle);
	    swap4bytes(xmessage->drop.src_window);
	}
	break;

      case XmDROP_SITE_LEAVE:
      default:
	break;
    }
}

/************************************************************************
 *
 *  _XmICCEventToICCCallback()
 *
 ***********************************************************************/

Boolean 
#ifdef _NO_PROTO
_XmICCEventToICCCallback( msgEv, callback, type )
        XClientMessageEvent *msgEv ;
        XmICCCallback callback ;
	XmICCEventType	type ;
#else
_XmICCEventToICCCallback(
        XClientMessageEvent *msgEv,
        XmICCCallback callback,
	XmICCEventType	type )
#endif /* _NO_PROTO */
{
    Atom		motif_dnd_message_atom;
    xmICCMessage	xmessage;

    if ((msgEv->type != ClientMessage) || (msgEv->format != 8))
      return (False);

    motif_dnd_message_atom = XmInternAtom(msgEv->display,
				      _Xm_MOTIF_DRAG_AND_DROP_MESSAGE, False);

    if (msgEv->message_type != motif_dnd_message_atom)
      return (False);

    xmessage = (xmICCMessage)&msgEv->data.b[0];
    
    if (xmessage->any.byte_order != _XmByteOrderChar) {
	/*
	 * swap it inplace and update the byte_order field so no one
	 * else will try to reswap it.  This could happen since we're
	 * probably being called out of an event Handler on a list
	 */
	SwapMessageData(xmessage);
	xmessage->any.byte_order = _XmByteOrderChar;
    }

    if (type == GetMessageData(msgEv->display, xmessage, callback))
	return(True);
    else
	return(False);
}

/************************************************************************
 *
 *  _XmReadDragBuffer()
 *
 ***********************************************************************/

CARD16 
#ifdef _NO_PROTO
_XmReadDragBuffer( propBuf, which, ptr, size )
        xmPropertyBuffer propBuf ;
        BYTE which ;
        BYTE *ptr ;
        CARD32 size ;
#else
_XmReadDragBuffer(
        xmPropertyBuffer propBuf,
#if NeedWidePrototypes
        int which,
#else
        BYTE which,
#endif /* NeedWidePrototypes */
        BYTE *ptr,
        CARD32 size )
#endif /* _NO_PROTO */
{
    xmByteBufRec  *buf;
    CARD32	numCurr;

    if (which == BUFFER_DATA) /* data */
      buf = &propBuf->data;
    else
      buf = &propBuf->heap;
    
    numCurr = (CARD32)(buf->curr - buf->bytes);
    if (numCurr + size > buf->size) {
	size = buf->size - numCurr;
    }
    memcpy(ptr, buf->curr, (unsigned int)size);
    buf->curr += size;
    return size;
}

/************************************************************************
 *
 *  _XmWriteDragBuffer()
 *
 ***********************************************************************/

CARD16 
#ifdef _NO_PROTO
_XmWriteDragBuffer( propBuf, which, ptr, size )
        xmPropertyBuffer propBuf ;
        BYTE which ;
        BYTE *ptr ;
        CARD32 size ;
#else
_XmWriteDragBuffer(
        xmPropertyBuffer propBuf,
#if NeedWidePrototypes
        int which,
#else
        BYTE which,
#endif /* NeedWidePrototypes */
        BYTE *ptr,
        CARD32 size )
#endif /* _NO_PROTO */
{
    CARD16	returnVal;
    xmByteBufRec  *buf;

    if (which == BUFFER_DATA) /* data */
      buf = &propBuf->data;
    else
      buf = &propBuf->heap;

    if (buf->size + size > buf->max) {
       buf->max += 1000;
       if (buf->bytes == buf->stack) {
	  buf->bytes = (BYTE*)XtMalloc(buf->max);
	  memcpy(buf->bytes, buf->stack, buf->size);
       }
       else {
	  buf->bytes = (BYTE*)XtRealloc((char *)buf->bytes, (Cardinal)buf->max);
       }
    }
    memcpy(buf->bytes + buf->size, ptr, (unsigned int)size);
    returnVal = buf->size;
    buf->size += (size_t) size;
    return returnVal;
}

/************************************************************************
 *
 *  _XmWriteInitiatorInfo()
 *
 ***********************************************************************/

void 
#ifdef _NO_PROTO
_XmWriteInitiatorInfo( dc )
        Widget dc ;
#else
_XmWriteInitiatorInfo(
        Widget dc )
#endif /* _NO_PROTO */
{
    BYTE		stackData[MAXSTACK];
    BYTE		stackHeap[MAXSTACK];
    xmPropertyBufferRec	propBufRec;
    xmDragInitiatorInfoStruct	infoRec;
    Atom		initiatorAtom;
    Cardinal		i = 0;
    Window		srcWindow;
    Arg			args[8];
    XmDisplay		xmDisplay = (XmDisplay)XtParent(dc);
    Atom		*exportTargets;
    Cardinal		numExportTargets;
    Atom		iccHandle;

    XtSetArg(args[i], XmNexportTargets, &exportTargets);i++;
    XtSetArg(args[i], XmNnumExportTargets, &numExportTargets);i++;
    XtSetArg(args[i], XmNsourceWindow, &srcWindow);i++;
    XtSetArg(args[i], XmNiccHandle, &iccHandle);i++;
    XtGetValues(dc, args, i);

    propBufRec.data.stack = 
      propBufRec.data.bytes = stackData;
    propBufRec.data.size = 0;
    propBufRec.data.max = MAXSTACK;
    propBufRec.heap.stack = 
      propBufRec.heap.bytes = stackHeap;
    propBufRec.heap.size = 0;
    propBufRec.heap.max = MAXSTACK;

    infoRec.byte_order = _XmByteOrderChar;
    infoRec.protocol_version = _MOTIF_DRAG_PROTOCOL_VERSION;
    infoRec.targets_index = (CARD16)
      _XmTargetsToIndex((Widget)xmDisplay, exportTargets, numExportTargets);
    infoRec.icc_handle = iccHandle;

    _XmWriteDragBuffer (&propBufRec, BUFFER_DATA, (BYTE*)&infoRec,
		        sizeof(xmDragInitiatorInfoStruct));

    initiatorAtom = XmInternAtom(XtDisplayOfObject(dc),
				 XS_MOTIF_INITIATOR,
				 False);

    /* write the buffer to the property */
    XChangeProperty (XtDisplayOfObject(dc), 
		     srcWindow,
		     iccHandle, initiatorAtom,
		     8, PropModeReplace, 
		     (unsigned char *)propBufRec.data.bytes,
		     propBufRec.data.size);
    if (propBufRec.data.bytes != propBufRec.data.stack)
      XtFree((char *)propBufRec.data.bytes);

    if (propBufRec.heap.size) {
	XChangeProperty (XtDisplayOfObject(dc),
			 srcWindow,
			 initiatorAtom, initiatorAtom,
			 8, PropModeAppend, 
			 (unsigned char *)propBufRec.heap.bytes,
			 propBufRec.heap.size);
	if (propBufRec.heap.bytes != propBufRec.heap.stack)
	  XtFree((char *)propBufRec.heap.bytes);
    }
}

/************************************************************************
 *
 *  _XmReadInitiatorInfo()
 *
 *  We assume that the dc has been initialized enough to have it's
 *  source window field set.
 ***********************************************************************/

void 
#ifdef _NO_PROTO
_XmReadInitiatorInfo( dc )
        Widget dc ;
#else
_XmReadInitiatorInfo(
        Widget dc )
#endif /* _NO_PROTO */
{
    xmDragInitiatorInfoStruct	*info = NULL ;
    Atom			initiatorAtom;
    int				format;
    unsigned long 		bytesafter, lengthRtn;
    long 			length; 
    Atom			type;
    Arg				args[4];
    Cardinal			i;
    Window			srcWindow;
    Atom			iccHandle;
    Atom			*exportTargets;
    Cardinal			numExportTargets;
    
    
    i = 0;
    XtSetArg(args[i], XmNsourceWindow, &srcWindow);i++;
    XtSetArg(args[i], XmNiccHandle, &iccHandle);i++;
    
    XtGetValues(dc, args, i);
    
    initiatorAtom = XmInternAtom(XtDisplayOfObject(dc),
				 XS_MOTIF_INITIATOR,
				 FALSE);
    length = 100000L;
    if (XGetWindowProperty (XtDisplayOfObject(dc), 
			     srcWindow,
			     iccHandle,
			     0L,
			     length,
			     False,
			     initiatorAtom,
			     &type,
			     &format,
			     &lengthRtn,
			     &bytesafter,
			     (unsigned char **) &info) == Success)
      {
	if(lengthRtn >= sizeof(xmDragInitiatorInfoStruct))
	  {
	    if (info->byte_order != _XmByteOrderChar) {
	      swap2bytes(info->targets_index);
	      swap4bytes(info->icc_handle);
	    }
	    numExportTargets = 
	      _XmIndexToTargets(dc, info->targets_index, &exportTargets);
	
	    i = 0;
	    XtSetArg(args[i], XmNexportTargets, exportTargets);i++;
	    XtSetArg(args[i], XmNnumExportTargets, numExportTargets);i++;
	    XtSetValues(dc, args, i);
	  }
        /* free the memory that Xlib passed us */
	if(    info != NULL    )
	  {
	    XFree( (void *)info) ;
	  }
      }
}

/************************************************************************
 *
 *  _XmGetDragReceiverInfo()
 *
 *  The caller is responsible for freeing (using XFree) the dataRtn
 *  pointer that passes thru the memory allocated by XGetWindowProperty.
 ***********************************************************************/

Boolean 
#ifdef _NO_PROTO
_XmGetDragReceiverInfo( display, window, receiverInfoRtn )
        Display *display ;
        Window window ;
        XmDragReceiverInfoStruct *receiverInfoRtn ;
#else
_XmGetDragReceiverInfo(
        Display *display,
        Window window,
        XmDragReceiverInfoStruct *receiverInfoRtn )
#endif /* _NO_PROTO */
{
    xmDragReceiverInfoStruct	*iccInfo = NULL ;
    Atom			drag_hints_atom;
    int				format;
    unsigned long 		bytesafter, lengthRtn, length; 
    Atom			type;
    XmReceiverDSTreeStruct	*dsmInfo;
    Window			root;
    unsigned int		bw;
    XmDisplay           dd = (XmDisplay) XmGetXmDisplay(display);
    
    drag_hints_atom = XmInternAtom(display,
				   XS_MOTIF_RECEIVER,
				   FALSE);
    length = 100000L;
    if (XGetWindowProperty(display,
			    window,
			    drag_hints_atom,
			    0L,
			    length,
			    False,
			    drag_hints_atom,
			    &type,
			    &format,
			    &lengthRtn,
			    &bytesafter,
			    (unsigned char **) &iccInfo) == Success)
      {
	if (lengthRtn >= sizeof(xmDragReceiverInfoStruct)) {
	  if (iccInfo->protocol_version != _MOTIF_DRAG_PROTOCOL_VERSION)
	    {
	      _XmWarning ((Widget) XmGetXmDisplay (display), MESSAGE2);
	    }
	  if (iccInfo->byte_order != _XmByteOrderChar) {
	    swap2bytes(iccInfo->num_drop_sites);
	    swap4bytes(iccInfo->proxy_window);
	    swap4bytes(iccInfo->heap_offset);
	  }

	  dd->display.proxyWindow = iccInfo->proxy_window;
	
	  (receiverInfoRtn)->dragProtocolStyle = iccInfo->drag_protocol_style;
	
	  dsmInfo = XtNew(XmReceiverDSTreeStruct);
	  dsmInfo->byteOrder = iccInfo->byte_order;
	  dsmInfo->numDropSites = iccInfo->num_drop_sites;
	  dsmInfo->currDropSite = 0;
	  dsmInfo->propBufRec.data.bytes = (BYTE*)iccInfo;
	  dsmInfo->propBufRec.data.size = (size_t) iccInfo->heap_offset;
	  dsmInfo->propBufRec.heap.bytes = (BYTE*)
	    (iccInfo->heap_offset + (CARD32)iccInfo);
	  dsmInfo->propBufRec.heap.size = (size_t)
	    (lengthRtn - iccInfo->heap_offset);
	  /* 
	   * skip over the info that we've already got
	   */
	  dsmInfo->propBufRec.data.curr = 	
	    (BYTE*)iccInfo + sizeof(xmDragReceiverInfoStruct);
	  /*
	   * now get their geometry
	   */
	  XGetGeometry(display, 
		       window,
		       &root,
		       &(receiverInfoRtn->xOrigin),
		       &(receiverInfoRtn->yOrigin),
		       &(receiverInfoRtn->width),
		       &(receiverInfoRtn->height),
		       &bw,
		       &(receiverInfoRtn->depth));
	  (void) XTranslateCoordinates(display,
				     window,
				     root,
				     (int) -bw,
				     (int) -bw,
				     &(receiverInfoRtn->xOrigin), 
				     &(receiverInfoRtn->yOrigin), 
				     &root);
	  (receiverInfoRtn)->iccInfo = (XtPointer) dsmInfo;
	  return True;
	}
	else {
	  (receiverInfoRtn)->dragProtocolStyle = XmDRAG_NONE;
	  if (iccInfo)
	    XFree((void *)iccInfo);
	  return False;
	}
      }
      else 
	return False;
  }

/************************************************************************
 *
 *  _XmReadDSFromStream()
 *
 ***********************************************************************/
/*ARGSUSED*/
Boolean 
#ifdef _NO_PROTO
_XmReadDSFromStream( dsm, iccInfo, dropSiteInfoRtn )
        XmDropSiteManagerObject dsm ;
        XtPointer iccInfo ;
        XmICCDropSiteInfo dropSiteInfoRtn ;
#else
_XmReadDSFromStream(
        XmDropSiteManagerObject dsm,
        XtPointer iccInfo,
        XmICCDropSiteInfo dropSiteInfoRtn )
#endif /* _NO_PROTO */
{
    xmDSHeaderStruct	dsHeader;
    XmReceiverDSTree	dsmInfo = (XmReceiverDSTree)iccInfo;
    xmPropertyBufferRec	*propBuf = &dsmInfo->propBufRec;
    Cardinal		i;
    xmICCRegBoxRec	box;
    XmRegion		region;

    _XmReadDragBuffer (propBuf, BUFFER_DATA,
		       (BYTE*)&dsHeader, sizeof(xmDSHeaderStruct));
    
    if (dsmInfo->byteOrder != _XmByteOrderChar) {
	swap2bytes(dsHeader.flags);
	swap2bytes(dsHeader.import_targets_id);
	swap4bytes(dsHeader.dsRegionNumBoxes);
    }
    dropSiteInfoRtn->header.traversalType =
	(unsigned char) GET_TRAVERSAL_TYPE(dsHeader.flags);
    dropSiteInfoRtn->header.dropActivity =
	(unsigned char) GET_DS_ACTIVITY(dsHeader.flags);
    dropSiteInfoRtn->header.dropType =
	(unsigned char) GET_DS_TYPE(dsHeader.flags);
    dropSiteInfoRtn->header.operations =
	(unsigned char) GET_MULTIOPS(dsHeader.flags);
    dropSiteInfoRtn->header.animationStyle =
	(unsigned char) GET_ANIMATION_STYLE(dsHeader.flags);
    dropSiteInfoRtn->header.importTargetsID =
	(unsigned short) dsHeader.import_targets_id;

    switch (dropSiteInfoRtn->header.animationStyle) {

	case XmDRAG_UNDER_HIGHLIGHT:
	    {
		XmICCDropSiteHighlight	info =
		    (XmICCDropSiteHighlight) dropSiteInfoRtn;
		xmDSHighlightDataStruct	dsHighlight;

		_XmReadDragBuffer (propBuf, BUFFER_DATA,
		                   (BYTE*)&dsHighlight,
				   sizeof(xmDSHighlightDataStruct));
		if (dsmInfo->byteOrder != _XmByteOrderChar) {
		    swap2bytes(dsHighlight.borderWidth);
		    swap2bytes(dsHighlight.highlightThickness);
		    swap4bytes(dsHighlight.background);
		    swap4bytes(dsHighlight.highlightColor);
		    swap4bytes(dsHighlight.highlightPixmap);
		}

                info->animation_data.borderWidth =
		    (Dimension) dsHighlight.borderWidth;
		info->animation_data.highlightThickness =
		    (Dimension) dsHighlight.highlightThickness;

                info->animation_data.background =
		    (Pixel) dsHighlight.background;
		info->animation_data.highlightColor =
		    (Pixel) dsHighlight.highlightColor;
		info->animation_data.highlightPixmap =
		    (Pixmap) dsHighlight.highlightPixmap;
	    }
	    break;

	case XmDRAG_UNDER_SHADOW_IN:
	case XmDRAG_UNDER_SHADOW_OUT:
	    {
		XmICCDropSiteShadow	info =
		    (XmICCDropSiteShadow) dropSiteInfoRtn;
		xmDSShadowDataStruct	dsShadow;

		_XmReadDragBuffer (propBuf, BUFFER_DATA,
		                   (BYTE*)&dsShadow,
				   sizeof(xmDSShadowDataStruct));
		if (dsmInfo->byteOrder != _XmByteOrderChar) {
		    swap2bytes(dsShadow.borderWidth);
		    swap2bytes(dsShadow.highlightThickness);
		    swap2bytes(dsShadow.shadowThickness);
		    swap4bytes(dsShadow.foreground);
		    swap4bytes(dsShadow.topShadowColor);
		    swap4bytes(dsShadow.bottomShadowColor);
		    swap4bytes(dsShadow.topShadowPixmap);
		    swap4bytes(dsShadow.bottomShadowPixmap);
		}

                info->animation_data.borderWidth =
		    (Dimension) dsShadow.borderWidth;
		info->animation_data.highlightThickness =
		    (Dimension) dsShadow.highlightThickness;
		info->animation_data.shadowThickness =
		    (Dimension) dsShadow.shadowThickness;

                info->animation_data.foreground =
		    (Pixel) dsShadow.foreground;
		info->animation_data.topShadowColor =
		    (Pixel) dsShadow.topShadowColor;
		info->animation_data.bottomShadowColor =
		    (Pixel) dsShadow.bottomShadowColor;
		info->animation_data.topShadowPixmap =
		    (Pixmap) dsShadow.topShadowPixmap;
		info->animation_data.bottomShadowPixmap =
		    (Pixmap) dsShadow.bottomShadowPixmap;
	    }
	    break;

	case XmDRAG_UNDER_PIXMAP:
	    {
		XmICCDropSitePixmap	info =
		    (XmICCDropSitePixmap) dropSiteInfoRtn;
		xmDSPixmapDataStruct	dsPixmap;

		_XmReadDragBuffer (propBuf, BUFFER_DATA,
		                   (BYTE*)&dsPixmap,
				   sizeof(xmDSPixmapDataStruct));
		if (dsmInfo->byteOrder != _XmByteOrderChar) {
		    swap2bytes(dsPixmap.borderWidth);
		    swap2bytes(dsPixmap.highlightThickness);
		    swap2bytes(dsPixmap.shadowThickness);
		    swap2bytes(dsPixmap.animationPixmapDepth);
		    swap4bytes(dsPixmap.foreground);
		    swap4bytes(dsPixmap.background);
		    swap4bytes(dsPixmap.animationPixmap);
		    swap4bytes(dsPixmap.animationMask);
		}

                info->animation_data.borderWidth =
		    (Dimension) dsPixmap.borderWidth;
		info->animation_data.highlightThickness =
		    (Dimension) dsPixmap.highlightThickness;
		info->animation_data.shadowThickness =
		    (Dimension) dsPixmap.shadowThickness;
		info->animation_data.animationPixmapDepth =
		    (Cardinal) dsPixmap.animationPixmapDepth;

                info->animation_data.foreground =
		    (Pixel) dsPixmap.foreground;
                info->animation_data.background =
		    (Pixel) dsPixmap.background;
		info->animation_data.animationPixmap =
		    (Pixmap) dsPixmap.animationPixmap;
		info->animation_data.animationMask =
		    (Pixmap) dsPixmap.animationMask;
	    }
	    break;

	case XmDRAG_UNDER_NONE:
	    {
		XmICCDropSiteNone	info =
		    (XmICCDropSiteNone) dropSiteInfoRtn;
		xmDSNoneDataStruct	dsNone;

		_XmReadDragBuffer (propBuf, BUFFER_DATA,
		                   (BYTE*)&dsNone,
				   sizeof(xmDSNoneDataStruct));
		if (dsmInfo->byteOrder != _XmByteOrderChar) {
		    swap2bytes(dsNone.borderWidth);
		}

                info->animation_data.borderWidth =
		    (Dimension) dsNone.borderWidth;
	    }
	    break;
	default:
	    break;
    }

    /*
     *  Read the region, byte swapping if necessary.
     */

    region = dropSiteInfoRtn->header.region =
	_XmRegionCreateSize ((long) dsHeader.dsRegionNumBoxes);

    for (i = 0; i < (long) dsHeader.dsRegionNumBoxes; i++) {
	_XmReadDragBuffer(propBuf, BUFFER_DATA, (BYTE*) &box,
		              sizeof(xmICCRegBoxRec));
	if (dsmInfo->byteOrder != _XmByteOrderChar) {
	    swap2bytes(box.x1);
	    swap2bytes(box.x2);
	    swap2bytes(box.y1);
	    swap2bytes(box.y2);
	}
        region->rects[i].x1 = (short) cvtINT16toShort(box.x1);
        region->rects[i].x2 = (short) cvtINT16toShort(box.x2);
        region->rects[i].y1 = (short) cvtINT16toShort(box.y1);
        region->rects[i].y2 = (short) cvtINT16toShort(box.y2);
    }
    region->numRects = (long) dsHeader.dsRegionNumBoxes;
    _XmRegionComputeExtents (region);

    if (++dsmInfo->currDropSite == dsmInfo->numDropSites) {
	/* free all the wire data */
	XtFree((char *)dsmInfo->propBufRec.data.bytes);
	XtFree((char *)dsmInfo);
#ifdef DEBUG
	printf("freed the dsmInfo, all done\n");
#endif
    }
    return True;
}

/************************************************************************
 *
 *  _XmWriteDSToStream()
 *
 ***********************************************************************/
/*ARGSUSED*/
void 
#ifdef _NO_PROTO
_XmWriteDSToStream( dsm, stream, dropSiteInfo )
        XmDropSiteManagerObject dsm ;
        XtPointer stream ;
        XmICCDropSiteInfo dropSiteInfo ;
#else
_XmWriteDSToStream(
        XmDropSiteManagerObject dsm,
        XtPointer stream,
        XmICCDropSiteInfo dropSiteInfo )
#endif /* _NO_PROTO */
{
    xmDSHeaderStruct	dsHeader;
    XmReceiverDSTree	dsmInfo = (XmReceiverDSTree)stream;
    xmPropertyBufferRec	*propBuf = &dsmInfo->propBufRec;
    Cardinal		i;
    xmICCRegBoxRec	box;
    XmRegion		region = dropSiteInfo->header.region;

    dsHeader.flags = 0;
    dsHeader.flags |= PUT_TRAVERSAL_TYPE(dropSiteInfo->header.traversalType);
    dsHeader.flags |= PUT_DS_ACTIVITY(dropSiteInfo->header.dropActivity);
    dsHeader.flags |= PUT_DS_TYPE(dropSiteInfo->header.dropType);
    dsHeader.flags |= PUT_MULTIOPS(dropSiteInfo->header.operations);
    dsHeader.flags |= PUT_ANIMATION_STYLE(dropSiteInfo->header.animationStyle);
    dsHeader.import_targets_id = (CARD16) dropSiteInfo->header.importTargetsID;
    dsHeader.dsRegionNumBoxes = (CARD32) region->numRects;
    
    _XmWriteDragBuffer(propBuf, BUFFER_DATA, (BYTE*)&dsHeader,
		       sizeof(xmDSHeaderStruct));

    switch (dropSiteInfo->header.animationStyle) {

	case XmDRAG_UNDER_HIGHLIGHT:
	    {
		XmICCDropSiteHighlight	info =
		    (XmICCDropSiteHighlight) dropSiteInfo;
		xmDSHighlightDataStruct	dsHighlight;

                dsHighlight.borderWidth =
		    (CARD16) info->animation_data.borderWidth;
                dsHighlight.highlightThickness =
		    (CARD16) info->animation_data.highlightThickness;

                dsHighlight.background =
		    (CARD32) info->animation_data.background;
                dsHighlight.highlightColor =
		    (CARD32) info->animation_data.highlightColor;
                dsHighlight.highlightPixmap = 
		    (CARD32) info->animation_data.highlightPixmap;
    
		_XmWriteDragBuffer (propBuf, BUFFER_DATA,
				    (BYTE*)&dsHighlight,
		                    sizeof(xmDSHighlightDataStruct));
	    }
	    break;

	case XmDRAG_UNDER_SHADOW_IN:
	case XmDRAG_UNDER_SHADOW_OUT:
	    {
		XmICCDropSiteShadow	info =
		    (XmICCDropSiteShadow) dropSiteInfo;
		xmDSShadowDataStruct	dsShadow;

                dsShadow.borderWidth =
		    (CARD16) info->animation_data.borderWidth;
                dsShadow.highlightThickness =
		    (CARD16) info->animation_data.highlightThickness;
                dsShadow.shadowThickness =
		    (CARD16) info->animation_data.shadowThickness;

                dsShadow.foreground =
		    (CARD32) info->animation_data.foreground;
                dsShadow.topShadowColor =
		    (CARD32) info->animation_data.topShadowColor;
                dsShadow.bottomShadowColor =
		    (CARD32) info->animation_data.bottomShadowColor;
                dsShadow.topShadowPixmap = 
		    (CARD32) info->animation_data.topShadowPixmap;
                dsShadow.bottomShadowPixmap = 
		    (CARD32) info->animation_data.bottomShadowPixmap;
    
		_XmWriteDragBuffer (propBuf, BUFFER_DATA,
				    (BYTE*)&dsShadow,
		                    sizeof(xmDSShadowDataStruct));
	    }
	    break;

	case XmDRAG_UNDER_PIXMAP:
	    {
		XmICCDropSitePixmap	info =
		    (XmICCDropSitePixmap) dropSiteInfo;
		xmDSPixmapDataStruct	dsPixmap;

                dsPixmap.borderWidth =
		    (CARD16) info->animation_data.borderWidth;
                dsPixmap.highlightThickness =
		    (CARD16) info->animation_data.highlightThickness;
                dsPixmap.shadowThickness =
		    (CARD16) info->animation_data.shadowThickness;
                dsPixmap.animationPixmapDepth = 
		    (CARD16) info->animation_data.animationPixmapDepth;

                dsPixmap.foreground =
		    (CARD32) info->animation_data.foreground;
                dsPixmap.background =
		    (CARD32) info->animation_data.background;
                dsPixmap.animationPixmap = 
		    (CARD32) info->animation_data.animationPixmap;
                dsPixmap.animationMask = 
		    (CARD32) info->animation_data.animationMask;
    
		_XmWriteDragBuffer (propBuf, BUFFER_DATA,
				    (BYTE*)&dsPixmap,
		                    sizeof(xmDSPixmapDataStruct));
	    }
	    break;

	case XmDRAG_UNDER_NONE:
	    {
		XmICCDropSiteNone	info =
		    (XmICCDropSiteNone) dropSiteInfo;
		xmDSNoneDataStruct	dsNone;

                dsNone.borderWidth =
		    (CARD16) info->animation_data.borderWidth;

		_XmWriteDragBuffer (propBuf, BUFFER_DATA,
				    (BYTE*)&dsNone,
		                    sizeof(xmDSNoneDataStruct));
	    }
	    break;
	default:
	    break;
    }

    /* write each rectangle box */

    for (i = 0; i < region->numRects; i++) {
        box.x1 = (INT16) region->rects[i].x1;
        box.x2 = (INT16) region->rects[i].x2;
        box.y1 = (INT16) region->rects[i].y1;
        box.y2 = (INT16) region->rects[i].y2;
        _XmWriteDragBuffer(propBuf, BUFFER_DATA, (BYTE*) &box,
		           sizeof(xmICCRegBoxRec));
    }
}

/************************************************************************
 *
 *  _XmFreeDragReceiverInfo()
 *
 ***********************************************************************/

void 
#ifdef _NO_PROTO
_XmFreeDragReceiverInfo( info )
        XtPointer info ;
#else
_XmFreeDragReceiverInfo(
        XtPointer info )
#endif /* _NO_PROTO */
{	
    XmReceiverDSTreeStruct	*dsmInfo = (XmReceiverDSTreeStruct *)info;

    if (dsmInfo) {
	XtFree((char *)dsmInfo->propBufRec.data.bytes);
	XtFree((char *)dsmInfo);
    }
}

/************************************************************************
 *
 *  _XmClearDragReceiverInfo()
 *
 *  We can pass in the shell since we're pushing a property at one of
 *  our windows.
 ***********************************************************************/

void 
#ifdef _NO_PROTO
_XmClearDragReceiverInfo( shell )
        Widget shell ;
#else
_XmClearDragReceiverInfo(
        Widget shell )
#endif /* _NO_PROTO */
{
    Atom			receiverAtom;

    receiverAtom = XmInternAtom(XtDisplayOfObject(shell),
				XS_MOTIF_RECEIVER,
				False);
    XDeleteProperty(XtDisplayOfObject(shell),
		    XtWindow(shell),
		    receiverAtom);
    return;
}

/************************************************************************
 *
 *  _XmSetDragReceiverInfo()
 *
 *  We can pass in the shell since we're pushing a property at one of
 *  our windows.  This procedure should not be called for receivers
 *  with a protocol style of NONE, because such receivers should not
 *  have any visible receiver info.
 ***********************************************************************/
 
void 
#ifdef _NO_PROTO
_XmSetDragReceiverInfo( dd, shell )
        XmDisplay dd ;
        Widget shell ;
#else
_XmSetDragReceiverInfo(
        XmDisplay dd,
        Widget shell )
#endif /* _NO_PROTO */
{
    Cardinal			numDropSites = 0;
    BYTE			stackData[MAXSTACK];
    BYTE			stackHeap[MAXSTACK];
    xmPropertyBufferRec		*propBuf;
    xmDragReceiverInfoStruct	infoRec;
    XmReceiverDSTreeStruct	dsmInfoRec;
    Atom			receiverAtom;
    XmDropSiteManagerObject 	dsm;
    BYTE			*heapOffsetPtr;

    dsm = _XmGetDropSiteManagerObject( dd) ;

    receiverAtom = XmInternAtom(XtDisplayOfObject(shell),
				XS_MOTIF_RECEIVER,
				False);
    
    dsmInfoRec.numDropSites = 0;
    dsmInfoRec.currDropSite = 0;
    
    propBuf = &dsmInfoRec.propBufRec;
    propBuf->data.stack = 
      propBuf->data.bytes = stackData;
    propBuf->data.size = 0;
    propBuf->data.max = MAXSTACK;
    propBuf->heap.stack = 
      propBuf->heap.bytes = stackHeap;
    propBuf->heap.size = 0;
    propBuf->heap.max = MAXSTACK;
    
    infoRec.byte_order = _XmByteOrderChar;
    infoRec.protocol_version = _MOTIF_DRAG_PROTOCOL_VERSION;

    infoRec.drag_protocol_style = dd->display.dragReceiverProtocolStyle;
	infoRec.proxy_window = None;


    _XmWriteDragBuffer (propBuf, BUFFER_DATA, (BYTE*)&infoRec,
		        sizeof(xmDragReceiverInfoStruct));

	/*
	 * Only attach a drop site tree to the property if the receiver
	 * protocol style is not dynamic and not drop only (this procedure
	 * shouldn't get called if style is NONE).
	 */
    if ((dd->display.dragReceiverProtocolStyle != XmDRAG_DYNAMIC) &&
        (dd->display.dragReceiverProtocolStyle != XmDRAG_DROP_ONLY))
      {
	  numDropSites = _XmDSMGetTreeFromDSM(dsm, shell, (XtPointer)&dsmInfoRec);
      }
    else
      {
	/*
	 * However, this procedure is called on the realize of the
	 * shell, and the drop site manager is counting on having a 
	 * chance to synchronize the drop site db with the actual
	 * widget information.
	 */

	 _XmSyncDropSiteTree(shell);
      }
    heapOffsetPtr = (propBuf->data.bytes +
                     XtOffsetOf(xmDragReceiverInfoStruct, num_drop_sites));
    *(CARD16 *)heapOffsetPtr = (CARD16)numDropSites;
    heapOffsetPtr = (propBuf->data.bytes +
		       XtOffsetOf(xmDragReceiverInfoStruct, heap_offset));
    *(CARD32 *)heapOffsetPtr = (CARD32)propBuf->data.size;
    
    /* write the buffer to the property */
    XChangeProperty (XtDisplayOfObject(shell), 
		     XtWindow(shell),
		     receiverAtom, receiverAtom,
		     8, 
		     PropModeReplace,
		     (unsigned char *)propBuf->data.bytes,
		     propBuf->data.size);
    if (propBuf->data.bytes != propBuf->data.stack)
      XtFree((char *)propBuf->data.bytes);
    
    if (propBuf->heap.size) {
	XChangeProperty (XtDisplayOfObject(shell),
			 XtWindow(shell),
			 receiverAtom, receiverAtom,
			 8, PropModeAppend, 
			 (unsigned char *)propBuf->heap.bytes,
			 propBuf->heap.size);
	if (propBuf->heap.bytes != propBuf->heap.stack)
	  XtFree((char *)propBuf->heap.bytes);
	
    }
}

/************************************************************************
 *
 *  _XmInitByteOrderChar()
 *
 ***********************************************************************/

void 
#ifdef _NO_PROTO
_XmInitByteOrderChar( )
#else
_XmInitByteOrderChar( void )
#endif /* _NO_PROTO */
{
    /* bootstrap the byteorder if needed */
    if (!_XmByteOrderChar) {
	unsigned int		endian = 1;

	/* get rid of irritating saber warning */
	/*SUPPRESS 112*/
	if (*((char *)&endian))
	  _XmByteOrderChar = 'l';
	else
	  _XmByteOrderChar = 'B';
	}
}

