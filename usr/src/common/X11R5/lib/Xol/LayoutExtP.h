#ifndef	NOIDENT
#ident	"@(#)layout:LayoutExtP.h	1.12"
#endif

#ifndef _LAYOUTEXTP_H
#define _LAYOUTEXTP_H

#include "array.h"

/*
 * Extension version and name:
 */

#define OlLayoutCoreClassExtensionVersion	1
#define XtNLayoutCoreClassExtension		"LayoutCoreClassExtension"

extern XrmQuark		XtQLayoutCoreClassExtension;

/*
 * Instance extension record:
 */

typedef struct LayoutWidgetExtensionRec {
	Widget			self;
	unsigned char		layout_flags;
}			LayoutWidgetExtensionRec,
		      * LayoutWidgetExtension;

#define _OlLayoutDo			0x01
#define _OlLayoutNotOKHint		0x02
#define _OlLayoutActive			0x04
#define _OlLayoutInChangeManaged	0x08
#define _OlLayoutInGeometryManager	0x10

/*
 * Class extension record:
 */

typedef void		(*OlLayoutProc) OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));

typedef
    _OlArrayStruct(LayoutWidgetExtensionRec, LayoutWidgetExtensionArray)
	LayoutWidgetExtensionArray;

typedef struct LayoutCoreClassExtensionRec {
	/*
	 * Common:
	 */
	XtPointer			next_extension;
	XrmQuark			record_type;
	long				version;
	Cardinal			record_size;
	/*
	 * Layout Extension, public:
	 */
	OlLayoutProc			layout;
	XtGeometryHandler		query_alignment;
	/*
	 * Layout Extension, private:
	 */
	LayoutWidgetExtensionArray	widgets;
}				LayoutCoreClassExtensionRec,
			      * LayoutCoreClassExtension;

#define XtInheritLayout		((OlLayoutProc)_XtInherit)
#define XtInheritQueryAlignment	((XtGeometryHandler)_XtInherit)

/*
 * Layout resources type:
 */

typedef struct OlLayoutResources {
	OlDefine		width;
	OlDefine		height;
	unsigned char		flags;
}			OlLayoutResources;

#define OlLayoutWidthNotSet	0x01
#define OlLayoutHeightNotSet	0x02

/*
 * Public routines for widget writers:
 */

OLBeginFunctionPrototypeBlock

extern XtGeometryResult
OlQueryGeometryFixedBorder OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
));
extern void
OlLayoutWidget OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
extern void
OlSimpleLayoutWidget OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			cached_best_fit_ok_hint
));
extern Boolean
OlLayoutWidgetIfLastClass OL_ARGS((
	Widget			w,
	WidgetClass		wc,
	Boolean			do_layout,
	Boolean			cached_best_fit_ok_hint
));
extern void
OlAvailableGeometry OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred,
	XtWidgetGeometry *	available
));
extern void
OlInitializeGeometry OL_ARGS((
	Widget			w,
	OlLayoutResources *	layout,
	Dimension		width,
	Dimension		height
));
extern void
OlConfigureChild OL_ARGS((
	Widget			child,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	Dimension		border_width,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	response
));
extern void
OlAdjustGeometry OL_ARGS((
	Widget			w,
	OlLayoutResources *	layout,
	XtWidgetGeometry *	best_fit,
	XtWidgetGeometry *	preferred
));
extern void
OlCheckLayoutResources OL_ARGS((
	Widget			w,
	OlLayoutResources *	/* new */,
	OlLayoutResources *	current
));
extern void
OlResolveGravity OL_ARGS((
	Widget			w,
	int			gravity,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	constrain,
	XtWidgetGeometry *	allocated,
	XtWidgetGeometry *	preferred,
	XtWidgetGeometry *	optimum,
	XtGeometryHandler	query_geometry
));
extern void
OlQueryChildGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	preferred
));
extern void
OlQueryGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	preferred,
	XtGeometryHandler	query_geometry
));
extern void
OlSetMinHints OL_ARGS((
	Widget			w
));
extern void
OlClearMinHints OL_ARGS((
	Widget			w
));
extern Boolean
OlIsLayoutActive OL_ARGS((
	Widget			w
));
extern XtGeometryResult
OlQueryAlignment OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));

OLEndFunctionPrototypeBlock

/*
 * Private routines:
 */

OLBeginFunctionPrototypeBlock

extern void
_OlInitializeLayoutCoreClassExtension OL_ARGS((
	void
));
extern void
_OlDefaultResize OL_ARGS((
	Widget			w
));
extern XtGeometryResult
_OlDefaultQueryGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
));
extern XtGeometryResult
_OlDefaultGeometryManager OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	_request,
	XtWidgetGeometry *	reply
));
extern void
_OlDefaultChangeManaged OL_ARGS((
	Widget			w
));

OLEndFunctionPrototypeBlock

#endif
