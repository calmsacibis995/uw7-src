#pragma ident	"@(#)m1.2libs:Xm/DropSMgrP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmDropSMgrP_h
#define _XmDropSMgrP_h

#include <Xm/XmP.h>
#include <Xm/DropSMgr.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _NO_PROTO
typedef void (*XmDSMCreateInfoProc)();
typedef void (*XmDSMDestroyInfoProc)();
typedef void (*XmDSMStartUpdateProc)();
typedef void (*XmDSMRetrieveInfoProc)();
typedef void (*XmDSMUpdateInfoProc)();
typedef void (*XmDSMEndUpdateProc)();
typedef void (*XmDSMUpdateProc)();
typedef void (*XmDSMProcessMotionProc)();
typedef void (*XmDSMProcessDropProc)();
typedef void (*XmDSMOperationChangedProc)();
typedef void (*XmDSMChangeRootProc)();
typedef void (*XmDSMInsertInfoProc)();
typedef void (*XmDSMRemoveInfoProc)();
typedef void (*XmDSMSyncTreeProc)();
typedef int  (*XmDSMGetTreeFromDSMProc)();
typedef void (*XmDSMCreateDSInfoTable)();
typedef void (*XmDSMDestroyDSInfoTable)();
typedef void (*XmDSMRegisterInfoProc)();
typedef XtPointer (*XmDSMWidgetToInfoProc)();
typedef void (*XmDSMUnregisterInfoProc)();
#else
typedef void (*XmDSMCreateInfoProc)
	(XmDropSiteManagerObject, Widget, ArgList, Cardinal);
typedef void (*XmDSMDestroyInfoProc)
	(XmDropSiteManagerObject, Widget);
typedef void (*XmDSMStartUpdateProc)
	(XmDropSiteManagerObject, Widget);
typedef void (*XmDSMRetrieveInfoProc)
	(XmDropSiteManagerObject, Widget, ArgList, Cardinal);
typedef void (*XmDSMUpdateInfoProc)
	(XmDropSiteManagerObject, Widget, ArgList, Cardinal);
typedef void (*XmDSMEndUpdateProc)
	(XmDropSiteManagerObject, Widget);
typedef void (*XmDSMUpdateProc)
	(XmDropSiteManagerObject, XtPointer, XtPointer);
typedef void (*XmDSMProcessMotionProc)
	(XmDropSiteManagerObject, XtPointer, XtPointer);
typedef void (*XmDSMProcessDropProc)
	(XmDropSiteManagerObject, XtPointer, XtPointer);
typedef void (*XmDSMOperationChangedProc)
	(XmDropSiteManagerObject, XtPointer, XtPointer);
typedef void (*XmDSMChangeRootProc)
	(XmDropSiteManagerObject, XtPointer, XtPointer);
typedef void (*XmDSMInsertInfoProc)
	(XmDropSiteManagerObject, XtPointer, XtPointer);
typedef void (*XmDSMRemoveInfoProc)
	(XmDropSiteManagerObject, XtPointer);
typedef void (*XmDSMSyncTreeProc)
	(XmDropSiteManagerObject, Widget);
typedef int  (*XmDSMGetTreeFromDSMProc)
	(XmDropSiteManagerObject, Widget, XtPointer);
typedef void (*XmDSMCreateDSInfoTable)
	(XmDropSiteManagerObject);
typedef void (*XmDSMDestroyDSInfoTable)
	(XmDropSiteManagerObject);
typedef void (*XmDSMRegisterInfoProc)
	(XmDropSiteManagerObject, Widget, XtPointer);
typedef XtPointer (*XmDSMWidgetToInfoProc)
	(XmDropSiteManagerObject, Widget);
typedef void (*XmDSMUnregisterInfoProc)
	(XmDropSiteManagerObject, XtPointer);
#endif

typedef struct {
    XmDSMCreateInfoProc		createInfo;
    XmDSMDestroyInfoProc	destroyInfo;
    XmDSMStartUpdateProc	startUpdate;
    XmDSMRetrieveInfoProc	retrieveInfo;
    XmDSMUpdateInfoProc		updateInfo;
    XmDSMEndUpdateProc		endUpdate;
    
    /* Used by DragController Object */
    
    XmDSMUpdateProc		updateDSM;
    
    /* Used by update proc */
    
    XmDSMProcessMotionProc	processMotion;
    XmDSMProcessDropProc	processDrop;
    XmDSMOperationChangedProc	operationChanged;
    XmDSMChangeRootProc		changeRoot;
    
    /* Used to manage DropSites */
    
    XmDSMInsertInfoProc		insertInfo;
    /* Need a get and a put function for update? */
    XmDSMRemoveInfoProc		removeInfo;

	/* Used to manage the pre-register information */

    XmDSMSyncTreeProc		syncTree;
    XmDSMGetTreeFromDSMProc	getTreeFromDSM;
    
    /* Used to hash between widgets and info */
    
    XmDSMCreateDSInfoTable	createTable;
    XmDSMDestroyDSInfoTable 	destroyTable;
    XmDSMRegisterInfoProc	registerInfo;
    XmDSMWidgetToInfoProc	widgetToInfo;
    XmDSMUnregisterInfoProc 	unregisterInfo;
    
    XtPointer			extension;
} XmDropSiteManagerClassPart;

/*  Full class record declaration for dropSite class  */

typedef struct _XmDropSiteManagerClassRec{
    ObjectClassPart		object_class;
    XmDropSiteManagerClassPart		dropManager_class;
} XmDropSiteManagerClassRec;

externalref XmDropSiteManagerClassRec 	xmDropSiteManagerClassRec;

/* Macros for calling methods */

#define DSMCreateInfo(dsm, widget, args, numArgs) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.createInfo) \
		((dsm), (widget), (args), (numArgs))

#define DSMDestroyInfo(dsm, widget) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.destroyInfo) \
		((dsm), (widget))

#define DSMStartUpdate(dsm, widget) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.startUpdate) \
		((dsm), (widget))

#define DSMRetrieveInfo(dsm, widget, args, numArgs) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.retrieveInfo) \
		((dsm), (widget), (args), (numArgs))

#define DSMUpdateInfo(dsm, widget, args, numArgs) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.updateInfo) \
		((dsm), (widget), (args), (numArgs))

#define DSMEndUpdate(dsm, widget) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.endUpdate) \
		((dsm), (widget))

#define DSMUpdate(dsm, clientData, callData) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.updateDSM) \
		((dsm), (clientData), (callData))

#define DSMProcessMotion(dsm, clientData, callData) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.processMotion) \
		((dsm), (clientData), (callData))

#define DSMProcessDrop(dsm, clientData, callData) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.processDrop) \
		((dsm),(clientData), (callData))

#define DSMOperationChanged(dsm, clientData, callData) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.operationChanged) \
		((dsm),(clientData), (callData))

#define DSMChangeRoot(dsm, clientData, callData) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.changeRoot) \
		((dsm), (clientData), (callData))

#define DSMInsertInfo(dsm, info, call_data) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.insertInfo) \
		((dsm), (info), (call_data))

#define DSMRemoveInfo(dsm, info) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.removeInfo) \
		((dsm), (info))

#define DSMSyncTree(dsm, shell) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.syncTree) \
		((dsm), (shell))

#define DSMGetTreeFromDSM(dsm, shell, dataPtr) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.getTreeFromDSM) \
		((dsm), (shell), (dataPtr))

#define DSMCreateTable(dsm) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.createTable) \
		((dsm))

#define DSMDestroyTable(dsm) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.destroyTable) \
		((dsm))

#define DSMRegisterInfo(dsm, widget, info) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.registerInfo) \
		((dsm), (widget), (info))

#define DSMWidgetToInfo(dsm, widget) \
	(XtPointer) ((((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.widgetToInfo) \
		((dsm), (widget)))

#define DSMUnregisterInfo(dsm, info) \
	(((XmDropSiteManagerObjectClass) XtClass(dsm))-> \
		dropManager_class.unregisterInfo) \
		  ((dsm), (info))

/* DropSiteManager instance record */

typedef struct _XmDropSiteManagerPart{
    XtCallbackProc	notifyProc;
    XtCallbackProc	treeUpdateProc;
    XtPointer		client_data;
    XtPointer		dragUnderData;
    XtPointer		curInfo;
    Time		curTime;
    Position		curX, curY, oldX, oldY;
    unsigned char	curDropSiteStatus;
    Widget		curDragContext;
    Boolean		curAnimate;
    unsigned char	curOperations;
    unsigned char	curOperation;
    XmRegion		curAncestorClipRegion;
    XmRegion		newAncestorClipRegion;
    XtPointer		dsTable;
    XtPointer		dsRoot;
    Position		rootX, rootY;
    Dimension		rootW, rootH;
    XtPointer		clipperList;
    XtPointer		updateInfo;
} XmDropSiteManagerPart, *XmDropSiteManagerPartPtr;

/* Full instance record declaration */

typedef struct _XmDropSiteManagerRec{
    ObjectPart			object;
    XmDropSiteManagerPart	dropManager;
} XmDropSiteManagerRec;

/* Internal update struct */

typedef struct __XmDropSiteUpdateInfoRec {
  XmDropSiteManagerObject	dsm;
  Widget			refWidget;
  XtPointer			*next;
} _XmDropSiteUpdateInfoRec, *_XmDropSiteUpdateInfo;

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmDSMUpdate() ;
extern int _XmDSMGetTreeFromDSM() ;
extern Boolean _XmDropSiteShell() ;
extern Boolean _XmDropSiteWrapperCandidate() ;
extern Widget _XmGetActiveDropSite() ;
extern void _XmSyncDropSiteTree() ;
extern void _XmIEndUpdate() ;

#else

extern void _XmDSMUpdate( 
                        XmDropSiteManagerObject dsm,
                        XtPointer clientData,
                        XtPointer callData) ;
extern int _XmDSMGetTreeFromDSM( 
                        XmDropSiteManagerObject dsm,
                        Widget shell,
                        XtPointer dataPtr) ;
extern Boolean _XmDropSiteShell( 
                        Widget widget) ;
extern Boolean _XmDropSiteWrapperCandidate( 
                        Widget widget) ;
extern Widget _XmGetActiveDropSite( 
                        Widget widget) ;
extern void _XmSyncDropSiteTree(
				Widget shell) ;
extern void _XmIEndUpdate(XtPointer client_data, XtIntervalId *interval_id);

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDropSMgrP_h */
