#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:container.c	1.10.1.1"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/ScrolledW.h>
#include "FIconBoxP.h"		/* FLH MORE: for _XmStringCreate: put in DtI.h?*/
#include "DtI.h"


#define EXTRA_SLOTS	16
#define IsSeparator(CH)	(strchr(sepstr, (CH)))

#ifndef MEMUTIL
extern char *strdup();
#endif /* MEMUTIL */

/* default set of separators recognized in icon labels */
#define DFLT_SEPARATORS		" _.-"

static char *sepstr = NULL;
static Dimension DtILabelWidth;

/****************************procedure*header*********************************
 *      DmWrappedLabel - generate a wrapped label
 *	Input:	separators - a list of separator characters
 *		width - max. width of a label
 *****************************************************************************/
void
DmSetWrappedLabelInfo(char *separators, Dimension width)
{
	if (sepstr)
		free(sepstr);

	if (separators)
		sepstr = strdup(separators);
	else
		sepstr = strdup(DFLT_SEPARATORS);

	DtILabelWidth = width;
} /* end of DmSetWrappedLabelInfo() */

/****************************procedure*header*********************************
 *      DmWrappedLabel - generate a wrapped label
 *****************************************************************************/
_XmString
DmWrappedLabel(XmFontList font, char *string)
{
	char *nameptr;
	XmString xmstr;
	_XmString _xmstr;

	xmstr = XmStringCreate( string, XmFONTLIST_DEFAULT_TAG);
	_xmstr = _XmStringCreate(xmstr);
	XmStringFree(xmstr);

	if (sepstr) {
	Dimension width, height;

	_XmStringExtent(font, _xmstr, &width, &height);
	if (width < DtILabelWidth)
		/* entire label does not exceed max. label width */
		return(_xmstr);
	else {
	char *sepptr; /* ptr to where the separator is */
	char *srcptr; /* source pointer */
	char *dstptr; /* destination pointer */
	char *rowptr; /* ptr to the beginning of a row */

	/*
	 * Allocate enough space for the worst case. That is, 1.5 times the
	 * string length, because each part(row) of a label must have at least
	 * 2 characters. Add one for the terminating NULL character.
	 */
	nameptr = (char *)malloc(strlen(string) * 3 / 2 + 1);

	/* always copy the first character, even if it is a separator */
	/* this should take care of dot files and others */
	sepptr = NULL;
	srcptr = string;
	dstptr = nameptr;
	rowptr = dstptr;
	*dstptr++ = *srcptr++;

	/* Now scan the source string for separators */
	for (; *srcptr; srcptr++) {
	   *dstptr++ = *srcptr;

	   /* A separator and is not the last character in the label */
	   if (IsSeparator(*srcptr) && (*(srcptr+1) != '\0')) {
	      /* found a separator */
	      *dstptr = '\0';
	      xmstr = XmStringCreate( rowptr, XmFONTLIST_DEFAULT_TAG);
	      _xmstr = _XmStringCreate(xmstr);
	      _XmStringExtent(font, _xmstr, &width, &height);
	      if (width > DtILabelWidth) {
	         if (sepptr) {
	            /* Break at the previous separator */
	            char *p;

	            /* make room for a newline */
	            for (p=dstptr; p > sepptr;) {
	                *p = *(p - 1);
	                p--;
	            }

	            *p = '\n';
	            rowptr = ++p;
	            sepptr = ++dstptr; /* compensate for the new linefeed */
	         }
	         else {
	            /* wrap label */
	            *dstptr++ = '\n';
	            rowptr = dstptr;
	            sepptr = NULL;
	         }
	      } /* if (width > DtILabelWidth) */
	      else {
	         /* remember this last found separator */
	         sepptr = dstptr;
	      }
	   } /* if IsSeparator */
	} /* for *srcptr */

	*dstptr = '\0'; /* terminating NULL char */

	if (sepptr) {
		/* Had a separator before, see if the rest of the string
		 * fits in a row.
		 */
		xmstr = XmStringCreate( rowptr, XmFONTLIST_DEFAULT_TAG);
		_xmstr = _XmStringCreate(xmstr);
		_XmStringExtent(font, _xmstr, &width, &height);

		if (width > DtILabelWidth) {
	            char *p;

	            /* make room for a newline */
	            for (p=dstptr; p > sepptr;) {
	                *p = *(p - 1);
	                p--;
	            }

	            *p = '\n';
		    *(dstptr+1) = '\0';
		}
	}

	xmstr = XmStringCreateLtoR( nameptr, XmFONTLIST_DEFAULT_TAG);
	_xmstr = _XmStringCreate(xmstr);
	XmStringFree(xmstr);
	free(nameptr);
	} /* else of (width < DtILabelWidth) */
	} /* if (sepstr) */

	return(_xmstr);
} /* end of DmWrappedLabel() */

/****************************procedure*header*********************************
 *      LabelAndSizeItem- common code to label an icon item and size it.
 *
 *	INPUTS: icon box
 *		item pointer
 *		options:
 *			DM_B_SPECIAL_NAME: app-specific label, don't make label
 *					   or calculate size
 *			DM_B_CALC_SIZE: calculate item size (assuming iconic
 *					view)
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
static void
LabelAndSizeItem(Widget box, DmItemPtr item, DtAttrs options)
{
    if (options & DM_B_SPECIAL_NAME)
    {
	item->label		= NULL;
	item->icon_width	= (XtArgVal)1;
	item->icon_height	= (XtArgVal)1;

    } else
    {
	/* FLH MORE: use Motif internal string representation
	   here.  Need to use Xm routines to free structure later.
	   Put LABEL macros in DtI.h. (copy from Dtm.h)
	   */
	if (options & DM_B_WRAPPED_LABEL)
		MAKE_WRAPPED_LABEL(item->label, FPART(box).font,
			 DmGetObjectName(ITEM_OBJ(item)));
	else
		MAKE_LABEL(item->label, DmGetObjectName(ITEM_OBJ(item)));

	/* Note: only attempt to size item if label is not "special" */
	if (options & DM_B_CALC_SIZE)
	{
	    DmSizeIcon(box, item);

	} else
	{
	    item->icon_width 	= (XtArgVal)1;
	    item->icon_height	= (XtArgVal)1;
	}
    }
}				/* end of LabelAndSizeItem */

/*****************************************************************************
 *  	DmResetIconContainer:  Given a new object list and an existing
 *		item list for a FIconBox, free memory associated with
 *		the old list and create a new list.
 *	INPUTS: existing icon_box
 *		options: DM_B_SHOW_DOT_DOT
 *			 DM_B_NO_INIT	(don't init object type)
 *			 DM_B_SPECIAL_NAME (passed to LabelAndSizeItem)
 *			 DM_B_CALC_SIZE (passed to LaberlAndSizeItem)
 *		object list	
 *		number of objects
 *		current item list
 *		size of current item list
 *		minimum number of extra slots (over num_objs) desired
 *	OUTPUTS: num_items modified if we had to grow the item list to
 *		satisfy the desired number of objects and extra_slots
 *	GLOBALS:
 *****************************************************************************/
void
DmResetIconContainer( Widget icon_box,
		      DtAttrs attrs,	/* options */
		      DmObjectPtr objp,	/* list of object descriptions */
		      Cardinal num_objs,/* number of objects */
		      DmItemPtr * itemp,/* ptr to FIconBox items */
		      Cardinal *num_items,/* number of items in/out */
		      Cardinal extra_slots)
{
    register DmObjectPtr op;
    register DmItemPtr ip;
    Cardinal num_desired = num_objs + extra_slots;
    Arg arg[5];
    int nargs = 0;
    int i;


    /* 
     * First free any memory associated with the old item list
     */
    for (i=0, ip = *itemp; i< *num_items; i++, ip++){
	if (ip->label != NULL)
	    FREE_LABEL(ip->label);
    }

    if (num_desired > *num_items){	
	/* MORE: this is a "grow only" policy.  Should we try shrinking
	 * the list when it has too many unneeded entries?
	 */
	*itemp = (DmItemPtr)REALLOC(*itemp, 
				    sizeof(DmItemRec) * num_desired);
	*num_items = num_desired;
	XtSetArg(arg[nargs], XmNitems, *itemp); nargs++;
	XtSetArg(arg[nargs], XmNnumItems, *num_items); nargs++;
    }

    ip = *itemp;

    for (i=0, op=objp; i < num_objs; i++, op=op->next) {
	if ((op->attrs & DM_B_HIDDEN) && 
	    !((attrs & DM_B_SHOW_DOT_DOT) && !strcmp(op->name, "..")))
	    continue;
	
	if (!(attrs & DM_B_NO_INIT))
	    DmInitObjType(icon_box, op);
	
	ip->x		= (XtArgVal)(op->x);
	ip->y		= (XtArgVal)(op->y);
	ip->managed	= (XtArgVal)TRUE;
	ip->select	= (XtArgVal)FALSE;
	ip->sensitive	= (XtArgVal)TRUE;
	ip->client_data	= (XtArgVal)NULL;
	ip->object_ptr	= (XtArgVal)op;
	LabelAndSizeItem(icon_box, ip, attrs);
	ip++;
    }
    
    /* Mark unused items as unmanaged.  Other fields will be
     * initialized when the items are made managed.
     */
    for (i=(int)(ip - *itemp); i < *num_items; i++, ip++) {
	ip->managed 	= (XtArgVal)FALSE;
	ip->select	= (XtArgVal)FALSE;
	ip->object_ptr	= (XtArgVal)NULL;	/* Needed for FMAP */
	ip->label 	= (XtArgVal)NULL;
	ip->icon_width 	= (XtArgVal)1;		/* Needed for FGraph.c */
	ip->icon_height	= (XtArgVal)1;		/* Needed for FGraph.c */

    }

    /* Now that we have sized the items, update the FlatIconBox. */
    XtSetArg(arg[nargs], XmNitemsTouched, True);nargs++;
    XtSetValues(icon_box, arg, nargs);

}	/* end of DmResetIconContainer */

/*
 * assumption: before calling this routine, each object's x and y values
 * should be calculated according to the view type.
 */
/*****************************************************************************
 * DmCreateIconContainer: Given an object list, create a FIconBox
 * 		and the associated item list to represent the objects
 *		in the FIconBox.  Optionally create a scrolled window
 *		as the parent of the FIconBox.
 *	INPUTS: parent widget for FIconBox
 *		args for FIconBox
 *		num_args
 *		options: DM_B_SHOW_DOT_DOT
 *			 DM_B_NO_INIT	(don't init object type)
 *			 DM_B_SPECIAL_NAME (passed to LabelAndSizeItem)
 *			 DM_B_CALC_SIZE (passed to LaberlAndSizeItem)
 *		object list	
 *		number of objects
 *		address for item list
 *		size of current item list
 *		address for swin widget
 *	OUTPUTS:  FIconBox widget (return value)
 *		  item list (returned in address passed as param)
 * 	GLOBALS:
 *****************************************************************************/
Widget
DmCreateIconContainer( Widget parent,	/* parent widget */
		      DtAttrs attrs,	/* options */
		      ArgList args,	/* arglist passed to flat icon box */
		      Cardinal num_args,/* number of args */
		      DmObjectPtr objp,	/* list of object descriptions */
		      Cardinal num_objs,/* number of objects */
		      DmItemPtr * itemp,/* ptr to FIconBox items */
		      Cardinal num_items,/* number of items */
		      Widget * swin)	/* widget id of scrolled window */
{
    register DmObjectPtr op;
    register DmItemPtr ip;
    int i;
    Widget flat = NULL;
    ArgList merged_args;
    Arg int_args[6];
    static String fields[] = {
	XmNlabelString,
	XmNx,
	XmNy,
	XmNwidth,
	XmNheight,
	XmNmanaged,
	XmNset,
	XmNsensitive,
	XmNuserData,
	XmNobjectData
	};
    
    /* allocate item list */
    if ((ip = (DmItemPtr)MALLOC(sizeof(DmItemRec) * num_items)) == NULL)
	return(NULL);
    *itemp = ip;
    
    if (swin) {
	Arg int_args[8];
	
	/* create scrolled window.  The FlatIconBox will set
	   the scrolling units.
	   */ 
	/* FLH MORE: do we need to set some attachments here? 
	 * Attach to right and left of FORM.  
	 * See BaseWGizmo.c.  
	 */
	XtSetArg(int_args[0], XmNscrollingPolicy, XmAPPLICATION_DEFINED);
	XtSetArg(int_args[1], XmNvisualPolicy, XmVARIABLE);
	XtSetArg(int_args[2], XmNscrollBarDisplayPolicy, XmSTATIC);
	XtSetArg(int_args[3], XmNshadowThickness, 0);
	*swin = XtCreateManagedWidget("ScrolledWin",
				      xmScrolledWindowWidgetClass, parent, 
				      int_args, 4);
	parent = *swin;
    }
    
    for (i=0, op=objp; i < num_objs; i++, op=op->next) {
	if ((op->attrs & DM_B_HIDDEN) && 
	    !((attrs & DM_B_SHOW_DOT_DOT) && !strcmp(op->name, "..")))
	    continue;
	
	if (!(attrs & DM_B_NO_INIT))
	    DmInitObjType(parent, op);
	
	ip->x		= (XtArgVal)(op->x);
	ip->y		= (XtArgVal)(op->y);
	ip->icon_width 	= (XtArgVal)1;		/* Needed for FGraph.c */
	ip->icon_height	= (XtArgVal)1;		/* Needed for FGraph.c */
	ip->managed	= (XtArgVal)FALSE;
	ip->select	= (XtArgVal)FALSE;
	ip->sensitive	= (XtArgVal)TRUE;
	ip->client_data	= (XtArgVal)NULL;
	ip->object_ptr	= (XtArgVal)op;
	ip->label = (XtArgVal) NULL;
	ip++;
    }
    
    /* Mark unused items as unmanaged.  Other fields will be
     * initialized when the items are made managed.
     */
    for (i=(int)(ip - *itemp); i < num_items; i++, ip++) {
	ip->managed 	= (XtArgVal)FALSE;
	ip->object_ptr	= (XtArgVal)NULL;	/* Needed for FMAP */
	ip->label 	= (XtArgVal) NULL;
	ip->icon_width 	= (XtArgVal)1;		/* Needed for FGraph.c */
	ip->icon_height	= (XtArgVal)1;		/* Needed for FGraph.c */

    }
    
    /*
     * If we do not yet have a FIconBox, we must create one before
     * sizing the items because we need the XmFontList from the 
     * FlatIconBox to properly size the items.
     * 
     * Initialize those item fields that we can.
     */
    XtSetArg(int_args[0], XmNitemFields, fields);
    XtSetArg(int_args[1], XmNnumItemFields, XtNumber(fields));
    XtSetArg(int_args[2], XmNdragCursorProc, DmCreateIconCursor);
    XtSetArg(int_args[3], XmNitems, *itemp);
    XtSetArg(int_args[4], XmNnumItems, num_items);
    i = 5;
    if (num_args != 0)
	merged_args = XtMergeArgLists(int_args, i, args, num_args);
    else
	merged_args = int_args;
    
    /* Need to create the FIconBox before sizing and managing
     *  the items.
     */
    flat = XtCreateManagedWidget("flat", exmFlatIconBoxWidgetClass, parent,
				 merged_args, i + num_args);
    ip = *itemp;
    for (i=0, op=objp; i < num_objs; i++, op=op->next) {
	if ((op->attrs & DM_B_HIDDEN) && 
	    !((attrs & DM_B_SHOW_DOT_DOT) && !strcmp(op->name, "..")))
	    continue;
	ip->managed	= (XtArgVal)TRUE;
	LabelAndSizeItem(flat, ip, attrs);
	ip++;
    }
    /* Now that we have sized the items, update the FlatIconBox. */
    XtSetArg(int_args[0], XmNitemsTouched, True);
    XtSetValues(flat, int_args, 1);
    
    if (num_args != 0)
	FREE(merged_args);
    
    return(flat);
} /* DmCreateIconContainer */

/****************************procedure*header*****************************
    Dm__GetFreeItems- return a free item from 'items'.  'need_cnt' is a hint
	to indicate how many items are needed.  Return:
	   0 - unmanaged items found
	   1 - items array had to be expanded
	  -1 - error
*/
int
Dm__GetFreeItems(DmItemPtr * items, Cardinal * num_items, Cardinal need_cnt, DmItemPtr * ret_item)
{
    Cardinal	i;
    DmItemPtr	item;
    int		status = 0;
    Cardinal	cnt;
    Cardinal	indx;

    cnt = 0;
    indx = ExmNO_ITEM;
    for (i = 0, item = *items; i < *num_items; i++, item++)
	if ( !ITEM_MANAGED(item) )
	{
	    /* Save this item index.  Don't save item pointer since array
	       may get realloc'ed below.
	    */
	    indx = item - *items;

	    cnt++;
	    if (cnt == need_cnt)
		break;
	}

    if (cnt != need_cnt)	/* Not enough free items */
    {
	cnt = need_cnt - cnt + EXTRA_SLOTS;

	/* Expand items array */
	*items = (DmItemPtr)REALLOC(*items,
				    sizeof(DmItemRec) * (*num_items + cnt));

	if (*items == NULL)	/* Memory error */
	{
	    status = -1;
	    item = NULL;
	    goto quit;
	}

	/* zero out new/extra entries */
	(void)memset((void *)(*items + *num_items), 0, sizeof(DmItemRec)*cnt);

	status = 1;

	/* 'item' is unmanaged item or first new item */
	item = (indx == ExmNO_ITEM) ? *items + *num_items : *items + indx;
	*num_items += cnt;
    }

 quit:
    *ret_item = item;
    return(status);
}					/* end of Dm__GetFreeItems */

/****************************procedure*header*****************************
    Dm__AddObjToIcontainer-  

    !!WARNING!!: 
    
    For folder windows, objects should be added to containers
    via DmAddObjectToContainer so that all views of the object
    are correctly updated.  This routine, Dm__AddObjToIcontainer 
    will only update the iconbox passed in as a parameter; others
    will not be updated.
*/
Cardinal
Dm__AddObjToIcontainer(
	Widget ibox,			/* FIconBox widget */
	DmItemPtr * items, Cardinal * num_items,
	DmContainerPtr cp,
	DmObjectPtr op,			/* ptr to obj to add */
	WidePosition x, WidePosition y,	/* or UNSPECIFIED_POS */
	DtAttrs options,
	Dimension wrap_width,		/* for positioning (optional) */
	Dimension grid_width,		/* for positioning (optional) */
	Dimension grid_height)		/* for positioning (optional) */
{
    int		status;
    DmItemPtr	item;

    /* Add object to container */
    if (Dm__AddToObjList(cp, op, options) == -1)
	return(ExmNO_ITEM);

    /* Get an available item (items array may grow) */
    if ( (status = Dm__GetFreeItems(items, num_items, 1, &item)) == -1 )
	return(ExmNO_ITEM);

    if ( !(options & DM_B_NO_INIT) )
	DmInitObjType(ibox, op);

    /* Initialize item.  XtNmanaged is done below!! */

    item->select	= (XtArgVal)False;
    item->sensitive	= (XtArgVal)True;
    item->client_data	= (XtArgVal)NULL;
    item->object_ptr	= (XtArgVal)op;

    /* Label and size item before attempting to position it below */
    LabelAndSizeItem(ibox, item, options);

    /* Get "available" position if one is not specified */
    if ((x == UNSPECIFIED_POS) && (y == UNSPECIFIED_POS))
    {
	WidePosition icon_x, icon_y;

	DmGetAvailIconPos(ibox, *items, *num_items,
			  ITEM_WIDTH(item), ITEM_HEIGHT(item),
			  wrap_width, grid_width, grid_height,
			  &icon_x, &icon_y);

	item->x = (XtArgVal)icon_x;
	item->y = (XtArgVal)icon_y;
    } else
    {
	if (x == 0)
	{
	    item->x	= (XtArgVal)x;

	} else
	{
	    item->x	= (XtArgVal)(x - (ITEM_WIDTH(item) / 2));
	    if (ITEM_X(item) < 0)
		item->x = 0;
	}
	if (y == 0)
	{
	    item->y	= (XtArgVal)y;

	} else
	{
	    item->y	= (XtArgVal)(y - (GLYPH_PTR(item)->height / 2));
	    if (ITEM_Y(item) < 0)
		item->y = 0;
	}
    }

    if (status)			/* ie. items array was touched */
    {
	item->managed = (XtArgVal)True;
	XtSetArg(Dm__arg[0], XmNnumItems,	*num_items);
	XtSetArg(Dm__arg[1], XmNitems,		*items);
	XtSetValues(ibox, Dm__arg, 2);

    } else
    {
	XtSetArg(Dm__arg[0], XmNmanaged,	True);
	XtSetArg(Dm__arg[1], XmNx,		item->x);
	XtSetArg(Dm__arg[2], XmNy,		item->y);
	XtSetArg(Dm__arg[3], XmNwidth,		item->icon_width);
	XtSetArg(Dm__arg[4], XmNheight,		item->icon_height);
	ExmFlatSetValues(ibox, (Cardinal)(item - *items), Dm__arg, 5);
    }

    return( (Cardinal)(item - *items) );

}					/* end of Dm__AddObjToIcontainer */

void
DmDestroyIconContainer(Widget shell, Widget w, DmItemPtr ilist, int nitems)
{
    register DmItemPtr ip;

    /* destroy the widget tree */
    XtUnmapWidget(shell);
    XtDestroyWidget(shell);

    for (ip=ilist; nitems; nitems--, ip++)
	if (ITEM_MANAGED(ip) != False)
		FREE_LABEL(ITEM_LABEL(ip));

    FREE(ilist);
}

