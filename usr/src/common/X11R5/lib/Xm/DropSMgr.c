#pragma ident	"@(#)m1.2libs:Xm/DropSMgr.c	1.2.1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED
*/ 
/*
ifndef OSF_v1_2_4
 * Motif Release 1.2.3
else
 * Motif Release 1.2.4
endif
*/
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/GadgetP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <Xm/DragC.h>
#include <Xm/DropTrans.h>
#include <Xm/DropSMgrP.h>
#include "DropSMgrI.h"
#include "TraversalI.h"
#include "DragCI.h"
#include "DragICCI.h"
#include "MessagesI.h"

#define MESSAGE1 _XmMsgDropSMgr_0001
#define MESSAGE2 _XmMsgDropSMgr_0002
#define MESSAGE3 _XmMsgDropSMgr_0003
#define MESSAGE4 _XmMsgDropSMgr_0004
#define MESSAGE5 _XmMsgDropSMgr_0005
#define MESSAGE6 _XmMsgDropSMgr_0006
#define MESSAGE7 _XmMsgDropSMgr_0007
#define MESSAGE8 _XmMsgDropSMgr_0008
#define MESSAGE9 _XmMsgDropSMgr_0009

#ifdef I18N_MSG
#include "XmMsgI.h"
#endif

  
/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInit() ;
static void ClassPartInit() ;
static void DropSiteManagerInitialize() ;
static void Destroy() ;
static Boolean SetValues() ;
static void CreateTable() ;
static void DestroyTable() ;
static void ExpandDSTable() ;
static void RegisterInfo() ;
static void UnregisterInfo() ;
static XtPointer WidgetToInfo() ;
static Boolean Coincident() ;
static Boolean IsDescendent() ;
static void DetectAncestorClippers() ;
static void DetectImpliedClipper() ;
static void DetectAllClippers() ;
static Boolean InsertClipper() ;
static void DetectAndInsertAllClippers() ;
static void RemoveClipper() ;
static void RemoveAllClippers() ;
static void DestroyDSInfo() ;
static XmDSInfo CreateShellDSInfo() ;
static XmDSInfo CreateClipperDSInfo() ;
static void InsertInfo() ;
static void RemoveInfo() ;
static Boolean IntersectWithWidgetAncestors() ;
static Boolean IntersectWithDSInfoAncestors() ;
static Boolean CalculateAncestorClip() ;
static Boolean PointInDS() ;
static XmDSInfo PointToDSInfo() ;
static void DoAnimation() ;
static void ProxyDragProc() ;
static void HandleEnter() ;
static void HandleMotion() ;
static void HandleLeave() ;
static void ProcessMotion() ;
static void ProcessDrop() ;
static void ChangeOperation() ;
static void PutDSToStream() ;
static void GetDSFromDSM() ;
static int GetTreeFromDSM() ;
static XmDSInfo GetDSFromStream() ;
static void GetNextDS() ;
static XmDSInfo ReadTree() ;
static void FreeDSTree() ;
static void ChangeRoot() ;
static int CountDropSites() ;
static void CreateInfo() ;
static void CopyVariantIntoFull() ;
static void RetrieveInfo() ;
static void CopyFullIntoVariant() ;
static void UpdateInfo() ;
static void StartUpdate() ;
static void EndUpdate() ;
static void DestroyInfo() ;
static void SyncDropSiteGeometry() ;
static void SyncTree() ;
static void Update() ;
static Boolean HasDropSiteDescendant() ;
static void DestroyCallback() ;

#else

static void ClassInit( void ) ;
static void ClassPartInit( 
                        WidgetClass wc) ;
static void DropSiteManagerInitialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget w) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void CreateTable( 
                        XmDropSiteManagerObject dsm) ;
static void DestroyTable( 
                        XmDropSiteManagerObject dsm) ;
static void ExpandDSTable( 
                        register DSTable tab) ;
static void RegisterInfo( 
                        register XmDropSiteManagerObject dsm,
                        register Widget widget,
                        register XtPointer info) ;
static void UnregisterInfo( 
                        register XmDropSiteManagerObject dsm,
                        register XtPointer info) ;
static XtPointer WidgetToInfo( 
                        register XmDropSiteManagerObject dsm,
                        register Widget widget) ;
static Boolean Coincident( 
                        XmDropSiteManagerObject dsm,
                        Widget w,
                        XmDSClipRect *r) ;
static Boolean IsDescendent( 
                        Widget parentW,
                        Widget childW) ;
static void DetectAncestorClippers( 
                        XmDropSiteManagerObject dsm,
                        Widget w,
                        XmDSClipRect *r,
                        XmDSInfo info) ;
static void DetectImpliedClipper( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo info) ;
static void DetectAllClippers( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo parentInfo) ;
static Boolean InsertClipper( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo parentInfo,
                        XmDSInfo clipper) ;
static void DetectAndInsertAllClippers( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo root) ;
static void RemoveClipper( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo clipper) ;
static void RemoveAllClippers( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo parentInfo) ;
static void DestroyDSInfo( 
                        XmDSInfo info,
#if NeedWidePrototypes
                        int substructures) ;
#else
                        Boolean substructures) ;
#endif /* NeedWidePrototypes */
static XmDSInfo CreateShellDSInfo( 
                        XmDropSiteManagerObject dsm,
                        Widget widget) ;
static XmDSInfo CreateClipperDSInfo( 
                        XmDropSiteManagerObject dsm,
                        Widget clipW) ;
static void InsertInfo( 
                        XmDropSiteManagerObject dsm,
                        XtPointer info,
                        XtPointer call_data) ;
static void RemoveInfo( 
                        XmDropSiteManagerObject dsm,
                        XtPointer info) ;
static Boolean IntersectWithWidgetAncestors( 
                        Widget w,
                        XmRegion r) ;
static Boolean IntersectWithDSInfoAncestors( 
                        XmDSInfo parent,
                        XmRegion r) ;
static Boolean CalculateAncestorClip( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo info,
                        XmRegion r) ;
static Boolean PointInDS( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo info,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
static XmDSInfo PointToDSInfo( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo info,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
static void DoAnimation( 
                        XmDropSiteManagerObject dsm,
                        XmDragMotionClientData motionData,
                        XtPointer callback) ;
static void ProxyDragProc( 
                        XmDropSiteManagerObject dsm,
                        XtPointer client_data,
                        XmDragProcCallbackStruct *callback) ;
static void HandleEnter( 
                        XmDropSiteManagerObject dsm,
                        XmDragMotionClientData motionData,
                        XmDragMotionCallbackStruct *callback,
                        XmDSInfo info,
#if NeedWidePrototypes
                        unsigned int style) ;
#else
                        unsigned char style) ;
#endif /* NeedWidePrototypes */
static void HandleMotion( 
                        XmDropSiteManagerObject dsm,
                        XmDragMotionClientData motionData,
                        XmDragMotionCallbackStruct *callback,
                        XmDSInfo info,
#if NeedWidePrototypes
                        unsigned int style) ;
#else
                        unsigned char style) ;
#endif /* NeedWidePrototypes */
static void HandleLeave( 
                        XmDropSiteManagerObject dsm,
                        XmDragMotionClientData motionData,
                        XmDragMotionCallbackStruct *callback,
                        XmDSInfo info,
#if NeedWidePrototypes
                        unsigned int style,
                        int enterPending) ;
#else
                        unsigned char style,
                        Boolean enterPending) ;
#endif /* NeedWidePrototypes */
static void ProcessMotion( 
                        XmDropSiteManagerObject dsm,
                        XtPointer clientData,
                        XtPointer calldata) ;
static void ProcessDrop( 
                        XmDropSiteManagerObject dsm,
                        XtPointer clientData,
                        XtPointer cb) ;
static void ChangeOperation( 
                        XmDropSiteManagerObject dsm,
                        XtPointer clientData,
                        XtPointer calldata) ;
static void PutDSToStream( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo dsInfo,
#if NeedWidePrototypes
                        int last,
#else
                        Boolean last,
#endif /* NeedWidePrototypes */
                        XtPointer dataPtr) ;
static void GetDSFromDSM( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo parentInfo,
#if NeedWidePrototypes
                        int last,
#else
                        Boolean last,
#endif /* NeedWidePrototypes */
                        XtPointer dataPtr) ;
static int GetTreeFromDSM( 
                        XmDropSiteManagerObject dsm,
                        Widget shell,
                        XtPointer dataPtr) ;
static XmDSInfo GetDSFromStream( 
                        XmDropSiteManagerObject dsm,
                        XtPointer dataPtr,
                        Boolean *close,
                        unsigned char *type) ;
static void GetNextDS( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo parentInfo,
                        XtPointer dataPtr) ;
static XmDSInfo ReadTree( 
                        XmDropSiteManagerObject dsm,
                        XtPointer dataPtr) ;
static void FreeDSTree( 
                        XmDSInfo tree) ;
static void ChangeRoot( 
                        XmDropSiteManagerObject dsm,
                        XtPointer clientData,
                        XtPointer callData) ;
static int CountDropSites( 
                        XmDSInfo info) ;
static void CreateInfo( 
                        XmDropSiteManagerObject dsm,
                        Widget widget,
                        ArgList args,
                        Cardinal argCount) ;
static void CopyVariantIntoFull( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo variant,
                        XmDSFullInfo full_info) ;
static void RetrieveInfo( 
                        XmDropSiteManagerObject dsm,
                        Widget widget,
                        ArgList args,
                        Cardinal argCount) ;
static void CopyFullIntoVariant( 
                        XmDSFullInfo full_info,
                        XmDSInfo variant) ;
static void UpdateInfo( 
                        XmDropSiteManagerObject dsm,
                        Widget widget,
                        ArgList args,
                        Cardinal argCount) ;
static void StartUpdate( 
                        XmDropSiteManagerObject dsm,
                        Widget refWidget) ;
static void EndUpdate( 
                        XmDropSiteManagerObject dsm,
                        Widget refWidget) ;
static void DestroyInfo( 
                        XmDropSiteManagerObject dsm,
                        Widget widget) ;
static void SyncDropSiteGeometry( 
                        XmDropSiteManagerObject dsm,
                        XmDSInfo info) ;
static void SyncTree( 
                        XmDropSiteManagerObject dsm,
                        Widget shell) ;
static void Update( 
                        XmDropSiteManagerObject dsm,
                        XtPointer clientData,
                        XtPointer callData) ;
static Boolean HasDropSiteDescendant( 
                        XmDropSiteManagerObject dsm,
                        Widget widget) ;
static void DestroyCallback( 
                        Widget widget,
                        XtPointer client_data,
                        XtPointer call_data) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/
  
static XtResource resources[] = {
{ XmNnotifyProc, XmCNotifyProc, XmRCallbackProc,
  sizeof(XtCallbackProc),
  XtOffsetOf( struct _XmDropSiteManagerRec, dropManager.notifyProc),
  XmRImmediate, NULL },
{ XmNtreeUpdateProc, XmCTreeUpdateProc, XmRCallbackProc,
  sizeof(XtCallbackProc),
  XtOffsetOf( struct _XmDropSiteManagerRec, dropManager.treeUpdateProc),
  XmRImmediate, NULL },
{ XmNclientData, XmCClientData, XmRPointer,
  sizeof(XtPointer),
  XtOffsetOf( struct _XmDropSiteManagerRec, dropManager.client_data),
  XmRImmediate, NULL },
};

/*  class record definition  */

externaldef(xmdropsitemanagerclassrec) 
    XmDropSiteManagerClassRec xmDropSiteManagerClassRec = 
{
	{
		(WidgetClass) &objectClassRec,    /* superclass	         */	
		"XmDropSiteManager",              /* class_name	         */	
		sizeof(XmDropSiteManagerRec),     /* widget_size	         */	
		ClassInit, 			  /* class_initialize      */
		ClassPartInit,                    /* class part initialize */
		False,                            /* class_inited          */	
		DropSiteManagerInitialize,        /* initialize	         */	
		NULL,                             /* initialize_hook       */
		NULL,                             /* obj1                  */	
		NULL,							  /* obj2                  */	
		0,								  /* obj3	                 */	
		resources,                        /* resources	         */	
		XtNumber(resources),              /* num_resources         */	
		NULLQUARK,                        /* xrm_class	         */	
		True,                             /* obj4                  */
		True,                             /* obj5                  */	
		True,                             /* obj6                  */
		False,                            /* obj7                  */
		Destroy,                          /* destroy               */	
		NULL,                             /* obj8                  */	
		NULL,				              /* obj9                  */	
		SetValues,                        /* set_values	         */	
		NULL,                             /* set_values_hook       */
		NULL,                             /* obj10                 */
		NULL,                             /* get_values_hook       */
		NULL,                             /* obj11    	         */	
		XtVersion,                        /* version               */
		NULL,                             /* callback private      */
		NULL,                             /* obj12                 */
		NULL,                             /* obj13                 */
		NULL,				              /* obj14                 */
		NULL,                             /* extension             */
	},
	{
		CreateInfo,          /* createInfo           */
		DestroyInfo,         /* destroyInfo          */
		StartUpdate,         /* startUpdate          */
		RetrieveInfo,        /* retrieveInfo         */
		UpdateInfo,          /* updateInfo           */
		EndUpdate,           /* endUpdate            */

		Update,              /* updateDSM            */

		ProcessMotion,       /* processMotion        */
		ProcessDrop,         /* processDrop          */
		ChangeOperation,     /* operationChanged     */
		ChangeRoot,          /* changeDSRoot         */

		InsertInfo,          /* insertInfo           */
		RemoveInfo,          /* removeInfo           */

		SyncTree,            /* syncTree             */
		GetTreeFromDSM,      /* getTreeFromDSM       */

		CreateTable,         /* createTable          */
		DestroyTable,        /* destroyTable         */
		RegisterInfo,        /* registerInfo         */
		WidgetToInfo,        /* widgetToInfo         */
		UnregisterInfo,      /* unregisterInfo       */

		NULL,                /* extension            */
	},
};

externaldef(xmdropsitemanagerobjectclass) WidgetClass
	xmDropSiteManagerObjectClass = (WidgetClass)
		&xmDropSiteManagerClassRec;

static void 
#ifdef _NO_PROTO
ClassInit()
#else
ClassInit( void )
#endif /* _NO_PROTO */
{
    _XmRegisterPixmapConverters();
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ClassPartInit( wc )
		WidgetClass wc ;
#else
ClassPartInit(
		WidgetClass wc )
#endif /* _NO_PROTO */
{
	/*EMPTY*/
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DropSiteManagerInitialize( rw, nw, args, num_args )
		Widget rw ;
		Widget nw ;
		ArgList args ;
		Cardinal *num_args ;
#else
DropSiteManagerInitialize(
		Widget rw,
		Widget nw,
		ArgList args,
		Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject 	dsm = (XmDropSiteManagerObject)nw;
	XmDSFullInfoRec info_rec;
	XmDSFullInfo info = &(info_rec);

	dsm->dropManager.dragUnderData = NULL;
	dsm->dropManager.curInfo = NULL;
	dsm->dropManager.curTime = 0;
	dsm->dropManager.oldX = dsm->dropManager.curX = 0;
	dsm->dropManager.oldY = dsm->dropManager.curY = 0;
	dsm->dropManager.curDropSiteStatus = XmINVALID_DROP_SITE;
	dsm->dropManager.curDragContext = NULL;
	dsm->dropManager.curAnimate = True;
	dsm->dropManager.curOperations = XmDROP_NOOP;
	dsm->dropManager.curOperation = XmDROP_NOOP;
	dsm->dropManager.curAncestorClipRegion = _XmRegionCreate();
	dsm->dropManager.newAncestorClipRegion = _XmRegionCreate();
	DSMCreateTable(dsm);
	dsm->dropManager.dsRoot = NULL;
	dsm->dropManager.rootX = dsm->dropManager.rootY = 0;
	dsm->dropManager.rootW = dsm->dropManager.rootH = ~0;
	dsm->dropManager.clipperList = NULL;
	dsm->dropManager.updateInfo = NULL;

	/* Patch around broken Xt interfaces */
	XtGetSubresources(nw, info, NULL, NULL, _XmDSResources,
		_XmNumDSResources, NULL, 0);
}

static void 
#ifdef _NO_PROTO
Destroy( w )
		Widget w ;
#else
Destroy(
		Widget w )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject	dsm = (XmDropSiteManagerObject)w;

	DSMDestroyTable(dsm);
	_XmRegionDestroy(dsm->dropManager.curAncestorClipRegion);
	_XmRegionDestroy(dsm->dropManager.newAncestorClipRegion);
}

/*ARGSUSED*/
static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args, num_args )
		Widget cw ;
		Widget rw ;
		Widget nw ;
		ArgList args ;
		Cardinal *num_args ;
#else
SetValues(
		Widget cw,
		Widget rw,
		Widget nw,
		ArgList args,
		Cardinal *num_args )
#endif /* _NO_PROTO */
{
	/*EMPTY*/
	return False;
}

static void 
#ifdef _NO_PROTO
CreateTable( dsm )
		XmDropSiteManagerObject dsm ;
#else
CreateTable(
		XmDropSiteManagerObject dsm )
#endif /* _NO_PROTO */
{
	register DSTable * tab = (DSTable *) &(dsm->dropManager.dsTable);

	*tab = (DSTable) XtCalloc(1, sizeof(DSTableRec));

    (*tab)->mask = 0x7f;
    (*tab)->rehash = (*tab)->mask - 2;
    (*tab)->entries = (XtPointer *) XtCalloc(((*tab)->mask + 1),
		sizeof(XmDSInfo));
    (*tab)->occupied = 0;
    (*tab)->fakes = 0;
}

static void 
#ifdef _NO_PROTO
DestroyTable( dsm )
        XmDropSiteManagerObject dsm ;
#else
DestroyTable(
        XmDropSiteManagerObject dsm )
#endif /* _NO_PROTO */
{
    register DSTable * tab = (DSTable *) &(dsm->dropManager.dsTable);

    XtFree((char *)(*tab)->entries);
    XtFree((char *)*tab);
    tab = NULL;
}

static char DSfake;	/* placeholder for deletions */

#ifndef OSF_v1_2_4
#define DSHASH(tab,widget) ((int)(((int)widget) & tab->mask))
#else /* OSF_v1_2_4 */
#define DSHASH(tab,widget) ((int)(((int)(long)widget) & tab->mask))
#endif /* OSF_v1_2_4 */
#ifndef OSF_v1_2_4
#define DSREHASHVAL(tab,widget) \
	((int)(((((int)widget) % tab->rehash) + 2) | 1))
#else /* OSF_v1_2_4 */
#define DSREHASHVAL(tab,widget) \
	((int)(((((int)(long)widget) % tab->rehash) + 2) | 1))
#endif /* OSF_v1_2_4 */
#define DSREHASH(tab,idx,rehash) ((idx + rehash) & tab->mask)
#define DSTABLE(dsm) ((DSTable)(dsm->dropManager.dsTable))

static void 
#ifdef _NO_PROTO
ExpandDSTable( tab )
        register DSTable tab ;
#else
ExpandDSTable(
        register DSTable tab )
#endif /* _NO_PROTO */
{
    unsigned int oldmask;
    register XtPointer *oldentries, *entries;
    register int oldidx, newidx, rehash;
    register XtPointer entry;
    
    oldmask = tab->mask;
    oldentries = tab->entries;
    tab->occupied -= tab->fakes;
    tab->fakes = 0;
    if ((tab->occupied + (tab->occupied >> 2)) > tab->mask)
	{
		tab->mask = (tab->mask << 1) + 1;
		tab->rehash = tab->mask - 2;
    }
    entries = tab->entries = (XtPointer *) 
		XtCalloc(tab->mask+1, sizeof(XtPointer));
    for (oldidx = 0; oldidx <= oldmask; oldidx++)
	{
		if ((entry = oldentries[oldidx]) && entry != &DSfake)
		{
			newidx = DSHASH(tab, GetDSWidget(entry));
			if (entries[newidx])
			{
				rehash = DSREHASHVAL(tab, GetDSWidget(entry));
				do {
					newidx = DSREHASH(tab, newidx, rehash);
				} while (entries[newidx]);
			}
			entries[newidx] = entry;
		}
	}
    XtFree((char *)oldentries);
}

static void 
#ifdef _NO_PROTO
RegisterInfo( dsm, widget, info )
        register XmDropSiteManagerObject dsm ;
        register Widget widget ;
        register XtPointer info ;
#else
RegisterInfo(
        register XmDropSiteManagerObject dsm,
        register Widget widget,
        register XtPointer info )
#endif /* _NO_PROTO */
{
    register DSTable tab;
    register int idx, rehash;
    register XtPointer entry;
    
	if (GetDSRegistered(info))
		return;

    tab = DSTABLE(dsm);
    
    if ((tab->occupied + (tab->occupied >> 2)) > tab->mask)
      ExpandDSTable(tab);
    
    idx = DSHASH(tab, widget);
    if ((entry = tab->entries[idx]) && entry != &DSfake)
	{
		rehash = DSREHASHVAL(tab, widget);
		do {
			idx = DSREHASH(tab, idx, rehash);
		} while ((entry = tab->entries[idx]) && (entry != &DSfake));
    }
    if (!entry)
      tab->occupied++;
    else if (entry == &DSfake)
      tab->fakes--;

    tab->entries[idx] = info;

	SetDSRegistered(info, True);
}

static void 
#ifdef _NO_PROTO
UnregisterInfo( dsm, info )
        register XmDropSiteManagerObject dsm ;
        register XtPointer info ;
#else
UnregisterInfo(
        register XmDropSiteManagerObject dsm,
        register XtPointer info )
#endif /* _NO_PROTO */
{ 
    register DSTable tab;
    register int idx, rehash;
    register XtPointer entry;
    Widget widget = GetDSWidget(info);
    
    if ((info == NULL) || !GetDSRegistered(info))
      return;

    tab = DSTABLE(dsm);
    idx = DSHASH(tab, widget);
    if ((entry = tab->entries[idx]) != NULL)
	{
		if (entry != info)
		{
			rehash = DSREHASHVAL(tab, widget);
			do {
				idx = DSREHASH(tab, idx, rehash);
				if (!(entry = tab->entries[idx]))
				  return;
			} while (entry != info);
		}
		tab->entries[idx] = (XtPointer) &DSfake;
		tab->fakes++;
    }

    SetDSRegistered(info, False);
}

static XtPointer 
#ifdef _NO_PROTO
WidgetToInfo( dsm, widget )
        register XmDropSiteManagerObject dsm ;
        register Widget widget ;
#else
WidgetToInfo(
        register XmDropSiteManagerObject dsm,
        register Widget widget )
#endif /* _NO_PROTO */
{
    register DSTable tab;
    register int idx, rehash;
    register XmDSInfo entry;
    
    tab = DSTABLE(dsm);
    idx = DSHASH(tab, widget);
    if ((entry = (XmDSInfo) tab->entries[idx]) &&
		((entry == (XmDSInfo) &DSfake) ||
			(GetDSWidget(entry) != widget)))
    {
		rehash = DSREHASHVAL(tab, widget);
		do {
			idx = DSREHASH(tab, idx, rehash);
		} while ((entry = (XmDSInfo) tab->entries[idx]) &&
			((entry == (XmDSInfo) &DSfake) ||
				(GetDSWidget(entry) != widget)));
    }

    if (entry)
      return((XtPointer) entry);
    else
		return NULL;
}

static Boolean 
#ifdef _NO_PROTO
Coincident( dsm, w, r )
        XmDropSiteManagerObject dsm ;
        Widget w ;
        XmDSClipRect *r ;
#else
Coincident(
        XmDropSiteManagerObject dsm,
        Widget w,
        XmDSClipRect *r )
#endif /* _NO_PROTO */
{
	XRectangle wR;
	Boolean hit = False;

	if (!XtIsShell(w))
	{
		/* r is shell relative, so wR needs to be translated */
		XtTranslateCoords(XtParent(w), XtX(w), XtY(w),
			&(wR.x), &(wR.y));
		wR.x -= dsm->dropManager.rootX;
		wR.y -= dsm->dropManager.rootY;
	}
	else
	{
		wR.x = wR.y = 0;
	}
	

	wR.width = XtWidth(w);
	wR.height = XtHeight(w);

	if ( !(r->detected & XmDROP_SITE_LEFT_EDGE) && (r->x == wR.x))
	{
		r->detected |= XmDROP_SITE_LEFT_EDGE;
		hit = True;
	}

	if ( !(r->detected & XmDROP_SITE_RIGHT_EDGE) &&
		((r->x + r->width) == (wR.x + wR.width)))
	{
		r->detected |= XmDROP_SITE_RIGHT_EDGE;
		hit = True;
	}

	if ( !(r->detected & XmDROP_SITE_TOP_EDGE) && (r->y == wR.y))
	{
		r->detected |= XmDROP_SITE_TOP_EDGE;
		hit = True;
	}

	if ( !(r->detected & XmDROP_SITE_BOTTOM_EDGE) &&
		((r->y + r->height) == (wR.y + wR.height)))
	{
		r->detected |= XmDROP_SITE_BOTTOM_EDGE;
		hit = True;
	}

	return(hit);
}

static Boolean 
#ifdef _NO_PROTO
IsDescendent( parentW, childW )
        Widget parentW ;
        Widget childW ;
#else
IsDescendent(
        Widget parentW,
        Widget childW )
#endif /* _NO_PROTO */
{
	Widget tmp = XtParent(childW);

	if ((parentW == NULL) || (childW == NULL))
		return(False);
	
	while (tmp != parentW)
	{
		if (XtIsShell(tmp))
			return(False);
		
		tmp = XtParent(tmp);
	}

	return(True);
}

static void 
#ifdef _NO_PROTO
DetectAncestorClippers( dsm, w, r, info )
        XmDropSiteManagerObject dsm ;
        Widget w ;
        XmDSClipRect *r ;
        XmDSInfo info ;
#else
DetectAncestorClippers(
        XmDropSiteManagerObject dsm,
        Widget w,
        XmDSClipRect *r,
        XmDSInfo info )
#endif /* _NO_PROTO */
{
	/* 
	 * We know that r represents the visible region of the dropSite
	 * as clipped by its ancestors in shell relative coordinates.  We
	 * now search for the most ancient ancestor who provides that clip.
	 * We do this by looking from the shell downward for the parent
	 * whose edge is coincident with a clipped edge.
	 *
	 * We can add as many as four clippers to the tree as a result
	 * of this routine.
	 */


	/*
	 * Hygiene.
	 */
	if (w == NULL)
		return;

	if (!XtIsShell(w))
		DetectAncestorClippers(dsm, XtParent(w), r, info);

	/*
	 * We never need to add the shell to the tree as a clipper.
	 * We call Coincident first so that any clipping provided by
	 * the shell is marked in the cliprect structure.
	 */
	if ((Coincident(dsm, w, r)) && (!XtIsShell(w)))
	{
		XmDSInfo clipper;

		/* Have we already put this clipper in the tree? */
		if ((clipper = (XmDSInfo) DSMWidgetToInfo(dsm, w)) != NULL)
			return;

		clipper = CreateClipperDSInfo(dsm, w);
		DSMRegisterInfo(dsm, w, (XtPointer) clipper);
		SetDSParent(clipper, dsm->dropManager.clipperList);
		dsm->dropManager.clipperList = (XtPointer) clipper;
	}
}

static void 
#ifdef _NO_PROTO
DetectImpliedClipper( dsm, info )
        XmDropSiteManagerObject dsm ;
        XmDSInfo info ;
#else
DetectImpliedClipper(
        XmDropSiteManagerObject dsm,
        XmDSInfo info )
#endif /* _NO_PROTO */
{
	static XmRegion tmpRegion = NULL;

	if (tmpRegion == NULL)
	{
		tmpRegion = _XmRegionCreate();
	}

	if ((GetDSType(info) == XmDROP_SITE_SIMPLE) && GetDSHasRegion(info))
	{
		Widget w = GetDSWidget(info);
		XRectangle wr, tr, rr;

		/*
		 * This step only has meaning if there is a separately
		 * specified region for this drop site (which can only be done
		 * for simple drop sites).
		 */
		
		/* The region will be relative to the origin of the widget */
		wr.x = wr.y = 0;
		wr.width = XtWidth(w);
		wr.height = XtHeight(w);

		_XmRegionGetExtents(GetDSRegion(info), &rr);

		_XmIntersectionOf(&wr, &rr, &tr);

		if ((rr.x != tr.x) ||
			(rr.y != tr.y) ||
			(rr.width != tr.width) ||
			(rr.height != tr.height))
		{
			XmDSInfo clipper;
			/*
			 * The implied clipper is magic.  It's in the tree but not
			 * of it.  (It refers to the same widget, but it's not
			 * registered.)
			 */

			clipper = CreateClipperDSInfo(dsm, w);
			SetDSParent(clipper, dsm->dropManager.clipperList);
			dsm->dropManager.clipperList = (XtPointer) clipper;
		}
	}
}

static void 
#ifdef _NO_PROTO
DetectAllClippers( dsm, parentInfo )
        XmDropSiteManagerObject dsm ;
        XmDSInfo parentInfo ;
#else
DetectAllClippers(
        XmDropSiteManagerObject dsm,
        XmDSInfo parentInfo )
#endif /* _NO_PROTO */
{
	XmDSInfo childInfo;
	XmDSClipRect extents, clippedExtents;
	int i;
	Widget w;
	static XmRegion tmpR = NULL;

	if (GetDSLeaf(parentInfo))
		return;
	
	if (tmpR == NULL)
	{
		tmpR = _XmRegionCreate();
	}

	for (i = 0; i < GetDSNumChildren(parentInfo); i++)
	{
		childInfo = (XmDSInfo) GetDSChild(parentInfo, i);
		/*
		 * Because we don't allow composite drop sites to have
		 * arbitrary regions, and Motif doesn't support shaped
		 * widgets, we do a simple rectangle clip detection.
		 *
		 * IntersectWithAncestors expects the region to be in
		 * widget relative coordinates, and returns in shell relative
		 * coordinates (the ultimate ancestor is the shell).
		 */
		_XmRegionGetExtents(GetDSRegion(childInfo),
			(XRectangle *)(&extents));
		
		_XmRegionUnion(GetDSRegion(childInfo), GetDSRegion(childInfo),
			tmpR);

		w = GetDSWidget(childInfo);

		IntersectWithWidgetAncestors(w, tmpR);

		_XmRegionGetExtents(tmpR, (XRectangle *)(&clippedExtents));

		/* tmpR is now in shell relative position */

		clippedExtents.detected = 0;

		if ((clippedExtents.width < extents.width) ||
			(clippedExtents.height < extents.height))
		{
			/*
			 * We've been clipped.  Find out who did it and add
			 * them to the tree.
			 */
			DetectAncestorClippers(dsm,
				XtParent(GetDSWidget(childInfo)),
				&clippedExtents, childInfo);
		}

		/*
		 * We now have inserted clippers for any ancestors which may
		 * have clipped the widget.  Now we need to check for the
		 * case that the widget itself clips the region.
		 */
		DetectImpliedClipper(dsm, childInfo);

		/* Re-Curse */
		DetectAllClippers(dsm, childInfo);
	}
}

static Boolean 
#ifdef _NO_PROTO
InsertClipper( dsm, parentInfo, clipper )
        XmDropSiteManagerObject dsm ;
        XmDSInfo parentInfo ;
		XmDSInfo clipper ;
#else
InsertClipper(
        XmDropSiteManagerObject dsm,
        XmDSInfo parentInfo,
		XmDSInfo clipper )
#endif /* _NO_PROTO */
{
	int i;
	XmDSInfo childInfo;

	/*
	 * Do a tail-end recursion which will insert the clipper into 
	 * the info tree as a child of its closest ancestor in the tree.
	 */

	if (GetDSLeaf(parentInfo))
		return(False);

	for (i=0; i < GetDSNumChildren(parentInfo); i++)
	{
		childInfo = (XmDSInfo) GetDSChild(parentInfo, i);
		if (InsertClipper(dsm, childInfo, clipper))
			return(True);
	}

	if (IsDescendent(GetDSWidget(parentInfo), GetDSWidget(clipper)))
	{
		i = 0;
		while (i < GetDSNumChildren(parentInfo))
		{
			childInfo = (XmDSInfo) GetDSChild(parentInfo, i);
			if (IsDescendent(GetDSWidget(clipper),
				GetDSWidget(childInfo)))
			{
				RemoveDSChild(parentInfo, childInfo);
				AddDSChild(clipper, childInfo,
					GetDSNumChildren(clipper));
			}
			else
				/*
				 * Because RemoveDSChild monkeys with the num children,
				 * we only increment i if we haven't called
				 * RemoveDSChild.
				 */
				i++;
			DSMRegisterInfo(dsm, GetDSWidget(childInfo),
					childInfo);
		}
		
		AddDSChild(parentInfo, clipper, GetDSNumChildren(parentInfo));

		/* We have inserted the clipper into the tree */
		return(True);
	}
	else
		return(False);
}

static void 
#ifdef _NO_PROTO
DetectAndInsertAllClippers( dsm, root )
        XmDropSiteManagerObject dsm ;
        XmDSInfo root ;
#else
DetectAndInsertAllClippers(
        XmDropSiteManagerObject dsm,
        XmDSInfo root )
#endif /* _NO_PROTO */
{
	XmDSInfo clipper;

	if ((!GetDSShell(root)) || (GetDSRemote(root)))
		return;

	DetectAllClippers(dsm, root);

	while ((clipper = (XmDSInfo) dsm->dropManager.clipperList) != NULL)
	{
		dsm->dropManager.clipperList = GetDSParent(clipper);
		(void) InsertClipper(dsm, root, clipper);
	}
}

static void 
#ifdef _NO_PROTO
RemoveClipper( dsm, clipper)
        XmDropSiteManagerObject dsm ;
        XmDSInfo clipper ;
#else
RemoveClipper(
        XmDropSiteManagerObject dsm,
        XmDSInfo clipper )
#endif /* _NO_PROTO */
{
	XmDSInfo parentInfo = (XmDSInfo) GetDSParent(clipper);
	int i;

	/* Remove the clipper from its parent */
	RemoveDSChild(parentInfo, clipper);

	/*
	 * Pull all of the children up into the parent's child
	 * list between the clipper and the clipper's sibling.
	 */
	for (i = 0; i < GetDSNumChildren(clipper); i++)
		AddDSChild(parentInfo, (XmDSInfo) GetDSChild(clipper, i),
			GetDSNumChildren(parentInfo));

	/*
	 * Destroy the clipper
	 */
	DSMUnregisterInfo(dsm, clipper);
	DestroyDSInfo(clipper, True);
}

static void 
#ifdef _NO_PROTO
RemoveAllClippers( dsm, parentInfo )
        XmDropSiteManagerObject dsm ;
        XmDSInfo parentInfo ;
#else
RemoveAllClippers(
        XmDropSiteManagerObject dsm,
        XmDSInfo parentInfo )
#endif /* _NO_PROTO */
{
#ifndef OSF_v1_2_4
	XmDSInfo child;
	int i;
#else /* OSF_v1_2_4 */
  XmDSInfo child;
  int i;
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
	if (!GetDSLeaf(parentInfo))
	{
		for (i = 0; i < GetDSNumChildren(parentInfo); i++)
		{
			child = (XmDSInfo) GetDSChild(parentInfo, i);
			RemoveAllClippers(dsm, child);
			if (GetDSInternal(child))
				RemoveClipper(dsm, child);
		}
	}
#else /* OSF_v1_2_4 */
  if (!GetDSLeaf(parentInfo))
    {
      i = 0;
      while(i < GetDSNumChildren(parentInfo)) {
	child = (XmDSInfo) GetDSChild(parentInfo, i);
	RemoveAllClippers(dsm, child);
	if (GetDSInternal(child))
	  RemoveClipper(dsm, child);
	/* Only increment i if the current child wasn't
	   removed.  Otherwise we'll skip items in the list
	   unintentionally */
	if (child == (XmDSInfo) GetDSChild(parentInfo, i)) i++;
      }
    }
#endif /* OSF_v1_2_4 */
}

static void 
#ifdef _NO_PROTO
DestroyDSInfo( info, substructures )
        XmDSInfo info ;
        Boolean substructures ;
#else
DestroyDSInfo(
        XmDSInfo info,
#if NeedWidePrototypes
                        int substructures )
#else
                        Boolean substructures )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	DestroyDS(info, substructures);
}

static XmDSInfo 
#ifdef _NO_PROTO
CreateShellDSInfo( dsm, widget )
        XmDropSiteManagerObject dsm ;
        Widget widget ;
#else
CreateShellDSInfo(
        XmDropSiteManagerObject dsm,
        Widget widget )
#endif /* _NO_PROTO */
{
	XmDSInfo		info;
	XmRegion region = _XmRegionCreate();
	XRectangle rect;

	info = (XmDSInfo) XtCalloc(1, sizeof(XmDSLocalNoneNodeRec));

	SetDSLeaf(info, True);
	SetDSShell(info, True);
	SetDSAnimationStyle(info, XmDRAG_UNDER_NONE);
	SetDSType(info, XmDROP_SITE_COMPOSITE);
	SetDSInternal(info, True);
	SetDSActivity(info, XmDROP_SITE_INACTIVE);
	SetDSWidget(info, widget);

	rect.x = rect.y = 0;
	rect.width = XtWidth(widget);
	rect.height = XtHeight(widget);
	_XmRegionUnionRectWithRegion(&rect, region, region);
	SetDSRegion(info, region);

	XtAddCallback(widget, XmNdestroyCallback,
		DestroyCallback, dsm);

	return(info);
}

/*ARGSUSED*/
static XmDSInfo 
#ifdef _NO_PROTO
CreateClipperDSInfo( dsm, clipW )
        XmDropSiteManagerObject dsm ;
        Widget clipW ;
#else
CreateClipperDSInfo(
        XmDropSiteManagerObject dsm,
        Widget clipW )
#endif /* _NO_PROTO */
{
	XmDSInfo info = NULL;
	XmRegion region = _XmRegionCreate();
	XRectangle rect;

	info = (XmDSInfo) XtCalloc(1, sizeof(XmDSLocalNoneNodeRec));

	SetDSLeaf(info, True);
	SetDSInternal(info, True);
	SetDSType(info, XmDROP_SITE_COMPOSITE);
	SetDSAnimationStyle(info, XmDRAG_UNDER_NONE);
	SetDSWidget(info, clipW);
	SetDSActivity(info, XmDROP_SITE_ACTIVE);

	rect.x = rect.y = 0;
	rect.width = XtWidth(clipW);
	rect.height = XtHeight(clipW);
	_XmRegionUnionRectWithRegion(&rect, region, region);
	SetDSRegion(info, region);

	/*
	 * Don't need a destroy callback.  When this widget is destroyed
	 * the drop site children will be destroyed and as a side-effect
	 * of the last drop site being destroyed, the clipper will be
	 * destroyed.
	 */

	return(info);
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
InsertInfo( dsm, info, call_data )
        XmDropSiteManagerObject dsm ;
        XtPointer info ;
        XtPointer call_data ;
#else
InsertInfo(
        XmDropSiteManagerObject dsm,
        XtPointer info,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	XmDSInfo	childInfo = (XmDSInfo) info;
	XmDSInfo	parentInfo = NULL;
	Widget		parent = XtParent(GetDSWidget(childInfo));

	while (!(parentInfo = (XmDSInfo) DSMWidgetToInfo(dsm, parent)) &&
		!XtIsShell(parent))
	{
		parent = XtParent(parent);
	}
	if (parentInfo == NULL)
	{
		/* 
		 * We've traversed clear back to the shell and not found a
		 * parent for this info.  Therefore this must be the first drop
		 * site to be registered under this shell, and we must create a
		 * parent place holder at for shell
		 */
		parentInfo = CreateShellDSInfo(dsm, parent);
		DSMRegisterInfo(dsm, parent, (XtPointer) parentInfo);
		AddDSChild(parentInfo, childInfo, GetDSNumChildren(parentInfo));

		if ((dsm->dropManager.treeUpdateProc) &&
			(!XtIsRealized(parent) ||
				(_XmGetDragProtocolStyle(parent) == XmDRAG_DYNAMIC)))
		{
			/*
			 * If this is a preregister client and the shell isn't
			 * realized yet, we need to register this shell with the
			 * DragDisplay so the DragDisplay can register a realize
			 * callback on this shell.
			 *
			 * OR
			 *
			 * If this is a dynamic client, we need to notify the Drag-
			 * Display exactly once so that event handlers and such
			 * can be installed on this client.
			 */

			XmDropSiteTreeAddCallbackStruct	outCB;
			
			outCB.reason = XmCR_DROP_SITE_TREE_ADD;
			outCB.event = NULL;
			outCB.rootShell = parent;
			outCB.numDropSites = 0; /* Unused */
			outCB.numArgsPerDSHint = 0;

			(dsm->dropManager.treeUpdateProc)
			  ((Widget) dsm, NULL, (XtPointer) &outCB);
		}
	}
	else if (GetDSType(parentInfo) == XmDROP_SITE_COMPOSITE)
	{
		AddDSChild(parentInfo, childInfo, GetDSNumChildren(parentInfo));
	}
	else
	{

#ifdef I18N_MSG
		_XmWarning(GetDSWidget(childInfo), 
	                   catgets(Xm_catd,MS_DropS,MSG_DRS_1, MESSAGE1));
#else
		_XmWarning(GetDSWidget(childInfo), MESSAGE1);
#endif

	}
}

static void 
#ifdef _NO_PROTO
RemoveInfo( dsm, info )
        XmDropSiteManagerObject dsm ;
        XtPointer info ;
#else
RemoveInfo(
        XmDropSiteManagerObject dsm,
        XtPointer info )
#endif /* _NO_PROTO */
{
	Widget widget = GetDSWidget(info);
	XmDSInfo parentInfo = (XmDSInfo) GetDSParent(info);

	RemoveDSChild(parentInfo, (XmDSInfo) info);

	DSMUnregisterInfo(dsm, info);

	XtRemoveCallback(widget, XmNdestroyCallback, DestroyCallback, dsm);

	if ((parentInfo != NULL) &&
		(GetDSNumChildren(parentInfo) == 0) &&
		(GetDSInternal(parentInfo)))
	{
		if (XtIsShell(GetDSWidget(parentInfo)))
		{
			/*
			 * Need to notify the DragDisplay that this shell no
			 * longer has any drop sites in it.
			 */
			if (dsm->dropManager.treeUpdateProc)
			{
				XmDropSiteTreeAddCallbackStruct	outCB;

				outCB.reason = XmCR_DROP_SITE_TREE_REMOVE;
				outCB.event = NULL;
				outCB.rootShell = GetDSWidget(parentInfo);
				(dsm->dropManager.treeUpdateProc)
				  ((Widget)dsm, NULL, (XtPointer) &outCB);
			}
		}

		DSMDestroyInfo(dsm, GetDSWidget(parentInfo));
	}
}


static Boolean 
#ifdef _NO_PROTO
IntersectWithWidgetAncestors( w, r )
        Widget w ;
        XmRegion r ;
#else
IntersectWithWidgetAncestors(
        Widget w,
        XmRegion r )
#endif /* _NO_PROTO */
{
	/*
	 * r is the bounding box of the region.  It is in widget relative
	 * coordinates.
	 */
	XRectangle parentR;
	static XmRegion tmpR = NULL;
	Dimension bw = XtBorderWidth(w);

	if (XtIsShell(w))
	{
		return(True);
	}

	if (tmpR == NULL)
	{
		tmpR = _XmRegionCreate();
	}

	/* Translate the coordinates into parent relative coords */
	_XmRegionOffset(r, (XtX(w) + bw), (XtY(w) + bw));

	parentR.x = parentR.y = 0;
	parentR.width = XtWidth(XtParent(w));
	parentR.height = XtHeight(XtParent(w));

	_XmRegionClear(tmpR);
	_XmRegionUnionRectWithRegion(&parentR, tmpR, tmpR);

	_XmRegionIntersect(tmpR, r, r);

	if (!_XmRegionIsEmpty(r))
		return(IntersectWithWidgetAncestors(XtParent(w), r));
	else
		return(False);
}


static Boolean 
#ifdef _NO_PROTO
IntersectWithDSInfoAncestors( parent, r )
        XmDSInfo parent ;
        XmRegion r ;
#else
IntersectWithDSInfoAncestors(
        XmDSInfo parent,
        XmRegion r )
#endif /* _NO_PROTO */
{
	static XmRegion testR = (XmRegion) NULL;
	static XmRegion pR = (XmRegion) NULL;
	Dimension bw;

	if (testR == NULL)
	{
		testR = _XmRegionCreate();
		pR = _XmRegionCreate();
	}

	/*
	 * A simplifying assumption in this code is that the regions
	 * are all relative to the shell widget.  We don't have to 
	 * do any fancy translations.
	 *
	 * All that we have to do is successively intersect the drop site
	 * region with its ancestors until we reach the top of the drop
	 * site tree.
	 */

	/*
	 * If got to the top, then there is some part of the 
	 * region which is visible.
	 */
	if (parent == NULL)
		return(True);

	_XmRegionUnion(GetDSRegion(parent), GetDSRegion(parent), pR);

	if ((bw = GetDSBorderWidth(parent)) != 0)
	{
		/* 
		 * Adjust for the border width ala X clipping
		 * Recall that all Composite's drop rectangles represent the
		 * refW's sensitive area (including border width), but clipping
		 * should be done to the window not the border.  The clip
		 * region is smaller than the sensitive region.
		 */
		_XmRegionShrink(pR, bw, bw);
	}

	_XmRegionIntersect(r, pR, r);

	/* C will ensure that we only recurse if testR is non-empty */
	return((!_XmRegionIsEmpty(r)) &&
		(IntersectWithDSInfoAncestors(
			(XmDSInfo) GetDSParent(parent), r)));
}


static Boolean 
#ifdef _NO_PROTO
CalculateAncestorClip( dsm, info, r )
        XmDropSiteManagerObject dsm ;
        XmDSInfo info ;
        XmRegion r ;
#else
CalculateAncestorClip(
        XmDropSiteManagerObject dsm,
        XmDSInfo info,
        XmRegion r )
#endif /* _NO_PROTO */
{
	/*
	 * When this procedure finishes, r will contain the composite 
	 * clip region for all ancestors of info.  The clip region will
	 * be in shell relative coordinates.
	 */
	_XmRegionClear(r);

    if (GetDSRemote(info))
	{
		XRectangle universe;

		/* Set it to the "universe" -- which is shell relative */
		universe.x = universe.y = 0;
		universe.width = dsm->dropManager.rootW;
		universe.height = dsm->dropManager.rootH;

		_XmRegionUnionRectWithRegion(&universe, r, r);

		/*
		 * IntersectWithDSInfoAncestors will shoot the universe
		 * through all of the DSInfo ancestors and return us what
		 * is left in r.
		 */
		return(IntersectWithDSInfoAncestors(
			(XmDSInfo) GetDSParent(info), r));
	}
	else
	{
		XRectangle parentR;
		Widget parentW = XtParent(GetDSWidget(info));

		if (parentW == NULL)
			return(True);
		else
		{
			parentR.x = parentR.y = -(XtBorderWidth(parentW));
			parentR.width = XtWidth(parentW);
			parentR.height = XtHeight(parentW);

			_XmRegionUnionRectWithRegion(&parentR, r, r);

			/*
			 * IntersectWithWidgetAncestors will intersect the parent
			 * of info with all successive parents and return us what
			 * is left in r.
			 */
			return(IntersectWithWidgetAncestors(parentW, r));
		}
	}
}


static Boolean 
#ifdef _NO_PROTO
PointInDS( dsm, info, x, y )
        XmDropSiteManagerObject dsm ;
        XmDSInfo info ;
        Position x ;
        Position y ;
#else
PointInDS(
        XmDropSiteManagerObject dsm,
        XmDSInfo info,
#if NeedWidePrototypes
        int x,
        int y )
#else
        Position x,
        Position y )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	static XmRegion testR = (XmRegion) NULL;
	static XmRegion tmpR = (XmRegion) NULL;
	XmRegion *visR = &(dsm->dropManager.newAncestorClipRegion);
	Widget w = GetDSWidget(info);

	if (testR == NULL)
	{
		testR = _XmRegionCreate();
		tmpR = _XmRegionCreate();
	}

	/* 
	 * CalculateAncestorClip will intersect the universe with all of
	 * the ancestors.  If anything is left, it will return true the
	 * intersection will be in tmpR.
	 */

	if (!CalculateAncestorClip(dsm, info, tmpR))
		return(False);
	
	if (GetDSRemote(info))
	{
		/* 
		 * We know that the region in the info struct is shell relative
		 */
		_XmRegionIntersect(tmpR, GetDSRegion(info), testR);
	}
	else
	{
		Position tmpX, tmpY;

		_XmRegionUnion(GetDSRegion(info), GetDSRegion(info), testR);

#ifdef _update_code_not_working
		if (GetDSHasRegion(info))
		{
			_XmRegionUnion(GetDSRegion(info), GetDSRegion(info), testR);
		}
		else
		{
			/*
			 * The region is the widget rectangle
			 */
			XRectangle rect;
			Dimension bw = XtBorderWidth(w);

			rect.x = rect.y = -bw;
			rect.width = XtWidth(w) + (2 * bw);
			rect.height = XtHeight(w) + (2 * bw);

			_XmRegionClear(testR);
			_XmRegionUnionRectWithRegion(&rect, testR, testR);

			/* update the ds region */
			_XmRegionUnion(testR, testR, GetDSRegion(info));
		}
#endif

		/* 
		 * We know that the information is widget
		 * relative so we will have to translate it.
		 */

		XtTranslateCoords(w, 0, 0, &tmpX, &tmpY);

		_XmRegionOffset(testR, (tmpX - dsm->dropManager.rootX),
			(tmpY - dsm->dropManager.rootY));
		_XmRegionIntersect(tmpR, testR, testR);
	}

	if ((!_XmRegionIsEmpty(testR)) &&
		(_XmRegionPointInRegion(testR, x, y)))
	{
		_XmRegionUnion(tmpR, tmpR, *visR);
		return(True);
	}
	else
		return(False);
}


static XmDSInfo 
#ifdef _NO_PROTO
PointToDSInfo( dsm, info, x, y )
        XmDropSiteManagerObject dsm ;
        XmDSInfo info ;
        Position x ;
        Position y ;
#else
PointToDSInfo(
        XmDropSiteManagerObject dsm,
        XmDSInfo info,
#if NeedWidePrototypes
        int x,
        int y )
#else
        Position x,
        Position y )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	unsigned int	i;
	XmDSInfo		child = NULL;

	if (!GetDSLeaf(info))
	{
		/*
		 * This should be optimized at some point.
		 * CalculateAncestorClip is having to do potentially
		 * unneccessary work, because it starts from scratch each time.
		 */
		for (i = 0; i < GetDSNumChildren(info); i++)
		{
			child = (XmDSInfo) GetDSChild(info,i);
			if (PointInDS(dsm, child, x, y))
			{
				/*
				 * The lights are on, but nobody is home.
				 */
				if (GetDSActivity(child) == XmDROP_SITE_INACTIVE)
					return(NULL);

				if (!GetDSLeaf(child))
				{
					XmDSInfo descendant = PointToDSInfo(dsm, child,
						x, y);

					if (descendant != NULL)
						return(descendant);
				}

				if (!GetDSInternal(child))
					return(child);
			}
		}
	}

	return(NULL);
}

static void 
#ifdef _NO_PROTO
DoAnimation( dsm, motionData, callback )
        XmDropSiteManagerObject dsm ;
        XmDragMotionClientData motionData ;
        XtPointer callback ;
#else
DoAnimation(
        XmDropSiteManagerObject dsm,
        XmDragMotionClientData motionData,
        XtPointer callback )
#endif /* _NO_PROTO */
{

	XmDSInfo info = (XmDSInfo) (dsm->dropManager.curInfo);
	XmDSInfo parentInfo = (XmDSInfo) GetDSParent(info);
	Widget w;
	int i, n;
	XmDSInfo child;
	XmAnimationDataRec animationData;
	static XmRegion dsRegion = (XmRegion) NULL;
	static XmRegion clipRegion = (XmRegion) NULL;
	static XmRegion tmpRegion = (XmRegion) NULL;
	Widget dc = dsm->dropManager.curDragContext;
	Boolean sourceIsExternal;
	Dimension bw = 0;
	Arg args[1];

	if (GetDSAnimationStyle(info) == XmDRAG_UNDER_NONE)
		return;

	/*
	 * Should we have saved this from the last top level enter?
	 */
	n = 0;
	XtSetArg(args[n], XmNsourceIsExternal, &sourceIsExternal); n++;
	XtGetValues(dc, args, n);

	if (dsRegion == NULL)
	{
		dsRegion   = _XmRegionCreate();
		clipRegion = _XmRegionCreate();
		tmpRegion  = _XmRegionCreate();
	}

	if (sourceIsExternal)
	{
		animationData.dragOver = NULL;

		/*
		 * The window is expected to the the shell window which will be
		 * drawn in with include inferiors.
		 */
		animationData.window = XtWindow(GetDSWidget(
			dsm->dropManager.dsRoot));
		animationData.screen = XtScreen(GetDSWidget(
			dsm->dropManager.dsRoot));
	}
	else
	{
		animationData.dragOver = motionData->dragOver;
		animationData.window = motionData->window;
		animationData.screen = XtScreen(motionData->dragOver);
	}

	animationData.windowX = dsm->dropManager.rootX;
	animationData.windowY = dsm->dropManager.rootY;
	animationData.saveAddr =
		(XtPointer) &(dsm->dropManager.dragUnderData);

	/* We're going to need a copy. */
	_XmRegionUnion(GetDSRegion(info), GetDSRegion(info), dsRegion);

	bw = GetDSBorderWidth(info);

	if (!GetDSRemote(info))
	{
		Position wX, wY;

		w = GetDSWidget(info);

		XtTranslateCoords(w, 0, 0, &wX, &wY);

		_XmRegionOffset(dsRegion, (wX - dsm->dropManager.rootX),
			(wY - dsm->dropManager.rootY));
	}

	/* All drawing occurs within the drop site */
	_XmRegionUnion(dsRegion, dsRegion, clipRegion);

	if (bw && !GetDSHasRegion(info))
	{
		/*
		 * The region is stored widget relative, and it represents
		 * the entire drop-sensitive area of the drop site.  In the
		 * case that we provided the region this includes the
		 * border area (the x,y position of the bounding box is
		 * negative), however, we don't animate the entire
		 * sensitive region; we only animate the sensitive region
		 * within the border.  Sooo, if we provided the region, and
		 * the widget has a border, shrink down the region 
		 * (which will offset it) before passing it on to the
		 * animation code.
		 */
	
		_XmRegionShrink(clipRegion, bw, bw);
	}

	/*
	 * trim off anything clipped by ancestors
	 * ancestorClip region is in shell relative coordinates.
	 */
	_XmRegionIntersect(clipRegion,
		dsm->dropManager.curAncestorClipRegion, clipRegion);

	/* trim off anything obsucred by a sibling stacked above us */
	if (parentInfo != NULL)
	{
		for (i = 0; i < GetDSNumChildren(parentInfo); i++)
		{
			child = (XmDSInfo) GetDSChild(parentInfo, i);
			if (child == info)
				break;
			else
			{
				if (GetDSRemote(child))
				{
					/*
					 * Non-local case.  The info region is in shell
					 * relative coordinates.
					 */
					_XmRegionSubtract(clipRegion, GetDSRegion(child),
						clipRegion);
				}
				else
				{
					/*
					 * Local case.  We have to translate the region.
					 */
					Position wX, wY;
					Widget sibling = GetDSWidget(child);

					XtTranslateCoords(sibling, 0, 0, &wX, &wY);

					_XmRegionUnion(GetDSRegion(child),
						GetDSRegion(child), tmpRegion);

					_XmRegionOffset(tmpRegion,
						(wX - dsm->dropManager.rootX),
						(wY - dsm->dropManager.rootY));

					_XmRegionSubtract(clipRegion, tmpRegion,
						clipRegion);
				}
			}
		}
	}
	animationData.clipRegion = clipRegion;
	animationData.dropSiteRegion = dsRegion;

	_XmDragUnderAnimation((Widget)dsm,
		(XtPointer) &animationData,
		(XtPointer) callback);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ProxyDragProc( dsm, client_data, callback )
        XmDropSiteManagerObject dsm ;
		XtPointer client_data;
        XmDragProcCallbackStruct *callback ;
#else
ProxyDragProc(
        XmDropSiteManagerObject dsm,
		XtPointer client_data,
        XmDragProcCallbackStruct *callback )
#endif /* _NO_PROTO */
{
	XmDSInfo info = (XmDSInfo) dsm->dropManager.curInfo;
	XmDragContext dc = (XmDragContext) callback->dragContext;
	Atom *import_targets = NULL, *export_targets = NULL;
	Cardinal num_import = 0, num_export = 0;
	int n;
	Arg args[10];
	Widget shell;

#ifdef NON_OSF_FIX	/* CR # 5937 */
	unsigned char operations;

	operations = callback->operations & GetDSOperations(info);
	if (XmDROP_MOVE & operations)
		callback->operation = XmDROP_MOVE;
	else if (XmDROP_COPY & operations)
		callback->operation = XmDROP_COPY;
	else if (XmDROP_LINK & operations)
		callback->operation = XmDROP_LINK;
	else 
		callback->operation = XmDROP_NOOP;
#else
	callback->operations = (callback->operations
		& GetDSOperations(info));
	
	if (XmDROP_MOVE & callback->operations)
		callback->operation = XmDROP_MOVE;
	else if (XmDROP_COPY & callback->operations)
		callback->operation = XmDROP_COPY;
	else if (XmDROP_LINK & callback->operations)
		callback->operation = XmDROP_LINK;
	else 
	{
		callback->operation = XmDROP_NOOP;

		/* XmDROP_NOOP ought to be 0, but just in case */
		callback->operations = XmDROP_NOOP;
	}
#endif

	n = 0;
	XtSetArg(args[n], XmNexportTargets, &export_targets); n++;
	XtSetArg(args[n], XmNnumExportTargets, &num_export); n++;
	XtGetValues ((Widget)dc, args, n);

	if (GetDSRemote(info))
		shell = XtParent(dsm);
	else
		shell = GetDSWidget(info);

	while (!XtIsShell(shell))
		shell = XtParent(shell);

	num_import = _XmIndexToTargets(shell,
		GetDSImportTargetsID(info), &import_targets);
	
	if ((callback->operation != XmDROP_NOOP) &&
		(XmTargetsAreCompatible (XtDisplay (dsm),
			export_targets, num_export, import_targets, num_import)))
		callback->dropSiteStatus = XmVALID_DROP_SITE;
	else
		callback->dropSiteStatus = XmINVALID_DROP_SITE;
	
	callback->animate = True;
}

static void 
#ifdef _NO_PROTO
HandleEnter( dsm, motionData, callback, info, style )
        XmDropSiteManagerObject dsm ;
        XmDragMotionClientData motionData ;
        XmDragMotionCallbackStruct *callback ;
        XmDSInfo info ;
        unsigned char style ;
#else
HandleEnter(
        XmDropSiteManagerObject dsm,
        XmDragMotionClientData motionData,
        XmDragMotionCallbackStruct *callback,
        XmDSInfo info,
#if NeedWidePrototypes
        unsigned int style )
#else
        unsigned char style )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	XmDragProcCallbackStruct cbRec;
	Position tmpX, tmpY;
	XRectangle extents;

	cbRec.reason = XmCR_DROP_SITE_ENTER_MESSAGE;
	cbRec.event = (XEvent *) NULL;
	cbRec.timeStamp = callback->timeStamp;
	cbRec.dragContext = dsm->dropManager.curDragContext;
	cbRec.x = dsm->dropManager.curX;
	cbRec.y = dsm->dropManager.curY;
	cbRec.dropSiteStatus = XmVALID_DROP_SITE;
	cbRec.operations = callback->operations;
	cbRec.operation = callback->operation;
	cbRec.animate = True;

	ProxyDragProc(dsm, NULL, &cbRec);

	if ((style == XmDRAG_DYNAMIC) &&
		(!GetDSRemote(info)) &&
		(GetDSDragProc(info) != NULL))
	{
		Widget	widget = GetDSWidget(info);
		/* Make the coordinates widget relative */

		/* Return if this is not a managed widget, CR5215 */
		if (! XtIsManaged(widget)) return;

		XtTranslateCoords(widget, 0, 0, &tmpX, &tmpY);

		cbRec.x -= tmpX;
		cbRec.y -= tmpY;

		(*(GetDSDragProc(info)))
			(widget, NULL, (XtPointer) &cbRec);
	}

	if ((cbRec.animate) &&
		(cbRec.dropSiteStatus == XmVALID_DROP_SITE))
		DoAnimation(dsm, motionData, (XtPointer) &cbRec);

	dsm->dropManager.curDropSiteStatus = cbRec.dropSiteStatus;
	dsm->dropManager.curAnimate = cbRec.animate;
	dsm->dropManager.curOperations = cbRec.operations;
	dsm->dropManager.curOperation = cbRec.operation;

	if (dsm->dropManager.notifyProc)
	{
		XmDropSiteEnterCallbackStruct	outCB;

		_XmRegionGetExtents(GetDSRegion(info), &extents);

		outCB.reason = XmCR_DROP_SITE_ENTER;
		outCB.event = NULL;
		outCB.timeStamp = cbRec.timeStamp;
		outCB.dropSiteStatus = cbRec.dropSiteStatus;
		outCB.operations = cbRec.operations;
		outCB.operation = cbRec.operation;

		/*
		 * Pass outCB.x and outCB.y as the root relative position 
		 * of the entered drop site.  Remote info's are already
		 * in shell coordinates; Local info's are in widget
		 * relative coordinates.
		 */
		if (GetDSRemote(info))
		{
			outCB.x = extents.x + dsm->dropManager.rootX;
			outCB.y = extents.y + dsm->dropManager.rootY;
		}
		else
		{
			Widget	widget = GetDSWidget(info);

			XtTranslateCoords(widget, 0, 0, &tmpX, &tmpY);

			outCB.x = extents.x + tmpX;
			outCB.y = extents.y + tmpY;
		}

		(*(dsm->dropManager.notifyProc))
			((Widget)dsm, dsm->dropManager.client_data,
			 (XtPointer) &outCB);
	}
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
HandleMotion( dsm, motionData, callback, info, style )
        XmDropSiteManagerObject dsm ;
        XmDragMotionClientData motionData ;
        XmDragMotionCallbackStruct *callback ;
        XmDSInfo info ;
        unsigned char style ;
#else
HandleMotion(
        XmDropSiteManagerObject dsm,
        XmDragMotionClientData motionData,
        XmDragMotionCallbackStruct *callback,
        XmDSInfo info,
#if NeedWidePrototypes
        unsigned int style )
#else
        unsigned char style )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	XmDragProcCallbackStruct cbRec;

	cbRec.reason = XmCR_DROP_SITE_MOTION_MESSAGE;
	cbRec.event = (XEvent *) NULL;
	cbRec.timeStamp = callback->timeStamp;
	cbRec.dragContext = dsm->dropManager.curDragContext;
	cbRec.x = dsm->dropManager.curX;
	cbRec.y = dsm->dropManager.curY;
	cbRec.animate = dsm->dropManager.curAnimate;
	cbRec.dropSiteStatus = dsm->dropManager.curDropSiteStatus;

	if (info != NULL)
	{
		cbRec.operations = dsm->dropManager.curOperations;
		cbRec.operation = dsm->dropManager.curOperation;

		if ((style == XmDRAG_DYNAMIC) &&
			(!GetDSRemote(info)) &&
			(GetDSDragProc(info) != NULL))
		{
			Widget	widget = GetDSWidget(info);
			Position tmpX, tmpY;

			/* Make the coordinates widget relative */

			XtTranslateCoords(widget, 0, 0, &tmpX, &tmpY);

			cbRec.x -= tmpX;
			cbRec.y -= tmpY;

			(*(GetDSDragProc(info)))
				(widget, NULL, (XtPointer) &cbRec);
		}

		if ((cbRec.animate) &&
			(cbRec.dropSiteStatus !=
				dsm->dropManager.curDropSiteStatus))
		{
			if (cbRec.dropSiteStatus == XmVALID_DROP_SITE)
				cbRec.reason = XmCR_DROP_SITE_ENTER;
			else
				cbRec.reason = XmCR_DROP_SITE_LEAVE;

			DoAnimation(dsm, motionData, &cbRec);
			cbRec.reason = XmCR_DROP_SITE_MOTION_MESSAGE;
		}

		dsm->dropManager.curDropSiteStatus = cbRec.dropSiteStatus;
		dsm->dropManager.curAnimate = cbRec.animate;
		dsm->dropManager.curOperations = cbRec.operations;
		dsm->dropManager.curOperation = cbRec.operation;
	}
	else
	{
		cbRec.operations = callback->operations;
		cbRec.operation = callback->operation;
		cbRec.dropSiteStatus = XmNO_DROP_SITE;
	}

	if (dsm->dropManager.notifyProc)
	{
		XmDragMotionCallbackStruct	outCB;

		outCB.reason = XmCR_DRAG_MOTION;
		outCB.event = NULL;
		outCB.timeStamp = cbRec.timeStamp;
		outCB.dropSiteStatus = cbRec.dropSiteStatus;
		outCB.x = dsm->dropManager.curX;
		outCB.y = dsm->dropManager.curY;
		outCB.operations = cbRec.operations;
		outCB.operation = cbRec.operation;

		(*(dsm->dropManager.notifyProc))
			((Widget)dsm, dsm->dropManager.client_data,
			 (XtPointer)&outCB);
	}
}

static void 
#ifdef _NO_PROTO
HandleLeave( dsm, motionData, callback, info, style, enterPending )
        XmDropSiteManagerObject dsm ;
        XmDragMotionClientData motionData ;
        XmDragMotionCallbackStruct *callback ;
        XmDSInfo info ;
        unsigned char style ;
        Boolean enterPending ;
#else
HandleLeave(
        XmDropSiteManagerObject dsm,
        XmDragMotionClientData motionData,
        XmDragMotionCallbackStruct *callback,
        XmDSInfo info,
#if NeedWidePrototypes
        unsigned int style,
        int enterPending )
#else
        unsigned char style,
        Boolean enterPending )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	XmDragProcCallbackStruct cbRec;
    
	cbRec.reason = XmCR_DROP_SITE_LEAVE_MESSAGE;
	cbRec.event = (XEvent *) NULL;
	cbRec.timeStamp = callback->timeStamp;
	cbRec.dragContext = dsm->dropManager.curDragContext;
	cbRec.x = dsm->dropManager.oldX;
	cbRec.y = dsm->dropManager.oldY;
	cbRec.operations = callback->operations;
	cbRec.operation = callback->operation;
	cbRec.animate = dsm->dropManager.curAnimate;
	cbRec.dropSiteStatus = dsm->dropManager.curDropSiteStatus;

	if ((style == XmDRAG_DYNAMIC) && 
		(!GetDSRemote(info) && (GetDSDragProc(info) != NULL)))
	{
		Widget widget = GetDSWidget(info);
		Position tmpX, tmpY;

		/* Make the coordinates widget relative */

		XtTranslateCoords(widget, 0, 0, &tmpX, &tmpY);

		cbRec.x -= tmpX;
		cbRec.y -= tmpY;

		(*(GetDSDragProc(info)))
			(widget, NULL, (XtPointer) &cbRec);
	}

	if ((cbRec.animate) &&
		(cbRec.dropSiteStatus == XmVALID_DROP_SITE))
		DoAnimation(dsm, motionData, (XtPointer) &cbRec);

	if (dsm->dropManager.notifyProc)
	{
		XmDropSiteEnterPendingCallbackStruct	outCB;

		outCB.reason = XmCR_DROP_SITE_LEAVE;
		outCB.event = callback->event;
		outCB.timeStamp = cbRec.timeStamp;
                outCB.enter_pending = enterPending;

		(*(dsm->dropManager.notifyProc))
			((Widget)dsm, dsm->dropManager.client_data,
			 (XtPointer)&outCB);
	}
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ProcessMotion( dsm, clientData, calldata )
        XmDropSiteManagerObject dsm ;
        XtPointer clientData ;
        XtPointer calldata ;
#else
ProcessMotion(
        XmDropSiteManagerObject dsm,
        XtPointer clientData,
        XtPointer calldata )
#endif /* _NO_PROTO */
{
	XmDragMotionCallbackStruct *callback =
		(XmDragMotionCallbackStruct *) calldata;
	XmDragMotionClientData	motionData = 
		(XmDragMotionClientData) clientData;
    Position	x = callback->x, y = callback->y;
    XmDSInfo	dsRoot = (XmDSInfo) (dsm->dropManager.dsRoot);
    XmDSInfo	curDSInfo = (XmDSInfo)(dsm->dropManager.curInfo);
    XmDSInfo	newDSInfo;
	unsigned char style;

	if (dsm->dropManager.curDragContext == NULL)
	{

#ifdef I18N_MSG
		_XmWarning((Widget)dsm, catgets(Xm_catd,MS_DropS,MSG_DRS_2,
		    MESSAGE2));
#else
		_XmWarning((Widget)dsm, MESSAGE2);
#endif

		return;
	}

	style = _XmGetActiveProtocolStyle(dsm->dropManager.curDragContext);
	dsm->dropManager.curTime = callback->timeStamp;
    dsm->dropManager.oldX = dsm->dropManager.curX;
    dsm->dropManager.oldY = dsm->dropManager.curY;
    dsm->dropManager.curX = x;
    dsm->dropManager.curY = y;
    
	if (dsRoot)
	{
		/*
		 * Make x and y shell relative, preregister info's are shell
		 * relative, and CalculateAncestorClip (called for dynammic)
		 * returns a shell relative rectangle.
		 */
		x -= dsm->dropManager.rootX;
		y -= dsm->dropManager.rootY;

		newDSInfo =  PointToDSInfo(dsm, dsRoot, x, y);

		if (curDSInfo != newDSInfo)
		{
			if (curDSInfo) {
#ifndef OSF_v1_2_4
				/* if we are entering a drop site as we leave
				 * the old drop site, we don't want to set
				 * the drop site status to NO_DROP_SITE. We
				 * are using the event field of the callback
				 * (since is is guarenteed to be NULL) for
				 * use as a flag, since we can't modify the
				 * public callback struct.
				 */
				if (newDSInfo)
				   HandleLeave(dsm, motionData, callback,
				               curDSInfo, style, True);
				else
				   HandleLeave(dsm, motionData, callback,
				               curDSInfo, style, False);
#else /* OSF_v1_2_4 */
			  XmDragContext dc = 
			    (XmDragContext)dsm->dropManager.client_data;
			  /* if we are entering a drop site as we leave
			   * the old drop site, we don't want to set
			   * the drop site status to NO_DROP_SITE. We
			   * are using the event field of the callback
			   * (since is is guarenteed to be NULL) for
			   * use as a flag, since we can't modify the
			   * public callback struct.
			   */
			  if (newDSInfo)
			    HandleLeave(dsm, motionData, callback,
					curDSInfo, style, True);
			  else {
			    /* Fix for CR 7098.  Don't restore 
			       operations if there is a new valid
			       DS */
			    HandleLeave(dsm, motionData, callback,
					curDSInfo, style, False);
			    /* Fix for CR 5937,  restore operations */
			    callback->operations = dc->drag.operations;
			    callback->operation  = dc->drag.operation;
			  }
#endif /* OSF_v1_2_4 */
			}

			dsm->dropManager.curInfo = (XtPointer) newDSInfo;
			_XmRegionUnion(dsm->dropManager.newAncestorClipRegion,
				dsm->dropManager.newAncestorClipRegion,
				dsm->dropManager.curAncestorClipRegion);

			if (newDSInfo)
				HandleEnter(dsm, motionData, callback,
					newDSInfo, style);
			
			return;
		}
	}

	HandleMotion(dsm, motionData, callback, curDSInfo, style);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ProcessDrop( dsm, clientData, cb )
        XmDropSiteManagerObject dsm ;
        XtPointer clientData ;
        XtPointer cb ;
#else
ProcessDrop(
        XmDropSiteManagerObject dsm,
        XtPointer clientData,
        XtPointer cb )
#endif /* _NO_PROTO */
{
	XmDragTopLevelClientData cd =
		(XmDragTopLevelClientData) clientData;
	XmDropStartCallbackStruct *callback = 
		(XmDropStartCallbackStruct *) cb;
	XmDropProcCallbackStruct cbRec;
	Widget	dragContext =
		XmGetDragContext((Widget)dsm, callback->timeStamp);
	XmDSInfo	info = NULL;
	Widget	widget;
	Position x, y, tmpX, tmpY;
	XmDSInfo savRoot, savInfo;
	XmDSInfo newRoot = (XmDSInfo) DSMWidgetToInfo(dsm, cd->destShell);
	Position savX, savY;
	Dimension savW, savH;
	Time savTime;

	if (dragContext == NULL)
	{
		/*
		 * Can't do a failure transfer.  Just give up.
		 *
		 * Should we send a warning message?
		 */
		return;
	}

	/*
	 * Look out for race conditions.
	 *
	 * This should be enough state saving to allow the drop to occur
	 * in the middle of some other drag.
	 */
	savRoot = (XmDSInfo) dsm->dropManager.dsRoot;
	savInfo = (XmDSInfo) dsm->dropManager.curInfo;
	savX = dsm->dropManager.rootX;
	savY = dsm->dropManager.rootY;
	savW = dsm->dropManager.rootW;
	savH = dsm->dropManager.rootH;
	savTime = dsm->dropManager.curTime;

	dsm->dropManager.curTime = callback->timeStamp;
	dsm->dropManager.dsRoot = (XtPointer) newRoot;
	dsm->dropManager.rootX = cd->xOrigin;
	dsm->dropManager.rootY = cd->yOrigin;
	dsm->dropManager.rootW = cd->width;
	dsm->dropManager.rootH = cd->height;

	x = callback->x - dsm->dropManager.rootX;
	y = callback->y - dsm->dropManager.rootY;

	if (newRoot != NULL)
		info = PointToDSInfo(dsm, (XmDSInfo) dsm->dropManager.dsRoot, x, y);

	if (info != NULL) widget = GetDSWidget(info);

	/* Handle error conditions nicely */
	if ((info == NULL) ||
	    ! XtIsManaged(widget) || /* CR 5215 */
	    /* These are wierd conditions */
	    (newRoot == NULL)   ||
	    (GetDSRemote(info)) ||
	    (GetDSDropProc(info) == NULL))
	{
		/* we will do a failure drop transfer */
		Arg		args[4];
		Cardinal	i = 0;

		XtSetArg(args[i], XmNtransferStatus, XmTRANSFER_FAILURE); i++;
		XtSetArg(args[i], XmNnumDropTransfers, 0); i++;
		(void) XmDropTransferStart(dragContext, args, i);

		/* ???
		 * Should we do something interesting with the callback
		 * struct before calling notify in these cases?
		 * ???
		 */
	}
	else
	{

		/* This will be needed by the ProxyDragProc */
		dsm->dropManager.curInfo = (XtPointer) info;

		/* Make the coordinates widget relative */
		XtTranslateCoords(widget, 0, 0, &tmpX, &tmpY);

		/* Load the dropProcStruct */
		cbRec.reason = XmCR_DROP_MESSAGE;
		cbRec.event = callback->event;
		cbRec.timeStamp = callback->timeStamp;
		cbRec.dragContext = dragContext;

		/* Make the coordinates widget relative */
		XtTranslateCoords(widget, 0, 0, &tmpX, &tmpY);

		cbRec.x = callback->x - tmpX;
		cbRec.y = callback->y - tmpY;


		{ /* Nonsense to pre-load the cbRec correctly */
			XmDragProcCallbackStruct junkRec;

			junkRec.reason = XmCR_DROP_SITE_MOTION_MESSAGE;
			junkRec.event = callback->event;
			junkRec.timeStamp = cbRec.timeStamp;
			junkRec.dragContext = dragContext;
			junkRec.x = cbRec.x;
			junkRec.y = cbRec.y;
			junkRec.dropSiteStatus = dsm->dropManager.curDropSiteStatus;
			junkRec.operation = callback->operation;
			junkRec.operations = callback->operations;
			junkRec.animate = dsm->dropManager.curAnimate;

			ProxyDragProc(dsm, NULL, &junkRec);

			cbRec.dropSiteStatus = junkRec.dropSiteStatus;
			cbRec.operation = junkRec.operation;
			cbRec.operations = junkRec.operations;
		}

		cbRec.dropAction = callback->dropAction;

		/* Call the drop site's drop proc */
		(*(GetDSDropProc(info))) (widget, NULL, (XtPointer) &cbRec);

		callback->operation = cbRec.operation;
		callback->operations = cbRec.operations;
		callback->dropSiteStatus = cbRec.dropSiteStatus;
		callback->dropAction = cbRec.dropAction;
	}

	if (dsm->dropManager.notifyProc)
	{
		(*(dsm->dropManager.notifyProc))
			((Widget)dsm, dsm->dropManager.client_data,
			(XtPointer)callback);
	}

	dsm->dropManager.dsRoot = (XtPointer) savRoot;
	dsm->dropManager.curInfo = (XtPointer) savInfo;
	dsm->dropManager.rootX = savX;
	dsm->dropManager.rootY = savY;
	dsm->dropManager.rootW = savW;
	dsm->dropManager.rootH = savH;
	dsm->dropManager.curTime = savTime;
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ChangeOperation( dsm, clientData, calldata )
        XmDropSiteManagerObject dsm ;
        XtPointer clientData ;
        XtPointer calldata ;
#else
ChangeOperation(
        XmDropSiteManagerObject dsm,
        XtPointer clientData,
        XtPointer calldata )
#endif /* _NO_PROTO */
{
	XmOperationChangedCallbackStruct *callback =
		(XmOperationChangedCallbackStruct *) calldata;
	XmDragMotionClientData	motionData = 
		(XmDragMotionClientData) clientData;
	XmDragProcCallbackStruct cbRec;
	XmDSInfo info = (XmDSInfo) dsm->dropManager.curInfo;
	unsigned char style;

	if ((cbRec.dragContext = dsm->dropManager.curDragContext) == NULL)
	{

#ifdef I18N_MSG
		_XmWarning((Widget)dsm, catgets(Xm_catd,MS_DropS,MSG_DRS_3,
		MESSAGE3));
#else
		_XmWarning((Widget)dsm, MESSAGE3);
#endif

		return;
	}
	else
	{
		style = _XmGetActiveProtocolStyle(
			dsm->dropManager.curDragContext); 
	}

	cbRec.reason = callback->reason;
	cbRec.event = callback->event;
	cbRec.timeStamp = callback->timeStamp;

	cbRec.x = dsm->dropManager.curX;
	cbRec.y = dsm->dropManager.curY;
	cbRec.dropSiteStatus = dsm->dropManager.curDropSiteStatus;
	cbRec.animate = dsm->dropManager.curAnimate;

	cbRec.operation = callback->operation;
	cbRec.operations = callback->operations;

	if (info != NULL)
	{
		ProxyDragProc(dsm, NULL, &cbRec);

		if ((style == XmDRAG_DYNAMIC) &&
			(!GetDSRemote(info)) &&
			(GetDSDragProc(info) != NULL))
		{
			Widget widget = GetDSWidget(info);
			Position tmpX, tmpY;

			/* Make the coordinates widget relative */
			XtTranslateCoords(widget, 0, 0, &tmpX, &tmpY);

			cbRec.x -= tmpX;
			cbRec.y -= tmpY;

			(*(GetDSDragProc(info)))
				(widget, NULL, (XtPointer) &cbRec);
		}

		if ((cbRec.animate) &&
			(cbRec.dropSiteStatus !=
				dsm->dropManager.curDropSiteStatus))
		{
			if (cbRec.dropSiteStatus == XmVALID_DROP_SITE)
				cbRec.reason = XmCR_DROP_SITE_ENTER_MESSAGE;
			else
				cbRec.reason = XmCR_DROP_SITE_LEAVE_MESSAGE;

			DoAnimation(dsm, motionData, &cbRec);
			cbRec.reason = callback->reason;
		}

		/* Update the callback rec */
		callback->operations = cbRec.operations;
		callback->operation = cbRec.operation;
		callback->dropSiteStatus = cbRec.dropSiteStatus;

		/* Update the drop site manager */
		dsm->dropManager.curDropSiteStatus = cbRec.dropSiteStatus;
		dsm->dropManager.curAnimate = cbRec.animate;
		dsm->dropManager.curOperations = cbRec.operations;
		dsm->dropManager.curOperation = cbRec.operation;
	}
	else
	{
		callback->dropSiteStatus = XmNO_DROP_SITE;
	}

	if (dsm->dropManager.notifyProc)
	{
		(*(dsm->dropManager.notifyProc))
			((Widget)dsm, dsm->dropManager.client_data,
			(XtPointer)callback);
	}
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
PutDSToStream( dsm, dsInfo, last, dataPtr )
        XmDropSiteManagerObject dsm ;
        XmDSInfo dsInfo ;
        Boolean last ;
        XtPointer dataPtr ;
#else
PutDSToStream(
        XmDropSiteManagerObject dsm,
        XmDSInfo dsInfo,
#if NeedWidePrototypes
        int last,
#else
        Boolean last,
#endif /* NeedWidePrototypes */
        XtPointer dataPtr )
#endif /* _NO_PROTO */
{
	static XmRegion tmpRegion = NULL;
	unsigned char dsType = 0, tType = 0;
	unsigned char unitType = XmPIXELS;
	Position wX, wY;
	Widget w = GetDSWidget(dsInfo);
	Dimension bw = XtBorderWidth(w);
	XmICCDropSiteInfoStruct iccInfo;
	Arg args[30];
	int n;

	if (tmpRegion == NULL)
	{
		tmpRegion = _XmRegionCreate();
	}

	/*
	 * Clear out the info.  This is especially important in the cases
	 * that the widget does not define resources all of the required
	 * animation resources.
	 */
	memset(((void *) &iccInfo), 0, sizeof(iccInfo));

	if (last)
		tType |= XmDSM_T_CLOSE;
	else
		tType &= ~ XmDSM_T_CLOSE;
	

	if (GetDSLeaf(dsInfo) || (!GetDSNumChildren(dsInfo)))
		dsType |= XmDSM_DS_LEAF;
	else
		dsType &= ~ XmDSM_DS_LEAF;
	
	if (GetDSInternal(dsInfo))
		dsType |= XmDSM_DS_INTERNAL;
	else
		dsType &= ~ XmDSM_DS_INTERNAL;
	
	if (GetDSHasRegion(dsInfo))
		dsType |= XmDSM_DS_HAS_REGION;
	else
		dsType &= ~ XmDSM_DS_HAS_REGION;

	/*
	 * The local drop site tree is always kept in widget relative
	 * coordinates.  We have to put shell relative coordinates on
	 * the wire however, so we to a copy and translate into a tmp
	 * region.
	 */
	XtTranslateCoords(w, 0, 0, &wX, &wY);
	
	if (GetDSHasRegion(dsInfo))
	{
		_XmRegionUnion(GetDSRegion(dsInfo), GetDSRegion(dsInfo),
			tmpRegion);
	}
	else
	{
		XRectangle rect;

		rect.x = rect.y = -bw;
		rect.width = XtWidth(w) + (2 * bw);
		rect.height = XtHeight(w) + (2 * bw);

		_XmRegionClear(tmpRegion);
		_XmRegionUnionRectWithRegion(&rect, tmpRegion, tmpRegion);
	}

	_XmRegionOffset(tmpRegion, (wX - dsm->dropManager.rootX),
		(wY - dsm->dropManager.rootY));
	
	/*
	 * We need to pull up the relevant visual information from
	 * the widget so it will be available for correct animation
	 * by a non-local peregister initiator.
	 */
	iccInfo.header.dropType = dsType;
	iccInfo.header.dropActivity = GetDSActivity(dsInfo);
	iccInfo.header.traversalType = tType;
	iccInfo.header.animationStyle = GetDSAnimationStyle(dsInfo);
	iccInfo.header.operations = GetDSOperations(dsInfo);
	iccInfo.header.importTargetsID = GetDSImportTargetsID(dsInfo);
	iccInfo.header.region = tmpRegion;

	/*
	 * We need to retrieve information from the widget. XtGetValues is
	 * too slow, so retrieve the information directly from the widget
	 * instance.
	 *
	 * XtGetValues is used for non-Motif widgets, just in case they provide
	 * Motif-style resources.
	 *
	 * (See also XmDropSiteGetActiveVisuals() )
	 */

	if (XmIsPrimitive(w))
	{
	     XmPrimitiveWidget pw= (XmPrimitiveWidget)w;

	    switch(iccInfo.header.animationStyle)
	    {
		case XmDRAG_UNDER_HIGHLIGHT:
		{
			XmICCDropSiteHighlight info =
				(XmICCDropSiteHighlight)  (&iccInfo);

			info->animation_data.highlightPixmap = XmUNSPECIFIED_PIXMAP;
			if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth =
					pw->core.border_width;
			info->animation_data.highlightThickness =
				pw->primitive.highlight_thickness;
			info->animation_data.highlightColor =
				pw->primitive.highlight_color;
			info->animation_data.highlightPixmap =
				pw->primitive.highlight_pixmap;
			info->animation_data.background =
				pw->core.background_pixel;
		}
		break;
		case XmDRAG_UNDER_SHADOW_IN:
		case XmDRAG_UNDER_SHADOW_OUT:
		{
			XmICCDropSiteShadow info =
				(XmICCDropSiteShadow)  (&iccInfo);
			
			info->animation_data.topShadowPixmap = 
				info->animation_data.bottomShadowPixmap = 
				XmUNSPECIFIED_PIXMAP;

			if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth =
					pw->core.border_width;
			info->animation_data.highlightThickness =
				pw->primitive.highlight_thickness;
			info->animation_data.shadowThickness =
				pw->primitive.shadow_thickness;
			info->animation_data.foreground =
				pw->primitive.foreground;
			info->animation_data.topShadowColor =
				pw->primitive.top_shadow_color;
			info->animation_data.topShadowPixmap =
				pw->primitive.top_shadow_pixmap;
			info->animation_data.bottomShadowColor =
				pw->primitive.bottom_shadow_color;
			info->animation_data.bottomShadowPixmap =
				pw->primitive.bottom_shadow_pixmap;
		}
		break;
		case XmDRAG_UNDER_PIXMAP:
		{
			XmICCDropSitePixmap info =
				(XmICCDropSitePixmap)  (&iccInfo);
			XmDSLocalPixmapStyle ps =
				(XmDSLocalPixmapStyle) GetDSLocalAnimationPart(dsInfo);

			info->animation_data.animationPixmapDepth =
				ps->animation_pixmap_depth;
			info->animation_data.animationPixmap =
				ps->animation_pixmap;
			info->animation_data.animationMask = ps->animation_mask;

			if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth =
					pw->core.border_width;
			info->animation_data.highlightThickness =
				pw->primitive.highlight_thickness;
			info->animation_data.shadowThickness =
				pw->primitive.shadow_thickness;
			info->animation_data.foreground =
				pw->primitive.foreground;
			info->animation_data.background =
				pw->core.background_pixel;
		}
		break;
		case XmDRAG_UNDER_NONE:
		{
			XmICCDropSiteNone info =
				(XmICCDropSiteNone)  (&iccInfo);

			if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth = pw->core.border_width;
			else
				info->animation_data.borderWidth = 0;
		}
		default:
		{
			/*EMPTY*/
		}
		break;
	    }
	}
	else if (XmIsManager(w) || XmIsGadget(w))
	{
	    XmManagerWidget mw;
            XmGadget g;
            Boolean is_gadget;

            if (XmIsGadget(w)) {
	       mw = (XmManagerWidget) XtParent(w);
	       g = (XmGadget) w;
               is_gadget = True;
            } else {
	       mw = (XmManagerWidget) w;
	       g = NULL;
               is_gadget = False;
            }

	    switch(iccInfo.header.animationStyle)
	    {
		case XmDRAG_UNDER_HIGHLIGHT:
		{
			XmICCDropSiteHighlight info =
				(XmICCDropSiteHighlight)  (&iccInfo);

			info->animation_data.highlightPixmap = XmUNSPECIFIED_PIXMAP;
			if (!GetDSHasRegion(dsInfo))
			{
                           if (is_gadget)
			      info->animation_data.borderWidth =
					w->core.border_width;
                           else
			      info->animation_data.borderWidth =
					mw->core.border_width;
                        }	

		       /* Temporary hack until we support full defaulting */
		        info->animation_data.highlightThickness = 1;
			info->animation_data.highlightColor =
				mw->manager.highlight_color;
			info->animation_data.highlightPixmap =
				mw->manager.highlight_pixmap;
			info->animation_data.background =
				mw->core.background_pixel;
		}
		break;
		case XmDRAG_UNDER_SHADOW_IN:
		case XmDRAG_UNDER_SHADOW_OUT:
		{
			XmICCDropSiteShadow info =
				(XmICCDropSiteShadow)  (&iccInfo);
			
			info->animation_data.topShadowPixmap = 
				info->animation_data.bottomShadowPixmap = 
				XmUNSPECIFIED_PIXMAP;

                        if (is_gadget)
			{
			   if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth =
					w->core.border_width;

			   info->animation_data.shadowThickness =
				   g->gadget.shadow_thickness;
                        }
			else
			{
			   if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth =
					mw->core.border_width;

			   info->animation_data.shadowThickness =
				   mw->manager.shadow_thickness;
                        }
			info->animation_data.highlightThickness = 0;
			info->animation_data.foreground =
				mw->manager.foreground;
			info->animation_data.topShadowColor =
				mw->manager.top_shadow_color;
			info->animation_data.topShadowPixmap =
				mw->manager.top_shadow_pixmap;
			info->animation_data.bottomShadowColor =
				mw->manager.bottom_shadow_color;
			info->animation_data.bottomShadowPixmap =
				mw->manager.bottom_shadow_pixmap;
		}
		break;
		case XmDRAG_UNDER_PIXMAP:
		{
			XmICCDropSitePixmap info =
				(XmICCDropSitePixmap)  (&iccInfo);
			XmDSLocalPixmapStyle ps =
				(XmDSLocalPixmapStyle) GetDSLocalAnimationPart(dsInfo);

			info->animation_data.animationPixmapDepth =
				ps->animation_pixmap_depth;
			info->animation_data.animationPixmap =
				ps->animation_pixmap;
			info->animation_data.animationMask = ps->animation_mask;

                        if (is_gadget)
			{
			   if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth =
					w->core.border_width;

			   info->animation_data.shadowThickness =
				   g->gadget.shadow_thickness;
                        }
			else
			{
			   if (!GetDSHasRegion(dsInfo))
				info->animation_data.borderWidth =
					mw->core.border_width;

			   info->animation_data.shadowThickness =
				   mw->manager.shadow_thickness;
                        }
			info->animation_data.highlightThickness = 0;
			info->animation_data.foreground =
				mw->manager.foreground;
			info->animation_data.background =
				mw->core.background_pixel;
		}
		break;
		case XmDRAG_UNDER_NONE:
		{
			XmICCDropSiteNone info =
				(XmICCDropSiteNone)  (&iccInfo);

			if (!GetDSHasRegion(dsInfo))
			{
                           if (is_gadget)
			      info->animation_data.borderWidth =
					w->core.border_width;
                           else
			      info->animation_data.borderWidth =
					mw->core.border_width;
                        } else
			   info->animation_data.borderWidth = 0;
		}
		default:
		{
			/*EMPTY*/
		}
		break;
	    }
	}
	else /* non-Motif subclass */
	{
		n = 0;
		XtSetArg(args[n], XmNunitType, &unitType); n++;
		XtGetValues(w, args, n);

		if (unitType != XmPIXELS) { /* we need values in pixels */
		    n = 0;
		    XtSetArg(args[n], XmNunitType, XmPIXELS); n++;
		    XtSetValues(w, args, n);
		}

	 	switch(iccInfo.header.animationStyle)
	 	{
		    case XmDRAG_UNDER_HIGHLIGHT:
		    {
			XmICCDropSiteHighlight info =
				(XmICCDropSiteHighlight)  (&iccInfo);

			/*
			 * Pre-load a sane pixmap default in case the
			 * widget doesn't have a pixmap resource.
			 */
			info->animation_data.highlightPixmap = XmUNSPECIFIED_PIXMAP;

			n = 0;
			if (!GetDSHasRegion(dsInfo))
			{
				XtSetArg(args[n], XmNborderWidth,
					&(info->animation_data.borderWidth)); n++;
			}
			XtSetArg(args[n], XmNhighlightThickness,
				&(info->animation_data.highlightThickness)); n++;
			XtSetArg(args[n], XmNbackground,
				&(info->animation_data.background)); n++;
			XtSetArg(args[n], XmNhighlightColor,
				&(info->animation_data.highlightColor)); n++;
			XtSetArg(args[n], XmNhighlightPixmap,
				&(info->animation_data.highlightPixmap)); n++;
			XtGetValues(w, args, n);
		    }
		    break;
		    case XmDRAG_UNDER_SHADOW_IN:
		    case XmDRAG_UNDER_SHADOW_OUT:
		    {
			XmICCDropSiteShadow info =
				(XmICCDropSiteShadow)  (&iccInfo);
			
			/* Pre-load some sane pixmap defaults */
			info->animation_data.topShadowPixmap = 
				info->animation_data.bottomShadowPixmap = 
				XmUNSPECIFIED_PIXMAP;

			n = 0;
			if (!GetDSHasRegion(dsInfo))
			{
				XtSetArg(args[n], XmNborderWidth,
					&(info->animation_data.borderWidth)); n++;
			}
			XtSetArg(args[n], XmNhighlightThickness,
				&(info->animation_data.highlightThickness)); n++;
			XtSetArg(args[n], XmNshadowThickness,
				&(info->animation_data.shadowThickness)); n++;
			XtSetArg(args[n], XmNforeground,
				&(info->animation_data.foreground)); n++;
			XtSetArg(args[n], XmNtopShadowColor,
				&(info->animation_data.topShadowColor)); n++;
			XtSetArg(args[n], XmNbottomShadowColor,
				&(info->animation_data.bottomShadowColor)); n++;
			XtSetArg(args[n], XmNtopShadowPixmap,
				&(info->animation_data.topShadowPixmap)); n++;
			XtSetArg(args[n], XmNbottomShadowPixmap,
				&(info->animation_data.bottomShadowPixmap)); n++;
			XtGetValues(w, args, n);
		    }
		    break;
		    case XmDRAG_UNDER_PIXMAP:
		    {
			XmICCDropSitePixmap info =
				(XmICCDropSitePixmap)  (&iccInfo);
			XmDSLocalPixmapStyle ps =
				(XmDSLocalPixmapStyle) GetDSLocalAnimationPart(dsInfo);

			info->animation_data.animationPixmapDepth =
				ps->animation_pixmap_depth;
			info->animation_data.animationPixmap =
				ps->animation_pixmap;
			info->animation_data.animationMask = ps->animation_mask;

			n = 0;
			if (!GetDSHasRegion(dsInfo))
			{
				XtSetArg(args[n], XmNborderWidth,
					&(info->animation_data.borderWidth)); n++;
			}
			XtSetArg(args[n], XmNhighlightThickness,
				&(info->animation_data.highlightThickness)); n++;
			XtSetArg(args[n], XmNshadowThickness,
				&(info->animation_data.shadowThickness)); n++;
			XtSetArg(args[n], XmNforeground,
				&(info->animation_data.foreground)); n++;
			XtSetArg(args[n], XmNbackground,
				&(info->animation_data.background)); n++;
			XtGetValues(w, args, n);
		}
		    break;
		    case XmDRAG_UNDER_NONE:
		    {
			XmICCDropSiteNone info =
				(XmICCDropSiteNone)  (&iccInfo);

			if (!GetDSHasRegion(dsInfo))
			{
				n = 0;
				XtSetArg(args[n], XmNborderWidth,
					&(info->animation_data.borderWidth)); n++;
				XtGetValues(w, args, n);
			}
			else
				info->animation_data.borderWidth = 0;
		    }
		    default:
		    {
			/*EMPTY*/
		    }
		    break;

		}

		if (unitType != XmPIXELS) {
		    n = 0;
		    XtSetArg(args[n], XmNunitType, unitType); n++;
		    XtSetValues(w, args, n);
		}
	}
	_XmWriteDSToStream(dsm, dataPtr, &iccInfo);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
GetDSFromDSM( dsm, parentInfo, last, dataPtr )
        XmDropSiteManagerObject dsm ;
        XmDSInfo parentInfo ;
        Boolean last ;
        XtPointer dataPtr ;
#else
GetDSFromDSM(
        XmDropSiteManagerObject dsm,
        XmDSInfo parentInfo,
#if NeedWidePrototypes
        int last,
#else
        Boolean last,
#endif /* NeedWidePrototypes */
        XtPointer dataPtr )
#endif /* _NO_PROTO */
{
	XmDSInfo child;
	int i;

	PutDSToStream(dsm, parentInfo, last, dataPtr);

	last = False;
	for (i = 0; i < GetDSNumChildren(parentInfo); i++)
	{
		if ((i + 1) == GetDSNumChildren(parentInfo))
			last = True;

		child = (XmDSInfo) GetDSChild(parentInfo, i);
		if (!GetDSLeaf(child))
			GetDSFromDSM(dsm, child, last, dataPtr);
		else
			PutDSToStream(dsm, child, last, dataPtr);
	}
}

/*ARGSUSED*/
static int 
#ifdef _NO_PROTO
GetTreeFromDSM( dsm, shell, dataPtr )
        XmDropSiteManagerObject dsm ;
        Widget shell ;
        XtPointer dataPtr ;
#else
GetTreeFromDSM(
        XmDropSiteManagerObject dsm,
        Widget shell,
        XtPointer dataPtr )
#endif /* _NO_PROTO */
{
	XmDSInfo root = (XmDSInfo) DSMWidgetToInfo(dsm, shell);
	Position shellX, shellY, savX, savY;

	if (root == NULL)
		return(0);
	XtTranslateCoords(shell, 0, 0, &shellX, &shellY);

	/* Save current */
	savX = dsm->dropManager.rootX;
	savY = dsm->dropManager.rootY;

	dsm->dropManager.rootX = shellX;
	dsm->dropManager.rootY = shellY;

	DSMSyncTree(dsm, shell);
	GetDSFromDSM(dsm, root, True, dataPtr);

	dsm->dropManager.rootX = savX;
	dsm->dropManager.rootY = savY;

	return(CountDropSites(root));
}

/*ARGSUSED*/
static XmDSInfo 
#ifdef _NO_PROTO
GetDSFromStream( dsm, dataPtr, close, type )
        XmDropSiteManagerObject dsm ;
        XtPointer dataPtr ;
        Boolean *close ;
        unsigned char *type ;
#else
GetDSFromStream(
        XmDropSiteManagerObject dsm,
        XtPointer dataPtr,
        Boolean *close,
        unsigned char *type )
#endif /* _NO_PROTO */
{
	XmDSInfo info;
	XmICCDropSiteInfoStruct iccInfo;
#ifndef OSF_v1_2_4
	Cardinal size0, size1, size2;
#else /* OSF_v1_2_4 */
	size_t size;
#endif /* OSF_v1_2_4 */

	_XmReadDSFromStream(dsm, dataPtr, &iccInfo);

#ifndef OSF_v1_2_4
	size0 = sizeof(XmDSStatusRec);

	if (iccInfo.header.dropType & XmDSM_DS_LEAF)
		size1 = sizeof(XmDSRemoteLeafRec);
	else
		size1 = sizeof(XmDSRemoteNodeRec);
	
#endif /* OSF_v1_2_4 */
	switch(iccInfo.header.animationStyle)
	{
		case XmDRAG_UNDER_HIGHLIGHT:
#ifndef OSF_v1_2_4
			size2 = sizeof(XmDSRemoteHighlightStyleRec);
#else /* OSF_v1_2_4 */
			if (iccInfo.header.dropType & XmDSM_DS_LEAF)
				size = sizeof(XmDSRemoteHighlightLeafRec);
			else
				size = sizeof(XmDSRemoteHighlightNodeRec);
#endif /* OSF_v1_2_4 */
		break;
		case XmDRAG_UNDER_SHADOW_IN:
		case XmDRAG_UNDER_SHADOW_OUT:
#ifndef OSF_v1_2_4
			size2 = sizeof(XmDSRemoteShadowStyleRec);
#else /* OSF_v1_2_4 */
			if (iccInfo.header.dropType & XmDSM_DS_LEAF)
				size = sizeof(XmDSRemoteShadowLeafRec);
			else
				size = sizeof(XmDSRemoteShadowNodeRec);
#endif /* OSF_v1_2_4 */
		break;
		case XmDRAG_UNDER_PIXMAP:
#ifndef OSF_v1_2_4
			size2 = sizeof(XmDSRemotePixmapStyleRec);
#else /* OSF_v1_2_4 */
			if (iccInfo.header.dropType & XmDSM_DS_LEAF)
				size = sizeof(XmDSRemotePixmapLeafRec);
			else
				size = sizeof(XmDSRemotePixmapNodeRec);
#endif /* OSF_v1_2_4 */
		break;
		case XmDRAG_UNDER_NONE:
#ifndef OSF_v1_2_4
			size2 = sizeof(XmDSRemoteNoneStyleRec);
		break;
#endif /* OSF_v1_2_4 */
		default:
#ifndef OSF_v1_2_4
			size2 = 0;
#else /* OSF_v1_2_4 */
			if (iccInfo.header.dropType & XmDSM_DS_LEAF)
				size = sizeof(XmDSRemoteNoneLeafRec);
			else
				size = sizeof(XmDSRemoteNoneNodeRec);
#endif /* OSF_v1_2_4 */
		break;
	}

#ifndef OSF_v1_2_4
	info = (XmDSInfo) XtCalloc(1, (size0 + size1 + size2));
#else /* OSF_v1_2_4 */
	info = (XmDSInfo) XtCalloc(1, size);
#endif /* OSF_v1_2_4 */

	/* Load the Status fields */

	SetDSRemote(info, True);

	if (iccInfo.header.dropType & XmDSM_DS_LEAF)
	{
		SetDSLeaf(info, True);
		SetDSType(info, XmDROP_SITE_SIMPLE);
	}
	else
	{
		SetDSLeaf(info, False);
		SetDSType(info, XmDROP_SITE_COMPOSITE);
	}

	SetDSAnimationStyle(info, iccInfo.header.animationStyle);

	if (iccInfo.header.dropType & XmDSM_DS_INTERNAL)
		SetDSInternal(info, True);
	else
		SetDSInternal(info, False);

	if (iccInfo.header.dropType & XmDSM_DS_HAS_REGION)
		SetDSHasRegion(info, True);
	else
		SetDSHasRegion(info, False);
	
	SetDSActivity(info, iccInfo.header.dropActivity);
	SetDSImportTargetsID(info, iccInfo.header.importTargetsID);
	SetDSOperations(info, iccInfo.header.operations);
	SetDSRegion(info, iccInfo.header.region);

	/* Load the animation data */
	switch(GetDSAnimationStyle(info))
	{
		case XmDRAG_UNDER_HIGHLIGHT:
		{
			XmDSRemoteHighlightStyle hs =
				(XmDSRemoteHighlightStyle)
					GetDSRemoteAnimationPart(info);
			XmICCDropSiteHighlight hi =
				(XmICCDropSiteHighlight) (&iccInfo);
			
			hs->highlight_color = hi->animation_data.highlightColor;
			hs->highlight_pixmap =
				hi->animation_data.highlightPixmap;
			hs->background = hi->animation_data.background;
			hs->highlight_thickness =
				hi->animation_data.highlightThickness;
			hs->border_width =
				hi->animation_data.borderWidth;
		}
		break;
		case XmDRAG_UNDER_SHADOW_IN:
		case XmDRAG_UNDER_SHADOW_OUT:
		{
			XmDSRemoteShadowStyle ss =
				(XmDSRemoteShadowStyle) GetDSRemoteAnimationPart(info);
			XmICCDropSiteShadow si =
				(XmICCDropSiteShadow) (&iccInfo);
			
			ss->top_shadow_color =
				si->animation_data.topShadowColor;
			ss->top_shadow_pixmap =
				si->animation_data.topShadowPixmap;
			ss->bottom_shadow_color =
				si->animation_data.bottomShadowColor;
			ss->bottom_shadow_pixmap =
				si->animation_data.bottomShadowPixmap;
			ss->foreground = si->animation_data.foreground;
			ss->shadow_thickness =
				si->animation_data.shadowThickness;
			ss->highlight_thickness =
				si->animation_data.highlightThickness;
			ss->border_width = si->animation_data.borderWidth;
		}
		break;
		case XmDRAG_UNDER_PIXMAP:
		{
			XmDSRemotePixmapStyle ps =
				(XmDSRemotePixmapStyle) GetDSRemoteAnimationPart(info);
			XmICCDropSitePixmap pi =
				(XmICCDropSitePixmap) (&iccInfo);
			
			ps->animation_pixmap =
				pi->animation_data.animationPixmap;
			ps->animation_pixmap_depth =
				pi->animation_data.animationPixmapDepth;
			ps->animation_mask = pi->animation_data.animationMask;
			ps->background = pi->animation_data.background;
			ps->foreground = pi->animation_data.foreground;
			ps->shadow_thickness =
				pi->animation_data.shadowThickness;
			ps->highlight_thickness =
				pi->animation_data.highlightThickness;
			ps->border_width = pi->animation_data.borderWidth;
		}
		break;
		case XmDRAG_UNDER_NONE:
		{
			XmDSRemoteNoneStyle ns =
				(XmDSRemoteNoneStyle) GetDSRemoteAnimationPart(info);
			XmICCDropSiteNone ni =
				(XmICCDropSiteNone) (&iccInfo);
			
			ns->border_width = ni->animation_data.borderWidth;
		}
		break;
		default:
		break;
	}

	*close = (iccInfo.header.traversalType & XmDSM_T_CLOSE);
	*type = iccInfo.header.dropType;
	return(info);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
GetNextDS( dsm, parentInfo, dataPtr )
        XmDropSiteManagerObject dsm ;
        XmDSInfo parentInfo ;
        XtPointer dataPtr ;
#else
GetNextDS(
        XmDropSiteManagerObject dsm,
        XmDSInfo parentInfo,
        XtPointer dataPtr )
#endif /* _NO_PROTO */
{
	Boolean close = TRUE;
	unsigned char type;
	XmDSInfo new_w = GetDSFromStream(dsm, dataPtr, &close, &type);

	while (!close)
	{
		AddDSChild(parentInfo, new_w, GetDSNumChildren(parentInfo));
		if (! (type & XmDSM_DS_LEAF))
			GetNextDS(dsm, new_w, dataPtr);
		new_w = GetDSFromStream(dsm, dataPtr, &close, &type);
	}

	AddDSChild(parentInfo, new_w, GetDSNumChildren(parentInfo));
	if (! (type & XmDSM_DS_LEAF))
		GetNextDS(dsm, new_w, dataPtr);
}



/*ARGSUSED*/
static XmDSInfo 
#ifdef _NO_PROTO
ReadTree( dsm, dataPtr )
        XmDropSiteManagerObject dsm ;
        XtPointer dataPtr ;
#else
ReadTree(
        XmDropSiteManagerObject dsm,
        XtPointer dataPtr )
#endif /* _NO_PROTO */
{
	Boolean junkb;
	unsigned char junkc;

	XmDSInfo root = GetDSFromStream(dsm, dataPtr, &junkb, &junkc);
	SetDSShell(root, True);
	GetNextDS(dsm, root, dataPtr);
	return root;
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
FreeDSTree( tree )
        XmDSInfo tree ;
#else
FreeDSTree(
        XmDSInfo tree )
#endif /* _NO_PROTO */
{
	int i;
	XmDSInfo child;

	if (!GetDSLeaf(tree))
		for (i = 0; i < GetDSNumChildren(tree); i++)
		{
			child = (XmDSInfo) GetDSChild(tree, i);
			FreeDSTree(child);
		}
	DestroyDSInfo(tree, True);
}

static void 
#ifdef _NO_PROTO
ChangeRoot( dsm, clientData, callData )
        XmDropSiteManagerObject dsm ;
        XtPointer clientData ;
        XtPointer callData ;
#else
ChangeRoot(
        XmDropSiteManagerObject dsm,
        XtPointer clientData,
        XtPointer callData )
#endif /* _NO_PROTO */
{
	XmDragTopLevelClientData cd = 
		(XmDragTopLevelClientData) clientData;
	XmTopLevelEnterCallback callback = 
		(XmTopLevelEnterCallback) callData;
	Widget		newRoot = cd->destShell;
	XtPointer	dataPtr = cd->iccInfo;

	dsm->dropManager.curTime = callback->timeStamp;

	if (callback->reason == XmCR_TOP_LEVEL_ENTER)
	{
		/*
		 * We assume that the drag context will not change without
		 * a call to change the root.
		 */
		dsm->dropManager.curDragContext = (Widget) XmGetDragContext(
			(Widget)dsm, callback->timeStamp);

		if (newRoot)
		{
			dsm->dropManager.dsRoot = DSMWidgetToInfo(dsm, newRoot);
			/*
			 * Do we need to do anything for prereg emulation of dyn?
			 */
		}
		else
		{
			dsm->dropManager.dsRoot =
				(XtPointer) ReadTree(dsm, dataPtr);
		}

		dsm->dropManager.rootX = cd->xOrigin;
		dsm->dropManager.rootY = cd->yOrigin;
		dsm->dropManager.rootW = cd->width;
		dsm->dropManager.rootH = cd->height;
	}
	else if (dsm->dropManager.dsRoot)/* XmCR_TOP_LEVEL_LEAVE */
	{
		if (dsm->dropManager.curInfo != NULL)
		{
			XmDragMotionCallbackStruct cbRec ;
			XmDragMotionClientDataStruct cdRec ;
			unsigned char style = _XmGetActiveProtocolStyle(
				dsm->dropManager.curDragContext);

			/* Fake out a motion message from the DragC */
			cbRec.reason = XmCR_DROP_SITE_LEAVE;
			cbRec.event = callback->event;
			cbRec.timeStamp = callback->timeStamp;
			cbRec.x = dsm->dropManager.curX;
			cbRec.y = dsm->dropManager.curY;

			/* These fields are irrelevant on a leave */
			cbRec.operations = cbRec.operation = 0;
			cbRec.dropSiteStatus = 0;

			/* Need these too */
			cdRec.window = cd->window;
			cdRec.dragOver = cd->dragOver;
			
			HandleLeave(dsm, &cdRec, &cbRec,
				    (XmDSInfo) dsm->dropManager.curInfo,
				    style, False);

			dsm->dropManager.curInfo = NULL;
		}

		if (GetDSRemote((XmDSInfo)(dsm->dropManager.dsRoot)))
			FreeDSTree((XmDSInfo)dsm->dropManager.dsRoot);

		/* Invalidate the root--force errors to show themselves */
		dsm->dropManager.curDragContext = NULL;
		dsm->dropManager.dsRoot = (XtPointer) NULL;
		dsm->dropManager.rootX = (Position) -1;
		dsm->dropManager.rootY = (Position) -1;
		dsm->dropManager.rootW = 0;
		dsm->dropManager.rootH = 0;
	}
}

static int 
#ifdef _NO_PROTO
CountDropSites( info )
        XmDSInfo info ;
#else
CountDropSites(
        XmDSInfo info )
#endif /* _NO_PROTO */
{
	int i;
	XmDSInfo child;
	int acc = 1;

	if (!GetDSLeaf(info))
	{
		for (i = 0; i < GetDSNumChildren(info); i++)
		{
			child = (XmDSInfo) GetDSChild(info, i);
			acc += CountDropSites(child);
		}
	}

	return(acc);
}

static void 
#ifdef _NO_PROTO
CreateInfo( dsm, widget, args, argCount )
        XmDropSiteManagerObject dsm ;
        Widget widget ;
        ArgList args ;
        Cardinal argCount ;
#else
CreateInfo(
        XmDropSiteManagerObject dsm,
        Widget widget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	XmDSFullInfoRec fullInfoRec;
    XmDSInfo new_info, prev_info;
	XmRegion region = _XmRegionCreate();
	Widget shell = widget;
#ifndef OSF_v1_2_4
	Cardinal size0, size1, size2;
#else /* OSF_v1_2_4 */
	size_t size;
#endif /* OSF_v1_2_4 */

	DSMStartUpdate(dsm, widget);

	/* zero out the working info struct */
	memset((void *)(&fullInfoRec), 0, sizeof(fullInfoRec));

	/* Load that puppy */
	SetDSLeaf(&fullInfoRec, True);
	fullInfoRec.widget = widget;
    XtGetSubresources(widget, &fullInfoRec, NULL, NULL, _XmDSResources,
		_XmNumDSResources, args, argCount);
	
	/* Do some input validation */

	if ((fullInfoRec.activity == XmDROP_SITE_ACTIVE) &&
		(fullInfoRec.drop_proc == NULL))
	{

#ifdef I18N_MSG
		_XmWarning(widget, catgets(Xm_catd,MS_DropS,MSG_DRS_1,
		           MESSAGE4));
#else
		_XmWarning(widget, MESSAGE4);
#endif

	}
	
	if ((fullInfoRec.animation_style == XmDRAG_UNDER_PIXMAP) &&
		(fullInfoRec.animation_pixmap != XmUNSPECIFIED_PIXMAP) &&
		(fullInfoRec.animation_pixmap_depth == 0))
	{
		/*
		 * They didn't tell us the depth of the pixmaps.  We'll have
		 * to ask the server for it.  If they gave us a bogus pixmap,
		 * we'll find out here.
		 */
		
		int junkInt;
		unsigned int junkUInt;
		Window junkWin;

		XGetGeometry(XtDisplayOfObject(widget),
			fullInfoRec.animation_pixmap,
			&junkWin, &junkInt, &junkInt,
			&junkUInt, &junkUInt, &junkUInt,
			&(fullInfoRec.animation_pixmap_depth));
	}

	if ((fullInfoRec.type == XmDROP_SITE_COMPOSITE) &&
		((fullInfoRec.rectangles != NULL) ||
		(fullInfoRec.num_rectangles != 1)))
	{

#ifdef I18N_MSG
		_XmWarning(widget, catgets(Xm_catd,MS_DropS,MSG_DRS_5,
	                   MESSAGE5));
#else
		_XmWarning(widget, MESSAGE5);
#endif

		fullInfoRec.rectangles = NULL;
		fullInfoRec.num_rectangles = 1;
	}

	/* Handle the region*/
	if (fullInfoRec.rectangles == NULL)
	{
		XRectangle rect;
		Dimension bw = XtBorderWidth(widget);

		rect.x = rect.y = -bw;
		rect.width = XtWidth(widget) + (2 * bw);
		rect.height = XtHeight(widget) + (2 * bw);

		_XmRegionUnionRectWithRegion(&rect, region, region);

		fullInfoRec.region = region;

		/*
		 * Leave HasRegion == 0 indicating that we created this 
		 * region for the drop site.
		 */
	}
	else
	{
		int i;
		XRectangle *rects = fullInfoRec.rectangles;

		for (i=0; i < fullInfoRec.num_rectangles; i++)
			_XmRegionUnionRectWithRegion(&(rects[i]), region, region);

		fullInfoRec.region = region;
		fullInfoRec.status.has_region = True;
	}

	XtAddCallback(widget, XmNdestroyCallback, DestroyCallback, dsm);
	
	while(!XtIsShell(shell))
		shell = XtParent(shell);
	
	fullInfoRec.import_targets_ID = _XmTargetsToIndex(shell,
		fullInfoRec.import_targets, fullInfoRec.num_import_targets);
	
#ifndef OSF_v1_2_4
	size0 = sizeof(XmDSStatusRec);

	if (fullInfoRec.type == XmDROP_SITE_COMPOSITE)
		size1 = sizeof(XmDSLocalNodeRec);
	else
		size1 = sizeof(XmDSLocalLeafRec);
	
#endif /* OSF_v1_2_4 */
	switch(fullInfoRec.animation_style)
	{
		case XmDRAG_UNDER_PIXMAP:
#ifndef OSF_v1_2_4
			size2 = sizeof(XmDSLocalPixmapStyleRec);
#else /* OSF_v1_2_4 */
			if (fullInfoRec.type == XmDROP_SITE_COMPOSITE)
				size = sizeof(XmDSLocalPixmapNodeRec);
			else
				size = sizeof(XmDSLocalPixmapLeafRec);
#endif /* OSF_v1_2_4 */
		break;
		case XmDRAG_UNDER_HIGHLIGHT:
#ifdef animation_data_in_info_rec
#ifndef OSF_v1_2_4
			size2 = sizeof(XmDSHighlightStyleRec);
#else /* OSF_v1_2_4 */
			if (fullInfoRec.type == XmDROP_SITE_COMPOSITE)
				size = sizeof(XmDSLocalHighlightNodeRec);
			else
				size = sizeof(XmDSLocalHighlightLeafRec);
#endif /* OSF_v1_2_4 */
		break;
#endif /* animation_data_in_info_rec */
		case XmDRAG_UNDER_SHADOW_IN:
		case XmDRAG_UNDER_SHADOW_OUT:
#ifdef animation_data_in_info_rec
#ifndef OSF_v1_2_4
			size2 = sizeof(XmDSShadowStyleRec);
#else /* OSF_v1_2_4 */
			if (fullInfoRec.type == XmDROP_SITE_COMPOSITE)
				size = sizeof(XmDSLocalShadowNodeRec);
			else
				size = sizeof(XmDSLocalShadowLeafRec);
#endif /* OSF_v1_2_4 */
		break;
#endif /* animation_data_in_info_rec */
		case XmDRAG_UNDER_NONE:
		default:
#ifndef OSF_v1_2_4
			size2 = 0;
#else /* OSF_v1_2_4 */
			if (fullInfoRec.type == XmDROP_SITE_COMPOSITE)
				size = sizeof(XmDSLocalNoneNodeRec);
			else
				size = sizeof(XmDSLocalNoneLeafRec);
#endif /* OSF_v1_2_4 */
		break;
	}

#ifndef OSF_v1_2_4
	new_info = (XmDSInfo) XtCalloc(1, (size0 + size1 + size2));
#else /* OSF_v1_2_4 */
	new_info = (XmDSInfo) XtCalloc(1, size);
#endif /* OSF_v1_2_4 */

	CopyFullIntoVariant(&fullInfoRec, new_info);

	if ((prev_info = (XmDSInfo) DSMWidgetToInfo(dsm, widget)) == NULL)
	{
		DSMRegisterInfo(dsm, widget, (XtPointer) new_info);
	}
	else
	{
		if (GetDSInternal(prev_info))
		{
			/*
			 * They are registering a widget for which we already had
			 * to register internally.  The only types of widgets which
			 * we register internally are of type
			 * XmDROP_SITE_COMPOSITE with children!  This means that
			 * they are trying to create their drop sites out of order
			 * (parents must be registered before their children).
			 */
			

#ifdef I18N_MSG
			_XmWarning(widget, catgets(Xm_catd,MS_DropS,MSG_DRS_6,
			           MESSAGE6));
#else
			_XmWarning(widget, MESSAGE6);
#endif

		}
		else
		{

#ifdef I18N_MSG
			_XmWarning(widget, catgets(Xm_catd,MS_DropS,MSG_DRS_7,
				   MESSAGE7));
#else
			_XmWarning(widget, MESSAGE7);
#endif

		}

		DestroyDSInfo(new_info, True);
		return;
	}
    
	DSMInsertInfo(dsm, (XtPointer) new_info, NULL);

	DSMEndUpdate(dsm, widget);
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
CopyVariantIntoFull( dsm, variant, full_info )
        XmDropSiteManagerObject dsm ;
        XmDSInfo variant ;
        XmDSFullInfo full_info ;
#else
CopyVariantIntoFull(
        XmDropSiteManagerObject dsm,
        XmDSInfo variant,
        XmDSFullInfo full_info )
#endif /* _NO_PROTO */
{
	Widget shell;
	Atom *targets;
	Cardinal num_targets;
	long num_rects;
	XRectangle *rects;

	if (GetDSRemote(variant))
		shell = XtParent(dsm);
	else
		shell = GetDSWidget(variant);

	while (!XtIsShell(shell))
		shell = XtParent(shell);

	/*
	 * Clear the full info back to the default (kind of) state.
	 */
	memset((void *)(full_info), 0, sizeof(XmDSFullInfoRec));
	full_info->animation_pixmap = XmUNSPECIFIED_PIXMAP;
	full_info->animation_mask = XmUNSPECIFIED_PIXMAP;

	/* Structure copy the status stuff across */
	full_info->status = variant->status;

	full_info->parent = GetDSParent(variant);
	full_info->import_targets_ID = GetDSImportTargetsID(variant);
	full_info->operations = GetDSOperations(variant);
	full_info->region = GetDSRegion(variant);
	full_info->drag_proc = GetDSDragProc(variant);
	full_info->drop_proc = GetDSDropProc(variant);

	full_info->widget = GetDSWidget(variant);

	full_info->type = GetDSType(variant);
	full_info->animation_style = GetDSAnimationStyle(variant);
	full_info->activity = GetDSActivity(variant);

	num_targets = _XmIndexToTargets(shell,
		GetDSImportTargetsID(variant), &targets);
	full_info->num_import_targets = num_targets;
	full_info->import_targets = targets;

	_XmRegionGetRectangles(GetDSRegion(variant), &rects, &num_rects);
	full_info->rectangles = rects;
	full_info->num_rectangles = (Cardinal) num_rects;

	if (GetDSRemote(variant))
	{
		switch(GetDSAnimationStyle(variant))
		{
			case XmDRAG_UNDER_HIGHLIGHT:
			{
				XmDSRemoteHighlightStyle hs =
					(XmDSRemoteHighlightStyle)
						GetDSRemoteAnimationPart(variant);
				
				full_info->highlight_color = hs->highlight_color;
				full_info->highlight_pixmap = hs->highlight_pixmap;
				full_info->background = hs->background;
				full_info->highlight_thickness =
					hs->highlight_thickness;
				full_info->border_width = hs->border_width;
			}
			break;
			case XmDRAG_UNDER_SHADOW_IN:
			case XmDRAG_UNDER_SHADOW_OUT:
			{
				XmDSRemoteShadowStyle ss =
					(XmDSRemoteShadowStyle)
						GetDSRemoteAnimationPart(variant);
				
				full_info->top_shadow_color = ss->top_shadow_color;
				full_info->top_shadow_pixmap = ss->top_shadow_pixmap;
				full_info->bottom_shadow_color =
					ss->bottom_shadow_color;
				full_info->bottom_shadow_pixmap =
					ss->bottom_shadow_pixmap;
				full_info->foreground = ss->foreground;
				full_info->shadow_thickness = ss->shadow_thickness;
				full_info->highlight_thickness = ss->highlight_thickness;
				full_info->border_width = ss->border_width;
			}
			break;
			case XmDRAG_UNDER_PIXMAP:
			{
				XmDSRemotePixmapStyle ps =
					(XmDSRemotePixmapStyle)
						GetDSRemoteAnimationPart(variant);

				full_info->animation_pixmap = ps->animation_pixmap;
				full_info->animation_pixmap_depth =
					ps->animation_pixmap_depth;
				full_info->animation_mask = ps->animation_mask;
				full_info->background = ps->background;
				full_info->foreground = ps->foreground;
				full_info->shadow_thickness = ps->shadow_thickness;
				full_info->highlight_thickness =
					ps->highlight_thickness;
				full_info->border_width = ps->border_width;
			}
			break;
			case XmDRAG_UNDER_NONE:
			default:
			break;
		}
	}
	else
	{
		switch(GetDSAnimationStyle(variant))
		{
			case XmDRAG_UNDER_HIGHLIGHT:
#ifdef animation_data_in_info_rec
			{
				XmDSHighlightStyle hs =
					(XmDSHighlightStyle)
						GetDSLocalAnimationPart(variant);
				
				full_info->highlight_color = hs->highlight_color;
				full_info->highlight_pixmap = hs->highlight_pixmap;
				full_info->background = hs->background;
				full_info->highlight_thickness =
					hs->highlight_thickness;
				full_info->border_width = hs->border_width;
			}
#endif /* animation_data_in_info_rec */
			break;
			case XmDRAG_UNDER_SHADOW_IN:
			case XmDRAG_UNDER_SHADOW_OUT:
#ifdef animation_data_in_info_rec
			{
				XmDSShadowStyle ss =
					(XmDSShadowStyle) GetDSLocalAnimationPart(variant);
				
				full_info->top_shadow_color = ss->top_shadow_color;
				full_info->top_shadow_pixmap = ss->top_shadow_pixmap;
				full_info->bottom_shadow_color =
					ss->bottom_shadow_color;
				full_info->bottom_shadow_pixmap =
					ss->bottom_shadow_pixmap;
				full_info->foreground = ss->foreground;
				full_info->shadow_thickness = ss->shadow_thickness;
				full_info->highlight_thickness = ss->highlight_thickness;
				full_info->border_width = ss->border_width;
			}
#endif /* animation_data_in_info_rec */
			break;
			case XmDRAG_UNDER_PIXMAP:
			{
				XmDSLocalPixmapStyle ps =
					(XmDSLocalPixmapStyle)
						GetDSLocalAnimationPart(variant);

				full_info->animation_pixmap = ps->animation_pixmap;
				full_info->animation_pixmap_depth =
					ps->animation_pixmap_depth;
				full_info->animation_mask = ps->animation_mask;
#ifdef animation_data_in_info_rec
				full_info->background = ps->background;
				full_info->foreground = ps->foreground;
				full_info->shadow_thickness = ps->shadow_thickness;
				full_info->highlight_thickness =
					ps->highlight_thickness;
				full_info->border_width = ps->border_width;
#endif /* animation_data_in_info_rec */
			}
			break;
			case XmDRAG_UNDER_NONE:
			default:
			break;
		}
	}
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
RetrieveInfo( dsm, widget, args, argCount )
        XmDropSiteManagerObject dsm ;
        Widget widget ;
        ArgList args ;
        Cardinal argCount ;
#else
RetrieveInfo(
        XmDropSiteManagerObject dsm,
        Widget widget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	XmDSFullInfoRec full_info_rec;
	XmDSInfo	info;
	XRectangle	*rects;

	if (XmIsDragContext(widget))
	{
		if (widget != dsm->dropManager.curDragContext)
			return;
		else
			info = (XmDSInfo) (dsm->dropManager.curInfo);
	}
	else
		info = (XmDSInfo) DSMWidgetToInfo(dsm, widget);
	
	if (info == NULL)
		return;

	CopyVariantIntoFull(dsm, info, &full_info_rec);

	XtGetSubvalues((XtPointer)(&full_info_rec),
		(XtResourceList)(_XmDSResources), (Cardinal)(_XmNumDSResources),
		(ArgList)(args), (Cardinal)(argCount));

	rects = full_info_rec.rectangles;

	if (rects)
		XtFree((char *) rects);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
CopyFullIntoVariant( full_info, variant )
        XmDSFullInfo full_info ;
        XmDSInfo variant ;
#else
CopyFullIntoVariant(
        XmDSFullInfo full_info,
        XmDSInfo variant )
#endif /* _NO_PROTO */
{
#ifdef animation_data_in_info_rec
	Widget widget;
#endif /* animation_data_in_info_rec */
	/*
	 * This procedure assumes that variant is a variant of Local,
	 * there should be no calls to this procedure with variant of
	 * remote.
	 */
	if (GetDSRemote(full_info))
		return;
	
#ifdef animation_data_in_info_rec
	widget = full_info->widget;
#endif /* animation_data_in_info_rec */

	/* Magic internal fields */
	SetDSRemote(variant, GetDSRemote(full_info));
	SetDSLeaf(variant, GetDSLeaf(full_info));
	SetDSShell(variant, GetDSShell(full_info));
	SetDSHasRegion(variant, full_info->status.has_region);

	/* Externally visible fields */
	SetDSAnimationStyle(variant, full_info->animation_style);
	SetDSType(variant, full_info->type);
	SetDSActivity(variant, full_info->activity);

	SetDSImportTargetsID(variant, full_info->import_targets_ID);
	SetDSOperations(variant, full_info->operations);
	SetDSRegion(variant, full_info->region);
	SetDSDragProc(variant, full_info->drag_proc);
	SetDSDropProc(variant, full_info->drop_proc);
	SetDSWidget(variant, full_info->widget);

	switch(full_info->animation_style)
	{
		case XmDRAG_UNDER_HIGHLIGHT:
#ifdef animation_data_in_info_rec
		{
			XmDSHighlightStyle hs =
				(XmDSHighlightStyle) GetDSLocalAnimationPart(variant);
			
			hs->highlight_color = full_info->highlight_color;
			hs->highlight_pixmap = full_info->highlight_pixmap;
			if (XtIsWidget(widget))
				hs->background = widget->core.background_pixel;
			else
				hs->background =
					widget->core.parent->core.background_pixel;
			hs->highlight_thickness = full_info->highlight_thickness;
			if (!GetDSHasRegion(full_info))
				hs->border_width = XtBorderWidth(widget);
		}
#endif /* animation_data_in_info_rec */
		break;
		case XmDRAG_UNDER_SHADOW_IN:
		case XmDRAG_UNDER_SHADOW_OUT:
#ifdef animation_data_in_info_rec
		{
			XmDSShadowStyle ss =
				(XmDSShadowStyle) GetDSLocalAnimationPart(variant);
			
			ss->top_shadow_color = full_info->top_shadow_color;
			ss->top_shadow_pixmap = full_info->top_shadow_pixmap;
			ss->bottom_shadow_color = full_info->bottom_shadow_color;
			ss->bottom_shadow_pixmap = full_info->bottom_shadow_pixmap;

			if (XmIsPrimitive(widget))
				ss->foreground = ((XmPrimitiveWidget) widget)->
					primitive.foreground;
			else if (XmIsGadget(widget))
				ss->foreground =
					((XmManagerWidget)(widget->core.parent))->
						manager.foreground;
			else
				ss->foreground = 0;

			ss->shadow_thickness = full_info->shadow_thickness;
			ss->highlight_thickness = full_info->highlight_thickness;
			if (!GetDSHasRegion(full_info))
				ss->border_width = XtBorderWidth(widget);
		}
#endif /* animation_data_in_info_rec */
		break;
		case XmDRAG_UNDER_PIXMAP:
		{
			XmDSLocalPixmapStyle ps =
				(XmDSLocalPixmapStyle) GetDSLocalAnimationPart(variant);
			
			ps->animation_pixmap = full_info->animation_pixmap;
			ps->animation_pixmap_depth =
				full_info->animation_pixmap_depth;
			ps->animation_mask = full_info->animation_mask;
#ifdef animation_data_in_info_rec
			if (XtIsWidget(widget))
				ps->background = widget->core.background_pixel;
			else
				ps->background = 
					widget->core.parent->core.background_pixel;
			if (XmIsPrimitive(widget))
				ps->foreground = ((XmPrimitiveWidget)widget)->
					primitive.foreground;
			else if (XmIsGadget(widget))
				ps->foreground =
					((XmManagerWidget)(widget->core.parent))->
						manager.foreground;
			else
				ps->foreground = 0;

			ps->shadow_thickness = full_info->shadow_thickness;
			ps->highlight_thickness = full_info->highlight_thickness;
			if (!GetDSHasRegion(full_info))
				ps->border_width = XtBorderWidth(widget);
#endif /* animation_data_in_info_rec */
		}
		break;
		case XmDRAG_UNDER_NONE:
		default:
		break;
	}
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
UpdateInfo( dsm, widget, args, argCount )
        XmDropSiteManagerObject dsm ;
        Widget widget ;
        ArgList args ;
        Cardinal argCount ;
#else
UpdateInfo(
        XmDropSiteManagerObject dsm,
        Widget widget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	XmDSFullInfoRec	full_info_rec;
	XmDSFullInfo	full_info = &full_info_rec;
	XmDSInfo	info = (XmDSInfo) DSMWidgetToInfo(dsm, widget);
	unsigned char	type;
	XmRegion	old_region;
	XRectangle	*rects;
	long		num_rects;

	if ((info == NULL) || GetDSInternal(info))
		return;

        rects = NULL;

	DSMStartUpdate(dsm, widget);

	CopyVariantIntoFull(dsm, info, full_info);

	/* Save the type and region in case they try to cheat */
	type = GetDSType(info);
	old_region = GetDSRegion(info);

/* BEGIN OSF Fix CR 5335 */
	/*
	 * Set up the rectangle list stuff.  
	 */
	rects = full_info->rectangles;
	num_rects = (long) full_info->num_rectangles;
/* END OSF Fix CR 5335 */

	/* Update the info */
	XtSetSubvalues(full_info, _XmDSResources, _XmNumDSResources,
		args, argCount);

	if (full_info->type != type)
	{

#ifdef I18N_MSG
		_XmWarning(widget, catgets(Xm_catd,MS_DropS,MSG_DRS_8,
			   MESSAGE8));
#else
		_XmWarning(widget, MESSAGE8);
#endif

		full_info->type = type;
	}

	if ((full_info->rectangles != rects) ||
		(full_info->num_rectangles != num_rects))
	{
		if (type == XmDROP_SITE_SIMPLE)
		{
			int i;
			XmRegion new_region = _XmRegionCreate();

			for (i=0; i < full_info->num_rectangles; i++)
				_XmRegionUnionRectWithRegion(
					&(full_info->rectangles[i]), new_region,
					new_region);
			
			full_info->region = new_region;
			full_info->status.has_region = True;
			_XmRegionDestroy(old_region);
		}
		else
		{

#ifdef I18N_MSG
			_XmWarning(widget, catgets(Xm_catd,MS_DropS,MSG_DRS_9,
		 		   MESSAGE9));
#else
			_XmWarning(widget, MESSAGE9);
#endif

		}

/* BEGIN OSF Fix CR 5335 */
/* END OSF Fix CR 5335 */
	}

	if ((full_info->animation_style == XmDRAG_UNDER_PIXMAP) &&
		(full_info->animation_pixmap_depth == 0))
	{
		/*
		 *If it is not equal to zero, we're just going to trust them.
		 */
		int junkInt;
		unsigned int junkUInt;
		Window junkWin;
		Widget widget = GetDSWidget(info);

		XGetGeometry(XtDisplayOfObject(widget),
			full_info->animation_pixmap,
			&junkWin, &junkInt, &junkInt,
			&junkUInt, &junkUInt, &junkUInt,
			&(full_info->animation_pixmap_depth));
	}

	/*
	 * If the animation style has changed, we need to change info
	 * into a different variant.
	 */
	
	if (full_info->animation_style != GetDSAnimationStyle(info))
	{
		XmDSInfo new_info;
#ifndef OSF_v1_2_4
		Cardinal size0, size1, size2;
#else /* OSF_v1_2_4 */
		size_t size;
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
		size0 = sizeof(XmDSStatusRec);

		if (GetDSType(info) == XmDROP_SITE_COMPOSITE)
			size1 = sizeof(XmDSLocalNodeRec);
		else
			size1 = sizeof(XmDSLocalLeafRec);
		
#endif /* OSF_v1_2_4 */
		switch (full_info->animation_style)
		{
			case XmDRAG_UNDER_PIXMAP:
#ifndef OSF_v1_2_4
				size2 = sizeof(XmDSLocalPixmapStyleRec);
#else /* OSF_v1_2_4 */
				if (GetDSType(info) == XmDROP_SITE_COMPOSITE)
					size = sizeof(XmDSLocalPixmapNodeRec);
				else
					size = sizeof(XmDSLocalPixmapLeafRec);
#endif /* OSF_v1_2_4 */
			break;
			case XmDRAG_UNDER_HIGHLIGHT:
#ifdef animation_data_in_info_rec
#ifndef OSF_v1_2_4
				size2 = sizeof(XmDSHighlightStyleRec);
#else /* OSF_v1_2_4 */
				if (GetDSType(info) == XmDROP_SITE_COMPOSITE)
					size = sizeof(XmDSLocalHighlightNodeRec);
				else
					size = sizeof(XmDSLocalHighlightLeafRec);
#endif /* OSF_v1_2_4 */
			break;
#endif /* animation_data_in_info_rec */
			case XmDRAG_UNDER_SHADOW_IN:
			case XmDRAG_UNDER_SHADOW_OUT:
#ifdef animation_data_in_info_rec
#ifndef OSF_v1_2_4
				size2 = sizeof(XmDSShadowStyleRec);
#else /* OSF_v1_2_4 */
				if (GetDSType(info) == XmDROP_SITE_COMPOSITE)
					size = sizeof(XmDSLocalShadowNodeRec);
				else
					size = sizeof(XmDSLocalShadowLeafRec);
#endif /* OSF_v1_2_4 */
			break;
#endif /* animation_data_in_info_rec */
			case XmDRAG_UNDER_NONE:
			default:
#ifndef OSF_v1_2_4
				size2 = 0;
#else /* OSF_v1_2_4 */
				if (GetDSType(info) == XmDROP_SITE_COMPOSITE)
					size = sizeof(XmDSLocalNoneNodeRec);
				else
					size = sizeof(XmDSLocalNoneLeafRec);
#endif /* OSF_v1_2_4 */
			break;
		}

		/* Allocate the new info rec */
#ifndef OSF_v1_2_4
		new_info = (XmDSInfo) XtCalloc(1, (size0 + size1 + size2));
#else /* OSF_v1_2_4 */
		new_info = (XmDSInfo) XtCalloc(1, size);
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
		/*
		 * Load the new one from the old one and from the result
		 * of the arg list processing.  (size0 and size1 will be the
		 * same for both new and old
		 */
#else /* OSF_v1_2_4 */
		/*
		 * Load the new one from the old one and from the result
		 * of the arg list processing.
		 */
#endif /* OSF_v1_2_4 */
#ifndef OSF_v1_2_4
		memcpy(new_info, info, (size0 + size1));
#endif /* OSF_v1_2_4 */

		CopyFullIntoVariant(full_info, new_info);

#ifdef OSF_v1_2_4
		SetDSNumChildren(new_info, GetDSNumChildren(info));
		SetDSChildren(new_info, GetDSChildren(info));

#endif /* OSF_v1_2_4 */
		/*
		 * Fix the parent pointers of the children
		 */
		if ((GetDSType(new_info) == XmDROP_SITE_COMPOSITE) &&
			(GetDSNumChildren(new_info)))
		{
			XmDSInfo child;
			int i;

			for (i=0; i < GetDSNumChildren(new_info); i++)
			{
				child = (XmDSInfo) GetDSChild(new_info, i);
				SetDSParent(child, new_info);
			}
		}

		/* Clear the registered bit on the new one */
		SetDSRegistered(new_info, False);

		/* Remove the old one from the hash table */
		DSMUnregisterInfo(dsm, info);

		/* Replace the old one in the drop site tree */
		ReplaceDSChild(info, new_info);

		/* Destroy the old one, but not anything it points to */
		DestroyDSInfo(info, False);

		/* Register the new one */
		DSMRegisterInfo(dsm, widget, (XtPointer) new_info);
	}
	else
	{
		CopyFullIntoVariant(full_info, info);
	}

	DSMEndUpdate(dsm, widget);

	if (rects!=NULL) XtFree ((char *)rects);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
StartUpdate( dsm, refWidget )
        XmDropSiteManagerObject dsm ;
        Widget refWidget ;
#else
StartUpdate(
        XmDropSiteManagerObject dsm,
        Widget refWidget )
#endif /* _NO_PROTO */
{
	Widget shell = refWidget;
	XmDSInfo shellInfo;

	while(!(XtIsShell(shell)))
		shell = XtParent(shell);
	
	shellInfo = (XmDSInfo) DSMWidgetToInfo(dsm, shell);

	if (shellInfo)
		SetDSUpdateLevel(shellInfo, (GetDSUpdateLevel(shellInfo) + 1));
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
EndUpdate( dsm, refWidget )
        XmDropSiteManagerObject dsm ;
        Widget refWidget ;
#else
EndUpdate(
        XmDropSiteManagerObject dsm,
        Widget refWidget )
#endif /* _NO_PROTO */
{
  _XmDropSiteUpdateInfo dsupdate, oldupdate;
  Boolean found = False;
  Boolean clean;
  Widget shell;
  XmDSInfo shellInfo;

  dsupdate = dsm -> dropManager.updateInfo;
  clean = (dsupdate == NULL);

  shell = refWidget;

  while(!(XtIsShell(shell)))
    shell = XtParent(shell);
  
  shellInfo = (XmDSInfo) DSMWidgetToInfo(dsm, shell);

  if (shellInfo == NULL) return;
	
  if (GetDSUpdateLevel(shellInfo) > 0)
    SetDSUpdateLevel(shellInfo, (GetDSUpdateLevel(shellInfo) - 1));

  if (GetDSUpdateLevel(shellInfo) > 0) return;

  /* Fix CR 7976,  losing track of some updates because of bad
     list manipulation */
  oldupdate = dsupdate;

  /* Really,  keep track of toplevel widgets to be updated */
  while(dsupdate) {
    if (dsupdate -> refWidget == shell) {
      found = True;
      break;
    }
#ifndef OSF_v1_2_4
    dsupdate = (_XmDropSiteUpdateInfo) dsupdate -> next;
#else /* OSF_v1_2_4 */
    dsupdate = dsupdate -> next;
#endif /* OSF_v1_2_4 */
  }

  if (! found) {
    /* Queue real end update to a timeout */
    dsupdate = (_XmDropSiteUpdateInfo) 
      XtMalloc(sizeof(_XmDropSiteUpdateInfoRec));
    dsupdate -> dsm = dsm;
    dsupdate -> refWidget = shell;
#ifndef OSF_v1_2_4
    dsupdate -> next = (XtPointer) oldupdate;
#else /* OSF_v1_2_4 */
    dsupdate -> next = oldupdate;
#endif /* OSF_v1_2_4 */
    dsm -> dropManager.updateInfo = dsupdate;
  }

  /* We don't add a timeout if the record is already marked for update */
  if (clean) {
    XtAppAddTimeOut(XtWidgetToApplicationContext(shell), 0,
		    _XmIEndUpdate, dsm);
  }
}

/*ARGSUSED*/
void
#ifdef _NO_PROTO
_XmIEndUpdate(client_data, interval_id)
     XtPointer client_data;
     XtIntervalId *interval_id;
#else
_XmIEndUpdate(XtPointer client_data, XtIntervalId *interval_id)
#endif /* _NO_PROTO */
{
        XmDropSiteManagerObject dsm = (XmDropSiteManagerObject) client_data;
        _XmDropSiteUpdateInfo dsupdate;
        Widget refWidget;
	Widget shell;
	XmDSInfo shellInfo;

	/* Return if all updates have already happened */
	while(dsm -> dropManager.updateInfo != NULL) {
	dsupdate = (_XmDropSiteUpdateInfo) dsm -> dropManager.updateInfo;
	shell = refWidget = dsupdate -> refWidget;
	dsm -> dropManager.updateInfo = dsupdate -> next;
	XtFree((XtPointer) dsupdate);

	while(!(XtIsShell(shell)))
		shell = XtParent(shell);
	
	shellInfo = (XmDSInfo) DSMWidgetToInfo(dsm, shell);

	if (shellInfo == NULL) return;
	
	if (XtIsRealized(shell))
	{
		/* This one's for real */

		if (_XmGetDragProtocolStyle(shell) != XmDRAG_DYNAMIC)
		{
			XmDropSiteTreeAddCallbackStruct	outCB;

			/* Gotta' update that window property. */
			
			outCB.reason = XmCR_DROP_SITE_TREE_ADD;
			outCB.event = NULL;
			outCB.rootShell = shell;
			outCB.numDropSites = CountDropSites(shellInfo);
			outCB.numArgsPerDSHint = 0;

			if (dsm->dropManager.treeUpdateProc)
				(dsm->dropManager.treeUpdateProc)
					((Widget) dsm, NULL, (XtPointer) &outCB);
		}
		else
		{
			/* We have to Sync the regions with the widgets */
			SyncTree(dsm, shell);
		}
	}
    }
}

static void 
#ifdef _NO_PROTO
DestroyInfo( dsm, widget )
        XmDropSiteManagerObject dsm ;
        Widget widget ;
#else
DestroyInfo(
        XmDropSiteManagerObject dsm,
        Widget widget )
#endif /* _NO_PROTO */
{
	XmDSInfo info = (XmDSInfo) DSMWidgetToInfo(dsm, widget);

	if (info == NULL)
		return;

	DSMStartUpdate(dsm, widget);

	if (info == (XmDSInfo) (dsm->dropManager.curInfo))
	{
		Widget shell;
		XmDragMotionCallbackStruct cbRec ;
		XmDragMotionClientDataStruct cdRec ;
		unsigned char style = _XmGetActiveProtocolStyle(
			dsm->dropManager.curDragContext);

		/* Fake out a motion message from the DragC */
		cbRec.reason = XmCR_DROP_SITE_LEAVE;
		cbRec.event = NULL;
		cbRec.timeStamp = dsm->dropManager.curTime;
		cbRec.x = dsm->dropManager.curX;
		cbRec.y = dsm->dropManager.curY;

		/* These fields are irrelevant on a leave */
		cbRec.operations = cbRec.operation = 0;
		cbRec.dropSiteStatus = 0;

		/* Need these too */
		shell = GetDSWidget(info);

		while (!XtIsShell(shell))
			shell = XtParent(shell);

		cdRec.window = XtWindow(shell);
		cdRec.dragOver = (Widget)
			(((XmDragContext)(dsm->dropManager.curDragContext))
				->drag.curDragOver);
		
		HandleLeave(dsm, &cdRec, &cbRec,
			    (XmDSInfo) dsm->dropManager.curInfo, style, False);

		dsm->dropManager.curInfo = NULL;
	}

	DSMRemoveInfo(dsm, (XtPointer) info);

	DestroyDSInfo(info, True);

	DSMEndUpdate(dsm, widget);
}

static void 
#ifdef _NO_PROTO
SyncDropSiteGeometry( dsm, info )
        XmDropSiteManagerObject dsm ;
        XmDSInfo info ;
#else
SyncDropSiteGeometry(
        XmDropSiteManagerObject dsm,
        XmDSInfo info )
#endif /* _NO_PROTO */
{
	XmDSInfo child;

	if (!GetDSLeaf(info))
	{
		int i;

		for (i = 0; i < GetDSNumChildren(info); i++)
		{
			child = (XmDSInfo) GetDSChild(info, i);
			SyncDropSiteGeometry(dsm, child);
		}
	}

	if (!GetDSHasRegion(info))
	{
		Widget w = GetDSWidget(info);
		XRectangle rect;
		Dimension bw = XtBorderWidth(w);

		/* The region is the object rectangle */

		/* assert(GetDSRegion(info) != NULL)  */

		/* The region comes from the widget */

		rect.x = rect.y = -bw;
		rect.width = XtWidth(w) + (2 * bw);
		rect.height = XtHeight(w) + (2 * bw);

		_XmRegionClear(GetDSRegion(info));
		_XmRegionUnionRectWithRegion(&rect, GetDSRegion(info),
			GetDSRegion(info));
	}
}

static void 
#ifdef _NO_PROTO
SyncTree( dsm, shell )
        XmDropSiteManagerObject dsm ;
        Widget shell ;
#else
SyncTree(
        XmDropSiteManagerObject dsm,
        Widget shell )
#endif /* _NO_PROTO */
{
	XmDSInfo saveRoot;
	XmDSInfo root = (XmDSInfo) DSMWidgetToInfo(dsm, shell);
	Position shellX, shellY, savX, savY;

	if ((root == NULL) || (GetDSRemote(root)))
		return;
	
	/*
	 * Set things up so that the shell coordinates are trivially
	 * available.
	 */
	saveRoot = (XmDSInfo) dsm->dropManager.dsRoot;
	savX = dsm->dropManager.rootX;
	savY = dsm->dropManager.rootY;

	dsm->dropManager.dsRoot = (XtPointer) root;
	XtTranslateCoords(GetDSWidget(root), 0, 0, &shellX, &shellY);
	dsm->dropManager.rootX = shellX;
	dsm->dropManager.rootY = shellY;

	/* Do the work */
	RemoveAllClippers(dsm, root);
	SyncDropSiteGeometry(dsm, root);
	DetectAndInsertAllClippers(dsm, root);

	/* Restore the DSM */
	dsm->dropManager.dsRoot = (XtPointer) saveRoot;
	dsm->dropManager.rootX = savX;
	dsm->dropManager.rootY = savY;
}

void 
#ifdef _NO_PROTO
_XmSyncDropSiteTree(shell)
        Widget shell;
#else
_XmSyncDropSiteTree(
        Widget shell )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(shell)));

	DSMSyncTree(dsm, shell);
}



static void 
#ifdef _NO_PROTO
Update( dsm, clientData, callData )
        XmDropSiteManagerObject dsm ;
        XtPointer clientData ;
        XtPointer callData ;
#else
Update(
        XmDropSiteManagerObject dsm,
        XtPointer clientData,
        XtPointer callData )
#endif /* _NO_PROTO */
{
	XmAnyCallbackStruct	*callback = (XmAnyCallbackStruct *)callData;

	switch(callback->reason)
	{
		case XmCR_TOP_LEVEL_ENTER:
		case XmCR_TOP_LEVEL_LEAVE:
			DSMChangeRoot(dsm, clientData, callData);
		break;
		case XmCR_DRAG_MOTION:
			DSMProcessMotion(dsm, clientData, callData);
		break;
		case XmCR_DROP_START:
			DSMProcessDrop(dsm, clientData, callData);
		break;
		case XmCR_OPERATION_CHANGED:
			DSMOperationChanged(dsm, clientData, callData);
		default:
		break;
	}
}

void 
#ifdef _NO_PROTO
_XmDSMUpdate( dsm, clientData, callData )
        XmDropSiteManagerObject dsm ;
        XtPointer clientData ;
        XtPointer callData ;
#else
_XmDSMUpdate(
        XmDropSiteManagerObject dsm,
        XtPointer clientData,
        XtPointer callData )
#endif /* _NO_PROTO */
{
	DSMUpdate(dsm, clientData, callData);
}


int 
#ifdef _NO_PROTO
_XmDSMGetTreeFromDSM( dsm, shell, dataPtr )
        XmDropSiteManagerObject dsm ;
        Widget shell ;
        XtPointer dataPtr ;
#else
_XmDSMGetTreeFromDSM(
        XmDropSiteManagerObject dsm,
        Widget shell,
        XtPointer dataPtr )
#endif /* _NO_PROTO */
{
	return(DSMGetTreeFromDSM(dsm, shell, dataPtr));
}

Boolean
#ifdef _NO_PROTO
_XmDropSiteShell(widget)
	Widget widget;
#else
_XmDropSiteShell(
        Widget widget )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));

	if ((XtIsShell(widget)) && (DSMWidgetToInfo(dsm, widget) != NULL))
		return(True);
	else
		return(False);
}

static Boolean
#ifdef _NO_PROTO
HasDropSiteDescendant(dsm, widget)
	XmDropSiteManagerObject dsm ;
	Widget widget ;
#else
HasDropSiteDescendant(
	XmDropSiteManagerObject dsm,
	Widget widget )
#endif /* _NO_PROTO */
{
	CompositeWidget cw;
	int i;
	Widget child;

	if (!XtIsComposite(widget))
		return(False);
	
	cw = (CompositeWidget) widget;
	for (i = 0; i < cw->composite.num_children; i++)
	{
		child = cw->composite.children[i];
		if ((DSMWidgetToInfo(dsm, child) != NULL) ||
			(HasDropSiteDescendant(dsm, child)))
		{
			return(True);
		}
	}

	return(False);
}

Boolean
#ifdef _NO_PROTO
_XmDropSiteWrapperCandidate(widget)
	Widget widget;
#else
_XmDropSiteWrapperCandidate(
        Widget widget )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));
	Widget shell;

	if (widget == NULL)
		return(False);
	
	if (DSMWidgetToInfo(dsm, widget) != NULL)
		return(True);
	else if (!XtIsComposite(widget))
		return(False);

	/*
	 * Make sure that there might be a drop site somewhere in
	 * this shell before traversing the descendants.
	 */
	shell = widget;
	while (!XtIsShell(shell))
		shell = XtParent(shell);
	
	if (!_XmDropSiteShell(shell))
		return(False);
	
	return(HasDropSiteDescendant(dsm, widget));
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DestroyCallback( widget, client_data, call_data )
        Widget widget ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
DestroyCallback(
        Widget widget,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject) client_data;

	DSMDestroyInfo(dsm, widget);

	/* Force Update */
	_XmIEndUpdate((XtPointer) dsm, (XtIntervalId *) NULL);
}

void 
#ifdef _NO_PROTO
XmDropSiteRegister( widget, args, argCount )
        Widget widget ;
        ArgList args ;
        Cardinal argCount ;
#else
XmDropSiteRegister(
        Widget widget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));

#ifdef NON_OSF_FIX	/* OSF Contact Number 19146 */
	if (XtIsShell(widget)) {
#ifdef I18N_MSG
	    _XmWarning(widget, catgets(Xm_catd,MS_DropS,MSG_DRS_1, MESSAGE1));
#else
	    _XmWarning(widget, MESSAGE1);
#endif
	}
	else
#endif
	    DSMCreateInfo(dsm, widget, args, argCount);
}

void 
#ifdef _NO_PROTO
XmDropSiteUnregister( widget )
        Widget widget ;
#else
XmDropSiteUnregister(
        Widget widget )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));

	DSMDestroyInfo(dsm, widget);
}

void 
#ifdef _NO_PROTO
XmDropSiteStartUpdate( refWidget )
        Widget refWidget ;
#else
XmDropSiteStartUpdate(
        Widget refWidget )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(refWidget)));

	DSMStartUpdate(dsm, refWidget);
}

void 
#ifdef _NO_PROTO
XmDropSiteUpdate( enclosingWidget, args, argCount )
        Widget enclosingWidget ;
        ArgList args ;
        Cardinal argCount ;
#else
XmDropSiteUpdate(
        Widget enclosingWidget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(enclosingWidget)));

	DSMUpdateInfo(dsm, enclosingWidget, args, argCount);
}

void 
#ifdef _NO_PROTO
XmDropSiteEndUpdate( refWidget )
        Widget refWidget ;
#else
XmDropSiteEndUpdate(
        Widget refWidget )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(refWidget)));

	DSMEndUpdate(dsm, refWidget);
}

void 
#ifdef _NO_PROTO
XmDropSiteRetrieve( enclosingWidget, args, argCount )
        Widget enclosingWidget ;
        ArgList args ;
        Cardinal argCount ;
#else
XmDropSiteRetrieve(
        Widget enclosingWidget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(enclosingWidget)));

	/* Update if dsm is dirty */
	_XmIEndUpdate((XtPointer) dsm, (XtIntervalId *) NULL);

	DSMRetrieveInfo(dsm, enclosingWidget, args, argCount);
}

Status 
#ifdef _NO_PROTO
XmDropSiteQueryStackingOrder( widget, parent_rtn, children_rtn, num_children_rtn )
        Widget widget ;
        Widget *parent_rtn ;
        Widget **children_rtn ;
        Cardinal *num_children_rtn ;
#else
XmDropSiteQueryStackingOrder(
        Widget widget,
        Widget *parent_rtn,
        Widget **children_rtn,
        Cardinal *num_children_rtn )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));
	XmDSInfo info = (XmDSInfo) DSMWidgetToInfo(dsm, widget);
	XmDSInfo parentInfo;
	Cardinal num_visible_children = 0; /* visible to application code */
	int i,j;

	/* Update if dsm is dirty */
	_XmIEndUpdate((XtPointer) dsm, (XtIntervalId *) NULL);

	if (info == NULL)
		return(0);

	if (!GetDSLeaf(info))
	{
		for (i=0; i < GetDSNumChildren(info); i++)
		{
			XmDSInfo child = (XmDSInfo) GetDSChild(info, i);
			if (!GetDSInternal(child))
				num_visible_children++;
		}

		if (num_visible_children)
		{
			*children_rtn = (Widget *) XtMalloc(sizeof(Widget) *
				num_visible_children);

			/* Remember to reverse the order */
			for (j=0, i=(GetDSNumChildren(info) - 1); i >= 0; i--)
			{
				XmDSInfo child = (XmDSInfo) GetDSChild(info, i);
				if (!GetDSInternal(child))
					(*children_rtn)[j++] = GetDSWidget(child);
			}
			/* assert(j == num_visible_children) */
		}
		else
			*children_rtn = NULL;

		*num_children_rtn = num_visible_children;
	}
	else
	{
		*children_rtn = NULL;
		*num_children_rtn = 0;
	}

	parentInfo = (XmDSInfo) GetDSParent(info);

	if (GetDSInternal(parentInfo))
	{
		*parent_rtn = NULL;
		while ((parentInfo = (XmDSInfo) GetDSParent(parentInfo)) !=
		       NULL)
			if (!GetDSInternal(parentInfo))
				*parent_rtn = GetDSWidget(parentInfo);
	}
	else
		*parent_rtn = GetDSWidget(parentInfo);

	return(1);
}

void 
#ifdef _NO_PROTO
XmDropSiteConfigureStackingOrder( widget, sibling, stack_mode )
        Widget widget ;
        Widget sibling ;
        Cardinal stack_mode ;
#else
XmDropSiteConfigureStackingOrder(
        Widget widget,
        Widget sibling,
        Cardinal stack_mode )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm;
	XmDSInfo info;
	XmDSInfo parent;

	if (widget == NULL)
		return;

	dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));

	info = (XmDSInfo) DSMWidgetToInfo(dsm, widget);

	if ((widget == sibling) || (info == NULL))
		return;
	
	parent = (XmDSInfo) GetDSParent(info);
	
	if (sibling != NULL)
	{
		XmDSInfo sib = (XmDSInfo) DSMWidgetToInfo(dsm, sibling);
		Cardinal index, sib_index;
		int i;

		if ((sib == NULL) ||
			(((XmDSInfo) GetDSParent(sib)) != parent) ||
			(XtParent(widget) != XtParent(sibling)))
			return;
		
		index = GetDSChildPosition(parent, info);
		sib_index = GetDSChildPosition(parent, sib);

		switch(stack_mode)
		{
			case XmABOVE:
				if (index > sib_index)
					for(i=index; i > sib_index; i--)
						SwapDSChildren(parent, i, i - 1);
				else
					for (i=index; i < (sib_index - 1); i++)
						SwapDSChildren(parent, i, i + 1);
			break;
			case XmBELOW:
				if (index > sib_index)
					for(i=index; i > (sib_index + 1); i--)
						SwapDSChildren(parent, i, i - 1);
				else
					for (i=index; i < sib_index; i++)
						SwapDSChildren(parent, i, i + 1);
			break;
			default:
				/* do nothing */
			break;
		}
	}
	else
	{
		Cardinal index = GetDSChildPosition(parent, info);
		int i;

		switch(stack_mode)
		{
			case XmABOVE:
				for (i=index; i > 0; i--)
					SwapDSChildren(parent, i, i - 1);
			break;
			case XmBELOW:
				for (i=index; i < (GetDSNumChildren(parent) - 1); i++)
					SwapDSChildren(parent, i, i + 1);
			break;
			default:
				/* do nothing */
			break;
		}
	}
}

XmDropSiteVisuals 
#ifdef _NO_PROTO
XmDropSiteGetActiveVisuals( widget )
        Widget widget ;
#else
XmDropSiteGetActiveVisuals(
        Widget widget )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));
	XmDSInfo info = (XmDSInfo) dsm->dropManager.curInfo;
	XmDropSiteVisuals dsv = (XmDropSiteVisuals) XtCalloc(1,
		sizeof(XmDropSiteVisualsRec));

	/* Update if dsm is dirty */
	_XmIEndUpdate((XtPointer) dsm, (XtIntervalId *) NULL);

	if (info == NULL) 
	{
		XtFree((char *)dsv);
		return(NULL);
	}
	
	if (!GetDSRemote(info))
	{
		Arg args[30];
		int n;
		Widget w;
                unsigned char unitType;
		w = GetDSWidget(info);

		/*
		 * We need to retrieve information from the widget. XtGetValues is
		 * too slow, so retrieve the information directly from the widget
		 * instance.
		 *
		 * XtGetValues is used for gadgets, since part of the instance
		 * structure will be in the cache and not directly accessable.
		 *
		 * XtGetValues is used for non-Motif widgets, just in case they provide
		 * Motif-style resources.
		 *
		 * (See also PutDSToStream() )
		 */

		if (XmIsPrimitive(w))
		{
			XmPrimitiveWidget pw = (XmPrimitiveWidget)w;

			dsv->background = pw->core.background_pixel;
			dsv->foreground = pw->primitive.foreground;
			dsv->topShadowColor = pw->primitive.top_shadow_color;
			dsv->topShadowPixmap = pw->primitive.top_shadow_pixmap;
			dsv->bottomShadowColor = pw->primitive.bottom_shadow_color;
			dsv->bottomShadowPixmap = pw->primitive.bottom_shadow_pixmap;
			dsv->shadowThickness = pw->primitive.shadow_thickness;
			dsv->highlightColor = pw->primitive.highlight_color;
			dsv->highlightPixmap = pw->primitive.highlight_pixmap;
			dsv->highlightThickness = pw->primitive.highlight_thickness;
			if (!GetDSHasRegion(info))
			    dsv->borderWidth = pw->core.border_width;
			else
			    dsv->borderWidth = 0;
		}
		else if (XmIsManager(w))
		{
			XmManagerWidget mw = (XmManagerWidget)w;

			dsv->background = mw->core.background_pixel;
			dsv->foreground = mw->manager.foreground;
			dsv->topShadowColor = mw->manager.top_shadow_color;
			dsv->topShadowPixmap = mw->manager.top_shadow_pixmap;
			dsv->bottomShadowColor = mw->manager.bottom_shadow_color;
			dsv->bottomShadowPixmap = mw->manager.bottom_shadow_pixmap;
			dsv->shadowThickness = mw->manager.shadow_thickness;
			dsv->highlightColor = mw->manager.highlight_color;
			dsv->highlightPixmap = mw->manager.highlight_pixmap;
			/* Temporary hack, until we support full defaulting */
			if (GetDSAnimationStyle(info) == XmDRAG_UNDER_HIGHLIGHT)
			    dsv->highlightThickness = 1;
			else
			    dsv->highlightThickness = 0;
			if (!GetDSHasRegion(info))
			    dsv->borderWidth = mw->core.border_width;
			else
			    dsv->borderWidth = 0;
		}
		else /* XmGadget or non-Motif subclass */
		{
			n = 0;
			XtSetArg(args[n], XmNunitType, &unitType); n++;
			XtGetValues(w, args, n);

                	if (unitType != XmPIXELS) { /* we need values in pixels */
			   n = 0;
			   XtSetArg(args[n], XmNunitType, XmPIXELS); n++;
			   XtSetValues(w, args, n);
                	}

			n = 0;
			XtSetArg(args[n], XmNbackground, &(dsv->background)); n++;
			XtSetArg(args[n], XmNforeground, &(dsv->foreground)); n++;
			XtSetArg(args[n], XmNtopShadowColor,
				&(dsv->topShadowColor)); n++;
			XtSetArg(args[n], XmNtopShadowPixmap,
				&(dsv->topShadowPixmap)); n++;
			XtSetArg(args[n], XmNbottomShadowColor,
				&(dsv->bottomShadowColor)); n++;
			XtSetArg(args[n], XmNbottomShadowPixmap,
				&(dsv->bottomShadowPixmap)); n++;
			XtSetArg(args[n], XmNshadowThickness,
				&(dsv->shadowThickness)); n++;
			XtSetArg(args[n], XmNhighlightColor,
				&(dsv->highlightColor)); n++;
			XtSetArg(args[n], XmNhighlightPixmap,
				&(dsv->highlightPixmap)); n++;
			XtSetArg(args[n], XmNhighlightThickness,
				&(dsv->highlightThickness)); n++;
			if (!GetDSHasRegion(info))
			{
			    XtSetArg(args[n], XmNborderWidth,
				&(dsv->borderWidth)); n++;
			}
			else
			    dsv->borderWidth = 0;

			XtGetValues(w, args, n);

			if (unitType != XmPIXELS) {
			    n = 0;
			    XtSetArg(args[n], XmNunitType, unitType); n++;
			    XtSetValues(w, args, n);
			}
		}
	}
	else
	{
		switch (GetDSAnimationStyle(info))
		{
			case XmDRAG_UNDER_HIGHLIGHT:
			{
				XmDSRemoteHighlightStyle hs =
					(XmDSRemoteHighlightStyle)
						GetDSRemoteAnimationPart(info);
				
				dsv->highlightColor = hs->highlight_color;
				dsv->highlightPixmap = hs->highlight_pixmap;
				dsv->background = hs->background;
				dsv->highlightThickness = hs->highlight_thickness;
				dsv->borderWidth = hs->border_width;
			}
			break;
			case XmDRAG_UNDER_SHADOW_IN:
			case XmDRAG_UNDER_SHADOW_OUT:
			{
				XmDSRemoteShadowStyle ss =
					(XmDSRemoteShadowStyle)
						GetDSRemoteAnimationPart(info);
				
				dsv->topShadowColor = ss->top_shadow_color;
				dsv->topShadowPixmap = ss->top_shadow_pixmap;
				dsv->bottomShadowColor = ss->bottom_shadow_color;
				dsv->bottomShadowPixmap = ss->bottom_shadow_pixmap;
				dsv->foreground = ss->foreground;
				dsv->shadowThickness = ss->shadow_thickness;
				dsv->highlightThickness = ss->highlight_thickness;
				dsv->borderWidth = ss->border_width;
			}
			break;
			case XmDRAG_UNDER_PIXMAP:
			{
				XmDSRemotePixmapStyle ps =
					(XmDSRemotePixmapStyle)
						GetDSRemoteAnimationPart(info);

				dsv->background = ps->background;
				dsv->foreground = ps->foreground;
				dsv->shadowThickness = ps->shadow_thickness;
				dsv->highlightThickness = ps->highlight_thickness;
				dsv->borderWidth = ps->border_width;
			}
			break;
			case XmDRAG_UNDER_NONE:
			default:
			break;
		}
	}

	return(dsv);
}

Widget
#ifdef _NO_PROTO
_XmGetActiveDropSite( widget )
        Widget widget ;
#else
_XmGetActiveDropSite(
        Widget widget )
#endif /* _NO_PROTO */
{
	XmDropSiteManagerObject dsm = (XmDropSiteManagerObject)
		_XmGetDropSiteManagerObject((XmDisplay) XmGetXmDisplay(
			XtDisplayOfObject(widget)));
	XmDSInfo info = (XmDSInfo) dsm->dropManager.curInfo;

	/* Update if dsm is dirty */
	_XmIEndUpdate((XtPointer) dsm, (XtIntervalId *) NULL);

	if ((!XmIsDragContext(widget)) || (GetDSRemote(info)))
		return(NULL);
	else
		return(GetDSWidget(info));

}
