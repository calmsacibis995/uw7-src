#pragma ident	"@(#)m1.2libs:Xm/ScrollBarP.h	1.2"
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
#ifndef _XmScrollBarP_h
#define _XmScrollBarP_h


#include <Xm/ScrollBar.h>
#include <Xm/PrimitiveP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Minimum slider width or height  */

#define MIN_SLIDER_THICKNESS	1
#define MIN_SLIDER_LENGTH	6


/*  ScrollBar class structure  */

typedef struct _XmScrollBarClassPart
{
   XtPointer extension;   /* Pointer to extension record */
} XmScrollBarClassPart;


/*  Full class record declaration for CheckBox class  */

typedef struct _XmScrollBarClassRec
{
   CoreClassPart        core_class;
   XmPrimitiveClassPart primitive_class;
   XmScrollBarClassPart scrollBar_class;
} XmScrollBarClassRec;

externalref XmScrollBarClassRec xmScrollBarClassRec;


/*  The ScrollBar instance record  */

typedef struct _XmScrollBarPart
{
   int value;
   int minimum;
   int maximum;
   int slider_size;

   unsigned char orientation;
   unsigned char processing_direction;
   Boolean show_arrows;

   int increment;
   int page_increment;

   int initial_delay;
   int repeat_delay;

   XtCallbackList value_changed_callback;
   XtCallbackList increment_callback;
   XtCallbackList decrement_callback;
   XtCallbackList page_increment_callback;
   XtCallbackList page_decrement_callback;
   XtCallbackList to_top_callback;
   XtCallbackList to_bottom_callback;
   XtCallbackList drag_callback;

   /* obsolete */
   GC unhighlight_GC;
   /***********/

   GC foreground_GC;
   Pixel trough_color;

   Drawable pixmap;
   Boolean  sliding_on;
   Boolean  etched_slider;
   int saved_value;

   unsigned char flags;

   unsigned char change_type;
   XtIntervalId timer;

   short initial_x;
   short initial_y;
   short separation_x;
   short separation_y;

   short slider_x;
   short slider_y;
   short slider_width;
   short slider_height;

   short slider_area_x;
   short slider_area_y;
   short slider_area_width;
   short slider_area_height;

   short arrow1_x;
   short arrow1_y;
   unsigned char arrow1_orientation;
   Boolean arrow1_selected;

   short arrow2_x;
   short arrow2_y;
   unsigned char arrow2_orientation;
   Boolean arrow2_selected;

   short arrow_width;
   short arrow_height;

   /*  Obsolete fields */
   short arrow1_top_count;
   short arrow1_cent_count;
   short arrow1_bot_count;

   XRectangle * arrow1_top;
   XRectangle * arrow1_cent;
   XRectangle * arrow1_bot;

   short arrow2_top_count;
   short arrow2_cent_count;
   short arrow2_bot_count;

   XRectangle * arrow2_top;
   XRectangle * arrow2_cent;
   XRectangle * arrow2_bot;
   /***********/


   /* new for 1.2 */
   GC	unavailable_GC;
} XmScrollBarPart;


/*  Full instance record declaration  */

typedef struct _XmScrollBarRec
{
   CorePart	   core;
   XmPrimitivePart primitive;
   XmScrollBarPart scrollBar;
} XmScrollBarRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmSetEtchedSlider() ;

#else

extern void _XmSetEtchedSlider( 
                        XmScrollBarWidget sbw) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmScrollBarP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
