#pragma ident	"@(#)libDtI:container.c	1.37"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include "DtI.h"

#define VSTEPSIZE(W) (OlScreenPointToPixel(OL_VERTICAL,16,XtScreen(W)))
#define HSTEPSIZE(W) (OlScreenPointToPixel(OL_HORIZONTAL,16,XtScreen(W)))
#define EXTRA_SLOTS	16

#ifndef MEMUTIL
extern char *strdup();
#endif /* MEMUTIL */

static Widget
Dm__CreateIconContainer( Widget iconbox, /* passed in flat icon box */
		      Widget parent,	/* parent widget */
		      DtAttrs attrs,	/* options */
		      ArgList args,	/* arglist passed to flat icon box */
		      Cardinal num_args,/* number of args */
		      DmObjectPtr objp,	/* list of object descriptions */
		      Cardinal num_objs,/* number of objects */
		      DmItemPtr * itemp,/* ptr to FIconBox items */
		      Cardinal num_items,/* number of items */
		      Widget * swin,	/* widget id of scrolled window */
		      OlFontList *fontlist,/* font list for drawing labels */
		      XFontStruct *font,/* font for drawing labels */
		      int charheight);	/* height of label */


/****************************procedure*header*****************************
    LabelAndSizeItem- common code to label an icon item and size it.
*/
static void
LabelAndSizeItem(DmItemPtr item, DtAttrs options, OlFontList * fontlist,
		 XFontStruct * font)
{
    if (options & DM_B_SPECIAL_NAME)
    {
	item->label		= NULL;
	item->icon_width	= (XtArgVal)1;
	item->icon_height	= (XtArgVal)1;

    } else
    {
	item->label = (XtArgVal)strdup(DmGetObjectName(ITEM_OBJ(item)));

	/* Note: only attempt to size item if label is not "special" */
	if (options & DM_B_CALC_SIZE)
	{
	    DmSizeIcon(item, fontlist, font);

	} else
	{
	    item->icon_width 	= (XtArgVal)1;
	    item->icon_height	= (XtArgVal)1;
	}
    }
}				/* end of LabelAndSizeItem */

/*****************************************************************************
 *  	DmResetIconContainer:
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Widget
DmResetIconContainer( Widget flaticonbox,
		      Widget parent,	/* parent widget */
		      DtAttrs attrs,	/* options */
		      ArgList args,	/* arglist passed to flat icon box */
		      Cardinal num_args,/* number of args */
		      DmObjectPtr objp,	/* list of object descriptions */
		      Cardinal num_objs,/* number of objects */
		      DmItemPtr * itemp,/* ptr to FIconBox items */
		      Cardinal num_items,/* number of items */
		      Widget * swin,	/* widget id of scrolled window */
		      OlFontList *fontlist,/* font list for drawing labels */
		      XFontStruct *font,/* font for drawing labels */
		      int charheight)	/* height of label */
{
    Widget flat;

    flat = Dm__CreateIconContainer(flaticonbox, parent, attrs, args, num_args,
				    objp, num_objs, itemp, num_items, swin,
				    fontlist, font, charheight);
    return(flat);
}	/* end of DmResetIconContainer */
/*
 * assumption: before calling this routine, each object's x and y values
 * should be calculated according to the view type.
 */
Widget
DmCreateIconContainer(Widget parent,	/* parent widget */
		      DtAttrs attrs,	/* options */
		      ArgList args,	/* arglist passed to flat icon box */
		      Cardinal num_args,/* number of args */
		      DmObjectPtr objp,	/* list of object descriptions */
		      Cardinal num_objs,/* number of objects */
		      DmItemPtr * itemp,/* ptr to FIconBox items */
		      Cardinal num_items,/* number of items */
		      Widget * swin,	/* widget id of scrolled window */
		      OlFontList *fontlist,/* font list for drawing labels */
		      XFontStruct *font,/* font for drawing labels */
		      int charheight)	/* height of label */
{
    Widget flat;

    flat = Dm__CreateIconContainer(NULL, parent, attrs, args, num_args,
				    objp, num_objs, itemp, num_items, swin,
				    fontlist, font, charheight);
    return(flat);
}

static Widget
Dm__CreateIconContainer( Widget iconbox, /* passed in flat icon box */
		      Widget parent,	/* parent widget */
		      DtAttrs attrs,	/* options */
		      ArgList args,	/* arglist passed to flat icon box */
		      Cardinal num_args,/* number of args */
		      DmObjectPtr objp,	/* list of object descriptions */
		      Cardinal num_objs,/* number of objects */
		      DmItemPtr * itemp,/* ptr to FIconBox items */
		      Cardinal num_items,/* number of items */
		      Widget * swin,	/* widget id of scrolled window */
		      OlFontList *fontlist,/* font list for drawing labels */
		      XFontStruct *font,/* font for drawing labels */
		      int charheight)	/* height of label */
{
    register DmObjectPtr op;
    register DmItemPtr ip;
    int i;
    Widget flat = NULL;
    ArgList merged_args;
    Arg int_args[6];
    static String fields[] = {
	XtNlabel,
	XtNx,
	XtNy,
	XtNwidth,
	XtNheight,
	XtNmanaged,
	XtNset,
	XtNbusy,
	XtNuserData,
	XtNobjectData
	};

    /* allocate item list */
    if ((ip = (DmItemPtr)MALLOC(sizeof(DmItemRec) * num_items)) == NULL)
	return(NULL);
    *itemp = ip;

    if (swin) {
	Arg int_args[3];

	/* create scrolled window */
	XtSetArg(int_args[0], XtNvStepSize, 16);
	XtSetArg(int_args[1], XtNhStepSize, 16);
	XtSetArg(int_args[2], XtNweight, 1);
	*swin = XtCreateManagedWidget("ScrolledWin",
				      scrolledWindowWidgetClass, parent, int_args, 3);
	parent = *swin;
    }

    for (i=0, op=objp; i < num_objs; i++, op=op->next) {
	if ((op->attrs & DM_B_HIDDEN) && !((attrs & DM_B_SHOW_DOT_DOT) && !strcmp(op->name, "..")))
	    continue;

	if (!(attrs & DM_B_NO_INIT))
	    DmInitObjType(parent, op);

	ip->x		= (XtArgVal)(op->x);
	ip->y		= (XtArgVal)(op->y);
	ip->managed	= (XtArgVal)TRUE;
	ip->select	= (XtArgVal)FALSE;
	ip->busy	= (XtArgVal)FALSE;
	ip->client_data	= (XtArgVal)NULL;
	ip->object_ptr	= (XtArgVal)op;
	LabelAndSizeItem(ip, attrs, fontlist, font);
	ip++;
    }

    for (i=(int)(ip - *itemp); i < num_items; i++, ip++) {
	ip->label	= (XtArgVal)0;
	ip->x 		= (XtArgVal)0;
	ip->y 		= (XtArgVal)0;
	ip->icon_width 	= (XtArgVal)1;
	ip->icon_height	= (XtArgVal)1;
	ip->managed 	= (XtArgVal)FALSE;
	ip->select 	= (XtArgVal)FALSE;
	ip->busy 	= (XtArgVal)FALSE;
	ip->client_data = (XtArgVal)NULL;
	ip->object_ptr 	= (XtArgVal)NULL;
    }

    XtSetArg(int_args[0], XtNitemFields, fields);
    XtSetArg(int_args[1], XtNnumItemFields, XtNumber(fields));
    XtSetArg(int_args[2], XtNdragCursorProc, DmCreateIconCursor);
    XtSetArg(int_args[3], XtNitems, *itemp);
    XtSetArg(int_args[4], XtNnumItems, num_items);
    i = 5;
    if (num_args != 0)
	merged_args = XtMergeArgLists(int_args, i, args, num_args);
    else
	merged_args = int_args;

    if (!iconbox)
    	flat = XtCreateManagedWidget("flat", flatIconBoxWidgetClass, parent,
				 merged_args, i + num_args);
    else
    	XtSetValues(iconbox, merged_args, i + num_args);
    if (num_args != 0)
	FREE(merged_args);

    return(flat);
}

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
    indx = OL_NO_ITEM;
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
	item = (indx == OL_NO_ITEM) ? *items + *num_items : *items + indx;
	*num_items += cnt;
    }

 quit:
    *ret_item = item;
    return(status);
}					/* end of Dm__GetFreeItems */

/****************************procedure*header*****************************
    Dm__AddObjToIcontainer-

    WARNING: for folder windows, objects should be added to containers
    via DmAddObjectToContainer so that all views of the object
    are correctly updated.  This routine will only update the
    iconbox passed in as a parameter.
*/
Cardinal
Dm__AddObjToIcontainer(
	Widget ibox,			/* FIconBox widget */
	DmItemPtr * items, Cardinal * num_items,
	DmContainerPtr cp,
	DmObjectPtr op,			/* ptr to obj to add */
	Position x, Position y,		/* or UNSPECIFIED_POS */
	DtAttrs options,
	OlFontList * fontlist,		/* for sizing (optional) */
	XFontStruct * font,		/* for sizing (optional) */
	Dimension wrap_width,		/* for positioning (optional) */
	Dimension grid_width,		/* for positioning (optional) */
	Dimension grid_height)		/* for positioning (optional) */
{
    int		status;
    DmItemPtr	item;

    /* Add object to container */
    Dm__AddToObjList(cp, op, options);

    /* Get an available item (items array may grow) */
    if ( (status = Dm__GetFreeItems(items, num_items, 1, &item)) == -1 )
	return(OL_NO_ITEM);

    if ( !(options & DM_B_NO_INIT) )
	DmInitObjType(ibox, op);

    /* Initialize item.  XtNmanaged is done below!! */

    item->select	= (XtArgVal)False;
    item->busy		= (XtArgVal)False;
    item->client_data	= (XtArgVal)NULL;
    item->object_ptr	= (XtArgVal)op;

    /* Label and size item before attempting to position it below */
    LabelAndSizeItem(item, options, fontlist, font);

    /* Get "available" position if one is not specified */
    if ((x == UNSPECIFIED_POS) && (y == UNSPECIFIED_POS))
    {
	Position icon_x, icon_y;

	DmGetAvailIconPos(*items, *num_items,
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
	XtSetArg(Dm__arg[0], XtNnumItems,	*num_items);
	XtSetArg(Dm__arg[1], XtNitems,		*items);
	XtSetValues(ibox, Dm__arg, 2);

    } else
    {
	XtSetArg(Dm__arg[0], XtNmanaged,	True);
	XtSetArg(Dm__arg[1], XtNx,		item->x);
	XtSetArg(Dm__arg[2], XtNy,		item->y);
	XtSetArg(Dm__arg[3], XtNwidth,		item->icon_width);
	XtSetArg(Dm__arg[4], XtNheight,		item->icon_height);
	OlFlatSetValues(ibox, (Cardinal)(item - *items), Dm__arg, 5);
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
		XtFree(ITEM_LABEL(ip));

    FREE(ilist);
}

