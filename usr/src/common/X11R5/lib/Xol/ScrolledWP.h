#ifndef NOIDENT
#ident	"@(#)scrollwin:ScrolledWP.h	1.22"
#endif

/*
 * ScrolledWi.h
 *
 */

#ifndef _ScrolledWP_h
#define _ScrolledWP_h

#include <Xol/ManagerP.h>
#include <Xol/ScrolledWi.h>

typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} ScrolledWindowClassPart;

typedef struct _ScrolledWindowClassRec
   {
   CoreClassPart                 core_class;
   CompositeClassPart            composite_class;
   ConstraintClassPart           constraint_class;
   ManagerClassPart              manager_class;
   ScrolledWindowClassPart       scrolled_window_class;
   } ScrolledWindowClassRec;

extern ScrolledWindowClassRec scrolledWindowClassRec;

typedef void (*PFV)();

typedef struct _ScrolledWindowPart
   {
   /* resources */
   int                   current_page;
   OlDefine              show_page;
   int                   h_granularity;
   int                   h_initial_delay;
   int                   h_repeat_rate;
   XtCallbackList        h_slider_moved;
   int                   v_granularity;
   int                   v_initial_delay;
   int                   v_repeat_rate;
   XtCallbackList        v_slider_moved;
   PFV                   compute_geometries;
   Boolean               recompute_view_height;
   Boolean               recompute_view_width;
   Dimension             view_height;
   Dimension             view_width;
   Boolean               force_hsb;
   Boolean               force_vsb;
   OlDefine              align_horizontal;
   OlDefine              align_vertical;
   Boolean               vAutoScroll;
   Boolean               hAutoScroll;
   int                   init_x;
   int                   init_y;
   Pixel                 foreground;

   /* private state */
   BulletinBoardWidget   bboard;
   ScrollbarWidget       hsb;
   ScrollbarWidget       vsb;
   Widget                bb_child;
   Dimension             child_width;
   Dimension             child_bwidth;
   Dimension             child_height;
   PFV                   post_modify_geometry_notification;
   } ScrolledWindowPart;

typedef struct _ScrolledWindowRec 
   {
   CorePart            core;
   CompositePart       composite;
   ConstraintPart      constraint;
   ManagerPart         manager;
   ScrolledWindowPart  scrolled_window;
   } ScrolledWindowRec;

#endif
