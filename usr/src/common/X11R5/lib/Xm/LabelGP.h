#pragma ident	"@(#)m1.2libs:Xm/LabelGP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmLabelGP_h
#define _XmLabelGP_h

#include <Xm/LabelG.h>
#include <Xm/GadgetP.h>
#include <Xm/ExtObjectP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************/
/* The Label Gadget Cache Object's class and instance records*/
/*************************************************************/


typedef struct _XmLabelGCacheObjClassPart
{
    int foo;
} XmLabelGCacheObjClassPart;


typedef struct _XmLabelGCacheObjClassRec     /* label cache class record */
{
    ObjectClassPart			object_class;
    XmExtClassPart                 	ext_class;
    XmLabelGCacheObjClassPart	 	label_class_cache;
} XmLabelGCacheObjClassRec;

externalref XmLabelGCacheObjClassRec xmLabelGCacheObjClassRec;


/*  The Label Gadget Cache instance record  */

typedef struct _XmLabelGCacheObjPart
{
        unsigned char	label_type;
        unsigned char	alignment;
        unsigned char	string_direction;
	
	Dimension	margin_height;   /* margin around widget */
	Dimension	margin_width;

	Dimension	margin_left;    /* additional margins on */
	Dimension	margin_right;   /* each side of widget */
	Dimension	margin_top;     /* text (or pixmap) is placed */
	Dimension	margin_bottom;  /* inside the margins */

	Boolean recompute_size;

	Boolean		skipCallback; /* set by RowColumn when entryCallback
					is provided. */
	unsigned char   menu_type;
} XmLabelGCacheObjPart;

typedef struct _XmLabelGCacheObjRec
{
    ObjectPart               object;
    XmExtPart	             ext;
    XmLabelGCacheObjPart     label_cache;
} XmLabelGCacheObjRec;

/*  The Label Widget Class and instance records  */
/*************************************************/

typedef struct _XmLabelGadgetClassPart     /* label class record */
{
        XtWidgetProc		setOverrideCallback;
	XmMenuProc		menuProcs;
        XtPointer			extension; /* Pointer to extension record */
} XmLabelGadgetClassPart;

typedef struct _XmLabelGadgetClassRec
{
    RectObjClassPart       rect_class;
    XmGadgetClassPart      gadget_class;
    XmLabelGadgetClassPart label_class;
} XmLabelGadgetClassRec;

externalref XmLabelGadgetClassRec xmLabelGadgetClassRec;

typedef struct _XmLabelGadgetPart
{
	_XmString	_label;  /* String sent to this widget */
        _XmString       _acc_text;
        KeySym            mnemonic;
	XmStringCharSet mnemonicCharset;
        char 		*accelerator;
        XmFontList	font;

        Pixmap		pixmap; 
        Pixmap		pixmap_insen; 

        /* PRIVATE members -- values computed by LabelWidgetClass methods */

        GC		normal_GC;   /* GC for text */	
	GC		insensitive_GC;
        XRectangle	TextRect;    /* The bounding box of the text, or clip
                                        rectangle of the window; whichever is
                                        smaller */
        XRectangle	acc_TextRect; /* The bounding box of the text, or clip
                                        rectangle of the window; whichever is
                                        smaller */

	XmLabelGCacheObjPart *cache;

} XmLabelGadgetPart;


typedef struct _XmLabelGadgetRec
{
   ObjectPart        object;
   RectObjPart       rectangle;
   XmGadgetPart      gadget;
   XmLabelGadgetPart label;
} XmLabelGadgetRec;

/* Inherited  Functions exported by label */

#define XmInheritSetOverrideCallback ((XtWidgetProc) _XtInherit)
#define XmInheritResize  ((XtWidgetProc) _XtInherit)

/* Padding between label text and accelerator text */

#define LABELG_ACC_PAD		15

/* MACROS */
/********
 * Macros for cached instance fields
 */
#define LabG_LabelType(w)		(((XmLabelGadget)(w)) -> \
					   label.cache-> label_type)
#define LabG_Alignment(w)		(((XmLabelGadget)(w)) -> \
					   label.cache-> alignment)
#define LabG_StringDirection(w)		(((XmLabelGadget)(w)) -> \
					   label.cache-> string_direction)
#define LabG_MarginHeight(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> margin_height)
#define LabG_MarginWidth(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> margin_width)
#define LabG_MarginLeft(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> margin_left)
#define LabG_MarginRight(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> margin_right)
#define LabG_MarginTop(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> margin_top)
#define LabG_MarginBottom(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> margin_bottom)
#define LabG_RecomputeSize(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> recompute_size)
#define LabG_SkipCallback(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> skipCallback)
#define LabG_MenuType(w)		(((XmLabelGadget)(w)) -> \
                                           label.cache-> menu_type)
/********
 * Macros for UNcached instance fields
 */
#define LabG__label(w)			(((XmLabelGadget)(w)) -> \
					   label._label)
#define LabG__acceleratorText(w)	(((XmLabelGadget)(w)) -> \
                                           label._acc_text)
#define LabG_Font(w)			(((XmLabelGadget)(w)) -> \
					   label.font)
#define LabG_Mnemonic(w)		(((XmLabelGadget)(w)) -> \
					   label.mnemonic)
#define LabG_MnemonicCharset(w)         (((XmLabelGadget)(w)) -> \
                                           label.mnemonicCharset)
#define LabG_Accelerator(w)		(((XmLabelGadget)(w)) -> \
                                           label.accelerator)
#define LabG_Pixmap(w)			(((XmLabelGadget)(w)) -> \
                                           label.pixmap)
#define LabG_PixmapInsensitive(w)	(((XmLabelGadget)(w)) -> \
                                           label.pixmap_insen)
#define LabG_NormalGC(w)		(((XmLabelGadget)(w)) -> \
                                           label.normal_GC)
#define LabG_InsensitiveGC(w)		(((XmLabelGadget)(w)) -> \
                                           label.insensitive_GC)
#define LabG_TextRect(w)		(((XmLabelGadget)(w)) -> \
                                           label.TextRect)
#define LabG_AccTextRect(w)		(((XmLabelGadget)(w)) -> \
                                           label.acc_TextRect)


/********
 * Convenience Macros 
 */
#define LabG_TextRect_x(w)		(LabG_TextRect(w).x)
                                           
#define LabG_TextRect_y(w)		(LabG_TextRect(w).y)
                                           
#define LabG_TextRect_width(w)		(LabG_TextRect(w).width)
                                           
#define LabG_TextRect_height(w)		(LabG_TextRect(w).height)
                                           
#define LabG_IsText(w)			(LabG_LabelType(w) == XmSTRING)

#define LabG_IsPixmap(w)		(LabG_LabelType(w) == XmPIXMAP)

#define LabG_Cache(w)			(((XmLabelGadget)(w))-> \
					   label.cache)
#define LabG_Shadow(w)                   (((XmLabelGadget)(w))->gadget.shadow_thickness)
#define LabG_Highlight(w)                (((XmLabelGadget)(w))->gadget.highlight_thickness)
#define LabG_Baseline(w)                 (_XmStringBaseline ( \
                                         ((XmLabelGadget)(w))->label.font,\
                                         ((XmLabelGadget)(w))->label._label))
#define LabG_ClassCachePart(w) \
	(((XmLabelGadgetClass)xmLabelGadgetClass)->gadget_class.cache_part)


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern int _XmLabelCacheCompare() ;
extern void _XmCalcLabelGDimensions() ;
extern void _XmReCacheLabG() ;
extern void _XmAssignLabG_MarginHeight() ;
extern void _XmAssignLabG_MarginWidth() ;
extern void _XmAssignLabG_MarginLeft() ;
extern void _XmAssignLabG_MarginRight() ;
extern void _XmAssignLabG_MarginTop() ;
extern void _XmAssignLabG_MarginBottom() ;
extern void _XmProcessDrag() ;

#else

extern int _XmLabelCacheCompare( 
                        XtPointer A,
                        XtPointer B) ;
extern void _XmCalcLabelGDimensions( 
                        Widget wid) ;
extern void _XmReCacheLabG( 
                        Widget wid) ;
extern void _XmAssignLabG_MarginHeight( 
                        XmLabelGadget lw,
#if NeedWidePrototypes
                        int value) ;
#else
                        Dimension value) ;
#endif /* NeedWidePrototypes */
extern void _XmAssignLabG_MarginWidth( 
                        XmLabelGadget lw,
#if NeedWidePrototypes
                        int value) ;
#else
                        Dimension value) ;
#endif /* NeedWidePrototypes */
extern void _XmAssignLabG_MarginLeft( 
                        XmLabelGadget lw,
#if NeedWidePrototypes
                        int value) ;
#else
                        Dimension value) ;
#endif /* NeedWidePrototypes */
extern void _XmAssignLabG_MarginRight( 
                        XmLabelGadget lw,
#if NeedWidePrototypes
                        int value) ;
#else
                        Dimension value) ;
#endif /* NeedWidePrototypes */
extern void _XmAssignLabG_MarginTop( 
                        XmLabelGadget lw,
#if NeedWidePrototypes
                        int value) ;
#else
                        Dimension value) ;
#endif /* NeedWidePrototypes */
extern void _XmAssignLabG_MarginBottom( 
                        XmLabelGadget lw,
#if NeedWidePrototypes
                        int value) ;
#else
                        Dimension value) ;
#endif /* NeedWidePrototypes */
extern void _XmProcessDrag( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmLabelGP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
