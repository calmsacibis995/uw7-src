/*		copyright	"%c%"	*/

#pragma ident	"@(#)dtm:f_view.c	1.87.1.16"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-view related functions.
*/
						/* #includes go here	*/
#include <libgen.h>
#include <stdlib.h>
#include <locale.h>

#include <Xm/Xm.h>
#include <Xm/FontObj.h>
#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/TextGizmo.h>
#include "Dtm.h"
#include "extern.h"
#ifdef MOTIF_CLIST
#include "CListGizmo.h"
#endif /* MOTIF_CLIST */
#include <MGizmo/SpaceGizmo.h>
#include "dm_strings.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static int	ByName(DmItemPtr n1, DmItemPtr n2);
static int	ByPosition(DmItemPtr n1, DmItemPtr n2);
static int	BySize(DmItemPtr n1, DmItemPtr n2);
static int	ByTime(DmItemPtr n1, DmItemPtr n2);
static int	ByType(DmItemPtr n1, DmItemPtr n2);
static void	FilterView(DmFolderWindow);
static Boolean	FindFileMatch(char * objname, char * pattern);
static void	ShowCB(Widget, XtPointer, XtPointer);

					/* public procedures		*/
int		DmAddObjectToView(DmFolderWindow, Cardinal, DmObjectPtr, 
				  WidePosition, WidePosition);
void		DmRmObjectFromView(DmFolderWindow, Cardinal, DmObjectPtr);

void		DmFormatView(DmFolderWindow, DmViewFormatType);
void		DmSortItems(DmFolderWindow,
			    DmViewSortType, DtAttrs, DtAttrs, Dimension);
void		DmViewAlignCB(Widget, XtPointer, XtPointer);
void		DmViewCustomizedCB(Widget, XtPointer, XtPointer);
void		DmViewFormatCB(Widget, XtPointer, XtPointer);
void		DmViewSortCB(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* Make table of compare func's global */
PFI compare_func[] = { ByType, ByName, BySize, ByTime, ByPosition };


static InputGizmo pattern = {
	NULL, "pattern", "", 24, NULL, NULL
};

static GizmoRec pattern_rec[] =  {
	{ InputGizmoClass, &pattern },
};

static LabelGizmo pattern_label = {
	NULL,			/* help */
	"fname_label",		/* widget name */
	TXT_FILE_PATTERN,	/* caption label */
	False,			/* align caption */
	pattern_rec,		/* gizmo array */
	XtNumber(pattern_rec), /* number of gizmos */
};

static CListGizmo custom_clist = {
	NULL,			/* help */
	"customlist",		/* name */
	4,			/* view width */
	NULL,			/* required property */
	False,		        /* file */
	False,			/* sys class */
	True,			/* xenix class */
	False,			/* usr class */
 	True,			/* overridden class */
	False,			/* exclusives behavior */
	False,			/* noneset behaviour */
	NULL,			/* select proc */
};

static GizmoRec type_rec[] = {
	{ CListGizmoClass, &custom_clist },
};

static LabelGizmo type_label = {
	NULL,			/* help */
	"class",		/* widget name */
	TXT_IB_FILE_TYPE,	/* caption label */
	False,			/* align caption */
	type_rec,		/* gizmo array */
	XtNumber(type_rec),	/* number of gizmos */
};



static GizmoRec CustomGiz[] = {
	{LabelGizmoClass,	&pattern_label},
	{LabelGizmoClass,	&type_label},
};


/* Define the Show menu */

static MenuItems ShowMenubarItems[] = {
	MENU_ITEM( TXT_APPLY,	TXT_M_APPLY,	NULL ),
	MENU_ITEM( TXT_RESET,	TXT_M_RESET,	NULL ),
	MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
	MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
	{ NULL }
};
MENU_BAR("ShowMenubar", ShowMenubar, ShowCB, 0, 2);	/* default: Apply */

static PopupGizmo CustomWindow = {
	NULL,			/* help */
	"custom",		/* widget name */
	TXT_FILTER_TITLE,	/* title */
	&ShowMenubar,		/* menu */
	CustomGiz,		/* gizmo array */
	XtNumber(CustomGiz),	/* number of gizmos */
	NULL,			/* No footer for customize */
};

/***************************private*procedures****************************

    Private Procedures
*/


/****************************procedure*header*****************************
    AlignView-
    FLH MORE: Do we ever need to specify DM_B_CALC_SIZE for the items?
    At this point, we should have already sized the items.
    With dynamic fonts, the items for all folders are resized when
    the font changes.
*/
static void
AlignView(DmFolderWindow folder)
{

    Dimension		wrap_width;
    DmItemPtr		item;

    _DPRINT1(stderr,"AlignView: %s\n", DM_WIN_PATH(folder));

    /* they are already arranged in other views, not enough reason to
       dim 'Align' button, I guess !
    */
    if (folder->views[0].view_type == DM_LONG)
	return;

#ifdef MOTIF_WRAP_WIDTH /* FLH MORE how do we get wrap_width here? */
    OlSWGeometries	swin_geom;
    swin_geom = GetOlSWGeometries((ScrolledWindowWidget)(folder->views[0].swin));
    wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
#else
    wrap_width = 0;
#endif
    if (folder->views[0].view_type == DM_ICONIC)
    {
	Dimension quad_grid_height = GRID_HEIGHT(Desktop) / 4;

    	/* Fix for sorting by position: normalize 'y' to disambiguate */
    	for (item = folder->views[0].itp; item < folder->views[0].itp + folder->views[0].nitems; item++)
		if (ITEM_MANAGED(item))
	    		item->y = (int)(ITEM_Y(item) + quad_grid_height) /
				  (int)GRID_HEIGHT(Desktop);
	/* FLH MORE: removed DM_B_CALC_SIZE.  item sizes have 
	   already been calculated.
	 */
    	DmSortItems(folder, DM_BY_POSITION, 0 , 0, wrap_width);

	return;
    }
    if (folder->views[0].view_type == DM_NAME)
	/* FLH MORE: remove DM_B_CALC_SIZE ?? */
    	DmSortItems(folder, folder->views[0].sort_type, 0,  0, wrap_width);
    return;
}					/* End of AlignView */

/****************************procedure*header*****************************
    ByName-
*/
static int 
ByName(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(0);

    if (!ITEM_MANAGED(n1))
	return (1);

    return(DmCompareItemLabels(n1, n2));
}				/* end of ByName */

/****************************procedure*header*****************************
    ByPosition-
*/
static int
ByPosition(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(-1);

    if (!ITEM_MANAGED(n1))
	return (1);

    if (n1->y != n2->y)
	return( (int)n1->y - (int)n2->y );

    /* Items in same row: return their horiz relationship */
    return( (int)n1->x - (int)n2->x );

}				/* end of ByPosition */

/****************************procedure*header*****************************
    BySize-
*/
static int
BySize(DmItemPtr n1, DmItemPtr n2)
{

    if (!ITEM_MANAGED(n2))
	return(0);

    if (!ITEM_MANAGED(n1))
	return (1);

    return (FILEINFO_PTR(n1)->size - FILEINFO_PTR(n2)->size);

}				/* end of BySize */

/****************************procedure*header*****************************
    ByTime-
*/
static int
ByTime(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(0);

    if (!ITEM_MANAGED(n1))
	return (1);

    return (FILEINFO_PTR(n2)->mtime - FILEINFO_PTR(n1)->mtime);

}				/* end of ByTime */

/****************************procedure*header*****************************
    ByType-
*/
static int
ByType(DmItemPtr n1, DmItemPtr n2)
{
    int ret;

    if (!ITEM_MANAGED(n2))
	return(-1);

    if (!ITEM_MANAGED(n1))
	return (1);

    /* dir types gets sorted first */
    if (ret = ((int)(ITEM_OBJ(n1)->ftype) - (int)(ITEM_OBJ(n2)->ftype)))
	;
    else {
	if (ret = strcmp(((DmFnameKeyPtr)(FCLASS_PTR(n1))->key)->name,
		 ((DmFnameKeyPtr)(FCLASS_PTR(n2))->key)->name))
		;
    	else
        	ret = DmCompareItemLabels(n1, n2);
    }

    return(ret);
}				/* end of ByType */

/****************************procedure*header*****************************
    FilterView - Filter the view according to the filter info
		   stored in the folder structure.

*/
static void
FilterView(DmFolderWindow folder)
{
    int			i;
    DmItemPtr		ip;
    DmObjectPtr		objptr;
    Boolean		allset;
    Boolean		touched = False;
    CListGizmo *	clist = (CListGizmo *)QueryGizmo(PopupGizmoClass, 
						folder->customWindow,
                                                GetGizmoGizmo, "customlist");
    Gizmo		inputG = (Gizmo )QueryGizmo(PopupGizmoClass,
				folder->customWindow, GetGizmoGizmo, "pattern");

    /* Get the char * pattern entered by the user.
       It is up to dtm to free already allocated patterns.
    */
    if (folder->filter.pattern)
	FREE((void *)(folder->filter.pattern));

    folder->filter.pattern = GetInputGizmoText(inputG);

    if (folder->filter.pattern[0] == '\0')
    {
	FREE(folder->filter.pattern);
	folder->filter.pattern = strdup("*");
    }

    /* Assume at least one of the types has to be set */
    allset = True;
    for(i = 0; i < clist->cp->num_objs ; i++)
	if (ITEM_SELECT(clist->itp + i))
	{
	    folder->filter.type[i] = True;
	} else
	{
	    folder->filter.type[i] = False;
	    allset = False;
	}

    /* Filter is off if all types set AND "no pattern" set */
    folder->filter_state = !allset || strcmp(folder->filter.pattern, "*");

    for (i = 0, objptr = folder->views[0].cp->op;
	 i < folder->views[0].cp->num_objs; i++, objptr = objptr->next)
    {
	if ((objptr->name == NULL) || (objptr->attrs & DM_B_HIDDEN))
	    continue;

	if (DmObjMatchFilter(folder, objptr))
	    touched |= DmAddObjectToView(folder, 0, objptr, 
					 UNSPECIFIED_POS, UNSPECIFIED_POS);

	else if ((ip = DmObjectToItem((DmWinPtr)folder, objptr)) != NULL)
	{
	    touched = True;
	    ip->managed = False;
	    FREE_LABEL(ip->label);
	    ip->label = NULL;
	    objptr->x = 0;
	    objptr->y = 0;
	}
    }

    /* Re-sort if items "touched" regardless of whether state of filter
       has changed.  (Just re-align in iconic view?)
    */
    if (touched)
    {
	if (folder->views[0].view_type == DM_ICONIC)
	    AlignView(folder);
	else
	    /* FLH MORE: check this: size is already calculated. don't
	     * need to do it again for NAME view.  Size calculation will
	     * always be done in LONG view
	     */
	    DmSortItems(folder, folder->views[0].sort_type, 0, 0, 0);
	DmDisplayStatus((DmWinPtr)folder);
    }

    DmVaDisplayState((DmWinPtr)folder,
		     folder->filter_state ? TXT_FILTER_ON : NULL);

}					/* end of FilterView */

/****************************procedure*header*****************************
    FindFileMatch(char * objname, char * pattern)-
*/

/* 
 * FLH MORE: need to handle pattern searching for _XmStrings 
 */
static Boolean
FindFileMatch(char * objname, char * pattern)
{
        char * tmpstring;
        char * localstring;

        localstring = strdup(pattern);
        tmpstring = strtok(localstring, " \t\n");
        while (tmpstring) {
                if (gmatch(objname, tmpstring)) {
                        FREE(localstring);
                        return(True);
                }
                else{
                        tmpstring = strtok(NULL, " \t\n");
                }
        }
        FREE(localstring);
        return(False);
}

/****************************procedure*header*****************************
    ShowCB-
*/
static void
ShowCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    int			i;
    MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWindow	folder = (DmFolderWindow) cbs->clientData;
    Widget		shell = GetPopupGizmoShell(folder->customWindow);
    CListGizmo *	clist = (CListGizmo *)QueryGizmo(PopupGizmoClass, 
						folder->customWindow,
                                                GetGizmoGizmo, "customlist");

    Gizmo		inputG = (Gizmo )QueryGizmo(PopupGizmoClass,
				folder->customWindow, GetGizmoGizmo, "pattern");

    switch(cbs->index)
    {
    case DM_SHOW_CV:
	FilterView(folder);
	BringDownPopup(shell);
        break;

    case DM_RESET_CV:
	SetInputGizmoText(inputG, "");
	folder->filter.pattern = GetInputGizmoText(inputG);
	for(i = 0; i < clist->cp->num_objs ; i++) 
	{
	    XtSetArg(Dm__arg[0], XmNset, True);
	    ExmFlatSetValues(clist->boxWidget, i, Dm__arg, 1);
	}
        break;

    case DM_CANCEL_CV:
        XtPopdown( (Widget)DtGetShellOfWidget(w) );
        break;

    case DM_HELP_CV:
     DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
		FOLDER_HELPFILE, FOLDER_VIEW_FILTER_SECT);
        break;
    }
}

/***************************public*procedures*****************************

    Public Procedures
*/

/*****************************************************************************
 *      DmAddObjectToView - Create an item in view to represent object in
 * 			container.  Don't add an item if the object
 * 			is marked HIDDEN.  The flat icon box is
 * 			not notified of changes made to the items list; 
 * 			the caller must do this.
 *	INPUTS: folder
 *		object
 *		view index
 *	OUTPUTS: 0 if the item was already present (and managed)
 *		 1 if an item was added
 *	GLOBALS:
 *****************************************************************************/
int
DmAddObjectToView(DmFolderWindow folder, Cardinal view_index, DmObjectPtr obj,
		  WidePosition x, WidePosition y)
{
    DmItemPtr 	 ip;
    DmFolderView view = &folder->views[view_index];
    WidePosition 	 icon_x, icon_y;
    int		 pos;


    _DPRINT3(stderr, "DmAddObjectToView: %s:%s\n", 
	     obj->container->path, obj->name);

    if (obj->attrs & DM_B_HIDDEN){
	return(0);
    }

    /* If a managed Item with this object pointer is already in the
       item list, just reuse it.  Don't reuse an unmanaged item,
       because we may have switched view types while it was unmanaged.
       (we free the labels below when we unmanage items)
    */
    if ( (ip = DmObjectToItem((DmWinPtr)folder, obj)) != NULL )
    {
	if (ITEM_MANAGED(ip)){
	    return(0);			/* new item not added to view */
	}

    } else
    {
	Dimension icon_width;
	Dimension icon_height;

	/* No Item with this object exists so get a free item from the list. */
	(void)Dm__GetFreeItems(&view->itp,
			       &view->nitems, 1, &ip);

	ip->object_ptr	= (XtArgVal)obj;
        if (view->view_type == DM_ICONIC)
		MAKE_WRAPPED_LABEL(ip->label, FPART(view->box).font,
			DmGetObjectName(ITEM_OBJ(ip)));
	else
		MAKE_LABEL(ip->label,Dm__MakeItemLabel(ip,view->view_type,0));

	DmComputeItemSize(view->box, ip, view->view_type, &icon_width,
			  &icon_height);
	ip->icon_width = (XtArgVal)icon_width;
	ip->icon_height = (XtArgVal)icon_height;
    }

    /* Get available position if ICONIC view */
    if (view->view_type == DM_ICONIC) {
	Dimension		width;

	if ((x == UNSPECIFIED_POS) && (y == UNSPECIFIED_POS)){

	    XtSetArg(Dm__arg[0], XmNwidth, &width);
	    XtGetValues(view->box, Dm__arg, 1);
	    DmGetAvailIconPos(view->box,
			      view->itp, 
			      view->nitems,
			      ITEM_WIDTH(ip), ITEM_HEIGHT(ip),
			      width, GRID_WIDTH(Desktop), GRID_HEIGHT(Desktop),
			      &icon_x, &icon_y);

	    ip->x = (XtArgVal)icon_x;
	    ip->y = (XtArgVal)icon_y;
	} else {
	    ip->x = ((x == 0) ? (XtArgVal) 0 :
		     ((pos = x - (ITEM_WIDTH(ip)/2)) < 0) ? (XtArgVal) 0 :
		     (XtArgVal) pos);

	    ip->y = ((y == 0) ? (XtArgVal) 0 :
		     ((pos = y - (GLYPH_PTR(ip)->height/2)) < 0) ? 
		     (XtArgVal) 0 : (XtArgVal) pos);
	}
    }
    ip->managed		= True;
    ip->select		= False;
    ip->sensitive	= True;
    ip->client_data	= NULL;

    return(1);
}					/* DmAddObjectToView */


/****************************procedure*header*****************************
    DmFormatView- format the view according to 'type'.

    FLH MORE: this needs some reorganization.  It is used both
    at view initialization time (folder->view_type == -1) and when
    switching between views.  This distinction needs to be more
    clearly shown

	Assumes 'type' is not the current view type.
*/
void
DmFormatView(DmFolderWindow folder, DmViewFormatType type)
{
    Dimension		wrap_width;
    DtAttrs		layout_options;
    DmFolderView 	view = &folder->views[0];
    Widget 		icon_box = view->box;
    XmFontList		font_list = NULL;
    int 		i = 0;

    _DPRINT1(stderr, "DmFormatView: %s\n",DM_WIN_PATH(folder));

    layout_options =
	/* When changing FROM iconic, save icon positions */
	(view->view_type == DM_ICONIC) ? SAVE_ICON_POS :

    /* When changing FROM long view, restore labels */
	(view->view_type == DM_LONG) ? UPDATE_LABEL : 0;

    if (type != -1)
    	layout_options |= UPDATE_LABEL;

    /* When changing TO iconic:
	    Restore icon positions
	    Use width of icon box for wrapping (DmSortItems will get this)
       Otherwise:
	    Use the width of the view.

       Must always sort items: for non-ICONIC views, items are always
       maintained in sorted order.  For ICONIC, items with no position
       (eg, new items) must be sorted.
    */
    if (type == DM_ICONIC)
    {
	layout_options |= RESTORE_ICON_POS;
	wrap_width = 0;
    } else
    {
#ifdef MOTIF_WRAP_WIDTH /* FLH REMOVE */
	OlSWGeometries swin_geom =
	    GetOlSWGeometries((ScrolledWindowWidget)(view->swin));

	wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
#else
	wrap_width = 0;
#endif

	/* SPECIAL: when a folder is brought up initially in ICONIC view
	   and there is no .dtinfo, 'type' is passed in as '-1'.
	   */
	if (type == (DmViewFormatType)-1)
	    type = DM_ICONIC;
    }
    /* 
     *	reset the font if we are switching to or from LONG view 
     */
    if (USE_FONT_OBJ(Desktop)){
	/* Get the fonts cached in the Desktop struct.
	 * No need to retrieve the fontObject
	 */
	if (type == DM_LONG){
	    font_list = DESKTOP_MONOSPACED(Desktop).font;
	}
	else if (view->view_type == DM_LONG){
	    font_list = DESKTOP_SANS_SERIF(Desktop).font;
	}
	if (font_list){
	    XtSetArg(Dm__arg[0], XmNfontList, font_list);
	    XtSetValues(icon_box, Dm__arg, 1);
	}
    }

    /* remember view type in folder (also, DmSortItems needs this) */
    view->view_type = type;
    
    DmSortItems(folder, view->sort_type, DM_B_CALC_SIZE,
		layout_options, wrap_width);


    /* set drawProc */
    XtSetArg(Dm__arg[i], XmNdrawProc,
	     (folder->views[0].view_type == DM_LONG) ?	DmDrawLongIcon :
	     (folder->views[0].view_type == DM_NAME) ?	DmDrawNameIcon :
	     /* else */				DmDrawLinkIcon); i++;


    /* Icons are only movable in ICONIC view */
    XtSetArg(Dm__arg[i], XmNmovableIcons, 
	     folder->views[0].view_type == DM_ICONIC);i++;

    /* set gridWidth and gridHeight on the FIconBox so it
     * will update the scrollbar granularity
     */
    XtSetArg(Dm__arg[i], XmNgridHeight, (type == DM_ICONIC) ? 
	     GRID_HEIGHT(Desktop) / 2 :
	     (type == DM_LONG) ? DM_LongRowHeight(icon_box) :
	     DmFontHeight(icon_box)); i++;
    XtSetArg(Dm__arg[i], XmNgridWidth, (type == DM_LONG) ?
	     DmFontWidth(icon_box) : GRID_WIDTH(Desktop) / 2);i++;
    /* FLH MORE: we don't need to do a TouchItems; DmSortItems
     * does this for us.  Unless we are called after DmFilterView.
     */
    XtSetValues(icon_box, Dm__arg, i);
}					/* end of DmFormatView() */

/****************************procedure*header*****************************
    DmObjMatchFilter-
*/
Boolean
DmObjMatchFilter(DmFolderWindow folder, DmObjectPtr objptr)
{
    return(folder->filter.type[objptr->ftype -1] &&
	   FindFileMatch(objptr->name, folder->filter.pattern));
}

/*****************************************************************************
 * 	DmRmObjectFromView: remove an object from a view (if its in the view)
 *	INPUTS: folder
 *		view index
 *		object
 *	OUTPUTS: True if items removed/False otherwise
 *	GLOBALS:
 *****************************************************************************/
#ifdef NOT_USED
Boolean
DmRmObjectsFromView(DmFolderWindow folder, Cardinal view_index, 
		    DmObjectPtr *obj_list, Cardinal num_objects)
{
    DmItemPtr item;
    int i;
    Boolean touched = False;

    while (num_objects > 0){

	_DPRINT3(stderr,"DmRmObjectsFromView: removing %s from %s\n",
		(*obj_list)->name, DM_WIN_PATH(folder));

	if ((item =  DmObjectToItem((DmWinPtr) folder, *obj_list)) != NULL){
	    touched = True;
	    item->managed = False;
	    FREE_LABEL(ITEM_LABEL(item));
	    item->label = NULL;
	}
	obj_list++;
	num_objects--;
    }
    if (touched){
	if (folder->views[0].view_type != DM_ICONIC)
	    DmSortItems(folder, folder->views[0].sort_type, 0, 0, 0);	    
	else
	    DmTouchIconBox((DmWinPtr) folder, NULL, 0);
    }
    return(touched);
}	/* end of DmRmObjectsFromView */
#endif

/****************************procedure*header*****************************
    DmSortItems-
*/
void
DmSortItems(DmFolderWindow folder, DmViewSortType sort_type,
	    DtAttrs geom_options, DtAttrs layout_options,
	    Dimension wrap_width)
{

    _DPRINT1(stderr, "DmSortItems: %s\n", DM_WIN_PATH(folder));
    /* FLH MORE: change wrap_width to WideDimension ?*/
    /* Sort items */
    qsort(folder->views[0].itp, folder->views[0].nitems,		
	  sizeof(DmItemRec), compare_func[sort_type]);


    /* As a convenience, if 'wrap_with' is "unspecified", get the width of
       the icon box and use it.
    */
    if (wrap_width == 0)
    {
	XtSetArg(Dm__arg[0], XmNwidth, &wrap_width);
	XtGetValues(folder->views[0].box, Dm__arg, 1);
    }

    /* Recompute layout */
    DmComputeLayout(folder->views[0].box, folder->views[0].itp, folder->views[0].nitems,
		    folder->views[0].view_type, wrap_width,
		    geom_options, layout_options);

    DmTouchIconBox((DmWinPtr)folder, NULL, 0);		/* Now "touch_items" */

}				/* end of DmSortItems */

/****************************procedure*header*****************************
    DmViewAlignCB-
*/
void 
DmViewAlignCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    AlignView( (DmFolderWindow)DmGetWinPtr(w) );
}

/****************************procedure*header*****************************
    DmViewCustomizedCB-
*/
void 
DmViewCustomizedCB(Widget w, XtPointer client_data, XtPointer call_data)
{

    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    if ( folder->customWindow == NULL )
    {
	CListGizmo *	clist;
	DmItemPtr	item;

	((MenuGizmo *)(CustomWindow.menu))->clientData = (char *)folder;
	folder->customWindow = CreateGizmo(folder->shell, PopupGizmoClass, 
						&CustomWindow, NULL, 0);

	clist = (CListGizmo *)QueryGizmo(PopupGizmoClass, folder->customWindow,
						GetGizmoGizmo, "customlist");


	for (item = clist->itp; item < clist->itp + clist->cp->num_objs; item++)
	    item->select = True;

	XtSetArg(Dm__arg[0], XmNitemsTouched, True);
	XtSetArg(Dm__arg[1], XmNselectCount, clist->cp->num_objs);
	XtSetValues(clist->boxWidget, Dm__arg, 2);

	/* register context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(folder->customWindow),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_VIEW_FILTER_SECT);
    }

    MapGizmo(PopupGizmoClass, folder->customWindow);
}

/****************************procedure*header*****************************
    DmViewFormatCB-
*/
void 
DmViewFormatCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    DmViewFormatType	view_type = (DmViewFormatType) client_data;


    if (view_type == folder->views[0].view_type)
	return;

    DmFormatView(folder, view_type);
}

/****************************procedure*header*****************************
    DmViewSortCB-
*/
void 
DmViewSortCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    DmViewSortType	sort_type = (DmViewSortType)client_data;
    Dimension		wrap_width;

    _DPRINT1(stderr,"DmViewSortCB: %s\n", 
	     sort_type == DM_BY_TYPE ? "DM_BY_TYPE" :
	     sort_type == DM_BY_NAME ? "DM_BY_NAME" : 	
	     sort_type == DM_BY_SIZE ? "DM_BY_SIZE" :
	     sort_type == DM_BY_TIME ? "DM_BY_TIME" :
	     "DM_BY_POSITION");

    /* If non-iconic, items are maintained in sorted order */
    if ((folder->views[0].view_type != DM_ICONIC) && (folder->views[0].sort_type == sort_type))
      return;

    folder->views[0].sort_type = sort_type;	/* remember sort type */

#ifdef MOTIF_WRAP_WIDTH /* FLH REMOVE */
    swin_geom = GetOlSWGeometries((ScrolledWindowWidget)(folder->views[0].swin));
    wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
#else
    wrap_width = 0;
#endif
    DmSortItems(folder, sort_type, 0, 0, wrap_width);

}					/* end  of DmViewSortCB */
