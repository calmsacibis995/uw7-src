#ifndef	NOIDENT
#ident	"@(#)flat:FlatState.c	1.44"
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
#include <Xol/OpenLookP.h>
#include <Xol/FlatP.h>

#if OlNeedFunctionPrototypes
#include <stdarg.h>
#define VA_START(a,n)	va_start(a,n)
#else /* OlNeedFunctionPrototypes */
#include <varargs.h>
#define VA_START(a,n)	va_start(a)
#endif /* OlNeedFunctionPrototypes */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define CLEAR_IT(w,x,y,wi,ht) (void)XClearArea(XtDisplay(w),XtWindow(w),\
			(int)x, (int)y,(unsigned)wi,(unsigned)ht,(Bool)1)

#ifdef __STDC__
#define SIZE_T	size_t
#else
#define SIZE_T	int
#endif
				/* Declare some private Xt externs	*/

extern void	_XtCountVaList OL_ARGS((va_list, int *, int *));
extern void	_XtVaToArgList OL_ARGS((Widget, va_list, int,
						ArgList*, Cardinal*));

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

#define IGNORE_FIELD		(~0)

#define	NUM_RSCS(w)	(OL_FLATCLASS(w).num_item_resources)
#define	RSCS(w)		(OL_FLATCLASS(w).item_resources)
#define	RSC_QNAME(w,i)	(OL_FLATCLASS(w).quarked_items[i])
#define	RSC_SIZE(w,i)	(RSCS(w)[i].resource_size)
#define	RSC_OFFSET(w,i)	(RSCS(w)[i].resource_offset)
#define REQ_RSC(w, i)	(OL_FLATCLASS(w).required_resources[i])
#define RSC_INDEX(w, i)	(REQ_RSC(w, i).rsc_index)
#define DESTROY_PROC(w,i) (REQ_RSC(w, i).destroy)

#define FPART(w)	(((FlatWidget)(w))->flat)
#define FCLASS(wc)	(((FlatWidgetClass)wc)->flat_class)

			/* Define a structure to cache items	*/

typedef struct _ItemCache {
	struct _ItemCache *	next;		/* next node		*/
	Cardinal		ref_count;	/* reference count	*/
	Cardinal		item_index;	/* index of item	*/
	FlatItem		item;		/* cached item		*/
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

static void	BuildExtractorList OL_ARGS((Widget));
static void	BuildRequiredList OL_ARGS((Widget));
static FlatItem CacheManager OL_ARGS((Widget, Cardinal, FlatItem, int));
static Boolean	CallItemProcs OL_ARGS((WidgetClass, CallOpcode, Widget,
			Widget, Widget, FlatItem, FlatItem, FlatItem,
			ArgList, Cardinal *));
static void	CreateSupplementalList OL_ARGS((Widget, Cardinal, ArgList));
static void	DestroyReqRsc OL_ARGS((Widget, char *, Cardinal,
					OlFlatReqRsc *, Cardinal));
static int	DoGeometry OL_ARGS((Widget, FlatItem, FlatItem));
static void	ExpandItem OL_ARGS((Widget, Cardinal, FlatItem));
static void	GetItemState OL_ARGS((Widget, Cardinal, ArgList, Cardinal *));
static Boolean	InitializeItems OL_ARGS((Widget, Widget, Widget,
						ArgList, Cardinal *));
static void	InitializeReqResources OL_ARGS((Widget));
static void	SetItemState OL_ARGS((Widget, Cardinal, ArgList, Cardinal *));

					/* public procedures		*/

void		_OlFlatStateDestroy OL_ARGS((Widget));
void		_OlFlatStateInitialize OL_ARGS((Widget, Widget,
						ArgList, Cardinal *));
Boolean		_OlFlatStateSetValues OL_ARGS((Widget, Widget, Widget,
						ArgList, Cardinal *));
Boolean		OlFlatItemActivate OL_ARGS((Widget, FlatItem,
					OlVirtualName, XtPointer));
void		OlFlatExpandItem OL_ARGS((Widget, Cardinal, FlatItem));
void		OlFlatGetValues OL_ARGS((Widget, Cardinal, ArgList, Cardinal));
void		OlFlatSetValues OL_ARGS((Widget, Cardinal, ArgList, Cardinal));
void		OlVaFlatGetValues OL_ARGS((Widget, Cardinal, ...));
void		OlVaFlatSetValues OL_ARGS((Widget, Cardinal, ...));
void		OlFlatChangeManagedItems OL_ARGS((Widget, Cardinal *, Cardinal,
							Cardinal *, Cardinal));
void		OlFlatSyncItem OL_ARGS((Widget, FlatItem));
XtGeometryResult OlFlatMakeGeometryRequest OL_ARGS((Widget, 
			FlatItem, OlFlatItemGeometry*,OlFlatItemGeometry*));

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
BuildExtractorList OLARGLIST((w))
	OLGRA( Widget,		w)	/* Flat widget id		*/
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
		qlist = OL_FLATCLASS(w).quarked_items;

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

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNinvalidResource, OleTbadItemResource,
				OleCOlToolkitWarning,
				OleMinvalidResource_badItemResource,
				XrmQuarkToString(w->core.xrm_name),
				XtClass(w)->core_class.class_name,
				resource);

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
} /* END OF BuildExtractorList() */

/*
 *************************************************************************
 * BuildRequiredList - this procedure builds an array of information that
 * contains indices of resources that a sub-object requires so that
 * it is initialized properly.   The maximum allowable set of resources
 * that can have indices in this list are those specified in this
 * subclass's required_resources list.
 ****************************procedure*header*****************************
 */
static void
BuildRequiredList OLARGLIST((w))
	OLGRA( Widget,		w)	/* Flat widget id		*/
{
	Cardinal		num_req =
					OL_FLATCLASS(w).num_required_resources;
	OlFlatResourceInfo *	ri = &(FPART(w).resource_info);
	OlFlatReqRsc *		req_rsc;
	Cardinal *		new_list;
	Cardinal		new_entries;
	Cardinal		record_size;
	Cardinal		req_rsc_index;
	Cardinal		xlist_index;

	if (num_req == (Cardinal)0 ||
	    FPART(w).num_items == (Cardinal)0)
	{
		return;
	}

			/* Loop over all the required resources for this
			 * widget to see which ones were not in the extractor
			 * list.  If required resource is not in the extractor
			 * list, then add it to the instance required_resource
			 * index list only if it has a NULL predicate
			 * procedure or the predicate procedure returns
			 * TRUE.					*/

	for (req_rsc_index = record_size = new_entries = (Cardinal)0,
	     req_rsc = OL_FLATCLASS(w).required_resources,
	     new_list = (Cardinal *)XtMalloc((num_req+1) * sizeof(Cardinal));
	     req_rsc_index < num_req;
	     ++req_rsc_index, ++req_rsc)
	{
			/* See if the required resource is in the
			 * extractor list.				*/

		for (xlist_index=0; xlist_index < ri->num_xlist; ++xlist_index)
		{
			if (req_rsc->rsc_index == ri->xlist[xlist_index])
				break;
		}

			/* the required resource is not in the extractor
			 * list, so check the predicate procedure.	*/

		if (xlist_index == ri->num_xlist &&
		    (req_rsc->predicate == (OlFlatReqRscPredicateFunc)NULL ||
		     (*req_rsc->predicate)(w,
					RSC_OFFSET(w, req_rsc->rsc_index),
					req_rsc->name) == (Boolean)True))
		{
				/* cache the class req_rsc index in the
				 * cardinal array.			*/
			new_list[new_entries] = req_rsc_index;
			record_size += RSC_SIZE(w, req_rsc->rsc_index);
			++new_entries;
		}
	}

	if (new_entries == (Cardinal)0)
	{
		XtFree((char *)new_list);
	}
	else
	{
		if (new_entries < num_req)
		{
					/* realloc, leaving enough room
					 * for the record_size		*/

			new_list = (Cardinal *)XtRealloc((char *)new_list,
				(new_entries+1)*sizeof(Cardinal));
		}

				/* Store record size in the last slot.	*/

		new_list[new_entries] = record_size;

		ri->rlist	= new_list;
		ri->num_rlist	= new_entries;

				/* Allocate enough bytes to 'num_items'
				 * of records and initialize them.	*/

		ri->rdata	= XtMalloc(record_size * FPART(w).num_items);

		InitializeReqResources(w);
	}
} /* END OF BuildRequiredList() */

/*
 *************************************************************************
 * CacheManager - this routine manages an item cache.  What the routine
 * actually does is dependent on the supplied action flag.  The routine
 * returns the address of the expanded item or NULL.
 ****************************procedure*header*****************************
 */
static FlatItem
CacheManager OLARGLIST((w, item_index, item, action))
	OLARG( Widget,		w)		/* flat widget id	*/
	OLARG( Cardinal,	item_index)	/* requested item	*/
	OLARG( FlatItem,	item)		/* item or NULL		*/
	OLGRA( int,		action)		/* What to do		*/
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

		OlFlatSyncItem(w, i_self->item);

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
				(OLconst XtPointer)i_self->item,
				(SIZE_T)OL_FLATCLASS(w).rec_size);
		}
		break;
	}
	return(item);
} /* END OF CacheManager() */

/*
 *************************************************************************
 * CallItemProcs - this routine recursively calls the initialize item
 * procedure for each flat subclass.
 ****************************procedure*header*****************************
 */
static Boolean
CallItemProcs OLARGLIST((wc, opcode, c_w, r_w, w, current, request, new,
			args, num_args))
	OLARG( WidgetClass, wc)		/* Current WidgetClass		*/
	OLARG( CallOpcode,  opcode)	/* procedure to call		*/
	OLARG( Widget,	    c_w)	/* Current Flat Widget		*/
	OLARG( Widget,	    r_w)	/* Request Flat Widget		*/
	OLARG( Widget,	    w)		/* New Flat Widget		*/
	OLARG( FlatItem,    current)	/* expanded current item	*/
	OLARG( FlatItem,    request)	/* expanded requested item	*/
	OLARG( FlatItem,    new)	/* expanded new item		*/
	OLARG( ArgList,	    args)
	OLGRA( Cardinal *,  num_args)
{
	Boolean		return_val = False;
	FlatClassPart *	fcp = &FCLASS(wc);

	if (wc != flatWidgetClass)
	{
		if (CallItemProcs(wc->core_class.superclass, opcode, c_w, r_w,
			w, current, request, new, args, num_args) == True)
		{
			return_val = True;
		}
	}

					/* Call subclass's routine	*/

	switch(opcode)
	{
	case INITIALIZE:
		if (fcp->initialize != (XtInitProc)NULL)
		{
			(*fcp->initialize)(r_w, w, args, num_args);
		}
		break;
	case SET_VALUES:
		if (fcp->set_values != (XtSetValuesFunc)NULL)
		{
			if ((*fcp->set_values)(c_w, r_w, w,
					args, num_args) == True)
			{
				return_val = True;
			}
		}
		break;
	case DEFAULT_ITEM_INITIALIZE:
		if (fcp->default_initialize != (OlFlatItemInitializeProc)NULL)
		{
			(*fcp->default_initialize)(w, request, new,
							args, num_args);
		}
		break;
	case DEFAULT_ITEM_SETVALUES:
		if (fcp->default_set_values != (OlFlatItemSetValuesFunc)NULL)
		{
			if ((*fcp->default_set_values)(w, current, request,
					new, args, num_args) == True)
			{
				return_val = True;
			}
		}
		break;
	case ITEM_GETVALUES:
		if (fcp->item_get_values != (OlFlatItemGetValuesProc)NULL)
		{
			(*fcp->item_get_values)(w, current, args, num_args);
		}
		break;
	case ITEM_INITIALIZE:
		if (fcp->item_initialize != (OlFlatItemInitializeProc)NULL)
		{
			(*fcp->item_initialize)(w, request, new,
							args, num_args);
		}
		break;
	case ITEM_SETVALUES:
		if (fcp->item_set_values != (OlFlatItemSetValuesFunc)NULL)
		{
			if ((*fcp->item_set_values)(w, current, request,
						new, args, num_args) == True)
			{
				return_val = True;
			}
		}
		break;
	default:				/* ANALYZE_ITEMS	*/
		if (fcp->analyze_items != (OlFlatAnalyzeItemsProc)NULL)
		{
			(*fcp->analyze_items)(w, args, num_args);
		}
		break;
	}
	return(return_val);
} /* END OF CallItemProcs() */

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
ExpandItem OLARGLIST((w, item_index, item))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLGRA( FlatItem, 	item)	/* uninitialized expanded item	*/
{
	char *			dest = (char *)item;
	OLconst char *		src;	/* sub_object base address	*/
	register Cardinal *	list;	/* extractor list		*/
	Cardinal		num;

				/* Load the default item and stick the
				 * item index in the expanded item.	*/

	(void)memcpy((XtPointer)item, (OLconst XtPointer)OlFlatDefaultItem(w),
		(SIZE_T)OL_FLATCLASS(w).rec_size);

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
				_OlCopyFromXtArgVal(*((XtArgVal *)src), 
					(char *)(dest + RSC_OFFSET(w, *list)),
					RSC_SIZE(w, *list));
			}
			src += sizeof(XtArgVal);
		}
	}

			/* If there are required sub-object resources being
			 * managed internally, copy them into the
			 * expanded item.				*/

	if ((num = FPART(w).resource_info.num_rlist) != (Cardinal)0)
	{
		Cardinal	record_size;
		XtResource *	rsc;

		list		= FPART(w).resource_info.rlist;
		record_size	= list[num];
		src		= FPART(w).resource_info.rdata +
					(item_index * record_size);

		for ( ; num != (Cardinal)0; --num, ++list)
		{
			rsc = RSCS(w) + RSC_INDEX(w, *list);

			(void)memcpy((XtPointer)(dest + rsc->resource_offset),
					(OLconst XtPointer)src,
					(SIZE_T)rsc->resource_size);
			src += rsc->resource_size;
		}
	}
} /* END OF ExpandItem() */

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
GetItemState OLARGLIST((w, item_index, args, num_args))
	OLARG( Widget,		w)		/* Flat widget subclass	*/
	OLARG( Cardinal,	item_index)	/* item to be expanded	*/
	OLARG( ArgList,		args)		/* list of Args to get	*/
	OLGRA( Cardinal *,	num_args)	/* number of Args	*/
{
	XrmQuarkList	qlist;		/* all subclass item resources	*/
	XrmQuark	quark;
	Cardinal	num;
	Cardinal	count;
	FlatItem	new;
	OL_FLAT_ALLOC_ITEM(w, FlatItem, new_buf);

		/* Expand the item with the CacheManager.  We saved the
		 * address of 'new' since the call to CacheManager may
		 * return an address other than 'new'.			*/

	new = CacheManager(w, item_index, new_buf, ADD_REFERENCE);

			/* Call the subclasses to add information
			 * or possibly change some of it.		*/

	(void)CallItemProcs(XtClass(w), ITEM_GETVALUES, (Widget)NULL,
			(Widget)NULL, w, new, (FlatItem)NULL,
			(FlatItem)NULL, args, num_args);

						/* Loop over the args.	*/

	for (count = (Cardinal)0; count < *num_args; ++count, ++args)
	{
		qlist = OL_FLATCLASS(w).quarked_items;
		quark = XrmStringToQuark(args->name);

		for (num=0; num < NUM_RSCS(w); ++qlist, ++num)
		{
			if (quark == *qlist)
			{
				/* Copy the value out of the expanded
				 * item and place it in the application's
				 * arg list.				*/

				_OlCopyToXtArgVal((char *) ((char *)new +
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

	OL_FLAT_FREE_ITEM(new_buf);

} /* END OF GetItemState() */

/*
 *************************************************************************
 * InitializeReqResources - initializes required resources for an
 * flattened widget instance.  The default value in the item resource
 * list is used for the initializing.
 ****************************procedure*header*****************************
 */
static void
InitializeReqResources OLARGLIST((w))
	OLGRA( Widget,		w)
{
	OlFlatResourceInfo *	ri = &(FPART(w).resource_info);
	XtResource *		rsc;
	char *			dest;
	Cardinal		i;
	Cardinal		size;

				/* Initialize the required_resources	*/

	for (size=i=0, dest=ri->rdata; i < ri->num_rlist; ++i, dest += size)
	{
		rsc	= RSCS(w) + RSC_INDEX(w, ri->rlist[i]);
		size	= rsc->resource_size;

		if (!strcmp(rsc->default_type, XtRCallProc))
		{
			XrmValue		to_val;
			XtResourceDefaultProc	proc = (XtResourceDefaultProc)
							rsc->default_addr;
			to_val.size = (unsigned int)0;

			(*proc)(w, rsc->resource_offset, &to_val);

			if ((Cardinal)to_val.size == size)
			{
				(void)memcpy(dest, (OLconst char *)to_val.addr,
						(int)size);
			}
			else
			{
				(void)memset(dest, 0, (int)size);
			}
		}
		else		/* default and resource types are same	*/
		{
			(void)memcpy(dest,
				(OLconst char *)rsc->default_addr, (int)size);
		}
	}

		/* At this point, we've only initialized the first record
		 * for the internally-managed required resource list.
		 * So for now, copy the first record to all the remaining
		 * records.						*/

	size = ri->rlist[ri->num_rlist];	/* the record_size	*/

	for (i=1, dest=ri->rdata + size;
	     i < FPART(w).num_items; ++i, dest+=size)
	{
		(void)memcpy((XtPointer)dest, (OLconst XtPointer)ri->rdata,
				(SIZE_T)size);
	}
} /* END OF InitializeReqResources() */

/*
 *************************************************************************
 * _OlFlatStateDestroy - destroys state information associated
 * with the item's.  It calls the destroy procedures for all required
 * resources that have them and then it frees the extractor and required
 * resource list as well as the buffer used to hold the required resources.
 * Once the information is destroy, the fields are re-initialized
 * since this routine is called when either the widget is being destroyed,
 * or when the widget's item list is removed.
 ****************************procedure*header*****************************
 */
void
_OlFlatStateDestroy OLARGLIST((w))
	OLGRA( Widget,		w)	/* The Flat Widget		*/
{
	Cardinal	num;

	if ((num = FPART(w).resource_info.num_rlist) != (Cardinal)0 &&
	    FPART(w).num_items != (Cardinal)0)
	{
		Cardinal *	list = FPART(w).resource_info.rlist;
		char *		dest = FPART(w).resource_info.rdata;
		Cardinal	record_size = list[num];
		XtResource *	rsc;

		for ( ; num != (Cardinal)0; --num, ++list)
		{
			rsc = RSCS(w) + RSC_INDEX(w, *list);

				/* If this resource has a destroy
				 * procedure, call it for every item.	*/

			if (DESTROY_PROC(w, *list))
			{
				DestroyReqRsc(w, dest, record_size,
						&REQ_RSC(w, *list),
						FPART(w).num_items);
			}
			dest += rsc->resource_size;
		}
	}

	XtFree((XtPointer) FPART(w).resource_info.rdata);
	XtFree((XtPointer) FPART(w).resource_info.rlist);
	XtFree((XtPointer) FPART(w).resource_info.xlist);

	FPART(w).resource_info.rdata		= (char *)NULL;
	FPART(w).resource_info.rlist		= (Cardinal *)NULL;
	FPART(w).resource_info.xlist		= (Cardinal *)NULL;
	FPART(w).resource_info.num_rlist	= (Cardinal)0;
	FPART(w).resource_info.num_xlist	= (Cardinal)0;

} /* END OF _OlFlatStateDestroy() */

/*
 *************************************************************************
 * DestroyReqRsc - routine for destroying required resources
 ****************************procedure*header*****************************
 */
static void
DestroyReqRsc OLARGLIST((w, addr, record_size, r_rsc, num))
	OLARG( Widget,		w)	/* The Flat Widget		*/
	OLARG( char *,		addr)	/* Address of first data	*/
	OLARG( Cardinal,	record_size)
	OLARG( OlFlatReqRsc *,	r_rsc)	/* the required resource	*/
	OLGRA( Cardinal,	num)	/* number of iterations		*/
{
	char		buf[sizeof(XtPointer)];
	XtPointer	dest;
	XtResource *	rsc = RSCS(w) + r_rsc->rsc_index;

	dest = (rsc->resource_size <= sizeof(buf) ? (XtPointer)buf :
			XtMalloc(rsc->resource_size));

			/* When destroying information from the required
			 * resource data section, we have to do a memcpy
			 * to a place which won't have alignment problems.
			 * Therefore, we use a local buffer (or an
			 * allocated one if necessary).  We'll then
			 * supply the address of that buffer to the
			 * destroy proc to free.			*/

	while (num--)
	{
		(void)memcpy(dest, (OLconst XtPointer)addr,
					(SIZE_T)rsc->resource_size);
		(*r_rsc->destroy)(w, rsc->resource_offset,
					rsc->resource_name, dest);
		addr += record_size;
	}

	if (dest != (XtPointer)buf)
	{
		XtFree((XtPointer)dest);
	}
} /* END OF DestroyReqRsc() */

/*
 *************************************************************************
 * OlFlatSyncItem - this routine copies values from an item and places
 * them into the application's list and for any required_resources, puts
 * the values into the required resource list.
 ****************************procedure*header*****************************
 */
void
OlFlatSyncItem OLARGLIST((w, item))
	OLARG( Widget,		w)
	OLGRA( FlatItem,	item)
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
				_OlConvertToXtArgVal((src +
					RSC_OFFSET(w, *list)),
					(XtArgVal *)dest, RSC_SIZE(w, *list));
			}
			dest += sizeof(XtArgVal);
		}
	}

			/* If there are required sub-object resources being
			 * managed internally, copy them from the expanded
			 * item back into the internal storage.		*/

	if ((num = FPART(w).resource_info.num_rlist) != (Cardinal)0)
	{
		Cardinal	record_size;
		XtResource *	rsc;

		list		= FPART(w).resource_info.rlist;
		record_size	= list[num];
		dest		= FPART(w).resource_info.rdata +
					(item->flat.item_index * record_size);

		for ( ; num != (Cardinal)0; --num, ++list)
		{
			rsc = RSCS(w) + RSC_INDEX(w, *list);

				/* If this resource has a destroy procedure,
				 * call it on the destination if the
				 * destination and the source differ.	*/

			if (DESTROY_PROC(w, *list) &&
			    memcmp((OLconst XtPointer)dest,
				(OLconst XtPointer)(src + rsc->resource_offset),
				(SIZE_T)rsc->resource_size))
			{
				DestroyReqRsc(w, dest, record_size,
						&REQ_RSC(w, *list), 1);
			}

			(void)memcpy((XtPointer)dest,
				(OLconst XtPointer)(src + rsc->resource_offset),
				(SIZE_T)rsc->resource_size);

			dest += rsc->resource_size;
		}
	}
} /* END OF OlFlatSyncItem() */

/*
 *************************************************************************
 * OlFlatMakeGeometryRequest - used so that an item can request its
 * managing widget to make a change in the item's size or position.
 ****************************procedure*header*****************************
 */
XtGeometryResult
OlFlatMakeGeometryRequest OLARGLIST((w, item, request, reply))
	OLARG( Widget, 			w)
	OLARG( FlatItem,		item)
	OLARG( OlFlatItemGeometry *,	request)
	OLGRA( OlFlatItemGeometry *,	reply)
{
	Position		x;
	Position		y;
	Dimension		height;
	Dimension		width;
	XtGeometryResult	result;
	OlFlatItemGeometry	tmp;

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

	result = (*OL_FLATCLASS(w).geometry_handler)(w, item, request,
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

			OlFlatSyncItem(w, item);
		}
		else if (result == XtGeometryDone)
		{
			result = XtGeometryYes;
		}
	}
	return(result);
} /* END OF OlFlatMakeGeometryRequest() */

/*
 *************************************************************************
 * DoGeometry - handles geometry negoiations that emanated from setting
 * the state on an item.  This routine returns True if the item has
 * already been refreshed due to resolving the geometry resolution.
 ****************************procedure*header*****************************
 */
static int
DoGeometry OLARGLIST((w, current, new))
	OLARG( Widget,		w)
	OLARG( FlatItem,	current)
	OLGRA( FlatItem,	new)
{
	XtGeometryResult	result = XtGeometryNo;
	OlFlatItemGeometry	request;

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
		OlFlatItemGeometry	reply;
		OlFlatItemResizeProc	resize_proc =
						OL_FLATCLASS(w).item_resize;
		OlFlatItemSetValuesAlmostProc	almost_proc =
					OL_FLATCLASS(w).item_set_values_almost;

		do
		{
			result = OlFlatMakeGeometryRequest(w, new,
						&request, &reply);

			if (result == XtGeometryYes)
			{
				break;
			}

				/* Since OlFlatMakeGeometryRequest never
				 * returns XtGeometryDone, we're here
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

			if (almost_proc != (OlFlatItemSetValuesAlmostProc)NULL)
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

				/* If the result is Yes, then the item has
				 * been configured already (so we don't need
				 * to generate an exposure).  All we need to
				 * to is call the resize procedure if it
				 * exists.
				 * Else, just clear the container so it
				 * can redraw itself.			*/

		if (result == XtGeometryYes)
		{
			if ((request.request_mode & (CWWidth|CWHeight)) &&
				resize_proc)
			{
				(*resize_proc)(w, new); 
			}
		}
	} /* end of request.request_mode != 0 */

	return(result == XtGeometryYes);
} /* END OF DoGeometry() */

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
SetItemState OLARGLIST((w, item_index, args, num_args))
	OLARG( Widget,		w)		/* Flat widget subclass	*/
	OLARG( Cardinal,	item_index)	/* item to be expanded	*/
	OLARG( ArgList,		args)		/* list of Args to touch*/
	OLGRA( Cardinal *,	num_args)	/* number of Args	*/
{
	Boolean			redisplay;
	Cardinal		num;
	Cardinal		count;
	Cardinal *		list;
	FlatItem		new;
	ArgList			cached_args = args;
	XrmQuark		quark;
	OL_FLAT_ALLOC_ITEM(w, FlatItem, current);
	OL_FLAT_ALLOC_ITEM(w, FlatItem, request);
	OL_FLAT_ALLOC_ITEM(w, FlatItem, new_buf);

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
		OlFlatGetItemGeometry(w, item_index, &new->flat.x,
			&new->flat.y, &new->flat.width, &new->flat.height);
	}

	(void)memcpy((XtPointer)current, (OLconst XtPointer)new,
				(SIZE_T)OL_FLATCLASS(w).rec_size);

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

				_OlCopyFromXtArgVal(args->value,
					(char *)((char *)new +
						RSC_OFFSET(w, *list)),
					RSC_SIZE(w, *list));
				break;
			}
		} /* looping over item resources for this subclass */

			/* If the arg was not in the extractor list,
			 * check to see if it's in the required resource
			 * list.					*/

		if (num == (Cardinal)0 &&
		   (num = FPART(w).resource_info.num_rlist) != (Cardinal)0)
		{
			Cardinal	rsc_index;

			for (list = FPART(w).resource_info.rlist;
			     num != (Cardinal)0; --num, ++list)
			{
				rsc_index = RSC_INDEX(w, *list);

				if (quark == RSC_QNAME(w, rsc_index))
				{
					XtResource *	rsc = RSCS(w) +
							rsc_index;

					_OlCopyFromXtArgVal(args->value,
						(char *)((char *)new +
						rsc->resource_offset),
						(int)rsc->resource_size);
					break;
				}
			}
				/* if we couldn't find thru the	*/
				/* required list, we better do	*/
				/* something about it		*/
			if (num == (Cardinal)0)
			{
				XrmQuarkList	qlist;

				qlist = OL_FLATCLASS(w).quarked_items;
				
					/* make sure this is a flat resource */
				for (num = 0; num < NUM_RSCS(w); ++num, ++qlist)
				{
					if (quark == *qlist)
					{
						CreateSupplementalList(
							w, item_index, args
						);
						break;
					}
				}
			}
		}

	} /* looping over num_args */

				/* Make the request item be a copy of the
				 * new item before the widget gets to
				 * see the changes.			*/

	(void) memcpy((XtPointer)request, (OLconst XtPointer)new,
			(SIZE_T)OL_FLATCLASS(w).rec_size);

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
				/* OlFlatChangeManagedItems() will do	*/
				/* the redisplay for us			*/
			redisplay = False;

			if (new->flat.managed == True)
			{
				OlFlatChangeManagedItems(w,
					&new->flat.item_index, (Cardinal)1,
					(Cardinal *)NULL, (Cardinal)0);
			}
			else
			{
				OlFlatChangeManagedItems(w,
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
			OlFlatRefreshExpandedItem(w, new, True);
		}
	}

			/* Decrement the cached item's reference count,
			 * and free any local storage.			*/

	(void)CacheManager(w, item_index, new, DEC_REFERENCE);

	OL_FLAT_FREE_ITEM(current);
	OL_FLAT_FREE_ITEM(request);
	OL_FLAT_FREE_ITEM(new_buf);

} /* END OF SetItemState() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlFlatExpandItem - this procedure initializes an application supplied
 * item.  The returned item is considered read-only since any changes
 * made to it will not be saved.  When initializing the item, the routine
 * checks if the requested item is currently cached.  If it is, it
 * returns a copy of the cached item.  If it is not, it does the expansion.
 ****************************procedure*header*****************************
 */
void
OlFlatExpandItem OLARGLIST((w, item_index, item))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)	/* item to expand	*/
	OLGRA( FlatItem,	item)		/* destination address	*/
{
	if (item == (FlatItem)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemAddress,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemAddress_flatState,
			XtName(w), OlWidgetToClassName(w), "OlFlatExpandItem",
			(unsigned) item_index);
	}
	else
	{
		item = CacheManager(w, item_index, item, LOOKUP);
	}
} /* END OF OlFlatExpandItem() */

/*
 *************************************************************************
 * InitializeItems - handles calling routines that update and monitor
 * the default item and the individual items.   This routine is called
 * by OlFlatStateSetValues()  (which is called whenever the flat widget 
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
InitializeItems OLARGLIST((c_w, r_w, w, args, num_args))
	OLARG( Widget,		c_w)		/* Current flat widget	*/
	OLARG( Widget,		r_w)		/* Request flat widget	*/
	OLARG( Widget,		w)		/* New flat widget	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	Boolean			redisplay = False;
	Cardinal		i;		/* current item index	*/
	FlatItem		new;
	OL_FLAT_ALLOC_ITEM(w, FlatItem, request);
	OL_FLAT_ALLOC_ITEM(w, FlatItem, new_buf);

	if (FPART(w).num_item_fields && FPART(w).item_fields == (String *)NULL)
	{
	    OlVaDisplayWarningMsg(XtDisplay(w),
				  OleNinvalidResource, OleTnullList,
				  OleCOlToolkitWarning,
				  OleMinvalidResource_nullList,
				  XtName(w), OlWidgetToClassName(w),
				  "XtNitemFields", "XtNnumItemFields");

	    /* set the count to zero	*/
	    FPART(w).num_item_fields = 0;
	}

			/* copy the new default item into the request
			 * item so the widget's can check changes that
			 * have been refused.				*/

	(void)memcpy((XtPointer)request,
			(OLconst XtPointer)OlFlatDefaultItem(w),
			(SIZE_T)OL_FLATCLASS(w).rec_size);

	if (c_w == NULL)
	{
		(void)CallItemProcs(XtClass(w), DEFAULT_ITEM_INITIALIZE,
			c_w, r_w, w, (FlatItem)NULL, request, 
			OlFlatDefaultItem(w), args, num_args);
	}
	else
	{
		if (CallItemProcs(XtClass(w), DEFAULT_ITEM_SETVALUES,
			c_w, r_w, w, OlFlatDefaultItem(c_w), request, 
			OlFlatDefaultItem(w), args, num_args) == TRUE)
		{
			redisplay = TRUE;
		}
	}

			/* No items were touched, then return now.	*/

	if (FPART(w).items_touched == FALSE)
	{
		OL_FLAT_FREE_ITEM(request);
		OL_FLAT_FREE_ITEM(new_buf);

		return(redisplay);
	}

			/* Since the items were touched, destroy state
			 * information associated with the old items.
			 * Then, begin building new state information.
			 * Note, we don't have to destroy any information
			 * at initialization time (when c_w == NULL).	*/

	if (c_w != (Widget)NULL)
	{
		_OlFlatStateDestroy(c_w);

			/* The following fields have to be zero'd out	*/
			/* otherwise it will have problems when passing	*/
			/* in a NULL item list. This is because		*/
			/* BuildExtractorList() will simply return in	*/
			/* this case. As a result, the old pointers are	*/
			/* still around and will get into trouble when	*/
			/* the item list is touched again...		*/
		FPART(w).resource_info.xlist		= (Cardinal *)NULL;
		FPART(w).resource_info.num_xlist	= (Cardinal)0;
		FPART(w).resource_info.rlist		= (Cardinal *)NULL;
		FPART(w).resource_info.num_rlist	= (Cardinal)0;
		FPART(w).resource_info.rdata		= (char *)NULL;
	}

				/* build the extractor list and the
				 * required resource list		*/

	BuildExtractorList(w);
	BuildRequiredList(w);

	for (i=0; i < FPART(w).num_items; ++i)
	{
		/* Expand the item as it is before the update. We saved
		 * the address of 'new' since the call to CacheManager
		 * may return an address other than 'new'.		*/

		new = CacheManager(w, i, new_buf, ADD_REFERENCE);

				/* Copy the new item into the request
				 * item so the flat widget can compare
				 * differences.				*/

		(void)memcpy((XtPointer)request, (OLconst XtPointer)new,
				(SIZE_T)OL_FLATCLASS(w).rec_size);

			/* Call subclass's item_initialize routine	*/

		(void)CallItemProcs(XtClass(w), ITEM_INITIALIZE, c_w, r_w, w,
			(FlatItem)NULL, request, new, args, num_args);

				/* Decrement the cached item's reference
				 * count.				*/

		(void)CacheManager(w, i, new, DEC_REFERENCE);
	}

	OL_FLAT_FREE_ITEM(request);
	OL_FLAT_FREE_ITEM(new_buf);

			/* Call subclass's analyze_items routine so
			 * that the flat widget can look at all items
			 * collectively.				*/

	(void)CallItemProcs(XtClass(w), ANALYZE_ITEMS, c_w, r_w, w,
			(FlatItem)NULL, (FlatItem)NULL, (FlatItem)NULL,
			args, num_args);

	return(TRUE);
} /* END OF InitializeItems() */

/*
 *************************************************************************
 * _OlFlatStateInitialize - responsible for calling the appropriate
 * procedures to maintain the items and the container's knowledge of
 * changes to the items caused by application requests at the container's
 * initialization time.
 *
 * Only the superclass of all flat widgets (i.e., flatClass) should call
 * this routine.  It's called from that class's CORE Initialize.  This
 * routine calls _OlFlatStateSetValues() to do all of the work.
 ****************************procedure*header*****************************
 */
void
_OlFlatStateInitialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)	/* reqeust flat widget	*/
	OLARG( Widget,		new)		/* new flat widget	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	FPART(new).resource_info.xlist		= (Cardinal *) NULL;
	FPART(new).resource_info.num_xlist	= (Cardinal) 0;
	FPART(new).resource_info.rlist		= (Cardinal *) NULL;
	FPART(new).resource_info.num_rlist	= (Cardinal) 0;
	FPART(new).resource_info.rdata		= (char *) NULL;
	FPART(new).relayout_hint		= (Boolean)True;
	FPART(new).items_touched		=
			(FPART(new).num_items != (Cardinal)0 ? True : False);

	OlFlatDefaultItem(new)->flat.item_index = OL_DEFAULT_ITEM;

	(void) _OlFlatStateSetValues((Widget)NULL, request, new,
						args, num_args);
} /* END OF _OlFlatStateInitialize () */


/* PERF: (Performance improvement work, 4/92)                             */
/*       These macros (ALLOCATE_MAYBE and FREE_MAYBE) are substituted for */
/*       inoperative ALLOCATE_LOCAL and DEALLOCATE_LOCAL when a probably  */
/*       constant amount of storage is required a number of times.  That  */
/*       storage is set aside (Local...Buf) and used if it is sufficiently*/
/*       large for ALLOCATE_LOCAL demands.  If it isn't big enough, or if */
/*       it's already in use, a real ALLOC takes place.  FREE_MAYBE       */
/*       unwinds this situation.   [AS]                                   */

#define ALLOCATE_MAYBE(num,auto,inuse) \
	((!inuse&&(num)<=sizeof(auto))?inuse++,(char *)(auto):(char *)(malloc(num)))
#define FREE_MAYBE(actual,auto,inuse) \
	{if ((actual)!=(auto)) free(actual); else inuse--;}


/*
 *************************************************************************
 * _OlFlatStateSetValues - responsible for calling the appropriate
 * procedures to maintain the items and the container's knowledge of
 * changes to the items caused by application requests.
 *
 * Only the superclass of all flat widgets (i.e., flatClass) should call
 * this routine.  It's called from that class's CORE Initialize (indirectly
 * through _OlFlatStateInitialize) and SetValues routines.
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
_OlFlatStateSetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)	/* current flat widget	*/
	OLARG( Widget,		request)	/* reqeust flat widget	*/
	OLARG( Widget,		new)		/* new flat widget	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	Boolean redisplay = False;
        union { double force_align; char localBuf[1024];} u; /* PERF tuning */
        char localBufInUse = 0;                             /* PERF tuning */

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

		/* XtResourceList rscs = (XtResourceList) XtMalloc( /* PERF */
		/* sizeof(XtResource) * 		  /** PERF: see above */
		/* OL_FLATCLASS(new).num_item_resources); /** PERF: see above */

		XtResourceList	rscs = (XtResourceList) ALLOCATE_MAYBE(
		  (sizeof(XtResource) * OL_FLATCLASS(new).num_item_resources),
		  u.localBuf, localBufInUse); 	/** PERF: see above */

		(void) memcpy((XtPointer)rscs, (OLconst XtPointer)
				OL_FLATCLASS(new).item_resources,
				(SIZE_T)(sizeof(XtResource) * 
					OL_FLATCLASS(new).num_item_resources));

		XtGetSubresources(new, (XtPointer)OlFlatDefaultItem(new),
				(String)NULL, (String)NULL,
				rscs, OL_FLATCLASS(new).num_item_resources,
				args, *num_args);

		/* XtFree((XtPointer) rscs); 	/** PERF: see above */
		FREE_MAYBE((XtPointer)rscs,u.localBuf,localBufInUse);  /* PERF */

		(void)CallItemProcs(XtClass(new), INITIALIZE, current,
			request, new, (FlatItem)NULL, (FlatItem)NULL,
			(FlatItem)NULL, args, num_args);
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

		if ((DIFFERENT(items_touched) &&
			FPART(new).items_touched == False)	||
		    DIFFERENT(item_fields)			||
		    DIFFERENT(num_item_fields)			||
		    DIFFERENT(items)				||
		    DIFFERENT(num_items))
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
		
			for(j=0; j < OL_FLATCLASS(new).num_item_resources; ++j)
			{
				if (quark == OL_FLATCLASS(new).quarked_items[j])
				{
					rscs[matches] = OL_FLATCLASS(new).
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
				new, (XtPointer)OlFlatDefaultItem(new),
				(String)NULL, (String)NULL,
				rscs, matches,
				args, *num_args);
		}

		if (rscs != auto_rscs)
		{
			XtFree((XtPointer)rscs);
		}

		redisplay = CallItemProcs(XtClass(new), SET_VALUES, current,
			request, new, (FlatItem)NULL, (FlatItem)NULL,
			(FlatItem)NULL, args, num_args);
	}

		/* Now that the changes have been applied to the default
		 * item and that the container's initialize or set_values
		 * routines have been called, see if any work needs to
		 * be done on the items.				*/

	if (InitializeItems(current, request, new, args, num_args) == True)
	{
		redisplay = True;
	}

	if (FPART(new).items_touched == True ||
	    FPART(new).relayout_hint == True)
	{

			/* Call the change managed routine to do the
			 * re-layout.  Specifying a count of zero implies
			 * that this request is not a result of a set
			 * of items changing their managed state.	*/

		(*OL_FLATCLASS(new).change_managed)(new, (FlatItem *)NULL,
							(Cardinal)0);

				/* Now, set the redisplay flag and
				 * reset the internal state flags	*/

		redisplay			= True;
		FPART(new).items_touched	= False;
		FPART(new).relayout_hint	= False;

		if (new->core.width == (Dimension)0 ||
		    new->core.height == (Dimension)0)
		{
			OlVaDisplayWarningMsg(XtDisplay(new),
				OleNinvalidDimension,
				OleTwidgetSize, OleCOlToolkitWarning,
				OleMinvalidDimension_widgetSize,
				XtName(new),OlWidgetToClassName(new));

			if (new->core.width == (Dimension)0)
			{
				new->core.width = (Dimension)1;
			}
			if (new->core.height == (Dimension)0)
			{
				new->core.height = (Dimension)1;
			}
		}

#define FOO	(((FlatWidget)(new))->primitive).shadow_thickness

		if (FOO != 0)
		{
			new->core.width += 2 * FOO;
			new->core.height += 2 * FOO;
		}
#undef FOO
	}
	return(redisplay);
} /* END OF _OlFlatStateSetValues() */

/*
 *************************************************************************
 * OlFlatGetValues() - gets sub-object resources
 ****************************procedure*header*****************************
 */
void
OlFlatGetValues OLARGLIST((w, item_index, args, num_args))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( ArgList,		args)
	OLGRA( Cardinal,	num_args)
{
	static char *	proc_name = "OlFlatGetValues()";

	if (num_args == (Cardinal)0)
	{
		return;
	}
	else if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name);
		return;
	}
	else if (args == (ArgList)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNinvalidArgCount,
				      OleTflatState, OleCOlToolkitWarning,
				      OleMinvalidArgCount_flatState,
				      (unsigned) num_args, proc_name,
				      XtName(w), OlWidgetToClassName(w),
				      (unsigned) item_index);
		return;
	}
	else if (item_index == (Cardinal)OL_NO_ITEM ||
		    item_index >= FPART(w).num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XtName(w), OlWidgetToClassName(w), proc_name,
			(unsigned) item_index);
		return;
	}
	else
	{
		GetItemState(w, item_index, args, &num_args);
	}
} /* END OF OlFlatGetValues() */

/*
 *************************************************************************
 * OlFlatSetValues() - sets sub-object resources.  If the width or height
 * of the sub-object changes during the setting of its resources,
 * the container's relayout procedure will be called.
 ****************************procedure*header*****************************
 */
void
OlFlatSetValues OLARGLIST((w, item_index, args, num_args))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( ArgList,		args)
	OLGRA( Cardinal,	num_args)
{
	static char *	proc_name = "OlFlatSetValues()";

	if (num_args == (Cardinal)0)
	{
		return;
	}
	else if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name);
		return;
	}
	else if (args == (ArgList)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNinvalidArgCount,
			OleTflatState, OleCOlToolkitWarning,
			OleMinvalidArgCount_flatState,
			(unsigned) num_args, proc_name,
			XtName(w), OlWidgetToClassName(w),
			(unsigned) item_index);
		return;
	}
	else if (item_index == (Cardinal)OL_NO_ITEM ||
		    item_index >= FPART(w).num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XtName(w), OlWidgetToClassName(w), proc_name,
			(unsigned) item_index);
		return;
	}
	else
	{
		SetItemState(w, item_index, args, &num_args);
	}
} /* END OF OlFlatSetValues() */

/*
 *************************************************************************
 * OlVaFlatGetValues() - variable argument interface to OlFlatGetValues.
 ****************************procedure*header*****************************
 */
void
OlVaFlatGetValues OLARGLIST((w, item_index, OLVARGLIST))
	OLARG(Widget,		w)
	OLARG(Cardinal,	item_index)
	OLVARGS
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
		OlFlatGetValues(w, item_index, args, num_args);
		XtFree((char *)args);
	}
	va_end(vargs);
} /* END OF OlVaFlatGetValues() */

/*
 *************************************************************************
 * OlVaFlatSetValues() - variable argument interface to OlFlatSetValues.
 ****************************procedure*header*****************************
 */
void
OlVaFlatSetValues OLARGLIST((w, item_index, OLVARGLIST))
	OLARG(Widget,		w)
	OLARG(Cardinal,	item_index)
	OLVARGS
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
		OlFlatSetValues(w, item_index, args, num_args);
		XtFree((char *)args);
	}
	va_end(vargs);
} /* END OF OlVaFlatSetValues() */

/*
 *************************************************************************
 * OlFlatItemActivate - routine for activating a flat item.
 * This routine was made a procedure since we may want to chain item
 * activate procedures in the future.
 ****************************procedure*header*****************************
*/
Boolean
OlFlatItemActivate OLARGLIST((w, item, type, data))
	OLARG( Widget,		w)
	OLARG( FlatItem,	item)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	data)
{
	return((item->flat.sensitive && OL_FLATCLASS(w).item_activate ?
		(*OL_FLATCLASS(w).item_activate)(w, item, type, data) :
		False));
} /* END OF OlFlatItemActivate() */

/*
 *************************************************************************
 * OlFlatChangeManagedItems - public routine for changing the managed
 * state of items.  This routine will clear the areas of the items whose
 * managed state is being changed.
 ****************************procedure*header*****************************
 */
void
OlFlatChangeManagedItems OLARGLIST((w, managed_indexes, num_managed,
				unmanaged_indexes, num_unmanaged))
	OLARG( Widget,		w)
	OLARG( Cardinal *,	managed_indexes)
	OLARG( Cardinal,	num_managed)
	OLARG( Cardinal *,	unmanaged_indexes)
	OLGRA( Cardinal,	num_unmanaged)
{
#define LOCAL_ITEMS		4
#define LOCAL_REC_SIZE		32
#define TOTAL_LOCAL_REC_SIZE	LOCAL_ITEMS * LOCAL_REC_SIZE

	Cardinal	i;
	Cardinal	total = 0;
	Cardinal	max_changed = num_managed + num_unmanaged;
	FlatItem	local_items[LOCAL_ITEMS];	/* on stack	     */
				/* See Xt:XtSetValues() for why using double */
	double		local_items_recs[TOTAL_LOCAL_REC_SIZE]; /* on stack  */
	FlatItemRec *	items_buf;
	FlatItem *	items;

	if (max_changed > (Cardinal)LOCAL_ITEMS)
	{
		items = (FlatItem *)XtMalloc(max_changed * sizeof(FlatItem));
	}
	else
	{
		items = local_items;
	}

	if (OL_FLATCLASS(w).rec_size >
		(Cardinal)(sizeof(double) * LOCAL_REC_SIZE))
	{
		items_buf = (FlatItemRec *)XtMalloc(max_changed *
						OL_FLATCLASS(w).rec_size);
	}
	else
	{
		items_buf = (FlatItemRec *)local_items_recs;
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
			Position	x;
			Position	y;
			Dimension	width;
			Dimension	height;

			OlFlatGetItemGeometry(w, unmanaged_indexes[i],
					&x, &y, &width, &height);
			CLEAR_IT(w, x, y, width, height);
		}
	}

	if (total)
	{
		Dimension	old_width = w->core.width;
		Dimension	old_height = w->core.height;

		(*OL_FLATCLASS(w).change_managed)(w, items, total);

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
				Position	x;
				Position	y;
				Dimension	width;
				Dimension	height;

				OlFlatGetItemGeometry(w,
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

	if (OL_FLATCLASS(w).rec_size >
		(Cardinal)(sizeof(double) * LOCAL_REC_SIZE))
	{
		XtFree((XtPointer) items_buf);
	}

#undef TOTAL_LOCAL_REC_SIZE
#undef LOCAL_REC_SIZE
#undef LOCAL_ITEMS
} /* END OF OlFlatChangeManagedItems() */

/*
 *************************************************************************
 * CreateSupplementalList - currently, this routine issues a warning
 * message if an application tries to set (thru Ol[Va]FlatSetValues())
 * a resource that doesn't appear in the "itemFields" list.
 *
 * Note that: this is a temporary solution for now and an enhancement
 * should be provided in the near future.
 ****************************procedure*header*****************************
 */
static void
CreateSupplementalList OLARGLIST((w, item_index, arg))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLGRA( ArgList,		arg)
{
#ifndef OlWidgetToClassName
#define OlWidgetClassToClassName(wc)	((wc)->core_class.class_name)
#define OlWidgetToClassName(w)		OlWidgetClassToClassName(XtClass(w))
#endif

	/* these two should be added into StringList or...	*/
#define OleNfileFlatState		"fileFlatState"
#define OleMmsg				"widget \"%s\" (class \"%s\"):\
 cannot do OlFlatSetValues(item_index = %d) on \"%s\" because it is not\
 in the \"%s\" list"

	OlVaDisplayWarningMsg(
		XtDisplay(w),
		OleNfileFlatState,
		OleTflatState,
		OleCOlToolkitWarning,
		OleMmsg,
		XtName(w), OlWidgetToClassName(w),
		item_index,arg->name, XtNitemFields
	);
		
#ifndef OlWidgetToClassName
#undef OlWidgetClassToClassName
#undef OlWidgetToClassName
#endif
#undef OleNfileFlatState
#undef OleMmsg
} /* END OF CreateSupplementalList() */
