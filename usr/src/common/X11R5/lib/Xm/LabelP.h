#pragma ident	"@(#)m1.2libs:Xm/LabelP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmLabelP_h
#define _XmLabelP_h

#include <Xm/Label.h>
#include <Xm/PrimitiveP.h>


#define XmUNSPECIFIED		(~0)

#ifdef __cplusplus
extern "C" {
#endif

/*  The Label Widget Class and instance records  */

typedef struct _XmLabelClassPart     /* label class record */
{
        XtWidgetProc	setOverrideCallback;
	XmMenuProc	menuProcs;
        String  /* XtTranslations */  translations;
        XtPointer		extension; /* Pointer to extension record */
} XmLabelClassPart;

typedef struct _XmLabelClassRec
{
    CoreClassPart        core_class;
    XmPrimitiveClassPart primitive_class;
    XmLabelClassPart	 label_class;
} XmLabelClassRec;

externalref XmLabelClassRec xmLabelClassRec;


/* Inherited  Functions exported by label */

#define XmInheritSetOverrideCallback ((XtWidgetProc) _XtInherit)
#define XmInheritResize  ((XtWidgetProc) _XtInherit)
#define XmInheritRealize  ((XtRealizeProc) _XtInherit)

/* The padding between label text and accelerator text */

# define LABEL_ACC_PAD 		15

/*  The Label instance record  */

typedef struct _XmLabelPart
{
	_XmString	_label;  /* String sent to this widget */
        _XmString	_acc_text;
	 KeySym		mnemonic;
/**        char            mnemonic;	**/
        XmStringCharSet mnemonicCharset;
        char 		*accelerator;
        unsigned char	label_type;
        unsigned char	alignment;
        unsigned char	string_direction;
        XmFontList	font;
	
	Dimension	margin_height;   /* margin around widget */
        Dimension	margin_width;

	Dimension 	margin_left;    /* additional margins on */
        Dimension	margin_right;   /* each side of widget */
        Dimension	margin_top;     /* text (or pixmap) is placed */
        Dimension	margin_bottom;  /* inside the margins */

        Boolean 	recompute_size;

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

	Boolean		skipCallback; /* set by RowColumn when entryCallback
					is provided. */
	unsigned char   menu_type;

} XmLabelPart;


typedef struct _XmLabelRec
{
   CorePart         core;
   XmPrimitivePart  primitive;
   XmLabelPart	    label;
} XmLabelRec;

/* MACROS */

#define Lab_MarginWidth(w)		(((XmLabelWidget)(w)) ->label.margin_width)
#define Lab_MarginHeight(w)		(((XmLabelWidget)(w)) ->label.margin_height)
#define Lab_MarginTop(w)		(((XmLabelWidget)(w)) ->label.margin_top)
#define Lab_MarginBottom(w)		(((XmLabelWidget)(w)) ->label.margin_bottom)
#define Lab_MarginRight(w)		(((XmLabelWidget)(w)) ->label.margin_right)
#define Lab_MarginLeft(w)		(((XmLabelWidget)(w)) ->label.margin_left)
#define Lab_TextRect_x(w)		(((XmLabelWidget)(w)) ->label.TextRect.x)
#define Lab_TextRect_y(w)		(((XmLabelWidget)(w)) ->label.TextRect.y)
#define Lab_TextRect_width(w)		(((XmLabelWidget)(w)) ->label.TextRect.width)
#define Lab_TextRect_height(w)		(((XmLabelWidget)(w)) ->label.TextRect.height)

#define Lab_IsText(w)			(((XmLabelWidget)(w)) ->label.label_type == XmSTRING)
#define Lab_IsPixmap(w)			(((XmLabelWidget)(w)) ->label.label_type == XmPIXMAP)

#define Lab_Font(w)			(((XmLabelWidget)(w)) ->label.font)

#define Lab_Mnemonic(w)			(((XmLabelWidget)(w)) ->label.mnemonic)
#define Lab_Accelerator(w)		(((XmLabelWidget)(w)) ->label.accelerator)
#define Lab_AcceleratorText(w)		(((XmLabelWidget)(w)) ->label.acc_text)
#define Lab_MenuType(w)			(((XmLabelWidget)(w))->label.menu_type)
#define Lab_Shadow(w)                   (((XmLabelWidget)(w))->primitive.shadow_thickness)
#define Lab_Highlight(w)                (((XmLabelWidget)(w))->primitive.highlight_thickness)
#define Lab_Baseline(w)                 (_XmStringBaseline ( \
                                         ((XmLabelWidget)(w))->label.font,\
                                         ((XmLabelWidget)(w))->label._label))
/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmCalcLabelDimensions() ;

#else

extern void _XmCalcLabelDimensions( 
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmLabelP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
