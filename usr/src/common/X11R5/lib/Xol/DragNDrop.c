/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

/*#ident	"@(#)olmisc:DragNDrop.c	1.5"*/
#ifndef NOIDENT
#ident	"@(#)olmisc:DragNDrop.c	1.9"
#endif

/*
 * Description:
 *		This file contains routines for the drag and drop
 *		operations.
 */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <DnD/OlDnDVCX.h>		/* drag and drop */

extern OlDnDDragKeyStatus
		_OlHandleDragKey OL_ARGS((Widget, XEvent *));


extern Boolean	OlDragAndDrop OL_ARGS((
			Widget, Window *, Position *, Position *));

extern void	OlGrabDragPointer OL_ARGS((Widget, Cursor, Window));

extern void	OlUngrabDragPointer OL_ARGS((Widget));

/*
 * OlDragAndDrop
 *
 * The \fIOlDragAndDrop\fR function is used to monitor a direct
 * manipulation operation; returning, when the operation is completed,
 * the \fIdrop_window\fR and the \fIx\fR and \fIy\fR coordinates
 * corresponding to the location of the drop.  These return values
 * will reflect the highest (in the stacking order) window located
 * under the pointer at the time of the button release.
 *
 * OlDragAndDrop returns True if the drop terminated normally, False
 * if it was aborted (i.e., via the cancel key)
 *
 */
extern Boolean
OlDragAndDrop OLARGLIST((w, window, xPosition, yPosition))
	OLARG( Widget,		w)
	OLARG( Window *,	window)
	OLARG( Position *,	xPosition)
	OLGRA( Position *,	yPosition)
{
	return OlDnDDragAndDrop(w, window, xPosition, yPosition, 
				(OlDnDDragDropInfoPtr)NULL,
				(OlDnDPreviewAnimateCbP)NULL,
				(XtPointer)NULL);
} /* for backwards compatibility */

/*
**  If I'm in the process of a drag-and-drop operation, this routine
**  provides special handling for keys. 
*/
extern OlDnDDragKeyStatus
_OlHandleDragKey OLARGLIST((w, pevent))
	OLARG( Widget,		w)
	OLGRA( XEvent *,	pevent)
{
#define	DONT_CARE	(Button1Mask | Button2Mask |			\
			 Button3Mask | Button4Mask | Button5Mask)

	OlVirtualEventRec	nve;
	Position		deltax = 0,
				deltay = 0;
	Window			root;
	int			rootx,
				rooty;
	OlDnDDragKeyStatus	retval = OlDnDStillDragging;

	root  = pevent->xkey.root;
	rootx = pevent->xkey.x_root;
	rooty = pevent->xkey.y_root;

	pevent->xkey.state &= ~DONT_CARE;
	OlLookupInputEvent(w, pevent, &nve, OL_CORE_IE);

	switch (nve.virtual_name) {
	case OL_MOVERIGHT:
		deltax = (Position)1;
		break;
	case OL_MOVELEFT:
		deltax = (Position)-1;
		break;
	case OL_MOVEUP:
		deltay = (Position)-1;
		break;
	case OL_MOVEDOWN:
		deltay = (Position)1;
		break;
	case OL_MULTIRIGHT:
		deltax = _OlGetMultiObjectCount(w) * 2;
		break;
	case OL_MULTILEFT:
		deltax = -_OlGetMultiObjectCount(w) * 2;
		break;
	case OL_MULTIUP:
		deltay = -_OlGetMultiObjectCount(w) * 2;
		break;
	case OL_MULTIDOWN:
		deltay = _OlGetMultiObjectCount(w) * 2;
		break;
	case OL_DEFAULTACTION:
	case OL_DROP:
		retval = OlDnDDropped;
		break;
	case OL_CANCEL:
		retval = OlDnDCanceled;
		break;
	case OL_DRAG:
		_OlBeepDisplay(w, 1);
		break;
	default:
		break;
	}

		/* do it only when necessary...		*/
	if (deltax != 0 || deltay != 0)
	{
		XWarpPointer(
			XtDisplayOfObject(w),
			None, RootWindowOfScreen(XtScreenOfObject(w)),
			0, 0, 0, 0, rootx+deltax, rooty+deltay
		);
	}

	return(retval);

#undef	DONT_CARE
} /* end of HandleDragKey */

/*
 * OlGrabDragPointer
 *
 * The \fIOlGrabDragPointer\fR procedure is used to effect an active
 * grab of the mouse pointer and the keyboard.  This function is normally
 * called after a mouse drag operation is experienced and prior to calling
 * the OlDragAndDrop/OlDnDDragAndDrop procedure which is used to monitor
 * a drag operation.
 *
 */
extern void
OlGrabDragPointer OLARGLIST((w, cursor, window))
	OLARG( Widget,	w)
	OLARG( Cursor,	cursor)
	OLGRA( Window,	window)
{
	OlDnDGrabDragCursor(w, cursor, window);
} /* end of OlGrabDragPointer */

/*
 * OlUngrabDragPointer
 *
 * The \fIOlUngrabDragPointer\fR procedure is used to relinquish the
 * active pointer grab which was initiated by the OlGrabDragPointer
 * procedure.  This function simply ungrabs the pointer and the keyboard.
 *
 */
extern void
OlUngrabDragPointer OLARGLIST((w))
	OLGRA( Widget,	w)
{
	OlDnDUngrabDragCursor(w);
} /* end of OlUngrabDragPointer */
