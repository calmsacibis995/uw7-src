#pragma ident	"@(#)m1.2libs:Xm/GadgetP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1989, 1990 DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#ifndef _XmGadgetP_h
#define _XmGadgetP_h

#ifndef _XmNO_BC_INCL
#define _XmNO_BC_INCL
#endif

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif


/*  Masks to define input the gadget is interested in  */

#define XmNO_EVENT              0x000
#define XmENTER_EVENT           0x001
#define XmLEAVE_EVENT           0x002
#define XmFOCUS_IN_EVENT        0x004
#define XmFOCUS_OUT_EVENT       0x008
#define XmMOTION_EVENT          0x010
#define XmARM_EVENT             0x020
#define XmACTIVATE_EVENT        0x040
#define XmHELP_EVENT            0x080
#define XmKEY_EVENT             0x100
#define XmMULTI_ARM_EVENT       0x200
#define XmMULTI_ACTIVATE_EVENT  0x400
#define XmBDRAG_EVENT		0x800
#define XmALL_EVENT             0xFFF


/* Gadget access Macros */
#define G_ShadowThickness(g) \
	(((XmGadget)(g))->gadget.shadow_thickness)
#define G_HighlightThickness(g) \
	(((XmGadget)(g))->gadget.highlight_thickness)

#define GCEPTR(wc)  ((XmGadgetClassExt *)(&(((XmGadgetClass)(wc))->gadget_class.extension)))
#define _XmGetGadgetClassExtPtr(wc, owner) \
  ((*GCEPTR(wc) && (((*GCEPTR(wc))->record_type) == owner))\
   ? GCEPTR(wc) \
   :((XmGadgetClassExt *) _XmGetClassExtensionPtr(((XmGenericClassExt *)GCEPTR(wc)), owner)))

#define XmGadgetClassExtVersion 1L


/* Gadget cache header for each gadget's Cache Part */

typedef struct _XmGadgetCache
{
   struct _XmGadgetCache	*next;
   struct _XmGadgetCache	*prev;
   int				ref_count;
} XmGadgetCache, *XmGadgetCachePtr;


/* A cache entry for each class which desires gadget caching */

typedef struct _XmCacheClassPart 
{
   XmGadgetCache	cache_head;
   XmCacheCopyProc	cache_copy;
   XmGadgetCacheProc	cache_delete;
   XmCacheCompareProc   cache_compare;
} XmCacheClassPart, *XmCacheClassPartPtr;

typedef struct _XmGadgetCacheRef
{
   struct _XmGadgetCache	*next;
   struct _XmGadgetCache	*prev;
   int				ref_count;
   int				data;
} XmGadgetCacheRef, *XmGadgetCacheRefPtr;

/*  Gadget class structure  */

typedef struct _XmGadgetClassExtRec{
    XtPointer           next_extension;
    XrmQuark            record_type;
    long                version;
    Cardinal            record_size;
    XmWidgetBaselineProc widget_baseline;
    XmWidgetDisplayRectProc  widget_display_rect;
}XmGadgetClassExtRec, *XmGadgetClassExt;

typedef struct _XmGadgetClassPart
{
   XtWidgetProc         border_highlight;
   XtWidgetProc         border_unhighlight;
   XtActionProc         arm_and_activate;
   XmWidgetDispatchProc input_dispatch;
   XmVisualChangeProc   visual_change;
   XmSyntheticResource * syn_resources;   
   int                  num_syn_resources;   
   XmCacheClassPartPtr	cache_part;
   XtPointer		extension;
} XmGadgetClassPart;


/*  Full class record declaration for Gadget class  */

typedef struct _XmGadgetClassRec
{
   RectObjClassPart  rect_class;
   XmGadgetClassPart gadget_class;
} XmGadgetClassRec;

#ifndef GADGET
externalref XmGadgetClassRec xmGadgetClassRec;
#endif


/*  The Gadget instance record  */

typedef struct _XmGadgetPart
{
   Dimension shadow_thickness;
   Dimension highlight_thickness;

   XtCallbackList help_callback;
   XtPointer      user_data;

   Boolean traversal_on;
   Boolean highlight_on_enter;
   Boolean have_traversal;

   unsigned char unit_type;
   XmNavigationType navigation_type;

   Boolean highlight_drawn;
   Boolean highlighted;
   Boolean visible;

   Mask event_mask;
} XmGadgetPart;

/*  Full instance record declaration  */

typedef struct _XmGadgetRec
{
   ObjectPart   object;
   RectObjPart  rectangle;
   XmGadgetPart gadget;
} XmGadgetRec;



/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmBuildGadgetResources() ;

#else

extern void _XmBuildGadgetResources( 
                        WidgetClass c) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmGadgetP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
