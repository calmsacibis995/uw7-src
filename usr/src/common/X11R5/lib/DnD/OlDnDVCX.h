/*
 *      Copyright (C) 1986,1992  Sun Microsystems, Inc
 *                      All rights reserved.
 *              Notice of copyright on this source code
 *              product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *      Sun Microsystems, Inc., 2550 Garcia Avenue,
 *      Mountain View, California 94043.
 *
 */

#ifndef NOIDENT
#ident	"@(#)oldnd:OlDnDVCX.h	1.8"
#endif

#ifndef	_OlDnDVCX_h_
#define	_OlDnDVCX_h_

	/* this header contains marco defns for "prototype"	*/
#include <DnD/FuncProto.h>

/*
 *  Xt[C|N] strings for DnD
 */
#define XtCDropSiteID			"DropSiteID"
#define XtCAutoAssertDropsiteRegistry	"AutoAssertDropsiteRegistry"
#define XtCPreviewForwardedSites	"PreviewForwardedSites"

#define XtNdropSiteID			"dropSiteID"
#define XtNregistryUpdateTimestamp	"registryUpdateTimestamp"
#define XtNnumberOfDropSites		"numberOfDropSites"
#define XtNrootX			"rootX"
#define XtNrootY			"rootY"
#define XtNautoAssertDropsiteRegistry	"autoAssertDropsiteRegistry"
#define XtNdirty			"dirty"
#define XtNpendingDSDMInfo		"pendingDSDMInfo"
#define XtNdsdmPresent			"dsdmPresent"
#define XtNdoingDrag			"doingDrag"
#define XtNdefaultDropSiteID		"defaultDropSiteID"
#define XtNdsdmLastLoaded		"dsdmLastLoaded"
#define XtNpreviewForwardedSites	"previewForwardedSites"

extern	Atom	_SUN_AVAILABLE_TYPES;

extern	Atom	_SUN_LOAD;
extern	Atom	_SUN_DATA_LABEL;
extern	Atom	_SUN_FILE_HOST_NAME;

extern	Atom	_SUN_ENUMERATION_COUNT;
extern	Atom	_SUN_ENUMERATION_ITEM;

extern	Atom	_SUN_ALTERNATE_TRANSPORT_METHODS;
extern	Atom	_SUN_LENGTH_TYPE;
extern	Atom	_SUN_ATM_TOOL_TALK;
extern	Atom	_SUN_ATM_FILE_NAME;

typedef	struct	oldnd_drop_site	*OlDnDDropSiteID;


/*********************/
/* OlDnDDragDropInfo */
/*********************/

typedef	struct _ol_dnd_root_info {
	Window		root_window;
	Position	root_x;
	Position	root_y;
	Time		drop_timestamp;
} OlDnDDragDropInfo, *OlDnDDragDropInfoPtr;

/*****************/
/* OlDnDSiteRect */
/*****************/

typedef XRectangle OlDnDSiteRect, *OlDnDSiteRectPtr;

/***********************/
/* OlDnDProtocolAction */
/***********************/

typedef	enum oldnd_protocol_action {
		OlDnDSelectionTransactionBegins,
		OlDnDSelectionTransactionEnds,
		OlDnDSelectionTransactionError,
		OlDnDDragNDropTransactionDone
}OlDnDProtocolAction;

/***********************************/
/* OlDnDProtocolActionCallbackProc */
/***********************************/

/* requestor callback resulting from calling OlDnD*SelectionTransaction */

typedef  void   (*OlDnDProtocolActionCallbackProc) OL_ARGS((Widget,
							   Atom,
							   OlDnDProtocolAction,
							   Boolean,
							   XtPointer
));

typedef	OlDnDProtocolActionCallbackProc	OlDnDProtocolActionCbP;

/**********************************/
/* OlDnDSelectionTransactionState */
/**********************************/

typedef	enum oldnd_transaction_state {
		OlDnDTransactionBegins,
		OlDnDTransactionEnds,
		OlDnDTransactionDone,
		OlDnDTransactionRequestorError,
		OlDnDTransactionRequestorWindowDeath
} OlDnDTransactionState;

/*********************************/
/* OlDnDTransactionStateCallback */
/*********************************/

typedef void (*OlDnDTransactionStateCallback)
			OL_ARGS (( Widget, Atom, OlDnDTransactionState, Time,
				   XtPointer ));


/*************************/
/* OLDnDSitePreviewHints */
/*************************/

typedef enum oldnd_site_preview_hints { 
		OlDnDSitePreviewNone,
		OlDnDSitePreviewEnterLeave = (1 << 0),
		OlDnDSitePreviewMotion = (1 << 1),
		OlDnDSitePreviewBoth = (OlDnDSitePreviewEnterLeave |
				 	OlDnDSitePreviewMotion),
		OlDnDSitePreviewDefaultSite = (1 << 2),
		OlDnDSitePreviewForwarded = (1 << 3), /* not implemented */
		OlDnDSitePreviewInsensitive = (1 << 4)
} OlDnDSitePreviewHints;

/*************************/
/* OlDnDTriggerOperation */
/*************************/

typedef enum oldnd_trigger_Operation {
		OlDnDTriggerCopyOp,
		OlDnDTriggerMoveOp,
		OlDnDTriggerLinkOp	/* new to DnD */
} OlDnDTriggerOperation;

/****************************************/
/* OlDnDRegister{Widget|Window}DropSite */
/****************************************/

typedef	Boolean	(*OlDnDTriggerMessageNotifyProc) OL_ARGS((
			/* widget, window */		Widget, Window,
			/* x, y in root window */	Position, Position,
			/* selection, timestamp*/	Atom, Time,
			/* drop site */			OlDnDDropSiteID,
			/* operation */			OlDnDTriggerOperation,
			/* send_done */			Boolean,
/* not implemented */	/* forwarded */			Boolean,
			/* closure */			XtPointer
));

typedef	OlDnDTriggerMessageNotifyProc	OlDnDTMNotifyProc;

typedef void	(*OlDnDPreviewMessageNotifyProc) OL_ARGS((
			/* widget , window */		Widget, Window,
			/* x, y in root window */	Position, Position,
			/* Enter, Leave or Motion */	int, 
			/* timestamp */			Time,
			/* drop site */			OlDnDDropSiteID,
/* not implemented */	/* forwarded */			Boolean,
			/* closure */			XtPointer
));

typedef	OlDnDPreviewMessageNotifyProc	OlDnDPMNotifyProc;

/*
 * This header contains additions from USL.
 *
 */
#include <DnD/OlDnDUtil.h>

OLBeginFunctionPrototypeBlock

extern OlDnDDropSiteID
OlDnDRegisterWidgetDropSite	OL_ARGS((
				Widget,				/* widget */
				OlDnDSitePreviewHints,		/* hints */
				OlDnDSiteRectPtr,		/* sites */
				unsigned int,			/* num sites */
				OlDnDTMNotifyProc,		/* tmnotify */
				OlDnDPMNotifyProc,		/* pmnotify */
				Boolean,			/* on_interest*/
				XtPointer			/* closure */
));

extern OlDnDDropSiteID
OlDnDRegisterWindowDropSite	OL_ARGS((
				Display *,			/* dpy */
				Window,				/* window */
				OlDnDSitePreviewHints,		/* hints */
				OlDnDSiteRectPtr,		/* sites */
				unsigned int,			/* num sites */
				OlDnDTMNotifyProc,		/* tmnotify */
				OlDnDPMNotifyProc,		/* pmnotify */
				Boolean,			/* on_interest*/
				XtPointer			/* closure */
));

/*******************************/
/* OlDnDUpdateDropSiteGeometry */
/*******************************/

extern Boolean
OlDnDUpdateDropSiteGeometry   OL_ARGS(( OlDnDDropSiteID,	/* drop site */
					OlDnDSiteRectPtr,	/* new rects */
					unsigned int		/* num rects */
));

/***************************************/
/* OlDnDChangeDropSitePreviewHints     */
/***************************************/

extern Boolean
OlDnDChangeDropSitePreviewHints OL_ARGS(( OlDnDDropSiteID, 
					  OlDnDSitePreviewHints ));

/**************************/
/* OlDnDQueryDropSiteInfo */
/**************************/

extern Boolean
OlDnDQueryDropSiteInfo	      OL_ARGS(( OlDnDDropSiteID,	/* drop site */
					Widget *,		/* widget */
					Window *,		/* window */
					OlDnDSitePreviewHints *,/* hints  */
					OlDnDSiteRectPtr *,	/* rects */
					unsigned int *,		/* num rects */
					Boolean *		/* on_interest*/
));

/****************************/
/* OlDnDSetDropSiteInterest */
/****************************/

extern Boolean
OlDnDSetDropSiteInterest OL_ARGS(( OlDnDDropSiteID, Boolean ));

/********************************/
/* OlDnDSetInterestInWidgetHier */
/********************************/

extern void
OlDnDSetInterestInWidgetHier OL_ARGS(( Widget, Boolean ));

/************************/
/* OlDnDDestroyDropSite */
/************************/

extern void
OlDnDDestroyDropSite	OL_ARGS(( OlDnDDropSiteID ));

/****************************/
/* OlDnDInitializeDragState */
/****************************/

extern Boolean
OlDnDInitializeDragState OL_ARGS(( Widget ));

/***********************/
/* OlDnDClearDragState */
/***********************/

extern void
OlDnDClearDragState OL_ARGS(( Widget ));

/******************************/
/* OlDnDPreviewAndAnimate     */
/******************************/

typedef	void (*OlDnDPreviewAnimateCallbackProc) OL_ARGS(( Widget,
							  int, 
							  Time,
		/* not implemented, sensitivity-> */	  Boolean,
							  XtPointer ));

typedef	OlDnDPreviewAnimateCallbackProc	OlDnDPreviewAnimateCbP;

extern Boolean
OlDnDPreviewAndAnimate OL_ARGS(( Widget, Window, Position, Position, Time, 
				 OlDnDPreviewAnimateCbP, XtPointer ));

/******************************/
/* OlDnDDeliverPreviewMessage */
/******************************/

extern Boolean
OlDnDDeliverPreviewMessage OL_ARGS(( Widget,  Window, Position, Position, Time ));

/******************************/
/* OlDnDDeliverTriggerMessage */
/******************************/

extern Boolean
OlDnDDeliverTriggerMessage OL_ARGS(( Widget, Window, Position, Position, Atom,
				     OlDnDTriggerOperation, Time ));

/********************/
/* OlDnDDragAndDrop */
/********************/

extern Boolean OlDnDDragAndDrop OL_ARGS(( Widget, Window *, Position *,
					  Position *, OlDnDDragDropInfoPtr,
					  OlDnDPreviewAnimateCbP, XtPointer ));

/**********************************/
/* OlDnD{Alloc|Free}TranisentAtom */
/**********************************/

extern Atom
OlDnDAllocTransientAtom OL_ARGS(( Widget ));

extern void
OlDnDFreeTransientAtom OL_ARGS(( Widget, Atom ));

#define FreeAllTransientAtoms	((Atom)-1)

/**********************************/
/* OlDnDBeginSelectionTransaction */
/**********************************/

extern void
OlDnDBeginSelectionTransaction OL_ARGS(( Widget, Atom, Time, 
					 OlDnDProtocolActionCbP,
					 XtPointer ));

/********************************/
/* OlDnDEndSelectionTransaction */
/********************************/

extern void
OlDnDEndSelectionTransaction OL_ARGS(( Widget, Atom, Time, 
					 OlDnDProtocolActionCbP,
					 XtPointer ));

/**********************/
/* OlDnDDragNDropDone */
/**********************/

extern void
OlDnDDragNDropDone OL_ARGS(( Widget, Atom, Time, 
			     OlDnDProtocolActionCbP, XtPointer ));

/****************************************/
/* OlDnDErrorDuringSelectionTransaction */
/****************************************/

extern void
OlDnDErrorDuringSelectionTransaction OL_ARGS(( Widget, Atom, Time, 
					       OlDnDProtocolActionCbP,
					       XtPointer ));

/****************************/
/* OlDnDGetWidgetOfDropSite */
/****************************/

extern Widget
OlDnDGetWidgetOfDropSite OL_ARGS(( OlDnDDropSiteID ));

/****************************/
/* OlDnDGetWindowOfDropSite */
/****************************/

extern Window
OlDnDGetWindowOfDropSite OL_ARGS(( OlDnDDropSiteID ));

/*****************************/
/* OlDnDGetDropSitesOfWidget */
/*****************************/

extern OlDnDDropSiteID *
OlDnDGetDropSitesOfWidget OL_ARGS(( Widget, Cardinal * ));

/*****************************/
/* OlDnDGetDropSitesOfWindow */
/*****************************/

extern OlDnDDropSiteID *
OlDnDGetDropSitesOfWindow OL_ARGS(( Display *, Window, Cardinal * ));

/****************************************/
/* OlDnDGetCurrentSelectionsForWidget   */
/****************************************/

extern Boolean
OlDnDGetCurrentSelectionsForWidget OL_ARGS(( Widget, Atom**, Cardinal * ));

/*********************/
/* OlDnDOwnSelection */
/*********************/

extern Boolean
OlDnDOwnSelection OL_ARGS (( Widget, Atom, Time, XtConvertSelectionProc,
			   XtLoseSelectionProc, XtSelectionDoneProc,
			   OlDnDTransactionStateCallback, XtPointer
));

/********************************/
/* OlDnDOwnSelectionIncremental */
/********************************/

extern Boolean
OlDnDOwnSelectionIncremental OL_ARGS (( Widget, Atom, Time,
				       XtConvertSelectionIncrProc,
				       XtLoseSelectionIncrProc,
				       XtSelectionDoneIncrProc,
				       XtCancelConvertSelectionProc,
				       XtPointer,
				       OlDnDTransactionStateCallback
));

/************************/
/* OlDnDDisownSelection */
/************************/

extern void
OlDnDDisownSelection OL_ARGS (( Widget, Atom, Time ));

extern void
OlDnDWidgetConfiguredInHier OL_ARGS((Widget)); /* not implemented */

OLEndFunctionPrototypeBlock

#endif	/* _OlDnDVCX_h */
