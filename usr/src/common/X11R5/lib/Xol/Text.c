/*		copyright	"%c%" 	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)oldtext:Text.c	1.57"
#endif

/*************************************************************************
 *
 * Description:	This file along with Text.h and TextP.h implements
 *		the OPEN LOOK Text Widget
 *
 *******************************file*header******************************/


/*****************************************************************************
 *									     *
 *		Copyright (c) 1988 by Hewlett-Packard Company		     *
 *     Copyright (c) 1988 by the Massachusetts Institute of Technology	     *
 *									     *
 *****************************************************************************/

						/* #includes go here	*/

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/Primitive.h>
#include <Xol/TextP.h>
#include <Xol/TextPane.h>

/*************************************************************************
 *
 * Foward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations**************************/

					/* private procedures		*/

static Boolean	CheckWidth();
static Boolean	CheckHeight();
static void	ConditionalUpdate();
static void	DoLayout();
static Boolean	LayoutHSB();
static Boolean	LayoutVSB();

					/* class procedures		*/

static Boolean		AcceptFocus OL_ARGS((Widget, Time *));
static void		ClassInitialize();
static void		Destroy();
static XtGeometryResult	GeometryManager();
static void		GetValuesHook();
static void		HighlightHandler OL_ARGS((Widget, OlDefine));
static void		Initialize();
static void		InitializeHook();
static void		InsertChild();
static void		Realize();
static Widget		RegisterFocus();
static void		Resize();
static Boolean		SetValues();
static Boolean		SetValuesHook();

					/* action procedures		*/

static void	KidKilled();
static void	HSBMoved();
static void	VSBMoved();

					/* public procedures		*/
/* none */

/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables*****************************/

#define SW_BB_BORDER_WIDTH	(OlPointToPixel(OL_HORIZONTAL, 1))
#define SW_HSB_GAP		(OlPointToPixel(OL_VERTICAL, 1))
#define SW_MINIMUM_HEIGHT	200
#define SW_MINIMUM_WIDTH	200
#define SW_SB_BORDER_WIDTH	0
#define SW_SB_GRANULARITY	1
#define SW_SB_SLIDER_MAX	0
#define SW_SB_SLIDER_MIN	0
#define SW_SB_SLIDER_VALUE	0
#define SW_VSB_GAP		(OlPointToPixel(OL_HORIZONTAL, 1))
#define _SW_POSITION		1
#define _SW_DIMENSION		2


typedef struct
{
  Position	x, y;
  Dimension	width, height;
  Dimension	real_width, real_height;
  Dimension	border_width;
}	_SWGeometryInfo;


static XtCallbackRec hsb_moved[] =
{
  {HSBMoved, (XtPointer)NULL},
  {NULL, (XtPointer)NULL}
};

static XtCallbackRec hsb_paged[] =
{
  {HSBMoved, (XtPointer)NULL},
  {NULL, (XtPointer)NULL}
};

static XtCallbackRec vsb_moved[] =
{
  {VSBMoved, (XtPointer)NULL},
  {NULL, (XtPointer)NULL}
};

static XtCallbackRec vsb_paged[] =
{
  {VSBMoved, (XtPointer)NULL},
  {NULL, (XtPointer)NULL}
};

/*************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************/


/*************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources****************************/

static XtResource resources[] = 
{
					/* core resources */
  { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffset(TextWidget, core.border_width), XtRImmediate, 0
  },

					/* text resources */
#define OFFSET(field)	XtOffsetOf(TextRec, text.field)

  { XtNalignHorizontal, XtCAlignHorizontal, XtRInt, sizeof(int),
    OFFSET(align_horizontal), XtRImmediate, (XtPointer)OL_BOTTOM
  },
  { XtNalignVertical, XtCAlignVertical, XtRInt, sizeof(int),
    OFFSET(align_vertical), XtRImmediate, (XtPointer)OL_RIGHT
  },
  { XtNcurrentPage, XtCCurrentPage, XtRInt, sizeof(int),
    OFFSET(current_page), XtRImmediate, (XtPointer)1
  },
  { XtNhorizontalSB, XtCHorizontalSB, XtRBoolean, sizeof(Boolean),
    OFFSET(force_hsb), XtRImmediate, False
  },
  { XtNverticalSB, XtCVerticalSB, XtRBoolean, sizeof(Boolean),
    OFFSET(force_vsb), XtRImmediate, False
  },
  { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(foreground), XtRString, "Black"
  },
  { XtNhStepSize, XtCHStepSize, XtRInt, sizeof(int),
    OFFSET(h_granularity), XtRImmediate, (XtPointer)1
  },
  { XtNhInitialDelay, XtCHInitialDelay, XtRInt, sizeof(int),
    OFFSET(h_initial_delay), XtRImmediate, (XtPointer)500
  },
  { XtNhMenuPane, XtCHMenuPane, XtRWidget, sizeof(Widget),
    OFFSET(h_menu_pane), XtRPointer, (XtPointer)NULL
  },
  { XtNhRepeatRate, XtCHRepeatRate, XtRInt, sizeof(int),
    OFFSET(h_repeat_rate), XtRImmediate, (XtPointer)100
  },
  {  XtNhSliderMoved, XtCHSliderMoved, XtRCallback, sizeof(XtPointer),
    OFFSET(h_slider_moved), XtRCallback, (XtPointer)NULL
  },
  { XtNinitialX, XtCInitialX, XtRInt, sizeof(int),
    OFFSET(init_x), XtRImmediate, 0
  },
  { XtNinitialY, XtCInitialY, XtRInt, sizeof(int),
    OFFSET(init_y), XtRImmediate, 0
  },
  { XtNrecomputeHeight, XtCRecomputeHeight, XtRBoolean,sizeof(Boolean),
    OFFSET(recompute_view_height), XtRImmediate, False
  },
  { XtNrecomputeWidth, XtCRecomputeWidth, XtRBoolean, sizeof(Boolean),
    OFFSET(recompute_view_width), XtRImmediate, False
  },
  { XtNshowPage, XtCShowPage, XtROlDefine, sizeof(OlDefine),
    OFFSET(show_page), XtRImmediate, (XtPointer)OL_NONE
  },
  { XtNvStepSize, XtCVStepSize, XtRInt, sizeof(int),
    OFFSET(v_granularity), XtRImmediate, (XtPointer)1
  },
  { XtNvInitialDelay, XtCVInitialDelay, XtRInt, sizeof(int),
    OFFSET(v_initial_delay), XtRImmediate, (XtPointer)500
  },
  { XtNvMenuPane, XtCVMenuPane, XtRWidget, sizeof(Widget),
    OFFSET(v_menu_pane), XtRWidget, NULL
  },
  { XtNvRepeatRate, XtCVRepeatRate, XtRInt, sizeof(int),
    OFFSET(v_repeat_rate), XtRImmediate, (XtPointer)100
  },
  { XtNvSliderMoved, XtCVSliderMoved, XtRCallback, sizeof(XtPointer),
    OFFSET(v_slider_moved), XtRCallback, (XtPointer)NULL
  },
  { XtNviewHeight, XtCViewHeight, XtRInt, sizeof(int),
    OFFSET(view_height), XtRImmediate, 0
  },
  { XtNviewWidth, XtCViewWidth, XtRInt, sizeof(int),
    OFFSET(view_width), XtRImmediate, 0
  },
  { XtNmark, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(mark), XtRCallback, (XtPointer) NULL
  },
  { XtNmotionVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(motion_verification), XtRCallback, (XtPointer) NULL
  },

  { XtNmodifyVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(modify_verification), XtRCallback, (XtPointer) NULL
  },

  { XtNleaveVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(leave_verification), XtRCallback, (XtPointer) NULL
  },
};
#undef OFFSET

/*************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record**************************/

TextClassRec textClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass)&managerClassRec,
    /* class_name         */    "Text",
    /* widget_size        */    sizeof(TextRec),
    /* class_initialize   */    ClassInitialize,
    /* class_partinit     */    NULL,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* Init hook	  */    (XtArgsProc)InitializeHook,
    /* realize            */    Realize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    Resize,
    /* expose             */    NULL,
    /* set_values         */    SetValues,
    /* set values hook    */    SetValuesHook,
    /* set values almost  */    XtInheritSetValuesAlmost,
    /* get values hook    */    GetValuesHook,
    /* accept_focus       */    AcceptFocus,
    /* Version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* TM Table           */    NULL,
    /* query_geom         */    NULL,
  },
  {
/* composite_class fields */
    /* geometry_manager   */    (XtGeometryHandler)GeometryManager,
    /* change_managed     */    NULL,
    /* insert_child	  */	InsertChild,
    /* delete_child	  */	NULL,
#if XtVersion < 11003
    /* move_focus_to_next */    NULL,
    /* move_focus_to_prev */    NULL
#else
    /* extension	  */	NULL
#endif
  },
  {
/* constraint_class fields */
    /* resources	  */	(XtResourceList)NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	(XtInitProc)NULL,
    /* destroy		  */	(XtWidgetProc)NULL,
    /* set_values	  */	(XtSetValuesFunc)NULL
  },
  {
/* manager_class fields   */
    /* highlight_handler  */	HighlightHandler,
    /* focus_on_select	  */	True,
    /* traversal_handler  */    NULL,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */	RegisterFocus,
    /* version		  */	OlVersion,
    /* extension	  */	NULL
  },	/* End of ManagerClass field initializations */
  {
/* text_class - none */     
   /* empty */			0
  }	
};

/*************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition************************/

WidgetClass textWidgetClass = (WidgetClass)&textClassRec;

/*************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures***************************/

static void
ConditionalUpdate(w, w_info)
  Widget		w;
  _SWGeometryInfo	*w_info;
{
  int	different_position;
  int	different_size;

  different_position = (w->core.x != w_info->x || w->core.y != w_info->y);
  different_size = (w->core.width != w_info->width ||
		    w->core.height != w_info->height);

  if (different_position && different_size)
    XtConfigureWidget(w, w_info->x, w_info->y, w_info->width, w_info->height,
		      w_info->border_width / 2);
  else
    if (different_position)
      XtMoveWidget(w, w_info->x, w_info->y);
    else
      if (different_size)
	XtResizeWidget(w, w_info->width, w_info->height,
		       w_info->border_width / 2);

} /* ConditionalUpdate() */


#define _SWGetGeomInfo(w, gi) \
((void)( \
  (gi).x = (w)->core.x, \
  (gi).y = (w)->core.y, \
  (gi).width = (w)->core.width, \
  (gi).height = (w)->core.height, \
  (gi).border_width = 2 * (w)->core.border_width, \
  (gi).real_height = (w)->core.height + (gi).border_width, \
  (gi).real_width = (w)->core.width + (gi).border_width \
))

#define _SWMIN(x, y)	((x) < (y) ? (x) : (y))

#define _SWSetInfoHeight(gi, h) \
((void)( \
  (gi).height = (h), \
  (gi).real_height = (gi).height + (gi).border_width \
))

#define _SWSetInfoRealHeight(gi, h) \
((void)( \
  (gi).real_height = (h), \
  (gi).height = (gi).real_height - (gi).border_width \
))

#define _SWSetInfoWidth(gi, w) \
((void)( \
  (gi).width = (w), \
  (gi).real_width = (gi).width + (gi).border_width \
))

#define _SWSetInfoRealWidth(gi, w) \
((void)( \
  (gi).real_width = (w), \
  (gi).width = (gi).real_width - (gi).border_width \
))


static Boolean
CheckHeight(sw, sw_info, bb_info, bbc_info, vsb, vsb_info)
  TextWidget	sw;
  _SWGeometryInfo	*sw_info;
  _SWGeometryInfo	*bb_info;
  _SWGeometryInfo	*bbc_info;
  ScrollbarWidget	vsb;
  _SWGeometryInfo	*vsb_info;
{
  Dimension		extra_space;
  XtWidgetGeometry	intended;
  XtWidgetGeometry	preferred;


  if (sw->text.force_vsb)
    {
      intended.request_mode = CWHeight;
      intended.height = bb_info->real_height - vsb_info->border_width;
      switch(XtQueryGeometry((Widget)vsb, &intended, &preferred))
	{
	case XtGeometryYes:
	  break;
	case XtGeometryAlmost:
        case XtGeometryNo:
	  if(preferred.height > intended.height)
	    for (--intended.height; intended.height > 0; --intended.height)
	      if (XtQueryGeometry((Widget)vsb, &intended, &preferred) == XtGeometryYes
		  || preferred.height <= intended.height)
		break;
	  break;
	}
      _SWSetInfoRealHeight(*bb_info,
			   preferred.height + vsb_info->border_width);

      return TRUE;
    }

  return FALSE;
} /* CheckHeight() */


static Boolean
CheckWidth(sw, sw_info, bb_info, bbc_info, hsb, hsb_info)
  TextWidget	sw;
  _SWGeometryInfo	*sw_info;
  _SWGeometryInfo	*bb_info;
  _SWGeometryInfo	*bbc_info;
  ScrollbarWidget	hsb;
  _SWGeometryInfo	*hsb_info;
{
  XtWidgetGeometry	intended;
  XtWidgetGeometry	preferred;


  if (sw->text.force_hsb)
    {
      intended.request_mode = CWWidth;
      intended.width = bb_info->real_width - hsb_info->border_width;
      switch(XtQueryGeometry((Widget)hsb, &intended, &preferred))
	{
	case XtGeometryYes:
	  break;
	case XtGeometryAlmost:
	case XtGeometryNo:
	  if (preferred.width > intended.width)
	    for (--intended.width; intended.width > 0; --intended.width)
	      if (XtQueryGeometry((Widget)hsb, &intended, &preferred) == XtGeometryYes
		  || preferred.width <= intended.width)
		break;
	  break;
	}
      _SWSetInfoRealWidth(*bb_info,
			  preferred.width + hsb_info->border_width);

      return TRUE;
    }
  else
    return FALSE;

} /* CheckWidth() */


static void
DoLayout(sw)
  TextWidget	sw;
{
  BulletinBoardWidget	bb;
  Dimension		bb_width;
  _SWGeometryInfo	bb_info;
  Widget		bbc;
  _SWGeometryInfo	bbc_info;
  Position		h_gap;
  ScrollbarWidget	hsb;
  Arg			hsb_arg[10];
  _SWGeometryInfo	hsb_info;
  Dimension		min_width;
  Dimension		min_height;
  int			n;
  Dimension		pref_height;
  Dimension		pref_width;
  int			slider_max;
  _SWGeometryInfo	sw_info;
  Position		v_gap;
  ScrollbarWidget	vsb;
  Arg			vsb_arg[10];
  _SWGeometryInfo	vsb_info;


  bb = sw->text.bboard;
  bbc = sw->text.bb_child;
  hsb = sw->text.hsb;
  vsb = sw->text.vsb;

  _SWGetGeomInfo((Widget)bb, bb_info);
  _SWGetGeomInfo((Widget)hsb, hsb_info);
  _SWGetGeomInfo((Widget)sw, sw_info);
  _SWGetGeomInfo((Widget)vsb, vsb_info);

  if (sw->text.view_width)
    min_width = sw->text.view_width;
  else
    min_width = sw_info.width - bb_info.border_width;

  if (sw->text.view_height)
    min_height = sw->text.view_height;
  else
    min_height = sw_info.height - bb_info.border_width;

  if (bbc == (Widget)NULL)
    bbc_info.x = bbc_info.y = bbc_info.width = bbc_info.height =
      bbc_info.real_width = bbc_info.real_height = bbc_info.border_width = 0;
  else
    {
      _SWGetGeomInfo(bbc, bbc_info);
      if (sw->text.recompute_view_width)
	min_width = _SWMIN(bbc_info.real_width, min_width);
      if (sw->text.recompute_view_height)
	min_height = _SWMIN(bbc_info.real_height, min_height);
    }

  _SWSetInfoWidth(bb_info, min_width);
  _SWSetInfoHeight(bb_info, min_height);

  if (sw->text.view_width || sw->text.view_height)
    {
      if (sw->text.view_width)
	_SWSetInfoWidth(sw_info,
			bb_info.real_width + vsb_info.real_width +
			(Dimension)SW_VSB_GAP);
      if (sw->text.view_height)
	_SWSetInfoHeight(sw_info,
			 bb_info.real_height + hsb_info.real_height +
			 (Dimension)SW_HSB_GAP);
      if (XtMakeResizeRequest((Widget)sw, sw_info.width, sw_info.height,
			      &pref_width, &pref_height)
	  == XtGeometryAlmost)
	{
	  _SWSetInfoWidth(sw_info, pref_width);
	  _SWSetInfoHeight(sw_info, pref_height);
	  (void)XtMakeResizeRequest((Widget)sw, sw_info.width, sw_info.height,
				    (Dimension *)NULL, (Dimension *)NULL);
	}
    }

  (void)LayoutHSB(sw, &sw_info, &bb_info, &bbc_info, hsb, &hsb_info);
  if(LayoutVSB(sw, &sw_info, &bb_info, &bbc_info, vsb, &vsb_info))
    if (LayoutHSB(sw, &sw_info, &bb_info, &bbc_info, hsb, &hsb_info))
      (void)CheckHeight(sw, &sw_info, &bb_info, &bbc_info, vsb, &vsb_info);

  if (sw->text.align_horizontal == OL_BOTTOM)
    {
      bb_info.y = 0;
      hsb_info.y = bb_info.real_height + (Dimension)SW_HSB_GAP;
    }
  else
    {
      hsb_info.y = 0;
      bb_info.y = hsb_info.real_height + (Dimension)SW_HSB_GAP;
    }

  if (sw->text.align_vertical == OL_RIGHT)
    {
      bb_info.x = 0;
      vsb_info.x = bb_info.real_width + (Dimension)SW_VSB_GAP;
    }
  else
    {
      vsb_info.x = 0;
      bb_info.x = vsb_info.real_width + (Dimension)SW_VSB_GAP;
    }

  vsb_info.y = bb_info.y;
  _SWSetInfoRealHeight(vsb_info, bb_info.real_height);

  hsb_info.x = bb_info.x;
  _SWSetInfoRealWidth(hsb_info, bb_info.real_width);

  ConditionalUpdate(bb, &bb_info);
  ConditionalUpdate(hsb, &hsb_info);
  ConditionalUpdate(vsb, &vsb_info);

  if (XtIsRealized((Widget)hsb))
    if (sw->text.force_hsb)
      XtMapWidget((Widget)hsb);
    else
      XtUnmapWidget((Widget)hsb);

  if (XtIsRealized((Widget)vsb))
    if (sw->text.force_vsb)
      XtMapWidget((Widget)vsb);
    else
      XtUnmapWidget((Widget)vsb);

  if (bbc != (Widget)NULL)
    {
      if ((h_gap = bb_info.width - (bbc_info.x + bbc_info.real_width)) > 0)
	bbc_info.x = _SWMIN(bbc_info.x + h_gap, 0);
      if ((v_gap = bb_info.height - (bbc_info.y + bbc_info.real_height)) > 0)
	bbc_info.y = _SWMIN(bbc_info.y + v_gap, 0);

      _SWSetInfoRealWidth(bbc_info, bb_info.width);
      _SWSetInfoRealHeight(bbc_info, bb_info.height);

      ConditionalUpdate(bbc, &bbc_info);
    }

#if 0
  slider_max = bbc_info.real_width - bb_info.width;
  n = 0;
  XtSetArg(hsb_arg[n], XtNproportionLength, bb_info.width);		n++;
  XtSetArg(hsb_arg[n], XtNsliderMax, slider_max);			n++;
  XtSetArg(hsb_arg[n], XtNsliderValue, -bbc_info.x);			n++;
  XtSetValues(hsb, hsb_arg, n);

  if (slider_max > 0)
    {
      if (!XtIsSensitive((Widget)hsb))
	XtSetSensitive((Widget)hsb, TRUE);
    }
  else
    if (XtIsSensitive((Widget)hsb))
      XtSetSensitive((Widget)hsb, FALSE);

  slider_max = bbc_info.real_height - bb_info.height;
  n = 0;
  XtSetArg(vsb_arg[n], XtNproportionLength, bb_info.height);		n++;
  XtSetArg(vsb_arg[n], XtNsliderMax, slider_max);			n++;
  XtSetArg(vsb_arg[n], XtNsliderValue, -bbc_info.y);			n++;
  XtSetValues(vsb, vsb_arg, n);

  if (slider_max > 0)
    {
      if (!XtIsSensitive((Widget)vsb))
	XtSetSensitive((Widget)vsb, TRUE);
    }
  else
    if (XtIsSensitive((Widget)vsb))
      XtSetSensitive((Widget)vsb, FALSE);
#endif
} /* DoLayout() */


static Boolean
LayoutHSB(sw, sw_info, bb_info, bbc_info, hsb, hsb_info)
  TextWidget	sw;
  _SWGeometryInfo	*sw_info;
  _SWGeometryInfo	*bb_info;
  _SWGeometryInfo	*bbc_info;
  ScrollbarWidget	hsb;
  _SWGeometryInfo	*hsb_info;
{
  Dimension	extra_space;


  if (CheckWidth(sw, sw_info, bb_info, bbc_info, hsb, hsb_info))
    {
      extra_space = hsb_info->real_height + (Dimension)SW_HSB_GAP;
      if ((Dimension)(sw_info->height - bb_info->real_height) < extra_space)
	{
	  _SWSetInfoRealHeight(*bb_info, bb_info->real_height - extra_space);
	  return TRUE;
	}
    }
  return FALSE;
} /* LayoutHSB() */


static Boolean
LayoutVSB(sw, sw_info, bb_info, bbc_info, vsb, vsb_info)
  TextWidget	sw;
  _SWGeometryInfo	*sw_info;
  _SWGeometryInfo	*bb_info;
  _SWGeometryInfo	*bbc_info;
  ScrollbarWidget	vsb;
  _SWGeometryInfo	*vsb_info;
{
  Dimension	extra_space;


  if (CheckHeight(sw, sw_info, bb_info, bbc_info, vsb, vsb_info))
    {
      extra_space = vsb_info->real_width + (Dimension)SW_VSB_GAP;
      if ((Dimension)(sw_info->width - bb_info->real_width) < extra_space)
	{
	  _SWSetInfoRealWidth(*bb_info, bb_info->real_width - extra_space);
	  return TRUE;
	}
    }
  return FALSE;
} /* LayoutVSB() */


/*************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures****************************/

/*
 * AcceptFocus - set focus to our TextPane child
 */
static Boolean
AcceptFocus OLARGLIST((w, timestamp))
	OLARG( Widget,	w)		/* Text Widget	*/
	OLGRA( Time *,	timestamp)
{
	return(XtCallAcceptFocus(((TextWidget)w)->text.bb_child, timestamp));
} /* END OF AcceptFocus() */

static void
ClassInitialize()
{
  TextWidgetClass	myclass;
  CompositeWidgetClass		superclass;

  myclass = (TextWidgetClass)textWidgetClass;
  superclass = (CompositeWidgetClass)managerWidgetClass;

  myclass->composite_class.delete_child =
    superclass->composite_class.delete_child;
  myclass->composite_class.change_managed =
    superclass->composite_class.change_managed;

	_OlAddOlDefineType ("grow_off",        OL_GROW_OFF);
	_OlAddOlDefineType ("grow_horizontal", OL_GROW_HORIZONTAL);
	_OlAddOlDefineType ("grow_vertical",   OL_GROW_VERTICAL);
	_OlAddOlDefineType ("grow_both",       OL_GROW_BOTH);

} /* ClassInitialize() */


static void
Destroy(w)
  Widget	w;
{

  XtRemoveAllCallbacks(w, XtNleaveVerification);
  XtRemoveAllCallbacks(w, XtNmodifyVerification);
  XtRemoveAllCallbacks(w, XtNmotionVerification);
  XtRemoveAllCallbacks(w, XtNmark);

} /* Destroy() */


static XtGeometryResult
GeometryManager(w, request, preferred_return)
  Widget		w;
  XtWidgetGeometry	*request;
  XtWidgetGeometry	*preferred_return;
{

  DoLayout((TextWidget)w->core.parent);
  return XtGeometryYes;

} /* GeometryManager() */


static void
GetValuesHook(w, args, num_args)
  Widget	w;
  ArgList	args;
  Cardinal	*num_args;
{
  ArgList		composed_args;
  Cardinal		composed_num_args;
  static MaskArg	mask_args[] =
    {
      {XtNbottomMargin, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNcurrentPage, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNdisplayPosition, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNeditType, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNfile, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNfontColor, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNgrow, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNhorizontalSB, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNcursorPosition, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNleftMargin, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNmaximumSize, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNrightMargin, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNshowPage, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNsourceType, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNstring, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextClearBuffer, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextCopyBuffer, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextGetInsertPoint, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextGetLastPos,  (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextInsert, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextReadSubStr, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextRedraw, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextReplace, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextSetInsertPoint, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtextUpdate, (XtArgVal)NULL, OL_SOURCE_PAIR},
      {XtNtraversalOn, (XtArgVal)NULL, OL_SOURCE_PAIR}
    };


  _OlComposeArgList(args, *num_args, mask_args, XtNumber(mask_args),
		    &composed_args, &composed_num_args);

  if (composed_num_args > (Cardinal)0)
    {
      XtGetValues(((TextWidget)w)->text.bb_child, composed_args,
		  composed_num_args);
      free(composed_args);
    }

} /* GetValuesHook() */


static void
#if OlNeedFunctionPrototypes
HighlightHandler(
	Widget		w,
	OlDefine	type)
#else
HighlightHandler(w, type)
	Widget		w;
	OlDefine	type;
#endif
{
	_OlCallHighlightHandler(((TextWidget)w)->text.bb_child, type);
} /* END OF HighlightHandler() */

/* ARGSUSED */
static void
Initialize(request, new, arg, num_args)
  Widget	request, new;
  ArgList	arg;
  Cardinal	num_args;
{
  BulletinBoardWidget	bb;
  Arg			bb_arg[10];
  Arg			hsb_arg[20];
  int			n;
  TextWidget	nsw;
  Arg			vsb_arg[20];


  nsw = (TextWidget)new;

  nsw->text.in_init = TRUE;

  if (nsw->core.width == 0)
    nsw->core.width = SW_MINIMUM_WIDTH;
  if (nsw->core.height == 0)
    nsw->core.height = SW_MINIMUM_HEIGHT;

  if (nsw->text.init_x > 0)
    {
		  OlVaDisplayWarningMsg(XtDisplay(new),
					OleNfileText,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileText_msg1,
					"XtNinitialX");
      nsw->text.init_x = 0;
    }

  if (nsw->text.init_y > 0)
    {
		  OlVaDisplayWarningMsg(XtDisplay(new),
					OleNfileText,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileText_msg1,
					"XtNinitialY");
      nsw->text.init_y = 0;
    }

  nsw->core.background_pixel = nsw->core.parent->core.background_pixel;
  nsw->core.background_pixmap = nsw->core.parent->core.background_pixmap;

  n = 0;
  XtSetArg(bb_arg[n], XtNbackground,
		nsw->core.parent->core.background_pixel);		n++;
  XtSetArg(bb_arg[n], XtNbackgroundPixmap,
		nsw->core.parent->core.background_pixmap);		n++;
  XtSetArg(bb_arg[n], XtNborderColor, nsw->text.foreground);		n++;
  XtSetArg(bb_arg[n], XtNborderWidth, SW_BB_BORDER_WIDTH);		n++;
  XtSetArg(bb_arg[n], (char *)XtNheight,
	   (XtArgVal)nsw->core.height - 2 * SW_BB_BORDER_WIDTH);			n++;
  XtSetArg(bb_arg[n], XtNlayout, OL_MINIMIZE);				n++;
  XtSetArg(bb_arg[n], XtNwidth,
	   (XtArgVal)nsw->core.width - 2 * SW_BB_BORDER_WIDTH);			n++;
  XtSetArg(bb_arg[n], XtNshadowThickness, 0);				n++;
  nsw->text.bboard =
    (BulletinBoardWidget)XtCreateWidget("BulletinBoard",
					bulletinBoardWidgetClass,
					(Widget)nsw,
					bb_arg, n);
  bb = nsw->text.bboard;

  n = 0;
  XtSetArg(hsb_arg[n], XtNborderColor,
	   nsw->text.foreground);					n++;
  XtSetArg(hsb_arg[n], XtNborderWidth, SW_SB_BORDER_WIDTH);		n++;
  XtSetArg(hsb_arg[n], XtNforeground, nsw->text.foreground);		n++;
  XtSetArg(hsb_arg[n], XtNgranularity,
	   nsw->text.h_granularity);					n++;
  XtSetArg(hsb_arg[n], XtNinitialDelay,
	   nsw->text.h_initial_delay);					n++;
  XtSetArg(hsb_arg[n], XtNmenuPane, nsw->text.h_menu_pane);		n++;
  XtSetArg(hsb_arg[n], XtNorientation, OL_HORIZONTAL);			n++;
  XtSetArg(hsb_arg[n], XtNrepeatRate,
	   nsw->text.h_repeat_rate);					n++;
  XtSetArg(hsb_arg[n], XtNsliderMoved, hsb_moved);			n++;
  XtSetArg(hsb_arg[n], XtNsliderMin, SW_SB_SLIDER_MIN);			n++;
  XtSetArg(hsb_arg[n], XtNsliderValue, SW_SB_SLIDER_VALUE);		n++;
  XtSetArg(hsb_arg[n], XtNwidth,
	   bb->core.width + 2 * bb->core.border_width);			n++;
  nsw->text.hsb =
    (ScrollbarWidget)XtCreateManagedWidget("Scrollbar",
					   scrollbarWidgetClass,
					   (Widget)nsw,
					   hsb_arg, n);

  n = 0;
  XtSetArg(vsb_arg[n], XtNborderColor,
           nsw->text.foreground);					n++;
  XtSetArg(vsb_arg[n], XtNborderWidth, SW_SB_BORDER_WIDTH);		n++;
  XtSetArg(vsb_arg[n], XtNcurrentPage,
	   nsw->text.current_page);					n++;
  XtSetArg(vsb_arg[n], XtNforeground, nsw->text.foreground);		n++;
  XtSetArg(vsb_arg[n], XtNgranularity,
	   nsw->text.v_granularity);					n++;
  XtSetArg(vsb_arg[n], XtNinitialDelay,
	   nsw->text.v_initial_delay);					n++;
  XtSetArg(vsb_arg[n], XtNheight,
	   bb->core.height + 2 * bb->core.border_width);		n++;
  XtSetArg(vsb_arg[n], XtNmenuPane, nsw->text.v_menu_pane);		n++;
  XtSetArg(vsb_arg[n], XtNorientation, OL_VERTICAL);			n++;
  XtSetArg(vsb_arg[n], XtNrepeatRate,
	   nsw->text.v_repeat_rate);					n++;
  XtSetArg(vsb_arg[n], XtNshowPage, nsw->text.show_page);		n++;
  XtSetArg(vsb_arg[n], XtNsliderMoved, vsb_moved);			n++;
  XtSetArg(vsb_arg[n], XtNsliderMin, SW_SB_SLIDER_MIN);			n++;
  XtSetArg(vsb_arg[n], XtNsliderValue, SW_SB_SLIDER_VALUE);		n++;
  nsw->text.vsb =
    (ScrollbarWidget)XtCreateManagedWidget("Scrollbar",
					scrollbarWidgetClass,
					(Widget)nsw,
					vsb_arg, n);

  nsw->core.background_pixel = nsw->core.parent->core.background_pixel;
  nsw->core.background_pixmap = nsw->core.parent->core.background_pixmap;
  nsw->core.border_pixel = nsw->text.foreground;
  nsw->text.bb_child = NULL;

  nsw->text.in_init = FALSE;
} /* Initialize() */


static void
InitializeHook(w, args, num_args)
    Widget	w;
    ArgList	args;
    Cardinal *	num_args;
{
    TextWidget	sw = (TextWidget) w;
    Widget	pane;
    Cardinal	n;
    Arg		text_arg[10];
    ArgList	merged_args;

    n = 0;
    if (sw->text.force_hsb)  {
	XtSetArg(text_arg[n], XtNgrow, OL_GROW_HORIZONTAL);	n++;
	XtSetArg(text_arg[n], XtNwrap, FALSE);			n++;
    }
    XtSetArg(text_arg[n], XtNborderWidth, 0);			n++;
    XtSetArg(text_arg[n], XtNheight,
	     sw->text.bboard->core.height);			n++;
    XtSetArg(text_arg[n], XtNwidth,
	     sw->text.bboard->core.width);			n++;
    if (sw->text.force_vsb) {
	XtSetArg(text_arg[n], XtNverticalSBWidget, sw->text.vsb); n++;
    }

    merged_args = XtMergeArgLists(args, *num_args, text_arg, n);

    pane = XtCreateManagedWidget("TextPane", textPaneWidgetClass,
					w, merged_args, n + *num_args);
    XFree((char *)merged_args);

				/* Associate the scrollbars with
				 * the pane and the pane with the
				 * parent textWidget.
				 */
    OlAssociateWidget(pane, (Widget) sw->text.hsb, TRUE);
    OlAssociateWidget(pane, (Widget) sw->text.vsb, TRUE);
    OlAssociateWidget(w, pane, TRUE);

    _OlDeleteDescendant(pane);	/* delete pane from traversal list */
				/*  and add myself instead */
    _OlUpdateTraversalWidget(w, sw->manager.reference_name,
				sw->manager.reference_widget, True);

#if 0
    if (sw->text.bb_child->core.width != 0 && !sw->text.force_hsb)
	sw->text.view_width = sw->text.bb_child->core.width;
    if (sw->text.bb_child->core.height != 0 && !sw->text.force_vsb)
	sw->text.view_height = sw->text.bb_child->core.height;
#endif
}				/*  InitializeHook  */


static void
InsertChild(w)
    Widget w;
{
    TextWidget		sw = (TextWidget)XtParent(w);
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(textClassRec.core_class.superclass))->composite_class.insert_child;
  
    if (w == (Widget)sw->text.hsb ||
	w == (Widget)sw->text.vsb ||
	w == (Widget)sw->text.bboard ||
	sw->text.in_init == TRUE) {
	if (insert_child)
	    (*insert_child)(w);
	XtManageChild(w);
    } else {
	if (sw->text.bb_child == NULL ||
	    sw->text.bb_child->core.being_destroyed == TRUE) {
	    Arg child_arg[2];
	    int n;
	    sw->text.bb_child = w;
	    XtParent(w) = (Widget)sw->text.bboard;
	    XtAddCallback(w, XtNdestroyCallback, KidKilled, (XtPointer)NULL);

	    n = 0;
	    XtSetArg(child_arg[n], XtNx, sw->text.init_x); n++;
	    XtSetArg(child_arg[n], XtNy, sw->text.init_y); n++;
	    XtSetValues(w, child_arg, n);

	    if (insert_child)
		(*insert_child)(w);
#if 0
	    if (sw->text.init_x || sw->text.init_y)
		XtMoveWidget(w, sw->text.init_x, sw->text.init_y);
#endif
	    DoLayout(sw);
	} else
		  OlVaDisplayErrorMsg(XtDisplay(w),
					OleNfileText,
					OleTmsg2,
					OleCOlToolkitError,
					OleMfileText_msg2);
    }

} /* InsertChild() */


static void
Realize(w, p_value_mask, attributes)
  Widget		w;
  Mask			*p_value_mask;
  XSetWindowAttributes 	*attributes;
{
  TextWidget	sw;
  Mask			value_mask;


  sw = (TextWidget)w;
  value_mask = *p_value_mask;

  value_mask |= CWBitGravity;
  attributes->bit_gravity = NorthWestGravity;

  DoLayout(sw);

  XtCreateWindow((Widget)sw, InputOutput, (Visual *)CopyFromParent,
		 value_mask, attributes);
} /* Realize() */


/******************************function*header*************************
 * RegisterFocus - return widget id to register on Shell
 */
static Widget
RegisterFocus(w)
  Widget	w;
{
    return (w);		/* return self */
} /* RegisterFocus() */


static void
Resize(w)
  Widget	w;
{
  
  DoLayout((TextWidget)w);
} /* Resize() */


/* ARGSUSED */
static Boolean
SetValues (current, request, new, args, num_args)
  Widget	current, request, new;
  ArgList		args;
  Cardinal *	num_args;
{
  int			b;
  Arg			bb_arg[10];
  TextWidget		cur_sw;
  TextPart		cur_sw_part;
  Boolean		do_layout;
  Boolean		flag;
  int 			h;
  Arg			hsb_arg[20];
  TextWidget		new_sw;
  TextPart   		new_sw_part;
  int			slider_max;
  int			v;
  Arg			vsb_arg[20];


  flag = FALSE;
  do_layout = FALSE;

  b = 0;
  h = 0;
  v = 0;

  cur_sw = (TextWidget)current;
  cur_sw_part = cur_sw->text;
  new_sw = (TextWidget)new;
  new_sw_part = new_sw->text;

  if (new_sw_part.align_horizontal != cur_sw_part.align_horizontal ||
      new_sw_part.align_vertical != cur_sw_part.align_vertical ||
      new_sw_part.force_hsb != cur_sw_part.force_hsb ||
      new_sw_part.force_vsb != cur_sw_part.force_vsb ||
      new_sw_part.recompute_view_height != cur_sw_part.recompute_view_height ||
      new_sw_part.recompute_view_width != cur_sw_part.recompute_view_width ||
      new_sw_part.view_height != cur_sw_part.view_height ||
      new_sw_part.view_width != cur_sw_part.view_width)
    do_layout = TRUE;

  if (new_sw_part.current_page != cur_sw_part.current_page)
    {
      XtSetArg(vsb_arg[v], XtNcurrentPage, new_sw_part.current_page);	v++;
    }

  if (new_sw_part.foreground != cur_sw_part.foreground)
    {
      XtSetArg(bb_arg[b], XtNborderColor, new_sw_part.foreground);	b++;
      XtSetArg(hsb_arg[h], XtNborderColor,
	       new_sw_part.foreground);					h++;
      XtSetArg(hsb_arg[h], XtNforeground, new_sw_part.foreground);	h++;
      XtSetArg(vsb_arg[v], XtNborderColor,
	       new_sw_part.foreground);					v++;
      XtSetArg(vsb_arg[v], XtNforeground, new_sw_part.foreground);	v++;
    }

#if 0
	if (new_sw->core.background_pixel != cur_sw->core.background_pixel)  {
    		Mask    window_mask = 0;
    		XSetWindowAttributes attributes;

  		XtSetArg(bb_arg[b], XtNbackground,
			cur_sw->core.background_pixel);			b++;
		XtSetValues(new_sw->text.bboard, bb_arg, b);
    		if (XtIsRealized(cur_sw)) {
	   		window_mask |= CWBackPixel;
	    		attributes.background_pixmap =
				cur_sw->core.background_pixel;
	    		XChangeWindowAttributes(XtDisplay(new_sw),
				XtWindow(new_sw), window_mask, &attributes);
			}
		else
			new_sw->core.background_pixel = 
				new_sw->core.background_pixel;
		}
#endif

  if (new_sw_part.h_granularity != cur_sw_part.h_granularity)
    {
      XtSetArg(hsb_arg[h], XtNgranularity, new_sw_part.h_granularity);	h++;
    }

  if (new_sw_part.h_initial_delay != cur_sw_part.h_initial_delay)
    {
      XtSetArg(hsb_arg[h], XtNinitialDelay,
	       new_sw_part.h_initial_delay);				h++;
    }

  if (new_sw_part.h_menu_pane != cur_sw_part.h_menu_pane)
    {
      XtSetArg(hsb_arg[h], XtNmenuPane, new_sw_part.h_menu_pane);	h++;
    }

  if (new_sw_part.h_repeat_rate != cur_sw_part.h_repeat_rate)
    {
      XtSetArg(hsb_arg[h], XtNrepeatRate, new_sw_part.h_repeat_rate);	h++;
    }

  if (new_sw_part.show_page != cur_sw_part.show_page)
    {
      XtSetArg(vsb_arg[v], XtNshowPage, new_sw_part.show_page);		v++;
    }

  if (new_sw_part.v_granularity != cur_sw_part.v_granularity)
    {
      XtSetArg(vsb_arg[v], XtNgranularity, new_sw_part.v_granularity);	v++;
#if 0
      slider_max = (new_sw_part.bb_child->core.height +
		    2 * new_sw_part.bb_child->core.border_width -
		    new_sw_part.bboard->core.height) /
		   new_sw_part.v_granularity;
      XtSetArg(vsb_arg[v], XtNsliderMax,
	       slider_max < 0 ? 0 : slider_max);			v++;
#endif
    }

  if (new_sw_part.v_initial_delay != cur_sw_part.v_initial_delay)
    {
      XtSetArg(vsb_arg[v], XtNinitialDelay,
	       new_sw_part.v_initial_delay);				v++;
    }

  if (new_sw_part.v_menu_pane != cur_sw_part.v_menu_pane)
    {
      XtSetArg(vsb_arg[v], XtNmenuPane, new_sw_part.v_menu_pane);	v++;
    }

  if (new_sw_part.v_repeat_rate != cur_sw_part.v_repeat_rate)
    {
      XtSetArg(vsb_arg[v], XtNrepeatRate, new_sw_part.v_repeat_rate);	v++;
    }

  if (do_layout)
    {
      DoLayout(new_sw);
      flag = TRUE;
    }

  if (h != 0 || v != 0) {
    if (h != 0) {
      XtSetValues((Widget)new_sw_part.hsb, hsb_arg, h);
    }
    if (v != 0) {
      XtSetValues((Widget)new_sw_part.vsb, vsb_arg, v);
    }
    flag = TRUE;
  }

  return (flag);
 }	/*  SetValues()  */


static Boolean
SetValuesHook(w, args, num_args)
Widget w;
ArgList args;
Cardinal *num_args;
{
	TextWidget	sw = (TextWidget) w;
	Cardinal	n;
	Arg		text_arg[10];
	ArgList		new_list;
	Cardinal	newcount;
/*
 * This is the # of must-use entries, there may be a few optional
 * entries added at the end
 */
#define MASK_OFFSET	32
	static MaskArg mask_args[] = {
		{ XtNbackground,	NULL,	OL_SOURCE_PAIR },
		{ XtNbottomMargin,	NULL,	OL_SOURCE_PAIR },
		{ XtNdisplayPosition,	NULL,	OL_SOURCE_PAIR },
		{ XtNeditType,		NULL,	OL_SOURCE_PAIR },
		{ XtNfile,		NULL,	OL_SOURCE_PAIR },
		{ XtNfont,		NULL,	OL_SOURCE_PAIR },
		{ XtNfontColor,		NULL,	OL_SOURCE_PAIR },
		{ XtNgrow,		NULL,	OL_SOURCE_PAIR },
		{ XtNhorizontalSB,	NULL,	OL_SOURCE_PAIR },
		{ XtNcursorPosition,	NULL,	OL_SOURCE_PAIR },
		{ XtNleaveVerification,	NULL,	OL_SOURCE_PAIR },
		{ XtNleftMargin,	NULL,	OL_SOURCE_PAIR },
		{ XtNmaximumSize,	NULL,	OL_SOURCE_PAIR },
		{ XtNmodifyVerification,NULL,	OL_SOURCE_PAIR },
		{ XtNmotionVerification,NULL,	OL_SOURCE_PAIR },
		{ XtNrightMargin,	NULL,	OL_SOURCE_PAIR },
		{ XtNsourceType,	NULL,	OL_SOURCE_PAIR },
		{ XtNstring,		NULL,	OL_SOURCE_PAIR },
		{ XtNtextClearBuffer,	NULL,	OL_SOURCE_PAIR },
		{ XtNtextCopyBuffer,	NULL,	OL_SOURCE_PAIR },
		{ XtNtextGetInsertPoint,NULL,	OL_SOURCE_PAIR },
		{ XtNtextGetLastPos,	NULL,	OL_SOURCE_PAIR },
		{ XtNtextInsert,	NULL,	OL_SOURCE_PAIR },
		{ XtNtextReadSubStr,	NULL,	OL_SOURCE_PAIR },
		{ XtNtextRedraw,	NULL,	OL_SOURCE_PAIR },
		{ XtNtextReplace,	NULL,	OL_SOURCE_PAIR },
		{ XtNtextSetInsertPoint,NULL,	OL_SOURCE_PAIR },
		{ XtNtextUpdate,	NULL,	OL_SOURCE_PAIR },
		{ XtNtopMargin,		NULL,	OL_SOURCE_PAIR },
		{ XtNverticalSB,	NULL,	OL_SOURCE_PAIR },
		{ XtNwrap,		NULL,	OL_SOURCE_PAIR },
		{ XtNwrapBreak,		NULL,	OL_SOURCE_PAIR },
		{ NULL,		NULL,	OL_MASK_PAIR },
		{ NULL,		NULL,	OL_MASK_PAIR },
		{ NULL,		NULL,	OL_MASK_PAIR },
	};

	n = 0;
	if (sw->text.force_hsb)  {
		_OlSetMaskArg(mask_args[MASK_OFFSET + n], XtNgrow,
			OL_GROW_HORIZONTAL, OL_MASK_PAIR);
		n++;
		_OlSetMaskArg(mask_args[MASK_OFFSET + n], XtNwrap,
			FALSE, OL_MASK_PAIR);
		n++;
	}
	if (sw->text.force_vsb) {
		_OlSetMaskArg(mask_args[MASK_OFFSET + n], XtNverticalSBWidget,
			sw->text.vsb, OL_OVERRIDE_PAIR); n++;
	}

	_OlComposeArgList(args, *num_args, mask_args, XtNumber(mask_args)-3+n,
			 &new_list, &newcount);
	if (newcount) {
		XtSetValues(sw->text.bb_child, new_list, newcount);
		XFree((char *)new_list);
	}
	return(FALSE);
}	/*  SetValuesHook  */


/*************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures***************************/

static void
KidKilled(w, closure, call_data)
  Widget	w;
  XtPointer	closure, call_data;
{
  TextWidget	sw;
    
  sw = (TextWidget)(w->core.parent)->core.parent;
  if (sw->text.bb_child->core.being_destroyed == TRUE)
    sw->text.bb_child = NULL;
} /* KidKilled() */


static void
HSBMoved(w, closure, call_data)
  Widget	w;
  XtPointer	closure, call_data;
{
  OlScrollbarVerify	*olsbv;
  TextWidget	sw;


  sw = (TextWidget)w->core.parent;

  olsbv = (OlScrollbarVerify *)call_data;
  if (sw->text.bb_child == (Widget)NULL)
    olsbv->ok = FALSE;

  XtCallCallbacks((Widget)sw, XtNhSliderMoved, call_data);

  if (olsbv->ok == TRUE)
    XtMoveWidget(sw->text.bb_child,
		 (Position) (-olsbv->new_location),
		 (Position)(sw->text.bb_child->core.y));
} /* HSBMoved() */


static void
VSBMoved(w, closure, call_data)
  Widget	w;
  XtPointer	closure, call_data;
{
  OlScrollbarVerify	*olsbv;
  TextWidget		tw;


  olsbv = (OlScrollbarVerify *)call_data;
  tw = (TextWidget)w->core.parent;

  if (tw->text.bb_child == (Widget)NULL)
    olsbv->ok = FALSE;

  XtCallCallbacks((Widget)tw, XtNvSliderMoved, call_data);

  if (olsbv->ok == TRUE)
    _OlTextScrollAbsolute((TextPaneWidget)tw->text.bb_child,
			  (unsigned)olsbv->new_location, TRUE);
} /* VSBMoved() */


/*************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures***************************/

/* none */
