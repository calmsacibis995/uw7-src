#pragma ident	"@(#)m1.2libs:Xm/ManagerP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1989, 1990 DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#ifndef _XmManagerP_h
#define _XmManagerP_h

#ifndef _XmNO_BC_INCL
#define _XmNO_BC_INCL
#endif

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Access Macros  */

#define XmParentTopShadowGC(w) (((XmManagerWidget)			\
	(((XmGadget)(w))->object.parent))->manager.top_shadow_GC)

#define XmParentBottomShadowGC(w) (((XmManagerWidget)			\
	(((XmGadget)(w))->object.parent))->manager.bottom_shadow_GC)

#define XmParentHighlightGC(w) (((XmManagerWidget)			\
	(((XmGadget)(w))->object.parent))->manager.highlight_GC)

#define XmParentBackgroundGC(w) (((XmManagerWidget)                     \
        (((XmGadget)(w))->object.parent))->manager.background_GC)

#define MGR_KeyboardList(m)		\
			(((XmManagerRec *) (m))->manager.keyboard_list)
#define MGR_NumKeyboardEntries(m)	\
			(((XmManagerRec *) (m))->manager.num_keyboard_entries)
#define MGR_SizeKeyboardList(m)		\
			(((XmManagerRec *) (m))->manager.size_keyboard_list)
#define MGR_ShadowThickness(m)		\
			(((XmManagerRec *) (m))->manager.shadow_thickness)


#define XmInheritTraversalChildrenProc ((XmTraversalChildrenProc) _XtInherit)

#ifdef _NO_PROTO
typedef Boolean (*XmTraversalChildrenProc)() ;
#else
typedef Boolean (*XmTraversalChildrenProc)( Widget, Widget **, Cardinal *) ;
#endif


/*  Structure used for storing accelerator and mnemonic information.  */

typedef struct 
{
   unsigned int eventType;
   KeySym       keysym;
   KeyCode      key;
   unsigned int modifiers;
   Widget       component;
   Boolean      needGrab;
   Boolean      isMnemonic;
} XmKeyboardData;


/*  The class definition  */

typedef struct {
    XtPointer next_extension;
    XrmQuark record_type;
    long version;
    Cardinal record_size;
    XmTraversalChildrenProc traversal_children ;
} XmManagerClassExtRec, *XmManagerClassExt ;

#define XmManagerClassExtVersion 1L

typedef struct _XmManagerClassPart
{
   String               translations;
   XmSyntheticResource * syn_resources;   
   int                  num_syn_resources;   
   XmSyntheticResource * syn_constraint_resources;   
   int                  num_syn_constraint_resources;   
   XmParentProcessProc  parent_process;
   XtPointer            extension;
} XmManagerClassPart;

typedef struct _XmManagerClassRec
{
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XmManagerClassPart  manager_class;
} XmManagerClassRec;

#ifndef MANAGER
externalref XmManagerClassRec xmManagerClassRec;
#endif

/*  The instance definition  */

typedef struct _XmManagerPart
{
   Pixel   foreground;

   Dimension   shadow_thickness;
   Pixel   top_shadow_color;
   Pixmap  top_shadow_pixmap;
   Pixel   bottom_shadow_color;
   Pixmap  bottom_shadow_pixmap;

   Pixel   highlight_color;
   Pixmap  highlight_pixmap;

   XtCallbackList help_callback;
   XtPointer      user_data;

   Boolean traversal_on;
   unsigned char unit_type;
   XmNavigationType navigation_type;
   
   Boolean event_handler_added;
   Widget  active_child;
   Widget  highlighted_widget;
   Widget  accelerator_widget;

   Boolean has_focus;

   XmStringDirection string_direction;

   XmKeyboardData * keyboard_list;
   short num_keyboard_entries;
   short size_keyboard_list;

   XmGadget selected_gadget;
   XmGadget eligible_for_multi_button_event;

   GC      background_GC;
   GC      highlight_GC;
   GC      top_shadow_GC;
   GC      bottom_shadow_GC;

   Widget  initial_focus;
} XmManagerPart;

typedef struct _XmManagerRec
{
   CorePart       core;
   CompositePart  composite;
   ConstraintPart constraint;
   XmManagerPart  manager;
} XmManagerRec;


/*  The constraint definition  */

typedef struct _XmManagerConstraintPart
{
   int unused;
} XmManagerConstraintPart;

typedef struct _XmManagerConstraintRec
{
   XmManagerConstraintPart manager;
} XmManagerConstraintRec, * XmManagerConstraintPtr;



/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmGadgetTraversePrevTabGroup() ;
extern void _XmGadgetTraverseNextTabGroup() ;
extern void _XmGadgetTraverseLeft() ;
extern void _XmGadgetTraverseRight() ;
extern void _XmGadgetTraverseUp() ;
extern void _XmGadgetTraverseDown() ;
extern void _XmGadgetTraverseNext() ;
extern void _XmGadgetTraversePrev() ;
extern void _XmGadgetTraverseHome() ;
extern void _XmGadgetSelect() ;
extern void _XmManagerParentActivate() ;
extern void _XmManagerParentCancel() ;
extern void _XmGadgetButtonMotion() ;
extern void _XmGadgetKeyInput() ;
extern void _XmGadgetArm() ;
extern void _XmGadgetDrag() ;
extern void _XmGadgetActivate() ;
extern void _XmManagerHelp() ;
extern void _XmGadgetMultiArm() ;
extern void _XmGadgetMultiActivate() ;

extern void _XmSocorro() ;
extern Boolean _XmParentProcess() ;
extern void _XmClearShadowType() ;
extern void _XmDestroyParentCallback() ;

#else

extern void _XmGadgetTraversePrevTabGroup( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraverseNextTabGroup( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraverseLeft( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraverseRight( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraverseUp( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraverseDown( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraverseNext( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraversePrev( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetTraverseHome( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerParentActivate( 
                        Widget mw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerParentCancel( 
                        Widget mw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetButtonMotion( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetKeyInput( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetArm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetDrag( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetActivate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerHelp( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetMultiArm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmGadgetMultiActivate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;

extern void _XmSocorro( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern Boolean _XmParentProcess( 
                        Widget widget,
                        XmParentProcessData data) ;
extern void _XmClearShadowType( 
                        Widget w,
#if NeedWidePrototypes
                        int old_width,
                        int old_height,
                        int old_shadow_thickness,
                        int old_highlight_thickness) ;
#else
                        Dimension old_width,
                        Dimension old_height,
                        Dimension old_shadow_thickness,
                        Dimension old_highlight_thickness) ;
#endif /* NeedWidePrototypes */
extern void _XmDestroyParentCallback( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmManagerP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
