#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:HyperText.c	1.25"
#endif
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

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#ifdef USE_COLOR_OBJECT
#include <Xm/ColorObj.h>
#endif

#include "HyperTextP.h"
#include "ScrollUtil.h"

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

	/* NOTIFY_NEW_STRING - indicates the caller is interested in knowing
	 *		whether XmNstring is changed or not.
	 * GOT_NEW_STRING - if NOTIFY_NEW_STRING is set, then SetValues()
	 *		have to turn this bit on, the caller will have
	 *		to zero out before returning */
#define NOTIFY_NEW_STRING	(1L << 1)
#define GOT_NEW_STRING		(1L << 2)

#define CPART(w)	(((ExmHyperTextWidget)(w))->core)
#define PPART(w)	(((ExmHyperTextWidget)(w))->primitive)
#define HPART(w)	(((ExmHyperTextWidget)(w))->hyper_text)

					/* SPACE_ABOVE can be 0, but
					 * SPACE_BELOW has to be at least 1 */
#define SPACE_ABOVE		1
#define SPACE_BELOW		1
#define TAB_WIDTH		2

					/* define left/right margins  */
					/* define top/bottom margins  */
#define SIDE_MARGIN(w)		(HPART(w).tab_width / TAB_WIDTH)
#define TB_MARGIN(w)		HPART(w).line_height
#define X_UOM(w)		(HPART(w).tab_width / TAB_WIDTH)
#define Y_UOM(w)		HPART(w).line_height

#define HIGHLIGHT_COLOR		"Blue"	/* default highlight color */

#define IsColor(S)		(DefaultDepthOfScreen(S) > 1)

#define INITIAL_DELAY		(unsigned long)500
#define ADD_TIMER(P,W,I)	XtAppAddTimeOut(XtWidgetToApplicationContext(\
					(Widget)(W)), (I), P, (XtPointer)(W))

#define RM_TIMER(ID)	if ((ID)) { XtRemoveTimeOut(ID); ID = 0; }

					/* define font related macros... */
#define GET_FONT(W, F)\
	if (HPART(W).font == (XFontStruct *)NULL) {\
		short	ignored;\
		_XmFontListSearch(HPART(W).font_list,\
				XmFONTLIST_DEFAULT_TAG, &ignored, &F);\
	} else F = HPART(W).font

static _XmConst String	tab_str = "nn";
#define INIT_FONT_HI_WD(W) {\
	XmFontType	ft;\
	HPART(W).font = XmFontListEntryGetFont(HPART(W).font_list, &ft);\
	if (ft != XmFONT_IS_FONT) HPART(W).font = (XFontStruct *)NULL; }\
	if (HPART(W).font == (XFontStruct *)NULL) {\
		XmString	s;\
		s = XmStringCreate(tab_str, XmFONTLIST_DEFAULT_TAG);\
		HPART(W).line_height = XmStringHeight(HPART(W).font_list,s);\
		HPART(W).tab_width = XmStringWidth(HPART(W).font_list,s);\
		XmStringFree(s);\
	} else {\
		HPART(W).line_height = HPART(W).font->ascent +\
					HPART(W).font->descent;\
		HPART(W).tab_width = XTextWidth(HPART(W).font,tab_str,TAB_WIDTH);\
	}

#define CALC_FONT_WD(WD,W,S,L)\
	if (HPART(W).font == (XFontStruct *)NULL) {\
		XmString	s;\
		s = XmStringCreate(S, XmFONTLIST_DEFAULT_TAG);\
		WD = XmStringWidth(HPART(W).font_list,s);\
		XmStringFree(s);\
	} else {\
		WD = XTextWidth(HPART(W).font,(char *)(S),L);\
	}

	/* When we are not in C locale, use HS->line_height for now
	 * because it's expense to get it right (also NEC solution)!! */
#define Y_TEXT(W,HY)	(HPART(W).font) ?\
				HY + HPART(W).font->ascent + SPACE_ABOVE :\
				HY + SPACE_ABOVE + HPART(W).line_height

	/* the last parameter for XmStringDraw is the clip rectangle,
	 * we should take advantage of that... */
#define DRAW_STRING(D,W,HS,HX,HY)\
	if (HPART(W).font == (XFontStruct *)NULL) {\
		XmString	s;\
		Dimension	wd;\
		s = XmStringCreate(HS->text, XmFONTLIST_DEFAULT_TAG);\
		wd = XmStringWidth(HPART(W).font_list, s);\
		XmStringDraw(D, XtWindow(W),\
				HPART(W).font_list, s, HPART(W).gc,\
				HX, HY + SPACE_ABOVE, wd,\
				XmALIGNMENT_BEGINNING,\
				XmSTRING_DIRECTION_DEFAULT,\
				NULL); /* clip, optimization here */\
		XmStringFree(s);\
	} else XDrawString(D, XtWindow((Widget)(W)), HPART(W).gc, HX,\
				HY + HPART(W).font->ascent + SPACE_ABOVE,\
				HS->text, HS->len)


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

static ExmHyperSegment *FindByPos(Widget, WidePosition, WidePosition);
static ExmHyperSegment *GetFirstCmdSegment(Widget);

static void AdjustSize(Widget, Dimension, Dimension);
static void DrawSegment(Widget, ExmHyperSegment *, Boolean, Boolean);
static void DrawFocusSegment(Widget, ExmHyperSegment *, Boolean);
static void HighlightSegment(Widget, ExmHyperSegment *);
static void HsbOrVsbCB(Widget, XtPointer, XtPointer);
static void HyperLineFree(ExmHyperLine *);
static void HyperSegmentFree(ExmHyperSegment *);
static void HyperTextExpose(Widget, WidePosition, WidePosition,
							Dimension, Dimension);
static void Layout(Widget, Boolean, Dimension, Dimension);
static WideDimension
	    LayoutLine(Widget, ExmHyperLine *, Dimension, Dimension, Dimension);
static void SetLine(Widget, Widget);
static void TreatScratchGC(Widget, unsigned long);
static void UnhighlightSegment(Widget);
static Boolean GetColorPixels(Widget, Pixel *, Pixel *, Pixel *);

					    /* class procedures     */

static void BorderHighlight(Widget);
static void BorderUnhighlight(Widget);
static void ClassInitialize(void);
static void Destroy(Widget);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static XtGeometryResult
	    QueryGeom(Widget, XtWidgetGeometry *, XtWidgetGeometry *);
static void Realize(Widget, XtValueMask *, XSetWindowAttributes *);
static void Redisplay(Widget, XEvent *, Region);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);

					    /* action procedures        */

static void  BtnHandler(Widget, XEvent *, String *, Cardinal *);
static void  HandleSelect(Widget);
static void  KeyHandler(Widget, XEvent *, String *, Cardinal *);
static void  TimerProc(XtPointer, XtIntervalId *);
static void  TraversalHandler(Widget, XEvent *, String *, Cardinal *);
static void  PutSegmentInView(Widget, ExmHyperSegment *);

                    /* public procedures        */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

#define K_MOVE_PREV	'p'
#define K_MOVE_NEXT	'n'

#define K_PAGE_UP	'0'
#define K_PAGE_DOWN	'1'
#define K_PAGE_LEFT	'2'
#define K_PAGE_RIGHT	'3'
#define K_TOP		'4'
#define K_BOT		'5'
#define K_LEFT_EDGE	'6'
#define K_RIGHT_EDGE	'7'
#define K_SELECT	'8'

static _XmConst char
translations[] = "\
~c ~s ~a ~m<Btn1Down>:BtnHandler()\n\
~c ~s ~a ~m<Btn1Up>:BtnHandler()\n\
~s ~m ~a<Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfSelect:KeyHandler(8)\n\
~s ~m ~a<Key>space:KeyHandler(8)\n\
<Key>osfHelp:PrimitiveHelp()\n\
s ~m ~a<Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a<Key>Tab:PrimitiveNextTabGroup()\n\
<Key>osfUp:TraversalHandler(p)\n\
<Key>osfLeft:TraversalHandler(p)\n\
<Key>osfDown:TraversalHandler(n)\n\
<Key>osfRight:TraversalHandler(n)\n\
~c ~s ~a ~m<Key>osfPageUp:KeyHandler(0)\n\
~c ~s ~a ~m<Key>osfPageDown:KeyHandler(1)\n\
~s ~a ~m<Key>osfPageUp:KeyHandler(2)\n\
~s ~a ~m<Key>osfPageDown:KeyHandler(3)\n\
~c ~s ~a ~m<Key>osfPageLeft:KeyHandler(2)\n\
~c ~s ~a ~m<Key>osfPageRight:KeyHandler(3)\n\
~c ~s ~a ~m<Key>osfBeginLine:KeyHandler(4)\n\
~c ~s ~a ~m<Key>osfEndLine:KeyHandler(5)\n\
~s ~a ~m<Key>osfBeginLine:KeyHandler(6)\n\
~s ~a ~m<Key>osfEndLine:KeyHandler(7)\n\
<FocusIn>:PrimitiveFocusIn()\n\
<FocusOut>:PrimitiveFocusOut()\n\
<Unmap>:PrimitiveUnmap()";

static XtActionsRec actions[] = {
	{ "TraversalHandler",	TraversalHandler },
	{ "KeyHandler",		KeyHandler },
	{ "BtnHandler",		BtnHandler },
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define offset(f) XtOffset(ExmHyperTextWidget, f)

static XtResource resources[] = {
	{ XmNbackground, XmCTextBackground, XmRPixel, sizeof(Pixel),
	  offset(core.background_pixel), XmRString, XtDefaultBackground },

	{ XmNborderWidth, XmCBorderWidth, XmRDimension, sizeof(Dimension),
	  offset(core.border_width), XmRImmediate, (XtPointer)0 },

		/* Note that - 0 cols and/or 0 rows mean that the width
		 * and/or height value(s) will be computed from it parent.
		 */
	{ XmNrows, XmCRows, XmRUnsignedChar, sizeof(unsigned char),
	  offset(hyper_text.rows), XmRImmediate, (XtPointer)12 },

	{ XmNcolumns, XmCColumns, XmRUnsignedChar, sizeof(unsigned char),
	  offset(hyper_text.cols), XmRImmediate, (XtPointer)40 },

	{ XmNcacheString, XmCCacheString, XmRBoolean, sizeof(Boolean),
	  offset(hyper_text.cache_string), XmRImmediate, (XtPointer)False },

	{ XmNdiskSource, XmCDiskSource, XmRBoolean, sizeof(Boolean),
	  offset(hyper_text.disk_source), XmRImmediate, (XtPointer)False },

	{ XmNstring, XmCString, XmRString, sizeof(String),
	  offset(hyper_text.string), XmRString, (XtPointer)NULL },

        { XmNfocusColor, XmCFocusColor, XmRPixel, sizeof(Pixel),
          offset(hyper_text.focus_color), XmRString, (XtPointer)"Red" },

	{ XmNkeyColor, XmCForeground, XmRPixel, sizeof(Pixel),
	  offset(hyper_text.key_color), XmRString, (XtPointer)HIGHLIGHT_COLOR },

        { XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList *),
          offset(hyper_text.font_list), XtRImmediate, (XtPointer)NULL },

	{ XmNselect, XmCCallback, XmRCallback, sizeof(XtPointer),
	  offset(hyper_text.callbacks), XmRCallback, (XtPointer)NULL },

	{ XmNnavigationType, XmCNavigationType, XmRNavigationType,
	  sizeof(unsigned char), offset(primitive.navigation_type),
	  XmRImmediate, (XtPointer)XmTAB_GROUP },
	{ XmNtopRow, XmCTopRow, XtRInt, sizeof(int),
	  offset(hyper_text.top_row), XmRImmediate, (XtPointer)0 },
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

ExmHyperTextClassRec exmHyperTextClassRec = {
 {
 (WidgetClass) &xmPrimitiveClassRec,/* superclass            */
 "ExmHyperText",                    /* class_name            */
 sizeof(ExmHyperTextRec),           /* size                  */
 ClassInitialize,                   /* class_initialize      */
 NULL,                              /* class_part_initialize */
 False,                             /* class_inited          */
 Initialize,                        /* initialize            */
 NULL,                              /* initialize_hook       */
 Realize,                  	    /* realize               */
 actions,                           /* actions               */
 XtNumber(actions),                 /* num_actions           */
 resources,                         /* resources             */
 XtNumber(resources),               /* resource_count        */
 NULLQUARK,                         /* xrm_class             */
 True,                              /* compress_motion       */
 XtExposeCompressMultiple | XtExposeGraphicsExposeMerged, /*compress_exposure*/
 True,                              /* compress_enterleave   */
 False,                             /* visible_interest      */
 Destroy,                           /* destroy               */
 NULL,                              /* resize                */
 Redisplay,                         /* expose                */
 SetValues,                         /* set_values            */
 NULL,                              /* set_values_hook       */
 XtInheritSetValuesAlmost,          /* set_values_almost     */
 NULL,                              /* get_values_hook       */
 NULL,              		    /* accept_focus          */
 XtVersion,                         /* version               */
 NULL,                              /* callback_private      */
 translations,             	    /* tm_table              */
 QueryGeom,                         /* query_geometry        */
 NULL,				    /* display_accelerator   */
 NULL		    		    /* extension	     */
},     /* End of CoreClass fields initialization */

{
 BorderHighlight,	            /* border_highlight      */
 BorderUnhighlight,	    	    /* border_unhighlight    */
 NULL,		    	            /* translations	     */
 NULL,				    /* arm_and_activate      */
 NULL,				    /* syn resources	     */
 0,				    /* num syn_resources     */
 NULL,				    /* extension	     */
 },    /* End of XmPrimitive field initializations */

 {
 0,                                 /* field not used        */
 },    /* End of ExmHyperTextClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass exmHyperTextWidgetClass = (WidgetClass)&exmHyperTextClassRec;

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * HighlightSegment - Makes the given segment the current selection.
 * Also unhighlight the previous selection.
 * Input: hs may be NULL (will still do UnhighlightSegment(w)).
 *************************************************************************
*/
static void
HighlightSegment(Widget w, ExmHyperSegment * hs)
{
	if (hs == NULL || HPART(w).highlight == hs)
		return;

	UnhighlightSegment(w);
	PutSegmentInView(w, hs);

	HPART(w).highlight = hs;
	if (XtIsRealized(w))
		DrawFocusSegment(w, hs, False);

} /* end of HighlightSegment */


/*
 ****************************procedure*header*****************************
 * UnhighlightSegment - Unhighlights the current segment selection.
 *************************************************************************
*/
static void
UnhighlightSegment(Widget w)
{
	ExmHyperSegment * hs;

	hs = HPART(w).highlight;
	HPART(w).highlight = NULL;

	if (hs == NULL)
		return;

	if (XtIsRealized(w))
		DrawFocusSegment(w, hs, True);
} /* end of UnhighlightSegment */

/*
 ****************************procedure*header*****************************
 * FindByPos - Finds the hypersegment covering the point <x1,y1>.
 * Returns (HyperSegment*) which may be NULL.
 *************************************************************************
*/
static ExmHyperSegment *
FindByPos(Widget w, WidePosition x1, WidePosition y1)
{
	int			i;
	ExmHyperLine *		hl;
	ExmHyperSegment *	hs;

		/* do adjustment */
	x1 += HPART(w).x_offset;
	y1 += HPART(w).y_offset;

	/* the first line to be drawn */
	i = (y1 >= (int)(TB_MARGIN(w))) ?
			(y1 - (int)(TB_MARGIN(w))) /
				(int)(HPART(w).line_height) : -1;

	/* skip lines */
	for (hl = HPART(w).first_line; hl && i > 0; hl = hl->next, i--);

	if (hl == NULL)
		return(NULL);

	for (hs = hl->first_segment; hs; hs = hs->next) {
		if (((int)(hs->x) <= x1) &&
		    ((int)(hs->x) + (int)(hs->w) - 1) >= x1)
 			return(hs);
	}
	return(NULL);
} /* end of FindByPos */

/*
 *************************************************************************
 *
 * Private Procedures - actions for translation manager.
 *
 ***************************private*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * TimerProc -
 *************************************************************************
*/
static void
TimerProc(XtPointer client_data, XtIntervalId * id)
{
	Widget		w = (Widget)client_data;
	Window		ignored_win;
	int		ignored_int, x, y;
	unsigned int	mask;

	ExmHyperSegment *last_hs;
	ExmHyperSegment *next_hs;

	last_hs = HPART(w).highlight;

		/* should we check mask? */
	XQueryPointer(XtDisplay(w), XtWindow(w), &ignored_win, &ignored_win,
			&ignored_int, &ignored_int, &x, &y, &mask);

	next_hs = FindByPos(w, (WidePosition)x, (WidePosition)y);

	if (last_hs != next_hs)
	{
		UnhighlightSegment(w);
		if (next_hs && next_hs->key)
			HighlightSegment(w, next_hs);
	}

	HPART(w).timer_id = ADD_TIMER(TimerProc, w, (unsigned long)0);
} /* end of TimerProc */

/*
 ****************************procedure*header*****************************
 * HsbOrVsbCB -
 *************************************************************************
*/
static void
HsbOrVsbCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget			   wanted = (Widget)client_data;
	XmScrollBarCallbackStruct *cd = (XmScrollBarCallbackStruct *)call_data;

	ExmSWinHandleValueChange(
		wanted,
		w,				/* H/V XmScrollBarWidget */
		HPART(wanted).gc,
		cd->value,
		X_UOM(wanted), Y_UOM(wanted),
		&HPART(wanted).x_offset,	/* current offset values */
		&HPART(wanted).y_offset);
	HPART(wanted).top_row = cd->value;
} /* end of HsbOrVsbCB */

/*************** widget class procedures ***********************/

/*****************************procedure*header*****************************
 * ClassInitialize -
 */
static void
ClassInitialize(void)
{
    void *handle;
    void *font_proc;

#ifdef USE_FONT_OBJECT
    handle = dlopen(NULL, RTLD_LAZY);
    font_proc = dlsym(handle, "XmAddDynamicFontListClassExtension");
    if (font_proc)
	XmAddDynamicFontListClassExtension(exmHyperTextWidgetClass);
    dlclose(handle);
#endif
}					/* end of ClassInitialize() */


/*
 ****************************procedure*header*****************************
 *  Initialize - Initialize procedure.
 *************************************************************************
*/
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal * num_args)
{
	Dimension	req_wd, req_hi;
	Pixel		foreground, background, key_color;

	if (HPART(new).font_list == NULL)
	{
		HPART(new).font_list = _XmGetDefaultFontList(
						new, XmLABEL_FONTLIST);
	}

	/* make a local copy of the font list, see Label.c:Initialize */
	HPART(new).font_list = XmFontListCopy(HPART(new).font_list);

	INIT_FONT_HI_WD(new); /* calc line_height and tab_width */

	/* get the color pixels for the current palette */
	if (GetColorPixels(new, &key_color, &foreground, &background)) {
		CPART(new).background_pixel = background;
		PPART(new).foreground = foreground;
		HPART(new).key_color = key_color;
	}

	HPART(new).gc = (GC)NULL;
	TreatScratchGC(new, (unsigned long)(GCStipple | GCFont|GCBackground));

	HPART(new).highlight = NULL;
	HPART(new).timer_id = (XtIntervalId)0;

	ExmSWinCreateScrollbars(
		new,
		&HPART(new).hsb,	/* H/V XmScrollBarWidgets */
		&HPART(new).vsb,
		&HPART(new).x_offset,	/* initial offset values */
		&HPART(new).y_offset,
		HsbOrVsbCB);

	SetLine(new, (Widget)NULL);
	if (!HPART(new).vsb)	/* not in a scrolled window */
	{
		req_wd = req_hi = 0;
	}
	else
	{
			/* add the padding if cols and/or rows is/are set */
		if (HPART(new).cols)
			req_wd = HPART(new).cols * X_UOM(new) +
							2 * SIDE_MARGIN(new);

		if (HPART(new).rows)
			req_hi = HPART(new).rows * Y_UOM(new) +
							2 * TB_MARGIN(new);
	}

	Layout(new, /* do adjust size */ False, req_wd, req_hi);
} /* end of Initialize */

/*
 ****************************procedure*header*****************************
 *  Destroy - Destroy procedure.
 *************************************************************************
*/
static void
Destroy(Widget w)
{
	HyperLineFree(HPART(w).first_line);
	HPART(w).first_line = NULL;
	HPART(w).last_line = NULL;
	HPART(w).highlight = NULL;

	if (HPART(w).font_list != NULL) XmFontListFree(HPART(w).font_list);

	XFreeGC(XtDisplay(w), HPART(w).gc);
	if (HPART(w).string)
		XtFree(HPART(w).string);

} /* end of Destroy */

/*
 ****************************procedure*header*****************************
 *  Realize -
 *************************************************************************
*/
static void
Realize(Widget w, XtValueMask * mask, XSetWindowAttributes * attrs)
{
	*mask |= CWBitGravity;
	attrs->bit_gravity = NorthWestGravity;
#define SUPERCLASS      \
	((ExmHyperTextClassRec *)exmHyperTextClassRec.core_class.superclass)

        (*SUPERCLASS->core_class.realize)(w, mask, attrs);

#undef SUPERCLASS
} /* end of Realize */

/*
 ****************************procedure*header*****************************
 *  Redisplay - Expose procedure.
 *************************************************************************
*/
static void
Redisplay(Widget w, XEvent * ev, Region region)
{

	int	X, Y, WD, HI;

	if (ev->type == Expose) {

		X  = ev->xexpose.x;
		Y  = ev->xexpose.y;
		WD = ev->xexpose.width;
		HI = ev->xexpose.height;
	} else if (ev->type == GraphicsExpose) {

		X  = ev->xgraphicsexpose.x;
		Y  = ev->xgraphicsexpose.y;
		WD = ev->xgraphicsexpose.width;
		HI = ev->xgraphicsexpose.height;
	}

	HyperTextExpose(w, (WidePosition)X, (WidePosition)Y,
						(Dimension)WD, (Dimension)HI);

	if (XtIsSensitive(w) == False)
	{
		XGCValues	values;

		values.fill_style = FillStippled;
		values.foreground = CPART(w).background_pixel;
		XChangeGC(XtDisplay(w), HPART(w).gc,
			(unsigned long)(GCFillStyle | GCForeground), &values);
		XFillRectangle(XtDisplay(w), XtWindow(w), HPART(w).gc,
				X, Y, WD, HI);
		XSetFillStyle(XtDisplay(w), HPART(w).gc, FillSolid);
	}
} /* end of Redisplay */

/*
 ****************************procedure*header*****************************
 * QueryGeom - we will handle `resize' when ExmHyperText is a child of
 *	XmScrolledWindow.
 *************************************************************************
*/
static XtGeometryResult
QueryGeom(Widget w, XtWidgetGeometry * req, XtWidgetGeometry * rep)
{
	XtGeometryResult	result = XtGeometryYes;

	if (!(req->request_mode & (CWWidth | CWHeight)))
		return(result);

	if (HPART(w).vsb)
	{
		result = ExmSWinHandleResize(
				w, HPART(w).hsb, HPART(w).vsb,
				(int)0, (int)0,	/* min_x and min_y */
				HPART(w).actual_width, HPART(w).actual_height,
				X_UOM(w), Y_UOM(w),
				&HPART(w).x_offset, &HPART(w).y_offset,
				req, rep);
	}
	return(result);
} /* end of QuerGeom */

/*
 ****************************procedure*header*****************************
 * SetValues - Compares the requested values with the current values
 * and sets them in the new widget.  It returns True if the widget must
 * be redisplayed.
 *************************************************************************
*/
/*
static Boolean
*/
Boolean
SetValues(Widget current, Widget request, Widget new,
	ArgList args, Cardinal * num_args)
{
	Boolean			need_layout = False;
	Boolean			was_changed = False;
	ExmHyperSegment *	hs;
	unsigned long		mask = 0;
	Arg			Sargs[2];

/* MOOLIT: NEED understand the line below */
	UnhighlightSegment(new);
	if (XtIsSensitive(new) != XtIsSensitive(current))
		was_changed = True;

		/* Can't change it in set_values() */
	if ( HPART(new).cache_string != HPART(current).cache_string)
	{
		HPART(new).cache_string = HPART(current).cache_string;
		/* maybe a warning if this goes to public */
	}

#define STR_DIFF(a,b)	( (!(a) && !(b)) ? 0 : (a && b) ? strcmp((a), (b)) : 1)


	if ( HPART(new).disk_source != HPART(current).disk_source ||
	     STR_DIFF(HPART(new).string, HPART(current).string) )
	{
		/* we have to reconstruct the hyper line field */
		SetLine(new, current);
		HPART(new).highlight = NULL;
		HPART(new).top_row = 0;
		need_layout = True;
	 	was_changed = True;
	}
#undef STR_DIFF

		/* see if font resource was changed */
	if (HPART(new).font_list != HPART(current).font_list)
	{
			/* Label.c should do the same... */
		XmFontListFree(HPART(current).font_list);
		if (HPART(new).font_list == NULL)	/* set to default */
		{
			HPART(new).font_list = _XmGetDefaultFontList(
							new, XmLABEL_FONTLIST);
		}
		HPART(new).font_list = XmFontListCopy(HPART(new).font_list);

		INIT_FONT_HI_WD(new);

		need_layout = True;
		was_changed = True;
		mask |= GCFont;
	}

	/* see if scroll is needed */
	if (HPART(new).top_row != HPART(current).top_row) {
		int	smax, ssize, max_scroll;

		/* manage it if the vertical scroll bar is not managed */
		if (!XtIsManaged(HPART(new).vsb))
			XtManageChild(HPART(new).vsb);

		ExmSWinHandleValueChange(new, HPART(new).vsb,
			HPART(new).gc, HPART(new).top_row,
			X_UOM(new), Y_UOM(new),
			&HPART(new).x_offset, &HPART(new).y_offset);

		XtSetArg(Sargs[0], XmNmaximum, &smax);
		XtSetArg(Sargs[1], XmNsliderSize, &ssize);
		XtGetValues(HPART(new).vsb, Sargs, 2);
		max_scroll = smax - ssize;

		XtSetArg(Sargs[0], XmNvalue, HPART(new).top_row > max_scroll ?
					max_scroll : HPART(new).top_row);
		XtSetValues(HPART(new).vsb, Sargs, 1);
		was_changed = True;
	}

		/* see if background resource was changed */
	if (CPART(new).background_pixel != CPART(current).background_pixel) {
		mask |= GCBackground;
		was_changed = True;
	}

		/* XmPrimitiveWidget didn't check foreground in set_values() */
	if (PPART(new).foreground != PPART(current).foreground ||
	    HPART(new).key_color != HPART(current).key_color ||
	    HPART(new).focus_color != HPART(current).focus_color) {
		was_changed = True;
	}

	if (mask)
		TreatScratchGC(new, mask);

	if (need_layout)
		Layout(new, /* do adjust size */True,(Dimension)0,(Dimension)0);

		/* If widget currently has focus, assign input focus to
		 * the first command segment for now. NEED to understand this*/
	if (HPART(current).highlight) {
		hs = GetFirstCmdSegment(new);
		if (hs)
			HighlightSegment(new, hs);
	}
		/* Future enhancement: if font was changed s.t. focus segment
		 * is not in view, update view.
		 */
	return(was_changed);
} /* end of SetValues */

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
HyperTextExpose(Widget w, WidePosition x1, WidePosition y1,
						Dimension w1, Dimension h1)
{
	int		i, j, n;
	WidePosition	x2;
	ExmHyperLine *	hl;

	x1 += HPART(w).x_offset;
	y1 += HPART(w).y_offset;

	/* the first line to be drawn */
	i = (y1 >= (int)(TB_MARGIN(w))) ?
			(y1-(int)(TB_MARGIN(w))) /
				(int)(HPART(w).line_height) : 0;

	/* the number of lines to be drawn */
	j = ((y1+(int)h1-1) >= (int)(TB_MARGIN(w))) ?
			(y1+(int)h1-1-(int)(TB_MARGIN(w))) /
				(int)(HPART(w).line_height) : 0;

	n = j - i + 1;

	/* draw line[i] to lines[i+n-1] within the [x1,x1+w1-1] boundary */
	x2 = x1 + w1 - 1;

		/* skip lines */
	for (hl = HPART(w).first_line; hl && i > 0; hl = hl->next, i--);

		/* draw lines */
	for (; hl && n > 0; hl = hl->next, n--)
	{
		register ExmHyperSegment *hs;

			/* skip segments that are at left of x1 */
		for (hs = hl->first_segment;
		     hs && (((int)(hs->x) + (int)(hs->w) - 1) < x1);
		     hs = hs->next)
				;

			/* draw segments that are at left of x2 */
		for (; hs && ((int)hs->x < x2); hs = hs->next)
			DrawSegment(w, hs, False, False);
	}
} /* end of HyperTextExpose */

/*
 ****************************procedure*header*****************************
 * Create one scratch GC to simply the checking in SetValues() and also
 * by assumptions that, create a GC is expense, but not when changing it.
 *************************************************************************
*/
static void
TreatScratchGC(Widget w, unsigned long mask)
{
#define stipple_width 16
#define stipple_height 16
static unsigned char stipple_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa};

	static Pixmap bm = None;

	XGCValues	values;
	XFontStruct *	font;

	if (bm == None)
	{
		bm = XCreatePixmapFromBitmapData(
				XtDisplay(w),
				RootWindowOfScreen(XtScreen(w)),
				(char *)stipple_bits, stipple_width,
				stipple_height,
				0, /* foreground always off */
				1, /* background always on  */
				(unsigned)1);
	}

	if (mask & GCStipple)
		values.stipple = bm;
	if (mask & GCFont)
	{
		GET_FONT(w, font);
		values.font = font->fid;
	}
	if (mask & GCBackground)
		values.background = CPART(w).background_pixel;

	if (HPART(w).gc != (GC)NULL)
	{
		XChangeGC(XtDisplay(w), HPART(w).gc, mask, &values);
	}
	else
	{
		values.graphics_exposures = True; /* for scrolling. */
		mask |= GCGraphicsExposures;
		HPART(w).gc = XCreateGC(
				XtDisplay(w),
				RootWindowOfScreen(XtScreen(w)),
				mask, &values);
	}
} /* end of TreatScratchGC */

/*------------ layout hyper text positions -------------*/

/*
 ****************************procedure*header*****************************
 * SetLine - Frees existing hyperline of current widget (i.e. if this is
 * called from SetValues()).  Constructs the hyper line for the new widget.
 *************************************************************************
*/
static void
SetLine(Widget new, Widget cur)
{
	ExmHyperLine *	hl;

	HPART(new).first_line = NULL;
	HPART(new).last_line = NULL;

		/* Call XtFree() only when `string' is cached by me,
		 * and free all hyper lines if there is any... */
	if (cur)
	{
		if (HPART(cur).cache_string && HPART(cur).string)
			XtFree((XtPointer)HPART(cur).string);

		HyperLineFree(HPART(cur).first_line);
		HPART(cur).first_line = NULL;
		HPART(cur).last_line = NULL;
		HPART(cur).highlight = NULL;

		if (HPART(new).misc_flag & NOTIFY_NEW_STRING)
			HPART(new).misc_flag |= GOT_NEW_STRING;
	}

	if (!HPART(new).string)
		return;

	/* either init the hyper line from the file or from the string */
	if (HPART(new).disk_source)	/* FILE */
		HPART(new).first_line = ExmHyperLineFromFile(
						new, HPART(new).string);
	else				/* STRING */
		HPART(new).first_line = ExmHyperLineFromString(
						new, HPART(new).string);

#define STR_DUP(str)         ((str) ? strdup(str) : NULL)

		/* Call strdup() only when `string' is cached by me */
	if (HPART(new).cache_string)
		HPART(new).string = STR_DUP(HPART(new).string);
	else	/* reset it to NULL after parsing it */
		HPART(new).string = (String)NULL;

#undef STR_DUP

	/* Find the last hyper line and also set the ptr to the
	 * previous hyperline along the way.  Would be more efficient
	 * to set this up as the text is read in but this will do
	 * for now.
	 */
	HPART(new).first_line->prev = NULL;
	hl = HPART(new).first_line->next;

	if (hl == NULL) {
		HPART(new).last_line = HPART(new).first_line;
	} else {
		ExmHyperLine *save = hl;

		hl->prev = HPART(new).first_line;
		hl = hl->next;
		while (hl) {
			hl->prev = save;
			save = hl;
			hl = hl->next;
			if (hl == NULL)
				HPART(new).last_line = save;
		}
	}
} /* end of SetLine */

/*
 ****************************procedure*header*****************************
 * Layout - 
 *
 *	y      _________________________
 *	y_text _________________________<- space above text
 *			XXX
 *			X X             <- ascent
 *	       ________ XXX ____________
 *		 ------   X ----------- <- underline
 *			X X             <- descent
 *	       ________ XXX ____________
 *	       _________________________<- space below text
 *
 *************************************************************************
*/
static void
Layout(Widget w, Boolean adjust_size_flag, Dimension req_wd, Dimension req_hi)
{
	WideDimension	hi, wd;
	ExmHyperLine *	hl;

	wd = SIDE_MARGIN(w);
	hi = TB_MARGIN(w);

#define MAX(X, Y)	(((X) > (Y)) ? (X) : (Y))

	for (hl = HPART(w).first_line; hl ; hl = hl->next)
	{
		WideDimension	tmp;

		tmp = LayoutLine(
			w, hl, HPART(w).line_height, HPART(w).tab_width, hi);

		wd  = MAX(wd, tmp);
		hi += HPART(w).line_height;
	}

#undef MAX

		/* (wd, hi) are the actual size... */
	wd += SIDE_MARGIN(w);

		/* when the HyperText widget is in a scrolled window,
		 * top/bottom margins are not useful specially UOM is
		 * going to based on `line_hi' in vertical direction. */
	hi += TB_MARGIN(w);

	HPART(w).actual_width = wd;
	HPART(w).actual_height = hi;

		/* now figure out the view size if it's in a scrolled win */
	if (HPART(w).vsb)
	{
		(void)ExmSWinCalcViewSize(
			w,		/* child of XmScrolledWindowWidget */
			HPART(w).hsb, HPART(w).vsb,
			X_UOM(w), Y_UOM(w),
			req_wd, req_hi,
			(int)0, (int)0,	/* min_x and min_y */
			&HPART(w).x_offset, &HPART(w).y_offset,
			&wd, &hi	/* actual window size */
		);
	}

	if (adjust_size_flag && !HPART(w).vsb)
	{
		AdjustSize(w, wd, hi);
	}
	else
	{
		if ((CPART(w).width = wd) < 0)
			CPART(w).width = 1;

		if ((CPART(w).height = hi) < 0)
			CPART(w).height = 1;

	}
} /* end of Layout */

/*
 ****************************procedure*header*****************************
 * LayoutLine - returns the width required for displaying the line.
 *************************************************************************
*/
static WideDimension
LayoutLine(Widget w, ExmHyperLine * hl,
		Dimension line_height, Dimension tab_width, Dimension y)
{
	register ExmHyperSegment *	hs;
	WidePosition			x;

	x = SIDE_MARGIN(w);

#define WP	WidePosition
	for (hs = hl->first_segment; hs; hs = hs->next) {
		if (hs->tabs > 0)
			x  = ((x - (WP)SIDE_MARGIN(w) + ((WP)hs->tabs *
				(WP)tab_width)) / (WP)tab_width) *
				(WP)tab_width + (WP)SIDE_MARGIN(w);
#undef WP

		hs->x = x;
		hs->y = (WidePosition)y;

			/* performance hit here if it's not in C locale */
		CALC_FONT_WD(hs->w, w, hs->text, hs->len);

		hs->h = line_height;
		x += hs->w;
	}
	return((WideDimension)x);
} /* end of LayoutLine */

/*
 ****************************procedure*header*****************************
 * AdjustSize - assumption here is that, checking for width/height are
 *	already done and this routine just sets up the info and makes
 *	the geometry request.
 *************************************************************************
*/
static void
AdjustSize(Widget w, Dimension width, Dimension height)
{
	XtGeometryResult	p_result;
	XtWidgetGeometry	p_request;
	XtWidgetGeometry	p_reply;

	p_request.request_mode = 0;

	if (CPART(w).width != width)
	{
		p_request.request_mode |= CWWidth;
		p_request.width = width;
	}

	if (CPART(w).height != height)
	{
		p_request.request_mode |= CWHeight;
		p_request.width = height;
	}

	if ((p_result = XtMakeGeometryRequest(w, &p_request,
					&p_reply)) == XtGeometryAlmost)
	{
			/* we should re-calc view size if this's
			 * happening..., same applies to FGraph */
		p_request = p_reply;
		p_result = XtMakeGeometryRequest(
					w, &p_request, &p_reply);
	}
} /* end of AdjustSize */

/*
 ****************************procedure*header*****************************
 * DrawSegment -
 *************************************************************************
*/
static void
DrawSegment(Widget w, ExmHyperSegment *hs,
		Boolean clear_flag /* do XClearArea? */,
		Boolean busy_flag  /* make text segment busy? */)
{
	Screen *	screen = XtScreen(w);
	Display *	dpy = XtDisplay(w);
	Window		win = XtWindow(w);
	XGCValues	values;
	Boolean		draw_string = False;
	Pixel		fg;
	int		hx, hy;

	if (hs->len == 0)
		return;

	hx = hs->x - HPART(w).x_offset;
	hy = hs->y - HPART(w).y_offset;

	if (clear_flag == True) {
		XClearArea(dpy, win, hx, hy, hs->w, hs->h, False);
	}

		/* GCStipple/GCFont/GCBackground are set in TreatScratchGC(),
		 * we have to set others before the drawing... */
	if (busy_flag) {
		XSetFillStyle(dpy, HPART(w).gc, FillOpaqueStippled);
		XFillRectangle(dpy, win, HPART(w).gc, hx, hy, hs->w, hs->h);
		XSetFillStyle(dpy, HPART(w).gc, FillSolid);
	} else if (hs->reverse_video) { /* dot-underline terms */
		XSetForeground(dpy, HPART(w).gc, PPART(w).foreground);
		DRAW_STRING(dpy, w, hs, hx, hy);

		values.foreground = IsColor(screen) ? HPART(w).key_color :
						      HPART(w).focus_color;
		values.line_style = LineOnOffDash;
		values.dashes = 2;
		XChangeGC(dpy, HPART(w).gc,
			(unsigned long)(GCForeground | GCLineStyle |
							GCDashList), &values);
		XDrawLine(dpy, win, HPART(w).gc,
			hx, Y_TEXT(w, hy) + 2,
			hx + hs->w - 1, Y_TEXT(w, hy) + 2);
		values.line_style = LineSolid;
		values.dashes = 4;
		XChangeGC(dpy, HPART(w).gc,
			(unsigned long)(GCLineStyle | GCDashList), &values);

	} else if (hs->key || hs->shell_cmd) { /* draw links */
		/* display links in reverse video on monochrome display */
		if (IsColor(screen) == False)
		{
			XSetForeground(dpy, HPART(w).gc, PPART(w).foreground);
			XFillRectangle(dpy, win, HPART(w).gc,
					hx, hy + 1, hs->w, hs->h - 3);
			fg = CPART(w).background_pixel;
		}
		else
		{
			fg = HPART(w).key_color;
		}
		draw_string = True;
	} else {
			/* draw non-command segments */
		fg = PPART(w).foreground;
		draw_string = True;
	}

	if (draw_string)
	{
		XSetForeground(dpy, HPART(w).gc, fg);
		/* place program link in a box */
		if (hs->shell_cmd)
			XDrawRectangle(dpy, XtWindow(w), HPART(w).gc,
				hx, hy-1, hs->w, hs->h);
		DRAW_STRING(dpy, w, hs, hx, hy);
	}

	/* draw current focus segment */
	if (hs == HPART(w).highlight)
		DrawFocusSegment(w, hs, False);

	if (busy_flag)		/* why this call */
		XSync(dpy, 0);
} /* end of DrawSegment */

/*
 ****************************procedure*header*****************************
 * Underlines with input focus color an item which has just gained focus
 * and un-underline an item which has just lost focus.
 *
 * Called from HighlightSegment() and UnhighlightSegment().
 *************************************************************************
*/
static void
DrawFocusSegment(Widget w, ExmHyperSegment *hs, Boolean unhi_lite)
{
	Display *	dpy = XtDisplay(w);
	Window		win = XtWindow(w);
	Screen *	screen = XtScreen(w);
	XGCValues	values;
	unsigned long	mask = 0;
	int		hx, hy;

	if (hs->len == 0)
		return;

	hx = hs->x - HPART(w).x_offset;
	hy = hs->y - HPART(w).y_offset;

	if (unhi_lite)
	{
		XSetForeground(dpy, HPART(w).gc, CPART(w).background_pixel);
		XDrawLine(dpy, win, HPART(w).gc,
			hx, Y_TEXT(w, hy) + 2,
			hx + hs->w - 1, Y_TEXT(w, hy) + 2);

			/* redraw dotted line */
		if (hs->reverse_video == True)
		{
			values.foreground = IsColor(screen) ?
				HPART(w).key_color : PPART(w).foreground;
			values.line_style = LineOnOffDash;
			values.dashes = 2;
			mask = GCForeground | GCLineStyle | GCDashList;
		}
	}
	else
	{
		if (IsColor(screen))
		{
			values.foreground = HPART(w).focus_color;
		}
		else
		{
#define BLACK           BlackPixelOfScreen(screen)
#define WHITE           WhitePixelOfScreen(screen)

			if (CPART(w).background_pixel == BLACK)
				values.foreground = WHITE;
			else
				values.foreground = BLACK;
#undef BLACK
#undef WHITE
		}
		mask = GCForeground;
	}

	if (mask)
	{
		XChangeGC(dpy, HPART(w).gc, mask, &values);
		XDrawLine(dpy, win, HPART(w).gc,
			hx, Y_TEXT(w, hy) + 2,
			hx + hs->w - 1, Y_TEXT(w, hy) + 2);
		if (mask & GCLineStyle)
		{
			values.line_style = LineSolid;
			values.dashes = 4;
			mask = GCLineStyle | GCDashList;
			XChangeGC(dpy, HPART(w).gc, mask, &values);
		}
	}
} /* end of DrawFocusSegment */

/*
 ****************************procedure*header*****************************
 * HandleSelect - Make selected segment look busy, invoke select callback,
 * then unbusy the segment if string is not changed (i.e. the selected
 * segment still exists).
 *************************************************************************
*/
static void
HandleSelect(Widget w)
{
	ExmHyperSegment		hs_rec;
	ExmHyperSegment *	old_hs;

	HPART(w).misc_flag = NOTIFY_NEW_STRING;
	old_hs = HPART(w).highlight;
	hs_rec = *HPART(w).highlight;
	DrawSegment(w, &hs_rec, False, True);

	if (XtHasCallbacks(w, XmNselect) == XtCallbackHasSome)
		XtCallCallbacks(w, XmNselect, (XtPointer)&hs_rec);

		/* Replaced with a new string? */
	if (!(HPART(w).misc_flag & GOT_NEW_STRING))
	{
			/* Assume that app writers can't destroy hyper_segments
			 * or hyper_lines, otherwise, we are in trouble! */
		DrawSegment(w, old_hs, True, False);
	}

	HPART(w).misc_flag = 0;
} /* end of HandleSelect */

/*
 ****************************procedure*header*****************************
 * BtnHandler -
 *************************************************************************
*/
static void
BtnHandler(Widget w, XEvent * xe, String * params, Cardinal * n_params)
{
	ExmHyperSegment *	hs;

		/* Locate the selected segment */
	hs = FindByPos(w, (WidePosition)xe->xbutton.x,
					(WidePosition)xe->xbutton.y);

	if (xe->type == ButtonPress)
	{
		if (hs && hs->key)
			HighlightSegment(w, hs);

		HPART(w).timer_id = ADD_TIMER(TimerProc, w, INITIAL_DELAY);
	}
	else
	{
		RM_TIMER(HPART(w).timer_id);

		if (hs && hs->key)
		{
			ExmHyperSegment *	hs1;

			hs1 = HPART(w).highlight;
			if (hs1 != hs)
				UnhighlightSegment(w);

				/* grey on */
			HPART(w).highlight = hs;
			HandleSelect(w);
		}
	}
} /* end of BtnHandler */

/*
 ****************************procedure*header*****************************
 * KeyHandler - handler SELECT_KEY...
 *************************************************************************
*/
static void
KeyHandler(Widget w, XEvent * xe, String * params, Cardinal * n_params)
{
	Widget		this_widget;
	String		this_action = NULL;
	String		this_params[1];
	Cardinal	this_n_params;

	if (*n_params != 1 || xe->type != KeyPress)
		return;

	switch(*params[0]) {

		/* no need to check if segment is a command segment
		 * - it has to be.  Show busy visual.
		 */
	case K_SELECT:
		HandleSelect(w);
		break;
	case K_PAGE_UP:		/* FALL THROUGH */
	case K_PAGE_LEFT:
		if (HPART(w).hsb) {

			this_widget = *params[0] == K_PAGE_UP ? HPART(w).vsb :
								HPART(w).hsb;
			this_action = "PageUpOrLeft";
			this_params[0] = *params[0] == K_PAGE_UP ? "0" : "1";
			this_n_params = 1;
		}
		break;
	case K_PAGE_DOWN:	/* FALL THROUGH */
	case K_PAGE_RIGHT:
		if (HPART(w).hsb) {

			this_widget = *params[0] == K_PAGE_DOWN ? HPART(w).vsb :
								  HPART(w).hsb;
			this_action = "PageDownOrRight";
			this_params[0] = *params[0] == K_PAGE_DOWN ? "0" : "1";
			this_n_params = 1;
		}
		break;
	case K_TOP:		/* FALL THROUGH */
	case K_BOT:		/* FALL THROUGH */
	case K_LEFT_EDGE:	/* FALL THROUGH */
	case K_RIGHT_EDGE:	/* FALL THROUGH */
		if (HPART(w).hsb) {

			this_widget = (*params[0] == K_TOP ||
				       *params[0] == K_BOT) ? HPART(w).vsb :
							      HPART(w).hsb;
			this_action = "TopOrBottom";
			this_params[0] = NULL;
			this_n_params = 0;
		}
		break;
	}

	if (this_action && XtIsManaged(this_widget)) {
		XtCallActionProc(this_widget, this_action, xe,
						this_params, this_n_params);
	}
} /* end of KeyHandler */

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
static void
TraversalHandler(Widget w, XEvent * xe, String * params, Cardinal * n_params)
{
	ExmHyperLine *	 hl;
	ExmHyperSegment *hs;
	Boolean		 hit = False;	/* hit current focus segment */

	if (*n_params != 1 || xe->type != KeyPress)
	{
		return;
	}
	switch(*params[0]) {
	case K_MOVE_PREV:

		/* Scan each segment in each line for the highlighted segment
		 * starting from the last segment in a hyper line.  Once the
		 * current segment with focus is found, turn the flag "hit"
		 * on and start looking for the previous command segment.
		 * Keep backtracking until the previous command segment is
		 * found or until the beginning of the text is reached.
		 */
		for (hl = HPART(w).last_line; hl; hl = hl->prev)
		{
			for (hs = hl->last_segment; hs; hs = hs->prev)
			{
				if (hit && hs->key != NULL)
					break;
				else if (hs == HPART(w).highlight)
				{
					hit = True;
					continue;
				}
			}
			if (hit && hs->key != NULL)
				break;
		}

		/* if hs is still NULL then try the last command segment */
		if (!hs)
		{
			for (hl = HPART(w).last_line; hl; hl = hl->prev)
			{
				for (hs = hl->last_segment; hs; hs = hs->prev)
				{
					if (hs->key != NULL)
						break;
				}
				if (hs->key != NULL)
					break;
			}
		}

		if (hs)
			HighlightSegment(w, hs);
		break;
	case K_MOVE_NEXT:

		/* Scan each segment in each line for the highlighted segment.
		 * Then look for the next command segment, which could be
		 * either in the same line as the current segment with focus
		 * or on another line. The flag "hit" is used to indicate
		 * whether we've scanned past the current segment with focus. 
		 */
		for (hl = HPART(w).first_line; hl; hl = hl->next)
		{
			for (hs = hl->first_segment; hs; hs = hs->next)
			{
				if (hit && hs->key != NULL)
					break;
				else if (hs == HPART(w).highlight)
				{
					hit = True;
					continue;
				}
			}
			if (hit && hs->key != NULL)
				break;
		}

		/* if hs is still NULL then try the first command segment */
		if (!hs)
			hs = GetFirstCmdSegment(w);

		if (hs)
			HighlightSegment(w, hs);
		break;
	default:
		break;
	}
} /* end of TraversalHandler */

/*
 ****************************procedure*header*****************************
 * BorderHighlight-
 *************************************************************************
*/
static void
BorderHighlight(Widget w)
{
		/* highlight the first segment with hs->key != NULL */
	HighlightSegment(w, GetFirstCmdSegment(w));
} /* end of BorderHighlight*/

/*
 ****************************procedure*header*****************************
 * BorderUnhighlight-
 *************************************************************************
*/
static void
BorderUnhighlight(Widget w)
{
		/* find current segment with focus */
	if (HPART(w).highlight)
		UnhighlightSegment(w);
} /* end of BorderUnhighlight */

/*
 ****************************procedure*header*****************************
 * GetFirstCmdSegment - Returns the ptr to the first cmd segment.
 *************************************************************************
*/
static ExmHyperSegment *
GetFirstCmdSegment(Widget w)
{
	ExmHyperLine *		hl;
	ExmHyperSegment *	hs;

	for (hl = HPART(w).first_line; hl; hl = hl->next) {
		for (hs = hl->first_segment; hs; hs = hs->next) {
			if (hs->key != NULL)
				return(hs);
		}
	}
	return(NULL);
} /* end of GetFirstCmdSegment */

/*
 ****************************procedure*header*****************************
 * HyperSegmentFree -
 *************************************************************************
*/
static void
HyperSegmentFree(ExmHyperSegment * hs)
{
	if (hs == NULL)
		return;

	if (hs->text)
		XtFree((XtPointer)hs->text);

	if (hs->key)
		XtFree((XtPointer)hs->key);

	if (hs->script)
		XtFree((XtPointer)hs->script);

	hs->text = hs->key = hs->script = NULL;
	free(hs);

} /* end of HyperSegmentFree */

/*
 ****************************procedure*header*****************************
 * HyperLineFree - Frees resources allocated for a HyperLine.
 *************************************************************************
*/
static void
HyperLineFree(ExmHyperLine * hl)
{
	ExmHyperLine *		hl1;
	ExmHyperSegment *	hs;
	ExmHyperSegment *	hs1;

	if (hl == NULL)
		return;

	for (; hl; hl = hl1) {

		hl1 = hl->next;

		for (hs = hl->first_segment; hs; hs = hs1) {
			hs1 = hs->next;
			HyperSegmentFree(hs);
		}
		hl->first_segment = NULL;
		hl->last_segment = NULL;
		XtFree((XtPointer)hl);
	}
} /* end of HyperLineFree */

/* 
 **************************************************************************
	GetColorPixels - Gets the pixel for highlighting in the current
	palette.  Returns NULL on failure.
 **************************************************************************
 */
static Boolean
GetColorPixels(Widget w, Pixel *select, Pixel *foreground, Pixel *background)
{
#ifdef USE_COLOR_OBJECT
	short active, inactive, primary, secondary;
	int colorUse;
	PixelSet pPixelData[NUM_COLORS];

	/*
	 * Hard code the index to the pixel set for XmText widget
	 * class for now. Waiting for the CDE ColorObj code to provide
	 * an interface to specify or request pixel set for a widget class.
	 */
	short text_index = 3;

	/*
       	 * ASSUMPTION:  If _XmGetPixelData() returns true,
	 *  we have a good color server at our disposal.
	 */
	if (_XmGetPixelData(DefaultScreen(XtDisplay(w)), &colorUse, &pPixelData,
				&active, &inactive, &primary, &secondary)) {
		*select = pPixelData[text_index].sc;
		*foreground = pPixelData[text_index].fg;
		*background = pPixelData[text_index].bg;
		return (True);
	}
#endif
	return (False);

} /* end of GetColorPixels */

/****************************procedure*header*****************************
 * PutSegmentInView-
 */
static void
PutSegmentInView(Widget w, ExmHyperSegment *info)
{
#define X_CHANGED	(1L << 0)
#define Y_CHANGED	(1L << 1)
#define UPPER_X		(HPART(w).x_offset)
#define UPPER_Y		(HPART(w).y_offset)
#define LOWER_X		(UPPER_X + (WidePosition)CPART(w).width)
#define LOWER_Y		(UPPER_Y + (WidePosition)CPART(w).height)
#define ITEM_UX		(info->x)
#define ITEM_UY		(info->y)
#define ITEM_LX		(ITEM_UX + (WidePosition)info->w)
#define ITEM_LY		(ITEM_UY + (WidePosition)info->h)
#define IN_RANGE(L,M,H)	((L) < (M) && (M) < (H))

	Arg		args[1];
	Boolean		u_in, l_in, round_up;
	WidePosition	x_offset, y_offset;
	unsigned long	offset_changed = 0;

	int		tmp_val, max_val, slider_size, value;

    if (!HPART(w).vsb) /* in SWin? (after initialization) */
	return;

    if (XtIsManaged(HPART(w).vsb)) { /* Determine new y_offset */

	u_in = IN_RANGE(UPPER_Y, ITEM_UY, LOWER_Y);
	l_in = IN_RANGE(UPPER_Y, ITEM_LY, LOWER_Y);

	if (u_in && l_in) {		/* in view */

		y_offset = HPART(w).y_offset;
		round_up = False;		/* no-op */
	} else if (info->h >= CPART(w).height) { /* taller than view hi */

		y_offset = ITEM_UY;
		round_up = True;
	} else if (!u_in && !l_in) {	/* completely out of view */

		if (ITEM_LY <= UPPER_Y) { 	       /* above view */

			y_offset = ITEM_UY;
			round_up = True;
		} else {			       /* below view */

			y_offset = HPART(w).y_offset + ITEM_UY -
						LOWER_Y + info->h;
			round_up = False;
		}
	} else if (u_in) {		/* upper corner in view but not lower */

		if ((tmp_val = LOWER_Y - ITEM_UY - info->h) < 0)
			tmp_val = -tmp_val;
		y_offset = HPART(w).y_offset + tmp_val;
		round_up = False;
	} else {			/* lower corner in view but not upper */
		y_offset = ITEM_UY;
		round_up = True;
	}

	if ((tmp_val = y_offset % (int)Y_UOM(w))) {
		if (!round_up)
			y_offset += ((int)Y_UOM(w) - tmp_val);
		else
			y_offset -= tmp_val;
	}

	if (y_offset != HPART(w).y_offset) {

		offset_changed |= Y_CHANGED;

		max_val = (int)HPART(w).actual_height / (int)Y_UOM(w);
		if ((int)HPART(w).actual_height % (int)Y_UOM(w))
			max_val++;

		slider_size	= (int)CPART(w).height / (int)Y_UOM(w);
		value		= (int)y_offset / (int)Y_UOM(w);

		if (value > (tmp_val = max_val - slider_size)) {
			value = tmp_val;
			y_offset = tmp_val * Y_UOM(w);
		}

		HPART(w).y_offset = y_offset;
		XtSetArg(args[0], XmNvalue, value);
		XtSetValues(HPART(w).vsb, args, 1);
	}
    }


    if (XtIsManaged(HPART(w).hsb)) { /* Determine new x_offset */

	u_in = IN_RANGE(UPPER_X, ITEM_UX, LOWER_X);
	l_in = IN_RANGE(UPPER_X, ITEM_LX, LOWER_X);

	if (u_in && l_in) {		/* in view */

		x_offset = HPART(w).x_offset;
		round_up = False;		/* no-op */
	} else if (info->w >= CPART(w).width) { /* wider than view width */

		x_offset = ITEM_UX;
		round_up = True;
	} else if (!u_in && !l_in) {	/* completely out of view */

		if (ITEM_LX <= UPPER_X) { 	     /* left view */

			x_offset = ITEM_UX;
			round_up = True;
		} else {			     /* right view */

			x_offset = HPART(w).x_offset + ITEM_UX -
						LOWER_X + info->w;
			round_up = False;
		}
	} else if (u_in) {		/* left corner in view but not right */

		if ((tmp_val = LOWER_X - ITEM_UX - info->w) < 0)
			tmp_val = -tmp_val;
		x_offset = HPART(w).x_offset + tmp_val;
		round_up = False;
	} else {			/* right corner in view but not left */

		x_offset = ITEM_UX;
		round_up = True;
	}

	if ((tmp_val = x_offset % (int)X_UOM(w))) {
		if (!round_up)
			x_offset += ((int)X_UOM(w) - tmp_val);
		else
			x_offset -= tmp_val;
	}

	if (x_offset != HPART(w).x_offset) {

		offset_changed |= X_CHANGED;

		max_val = (int)HPART(w).actual_width / (int)X_UOM(w);
		if ((int)HPART(w).actual_width % (int)X_UOM(w))
			max_val++;

		slider_size	= (int)CPART(w).width / (int)X_UOM(w);
		value		= (int)x_offset / (int)X_UOM(w);

		if (value > (tmp_val = max_val - slider_size)) {

			value = tmp_val;
			x_offset = tmp_val * X_UOM(w);
		}

		HPART(w).x_offset = x_offset;
		XtSetArg(args[0], XmNvalue, value);
		XtSetValues(HPART(w).hsb, args, 1);
	}
    }

    /* Force an expose if offset was changed */
    if (offset_changed)
	XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
			CPART(w).width, CPART(w).height, True);

}
