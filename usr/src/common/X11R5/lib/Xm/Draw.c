#pragma ident	"@(#)m1.2libs:Xm/Draw.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.2
else
 * Motif Release 1.2.4
endif
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/DrawP.h>

/*---------------------------------------------------------------*/
/* Functions used by Xm 1.2 widgets for the Motif visual drawing */
/*---------------------------------------------------------------*/
/* All these functions have an Xlib draw like API: 
      a Display*, a Drawable, then GCs, Positions and Dimensions 
      and finally some specific paramaters */
/*----------------------------------------------------------------*/
/*  _XmDrawShadows, 
       use in place of the 1.1 _XmDrawShadow and _XmDrawShadowType
       with changes to the interface (widget vs window, offsets, new order)
       and in the implementation (uses XSegments instead of XRectangles).
       Both etched and regular shadows use now a single private routine
       xmDrawSimpleShadow.
    _XmDrawSimpleHighlight.
       Implementation using FillRectangles, for solid highlight only. 
    _XmDrawHighlight.
       Highlight using wide lines, so that dash mode works. 
    _XmClearBorder,    
       new name for _XmEraseShadow  (_XmClearShadowType, which clear half a 
       shadow with a 'widget' API stays in Manager.c ) 
       XmClearBorder is only usable on window, not on drawable.
    _XmDrawSeparator, 
       use in place of the duplicate redisplay method of both separator and 
       separatorgadget (highlight_thickness not used, must be incorporated
       in the function call parameters). use xmDrawSimpleShadow.
       Has 2 new separator types for dash shadowed lines.
    _XmDrawDiamond, 
       new interface for _XmDrawDiamondButton (_XmDrawSquareButton is
       really a simple draw shadow and will be in the widget file as is).
    _XmDrawArrow, 
       same algorithm as before but in one function that re-uses the malloced
       rects and does not store anything in the wigdet instance.*/
/*--------------------------------------------------------------*/


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void xmDrawSimpleShadow() ;

#else

static void xmDrawSimpleShadow( 
                        Display *display,
                        Drawable d,
                        GC top_gc,
                        GC bottom_gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int shadow_thick,
                        int cor) ;
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension shadow_thick,
                        Dimension cor) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*-------------------- Private functions ----------------------*/
/*-------------------------------------------------------------*/

#ifdef _NO_PROTO
static void xmDrawSimpleShadow (display, d, top_gc, bottom_gc, 
                                x, y, width, height, shadow_thick, cor)
Display * display;
Drawable d;
GC top_gc;
GC bottom_gc;
Position x, y;
Dimension width, height, shadow_thick, 
          cor;  /* correction because etched have different visual */

#else /* _NO_PROTO */
static void xmDrawSimpleShadow (Display *display, Drawable d, 
                                GC top_gc, GC bottom_gc, 
#if NeedWidePrototypes
                                int x, int y, 
                                int width, int height, 
                                int shadow_thick, int cor)
#else
                                Position x, Position y, 
                                Dimension width, Dimension height, 
                                Dimension shadow_thick, Dimension cor)
#endif
#endif /* _NO_PROTO */
/* New implementation (1.2 vs 1.1) uses XSegments instead of 
   XRectangles. */
/* Used for the simple shadow, the etched shadow and the separators */
/* Segment has been faster than Rectangles in all my benches, either
   on Hp, Sun or Pmax. Lines has been slower, that I don't understand... */
{
   static XSegment * segms = NULL;
   static int segm_count = 0;
   register int i, size2, size3;

   if (!d) return;
   if (shadow_thick > (width >> 1)) shadow_thick = (width >> 1);
   if (shadow_thick > (height >> 1)) shadow_thick = (height >> 1);
   if (shadow_thick <= 0) return;

   size2 = (shadow_thick << 1);
   size3 = size2 + shadow_thick;

   if (segm_count < shadow_thick) {
      segms = (XSegment *) XtRealloc((char*)segms, 
                                     sizeof (XSegment) * (size2 << 1));
      segm_count = shadow_thick;
   }

   for (i = 0; i < shadow_thick; i++) {
       /*  Top segments  */
       segms[i].x1 = x;
       segms[i].y2 = segms[i].y1 = y + i;
       segms[i].x2 = x + width - i - 1;
       /*  Left segments  */
       segms[i + shadow_thick].x2 = segms[i + shadow_thick].x1 = x + i;
       segms[i + shadow_thick].y1 = y + shadow_thick;
       segms[i + shadow_thick].y2 = y + height - i - 1;

       /*  Bottom segments  */
       segms[i + size2].x1 = x + i + ((cor)?0:1) ;
       segms[i + size2].y2 = segms[i + size2].y1 = y + height - i - 1;
       segms[i + size2].x2 = x + width - 1 ;
       /*  Right segments  */
       segms[i + size3].x2 = segms[i + size3].x1 = x + width - i - 1;
       segms[i + size3].y1 = y + i + 1 - cor;
       segms[i + size3].y2 = y + height - 1 ;
   }

   XDrawSegments (display, d, top_gc, &segms[0], size2);
   XDrawSegments (display, d, bottom_gc, &segms[size2], size2);
}

/**************************** Public functions *************************/
/***********************************************************************/

/****************************_XmDrawShadows****************************/
#ifdef _NO_PROTO
void _XmDrawShadows(display, d, top_gc, bottom_gc,
                     x, y, width, height, shad_thick, 
                     shad_type)
Display *       display ;
Drawable        d;
GC              top_gc, bottom_gc;
Position        x, y;
Dimension       width, height, shad_thick;
unsigned int    shad_type;
#else /* _NO_PROTO */
void _XmDrawShadows(Display *display, Drawable d, 
                  GC top_gc, GC bottom_gc, 
#if NeedWidePrototypes
                                           int x, int y, 
                  int width, int height, int shad_thick, 
#else
                                           Position x, Position y, 
                  Dimension width, Dimension height, Dimension shad_thick, 
#endif
                  unsigned int shad_type)
#endif /* _NO_PROTO */
{
    GC  tmp_gc ;

    if(!d) return ;

    if ((shad_type == XmSHADOW_IN) || (shad_type == XmSHADOW_ETCHED_IN)) {
        tmp_gc = top_gc ;
        top_gc = bottom_gc ;  /* switch top and bottom shadows */
        bottom_gc = tmp_gc ;
    }

    if ((shad_type == XmSHADOW_ETCHED_IN || 
         shad_type == XmSHADOW_ETCHED_OUT) && (shad_thick != 1)) {
        xmDrawSimpleShadow (display, d, top_gc, bottom_gc, x, y, 
                            width, height, shad_thick/2, 1);
        xmDrawSimpleShadow (display, d, bottom_gc, top_gc, 
                            x + shad_thick/2, y + shad_thick/2, 
                            width - (shad_thick/2)*2, 
                            height - (shad_thick/2)*2, shad_thick/2, 1);
    } else
        xmDrawSimpleShadow (display, d, top_gc, bottom_gc, x, y, 
                            width, height, shad_thick, 0);
} 


/*****************************_XmClearBorder*********************************/
#ifdef _NO_PROTO
void _XmClearBorder (display, w, x, y, width, height, shadow_thick)
Display * display;
Window w;  
Position x, y;
Dimension width, height, shadow_thick;

#else /* _NO_PROTO */
void _XmClearBorder (Display *display, Window w, 
#if NeedWidePrototypes
                                                 int x, int y, 
                    int width, int height, int shadow_thick)
#else
                                                 Position x, Position y, 
                    Dimension width, Dimension height, Dimension shadow_thick)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

    if (!w || !shadow_thick || !width || !height) return ;

    XClearArea (display, w, x, y, width, shadow_thick, FALSE);
    XClearArea (display, w, x, y + height - shadow_thick, width, 
                shadow_thick, FALSE);
    XClearArea (display, w, x, y, shadow_thick, height, FALSE);
    XClearArea (display, w, x + width - shadow_thick, y, shadow_thick, 
                height, FALSE);
}

/******************************_XmDrawSeparator**********************/
#ifdef _NO_PROTO
void _XmDrawSeparator(display, d, top_gc, bottom_gc, separator_gc,
                     x, y, width, height, shadow_thick, margin, 
                     orientation, separator_type)
Display * display;
Drawable d;
GC top_gc, bottom_gc, separator_gc;
Position x, y ;
Dimension width, height, shadow_thick, margin;
unsigned char orientation, separator_type;
#else /* _NO_PROTO */
void _XmDrawSeparator(Display *display, Drawable d, 
                     GC top_gc, GC bottom_gc, GC separator_gc, 
#if NeedWidePrototypes
                     int x, int y, 
                     int width, int height, 
                     int shadow_thick, 
                     int margin, unsigned int orientation, 
                     unsigned int separator_type)
#else
                     Position x, Position y, 
                     Dimension width, Dimension height, 
                     Dimension shadow_thick, 
                     Dimension margin, unsigned char orientation, 
                     unsigned char separator_type)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */

{
   Position center;
   XSegment segs[2];
   GC   tmp_gc;
   int i, ndash, shadow_dash_size ;

   if (!d || (separator_type == XmNO_LINE)) return ;

   if (orientation == XmHORIZONTAL) {
       center = y + height / 2;
   } else {
       center = x + width / 2;
   }
           
   if (separator_type == XmSINGLE_LINE ||
       separator_type == XmSINGLE_DASHED_LINE) {
       if (orientation == XmHORIZONTAL) {
           segs[0].x1 = x + margin;
           segs[0].y1 = segs[0].y2 = center;
           segs[0].x2 = x + width - margin - 1;
       } else {
           segs[0].y1 = y + margin;
           segs[0].x1 = segs[0].x2 = center;
           segs[0].y2 = y + height - margin - 1;
       }
       XDrawSegments (display, d, separator_gc, segs, 1);
       return;
   }

   if (separator_type == XmDOUBLE_LINE ||
       separator_type == XmDOUBLE_DASHED_LINE) {
       if (orientation == XmHORIZONTAL) {
           segs[0].x1 = segs[1].x1 = x + margin;
           segs[0].x2 = segs[1].x2 = x + width - margin - 1;
           segs[0].y1 = segs[0].y2 = center - 1;
           segs[1].y1 = segs[1].y2 = center + 1;
       } else {
           segs[0].y1 = segs[1].y1 = y + margin;
           segs[0].y2 = segs[1].y2 = y + height - margin - 1;
           segs[0].x1 = segs[0].x2 = center - 1;
           segs[1].x1 = segs[1].x2 = center + 1;
       }
       XDrawSegments (display, d, separator_gc, segs, 2);
       return;
   }

   /* only shadowed stuff in the following, so shadowThickness has to be
      something real */
   if (shadow_thick < 1) return ;

   /* do the in/out effect now */
   if (separator_type == XmSHADOW_ETCHED_IN || 
       separator_type == XmSHADOW_ETCHED_IN_DASH) {
       tmp_gc = top_gc ;
       top_gc = bottom_gc ;
       bottom_gc = tmp_gc ;
   }

   /* In the following, we need to special case the shadow_thick = 2 or 3 case,
      since : it's the default and we don't like changes in visual here, 
      and also it looks non symetrical the way it is without special code:
                                    ......
	                            .,,,,,
	and you really want to have ......
                                    ,,,,,,

     So we won't use xmDrawSimpleShadow in this case, painful but hey..
   */


   /* Now the regular shadowed cases, in one pass with one looong dash 
      for the non dashed case */

   if (separator_type == XmSHADOW_ETCHED_IN_DASH ||
       separator_type == XmSHADOW_ETCHED_OUT_DASH)
   /* for now, shadowed dash use three time the shadow thickness as a 
      dash size, and worried about the shadow_thick odd values as well */
       shadow_dash_size = (shadow_thick/2)*2*3 ;
   else

   /* regular case, only one dash, the length of the separator */
       shadow_dash_size = (orientation == XmHORIZONTAL)?
           (width - 2*margin):(height - 2*margin) ;

#ifndef OSF_v1_2_4
   /* handle the shadow_thick = 1 case */
   if (!shadow_dash_size) shadow_dash_size = 5 ;

#else /* OSF_v1_2_4 */
   if(    shadow_dash_size == 0    )
     {
       /* Avoid division by zero below. */
       shadow_dash_size = 1 ;
     }
#endif /* OSF_v1_2_4 */
   /* ndash value will be 1 for the regular shadow case (not dashed) */
   if (orientation == XmHORIZONTAL) {
       ndash = ((width - 2*margin)/shadow_dash_size + 1)/2 ;       
       for (i=0; i<ndash; i++)
           if (shadow_thick < 4) {
	       XDrawLine(display, d, top_gc, 
			 x + margin + 2*i*shadow_dash_size, 
			 center - shadow_thick/2, 
			 x + margin + (2*i + 1)*shadow_dash_size -1, 
			 center - shadow_thick/2); 
	       if (shadow_thick > 1)
	           XDrawLine(display, d, bottom_gc, 
			 x + margin + 2*i*shadow_dash_size, 
			 center, 
			 x + margin + (2*i + 1)*shadow_dash_size -1, 
			 center); 
	   } else {
	       xmDrawSimpleShadow(display, d, top_gc, bottom_gc, 
				  x + margin + i*2*shadow_dash_size, 
				  center - shadow_thick/2, 
				  shadow_dash_size, (shadow_thick/2)*2, 
				  shadow_thick/2, 0);
	   }
       /* draw the last dash, with possibly a different size */
       if (i*2*shadow_dash_size < (width - 2*margin))
           if (shadow_thick < 4) {
	       XDrawLine(display, d, top_gc, 
			 x + margin + 2*i*shadow_dash_size, 
			 center - shadow_thick/2, 
			 x + (width - 2*margin), 
			 center - shadow_thick/2); 
	       if (shadow_thick > 1)
	           XDrawLine(display, d, bottom_gc, 
			 x + margin + 2*i*shadow_dash_size, 
			 center, 
			 x + (width - 2*margin), 
			 center); 
	   } else {
	       xmDrawSimpleShadow(display, d, top_gc, bottom_gc, 
				  x + margin + i*2*shadow_dash_size, 
				  center - shadow_thick/2, 
				  (width - 2*margin) - i*2*shadow_dash_size, 
				  (shadow_thick/2)*2, 
				  shadow_thick/2, 0);
	   }
   } else {
       ndash = ((height - 2*margin)/shadow_dash_size + 1)/2 ;
       for (i=0; i<ndash; i++)
           if (shadow_thick < 4) {
	       XDrawLine(display, d, top_gc, 
			 center - shadow_thick/2, 
			 y + margin + 2*i*shadow_dash_size, 
			 center - shadow_thick/2,
			 y + margin + (2*i + 1)*shadow_dash_size -1); 
	       if (shadow_thick > 1)
	           XDrawLine(display, d, bottom_gc, 
			 center, 
			 y + margin + 2*i*shadow_dash_size, 
			 center, 
			 y + margin + (2*i + 1)*shadow_dash_size -1); 
	   } else {
	       xmDrawSimpleShadow(display, d, top_gc, bottom_gc, 
				  center - shadow_thick/2, 
				  y + margin + i*2*shadow_dash_size, 
				  (shadow_thick/2)*2, shadow_dash_size, 
				  shadow_thick/2, 0);
	   }
       if (i*2*shadow_dash_size < (height - 2*margin))
           if (shadow_thick < 4) {
	       XDrawLine(display, d, top_gc, 
			 center - shadow_thick/2, 
			 y + margin + 2*i*shadow_dash_size, 
			 center - shadow_thick/2,
			 y + (height - 2*margin)); 
	       if (shadow_thick > 1)
	           XDrawLine(display, d, bottom_gc, 
			 center, 
			 y + margin + 2*i*shadow_dash_size, 
			 center, 
			 y + (height - 2*margin)); 
	   } else {
	       xmDrawSimpleShadow(display, d, top_gc, bottom_gc, 
				  center - shadow_thick/2, 
				  y + margin + i*2*shadow_dash_size, 
				  (shadow_thick/2)*2, 
				  (height - 2*margin) - i*2*shadow_dash_size, 
				  shadow_thick/2, 0);
	   }
   }
}


/***********************_XmDrawDiamond**********************************/
#ifdef _NO_PROTO
void _XmDrawDiamond (display, d, top_gc, bottom_gc, center_gc, 
                    x, y, width, height, shadow_thick, fill)
Display * display;
Drawable d;
GC top_gc, bottom_gc, center_gc;
Position x, y;
Dimension width, height, shadow_thick, fill;
#else /* _NO_PROTO */
void _XmDrawDiamond(Display *display, Drawable d, 
                    GC top_gc, GC bottom_gc, GC center_gc, 
#if NeedWidePrototypes
                    int x, int y, 
                    int width, int height, 
                    int shadow_thick, int fill)
#else
                    Position x, Position y, 
                    Dimension width, Dimension height, 
                    Dimension shadow_thick, Dimension fill)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XSegment seg[12];
   XPoint   pt[4];
   int midX, midY;
   int delta;

   if (!d || !width) return ;
   if (width % 2 == 0) width--;

   if (width == 1) {
       XDrawPoint(display, d, top_gc, x,y);
       return ;
   } else
   if (width == 3) {
       seg[0].x1 = x;                   
       seg[0].y1 = seg[0].y2 = y + 1;
       seg[0].x2 = x + 2;

       seg[1].x1 = seg[1].x2 = x + 1;           
       seg[1].y1 = y ;
       seg[1].y2 = y + 2;
       XDrawSegments (display, d, top_gc, seg, 2);
       return ;
   } else   {        /* NORMAL SIZED ToggleButtonS : initial width >= 5 */
       midX = x + (width + 1) / 2;
       midY = y + (width + 1) / 2;
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

       seg[3].x1 = midX - 1;            /*  5  */
       seg[3].y1 = y;
       seg[3].x2 = x + width - 1;       /*  6  */
       seg[3].y2 = midY - 1;

       seg[4].x1 = midX - 1;            /*  7  */
       seg[4].y1 = y + 1;
       seg[4].x2 = x + width - 2;       /*  8  */
       seg[4].y2 = midY - 1;

       seg[5].x1 = midX - 1;            /*  7  */
       seg[5].y1 = y + 2;
       seg[5].x2 = x + width - 3;       /*  8  */
       seg[5].y2 = midY - 1;

       /*  The bottom shadow segments  */
       seg[6].x1 = x;                   /*  9  */
       seg[6].y1 = midY - 1;
       seg[6].x2 = midX - 1;            /*  10  */
       seg[6].y2 = y + width - 1;

       seg[7].x1 = x + 1;               /*  11  */
       seg[7].y1 = midY - 1;
       seg[7].x2 = midX - 1;            /*  12  */
       seg[7].y2 = y + width - 2;

       seg[8].x1 = x + 2;               /*  11  */
       seg[8].y1 = midY - 1;
       seg[8].x2 = midX - 1;            /*  12  */
       seg[8].y2 = y + width - 3;

       seg[9].x1 = midX - 1;            /*  13  */
       seg[9].y1 = y + width - 1;
       seg[9].x2 = x + width - 1;       /*  14  */
       seg[9].y2 = midY - 1;

       seg[10].x1 = midX - 1;           /*  15  */
       seg[10].y1 = y + width - 2;
       seg[10].x2 = x + width - 2;      /*  16  */
       seg[10].y2 = midY - 1;

       seg[11].x1 = midX - 1;           /*  15  */
       seg[11].y1 = y + width - 3;
       seg[11].x2 = x + width - 3;      /*  16  */
       seg[11].y2 = midY - 1;
   }

   XDrawSegments (display, d, top_gc, &seg[3], 3);
   XDrawSegments (display, d, bottom_gc, &seg[6], 6);
   XDrawSegments (display, d, top_gc, &seg[0], 3);

   if (width == 5 || !center_gc) return ;    /* <= 5 in fact */

   /* Adjust visual depending on the value of fill. If false, leave a
      1 pixel background color border around the diamond. */

   delta = (fill ? 0 : 1);

   pt[0].x = x + 3 + delta;
   pt[0].y = pt[2].y = midY - 1;
   pt[1].x = pt[3].x = midX - 1 ;
   pt[1].y = y + 2 + delta;
   pt[2].x = x + width - 3 - delta;
   pt[3].y = y + width - 3 - delta;
   
   XFillPolygon (display, d, center_gc, pt, 4, Convex, CoordModeOrigin);
}


/****************************_XmDrawHighlight***************************
 *
 * This function modifies the given gc, which therefore needs to be created
 *   using XCreateGC or XtAllocateGC.
 *
 ***************************************************************************/

#ifdef _NO_PROTO
void _XmDrawHighlight (display, d, gc, x, y, width, height, 
			   highlight_thickness, line_style)
Display * display;
Drawable d;
GC gc;
Position x;
Position y;
Dimension width;
Dimension height;
Dimension highlight_thickness;
int line_style ;
#else /* _NO_PROTO */
void _XmDrawHighlight(Display *display, Drawable d, 
			  GC gc, 
#if NeedWidePrototypes
                          int x, int y, 
			  int width, int height,
			  int highlight_thickness,
#else
                          Position x, Position y, 
			  Dimension width, Dimension height,
			  Dimension highlight_thickness,
#endif /* NeedWidePrototypes */
                   int line_style)
#endif /* _NO_PROTO */
{
   XSegment seg[4];
   register Dimension half_hl = highlight_thickness/2 ;
   register Dimension cor = highlight_thickness % 2 ;

   if (!d || !highlight_thickness || !width || !height) return ;

   /* the XmList dash case relies on this particular order of X segments */
   
   seg[0].x1 = seg[2].x1 = x ;
   seg[0].y1 = seg[0].y2 = y + half_hl ;
   seg[0].x2 = x + width - highlight_thickness ;
   seg[1].x1 = seg[1].x2 = x + width - half_hl - cor;
   seg[1].y1 = seg[3].y1 = y ;
   seg[3].y2 = y + height - half_hl;
   seg[2].y1 = seg[2].y2 = y + height - half_hl - cor;
   seg[3].x1 = seg[3].x2 = x + half_hl ;
   seg[2].x2 = x + width ;
   seg[1].y2 = y + height ;

   XSetLineAttributes(display, gc,  highlight_thickness, line_style, 
		      CapButt, JoinMiter);
   XDrawSegments (display, d, gc, seg, 4);
  
}

/****************************_XmDrawSimpleHighlight*************************/
#ifdef _NO_PROTO
void _XmDrawSimpleHighlight (display, d, gc, x, y, width, height, 
			     highlight_thickness)
Display * display;
Drawable d;
GC gc;
Position x;
Position y;
Dimension width;
Dimension height;
Dimension highlight_thickness;
#else /* _NO_PROTO */
void _XmDrawSimpleHighlight(Display *display, Drawable d, 
			    GC gc, 
#if NeedWidePrototypes
			    int x, int y, 
			    int width, int height,
			    int highlight_thickness)
#else
                            Position x, Position y, 
                            Dimension width, Dimension height,
                            Dimension highlight_thickness)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XRectangle rect[4] ;

    if (!d || !highlight_thickness || !width || !height) return ;

    rect[0].x = rect[1].x = rect[2].x = x ;
    rect[3].x = x + width - highlight_thickness ;
    rect[0].y = rect[2].y = rect[3].y = y ;
    rect[1].y = y + height - highlight_thickness ;
    rect[0].width = rect[1].width = width ;
    rect[2].width = rect[3].width = highlight_thickness ;
    rect[0].height = rect[1].height = highlight_thickness ;
    rect[2].height = rect[3].height = height ;
    
    XFillRectangles (display, d, gc, rect, 4);

}

/****************************_XmDrawArrow**********************************/
#ifdef _NO_PROTO
void _XmDrawArrow(display, d, top_gc, bot_gc, cent_gc, 
                  x, y, width, height, shadow_thick, direction)
Display * display;
Drawable d;
GC top_gc, bot_gc, cent_gc;
Position x, y;
Dimension width, height, 
          shadow_thick; /* not used in this version - always 2 */
unsigned char direction;
#else /* _NO_PROTO */
void _XmDrawArrow(Display *display, Drawable d, 
                  GC top_gc, GC bot_gc, GC cent_gc, 
#if NeedWidePrototypes
                  int x, int y, 
                  int width, int height, int shadow_thick, 
                  unsigned int direction)
#else
                  Position x, Position y, 
                  Dimension width, Dimension height, Dimension shadow_thick, 
                  unsigned char direction)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   /* cent_gc might be NULL, which means don't draw anything on the center */
   /* in the current implementation, on shadow_thick = 2 or 0 are supported */

   static unsigned int allocated = 0;
   static XRectangle * top, * cent, * bot ;
   XRectangle * rect_tmp;
   int size, xOffset = 0, yOffset = 0, wwidth, start;
   register int temp, yy, i, h, w;
   short t = 0 , b = 0 , c = 0 ;

   if (!d) return ;

   /*  Get the size and the position and allocate the rectangle lists  */

   if (width > height) {
      size = height - 2;
      xOffset = (width - height) / 2;
   } else {
      size = width - 2 ;
      yOffset = (height - width) / 2;
   }
   if (size < 1) return;
   if (allocated < size) {
      top  = (XRectangle *) XtRealloc ((char*)top, 
                                       sizeof (XRectangle) * (size/2+6));
      cent = (XRectangle *) XtRealloc ((char*)cent, 
                                       sizeof (XRectangle) * (size/2+6));
      bot  = (XRectangle *) XtRealloc ((char*)bot, 
                                       sizeof (XRectangle) * (size/2+6));
      allocated = size;
   }

#define SWAP(x,y) temp = x ; x = y; y = temp

   if (direction == XmARROW_RIGHT || direction == XmARROW_LEFT) {
      SWAP(xOffset,yOffset) ;
   }

   /*  Set up a loop to generate the segments.  */

   wwidth = size;
   yy = size - 1 + yOffset;
   start = 1 + xOffset;

   while (wwidth > 0) {
       if (wwidth == 1) {
           top[t].x = start; top[t].y = yy + 1;
           top[t].width = 1; top[t].height = 1;
           t++;
       }
       else if (wwidth == 2) {
           if (size == 2 || (direction == XmARROW_UP || 
                             direction == XmARROW_LEFT)) {
               top[t].x = start; top[t].y = yy;
               top[t].width = 2; top[t].height = 1;
               t++;
               top[t].x = start; top[t].y = yy + 1;
               top[t].width = 1; top[t].height = 1;
               t++;
               bot[b].x = start + 1; bot[b].y = yy + 1;
               bot[b].width = 1; bot[b].height = 1;
               b++;
           }
           else if (direction == XmARROW_UP || direction == XmARROW_LEFT) {
               top[t].x = start; top[t].y = yy;
               top[t].width = 2; top[t].height = 1;
               t++;
               bot[b].x = start; bot[b].y = yy + 1;
               bot[b].width = 2; bot[b].height = 1;
               b++;
           }
       }
       else {
           if (start == 1 + xOffset)
               {
                   if (direction == XmARROW_UP || direction == XmARROW_LEFT) {
                       top[t].x = start; top[t].y = yy;
                       top[t].width = 2; top[t].height = 1;
                       t++;
                       top[t].x = start; top[t].y = yy + 1;
                       top[t].width = 1; top[t].height = 1;
                       t++;
                       bot[b].x = start + 1; bot[b].y = yy + 1;
                       bot[b].width = 1; bot[b].height = 1;
                       b++;
                       bot[b].x = start + 2; bot[b].y = yy;
                       bot[b].width = wwidth - 2; bot[b].height = 2;
                       b++;
                   }
                   else {
                       top[t].x = start; top[t].y = yy;
                       top[t].width = 2; top[t].height = 1;
                       t++;
                       bot[b].x = start; bot[b].y = yy + 1;
                       bot[b].width = 2; bot[b].height = 1;
                       b++;
                       bot[b].x = start + 2; bot[b].y = yy;
                       bot[b].width = wwidth - 2; bot[b].height = 2;
                       b++;
                   }
               }
           else {
               top[t].x = start; top[t].y = yy;
               top[t].width = 2; top[t].height = 2;
               t++;
               bot[b].x = start + wwidth - 2; bot[b].y = yy;
               bot[b].width = 2; bot[b].height = 2;
               if (wwidth == 3) {
                   bot[b].width = 1;
                   bot[b].x += 1;
               }
               b++;
               if (wwidth > 4) {
                   cent[c].x = start + 2; cent[c].y = yy;
                   cent[c].width = wwidth - 4; cent[c].height = 2;
                   c++;
               }
           }
       }
       start++;
       wwidth -= 2;
       yy -= 2;
   }

   if (direction == XmARROW_DOWN || direction == XmARROW_RIGHT) {
       rect_tmp = top; top = bot; bot = rect_tmp;
       SWAP(t, b);
   }


   /*  Transform the "up" pointing arrow to the correct direction  */

   switch (direction) {
   case XmARROW_LEFT:
       i = -1;
       do {
           i++;
           if (i < t) {
               SWAP(top[i].y, top[i].x);
               SWAP(top[i].width, top[i].height);
           }             
           if (i < b) {
               SWAP(bot[i].y, bot[i].x);
               SWAP(bot[i].width, bot[i].height);
           }             
           if (i < c) {
               SWAP(cent[i].y, cent[i].x);
               SWAP(cent[i].width, cent[i].height);
           }             
       } while (i < t || i < b || i < c);
       break;

   case XmARROW_RIGHT: 
       h = height - 2;
       w = width - 2;
       i = -1;
       do {
           i++;
           if (i < t) {
               SWAP(top[i].y, top[i].x); 
               SWAP(top[i].width, top[i].height);
               top[i].x = w - top[i].x - top[i].width + 2;
               top[i].y = h - top[i].y - top[i].height + 2;
           }             
           if (i < b) {
               SWAP(bot[i].y, bot[i].x); 
               SWAP(bot[i].width, bot[i].height);
               bot[i].x = w - bot[i].x - bot[i].width + 2;
               bot[i].y = h - bot[i].y - bot[i].height + 2;
           }             
           if (i < c) {
               SWAP(cent[i].y, cent[i].x); 
               SWAP(cent[i].width, cent[i].height);
               cent[i].x = w - cent[i].x - cent[i].width + 2;
               cent[i].y = h - cent[i].y - cent[i].height + 2;
           }
       } while (i < t || i < b || i < c);
       break;

   case XmARROW_DOWN:
       w = width - 2;
       h = height - 2;
       i = -1;
       do {
           i++;
           if (i < t) {
               top[i].x = w - top[i].x - top[i].width + 2;
               top[i].y = h - top[i].y - top[i].height + 2;
           }
           if (i < b) {
               bot[i].x = w - bot[i].x - bot[i].width + 2;
               bot[i].y = h - bot[i].y - bot[i].height + 2;
           }
           if (i < c) {
               cent[i].x = w - cent[i].x - cent[i].width + 2;
               cent[i].y = h - cent[i].y - cent[i].height + 2;
           }
       } while (i < t || i < b || i < c);
       break;
   }

   if (x != 0 || y != 0) {
       for (i = 0; i < t; i++) {
           top[i].x += x;
           top[i].y += y;
       }
       for (i = 0; i < c; i++) {
           cent[i].x += x;
           cent[i].y += y;
       }
       for (i = 0; i < b; i++) {
           bot[i].x += x;
           bot[i].y += y;
       }
   }

   if (shadow_thick) {
       XFillRectangles (display, d, top_gc, top, t);
       XFillRectangles (display, d, bot_gc, bot, b);
   } else {
       /* handle the case where arrow shadow_thickness = 0, which give
          a flat arrow */
       if (cent_gc) {
           XFillRectangles (display, d, cent_gc, top, t);
           XFillRectangles (display, d, cent_gc, bot, b);
       }
   } 
   if (shadow_thick == 1) {
       _XmDrawArrow(display, d, top_gc, bot_gc, cent_gc,
                  x+1, y+1, width-2, height-2, 0, direction) ;
   } else
   if (cent_gc) XFillRectangles (display, d, cent_gc, cent, c);
}
