#pragma ident	"@(#)m1.2libs:Xm/TraversalI.h	1.2"
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
#ifndef _XmTraversalI_h
#define _XmTraversalI_h

#include "XmI.h"

#ifdef __cplusplus
extern "C" {
#endif


#define NavigTypeIsTabGroup(navigation_type) \
  ((navigation_type == XmTAB_GROUP) || \
   (navigation_type == XmSTICKY_TAB_GROUP) || \
   (navigation_type == XmEXCLUSIVE_TAB_GROUP))


typedef enum {
    XmUnrelated,
    XmMyAncestor,
    XmMyDescendant,
    XmMyCousin,
    XmMySelf
}XmGeneology;

typedef struct _XmTravGraphRec
{   
    union _XmTraversalNodeRec *head ;
    Widget top ;
    union _XmTraversalNodeRec *current ;
    unsigned short num_entries ;
    unsigned short num_alloc ;
    unsigned short next_alloc ;
    unsigned short exclusive ;
    unsigned short tab_list_alloc ;
    unsigned short num_tab_list ;
    Widget *excl_tab_list ;
    } XmTravGraphRec, * XmTravGraph ;


typedef struct _XmFocusDataRec {
    Widget	active_tab_group;
    Widget	focus_item;
    Widget	old_focus_item;
    Widget	pointer_item;
    Widget	old_pointer_item;
    int		unused1;        /* These fields can be re-used in Motif 1.3. */
    int		unused2;
    Widget *	unused3;
    Cardinal	unused4;
    Boolean	needToFlush;
    XCrossingEvent lastCrossingEvent;
    XmGeneology focalPoint;
    unsigned char focus_policy ; /* Mirrors focus_policy resource when focus */
    XmTravGraphRec trav_graph ;  /*   data retrieved using _XmGetFocusData().*/
    Widget      first_focus ;
} XmFocusDataRec ;

typedef enum
{
  XmTAB_GRAPH_NODE, XmTAB_NODE, XmCONTROL_GRAPH_NODE, XmCONTROL_NODE
} XmTravGraphNodeType ;

typedef union _XmDeferredGraphLink
{
  int offset ;
  struct _XmGraphNodeRec *link ;
} XmDeferredGraphLink ;

typedef struct _XmAnyNodeRec               /* Common */
{
  unsigned char type ;
  XmNavigationType nav_type ;
  XmDeferredGraphLink tab_parent ;
  Widget widget ;
  XRectangle rect ;
  union _XmTraversalNodeRec *next ;
  union _XmTraversalNodeRec *prev ;
} XmAnyNodeRec, *XmAnyNode ;

typedef struct _XmControlNodeRec
{
  XmAnyNodeRec any ;
  union _XmTraversalNodeRec *up ;
  union _XmTraversalNodeRec *down ;
} XmControlNodeRec, *XmControlNode ;

typedef struct _XmTabNodeRec
{
  XmAnyNodeRec any ;
} XmTabNodeRec, *XmTabNode ;

typedef struct _XmGraphNodeRec
{
  XmAnyNodeRec any ;
  union _XmTraversalNodeRec *sub_head ;
  union _XmTraversalNodeRec *sub_tail ;
} XmGraphNodeRec, *XmGraphNode ;

typedef union _XmTraversalNodeRec
{
  XmAnyNodeRec any ;
  XmControlNodeRec control ;
  XmTabNodeRec tab ;
  XmGraphNodeRec graph ;
} XmTraversalNodeRec, *XmTraversalNode ;



/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern XmNavigability _XmGetNavigability() ;
extern Boolean _XmIsViewable() ;
extern Widget _XmGetClippingAncestor() ;
extern Widget _XmIsScrollableClipWidget() ;
extern Boolean _XmGetEffectiveView() ;
extern Boolean _XmIntersectionOf() ;
extern XmNavigationType _XmGetNavigationType() ;
extern Widget _XmGetActiveTabGroup() ;
extern Widget _XmTraverseAway() ;
extern Widget _XmTraverse() ;
extern void _XmFreeTravGraph() ;
extern void _XmTravGraphRemove() ;
extern void _XmTravGraphAdd() ;
extern void _XmTravGraphUpdate() ;
extern Boolean _XmNewTravGraph() ;
extern Boolean _XmSetInitialOfTabGraph() ;
extern void _XmTabListAdd() ;
extern void _XmTabListDelete() ;

#else

extern XmNavigability _XmGetNavigability( 
                        Widget wid) ;
extern Boolean _XmIsViewable( 
                        Widget wid) ;
extern Widget _XmGetClippingAncestor( 
                        Widget wid,
                        XRectangle *visRect) ;
extern Widget _XmIsScrollableClipWidget( 
                        Widget wid,
                        XRectangle *visRect) ;
extern Boolean _XmGetEffectiveView( 
                        Widget wid,
                        XRectangle *visRect) ;
extern Boolean _XmIntersectionOf( 
                        register XRectangle *srcRectA,
                        register XRectangle *srcRectB,
                        register XRectangle *destRect) ;
extern XmNavigationType _XmGetNavigationType( 
                        Widget widget) ;
extern Widget _XmGetActiveTabGroup( 
                        Widget wid) ;
extern Widget _XmTraverseAway( 
                        XmTravGraph list,
                        Widget wid,
#if NeedWidePrototypes
                        int wid_is_control) ;
#else
                        Boolean wid_is_control) ;
#endif /* NeedWidePrototypes */
extern Widget _XmTraverse( 
                        XmTravGraph list,
                        XmTraversalDirection action,
                        Widget reference_wid) ;
extern void _XmFreeTravGraph( 
                        XmTravGraph trav_list) ;
extern void _XmTravGraphRemove( 
                        XmTravGraph tgraph,
                        Widget wid) ;
extern void _XmTravGraphAdd( 
                        XmTravGraph tgraph,
                        Widget wid) ;
extern void _XmTravGraphUpdate( 
                        XmTravGraph tgraph,
                        Widget wid) ;
extern Boolean _XmNewTravGraph( 
                        XmTravGraph trav_list,
                        Widget top_wid,
                        Widget init_current) ;
extern Boolean _XmSetInitialOfTabGraph( 
                        XmTravGraph trav_graph,
                        Widget tab_group,
                        Widget init_focus) ;
extern void _XmTabListAdd( 
                        XmTravGraph graph,
                        Widget wid) ;
extern void _XmTabListDelete( 
                        XmTravGraph graph,
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmTraversalI_h */
