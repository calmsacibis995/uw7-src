#pragma ident	"@(#)m1.2libs:Xm/ArrowBGP.h	1.2"
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
#ifndef _XmArrowGadgetP_h
#define _XmArrowGadgetP_h

#include <Xm/ArrowBG.h>
#include <Xm/GadgetP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Arrow class structure  */

typedef struct _XmArrowButtonGadgetClassPart
{
   XtPointer extension;
} XmArrowButtonGadgetClassPart;


/*  Full class record declaration for Arrow class  */

typedef struct _XmArrowButtonGadgetClassRec
{
   RectObjClassPart             rect_class;
   XmGadgetClassPart            gadget_class;
   XmArrowButtonGadgetClassPart arrow_button_class;
} XmArrowButtonGadgetClassRec;

externalref XmArrowButtonGadgetClassRec xmArrowButtonGadgetClassRec;

/* "Gadget caching" is currently under investigation for ArrowBG.  It would
 * be very desirable to cache the XRectangles, requiring immediate reference
 * instead of indirectly through pointers.  ArrowBG will be cached by Beta.
 */
/*  The Arrow instance record  */

typedef struct _XmArrowButtonGadgetPart
{
   XtCallbackList activate_callback;
   XtCallbackList arm_callback;
   XtCallbackList disarm_callback;
   unsigned char direction;	  /*  the direction the arrow is pointing  */

   Boolean selected;

   short        top_count;
   short        cent_count;
   short        bot_count;
   XRectangle * top;
   XRectangle * cent;
   XRectangle * bot;

   Position old_x;
   Position old_y;

   GC               arrow_GC;	    /* graphics context for arrow drawing */
   XtIntervalId     timer;	
   unsigned char    multiClick;     /* KEEP/DISCARD resource */
   int              click_count;
   GC       	    insensitive_GC; /* graphics context for insensitive arrow drawing */

} XmArrowButtonGadgetPart;


/*  Full instance record declaration  */

typedef struct _XmArrowButtonGadgetRec
{
   ObjectPart              object;
   RectObjPart             rectangle;
   XmGadgetPart            gadget;
   XmArrowButtonGadgetPart arrowbutton;
} XmArrowButtonGadgetRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmArrowGadgetP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
