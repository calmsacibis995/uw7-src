#ifndef EXM_FLAT_H
#define EXM_FLAT_H

#ifndef	NOIDENT
#pragma	ident	"@(#)libMDtI:Flat.h	1.7"
#endif


/************************************************************************
  Description:
	This is the flat container meta class's public header file.
*/

#include <Xm/Xm.h>
#include "WidePosDef.h"

/************************************************************************
  Define class and instance pointers:
	- extern pointer to class data/procedures
	- typedef pointer to widget's class structure
	- typedef pointer to widget's instance structure
*/

externalref WidgetClass		exmFlatWidgetClass;
typedef struct _ExmFlatClassRec*ExmFlatWidgetClass;
typedef struct _ExmFlatRec *	ExmFlatWidget;

			/* Convenience macro to check for subclass of Flat */
#ifndef ExmIsFlat
#define ExmIsFlat(w)	XtIsSubclass(w, exmFlatWidgetClass)
#endif

/************************************************************************
  Define Flat resource strings
*/
#define XmNitemFields 		(_XmConst char *)"itemFields"
#define XmCItemFields 		(_XmConst char *)"ItemFields"
#define XmNitemsTouched 	(_XmConst char *)"itemsTouched"
#define XmCItemsTouched		(_XmConst char *)"ItemsTouched"
#define XmNnumItems		(_XmConst char *)"numItems"
#define XmCNumItems 		(_XmConst char *)"NumItems"
#define XmNnumItemFields 	(_XmConst char *)"numItemFields"
#define XmCNumItemFields 	(_XmConst char *)"NumItemFields"
#define XmNlabelImage 		(_XmConst char *)"labelImage"
#define XmCLabelImage		(_XmConst char *)"LabelImage"
#define XmNmanaged 		(_XmConst char *)"managed"
#define XmCManaged		(_XmConst char *)"Managed"

#define XmNdragCursorProc	(_XmConst char *)"dragCursorProc"
#define XmNdragDropFinishProc	(_XmConst char *)"dragDropFinishProc"
#define XmNtargets		(_XmConst char *)"targets"
#define XmCTargets		(_XmConst char *)"Targets"
#define XmNnumTargets		(_XmConst char *)"numTargets"
#define XmCNumTargets		(_XmConst char *)"NumTargets"
#define XmCCallbackProc		(_XmConst char *)"CallbackProc"

#define XmNexclusives		(_XmConst char *)"exclusives"
#define XmCExclusives		(_XmConst char *)"Exclusives"
#define XmNlastSelectItem	(_XmConst char *)"lastSelectItem"
#define XmCLastSelectItem	(_XmConst char *)"LastSelectItem"
#define XmNnoneSet		(_XmConst char *)"noneSet"
#define XmCNoneSet		(_XmConst char *)"NoneSet"
#define XmNselectCount		(_XmConst char *)"selectCount"
#define XmCSelectCount		(_XmConst char *)"SelectCount"

#define XmNselectProc		(_XmConst char *)"selectProc"
#define XmNdblSelectProc	(_XmConst char *)"dblSelectProc"
#define XmNunselectProc		(_XmConst char *)"adjustProc"

#define XmNwantGraphicsExpose	(_XmConst char *)"wantGraphicsExpose"
#define XmCWantGraphicsExpose	(_XmConst char *)"WantGraphicsExpose"

/************************************************************************
 * Declare stuff used with DetermineMouseAction
 */
#define ExmAddTimeOut(W,I,C,D) \
	XtAppAddTimeOut(XtWidgetToApplicationContext(W),(I),(C),(D))

typedef enum {
	NOT_DETERMINED, MOUSE_CLICK, MOUSE_MOVE, MOUSE_MULTI_CLICK
} ExmButtonAction;

typedef void 	(*ExmMouseActionCallback)(
	Widget,		/* w			*/
	ExmButtonAction,/* action		*/
	Cardinal,	/* count		*/
	Time,		/* time			*/
	XtPointer	/* client_data		*/
);

extern void
ExmResetMouseAction(
	Widget		/* widget id		*/
);

extern ExmButtonAction
ExmDetermineMouseAction(
	Widget,		/* widget id		*/
	XEvent *	/* X event		*/
);

extern Cardinal
ExmFlatSetMouseDampingFactor(
	Widget,		/* w - 	   widget id	*/
	Cardinal 	/* value - new_value	*/
);

/************************************************************************
  Declare structures used with flattened widgets
*/

#define ExmDEFAULT_ITEM	((Cardinal) (~0)-1)
#define ExmNO_ITEM	( (Cardinal)( ~0 ) )

	/* Define flat widget call data.  This includes item data which is
	   passed to all call backs.
	*/
typedef struct {
	Cardinal	item_index;	/* sub-object initiating callb.	*/
	XtPointer	items;		/* sub-object list		*/
	Cardinal	num_items;	/* number of sub-objects	*/
	String *	item_fields;	/* key of fields for list	*/
	Cardinal	num_item_fields;/* number of item fields	*/
	XtPointer	user_data;	/* widget's user_data value	*/
	XtPointer	item_user_data;	/* item's user_data value	*/
} ExmFlatCallData;

	/* Define Drop data: this is supplied to drop call backs by flat
	   subclasses which support Drag and Drop.
	 */
typedef enum {
    /* an internal drop, it will be seen by the destination client in
     * XmDropSite:XmNdropProc. It gives the app a chance for optimization, if
     * she/he chose not to, then ExmFlatDropCallData:reason should set to
     * ExmEXTERNAL_DROP
     */
    ExmINTERNAL_DROP,

    /* an external drop, it will be seen by both the destination
     * (XmDropSite:XmNdropProc) and source
     * (XmDragContext:XmNdropStartCallback) clients. The destination client
     * should setup *targets* and hand them over to libXm for starting
     * conversion request(s), the source client should setup conversion data,
     * so that * XmDragContext:XmNconvertProc can process!!!
     */
    ExmEXTERNAL_DROP
} ExmFlatDropReason;

typedef struct {
    ExmFlatCallData	item_data;	/* For XmDropSite:XmNdropProc, info
					 * about `receiver' (item_index can
					 * be NO_ITEM.)  For
					 * XmNdropStartCallback, info about
					 * `source'.
					 */
    ExmFlatDropReason	reason;		/* See ExmFlatDropReason above */
    XtPointer		data;		/* This field is for
					 * XmNdropStartCallback.  This will
					 * be set to XmDragContext:
					 * XmNclientData.
					 */
    Atom		selection;	/* selection id. This field is for
					 * XmNdropStartCallback. e.g., dtm
					 * is using it for setting up dragged
					 * object data for conversion
					 */
    /* The four fields below are for XmDropSite:XmNdropProc only!! */
    Widget		source;		/* Source widget id */
    WidePosition	x;		/* Position of the drop, it's */
    WidePosition	y;		/* relative to destination widget */
    Cardinal		src_item_index;	/* dragged object's item index */
    unsigned char	operation;	/* XmDROP_MOVE/COPY/LINK */

    /* NOTE: OlDnDDragDropInfoPtr and OlDnDDropStatus are removed.
     *
     * do we need timestamp?
     */
} ExmFlatDropCallData;

	/* Define Drag Cursor data: this is supplied to drag cursor call
	 * backs by flat subclasses which support Drag and Drop.
	 *
	 * For having controll easier (due to Motif), I don't want
	 * user to specify state and operation icons. Looking at
	 * the original code, no one is doing that anyway, note that
	 * state_icon in this case is the No cursor in MoOLIT!.
	 */
typedef struct {
    ExmFlatCallData	item_data;
    Widget		source_icon;	/* dragging cursor in DnD.  Will use
					 * Motif default if not specified
					 * (i.e., NULL).
					 */
    Position		x_hot;		/* hot spot of the source icon  */
    Position		y_hot;
    Boolean		static_icon;	/* specify whether application
					 * supplied source_icon should be
					 * freed after DnD by widget. Default
					 * is True.
					 */
} ExmFlatDragCursorCallData;

typedef struct {
    XtPointer	data;		/* client_data in XmNdropProc when reason is
				 * ExmEXTERNAL_DROP
				 */
    Atom	selection;	/* the selection id for the dnd transaction */

    /* there is no item_data because there is no record of which one is
     * dragged object at this point, do we really need one?  I don't think
     * so...
     */
} ExmFlatDragDropFinishCallData;

	/* Define a structure for the complex flat help id */
typedef struct _ExmFlatHelpId {
	Widget		widget;
	Cardinal	item_index;
} ExmFlatHelpId;

/************************************************************************
  Declare external functions that applications can use.
*/

#if defined(__cplusplus) || defined(c_plusplus)
#define ExmBeginFunctionPrototypeBlock	extern "C" {
#define ExmEndFunctionPrototypeBlock	}
#else /* defined(__cplusplus) || defined(c_plusplus) */
#define ExmBeginFunctionPrototypeBlock
#define ExmEndFunctionPrototypeBlock
#endif /* defined(__cplusplus) || defined(c_plusplus) */

ExmBeginFunctionPrototypeBlock

extern Boolean
ExmFlatCallAcceptFocus(
	Widget,		/* Flat widget id	*/
	Cardinal	/* Item index		*/
);

extern void
ExmFlatChangeManagedItems(
	Widget,		/* w;			container widget id	*/
	Cardinal *,	/* managed_items;	items to manage		*/
	Cardinal,	/* num_managed;		num items to manage	*/
	Cardinal *,	/* unmanaged_items;	items to unmanage	*/
	Cardinal	/* num_unmanaged;	num items to unmanage	*/
);

extern Cardinal
ExmFlatGetFocusItem(
	Widget		/* Flat widget id	*/
);

extern void
ExmFlatGetItemGeometry(
	Widget,		/* Flat widget id	*/
	Cardinal,	/* item index		*/
	WidePosition *,	/* x return		*/
	WidePosition *,	/* y return		*/
	Dimension *,	/* width return		*/
	Dimension *	/* height return	*/
);

extern Cardinal
ExmFlatGetItemIndex(
	Widget,		/* Flat widget id	*/
	WidePosition,	/* x location		*/
	WidePosition	/* y location		*/
);

extern void
ExmFlatGetValues(
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to query		*/
	ArgList,	/* args;	- querying args		*/
	Cardinal	/* num_args;	- number of args	*/
);

		/* OlFlatRefreshItem is an interface to
		 * OlFlatRefreshExpandItem				*/
extern void
ExmFlatRefreshItem(
	Widget,		/* w;		container widget id		*/
	Cardinal,	/* item_index;	Item to draw			*/
	Boolean		/* clear_area;	should the area be cleared?	*/
);

extern void
ExmFlatSetValues(
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to modify	*/
	ArgList,	/* args;	- modifying args	*/
	Cardinal	/* num_args;	- number of args	*/
);

extern void
ExmVaFlatGetValues(
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to query		*/
	...		/* NULL terminated name/value pairs	*/
);

extern void
ExmVaFlatSetValues(
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to modify	*/
	...		/* NULL terminated name/value pairs	*/
);

extern void
ExmInitDnDIcons(
	Widget		/* widget;	- widget to get Screen pointer */
);

ExmEndFunctionPrototypeBlock

#endif /* EXM_FLAT_H */
