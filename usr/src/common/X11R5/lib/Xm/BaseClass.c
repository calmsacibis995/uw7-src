#pragma ident	"@(#)m1.2libs:Xm/BaseClass.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1989, 1990 DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#define HAVE_EXTENSIONS

#include <Xm/BaseClassP.h>
#include <Xm/ExtObjectP.h>
#include <X11/ShellP.h>
#include <Xm/MenuShell.h>
#include <Xm/DropSMgr.h>
#include <Xm/DropSMgrP.h>
#include <Xm/Screen.h>
#include <Xm/VendorSEP.h>
#include "MessagesI.h"
#include "CallbackI.h"


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_BaseC,MSG_BC_1, _XmMsgBaseClass_0000)
#define MSG2	catgets(Xm_catd,MS_BaseC,MSG_BC_2, _XmMsgBaseClass_0001)
#define MSG3	catgets(Xm_catd,MS_BaseC,MSG_BC_3, _XmMsgBaseClass_0002)
#define MSG4	catgets(Xm_catd,MS_BaseC,MSG_BC_4, _XmMsgGetSecRes_0000)
#else
#define MSG1	 _XmMsgBaseClass_0000
#define MSG2	 _XmMsgBaseClass_0001
#define MSG3	 _XmMsgBaseClass_0002
#define MSG4	_XmMsgGetSecRes_0000
#endif



#define IsBaseClass(wc) \
  ((wc == xmGadgetClass) 		||\
   (wc == xmManagerWidgetClass)		||\
   (wc == xmPrimitiveWidgetClass)	||\
   (wc == vendorShellWidgetClass) 	||\
   (wc == xmDisplayClass)		||\
   (wc == xmScreenClass)		||\
   (wc == xmExtObjectClass)		||\
   (wc == xmMenuShellWidgetClass))

#define isWrappedXtClass(wc) \
   ((wc == rectObjClass)	||\
	(wc == compositeWidgetClass))

#define DiffWrapperClasses(wd)  \
   (XtIsConstraint(wd->widgetClass) ^ XtIsConstraint(wd->next->widgetClass))
	  
externaldef(baseclass) XrmQuark	XmQmotif;
externaldef(baseclass) XmBaseClassExt	*_Xm_fastPtr;

typedef struct _XmObjectClassWrapper{
    XtInitProc		initialize;
    XtSetValuesFunc	setValues;
    XtArgsProc		getValues;
    XtWidgetClassProc	classPartInit;
}XmObjectClassWrapper;

static XmObjectClassWrapper objectClassWrapper;

static XmObjectClassWrapper nestingCompare;

externaldef(xminheritclass) int _XmInheritClass = 0;


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XmWrapperData _XmGetWrapperData() ;
static XmWrapperData _XmPushWrapperData() ;
static XmWrapperData _XmPopWrapperData() ;
static XContext ExtTypeToContext() ;
static Boolean IsSubclassOf() ;
static void RealizeWrapper0() ;
static void RealizeWrapper1() ;
static void RealizeWrapper2() ;
static void RealizeWrapper3() ;
static void RealizeWrapper4() ;
static void RealizeWrapper5() ;
static void RealizeWrapper6() ;
static void RealizeWrapper7() ;
static Cardinal GetRealizeDepth() ;
static void RealizeWrapper() ;
static void ResizeWrapper0() ;
static void ResizeWrapper1() ;
static void ResizeWrapper2() ;
static void ResizeWrapper3() ;
static void ResizeWrapper4() ;
static void ResizeWrapper5() ;
static void ResizeWrapper6() ;
static void ResizeWrapper7() ;
static void ResizeWrapper8() ;
static void ResizeWrapper9() ;
static void ResizeWrapper10() ;
static Cardinal GetResizeDepth() ;
static void ResizeWrapper() ;
static XtGeometryResult GeometryHandlerWrapper0() ;
static XtGeometryResult GeometryHandlerWrapper1() ;
static XtGeometryResult GeometryHandlerWrapper2() ;
static XtGeometryResult GeometryHandlerWrapper3() ;
static XtGeometryResult GeometryHandlerWrapper4() ;
static XtGeometryResult GeometryHandlerWrapper5() ;
static XtGeometryResult GeometryHandlerWrapper6() ;
static XtGeometryResult GeometryHandlerWrapper7() ;
static XtGeometryResult GeometryHandlerWrapper8() ;
static XtGeometryResult GeometryHandlerWrapper9() ;
static Cardinal GetGeometryHandlerDepth() ;
static XtGeometryResult GeometryHandlerWrapper() ;
static XmBaseClassExt * BaseClassPartInitialize() ;
static void ClassPartInitRootWrapper() ;
static void ClassPartInitLeafWrapper() ;
static void InitializeRootWrapper() ;
static void InitializeLeafWrapper() ;
static Boolean SetValuesRootWrapper() ;
static Boolean SetValuesLeafWrapper() ;
static void GetValuesRootWrapper() ;
static void GetValuesLeafWrapper() ;
static Cardinal GetSecResData() ;
static XtResourceList * XmCreateIndirectionTable() ;

#else

static XmWrapperData _XmGetWrapperData( 
                        WidgetClass w_class) ;
static XmWrapperData _XmPushWrapperData( 
                        WidgetClass w_class) ;
static XmWrapperData _XmPopWrapperData( 
                        WidgetClass w_class) ;
static XContext ExtTypeToContext( 
#if NeedWidePrototypes
                        unsigned int extType) ;
#else
                        unsigned char extType) ;
#endif /* NeedWidePrototypes */
static Boolean IsSubclassOf( 
                        WidgetClass wc,
                        WidgetClass sc) ;
static void RealizeWrapper0( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static void RealizeWrapper1( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static void RealizeWrapper2( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static void RealizeWrapper3( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static void RealizeWrapper4( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static void RealizeWrapper5( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static void RealizeWrapper6( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static void RealizeWrapper7( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr) ;
static Cardinal GetRealizeDepth( 
                        WidgetClass wc) ;
static void RealizeWrapper( 
                        Widget w,
                        Mask *vmask,
                        XSetWindowAttributes *attr,
                        Cardinal depth) ;
static void ResizeWrapper0( 
                        Widget w) ;
static void ResizeWrapper1( 
                        Widget w) ;
static void ResizeWrapper2( 
                        Widget w) ;
static void ResizeWrapper3( 
                        Widget w) ;
static void ResizeWrapper4( 
                        Widget w) ;
static void ResizeWrapper5( 
                        Widget w) ;
static void ResizeWrapper6( 
                        Widget w) ;
static void ResizeWrapper7( 
                        Widget w) ;
static void ResizeWrapper8( 
                        Widget w) ;
static void ResizeWrapper9( 
                        Widget w) ;
static void ResizeWrapper10( 
                        Widget w) ;
static Cardinal GetResizeDepth( 
                        WidgetClass wc) ;
static void ResizeWrapper( 
                        Widget w,
                        int depth) ;
static XtGeometryResult GeometryHandlerWrapper0( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper1( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper2( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper3( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper4( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper5( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper6( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper7( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper8( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static XtGeometryResult GeometryHandlerWrapper9( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static Cardinal GetGeometryHandlerDepth( 
                        WidgetClass wc) ;
static XtGeometryResult GeometryHandlerWrapper( 
                        Widget w,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed,
                        int depth) ;
static XmBaseClassExt * BaseClassPartInitialize( 
                        WidgetClass wc) ;
static void ClassPartInitRootWrapper( 
                        WidgetClass wc) ;
static void ClassPartInitLeafWrapper( 
                        WidgetClass wc) ;
static void InitializeRootWrapper( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void InitializeLeafWrapper( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValuesRootWrapper( 
                        Widget current,
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValuesLeafWrapper( 
                        Widget current,
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesRootWrapper( 
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesLeafWrapper( 
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Cardinal GetSecResData( 
                        WidgetClass w_class,
                        XmSecondaryResourceData **secResDataRtn) ;
static XtResourceList * XmCreateIndirectionTable( 
                        XtResourceList resources,
                        Cardinal num_resources) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


Boolean 
#ifdef _NO_PROTO
_XmIsSlowSubclass( wc, bit )
        WidgetClass wc ;
        unsigned int bit ;
#else
_XmIsSlowSubclass(
        WidgetClass wc,
        unsigned int bit )
#endif /* _NO_PROTO */
{
    XmBaseClassExt	*wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif);

    if (!wcePtr || !(*wcePtr))
      return False;

    if (_XmGetFlagsBit((*wcePtr)->flags, bit))
      return True;
    else
      return False;
}

XmGenericClassExt * 
#ifdef _NO_PROTO
_XmGetClassExtensionPtr( listHeadPtr, owner )
        XmGenericClassExt *listHeadPtr ;
        XrmQuark owner ;
#else
_XmGetClassExtensionPtr(
        XmGenericClassExt *listHeadPtr,
        XrmQuark owner )
#endif /* _NO_PROTO */
{
    XmGenericClassExt	*lclPtr = listHeadPtr;

#ifdef DEBUG    
    if (!lclPtr) 
      {
	  _XmWarning(NULL, "_XmGetClassExtensionPtr: invalid class ext pointer");
	  return NULL;
      }
#endif /* DEBUG */
    for (; 
	 lclPtr && *lclPtr && ((*lclPtr)->record_type != owner);
	 lclPtr = (XmGenericClassExt *) &((*lclPtr)->next_extension)
	 ) ;

    return lclPtr;
}


static XmWrapperData 
#ifdef _NO_PROTO
_XmGetWrapperData( w_class )
        WidgetClass w_class ;
#else
_XmGetWrapperData(
        WidgetClass w_class )
#endif /* _NO_PROTO */
{
    XmBaseClassExt *wcePtr;

    wcePtr = _XmGetBaseClassExtPtr( w_class, XmQmotif);

    if(    !*wcePtr    )
    {   *wcePtr = (XmBaseClassExt) XtCalloc(1, sizeof( XmBaseClassExtRec)) ;
        (*wcePtr)->next_extension = NULL ;
        (*wcePtr)->record_type 	= XmQmotif ;
        (*wcePtr)->version	= XmBaseClassExtVersion ;
        (*wcePtr)->record_size	= sizeof( XmBaseClassExtRec) ;
        }
    if(    (*wcePtr)->version < XmBaseClassExtVersion    )
    {   return( NULL) ;
        } 
    if(    !((*wcePtr)->wrapperData)    )
    {   (*wcePtr)->wrapperData = 
	  (XmWrapperData) XtCalloc(1, sizeof( XmWrapperDataRec)) ;
        } 
    return (*wcePtr)->wrapperData;
    }

static XmWrapperData 
#ifdef _NO_PROTO
_XmPushWrapperData( w_class )
WidgetClass w_class ;
#else
_XmPushWrapperData(
		   WidgetClass w_class )
#endif /* _NO_PROTO */
{
    XmBaseClassExt *wcePtr;
    XmWrapperData   wrapperData;

    wcePtr = _XmGetBaseClassExtPtr( w_class, XmQmotif);
    
    if(    !*wcePtr    )
      { 
	  *wcePtr = (XmBaseClassExt) XtCalloc(1, sizeof( XmBaseClassExtRec)) ;
	  (*wcePtr)->next_extension = NULL ;
	  (*wcePtr)->record_type 	= XmQmotif ;
	  (*wcePtr)->version	= XmBaseClassExtVersion ;
	  (*wcePtr)->record_size	= sizeof( XmBaseClassExtRec) ;
      }
    if(    (*wcePtr)->version < XmBaseClassExtVersion    )
      {  
	  return( NULL) ;
      } 

    wrapperData = (XmWrapperData) XtMalloc(sizeof( XmWrapperDataRec)) ;
    /*
     * be paranoid about a wrapped proc being called during chaining
     */
    if ((*wcePtr)->wrapperData) {
#ifdef OLD
	XmWrapperData	oldWd = wrapperData->next;
#else
	XmWrapperData	oldWd = (*wcePtr)->wrapperData;
#endif

	memcpy((char *)wrapperData, (char *)oldWd,
	       sizeof(XmWrapperDataRec));
	wrapperData->widgetClass = w_class;
	wrapperData->next = oldWd;
    }
    else {
	memset((char *)wrapperData, 0, sizeof(XmWrapperDataRec));
    }
    (*wcePtr)->wrapperData = wrapperData;
    return wrapperData;
}


static XmWrapperData
#ifdef _NO_PROTO
_XmPopWrapperData( w_class )
        WidgetClass w_class ;
#else
_XmPopWrapperData(
        WidgetClass w_class )
#endif /* _NO_PROTO */
{
    XmBaseClassExt *wcePtr;
    XmWrapperData  wrapperData;

    wcePtr = _XmGetBaseClassExtPtr(w_class, XmQmotif);

    if ((wrapperData = (*wcePtr)->wrapperData) != NULL)
      (*wcePtr)->wrapperData = wrapperData->next;
    return wrapperData;
}

typedef struct _ExtToContextRec{
    unsigned char	extType;
    XContext		context;
}ExtToContextRec, *ExtToContext;

static XContext 
#ifdef _NO_PROTO
ExtTypeToContext( extType )
        unsigned char extType ;
#else
ExtTypeToContext(
#if NeedWidePrototypes
        unsigned int extType )
#else
        unsigned char extType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    static ExtToContextRec 	extToContextMap[16];
    Cardinal			i;
    ExtToContext		curr;
    XContext			context = (XContext) NULL;

    for (i = 0, curr = &extToContextMap[0];
	 i < XtNumber(extToContextMap) && !context;
	 i++, curr++)
      {
	  if (curr->extType == extType)
	    context = curr->context;
	  else if (!curr->extType)
	    {
		curr->extType = extType;
		context =  
		  curr->context = 
		    XUniqueContext();
	    }
      }
    if (!context)

      _XmWarning(NULL, MSG1);
    return context;
}
	 
typedef struct _XmAssocDataRec{
    XtPointer			data;
    struct _XmAssocDataRec	*next;
}XmAssocDataRec, *XmAssocData;

void 
#ifdef _NO_PROTO
_XmPushWidgetExtData( widget, data, extType )
        Widget widget ;
        XmWidgetExtData data ;
        unsigned char extType ;
#else
_XmPushWidgetExtData(
        Widget widget,
        XmWidgetExtData data,
#if NeedWidePrototypes
        unsigned int extType )
#else
        unsigned char extType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmAssocData		newData;
    XmAssocData		assocData = NULL;
    XmAssocData		*assocDataPtr;
    Boolean		empty;
    XContext		widgetExtContext = ExtTypeToContext(extType);

    newData= (XmAssocData) XtCalloc(1, sizeof(XmAssocDataRec));

    newData->data = (XtPointer)data;

    if (XFindContext(XtDisplay(widget),
		     (Window) widget,
		     widgetExtContext,
		     (char **) &assocData))
      empty = True;
    else
      empty = False;

    for (assocDataPtr= &assocData; 
	 *assocDataPtr; 
	 assocDataPtr = &((*assocDataPtr)->next))
      {};

    *assocDataPtr = newData;

    if (empty)
      XSaveContext(XtDisplay(widget),
		   (Window) widget,
		   widgetExtContext,
		   (XPointer) assocData);
}

void
#ifdef _NO_PROTO
_XmPopWidgetExtData( widget, dataRtn, extType )
        Widget widget ;
        XmWidgetExtData *dataRtn ;
        unsigned char extType ;
#else
_XmPopWidgetExtData(
        Widget widget,
        XmWidgetExtData *dataRtn,
#if NeedWidePrototypes
        unsigned int extType )
#else
        unsigned char extType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmAssocData		assocData = NULL,  *assocDataPtr;
    XContext		widgetExtContext = ExtTypeToContext(extType);

    if (XFindContext(XtDisplay(widget),
		     (Window) widget,
		     widgetExtContext,
		     (char **) &assocData))
      {
#ifdef DEBUG
	  _XmWarning(NULL, MSG2);
#endif 
      *dataRtn = NULL;
	  return;
      }

    for (assocDataPtr= &assocData; 
	 (*assocDataPtr) && (*assocDataPtr)->next; 
	 assocDataPtr = &((*assocDataPtr)->next)){};

    if (*assocDataPtr == assocData)
      {
	  XDeleteContext(XtDisplay(widget),
			 (Window)widget,
			 widgetExtContext);
      }
    if (*assocDataPtr) {
	*dataRtn = (XmWidgetExtData) (*assocDataPtr)->data;
	XtFree((char *) *assocDataPtr);
	*assocDataPtr = NULL;
    }
    else
      *dataRtn = NULL;
}

XmWidgetExtData 
#ifdef _NO_PROTO
_XmGetWidgetExtData( widget, extType )
        Widget widget ;
        unsigned char extType ;
#else
_XmGetWidgetExtData(
        Widget widget,
#if NeedWidePrototypes
        unsigned int extType )
#else
        unsigned char extType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmAssocData		assocData = NULL,  *assocDataPtr;
    XContext		widgetExtContext = ExtTypeToContext(extType);

    
    if ((XFindContext(XtDisplay(widget),
		      (Window) widget,
		      widgetExtContext,
		      (char **) &assocData)))
      {
#ifdef DEBUG
	  _XmWarning(NULL, "no extension data on stack");
#endif /* DEBUG */
#ifdef TEMP_EXT_BC
	  assocData = (XmAssocData) XtCalloc(1, sizeof(XmAssocDataRec));
	  assocData->data = (XtPointer) XtCalloc(1, sizeof(XmWidgetExtDataRec));
	  XSaveContext(XtDisplay(widget),
		       (Window) widget,
		       widgetExtContext,
		       assocData);
	  return (XmWidgetExtData)assocData->data ;
#else
	  return NULL;
#endif
      }
    else
      {
	  for (assocDataPtr= &assocData; 
	       (*assocDataPtr)->next; 
	       assocDataPtr = &((*assocDataPtr)->next)){};

	  return (XmWidgetExtData) (*assocDataPtr)->data;
      }
}

void 
#ifdef _NO_PROTO
_XmFreeWidgetExtData( widget )
        Widget widget ;
#else
_XmFreeWidgetExtData(
        Widget widget )
#endif /* _NO_PROTO */
{

    _XmWarning(NULL, MSG3);

}

static Boolean
#ifdef _NO_PROTO
IsSubclassOf( wc, sc )
	WidgetClass wc;
	WidgetClass sc;
#else
IsSubclassOf(
	WidgetClass wc,
	WidgetClass sc)
#endif /* _NO_PROTO */
{
	WidgetClass p = wc;

	for(; (p) && (p != sc); p = p->core_class.superclass);
	return (p == sc);
}

void 
#ifdef _NO_PROTO
_XmBaseClassPartInitialize( wc )
        WidgetClass wc ;
#else
_XmBaseClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
#ifdef notdef
    _XmWarning(NULL, "_XmBaseClassPartInitialize is an unsupported routine");
#endif /* notdef */
}

/*********************************************************************
 *
 *  RealizeWrappers for vendorShell
 *
 *********************************************************************/

static void 
#ifdef _NO_PROTO
RealizeWrapper0( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper0(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 0);
}

static void 
#ifdef _NO_PROTO
RealizeWrapper1( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper1(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 1);
}

static void 
#ifdef _NO_PROTO
RealizeWrapper2( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper2(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 2);
}

static void 
#ifdef _NO_PROTO
RealizeWrapper3( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper3(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 3);
}

static void 
#ifdef _NO_PROTO
RealizeWrapper4( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper4(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 4);
}

static void 
#ifdef _NO_PROTO
RealizeWrapper5( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper5(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 5);
}

static void 
#ifdef _NO_PROTO
RealizeWrapper6( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper6(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 6);
}

static void 
#ifdef _NO_PROTO
RealizeWrapper7( w, vmask, attr )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
#else
RealizeWrapper7(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
    RealizeWrapper(w, vmask, attr, 7);
}

static XtRealizeProc realizeWrappers[] = {
    RealizeWrapper0,
    RealizeWrapper1,
    RealizeWrapper2,
    RealizeWrapper3,
    RealizeWrapper4,
    RealizeWrapper5,
    RealizeWrapper6,
    RealizeWrapper7,
};

static Cardinal 
#ifdef _NO_PROTO
GetRealizeDepth( wc )
        WidgetClass wc ;
#else
GetRealizeDepth(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    Cardinal i;

    for (i = 0; 
	 wc && wc != vendorShellWidgetClass;
	 i++, wc = wc->core_class.superclass) {};

    if (wc)
      return i;
#ifdef DEBUG
    else
      XtError("bad class for shell realize");
#endif /* DEBUG */
    return 0 ;
}

/************************************************************************
 *
 *  RealizeWrapper
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
RealizeWrapper( w, vmask, attr, depth )
        Widget w ;
        Mask *vmask ;
        XSetWindowAttributes *attr ;
        Cardinal depth ;
#else
RealizeWrapper(
        Widget w,
        Mask *vmask,
        XSetWindowAttributes *attr,
        Cardinal depth )
#endif /* _NO_PROTO */
{
    if (XmIsVendorShell(w))
      {
	  XmWidgetExtData	extData;
	  WidgetClass	wc = XtClass(w);
	  XmWrapperData	wrapperData;
	  Cardinal	leafDepth = GetRealizeDepth(wc);
	  Cardinal	depthDiff = leafDepth - depth;
	  for (;
	       depthDiff;
	       depthDiff--, wc = wc->core_class.superclass)
	    {};
	  
	  wrapperData = _XmGetWrapperData(wc);
	  (*(wrapperData->realize))(w, vmask, attr);

/*
 * Fix for CR 3353 - Avoid calling the RealizeCallback twice for DialogShells 
 *                   by checking the WidgetClass name.  If it is XmDialogShell,
 *                   do not call the callback (it will be called prior when
 *                   the WidgetClass is VendorShell).
 */ 
	  if(   ((extData = _XmGetWidgetExtData(w, XmSHELL_EXTENSION)) != NULL)
	     && (extData->widget != NULL) &&
             strcmp(wc->core_class.class_name, "XmDialogShell") )
	    {
	      _XmCallCallbackList( extData->widget, ((XmVendorShellExtObject)
			   (extData->widget))->vendor.realize_callback, NULL) ;
	    }
#ifdef DEBUG
	  else
	    _XmWarning(NULL, "we only support realize callbacks on shells");
#endif /* DEBUG */
      }
}

/*********************************************************************
 *
 *  ResizeWrappers for rectObj
 *
 *********************************************************************/

static void 
#ifdef _NO_PROTO
ResizeWrapper0( w)
        Widget w ;
#else
ResizeWrapper0(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 0);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper1( w)
        Widget w ;
#else
ResizeWrapper1(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 1);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper2( w)
        Widget w ;
#else
ResizeWrapper2(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 2);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper3( w)
        Widget w ;
#else
ResizeWrapper3(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 3);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper4( w)
        Widget w ;
#else
ResizeWrapper4(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 4);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper5( w)
        Widget w ;
#else
ResizeWrapper5(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 5);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper6( w)
        Widget w ;
#else
ResizeWrapper6(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 6);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper7( w)
        Widget w ;
#else
ResizeWrapper7(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 7);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper8( w)
        Widget w ;
#else
ResizeWrapper8(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 8);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper9( w)
        Widget w ;
#else
ResizeWrapper9(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 9);
}

static void 
#ifdef _NO_PROTO
ResizeWrapper10( w)
        Widget w ;
#else
ResizeWrapper10(
        Widget w )
#endif /* _NO_PROTO */
{
    ResizeWrapper(w, 10);
}

static XtWidgetProc resizeWrappers[] = {
    ResizeWrapper0,
    ResizeWrapper1,
    ResizeWrapper2,
    ResizeWrapper3,
    ResizeWrapper4,
    ResizeWrapper5,
    ResizeWrapper6,
    ResizeWrapper7,
    ResizeWrapper8,
    ResizeWrapper9,
    ResizeWrapper10,
};

static Cardinal 
#ifdef _NO_PROTO
GetResizeDepth( wc )
        WidgetClass wc ;
#else
GetResizeDepth(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    Cardinal i;

    for (i = 0; 
	 wc && wc != rectObjClass;
	 i++, wc = wc->core_class.superclass) {};

    if (wc)
      return i;
    return 0 ;
}


/************************************************************************
 *
 *  ResizeWrapper
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ResizeWrapper(w, depth)
        Widget w ;
		int depth;
#else
ResizeWrapper(
        Widget w,
		int depth )
#endif /* _NO_PROTO */
{
	static Widget refW = NULL;
	WidgetClass	wc = XtClass(w);
	XmWrapperData	wrapperData;
	Cardinal	leafDepth = GetResizeDepth(wc);
	Cardinal	depthDiff = leafDepth - depth;
	Boolean         call_navig_resize = FALSE ;

        /* Call _XmNavigResize() only once per resize event, so nested
         * resize calls are completed before evaluating the status of
         * the focus widget.  Only check for lost focus in
         * response to resize events from a Shell; otherwise
         * _XmNavigResize may (prematurely) determine that the
         * focus widget is no longer traversable before the
         * new layout is complete.
         */
	if(    XtParent( w)  &&  XtIsShell( XtParent( w))    )
	  {
	    call_navig_resize = TRUE ;
	  }
	for (;
		depthDiff;
		depthDiff--, wc = wc->core_class.superclass)
		{};

	wrapperData = _XmGetWrapperData(wc);

	if(    wrapperData->resize    )
	  {
	    if ((refW == NULL) && (_XmDropSiteWrapperCandidate(w)))
	      {
		refW = w;
		XmDropSiteStartUpdate(refW);
		(*(wrapperData->resize))(w);
		XmDropSiteEndUpdate(refW);
		refW = NULL;
	      }
	    else
	      (*(wrapperData->resize))(w);
	  }
	if(    call_navig_resize    )
	  {
	    _XmNavigResize( w) ;
	  }
}


/*********************************************************************
 *
 *  GeometryHandlerWrappers for composite
 *
 *********************************************************************/

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper0( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper0(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 0));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper1( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper1(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 1));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper2( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper2(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 2));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper3( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper3(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 3));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper4( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper4(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 4));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper5( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper5(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 5));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper6( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper6(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 6));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper7( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper7(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 7));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper8( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper8(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 8));
}

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper9( w, desired, allowed)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
#else
GeometryHandlerWrapper9(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
	return(GeometryHandlerWrapper(w, desired, allowed, 9));
}

static XtGeometryHandler geometryHandlerWrappers[] = {
    GeometryHandlerWrapper0,
    GeometryHandlerWrapper1,
    GeometryHandlerWrapper2,
    GeometryHandlerWrapper3,
    GeometryHandlerWrapper4,
    GeometryHandlerWrapper5,
    GeometryHandlerWrapper6,
    GeometryHandlerWrapper7,
    GeometryHandlerWrapper8,
    GeometryHandlerWrapper9,
};


static Cardinal 
#ifdef _NO_PROTO
GetGeometryHandlerDepth( wc )
        WidgetClass wc ;
#else
GetGeometryHandlerDepth(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    Cardinal i;

    for (i = 0; 
	 wc && wc != rectObjClass;
	 i++, wc = wc->core_class.superclass) {};

    if (wc)
      return i;
    return 0 ;
}

/************************************************************************
 *
 *  GeometryHandlerWrapper
 *
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
GeometryHandlerWrapper( w, desired, allowed, depth)
        Widget w ;
		XtWidgetGeometry *desired;
		XtWidgetGeometry *allowed;
		int depth;
#else
GeometryHandlerWrapper(
        Widget w,
		XtWidgetGeometry *desired,
		XtWidgetGeometry *allowed,
		int depth)
#endif /* _NO_PROTO */
{
	static Widget refW = NULL;
	XtGeometryResult result = XtGeometryNo;
	Widget		parent = XtParent(w);
	WidgetClass	wc = XtClass(parent);
	XmWrapperData	wrapperData;
	Cardinal	leafDepth = GetGeometryHandlerDepth(wc);
	Cardinal	depthDiff = leafDepth - depth;

	for (;
		depthDiff;
		depthDiff--, wc = wc->core_class.superclass)
		{};

	wrapperData = _XmGetWrapperData(wc);

	if ((refW == NULL) && (_XmDropSiteWrapperCandidate(w)))
	{
		refW = w;
		XmDropSiteStartUpdate(refW);
		result = (*(wrapperData->geometry_manager))
			(w, desired, allowed);
		XmDropSiteEndUpdate(refW);
		refW = NULL;
	}
	else
		result = (*(wrapperData->geometry_manager))
			(w, desired, allowed);

	return(result);
}


/************************************************************************
 *
 *  BaseClassPartInitialize
 *
 ************************************************************************/
static XmBaseClassExt * 
#ifdef _NO_PROTO
BaseClassPartInitialize( wc )
        WidgetClass wc ;
#else
BaseClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    XmBaseClassExt		*wcePtr, *scePtr;
    Cardinal			i;
    Boolean			inited;
	XmWrapperData	wcData, scData;
    Boolean			isBaseClass = IsBaseClass(wc);

    /* 
     * this routine is called out of the ClassPartInitRootWrapper. It
     * needs to make sure that this is a motif class and if it is,
     * then to initialize it. We assume that the base classes always
     * have a static initializer !!!
     */

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif);
    scePtr = _XmGetBaseClassExtPtr(wc->core_class.superclass, XmQmotif);

    if (!isBaseClass && 
		!isWrappedXtClass(wc) &&
		(!scePtr || !(*scePtr)))
      return NULL;

	if ((isBaseClass) || (scePtr && (*scePtr)))
	{
		if (!(*wcePtr))
		  {
		  inited = False;
		  *wcePtr = (XmBaseClassExt) 
		    XtCalloc(1, sizeof(XmBaseClassExtRec)) ;
		  (*wcePtr)->classPartInitPrehook = XmInheritClassPartInitPrehook;
		  (*wcePtr)->classPartInitPosthook = XmInheritClassPartInitPosthook;
		  (*wcePtr)->initializePrehook 	= XmInheritInitializePrehook;
		  (*wcePtr)->setValuesPrehook 	= XmInheritSetValuesPrehook;
		  (*wcePtr)->getValuesPrehook 	= XmInheritGetValuesPrehook;
		  (*wcePtr)->initializePosthook = XmInheritInitializePosthook;
		  (*wcePtr)->setValuesPosthook 	= XmInheritSetValuesPosthook;
		  (*wcePtr)->getValuesPosthook 	= XmInheritGetValuesPosthook;
		  (*wcePtr)->secondaryObjectClass = XmInheritClass;
		  (*wcePtr)->secondaryObjectCreate = XmInheritSecObjectCreate;
		  (*wcePtr)->getSecResData 	= XmInheritGetSecResData;
                  (*wcePtr)->widgetNavigable    = XmInheritWidgetNavigable;
                  (*wcePtr)->focusChange        = XmInheritFocusChange;
		  }
		else
		  inited = True;

		/* this should get done by the static initializers */
		for (i = 0; i < 32; i++)
		  (*wcePtr)->flags[i] = 0;

		if (scePtr && *scePtr)
		  {
		  if (!inited)
			{
			(*wcePtr)->next_extension = NULL;
			(*wcePtr)->record_type 	= (*scePtr)->record_type;
			(*wcePtr)->version	= (*scePtr)->version;
			(*wcePtr)->record_size	= (*scePtr)->record_size;
			}
		  if ((*wcePtr)->classPartInitPrehook == XmInheritClassPartInitPrehook)
			(*wcePtr)->classPartInitPrehook = (*scePtr)->classPartInitPrehook;
		  if ((*wcePtr)->classPartInitPosthook == XmInheritClassPartInitPosthook)
			(*wcePtr)->classPartInitPosthook = (*scePtr)->classPartInitPosthook;
		  if ((*wcePtr)->initializePrehook == XmInheritInitializePrehook)
			(*wcePtr)->initializePrehook = (*scePtr)->initializePrehook;
		  if ((*wcePtr)->setValuesPrehook == XmInheritSetValuesPrehook)
			(*wcePtr)->setValuesPrehook = (*scePtr)->setValuesPrehook;
		  if ((*wcePtr)->getValuesPrehook == XmInheritGetValuesPrehook)
			(*wcePtr)->getValuesPrehook = (*scePtr)->getValuesPrehook;
		  if ((*wcePtr)->initializePosthook == XmInheritInitializePosthook)
			(*wcePtr)->initializePosthook = (*scePtr)->initializePosthook;
		  if ((*wcePtr)->setValuesPosthook == XmInheritSetValuesPosthook)
			(*wcePtr)->setValuesPosthook = (*scePtr)->setValuesPosthook;
		  if ((*wcePtr)->getValuesPosthook == XmInheritGetValuesPosthook)
			(*wcePtr)->getValuesPosthook = (*scePtr)->getValuesPosthook;
		  if ((*wcePtr)->secondaryObjectClass == XmInheritClass)
			(*wcePtr)->secondaryObjectClass = (*scePtr)->secondaryObjectClass;
		  if ((*wcePtr)->secondaryObjectCreate == XmInheritSecObjectCreate)
			(*wcePtr)->secondaryObjectCreate = (*scePtr)->secondaryObjectCreate;
		  if ((*wcePtr)->getSecResData == XmInheritGetSecResData)
			(*wcePtr)->getSecResData = (*scePtr)->getSecResData;
		  if ((*wcePtr)->widgetNavigable == XmInheritWidgetNavigable)
			(*wcePtr)->widgetNavigable = (*scePtr)->widgetNavigable;
		  if ((*wcePtr)->focusChange == XmInheritFocusChange)
			(*wcePtr)->focusChange = (*scePtr)->focusChange;
		  }

		/*
		 * if this class has a secondary object class and that
		 * class does not have it's own extension (or has not
		 * been class inited becuase its a pseudo class) then
		 * we will give a dummy pointer so that fast subclass
		 * checking will not fail for the meta classes
		 * (gadget, manager, etc...)
		 */
		{
		    WidgetClass	sec = (*wcePtr)->secondaryObjectClass;
		    static XmBaseClassExtRec       xmExtExtensionRec = {
			NULL,                                     /* Next extension       */
			NULLQUARK,                                /* record type XmQmotif */
			XmBaseClassExtVersion,                    /* version              */
			sizeof(XmBaseClassExtRec),                /* size                 */
		    };

		    if (xmExtExtensionRec.record_type == NULLQUARK) {
			xmExtExtensionRec.record_type = XmQmotif;
		    }

		    if (sec && !sec->core_class.extension)
		      sec->core_class.extension = (XtPointer)&xmExtExtensionRec;
		}
#ifdef DEBUG
		else if (!IsBaseClass(wc))
		  XtError("class must have non-null superclass extension");
#endif /* DEBUG */
	}

	wcData = _XmGetWrapperData(wc);
	scData = _XmGetWrapperData(wc->core_class.superclass);

	if ((wc == vendorShellWidgetClass) ||
		IsSubclassOf(wc, vendorShellWidgetClass))
	{
		/* Wrap Realize */
		/*
		 * check if this widget was using XtInherit and got the wrapper
		 * from the superclass
		 */
		if (wc->core_class.realize == XtInheritRealize)
		{
			wcData->realize = scData->realize;
		}
		/*
		 * It has declared it's own realize routine so save it
		 */
		else
		{
			wcData->realize = wc->core_class.realize;
		}
		wc->core_class.realize = realizeWrappers[GetRealizeDepth(wc)];
	}

	if ((wc == rectObjClass) ||
		IsSubclassOf(wc, rectObjClass))
	{
		/* Wrap resize */
		/*
		 * check if this widget was using XtInherit and got the wrapper
		 * from the superclass
		 */
		if (wc->core_class.resize == XtInheritResize)
		{
			wcData->resize = scData->resize;
		}
		/*
		 * It has declared it's own resize routine so save it
		 */
		else
		{
			wcData->resize = wc->core_class.resize;
		}
		wc->core_class.resize = resizeWrappers[GetResizeDepth(wc)];
	}

	 if ((wc == compositeWidgetClass) ||
		IsSubclassOf(wc, compositeWidgetClass))
	{
		/* Wrap GeometryManager */
		/*
		 * check if this widget was using XtInherit and got the wrapper
		 * from the superclass
		 */
		if (((CompositeWidgetClass) wc)->
                                               composite_class.geometry_manager
                                                   == XtInheritGeometryManager)
		{
			wcData->geometry_manager = scData->geometry_manager;
		}
		/*
		 * It has declared it's own resize routine so save it
		 */
		else
		{
			wcData->geometry_manager = ((CompositeWidgetClass) wc)
				->composite_class.geometry_manager;
		}
		((CompositeWidgetClass) wc)->composite_class.geometry_manager =
			(XtGeometryHandler) geometryHandlerWrappers[
				GetGeometryHandlerDepth(wc)];
	}

    return wcePtr;
}


/*
 * This function replaces the objectClass classPartInit slot and is
 * called at the start of the first XtCreate invocation.
 */
static void 
#ifdef _NO_PROTO
ClassPartInitRootWrapper( wc )
        WidgetClass wc ;
#else
ClassPartInitRootWrapper(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    XtWidgetClassProc			*leafFuncPtr;
    XmBaseClassExt			*wcePtr;


    wcePtr = BaseClassPartInitialize(wc);
    /*
     * check that it's a class that we know about
     */
    if (wcePtr && *wcePtr)
      {
	  if ((*wcePtr)->classPartInitPrehook)
	    (*((*wcePtr)->classPartInitPrehook)) (wc);

	  /*
	   * if we have a prehook, then envelop the leaf class function
	   * that whould be called last. 
	   */
	  if ((*wcePtr)->classPartInitPosthook)
	    {
		XmWrapperData 		wrapperData;

		wrapperData = _XmGetWrapperData(wc);
		leafFuncPtr = (XtWidgetClassProc *)
		  &(wc->core_class.class_part_initialize);
		wrapperData->classPartInitLeaf = *leafFuncPtr;
		*leafFuncPtr = ClassPartInitLeafWrapper;
	    }
      }
    if (objectClassWrapper.classPartInit)
      (* objectClassWrapper.classPartInit) (wc);
}

static void 
#ifdef _NO_PROTO
ClassPartInitLeafWrapper( wc )
        WidgetClass wc ;
#else
ClassPartInitLeafWrapper(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    XtWidgetClassProc			*leafFuncPtr;
    XmBaseClassExt		*wcePtr;

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif); 
    
    if (*wcePtr && (*wcePtr)->classPartInitPosthook)
      {
	  XmWrapperData 		wrapperData;
	  wrapperData = _XmGetWrapperData(wc);
	  leafFuncPtr = (XtWidgetClassProc *)
	    &(wc->core_class.class_part_initialize);
	  
	  if (wrapperData->classPartInitLeaf)
	    (* wrapperData->classPartInitLeaf) (wc);
	  if ((*wcePtr)->classPartInitPosthook)
	    (*((*wcePtr)->classPartInitPosthook)) (wc);
#ifdef DEBUG
	  else
	    _XmWarning(NULL, "there should be a non-null hook for a leaf wrapper");
#endif /* DEBUG */
	  *leafFuncPtr = wrapperData->classPartInitLeaf;
	  wrapperData->classPartInitLeaf = NULL;
      }
}

/*
 * This function replaces the objectClass initialize slot and is
 * called at the start of every XtCreate invocation.
 */
static void 
#ifdef _NO_PROTO
InitializeRootWrapper( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InitializeRootWrapper(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XtInitProc			*leafFuncPtr;
    XmBaseClassExt		*wcePtr;
    WidgetClass			wc = XtClass(new_w);
 
    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif); 

    /*
     * check that it's a class that we know about
     */
    if (wcePtr && *wcePtr)
      {
	  if ((*wcePtr)->initializePrehook)
	    (*((*wcePtr)->initializePrehook)) (req, new_w, args,  num_args);

	  /*
	   * if we have a prehook, then envelop the leaf class function
	   * that whould be called last. This is different if the
	   * parent is a constraint subclass, since constraint
	   * initialize is called after core initialize.
	   */
	  if ((*wcePtr)->initializePosthook)
	    {
		XmWrapperData 		wrapperData;

		if (!XtIsShell(new_w) && XtParent(new_w) && XtIsConstraint(XtParent(new_w)))
		  {
		      ConstraintWidgetClass cwc;

		      cwc = (ConstraintWidgetClass)XtClass(XtParent(new_w));
		      wrapperData = _XmPushWrapperData((WidgetClass) cwc);
		      leafFuncPtr = (XtInitProc *)
			&(cwc->constraint_class.initialize);
		  }
		else
		  {
		      wrapperData = _XmPushWrapperData(wc);
		      leafFuncPtr = (XtInitProc *)
			&(wc->core_class.initialize);
		  }
		if (!wrapperData->next || 
		    !wrapperData->next->initializeLeaf ||
	            DiffWrapperClasses(wrapperData))
	          {
		      if (*leafFuncPtr)
                         wrapperData->initializeLeaf = *leafFuncPtr;
		      else
			 wrapperData->initializeLeaf = nestingCompare.initialize;

		      *leafFuncPtr = InitializeLeafWrapper;
	          }
	    }
      }
    if (objectClassWrapper.initialize)
      (* objectClassWrapper.initialize) (req, new_w, args, num_args);
}

static Boolean 
#ifdef _NO_PROTO
is_constraint_subclass(cls)
     WidgetClass cls;
#else
is_constraint_subclass(WidgetClass cls)
#endif /* _NO_PROTO */
{
  WidgetClass sc;

  for (sc = cls; sc != NULL; sc = sc->core_class.superclass)
    if (sc == (WidgetClass) &constraintClassRec)
      return True;

  return False;
}

static void 
#ifdef _NO_PROTO
InitializeLeafWrapper( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InitializeLeafWrapper(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XtInitProc			*leafFuncPtr;
    XmBaseClassExt		*wcePtr;
    WidgetClass			wc = XtClass(new_w);

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif); 

    if (*wcePtr && (*wcePtr)->initializePosthook)
      {
	  XmWrapperData 		wrapperData;
	  
	  if (!XtIsShell(new_w) && XtParent(new_w) && XtIsConstraint(XtParent(new_w)))
	    {
	        WidgetClass sc;
		ConstraintWidgetClass cwc;
		
		cwc = (ConstraintWidgetClass)XtClass(XtParent(new_w));
		wrapperData = _XmGetWrapperData((WidgetClass) cwc);
		leafFuncPtr = (XtInitProc *)
		  &(cwc->constraint_class.initialize);

		/* Other superclasses may be wrapped too! */
		if (wrapperData->init_depth == 0)
		  {
		    /* Initialize the count of wrapped classes. */
		    for (sc = (WidgetClass) cwc;
			 sc != NULL;
			 sc = sc->core_class.superclass)
		      if (is_constraint_subclass(sc) &&
			  (((ConstraintWidgetClass) sc)->
			   constraint_class.initialize ==InitializeLeafWrapper))
			(wrapperData->init_depth)++;
		  }

		if (wrapperData->init_depth > 1)
		  {
		    /* This is not the bottom-most wrapped class. */
		    int count = wrapperData->init_depth;
		    for (sc = (WidgetClass) cwc;
			 sc != NULL;
			 sc = sc->core_class.superclass)
		      if (is_constraint_subclass(sc) &&
			  (((ConstraintWidgetClass) sc)->
			   constraint_class.initialize==InitializeLeafWrapper)&&
			  (--count == 0))
			{
			  /* This should be the class we're really */
			  /* trying to initialize. */
			  (wrapperData->init_depth)--;

			  wrapperData = _XmGetWrapperData(sc);
			  if (wrapperData->initializeLeaf &&
			      (wrapperData->initializeLeaf !=
			       nestingCompare.initialize))
			    (* wrapperData->initializeLeaf)
			      (req, new_w, args, num_args);
			  return;
			}
		  }
		else
		  {
		    /* This is the final call.  Proceed normally. */
		    /* assert(wrapperData->init_depth == 1); */
		    (void) _XmPopWrapperData((WidgetClass) cwc);
		  }
	    }
	  else
	    {
		wrapperData = _XmPopWrapperData(wc);
		leafFuncPtr = (XtInitProc *)
		  &(wc->core_class.initialize);
	    }
	  if (wrapperData->initializeLeaf &&
	      (wrapperData->initializeLeaf != nestingCompare.initialize))
	    (* wrapperData->initializeLeaf) (req, new_w, args, num_args);
	  if ((*wcePtr)->initializePosthook)
	    (*((*wcePtr)->initializePosthook)) (req, new_w, args,  num_args);
#ifdef DEBUG
	  else
	    _XmWarning(NULL, "there should be a non-null hook for a leaf wrapper");
#endif /* DEBUG */
	  if (!wrapperData->next || 
              !wrapperData->next->initializeLeaf || 
	      DiffWrapperClasses(wrapperData))
	    {
		if (wrapperData->initializeLeaf != nestingCompare.initialize)
	        	*leafFuncPtr = wrapperData->initializeLeaf;
		else
			*leafFuncPtr = NULL;
	    }
	  XtFree((char *)wrapperData);
      }
}


/*
 * This function replaces the objectClass set_values slot and is
 * called at the start of every XtSetValues invocation.
 */
static Boolean 
#ifdef _NO_PROTO
SetValuesRootWrapper( current, req, new_w, args, num_args )
        Widget current ;
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesRootWrapper(
        Widget current,
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XtSetValuesFunc		*leafFuncPtr;
    XmBaseClassExt		*wcePtr;
    WidgetClass			wc = XtClass(new_w);
    Boolean			returnVal = False;

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif); 
    /*
     * check that it's a class that we know about
     */
    if (wcePtr && (*wcePtr))
      {
	  if ((*wcePtr)->setValuesPrehook)
	    returnVal |= 
	      (*((*wcePtr)->setValuesPrehook)) (current, req, new_w, args,  num_args);

	  /*
	   * if we have a prehook, then envelop the leaf class function
	   * that whould be called last. This is different if the
	   * parent is a constraint subclass, since constraint
	   * set_values is called after core set_values.
	   */
	  if ((*wcePtr)->setValuesPosthook)
	    {
		XmWrapperData 		wrapperData;

		if (!XtIsShell(new_w) && XtParent(new_w) && XtIsConstraint(XtParent(new_w)))
		  {
		      ConstraintWidgetClass cwc;
		      
		      cwc = (ConstraintWidgetClass)XtClass(XtParent(new_w));
		      wrapperData = _XmPushWrapperData((WidgetClass) cwc);
		      leafFuncPtr = (XtSetValuesFunc *)
			&(cwc->constraint_class.set_values);
		  }
		else
		  {
		      wrapperData = _XmPushWrapperData(wc);
		      leafFuncPtr = (XtSetValuesFunc *)
			&(wc->core_class.set_values);
		  }
		if (!wrapperData->next || 
	           !wrapperData->next->setValuesLeaf ||
		   DiffWrapperClasses(wrapperData))
		  {
		      if (*leafFuncPtr)
		         wrapperData->setValuesLeaf = *leafFuncPtr;
		      else
			 wrapperData->setValuesLeaf = nestingCompare.setValues;
		      *leafFuncPtr = SetValuesLeafWrapper;
		  }
	    }
      }
    if (objectClassWrapper.setValues)
      returnVal |= 
	(* objectClassWrapper.setValues) (current, req, new_w, args,
					  num_args);
    return returnVal;
}

static Boolean 
#ifdef _NO_PROTO
SetValuesLeafWrapper( current, req, new_w, args, num_args )
        Widget current ;
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesLeafWrapper(
        Widget current,
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XtSetValuesFunc		*leafFuncPtr;
    XmBaseClassExt		*wcePtr;
    XmWrapperData 		wrapperData;
    Boolean			returnVal = False;
    WidgetClass			wc = XtClass(new_w);

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif); 

    if (*wcePtr && (*wcePtr)->setValuesPosthook)
      {
	  
	  if (!XtIsShell(new_w) && XtParent(new_w) && XtIsConstraint(XtParent(new_w)))
	    {
		ConstraintWidgetClass cwc;
		
		cwc = (ConstraintWidgetClass)XtClass(XtParent(new_w));
		wrapperData = _XmPopWrapperData((WidgetClass) cwc);
		leafFuncPtr = (XtSetValuesFunc *)
		  &(cwc->constraint_class.set_values);
	    }
	  else
	    {
		wrapperData = _XmPopWrapperData(wc);
		leafFuncPtr = (XtSetValuesFunc *)
		  &(wc->core_class.set_values);
	    }
	  if (wrapperData->setValuesLeaf &&
	      (wrapperData->setValuesLeaf != nestingCompare.setValues))
	    returnVal = (* wrapperData->setValuesLeaf) (current, req, new_w, args, num_args);
	  if ((*wcePtr)->setValuesPosthook)
	    returnVal |= 
	      (*((*wcePtr)->setValuesPosthook)) (current, req, new_w, args,  num_args);
#ifdef DEBUG
	  else
	    _XmWarning(NULL, "there should be a non-null hook for a leaf wrapper");
#endif /* DEBUG */

	  if (!wrapperData->next || !wrapperData->next->setValuesLeaf ||
	      DiffWrapperClasses(wrapperData))
	    {
		if (wrapperData->setValuesLeaf != nestingCompare.setValues)
	        	*leafFuncPtr = wrapperData->setValuesLeaf;
		else
			*leafFuncPtr = NULL;
	    }
	  XtFree((char *)wrapperData);
      }
    return returnVal;
}

/*
 * This function replaces the objectClass get_values slot and is
 * called at the start of every XtGetValues invocation.
 */
static void 
#ifdef _NO_PROTO
GetValuesRootWrapper( new_w, args, num_args )
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesRootWrapper(
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XtArgsProc			*leafFuncPtr;
    XmBaseClassExt		*wcePtr;
    WidgetClass			wc = XtClass(new_w);
 
    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif); 
    /*
     * check that it's a class that we know about
     */
    if (wcePtr && (*wcePtr))
      {
	  if ((*wcePtr)->getValuesPrehook)
	    (*((*wcePtr)->getValuesPrehook)) (new_w, args,  num_args);

	  /*
	   * if we have a prehook, then envelop the leaf class function
	   * that whould be called last. This is different if the
	   * parent is a constraint subclass, since constraint
	   * get_values is called after core get_values.
	   */
	  if ((*wcePtr)->getValuesPosthook)
	    {
		XmWrapperData 		wrapperData;

#ifdef SUPPORT_GET_VALUES_EXT
		if (!XtIsShell(new_w) && XtParent(new_w) && XtIsConstraint(XtParent(new_w)))
		  {
		      ConstraintWidgetClass cwc;
		      
		      cwc = (ConstraintWidgetClass)XtClass(XtParent(new_w));
		      wrapperData = _XmPushWrapperData((WidgetClass)cwc);
		      leafFuncPtr = (XtGetValuesFunc *)
			&(cwc->constraint_class.get_values);
		  }
		else
#endif		  
		  {
		      wrapperData = _XmPushWrapperData(wc);
		      leafFuncPtr = (XtArgsProc *)
			&(wc->core_class.get_values_hook);
		  }
		wrapperData->getValuesLeaf = *leafFuncPtr;
		*leafFuncPtr = GetValuesLeafWrapper;
	    }
      }
    if (objectClassWrapper.getValues)
      (* objectClassWrapper.getValues) (new_w, args, num_args);
}

static void 
#ifdef _NO_PROTO
GetValuesLeafWrapper( new_w, args, num_args )
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesLeafWrapper(
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XtArgsProc			*leafFuncPtr;
    XmBaseClassExt		*wcePtr;
    XmWrapperData 		wrapperData;
    WidgetClass			wc = XtClass(new_w);

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif); 

    if (*wcePtr && (*wcePtr)->getValuesPosthook)
      {
#ifdef SUPPORT_GET_VALUES_EXT
	  if (!XtIsShell(new_w) && XtParent(new_w) && XtIsConstraint(XtParent(new_w)))
	    {
		ConstraintWidgetClass cwc;
		
		cwc = (ConstraintWidgetClass)XtClass(XtParent(new_w));
		wrapperData = _XmPopWrapperData(cwc);
		leafFuncPtr = (XtSetValuesFunc *)
		  &(cwc->constraint_class.get_values);
	    }
	  else
#endif
	    {
		wrapperData = _XmPopWrapperData(wc);
		leafFuncPtr = (XtArgsProc *)
		  &(wc->core_class.get_values_hook);
	    }
	  if (wrapperData->getValuesLeaf)
	    (* wrapperData->getValuesLeaf) (new_w, args, num_args);
	  if ((*wcePtr)->getValuesPosthook)
	    (*((*wcePtr)->getValuesPosthook)) (new_w, args,  num_args);
#ifdef DEBUG
	  else
	    _XmWarning(NULL, "there should be a non-null hook for a leaf wrapper");
#endif /* DEBUG */
	  if (!wrapperData->next || !wrapperData->next->getValuesLeaf)
	    {
		*leafFuncPtr = wrapperData->getValuesLeaf;
	    }
	  XtFree((char *)wrapperData);
      }
}

void 
#ifdef _NO_PROTO
_XmInitializeExtensions()
#else
_XmInitializeExtensions( void )
#endif /* _NO_PROTO */
{
    static Boolean		firstTime = True;

    if (firstTime)
      {
	  XmQmotif = XrmStringToQuark("OSF_MOTIF");

	  objectClassWrapper.initialize =
	    objectClass->core_class.initialize;
	  objectClassWrapper.setValues =
	    objectClass->core_class.set_values;
	  objectClassWrapper.getValues =
	    objectClass->core_class.get_values_hook;
	  objectClassWrapper.classPartInit =
	    objectClass->core_class.class_part_initialize;
	  objectClass->core_class.class_part_initialize = 
	    ClassPartInitRootWrapper;
	  objectClass->core_class.initialize = 
	    InitializeRootWrapper;
	  objectClass->core_class.set_values = 
	    SetValuesRootWrapper;
	  objectClass->core_class.get_values_hook =
	    GetValuesRootWrapper;
	  firstTime = False;

	  nestingCompare.initialize = InitializeLeafWrapper;
	  nestingCompare.setValues = SetValuesLeafWrapper;
	  nestingCompare.getValues = GetValuesLeafWrapper;
      }
}

Boolean
#ifdef _NO_PROTO
_XmIsStandardMotifWidgetClass( wc)
        WidgetClass wc ;
#else
_XmIsStandardMotifWidgetClass(
        WidgetClass wc)
#endif /* _NO_PROTO */
{
  /* This routine depends on ALL standard Motif classes use fast subclassing.
   */
  XmBaseClassExt * fastPtr ;
  XmBaseClassExt * superFastPtr ;
  WidgetClass super_wc = wc->core_class.superclass ;

  if(    (fastPtr = _XmGetBaseClassExtPtr( wc, XmQmotif))
     &&  *fastPtr    )
    {
      if(    !(superFastPtr = _XmGetBaseClassExtPtr( super_wc, XmQmotif))    )
        {
          /* This catches all Motif classes which are direct subclasses
           * of an Xt Intrinsics widget class.
           */
          return( TRUE) ;
        }
      if(    *superFastPtr    )
        {
          unsigned char * flags = (*fastPtr)->flags ;
          unsigned char * superFlags = (*superFastPtr)->flags ;
          unsigned numBytes = (XmLAST_FAST_SUBCLASS_BIT >> 3) + 1 ;
          unsigned Index ;

          Index = numBytes ;
          while(    Index--    )
            {
              if(    flags[Index] != superFlags[Index]    )
                {
                  /* Since all Motif classes use fast subclassing, the fast
                   * subclassing bits of any standard Motif class will be
                   * different than those of its superclass (the superclass
                   * will have one less bit set).  Any non-standard Motif
                   * subclass will have the same fast subclass bits as its
                   * superclass.
                   */
                  return( TRUE) ;
                }
            }
        }
    }
  return( FALSE) ;
}


Cardinal 
#ifdef _NO_PROTO
XmGetSecondaryResourceData( w_class, secondaryDataRtn )
        WidgetClass w_class ;
        XmSecondaryResourceData **secondaryDataRtn ;
#else
XmGetSecondaryResourceData(
        WidgetClass w_class,
        XmSecondaryResourceData **secondaryDataRtn )
#endif /* _NO_PROTO */
{
    int num;

    num =  GetSecResData(w_class, secondaryDataRtn);

    return (num);
}


/*
 * GetSecResData()
 *  - Called from : XmGetSecondaryResourceData ().
 */
static Cardinal 
#ifdef _NO_PROTO
GetSecResData( w_class, secResDataRtn )
        WidgetClass w_class ;
        XmSecondaryResourceData **secResDataRtn ;
#else
GetSecResData(
        WidgetClass w_class,
        XmSecondaryResourceData **secResDataRtn )
#endif /* _NO_PROTO */
{
    XmBaseClassExt  *bcePtr;   /*  bcePtr is really **XmBaseClassExtRec */
    Cardinal count = 0;

    bcePtr = _XmGetBaseClassExtPtr( w_class, XmQmotif); 
    if ((bcePtr) && (*bcePtr) && ((*bcePtr)->getSecResData))
	count = ( (*bcePtr)->getSecResData)( w_class, secResDataRtn);
    return (count);
}

Cardinal 
#ifdef _NO_PROTO
_XmSecondaryResourceData( bcePtr, secResDataRtn, client_data, name, class_name, basefunctionpointer )
        XmBaseClassExt bcePtr ;
        XmSecondaryResourceData **secResDataRtn ;
        XtPointer client_data ;
        String name ;
        String class_name ;
        XmResourceBaseProc basefunctionpointer ;
#else
_XmSecondaryResourceData(
        XmBaseClassExt bcePtr,
        XmSecondaryResourceData **secResDataRtn,
        XtPointer client_data,
        String name,
        String class_name,
        XmResourceBaseProc basefunctionpointer )
#endif /* _NO_PROTO */
{
    WidgetClass     secObjClass;
    XmSecondaryResourceData   secResData, *sd;
    Cardinal count = 0;

    if ( (bcePtr)  )
    { secObjClass = ( (bcePtr)->secondaryObjectClass);
      if (secObjClass)
      {
        secResData = XtNew(XmSecondaryResourceDataRec);

        _XmTransformSubResources(secObjClass->core_class.resources,
			         secObjClass->core_class.num_resources,
                                 &(secResData->resources),
				 &(secResData->num_resources));

        secResData->name = name;
        secResData->res_class = class_name;
        secResData->client_data = client_data;
        secResData->base_proc = basefunctionpointer;
        sd = (XmSecondaryResourceData *)
                XtMalloc ( sizeof (XmSecondaryResourceData )); 
	    *sd = secResData;
        *secResDataRtn = sd;
        count++;
      }
    }
   return (count);
}


/*
 * This function makes assumptions about what the Intrinsics is
 * doing with the resource lists.  It is based on the X11R5 version
 * of the Intrinsics.  It is used as a work around for a bug in
 * the Intrinsics that deals with recompiling already compiled resource
 * lists.  When that bug is fixed, this function should be removed.
 */
static XtResourceList*
#ifdef _NO_PROTO
XmCreateIndirectionTable (resources, num_resources)
    XtResourceList  resources;
    Cardinal        num_resources;
#else
XmCreateIndirectionTable (XtResourceList resources, Cardinal num_resources)
#endif /* _NO_PROTO */
{
    register int i;
    XtResourceList* table;

    table = (XtResourceList*)XtMalloc(num_resources * sizeof(XtResourceList));
    for (i = 0; i < num_resources; i++)
        table[i] = (XtResourceList)(&(resources[i]));
    return table;
}


/*
  * The statement in this function calls XmCreateIndirectionTable() which
  * is scheduled for removal (see comment in function above).
  * It is used as a work around for an X11R5 Intrinsics bug.  When the
  * bug is fixed, change to the assignement statement so the 
  * constraint_class.reources is assigned comp_resources.  Next,
  * change the class_inited field of the core_class record to False.
  * Then remove the check for class_inited, in the conditional statement
  * which calls XtInitializeWidgetClass() and move the call to
  * XtInitializeWidgetClass() to just above the call to
  * XtGetConstraintResourceList().  Then remove the call to
  * XtFree() and the last two assignment statements.
  */
void
#ifdef _NO_PROTO
_XmTransformSubResources(comp_resources, num_comp_resources,
			 resources, num_resources)
	XtResourceList comp_resources;
	Cardinal num_comp_resources;
	XtResourceList *resources;
	Cardinal *num_resources;
#else
_XmTransformSubResources(
	XtResourceList comp_resources,
	Cardinal num_comp_resources,
	XtResourceList *resources,
	Cardinal *num_resources)
#endif /* _NO_PROTO */
{
   static ConstraintClassRec shadowObjectClassRec = {
      {
       /* superclass         */    (WidgetClass) &constraintClassRec,
       /* class_name         */    "Shadow",
       /* widget_size        */    sizeof(ConstraintRec),
       /* class_initialize   */    NULL,
       /* class_part_initialize*/  NULL,
       /* class_inited       */    FALSE,
       /* initialize         */    NULL,
       /* initialize_hook    */    NULL,
       /* realize              */  XtInheritRealize,
       /* actions              */  NULL,
       /* num_actions          */  0,
       /* resources            */  NULL,
       /* num_resources        */  0,
       /* xrm_class            */  NULLQUARK,
       /* compress_motion      */  FALSE,
       /* compress_exposure    */  TRUE,
       /* compress_enterleave  */  FALSE,
       /* visible_interest     */  FALSE,
       /* destroy              */  NULL,
       /* resize               */  NULL,
       /* expose               */  NULL,
       /* set_values           */  NULL,
       /* set_values_hook      */  NULL,
       /* set_values_almost    */  XtInheritSetValuesAlmost,
       /* get_values_hook      */  NULL,
       /* accept_focus         */  NULL,
       /* version              */  XtVersion,
       /* callback_offsets     */  NULL,
       /* tm_table             */  NULL,
       /* query_geometry       */  NULL,
       /* display_accelerator  */  NULL,
       /* extension            */  NULL
     },

     { /**** CompositePart *****/
       /* geometry_handler     */  NULL,
       /* change_managed       */  NULL,
       /* insert_child         */  XtInheritInsertChild,
       /* delete_child         */  XtInheritDeleteChild,
       /* extension            */  NULL
     },

     { /**** ConstraintPart ****/
       /* resources            */  NULL,
       /* num_resources        */  0,
       /* constraint_size      */  0,
       /* initialize           */  NULL,
       /* destroy              */  NULL,
       /* set_values           */  NULL,
       /* extension            */  NULL
     }
  };

  if (((int)comp_resources[0].resource_offset) >= 0) {
     XtResourceList tmp_resources;

     tmp_resources = (XtResourceList)
		 XtMalloc(sizeof(XtResource) * num_comp_resources);

     memcpy(tmp_resources, comp_resources,
	    sizeof(XtResource) * num_comp_resources);

     *resources = tmp_resources;
     *num_resources = num_comp_resources;
  } else {
     if (!shadowObjectClassRec.core_class.class_inited)
        XtInitializeWidgetClass((WidgetClass) &shadowObjectClassRec);

     /* This next statement is marked for change */
     shadowObjectClassRec.constraint_class.resources = (XtResourceList)
	         XmCreateIndirectionTable(comp_resources, num_comp_resources);

     shadowObjectClassRec.constraint_class.num_resources = num_comp_resources;
  
     XtGetConstraintResourceList((WidgetClass) &shadowObjectClassRec,
			          resources, num_resources);

     if (shadowObjectClassRec.constraint_class.resources)
        XtFree((char *) shadowObjectClassRec.constraint_class.resources);

     shadowObjectClassRec.constraint_class.resources = NULL;
     shadowObjectClassRec.constraint_class.num_resources = 0;
  }
}

