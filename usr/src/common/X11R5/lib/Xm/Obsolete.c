#pragma ident	"@(#)m1.2libs:Xm/Obsolete.c	1.3"
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
#ifdef  REV_INFO
#ifndef lint
static char SCCSID[] = "OSF/Motif: @(#)Obsolete.c	1.3 92/02/20";
#endif /* lint */
#endif /* REV_INFO */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/XmP.h>
#include "TraversalI.h"
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/WorldP.h>
#include <Xm/DesktopP.h>
#include <Xm/ScreenP.h>
#include <Xm/DisplayP.h>
#include <Xm/FileSBP.h>
#include <Xm/List.h>
#include <Xm/GadgetP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <Xm/VendorSEP.h>
#include <Xm/BaseClassP.h>
#include <Xm/RowColumnP.h>
#include <Xm/MenuShellP.h>
#include <Xm/ScaleP.h>
#include <Xm/ScrolledWP.h>
#include <Xm/TransltnsP.h>
#include <ctype.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif


typedef struct {
    int		segment_size;
    char*	start;
    char*	current;
    int		bytes_remaining;
} XmHeapRec, *XmHeap;


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


externaldef(desktopobjectclass) WidgetClass 
      xmDesktopObjectClass = (WidgetClass) &xmDesktopClassRec;

externaldef(displayobjectclass) WidgetClass 
      xmDisplayObjectClass = (WidgetClass) (&xmDisplayClassRec);

externaldef(screenobjectclass) WidgetClass 
      xmScreenObjectClass = (WidgetClass) (&xmScreenClassRec);

externaldef(worldobjectclass) WidgetClass 
      xmWorldObjectClass = (WidgetClass) (&xmWorldClassRec);


XContext _XmDestinationContext = NULL;


int 
#ifdef _NO_PROTO
XmTextFieldGetBaseLine( w )         /* OBSOLETE */
        Widget w ;
#else
XmTextFieldGetBaseLine(
        Widget w )
#endif /* _NO_PROTO */
{
  return XmTextFieldGetBaseline( w) ;
}

int 
#ifdef _NO_PROTO
XmTextGetBaseLine( w )         /* OBSOLETE */
        Widget w ;
#else
XmTextGetBaseLine(
        Widget w )
#endif /* _NO_PROTO */
{
  return XmTextGetBaseline( w) ;
}

Boolean 
#ifdef _NO_PROTO
_XmTestTraversability( widget, visRect )         /* OBSOLETE */
        Widget widget ;
        XRectangle *visRect ;
#else
_XmTestTraversability(
        Widget widget,
        XRectangle *visRect )
#endif /* _NO_PROTO */
{   
  return XmIsTraversable( widget) ;
}

void 
#ifdef _NO_PROTO
_XmClearTabGroup( w )               /* OBSOLETE */
        Widget w ;
#else
_XmClearTabGroup(
        Widget w )
#endif /* _NO_PROTO */
{
  return ;
}

Widget 
#ifdef _NO_PROTO
_XmFindTabGroup( widget )             /* OBSOLETE */
        Widget widget ;
#else
_XmFindTabGroup(
        Widget widget )
#endif /* _NO_PROTO */
{   
  return XmGetTabGroup( widget) ;
}

void 
#ifdef _NO_PROTO
_XmClearKbdFocus( tabGroup )          /* OBSOLETE */
        Widget tabGroup ;
#else
_XmClearKbdFocus(
        Widget tabGroup )
#endif /* _NO_PROTO */
{
  return ;
}

Widget 
#ifdef _NO_PROTO
_XmGetTabGroup( w )     /* OBSOLETE */
        Widget w ;
#else
_XmGetTabGroup(
        Widget w )
#endif /* _NO_PROTO */
{   
  return XmGetTabGroup( w) ;
}

Boolean 
#ifdef _NO_PROTO
_XmWidgetIsTraversable( widget, navType, visRect )   /* OBSOLETE */
        Widget widget ;
        XmNavigationType navType ;
        XRectangle *visRect ;
#else
_XmWidgetIsTraversable(
        Widget widget,
#if NeedWidePrototypes
        int navType,
#else
        XmNavigationType navType,
#endif /* NeedWidePrototypes */
        XRectangle *visRect )
#endif /* _NO_PROTO */
{   
  return XmIsTraversable( widget) ;
}

Boolean 
#ifdef _NO_PROTO
_XmGetManagedInfo( w )   /* OBSOLETE */
        Widget w ;
#else
_XmGetManagedInfo(
        Widget w )
#endif /* _NO_PROTO */
{
  /* Depending upon the widget coming in, extract its mapped_when_managed flag
   * and its managed flag.
   */
    if (XmIsPrimitive (w))
      return (w->core.managed && w->core.mapped_when_managed);
    else if (XmIsGadget (w))
      return (w->core.managed);
    else
      {
	  /* Treat menupanes specially */
	  if (XmIsRowColumn(w) && XmIsMenuShell(XtParent(w)))
	    {
		return (True);
	    }
	  else
	    return (w->core.managed && w->core.mapped_when_managed);
      }
}

Boolean 
#ifdef _NO_PROTO
_XmChangeNavigationType( current, newNavType )    /* OBSOLETE */
        Widget current ;
        XmNavigationType newNavType ;
#else
_XmChangeNavigationType(
        Widget current,
#if NeedWidePrototypes
        int newNavType )
#else
        XmNavigationType newNavType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{   
  /* This is a convenience routine for widgets wanting to change
   * their navigation type without using XtSetValues().
   */
  XmFocusData focusData ;
  Widget new_wid = current->core.self ;
  XmNavigationType curNavType = _XmGetNavigationType( current) ;
  XmTravGraph tgraph ;

  if(    (curNavType != newNavType)
     &&  (focusData = _XmGetFocusData( new_wid))
     &&  (tgraph = &(focusData->trav_graph))->num_entries    )
    {
      _XmTravGraphUpdate( tgraph, new_wid) ;

      if(    (focusData->focus_policy == XmEXPLICIT)
	 &&  (focusData->focus_item == new_wid)
	 &&  !XmIsTraversable( new_wid)    )
	{
	  Widget new_focus = _XmTraverseAway( tgraph, new_wid,
				    (focusData->active_tab_group != new_wid)) ;
	  if(    !new_focus    )
	    {
	      new_focus = new_wid ;
	    }
	  _XmMgrTraversal( new_focus, XmTRAVERSE_CURRENT) ;
	}
    }
  return TRUE ;
}


/********************************************************************/
/* Following is the old code needed for subclasses already using it */
/* They can either define the macro and change nothing (but suffer
   the addition of code and the drawing performance) or adapt to the 
   new interface (better) */

/************************************************************************
 *
 *  Primitive:_XmDrawShadow, become XmDrawShadow
 *
 *      Draw an n segment wide bordering shadow on the drawable
 *      d, using the provided GC's and rectangle.
 *
 ************************************************************************/

#ifdef _NO_PROTO                 /* OBSOLETE */
void _XmDrawShadow (display, d, top_GC, bottom_GC, size, x, y, width, height)
Display * display;
Drawable d;
GC top_GC;
GC bottom_GC;
register int size;
register int x;
register int y;
register int width;
register int height;

#else /* _NO_PROTO */
void _XmDrawShadow (Display *display, Drawable d, 
                    GC top_GC, GC bottom_GC, int size, int x, int y, 
                    int width, int height)
#endif /* _NO_PROTO */
{
   static XRectangle * rects = NULL;
   static int rect_count = 0;
   register int i;
   register int size2;
   register int size3;

   if (size <= 0) return;
   if (size > width / 2) size = width / 2;
   if (size > height / 2) size = height / 2;
   if (size <= 0) return;

   if (rect_count == 0)
   {
      rects = (XRectangle *) XtMalloc (sizeof (XRectangle) * size * 4);
      rect_count = size;
   }

   if (rect_count < size)
   {
      rects = (XRectangle *) XtRealloc((char *)rects, sizeof (XRectangle) * size * 4);
      rect_count = size;
   }

   size2 = size + size;
   size3 = size2 + size;

   for (i = 0; i < size; i++)
   {
      /*  Top segments  */

      rects[i].x = x;
      rects[i].y = y + i;
      rects[i].width = width - i;
      rects[i].height = 1;


      /*  Left segments  */

      rects[i + size].x = x + i;
      rects[i + size].y = y;
      rects[i + size].width = 1;
      rects[i + size].height = height - i;


      /*  Bottom segments  */

      rects[i + size2].x = x + i + 1;
      rects[i + size2].y = y + height - i - 1;
      rects[i + size2].width = width - i - 1;
      rects[i + size2].height = 1;


      /*  Right segments  */

      rects[i + size3].x = x + width - i - 1;
      rects[i + size3].y = y + i + 1;
      rects[i + size3].width = 1;
      rects[i + size3].height = height - i - 1;
   }

   XFillRectangles (display, d, top_GC, &rects[0], size2);
   XFillRectangles (display, d, bottom_GC, &rects[size2], size2);
}

/************************************************************************
 *
 *  Primitive:_XmEraseShadow become XmClearShadow
 *
 *      Erase an n segment wide bordering shadow on the drawable
 *      d, using the provided  rectangle.
 *
 ************************************************************************/

#ifdef _NO_PROTO
void _XmEraseShadow (display, d, size, x, y, width, height)    /* OBSOLETE */
Display * display;
Drawable d;
register int size;
register int x;
register int y;
register int width;
register int height;
#else /* _NO_PROTO */
void _XmEraseShadow (Display *display, Drawable d, int size, 
                     int x, int y, int width, int height)
#endif /* _NO_PROTO */
{
   if (width > 0 && size > 0)
   {
      XClearArea (display, d, x, y, width, size, FALSE);
      XClearArea (display, d, x, y + height - size, width, size, FALSE);
   }

   if (size > 0 && height - (2 * size) > 0)
   {
      XClearArea (display, d, x, y + size, size, height - (2 * size), FALSE);
      XClearArea (display, d, x + width - size, y + size, size, 
                  height - (2 * size), FALSE);
   }
}



/************************************************************************
 *    ArrowBI:_XmGetArrowDrawRects, become XmDrawArrow
 *
 *      Calculate the drawing rectangles.
 *
 ************************************************************************/

#ifdef _NO_PROTO                   /* OBSOLETE */
void _XmGetArrowDrawRects (highlight_thickness, shadow_thickness,
                direction, core_width, core_height, top_count, cent_count, 
                bot_count, top, cent, bot)

int  highlight_thickness;
int  shadow_thickness;
unsigned char  direction;
int  core_width;
int  core_height;
short  *top_count;
short  *cent_count;
short  *bot_count;
XRectangle **top;
XRectangle **cent;
XRectangle **bot;

#else /* _NO_PROTO */
void _XmGetArrowDrawRects (int highlight_thickness, int shadow_thickness, unsigned int direction, int core_width, int core_height, short *top_count, short *cent_count, short *bot_count, XRectangle **top, XRectangle **cent, XRectangle **bot)
#endif /* _NO_PROTO */
{
   /*  Arrow rectangle generation function  */

   int size, width, start;
   register int y;
   XRectangle *tmp;
   register int temp;
   short t = 0;
   short b = 0;
   short c = 0;
   int xOffset = 0;
   int yOffset = 0;


   /*  Free the old lists  */

   if (*top != NULL)
   {
      XtFree ((char *) *top);   *top  = NULL;
      XtFree ((char *) *cent);  *cent = NULL;
      XtFree ((char *) *bot);   *bot  = NULL;
      *top_count = 0;
      *cent_count = 0;
      *bot_count = 0;
   }


   /*  Get the size and allocate the rectangle lists  */

   if (core_width > core_height) 
   {
      size = core_height - 2 - 
             2 * (highlight_thickness + shadow_thickness);
      xOffset = (core_width - core_height) / 2;
   }
   else
   {
      size = core_width - 2 - 
             2 * (highlight_thickness + shadow_thickness);
      yOffset = (core_height - core_width) / 2;
   }

   if (size < 1) return;


   if (direction == XmARROW_RIGHT ||
       direction == XmARROW_LEFT)
   {
      temp = xOffset;
      xOffset = yOffset;
      yOffset = temp;
   }

   *top  = (XRectangle *) XtMalloc (sizeof (XRectangle) * (size / 2 + 6));
   *cent = (XRectangle *) XtMalloc (sizeof (XRectangle) * (size  / 2 + 6));
   *bot  = (XRectangle *) XtMalloc (sizeof (XRectangle) * (size / 2 + 6));

   /*  Set up a loop to generate the segments.  */

   width = size;
   y = size + highlight_thickness + shadow_thickness - 1 + yOffset;

   start = highlight_thickness + shadow_thickness + 1 + xOffset;

   while (width > 0)
   {

      if (width == 1)
      {
         (*top)[t].x = start; (*top)[t].y = y + 1;
         (*top)[t].width = 1; (*top)[t].height = 1;
         t++;
      }
      else if (width == 2)
      {
         if (size == 2 || 
             (direction == XmARROW_UP ||
              direction == XmARROW_LEFT))
         {
            (*top)[t].x = start; (*top)[t].y = y;
            (*top)[t].width = 2; (*top)[t].height = 1;
            t++;
            (*top)[t].x = start; (*top)[t].y = y + 1;
            (*top)[t].width = 1; (*top)[t].height = 1;
            t++;
            (*bot)[b].x = start + 1; (*bot)[b].y = y + 1;
            (*bot)[b].width = 1; (*bot)[b].height = 1;
            b++;
         }
         else if (direction == XmARROW_UP ||
                  direction == XmARROW_LEFT)
         {
            (*top)[t].x = start; (*top)[t].y = y;
            (*top)[t].width = 2; (*top)[t].height = 1;
            t++;
            (*bot)[b].x = start; (*bot)[b].y = y + 1;
            (*bot)[b].width = 2; (*bot)[b].height = 1;
            b++;
         }
      }
      else
      {
         if (start == highlight_thickness +
                      shadow_thickness + 1 + xOffset)
         {
            if (direction == XmARROW_UP ||
                direction == XmARROW_LEFT)
            {
               (*top)[t].x = start; (*top)[t].y = y;
               (*top)[t].width = 2; (*top)[t].height = 1;
               t++;
               (*top)[t].x = start; (*top)[t].y = y + 1;
               (*top)[t].width = 1; (*top)[t].height = 1;
               t++;
               (*bot)[b].x = start + 1; (*bot)[b].y = y + 1;
               (*bot)[b].width = 1; (*bot)[b].height = 1;
               b++;
               (*bot)[b].x = start + 2; (*bot)[b].y = y;
               (*bot)[b].width = width - 2; (*bot)[b].height = 2;
               b++;
            }
            else
            {
               (*top)[t].x = start; (*top)[t].y = y;
               (*top)[t].width = 2; (*top)[t].height = 1;
               t++;
               (*bot)[b].x = start; (*bot)[b].y = y + 1;
               (*bot)[b].width = 2; (*bot)[b].height = 1;
               b++;
               (*bot)[b].x = start + 2; (*bot)[b].y = y;
               (*bot)[b].width = width - 2; (*bot)[b].height = 2;
               b++;
            }
         }
         else
         {
            (*top)[t].x = start; (*top)[t].y = y;
            (*top)[t].width = 2; (*top)[t].height = 2;
            t++;
            (*bot)[b].x = start + width - 2; (*bot)[b].y = y;
            (*bot)[b].width = 2; (*bot)[b].height = 2;
            if (width == 3)
            {
               (*bot)[b].width = 1;
               (*bot)[b].x += 1;
            }
            b++;
            if (width > 4)
            {
               (*cent)[c].x = start + 2; (*cent)[c].y = y;
               (*cent)[c].width = width - 4; (*cent)[c].height = 2;
               c++;
            }
         }
      }
      start++;
      width -= 2;
      y -= 2;
   }

   if (direction == XmARROW_UP ||
       direction == XmARROW_LEFT)
   {
      *top_count = t;
      *cent_count = c;
      *bot_count = b;
   }
   else
   {   
      tmp = *top;
      *top = *bot;
      *bot = tmp;
      *top_count = b;
      *cent_count = c;
      *bot_count = t;
   }


   /*  Transform the "up" pointing arrow to the correct direction  */

   switch (direction)
   {
      case XmARROW_LEFT:
      {
          register int i; 

          i = -1;
          do
          {
             i++;
             if (i < *top_count)
             {
                temp = (*top)[i].y; (*top)[i].y =
                    (*top)[i].x; (*top)[i].x = temp;
                temp = (*top)[i].width; 
                (*top)[i].width = (*top)[i].height; (*top)[i].height = temp;
             }             
             if (i < *bot_count)
             {
                temp = (*bot)[i].y; (*bot)[i].y =
                    (*bot)[i].x; (*bot)[i].x = temp;
                temp = (*bot)[i].width; 
                (*bot)[i].width = (*bot)[i].height; (*bot)[i].height = temp;
             }             
             if (i < *cent_count)
             {
                temp = (*cent)[i].y; (*cent)[i].y =
                    (*cent)[i].x; (*cent)[i].x = temp;
                temp = (*cent)[i].width; 
                (*cent)[i].width = (*cent)[i].height; (*cent)[i].height = temp;
             }             
          }
          while (i < *top_count || i < *bot_count || i < *cent_count);
      }
      break;

      case XmARROW_RIGHT:
      {
          register int h_right = core_height - 2;
          register int w_right = core_width - 2;
          register int i; 

          i = -1;
          do
          {
             i++;
             if (i < *top_count)
             {
                temp = (*top)[i].y; (*top)[i].y = (*top)[i].x; 
                (*top)[i].x = temp; 
                temp = (*top)[i].width; (*top)[i].width = (*top)[i].height; 
                (*top)[i].height = temp;
                (*top)[i].x = w_right - (*top)[i].x - (*top)[i].width + 2;
                (*top)[i].y = h_right - (*top)[i].y - (*top)[i].height + 2;
             }             
             if (i < *bot_count)
             {
                temp = (*bot)[i].y; (*bot)[i].y = (*bot)[i].x; 
                (*bot)[i].x = temp; 
                temp = (*bot)[i].width; (*bot)[i].width = (*bot)[i].height; 
                (*bot)[i].height = temp;
                (*bot)[i].x = w_right - (*bot)[i].x - (*bot)[i].width + 2;
                (*bot)[i].y = h_right - (*bot)[i].y - (*bot)[i].height + 2;
             }             
             if (i < *cent_count)
             {
                temp = (*cent)[i].y; (*cent)[i].y = (*cent)[i].x; 
                (*cent)[i].x = temp; 
                temp = (*cent)[i].width; (*cent)[i].width = (*cent)[i].height;
                (*cent)[i].height = temp;
                (*cent)[i].x = w_right - (*cent)[i].x - (*cent)[i].width + 2;
                (*cent)[i].y = h_right - (*cent)[i].y - (*cent)[i].height + 2;
             }
          }
          while (i < *top_count || i < *bot_count || i < *cent_count);
      }
      break;

      case XmARROW_UP:
      {
      }
      break;

      case XmARROW_DOWN:
      {
          register int w_down = core_width - 2;
          register int h_down = core_height - 2;
          register int i; 

          i = -1;
          do
          {
             i++;
             if (i < *top_count)
             {
                (*top)[i].x = w_down - (*top)[i].x - (*top)[i].width + 2;
                (*top)[i].y = h_down - (*top)[i].y - (*top)[i].height + 2;
             }
             if (i < *bot_count)
             {
                (*bot)[i].x = w_down - (*bot)[i].x - (*bot)[i].width + 2;
                (*bot)[i].y = h_down - (*bot)[i].y - (*bot)[i].height + 2;
             }
             if (i < *cent_count)
             {
                (*cent)[i].x = w_down - (*cent)[i].x - (*cent)[i].width + 2;
                (*cent)[i].y = h_down - (*cent)[i].y - (*cent)[i].height + 2;
             }
          }

          while (i < *top_count || i < *bot_count || i < *cent_count);
      }
      break;
   }
}

/************************************************************************
 *
 *    ArrowBI:_XmOffsetArrow, become XmDrawArrow
 *  
 *      Offset the arrow drawing rectangles, if needed, by the difference
 *      of the current x, y and the saved x, y);
 *
 ************************************************************************/
 
#ifdef _NO_PROTO
void _XmOffsetArrow (diff_x, diff_y, top, cent, bot,          /* OBSOLETE */
                     top_count, cent_count, bot_count)
register int diff_x;
register int diff_y;
XRectangle * top;
XRectangle * bot;
XRectangle * cent;
int top_count;
int cent_count;
int bot_count;

#else /* _NO_PROTO */
void _XmOffsetArrow (int diff_x, int diff_y, XRectangle *top, XRectangle *cent, XRectangle *bot, int top_count, int cent_count, int bot_count)
#endif /* _NO_PROTO */
{
   register int i;

   if (diff_x != 0 || diff_y != 0)
   {
      for (i = 0; i < top_count; i++)
      {
         (top + i)->x += diff_x;
         (top + i)->y += diff_y;
      }

      for (i = 0; i < cent_count; i++)
      {
         (cent + i)->x += diff_x;
         (cent + i)->y += diff_y;
      }

      for (i = 0; i < bot_count; i++)
      {
         (bot + i)->x += diff_x;
         (bot + i)->y += diff_y;
      }
   }
}

/*************************************<->*************************************
 *
 *  ToggleBI:_DrawSquareButton, become code
 *
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO                         /* OBSOLETE */
void _XmDrawSquareButton (w, x, y, size, topGC, bottomGC, centerGC, fill)
Widget w;
int x, y, size;
GC topGC, bottomGC, centerGC;
Boolean fill;
#else /* _NO_PROTO */
void _XmDrawSquareButton (Widget w, int x, int y, int size, GC topGC, GC bottomGC, GC centerGC, 
#if NeedWidePrototypes
int fill)
#else
Boolean fill)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   _XmDrawShadow (XtDisplay (w), XtWindow (w), 
                  topGC, bottomGC,
                  2, x, y, size, size);

   if (fill)
       if (size > 6)
           XFillRectangle (XtDisplay ((Widget) w), 
                           XtWindow ((Widget) w),
                           centerGC, 
                           ((fill) ? x+2 : x+3),
                           ((fill) ? y+2 : y+3),
                           ((fill) ? size-4 : size-6),
                           ((fill) ? size-4 : size-6));
} 

/************************************************************************
 *
 *  ToggleBI:DrawDiamondButton, become XmDrawDiamond
 *
 *      The diamond drawing routine.  Used in place of  widgets or gadgets
 *      draw routine when toggleButton's indicatorType is one_of_many.
 *
 ************************************************************************/


#ifdef _NO_PROTO                                 /* OBSOLETE */
void _XmDrawDiamondButton (tw, x, y, size, topGC, bottomGC, centerGC, fill)
Widget tw;
int x, y, size;
GC topGC, bottomGC, centerGC;
Boolean fill;
#else /* _NO_PROTO */
void _XmDrawDiamondButton (Widget tw, int x, int y, int size, GC topGC, GC bottomGC, GC centerGC, 
#if NeedWidePrototypes
int fill )
#else
Boolean fill)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XSegment seg[12];
   XPoint   pt[5];
   int midX, midY;

   if (size % 2 == 0)
      size--;

   midX = x + (size + 1) / 2;
   midY = y + (size + 1) / 2;

   /* COUNTER REVERSE DRAWING EFFECT ON TINY ToggleButtonS */
   if (size <= 3)
    {
       /*  The top shadow segments  */

       seg[0].x1 = x + size - 1;        /*  1  */
       seg[0].y1 = midY - 1;
       seg[0].x2 = midX - 1;            /*  2  */
       seg[0].y2 = y + size - 1;

       seg[1].x1 = x + size - 2;        /*  3  */
       seg[1].y1 = midY - 1;
       seg[1].x2 = midX - 1;            /*  4  */
       seg[1].y2 = y + size - 2;

       seg[2].x1 = x + size - 3;        /*  3  */
       seg[2].y1 = midY - 1;
       seg[2].x2 = midX - 1;            /*  4  */
       seg[2].y2 = y + size - 3;

       /*--*/

       seg[3].x1 = midX - 1;            /*  5  */
       seg[3].y1 = y + size - 1;
       seg[3].x2 = x;                   /*  6  */
       seg[3].y2 = midY - 1;

       seg[4].x1 = midX - 1;            /*  7  */
       seg[4].y1 = y + size - 2;
       seg[4].x2 = x + 1;               /*  8  */
       seg[4].y2 = midY - 1;

       seg[5].x1 = midX - 1;            /*  7  */
       seg[5].y1 = y + size - 3;
       seg[5].x2 = x + 2;               /*  8  */
       seg[5].y2 = midY - 1;

       /*  The bottom shadow segments  */

       seg[6].x1 = x + size - 1;        /*  9  */
       seg[6].y1 = midY - 1;
       seg[6].x2 = midX - 1;            /*  10  */
       seg[6].y2 = y;

       seg[7].x1 = x + size - 2;        /*  11  */
       seg[7].y1 = midY - 1;
       seg[7].x2 = midX - 1;            /*  12  */
       seg[7].y2 = y + 1;

       seg[8].x1 = x + size - 3;        /*  11  */
       seg[8].y1 = midY - 1;
       seg[8].x2 = midX - 1;            /*  12  */
       seg[8].y2 = y + 2;

       /*--*/

       seg[9].x1 = midX - 1;            /*  13  */
       seg[9].y1 = y;
       seg[9].x2 = x;                   /*  14  */
       seg[9].y2 = midY - 1;

       seg[10].x1 = midX - 1;           /*  15  */
       seg[10].y1 = y + 1;
       seg[10].x2 = x + 1;              /*  16  */
       seg[10].y2 = midY - 1;

       seg[11].x1 = midX - 1;           /*  15  */
       seg[11].y1 = y + 2;
       seg[11].x2 = x + 2;              /*  16  */
       seg[11].y2 = midY - 1;

    }
  else    /* NORMAL SIZED ToggleButtonS */
    {
       /*  The top shadow segments  */

       seg[0].x1 = x;                   /*  1  */
       seg[0].y1 = midY - 1;
       seg[0].x2 = midX - 1;            /*  2  */
       seg[0].y2 = y;

       seg[1].x1 = x + 1;               /*  3  */
       seg[1].y1 = midY - 1;
       seg[1].x2 = midX - 1;            /*  4  */
       seg[1].y2 = y + 1;

       seg[2].x1 = x + 2;               /*  3  */
       seg[2].y1 = midY - 1;
       seg[2].x2 = midX - 1;            /*  4  */
       seg[2].y2 = y + 2;

       /*--*/

       seg[3].x1 = midX - 1;            /*  5  */
       seg[3].y1 = y;
       seg[3].x2 = x + size - 1;        /*  6  */
       seg[3].y2 = midY - 1;

       seg[4].x1 = midX - 1;            /*  7  */
       seg[4].y1 = y + 1;
       seg[4].x2 = x + size - 2;        /*  8  */
       seg[4].y2 = midY - 1;

       seg[5].x1 = midX - 1;            /*  7  */
       seg[5].y1 = y + 2;
       seg[5].x2 = x + size - 3;        /*  8  */
       seg[5].y2 = midY - 1;


       /*  The bottom shadow segments  */
    
       seg[6].x1 = x;                   /*  9  */
       seg[6].y1 = midY - 1;
       seg[6].x2 = midX - 1;            /*  10  */
       seg[6].y2 = y + size - 1;

       seg[7].x1 = x + 1;               /*  11  */
       seg[7].y1 = midY - 1;
       seg[7].x2 = midX - 1;            /*  12  */
       seg[7].y2 = y + size - 2;

       seg[8].x1 = x + 2;               /*  11  */
       seg[8].y1 = midY - 1;
       seg[8].x2 = midX - 1;            /*  12  */
       seg[8].y2 = y + size - 3;

       /*--*/

       seg[9].x1 = midX - 1;            /*  13  */
       seg[9].y1 = y + size - 1;
       seg[9].x2 = x + size - 1;        /*  14  */
       seg[9].y2 = midY - 1;

       seg[10].x1 = midX - 1;           /*  15  */
       seg[10].y1 = y + size - 2;
       seg[10].x2 = x + size - 2;       /*  16  */
       seg[10].y2 = midY - 1;

       seg[11].x1 = midX - 1;           /*  15  */
       seg[11].y1 = y + size - 3;
       seg[11].x2 = x + size - 3;       /*  16  */
       seg[11].y2 = midY - 1;
    }

   XDrawSegments (XtDisplay ((Widget) tw), XtWindow ((Widget) tw),
                  topGC, &seg[3], 3);

   XDrawSegments (XtDisplay ((Widget) tw), XtWindow ((Widget) tw),
                  bottomGC, &seg[6], 6);

   XDrawSegments (XtDisplay ((Widget) tw), XtWindow ((Widget) tw),
                  topGC, &seg[0], 3);

  
/* For Fill */
   if (fill)
   {
      pt[0].x = x + 3;
      pt[0].y = midY - 1;
      pt[1].x = midX - 1 ;
      pt[1].y = y + 2;
      pt[2].x = x + size - 3;
      pt[2].y = midY - 1;
      pt[3].x = midX - 1 ;
      pt[3].y = y + size - 3;
   }
   else
   {
      pt[0].x = x + 4;
      pt[0].y = midY - 1;
      pt[1].x = midX - 1;
      pt[1].y = y + 3;
      pt[2].x = x + size - 4;
      pt[2].y = midY - 1;
      pt[3].x = midX - 1;
      pt[3].y = y + size - 4;
   }


   /* NOTE: code which handled the next two ifs by setting pt[1-3]
      to match pt[0] values was replaced with return statements because
      passing 4 identical coordinates to XFillPolygon caused the PMAX
      to give a bus error.  Dana@HP reports that the call is legitimate
      and that the error is in the PMAX server.  The return statements
      will stay until the situation with the PMAX is resolved. (mitch) */

   /* COUNTER REVERSE DRAWING EFFECT ON TINY ToggleButtonS */
   if (pt[0].x > pt[1].x)
     {
#ifdef CORRECT
       pt[1].x = pt[0].x;
       pt[2].x = pt[0].x;
       pt[3].x = pt[0].x;
#else
       return;
#endif
     }

   if (pt[0].y < pt[1].y)
     {
#ifdef CORRECT
       pt[1].x = pt[0].x;
       pt[2].x = pt[0].x;
       pt[3].x = pt[0].x;
#else
       return;
#endif
     }

   XFillPolygon (XtDisplay ((Widget) tw), XtWindow ((Widget) tw),
                 centerGC, pt, 4, Convex, CoordModeOrigin);
}

/************************************************************************
 *
 *  Manager:XmDrawEtchedShadow, become XmDrawShadow
 *
 *      Draw an n segment wide etched shadow on the drawable
 *      d, using the provided GC's and rectangle.
 *
 ************************************************************************/

static XRectangle *rects = NULL;
static int rect_count = 0;

static void
#ifdef _NO_PROTO
get_rects(max_i, offset, x, y, width, height,       /* OBSOLETE */
               pos_top, pos_left, pos_bottom, pos_right)

int max_i;
register int offset;
register int x;
register int y;
register int width;
register int height;
register int pos_top, pos_left, pos_bottom, pos_right;
#else
get_rects(
int max_i,
register int offset,
register int x,
register int y,
register int width,
register int height,
register int pos_top,
register int pos_left,
register int pos_bottom,
register int pos_right)
#endif /* _NO_PROTO */
{
   register int i;
   register int offsetX2;
   
   for (i = 0; i < max_i; i++, offset++)
   {
      offsetX2 = offset + offset;

      /*  Top segments  */

      rects[pos_top + i].x = x + offset;
      rects[pos_top + i].y = y + offset;
      rects[pos_top + i].width = width - offsetX2 -1;
      rects[pos_top + i].height = 1;


      /*  Left segments  */

      rects[pos_left + i].x = x + offset;
      rects[pos_left + i].y = y + offset;
      rects[pos_left + i].width = 1;
      rects[pos_left + i].height = height - offsetX2 - 1;


      /*  Bottom segments  */

      rects[pos_bottom + i].x = x + offset;
      rects[pos_bottom + i].y = y + height - offset - 1;
      rects[pos_bottom + i].width = width - offsetX2;
      rects[pos_bottom + i].height = 1;


      /*  Right segments  */

      rects[pos_right + i].x = x + width - offset - 1;
      rects[pos_right + i].y = y + offset;
      rects[pos_right + i].width = 1;
      rects[pos_right + i].height = height - offsetX2;
   }
}

static void
#ifdef _NO_PROTO
XmDrawEtchedShadow (display, d, top_GC, bottom_GC,   /* OBSOLETE */
                         size, x, y, width, height)
Display * display;
Drawable d;
GC top_GC;
GC bottom_GC;
register int size;
register int x;
register int y;
register int width;
register int height;
#else
XmDrawEtchedShadow (
Display * display,
Drawable d,
GC top_GC,
GC bottom_GC,
register int size,
register int x,
register int y,
register int width,
register int height)
#endif /* _NO_PROTO */
{
   int half_size;
   int size2;
   int size3;
   int pos_top, pos_left, pos_bottom, pos_right;

   if (size <= 0) return;
   if (size == 1) 
        {
      _XmDrawShadow (display, d,
         top_GC,  bottom_GC, size, x, y, width, height);
          return;
        } 

   if (size > width / 2) size = width / 2;
   if (size > height / 2) size = height / 2;
   if (size <= 0) return;

   size = (size % 2) ? (size-1) : (size);

   half_size = size / 2;
   size2 = size + size;
   size3 = size2 + size;
   
   if (rect_count == 0)
   {
      rects = (XRectangle *) XtMalloc (sizeof (XRectangle) * size * 4);
      rect_count = size;
   }

   if (rect_count < size)
   {
      rects = (XRectangle *) XtRealloc((char *)rects, sizeof (XRectangle) * size * 4);
      rect_count = size;
   }

   pos_top = 0;
   pos_left = half_size;
   pos_bottom = size2;
   pos_right = size2 + half_size;

   get_rects(half_size, 0, x, y, width, height, 
             pos_top, pos_left, pos_bottom, pos_right);

   pos_top = size3;
   pos_left = size3 + half_size;
   pos_bottom = size;
   pos_right = size + half_size;

   get_rects(half_size, half_size, x, y, width, height, 
                pos_top, pos_left, pos_bottom, pos_right);

   XFillRectangles (display, d, bottom_GC, &rects[size2], size2);
   XFillRectangles (display, d, top_GC, &rects[0], size2);
}


/*****************************************************************
 * Manager:_XmDrawShadowType, become XmDrawShadow
 *****************************************************************/
 
#ifdef _NO_PROTO
void _XmDrawShadowType(w, shadow_type, core_width, core_height,  /* OBSOLETE */
        shadow_thickness, highlight_thickness, top_shadow_GC,
        bottom_shadow_GC)
Widget w;
unsigned char shadow_type;
Dimension core_width;
Dimension core_height;
Dimension shadow_thickness;
Dimension highlight_thickness;
GC   top_shadow_GC;
GC   bottom_shadow_GC;

#else /* _NO_PROTO */
void _XmDrawShadowType (Widget w, unsigned int shadow_type,
#if NeedWidePrototypes 
			int core_width, int core_height, 
                        int shadow_thickness, int highlight_thickness, 
#else
                        Dimension core_width, Dimension core_height, 
                        Dimension shadow_thickness, Dimension highlight_thickness, 
#endif
                        GC top_shadow_GC, GC bottom_shadow_GC)
#endif /* _NO_PROTO */
{
   if (!XtIsRealized(w)) 
     return;
   switch (shadow_type)
   {
      case XmSHADOW_IN:
      case XmSHADOW_OUT:
         if (shadow_thickness > 0)
            _XmDrawShadow (XtDisplay (w), XtWindow (w), 
             (shadow_type == XmSHADOW_IN) ? bottom_shadow_GC : top_shadow_GC,
             (shadow_type == XmSHADOW_IN) ? top_shadow_GC : bottom_shadow_GC,
              shadow_thickness,
              highlight_thickness,
              highlight_thickness,
              core_width - 2 * highlight_thickness,
              core_height - 2 * highlight_thickness);
      break;

      case XmSHADOW_ETCHED_IN:
      case XmSHADOW_ETCHED_OUT:
           XmDrawEtchedShadow (XtDisplay(w), XtWindow(w), 
                 (shadow_type == XmSHADOW_ETCHED_IN) ?
                         bottom_shadow_GC : top_shadow_GC,
                 (shadow_type == XmSHADOW_ETCHED_IN) ?
                             top_shadow_GC : bottom_shadow_GC,
                 shadow_thickness, 
                 highlight_thickness,
                     highlight_thickness, 
                 core_width - 2 * highlight_thickness,
                 core_height - 2 * highlight_thickness);
            
            break;

   }
}

/************************************************************************
 * Primitive:_XmDrawBorder, become XmDrawHighlight 
 ************************************************************************/
 
#ifdef _NO_PROTO
void _XmDrawBorder (w, gc, x, y, width, height, highlight_width) /* OBSOLETE */
Widget w;
GC gc;
Position x;
Position y;
Dimension width;
Dimension height;
Dimension highlight_width;
#else /* _NO_PROTO */
void _XmDrawBorder ( Widget w, GC gc, 
#if NeedWidePrototypes
                                      int x, int y, 
        int width, int height, int highlight_width)
#else
                                      Position x, Position y, 
        Dimension width, Dimension height, Dimension highlight_width)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */

{
   XRectangle rect[4];

   rect[0].x = x;
   rect[0].y = y;
   rect[0].width = width;
   rect[0].height = highlight_width;

   rect[1].x = x;
   rect[1].y = y;
   rect[1].width = highlight_width;
   rect[1].height = height;

   rect[2].x = x + width - highlight_width;
   rect[2].y = y;
   rect[2].width = highlight_width;
   rect[2].height = height;

   rect[3].x = x;
   rect[3].y = y + height - highlight_width;
   rect[3].width = width;
   rect[3].height = highlight_width;

   XFillRectangles (XtDisplay (w), XtWindow (w), gc, &rect[0], 4);
}

/***************************************************************
  All these functions were private but global in 1.1, so instead of
   just moving them static, we also provide the compatibility stuff.
   They are just wrapping around the new static names 
****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxCreateFilterLabel( fsb )
        XmFileSelectionBoxWidget fsb ;
#else
_XmFileSelectionBoxCreateFilterLabel(
        XmFileSelectionBoxWidget fsb )
#endif /* _NO_PROTO */
{
  FS_FilterLabel( fsb) = _XmBB_CreateLabelG( (Widget) fsb, 
					      FS_FilterLabelString( fsb),
					      "FilterLabel") ;
}

void 
#ifdef _NO_PROTO
_XmFileSelectionBoxCreateDirListLabel( fsb )
        XmFileSelectionBoxWidget fsb ;
#else
_XmFileSelectionBoxCreateDirListLabel(
        XmFileSelectionBoxWidget fsb )
#endif /* _NO_PROTO */
{
    FS_DirListLabel( fsb) = _XmBB_CreateLabelG( (Widget) fsb,
					       FS_DirListLabelString( fsb),
					       "Dir") ;
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxCreateDirList( fsb )
        XmFileSelectionBoxWidget fsb ;
#else
_XmFileSelectionBoxCreateDirList(
        XmFileSelectionBoxWidget fsb )
#endif /* _NO_PROTO */
{
	Arg		al[20];
	register int	ac = 0;
            XtCallbackProc callbackProc ;
/****************/

    FS_DirListSelectedItemPosition( fsb) = 0 ;

    XtSetArg( al[ac], XmNvisibleItemCount,
                                        SB_ListVisibleItemCount( fsb)) ; ac++ ;
    XtSetArg( al[ac], XmNstringDirection, SB_StringDirection( fsb));  ac++;
    XtSetArg( al[ac], XmNselectionPolicy, XmBROWSE_SELECT);  ac++;
    XtSetArg( al[ac], XmNlistSizePolicy, XmCONSTANT);  ac++;
    XtSetArg( al[ac], XmNscrollBarDisplayPolicy, XmSTATIC);  ac++;
    XtSetArg( al[ac], XmNnavigationType, XmSTICKY_TAB_GROUP) ; ++ac ;

    FS_DirList( fsb) = XmCreateScrolledList( (Widget) fsb, "DirList", al, ac);

    callbackProc = ((XmSelectionBoxWidgetClass) fsb->core.widget_class)
                                          ->selection_box_class.list_callback ;
    if(    callbackProc    )
    {   
        XtAddCallback( FS_DirList( fsb), XmNsingleSelectionCallback,
                                               callbackProc, (XtPointer) fsb) ;
        XtAddCallback( FS_DirList( fsb), XmNbrowseSelectionCallback,
                                               callbackProc, (XtPointer) fsb) ;
        XtAddCallback( FS_DirList( fsb), XmNdefaultActionCallback,
                                               callbackProc, (XtPointer) fsb) ;
        } 
    XtManageChild( FS_DirList( fsb)) ;

    return ;
}

/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxCreateFilterText( fs )
        XmFileSelectionBoxWidget fs ;
#else
_XmFileSelectionBoxCreateFilterText(
        XmFileSelectionBoxWidget fs )
#endif /* _NO_PROTO */
{
            Arg             arglist[10] ;
            int             argCount ;
            char *          stext_value ;
            XtAccelerators  temp_accelerators ;
/****************/

    /* Get text portion from Compound String, and set
    *   fs_stext_charset and fs_stext_direction bits...
    */
    /* Should do this stuff entirely with XmStrings when the text
    *   widget supports it.
    */
    if(    !(stext_value = _XmStringGetTextConcat( FS_Pattern( fs)))    )
    {   stext_value = (char *) XtMalloc( 1) ;
        stext_value[0] = '\0' ;
        }
    argCount = 0 ;
    XtSetArg( arglist[argCount], XmNcolumns, 
                                            SB_TextColumns( fs)) ; argCount++ ;
    XtSetArg( arglist[argCount], XmNresizeWidth, FALSE) ; argCount++ ;
    XtSetArg( arglist[argCount], XmNvalue, stext_value) ; argCount++ ;
    XtSetArg( arglist[argCount], XmNnavigationType, 
                                             XmSTICKY_TAB_GROUP) ; argCount++ ;
#ifndef USE_TEXT_IN_DIALOGS
    FS_FilterText( fs) = XmCreateTextField( (Widget) fs, "FilterText",
                                                           arglist, argCount) ;
#else
    XtSetArg( arglist[argCount], XmNeditMode, XmSINGLE_LINE) ; argCount++ ;
    XtSetArg( arglist[argCount], XmNrows, 1) ; argCount++ ;
    FS_FilterText( fs) = XmCreateText( fs, "FilterText",
                                                           arglist, argCount) ;
#endif
    /*	Install text accelerators.
    */
    temp_accelerators = fs->core.accelerators ;
    fs->core.accelerators = SB_TextAccelerators( fs) ;
    XtInstallAccelerators( FS_FilterText( fs), (Widget) fs) ;
    fs->core.accelerators = temp_accelerators ;

    XtFree( stext_value) ;
    return ;
}

/****************************************************************/
void
#ifdef _NO_PROTO
_XmFileSelectionBoxGetDirectory( fs, resource, value)
            Widget fs ;
            int resource ;
            XtArgVal *value ;
#else
_XmFileSelectionBoxGetDirectory(
            Widget fs,
            int resource,
            XtArgVal *value)
#endif
{
    XmString        data ;
/****************/
  
    data = XmStringCopy(FS_Directory(fs));
    *value = (XtArgVal) data ;
}
/****************************************************************/
void
#ifdef _NO_PROTO
_XmFileSelectionBoxGetNoMatchString( fs, resource, value)
            Widget fs ;
            int resource ;
            XtArgVal *value ;
#else
_XmFileSelectionBoxGetNoMatchString(
            Widget fs,
            int resource,
            XtArgVal *value)
#endif
{
    XmString        data ;
  
    data = XmStringCopy(FS_NoMatchString(fs));
    *value = (XtArgVal) data ;
}
/****************************************************************/
void
#ifdef _NO_PROTO
_XmFileSelectionBoxGetPattern( fs, resource, value)
            Widget fs ;
            int resource ;
            XtArgVal *value ;
#else
_XmFileSelectionBoxGetPattern(
            Widget fs,
            int resource,
            XtArgVal *value)
#endif
{
    XmString        data ;
  
    data = XmStringCopy(FS_Pattern(fs));
    *value = (XtArgVal) data ;
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxGetFilterLabelString( fs, resource_offset, value )
        Widget fs ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmFileSelectionBoxGetFilterLabelString(
        Widget fs,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmString        data ;
            Arg             al[1] ;

    XtSetArg( al[0], XmNlabelString, &data) ;
    XtGetValues( FS_FilterLabel( fs), al, 1) ;
    *value = (XtArgVal) data ;
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxGetDirListLabelString( fs, resource_offset, value )
        Widget fs ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmFileSelectionBoxGetDirListLabelString(
        Widget fs,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmString        data ;
            Arg             al[1] ;

    XtSetArg( al[0], XmNlabelString, &data) ;
    XtGetValues( FS_DirListLabel( fs), al, 1) ;
    *value = (XtArgVal) data ;
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxGetDirListItems( fs, resource_offset, value )
        Widget fs ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmFileSelectionBoxGetDirListItems(
        Widget fs,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmString        data ;
            Arg             al[1] ;

    XtSetArg( al[0], XmNitems, &data) ;
    XtGetValues( FS_DirList( fs), al, 1) ;
    *value = (XtArgVal) data ;
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxGetDirListItemCount( fs, resource_offset, value )
        Widget fs ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmFileSelectionBoxGetDirListItemCount(
        Widget fs,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmString        data ;
            Arg             al[1] ;

    XtSetArg( al[0], XmNitemCount, &data) ;
    XtGetValues( FS_DirList( fs), al, 1) ;
    *value = (XtArgVal) data ;
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxGetListItems( fs, resource_offset, value )
        Widget fs ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmFileSelectionBoxGetListItems(
        Widget fs,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmString        data ;
            Arg             al[1] ;

    if(    FS_StateFlags( fs) & XmFS_NO_MATCH    )
    {   
        *value = (XtArgVal) NULL ;
        } 
    else
    {   XtSetArg( al[0], XmNitems, &data) ;
        XtGetValues( SB_List( fs), al, 1) ;
        *value = (XtArgVal) data ;
        } 
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxGetListItemCount( fs, resource_offset, value )
        Widget fs ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmFileSelectionBoxGetListItemCount(
        Widget fs,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmString        data ;
            Arg             al[1] ;
/****************/

    if(    FS_StateFlags( fs) & XmFS_NO_MATCH    )
    {   
        *value = (XtArgVal) 0 ;
        } 
    else
    {   XtSetArg( al[0], XmNitemCount, &data) ;
        XtGetValues( SB_List( fs), al, 1) ;
        *value = (XtArgVal) data ;
        } 
}
/*****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxGetDirMask( fs, resource_offset, value )
        Widget fs ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmFileSelectionBoxGetDirMask(
        Widget fs,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{   
            String          filterText ;
            XmString        data ;

#ifndef USE_TEXT_IN_DIALOGS
    filterText = XmTextFieldGetString( FS_FilterText(fs)) ;
#else
    filterText = XmTextGetString( FS_FilterText(fs)) ;
#endif
    data = XmStringLtoRCreate( filterText, XmFONTLIST_DEFAULT_TAG) ;
    *value = (XtArgVal) data ;
    XtFree( filterText) ; 

    return ;
}

/****************************************************************/
static Widget 
#ifdef _NO_PROTO
GetActiveText( fsb, event )
        XmFileSelectionBoxWidget fsb ;
        XEvent *event ;
#else
GetActiveText(
        XmFileSelectionBoxWidget fsb,
        XEvent *event )
#endif /* _NO_PROTO */
{
            Widget          activeChild = NULL ;
/****************/

    if(    _XmGetFocusPolicy( (Widget) fsb) == XmEXPLICIT    )
    {   
        if(    (fsb->manager.active_child == SB_Text( fsb))
            || (fsb->manager.active_child == FS_FilterText( fsb))    )
        {   
            activeChild = fsb->manager.active_child ;
            } 
        } 
    else
    {   
#ifdef TEXT_IS_GADGET
        activeChild = _XmInputInGadget( (CompositeWidget) fsb, 
                                                          event->x, event->y) ;
        if(    (activeChild != SB_Text( fsb))
            && (activeChild != FS_FilterText( fsb))    )
        {   
            activeChild = NULL ;
            } 
#else /* TEXT_IS_GADGET */
        if(    SB_Text( fsb)
            && (XtWindow( SB_Text( fsb))
                                   == ((XKeyPressedEvent *) event)->window)   )
        {   activeChild = SB_Text( fsb) ;
            } 
        else
        {   if(    FS_FilterText( fsb)
                && (XtWindow( FS_FilterText( fsb)) 
                                  ==  ((XKeyPressedEvent *) event)->window)   )
            {   activeChild = FS_FilterText( fsb) ;
                } 
            } 
#endif /* TEXT_IS_GADGET */
        } 
    return( activeChild) ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxUpOrDown( wid, event, argv, argc )
        Widget wid ;
        XEvent *event ;
        String *argv ;
        Cardinal *argc ;
#else
_XmFileSelectionBoxUpOrDown(
        Widget wid,
        XEvent *event,
        String *argv,
        Cardinal *argc )
#endif /* _NO_PROTO */
{   
            XmFileSelectionBoxWidget fsb = (XmFileSelectionBoxWidget) wid ;
            int	            visible ;
            int	            top ;
            int	            key_pressed ;
            Widget	    list ;
            int	*           position ;
            int	            count ;
            Widget          activeChild ;
            Arg             av[5] ;
            Cardinal        ac ;
/****************/

    if(    !(activeChild = GetActiveText( fsb, event))    )
    {   return ;
        } 
    if(    activeChild == SB_Text( fsb)    )
    {   
        if(    FS_StateFlags( fsb) & XmFS_NO_MATCH    )
        {   return ;
            } 
        list = SB_List( fsb) ;
        position = &SB_ListSelectedItemPosition( fsb) ;
        } 
    else /* activeChild == FS_FilterText( fsb) */
    {   list = fsb->file_selection_box.dir_list ;
        position = &FS_DirListSelectedItemPosition( fsb) ;
        } 
    if(    !list    )
    {   return ;
        } 
    ac = 0 ;
    XtSetArg( av[ac], XmNitemCount, &count) ; ++ac ;
    XtSetArg( av[ac], XmNtopItemPosition, &top) ; ++ac ;
    XtSetArg( av[ac], XmNvisibleItemCount, &visible) ; ++ac ;
    XtGetValues( (Widget) list, av, ac) ;

    if(    !count    )
    {   return ;
        } 
    key_pressed = atoi( *argv) ;

    if(    *position == 0    )
    {   /*  No selection, so select first item.
        */
        XmListSelectPos( list, ++*position, True) ;
        } 
    else
    {   if(    !key_pressed && (*position > 1)    )
        {   /*  up  */
            XmListDeselectPos( list, *position) ;
            XmListSelectPos( list, --*position, True) ;
            }
        else
        {   if(    (key_pressed == 1) && (*position < count)    )
            {   /*  down  */
                XmListDeselectPos( list, *position) ;
                XmListSelectPos( list, ++*position, True) ;
                } 
            else
            {   if(    key_pressed == 2    )
                {   /*  home  */
                    XmListDeselectPos( list, *position) ;
                    *position = 1 ;
                    XmListSelectPos( list, *position, True) ;
                    } 
                else
                {   if(    key_pressed == 3    )
                    {   /*  end  */
                        XmListDeselectPos( list, *position) ;
                        *position = count ;
                        XmListSelectPos( list, *position, True) ;
                        } 
                    } 
                } 
            }
        } 
    if(    top > *position    )
    {   XmListSetPos( list, *position) ;
        } 
    else
    {   if(    (top + visible) <= *position    )
        {   XmListSetBottomPos( list, *position) ;
            } 
        } 
    return ;
}
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxRestore( wid, event, argv, argc )
        Widget wid ;
        XEvent *event ;
        String *argv ;
        Cardinal *argc ;
#else
_XmFileSelectionBoxRestore(
        Widget wid,
        XEvent *event,
        String *argv,
        Cardinal *argc )
#endif /* _NO_PROTO */
{   
            XmFileSelectionBoxWidget fsb = (XmFileSelectionBoxWidget) wid ;
            String          itemString ;
            String          dir ;
            String          mask ;
            int             dirLen ;
            int             maskLen ;
            Widget          activeChild ;
/****************/

    if(    !(activeChild = GetActiveText( fsb, event))    )
    {   return ;
        } 
    if(    activeChild == SB_Text( fsb)    )
    {   _XmSelectionBoxRestore( (Widget) fsb, event, argv, argc) ;
        } 
    else /* activeChild == FS_FilterText( fsb) */
    {   /* Should do this stuff entirely with XmStrings when the text
        *   widget supports it.
        */
        if(    (dir = _XmStringGetTextConcat( FS_Directory( fsb))) != NULL    )
        {   
            dirLen = strlen( dir) ;

            if ((mask = _XmStringGetTextConcat( FS_Pattern( fsb))) != NULL)
            {   
                maskLen = strlen( mask) ;
                itemString = XtMalloc( dirLen + maskLen + 1) ;
                strcpy( itemString, dir) ;
                strcpy( &itemString[dirLen], mask) ;
#ifndef USE_TEXT_IN_DIALOGS
                XmTextFieldSetString( FS_FilterText( fsb), itemString) ;
                XmTextFieldSetCursorPosition( FS_FilterText( fsb),
			    XmTextFieldGetLastPosition( FS_FilterText( fsb))) ;
#else
                XmTextSetString( FS_FilterText( fsb), itemString) ;
                XmTextSetCursorPosition( FS_FilterText( fsb),
			    XmTextGetLastPosition( FS_FilterText( fsb))) ;
#endif
                XtFree( itemString) ;
                XtFree( mask) ;
                } 
            XtFree( dir) ;
            }
        } 
    return ;
}
/*****************************************************************/
XmGeoMatrix 
#ifdef _NO_PROTO
_XmFileSBGeoMatrixCreate( wid, instigator, desired )
        Widget wid ;
        Widget instigator ;
        XtWidgetGeometry *desired ;
#else
_XmFileSBGeoMatrixCreate(
        Widget wid,
        Widget instigator,
        XtWidgetGeometry *desired )
#endif /* _NO_PROTO */
{
  return (*(xmFileSelectionBoxClassRec.bulletin_board_class
	                      .geo_matrix_create))( wid, instigator, desired) ;
}
/****************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmFileSelectionBoxNoGeoRequest( geoSpec )
        XmGeoMatrix geoSpec ;
#else
_XmFileSelectionBoxNoGeoRequest(
        XmGeoMatrix geoSpec )
#endif /* _NO_PROTO */
{

    if(    BB_InSetValues( geoSpec->composite)
        && (XtClass( geoSpec->composite) == xmFileSelectionBoxWidgetClass)    )
    {   
        return( TRUE) ;
        } 
    return( FALSE) ;
}
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmFileSelectionBoxFocusMoved( wid, client_data, data )
        Widget wid ;
        XtPointer client_data ;
        XtPointer data ;
#else
_XmFileSelectionBoxFocusMoved(
        Widget wid,
        XtPointer client_data,
        XtPointer data )
#endif /* _NO_PROTO */
{            
  (*(xmFileSelectionBoxClassRec.bulletin_board_class.focus_moved_proc))(
						      wid, client_data, data );
}

XmHeap 
#ifdef _NO_PROTO
_XmHeapCreate( segment_size )
        int segment_size ;
#else
_XmHeapCreate(
        int segment_size )
#endif /* _NO_PROTO */
{
    XmHeap	heap;

    heap = XtNew(XmHeapRec);
    heap->start = NULL;
    heap->segment_size = segment_size;
    heap->bytes_remaining = 0;
    return heap;
}

char * 
#ifdef _NO_PROTO
_XmHeapAlloc( heap, bytes )
        XmHeap heap ;
        Cardinal bytes ;
#else
_XmHeapAlloc(
        XmHeap heap,
        Cardinal bytes )
#endif /* _NO_PROTO */
{
    register char* heap_loc;
    if (heap == NULL) return XtMalloc(bytes);
    if (heap->bytes_remaining < bytes) {
	if ((bytes + sizeof(char*)) >= (heap->segment_size>>1)) {
	    /* preserve current segment; insert this one in front */
#ifdef _TRACE_HEAP
	    printf( "allocating large segment (%d bytes) on heap %#x\n",
		    bytes, heap );
#endif
	    heap_loc = XtMalloc(bytes + sizeof(char*));
	    if (heap->start) {
		*(char**)heap_loc = *(char**)heap->start;
		*(char**)heap->start = heap_loc;
	    }
	    else {
		*(char**)heap_loc = NULL;
		heap->start = heap_loc;
	    }
	    return heap_loc;
	}
	/* else discard remainder of this segment */
#ifdef _TRACE_HEAP
	printf( "allocating new segment on heap %#x\n", heap );
#endif
	heap_loc = XtMalloc((unsigned)heap->segment_size);
	*(char**)heap_loc = heap->start;
	heap->start = heap_loc;
	heap->current = heap_loc + sizeof(char*);
	heap->bytes_remaining = heap->segment_size - sizeof(char*);
    }
#ifdef WORD64
    /* round to nearest 8-byte boundary */
    bytes = (bytes + 7) & (~7);
#else
    /* round to nearest 4-byte boundary */
    bytes = (bytes + 3) & (~3);
#endif /* WORD64 */
    heap_loc = heap->current;
    heap->current += bytes;
    heap->bytes_remaining -= bytes; /* can be negative, if rounded */
    return heap_loc;
}

void 
#ifdef _NO_PROTO
_XmHeapFree( heap )
        XmHeap heap ;
#else
_XmHeapFree(
        XmHeap heap )
#endif /* _NO_PROTO */
{
    char* segment = heap->start;
    while (segment != NULL) {
	char* next_segment = *(char**)segment;
	XtFree(segment);
	segment = next_segment;
    }
    heap->start = NULL;
    heap->bytes_remaining = 0;
}

typedef struct {
    Boolean	*traversal_on;
    Boolean	*have_traversal;
    Boolean	*sensitive;
    Boolean	*ancestor_sensitive;
    Boolean	*mapped_when_managed;
    Boolean	*highlighted;
    Boolean	*managed;
    unsigned char *navigation_type;
}*WidgetNavigPtrs;

void 
#ifdef _NO_PROTO
_XmGetWidgetNavigPtrs( widget, np )
        Widget widget ;
        WidgetNavigPtrs np ;
#else
_XmGetWidgetNavigPtrs(
        Widget widget,
        WidgetNavigPtrs np )
#endif /* _NO_PROTO */
{
    np->sensitive 		= &(widget->core.sensitive);
    np->ancestor_sensitive	= &(widget->core.ancestor_sensitive);
    np->managed			= &(widget->core.managed);

    if (XmIsManager(widget))
      {
	  XmManagerWidget w = (XmManagerWidget) widget;

	  np->traversal_on 		= &(w->manager.traversal_on);
	  np->mapped_when_managed 	= &(w->core.mapped_when_managed);
	  np->navigation_type	 	= &(w->manager.navigation_type);
	  np->highlighted		= NULL;
	  np->have_traversal		= NULL;
      }
    else if (XmIsPrimitive(widget))
      {
	  XmPrimitiveWidget w = (XmPrimitiveWidget) widget;

	  np->traversal_on 		= &(w->primitive.traversal_on);
	  np->mapped_when_managed 	= &(w->core.mapped_when_managed);
	  np->navigation_type	 	= &(w->primitive.navigation_type);
	  np->highlighted		= &(w->primitive.highlighted);
	  np->have_traversal		= &(w->primitive.have_traversal);
      }
    else if (XmIsGadget(widget))
      {
	  XmGadget w = (XmGadget) widget;

	  np->traversal_on 		= &(w->gadget.traversal_on);
	  np->mapped_when_managed 	= NULL;
	  np->navigation_type	 	= &(w->gadget.navigation_type);
	  np->highlighted		= &(w->gadget.highlighted);
	  np->have_traversal		= &(w->gadget.have_traversal);
      }
    else /* it must be an object or foriegn widget */
      {
	  np->traversal_on 		= NULL;
	  np->mapped_when_managed 	= NULL;
	  np->navigation_type	 	= NULL;
	  np->highlighted		= NULL;
	  np->have_traversal		= NULL;
      }
}

void 
#ifdef _NO_PROTO
GetWidgetNavigPtrs( widget, np )
        Widget widget ;
        WidgetNavigPtrs np ;
#else
GetWidgetNavigPtrs(
        Widget widget,
        WidgetNavigPtrs np )
#endif /* _NO_PROTO */
{
  _XmGetWidgetNavigPtrs( widget, np) ;
}

Boolean 
#ifdef _NO_PROTO
_XmFindTraversablePrim( tabGroup )
        CompositeWidget tabGroup ;
#else
_XmFindTraversablePrim(
        CompositeWidget tabGroup )
#endif /* _NO_PROTO */
{
  Widget first = _XmNavigate( (Widget) tabGroup, XmTRAVERSE_CURRENT) ;

  return first ? (XmCONTROL_NAVIGABLE == _XmGetNavigability( first)) : FALSE ;
}

typedef enum {
    HereOnly,
    AboveOnly,
    BelowOnly,
    AboveAndBelow
} XmNavigTestType;

Boolean 
#ifdef _NO_PROTO
_XmPathIsTraversable( widget, navType, testType, visRect )
        Widget widget ;
        XmNavigationType navType ;
        XmNavigTestType testType ;
        XRectangle *visRect ;
#else
_XmPathIsTraversable(
        Widget widget,
#if NeedWidePrototypes
        int navType,
#else
        XmNavigationType navType,
#endif /* NeedWidePrototypes */
        XmNavigTestType testType,
        XRectangle *visRect )
#endif /* _NO_PROTO */
{
  return _XmFindTraversablePrim( (CompositeWidget) widget) ;
}

void 
#ifdef _NO_PROTO
SetMwmStuff( ove, nve )
        XmVendorShellExtObject ove ;
        XmVendorShellExtObject nve ;
#else
SetMwmStuff(
        XmVendorShellExtObject ove,
        XmVendorShellExtObject nve )
#endif /* _NO_PROTO */
{
  /* Sorry Charlie, this doesn't work any more.
   */
  return ;
}

void 
#ifdef _NO_PROTO
InitializeScrollBars( w )
        Widget w ;
#else
InitializeScrollBars(
        Widget w )
#endif /* _NO_PROTO */
{
  _XmInitializeScrollBars( w) ;
}

void
#ifdef _NO_PROTO
_XmBB_GetDialogTitle( bb, resource, value)
        Widget bb ;
        int resource ;
        XtArgVal *value ;
#else
_XmBB_GetDialogTitle(
        Widget bb,
        int resource,
        XtArgVal *value)
#endif
{
  XmString data = XmStringCopy((XmString) ((XmBulletinBoardWidget) bb)
                                                ->bulletin_board.dialog_title);
  *value = (XtArgVal) data ;
}

static Boolean 
#ifdef _NO_PROTO
_isISO( charset )
        String charset ;
#else
_isISO(
        String charset )
#endif /* _NO_PROTO */
{
  register int	i;
  
  if (strlen(charset) == 5) 
    {
      for (i = 0; i < 5; i++) 
	{
	  if (!isdigit((unsigned char)charset[i])) return (False);
	}
      return (True);
    }
  else return (False);
}

char * 
#ifdef _NO_PROTO
_XmCharsetCanonicalize( charset )
        String charset ;
#else
_XmCharsetCanonicalize(
        String charset )
#endif /* _NO_PROTO */
{
  String	new_s;
  int		len;
  
  /* ASCII -> ISO8859-1 */
  if (!strcmp(charset, "ASCII"))
    {
      len = strlen(XmSTRING_ISO8859_1);
      
      new_s = XtMalloc(len + 1);
      strncpy(new_s, XmSTRING_ISO8859_1, len);
      new_s[len] = '\0';
    }
  else if (_isISO(charset))
    {
      /* "ISO####-#" */
      new_s = XtMalloc(3 + 4 + 1 + 1 + 1);
      sprintf(new_s, "ISO%s", charset);
      new_s[7] = '-';
      new_s[8] = charset[4];
      new_s[9] = '\0';
    }
  else
    /* Anything else is copied but not modified. */
    {
      len = strlen(charset);
      
      new_s = XtMalloc(len + 1);
      strncpy(new_s, charset, len);
      new_s[len] = '\0';
    }
  return (new_s);
}

Widget
#ifdef _NO_PROTO
_XmGetDisplayObject(shell, args, num_args)
    Widget	shell;
    ArgList	args;
    Cardinal	*num_args;
#else /* _NO_PROTO */
_XmGetDisplayObject(Widget shell, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
  return XmGetXmDisplay( XtDisplay( shell)) ;
}

Widget
#ifdef _NO_PROTO
_XmGetScreenObject(shell, args, num_args)
    Widget	shell;
    ArgList	args;
    Cardinal	*num_args;
#else /* _NO_PROTO */
_XmGetScreenObject(Widget shell, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
  return XmGetXmScreen( XtScreen( shell)) ;
}

XmWrapperData
#ifdef _NO_PROTO
_XmGetWrapperData(w_class)
    WidgetClass		w_class;
#else /* _NO_PROTO */
_XmGetWrapperData (WidgetClass w_class)
#endif /* _NO_PROTO */
{
  /* If somebody actually used this, they are probably in for a surprise.
   */
  return (XmWrapperData) XtCalloc( 1, sizeof( XmWrapperDataRec)) ;
}

void 
#ifdef _NO_PROTO
_XmLowerCase( source, dest )
         register char *source ;
         register char *dest ;
#else
_XmLowerCase(
         register char *source,
         register char *dest )
#endif /* _NO_PROTO */
{
    register char ch;
    int i;

    for (i = 0; (ch = *source) != 0 && i < 999; source++, dest++, i++) {
    	if ('A' <= ch && ch <= 'Z')
	    *dest = ch - 'A' + 'a';
	else
	    *dest = ch;
    }
    *dest = 0;
}

void dump_external(){}
void dump_fontlist_cache(){}
void dump_fontlist(){}
void dump_internal(){}

void
#ifdef _NO_PROTO
_XmButtonPopdownChildren (rowcol)
XmRowColumnWidget rowcol;
#else /* _NO_PROTO */
_XmButtonPopdownChildren (XmRowColumnWidget rowcol)
#endif /* _NO_PROTO */
{
  if (RC_PopupPosted(rowcol))
    {
      (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
                     menu_shell_class.popdownEveryone))(RC_PopupPosted(rowcol),
							NULL,NULL,NULL);
    }
}

void
#ifdef _NO_PROTO
_XmInitializeMenuCursor()
#else /* _NO_PROTO */
_XmInitializeMenuCursor (void)
#endif /* _NO_PROTO */
{
  /* It's unlucky to hit this one...
   */
}

void
#ifdef _NO_PROTO
_XmCreateMenuCursor (m)
Widget m;
#else /* _NO_PROTO */
_XmCreateMenuCursor (Widget m)
#endif /* _NO_PROTO */
{
  /* ... or this one.
   */
}
XContext _XmMenuCursorContext ; /* This won't help much either. */


static Boolean simplistic_transient_flag ;

Boolean
#ifdef _NO_PROTO
_XmGetTransientFlag (w)
   Widget w;
#else /* _NO_PROTO */
_XmGetTransientFlag (Widget w)
#endif /* _NO_PROTO */
{
  return simplistic_transient_flag ;
}

void
#ifdef _NO_PROTO
_XmSetTransientFlag (w, value)
   Widget w;
   Boolean value;
#else /* _NO_PROTO */
_XmSetTransientFlag (Widget w, Boolean value)
#endif /* _NO_PROTO */
{
  /* In a very simple case, this might work.  To implement more of this
   *  for BC would probably not help an application which was invasive
   *  enough to use this flag in the first place.
   */
  simplistic_transient_flag = value ;
}

Boolean
#ifdef _NO_PROTO
_XmQueryPixmapCache (screen, image_name, foreground, background)
Screen * screen;
char   * image_name;
Pixel    foreground;
Pixel    background;
#else /* _NO_PROTO */
_XmQueryPixmapCache (Screen *screen, char *image_name,
			     Pixel foreground, Pixel background)
#endif /* _NO_PROTO */
{
  /* Simple case: return FALSE (creating new pixmap will find it in cache).
   */
  return FALSE ;
}

void
#ifdef _NO_PROTO
_XmRC_GetLabelString( rc, resource, value)
            XmRowColumnWidget rc ;
            XrmQuark        resource ;
            XtArgVal *      value ;
#else
_XmRC_GetLabelString(
            XmRowColumnWidget rc,
            XrmQuark        resource,
            XtArgVal *      value)
#endif
{
  *value = (XtArgVal) XmStringCopy(RC_OptionLabel(rc));
}

void
#ifdef _NO_PROTO
_XmRC_GetMenuAccelerator( rc, resource, value)
            XmRowColumnWidget rc ;
            XrmQuark        resource ;
            XtArgVal *      value ;
#else
_XmRC_GetMenuAccelerator(
            XmRowColumnWidget rc,
            XrmQuark        resource,
            XtArgVal *      value)
#endif
{
  String        data ;

  if (rc->row_column.menu_accelerator != NULL) {
     data = (String)XtMalloc(strlen(RC_MenuAccelerator(rc)) + 1);
     strcpy(data, RC_MenuAccelerator(rc));
     *value = (XtArgVal) data ;
   }
  else *value = (XtArgVal) NULL;
}

void
#ifdef _NO_PROTO
_XmRC_GetMnemonicCharSet( rc, resource, value)
            XmRowColumnWidget rc ;
            XrmQuark        resource ;
            XtArgVal *      value ;
#else
_XmRC_GetMnemonicCharSet(
            XmRowColumnWidget rc,
            XrmQuark        resource,
            XtArgVal *      value)
#endif
{
  Widget	label = XmOptionLabelGadget((Widget)rc);

  if (label)
    {
      int n = 0;
      Arg           al[1] ;
      String        data ;

      XtSetArg(al[n], XmNmnemonicCharSet, &data); n++;
      XtGetValues(label, al, n);
      *value = (XtArgVal) data ;
    }
  else 
    {
      *value = (XtArgVal) NULL;
    }
}

void
#ifdef _NO_PROTO
_XmScaleGetTitleString(wid, resource, value)
        Widget wid;
        int resource;
        XtArgVal *value;
#else
_XmScaleGetTitleString(
        Widget wid,
        int resource,
        XtArgVal *value)
#endif /* _NO_PROTO */
{
  XmScaleWidget scale = (XmScaleWidget) wid ;

  if (scale->scale.title == NULL) {
    *value = NULL ;
  } else { 
    Arg           al[1] ;

    XtSetArg (al[0], XmNlabelString, value);
    XtGetValues (scale->composite.children[0], al, 1);
  }
}

void
#ifdef _NO_PROTO
_XmTextFieldDestinationVisible( w, turn_on )
        Widget w ;
        Boolean turn_on ;
#else
_XmTextFieldDestinationVisible(
        Widget w,
#if NeedWidePrototypes
        int turn_on )
#else
        Boolean turn_on )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
  return ;
}

int 
#ifdef _NO_PROTO
_XmTextGetBaseLine( widget )
        Widget widget ;
#else
_XmTextGetBaseLine(
        Widget widget )
#endif /* _NO_PROTO */
{
  return XmTextGetBaseline( widget) ;
}

#ifdef _NO_PROTO
void _XmTextOutLoadGCsAndRecolorCursors(old_tw, new_tw)
XmTextWidget old_tw, new_tw;
#else /* _NO_PROTO */
void _XmTextOutLoadGCsAndRecolorCursors(XmTextWidget old_tw, XmTextWidget new_tw)
#endif /*  _NO_PROTO */
{
  return ;
}

_XmConst char *_XmTextEventBindings1 = _XmTextIn_XmTextEventBindings1 ;
_XmConst char *_XmTextEventBindings2 = _XmTextIn_XmTextEventBindings2 ;
_XmConst char *_XmTextEventBindings3 = _XmTextIn_XmTextEventBindings3 ;


/************************************************************************
 *
 *	The following are the gadget traversal action routines
 *      DD; was in Manager.c but not used anywhere.
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmDoGadgetTraversal( mw, event, direction )
        XmManagerWidget mw ;
        XEvent *event ;
        int direction ;
#else
_XmDoGadgetTraversal(
        XmManagerWidget mw,
        XEvent *event,
        int direction )
#endif /* _NO_PROTO */
{
  Widget ref_wid = mw->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = (Widget) mw ;
    }
  _XmMgrTraversal( ref_wid, direction) ;
}



/**********************************************************************
 *
 *  _XmBuildManagerResources
 *	Build up the manager's synthetic and constraint synthetic
 *	resource processing list by combining the super classes with 
 *	this class.
 *  DD: This one should is now static in Manager.c
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmBuildManagerResources( c )
        WidgetClass c ;
#else
_XmBuildManagerResources(
        WidgetClass c )
#endif /* _NO_PROTO */
{
        XmManagerWidgetClass wc = (XmManagerWidgetClass) c ;
	XmManagerWidgetClass sc;

	sc = (XmManagerWidgetClass) wc->core_class.superclass;

	_XmInitializeSyntheticResources(wc->manager_class.syn_resources,
		wc->manager_class.num_syn_resources);

	_XmInitializeSyntheticResources(
		wc->manager_class.syn_constraint_resources,
		wc->manager_class.num_syn_constraint_resources);

	if (sc == (XmManagerWidgetClass) constraintWidgetClass) return;

	_XmBuildResources (&(wc->manager_class.syn_resources),
		&(wc->manager_class.num_syn_resources),
		sc->manager_class.syn_resources,
		sc->manager_class.num_syn_resources);

	_XmBuildResources (&(wc->manager_class.syn_constraint_resources),
		&(wc->manager_class.num_syn_constraint_resources),
		sc->manager_class.syn_constraint_resources,
		sc->manager_class.num_syn_constraint_resources);
}


/** Gadget synthetic hook from Manager.c. Are not static in Gadget.c **/

void 
#ifdef _NO_PROTO
_XmGetHighlightColor( w, offset, value)
        Widget w ;
		int offset ;
		XtArgVal *value ;
#else
_XmGetHighlightColor(
        Widget w,
		int offset,
		XtArgVal *value )
#endif /* _NO_PROTO */
{
	XmManagerWidget mw = (XmManagerWidget) XtParent(w);

	*value = (XtArgVal) mw->manager.highlight_color;
}

void 
#ifdef _NO_PROTO
_XmGetTopShadowColor( w, offset, value)
        Widget w ;
		int offset ;
		XtArgVal *value ;
#else
_XmGetTopShadowColor(
        Widget w,
		int offset,
		XtArgVal *value )
#endif /* _NO_PROTO */
{
	XmManagerWidget mw = (XmManagerWidget) XtParent(w);

	*value = (XtArgVal) mw->manager.top_shadow_color;
}

void 
#ifdef _NO_PROTO
_XmGetBottomShadowColor( w, offset, value)
        Widget w ;
		int offset ;
		XtArgVal *value ;
#else
_XmGetBottomShadowColor(
        Widget w,
		int offset,
		XtArgVal *value )
#endif /* _NO_PROTO */
{
	XmManagerWidget mw = (XmManagerWidget) XtParent(w);

	*value = (XtArgVal) mw->manager.bottom_shadow_color;
}



/************************************************************************
 *
 *  The border highlighting and unhighlighting routines.
 *
 *  These routines were originally in Primitive.c but not used anywhere.
 *
 ************************************************************************/

void 
#ifdef _NO_PROTO
_XmHighlightBorder( w )
        Widget w ;
#else
_XmHighlightBorder(
        Widget w )
#endif /* _NO_PROTO */
{
    if(    XmIsPrimitive( w)    ) {   
        (*(xmPrimitiveClassRec.primitive_class.border_highlight))( w) ;
    }  else  {   
	if(    XmIsGadget( w)    ) {   
	    (*(xmGadgetClassRec.gadget_class.border_highlight))( w) ;
	} 
    } 
    return ;
} 

void 
#ifdef _NO_PROTO
_XmUnhighlightBorder( w )
        Widget w ;
#else
_XmUnhighlightBorder(
        Widget w )
#endif /* _NO_PROTO */
{
    if(    XmIsPrimitive( w)    )
    {   
        (*(xmPrimitiveClassRec.primitive_class.border_unhighlight))( w) ;
        } 
    else
    {   if(    XmIsGadget( w)    )
        {   
            (*(xmGadgetClassRec.gadget_class.border_unhighlight))( w) ;
            } 
        } 
    return ;
    }

/************************************************************************
 *
 *  This routine was global in Primitive.c. It is now static.
 *
 ************************************************************************/

void 
#ifdef _NO_PROTO
_XmBuildPrimitiveResources( c )
        WidgetClass c ;
#else
_XmBuildPrimitiveResources(
        WidgetClass c )
#endif /* _NO_PROTO */
{
        XmPrimitiveWidgetClass wc = (XmPrimitiveWidgetClass) c ;
	XmPrimitiveWidgetClass sc;

	sc = (XmPrimitiveWidgetClass) wc->core_class.superclass;

	_XmInitializeSyntheticResources(wc->primitive_class.syn_resources,
		wc->primitive_class.num_syn_resources);

	if (sc == (XmPrimitiveWidgetClass) widgetClass) return;

	_XmBuildResources (&(wc->primitive_class.syn_resources),
		&(wc->primitive_class.num_syn_resources),
		sc->primitive_class.syn_resources,
		sc->primitive_class.num_syn_resources);
}

