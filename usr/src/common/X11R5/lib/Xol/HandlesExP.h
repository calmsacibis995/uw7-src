#ifndef	NOIDENT
#ident	"@(#)handles:HandlesExP.h	1.5"
#endif

#ifndef _HANDLESEXTP_H
#define _HANDLESEXTP_H

#include "array.h"

/*
 * Extension version and name:
 */

#define OlHandlesCoreClassExtensionVersion	1
#define XtNHandlesCoreClassExtension		"HandlesCoreClassExtension"

extern XrmQuark		XtQHandlesCoreClassExtension;

/*
 * Instance extension record:
 */

typedef struct HandlesWidgetExtensionRec {
	Widget			self;
	Widget			handles;
	WidgetArray *		set_list;
	struct handles_border {
		Dimension		horz;
		Dimension		vert;
	}			handles_border;
}			HandlesWidgetExtensionRec,
		      * HandlesWidgetExtension;

#define _OlHandlesDo		0x01
#define _OlHandlesNotOKHint	0x02
#define _OlHandlesActive		0x04

/*
 * Class extension record:
 */

typedef void		(*OlHandlesProc) OL_ARGS((
	Widget			w,
	Widget			handles
));

typedef
    _OlArrayStruct(HandlesWidgetExtensionRec, HandlesWidgetExtensionArray)
	HandlesWidgetExtensionArray;

typedef struct HandlesCoreClassExtensionRec {
	/*
	 * Common:
	 */
	XtPointer			next_extension;
	XrmQuark			record_type;
	long				version;
	Cardinal			record_size;
	/*
	 * Handles Extension, public:
	 */
	OlHandlesProc			update_handles;
	/*
	 * Handles Extension, private:
	 */
	HandlesWidgetExtensionArray	widgets;
}				HandlesCoreClassExtensionRec,
			      * HandlesCoreClassExtension;

#define XtInheritUpdateHandles	((OlHandlesProc)_XtInherit)

/*
 * Public routines for widget writers:
 */

OLBeginFunctionPrototypeBlock

extern void
OlUpdateHandles OL_ARGS((
	Widget			w
));
extern void
OlCheckRealize OL_ARGS((
	Widget			w
));
extern void
OlCheckInsertedChild OL_ARGS((
	Widget			w
));
extern void
OlCheckDeletedChild OL_ARGS((
	Widget			w
));
extern Boolean
OlIsHandles OL_ARGS((
	Widget			w
));
extern Boolean
OlIsHandlesWidget OL_ARGS((
	Widget			parent,
	Widget			w
));
extern Dimension
OlHandlesBorderThickness OL_ARGS((
	Widget			child,
	OlDefine		orientation
));
extern void
OlRealizeHandles OL_ARGS((
	Widget			w
));
extern void
OlUnrealizeHandles OL_ARGS((
	Widget			w
));
extern void
OlClearHandles OL_ARGS((
	Widget			w
));
extern void
OlLayoutHandles OL_ARGS((
	Widget			w
));
extern void
OlHandlesSetSelection OL_ARGS((
	Widget			w,
	Widget *		panes,
	Cardinal		num_panes,
	Boolean			selected
));
extern void
OlHandlesGetSelection OL_ARGS((
	Widget			w,
	Widget **		panes,
	Cardinal *		num_panes
));

OLEndFunctionPrototypeBlock

/*
 * Private routines:
 */

OLBeginFunctionPrototypeBlock

extern void
_OlInitializeHandlesCoreClassExtension OL_ARGS((
	void
));
extern void
_OlDefaultRealize OL_ARGS((
	Widget			w,
	XtValueMask *		value_mask,
	XSetWindowAttributes *	attributes
));
extern void
_OlDefaultInsertChild OL_ARGS((
	Widget			w
));
extern void
_OlDefaultDeleteChild OL_ARGS((
	Widget			w
));
extern void
_OlDefaultConstraintInitialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			/* new */,
	ArgList			args,
	Cardinal *		num_args
));
extern Boolean
_OlDefaultConstraintSetValues OL_ARGS((
	Widget			current,	/*NOTUSED*/
	Widget			request,	/*NOTUSED*/
	Widget			/* new */,
	ArgList			args,
	Cardinal *		num_args
));
extern void
_OlDefaultConstraintGetValuesHook OL_ARGS((
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
));
extern void
_OlDefaultUpdateHandles OL_ARGS((
	Widget			w,
	Widget			handles
));

OLEndFunctionPrototypeBlock

#endif
