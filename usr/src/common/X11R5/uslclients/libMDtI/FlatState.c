#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FlatState.c	1.2"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the routines that expand a flat container's
 *	sub-objects and routines that do Set and Get values on the 
 *	sub-objects.
 *
 ******************************file*header********************************
 */
						/* #includes go here	*/
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "FlatP.h"

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

#include <stdarg.h>
#define VA_START(a,n)	va_start(a,n)

#else

#include <varargs.h>
#define VA_START(a,n)	va_start(a)

#endif /* defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus) */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define CLEAR_IT(w,x,y,wi,ht) (void)XClearArea(XtDisplay(w),XtWindow(w),\
				(int)(x-FPART(w).x_offset),\
				(int)(y-FPART(w).y_offset),\
				(unsigned)wi,(unsigned)ht,(Bool)1)
#ifdef __STDC__
#define SIZE_T	size_t
#else
#define SIZE_T	int
#endif
				/* Declare some private Xt externs	*/

extern void	_XtCountVaList(va_list, int *, int *);
extern void	_XtVaToArgList(Widget, va_list, int, ArgList*, Cardinal*);

#define	LOOKUP			1
#define	ADD_REFERENCE		2
#define	DEC_REFERENCE		3

typedef enum {
	ANALYZE_ITEMS,
	ITEM_INITIALIZE,
	ITEM_SETVALUES,
	ITEM_GETVALUES,
	DEFAULT_ITEM_INITIALIZE,
	DEFAULT_ITEM_SETVALUES,
	INITIALIZE,
	SET_VALUES
} CallOpcode;

#define IGNORE_FIELD	  (~0)

#define	NUM_RSCS(w)	  (FCLASS(w).num_item_resources)
#define	RSCS(w)		  (FCLASS(w).item_resources)
#define	RSC_QNAME(w,i)	  (FCLASS(w).quarked_items[i])
#define	RSC_SIZE(w,i)	  (RSCS(w)[i].resource_size)
#define	RSC_OFFSET(w,i)	  (RSCS(w)[i].resource_offset)
#define REQ_RSC(w, i)	  (FCLASS(w).required_resources[i])
#define RSC_INDEX(w, i)	  (REQ_RSC(w, i).rsc_index)
#define DESTROY_PROC(w,i) (REQ_RSC(w, i).destroy)

			/* Define a structure to cache items	*/

typedef struct _ItemCache {
	struct _ItemCache *	next;		/* next node		*/
	Cardinal		ref_count;	/* reference count	*/
	Cardinal		item_index;	/* index of item	*/
	ExmFlatItem		item;		/* cached item		*/
} ItemCache;

typedef struct _WidgetCache {
	struct _WidgetCache *	next;		/* next node		*/
	Widget			w;		/* widget owning items	*/
	ItemCache *		i_root;		/* root of cache list	*/
} WidgetCache;

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public Procedures 
 *
 **************************forward*declarations***************************
 */
					/* private procedures		*/

static void	BuildExtractorList(Widget);
static ExmFlatItem CacheManager(Widget, Cardinal, ExmFlatItem, int);
static Boolean	CallItemProcs(WidgetClass, CallOpcode, Widget,
				Widget, Widget, ExmFlatItem, ExmFlatItem,
				ExmFlatItem, ArgList, Cardinal *);
static int	DoGeometry(Widget, ExmFlatItem, ExmFlatItem);
static void	ExpandItem(Widget, Cardinal, ExmFlatItem);
static void	GetItemState(Widget, Cardinal, ArgList, Cardinal *);
static Boolean	InitializeItems(Widget, Widget, Widget, ArgList, Cardinal *);
static void	SetItemState(Widget, Cardinal, ArgList, Cardinal *);

					/* public procedures		*/

void		ExmFlatStateDestroy(Widget);
void		ExmFlatStateInitialize(Widget, Widget, ArgList, Cardinal *);
Boolean		ExmFlatStateSetValues(Widget, Widget, Widget, ArgList,
								Cardinal *);
Boolean		ExmFlatItemActivate(Widget, ExmFlatItem, char, XtPointer);
void		ExmFlatExpandItem(Widget, Cardinal, ExmFlatItem);
void		ExmFlatGetValues(Widget, Cardinal, ArgList, Cardinal);
void		ExmFlatSetValues(Widget, Cardinal, ArgList, Cardinal);
void		ExmVaFlatGetValues(Widget, Cardinal, ...);
void		ExmVaFlatSetValues(Widget, Cardinal, ...);
void		ExmFlatChangeManagedItems(Widget, Cardinal *, Cardinal,
							Cardinal *, Cardinal);
void		ExmFlatSyncItem(Widget, ExmFlatItem);
XtGeometryResult ExmFlatMakeGeometryRequest(Widget, 
			ExmFlatItem, ExmFlatItemGeometry*,ExmFlatItemGeometry*);

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * BuildExtractorList - the procedure builds an array of information that
 * will be used to expand the sub-object item.
 ****************************procedure*header*****************************
 */
static void
BuildExtractorList(Widget w)	/* Flat widget id		*/
{
	Cardinal		count;	/* number of fields for this part*/
	register Cardinal	i;	/* loop counter			*/
	register Cardinal	n;	/* extractor field index	*/
	register String *	item_fields = FPART(w).item_fields;
	register XrmQuark	quark;	/* temporary quark holder	*/
	XrmQuarkList		qlist;
	Cardinal *		xlist;	/* extractor list		*/

	if (FPART(w).num_item_fields == (Cardinal)0 ||
	    FPART(w).num_items == (Cardinal)0 ||
	    FPART(w).items == (XtPointer)NULL)
	{
		return;
	}

			/* Allocate a large enough array to hold all
			 * application-specified fields			*/

	xlist = (Cardinal *) XtMalloc((Cardinal)
			(FPART(w).num_item_fields * sizeof(Cardinal)));

					/* Build the Extractor list	*/

	for (n=0, count=0; n < FPART(w).num_item_fields; ++n, ++item_fields)
	{
		quark = XrmStringToQuark(*item_fields);
		qlist = FCLASS(w).quarked_items;

		for (i=0; i < NUM_RSCS(w); ++i, ++qlist)
		{
		    if (quark == *qlist)
		    {
			xlist[count] = i;
			++count;
			break;
		    }
		}

		if (i == NUM_RSCS(w))
		{
			char *	resource = (item_fields != (String *)NULL &&
					    *item_fields != (String)NULL ?
					    *item_fields : "");

			OlVaDisplayWarningMsg((XtDisplay(w),
				OleNinvalidResource, OleTbadItemResource,
				OleCOlToolkitWarning,
				OleMinvalidResource_badItemResource,
				XrmQuarkToString(w->core.xrm_name),
				XtClass(w)->core_class.class_name,
				resource));

				/* Add a dummy resource to skip the
				 * bad field.				*/

			xlist[count] = (Cardinal) IGNORE_FIELD;
			++count;
		}
	}

		/* If this list contains less elements than we allocated,
		 * reallocate it					*/

	if (count < n)
	{
		if (count == (Cardinal)0)
		{
			XtFree((XtPointer)xlist);
			xlist = (Cardinal *)NULL;
		}
		else
		{
			xlist = (Cardinal *) XtRealloc((char *)xlist,
					(count * sizeof(Cardinal)));
		}
	}

	FPART(w).resource_info.xlist		= xlist;
	FPART(w).resource_info.num_xlist	= count;
} /* end of BuildExtractorList() */

/*
 *************************************************************************
 * CacheManager - this routine manages an item cache.  What the routine
 * actually does is dependent on the supplied action flag.  The routine
 * returns the address of the expanded item or NULL.
 ****************************procedure*header*****************************
 */
static ExmFlatItem
CacheManager(	Widget		w,		/* flat widget id	*/
		Cardinal	item_index,	/* requested item	*/
		ExmFlatItem	item,		/* item or NULL		*/
		int		action)		/* What to do		*/
{
	static WidgetCache *	w_root = (WidgetCache *)NULL;

	WidgetCache *		w_null = (WidgetCache *)NULL;
	WidgetCache *		w_self = w_root;
	WidgetCache *		w_prev = w_null;
	ItemCache *		i_null = (ItemCache *)NULL;
	ItemCache *		i_self = i_null;
	ItemCache *		i_prev = i_null;

			/* Search for the widget id.  Once that is
			 * found, search for the item index.		*/

	while (w_self != w_null)
	{
		if (w_self->w == w)
		{
			i_self = w_self->i_root;

			while(i_self != i_null &&
			      i_self->item_index != item_index)
			{
				i_prev	= i_self;
				i_self	= i_self->next;
			}
			break;
		}
		else
		{
			w_prev	= w_self;
			w_self	= w_self->next;
		}
	}

				/* now that the widget and item cache
				 * pointers are initialized, so some
				 * action.				*/

	switch(action)
	{
	case ADD_REFERENCE:
		if (i_self == i_null)
		{
			i_self = XtNew(ItemCache);
			i_self->next		= i_prev;
			i_self->item_index	= item_index;
			i_self->ref_count	= (Cardinal)0;
			i_self->item		= item;

			ExpandItem(w, item_index, i_self->item);
		}

				/* Increment the reference count and
				 * set the return item address.		*/

		++i_self->ref_count;
		item = i_self->item;

		if (w_self == w_null)
		{
			w_self = XtNew(WidgetCache);
			w_self->next	= w_root;
			w_root		= w_self;

			w_self->w	= w;
			w_self->i_root	= i_self;
		}
		else if (i_self->ref_count == 1)
		{
			i_self->next	= w_self->i_root;
			w_self->i_root	= i_self;
		}
		break;
	case DEC_REFERENCE:
			/* Update the application's list and the required
			 * resource buffer (if applicable) with the
			 * information stored in the new item.		*/

		ExmFlatSyncItem(w, i_self->item);

		if (--i_self->ref_count == 0)
		{
			if (i_prev == i_null)
			{
				w_self->i_root = i_self->next;
			}
			else
			{
				i_prev->next = i_self->next;
			}

			XtFree((XtPointer)i_self);

			if (w_self->i_root == i_null)
			{
				if (w_prev == w_null)
				{
					w_root = w_self->next;
				}
				else
				{
					w_prev->next = w_self->next;
				}

				XtFree((XtPointer)w_self);
			}
		}
		break;
	default:					/* LOOKUP	*/
		if (i_self == i_null)
		{
			ExpandItem(w, item_index, item);
		}
		else
		{
			(void)memcpy((XtPointer)item,
				(_XmConst XtPointer)i_self->item,
				(SIZE_T)FCLASS(w).rec_size);
		}
		break;
	}
	return(item);
} /* end of CacheManager() */

/*
 *************************************************************************
 * CallItemProcs - this routine recursively calls the initialize item
 * procedure for each flat subclass.
 ****************************procedure*header*****************************
 */
static Boolean
CallItemProcs(	WidgetClass	wc,	/* Current WidgetClass		*/
		CallOpcode	opcode,	/* procedure to call		*/
		Widget		c_w,	/* Current Flat Widget		*/
		Widget		r_w,	/* Request Flat Widget		*/
		Widget		w,	/* New Flat Widget		*/
		ExmFlatItem	current,/* expanded current item	*/
		ExmFlatItem	request,/* expanded requested item	*/
		ExmFlatItem	new,	/* expanded new item		*/
		ArgList		args,
		Cardinal *	num_args)
{
#define FC(WC)	( ((ExmFlatWidgetClass)wc)->flat_class )

    Boolean			return_val = False;

    if (wc != exmFlatWidgetClass)
	if ( CallItemProcs(wc->core_class.superclass, opcode, c_w, r_w,
			   w, current, request, new, args, num_args) )
	{
	    return_val = True;
	}

    /* Call subclass's routine	*/

    switch(opcode)
    {
    case INITIALIZE:
	if (FC(wc).initialize)
	    (*FC(wc).initialize)(r_w, w, args, num_args);
	break;

    case SET_VALUES:
	if (FC(wc).set_values)
	    if ( (*FC(wc).set_values)(c_w, r_w, w,args, num_args) )
		return_val = True;
	break;

    case DEFAULT_ITEM_INITIALIZE:
	if (FC(wc).default_initialize)
	    (*FC(wc).default_initialize)(w, request, new, args, num_args);
	break;

    case DEFAULT_ITEM_SETVALUES:
	if (FC(wc).default_set_values)
	    if ( (*FC(wc).default_set_values)(w, current, request,
					   new, args, num_args) )
	    {
		return_val = True;
	    }
	break;

    case ITEM_GETVALUES:
	if (FC(wc).item_get_values)
	    (*FC(wc).item_get_values)(w, current, args, num_args);
	break;

    case ITEM_INITIALIZE:
	if (FC(wc).item_initialize)
	    (*FC(wc).item_initialize)(w, request, new, args, num_args);
	break;

    case ITEM_SETVALUES:
	if (FC(wc).item_set_values)
	    if ( (*FC(wc).item_set_values)(w, current, request,
					 new, args, num_args) )
	    {
		return_val = True;
	    }
	break;

    default:				/* ANALYZE_ITEMS	*/
	if (FC(wc).analyze_items)
	    (*FC(wc).analyze_items)(w, args, num_args);
	break;
    }
    return(return_val);

#undef FC
}					/* end of CallItemProcs() */

/*
 *************************************************************************
 * ExpandItem - the procedure expands the subclasses contribution
 * to the item into the destination address.  The expansion process is
 * is a forward-chained process.  If the subclass has a part to expand,
 * it is expanded into the destination at the correct offset.
 *
 * Since a particular class does not know if a subclass has an additional
 * part contribution, the expansion must stop an the class specified
 * by the 'container' argument.  (If we don't stop here, we be mashing
 * memory since we will exceed the calling class's structure.)
 *
 * The procedure copies values (using their correct size) from the
 * application list into the supplied structure using the offsets defined
 * for each field.
 * This procedure expands only one sub-object element and assumes the
 * the src has fields of equal size.
 ****************************procedure*header*****************************
 */
static void
ExpandItem(	Widget		w,
		Cardinal	item_index,
		ExmFlatItem 	item)	/* uninitialized expanded item	*/
{
	char *			dest = (char *)item;
	_XmConst char *		src;	/* sub_object base address	*/
	register Cardinal *	list;	/* extractor list		*/
	Cardinal		num;

				/* Load the default item and stick the
				 * item index in the expanded item.	*/

	(void)memcpy((XtPointer)item, (_XmConst XtPointer)ExmFlatDefaultItem(w),
		(SIZE_T)FCLASS(w).rec_size);

	item->flat.item_index = item_index;

	if ((num = FPART(w).resource_info.num_xlist) != (Cardinal)0)
	{
		src = (char *)((char *)FPART(w).items +
				(item_index * sizeof(XtArgVal) *
					FPART(w).resource_info.num_xlist));

		for (list = FPART(w).resource_info.xlist;
		     num != (Cardinal)0; --num, ++list)
		{
			if (*list != (Cardinal)IGNORE_FIELD)
			{
				ExmCopyFromXtArgVal(*((XtArgVal *)src), 
					(char *)(dest + RSC_OFFSET(w, *list)),
					RSC_SIZE(w, *list));
			}
			src += sizeof(XtArgVal);
		}
	}

} /* end of ExpandItem() */

/*
 *************************************************************************
 * GetItemState - this routine queries the fields of an item
 * using the Args that are supplied.
 *
 * The source address of the item to be put back is determined by
 * taking the head of the item list as the base address.  Then the number
 * of items skipped over must be added (base + record_size * index).  Then,
 * we have to add the offset of the particular field, (num * part_size).
 ****************************procedure*header*****************************
 */
static void
GetItemState(	Widget		w,		/* Flat widget subclass	*/
		Cardinal	item_index,	/* item to be expanded	*/
		ArgList		args,		/* list of Args to get	*/
		Cardinal *	num_args)	/* number of Args	*/
{
	XrmQuarkList	qlist;		/* all subclass item resources	*/
	XrmQuark	quark;
	Cardinal	num;
	Cardinal	count;
	ExmFlatItem	new;
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, new_buf);

		/* Expand the item with the CacheManager.  We saved the
		 * address of 'new' since the call to CacheManager may
		 * return an address other than 'new'.			*/

	new = CacheManager(w, item_index, new_buf, ADD_REFERENCE);

			/* Call the subclasses to add information
			 * or possibly change some of it.		*/

	(void)CallItemProcs(XtClass(w), ITEM_GETVALUES, (Widget)NULL,
			(Widget)NULL, w, new, (ExmFlatItem)NULL,
			(ExmFlatItem)NULL, args, num_args);

						/* Loop over the args.	*/

	for (count = (Cardinal)0; count < *num_args; ++count, ++args)
	{
		qlist = FCLASS(w).quarked_items;
		quark = XrmStringToQuark(args->name);

		for (num=0; num < NUM_RSCS(w); ++qlist, ++num)
		{
			if (quark == *qlist)
			{
				/* Copy the value out of the expanded
				 * item and place it in the application's
				 * arg list.				*/

				ExmCopyToXtArgVal((char *) ((char *)new +
						RSC_OFFSET(w, num)),
						(XtArgVal *)&args->value,
						RSC_SIZE(w, num));
				break;
			}
		} /* looping over item resources for this subclass */
	} /* looping over num_args */

				/* Decrement the cached item's reference
				 * count.				*/

	(void)CacheManager(w, item_index, new, DEC_REFERENCE);

	ExmFLAT_FREE_ITEM(new_buf);

} /* end of GetItemState() */

/*
 *************************************************************************
 * ExmFlatStateDestroy - destroys state information associated with the
 * item's.  Once the information is destroy, the fields are re-initialized
 * since this routine is called when either the widget is being destroyed,
 * or when the widget's item list is removed.
 ****************************procedure*header*****************************
 */
void
ExmFlatStateDestroy(Widget w)	/* The Flat Widget		*/
{
	XtFree((XtPointer) FPART(w).resource_info.xlist);

	FPART(w).resource_info.xlist		= (Cardinal *)NULL;
	FPART(w).resource_info.num_xlist	= (Cardinal)0;

} /* end of ExmFlatStateDestroy() */

/*
 *************************************************************************
 * ExmFlatSyncItem - this routine copies values from an item and places
 * them into the application's list and for any required_resources, puts
 * the values into the required resource list.
 ****************************procedure*header*****************************
 */
void
ExmFlatSyncItem(Widget		w,
		ExmFlatItem	item)
{
	char *			src = (char *)item;
	char *			dest;
	Cardinal *		list;
	Cardinal		num;

	if ((num = FPART(w).resource_info.num_xlist) != (Cardinal)0)
	{
		list = FPART(w).resource_info.xlist;
		dest = (char *)((char *)FPART(w).items +
					(item->flat.item_index *
						sizeof(XtArgVal) * num));

		for ( ; num; --num, ++list)
		{
			if (*list != (Cardinal)IGNORE_FIELD)
			{
				ExmConvertToXtArgVal((src +
					RSC_OFFSET(w, *list)),
					(XtArgVal *)dest, RSC_SIZE(w, *list));
			}
			dest += sizeof(XtArgVal);
		}
	}

} /* end of ExmFlatSyncItem() */

/*
 *************************************************************************
 * ExmFlatMakeGeometryRequest - used so that an item can request its
 * managing widget to make a change in the item's size or position.
 ****************************procedure*header*****************************
 */
XtGeometryResult
ExmFlatMakeGeometryRequest(	Widget			w,
				ExmFlatItem		item,
				ExmFlatItemGeometry *	request,
				ExmFlatItemGeometry *	reply)
{
	WidePosition		x;
	WidePosition		y;
	Dimension		height;
	Dimension		width;
	XtGeometryResult	result;
	ExmFlatItemGeometry	tmp;

			/* If the items are being touched or the item is
			 * unmanaged, accept all changes		*/

	if (FPART(w).items_touched == True ||
	    item->flat.managed == False)
	{
		return(XtGeometryYes);
	}

	x	= item->flat.x;
	y	= item->flat.y;
	width	= item->flat.width;
	height	= item->flat.height;

	result = (*FCLASS(w).geometry_handler)(w, item, request,
						(reply ? reply : & tmp));

#define SET_IT(b,f)					\
	if (request->request_mode & (XtGeometryMask)b)	\
		item->flat.f = request->f

	if (!(request->request_mode & XtCWQueryOnly))
	{
		if (result == XtGeometryYes)
		{
			SET_IT(CWX, x);
			SET_IT(CWY, y);
			SET_IT(CWWidth, width);
			SET_IT(CWHeight, height);

				/* Now clear the old and new areas	*/

			if (XtIsRealized(w))
			{
				CLEAR_IT(w, x, y, width, height);
				CLEAR_IT(w, item->flat.x, item->flat.y,
					item->flat.width, item->flat.height);
			}

				/* Guarantee that the item is synched with
				 * its data.				*/

			ExmFlatSyncItem(w, item);
		}
		else if (result == XtGeometryDone)
		{
			result = XtGeometryYes;
		}
	}
	return(result);
} /* end of ExmFlatMakeGeometryRequest() */

/*
 *************************************************************************
 * DoGeometry - handles geometry negoiations that emanated from setting
 * the state on an item.  This routine returns True if the item has
 * already been refreshed due to resolving the geometry resolution.
 ****************************procedure*header*****************************
 */
static int
DoGeometry(	Widget		w,
		ExmFlatItem	current,
		ExmFlatItem	new)
{
	XtGeometryResult	result = XtGeometryNo;
	ExmFlatItemGeometry	request;

	request.request_mode = (XtGeometryMask)0;

#define CHK_N_SET(b,f)						\
	if (new->flat.f != current->flat.f)			\
	{							\
		request.request_mode |= (XtGeometryMask)b;	\
		request.f		= new->flat.f;		\
		new->flat.f		= current->flat.f;	\
	}

	CHK_N_SET(CWX, x);
	CHK_N_SET(CWY, y);
	CHK_N_SET(CWWidth, width);
	CHK_N_SET(CWHeight, height);

				/* If we need a resize, make a geometry
				 * request and continue looping until
				 * the request is resolved.		*/

	if (request.request_mode)
	{
		ExmFlatItemGeometry	reply;
		ExmFlatItemSetValuesAlmostProc	almost_proc =
					FCLASS(w).item_set_values_almost;

		do
		{
			result = ExmFlatMakeGeometryRequest(w, new,
						&request, &reply);

			if (result == XtGeometryYes || result == XtGeometryDone)
			{
				break;
			}

				/* We're here
				 * because of an XtGeometryAlmost or an
				 * XtGeometryNo return value.		*/

				/*
				 * According to the Intrinsics documentation,
				 * the request_mode in the reply structure
				 * must be set to 0 if the result is
				 * XtGeometryNo.  Let's do the same.	*/

			if (result == XtGeometryNo)
			{
				reply.request_mode = 0;
			}

			if (almost_proc != (ExmFlatItemSetValuesAlmostProc)NULL)
			{
				(*almost_proc)(w, current, new,
						&request, &reply);
			}
			else
			{
				/* Just accept the compromise	*/

				request = reply;
			}
		} while (request.request_mode != (XtGeometryResult)0);

	} /* end of request.request_mode != 0 */

	return(result == XtGeometryYes);
} /* end of DoGeometry() */

/*
 *************************************************************************
 * SetItemState - this routine updates the fields of an item
 * using the Args that are supplied.
 *
 * This routine also makes a geometry request if the changing of the
 * item's attributes causes a change in the layout and/or size of the
 * widget.  Note: the geometry request is not made if this routine is
 * called while the flat widget is checking items.
 *
 * The destination address of the item to be put back is determined by
 * taking the head of the item list as the base address.  Then the number
 * of items skipped over must be added (base + record_size * index).  Then,
 * we have to add the offset of the particular field, (num * part_size).
 ****************************procedure*header*****************************
 */
static void
SetItemState(	Widget		w,		/* Flat widget subclass	*/
		Cardinal	item_index,	/* item to be expanded	*/
		ArgList		args,		/* list of Args to touch*/
		Cardinal *	num_args)	/* number of Args	*/
{
	Boolean			redisplay;
	Cardinal		num;
	Cardinal		count;
	Cardinal *		list;
	ExmFlatItem		new;
	ArgList			cached_args = args;
	XrmQuark		quark;
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, current);
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, request);
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, new_buf);

		/* Expand the new item before the changes are applied
		 * and copy them into current.  We saved the
		 * address of 'new' since the call to CacheManager may
		 * return an address other than 'new'.			*/

	new = CacheManager(w, item_index, new_buf, ADD_REFERENCE);

		/* Save the geometry of the item before applying any
		 * changes.  Only do this if we're not dealing with a
		 * new list.  In some cases this call isn't necessary
		 * since the geometry fields may be required resources,
		 * but since not all subclasses do this, we have to
		 * explicitly set them.				*/

	if (FPART(w).items_touched == False)
	{
		ExmFlatGetItemGeometry(w, item_index, &new->flat.x,
			&new->flat.y, &new->flat.width, &new->flat.height);
	}

	(void)memcpy((XtPointer)current, (_XmConst XtPointer)new,
				(SIZE_T)FCLASS(w).rec_size);

			/* Loop over the args and copy the requested
			 * values into the new item.			*/

	for (count = (Cardinal)0; count < *num_args; ++count, ++args)
	{
		for (list = FPART(w).resource_info.xlist,
		     quark = XrmStringToQuark(args->name),
		     num = FPART(w).resource_info.num_xlist;
		     num != (Cardinal)0; --num, ++list)
		{
			if (*list != (Cardinal)IGNORE_FIELD &&
			    quark == RSC_QNAME(w, *list))
			{
				/* Copy the value out of the Arg list
				 * place it into the expanded item.	*/

				ExmCopyFromXtArgVal(args->value,
					(char *)((char *)new +
						RSC_OFFSET(w, *list)),
					RSC_SIZE(w, *list));
				break;
			}
		} /* looping over item resources for this subclass */

	} /* looping over num_args */

				/* Make the request item be a copy of the
				 * new item before the widget gets to
				 * see the changes.			*/

	(void)memcpy((XtPointer)request, (_XmConst XtPointer)new,
			(SIZE_T)FCLASS(w).rec_size);

			/* Call subclass's item_set_values routine	*/

	redisplay = CallItemProcs(XtClass(w), ITEM_SETVALUES, (Widget)NULL,
			(Widget)NULL, w, current, request, new,
			cached_args, num_args);

			/* If the managed state of the item changed,
			 * call the class's changed managed routine.
			 * Else, check to see if this item wants to
			 * change it's geometry.
			 * Note: we only do either of these checks
			 * if the items are not being touched since
			 * when items are being touched, since the
			 * entire container is refreshed when the items
			 * are touched.					*/

	if (FPART(w).items_touched == False)
	{
		if (new->flat.managed != current->flat.managed)
		{
				/* always turn off this flag, because	*/
				/* ExmFlatChangeManagedItems() will do	*/
				/* the redisplay for us			*/
			redisplay = False;

			if (new->flat.managed == True)
			{
				ExmFlatChangeManagedItems(w,
					&new->flat.item_index, (Cardinal)1,
					(Cardinal *)NULL, (Cardinal)0);
			}
			else
			{
				ExmFlatChangeManagedItems(w,
					(Cardinal *)NULL, (Cardinal)0,
					&new->flat.item_index, (Cardinal)1);
			}
		}
		else if (DoGeometry(w, current, new))
		{
			redisplay = False;
		}
			
#define SAME(f)	(new->flat.f == current->flat.f)

		if (redisplay == True)
		{
			ExmFlatRefreshExpandedItem(w, new, True);
		}
	}

			/* Decrement the cached item's reference count,
			 * and free any local storage.			*/

	(void)CacheManager(w, item_index, new, DEC_REFERENCE);

	ExmFLAT_FREE_ITEM(current);
	ExmFLAT_FREE_ITEM(request);
	ExmFLAT_FREE_ITEM(new_buf);

} /* end of SetItemState() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * ExmFlatExpandItem - this procedure initializes an application supplied
 * item.  The returned item is considered read-only since any changes
 * made to it will not be saved.  When initializing the item, the routine
 * checks if the requested item is currently cached.  If it is, it
 * returns a copy of the cached item.  If it is not, it does the expansion.
 ****************************procedure*header*****************************
 */
void
ExmFlatExpandItem(	Widget		w,
			Cardinal	item_index,/* item to expand	*/
			ExmFlatItem	item)	/* destination address	*/
{
	if (item == (ExmFlatItem)NULL)
	{
		OlVaDisplayWarningMsg((XtDisplay(w), OleNbadItemAddress,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemAddress_flatState,
			XtName(w), OlWidgetToClassName(w), "OlFlatExpandItem",
			(unsigned) item_index));
	}
	else
	{
		item = CacheManager(w, item_index, item, LOOKUP);
	}
} /* end of ExmFlatExpandItem() */

/*
 *************************************************************************
 * InitializeItems - handles calling routines that update and monitor
 * the default item and the individual items.   This routine is called
 * by ExmFlatStateSetValues()  (which is called whenever the flat widget 
 * container is initialized or when the application does a SetValues on
 * the widget, i.e., not when the application does a set values on
 * an individual item).
 *
 * This routine has two primary purposes:
 *	1. Monitor changes to the default Item
 *	2. Re-examine the items list if they were touched.
 *
 * The internal state flag "items_touched" is set prior to calling this
 * routine by either the flatWidgetClass base class's initialize or
 * set_values routine or by the application.   If the value of this flag
 * is FALSE, this routine returns after updating the default items.
 * If the flag is TRUE, the routine will call the procedures necessary
 * to examine the new items and/or item fields.
 *
 * The routine returns TRUE if a redisplay is recommended.  This is only
 * true when the call to the default item's set_values routine returns
 * TRUE.
 ****************************procedure*header*****************************
 */
static Boolean
InitializeItems(Widget		c_w,		/* Current flat widget	*/
		Widget		r_w,		/* Request flat widget	*/
		Widget		w,		/* New flat widget	*/
		ArgList		args,
		Cardinal *	num_args)
{
	Boolean			redisplay = False;
	Cardinal		i;		/* current item index	*/
	ExmFlatItem		new;
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, request);
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, new_buf);

	if (FPART(w).num_item_fields && FPART(w).item_fields == (String *)NULL)
	{
	    OlVaDisplayWarningMsg((XtDisplay(w),
				  OleNinvalidResource, OleTnullList,
				  OleCOlToolkitWarning,
				  OleMinvalidResource_nullList,
				  XtName(w), OlWidgetToClassName(w),
				  "XtNitemFields", "XtNnumItemFields"));

	    /* set the count to zero	*/
	    FPART(w).num_item_fields = 0;
	}

			/* copy the new default item into the request
			 * item so the widget's can check changes that
			 * have been refused.				*/

	(void)memcpy((XtPointer)request,
			(_XmConst XtPointer)ExmFlatDefaultItem(w),
			(SIZE_T)FCLASS(w).rec_size);

	if (c_w == NULL)
	{
		(void)CallItemProcs(XtClass(w), DEFAULT_ITEM_INITIALIZE,
			c_w, r_w, w, (ExmFlatItem)NULL, request, 
			ExmFlatDefaultItem(w), args, num_args);
	}
	else
	{
		if (CallItemProcs(XtClass(w), DEFAULT_ITEM_SETVALUES,
			c_w, r_w, w, ExmFlatDefaultItem(c_w), request, 
			ExmFlatDefaultItem(w), args, num_args) == TRUE)
		{
			redisplay = TRUE;
		}
	}

			/* No items were touched, then return now.	*/

	if (FPART(w).items_touched == FALSE)
	{
		ExmFLAT_FREE_ITEM(request);
		ExmFLAT_FREE_ITEM(new_buf);

		return(redisplay);
	}

			/* Since the items were touched, destroy state
			 * information associated with the old items.
			 * Then, begin building new state information.
			 * Note, we don't have to destroy any information
			 * at initialization time (when c_w == NULL).	*/

	if (c_w != (Widget)NULL)
	{
		ExmFlatStateDestroy(c_w);

			/* The following fields have to be zero'd out	*/
			/* otherwise it will have problems when passing	*/
			/* in a NULL item list. This is because		*/
			/* BuildExtractorList() will simply return in	*/
			/* this case. As a result, the old pointers are	*/
			/* still around and will get into trouble when	*/
			/* the item list is touched again...		*/
		FPART(w).resource_info.xlist		= (Cardinal *)NULL;
		FPART(w).resource_info.num_xlist	= (Cardinal)0;
	}

				/* build the extractor list and the
				 * required resource list		*/

	BuildExtractorList(w);

	for (i=0; i < FPART(w).num_items; ++i)
	{
		/* Expand the item as it is before the update. We saved
		 * the address of 'new' since the call to CacheManager
		 * may return an address other than 'new'.		*/

		new = CacheManager(w, i, new_buf, ADD_REFERENCE);

				/* Copy the new item into the request
				 * item so the flat widget can compare
				 * differences.				*/

		(void)memcpy((XtPointer)request, (_XmConst XtPointer)new,
				(SIZE_T)FCLASS(w).rec_size);

			/* Call subclass's item_initialize routine	*/

		(void)CallItemProcs(XtClass(w), ITEM_INITIALIZE, c_w, r_w, w,
			(ExmFlatItem)NULL, request, new, args, num_args);

				/* Decrement the cached item's reference
				 * count.				*/

		(void)CacheManager(w, i, new, DEC_REFERENCE);
	}

	ExmFLAT_FREE_ITEM(request);
	ExmFLAT_FREE_ITEM(new_buf);

			/* Call subclass's analyze_items routine so
			 * that the flat widget can look at all items
			 * collectively.				*/

	(void)CallItemProcs(XtClass(w), ANALYZE_ITEMS, c_w, r_w, w,
			(ExmFlatItem)NULL, (ExmFlatItem)NULL, (ExmFlatItem)NULL,
			args, num_args);

	return(TRUE);
} /* end of InitializeItems() */

/*
 *************************************************************************
 * ExmFlatStateInitialize - responsible for calling the appropriate
 * procedures to maintain the items and the container's knowledge of
 * changes to the items caused by application requests at the container's
 * initialization time.
 *
 * Only the superclass of all flat widgets (i.e., flatClass) should call
 * this routine.  It's called from that class's CORE Initialize.  This
 * routine calls ExmFlatStateSetValues() to do all of the work.
 ****************************procedure*header*****************************
 */
void
ExmFlatStateInitialize(	Widget		request,/* reqeust flat widget	*/
			Widget		new,	/* new flat widget	*/
			ArgList		args,
			Cardinal *	num_args)
{
	FPART(new).resource_info.xlist		= (Cardinal *) NULL;
	FPART(new).resource_info.num_xlist	= (Cardinal) 0;
	FPART(new).relayout_hint		= (Boolean)True;
	FPART(new).items_touched		=
			(FPART(new).num_items != (Cardinal)0 ? True : False);

	ExmFlatDefaultItem(new)->flat.item_index = ExmDEFAULT_ITEM;

	(void) ExmFlatStateSetValues((Widget)NULL, request, new,
						args, num_args);
} /* END OF ExmFlatStateInitialize () */


#define ALLOCATE_MAYBE(num,auto) \
	( ((num) <= sizeof((auto))) ?	(XtPointer)(auto) : \
					(XtPointer)(XtMalloc(num)) )
#define FREE_MAYBE(actual,auto) \
	if ((actual) != (auto)) XtFree((XtPointer)(actual)); else


/*
 *************************************************************************
 * ExmFlatStateSetValues - responsible for calling the appropriate
 * procedures to maintain the items and the container's knowledge of
 * changes to the items caused by application requests.
 *
 * Only the superclass of all flat widgets (i.e., exmFlatClass) should call
 * this routine.  It's called from that class's CORE Initialize (indirectly
 * through ExmFlatStateInitialize) and SetValues routines.
 *
 * A layout is always requested  under the following conditions:
 *	1. initialization time, 
 *	2. the 'items_touched' flag is TRUE, or
 *	3. the relayout hint is TRUE.
 *
 * This routine return TRUE if a redisplay of the widget is recommended.
 ****************************procedure*header*****************************
 */
Boolean
ExmFlatStateSetValues(	Widget		current,/* current flat widget	*/
			Widget		request,/* reqeust flat widget	*/
			Widget		new,	/* new flat widget	*/
			ArgList		args,
			Cardinal *	num_args)
{
	Boolean redisplay = False;
        union { double force_align; char localBuf[1024];} u;

			/* If 'current' widget is NULL, we got here because
			 * of a widget's initialization; else, because 
			 * of a set values on the widget.		*/

	if (!current)
	{
		/* Initialize the default item here so that it's ready to
		 * be examined by the derived classes.
		 * Since the Intrinsics compile the resource lists, we'll
		 * make a copy of the uncompiled resource list here rather
		 * than keep two copies around (i.e., a compiled and
		 * uncompiled list) since widgets are initialized only once
		 * in their lifetime.  So, we don't have to worry about
		 * having a list around that we'll never use once the
		 * widgets are done being created.
		 * Also, we must use all of the items' resources since
		 * we're querying the database when initializing the
		 * default item.					*/

#if 0 /* Performance changes */
		XtResourceList rscs = (XtResourceList) XtMalloc(
					sizeof(XtResource) *
					FCLASS(new).num_item_resources);
#endif

		XtResourceList	rscs = (XtResourceList) ALLOCATE_MAYBE(
		  (sizeof(XtResource) * FCLASS(new).num_item_resources),
		  u.localBuf);

		(void) memcpy((XtPointer)rscs, (_XmConst XtPointer)
				FCLASS(new).item_resources,
				(SIZE_T)(sizeof(XtResource) * 
					FCLASS(new).num_item_resources));

		XtGetSubresources(new, (XtPointer)ExmFlatDefaultItem(new),
				(String)NULL, (String)NULL,
				rscs, FCLASS(new).num_item_resources,
				args, *num_args);

#if 0 /* Performance changes */
		XtFree((XtPointer) rscs);
#endif
		FREE_MAYBE((XtPointer)rscs, u.localBuf);

		(void)CallItemProcs(XtClass(new), INITIALIZE, current,
			request, new, (ExmFlatItem)NULL, (ExmFlatItem)NULL,
			(ExmFlatItem)NULL, args, num_args);
	}
	else
	{
		/* Perform the SetValues on the default item here so it's
		 * ready to be examined by the derived classes.  Since
		 * the Intrinsics compile resource lists, we'll make a
		 * local copy of the needed resources rather than
		 * keep two copies of the resource lists since:
		 * 	a) SetValues is not in the critical event
		 *	   processing path i.e., a SetValues on the widget
		 *	   typically is done infrequently, and
		 *	b) most setValues involve only small number of args
		 *
		 * We'll optimize the code by providing a few args on
		 * the stack.
		 */

		Cardinal	i;
		Cardinal	j;
		Cardinal	matches;
		XrmQuark	quark;
		XtResource	auto_rscs[6];
		XtResourceList	rscs;

			/* Since this routine manages the state of the
			 * flat items, reset the relayout hint flag and
			 * reset the items_touched flag.		*/

#undef DIFFERENT
#define DIFFERENT(field)	(FPART(new).field != FPART(current).field)

		FPART(new).relayout_hint = False;

		if ( (DIFFERENT(items_touched) &&
			FPART(new).items_touched == False)	||
		      DIFFERENT(item_fields)			||
		      DIFFERENT(num_item_fields)			||
		      DIFFERENT(items)				||
		      DIFFERENT(num_items) )
		{
			FPART(new).items_touched = True;
		}
#undef DIFFERENT

		rscs = (*num_args <= XtNumber(auto_rscs) ? auto_rscs :
				(XtResourceList) XtMalloc(*num_args *
						sizeof(XtResource)));

		for (i = matches = 0; i < *num_args; ++i)
		{
			quark = XrmStringToQuark(args[i].name);
		
			for(j=0; j < FCLASS(new).num_item_resources; ++j)
			{
				if (quark == FCLASS(new).quarked_items[j])
				{
					rscs[matches] = FCLASS(new).
							item_resources[j];
					++matches;
					break;
				}
			}
		}

			/*
			 * This is a special note: we can't use
			 * XtSetSubvalues() because this proc is
			 * expecting a compiled resource list
			 * (which we tried to avoid in this new
			 * scheme). So we have to use
			 * XtGetSubresources() because this proc
			 * will compile the resource list and then
			 * override the values from the supplied
			 * argument list. This proc will also
			 * be using the default value in the resource
			 * list if the value is not in the supplied
			 * argument list (see reference manual), so
			 * we have to make sure the stuff
			 * in the resource list need to appear in the
			 * argument list (see above about how to
			 * collect these data).
			 */
		if (matches)
		{
			XtGetSubresources(
				new, (XtPointer)ExmFlatDefaultItem(new),
				(String)NULL, (String)NULL,
				rscs, matches,
				args, *num_args);
		}

		if (rscs != auto_rscs)
		{
			XtFree((XtPointer)rscs);
		}

		redisplay = CallItemProcs(XtClass(new), SET_VALUES, current,
			request, new, (ExmFlatItem)NULL, (ExmFlatItem)NULL,
			(ExmFlatItem)NULL, args, num_args);
	}

		/* Now that the changes have been applied to the default
		 * item and that the container's initialize or set_values
		 * routines have been called, see if any work needs to
		 * be done on the items.				*/

	if (InitializeItems(current, request, new, args, num_args) == True)
	{
		redisplay = True;
	}

	if (FPART(new).items_touched ||
	    FPART(new).relayout_hint)
	{

			/* Call the change managed routine to do the
			 * re-layout.  Specifying a count of zero implies
			 * that this request is not a result of a set
			 * of items changing their managed state.	*/

		if (!current)	/* indicate why change_managed() is called */
			FPART(new).relayout_hint |= ExmFLAT_INIT;

		(*FCLASS(new).change_managed)(new, (ExmFlatItem *)NULL,
							(Cardinal)0);

				/* Now, set the redisplay flag and
				 * reset the internal state flags	*/

		redisplay			= True;
		FPART(new).items_touched	= False;
		FPART(new).relayout_hint	= False;

		if (new->core.width == (Dimension)0 ||
		    new->core.height == (Dimension)0)
		{
			OlVaDisplayWarningMsg((XtDisplay(new),
				OleNinvalidDimension,
				OleTwidgetSize, OleCOlToolkitWarning,
				OleMinvalidDimension_widgetSize,
				XtName(new),OlWidgetToClassName(new)));

			if (new->core.width == (Dimension)0)
			{
				new->core.width = (Dimension)1;
			}
			if (new->core.height == (Dimension)0)
			{
				new->core.height = (Dimension)1;
			}
		}

#ifdef MOOLIT
	/* need to understand how we are going to use this resource in flat */

#define FOO	(((FlatWidget)(new))->primitive).shadow_thickness

		if (FOO != 0)
		{
			new->core.width += 2 * FOO;
			new->core.height += 2 * FOO;
		}
#undef FOO
#endif
	}
	return(redisplay);
} /* end of ExmFlatStateSetValues() */

/*
 *************************************************************************
 * ExmFlatGetValues() - gets sub-object resources
 ****************************procedure*header*****************************
 */
void
ExmFlatGetValues(Widget		w,
		 Cardinal	item_index,
		 ArgList	args,
		 Cardinal	num_args)
{
	static _XmConst char *	proc_name = "ExmFlatGetValues()";

	if (num_args == (Cardinal)0)
	{
		return;
	}
	else if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg(((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name));
		return;
	}
	else if (args == (ArgList)NULL)
	{
		OlVaDisplayWarningMsg((XtDisplay(w), OleNinvalidArgCount,
				      OleTflatState, OleCOlToolkitWarning,
				      OleMinvalidArgCount_flatState,
				      (unsigned) num_args, proc_name,
				      XtName(w), OlWidgetToClassName(w),
				      (unsigned) item_index));
		return;
	}
	else if (item_index == ExmNO_ITEM ||
		    item_index >= FPART(w).num_items)
	{
		OlVaDisplayWarningMsg((XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XtName(w), OlWidgetToClassName(w), proc_name,
			(unsigned) item_index));
		return;
	}
	else
	{
		GetItemState(w, item_index, args, &num_args);
	}
} /* end of ExmFlatGetValues() */

/*
 *************************************************************************
 * ExmFlatSetValues() - sets sub-object resources.  If the width or height
 * of the sub-object changes during the setting of its resources,
 * the container's relayout procedure will be called.
 ****************************procedure*header*****************************
 */
void
ExmFlatSetValues(Widget		w,
		Cardinal	item_index,
		ArgList		args,
		Cardinal	num_args)
{
	static _XmConst char *	proc_name = "ExmFlatSetValues()";

	if (num_args == (Cardinal)0)
	{
		return;
	}
	else if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg(((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name));
		return;
	}
	else if (args == (ArgList)NULL)
	{
		OlVaDisplayWarningMsg((XtDisplay(w), OleNinvalidArgCount,
			OleTflatState, OleCOlToolkitWarning,
			OleMinvalidArgCount_flatState,
			(unsigned) num_args, proc_name,
			XtName(w), OlWidgetToClassName(w),
			(unsigned) item_index));
		return;
	}
	else if (item_index == ExmNO_ITEM ||
		    item_index >= FPART(w).num_items)
	{
		OlVaDisplayWarningMsg((XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XtName(w), OlWidgetToClassName(w), proc_name,
			(unsigned) item_index));
		return;
	}
	else
	{
		SetItemState(w, item_index, args, &num_args);
	}
} /* end of ExmFlatSetValues() */

/*
 *************************************************************************
 * ExmVaFlatGetValues() - variable argument interface to ExmFlatGetValues.
 ****************************procedure*header*****************************
 */
void
ExmVaFlatGetValues(	Widget		w,
			Cardinal	item_index,
			...)
{
	va_list		vargs;
	ArgList		args;
	Cardinal	num_args;
	int		total_count;
	int		typed_count;

	VA_START(vargs, item_index);
	_XtCountVaList(vargs, &total_count, &typed_count);
	va_end(vargs);

	VA_START(vargs, item_index);
	_XtVaToArgList(w, vargs, total_count, &args, &num_args);

	if (num_args != (Cardinal)0)
	{
		ExmFlatGetValues(w, item_index, args, num_args);
		XtFree((char *)args);
	}
	va_end(vargs);
} /* end of ExmVaFlatGetValues() */

/*
 *************************************************************************
 * ExmVaFlatSetValues() - variable argument interface to ExmFlatSetValues.
 ****************************procedure*header*****************************
 */
void
ExmVaFlatSetValues(	Widget		w,
			Cardinal	item_index,
			...)
{
	va_list		vargs;
	ArgList		args;
	Cardinal	num_args;
	int		total_count;
	int		typed_count;

	VA_START(vargs, item_index);
	_XtCountVaList(vargs, &total_count, &typed_count);
	va_end(vargs);

	VA_START(vargs, item_index);
	_XtVaToArgList(w, vargs, total_count, &args, &num_args);

	if (num_args != (Cardinal)0)
	{
		ExmFlatSetValues(w, item_index, args, num_args);
		XtFree((char *)args);
	}
	va_end(vargs);
} /* end of ExmVaFlatSetValues() */

/*
 *************************************************************************
 * ExmFlatChangeManagedItems - public routine for changing the managed
 * state of items.  This routine will clear the areas of the items whose
 * managed state is being changed.
 ****************************procedure*header*****************************
 */
void
ExmFlatChangeManagedItems(	Widget		w,
				Cardinal *	managed_indexes,
				Cardinal	num_managed,
				Cardinal *	unmanaged_indexes,
				Cardinal	num_unmanaged)
{
#define LOCAL_ITEMS		4
#define LOCAL_REC_SIZE		32
#define TOTAL_LOCAL_REC_SIZE	LOCAL_ITEMS * LOCAL_REC_SIZE

	Cardinal	i;
	Cardinal	total = 0;
	Cardinal	max_changed = num_managed + num_unmanaged;
	ExmFlatItem	local_items[LOCAL_ITEMS];	/* on stack	     */
				/* See Xt:XtSetValues() for why using double */
	double		local_items_recs[TOTAL_LOCAL_REC_SIZE]; /* on stack  */
	ExmFlatItemRec *items_buf;
	ExmFlatItem *	items;

	if (max_changed > (Cardinal)LOCAL_ITEMS)
	{
		items = (ExmFlatItem *)XtMalloc(max_changed *
							sizeof(ExmFlatItem));
	}
	else
	{
		items = local_items;
	}

	if (FCLASS(w).rec_size >
		(Cardinal)(sizeof(double) * LOCAL_REC_SIZE))
	{
		items_buf = (ExmFlatItemRec *)XtMalloc(max_changed *
						FCLASS(w).rec_size);
	}
	else
	{
		items_buf = (ExmFlatItemRec *)local_items_recs;
	}

	for (i=0; i < num_managed; ++i)
	{
		items[total] = CacheManager(w, managed_indexes[i],
					(items_buf + total), ADD_REFERENCE);
		items[total++]->flat.managed = True;
	}

			/* Append the items that want to be unmanaged to
			 * the combined array.  Clear their areas before
			 * calling the class procedure since these items
			 * should disappear after calling the class
			 * procedure.					*/

	for (i=0; i < num_unmanaged; ++i)
	{
		items[total] = CacheManager(w, unmanaged_indexes[i],
					(items_buf + total), ADD_REFERENCE);
		items[total++]->flat.managed = False;

		if (XtIsRealized(w) == True)
		{
			WidePosition	x;
			WidePosition	y;
			Dimension	width;
			Dimension	height;

			ExmFlatGetItemGeometry(w, unmanaged_indexes[i],
					&x, &y, &width, &height);
			CLEAR_IT(w, x, y, width, height);
		}
	}

	if (total)
	{
		Dimension	old_width = w->core.width;
		Dimension	old_height = w->core.height;

		(*FCLASS(w).change_managed)(w, items, total);

			/* the code below is from FRowColumn:GeometryHandler
			 * FGraph probably will have the same code, so
			 * we probably should make it as a private
			 * routine...
			 */
		if (old_width != w->core.width ||
		    old_height != w->core.height)
		{
			Boolean			resize = False;
			XtGeometryResult	p_result;
			XtWidgetGeometry	p_request;
			XtWidgetGeometry	p_reply;

#define MODE(r) ((r)->request_mode)

			MODE(&p_request) = 0;

			if (old_width != w->core.width)
			{
				MODE(&p_request) |= CWWidth;
				p_request.width   = w->core.width;
				w->core.width     = old_width;
			}

			if (old_height != w->core.height)
			{
				MODE(&p_request) |= CWHeight;
				p_request.height  = w->core.height;
				w->core.height    = old_height;
			}

			if ((p_result = XtMakeGeometryRequest(w, &p_request,
					&p_reply)) == XtGeometryAlmost)
			{
				resize = True;
				p_request = p_reply;
				p_result = XtMakeGeometryRequest(w, &p_request,
							&p_reply);
			}

				/* If the result was a Yes, do nothing
				 * because we've already laid the items
				 * assuming the size is approved.  If the
				 * request is refused or a compromise was
				 * used, then we'll call the resize procedure
				 * to recover.				*/

			if ((p_result == XtGeometryNo || resize == True) &&
			    XtClass(w)->core_class.resize)
			{
				(*(XtClass(w)->core_class.resize))(w);
			}
#undef MODE
		}

		while (total--)
		{
				/* Since we've already cleared the items
				 * that are now unmanaged, clear the areas
				 * of the managed (and mapped) items.	*/

			if (XtIsRealized(w) == True			&&
			    items[total]->flat.managed == True		&&
			    items[total]->flat.mapped_when_managed == True)
			{
				WidePosition	x;
				WidePosition	y;
				Dimension	width;
				Dimension	height;

				ExmFlatGetItemGeometry(w,
						items[total]->flat.item_index,
						&x, &y, &width, &height);
				CLEAR_IT(w, x, y, width, height);
			}

				/* Unregister the items			*/

			(void)CacheManager(w, items[total]->flat.item_index,
						items[total], DEC_REFERENCE);
		}
	}

	if (max_changed > LOCAL_ITEMS)
	{
		XtFree((XtPointer) items);
	}

	if (FCLASS(w).rec_size >
		(Cardinal)(sizeof(double) * LOCAL_REC_SIZE))
	{
		XtFree((XtPointer) items_buf);
	}

#undef TOTAL_LOCAL_REC_SIZE
#undef LOCAL_REC_SIZE
#undef LOCAL_ITEMS
} /* end of ExmFlatChangeManagedItems() */
