#ifndef	NOIDENT
#ident	"@(#)flat:FlatPublic.c	1.8"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains convenience routines for public consumption.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/FlatP.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private  Procedures 
 *		2. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static 	int	CheckId OL_ARGS((Widget, OLconst char *, Boolean, Cardinal));

					/* public procedures		*/

#if we_dont_want_collisions_with_our_macros

Boolean		OlFlatCallAcceptFocus OL_ARGS((Widget, Cardinal, Time));
Cardinal	OlFlatGetFocusItem OL_ARGS((Widget));
Cardinal	OlFlatGetItemIndex OL_ARGS((Widget, Position, Position));
void		OlFlatGetItemGeometry OL_ARGS((Widget, Cardinal,
			Position *, Position *, Dimension *, Dimension *));
#endif /* we_dont_want_collisions_with_our_macros */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define FPART(w)	(((FlatWidget)(w))->flat)
#define FCPART(w)	(((FlatWidgetClass)XtClass(w))->flat_class)

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * CheckId -
 ****************************procedure*header*****************************
 */
static int
CheckId OLARGLIST((w, proc_name, check_index, item_index))
	OLARG( Widget,		w)
	OLARG( OLconst char *,	proc_name)
	OLARG( Boolean,		check_index)
	OLGRA( Cardinal,	item_index)
{
	int	success = 0;

	if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name);
	}
	else if (_OlIsFlat(w) == False)
	{
	    OlVaDisplayWarningMsg(XtDisplayOfObject(w),
				  OleNbadFlatSubclass,
				  OleTflatState,
				  OleCOlToolkitWarning,
				  OleMbadFlatSubclass_flatState,
				  proc_name,
				  XtName(w),
				  OlWidgetToClassName(w));
	}
	else if (check_index == True && item_index > FPART(w).num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState, XtName(w),
			OlWidgetToClassName(w), proc_name, item_index);
	}
	else
	{
		success = 1;
	}
	return(success);
} /* END OF CheckId() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlFlatCallAcceptFocus - public interface to setting focus
 * to a flattened widget item.
 ****************************procedure*header*****************************
 */
Boolean
OlFlatCallAcceptFocus OLARGLIST((w, i, time))
	OLARG( Widget,		w)
	OLARG( Cardinal,	i)
	OLGRA( Time,		time)
{
	Boolean	took_it = False;

	if (CheckId(w, (OLconst char *)"OlFlatCallAcceptFocus", True, i) &&
	    OL_FLATCLASS(w).item_accept_focus)
	{
		Time		timestamp = time;

		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

		OlFlatExpandItem(w, i, item);
		took_it = (*OL_FLATCLASS(w).item_accept_focus)(
					w, item, &timestamp);

		OL_FLAT_FREE_ITEM(item);
	}
	return (took_it);
} /* END OF OlFlatCallAcceptFocus() */

/*
 *************************************************************************
 * OlFlatGetFocusItem - returns the current focus item for a flat
 * widget.  If there is no current focus item, OL_NO_ITEM is returned.
 ****************************procedure*header*****************************
 */
Cardinal
OlFlatGetFocusItem OLARGLIST((w))
	OLGRA( Widget,	w)
{
	if (!CheckId(w, (OLconst char *)"OlFlatGetFocusItem", False, 0))
	{
		return((Cardinal)OL_NO_ITEM);
	}
	return(((FlatWidget)w)->flat.focus_item);
} /* END OF OlFlatGetFocusItem() */

/*
 *************************************************************************
 * OlFlatGetItemGeometry - returns the item at the given coordinates.
 ****************************procedure*header*****************************
 */
#undef OlFlatGetItemGeometry
void
OlFlatGetItemGeometry OLARGLIST((w, i, x_ret, y_ret, w_ret, h_ret))
	OLARG( Widget,		w)
	OLARG( Cardinal,	i)		/* item_index	*/
	OLARG( Position *,	x_ret)
	OLARG( Position *,	y_ret)
	OLARG( Dimension *,	w_ret)
	OLGRA( Dimension *,	h_ret)
{
	if (!CheckId(w, (OLconst char *)"OlFlatGetItemGeometry", True, i))
	{
		*x_ret = *y_ret = (Position)0;
		*w_ret = *h_ret = (Dimension)0;
	}
	else
	{
		OlFlatDrawInfo	di;

#define OlFlatGetItemGeometry(w,i,x,y,wi,h) \
	(*OL_FLATCLASS(w).get_item_geometry)(w,i,x,y,wi,h)

		OlFlatGetItemGeometry(w, i, &di.x, &di.y, &di.width,
					&di.height);

		*x_ret = di.x;
		*y_ret = di.y;
		*w_ret = di.width;
		*h_ret = di.height;
	}
} /* END OF OlFlatGetItemGeometry() */

/*
 *************************************************************************
 * OlFlatGetItemIndex - returns the item at the given coordinates.
 ****************************procedure*header*****************************
 */
Cardinal
OlFlatGetItemIndex OLARGLIST((w, x, y))
	OLARG( Widget,		w)
	OLARG( Position,	x)
	OLGRA( Position,	y)
{
	if (!CheckId(w, (OLconst char *)"OlFlatGetItemIndex", False, 0))
	{
		return((Cardinal)OL_NO_ITEM);
	}
	return(OlFlatGetIndex(w, x, y, False));
} /* END OF OlFlatGetItemIndex() */
