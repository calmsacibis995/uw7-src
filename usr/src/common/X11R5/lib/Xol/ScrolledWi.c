/* 	copyright	"%c%"	*/
#ifndef NOIDENT
#ident	"@(#)scrollwin:ScrolledWi.c	1.88"
#endif

/*
 * ScrolledWi.c
 *
 */

/*************************************************************************
 *
 * Description: This file along with ScrolledWi.h and ScrolledWP.h implements
 *              the OPEN LOOK ScrolledWindow Widget
 *
 *******************************file*header******************************/


/*****************************************************************************
 *                                                                           *
 *              Copyright (c) 1988 by Hewlett-Packard Company                *
 *     Copyright (c) 1988 by the Massachusetts Institute of Technology       *
 *                                                                           *
 *****************************************************************************/

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/BulletinBP.h>
#include <Xol/ScrollbarP.h>
#include <Xol/ScrolledWP.h>
#include <Xol/EventObj.h>
#include <Xol/Flat.h>

#include <DnD/OlDnDVCX.h>

#define ClassName ScrolledWindow
#include <Xol/NameDefs.h>

typedef struct {
   Position     x, y;
   Dimension    width, height;
   Dimension    real_width, real_height;
   Dimension    border_width;
} _SWGeometryInfo;

/* This is a temporary workaround for DnD... We should remove the code
 * below once we got new DnD code from Sun. See OlLayoutScrolledWindow()
 * for other info...
 *
 * The following is an optimization to reduce the call to
 * OlDnDUpdateDropSiteGeometry()
 *
 * START HERE - grep for DND
 */
#define DelCacheData(sw)	 ForDnDCache(False, sw,(Position)0, \
					(Position)0, (Dimension)0, (Dimension)0)
#define EquCacheData(sw,x,y,w,h) ForDnDCache(True,(Widget)sw,x,y,w,h)

static Boolean ForDnDCache OL_ARGS((Boolean, Widget, Position, Position,
					Dimension, Dimension));
/* END HERE */

static void	ConditionalUpdate OL_ARGS((Widget, _SWGeometryInfo *));

static void	DetermineViewSize OL_ARGS((ScrolledWindowWidget,
					   OlSWGeometries *, Widget, int));

static void	LayoutWithChild OL_ARGS((
				  ScrolledWindowWidget, _SWGeometryInfo *,
				  BulletinBoardWidget, _SWGeometryInfo *,
				  ScrollbarWidget, _SWGeometryInfo *,
				  ScrollbarWidget, _SWGeometryInfo *,
				  Widget, int));

static Boolean	LayoutHSB OL_ARGS((ScrolledWindowWidget, _SWGeometryInfo *,
				   _SWGeometryInfo *, _SWGeometryInfo *,
				  ScrollbarWidget, _SWGeometryInfo *,
				  int, int *));

static Boolean	LayoutVSB OL_ARGS((ScrolledWindowWidget, _SWGeometryInfo *,
				   _SWGeometryInfo *, _SWGeometryInfo *,
				  ScrollbarWidget, _SWGeometryInfo *,
				  int, int *));

static void	ClassInitialize OL_NO_ARGS();

static void	Destroy OL_ARGS((Widget));

static XtGeometryResult
		GeometryManager OL_ARGS((Widget, XtWidgetGeometry *,
					 XtWidgetGeometry *));

static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));

static void	InsertChild OL_ARGS((Widget));

static void	Realize OL_ARGS((Widget, Mask *, XSetWindowAttributes *));

static void	Resize OL_ARGS((Widget));

static void	Redisplay OL_ARGS((Widget, XEvent *, Region));

static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,
				   ArgList, Cardinal *));

static void	GetValuesHook OL_ARGS((Widget, ArgList, Cardinal *));

static void	ChildResized OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));

static void	ChildDestroyed OL_ARGS((Widget, XtPointer, XtPointer));

static void	HSBMoved OL_ARGS((Widget, XtPointer, XtPointer));

static void	VSBMoved OL_ARGS((Widget, XtPointer, XtPointer));

static Boolean	CheckWidth OL_ARGS((ScrolledWindowWidget, _SWGeometryInfo *,
				     _SWGeometryInfo *, _SWGeometryInfo *,
				     ScrollbarWidget, _SWGeometryInfo *));

static Boolean	CheckHeight OL_ARGS((ScrolledWindowWidget, _SWGeometryInfo *,
				     _SWGeometryInfo *, _SWGeometryInfo *,
				     ScrollbarWidget, _SWGeometryInfo *));

#define MIN(X,Y)            (int)(((int)(X) < (int)(Y)) ? (X) : (Y))
#define MAX(X,Y)            (int)(((int)(X) > (int)(Y)) ? (X) : (Y))

#define SW_BB_BORDER_W(W)   (int)(OlScreenPointToPixel(OL_HORIZONTAL,1,XtScreen(W)))
#define SW_HSB_GAP(W)       (int)(OlScreenPointToPixel(OL_VERTICAL,2,XtScreen(W)))
#define SW_VSB_GAP(W)       (int)(OlScreenPointToPixel(OL_HORIZONTAL,2,XtScreen(W)))
#define SW_MINIMUM_HEIGHT   200
#define SW_MINIMUM_WIDTH    200
#define SW_SB_GRANULARITY   1
#define SW_SB_SLIDER_MIN    0
#define SW_SB_SLIDER_VALUE  0
#define VorHShadow(sw)	    sw->manager.shadow_thickness

#define SUB(A,B) ((int)(A) > (int)(B)? (A)-(B) : 0)

#define _SWGetGeomInfo(w, gi) \
   ((void)( \
     (gi)->x = (w)->core.x, \
     (gi)->y = (w)->core.y, \
     (gi)->width = (w)->core.width, \
     (gi)->height = (w)->core.height, \
     (gi)->border_width = 2 * (w)->core.border_width, \
     (gi)->real_height = (w)->core.height + (gi)->border_width, \
     (gi)->real_width = (w)->core.width + (gi)->border_width \
   ))

#define _SWSetInfoHeight(gi, h) \
   ((void)( \
     (gi)->height = (h), \
     (gi)->real_height = (gi)->height + (gi)->border_width \
   ))

#define _SWSetInfoRealHeight(gi, h) \
   ((void)( \
     (gi)->real_height = (h), \
     (gi)->height = SUB((gi)->real_height, (gi)->border_width) \
   ))

#define _SWSetInfoWidth(gi, w) \
   ((void)( \
     (gi)->width = (w), \
     (gi)->real_width = (gi)->width + (gi)->border_width \
   ))

#define _SWSetInfoRealWidth(gi, w) \
   ((void)( \
     (gi)->real_width = (w), \
     (gi)->width = SUB((gi)->real_width, (gi)->border_width) \
   ))

#define offset(field)   XtOffset(ScrolledWindowWidget, scrolled_window.field)
static XtResource resources[] = {
  {
    XtNhScrollbar, XtCHScrollbar, XtRWidget, sizeof(Widget),
    offset(hsb), XtRImmediate, (XtPointer)NULL
  },
  {
    XtNvScrollbar, XtCVScrollbar, XtRWidget, sizeof(Widget),
    offset(vsb), XtRImmediate, (XtPointer)NULL
  },
  {
    XtNalignHorizontal, XtCAlignHorizontal, XtROlDefine, sizeof(OlDefine),
    offset(align_horizontal), XtRImmediate, (XtPointer)OL_BOTTOM
  },
  {
    XtNalignVertical, XtCAlignVertical, XtROlDefine, sizeof(OlDefine),
    offset(align_vertical), XtRImmediate, (XtPointer)OL_RIGHT
  },
  {
    XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffset(ScrolledWindowWidget, core.border_width),
    XtRImmediate, (XtPointer)0
  },
  {
    XtNcurrentPage, XtCCurrentPage, XtRInt, sizeof(int),
    offset(current_page), XtRImmediate, (XtPointer)1
  },
  {
    XtNvAutoScroll, XtCVAutoScroll, XtRBoolean, sizeof(Boolean),
    offset(vAutoScroll), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNhAutoScroll, XtCHAutoScroll, XtRBoolean, sizeof(Boolean),
    offset(hAutoScroll), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNforceHorizontalSB, XtCForceHorizontalSB, XtRBoolean, sizeof(Boolean),
    offset(force_hsb), XtRImmediate, (XtPointer)FALSE
  },
  {
    XtNforceVerticalSB, XtCForceVerticalSB, XtRBoolean, sizeof(Boolean),
    offset(force_vsb), XtRImmediate, (XtPointer)FALSE
  },
  {
    XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    offset(foreground), XtRString, "XtDefaultForeground"
  },
  {
    XtNhStepSize, XtCHStepSize, XtRInt, sizeof(int),
    offset(h_granularity), XtRImmediate, (XtPointer)1
  },
  {
    XtNhInitialDelay, XtCHInitialDelay, XtRInt, sizeof(int),
    offset(h_initial_delay), XtRImmediate, (XtPointer)500
  },
  {
    XtNhRepeatRate, XtCHRepeatRate, XtRInt, sizeof(int),
    offset(h_repeat_rate), XtRImmediate, (XtPointer)100
  },
  {  
    XtNhSliderMoved, XtCHSliderMoved, XtRCallback, sizeof(XtCallbackProc),
    offset(h_slider_moved), XtRCallback, (XtPointer)NULL
  },
  {
    XtNinitialX, XtCInitialX, XtRInt, sizeof(int),
    offset(init_x), XtRImmediate, (XtPointer)0
  },
  {
    XtNinitialY, XtCInitialY, XtRInt, sizeof(int),
    offset(init_y), XtRImmediate, (XtPointer)0
  },
  {
    XtNrecomputeHeight, XtCRecomputeHeight, XtRBoolean,sizeof(Boolean),
    offset(recompute_view_height), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNrecomputeWidth, XtCRecomputeWidth, XtRBoolean, sizeof(Boolean),
    offset(recompute_view_width), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNshowPage, XtCShowPage, XtROlDefine, sizeof(OlDefine),
    offset(show_page), XtRImmediate, (XtPointer)OL_NONE
  },
  {
    XtNvStepSize, XtCVStepSize, XtRInt, sizeof(int),
    offset(v_granularity), XtRImmediate, (XtPointer)1
  },
  {
    XtNvInitialDelay, XtCVInitialDelay, XtRInt, sizeof(int),
    offset(v_initial_delay), XtRImmediate, (XtPointer)500
  },
  {
    XtNvRepeatRate, XtCVRepeatRate, XtRInt, sizeof(int),
    offset(v_repeat_rate), XtRImmediate, (XtPointer)100
  },
  {  
    XtNvSliderMoved, XtCVSliderMoved, XtRCallback, sizeof(XtCallbackProc),
    offset(v_slider_moved), XtRCallback, (XtPointer)NULL
  },
  {
    XtNviewHeight, XtCViewHeight, XtRDimension, sizeof(Dimension),
    offset(view_height), XtRImmediate, (XtPointer)0
  },
  {
    XtNviewWidth, XtCViewWidth, XtRDimension, sizeof(Dimension),
    offset(view_width), XtRImmediate, (XtPointer)0
  },
  {
    XtNcomputeGeometries, XtCComputeGeometries, XtRFunction, sizeof(PFV),
    offset(compute_geometries), XtRFunction, (XtPointer)NULL
  },
  {
    XtNpostModifyGeometryNotification, XtCPostModifyGeometryNotification,
    XtRFunction, sizeof(PFV),
    offset(post_modify_geometry_notification), XtRFunction, (XtPointer)NULL
  },
  {
	/* uom: pixel. Note that both GUIs have the same default.	*/
    XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
    XtOffset(ScrolledWindowWidget, manager.shadow_thickness),
    XtRString, (XtPointer)"2 points"
  },
  {
	/* Note that both GUIs have the same default.			*/
    XtNshadowType, XtCShadowType, XtROlDefine, sizeof(OlDefine),
    XtOffset(ScrolledWindowWidget, manager.shadow_type),
    XtRImmediate, (XtPointer)OL_SHADOW_ETCHED_IN
  },
};
#undef offset

/*************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record**************************/

ScrolledWindowClassRec scrolledWindowClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass)&managerClassRec,
    /* class_name         */    "ScrolledWindow",
    /* widget_size        */    sizeof(ScrolledWindowRec),
    /* class_initialize   */    ClassInitialize,
    /* class_partinit     */    NULL,
    /* class_inited       */    FALSE,
    /* initialize         */    Initialize,
    /* Init hook          */    NULL,
    /* realize            */    Realize,
    /* actions            */    NULL,
    /* num_actions        */    0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    TRUE,
    /* compress_exposure  */    TRUE,
    /* compress_enterleave*/    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    Resize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set values hook    */    NULL,
    /* set values almost  */    XtInheritSetValuesAlmost,
    /* get values hook    */    GetValuesHook,
    /* accept_focus       */    NULL,
    /* Version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* TM Table           */    NULL,
    /* query_geom         */    NULL,
  },
  {
/* composite_class fields */
    /* geometry_manager   */    (XtGeometryHandler)GeometryManager,
    /* change_managed     */    NULL,
    /* insert_child       */    InsertChild,
    /* delete_child       */    NULL,
    /* extension          */    NULL
  },
  {
    /* constraint_class fields */
    /* resources      */        (XtResourceList)NULL,
    /* num_resources  */        0,
    /* constraint_size*/        0,
    /* initialize     */        (XtInitProc)NULL,
    /* destroy        */        (XtWidgetProc)NULL,
    /* set_values     */        (XtSetValuesFunc)NULL
  },{
    /* manager_class fields   */
    /* highlight_handler  */	NULL,
    /* focus_on_select	  */	True,
    /* traversal_handler  */    NULL,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */    NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ NULL, 0 },
    /* transparent_proc	  */	XtInheritTransparentProc,
  },{
/* Scrolled Window class - none */     
   /* empty */                  0
  }
};

WidgetClass scrolledWindowWidgetClass = (WidgetClass)&scrolledWindowClassRec;


/*
 * ConditionalUpdate
 *
 */
static void 
ConditionalUpdate OLARGLIST((w, w_info))
	OLARG( Widget,            w)
	OLGRA( _SWGeometryInfo *, w_info)
{
   int different_position;
   int different_size;

   if (w_info-> width == 0) 
      w_info-> width = 1;
   if (w_info-> height == 0) 
      w_info-> height = 1;

   different_position = (w-> core.x != w_info-> x || w-> core.y != w_info-> y);
   different_size = (w-> core.width != w_info-> width || 
                     w-> core.height != w_info-> height);

   if (different_position && different_size)
      XtConfigureWidget(w, w_info-> x, w_info-> y, 
                        w_info-> width, w_info-> height,
                        w_info-> border_width / 2);
   else
      if (different_position)
         XtMoveWidget(w, w_info-> x, w_info-> y);
      else
         if (different_size)
            XtResizeWidget(w, 
                        w_info-> width, w_info-> height,
                        w_info-> border_width / 2);

} /* end of ConditionalUpdate */

/*
 * DetermineViewSize
 *
 */
static void
DetermineViewSize OLARGLIST((sw, gg, bbc, being_resized))
	OLARG( ScrolledWindowWidget, sw)
	OLARG( OlSWGeometries *,     gg)
	OLARG( Widget,		     bbc)
	OLGRA( int,                  being_resized)
{
   if ((sw-> scrolled_window.compute_geometries != NULL) && (bbc)) {
      Boolean need_vsb = TRUE;

      (*sw-> scrolled_window.compute_geometries)(bbc, gg);
      sw-> scrolled_window.force_vsb = gg-> force_vsb;
      sw-> scrolled_window.force_hsb = gg-> force_hsb;
      if (sw-> scrolled_window.force_vsb ||
            gg-> sw_view_height < gg-> bbc_real_height)
         gg->sw_view_width = SUB(gg->sw_view_width, gg->vsb_width);
      else
         need_vsb = FALSE;

      if (sw-> scrolled_window.force_hsb ||
            gg-> sw_view_width < gg-> bbc_real_width)
      {
	  gg->sw_view_height = SUB(gg->sw_view_height, gg->hsb_height);
	  if (!need_vsb && gg-> sw_view_height < gg-> bbc_real_height)
	      gg->sw_view_width = SUB(gg->sw_view_width, gg->vsb_width);
      }
   }
   else {
      Dimension min_width;
      Dimension min_height;

      if (sw->scrolled_window.view_width) {
              if (being_resized && ((int)sw->core.width <
                      (int)(gg->sw_view_width + 2 * VorHShadow(sw))))
                      min_width = SUB(sw->core.width, 2 * VorHShadow(sw));
              else
                      min_width = sw->scrolled_window.view_width;
      }
      else
              min_width = SUB(sw->core.width, 2 * VorHShadow(sw));

      if (sw->scrolled_window.view_height) {
              if (being_resized && ((int)sw->core.height <
                      (int)(gg->sw_view_height + 2 * VorHShadow(sw))))
                      min_height = SUB(sw->core.height, 2 * VorHShadow(sw));
              else
                      min_height = sw->scrolled_window.view_height;
      }
      else
              min_height = SUB(sw->core.height, 2 * VorHShadow(sw));

      /* can't be smaller than 1 */
      min_width  = MAX(min_width,  1);
      min_height = MAX(min_height, 1);

        /* recompute view size based on child size */
      if (bbc != (Widget)NULL) {
              if (sw->scrolled_window.recompute_view_width == TRUE)
                      min_width = MIN(gg->bbc_real_width +
				 2 * bbc->core.border_width, min_width);
              if (sw->scrolled_window.recompute_view_height == TRUE)
                        min_height = MIN(gg->bbc_real_height +
				 2 * bbc->core.border_width, min_height);
      }

      gg->sw_view_width  = min_width;
      gg->sw_view_height = min_height;
   }
} /* end of DetermineViewSize */

static Boolean
CheckHeight OLARGLIST((sw, sw_info, bb_info, bbc_info, vsb, vsb_info))
	OLARG( ScrolledWindowWidget,    sw)
	OLARG( _SWGeometryInfo *,	sw_info)
	OLARG( _SWGeometryInfo *,	bb_info)
	OLARG( _SWGeometryInfo *,	bbc_info)
	OLARG( ScrollbarWidget, 	vsb)
	OLGRA( _SWGeometryInfo *,	vsb_info)
{
        XtWidgetGeometry        intended;
        XtWidgetGeometry        preferred;

        if ((sw->scrolled_window.force_vsb == TRUE) ||
            bb_info->height < bbc_info->real_height) {
                intended.request_mode = CWHeight;
                intended.height = SUB(bb_info->real_height, 
				      vsb_info->border_width);
                switch(XtQueryGeometry((Widget)vsb, &intended, &preferred)) {
                case XtGeometryYes:
                        break;
                case XtGeometryAlmost:
                case XtGeometryNo:
                        if(preferred.height > intended.height) {
                          for (--intended.height;intended.height != 0;
                                --intended.height)
                                if (XtQueryGeometry((Widget)vsb,&intended,&preferred)
                                         == XtGeometryYes
                                    || preferred.height <= intended.height)
                                                break;
                        }
                        break;
                }
		bb_info->real_height = preferred.height +vsb_info->border_width;
                bb_info->height = SUB(bb_info->real_height, 2 * VorHShadow(sw));
                return TRUE;
        }
        return FALSE;
} /* end of CheckHeight */

static Boolean
CheckWidth OLARGLIST((sw, sw_info, bb_info, bbc_info, hsb, hsb_info))
	OLARG( ScrolledWindowWidget,    sw)
	OLARG( _SWGeometryInfo *,	sw_info)
	OLARG( _SWGeometryInfo *,	bb_info)
	OLARG( _SWGeometryInfo *,	bbc_info)
	OLARG( ScrollbarWidget,		hsb)
	OLGRA( _SWGeometryInfo *,	hsb_info)
{
        XtWidgetGeometry        intended;
        XtWidgetGeometry        preferred;

        if ((sw->scrolled_window.force_hsb == TRUE) ||
            bb_info->width < bbc_info->real_width) {
                intended.request_mode = CWWidth;
                intended.width = SUB(bb_info->real_width, 
				     hsb_info->border_width);
                switch(XtQueryGeometry((Widget)hsb, &intended, &preferred)) {
                case XtGeometryYes:
                        break;
                case XtGeometryAlmost:
                case XtGeometryNo:
                        if (preferred.width > intended.width) {
                           for (--intended.width;intended.width != 0;
                                --intended.width)
                                if (XtQueryGeometry((Widget)hsb,&intended,&preferred)
                                         == XtGeometryYes
                                        || preferred.width <= intended.width)
                                                break;
                        }
                        break;
                }
		bb_info->real_width = preferred.width + hsb_info->border_width;
                bb_info->width = SUB(bb_info->real_width, 2 * VorHShadow(sw));
                return TRUE;
        }
        else
                return FALSE;
} /* end of CheckWidth */


/*
 * LayoutWithChild
 *
 *	Pass in pointers instead of structures (_SWGeometryInfo)
 *	for efficiencies (_SW* marcos also touched).
 *
 * Note that OlSWGeometries() should do same thing but I can't do
 *	this because this is a public routine, sigh...
 */
static void
LayoutWithChild OLARGLIST((sw, sw_info, bb, bb_info, vsb, vsb_info, hsb, hsb_info, bbc, being_resized))
	OLARG( ScrolledWindowWidget, sw)
	OLARG( _SWGeometryInfo *,    sw_info)
	OLARG( BulletinBoardWidget,  bb)
	OLARG( _SWGeometryInfo *,    bb_info)
	OLARG( ScrollbarWidget,      vsb)
	OLARG( _SWGeometryInfo *,    vsb_info)
	OLARG( ScrollbarWidget,      hsb)
	OLARG( _SWGeometryInfo *,    hsb_info)
	OLARG( Widget,               bbc)
	OLGRA( int,                  being_resized)
{
   _SWGeometryInfo     bbc_info;
   Arg                 arg[10];
   Position            gap;
   OlSWGeometries      geometries;
   int                 expanded_height_for_HSB = 0;
   int                 expanded_width_for_VSB  = 0;
   int		       n;
   Boolean save_hsb =  sw-> scrolled_window.force_hsb;
   Boolean save_vsb =  sw-> scrolled_window.force_vsb;
   Dimension           pref_height;
   Dimension           pref_width;

   if (bbc)
	_SWGetGeomInfo(bbc, &bbc_info);
   else
	memset((char *)&bbc_info, 0, sizeof(bbc_info));
   geometries = GetOlSWGeometries(sw);
   DetermineViewSize(sw, &geometries, bbc, being_resized);

   bb_info->width = geometries.sw_view_width;
   bb_info->real_width = geometries.sw_view_width + 2 * VorHShadow(sw);
   bb_info->height = geometries.sw_view_height;
   bb_info->real_height = geometries.sw_view_height + 2 * VorHShadow(sw);
   _SWSetInfoWidth(&bbc_info, geometries.bbc_width);
   _SWSetInfoHeight(&bbc_info, geometries.bbc_height);
  
   if (!being_resized) {
        _SWSetInfoWidth(sw_info, bb_info->real_width);
        _SWSetInfoHeight(sw_info, bb_info->real_height);
   }

   (void)LayoutHSB(sw, sw_info, bb_info, &bbc_info, hsb, hsb_info,
           being_resized, &expanded_height_for_HSB);
   if (LayoutVSB(sw, sw_info, bb_info, &bbc_info, vsb, vsb_info,
        being_resized, &expanded_width_for_VSB))
      if (LayoutHSB(sw, sw_info, bb_info, &bbc_info, hsb, hsb_info,
           being_resized, &expanded_height_for_HSB))
         (void)CheckHeight(sw, sw_info, bb_info, &bbc_info, vsb, vsb_info);
  
   if (!being_resized && ((bb_info->width > sw->core.width) ||
         (bb_info->height > sw->core.height) ||
         (sw_info->width != sw->core.width) ||
         (sw_info->height != sw->core.height))) {
             int ret;

             if ((ret = XtMakeResizeRequest((Widget)sw, sw_info->width,
                     sw_info->height, &pref_width,
		     &pref_height)) == XtGeometryAlmost) {
                      _SWSetInfoWidth(sw_info, pref_width);
                      _SWSetInfoHeight(sw_info, pref_height);
                      (void)XtMakeResizeRequest((Widget)sw,
                              sw_info->width, sw_info->height,
                              (Dimension *)NULL, (Dimension *)NULL);
             }

             if (ret == XtGeometryNo) {
                  /*
                   * Resize request rejected!!!
                   * If resize request was due to scrollbars,
                   * then shrink the bulletin board size to allocate
                   * space for scrollbars. Also don't forget to reset
                   * size of SW to original.
                   */
                      _SWGetGeomInfo((Widget)sw, sw_info);
                      if (expanded_height_for_HSB) {
                              Dimension needed_space;
                              Dimension extra_space;
                              Dimension diff;

                              needed_space = hsb_info->real_height +
						SW_HSB_GAP(sw);
                              extra_space  = SUB(sw_info->height,
                                                bb_info->real_height);
                              diff    = SUB(needed_space, extra_space);
                              bb_info->height = SUB(bb_info->height, diff);
			      bb_info->real_height = bb_info->height +
						2 * VorHShadow(sw);
                      }
                      if (expanded_width_for_VSB) {
                              Dimension needed_space;
                              Dimension extra_space;
                              Dimension diff;

                              needed_space = vsb_info->real_width +
						SW_VSB_GAP(sw);
                              extra_space  = SUB(sw_info->height,
                                                bb_info->real_height);
                              diff       = SUB(needed_space, extra_space);
                              bb_info->width = SUB(bb_info->width, diff);
 			      bb_info->real_width = bb_info->width +
						2 * VorHShadow(sw);
                      }
             }
   }

   if ( sw-> scrolled_window.post_modify_geometry_notification != NULL && bbc &&
        (bb_info->width != geometries.sw_view_width ||
	 bb_info->height != geometries.sw_view_height) )
   {
      geometries.sw_view_width = bb_info->width;
      geometries.sw_view_height = bb_info->height;
      (*sw-> scrolled_window.post_modify_geometry_notification)(
			bbc, &geometries);
   }

   if (being_resized)
          _SWGetGeomInfo((Widget)sw, sw_info);

   if ((gap = bb_info->width - (bbc_info.x + bbc_info.real_width)) > 0)
      bbc_info.x = MIN(bbc_info.x + gap, 0);
   if ((gap = bb_info->height - (bbc_info.y + bbc_info.real_height)) > 0)
      bbc_info.y = MIN(bbc_info.y + gap, 0);

   /* adjust positions for alignment */
   if (sw->scrolled_window.align_horizontal == OL_BOTTOM) {
        hsb_info->y = bb_info->real_height + SW_HSB_GAP(sw);
	bb_info->y = 0;
   }
   else {
        hsb_info->y = 0;
        bb_info->y = hsb_info->real_height + SW_HSB_GAP(sw);
   }

   if (sw->scrolled_window.align_vertical == OL_RIGHT) {
	bb_info->x = 0;
        vsb_info->x = bb_info->real_width + SW_VSB_GAP(sw);
   }
   else {
        vsb_info->x = 0;
        bb_info->x = vsb_info->real_width + SW_VSB_GAP(sw);
   }

   vsb_info->y = bb_info->y;
   _SWSetInfoRealHeight(vsb_info, bb_info->real_height);

   hsb_info->x = bb_info->x;
   _SWSetInfoRealWidth(hsb_info, bb_info->real_width);

   bb_info->x += VorHShadow(sw);
   bb_info->y += VorHShadow(sw);

   if (bbc)
	ConditionalUpdate(bbc, &bbc_info);
   ConditionalUpdate((Widget)bb, bb_info);
   ConditionalUpdate((Widget)hsb, hsb_info);
   ConditionalUpdate((Widget)vsb, vsb_info);
  
   if (sw-> scrolled_window.hAutoScroll)
   {
      XtSetArg(arg[0], XtNproportionLength, 
           MAX(1, MIN(bbc_info.real_width, (int)bb_info->width)));
      XtSetArg(arg[1], XtNsliderMax,        MAX(1, bbc_info.real_width));
      XtSetArg(arg[2], XtNsliderValue,      -bbc_info.x);
      XtSetArg(arg[3], XtNgranularity,      MIN(MAX(1, bbc_info.real_width),
                                sw->scrolled_window.h_granularity));
      XtSetValues((Widget)hsb, arg, 4);
    
      XtSetSensitive((Widget)hsb, (bbc_info.real_width > bb_info->width));
   }
   else
      XtSetSensitive((Widget)hsb, TRUE);
      
   if (sw-> scrolled_window.vAutoScroll)
   {
      XtSetArg(arg[0], XtNproportionLength, 
           MAX(1, MIN(bbc_info.real_height, (int)bb_info->height)));
      XtSetArg(arg[1], XtNsliderMax,        MAX(1, bbc_info.real_height));
      XtSetArg(arg[2], XtNsliderValue,      -bbc_info.y);
      XtSetArg(arg[3], XtNgranularity,      MIN(MAX(1, bbc_info.real_height),
                                sw->scrolled_window.v_granularity));
      XtSetValues((Widget)vsb, arg, 4);
 
      XtSetSensitive((Widget)vsb, (bbc_info.real_height > bb_info->height));
   }
   else
      XtSetSensitive((Widget)vsb, TRUE);

   /* n is used here temporarily */
   n = ((bb_info->width < bbc_info.real_width) ||
                 (sw->scrolled_window.force_hsb == TRUE));
   if (XtIsRealized((Widget)hsb)) {
        if (n)
                XtMapWidget((Widget)hsb);
        else
                XtUnmapWidget((Widget)hsb);
   }
   else
        XtSetMappedWhenManaged((Widget)hsb, (n) ? TRUE : FALSE);

   n = ((bb_info->height < bbc_info.real_height) ||
                 (sw->scrolled_window.force_vsb == TRUE));
   if (XtIsRealized((Widget)vsb)) {
        if (n)
                XtMapWidget((Widget)vsb);
        else
                XtUnmapWidget((Widget)vsb);
   }
   else 
        XtSetMappedWhenManaged((Widget)vsb, (n) ? TRUE : FALSE);

   sw-> scrolled_window.force_hsb = save_hsb;
   sw-> scrolled_window.force_vsb = save_vsb;
} /* end of LayoutWithChild */

/*
 * OlLayoutScrolledWindow
 *
 * The \fIOlLayoutScrolledWindow\fR procedure is used to programatically
 * force a layout of the components of the scrolled window widget \fIsw\fR.
 *
 * Synopsis:
 *
 *#include <ScrolledWi.h>
 * ...
 */
extern void
OlLayoutScrolledWindow OLARGLIST((sw, being_resized))
	OLARG( ScrolledWindowWidget,	sw)
	OLGRA( int,			being_resized)
{
   _SWGeometryInfo     sw_info;
   BulletinBoardWidget bb;
   _SWGeometryInfo     bb_info;
   ScrollbarWidget     hsb;
   _SWGeometryInfo     hsb_info;
   ScrollbarWidget     vsb;
   _SWGeometryInfo     vsb_info;
   Widget              bbc;


   bb  = sw-> scrolled_window.bboard;
   bbc = sw-> scrolled_window.bb_child;
   hsb = sw-> scrolled_window.hsb;
   vsb = sw-> scrolled_window.vsb;

   _SWGetGeomInfo((Widget)bb, &bb_info);
   _SWGetGeomInfo((Widget)hsb, &hsb_info);
   _SWGetGeomInfo((Widget)sw, &sw_info);
   _SWGetGeomInfo((Widget)vsb, &vsb_info);

   LayoutWithChild(sw, &sw_info, bb, &bb_info, vsb, &vsb_info, hsb, 
                &hsb_info, bbc, being_resized);

	/* This is a temporary workaround for DnD... We should	*/
	/* remove the code below once we got new DnD code from	*/
	/* Sun. The code is from Kai...				*/

	/* If we can't get new code from Sun then this code	*/
	/* need to be re-visited because it only checks on the	*/
	/* direct child of scrolled window and also assumes	*/
	/* that the whole window is the drop area...		*/
   {
	Arg		args[3];
	OlDnDDropSiteID	drop_site_id = (OlDnDDropSiteID)NULL;
	Position	x, y;

	if (bbc == (Widget)NULL)
	{
		return;
	}

	XtSetArg(args[0], XtNx,		(XtArgVal)&x);
	XtSetArg(args[1], XtNy,		(XtArgVal)&y);
	XtSetArg(args[2], XtNdropSiteID,(XtArgVal)&drop_site_id);

	XtGetValues(bbc, args, 3);

	if (drop_site_id != (OlDnDDropSiteID)NULL &&
	    EquCacheData(sw, x, y, bb_info.width, bb_info.height) == False)
	{
		OlDnDSiteRect	rect;

		rect.x		= (x < 0) ? -x : x;
		rect.y		= (y < 0) ? -y : y;
		rect.width	= bb_info.width;
		rect.height	= bb_info.height;

		OlDnDUpdateDropSiteGeometry(drop_site_id, &rect, 1);
	}
   }
} /* end of OlLayoutScrolledWindow */

/*
 * LayoutHSB
 *
 */
static Boolean
LayoutHSB OLARGLIST((sw, sw_info, bb_info, bbc_info, hsb, hsb_info, being_resized, expanded_height_for_HSB))
	OLARG( ScrolledWindowWidget,	sw)
	OLARG( _SWGeometryInfo *,	sw_info)
	OLARG( _SWGeometryInfo *,	bb_info)
	OLARG( _SWGeometryInfo *,	bbc_info)
	OLARG( ScrollbarWidget,		hsb)
	OLARG( _SWGeometryInfo *,	hsb_info)
	OLARG( int,			being_resized)
	OLGRA( int *,			expanded_height_for_HSB)
{
   int needed_space;
   int extra_space;
   int real_extra_space;
   int diff;

   if (CheckWidth(sw, sw_info, bb_info, bbc_info, hsb, hsb_info))
   {
      needed_space = hsb_info->real_height + SW_HSB_GAP(sw);
      extra_space  = sw_info->height - bb_info->real_height;
      real_extra_space  = sw->core.height - bb_info->real_height;
      diff         = needed_space - extra_space;
      if (diff > 0) {
           if (being_resized ||
               ((sw->scrolled_window.view_height == 0) &&
               (real_extra_space < needed_space))) {
		   bb_info->real_height = SUB(bb_info->real_height,
					  (Dimension)diff);
		   bb_info->height = SUB(bb_info->real_height, 
					 2 * VorHShadow(sw));
                   *expanded_height_for_HSB = 0;
           }
           else {
                   _SWSetInfoHeight(sw_info, sw_info->height +
                                   (Dimension)diff);
                   *expanded_height_for_HSB = 1;
           }
           return TRUE;
      }
   }
   return FALSE;

} /* end of LayoutHSB */

/*
 * LayoutVSB
 *
 */
static Boolean
LayoutVSB OLARGLIST((sw, sw_info, bb_info, bbc_info, vsb, vsb_info, being_resized, expanded_width_for_VSB))
	OLARG( ScrolledWindowWidget, sw)
	OLARG( _SWGeometryInfo *,    sw_info)
	OLARG( _SWGeometryInfo *,    bb_info)
	OLARG( _SWGeometryInfo *,    bbc_info)
	OLARG( ScrollbarWidget,      vsb)
	OLARG( _SWGeometryInfo *,    vsb_info)
	OLARG( int,		     being_resized)
	OLGRA( int *,		     expanded_width_for_VSB)
{
   int needed_space;
   int extra_space;
   int real_extra_space;
   int diff;

   if (CheckHeight(sw, sw_info, bb_info, bbc_info, vsb, vsb_info))
   {
      needed_space = vsb_info->real_width + SW_VSB_GAP(sw);
      extra_space  = sw_info->width - bb_info->real_width;
      real_extra_space  = sw->core.width - bb_info->real_width;
      diff         = needed_space - extra_space;
      if (diff > 0) {
           if (being_resized ||
               ((sw->scrolled_window.view_width == 0) &&
                (real_extra_space < needed_space))) {
		   bb_info->real_width = SUB(bb_info->real_width,
					     (Dimension)diff);
		   bb_info->width = SUB(bb_info->real_width,
					2 * VorHShadow(sw));
                   *expanded_width_for_VSB = 0;
           }
           else {
                   _SWSetInfoWidth(sw_info, sw_info->width +
                                   (Dimension)diff);
                   *expanded_width_for_VSB = 1;
           }
           return TRUE;
      }
   }
   return FALSE;

} /* end of LayoutVSB */

/*
 * ClassInitialize
 *
 */
static void
ClassInitialize OL_NO_ARGS()
{
   ScrolledWindowWidgetClass myclass;
   CompositeWidgetClass      superclass;

   myclass = (ScrolledWindowWidgetClass)scrolledWindowWidgetClass;
   superclass = (CompositeWidgetClass)compositeWidgetClass;

   myclass-> composite_class.delete_child =
      superclass-> composite_class.delete_child;
   myclass-> composite_class.change_managed =
      superclass-> composite_class.change_managed;
} /* end of ClassInitialize */

/*
 * Destroy
 *
 */
static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
	XtRemoveAllCallbacks(w, XtNhSliderMoved);
	XtRemoveAllCallbacks(w, XtNvSliderMoved);

	DelCacheData(w);	/* DND */
} /* end of Destroy */

/*
 * GeometryManager
 *
 */
static XtGeometryResult
GeometryManager OLARGLIST((w, request, preferred_return))
	OLARG( Widget,			w)
	OLARG( XtWidgetGeometry *,	request)
	OLGRA( XtWidgetGeometry *,	preferred_return)
{

   OlLayoutScrolledWindow((ScrolledWindowWidget)w-> core.parent, 0);

   return XtGeometryYes;
} /* end of GeometryManager */

/*
 * Initialize
 *
 */
/* ARGSUSED */
static void 
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,   	request)
	OLARG( Widget,   	new)
	OLARG( ArgList,  	args)
	OLGRA( Cardinal *,	num_args)
{
   BulletinBoardWidget     bb;
   ScrolledWindowWidget    nsw = (ScrolledWindowWidget)new;
   Arg                     arg[20];
   Dimension               view_width  = nsw-> scrolled_window.view_width;
   Dimension               view_height = nsw-> scrolled_window.view_height;
   Widget 		   vPane = NULL;
   Widget 		   hPane = NULL;

   XtSetArg(arg[0], XtNborderWidth, 0);

   if (view_width == 0)
   {
      view_width = nsw-> core.width == 0 ? SW_MINIMUM_WIDTH : 
                        SUB(nsw-> core.width, 2 * VorHShadow(nsw));
   }
   if (view_height == 0)
   {
      view_height = nsw-> core.height == 0 ? SW_MINIMUM_HEIGHT :
                        SUB(nsw-> core.height, 2 * VorHShadow(nsw));
   }

   if (view_width <= 0)
      view_width = SW_MINIMUM_WIDTH;

   if (view_height <= 0)
      view_height = SW_MINIMUM_HEIGHT;

   if (nsw-> scrolled_window.init_x > 0)
   {
      OlVaDisplayWarningMsg(XtDisplay((Widget)nsw),
			   OleNinvalidResource,
			   OleTinitializeDefault,
			   OleCOlToolkitWarning,
			   OleMinvalidResource_initializeDefault,
			   XtName((Widget)nsw),
			   OlWidgetToClassName((Widget)nsw),
			   XtNinitialX);
      nsw-> scrolled_window.init_x = 0;
   }

   if (nsw-> scrolled_window.init_y > 0)
   {
      OlVaDisplayWarningMsg(XtDisplay((Widget)nsw),
			   OleNinvalidResource,
			   OleTinitializeDefault,
			   OleCOlToolkitWarning,
			   OleMinvalidResource_initializeDefault,
			   XtName((Widget)nsw),
			   OlWidgetToClassName((Widget)nsw),
			   XtNinitialY);
      nsw-> scrolled_window.init_y = 0;
   }

   nsw-> core.border_width      = 0;
   nsw-> core.background_pixel  = nsw-> core.parent-> core.background_pixel;
   nsw-> core.background_pixmap = ParentRelative;

   XtSetArg(arg[1], XtNborderColor, nsw-> core.border_pixel);
   XtSetArg(arg[2], XtNheight,      view_height);
   XtSetArg(arg[3], XtNwidth,       view_width);
   XtSetArg(arg[4], XtNlayout,      OL_IGNORE);
   XtSetArg(arg[5], XtNshadowThickness,  0);
   bb = nsw->scrolled_window.bboard = (BulletinBoardWidget)
			XtCreateManagedWidget(
				"BulletinBoard",
				bulletinBoardWidgetClass, (Widget)nsw,
				arg, 6);

   if (OlGetGui() == OL_OPENLOOK_GUI)  {
        MaskArg         mask_args[4];
        if (*num_args != 0) {
         	_OlSetMaskArg(mask_args[0], XtNvMenuPane, &vPane,
			OL_COPY_SOURCE_VALUE);
               	_OlSetMaskArg(mask_args[1], NULL, sizeof(Widget),
			OL_COPY_SIZE);
         	_OlSetMaskArg(mask_args[2], XtNhMenuPane, &hPane,
			OL_COPY_SOURCE_VALUE);
               	_OlSetMaskArg(mask_args[3], NULL, sizeof(Widget),
			OL_COPY_SIZE);
               	_OlComposeArgList(args, *num_args, mask_args, 4, NULL, NULL);
    	}
   }
   nsw-> scrolled_window.hsb = (ScrollbarWidget)XtVaCreateManagedWidget
               ("HScrollbar", scrollbarWidgetClass, (Widget)nsw,
		XtNwidth,        view_width,
		XtNinitialDelay, nsw-> scrolled_window.h_initial_delay,
		XtNrepeatRate,   nsw-> scrolled_window.h_repeat_rate,
		XtNorientation,  OL_HORIZONTAL,
		XtNsliderMin,    SW_SB_SLIDER_MIN,
		XtNsliderValue,  SW_SB_SLIDER_VALUE,
		XtNmenuPane,     hPane,
		NULL);

   nsw-> scrolled_window.vsb = (ScrollbarWidget)XtVaCreateWidget
               ("VScrollbar", scrollbarWidgetClass, (Widget)nsw,
		XtNheight,      view_height,
		XtNinitialDelay,nsw-> scrolled_window.v_initial_delay,
		XtNrepeatRate,  nsw-> scrolled_window.v_repeat_rate,
		XtNorientation, OL_VERTICAL,
		XtNsliderMin,   SW_SB_SLIDER_MIN,
		XtNsliderValue, SW_SB_SLIDER_VALUE,
		XtNshowPage,    nsw-> scrolled_window.show_page,
		XtNcurrentPage, nsw-> scrolled_window.current_page,
		XtNmenuPane,    vPane,
		NULL);

   if (OlGetGui() == OL_OPENLOOK_GUI)
   {
	OlAssociateWidget(new, (Widget)(nsw->scrolled_window.vsb), TRUE);
	OlAssociateWidget(new, (Widget)(nsw->scrolled_window.hsb), TRUE);
   }

   XtAddCallback((Widget)nsw-> scrolled_window.vsb, XtNsliderMoved, VSBMoved, NULL);
   XtAddCallback((Widget)nsw-> scrolled_window.hsb, XtNsliderMoved, HSBMoved, NULL);

   if (nsw-> scrolled_window.force_vsb || nsw-> scrolled_window.force_hsb)
   {
      if (nsw-> scrolled_window.force_vsb && 
          !nsw-> scrolled_window.recompute_view_width)
            view_width = SUB(view_width, 
	     (nsw-> scrolled_window.vsb-> core.width + SW_VSB_GAP(new)));
      if (nsw-> scrolled_window.force_hsb && 
          !nsw-> scrolled_window.recompute_view_height)
            view_height = SUB(view_height,
             (nsw-> scrolled_window.hsb-> core.height + SW_HSB_GAP(new)));

      XtSetArg(arg[0], XtNwidth,  view_width);
      XtSetArg(arg[1], XtNheight, view_height);

      XtSetValues((Widget)nsw-> scrolled_window.bboard, arg, 2);
      XtSetValues((Widget)nsw-> scrolled_window.hsb, arg, 1);
      XtSetValues((Widget)nsw-> scrolled_window.vsb, &arg[1], 1);
   }

   nsw-> scrolled_window.bb_child = NULL;

   nsw-> core.width = bb-> core.width + 2 * VorHShadow(nsw) + 
                     (Dimension)((nsw-> scrolled_window.force_vsb) ?
                        (nsw-> scrolled_window.vsb-> core.width +
			SW_VSB_GAP(new)) : 0);

   nsw-> core.height = bb-> core.height + 2 * VorHShadow(nsw) + 
                       (Dimension)((nsw-> scrolled_window.force_hsb) ?
                          (nsw-> scrolled_window.hsb-> core.height +
			  SW_HSB_GAP(new)) : 0);
} /* end of Initialize */

/*
 * InsertChild
 *
 */
static void
InsertChild OLARGLIST((w))
	OLGRA( Widget,     w)
{

   Arg            arg[3];
   CompositeWidgetClass superclass;
   ScrolledWindowWidget sw = (ScrolledWindowWidget)w-> core.parent;
   int n;

   superclass = (CompositeWidgetClass)compositeWidgetClass;
    
   /* assumption: vsb is the last autochild to be added */
   if (sw-> scrolled_window.vsb == NULL)
   {
      (*superclass-> composite_class.insert_child)(w);
      XtManageChild(w);
   }
   else
   {
      if (sw-> scrolled_window.bb_child == NULL ||
          sw-> scrolled_window.bb_child-> core.being_destroyed == TRUE)
      {
	   if (OlGetGui() == OL_OPENLOOK_GUI &&
	       sw->scrolled_window.bb_child &&
	       sw->scrolled_window.bb_child->core.being_destroyed == TRUE) {
		   OlUnassociateWidget((Widget)sw);
	   }

	   if (OlGetGui() == OL_OPENLOOK_GUI)
	   {
			/* Associate the Scrolled window with the
			 * inserted child
			 */
		   OlAssociateWidget(w, (Widget)sw, TRUE);
	   }

           sw-> scrolled_window.bb_child = w;
           w-> core.parent = (Widget)sw-> scrolled_window.bboard;
           XtAddCallback(
		w, XtNdestroyCallback, ChildDestroyed, (XtPointer)NULL);
           if (!_OlIsGadget(w))
                XtAddEventHandler(w, StructureNotifyMask, FALSE, ChildResized,
                        (XtPointer)sw);

           /* save child configure info */
           sw-> scrolled_window.child_width  = w-> core.width;
           sw-> scrolled_window.child_height = w-> core.height;
           sw-> scrolled_window.child_bwidth = w-> core.border_width;

	   n = 0;
           XtSetArg(arg[n], XtNx, sw-> scrolled_window.init_x); n++;
           XtSetArg(arg[n], XtNy, sw-> scrolled_window.init_y); n++;

	   /* if superclass of bbc is Flat then don't change highlight */
	   if (!XtIsSubclass(w, flatWidgetClass)) {
	       XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
	   }
           XtSetValues(w, arg, n);

           (*superclass-> composite_class.insert_child)(w);

           if (XtIsRealized((Widget)sw))
      	          OlLayoutScrolledWindow(sw, 0);
      }
      else
           OlVaDisplayErrorMsg(XtDisplay((Widget)sw),
			 OleNgoodParent,
			 OleToneChild,
			 OleCOlToolkitError,
			 OleMgoodParent_oneChild,
			 XtName((Widget)sw),
			 OlWidgetToClassName((Widget)sw));
   }
} /* end of InsertChild */

/*
 * Realize
 *
 */
static void
Realize OLARGLIST((w, value_mask, attributes))
	OLARG( Widget,                 w)
	OLARG( Mask *,                 value_mask)
	OLGRA( XSetWindowAttributes *, attributes)
{
   ScrolledWindowWidget sw = (ScrolledWindowWidget)w;

   OlLayoutScrolledWindow(sw, 0);

   attributes->background_pixmap = ParentRelative;
   *value_mask |= CWBackPixmap;
   *value_mask &= ~CWBackPixel;
   XtCreateWindow((Widget)sw, InputOutput, (Visual *)CopyFromParent,
                  *value_mask, attributes);
} /* end of Realize */

/*
 * Resize
 *
 */
static void
Resize OLARGLIST((w))
	OLGRA( Widget,	w)
{
	ScrolledWindowWidget sw  = (ScrolledWindowWidget)w;
  
	OlLayoutScrolledWindow(sw, 1);
} /* end of Resize */

static void
Redisplay OLARGLIST((w, event, region))
	OLARG( Widget,		w)
	OLARG( XEvent *,	event)
	OLGRA( Region,		region)
{
	ScrolledWindowWidget sw  = (ScrolledWindowWidget)w;
	Widget bb = (Widget)(sw->scrolled_window.bboard);
	
	OlgDrawBorderShadow(
		XtScreen(w), XtWindow(w),
		sw->manager.attrs,
		sw->manager.shadow_type,
		sw->manager.shadow_thickness,
		bb->core.x - VorHShadow(sw),
		bb->core.y - VorHShadow(sw),
		bb->core.width + 2 * VorHShadow(sw),
		bb->core.height + 2 * VorHShadow(sw)
	);
} /* end of Redisplay */

/*
 * GetValuesHook
 *
 */
static void
GetValuesHook OLARGLIST((w, args, num_args))
	OLARG( Widget,     w)
	OLARG( ArgList,    args)
	OLGRA( Cardinal *, num_args)
{
   ScrolledWindowWidget sw;
   BulletinBoardWidget  bb;
   MaskArg              mask_args[8];
   Arg                  sv_args[1];
   Widget *             vpane = NULL;
   Widget *             hpane = NULL;

   if (*num_args != 0) 
   {
      sw = (ScrolledWindowWidget)w;
      bb = sw-> scrolled_window.bboard;

      _OlSetMaskArg(
	mask_args[0], XtNviewWidth, bb-> core.width, OL_COPY_MASK_VALUE);
      _OlSetMaskArg(mask_args[1], NULL, sizeof(Dimension), OL_COPY_SIZE);
      _OlSetMaskArg(
	mask_args[2], XtNviewHeight, bb-> core.height, OL_COPY_MASK_VALUE);
      _OlSetMaskArg(mask_args[3], NULL, sizeof(Dimension), OL_COPY_SIZE);
      _OlSetMaskArg(mask_args[4], XtNvMenuPane, &vpane, OL_COPY_SOURCE_VALUE);
      _OlSetMaskArg(mask_args[5], NULL, sizeof(Widget *), OL_COPY_SIZE);
      _OlSetMaskArg(mask_args[6], XtNhMenuPane, &hpane, OL_COPY_SOURCE_VALUE);
      _OlSetMaskArg(mask_args[7], NULL, sizeof(Widget *), OL_COPY_SIZE);

      _OlComposeArgList(args, *num_args, mask_args, 8, NULL, NULL);

      if (vpane) 
      {
         XtSetArg(sv_args[0], XtNmenuPane, vpane);
         XtGetValues((Widget)sw-> scrolled_window.vsb, sv_args, 1);
      }

      if (hpane) 
      {
         XtSetArg(sv_args[0], XtNmenuPane, hpane);
         XtGetValues((Widget)sw-> scrolled_window.hsb, sv_args, 1);
      }
   }
} /* end of GetValuesHook */

/*
 * SetValues
 *
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
   ScrolledWindowWidget cur_sw      = (ScrolledWindowWidget)current;
   ScrolledWindowPart * cur_part    = &cur_sw-> scrolled_window;
   ScrolledWindowWidget new_sw      = (ScrolledWindowWidget)new;
   ScrolledWindowPart * new_part    =  &new_sw-> scrolled_window;
   Boolean              do_layout   = FALSE;
   Boolean              flag        = FALSE;
   int                  b           = 0;
   int                  h           = 0;
   int                  v           = 0;
   Arg                  bb_arg[10];
   Arg                  hsb_arg[20];
   Arg                  vsb_arg[20];

   if (new_part-> align_horizontal      != cur_part-> align_horizontal      ||
       new_part-> align_vertical        != cur_part-> align_vertical        ||
       new_part-> force_hsb             != cur_part-> force_hsb             ||
       new_part-> force_vsb             != cur_part-> force_vsb             ||
       new_part-> recompute_view_height != cur_part-> recompute_view_height ||
       new_part-> recompute_view_width  != cur_part-> recompute_view_width  ||
       new_sw-> core.height             != cur_sw-> core.height             ||
       new_sw-> core.width              != cur_sw-> core.width              ||
       new_part-> view_height           != cur_part-> view_height           ||
       new_part-> view_width            != cur_part-> view_width)
            do_layout = TRUE;

   if (new_part-> current_page != cur_part-> current_page)
   {
      XtSetArg(vsb_arg[v], XtNcurrentPage, new_part-> current_page);     v++;
   }

   if (new_part-> foreground != cur_part-> foreground)
   {
      XtSetArg(hsb_arg[h], XtNborderColor, new_part-> foreground);       h++;
      XtSetArg(hsb_arg[h], XtNforeground,  new_part-> foreground);       h++;
      XtSetArg(vsb_arg[v], XtNborderColor, new_part-> foreground);       v++;
      XtSetArg(vsb_arg[v], XtNforeground,  new_part-> foreground);       v++;
   }

   /* now check border color */
   if (cur_sw->core.border_pixel != new_sw->core.border_pixel)
   {
      XtSetArg(bb_arg[b], XtNborderColor,new_sw->core.border_pixel); b++;
   }

	/* See comments on "v_granularity" below for
	 * why having the 2nd check
	 */
   if (new_part-> h_granularity != cur_part-> h_granularity &&
       !new_part-> hAutoScroll)
   {
      XtSetArg(hsb_arg[h], XtNgranularity, new_part-> h_granularity);    h++;
   }

   if (new_part-> h_initial_delay != cur_part-> h_initial_delay)
   {
      XtSetArg(hsb_arg[h], XtNinitialDelay, new_part-> h_initial_delay); h++;
   }

   if (new_part-> h_repeat_rate != cur_part-> h_repeat_rate)
   {
      XtSetArg(hsb_arg[h], XtNrepeatRate, new_part-> h_repeat_rate);     h++;
   }

   if (new_part-> show_page != cur_part-> show_page)
   {
      XtSetArg(vsb_arg[v], XtNshowPage, new_part-> show_page);           v++;
   }

	/* This applies to h_granularity too...
	 *
	 * Well, actually, we don't need to the code below in
	 * either cases (i.e., vAutoScroll on/off), because:
	 *
	 * if vAutoScroll, LayoutWithChild will do proper
	 * calculations (i.e., XtNproportionLength, XtNsliderMax,
	 * XtNsliderValue) and then pass "v_granularity/XtNgranularity"
	 * along with others to the scrollbar. Do it here, may cause
	 * scrollbar bad granularity warning...
	 *
	 * if !vAutoScroll, then applications have full control of
	 * scrollbar. Scrolled window won't be involved at all.
	 * So why we still keep the code here. "Compatiabilities",
	 * if a ?smart? application/widget writer does set_values
	 * to "scrollbar" without including XtNgranularity first, and
	 * then do set_values to scrolled window afterward. In this
	 * case, if we disable the code below then "scrollbar" won't
	 * know the change at all (yuck... but is the fact)
	 */
   if (new_part-> v_granularity != cur_part-> v_granularity &&
       !new_part-> vAutoScroll)
   {
      XtSetArg(vsb_arg[v], XtNgranularity, new_part-> v_granularity);    v++;
   }

   if (new_part-> v_initial_delay != cur_part-> v_initial_delay)
   {
      XtSetArg(vsb_arg[v], XtNinitialDelay, new_part-> v_initial_delay); v++;
   }

   if (new_part-> v_repeat_rate != cur_part-> v_repeat_rate)
   {
      XtSetArg(vsb_arg[v], XtNrepeatRate, new_part-> v_repeat_rate);     v++;
   }

   if (do_layout)
   {
      if (new_sw-> core.width  == cur_sw-> core.width &&
          new_sw-> core.height == cur_sw-> core.height)
            OlLayoutScrolledWindow(new_sw, 0);
      flag = TRUE;
   }

   if (h != 0 || v != 0 || b != 0)
   {
      if (h != 0) 
         XtSetValues((Widget)cur_part-> hsb, hsb_arg, h);
      if (v != 0) 
         XtSetValues((Widget)cur_part-> vsb, vsb_arg, v);
      if (b != 0) 
         XtSetValues((Widget)cur_part-> bboard, bb_arg, b);
      flag = TRUE;
   }

   return (flag);
} /* end of SetValues */

/*
 * ChildResized
 *
 */
/* ARGSUSED */
static void
ChildResized OLARGLIST((w, client_data, event, continue_to_dispatch))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLARG( XEvent *,	event)
	OLGRA( Boolean *,	continue_to_dispatch)
{
   ScrolledWindowWidget    sw = (ScrolledWindowWidget)client_data;

   if ((sw-> scrolled_window.child_width  != w-> core.width)  ||
       (sw-> scrolled_window.child_height != w-> core.height) ||
       (sw-> scrolled_window.child_bwidth != w-> core.border_width))
   {
      OlLayoutScrolledWindow(sw, 0);
      sw-> scrolled_window.child_width  = w-> core.width;
      sw-> scrolled_window.child_height = w-> core.height;
      sw-> scrolled_window.child_bwidth = w-> core.border_width;
   }
} /* end of ChildResized */

/*
 * ChildDestroyed
 *
 */
static void
ChildDestroyed OLARGLIST((w, closure, call_data))
	OLARG( Widget,    w)
	OLARG( XtPointer, closure)
	OLGRA( XtPointer, call_data)
{
   ScrolledWindowWidget sw = (ScrolledWindowWidget)
				(w-> core.parent)-> core.parent;

   if (sw-> scrolled_window.bb_child-> core.being_destroyed == TRUE)
   {
      sw-> scrolled_window.bb_child = NULL;
      if (OlGetGui() == OL_OPENLOOK_GUI)
      {
		OlUnassociateWidget((Widget)sw);
      }
      OlLayoutScrolledWindow(sw, 0);
   }
} /* end of ChildDestroyed */

/*
 * HSBMoved
 *
 */
static void
HSBMoved OLARGLIST((w, closure, call_data))
	OLARG( Widget,    w)
	OLARG( XtPointer, closure)
	OLGRA( XtPointer, call_data)
{
   OlScrollbarVerify *  olsbv = (OlScrollbarVerify *)call_data;
   ScrollbarWidget      sb    = (ScrollbarWidget)w;
   ScrolledWindowWidget sw    = (ScrolledWindowWidget)w-> core.parent;

   if (sw-> scrolled_window.bb_child == (Widget)NULL)
      olsbv-> ok = FALSE;

   XtCallCallbacks((Widget)sw, XtNhSliderMoved, call_data);

   if (olsbv-> ok)
      if (sw-> scrolled_window.hAutoScroll)
         XtMoveWidget(sw-> scrolled_window.bb_child,
                     (Position) (-olsbv-> new_location),
                     (Position)(sw-> scrolled_window.bb_child-> core.y));
} /* end of HSBMoved */

/*
 * VSBMoved
 *
 */
static void
VSBMoved OLARGLIST((w, closure, call_data))
	OLARG( Widget,    w)
	OLARG( XtPointer, closure)
	OLGRA( XtPointer, call_data)
{
   OlScrollbarVerify *  olsbv = (OlScrollbarVerify *)call_data;
   ScrollbarWidget      sb    = (ScrollbarWidget)w;
   ScrolledWindowWidget sw    = (ScrolledWindowWidget)w->core.parent;
   int                  page;

   if (sw-> scrolled_window.bb_child == (Widget)NULL)
      olsbv-> ok = FALSE;

   XtCallCallbacks((Widget)sw, XtNvSliderMoved, call_data);

   if (olsbv-> ok)
   {
      if (sw-> scrolled_window.vAutoScroll)
         XtMoveWidget(sw-> scrolled_window.bb_child,
                      (Position)(sw-> scrolled_window.bb_child-> core.x),
                      (Position) -(olsbv-> new_location));
   }
} /* end of VSBMoved */

/*
 * GetOlSWGeometries
 *
 * The \fIGetOlSWGeometries\fR function is used to retrieve the 
 * \fIOlSWGeometries\fR for the scrolled window widget \fIsw\fR.
 *
 * Synopsis:
 * 
 *#include <ScrolledWi.h>
 * ...
 */
extern OlSWGeometries
GetOlSWGeometries OLARGLIST((sw))
	OLGRA( ScrolledWindowWidget,  sw)
{
   BulletinBoardWidget  bb;
   Widget               bbc;
   ScrollbarWidget      hsb;
   ScrollbarWidget      vsb;
   OlSWGeometries geometries;

   bb  = sw->scrolled_window.bboard;
   bbc = sw->scrolled_window.bb_child;
   hsb = sw->scrolled_window.hsb;
   vsb = sw->scrolled_window.vsb;

   geometries.sw              = (Widget)sw;
   geometries.vsb             = (Widget)vsb;
   geometries.hsb             = (Widget)hsb;
   geometries.bb_border_width = VorHShadow(sw);
   geometries.vsb_width       = vsb-> core.width + SW_VSB_GAP(sw) +
       vsb->core.border_width * 2;
   geometries.vsb_min_height  = 36;
   geometries.hsb_height      = hsb-> core.height + SW_HSB_GAP(sw) +
       hsb->core.border_width * 2;
   geometries.hsb_min_width   = 36;
   geometries.sw_view_width   = SUB(sw-> core.width, 2 * VorHShadow(sw));
   geometries.sw_view_height  = SUB(sw-> core.height, 2 * VorHShadow(sw));
   if (bbc) {
	geometries.bbc_width       = bbc-> core.width;
	geometries.bbc_height      = bbc-> core.height;
	geometries.bbc_real_width  = bbc-> core.width;
	geometries.bbc_real_height = bbc-> core.height;
   }
   else {
	geometries.bbc_width       = 
	geometries.bbc_height      = 
	geometries.bbc_real_width  =
	geometries.bbc_real_height = 0;
   }
   geometries.force_vsb       = sw-> scrolled_window.force_vsb;
   geometries.force_hsb       = sw-> scrolled_window.force_hsb;

   return geometries;
} /* end of GetOlSWGeometries */

/* This is a temporary workaround for DnD... We should remove the code
 * below once we got new DnD code from Sun. See OlLayoutScrolledWindow()
 * for other info...
 *
 * The following is an optimization to reduce the call to
 * OlDnDUpdateDropSiteGeometry()
 *
 * DND
 */
typedef struct {
	Widget		key;	/* bbc	*/
	Position	x, y;
	Dimension	w, h;
} CacheDataRec, *CacheData;

static Boolean
ForDnDCache OLARGLIST((do_check, sw, x, y, w, h))
	OLARG( Boolean,		do_check)
	OLARG( Widget,		sw)	/* scrolled window id	*/
	OLARG( Position,	x)
	OLARG( Position,	y)
	OLARG( Dimension,	w)
	OLGRA( Dimension,	h)
{
	static CacheData	table = NULL;
	static short		slots_left = 0,
				slots_alloced = 0,
				entries = 0;

	Cardinal		i;

	if (do_check == False)
	{
		for (i = 0; i < entries; i++)
		{
			if (table[i].key == sw)
			{
				memcpy(&table[i], &table[i+1],
						entries - i - 1);
				slots_left++;
				entries--;
				break;
			}
		}
		return(True);
	}
	else
	{
		for (i = 0; i < entries; i++)
		{
			if (table[i].key == sw)
			{
#define MATCH	(table[i].x == x && table[i].y == y &&	\
		 table[i].w == w && table[i].h == h)

				if (MATCH == True)
					return(True);
				else
				{
					table[i].x = x;
					table[i].y = y;
					table[i].w = w;
					table[i].h = h;
					return(False);
				}
#undef MATCH
			}
		}

			/* if not found then add the data	*/
		if (table == NULL || slots_left == 0)
		{
			unsigned short	more_slots;

			more_slots	 = slots_alloced / 2 + 2;
			slots_left	+= more_slots;
			slots_alloced	+= more_slots;
			table = (CacheData)XtRealloc(
						(char *)table,
						(int)sizeof(CacheDataRec) *
						slots_alloced
			);
		}
		slots_left--;
		table[entries].key	= sw;
		table[entries].x	= x;
		table[entries].y	= y;
		table[entries].w	= w;
		table[entries].h	= h;
		entries++;

		return(False);
	}
} /* end of ForDnDCache */
