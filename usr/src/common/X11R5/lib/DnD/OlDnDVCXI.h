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
#ident	"@(#)oldnd:OlDnDVCXI.h	1.10"
#endif

#ifndef	_OlDnDVCXI_h_
#define	_OlDnDVCXI_h_

#ifdef GUI_DEP
#include	<Xol/OlDnDVCX.h>
#else
#include	<DnD/OlDnDVCX.h>
#endif

#define	_SUN_DRAGDROP_BEGIN_NAME	"_SUN_DRAGDROP_BEGIN"
#define	_SUN_SELECTION_END_NAME		"_SUN_SELECTION_END"
#define	_SUN_SELECTION_ERROR_NAME	"_SUN_SELECTION_ERROR"
#define	_SUN_DRAGDROP_DONE_NAME		"_SUN_DRAGDROP_DONE"

#define	_SUN_AVAILABLE_TYPES_NAME	"_SUN_AVAILABLE_TYPES"

#define	_SUN_LOAD_NAME			"_SUN_LOAD"
#define	_SUN_DATA_LABEL_NAME		"_SUN_DATA_LABEL"
#define	_SUN_FILE_HOST_NAME_NAME	"_SUN_FILE_HOST_NAME"

#define	_SUN_ENUMERATION_COUNT_NAME	"_SUN_ENUMERATION_COUNT"
#define	_SUN_ENUMERATION_ITEM_NAME	"_SUN_ENUMERATION_ITEM"

#define	_SUN_ALTERNATE_TRANSPORT_METHODS_NAME	\
					"_SUN_ALTERNATE_TRANSPORT_METHODS"

#define	_SUN_LENGTH_TYPE_NAME		"_SUN_LENGTH_TYPE"
#define	_SUN_ATM_TOOL_TALK_NAME		"_SUN_ATM_TOOL_TALK"
#define	_SUN_ATM_FILE_NAME_NAME		"_SUN_ATM_FILE_NAME"
#define	_SUN_DRAGDROP_ACK_NAME		"_SUN_DRAGDROP_ACK"


extern	Atom	_SUN_DRAGDROP_BEGIN;
extern	Atom	_SUN_SELECTION_END;
extern	Atom	_SUN_SELECTION_ERROR;
extern	Atom	_SUN_DRAGDROP_DONE;

/************************************************
 *
 * Definitions for _SUN_DRAGDROP_INTEREST property
 *
 ************************************************/

/*
 * Implementation note: where the last element of a structure is defined
 * 			below as an array with one element it is assumed
 *			that the length of the structure is variable in as
 *			much as the tail (i.e the array of elements) shall
 *			be defined dynamically, macros for calculating the
 *			size required, and accessors are provided.
 *			It is not recommended therefore that such structures
 *			should be defined at compile time.
 */

/**************/
/* WindowList */
/**************/

typedef	struct	window_list {
			unsigned int	window_count;
			Window		windows[1];	/* dummy */
} WindowList, *WindowListPtr;

#define	WindowListWindowCount(wl)	((wl).window_count)
#define	WindowListPtrWindowCount(wl)	((wl)->window_count)

#define	WindowListWindows(wl)		((wl).windows)
#define	WindowListPtrWindows(wl)	((wl)->windows)

#define	SizeOfWindowListForNWindows(nw)	(sizeof(WindowList) + 		\
					 (sizeof(Window) * ((nw) - 1)))	

/************/
/* RectList */
/************/

typedef	struct	_site_rect {
			int		x, y;
			unsigned int	width, height;
} SiteRect, *SiteRectPtr;

#define SiteRectX(r)        ((r).x)
#define SiteRectPtrX(r)     ((r)->x)

#define SiteRectY(r)        ((r).y)
#define SiteRectPtrY(r)     ((r)->y)

#define SiteRectWidth(r)    ((r).width)
#define SiteRectPtrWidth(r) ((r)->width)

#define SiteRectHeight(r)           ((r).height)
#define SiteRectPtrHeight(r)        ((r)->height)


typedef	struct	rect_list {
			unsigned int	rect_count;
			SiteRect	rects[1];	/* dummy */
} RectList, *RectListPtr;
			
#define	RectListRectCount(rl)		((rl).rect_count)
#define	RectListPtrRectCount(rl)	((rl)->rect_count)

#define	RectListRects(rl)		((rl).rects)
#define	RectListPtrRects(rl)		((rl)->rects)

#define	SizeOfRectListForNRects(nr)	(sizeof(RectList) +		\
					 (sizeof(Rect) * ((nr) - 1)))

/************/
/* AreaList */
/************/

typedef	enum area_type { IsRectList, IsWindowList} AreaType;

typedef	union	window_or_rect_list	{
						RectList	rect_list;
						WindowList	window_list;
} WindowOrRectList, *WindowOrRectListPtr;

typedef	struct	area_list {
			AreaType		type;
			WindowOrRectList	list;
} AreaList, *AreaListPtr;


#define	AreaListType(a)			((a).type)
#define	AreaListPtrType(a)		((a)->type)

#define	AreaListIsRectList(a)		((a).type == IsRectList)
#define	AreaListPtrIsRectList(a)	((a)->type == IsRectList)

#define	AreaListIsWindowList(a)		((a).type == IsWindowList)
#define	AreaListPtrIsWindowList(a)	((a)->type == IsWindowList)

#define	AreaListRectList(a)	((a).list.rect_list)
#define	AreaListPtrRectList(a)	((a)->list.rect_list)

#define	AreaListWindowList(a)		((a).list.window_list)
#define	AreaListPtrWindowList(a)	((a)->list.window_list)

#define	SizeOfAreaListHead		\
		(sizeof(AreaList) - sizeof(WindowOrRectList))

#define	SizeOfAreaListForNRects(nr)	(SizeOfAreaListHead +	\
					 (sizeof(RectList) * (nr)))

#define	SizeOfAreaListForNWindows(nw)	(SizeOfAreaListHead +   \
					 (sizeof(WindowList) * (nw)))

#define	NumOfRectsInAreaList(al)	(RectListRectCount(	\
						 AreaListRectList(al)))
#define	NumOfRectsInAreaListPtr(al)	(RectListRectCount(	\
						AreaListPtrRectList(al)))

#define	NumOfWindowsInAreaList(al)	(WindowListWindowCount(	\
						AreaListWindowList(al)))
#define	NumOfWindowsInAreaListPtr(al)	(WindowListWindowCount(	\
						AreaListPtrWindowList(al)))


/*******************/
/* SiteDescription */
/*******************/

typedef	struct	site_description {
			Window			event_window;
			unsigned int		site_id;
			OlDnDSitePreviewHints	preview_hints;
			AreaList		areas;
} SiteDescription, *SiteDescriptionPtr;

#define	SiteDescriptionEventWindow(sd)		((sd).event_window)
#define	SiteDescriptionPtrEventWindow(sd)	((sd)->event_window)
				
#define	SiteDescriptionSiteID(sd)		((sd).site_id)
#define	SiteDescriptionPtrSiteID(sd)		((sd)->site_id)

#define	SiteDescriptionPreviewHints(sd)		((sd).preview_hints)
#define	SiteDescriptionPtrPreviewHints(sd)	((sd)->preview_hints)

#define	IfSiteDescriptionPreviewHintsON(ds, hints)			\
	((SiteDescriptionPreviewHints(sd) & (hints)) == (hints))

#define	IfSiteDescriptionPtrPreviewHintsON(ds, hints)			\
	((SiteDescriptionPtrPreviewHints(sd) & (hints)) == (hints))

#define	SiteDescriptionWantsPreviewEnterLeave(sd)			     \
	(IfSiteDescriptionPreviewHintsON(sd, OlDnDSitePreviewEnterLeave) ||  \
	 IfSiteDescriptionPreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	SiteDescriptionPtrWantsPreviewEnterLeave(sd)			     \
	(IfSiteDescriptionPtrPreviewHintsON(sd, OlDnDSitePreviewEnterLeave)  \
	 || IfSiteDescriptionPtrPreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	SiteDescriptionWantsPreviewMotion(sd)				   \
	(IfSiteDescriptionPreviewHintsON(sd, OlDnDSitePreviewMotion) ||    \
	 IfSiteDescriptionPreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	SiteDescriptionPtrWantsPreviewMotion(sd)			    \
	(IfSiteDescriptionPtrPreviewHintsON(sd, OlDnDSitePreviewMotion) ||  \
	 IfSiteDescriptionPtrPreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	SiteDescriptionIsDefaultSite(sd)				   \
	IfSiteDescriptionPreviewHintsON(sd, OlDnDSitePreviewDefaultSite)

#define	SiteDescriptionPtrIsDefaultSite(sd)			    	   \
	IfSiteDescriptionPtrPreviewHintsON(sd, OlDnDSitePreviewDefaultSite)

#define	SiteDescriptionAreas(sd)		((sd).areas)
#define	SiteDescriptionPtrAreas(sd)		((sd)->areas)

#define	SiteDescriptionAreasType(sd)		\
		AreaListType(SiteDescriptionAreas(sd))
#define	SiteDescriptionPtrAreasType(sd)		\
		AreaListType(SiteDescriptionPtrAreas(sd))

#define	SiteDescriptionAreaIsRectList(sd)	\
		AreaListIsRectList(SiteDescriptionAreas(sd))
#define	SiteDescriptionPtrAreaIsRectList(sd)	\
		AreaListIsRectList(SiteDescriptionPtrAreas(sd))

#define	SiteDescriptionAreaRectList(sd)		\
		AreaListRectList(SiteDescriptionAreas(sd))
#define	SiteDescriptionPtrAreaRectList(sd)		\
		AreaListRectList(SiteDescriptionPtrAreas(sd))

#define	SiteDescriptionAreaIsWindowList(sd)	\
		AreaListIsWindowList(SiteDescriptionAreas(sd))
#define	SiteDescriptionPtrAreaIsWindowList(sd)	\
		AreaListIsWindowList(SiteDescriptionPtrAreas(sd))

#define	SiteDescriptionAreaWindowList(sd)		\
		AreaListWindowList(SiteDescriptionAreas(sd))
#define	SiteDescriptionPtrAreaWindowList(sd)		\
		AreaListWindowList(SiteDescriptionPtrAreas(sd))

#define	SizeOfSiteDescriptionHead	\
		(sizeof(SiteDescription) - sizeof(AreaList))

#define	SizeOfSiteDescriptionForNWindows(nw)		\
		(SizeOfSiteDescriptionHead + SizeOfAreaListForNWindows(nw))

#define	SizeOfSiteDescriptionForNRects(nr)		\
		(SizeOfSiteDescriptionHead + SizeOfAreaListForNRects(nr))

/********************/
/* InterestProperty */
/********************/

typedef	struct	interest_property {
			unsigned int		version_number;
			unsigned int		site_count;

			SiteDescription		site_descriptions[1];/* dummy */
} InterestProperty, *InterestPropertyPtr;


#define	InterestPropertyVersionNumber(ip)	((ip).version_number)
#define	InterestPropertyPtrVersionNumber(ip)	((ip)->version_number)

#define	InterestPropertySiteCount(ip)		((ip).site_count)
#define	InterestPropertyPtrSiteCount(ip)	((ip)->site_count)

#define	InterestPropertySiteDescriptions(ip)	((ip).site_descriptions)
#define	InterestPropertyPtrSiteDescriptions(ip)	\
						((ip)->site_descriptions)

#define	SizeOfInterestPropertyHead	\
			(sizeof(InterestProperty) - sizeof(SiteDescription))

extern	Atom	_SUN_DRAGDROP_INTEREST;		/* the Interest Property */

extern	Atom	_SUN_DRAGDROP_DSDM;		/* DSDM property */

#define	_SUN_DRAGDROP_INTEREST_NAME	"_SUN_DRAGDROP_INTEREST"
#define	_SUN_DRAGDROP_DSDM_NAME		"_SUN_DRAGDROP_DSDM"

/************************************************
 *
 * Definitions for _SUN_DRAGDROP_SITE_RECTS target
 *
 ************************************************/

typedef	struct	dsdm_site_rects {
				unsigned long		screen_number;
				unsigned long		site_id;
				Window			window_id;
				unsigned int		x, y;
				unsigned int		width, height;
				OlDnDSitePreviewHints	preview_hints;
} DSDMSiteRect, *DSDMSiteRectPtr;
				
#define	DSDMSiteRectScreenNumber(sr)	((sr).screen_number)
#define	DSDMSiteRectPtrScreenNumber(sr)	((sr)->screen_number)

#define	DSDMSiteRectSiteID(sr)		((sr).site_id)
#define	DSDMSiteRectPtrSiteID(sr)	((sr)->site_id)

#define	DSDMSiteRectWindowID(sr)	((sr).window_id)
#define	DSDMSiteRectPtrWindowID(sr)	((sr)->window_id)

#define	DSDMSiteRectX(sr)		((sr).x)
#define	DSDMSiteRectPtrX(sr)		((sr)->x)

#define	DSDMSiteRectY(sr)		((sr).y)
#define	DSDMSiteRectPtrY(sr)		((sr)->y)

#define	DSDMSiteRectWidth(sr)		((sr).width)
#define	DSDMSiteRectPtrWidth(sr)	((sr)->width)

#define	DSDMSiteRectHeight(sr)		((sr).height)
#define	DSDMSiteRectPtrHeight(sr)	((sr)->height)

#define	DSDMSiteRectPreviewHints(sr)	((sr).preview_hints)
#define	DSDMSiteRectPtrPreviewHints(sr)	((sr)->preview_hints)

extern	Atom	_SUN_DRAGDROP_SITE_RECTS;	/* DSDM target Atom */

#define	_SUN_DRAGDROP_SITE_RECTS_NAME	"_SUN_DRAGDROP_SITE_RECTS"

/************************************************
 *
 * Definitions for _SUN_DRAGDROP_TRIGGER message
 *
 ************************************************/


/*
 * private macro stuff for extracting "packed" x and y co-ords to/from
 * trigger and preview messages
 */

#define	_X_SHIFT	16
#if defined(att) || defined(USL)
#define	_X_MASK	(((unsigned long)0xffff) << _X_SHIFT)
#else
#define	_X_MASK	(0xffff << _X_SHIFT)
#endif
#define	_Y_MASK	~_X_MASK

#define	_GetXField(f)		(((unsigned long)(f) & _X_MASK) >> _X_SHIFT)
#define	_SetXField(f, x)						     \
				(*((unsigned long *)&(f)) |= 		     \
				(((unsigned long)(x) << _X_SHIFT) & _X_MASK))

#define	_GetYField(f)		((unsigned long)(f) & _Y_MASK)
#define	_SetYField(f, y)	(*((unsigned long *)&(f)) |= ((y) & _Y_MASK))

/* end of macro stuff .... now back to our main feature */

typedef enum oldnd_trigger_flags {
		OlDnDTriggerCopy = 0,
		OlDnDTriggerMove = (1 << 0),
		OlDnDTriggerAck  = (1 << 1),
		OlDnDTriggerTransient = (1 << 2),
		OlDnDTriggerLink = (1 << 10)	/* new to DnD */
} OlDnDTriggerFlags, *OlDnDTriggerFlagsPtr;

typedef	struct	trigger_message {
			Atom			type;
			Window			window;
			Atom			selection;
			unsigned long		x;
			unsigned long		y;
			Time			timestamp;
			unsigned long		site_id;
			OlDnDTriggerFlags	flags;
} TriggerMessage, *TriggerMessagePtr;

#define	TriggerMessageType(tm)		(tm).type
#define	TriggerMessagePtrType(tm)	(tm)->type

#define	TriggerMessageWindow(tm)	(tm).window
#define	TriggerMessagePtrWindow(tm)	(tm)->window

#define	TriggerMessageSelection(tm)	(tm).selection
#define	TriggerMessagePtrSelection(tm)	(tm)->selection

#define	TriggerMessageX(tm)		(tm).x
#define	TriggerMessagePtrX(tm)		(tm)->x

#define	TriggerMessageY(tm)		(tm).y
#define	TriggerMessagePtrY(tm)		(tm)->y

#define	TriggerMessageTimestamp(tm)	(tm).timestamp
#define	TriggerMessagePtrTimestamp(tm)	(tm)->timestamp

#define	TriggerMessageSiteID(tm)	(tm).site_id
#define	TriggerMessagePtrSiteID(tm)	(tm)->site_id

#define	TriggerMessageFlags(tm)		(tm).flags
#define	TriggerMessagePtrFlags(tm)	(tm)->flags

#define	CopyTriggerMessageToClientMessage(tmp, cmp)			\
		(cmp)->type = ClientMessage;				\
		(cmp)->window = TriggerMessagePtrWindow(tmp);		\
		(cmp)->message_type = _SUN_DRAGDROP_TRIGGER;		\
		(cmp)->format = 32;					\
		(cmp)->data.l[0] = TriggerMessagePtrSelection(tmp);	\
		(cmp)->data.l[1] = TriggerMessagePtrTimestamp(tmp);	\
		(cmp)->data.l[2] = 0L;					\
		_SetXField((cmp)->data.l[2], TriggerMessagePtrX(tmp));	\
		_SetYField((cmp)->data.l[2], TriggerMessagePtrY(tmp));	\
		(cmp)->data.l[3] = TriggerMessagePtrSiteID(tmp);	\
		(cmp)->data.l[4] = TriggerMessagePtrFlags(tmp)

#define	CopyTriggerMessageFromClientMessage(tmp, cmp)			\
		TriggerMessagePtrType(tmp) = (cmp)->message_type;	\
		TriggerMessagePtrWindow(tmp) = (cmp)->window;		\
		TriggerMessagePtrSelection(tmp) = (cmp)->data.l[0];	\
		TriggerMessagePtrTimestamp(tmp) = (cmp)->data.l[1];	\
		TriggerMessagePtrX(tmp) = _GetXField((cmp)->data.l[2]);	\
		TriggerMessagePtrY(tmp) = _GetYField((cmp)->data.l[2]); \
		TriggerMessagePtrSiteID(tmp) = (cmp)->data.l[3];	\
		TriggerMessagePtrFlags(tmp) = (cmp)->data.l[4]
		

extern	Atom	_SUN_DRAGDROP_TRIGGER;		/* Trigger Message Atom */

#define	_SUN_DRAGDROP_TRIGGER_NAME	"_SUN_DRAGDROP_TRIGGER"

/************************************************
 *
 * Definitions for _SUN_DRAGDROP_PREVIEW mesage
 *
 ************************************************/

typedef	struct	preview_message {
			Atom		type;	/* = _SUN_DRAGDROP_PREVIEW */
			Window		window;
			unsigned long	eventcode;
			Time 		timestamp;
			unsigned long	x;
			unsigned long	y;
			unsigned long	site_id;
} PreviewMessage, *PreviewMessagePtr;


#define	PreviewMessageType(pm)		(pm).type
#define	PreviewMessagePtrType(pm)	(pm)->type

#define	PreviewMessageWindow(pm)	(pm).window
#define	PreviewMessagePtrWindow(pm)	(pm)->window

#define	PreviewMessageEventcode(pm)	(pm).eventcode
#define	PreviewMessagePtrEventcode(pm)	(pm)->eventcode

#define	PreviewMessageTimestamp(pm)	(pm).timestamp
#define	PreviewMessagePtrTimestamp(pm)	(pm)->timestamp

#define	PreviewMessageX(pm)		(pm).x
#define	PreviewMessagePtrX(pm)		(pm)->x

#define	PreviewMessageY(pm)		(pm).y
#define	PreviewMessagePtrY(pm)		(pm)->y

#define	PreviewMessageSiteID(pm)	(pm).site_id
#define	PreviewMessagePtrSiteID(pm)	(pm)->site_id

#define	CopyPreviewMessageToClientMessage(pmp, cmp)			\
		(cmp)->type = ClientMessage;				\
		(cmp)->window = PreviewMessagePtrWindow(pmp);		\
		(cmp)->message_type = _SUN_DRAGDROP_PREVIEW;		\
		(cmp)->format = 32;					\
		(cmp)->data.l[0] = PreviewMessagePtrEventcode(pmp);	\
		(cmp)->data.l[1] = PreviewMessagePtrTimestamp(pmp);	\
		(cmp)->data.l[2] = 0L;					\
		_SetXField((cmp)->data.l[2], PreviewMessagePtrX(pmp));	\
		_SetYField((cmp)->data.l[2], PreviewMessagePtrY(pmp));	\
		(cmp)->data.l[3] = PreviewMessagePtrSiteID(pmp);	\
		(cmp)->data.l[4] = 0L

#define	CopyPreviewMessageFromClientMessage(pmp, cmp)			\
		PreviewMessagePtrType(pmp) = (cmp)->message_type;	\
		PreviewMessagePtrWindow(pmp) = (cmp)->window;		\
		PreviewMessagePtrEventcode(pmp) = (cmp)->data.l[0];	\
		PreviewMessagePtrTimestamp(pmp) = (cmp)->data.l[1];	\
		PreviewMessagePtrX(pmp)= _GetXField((cmp)->data.l[2]);	\
		PreviewMessagePtrY(pmp)= _GetYField((cmp)->data.l[2]);	\
		PreviewMessagePtrSiteID(pmp) = (cmp)->data.l[3]

extern	Atom	_SUN_DRAGDROP_PREVIEW;		/* Preview Message Atom */ 

#define	_SUN_DRAGDROP_PREVIEW_NAME	"_SUN_DRAGDROP_PREVIEW"

/****************** class internal data structures *************************/


/**********************
 * InternalDSR
 **********************/

typedef	struct	_internal_site_rect	*InternalDSRPtr;

typedef	struct	_internal_site_rect {
	InternalDSRPtr		next,
				prev;
	OlDnDDropSiteID		dropsite;
	int			local_x,
				local_y;
	OlDnDSiteRect		toplevel_rect;
} InternalDSR /*, *InternalSiteRectPtr*/;

#define	InternalDSRNext(isr)		((isr).next)
#define	InternalDSRPtrNext(isr)		((isr)->next)

#define	InternalDSRPrev(isr)		((isr).prev)
#define	InternalDSRPtrPrev(isr)		((isr)->prev)

#define	InternalDSRDropSite(isr)	((isr).dropsite)
#define	InternalDSRPtrDropSite(isr)	((isr)->dropsite)

#define	InternalDSRLocalX(isr)		((isr).local_x)
#define	InternalDSRPtrLocalX(isr)	((isr)->local_x)

#define	InternalDSRLocalY(isr)		((isr).local_y)
#define	InternalDSRPtrLocalY(isr)	((isr)->local_y)

#define	InternalDSRTopLevelRect(isr)	((isr).toplevel_rect)
#define	InternalDSRPtrTopLevelRect(isr)	((isr)->toplevel_rect)

#define	InternalDSRTopLevelRectX(isr)		\
					SiteRectX((isr).toplevel_rect)
#define	InternalDSRPtrTopLevelRectX(isr)	\
					SiteRectX((isr)->toplevel_rect)

#define	InternalDSRTopLevelRectY(isr)		\
					SiteRectY((isr).toplevel_rect)
#define	InternalDSRPtrTopLevelRectY(isr)	\
					SiteRectY((isr)->toplevel_rect)

#define	InternalDSRTopLevelRectWidth(isr)	\
				SiteRectWidth((isr).toplevel_rect)
#define	InternalDSRPtrTopLevelRectWidth(isr)	\
				SiteRectWidth((isr)->toplevel_rect)

#define	InternalDSRTopLevelRectHeight(isr)	\
				SiteRectHeight((isr).toplevel_rect)
#define	InternalDSRPtrTopLevelRectHeight(isr)	\
				SiteRectHeight((isr)->toplevel_rect)

#if defined(att) || defined(USL)
#define XYInInternalDSR(dsr, x, y)                                      \
                (((int)(x) >= (int)(InternalDSRPtrTopLevelRectX(dsr))   &&   \
                  (int)(x) <= (int)(InternalDSRPtrTopLevelRectX(dsr)  +      \
                          InternalDSRPtrTopLevelRectWidth(dsr))) &&     \
                 ((int)(y) >= (int)(InternalDSRPtrTopLevelRectY(dsr))   &&   \
                  (int)(y) <= (int)(InternalDSRPtrTopLevelRectY(dsr)  +      \
                          InternalDSRPtrTopLevelRectHeight(dsr))))
#else
#define XYInInternalDSR(dsr, x, y)                                      \
                (((int)(x) >= InternalDSRPtrTopLevelRectX(dsr)   &&     \
                  (int)(x) <= (InternalDSRPtrTopLevelRectX(dsr)  +      \
                          InternalDSRPtrTopLevelRectWidth(dsr))) &&     \
                 ((int)(y) >= InternalDSRPtrTopLevelRectY(dsr)   &&     \
                  (int)(y) <= (InternalDSRPtrTopLevelRectY(dsr)  +      \
                          InternalDSRPtrTopLevelRectHeight(dsr))))
#endif

/**********************
 * OlDnDDropSite
 **********************/

typedef	struct _ds_selection_atom *DSSelectionAtomPtr; /* foward */

typedef	enum	_oldnd_drop_site_state	{
					DropSiteState0,
					DropSitePreviewing,
					DropSiteTrigger,
					DropSiteAck,
					DropSiteBegin,
					DropSiteTx,
					DropSiteEnd,
					DropSiteDisinterested,
					DropSiteInSensitive,
					DropSiteError
} OlDnDDropSiteState, *OlDnDDropSiteStatePtr;

typedef	struct	_oldnd_drop_site *OlDnDDropSitePtr;

typedef	struct	_oldnd_drop_site {
	OlDnDDropSitePtr	next_site;
	
	Widget				owner;
	Window				window;
	Window				event_window;
	OlDnDSitePreviewHints		preview_hints;
	InternalDSRPtr			toplevel_rects;
	unsigned int			num_rects;
	Time				timestamp;
	OlDnDDropSiteState		state;
	OlDnDTriggerMessageNotifyProc	trigger_notify;
	OlDnDPreviewMessageNotifyProc	preview_notify;
	Boolean				on_interest;
	Boolean				got_ack;
	Boolean				incoming_transient;
	XtPointer			closure;
} OlDnDDropSite/*,*OlDnDDropSitePtr , *OlDnDDropSiteID */;

#define	OlDnDDropSiteNextSite(ds)		((ds).next_site)
#define	OlDnDDropSitePtrNextSite(ds)		((ds)->next_site)

#define	OlDnDDropSiteOwner(ds)			((ds).owner)
#define	OlDnDDropSitePtrOwner(ds)		((ds)->owner)

#define	OlDnDDropSiteWindow(ds)			((ds).window)
#define	OlDnDDropSitePtrWindow(ds)		((ds)->window)

#define	OlDnDDropSiteEventWindow(ds)		((ds).event_window)
#define	OlDnDDropSitePtrEventWindow(ds)		((ds)->event_window)

#define	OlDnDDropSitePreviewHints(ds)		((ds).preview_hints)
#define	OlDnDDropSitePtrPreviewHints(ds)	((ds)->preview_hints)

#define	OlDnDDropSiteTopLevelRects(ds)		((ds).toplevel_rects)
#define	OlDnDDropSitePtrTopLevelRects(ds)	((ds)->toplevel_rects)

#define	OlDnDDropSiteNumRects(ds)		((ds).num_rects)
#define	OlDnDDropSitePtrNumRects(ds)		((ds)->num_rects)

#define	OlDnDDropSiteTimestamp(ds)		((ds).timestamp)
#define	OlDnDDropSitePtrTimestamp(ds)		((ds)->timestamp)

#define	OlDnDDropSiteState(ds)			((ds).state)
#define	OlDnDDropSitePtrState(ds)		((ds)->state)

#define	OlDnDDropSiteTriggerNotify(ds)		((ds).trigger_notify)
#define	OlDnDDropSitePtrTriggerNotify(ds)	((ds)->trigger_notify)

#define	OlDnDDropSitePreviewNotify(ds)		((ds).preview_notify)
#define	OlDnDDropSitePtrPreviewNotify(ds)	((ds)->preview_notify)

#define	OlDnDDropSiteGotAck(ds)			((ds).got_ack)
#define	OlDnDDropSitePtrGotAck(ds)		((ds)->got_ack)

#define	OlDnDDropSiteOnInterest(ds)		((ds).on_interest)
#define	OlDnDDropSitePtrOnInterest(ds)		((ds)->on_interest)

#define	OlDnDDropSiteIncomingTransient(ds)	((ds).incoming_transient)
#define	OlDnDDropSitePtrIncomingTransient(ds)	((ds)->incoming_transient)

#define	OlDnDDropSiteClosure(ds)		((ds).closure)
#define	OlDnDDropSitePtrClosure(ds)		((ds)->closure)

#define IfOlDnDDropSitePreviewHintsON(sd, hints)			\
	((OlDnDDropSitePreviewHints(sd) & (hints)) == (hints))

#define IfOlDnDDropSitePtrPreviewHintsON(sd, hints)			\
	((OlDnDDropSitePtrPreviewHints(sd) & (hints)) == (hints))

#define	OlDnDDropSiteWantsPreviewEnterLeave(sd)			          \
	(IfOlDnDDropSitePreviewHintsON(sd, OlDnDSitePreviewEnterLeave) || \
	 IfOlDnDDropSitePreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	OlDnDDropSitePtrWantsPreviewEnterLeave(sd)			  \
	(IfOlDnDDropSitePtrPreviewHintsON(sd, OlDnDSitePreviewEnterLeave) \
	 || IfOlDnDDropSitePtrPreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	OlDnDDropSiteWantsPreviewMotion(sd)				  \
	(IfOlDnDDropSitePreviewHintsON(sd, OlDnDSitePreviewMotion) ||     \
	 IfOlDnDDropSitePreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	OlDnDDropSitePtrWantsPreviewMotion(sd)			          \
	(IfOlDnDDropSitePtrPreviewHintsON(sd, OlDnDSitePreviewMotion) ||  \
	 IfOlDnDDropSitePtrPreviewHintsON(sd, OlDnDSitePreviewBoth))

#define	OlDnDDropSiteIsInsensitive(sd)				  	  \
	IfOlDnDDropSitePreviewHintsON(sd, OlDnDSitePreviewInsensitive)

#define	OlDnDDropSitePtrIsInsensitive(sd)			          \
	IfOlDnDDropSitePtrPreviewHintsON(sd, OlDnDSitePreviewInsensitive)

#define	OlDnDDropSiteIsDefaultSite(sd)			                  \
	IfOlDnDDropSitePreviewHintsON(sd, OlDnDSitePreviewDefaultSite)

#define	OlDnDDropSitePtrIsDefaultSite(sd)			        \
	IfOlDnDDropSitePtrPreviewHintsON(sd, OlDnDSitePreviewDefaultSite)

#define	OlDnDDropSiteIsSensitive(ds)					\
		(OlDnDDropSiteOwner(ds)->core.sensitive &&		\
		 OlDnDDropSiteOwner(ds)->core.ancestor_sensitive &&	\
		 OlDnDDropSiteOnInterest(ds) &&				\
		 !OlDnDDropSiteIsInsensitive(ds))
		 

#define	OlDnDDropSitePtrIsSensitive(ds)					\
		(OlDnDDropSitePtrOwner(ds)->core.sensitive &&		\
		 OlDnDDropSitePtrOwner(ds)->core.ancestor_sensitive &&	\
		 OlDnDDropSitePtrOnInterest(ds) &&			\
		 !OlDnDDropSitePtrIsInsensitive(ds))

/*********************************
 *
 * InternalDSDMSR 
 *
 *********************************/

typedef struct _internal_dsdm_site_rect	*InternalDSDMSRPtr;

typedef	struct _internal_dsdm_site_rect {
		InternalDSDMSRPtr	next,
					prev;
		Boolean			in_site;

		DSDMSiteRectPtr		rect;
} InternalDSDMSR/*, InternalDSDMSRPtr */;

#define	InternalDSDMSRNext(sr)		((sr).next)
#define	InternalDSDMSRPtrNext(sr)	((sr)->next)

#define	InternalDSDMSRPrev(sr)		((sr).prev)
#define	InternalDSDMSRPtrPrev(sr)	((sr)->prev)

#define	InternalDSDMSRInSite(sr)	((sr).in_site)
#define	InternalDSDMSRPtrInSite(sr)	((sr)->in_site)

#define	InternalDSDMSRRect(sr)		((sr).rect)
#define	InternalDSDMSRPtrRect(sr)	((sr)->rect)

#define	InternalDSDMSRRectX(sr)		\
			DSDMSiteRectPtrX(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectX(sr)	\
			DSDMSiteRectPtrX(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectY(sr)		\
			DSDMSiteRectPtrY(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectY(sr)	\
			DSDMSiteRectPtrY(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectWidth(sr)		\
			DSDMSiteRectPtrWidth(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectWidth(sr)		\
			DSDMSiteRectPtrWidth(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectHeight(sr)		\
			DSDMSiteRectPtrHeight(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectHeight(sr)		\
			DSDMSiteRectPtrHeight(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectHeight(sr)		\
			DSDMSiteRectPtrHeight(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectHeight(sr)		\
			DSDMSiteRectPtrHeight(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectWindowID(sr)		\
			DSDMSiteRectPtrWindowID(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectWindowID(sr)		\
			DSDMSiteRectPtrWindowID(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectScreenNumber(sr)		\
			DSDMSiteRectPtrScreenNumber(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectScreenNumber(sr)		\
			DSDMSiteRectPtrScreenNumber(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectSiteID(sr)		\
			DSDMSiteRectPtrSiteID(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectSiteID(sr)		\
			DSDMSiteRectPtrSiteID(InternalDSDMSRPtrRect(sr))

#define	InternalDSDMSRRectPreviewHints(sr)		\
			DSDMSiteRectPtrPreviewHints(InternalDSDMSRRect(sr))
#define	InternalDSDMSRPtrRectPreviewHints(sr)		\
			DSDMSiteRectPtrPreviewHints(InternalDSDMSRPtrRect(sr))

#define XYInInternalDSDMSR(dsr, screen, x, y)                            \
		(((screen) == InternalDSDMSRPtrRectScreenNumber(dsr)) && \
                 ((int)(x) >= InternalDSDMSRPtrRectX(dsr)   &&           \
                  (int)(x) <= (InternalDSDMSRPtrRectX(dsr)  +            \
                          InternalDSDMSRPtrRectWidth(dsr))) &&           \
                 ((int)(y) >= InternalDSDMSRPtrRectY(dsr)   &&           \
                  (int)(y) <= (InternalDSDMSRPtrRectY(dsr)   +           \
                          InternalDSDMSRPtrRectHeight(dsr))))

#define	IfInternalDSDMSRRectPreviewHintsON(sr, hints)		     	      \
	((InternalDSDMSRRectPreviewHints(sr) & (hints)) == (hints))

#define	IfInternalDSDMSRPtrRectPreviewHintsON(sr, hints)	     	      \
	((InternalDSDMSRPtrRectPreviewHints(sr) & (hints)) == (hints))

#define	InternalDSDMSRRectWantsPreviewEnterLeave(sr)		     	      \
	(IfInternalDSDMSRRectPreviewHintsON(sr, OlDnDSitePreviewEnterLeave) ||\
	 IfInternalDSDMSRRectPreviewHintsON(sr, OlDnDSitePreviewBoth))

#define	InternalDSDMSRPtrRectWantsPreviewEnterLeave(sr)			       \
	(IfInternalDSDMSRPtrRectPreviewHintsON(sr, OlDnDSitePreviewEnterLeave) \
	 || IfInternalDSDMSRPtrRectPreviewHintsON(sr, OlDnDSitePreviewBoth))

#define	InternalDSDMSRRectWantsPreviewMotion(sr)			   \
	(IfInternalDSDMSRRectPreviewHintsON(sr, OlDnDSitePreviewMotion) || \
	 IfInternalDSDMSRRectPreviewHintsON(sr, OlDnDSitePreviewBoth))

#define	InternalDSDMSRPtrRectWantsPreviewMotion(sr)			     \
	(IfInternalDSDMSRPtrRectPreviewHintsON(sr, OlDnDSitePreviewMotion) ||\
	 IfInternalDSDMSRPtrRectPreviewHintsON(sr, OlDnDSitePreviewBoth))

#define	InternalDSDMSRRectIsDefaultSite(sr)			    \
	IfInternalDSDMSRRectPreviewHintsON(sr, OlDnDSitePreviewDefaultSite)

#define	InternalDSDMSRPtrRectIsDefaultSite(sr)			    \
	IfInternalDSDMSRPtrRectPreviewHintsON(sr, OlDnDSitePreviewDefaultSite)

/******************************
 *
 * TransientAtomList
 *
 ******************************/

typedef struct _transient_atom_list {
			unsigned int	used;
			unsigned int	alloc;
			struct  _transient_atoms {
				Atom		atom;
				Widget		owner;
			} atoms[1]; /* dummy */
} TransientAtomList, *TransientAtomListPtr;

#define	TransientAtomListUsed(tal)		((tal).used)
#define	TransientAtomListPtrUsed(tal)		((tal)->used)

#define	TransientAtomListAlloc(tal)		((tal).alloc)
#define	TransientAtomListPtrAlloc(tal)		((tal)->alloc)

#define	TransientAtomListAtoms(tal)		((tal).atoms)
#define	TransientAtomListPtrAtoms(tal)		((tal)->atoms)

#define	TransientAtomListAtom(tal,idx)		((tal).atoms[(idx)].atom)
#define	TransientAtomListPtrAtom(tal, idx)	((tal)->atoms[(idx)].atom)

#define	TransientAtomListOwner(tal,idx)			\
					((tal).atoms[(idx)].owner)
#define	TransientAtomListPtrOwner(tal,idx)		\
					((tal)->atoms[(idx)].owner)

#define	SizeOfTransientAtomListHead		\
		(sizeof(TransientAtomList) - sizeof(struct _transient_atoms))

#define	SizeOfTransientAtomListForNAtoms(n)	\
		(SizeOfTransientAtomListHead +  \
		 ((n) * sizeof(struct _transient_atoms)))

#define	NeedsLargerTransientAtomList(tal)	\
		(TransientAtomListPtrUsed(tal) == TransientAtomListPtrAlloc(tal))

/***************
 *
 * ReqProcClosure
 *
 ***************/

typedef	struct _req_proc_closure {
		Widget				widget;
		OlDnDProtocolActionCallbackProc callback;
		Atom				action;
		XtPointer			closure;
} ReqProcClosure, *ReqProcClosurePtr;

#define	ReqProcClosureWidget(pc)	((pc).widget)
#define	ReqProcClosurePtrWidget(pc)	((pc)->widget)

#define	ReqProcClosureCallback(pc)	((pc).callback)
#define	ReqProcClosurePtrCallback(pc)	((pc)->callback)

#define	ReqProcClosureAction(pc)	((pc).action)
#define	ReqProcClosurePtrAction(pc)	((pc)->action)

#define	ReqProcClosureClosure(pc)	((pc).closure)
#define	ReqProcClosurePtrClosure(pc)	((pc)->closure)

/******************
 *
 * OwnerProcClosure
 *
 ******************/

typedef struct _owner_proc_closure *OwnerProcClosurePtr;

typedef void  (*OlDnDTransactionCleanupProc) OL_ARGS (( OwnerProcClosurePtr ));

typedef struct _owner_proc_closure {
		Atom				transient;
		DSSelectionAtomPtr		assoc;
		XtConvertSelectionProc		convert_proc;
		XtLoseSelectionProc		lose_proc;
		XtSelectionDoneProc		done_proc;

		XtConvertSelectionIncrProc	convert_incr_proc;
		XtLoseSelectionIncrProc		lose_incr_proc;
		XtSelectionDoneIncrProc		done_incr_proc;
		XtCancelConvertSelectionProc	cancel_incr_proc;
		OlDnDTransactionStateCallback   state_proc;
		XtPointer			client_data;
		OlDnDTransactionCleanupProc	cleanup_proc;
		Boolean				selection_transient;
} OwnerProcClosure/*, *OwnerProcClosurePtr*/;

#define	OwnerProcClosureTransient(opc)		((opc).transient)
#define	OwnerProcClosurePtrTransient(opc)	((opc)->transient)

#define	OwnerProcClosureAssoc(opc)		((opc).assoc)
#define	OwnerProcClosurePtrAssoc(opc)		((opc)->assoc)

#define	OwnerProcClosureAssoc(opc)		((opc).assoc)
#define	OwnerProcClosurePtrAssoc(opc)		((opc)->assoc)

#define	OwnerProcClosureConvertProc(opc)	((opc).convert_proc)
#define	OwnerProcClosurePtrConvertProc(opc)	((opc)->convert_proc)

#define	OwnerProcClosureLoseProc(opc) 		((opc).lose_proc)
#define	OwnerProcClosurePtrLoseProc(opc) 	((opc)->lose_proc)

#define	OwnerProcClosureDoneProc(opc)		((opc).done_proc)
#define	OwnerProcClosurePtrDoneProc(opc)	((opc)->done_proc)

#define	OwnerProcClosureDoneIncrProc(opc)	((opc).done_incr_proc)
#define	OwnerProcClosurePtrDoneIncrProc(opc)	((opc)->done_incr_proc)

#define	OwnerProcClosureCancelIncrProc(opc)	((opc).cancel_incr_proc)
#define	OwnerProcClosurePtrCancelIncrProc(opc)	((opc)->cancel_incr_proc)

#define	OwnerProcClosureConvertIncrProc(opc)	((opc).convert_incr_proc)
#define	OwnerProcClosurePtrConvertIncrProc(opc)	((opc)->convert_incr_proc)

#define	OwnerProcClosureLoseIncrProc(opc)	((opc).lose_incr_proc)
#define	OwnerProcClosurePtrLoseIncrProc(opc)	((opc)->lose_incr_proc)

#define	OwnerProcClosureStateProc(opc)		((opc).state_proc)
#define	OwnerProcClosurePtrStateProc(opc)	((opc)->state_proc)

#define	OwnerProcClosureClientData(opc)		((opc).client_data)
#define	OwnerProcClosurePtrClientData(opc)	((opc)->client_data)

#define	OwnerProcClosureCleanupProc(opc)	((opc).cleanup_proc)
#define	OwnerProcClosurePtrCleanupProc(opc)	((opc)->cleanup_proc)

#define	OwnerProcClosureSelectionTransient(opc)	((opc).selection_transient)

#define	OwnerProcClosurePtrSelectionTransient(opc)	\
						((opc)->selection_transient)

/***************
 *
 * DSSelectionAtom
 *
 ***************/

typedef	struct _ds_selection_atom {
	DSSelectionAtomPtr	next;		/* chain in Part Extension */
	Widget			owner;
	Atom			selection_atom;
	Time			timestamp;
	OwnerProcClosurePtr	closure;
	Window			requestor_window;
	Display			*requestor_display;
} DSSelectionAtom/*,*DSSelectionAtomPtr*/;

#define	DSSelectionAtomNext(dssa)	((dssa).next)
#define	DSSelectionAtomPtrNext(dssa)	((dssa)->next)

#define	DSSelectionAtomOwner(dssa)		((dssa).owner)
#define	DSSelectionAtomPtrOwner(dssa)		((dssa)->owner)

#define	DSSelectionAtomSelectionAtom(dssa)	((dssa).selection_atom)
#define	DSSelectionAtomPtrSelectionAtom(dssa)	((dssa)->selection_atom)

#define	DSSelectionAtomTimestamp(dssa)		((dssa).timestamp)
#define	DSSelectionAtomPtrTimestamp(dssa)	((dssa)->timestamp)

#define	DSSelectionAtomClosure(dssa)		((dssa).closure)
#define	DSSelectionAtomPtrClosure(dssa)		((dssa)->closure)

#define	DSSelectionAtomRequestorWindow(dssa)	((dssa).requestor_window)
#define	DSSelectionAtomPtrRequestorWindow(dssa)	((dssa)->requestor_window)

#define	DSSelectionAtomRequestorDisplay(dssa)	 ((dssa).requestor_display)
#define	DSSelectionAtomPtrRequestorDisplay(dssa) ((dssa)->requestor_display)

#define FreeAllSelectionAtoms		((Atom)-1)
#endif	/* _OlDnDVCXI_h_ */
