#pragma ident	"@(#)libDtI:HyperText.c	1.45.1.1"

/***************************************************************
**
**      Copyright (c) 1990
**      AT&T Bell Laboratories (Department 51241)
**      All Rights Reserved
**
**      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
**      AT&T BELL LABORATORIES
**
**      The copyright notice above does not evidence any
**      actual or intended publication of source code.
**
**      Author: Hai-Chen Tu
**      File:   HyperText.c
**      Date:   08/06/90
**
**************************************************************************
*/

/*
 *************************************************************************
 *
 * Description:
 *   This file contains the source code for the hypertext widget.
 *
 ******************************file*header********************************
 */

                        /* #includes go here    */

#include <stdio.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/ScrolledWi.h>

#include "DtI.h"
#include "HyperTextP.h"

#define ClassName HyperText
#include <Xol/NameDefs.h>

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define IS_HYPERTEXT(w) (XtIsSubclass((Widget)w, hyperTextWidgetClass))
#define BLACK BlackPixelOfScreen(screen)
#define WHITE WhitePixelOfScreen(screen)

#define IsColor(screen) (DefaultDepthOfScreen(screen)>1)

#define STR_DIFF(a,b) (((a)==NULL)?((int)(b)):((b)==NULL?1:(strcmp((a),(b)))))

/* default highlight color */
#define HIGHLIGHT_COLOR	"Blue"

/* stipple bitmap used in GCs */
static Pixmap bm = 0;	/* this is cached */

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *      1. Private Procedures
 *      2. Class   Procedures
 *      3. Action  Procedures
 *      4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

                    /* private procedures       */

static HyperSegment *GetFirstCmdSegment(HyperTextWidget hw);
static HyperSegment *GetPrevCmdSegment(HyperTextWidget hw);
static HyperSegment *GetNextCmdSegment(HyperTextWidget hw);
static HyperLine    *HyperTextFindLine(HyperTextWidget hw, int y1);

static void HyperTextExpose(HyperTextWidget hw, int x1, int y1, int w1,int h1);
static void GetGCs(HyperTextWidget hw, Boolean new_font,
	Boolean new_focus_color, Boolean new_background, Boolean new_key_color);
static void GetKeyColorGCs(HyperTextWidget hw);
static void GetFocusLineGC(HyperTextWidget hw);
static void draw_lines(HyperTextWidget hw, int i, int n, int x1, int x2);
static void draw_line1(HyperTextWidget hw, Window win, HyperLine *hl,
		int x1, int x2);
static void draw_segment(HyperTextWidget hw, Window win, HyperSegment *hs,
		Boolean clear_flag, Boolean busy_flag);
static void draw_focus_segment(HyperTextWidget hw, Window win,
		HyperSegment *hs, Boolean clear_flag);
static void set_line(HyperTextWidget hw);
static void layout(HyperTextWidget hw, Boolean adjust_size_flag);
static void adjust_size(HyperTextWidget hw);
static int  layout_line(HyperTextWidget hw, HyperLine *hl, XFontStruct *font,
		int line_height, int left_margin, int tab_width, int y);
static XtGeometryResult QueryGeom(Widget w, XtWidgetGeometry *intended,
		XtWidgetGeometry *reply);
static void ViewSizeChanged(Widget w, OlSWGeometries *geom);

                    /* class procedures     */

static void Initialize(Widget request, Widget new, ArgList args,
	Cardinal *num_args);
static void Destroy(Widget w);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *ev, Region region);
static void HandleSelect(HyperTextWidget hw, XButtonPressedEvent *ev0);
static void HtButtonHandler(Widget w, OlVirtualEvent ve);
static Boolean SetValues(Widget old, Widget request, Widget new,
	ArgList args, Cardinal *num_args);

                    /* action procedures        */

static void    HighlightHandler(Widget w, OlDefine highlight_type);
static Boolean ActivateWidget(Widget w, OlVirtualName type,
		XtPointer call_data);
static Widget  TraversalHandler(Widget shell, Widget w, OlDefine direction,
		Time time);

                    /* public procedures        */
/* none */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

static OlEventHandlerRec ht_event_procs[] =
{
	{ ButtonPress,   HtButtonHandler },
	{ ButtonRelease, HtButtonHandler },
};

static int default_source_type = OL_STRING_SOURCE;

/*
 ***********************************************************************
 * Dynamic resources
 ***********************************************************************
*/
#define BYTE_OFFSET XtOffsetOf(HyperTextRec, hyper_text.dyn_flags)

static _OlDynResource dyn_res[] = {
     { { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
          XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_HYPERTEXT_BG,
          NULL
     },
     { { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel), 0,
          XtRString, XtDefaultForeground }, BYTE_OFFSET,
          OL_B_HYPERTEXT_FONTCOLOR, NULL
     },
};
#undef BYTE_OFFSET


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define offset(field) XtOffset(HyperTextWidget, field)

static XtResource resources[] = {
	{ XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
		offset(core.background_pixel), XtRString, XtDefaultBackground },

	{ XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
		offset(core.border_width), XtRImmediate, (XtPointer) 0 },

	{ XtNstring, XtCString, XtRString, sizeof(String),
		offset(hyper_text.string), XtRString, NULL },

	{ XtNfile, XtCFile, XtRString, sizeof(String),
		offset(hyper_text.file), XtRString, NULL },

	{ XtNsourceType, XtCSourceType, XtRInt, sizeof(int),
		offset(hyper_text.source_type), XtRInt,
		(XtPointer) &default_source_type },

     { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel),
          offset(primitive.font_color), XtRString, XtDefaultForeground },

     { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
          offset(primitive.font), XtRString, OlDefaultFont },

	{ XtNkeyColor, XtCForeground, XtRPixel, sizeof(Pixel),
		offset(hyper_text.key_color), XtRString, HIGHLIGHT_COLOR },

	{ XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.left_margin), XtRImmediate,
		(XtPointer)LEFT_MARGIN },

	{ XtNtopMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.top_margin), XtRImmediate,
		(XtPointer)TOP_MARGIN },

	{ XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.right_margin), XtRImmediate,
		(XtPointer)RIGHT_MARGIN },

	{ XtNbottomMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.bot_margin), XtRImmediate,
		(XtPointer)BOT_MARGIN },

	{ XtNselect, XtCCallback, XtRCallback, sizeof(XtPointer),
		offset(hyper_text.callbacks), XtRCallback, (XtPointer) NULL },

	{ XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
		offset(hyper_text.resizable), XtRBoolean, (XtPointer)"TRUE" },
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

/*
 * ...ClassData must be initialized at compile time.  Must initialize all
 * substructures.  (Actually, last two here need not be initialized since not
 * used.)
 */
HyperTextClassRec hyperTextClassRec = {
 {
 (WidgetClass) & primitiveClassRec, /* superclass            */
 "HyperText",                       /* class_name            */
 sizeof(HyperTextRec),              /* size                  */
 NULL,                              /* class_initialize      */
 NULL,                              /* class_part_initialize */
 FALSE,                             /* class_inited          */
 Initialize,                        /* initialize            */
 NULL,                              /* initialize_hook       */
 XtInheritRealize,                  /* realize               */
 NULL,                              /* actions               */
 0,                                 /* num_actions           */
 resources,                         /* resources             */
 XtNumber(resources),               /* resource_count        */
 NULLQUARK,                         /* xrm_class             */
 TRUE,                              /* compress_motion       */
 TRUE,                              /* compress_exposure     */
 TRUE,                              /* compress_enterleave   */
 FALSE,                             /* visible_interest      */
 Destroy,                           /* destroy               */
 Resize,                            /* resize                */
 Redisplay,                         /* expose                */
 SetValues,                         /* set_values            */
 NULL,                              /* set_values_hook       */
 XtInheritSetValuesAlmost,          /* set_values_almost     */
 NULL,                              /* get_values_hook       */
 XtInheritAcceptFocus,              /* accept_focus          */
 XtVersion,                         /* version               */
 NULL,                              /* callback_private      */
 XtInheritTranslations,             /* tm_table              */
 QueryGeom,                         /* query_geometry        */
},     /* End of CoreClass fields initialization */

{
 True,                              /* focus_on_select       */
 (OlHighlightProc)HighlightHandler, /* highlight_handler     */
 (OlTraversalFunc)TraversalHandler, /* traversal_handler     */
 XtInheritRegisterFocus,            /* register_focus        */
 (OlActivateFunc)ActivateWidget,    /* activate              */
 ht_event_procs,                    /* event_procs           */
 XtNumber(ht_event_procs),          /* num_event_procs       */
 OlVersion,                         /* version               */
 NULL,                              /* extension             */
 { dyn_res, XtNumber(dyn_res) },    /* dyn_data              */
 NULL                               /* transparent_proc      */
 },    /* End of Primitive field initializations */

 {
 0,                                 /* field not used        */
 },    /* End of HyperTextClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass hyperTextWidgetClass = (WidgetClass) & hyperTextClassRec;

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * HyperTextHighlightSegment - Makes the given segment the current selection.
 * Also unhighlight the previous selection.
 * Input: hs may be NULL (will still do HyperTextUnhighlightSegment(hw)).
 *************************************************************************
*/
void
HyperTextHighlightSegment(hw, hs)
HyperTextWidget hw;
HyperSegment *hs;
{
	if (!IS_HYPERTEXT(hw) || hs == NULL || hw->hyper_text.highlight == hs)
		return;

	HyperTextUnhighlightSegment(hw);
	hw->hyper_text.highlight = hs;
	if (XtIsRealized((Widget)hw))
		draw_focus_segment(hw, XtWindow(hw), hs, FALSE);

} /* end of HyperTextHighlightSegment */


/*
 ****************************procedure*header*****************************
 * HyperTextUnhighlightSegment - Unhighlights the current segment selection.
 *************************************************************************
*/
void
HyperTextUnhighlightSegment(hw)
HyperTextWidget hw;
{
	HyperSegment * hs;

	if (!IS_HYPERTEXT(hw))
		return;

	hs = hw->hyper_text.highlight;
	hw->hyper_text.highlight = NULL;
	hw->hyper_text.is_busy = FALSE;

	if (hs == NULL)
		return;

	if (XtIsRealized((Widget)hw))
		draw_focus_segment(hw, XtWindow(hw), hs, TRUE);

} /* end of HyperTextUnhighlightSegment */

/*
 ****************************procedure*header*****************************
 * HyperTextGetHighlightedSegment - Returns the current highlighted segment.
 *************************************************************************
*/
HyperSegment *
HyperTextGetHighlightedSegment(hw)
HyperTextWidget hw;
{
	if (!IS_HYPERTEXT(hw))
		return(NULL);

	return(hw->hyper_text.highlight);

} /* end of HyperTextGetHighlightedSegment */

/*
 ****************************procedure*header*****************************
 * HyperTextFindByPos - Finds the hypersegment covering the point <x1,y1>.
 * Returns (HyperSegment*) which may be NULL.
 *************************************************************************
*/
HyperSegment *
HyperTextFindByPos(hw, x1, y1)
HyperTextWidget hw;
int x1, y1;
{
	int i;
	HyperLine *hl;
	HyperSegment *hs;

	if (!IS_HYPERTEXT(hw))
		return;

	/* the first line to be drawn */
	i = (y1 >= (int)(hw->hyper_text.top_margin)) ?
			(y1-(int)(hw->hyper_text.top_margin)) /
				(int)(hw->hyper_text.line_height) : -1;

	/* skip lines */
	for (hl = hw->hyper_text.first_line; hl && i > 0; hl = hl->next, i--);

	if (hl == NULL)
		return(NULL);

	for (hs = hl->first_segment; hs; hs = hs->next) {
		if (((int)(hs->x) <= x1) && ((int)(hs->x) + (int)(hs->w) - 1) >= x1)
 			return(hs);
	}
	return(NULL);

} /* end of HyperTextFindByPos */

/*
 *************************************************************************
 *
 * Private Procedures - actions for translation manager.
 *
 ***************************private*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * HandleSelect - Select callback. 
 *************************************************************************
*/
static void
HandleSelect(hw, ev0)
HyperTextWidget hw;
XButtonPressedEvent *ev0;
{
	HyperSegment *hs;
	HyperSegment *hs1;
	Window junk_w;
	unsigned int kb;
	int x1, y1, x2, y2;
	XEvent ev;
	int x, y;
	Display *dpy = XtDisplay(hw);
	Window win = XtWindow(hw);

	/* locate the selected segment */
	x = ev0->x; y = ev0->y;
	hs = HyperTextFindByPos(hw, x, y);

	/* highlight the item if it is a keyword */
	if (hs && hs->key)
		HyperTextHighlightSegment(hw, hs);
	else 
		return;

	/* grab the pointer */
	while (XGrabPointer(dpy, win, 1, ButtonPressMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, 0, None, CurrentTime) != GrabSuccess)
			;

	/* move the pointer around until mouse pressed or released */
	/* selected item is changed based on new pointer position  */
	while (1) {
		XPending(dpy);
		if (XCheckMaskEvent(dpy,ButtonPressMask | ButtonReleaseMask,&ev)
			!= 0)
		{
				break;
		} else {
			XQueryPointer(dpy, win, &junk_w, &junk_w, &x1, &y1, &x2, &y2,
				&kb);
			if (x2 == x && y2 == y)
				continue;

			hs1 = HyperTextFindByPos(hw, x2, y2);
			if (hs1 == hs)
				continue;

			hs = hs1; x = x2; y = y2;
			HyperTextUnhighlightSegment(hw);
			if (hs && hs->key) {
				HyperTextHighlightSegment(hw, hs);
				XSync(dpy, 0);
			}
		}
	}

	/* make sure the pointer is released */
	while (ev.type != ButtonRelease)
		XNextEvent(XtDisplay(hw), &ev);

	/* ungrab button */
	XUngrabPointer(XtDisplay(hw), CurrentTime);
	XSync(XtDisplay(hw), 0);

	/* check if the released button is the same as the original pressed one */
	if (ev0->button != ev.xbutton.button) {
		HyperTextUnhighlightSegment(hw);
		return;
	}

	/* finally, check if we need to do call back -- iff keyword segment */
	if (hs && hs->key) {
		HyperSegment hs_rec;

		hs1 = HyperTextGetHighlightedSegment(hw);
		if (hs1 != hs)
			HyperTextUnhighlightSegment(hw);

		/* grey on */
		hw->hyper_text.highlight = hs;
		hw->hyper_text.is_busy = TRUE;
		hs_rec = *hs;
		draw_segment(hw, win, hs, FALSE, TRUE);
		XtCallCallbacks((Widget)hw, XtNselect, (XtPointer) hs);

		/*
		 * Note that the widget or segment could be changed or destroyed
		 * while in this callback.  If so, hs's values will be changed.
		 */
		if ((hw->hyper_text.is_busy == TRUE) &&
			hs_rec.text == hs->text &&
			hs_rec.x == hs->x &&
			hs_rec.y == hs->y &&
			hs_rec.w == hs->w &&
			hs_rec.h == hs->h)
		{
			hw->hyper_text.highlight = NULL;
			draw_segment(hw, win, hs, TRUE, FALSE);
		}
	}
} /* end of HandleSelect */


/*************** widget class procedures ***********************/

/*ARGSUSED*/    /* make lint happy */
/*
 ****************************procedure*header*****************************
 *  init - Initialize procedure.
 *************************************************************************
*/
static void
Initialize(request, new, args, num_args)
Widget request;
Widget new;
ArgList args; /* unused */
Cardinal *num_args; /* unused */
{
	Widget parent = XtParent(new);
	HyperTextWidget hw = (HyperTextWidget)new;

	/* Set all flags to True s.t. all GCs are created */
	GetGCs(hw, True, True, True, True);

	hw->hyper_text.line_height = OlFontHeight(hw->primitive.font,
		hw->primitive.font_list) + SPACE_ABOVE + SPACE_BELOW;

	hw->hyper_text.tab_width = DM_TextWidth(hw->primitive.font,
		hw->primitive.font_list, "nnnnnnnn", 8);

	hw->hyper_text.window_gravity_set = FALSE;
	hw->hyper_text.is_busy = FALSE;
	hw->hyper_text.src_copy = NULL;
	hw->hyper_text.highlight = NULL;

	set_line(hw);
	layout(hw, /* do adjust size */ FALSE);

	/* reset width and height if necessary */
	if (hw->core.width == 0)
		hw->core.width = hw->hyper_text.w0;

	if (hw->core.height == 0)
		hw->core.height = hw->hyper_text.h0;

	/*
	 * Check if widget is created inside a scrolled window.  If so,
	 * set the computeGeometries callback for the scrolled window.
	 */
	if (XtIsSubclass(parent, scrolledWindowWidgetClass) == True) {
		Arg args[1];

		XtSetArg(args[0], XtNcomputeGeometries, ViewSizeChanged);
		XtSetValues(parent, args, 1);
	}

} /* end of Initialize */

/*
 ****************************procedure*header*****************************
 *  Destroy - Destroy procedure.
 *************************************************************************
*/
static void
Destroy(w)
Widget w;
{
	HyperTextWidget hw = (HyperTextWidget)w;

	XtRemoveAllCallbacks((Widget)hw, (String)XtNselect);

	if (XtIsRealized(w)) {
		if (hw->hyper_text.fg_GC)
			XtReleaseGC(w, hw->hyper_text.fg_GC);
		if (hw->hyper_text.bg_GC)
			XtReleaseGC(w, hw->hyper_text.bg_GC);
		if (hw->hyper_text.key_GC)
			XtReleaseGC(w, hw->hyper_text.key_GC);
		if (hw->hyper_text.busy_GC)
			XtReleaseGC(w, hw->hyper_text.busy_GC);
		if (hw->hyper_text.dotted_line_GC)
			XtReleaseGC(w, hw->hyper_text.dotted_line_GC);
	}
	if (hw->hyper_text.src_copy)
		free(hw->hyper_text.src_copy);

} /* end of Destroy */

/*
 ****************************procedure*header*****************************
 * Resize - Resize procedure. 
 *************************************************************************
*/
static void
Resize(w)
Widget w;
{
} /* end of Resize */

/*
 ****************************procedure*header*****************************
 *  Redisplay - Expose procedure.
 *************************************************************************
*/
static void
Redisplay(w, ev, region)
Widget w;
XEvent *ev;
Region region;
{
	HyperTextWidget hw = (HyperTextWidget) w;
	int x, y, width, height;

	if (ev != NULL) {
		x = ev->xexpose.x;
		y = ev->xexpose.y;
		width = ev->xexpose.width;
		height = ev->xexpose.height;
	} else {
		x = 0;
		y = 0;
		width = w->core.width;
		height = w->core.height;
	}
	HyperTextExpose(hw, x, y, width, height);
	if (w->core.sensitive == FALSE || w->core.ancestor_sensitive == FALSE) {
		x_fill_grids(XtDisplay(w), XtWindow(w), w->core.background_pixel,
			x, y, width, height);
	}

} /* end of Redisplay */

/*
 ****************************procedure*header*****************************
 * SetValues - Compares the requested values with the current values
 * and sets them in the new widget.  It returns True if the widget must
 * be redisplayed.
 *************************************************************************
*/
static Boolean
SetValues(current, request, new, args, num_args)
Widget current;
Widget request;
Widget new;
ArgList args; /* unused */
Cardinal *num_args; /* unused */
{
	HyperTextWidget cur_hw = (HyperTextWidget)current;
	HyperTextWidget new_hw = (HyperTextWidget)new;
	Boolean new_key_color = FALSE;
	Boolean new_font = FALSE;
	Boolean new_focus_color = FALSE;
	Boolean new_background = FALSE;
	Boolean need_layout = FALSE;
	Boolean need_newgc  = FALSE;
	Boolean was_changed = FALSE;
	HyperSegment *hs;

	HyperTextUnhighlightSegment(new_hw);
	if (new_hw->core.sensitive != cur_hw->core.sensitive ||
		new_hw->core.ancestor_sensitive != cur_hw->core.ancestor_sensitive)
			was_changed = TRUE;

	/* see if original input data has changed */
	if (new_hw->hyper_text.source_type != cur_hw->hyper_text.source_type
		|| (new_hw->hyper_text.source_type == OL_DISK_SOURCE &&
		STR_DIFF(new_hw->hyper_text.file, new_hw->hyper_text.src_copy))
		|| (new_hw->hyper_text.source_type == OL_STRING_SOURCE &&
		STR_DIFF(new_hw->hyper_text.string, new_hw->hyper_text.src_copy)))
	{
		/* we have to reconstruct the hyper line field */
		set_line(new_hw);
		new_hw->hyper_text.highlight = NULL;
		new_hw->hyper_text.is_busy = NULL;
		need_layout = TRUE;
	 	was_changed = TRUE;
	}

	/* see if font resource was changed */
	if (new_hw->primitive.font      != cur_hw->primitive.font ||
	    new_hw->primitive.font_list != cur_hw->primitive.font_list)
	{
		new_hw->hyper_text.line_height =
			OlFontHeight(new_hw->primitive.font,
				new_hw->primitive.font_list) + SPACE_ABOVE + SPACE_BELOW;

		new_hw->hyper_text.tab_width = DM_TextWidth(new_hw->primitive.font,
			new_hw->primitive.font_list, "nnnnnnnn", 8);

		new_font = TRUE;
		need_newgc = TRUE;
		need_layout = TRUE;
		was_changed = TRUE;
	}

	/* see if margin resources were changed */
	if (new_hw->hyper_text.left_margin  != cur_hw->hyper_text.left_margin ||
	    new_hw->hyper_text.top_margin   != cur_hw->hyper_text.top_margin ||
	    new_hw->hyper_text.right_margin != cur_hw->hyper_text.right_margin ||
	    new_hw->hyper_text.bot_margin   != cur_hw->hyper_text.bot_margin)
	{
		need_layout = TRUE;
		was_changed = TRUE;
	}

	/* see if background resource was changed */
	if (new_hw->core.background_pixel != cur_hw->core.background_pixel) {
		new_background = TRUE;
		need_newgc = TRUE;
		was_changed = TRUE;
	}

	/* see if font color resource was changed */
	if (new_hw->primitive.font_color != cur_hw->primitive.font_color) {
		need_newgc = TRUE;
		was_changed = TRUE;
	}

	/* see if key color resource was changed */
	if (new_hw->hyper_text.key_color != cur_hw->hyper_text.key_color) {
		new_key_color = TRUE;
		was_changed = TRUE;
	}

	if (new_hw->primitive.input_focus_color !=
	    cur_hw->primitive.input_focus_color)
	{
		new_focus_color = TRUE;
		was_changed = TRUE;
	}

	if (need_newgc) {
		if (new_font || new_background || new_key_color) {
          	XtReleaseGC((Widget)new_hw, new_hw->hyper_text.key_GC);
          	XtReleaseGC((Widget)new_hw, new_hw->hyper_text.dotted_line_GC);
		}

		if (new_font || new_focus_color)
          	XtReleaseGC((Widget)new_hw, new_hw->hyper_text.focus_line_GC);

          XtReleaseGC((Widget)new_hw, new_hw->hyper_text.fg_GC);
          XtReleaseGC((Widget)new_hw, new_hw->hyper_text.bg_GC);
          XtReleaseGC((Widget)new_hw, new_hw->hyper_text.busy_GC);
		GetGCs(new_hw, new_font, new_focus_color, new_background,
			new_key_color);
	} else if (new_key_color) {
          XtReleaseGC((Widget)new_hw, new_hw->hyper_text.key_GC);
          XtReleaseGC((Widget)new_hw, new_hw->hyper_text.dotted_line_GC);
		GetKeyColorGCs(new_hw);
	} else if (new_focus_color) {
          XtReleaseGC((Widget)new_hw, new_hw->hyper_text.focus_line_GC);
		GetFocusLineGC(new_hw);
	}

	if (need_layout)
		layout(new_hw, /* do adjust size */ TRUE);

	/* If widget currently has focus, assign input focus to
	 * the first command segment for now.
	 */
	if (cur_hw->hyper_text.highlight) {
		hs = GetFirstCmdSegment(new_hw);
		if (hs)
			HyperTextHighlightSegment(new_hw, hs);
	}
	/* Future enhancement: if font was changed s.t. focus segment is
	 * not in view, update view.
	 */
	return(was_changed);

} /* end of SetValues */

/*
 ****************************procedure*header*****************************
 *  QueryGeom - Called by parent of hypertext widget to get geometry
 * information on it.
 *************************************************************************
*/
static XtGeometryResult
QueryGeom(w, intended, reply)
Widget w;
XtWidgetGeometry *intended;	/* parent's changes; may be NULL */
XtWidgetGeometry *reply;		/* child's preferred geometry; never NULL */
{
	XtGeometryResult result;

	/* Future enhancement: give widget a say in what it prefers */
	/* printf("wanted w=%d h=%d\n", intended->width, intended->height); */
	result = XtGeometryYes;
	return(result);

} /* end of QueryGeom */


/****************************************************************
 *
 * Private Procedures - the rest of it.
 *
 ****************************************************************/

/*
 ****************************procedure*header*****************************
 * HyperTextExpose - 
 *************************************************************************
*/
static void
HyperTextExpose(hw, x1, y1, w1, h1)
HyperTextWidget hw;
int x1, y1;
int w1, h1;
{
	int i, j;
	int n;

	/* the first line to be drawn */
	i = (y1 >= (int)(hw->hyper_text.top_margin)) ?
			(y1-(int)(hw->hyper_text.top_margin)) /
				(int)(hw->hyper_text.line_height) : 0;

	/* the number of lines to be drawn */
	j = ((y1+h1-1) >= (int)(hw->hyper_text.top_margin)) ?
			(y1+h1-1-(int)(hw->hyper_text.top_margin)) /
				(int)(hw->hyper_text.line_height) : 0;

	n = j - i + 1;

	/* draw line[i] to lines[i+n-1] within the [x1,x1+w1-1] boundary */
	draw_lines(hw, i, n, x1, x1+w1-1);

} /* end of HyperTextExpose */

/*
 ****************************procedure*header*****************************
 * HyperTextFindLine - 
 *************************************************************************
*/
static HyperLine *
HyperTextFindLine(hw, y1)
HyperTextWidget hw;
int y1;
{
	int i;
	HyperLine *hl;

	i = (y1 >= (int)(hw->hyper_text.top_margin)) ?
		(y1-(int)(hw->hyper_text.top_margin)) /
			(int)(hw->hyper_text.line_height) : -1;

	/* skip lines */
	for (hl = hw->hyper_text.first_line; hl && i > 0; hl = hl->next, i--);

	return(hl);
}

/*
 ****************************procedure*header*****************************
 * Creates GCs used by the hypertext widget.  Flags are used to indicate
 * which GCs need to be recreated.  GCs to be recreated are freed in
 * SetValues() first.
 *************************************************************************
*/
static void
GetGCs(HyperTextWidget hw, Boolean new_font, Boolean new_focus_color,
	Boolean new_background, Boolean new_key_color)
{
#define stipple_width 16
#define stipple_height 16
static unsigned char stipple_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa};

	XGCValues values;
	Display *dpy = XtDisplay(hw);
	Screen *screen = XtScreen(hw);

	if (!bm)
		bm = XCreatePixmapFromBitmapData(dpy, DefaultRootWindow(dpy),
				(char *)stipple_bits, stipple_width, stipple_height,
				0, /* foreground always off */
				1, /* background always on  */
				(unsigned)1);

	values.stipple = bm;
	values.font = hw->primitive.font->fid;
	values.foreground = hw->primitive.font_color;
	values.background = hw->core.background_pixel;

     /* create GC using primitive.font_color as foreground */
	hw->hyper_text.fg_GC = XtGetGC((Widget)hw,
		(unsigned) GCForeground | GCBackground | GCFont | GCStipple,
		&values);

     /* create GC using core.background_pixel as foreground */
     values.foreground = hw->core.background_pixel;
     hw->hyper_text.bg_GC = XtGetGC((Widget)hw,
		(unsigned) GCForeground | GCFont | GCStipple, &values);

     /* create GC using core.background_pixel as foreground */
     values.fill_style = FillOpaqueStippled;
     hw->hyper_text.busy_GC = XtGetGC((Widget)hw,
		(unsigned) GCBackground | GCFont | GCStipple | GCFillStyle, &values);

	if (new_font || new_background || new_key_color)
		GetKeyColorGCs(hw);

	if (new_font || new_focus_color)
		GetFocusLineGC(hw);

} /* end of GetGCs */

/*
 ****************************procedure*header*****************************
 * GetKeyColorGCs - Creates GC used for highlighting links and terms.
 *************************************************************************
*/
static void
GetKeyColorGCs(hw)
HyperTextWidget hw;
{
	XGCValues values;
	Screen *screen = XtScreen((Widget)hw);

     /* create GC using hyper_text.key_color as foreground */
     values.foreground = hw->hyper_text.key_color;
     values.background = hw->core.background_pixel;
	values.stipple    = bm;
     values.font       = hw->primitive.font->fid;
     hw->hyper_text.key_GC = XtGetGC((Widget)hw,
		(unsigned) GCForeground | GCBackground | GCFont | GCStipple,
		&values);

     /* create GC using for dotted line using key_color as foreground */
     values.line_style = LineOnOffDash;
     values.dashes = 2;
	if (IsColor(screen) == FALSE)
     	values.foreground = hw->primitive.font_color;
	else
     	values.foreground = hw->hyper_text.key_color;
     hw->hyper_text.dotted_line_GC = XtGetGC((Widget)hw,
               (unsigned) GCForeground | GCBackground | GCLineStyle |
			GCDashList | GCFont, &values);

} /* end of GetKeyColorGCs */

/*
 ****************************procedure*header*****************************
 * Creates GC used for underlining a segment with focus.
 *************************************************************************
*/
static void
GetFocusLineGC(hw)
HyperTextWidget hw;
{
	XGCValues values;
	Screen *screen = XtScreen((Widget)hw);

	if (IsColor(screen) == FALSE)
		if (hw->core.background_pixel == BLACK)
			values.foreground = WHITE;
		else
			values.foreground = BLACK;
	else
		values.foreground = hw->primitive.input_focus_color;

	values.font = hw->primitive.font->fid;
	hw->hyper_text.focus_line_GC = XtGetGC((Widget)hw,
		(unsigned) GCForeground | GCFont, &values);

} /* end of GetFocusLineGC */

/*------------ layout hyper text positions -------------*/

/*
 ****************************procedure*header*****************************
 * set_line - Constructs (initializes) the hyper line.
 *************************************************************************
*/
static void
set_line(hw)
HyperTextWidget hw;
{
	HyperLine	*hl;

	hw->hyper_text.first_line = NULL;
	hw->hyper_text.last_line = NULL;

	/* either init the hyper line from the file or from the string */
	if (hw->hyper_text.source_type == OL_DISK_SOURCE) {

		/* check the string */
		if (hw->hyper_text.file == NULL)
			return;

		if (hw->hyper_text.src_copy)
			free(hw->hyper_text.src_copy);

		hw->hyper_text.src_copy = HtNewString(hw->hyper_text.file);
		hw->hyper_text.first_line =  HyperLineFromFile((Widget)hw,
			hw->hyper_text.file);

	} else { /* from string: OL_STRING_SOURCE */

		/* check the string */
		if (hw->hyper_text.string == NULL)
			return;

		if (hw->hyper_text.src_copy)
			free(hw->hyper_text.src_copy);

		hw->hyper_text.src_copy = HtNewString(hw->hyper_text.string);
		hw->hyper_text.first_line =  HyperLineFromString((Widget)hw,
			hw->hyper_text.string);
	}

	/* Find the last hyper line and also set the ptr to the
	 * previous hyperline along the way.  Would be more efficient
	 * to set this up as the text is read in but this will do
	 * for now.
	 */
	hw->hyper_text.first_line->prev = NULL;
	hl = hw->hyper_text.first_line->next;

	if (hl == NULL) {
			hw->hyper_text.last_line = hw->hyper_text.first_line;
	} else {
		HyperLine *save = hl;

		hl->prev = hw->hyper_text.first_line;
		hl = hl->next;
		while (hl) {
			hl->prev = save;
			save = hl;
			hl = hl->next;
			if (hl == NULL)
				hw->hyper_text.last_line = save;
		}
	}

	/* we don't care whether or not the ...From... function failed */
	return;

} /* end of set_line */

/*
 ****************************procedure*header*****************************
 * layout - 
 *************************************************************************
*/
/*
	y      _________________________
	y_text _________________________<- space above text
			XXX
			X X             <- ascent
	       ________ XXX ____________
		 ------   X ----------- <- underline
			X X             <- descent
	       ________ XXX ____________
	       _________________________<- space below text

*/
static void
layout(HyperTextWidget hw, Boolean adjust_size_flag)
{
	int y;
	int w, w1;
	HyperLine *hl;
	XFontStruct *font = hw->primitive.font;
	int line_height = hw->hyper_text.line_height;
	int left_margin = hw->hyper_text.left_margin;
	int tab_width = hw->hyper_text.tab_width;

	w = left_margin;
	hw->hyper_text.h0 = y = hw->hyper_text.top_margin;

	for (hl = hw->hyper_text.first_line; hl ; hl = hl->next) {
		w1 = layout_line(hw, hl, font, line_height, left_margin,
				tab_width, y);
		w = _OlMax(w, w1);
		y += hw->hyper_text.line_height;
	}
	hw->hyper_text.w0 = w + hw->hyper_text.right_margin;
	hw->hyper_text.h0 = y + hw->hyper_text.bot_margin;

	if (hw->hyper_text.w0 <= 0)
		hw->hyper_text.w0 = 1;
	if (hw->hyper_text.h0 <= 0)
		hw->hyper_text.h0 = 1;

	if (adjust_size_flag)
		adjust_size(hw);

} /* end of layout */

/*
 ****************************procedure*header*****************************
 * layout_line - returns the width required for displaying the line.
 *************************************************************************
*/
static int
layout_line(hw, hl, font, line_height, left_margin, tab_width, y)
HyperTextWidget hw;
HyperLine *hl;
XFontStruct *font;
int line_height;
int left_margin;
int tab_width;
int y;
{
	register HyperSegment *hs;
	int x;

	x = left_margin;

	for (hs = hl->first_segment; hs; hs = hs->next) {
		if (hs->tabs > 0)
 			x  = ((x-left_margin+((int)(hs->tabs) * tab_width))/tab_width)
				* tab_width + left_margin;
		hs->x = x;
		hs->y = y;

		if (hw->primitive.font_list)
			hs->y_text = y + hw->primitive.font_list->max_bounds.ascent +
						SPACE_ABOVE;
		else
			hs->y_text = y + font->ascent + SPACE_ABOVE;

		hs->w = DM_TextWidth(font, hw->primitive.font_list, hs->text,
				hs->len);

		hs->h = line_height;
		x += hs->w;
	}
	return(x);

} /* end of layout_line */

/*
 ****************************procedure*header*****************************
 * adjust_size -
 *************************************************************************
*/
static void
adjust_size(hw)
HyperTextWidget hw;
{
	int w, h;
	int w1, h1;
	Widget parent;
	XtGeometryResult requestReturn;

	if (hw->hyper_text.resizable == FALSE)
		return;

	w = hw->hyper_text.w0;
	h = hw->hyper_text.h0;

	/* use core.width and core.height of bulletin board parent,
	 * not scrolled window.
	 */
	parent = XtParent(hw);
	if (XtParent(parent) &&
		XtIsSubclass(XtParent(parent), scrolledWindowWidgetClass))
	{
		if (w < (int)(parent->core.width)) w = parent->core.width;
		if (h < (int)(parent->core.height)) h = parent->core.height;
	}

	if (XtIsRealized((Widget)hw) &&
		hw->hyper_text.window_gravity_set == FALSE)
	{
		XSetWindowAttributes ws;
		unsigned int valuemask;

		/* prevent the window being cleared */
		valuemask = CWBitGravity;
		ws.bit_gravity = NorthWestGravity;
		XChangeWindowAttributes(XtDisplay(hw), XtWindow(hw), valuemask, &ws);

		hw->hyper_text.window_gravity_set = TRUE;
	}
	requestReturn = XtMakeResizeRequest((Widget)hw, w, h,
					(Dimension *)&w1, (Dimension *)&h1);

	switch (requestReturn) {
	case XtGeometryYes: /* Request accepted. */
		break;
	case XtGeometryNo:  /* Request denied. */
		break;
	case XtGeometryAlmost: /* Request denied, but willing to take replyBox. */
		XtMakeResizeRequest((Widget)hw, w1, h1, NULL, NULL);
		break;
	case XtGeometryDone:    /* Request accepted and done. */
		break;
	}
} /* end of adjust_size */

/*------------ draw hyper text -------------*/

/*
 ****************************procedure*header*****************************
 * draw_lines -
 *************************************************************************
*/
static void
draw_lines(hw, i, n, x1, x2)
HyperTextWidget hw;
int i, n;
int x1, x2;
{
	HyperLine *hl;

	/* skip lines */
	for (hl = hw->hyper_text.first_line; hl && i > 0; hl = hl->next, i--);

	/* draw lines */
	for (; hl && n > 0; hl = hl->next, n--)
		draw_line1(hw, XtWindow(hw), hl, x1, x2);

} /* end of draw_lines */

/*
 ****************************procedure*header*****************************
 * draw_line1 -
 *************************************************************************
*/
static void
draw_line1(hw, win, hl, x1, x2)
HyperTextWidget hw;
Window win;
HyperLine *hl;
int x1, x2;
{
	register HyperSegment *hs;

	/* skip segments that are at left of x1 */
	for (hs = hl->first_segment;
		hs && (((int)(hs->x) + (int)(hs->w) - 1) < x1); hs = hs->next)
			;

	/* draw segments that are at left of x2 */
	for (; hs && (hs->x < x2); hs = hs->next)
		draw_segment(hw, win, hs, FALSE, FALSE);
}

/*
 ****************************procedure*header*****************************
 * draw_segment -
 *************************************************************************
*/

static void
draw_segment(HyperTextWidget hw, Window win, HyperSegment *hs,
	Boolean clear_flag /* do XClearArea? */,
	Boolean busy_flag /* make text segment busy? */)
{
	Screen *screen = XtScreen(hw);
	Display *dpy   = XtDisplay(hw);

	if (hs->len == 0)
		return;

	if (clear_flag == TRUE) {
		XClearArea(dpy, win, hs->x, hs->y, hs->w, hs->h, FALSE);
	}

	if (busy_flag) {
		XFillRectangle(dpy, win, hw->hyper_text.busy_GC,
			hs->x, hs->y, hs->w, hs->h);

	} else if (hs->reverse_video == TRUE) { /* dot-underline terms */
		if (hw->primitive.font_list)
			OlDrawString(dpy, win, hw->primitive.font_list,
				hw->hyper_text.fg_GC, hs->x, hs->y_text,
				(unsigned char *)(hs->text), hs->len);
		else
			XDrawString(dpy, win, hw->hyper_text.fg_GC, hs->x, hs->y_text,
				hs->text, hs->len);

		XDrawLine(dpy, win, hw->hyper_text.dotted_line_GC, hs->x,
			hs->y_text+2,hs->x+hs->w-1,hs->y_text+2);

	} else if (hs->key) { /* draw links */
		/* display links in reverse video on monochrome display */
		if (IsColor(screen) ==  FALSE) {
			XFillRectangle(dpy, win, hw->hyper_text.fg_GC, hs->x, hs->y+1,
					hs->w, hs->h-3);

			if (hw->primitive.font_list)
				OlDrawString(dpy, win, hw->primitive.font_list,
					hw->hyper_text.bg_GC, hs->x, hs->y_text,
					(unsigned char *)(hs->text), hs->len);
			else
				XDrawString(dpy, win, hw->hyper_text.bg_GC, hs->x,
					hs->y_text, hs->text, hs->len);
		} else {
			if (hw->primitive.font_list)
				OlDrawString(dpy, win, hw->primitive.font_list,
					hw->hyper_text.key_GC, hs->x, hs->y_text,
					(unsigned char *)(hs->text), hs->len);
			else
				XDrawString(dpy, win, hw->hyper_text.key_GC, hs->x,
					hs->y_text, hs->text, hs->len);
		}
	} else {
		/* draw non-command segments */
		if (hw->primitive.font_list)
			OlDrawString(dpy, win, hw->primitive.font_list,
				hw->hyper_text.fg_GC, hs->x, hs->y_text,
				(unsigned char *)(hs->text), hs->len);
		else
			XDrawString(dpy, win, hw->hyper_text.fg_GC, hs->x, hs->y_text,
				hs->text, hs->len);
	}

	/* draw current focus segment */
	if (hs == hw->hyper_text.highlight)
		draw_focus_segment(hw, XtWindow(hw), hs, FALSE);

	if (busy_flag)
		XSync(dpy, 0);

} /* end of draw_segment */

/*
 ****************************procedure*header*****************************
 * Underlines with input focus color an item which has just gained focus
 * and un-underline an item which has just lost focus.
 *
 * Called from HyperTextHighlightSegment() and HyperTextUnhighlightSegment().
 *************************************************************************
*/
static void
draw_focus_segment(HyperTextWidget hw, Window win, HyperSegment *hs,
	Boolean clear_flag /* False for highlight, True for unhighlight */)
{
	Display *dpy = XtDisplay(hw);

	if (hs->len == 0)
		return;

	if (clear_flag == TRUE) {
		XDrawLine(dpy, win, hw->hyper_text.bg_GC, hs->x,
			hs->y_text+2,hs->x+hs->w-1,hs->y_text+2);
		/* redraw dotted line */
		if (hs->reverse_video == TRUE)
			XDrawLine(dpy, win, hw->hyper_text.dotted_line_GC, hs->x,
				hs->y_text+2,hs->x+hs->w-1,hs->y_text+2);
	} else
		XDrawLine(dpy, win, hw->hyper_text.focus_line_GC, hs->x,
			hs->y_text+2,hs->x+hs->w-1,hs->y_text+2);

} /* end of draw_focus_segment */

/*
 ****************************procedure*header*****************************
 * HtButtonHandler -
 *************************************************************************
*/
static void
HtButtonHandler(w, ve)
Widget w;
OlVirtualEvent ve;
{
	switch(ve->virtual_name) {
	case OL_SELECT:
		ve->consumed = True;
		if (ve->xevent->type == ButtonPress)
			HandleSelect((HyperTextWidget)w,
				(XButtonPressedEvent *)(ve->xevent));
		else
			; /* button up */
	}
} /* end of HtButtonHandler */

/*
 ****************************procedure*header*****************************
 * ActivateWidget -
 *************************************************************************
*/
static Boolean
ActivateWidget(Widget w, OlVirtualName type, XtPointer call_data)
{
	HyperTextWidget hw = (HyperTextWidget)w;

	switch(type) {
	case OL_SELECTKEY:
	{
		HyperSegment *hs;
		HyperSegment hs_rec;

		hs = HyperTextGetHighlightedSegment(hw);

		/* no need to check if segment is a command segment
		 * - it has to be.  Show busy visual.
		 */
		hw->hyper_text.is_busy = TRUE;
		hs_rec = *hs;
		draw_segment(hw, XtWindow(hw), hs, FALSE, TRUE);

		/* invoke callback */
		XtCallCallbacks((Widget)hw, XtNselect, (XtPointer) hs);

		/*
		 * If the widget or the segment is destroyed,
		 * hs's value will be different.
		 */
		if ((hw->hyper_text.is_busy == TRUE) &&
			hs_rec.text == hs->text &&
			hs_rec.x == hs->x &&
			hs_rec.y == hs->y &&
			hs_rec.w == hs->w &&
			hs_rec.h == hs->h) {
			draw_segment(hw, XtWindow(hw), hs, TRUE, FALSE);
		}
	}
		break;
	default:
		/* Ignore other key presses */
		break;
	}
} /* end of ActivateWidget */

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
static Widget
TraversalHandler(Widget shell, Widget w, OlDefine direction, Time time)
{
	HyperTextWidget hw = (HyperTextWidget)w;
	HyperSegment *hs;

	switch(direction) {
	case OL_MOVELEFT:
	case OL_MULTILEFT:
	case OL_MULTIUP:
	case OL_MOVEUP:
		/* Using current focus item, find and highlight
		 * previous cmd segment.
		 */
		hs = GetPrevCmdSegment(hw);
		if (hs)
			HyperTextHighlightSegment(hw, hs);
		break;
	case OL_MOVERIGHT:
	case OL_MULTIRIGHT:
	case OL_MULTIDOWN:
	case OL_MOVEDOWN:
		/* Using current focus item, find and highlight
		 * next cmd segment.
		 */
		hs = GetNextCmdSegment(hw);
		if (hs)
			HyperTextHighlightSegment(hw, hs);
		break;
	default:
		break;
	}
} /* end of TraversalHandler */

/*
 ****************************procedure*header*****************************
 * HighlightHandler -
 *************************************************************************
*/
static void
HighlightHandler(Widget w, OlDefine highlight_type)
{
	HyperTextWidget hw = (HyperTextWidget)w;
	HyperSegment *hs;

	if (highlight_type == OL_IN) {
		/* highlight the first segment with hs->key != NULL */
		hs = GetFirstCmdSegment(hw);
		HyperTextHighlightSegment(hw, hs);
	} else {
		/* find current segment with focus */
		hs = HyperTextGetHighlightedSegment(hw);
		if (hs)
			HyperTextUnhighlightSegment(hw);
	}

} /* end of HighlightHandler */

/*
 ****************************procedure*header*****************************
 * GetFirstCmdSegment - Returns the ptr to the first cmd segment.
 *************************************************************************
*/
static HyperSegment *
GetFirstCmdSegment(hw)
HyperTextWidget hw;
{
	int i;
	HyperLine *hl;
	HyperSegment *hs;

	if (!IS_HYPERTEXT(hw))
		return(NULL);

	for (hl = hw->hyper_text.first_line; hl; hl = hl->next) {
		for (hs = hl->first_segment; hs; hs = hs->next) {
			if (hs->key != NULL)
				return(hs);
		}
	}
	return(NULL);
} /* end of GetFirstCmdSegment */

/*
 ****************************procedure*header*****************************
 * GetNextCmdSegment - Returns the ptr to the next cmd segment in the list
 * following the currently highlighted segment.
 *************************************************************************
 */ 
static HyperSegment *
GetNextCmdSegment(hw)
HyperTextWidget hw;
{
	int i;
	HyperLine *hl;
	HyperSegment *hs;
	HyperSegment *next_hs;
	Boolean past = False; /* past current focus segment */

	/* Scan each segment in each line for the highlighted segment.
	 * Then look for the next command segment, which could be either
	 * in the same line as the current segment with focus or on another
	 * line.  The flag "past" is used to indicate whether we've scanned
	 * past the current segment with focus. 
	 */
	for (hl = hw->hyper_text.first_line; hl; hl = hl->next) {
		for (hs = hl->first_segment; hs; hs = hs->next) {
			if (hs == hw->hyper_text.highlight) {
				past = True;
				continue;
			} else if (past) {
				if (hs->key != NULL)
					return(hs);
			}
		}
	}
	return(NULL);
} /* end of GetNextCmdSegment */

/*
 ****************************procedure*header*****************************
 * GetPrevCmdSegment - Returns the ptr to the previous cmd segment in the
 * list preceeding the currently highlighted segment.
 *************************************************************************
 */ 
static HyperSegment *
GetPrevCmdSegment(hw)
HyperTextWidget hw;
{
	int i;
	HyperLine *hl;
	HyperSegment *hs;
	HyperSegment *prev_hs;
	Boolean hit = False; /* hit current focus segment */

	/* Scan each segment in each line for the highlighted segment
	 * starting from the last segment in a hyper line.  Once the
	 * current segment with focus is found, turn the flag "hit"
	 * on and start looking for the previous command segment.
	 * Keep backtracking until the previous command segment is
	 * found or until the beginning of the text is reached.
	 */
	for (hl = hw->hyper_text.last_line; hl; hl = hl->prev) {
		for (hs = hl->last_segment; hs; hs = hs->prev) {
			if (hit) {
				if (hs->key != NULL)
					return(hs);
			} else if (hs == hw->hyper_text.highlight) {
				hit = True;
				continue;
			}
		}
	}
	return(NULL);
} /* end of GetPrevCmdSegment */

/*
 ****************************procedure*header*****************************
 * ViewSizeChanged - ComputeGeometries callback from scrolled window parent
 * of hypertext window.  The purpose of this callback is to resize view
 * vertically only.  The width of the help window shell has to be adjusted
 * by the application to accomodate the presence of a vertical scrollbar.
 */
static void
ViewSizeChanged(w, geom)
Widget w;
OlSWGeometries *geom;
{
	Boolean need_vsb;
	Dimension height;
	Dimension width;

	/* get height of hypertext widget */
	XtVaGetValues(w, XtNwidth, &width, XtNheight, &height, NULL);

	if (height > geom->sw_view_height)
		need_vsb = True;
	else
		need_vsb = False;

	if (height < geom->sw_view_height)
		geom->bbc_height = geom->bbc_real_height = geom->sw_view_height;
	else
		geom->bbc_height = geom->bbc_real_height = height;

	if (width > geom->sw_view_width) {
		geom->bbc_width = geom->bbc_real_width = width;
		/* need to exclude height of horizontal scrollbar to avoid 
		 * an unnecessary vertical scrollbar but only if height is
		 * less than geom->sw_view_height.
		 */
		if (geom->bbc_height == geom->sw_view_height)
			geom->bbc_height = geom->bbc_real_height =
				geom->sw_view_height - geom->hsb_height;
	} else {
		/* need to exclude width of vertical scrollbar to avoid
		 * an unnecessary horizontal scrollbar.
		 */
		if (need_vsb)
			geom->bbc_width = geom->bbc_real_width =
				geom->sw_view_width - geom->vsb_width;
		else
			geom->bbc_width = geom->bbc_real_width = geom->sw_view_width;
	}
} /* end of ViewSizeChanged */
