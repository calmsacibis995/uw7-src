#pragma ident	"@(#)m1.2libs:Xm/DragCI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
*/ 
/*
 * Motif Release 1.2.3
*/
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/* $Header$ */
#ifndef _XmDragCI_h
#define _XmDragCI_h

#include <Xm/XmP.h>
#include <Xm/DragCP.h>
#include <Xm/DragIconP.h>
#include <Xm/DropSMgrP.h>
#include <Xm/DisplayP.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _XmDRAG_MASK_BASE \
(ButtonPressMask | ButtonReleaseMask | ButtonMotionMask)
#ifdef DRAG_USE_MOTION_HINTS
#define _XmDRAG_GRAB_MASK \
(_XmDRAG_MASK_BASE PointerMotionHintMask)
#else
#define _XmDRAG_GRAB_MASK _XmDRAG_MASK_BASE
#endif /* _XmDRAG_USE_MOTION_HINTS */

#define _XmDRAG_EVENT_MASK(dc) \
  ((((XmDragContext)dc)->drag.trackingMode == XmDRAG_TRACK_WM_QUERY) \
   ? (_XmDRAG_GRAB_MASK | EnterWindowMask | LeaveWindowMask) \
   : (_XmDRAG_GRAB_MASK))

enum{	XmCR_DROP_SITE_TREE_ADD = _XmNUMBER_DND_CB_REASONS,
	XmCR_DROP_SITE_TREE_REMOVE
	} ;
/*
 *  values for dragTrackingMode 
 */
enum { XmDRAG_TRACK_WM_QUERY, XmDRAG_TRACK_MOTION, XmDRAG_TRACK_WM_QUERY_PENDING };


/* Strings to use for the intern atoms */
typedef String	XmCanonicalString;

#define XmMakeCanonicalString( s) \
	  (XmCanonicalString) XrmQuarkToString( XrmStringToQuark( s))

#define _XmAllocAndCopy( data, len) \
		   memcpy( (XtPointer) XtMalloc(len), (XtPointer)(data), (len))


typedef struct _XmDragTopLevelClientDataStruct{
    Widget	destShell;
    Position	xOrigin, yOrigin;
	Dimension	width, height;
    XtPointer	iccInfo;
    Boolean	sourceIsExternal;
	Window	window;
	Widget	dragOver;
}XmDragTopLevelClientDataStruct, *XmDragTopLevelClientData;

typedef struct _XmDragMotionClientDataStruct{
    Window	window;
    Widget	dragOver;
}XmDragMotionClientDataStruct, *XmDragMotionClientData;


#if 0
/* These are not currently in use. */
typedef struct _XmTargetsTableEntryRec{
    Cardinal	numTargets;
    Atom	*targets;
}XmTargetsTableEntryRec, XmTargetsTableEntry;

typedef struct _XmTargetsTableRec{
    Cardinal	numEntries;
    XmTargetsTableEntryRec entries[1]; /* variable size array in-place */
}XmTargetsTableRec, *XmTargetsTable;
#endif

/*
 * dsm to dragDisplay comm
 */
/* Move to DropSMgrI.h */
typedef struct _XmDropSiteTreeAddCallbackStruct{
    int		    	reason;
    XEvent          	*event;
    Widget		rootShell;
    Cardinal		numDropSites;
    Cardinal		numArgsPerDSHint;
}XmDropSiteTreeAddCallbackStruct, *XmDropSiteTreeAddCallback;

/* Move to DropSMgrI.h */
typedef struct _XmDropSiteTreeRemoveCallbackStruct{
    int			reason;
    XEvent          	*event;
    Widget		rootShell;
}XmDropSiteTreeRemoveCallbackStruct, *XmDropSiteTreeRemoveCallback;

/* Move to DropSMgrI.h */
typedef struct _XmDropSiteTreeUpdateCallbackStruct{
    int			reason;
    XEvent          	*event;
    Widget		rootShell;
    Cardinal		numDropSites;
    Cardinal		numArgsPerDSHint;
}XmDropSiteTreeUpdateCallbackStruct, *XmDropSiteTreeUpdateCallback;

typedef struct _XmDropSiteEnterPendingCallbackStruct{
    int                 reason;
    XEvent              *event;
    Time                timeStamp;
    Boolean		enter_pending;
}XmDropSiteEnterPendingCallbackStruct, *XmDropSiteEnterPendingCallback;

/* Move to DropSMgrI.h */
typedef struct _XmAnimationData {
    Widget		dragOver;
    Window		window;
    Position		windowX, windowY;
    Screen		*screen;
    XmRegion		clipRegion;
    XmRegion		dropSiteRegion;
    XtPointer		saveAddr;
}XmAnimationDataRec, *XmAnimationData;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDragCI_h */
