/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/container.c	1.21"
#endif

/*
 * Module:	dtadmin: nfs  Graphical Administration of Network File Sharing
 * File:	container.c:   uses the flattened icon container
 *                             widget (aka. FlatList)  to display
 *			       resources from dtvfstab and dfstab
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/vfstab.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>

#include <Dt/Desktop.h>
#include <DtI.h>
#include <FIconBox.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>

#include "nfs.h"
#include "text.h"
#include "sharetab.h"
#include "local.h"

#define OFFSET  19

extern void	RemotePropertyCB();
extern void	LocalPropertyCB();
extern void	GetLocalContainerItems();
extern void	GetRemoteContainerItems();
extern Boolean	isMounted();
extern Boolean	isShared();
extern XtArgVal	GetValue();
static void	SelectProc(Widget, XtPointer, XtPointer);
static void	AdjustProc(Widget, XtPointer, XtPointer);
static int	itemSort();

#define BUF_SIZE 1024
static  XFontStruct	*font;


extern NFSWindow * nfsw;
extern Widget root;

extern void
alignIcons(Dimension Width)
{
    DmItemPtr   item;
    Dimension width;
    Cardinal nitems =  GetValue(nfsw-> iconbox, XtNnumItems);

	Position icon_x, icon_y;

    XtVaGetValues(nfsw-> iconbox, XtNwidth, &width, 0); 
    DEBUG1("WRAP WIDTH=%u\n", width);

    if (width < 30 || width > 800)
    {
	XtVaGetValues(nfsw-> baseWindow-> shell, XtNwidth, &width, 0);
	DEBUG1("WRAP WIDTH=%u\n", width);
    }

    width -= 20;		/* FIX: shouldn't be needed:  iconbox bug */
    
    qsort(nfsw-> itp, nitems, sizeof(DmItemRec), itemSort);

    for (item = nfsw-> itp; item < nfsw-> itp + nitems; item++)
	item-> x = item-> y = UNSPECIFIED_POS;

    for (item = nfsw-> itp; item < nfsw-> itp + nitems; item++)
        if (ITEM_MANAGED(item))
        {
	    DmGetAvailIconPos(nfsw->itp, nitems,
			      ITEM_WIDTH(item), ITEM_HEIGHT(item), width,
			      INC_X, INC_Y,
			      &icon_x, &icon_y);
	    item->x = (XtArgVal)icon_x;
	    item->y = (XtArgVal)icon_y;
	}
    SetValue(nfsw-> iconbox, XtNitemsTouched, True);	
    return;
}

static int
itemSort(DmItemPtr n1, DmItemPtr n2)
{
    if (!ITEM_MANAGED(n2))
	return(0);
    
    if (!ITEM_MANAGED(n1))
	return(1);

    return (strcoll((char *)(n1-> label), (char *)(n2->label)));
}

extern void
InitContainer(scroller, form)
Widget scroller, form;
{
    extern XFontStruct * _OlGetDefaultFont();
    Dimension	width;
    Cardinal    nitems;
    Arg 	arg[16];

    font   = _OlGetDefaultFont(form, NULL);
    width  = GetValue(form, XtNwidth);

    nitems = nfsw-> cp-> num_objs;

    /* Create icon container        */

    XtSetArg(arg[0], XtNpostSelectProc, (XtArgVal)SelectProc);
    XtSetArg(arg[1], XtNpostAdjustProc, (XtArgVal)AdjustProc);
    XtSetArg(arg[2], XtNdrawProc,       (XtArgVal)DmDrawIcon);
    XtSetArg(arg[3], XtNmovableIcons,   (XtArgVal)False);
    nfsw-> iconbox = DmCreateIconContainer(scroller,
					   DM_B_NO_INIT | DM_B_CALC_SIZE,
					   arg,
					   4,
					   nfsw-> cp-> op,
					   nitems,
					   &nfsw-> itp,
					   nitems,
					   NULL,
					   NULL,
					   font,
					   1);
    alignIcons(width);
}				/* InitContainer */



static void
selectRemote(struct vfstab * vp, Cardinal index)
{
    extern MenuGizmo EditMenu, ActionsMenu;
    DEBUG2("mountp=%s, label=%s\n", vp-> vfs_mountp, vp-> vfs_fsckpass);

    if (isMounted(vp-> vfs_mountp, vp-> vfs_special))
    {
	OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			  XtNsensitive, OwnerRemote, NULL);
	OlVaFlatSetValues(EditMenu.child, EditDelete,
			  XtNsensitive, OwnerRemote, NULL);
	OlVaFlatSetValues(EditMenu.child, EditProperties,
			  XtNsensitive, True, NULL);
	nfsw-> remoteSelectList-> op-> fcp = nfsw-> mounted_fcp;
	OlFlatRefreshItem(nfsw-> iconbox, index , True);
    }
    else
    {
	OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			  XtNsensitive, OwnerRemote, NULL);
	OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(EditMenu.child, EditDelete,
			  XtNsensitive, OwnerRemote, NULL);
	OlVaFlatSetValues(EditMenu.child, EditProperties,
			  XtNsensitive, True, NULL);
	nfsw-> remoteSelectList-> op-> fcp = nfsw-> remote_fcp;
	OlFlatRefreshItem(nfsw-> iconbox, index , True);
    }
    RemotePropertyCB(NULL, False, NULL);
}


static void
selectLocal(dfstab * dfsp, Cardinal index)
{
    extern MenuGizmo EditMenu, ActionsMenu;
    struct share * sharep;

    sharep = dfsp-> sharep;

    DEBUG2("path=%s, label=%s\n", sharep-> sh_path, sharep-> sh_res);

    if (isShared(sharep))
    {
	OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			  XtNsensitive, OwnerLocal, NULL);
	OlVaFlatSetValues(EditMenu.child, EditDelete,
			  XtNsensitive, OwnerLocal, NULL);
	OlVaFlatSetValues(EditMenu.child, EditProperties,
			  XtNsensitive, True, NULL);
	nfsw-> localSelectList-> op-> fcp = nfsw-> shared_fcp;
	OlFlatRefreshItem(nfsw-> iconbox, index , True);
    }
    else
    {
	OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			  XtNsensitive, OwnerLocal, NULL);
	OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(EditMenu.child, EditDelete,
			  XtNsensitive, OwnerLocal, NULL);
	OlVaFlatSetValues(EditMenu.child, EditProperties,
			  XtNsensitive, True, NULL);
	nfsw-> localSelectList-> op-> fcp = nfsw-> local_fcp;
	OlFlatRefreshItem(nfsw-> iconbox, index , True);
    }
    LocalPropertyCB(NULL, False, NULL);
}
	
static void
SelectProc(w, client_data, call_data)
Widget          w;
XtPointer       client_data;
XtPointer       call_data;
{
    extern NFSWindow * MainWindow;
    extern void FreeObjectList();
    OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;
    ObjectListPtr   slp;
   
    SetMessage(MainWindow, "", Base);
    FreeObjectList(*nfsw-> selectList);
    slp = *nfsw-> selectList = (ObjectListPtr)MALLOC(sizeof(ObjectList));
    slp-> op = OBJECT_CD(d-> item_data);
    slp-> index = d-> item_data.item_index;
    slp-> next = NULL;
    DEBUG2("SelectProc: idx=%d, cnt=%d\n",d-> item_data.item_index,d-> count);

    if (nfsw-> viewMode == ViewRemote)
	selectRemote((struct vfstab *)slp-> op-> objectdata, slp-> index);
    else
	selectLocal((dfstab *)slp-> op-> objectdata, slp-> index);

}				/* SelectProc */

	
static void
AdjustProc(w, client_data, call_data)
Widget          w;
XtPointer       client_data;
XtPointer       call_data;
{
    extern NFSWindow * MainWindow;
    extern MenuGizmo EditMenu, ActionsMenu;
    extern void FreeObjectList();
    OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;
    ObjectListPtr   slp, prev;
    DmItemPtr ip;
    DmObjectPtr op;
    DmFclassPtr fcp;
    Boolean mixedStates = False;
    int count;
   
    SetMessage(MainWindow, "", Base);

    count = (int)GetValue(nfsw-> iconbox, XtNselectCount);
    DEBUG1("count=%d\n", count);

    if (count == 0)
    {
	if (nfsw-> viewMode == ViewRemote)
	{
	    OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			      XtNsensitive, False, NULL);
	}
	else
	{
	    OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			      XtNsensitive, False, NULL);
	}
	OlVaFlatSetValues(EditMenu.child, EditDelete,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(EditMenu.child, EditProperties,
			  XtNsensitive, False, NULL);
	FreeObjectList(*nfsw-> selectList);
	*nfsw-> selectList = NULL;
    }
    else
    {
	ip = ITEM_CD(d-> item_data);
	if (ITEM_SELECT(ip) == True)
	{
	    /* add to select List */
	    if (count == 1)
		SelectProc(w, client_data, call_data);
	    else
	    {
		
		op = OBJECT_CD(d-> item_data);
		
		fcp = op-> fcp;
		mixedStates = False;

		DEBUG0("look for end of selectList\n");
		for (slp = *nfsw-> selectList; slp-> next; slp = slp-> next)
		{
		    if (slp-> op-> fcp != fcp)
			mixedStates = True;
		}
		if (slp-> op-> fcp != fcp) /* in case we didn't enter loop */
		    mixedStates = True;

		slp-> next = (ObjectListPtr)MALLOC(sizeof(ObjectList));
		slp = slp-> next;
		slp-> op = op;
		slp-> index = d-> item_data.item_index;
		slp-> next = NULL;
		DEBUG2("AdjustProc: added idx=%d, cnt=%d\n",
		       d-> item_data.item_index,d-> count); 
		OlVaFlatSetValues(EditMenu.child, EditProperties,
				  XtNsensitive, False, NULL);
		if (mixedStates == True)
		{		/* grey out buttons that don't apply to all selected items */

		    ActionsMenuItemIndex doit = ActionsConnect;
		    ActionsMenuItemIndex undoit = ActionsUnconnect ;

		    if (nfsw-> viewMode == ViewLocal)
		    {
			doit = ActionsAdvertise;
			undoit = ActionsUnadvertise;
		    }
		    OlVaFlatSetValues(ActionsMenu.child, doit,
				      XtNsensitive, False, NULL);
		    OlVaFlatSetValues(ActionsMenu.child, undoit,
				      XtNsensitive, False, NULL);
		}
	    }
	}
	else
	{
	    /* remove from select list */
	    
	    int index = d-> item_data.item_index;

            DEBUG0("remove frome selectList\n");

	    for (slp = *nfsw-> selectList, prev = NULL;
		 slp; prev = slp, slp = slp-> next)
	    {
		if ( slp-> index == index)
		{
		    if (prev == NULL) /* matched head */
			*nfsw-> selectList = slp-> next;
		    else
			prev -> next = slp-> next;
		    slp-> next = NULL;
		    FreeObjectList(slp);
		    break;
		}

	    }
	    mixedStates = False;

	    for (slp = *nfsw-> selectList, fcp = slp-> op-> fcp,
		 slp = slp-> next;
		 slp;  slp = slp-> next)
	    {
		if (slp-> op-> fcp != fcp)
		{
		    mixedStates = True;
		    break;
		}
	    }
	    if (mixedStates == False)
	    {

		if (nfsw-> viewMode == ViewLocal && 
		    OwnerLocal == True)
		{
		    if ((*nfsw-> selectList)-> op-> fcp == nfsw-> local_fcp)
			OlVaFlatSetValues(ActionsMenu.child,
					  ActionsAdvertise,
					  XtNsensitive, True, NULL);
		    else
			OlVaFlatSetValues(ActionsMenu.child,
					  ActionsUnadvertise,
					  XtNsensitive, True, NULL);
		}
		else if (nfsw-> viewMode == ViewRemote &&
			 OwnerRemote == True)
		{
		    if ((*nfsw-> selectList)-> op-> fcp == nfsw-> remote_fcp)
			OlVaFlatSetValues(ActionsMenu.child,
					  ActionsConnect,
					  XtNsensitive, True, NULL); 
		    else
			OlVaFlatSetValues(ActionsMenu.child,
					  ActionsUnconnect,
					  XtNsensitive, True, NULL); 
		}
	    }
	    if (count == 1)
	    {
		OlVaFlatSetValues(EditMenu.child, EditProperties,
				  XtNsensitive, True, NULL);
		if (nfsw-> viewMode == ViewRemote)
		    RemotePropertyCB(NULL, False, NULL);
		else
		    LocalPropertyCB(NULL, False, NULL);
	    }
	}
    }
    DEBUG2("AdjustProc: idx=%d, cnt=%d\n",d-> item_data.item_index, count);
}

extern DmObjectPtr
UpdateContainer(char * name, XtPointer vp, DmFclassPtr fcp,
		Cardinal *index, DataType dataType)
{
    extern DmObjectPtr new_object();
    DmObjectPtr op;
    DmItemPtr ip;
    Cardinal  nitems;
    int x, y;
    Dimension width;
    register i;
	
    XtVaGetValues(GetBaseWindowScroller(nfsw-> baseWindow), XtNwidth,
		  &width, 0); 
	
    x = y = UNSPECIFIED_POS;
    XtVaGetValues(nfsw->iconbox, XtNnumItems, &nitems, 0);

    if ((op=new_object(name, x, y, vp, dataType)) == (DmObjectPtr) NULL)
	NO_MEMORY_EXIT();
    op->fcp = fcp;

    *index = Dm__AddObjToIcontainer(nfsw->iconbox, &(nfsw->itp),
			       &(nitems),
			       nfsw->cp, op,
			       op->x, op->y, DM_B_NO_INIT | DM_B_CALC_SIZE,
			       NULL,
			       font,
			       width,
			       (Dimension)INC_X,
			       (Dimension)INC_Y);
    alignIcons(width);
    return op;
}				/* UpdateContainer */



extern void 
unselectAll()
{
    extern void FreeObjectList();
    static ObjectListPtr   old_slp = NULL;
    ObjectListPtr          slp;
    int                    nitems, index;

    nitems    = (int)GetValue(nfsw-> iconbox, XtNnumItems);

/**********
    int nitems, index, unset, nselected;
    DmItemPtr              ip;

    nselected = (int)GetValue(nfsw-> iconbox, XtNselectCount);
    for (index=0, unset=0, ip = nfsw->itp;
	 index < nitems && unset < nselected;
	 index++, ip++)
    {
  	    if(ITEM_SELECT(ip) == True)
	    { 
		OlVaFlatSetValues(nfsw-> iconbox, index, XtNset, False, NULL);
		unset++;
	    }
    }
************/
    SetValue(nfsw-> iconbox, XtNselectCount, 0);
    for (slp = *nfsw-> selectList; slp; slp = slp-> next)
	OlVaFlatSetValues(nfsw-> iconbox, slp-> index, XtNset, False, NULL);
    FreeObjectList(*nfsw-> selectList);
    *nfsw-> selectList = NULL;
}

extern void 
ReselectAll()
{
    extern void FreeObjectList();
    ObjectListPtr   slp, prev;
    int             nitems, index;

    nitems    = (int)GetValue(nfsw-> iconbox, XtNnumItems);
    if (*nfsw-> selectList != NULL)
    {
	for (slp = *nfsw-> selectList, prev = NULL;
	     slp; prev = slp, slp = slp-> next)
	{
	    if ((index = Dm__ObjectToIndex(nfsw-> iconbox, slp-> op)) !=
		OL_NO_ITEM) 
	    {
		OlVaFlatSetValues(nfsw-> iconbox, index, XtNset, True, NULL);
		slp-> index = index;
	    }
	    else  /* shouldn't ever hit this code, but... */
	    {
		if (prev == NULL)
		    *nfsw-> selectList = slp-> next;
		else
		    prev -> next = slp-> next;
		slp-> next = NULL;
		FreeObjectList(slp);
	    }
	}
	SetValue(nfsw-> iconbox, XtNitemsTouched, True);	
    }
}


