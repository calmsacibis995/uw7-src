#ifndef	NOIDENT
#ident	"@(#)oldnd:OlDnDUtil.h	1.13"
#endif

/*
 * OlDnDUtil.h -
 *
 * This header contains additions from USL. It includes:
 *
 *	A. routines for registering Desktop DnD Interest from an applic (2).
 *	B. routine for making Desktop DnD GUI independent (1).
 *	C. routines for starting, tracking, ending the DnD gesture (5).
 *	D. routines for registering the Open Windows 2.0 DnD and
 *		for delivering the trigger message (3).
 *	E. routine for getting an atom from a given display and a given
 *		atom name (1)
 *
 */

#ifndef _Ol_DnDUtil_h_
#define _Ol_DnDUtil_h_

	/* The following are defined in ICCCM...		*/
	/*	Note that XA_STRING is in Xatom.h		*/
	/*	Also note that "MULTIPLE" and "TIMESTAMP" are	*/
	/*		handled by Intrinsic automatically...	*/
#define OL_XA_TARGETS(d)	XInternAtom(d, "TARGETS", False)

#define OL_XA_TEXT(d)		XInternAtom(d, "TEXT", False)
#define OL_XA_COMPOUND_TEXT(d)	XInternAtom(d, "COMPOUND_TEXT", False)
#define OL_XA_FILE_NAME(d)	XInternAtom(d, "FILE_NAME", False)
#define OL_XA_HOST_NAME(d)	XInternAtom(d, "HOST_NAME", False)
#define OL_XA_DELETE(d)		XInternAtom(d, "DELETE", False)

	/* The following atoms are for Desktop DnD		*/
	/* Subject to change					*/
#define OL_USL_ITEM(d)		XInternAtom(d, "_USL_ITEM", False)
#define OL_USL_NUM_ITEMS(d)	XInternAtom(d, "_USL_NUM_ITEMS", False)

	/* This struture will be used when calling OlDnDTrackDragCursor	*/
typedef struct {
	Cursor			yes_cursor;
	Cursor			no_cursor;
} OlDnDAnimateCursors, *OlDnDAnimateCursorsPtr;

typedef struct {
	Window			window;	/* destination window id	*/
	Position		x,	/* coord is relative to window	*/
				y;
} OlDnDDestinationInfo, *OlDnDDestinationInfoPtr;

	/* this definition will be used by OlDnDDragKeyProc	*/
typedef enum {
	OlDnDCanceled,
	OlDnDDropped,
	OlDnDStillDragging,
} OlDnDDragKeyStatus;

typedef enum {
	OlDnDDropFailed,
	OlDnDDropSucceeded,
	OlDnDDropCanceled,
} OlDnDDropStatus;

typedef OlDnDDragKeyStatus
			(*OlDnDDragKeyProc) OL_ARGS((Widget, XEvent *));

OLBeginFunctionPrototypeBlock

extern void		OlDnDVCXInitialize OL_NO_ARGS();

extern void		OlDnDGrabDragCursor OL_ARGS((
				Widget, Cursor, Window));

extern void		OlDnDRegisterDragKeyProc OL_ARGS((
				OlDnDDragKeyProc));

extern void		OlDnDRegisterDDI OL_ARGS((
				Widget, OlDnDSitePreviewHints,
				OlDnDTMNotifyProc, OlDnDPMNotifyProc,
				Boolean, XtPointer));

extern Boolean		OlDnDSendTriggerMessage OL_ARGS((
				Widget, Window, Window, Atom,
				OlDnDTriggerOperation, Time));

extern OlDnDDropStatus	OlDnDTrackDragCursor OL_ARGS((
				Widget, OlDnDAnimateCursorsPtr,
				OlDnDDestinationInfoPtr, OlDnDDragDropInfoPtr));

extern void		OlDnDUngrabDragCursor OL_ARGS((Widget));


OLEndFunctionPrototypeBlock

#endif /* _Ol_DnDUtil_h_ */
