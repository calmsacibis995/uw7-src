#ifndef NOIDENT
#ident	"@(#)statictext:StaticText.c	1.86"
#endif

/*
 *************************************************************************
 *
 * Description:	Static Text widget.  
 *
 *******************************file*header*******************************
 */

/*************************************<+>*************************************
 *****************************************************************************
 **
 **		File:        StaticText.c
 **
 **		Project:     X Widgets
 **
 **		Description: Code/Definitions for StaticText widget class.
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 **   
 *****************************************************************************
 *************************************<+>*************************************/

/*
 * Include files & Static Routine Definitions
 */

#include <stdio.h>
#include <ctype.h>

#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/keysymdef.h>

#include <Xol/OpenLookP.h>
#include <Xol/StaticTexP.h>
#include <Xol/LayoutExtP.h>

#define ClassName StaticText
#include <Xol/NameDefs.h>

/*
 * Private types:
 */

#define CORE_P(w) ((Widget)(w))->core

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void CurrentDimensions OL_ARGS((
	StaticTextWidget		stw,
	String			fmt_str,
	Dimension *		width,
	Dimension *		height
));
static void FormatText OL_ARGS((
	StaticTextWidget	stw,
	Dimension	win_width,
	Dimension *	height
));
static Dimension HeightInPixels OL_ARGS((
	StaticTextWidget	stw,
	Dimension		numLines
));
static Dimension QueryStringHeight  OL_ARGS((
	StaticTextWidget	stw,
	Dimension		width
));
static void	Layout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static void ProcessBackslashes OL_ARGS((
	char *	output,
	char *	input
));
static void SetUpGCs OL_ARGS((
	StaticTextWidget	stw
));
static void StAddToLineTable OL_ARGS((
	StaticTextWidget	stw,
	Cardinal		line_number,
	String			start_of_line,
	int			line_len
));
static void ValidateInputs OL_ARGS((
	StaticTextWidget	stw
));

       /* dynamically linked private procedures */
static void (*_olmSTHighlightSelection) OL_ARGS((StaticTextWidget, Boolean));


					/* class procedures		*/
static void             ClassInitialize OL_NO_ARGS();
static void             Destroy OL_ARGS((Widget));
static void             Initialize
                                OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static void              Realize
                        OL_ARGS((Widget, Mask *, XSetWindowAttributes *));
static void              Redisplay OL_ARGS((Widget, XEvent *, Region));
static Boolean           SetValues
                        OL_ARGS((Widget, Widget, Widget, ArgList, Cardinal *));

					/* action procedures		*/

					/* public procedures		*/



/*************************************<->*************************************
 *
 *
 *	Description:  default translation table for class: StaticText
 *	-----------
 *
 *	Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/

static char defaultTranslations [] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<BtnMotion>:	OlAction()";

/*************************************<->*************************************
 *
 *
 *	Description:  action list for class: StaticText
 *	-----------
 *
 *	Matches string descriptors with internal routines.
 *
 *************************************<->***********************************/
/* this widget doesn't have an action list */

static OlEventHandlerRec event_procs[] =
{
	{ ButtonPress,		NULL	},	  /* See ClassInitialize */
	{ ButtonRelease,	NULL	},	  /* See ClassInitialize */
	{ MotionNotify,	NULL	}    /* See ClassInitialize */
};

/*************************************<->*************************************
 *
 *
 *	Description:  resource list for class: StaticText
 *	-----------
 *
 *	Provides default resource settings for instances of this class.
 *	To get full set of default settings, examine resouce list of super
 *	classes of this class.
 *
 *************************************<->***********************************/

static OLconst char two_points[] = "2 points";

static XtResource resources[] = {
	{ XtNhSpace,
		XtCHSpace,
		XtRDimension,
		sizeof(Dimension),
		XtOffset(StaticTextWidget, static_text.internal_width),
		XtRString,
		(XtPointer) two_points
	},
	{ XtNvSpace,
		XtCVSpace,
		XtRDimension,
		sizeof(Dimension),
		XtOffset(StaticTextWidget, static_text.internal_height),
	        XtRString,
	        (XtPointer) two_points
	},

	{ XtNalignment,
		XtCAlignment,
		XtROlDefine,
		sizeof(OlDefine),
		XtOffset(StaticTextWidget,static_text.alignment),
		XtRImmediate,
		(XtPointer) OL_LEFT
	},
	  
	{ XtNdragCursor, 
	  XtCCursor, 
	  XtRCursor, 
	  sizeof(Cursor),
 	  XtOffset(StaticTextWidget,static_text.dragCursor),
	  XtRString,
	  (XtPointer) NULL
	  },

	{ XtNgravity,
		XtCGravity,
		XtRGravity,
		sizeof(int),
		XtOffset(StaticTextWidget,static_text.gravity),
		XtRImmediate,
		(XtPointer) CenterGravity
	},
	{ XtNwrap,
		XtCWrap,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget,static_text.wrap),
		XtRImmediate,
		(XtPointer) True
	},
	{ XtNstrip,
		XtCStrip,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget,static_text.strip),
		XtRImmediate,
		(XtPointer) True
	},
	{ XtNlineSpace,
		XtCLineSpace,
		XtRInt,
		sizeof(int),
		XtOffset(StaticTextWidget,static_text.line_space),
		XtRImmediate,
		(XtPointer) 0
	},
	{ XtNstring,
		XtCString,
		XtRString,
		sizeof(char *),
		XtOffset(StaticTextWidget, static_text.input_string),
		XtRString,
		NULL
	},
	{ XtNrecomputeSize,
		XtCRecomputeSize,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget,static_text.recompute_size),
		XtRImmediate,
		(XtPointer)True
	},
	{ XtNbackground,
		XtCBackground,
		XtRPixel,
		sizeof(Pixel),
		XtOffset(StaticTextWidget, core.background_pixel),
		XtRString,
		XtDefaultBackground
	},
	{ XtNtraversalOn,
		XtCTraversalOn,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(StaticTextWidget, primitive.traversal_on),
		XtRImmediate,
		(XtPointer)False
	},
};

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************class*extension*records*************************
 */

static LayoutCoreClassExtensionRec layout_extension_rec = {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlLayoutCoreClassExtensionVersion,      /* version              */
        sizeof(LayoutCoreClassExtensionRec),    /* record_size          */
	Layout,					/* layout		*/
	NULL					/* query_alignment	*/
};


/*************************************<->*************************************
 *
 *
 *	Description:  global class record for instances of class: StaticText
 *	-----------
 *
 *	Defines default field settings for this class record.
 *
 *************************************<->***********************************/

StaticTextClassRec statictextClassRec = {
	{ /* core_class fields */
	/* superclass            */	(WidgetClass) &(primitiveClassRec),
	/* class_name            */	"StaticText",
	/* widget_size           */	sizeof(StaticTextRec),
	/* class_initialize      */	ClassInitialize,
	/* class_part_initialize */	NULL,
	/* class_inited          */	FALSE,
	/* initialize            */	Initialize,
	/* initialize_hook       */	NULL,
	/* realize               */	Realize,
	/* actions               */	NULL,
	/* num_actions           */	0,
	/* resources             */	resources,
	/* num_resources         */	XtNumber(resources),
	/* xrm_class             */	NULLQUARK,
	/* compress_motion       */	TRUE,
	/* compress_exposure     */	TRUE,
	/* compress_enterleave   */	TRUE,
	/* visible_interest      */	FALSE,
	/* destroy               */	Destroy,
	/* resize                */	XtInheritResize,
	/* expose                */ 	Redisplay,
	/* set_values            */	SetValues,
	/* set_values_hook       */	NULL,
	/* set_values_almost     */	XtInheritSetValuesAlmost,
	/* get_values_hook       */	NULL,
	/* accept_focus          */	XtInheritAcceptFocus,
	/* version               */	XtVersion,
	/* callback private      */	NULL,
	/* tm_table              */	defaultTranslations,
	/* query_geometry        */	XtInheritQueryGeometry,
	/* display_accelerator   */     XtInheritDisplayAccelerator,
	/* extension             */     (XtPointer)&layout_extension_rec
	},
  {					/* primitive class	*/
      True,				/* focus_on_select	*/
      XtInheritHighlightHandler,	/* highlight_handler	*/
      NULL,				/* traversal_handler	*/
      NULL,				/* register_focus	*/
      NULL,			/* activate	(see ClassInitialize)	*/ 
      event_procs,			/* event_procs		*/
      XtNumber(event_procs),		/* num_event_procs	*/
      OlVersion,			/* version		*/
      NULL				/* extension		*/
  },
	{
		NULL,
	},
};

/*
 *************************************************************************
 * Public Widget Class Definition of the Widget Class Record
 *************************public*class*definition*************************
 */
WidgetClass staticTextWidgetClass = (WidgetClass) &statictextClassRec;
WidgetClass statictextWidgetClass = (WidgetClass) &statictextClassRec;

/*
 *************************************************************************
 * Private Procedures
 ***************************private*procedures****************************
 */


static Dimension
GetTextWidth OLARGLIST((stw, str, len))
	OLARG(StaticTextWidget,		stw)
	OLARG(String,			str)
        OLGRA(int,		len)
{
    StaticTextPart	*stp = &(stw->static_text);
    XFontStruct	*font = stw->primitive.font;
    OlFontList	*font_list = stw->primitive.font_list;

    if (font_list)
	return (Dimension) OlTextWidth(font_list,(unsigned char *)str, len);
    else
	return (Dimension) XTextWidth(font,str, len);
} /* end of GetTextWidth */

				    
/*************************************<->*************************************
 *
 *	static void CurrentDimensions(w, fmt_str, width, height)
 *		StaticTextWidget	w;
 *		String			fmt_str;
 *		Dimension		* width, *height;
 *
 *	Description:
 *	-----------
 *		For a given string and the current font, returns the length of
 *		'\n' delimited series of characters and the height needed to
 *		display the string.
 *
 *	Inputs:
 *	------
 *		w = the static text widget used for font info.
 *		fmt_str =  The formatted string to be analyzed.
 *
 *	Outputs:
 *	-------
 *		width = The maximum length in pixels of the longest '\n'
 *			delimited series of characters.
 *		height = The maximum height in pixels to display the string.
 *
 *************************************<->***********************************/
static void 
CurrentDimensions OLARGLIST((stw, fmt_str, width, height))
	OLARG(StaticTextWidget,		stw)
	OLARG(String,			fmt_str)
	OLARG(Dimension *,		width)
	OLGRA(Dimension *,		height)
{
	StaticTextPart	*stp = &(stw->static_text);
	int	i;
	Dimension	cur = 0;
	char	*str2, *str3;
	int	alignment = stw->static_text.alignment;

	*width = 0;
	*height = 0;

	while(*fmt_str)
	{
		if ((stp->strip) && 
			((alignment == OL_LEFT) || 
			(alignment == OL_CENTER)))
			while (*fmt_str == ' ')
				fmt_str++;
		str2 = fmt_str;
		for(i=0; ((*fmt_str != '\n') && *fmt_str); i++, fmt_str++);
		if ((stp->strip) && 
			((alignment == OL_RIGHT) || 
			(alignment == OL_CENTER)))
		{
			str3 = fmt_str;
			str3--;
			while (*str3 == ' ')
				i--, str3--;
		}
		cur = GetTextWidth(stw, str2,i);
		if (cur > *width)
			*width = cur;

		/* insert ALIGNED line in line_table */
		StAddToLineTable(stw, (*height)++, str2, i);
		if (*fmt_str)  {
			fmt_str++; /* Step past the \n */
		}
	}

	StAddToLineTable(stw, *height, NULL, 0);

	*height = HeightInPixels(stw, *height);
}  /* end of CurrentDimensions() */


/*
 * Strip each line in the line_table according to the alignment
 */
static void 
StripText OLARGLIST((stw))
    OLGRA(StaticTextWidget,		stw)
{
    StaticTextPart* stp = &(stw->static_text);
    int i;
    int	alignment = stp->alignment;
    String fmt_str, eol;

    for (i=0; stp->line_table[i]; i++) {
	fmt_str = stp->line_table[i];
	eol = fmt_str + stp->line_len[i] - 1;
	if ((alignment == OL_LEFT) || (alignment == OL_CENTER)) {
	    while (*fmt_str == ' ')
		fmt_str++;
	}
	if ((alignment == OL_RIGHT) || (alignment == OL_CENTER)) {
	    while (*eol == ' ')
		eol--;
	}
	StAddToLineTable(stw, i, fmt_str, eol-fmt_str+1);
    }
} /* end of StripText */


/*************************************<->*************************************
 *
 *	FormatText
 *
 *	Description:
 *	-----------
 *		Inserts newlines where necessary to fit fmt_string
 *		into win_width (if specified).
 *
 *	Inputs:
 *	------
 *		stw = The StaticText widget to be formatted.
 *		fmt_str = the string to be formatted.
 *		win_width = the width of the window.
 *
 *	Outputs:
 *	-------
 *		height = the preferred height for the formatted string;
 *              line_table = updated according to win_width
 *
 *************************************<->***********************************/
static void 
FormatText OLARGLIST((stw, win_width, height))
	OLARG(StaticTextWidget,	stw)
	OLARG(Dimension,	win_width)
	OLGRA(Dimension *,	height)
{
    int	wordindex, line_cnt = 0;
    Dimension	width;
    char	*str2;
    Boolean	gotone;
    StaticTextPart	*stp = &(stw->static_text);
    String fmt_str = stp->output_string;   

    /* The available text window width is... */
    win_width = win_width - (2 * stp->internal_width)
	- (2 * stw->static_text.highlight_thickness);
    
    while (*fmt_str)
    {
	wordindex = -1;
	gotone = FALSE;
	str2 = fmt_str;  /* start of line */

	while (!gotone)
	{
	    if (stp->strip)
		while (*fmt_str == ' ')
		    fmt_str++;
	    /*
	     *Step through until a character that we can
	     * break on.
	     */
	    while ((*fmt_str != ' ') && (*fmt_str != '\n') && (*fmt_str))
		fmt_str++;
	    
	    wordindex++;
	    width = GetTextWidth(stw, str2, fmt_str - str2);
	    if (width < win_width)
	    {
		/*
		 * If the current string fragment is shorter than the 
		 * available window.  Check to see if we are at a 
		 * forced break or not, and process accordingly.
		 */
		switch (*fmt_str)
		{
		case ' ':
		    /*
		     * Step past the space
		     */
		    fmt_str++;
		    break;
		case '\n':
		    /*
		     * Forced break.  Step pase the \n.
		     */
		    StAddToLineTable(stw, line_cnt++, str2, fmt_str-str2);
		    fmt_str++;
		    gotone = TRUE;
		    break;
		default:
		    /*
		     * End of string.
		     */
		    StAddToLineTable(stw, line_cnt++, str2, fmt_str-str2);
		    gotone = TRUE;
		    break;
		}
	    }
	    else if (width > win_width)
	    {
		/*
		 * We know that we have something
		 */
		gotone = TRUE;
		
		/*
		 * See if there is at least one space to back up for.
		 */
		if (wordindex)
		{
		    fmt_str--;
		    while (*fmt_str != ' ')
			fmt_str--;
		    StAddToLineTable(stw, line_cnt++, str2, fmt_str-str2);
		    fmt_str++;
		}
		else {
		    /*
		     * We have a single word which is too long
		     * for the available window. Let the text
		     * clip rectangle handle it.
		     */
		    StAddToLineTable(stw, line_cnt++, str2, fmt_str-str2);
		    if (*fmt_str)
			fmt_str++;
		}
	    }
	    else /* (width == win_width) */
	    {
		if (*fmt_str) {
		    StAddToLineTable(stw, line_cnt++, str2, fmt_str-str2);
		    fmt_str++;
		    gotone = TRUE;
		}
		else {  /* end of string */
		    StAddToLineTable(stw, line_cnt++, str2, fmt_str-str2);
		    gotone = TRUE;
		}
	    }
	}
    } /* while (*fmt_str) */
    
    StAddToLineTable(stw, line_cnt, NULL, 0); 
    *height = HeightInPixels(stw, line_cnt);
    
    if (stp->strip)
	StripText(stw);
}  /* end of FormatText() */


static Dimension
HeightInPixels OLARGLIST((stw, numLines))
    OLARG(StaticTextWidget, stw)
    OLGRA(Dimension, numLines)
{
    Dimension height;
    Dimension fheight;
    OlFontList	*font_list = stw->primitive.font_list;
    XFontStruct	*font = stw->primitive.font;

    /*  Calculate the height of the string.  The number of lines is
	in height, now convert it to pixels taking into account the
	line spacing.  */
    fheight = (Dimension)OlFontHeight(font, font_list);
    height = (fheight * numLines) + (Dimension)
	((numLines - (Dimension)1) * (Dimension)stw->static_text.line_space * 
	 fheight) / 100;
    
    return(height);

}  /* end of HeightInPixels() */

static Dimension
QueryStringHeight  OLARGLIST((stw, width))
	OLARG(StaticTextWidget,	stw)
	OLGRA(Dimension,	width)
{
	String string;
	Dimension height;

	FormatText(stw, width, &height);
	height += (2 * stw->static_text.internal_height) +
			(2* stw->static_text.highlight_thickness);
	return(height);
}  /* end of QueryStringHeight() */

/**
 ** Layout()
 **/
/* ARGSUSED */
static void
Layout OLARGLIST((w, resizable, query_only, cached_best_fit_hint,
	who_asking, request, response))
	OLARG(Widget,			w)
	OLARG(Boolean,			resizable)
	OLARG(Boolean,			query_only)
	OLARG(Boolean,			cached_best_fit_hint)
	OLARG(Widget,			who_asking)
	OLARG(XtWidgetGeometry *,	request)
	OLGRA(XtWidgetGeometry *,	response)
{
	XtWidgetGeometry	preferred, reply;
	StaticTextWidget stw =	(StaticTextWidget) w;
	Dimension	string_width, string_height;
	Dimension	core_width, core_height;
	Dimension	side_width, side_height;
	StaticTextPart	*stp = &(stw->static_text);

	/*
	 * Compute optimum fit based on the string.
	 */
	CurrentDimensions(stw, stp->output_string, &string_width,
					&string_height);
	if (stp->recompute_size ||
		(! stp->recompute_size && stw->core.width == 0))
		preferred.width = string_width + (2 * stp->internal_width) +
					(2 * stp->highlight_thickness);
	else
		preferred.width = stw->core.width;

	if (stp->recompute_size ||
		(!stp->recompute_size && stw->core.height == 0))
		preferred.height = string_height + (2 * stp->internal_height) +
					(2 * stp->highlight_thickness);
	else
		preferred.height = stw->core.height;

	if ( query_only == True && stp->recompute_size )  {
		/*  When the Parent asks for the optimal size, it could
		    be giving a specific dimension to change.
		    With the current formatting routine, it is only
		    easy to check for specific widths.  */
		if (request && request->request_mode & CWWidth)  {
			/*  A specific width has been queried.  */
			if (stp->wrap) {
				if (preferred.width != request->width)  {
					preferred.height = QueryStringHeight(
						stw, request->width);
					preferred.width = request->width;
				}
                	}
		}
	}

	/* Protect against zero size. */
	if (preferred.width == 0)
		preferred.width = 1;
	if (preferred.height == 0)
		preferred.height = 1;
	/*
	 * If we are allowed to resize, ask our parent if we can
	 * get the optimum size.
	 */
	if (resizable) {
	    if ( !query_only
	     && (   preferred.width  != CORE_P(w).width
		 || preferred.height != CORE_P(w).height
		)
	    ) {
		XtGeometryResult	ans;

		preferred.request_mode = (CWWidth|CWHeight);
		ans = XtMakeGeometryRequest(w, &preferred, &reply);
		if (ans == XtGeometryAlmost)  {
			/*  We will honor the width in the Almost, and ask
			    for a different height.  */
			preferred.width = reply.width;
			preferred.height = QueryStringHeight(stw, preferred.width);
			ans = XtMakeGeometryRequest (w, &preferred, &reply);
			if (ans == XtGeometryAlmost)  {
				/*  Accept it at this point. */
				preferred.width = reply.width;
				preferred.height = reply.height;
				XtMakeGeometryRequest(w, &preferred, &reply);
			}
		}
		preferred.width = reply.width;
		preferred.height = reply.height;
	   }
	}

	/*
	 * Skip the rest of this if our parent is just asking.
	 */
	if (who_asking == w) {
		if (response) {
			response->x            = CORE_P(w).x;
			response->y            = CORE_P(w).y;
			response->width        = preferred.width;
			response->height       = preferred.height;
			response->border_width = CORE_P(w).border_width;
		}
		return;
	}

	/*
	 * Place the string in the space given in w->core.
	 */
	stp->TextRect.width = string_width;
	stp->TextRect.height = string_height;

	core_width = stw->core.width;
	/*
	 * We were requested with a specific width.  We must
	 * fit ourselves to it.
	 *
	 * The maximum available window width is...
	 */
	if (core_width != 0) {
		Dimension maxwin;

		maxwin = core_width - (2 * stp->internal_width)
			- (2 * stw->static_text.highlight_thickness);
		if (stp->wrap) {
			if (string_width != maxwin)  {
				stp->TextRect.width = maxwin; 
				FormatText(stw, 
					maxwin, &(stp->TextRect.height));
			}
        	}
		else if (string_width > maxwin)
			stp->TextRect.width = maxwin;
	}

	/*  Now calculate the x coordinate of the TextRect based on
	    the gravity resource.  */

	side_width = stp->highlight_thickness + stp->internal_width;

	switch (stp->gravity)  {
		case EastGravity:
		case NorthEastGravity:
		case SouthEastGravity:
			if ((int) core_width >
				(int) (side_width + stp->TextRect.width))  {
				stp->TextRect.x = (Position) core_width -
					side_width - stp->TextRect.width;
				break;
			}
			/*  else  fall through to just side_width  */
			/* FALLTHROUGH */
		case WestGravity:
		case NorthWestGravity:
		case SouthWestGravity:
			stp->TextRect.x = side_width;
			break;
		case CenterGravity:
		default:
			if ((int) (core_width - (2*side_width)) >
						(int)stp->TextRect.width)
				stp->TextRect.x = (Position)
					(core_width - stp->TextRect.width) / 2;
			else
				stp->TextRect.x = side_width;
			break;
	}

	/*
	 * See how tall the string wants to be, truncate the rectangle if
	 * necessary.
	 */
	side_height = stp->internal_height + stp->highlight_thickness;
	core_height = stw->core.height;

	if (core_height != 0)  {
		stp->TextRect.height = _OlMin((int) stp->TextRect.height,
			(int)(core_height - (2 * stp->internal_height)
			- (2 * stw->static_text.highlight_thickness)));
	}

	/*  Set the TextRect y coordinate according to gravity.  */
	switch (stp->gravity)  {
		case SouthGravity:
		case SouthWestGravity:
		case SouthEastGravity:
			if ((int)core_height >
				(int) (side_height + stp->TextRect.height))  {
				stp->TextRect.y = (Position) core_height -
					side_height - stp->TextRect.height;
				break;
			}
			/*  else  fall through to just side_width  */
			/* FALLTHROUGH */
		case NorthGravity:
		case NorthWestGravity:
		case NorthEastGravity:
			stp->TextRect.y = side_height;
			break;
		case CenterGravity:
		default:
			if ((int)(core_height - (2*side_height)) >
						(int) stp->TextRect.height)
				stp->TextRect.y = (Position)
					(core_height - stp->TextRect.height) / 2;
			else
				stp->TextRect.y = side_height;
			break;
	}

/*
 * the clip rectangle is used for border spacing and to truncate text
 */
	if (XtIsRealized((Widget)stw))  {
		XSetClipRectangles(XtDisplay((Widget)stw), stp->normal_GC,
			0, 0, &(stp->TextRect), 1, Unsorted);

		XSetClipRectangles(XtDisplay((Widget)stw), stp->hilite_GC,
			0, 0, &(stp->TextRect), 1, Unsorted);
	}

	return;
} /* end of Layout() */


static void 
ProcessBackslashes OLARGLIST((output,input))
	OLARG(char *,	output)
	OLGRA(char *,	input)
{
	char *out, *in;

	out = output;
	in = input;
	while(*in)
		if (*in == '\\')
		{
			in++;
			switch (*in)
			{
				case 'n':
					*out++ = '\n';
				break;
				case 't':
					*out++ = '\t';
				break;
				case 'b':
					*out++ = '\b';
				break;
				case 'r':
					*out++ = '\r';
				break;
				case 'f':
					*out++ = '\f';
				break;
				default:
					*out++ = '\\';
					*out++ = *in;
				break;
			}
			in++;
		}
		else
			*out++ = *in++;
	*out = '\0';
}  /* end of ProcessBackslashes() */


/*************************************<->*************************************
 *
 * static void SetUpGCs (stw)
 *	StaticTextWidget stw;
 *
 *	Description:
 *	-----------
 *		Sets up GCs for the static text widget to write to.
 *		One normal, one highlighted.
 *
 *	Inputs:
 *	------
 *		stw = Points to an instance structure which has primitive.foreground
 *	   	   core.background_pixel and primitive.font appropriately
 *	   	   filled in with values.
 *
 *	Outputs:
 *	-------
 *		Initializes static_text.normal_GC, .hilite_GC
 *
 *	Procedures Called
 *	-----------------
 *		XCreateGC
 *		XSetClipRectangles
 *
 *************************************<->***********************************/
static void
SetUpGCs OLARGLIST((stw))
	OLGRA(StaticTextWidget,	stw)
{
	XGCValues	values;
	XtGCMask	valueMask;

	/*
	** Normal GC
	*/
	valueMask = GCForeground | GCBackground | GCFont | GCFunction;

	if (stw->primitive.font_color != (Pixel) -1) {
		values.foreground = stw->primitive.font_color;
	}
	else {
		values.foreground = stw->primitive.foreground;
	}
	values.background = stw->core.background_pixel;
	values.font	= stw->primitive.font->fid;
	values.function	= GXcopy;

	stw->static_text.normal_GC = XCreateGC(XtDisplay(stw),XtWindow(stw),
		valueMask,&values);
	XSetClipRectangles(XtDisplay(stw),stw->static_text.normal_GC,
		0,0, &(stw->static_text.TextRect), 1, Unsorted);
	/*
	** Highlighted GC
	*/
	valueMask = GCForeground | GCBackground | GCFont | GCFunction;

	values.foreground = stw->core.background_pixel;
	if (stw->primitive.font_color != (Pixel) -1)
		values.background = stw->primitive.font_color;
	else
		values.background = stw->primitive.foreground;

	values.font	= stw->primitive.font->fid;
	values.function	= GXcopy;

	stw->static_text.hilite_GC = XCreateGC(XtDisplay(stw),XtWindow(stw),
		valueMask,&values);
	XSetClipRectangles(XtDisplay(stw),stw->static_text.hilite_GC,
		0,0, &(stw->static_text.TextRect), 1, Unsorted);
}  /* end of SetUpGCs() */


static void
StAddToLineTable OLARGLIST((stw, line_number, start_of_line, line_len))
	OLARG(StaticTextWidget,	stw)
	OLARG(Cardinal,		line_number)
	OLARG(String,		start_of_line)
	OLGRA(int,		line_len)
{
	StaticTextPart	*stp = &(stw->static_text);

	if (line_number >= stp->line_count-1)  {
		stp->line_count += stp->line_count;
		stp->line_table =  (char **)
				XtRealloc ((char *)stp->line_table, 
					(stp->line_count * sizeof(char *)));
		stp->line_len =  (int *)
				XtRealloc ((char *)stp->line_len, 
					(stp->line_count * sizeof(char *)));
	}
	stp->line_table[line_number] = start_of_line;
	stp->line_len[line_number] = line_len;

}  /* end of StAddToLineTable() */


static void
ValidateInputs OLARGLIST((stw))
	OLGRA(StaticTextWidget,	stw)
{
	StaticTextPart	*stp = &(stw->static_text);

	/* Check internal spacing */
	if (stp->internal_height < 0)
	{
	  OlVaDisplayWarningMsg(XtDisplay((Widget)stw),
				OleNinvalidDimension,
				OleTbadHeight,
				OleCOlToolkitWarning,
				OleMinvalidDimension_badHeight,
				XtName((Widget)stw),
				OlWidgetToClassName((Widget)stw));
		stp->internal_height = 0;
	}
	if (stp->internal_width < 0)
	{
	  OlVaDisplayWarningMsg(XtDisplay((Widget)stw),
				OleNinvalidDimension,
				OleTbadWidth,
				OleCOlToolkitWarning,
				OleMinvalidDimension_badWidth,
				XtName((Widget)stw),
				OlWidgetToClassName((Widget)stw));
		stp->internal_width = 0;
	}
	
	/* 
	 *Check line_spacing.  We will allow text to over write,
	 * we will not allow it to move from bottom to top.
	 */
	if (stp->line_space < -100)
	{
	  OlVaDisplayWarningMsg(XtDisplay((Widget)stw),
				OleNinvalidResource,
				OleTsetToDefault,
				OleCOlToolkitWarning,
				OleMinvalidResource_setToDefault,
				XtName((Widget)stw),
				OlWidgetToClassName((Widget)stw),
				XtNlineSpace);
		stp->line_space = 0;
	}

	/*
	 * Check Alignment
	 */
	
	switch(stp->alignment) {
		case OL_LEFT:
		case OL_CENTER:
		case OL_RIGHT:
			/* Valid values. */
			break;
		default:
			OlVaDisplayWarningMsg(XtDisplay((Widget)stw),
					      OleNinvalidResource,
					      OleTsetToSomething,
					      OleCOlToolkitWarning,
					      OleMinvalidResource_setToSomething,
					      XtName((Widget)stw),
					      OlWidgetToClassName((Widget)stw),
					      XtNalignment,
					      "OL_LEFT");
			stp->alignment = OL_LEFT;
			break;
	}
#ifdef new
	switch (stp->gravity)  {
	case CenterGravity:
	case NorthGravity:
	case SouthGravity:
	case EastGravity:
	case WestGravity:
	case NorthEastGravity:
	case SouthEastGravity:
	case NorthWestGravity:
	case SouthWestGravity:
	    /* Valid values. */
	    break;
	default:
	    OlVaDisplayWarningMsg(XtDisplay((Widget)stw),
				  OleNinvalidResource,
				  OleTsetToSomething,
				  OleCOlToolkitWarning,
				  OleMinvalidResource_setToSomething,
				  XtName((Widget)stw),
				  OlWidgetToClassName((Widget)stw),
				  XtNgravity,
				  "CenterGravity");
	    stp->gravity = CenterGravity;
	    break;
	}
#endif
}  /* end of ValidateInputs() */


/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 *  ClassInitialize
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void 
ClassInitialize()
{
    StaticTextClassRec *wc = (StaticTextClassRec *) staticTextWidgetClass;
    
    layout_extension_rec.record_type = XtQLayoutCoreClassExtension;

    _OlAddOlDefineType ("center", OL_CENTER);
    _OlAddOlDefineType ("left",   OL_LEFT);
    _OlAddOlDefineType ("right",  OL_RIGHT);
    
    /* dynamically link in the GUI dependent routines */
    OLRESOLVESTART
    OLRESOLVE(STActivateWidget, wc->primitive_class.activate)
    OLRESOLVE(STHandleButton, event_procs[0].handler)
    OLRESOLVE(STHandleMotion, event_procs[2].handler)
    OLRESOLVEEND(STHighlightSelection, _olmSTHighlightSelection)
			    
    event_procs[1].handler = event_procs[0].handler; 

} /* end of ClassInitialize() */

/*
 *************************************************************************
 *  Destroy
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void 
Destroy OLARGLIST((w))
	OLGRA(Widget, 	w)
{
	StaticTextWidget stw = (StaticTextWidget) w;

	XtFree(stw->static_text.input_string);
	XtFree((char *)stw->static_text.output_string);
	XtFree((char *)stw->static_text.line_table);
	XtFree((char *)stw->static_text.line_len);
	if (XtIsRealized(w))
	{
		XFreeGC(XtDisplay(w),stw->static_text.normal_GC);
		XFreeGC(XtDisplay(w),stw->static_text.hilite_GC);
	}
	if ( stw->static_text.clip_contents)
	    XtFree ( (char *)stw->static_text.clip_contents);


}  /* end of Destroy() */

/*
 *************************************************************************
 *  Initialize
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((req, new, args, num_args))
    OLARG(Widget,               req)
    OLARG(Widget,               new)
    OLARG(ArgList,              args)
    OLGRA(Cardinal *,           num_args)
{
	char *s;
	StaticTextWidget newstw = (StaticTextWidget) new;
	StaticTextPart *	newstp = &(newstw->static_text);
	

	if (newstp->input_string != NULL)  {
		/*
		 * Copy the input string into local space.
		 */
		s = XtMalloc(_OlStrlen(newstp->input_string)+1);
		ProcessBackslashes(s,newstp->input_string);
		newstp->input_string = s;
	}
	else  {
		s = XtMalloc(1);
		*s = 0;
		newstp->input_string = s;
	}

	ValidateInputs(newstw);
	newstp->output_string =
			XtNewString(newstp->input_string);

 	newstp->line_table = (char **) XtMalloc(5 * sizeof(char *));
 	newstp->line_len = (int*) XtMalloc(5 * sizeof(char *));
 	newstp->line_count = 5;

 	newstp->selection_mode = 0;
 	newstp->selection_start= (char *) NULL;
 	newstp->selection_end= (char *) NULL;
 	newstp->oldsel_start= (char *) NULL;
 	newstp->oldsel_end= (char *) NULL;
  	newstp->clip_contents= (char *) NULL;
 	newstp->ev_x = 0;
 	newstp->ev_y = 0;
 	newstp->old_x = 0;
 	newstp->old_y = 0;
 	newstp->highlight_thickness = 0;
	newstp->transient = (Atom) NULL;
	newstp->is_dragging = False;

	OlSimpleLayoutWidget(new, True, False);

}  /* end of Initialize() */

/*
 *************************************************************************
 *  Realize
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void 
Realize OLARGLIST((w, valueMask, attributes))
        OLARG(Widget, w)
        OLARG(Mask *, valueMask)
        OLGRA(XSetWindowAttributes *, attributes)
{

	(* (statictextClassRec.core_class.superclass->core_class.realize))
		(w, valueMask, attributes);

	SetUpGCs((StaticTextWidget) w);
}  /* end of Realize() */


/*
 *************************************************************************
 *  Redisplay
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void 
Redisplay OLARGLIST((w, xevent, region))
	OLARG(Widget,	w)
	OLARG(XEvent *, xevent)
	OLGRA(Region,	region)
{
	StaticTextWidget stw = (StaticTextWidget) w;
 	XEvent	junk_event;
 
	/* look for multiple exposes */
	while (XCheckWindowEvent(	XtDisplay(stw),
					XtWindow(stw),
					ExposureMask,
					&junk_event)) ;
	if (stw->static_text.line_table[0]) {
		_OlSTDisplaySubstring(	stw,
			      stw->static_text.line_table[0],
			      stw->static_text.line_table[0] +
			      strlen(stw->static_text.line_table[0]),
			      stw->static_text.normal_GC);
		(*_olmSTHighlightSelection)(stw, True);
	}
}  /* end of Redisplay() */


/*
 *************************************************************************
 *  SetValues
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
    OLARG(Widget,       current)
    OLARG(Widget,       request)
    OLARG(Widget,       new)
    OLARG(ArgList,      args)
    OLGRA(Cardinal *,   num_args)
{
	StaticTextWidget curstw = (StaticTextWidget) current;
	StaticTextWidget reqstw = (StaticTextWidget) request;
	StaticTextWidget newstw = (StaticTextWidget) new;
	Boolean	redisplay	= False;
	Boolean do_layout	= False;
	char	*s;
	StaticTextPart *	newstp = &(newstw->static_text);
	StaticTextPart *	curstp = &(curstw->static_text);

	if (newstp->input_string != curstp->input_string)  {
		if (strcmp(newstp->input_string, curstp->input_string) == 0) {
			newstp->input_string = curstp->input_string;
		}
		else  {
			do_layout = True;
			/*
			 * Copy the input string into local space.
			 */
			if (newstp->input_string == NULL)  {
				s = XtMalloc(1);
				*s = 0;
				}
			else
				s = XtMalloc(_OlStrlen(newstp->input_string)+1);
			ProcessBackslashes(s,newstp->input_string);
			/*
		 	* Deallocate the old string.
		 	*/
			XtFree(curstp->input_string);
			/*
		 	* Have everybody point to the new string.
		 	*/
			newstp->input_string = s;
			curstp->input_string = s;
			reqstw->static_text.input_string = s;
	
			/*  Make a copy of the new string to format. */
			XtFree(curstp->output_string);
			newstp->output_string = XtNewString(newstp->input_string);
	
			/*
		 	* clear the selection
		 	*/
 			newstp->selection_mode = 0;
 			newstp->selection_start= (char *) NULL;
 			newstp->selection_end= (char *) NULL;
 			newstp->oldsel_start= (char *) NULL;
 			newstp->oldsel_end= (char *) NULL;
		}
	}

	ValidateInputs(newstw);

#define	DIFFERENT(field)	(newstp->field != curstp->field)

	if ( DIFFERENT(strip) ||
		DIFFERENT(wrap) ||
		DIFFERENT(recompute_size) ||
		DIFFERENT(line_space) ||
		DIFFERENT(alignment) ||
		DIFFERENT(gravity) ||
		DIFFERENT(internal_height) ||
		DIFFERENT(internal_width) ||
		(reqstw->primitive.font != curstw->primitive.font) ||
		(reqstw->primitive.font_list != curstw->primitive.font_list))
	{
#undef DIFFERENT
/*
**	If we're allowed to recompute our size, set height/width to zero
**	to indicate this.
*/
		do_layout = True;
		redisplay = True;
	}

	if ((newstw->primitive.font_color != curstw->primitive.font_color) ||
	    (newstw->core.background_pixel != curstw->core.background_pixel) ||
	    (newstw->primitive.font != curstw->primitive.font)) {
		if (XtIsRealized((Widget)newstw)) {
			XFreeGC(XtDisplay(newstw),newstp->normal_GC);
			XFreeGC(XtDisplay(newstw),newstp->hilite_GC);
			SetUpGCs(newstw);

			/* WORKAROUND XtSetValues problem: new GC's are placed
			 * in 'new' widget but we may get called (ie. in Resize)
			 * with the 'current' widget.
			 */
			curstp->normal_GC = newstp->normal_GC;
			curstp->hilite_GC = newstp->hilite_GC;
		}
		redisplay = True;
	}
	
	/*  Layout procedure is only called at the end of the SetValues
	    chain to avoid extra calls.  */
	OlLayoutWidgetIfLastClass(new, staticTextWidgetClass, do_layout, False);

	/*  A slight problem with LayoutIfLastClass:  The new widget's
	    geometry is updated and has gone through all of the geometry
	    management that it needs.  XtSetValues will compare the new
	    geometry to the old geometry and go through the geometry
	    management all over again.  Usually this is harmless, but in
	    the case of the ControlArea as a parent, the x and y have
	    changed during the layout, and doing a geoemtry request
	    with different x and y values is always rejected by the
	    ControlArea.  This causes the actual state of the window that
	    was resized in the layout to get out of sync with the widget
	    size.  As a fix, the current widget's geometry is set to the
	    new widget's geometry to avoid the geoemtry management in
	    XtSetValues. A more general fix should be applied to 
	    LayoutWidgetIfLastClass to pass in the current widget and
	    modify the geometry as we do here. */
	
	if (do_layout)  {
		current->core.width = new->core.width;	
		current->core.height = new->core.height;	
		current->core.x = new->core.x;	
		current->core.y = new->core.y;	
		current->core.border_width = new->core.border_width;	
	}

	return(redisplay || do_layout);
}  /* end of SetValues() */

/*
** Given a static text widget and a char * within it, return the 
** (x,y) position of the lower left corner of the specified character.
** Also return the line index into the line_table.
*/

int
_OlSTXYForPosition OLARGLIST((stw,position,px,py))
	OLARG(StaticTextWidget,	stw)
	OLARG(char,	*position)
	OLARG(Position,	*px)
	OLGRA(Position, *py)
{
	XFontStruct	*font;
	OlFontList	*font_list = stw->primitive.font_list;
	XRectangle	*cliprect;
	Dimension	delta;
	int	line;
	StaticTextPart	*stp = &(stw->static_text);

	font = stw->primitive.font;
	cliprect = &(stp->TextRect);
/*
** Figure out which line the char belongs to, and set the Y value.
*/
	for (line = 0; position > (stp->line_table[line]+stp->line_len[line]);
	     line++) {

	    /* if we overflowed the table, then backup */
	    if (stp->line_table[line] == NULL) {
		if (line != 0)
		    line--;
		break;
	    }
	}
	if (font_list)
	        {
	        delta = ((stp->line_space + 100) * 
		         (font_list->max_bounds.descent +
			  font_list->max_bounds.ascent)) / 100;

		*py = cliprect->y + (Position) ((line * delta) + 
		      font_list->max_bounds.ascent);

/*
** And what is the X-coordinate for this specific char?
*/

		*px =	(Position) _OlSTGetOffset(stw, line) +
			OlTextWidth(	font_list, 
					(unsigned char *)stp->line_table[line],
					position - stp->line_table[line]);

		}
	else
		{
		delta = ((stp->line_space + 100) * 
			 (font->descent + font->ascent)) / 100;

		*py = cliprect->y + (Position) ((line * delta) + font->ascent);

/*
** And what is the X-coordinate for this specific char?
*/

		*px =	(Position) _OlSTGetOffset(stw, line) +
			XTextWidth(	font, 
				stp->line_table[line],
				position - stp->line_table[line]);
		}
	return line;

} /* end of _OlSTXYForPosition */

/*
** Figure the X offset for the given line with the widget's alignment.
*/

Dimension
_OlSTGetOffset OLARGLIST((stw, line))
	OLARG(StaticTextWidget, stw)
	OLGRA(int, line)
{
	Dimension xoff;
	StaticTextPart	*stp = &(stw->static_text);
	XRectangle	*cliprect = &(stp->TextRect);
	XFontStruct	*font = stw->primitive.font;
	OlFontList	*font_list = stw->primitive.font_list;
	char* pline = stp->line_table[line];
	int i = stp->line_len[line];

	switch (stp->alignment) {
	case OL_LEFT:
		xoff = 0;
		break;

	case OL_CENTER:
		if (font_list)
		    xoff = (Dimension) ((int)(cliprect->width - 
		      OlTextWidth(font_list, (unsigned char *)pline, i)) / 2);
		else
			xoff = (Dimension) ((int)(cliprect->width - 
				XTextWidth(font, pline, i)) / 2);
		break;

	case OL_RIGHT:
		if (font_list)
		    xoff = (Dimension) (cliprect->width -
			OlTextWidth(font_list, (unsigned char *)pline, i));
		else	xoff = (Dimension) (cliprect->width -
				XTextWidth(font, pline, i));
		break;
	default:  
/*
** How did we get here? 
*/
		OlVaDisplayWarningMsg(XtDisplay(stw),
				      OleNfileStaticText,
				      OleTmsg5,
				      OleCOlToolkitWarning,
				      OleMfileStaticText_msg5);
		xoff = 0;
		break;
	}

	xoff += (Dimension) (cliprect->x);
	return (xoff);
}


/*************************************<->*************************************
 *
 *	 void _OlSTDisplaySubstring(stw,start,finish,gc)
 *		StaticTextWidget	stw;
 *		char	*start, *finish;
 *		GC	gc;
 *
 *	Description:
 *		Display the string from "start" to "finish" with the
 *		supplied GC.  Include "start" but not "finish".
 *
 *************************************<->***********************************/
void 
_OlSTDisplaySubstring OLARGLIST((stw,start,finish,gc))
	OLARG(StaticTextWidget,	stw)
	OLARG(char,	*start)
	OLARG(char, *finish)
	OLGRA(GC,	gc)
{
    int	i, line;
    Position	cur_x, cur_y;	/* Current start of baseline */
    char	*str1;
    char*  eol;  /* end-of-line */

/*
 * This is to save typing for me and
 * pointer arithmetic for the machine.
 */
    str1 = start;

/*
 * Watch out for null strings.
 */
    while(str1 && *str1 && (str1 < finish)) {

/*
 * Find where the first char of the string goes on-screen
 */
	line = _OlSTXYForPosition(stw, str1, &cur_x, &cur_y);
	eol = stw->static_text.line_table[line]+stw->static_text.line_len[line];
/*
* If 'finish' is less than eol, only draw up to 'finish'
*/
	if (finish < eol)
	    i = finish - str1;
	else
	    i = eol - str1;

	if (i > 0) {
	    if (stw->primitive.font_list)
		OlDrawImageString(XtDisplay(stw), XtWindow(stw),
				  stw->primitive.font_list,
				  gc, (int)cur_x, (int)cur_y,
				  (unsigned char *)str1,i);
	    else
		XDrawImageString(XtDisplay(stw), XtWindow(stw), 
				 gc, cur_x, cur_y, str1,i);
	}
/*
* Move to the next line.
*/
	str1 = stw->static_text.line_table[line+1];
    }

} /* end of _OlSTDisplaySubstring */
