#pragma ident	"@(#)m1.2libs:Xm/DropSMgr.h	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmDropSMgr_h
#define _XmDropSMgr_h

#include <Xm/Xm.h>
#include <Xm/DragC.h>

#ifdef __cplusplus
extern "C" {
#endif


#define XmCR_DROP_SITE_LEAVE_MESSAGE  1
#define XmCR_DROP_SITE_ENTER_MESSAGE  2
#define XmCR_DROP_SITE_MOTION_MESSAGE 3
#define XmCR_DROP_MESSAGE             4

#define XmNO_DROP_SITE 		1
#define XmINVALID_DROP_SITE	2
#define XmVALID_DROP_SITE	3

/* begin fix for CR 5754 */
/* documented values are XmDROP_SITE_VALID and XmDROP_SITE_INVALID.
   However, we can't just throw out the incorrect Xm[IN]VALID_DROP_SITE
   now since people have probably started using them. Instead, we just
   define the correct values using the incorrect ones.
*/

#define XmDROP_SITE_INVALID XmINVALID_DROP_SITE
#define XmDROP_SITE_VALID XmVALID_DROP_SITE

/* end fix for CR 5754 */

enum { XmDRAG_UNDER_NONE, XmDRAG_UNDER_PIXMAP,
    XmDRAG_UNDER_SHADOW_IN, XmDRAG_UNDER_SHADOW_OUT,
    XmDRAG_UNDER_HIGHLIGHT };

enum { XmDROP_SITE_SIMPLE, XmDROP_SITE_COMPOSITE,
    XmDROP_SITE_SIMPLE_CLIP_ONLY = 128,
    XmDROP_SITE_COMPOSITE_CLIP_ONLY };

enum { XmABOVE, XmBELOW };

enum { XmDROP_SITE_ACTIVE, XmDROP_SITE_INACTIVE };

typedef struct _XmDragProcCallbackStruct {
    int				reason;
    XEvent *		event;
    Time			timeStamp;
	Widget			dragContext;
    Position		x, y;
    unsigned char	dropSiteStatus;
    unsigned char	operation;
    unsigned char	operations;
	Boolean			animate;
} XmDragProcCallbackStruct, * XmDragProcCallback;

typedef struct _XmDropProcCallbackStruct {
    int				reason;
    XEvent *		event;
    Time			timeStamp;
	Widget			dragContext;
    Position		x, y;
    unsigned char	dropSiteStatus;
    unsigned char	operation;
    unsigned char	operations;
	unsigned char	dropAction;
} XmDropProcCallbackStruct, * XmDropProcCallback;


typedef struct _XmDropSiteVisualsRec {
	Pixel	background;
	Pixel	foreground;
	Pixel	topShadowColor;
	Pixmap	topShadowPixmap;
	Pixel	bottomShadowColor;
	Pixmap	bottomShadowPixmap;
	Dimension	shadowThickness;
	Pixel	highlightColor;
	Pixmap	highlightPixmap;
	Dimension	highlightThickness;
	Dimension	borderWidth;
} XmDropSiteVisualsRec, * XmDropSiteVisuals;


/* DropSite Widget */

externalref WidgetClass xmDropSiteManagerObjectClass;

typedef struct _XmDropSiteManagerClassRec *XmDropSiteManagerObjectClass;
typedef struct _XmDropSiteManagerRec *XmDropSiteManagerObject;

#ifndef XmIsDropSiteManager
#define XmIsDropSiteManager(w)  XtIsSubclass((w), xmDropSiteManagerObjectClass)
#endif /* XmIsDropSite */

/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void XmDropSiteRegister() ;
extern void XmDropSiteUnregister() ;
extern void XmDropSiteStartUpdate() ;
extern void XmDropSiteUpdate() ;
extern void XmDropSiteEndUpdate() ;
extern void XmDropSiteRetrieve() ;
extern int XmDropSiteQueryStackingOrder() ;
extern void XmDropSiteConfigureStackingOrder() ;
extern XmDropSiteVisuals XmDropSiteGetActiveVisuals() ;

#else

extern void XmDropSiteRegister( 
                        Widget widget,
                        ArgList args,
                        Cardinal argCount) ;
extern void XmDropSiteUnregister( 
                        Widget widget) ;
extern void XmDropSiteStartUpdate( 
                        Widget refWidget) ;
extern void XmDropSiteUpdate( 
                        Widget enclosingWidget,
                        ArgList args,
                        Cardinal argCount) ;
extern void XmDropSiteEndUpdate( 
                        Widget refWidget) ;
extern void XmDropSiteRetrieve( 
                        Widget enclosingWidget,
                        ArgList args,
                        Cardinal argCount) ;
extern int XmDropSiteQueryStackingOrder( 
                        Widget widget,
                        Widget *parent_rtn,
                        Widget **children_rtn,
                        Cardinal *num_children_rtn) ;
extern void XmDropSiteConfigureStackingOrder( 
                        Widget widget,
                        Widget sibling,
                        Cardinal stack_mode) ;
extern XmDropSiteVisuals XmDropSiteGetActiveVisuals( 
                        Widget widget) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDropSMgr_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
