#ifndef	NOIDENT
#ident	"@(#)button:Button.c	1.33.1.86"
#endif


/*
 ************************************************************
 *
 *  Description:
 *	This file contains the source for the OPEN LOOK(tm)
 *	Button widget and gadget.
 *
 ************************************************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <ctype.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/Font.h>
#include <Xol/ButtonP.h>
#include <Xol/Menu.h>

#define ClassName Button
#include <Xol/NameDefs.h>


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

typedef struct {
    unsigned	callbackType;
    Screen	*scr;
    Drawable	win;
    OlgAttrs	*pAttrs;
    Position	x, y;
    Dimension	width, height;
    OlDefine	justification;
    Boolean	set;
    Boolean	sensitive;
} ProcLbl;

typedef union {
    OlgTextLbl		text;
    OlgPixmapLbl	image;
    Widget		proc_widget;
} ButtonLabel;
					/* private procedures		*/
static void	GetInverseTextGC();
static void	GetNormalGC();
static Boolean	IsMenuMode();
static void	OblongRedisplay();
static void	setLabel ();
static unsigned	getOblongFlags ();
static void	RectRedisplay();
static unsigned	getRectFlags ();
static void	SetLabelTile();
static void	SetDimensions();

					/* class procedures		*/

static void	ButtonDestroy();
static void	ButtonInitialize();
static void	ButtonRedisplay();
static void	ButtonRealize();
static void	ClassInitialize();
static Boolean	SetValues();
static void	HighlightHandler OL_ARGS((Widget, OlDefine));

					/* action procedures		*/

					/* public procedures		*/
void	_OlButtonPreview();
void	_OlDrawHighlightButton();
void	_OlDrawNormalButton();


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static int defShellBehavior = (int) BaseWindow;
static int defScale = 12;

#define find_button_font_color(w)	(_OlIsGadget((Widget)w) ? 	\
				(((ButtonGadget) (w))->event.font_color) :\
				(((ButtonWidget) (w))->primitive.font_color))

#define find_button_font(w)	(_OlIsGadget((Widget)w) ?		\
				(((ButtonGadget) (w))->event.font) :\
				(((ButtonWidget) (w))->primitive.font))

#define find_button_foreground(w)	(_OlIsGadget((Widget)w) ? 	\
				(((ButtonGadget) (w))->event.foreground) :\
				(((ButtonWidget) (w))->primitive.foreground))

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*  The Button widget does not have translations or actions  */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define offset(field) XtOffset(ButtonWidget, field)

static XtResource resources[] = { 

	{XtNbuttonType,
		XtCButtonType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.button_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_LABEL) },

	{XtNlabelType,
		XtCLabelType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_STRING) },

	{XtNlabelJustify,
		XtCLabelJustify,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_justify),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_LEFT) },

	{XtNmenuMark,
		XtCMenuMark,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.menumark),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_DOWN) },

	{XtNlabel,
		XtCLabel,
		XtRString,
		sizeof(String),
		offset(button.label),
		XtRString,
		(XtPointer)NULL},

	{XtNlabelImage,
		XtCLabelImage,
		XtRPointer,
		sizeof(XImage *),
		offset(button.label_image),
		XtRPointer,
		(XtPointer)NULL},

	{XtNlabelTile,
		XtCLabelTile,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.label_tile),
		XtRImmediate,
		(XtPointer) False},

	{XtNdefault,
		XtCDefault,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.is_default),
		XtRImmediate,
		(XtPointer) False},

	{XtNselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.select),
		XtRCallback,
		(XtPointer) NULL},

	{XtNunselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.unselect),
		XtRCallback,
		(XtPointer) NULL},

	{XtNset,
		XtCSet,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.set),
		XtRImmediate,
		(XtPointer) False},

	{XtNdim,
		XtCDim,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.dim),
		XtRImmediate,
		(XtPointer) False},

	{XtNbusy,
		XtCBusy,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.busy),
		XtRImmediate,
		(XtPointer) False},

	{XtNrecomputeSize,
		XtCRecomputeSize,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.recompute_size),
		XtRImmediate,
		(XtPointer) True},

	{XtNbackground,
		XtCBackground,
		XtRPixel,
		sizeof(Pixel),
		offset(button.background_pixel),
		XtRString,
		(XtPointer)XtDefaultBackground },

	{XtNpreview,
		XtCPreview,
		XtRWidget,
		sizeof(Widget),
		offset(button.preview),
		XtRWidget,
		(XtPointer) NULL},

	{XtNscale,
		XtCScale,
		XtRInt,
		sizeof(int),
		offset(button.scale),
		XtRInt,
		(XtPointer) &defScale},

	{XtNshellBehavior,
		XtCShellBehavior,
		XtRInt,
		sizeof(int),
		offset(button.shell_behavior),
		XtRInt,
		(XtPointer) &defShellBehavior},

	{XtNpostSelect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.post_select),
		XtRCallback,
		(XtPointer) NULL},


	{XtNlabelProc,
		XtCCallback,
		XtRCallback,
		sizeof (XtPointer),
		offset (button.label_proc),
		XtRCallback,
		(XtPointer) NULL},
};
#undef offset

#define offset(field) XtOffset(ButtonGadget, field)

static XtResource gadget_resources[] = {

	{XtNbuttonType,
		XtCButtonType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.button_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_LABEL) },

	{XtNlabelType,
		XtCLabelType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_STRING) },

	{XtNlabelJustify,
		XtCLabelJustify,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_justify),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_LEFT) },

	{XtNmenuMark,
		XtCMenuMark,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.menumark),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_DOWN) },

	{XtNlabel,
		XtCLabel,
		XtRString,
		sizeof(String),
		offset(button.label),
		XtRString,
		(XtPointer)NULL},

	{XtNlabelImage,
		XtCLabelImage,
		XtRPointer,
		sizeof(XImage *),
		offset(button.label_image),
		XtRPointer,
		(XtPointer)NULL},

	{XtNlabelTile,
		XtCLabelTile,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.label_tile),
		XtRImmediate,
		(XtPointer) False},

	{XtNdefault,
		XtCDefault,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.is_default),
		XtRImmediate,
		(XtPointer) False},

	{XtNselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.select),
		XtRCallback,
		(XtPointer) NULL},

	{XtNunselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.unselect),
		XtRCallback,
		(XtPointer) NULL},

	{XtNset,
		XtCSet,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.set),
		XtRImmediate,
		(XtPointer) False},

	{XtNdim,
		XtCDim,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.dim),
		XtRImmediate,
		(XtPointer) False},

	{XtNbusy,
		XtCBusy,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.busy),
		XtRImmediate,
		(XtPointer) False},

	{XtNrecomputeSize,
		XtCRecomputeSize,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.recompute_size),
		XtRImmediate,
		(XtPointer) True},

	{XtNbackground,
		XtCBackground,
		XtRPixel,
		sizeof(Pixel),
		offset(button.background_pixel),
		XtRString,
		(XtPointer)XtDefaultBackground },

	{XtNpreview,
		XtCPreview,
		XtRWidget,
		sizeof(Widget),
		offset(button.preview),
		XtRImmediate,
		(XtPointer) NULL},

	{XtNscale,
		XtCScale,
		XtRInt,
		sizeof(int),
		offset(button.scale),
		XtRInt,
		(XtPointer) &defScale},

	{XtNshellBehavior,
		XtCShellBehavior,
		XtRInt,
		sizeof(int),
		offset(button.shell_behavior),
		XtRInt,
		(XtPointer) &defShellBehavior},

	{XtNpostSelect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.post_select),
		XtRCallback,
		(XtPointer) NULL},

	{XtNlabelProc,
		XtCCallback,
		XtRCallback,
		sizeof (XtPointer),
		offset (button.label_proc),
		XtRCallback,
		(XtPointer) NULL},
};
#undef offset


/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

ButtonClassRec buttonClassRec = {
  {
    (WidgetClass) &(primitiveClassRec),	/* superclass		  */	
    "Button",				/* class_name		  */
    sizeof(ButtonRec),			/* size			  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    ButtonInitialize,			/* initialize		  */
    NULL,				/* initialize_hook	  */
    ButtonRealize,			/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    ButtonDestroy,			/* destroy		  */
    NULL,				/* resize		  */
    ButtonRedisplay,			/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    XtInheritAcceptFocus,		/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    XtInheritTranslations,		/* tm_table		  */
    NULL,				/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {
    True,				/* focus_on_select	*/
    HighlightHandler,			/* highlight_handler	*/
    NULL,				/* traversal_handler	*/
    NULL,				/* register_focus	*/
    NULL,				/* activate		*/
    NULL,				/* event_procs		*/
    0,					/* num_event_procs	*/
    OlVersion,				/* version		*/
    NULL,				/* extension		*/
    { NULL, 0 },			/* dyn_data		*/
    XtInheritTransparentProc,		/* transparent_proc	*/
  },	/* End of Primitive field initializations */
  {
    0,                                     /* field not used    */
  },  /* ButtonClass fields initialization */
};

ButtonGadgetClassRec buttonGadgetClassRec = {
  {
    (WidgetClass) &(eventObjClassRec),  /* superclass             */
    "Button",                           /* class_name             */
    sizeof(ButtonRec),                  /* size                   */
    ClassInitialize,                    /* class_initialize       */
    NULL,                               /* class_part_initialize  */
    FALSE,                              /* class_inited           */
    ButtonInitialize,                   /* initialize             */
    NULL,                               /* initialize_hook        */
    ButtonRealize,                      /* realize                */
    NULL,                               /* actions                */
    0,                                  /* num_actions            */
    gadget_resources,                   /* resources              */
    XtNumber(gadget_resources),         /* resource_count         */
    NULLQUARK,                          /* xrm_class              */
    FALSE,                              /* compress_motion        */
    TRUE,                               /* compress_exposure      */
    TRUE,                               /* compress_enterleave    */
    FALSE,                              /* visible_interest       */
    ButtonDestroy,                      /* destroy                */
    NULL,				/* resize                 */
    ButtonRedisplay,                    /* expose                 */
    SetValues,                          /* set_values             */
    NULL,                               /* set_values_hook        */
    XtInheritSetValuesAlmost,           /* set_values_almost      */
    NULL,                               /* get_values_hook        */
    (XtProc)XtInheritAcceptFocus,	/* accept_focus           */
    XtVersion,                          /* version                */
    NULL,                               /* callback_private       */
    NULL,                               /* tm_table               */
    NULL,                               /* query_geometry         */
  },  /* RectObjClass fields initialization */
  {
    True,				/* focus_on_select	*/
    HighlightHandler,			/* highlight_handler	*/
    NULL,				/* traversal_handler	*/
    NULL,				/* register_focus	*/
    NULL,				/* activate		*/
    NULL,				/* event_procs		*/
    0,					/* num_event_procs	*/
    OlVersion,				/* version		*/
    NULL				/* extension		*/
  },  /* EventClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* ButtonClass fields initialization */
};


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */


WidgetClass buttonWidgetClass = (WidgetClass) &buttonClassRec;
WidgetClass buttonGadgetClass = (WidgetClass) &buttonGadgetClassRec;


/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 ************************************************************
 *
 *  GetGCs - This function recalculates the colors
 *	used to draw a button based on whether it has input
 *	focus.
 *
 *********************function*header************************
 */

static void
GetGCs (w)
    ButtonWidget	w;
{
    Pixel	focus_color;
    Pixel	font_color;
    Pixel	foreground;
    Boolean	has_focus, isRectangular;
    ButtonPart	*bp;

    bp = find_button_part(w);
    if (_OlIsGadget ((Widget)w))
    {
	focus_color = ((ButtonGadget) w)->event.input_focus_color;
	has_focus   = ((ButtonGadget) w)->event.has_focus;
	font_color  = ((ButtonGadget) w)->event.font_color;
	foreground  = ((ButtonGadget) w)->event.foreground;
    }
    else
    {
	focus_color = ((ButtonWidget) w)->primitive.input_focus_color;
	has_focus   = ((ButtonWidget) w)->primitive.has_focus;
	font_color  = ((ButtonWidget) w)->primitive.font_color;
	foreground  = ((ButtonWidget) w)->primitive.foreground;
    }

    if (bp->normal_GC)
	XtReleaseGC((Widget)w, bp->normal_GC);
    if (bp->inverse_text_GC)
	XtReleaseGC((Widget)w, bp->inverse_text_GC);
    if (bp->pAttrs)
	OlgDestroyAttrs (bp->pAttrs);

    GetNormalGC (w);
    GetInverseTextGC (w);

    isRectangular = (bp->button_type == OL_RECTBUTTON ||
		     bp->button_type == OL_LABEL);

    if (has_focus)
    {
	if (font_color == focus_color ||
	    bp->background_pixel == focus_color)
	{
	    GC	tmp;

	    /* reverse fg and bg for both the button and its label.
	     * If we are 2-D, then only reverse the label colors for
	     * rect buttons.  2-D oblong buttons are drawn as if they
	     * are selected.  (See getOblongFlags)
	     */
	    if (OlgIs3d () || isRectangular)
	    {
		tmp = bp->normal_GC;
		bp->normal_GC = bp->inverse_text_GC;
		bp->inverse_text_GC = tmp;
		bp->pAttrs = OlgCreateAttrs(
					XtScreenOfObject((Widget)w),
					bp->background_pixel,
					(OlgBG *)&(foreground),
					False, bp->scale);
	    }
	    else
	    {
		bp->pAttrs = OlgCreateAttrs(
					XtScreenOfObject((Widget)w),
					foreground,
					(OlgBG *)&(bp->background_pixel),
					False, bp->scale);
	    }
	}
	else
	    bp->pAttrs = OlgCreateAttrs(
				XtScreenOfObject ((Widget)w),
				foreground,
				(OlgBG *)&(focus_color),
				False, bp->scale);
    }
    else
	bp->pAttrs = OlgCreateAttrs(
				XtScreenOfObject ((Widget)w),
			     	foreground,
				(OlgBG *)&(bp->background_pixel),
				False, bp->scale);
}

/*
 ************************************************************
 *
 *  GetInverseTextGC - this function sets the button's font,
 *	foreground, and background in a Graphics Context for
 *	drawing the button in its inverse state.
 *
 *********************function*header************************
 */
static void
GetInverseTextGC(bw)
ButtonWidget bw;
{
	XGCValues	values;
	ButtonPart *bp;

	bp = find_button_part(bw);


	values.foreground = bp->background_pixel;
	values.background = find_button_font_color(bw);
	values.font = find_button_font(bw)->fid;

	bp->inverse_text_GC = XtGetGC(XtParent(bw),
		GCForeground | GCFont | GCBackground,
		&values);

}	/* GetInverseTextGC */

/*
 ************************************************************
 *
 *  GetNormalGC - this function sets the button's font,
 *	foreground, and background in a Graphics Context for
 *	drawing the button in its normal state.
 *
 *********************function*header************************
 */

static void
GetNormalGC(bw)
ButtonWidget bw;
{
	XGCValues	values;
	ButtonPart *bp;

	bp = find_button_part(bw);


	values.foreground = find_button_font_color(bw);
	values.background = bp->background_pixel;
	values.font = find_button_font(bw)->fid;

	bp->normal_GC = XtGetGC(XtParent(bw),
		GCForeground | GCFont | GCBackground,
		&values);
	
}	/* GetNormalGC */

static XrmQuark	XrmQEleft;
static XrmQuark	XrmQEcenter;

/*
 ************************************************************
 *
 *  IsMenuMode - this function checks if button is in a menu
 *	context since visual (and size) is different.
 *
 *********************function*header************************
 */

static Boolean
IsMenuMode(bw)
ButtonWidget bw;
{
	Boolean retval;
	ButtonPart *bp;

	bp = find_button_part(bw);

	switch (bp->shell_behavior) {

		case PinnedMenu:
		case PressDragReleaseMenu:
		case StayUpMenu:
		case UnpinnedMenu:

			retval = TRUE;
			break;

		case BaseWindow:
		case PopupWindow:
		case PinnedWindow:
		case OtherBehavior:
			retval = FALSE;
			break;
	}
	return retval;

}	/* IsMenuMode */


/*
 ************************************************************
 *
 *  OblongRedisplay - This function redisplays the oblong
 *	button in its current state.
 *
 *********************function*header************************
 */
static void
OblongRedisplay(w, event, region)
Widget w;
XEvent *event;		/* unused */
Region region;		/* unused */
{
	ButtonWidget	bw = (ButtonWidget) w;
	ButtonPart *	bp = find_button_part(bw);

	if (bp->set)
		_OlDrawHighlightButton(bw);
	else
		_OlDrawNormalButton(bw);

}	/* OblongRedisplay */

/*
 ***********************************************************
 *
 *  getOblongFlags - Determine the flags needed for
 *	OlgDrawOblongButton according to the state of the
 *	button.
 *
 *********************function*header************************
 */
static unsigned
getOblongFlags (bw)
register Widget	bw;
{
	register ButtonPart *	bp = find_button_part(bw);
	register unsigned	flags = 0;

        if (bp->set)
	    flags |= OB_SELECTED;
	else
	{
	    /* If the button is 2-D and has input focus, the coloration is
	     * as if the button is set if the input focus color conflicts with
	     * either the label color or background.  If the input focus color
	     * is different from either of these, than normal button coloration
	     * is used.
	     */
	    if (!OlgIs3d ())
	    {
		Pixel	focus_color;
		Pixel	font_color;
		Boolean	has_focus;

		if (_OlIsGadget (bw))
		{
		    focus_color = ((ButtonGadget) bw)->event.input_focus_color;
		    has_focus = ((ButtonGadget) bw)->event.has_focus;
		    font_color = ((ButtonGadget) bw)->event.font_color;
		}
		else
		{
		    focus_color = ((ButtonWidget) bw) ->
			primitive.input_focus_color;
		    has_focus = ((ButtonWidget) bw)->primitive.has_focus;
		    font_color = ((ButtonWidget) bw)->primitive.font_color;
		}

		if (has_focus && (focus_color == font_color ||
			 focus_color == bp->background_pixel))
		    flags |= OB_SELECTED;
	    }
	}

	if (bp->is_default)
	    flags |= OB_DEFAULT;
	if (bp->busy)
	{
	    flags |= OB_BUSY;
	    flags &= ~OB_SELECTED;
	}
	if (!XtIsSensitive (bw))
	    flags |= OB_INSENSITIVE;
	if (IsMenuMode (bw))
	    flags |= OB_MENUITEM;
	if (bp->button_type == OL_BUTTONSTACK)
	    flags |= (bp->menumark == OL_RIGHT) ? OB_MENU_R : OB_MENU_D;

	return flags;
}

/*
 ************************************************************
 *
 *  sizeProcLabel - Determine the size of a procedurally defined
 *  label.  Package up some button attributes and call the user's
 *  function.
 ************************************************************
 */

static void
sizeProcLabel (scr, pAttrs, labeldata, pWidth, pHeight)
    Screen	*scr;
    OlgAttrs	*pAttrs;
    XtPointer	labeldata;
    Dimension	*pWidth, *pHeight;
{
    Widget	w = *((Widget *) labeldata);
    ProcLbl	lbl;
    ButtonPart	*bp = find_button_part ((ButtonWidget) w);

    lbl.callbackType = OL_SIZE_PROC;
    lbl.pAttrs = pAttrs;
    lbl.justification = bp->label_justify;
    lbl.set = bp->set;

    XtCallCallbacks (w, XtNlabelProc, (caddr_t) &lbl);

    *pWidth = lbl.width;
    *pHeight = lbl.height;
}

/*
 ************************************************************
 *
 *  drawProcLabel - Draw a procedurally defined label.  Package
 *  up some button attributes and call the user's function.
 */

static void
drawProcLabel (scr, win, pAttrs, x, y, width, height, labeldata)
    Screen	*scr;
    Drawable	win;
    OlgAttrs	*pAttrs;
    Position	x, y;
    Dimension	width, height;
    XtPointer	labeldata;
{
    Widget	w = *((Widget *) labeldata);
    ProcLbl	lbl;
    ButtonPart	*bp = find_button_part ((ButtonWidget) w);

    /* Stuff relevant data into a structure to pass to the user function.
     * Note that the screen and window to draw in might be different than
     * those in the widget.  Previewing does make life difficult.
     */
    lbl.callbackType = OL_DRAW_PROC;
    lbl.scr = scr;
    lbl.win = win;
    lbl.pAttrs = pAttrs;
    lbl.x = x;
    lbl.y = y;
    lbl.width = width;
    lbl.height = height;
    lbl.justification = bp->label_justify;
    lbl.set = bp->set;
    lbl.sensitive = XtIsSensitive (w);

    XtCallCallbacks (w, XtNlabelProc, (caddr_t) &lbl);
}

/*
 ************************************************************
 *
 *  setLabel - populate the label structure and select the
 *	proper sizing and drawing functions.
 *
 *********************function*header************************
 */
static void
setLabel (bw, isSensitive, bg, lbl, sizeProc, drawProc)
ButtonWidget	bw;
Boolean		isSensitive;
Pixel		bg;
ButtonLabel	*lbl;
void		(**sizeProc)();
OlgLabelProc	*drawProc;
{
    ButtonPart	*bp = find_button_part (bw);

    /*
     * WARNING: This temporary measure for showing accelerators/mnemonics
     * assumes that the "lbl->text.label" value is only needed for a short
     * time. If this routine can be reentered before the "lbl->text.label"
     * from a previous call is used, havoc will ensue.
     */

    switch (bp->label_type)  {
    case OL_STRING:
    case OL_POPUP:
	lbl->text.label = bp->label;
	lbl->text.flags = (bp->label_type == OL_POPUP) ? TL_POPUP : 0;
	lbl->text.font_list = find_font_list(bw);

	lbl->text.normalGC = bp->normal_GC;
	lbl->text.inverseGC = bp->inverse_text_GC;
	if (bp->set && !OlgIs3d () &&
	    (bp->button_type==OL_OBLONG || bp->button_type==OL_BUTTONSTACK))
	    lbl->text.flags |= TL_SELECTED;

	/* if the button is insensitive (the brute!), we have to add a stipple
	 * to the GC.  Make a copy of the GC into a scratch version.
	 */
	if (!isSensitive)
	{
	    GC	gc = OlgGetScratchGC (bp->pAttrs);
	    Display	*dpy = DisplayOfScreen (OlgGetScreen (bp->pAttrs));

	    XCopyGC (dpy, lbl->text.normalGC, ~0, gc);
	    XSetStipple (dpy, gc, OlgGetInactiveStipple (bp->pAttrs));
	    XSetFillStyle (dpy, gc, FillStippled);

	    lbl->text.normalGC = gc;
	}

	lbl->text.font = find_button_font(bw);
	switch (bp->label_justify) {
	case OL_LEFT:
	    lbl->text.justification = TL_LEFT_JUSTIFY;
	    break;

	case OL_CENTER:
	    lbl->text.justification = TL_CENTER_JUSTIFY;
	    break;

	default:
	    lbl->text.justification = TL_RIGHT_JUSTIFY;
	    break;
	}
	
	lbl->text.mnemonic = _OlIsGadget((Widget)bw)?
	    ((ButtonGadget)(bw))->event.mnemonic :
		((ButtonWidget)(bw))->primitive.mnemonic;
	
	lbl->text.accelerator = _OlIsGadget((Widget)bw) ?
	    ((ButtonGadget)(bw))->event.accelerator_text :
		((ButtonWidget)(bw))->primitive.accelerator_text;

	*sizeProc = OlgSizeTextLabel;
	*drawProc = (OlgLabelProc)OlgDrawTextLabel;
	break;

    case OL_IMAGE:
	lbl->image.label.image = bp->label_image;
	lbl->image.type = PL_IMAGE;

	lbl->image.normalGC = bp->normal_GC;

	if (!isSensitive)
	{
	    lbl->image.flags = PL_INSENSITIVE;
	    lbl->image.stippleColor = bg;
	}
	else
	    lbl->image.flags = 0;

	switch (bp->label_justify) {
	case OL_LEFT:
	    lbl->image.justification = TL_LEFT_JUSTIFY;
	    break;

	case OL_CENTER:
	    lbl->image.justification = TL_CENTER_JUSTIFY;
	    break;

	default:
	    lbl->image.justification = TL_RIGHT_JUSTIFY;
	    break;
	}
	*sizeProc = OlgSizePixmapLabel;
	*drawProc = (OlgLabelProc)OlgDrawPixmapLabel;
	break;

    case OL_PROC:
	lbl->proc_widget = (Widget) bw;
	*sizeProc = sizeProcLabel;
	*drawProc = (OlgLabelProc)drawProcLabel;
	break;
    }
}

/*
 ************************************************************
 *
 *  RectRedisplay - The Redisplay function must clear the
 *	entire window before drawing the button from scratch;
 *	there is no attempt to repair damage on the button.
 *	Note: this function also used for OL_LABEL buttontype.
 *
 *********************function*header************************
 */
static void
RectRedisplay(w, event, region)
Widget w;
XEvent *event;		/* unused */
Region region;		/* unused */
{
        Screen *	bw_screen = XtScreenOfObject (w);
	ButtonPart *	bp = find_button_part(w);
	ButtonLabel	lbl;
	void		(*sizeProc)();
	OlgLabelProc	drawProc;
	unsigned	flags;

	if(!XtIsRealized(w))
		return;

	setLabel (w, XtIsSensitive (w), w->core.background_pixel,
		  &lbl, &sizeProc, &drawProc);

	if (bp->button_type == OL_LABEL)
	{
	    (*drawProc) (bw_screen, XtWindowOfObject (w), bp->pAttrs,
			 _OlXTrans (w, 0), _OlYTrans (w, 0),
			 w->core.width, w->core.height, (caddr_t)&lbl);
	}
	else
	{
	    flags = getRectFlags (w);
	    OlgDrawRectButton (bw_screen, XtWindowOfObject (w), bp->pAttrs,
			       _OlXTrans (w, 0), _OlYTrans (w, 0),
			       w->core.width, w->core.height,
			       &lbl, drawProc, flags);
	}
}	/* RectRedisplay */

/*
 ***********************************************************
 *
 *  getRectFlags - Determine the flags needed for
 *	OlgDrawRectButton according to the state of the
 *	button.
 *
 *********************function*header************************
 */
static unsigned
getRectFlags (bw)
register Widget	bw;
{
	register ButtonPart *	bp = find_button_part(bw);
	register unsigned	flags = 0;

        if (bp->set)
	    flags |= RB_SELECTED;
	if (bp->is_default)
	    flags |= RB_DEFAULT;
	if (bp->dim)
	    flags |= RB_DIM;
	if (!XtIsSensitive (bw))
	    flags |= OB_INSENSITIVE;

	return flags;
}

/*
 ************************************************************
 *
 *  SetLabelTile - this function creates a pixmap of the
 *	current image and sets it as the tile to the necessary
 *	GCs.
 *
 *********************function*header************************
 */
static void
SetLabelTile(bw)
ButtonWidget bw;
{
	XGCValues	values;
	Pixel		font_color;
	Pixel		foreground;
	XFontStruct	*font;
	ButtonPart *	bp = find_button_part(bw);
	Display *	bw_display = XtDisplayOfObject((Widget)bw);
	Widget w = (Widget) bw;

	font       = find_button_font(bw);
	font_color = find_button_font_color(bw);
	foreground = find_button_foreground(bw);
	if (bp->label_tile)  {
		Pixmap tile_pixmap;
		Pixmap highlight_tile_pixmap;

		if (bp->label_image == (XImage *) NULL) {

                	bp->label_type = OL_STRING;
                	bp->label_tile = FALSE;
	
                	OlVaDisplayWarningMsg(  XtDisplayOfObject(w),
                                        OleNinvalidResource,
                                        OleTinitialize,
                                        OleCOlToolkitWarning,
                                        OleMinvalidResource_initialize,
                                        XtName(w),
                                        OlWidgetToClassName(w),
                                        XtNlabelType,
                                        "OL_IMAGE");
                	return;
                }

		tile_pixmap = XCreatePixmap(bw_display,
			DefaultRootWindow(bw_display),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height,
			(unsigned int) XDefaultDepthOfScreen(
						XtScreenOfObject((Widget)bw)
					)
			);

		highlight_tile_pixmap = XCreatePixmap(bw_display,
			DefaultRootWindow(bw_display),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height,
			(unsigned int) XDefaultDepthOfScreen(
						XtScreenOfObject((Widget)bw)
					)
			);

		XPutImage(bw_display,
			tile_pixmap,
			bp->inverse_text_GC,
			bp->label_image,
			0,
			0,
			(int) _OlXTrans((Widget)bw, 0),
			(int) _OlYTrans((Widget)bw, 0),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height);

		XPutImage(bw_display,
			highlight_tile_pixmap,
			bp->normal_GC,
			bp->label_image,
			0,
			0,
			(int) _OlXTrans((Widget)bw, 0),
			(int) _OlYTrans((Widget)bw, 0),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height);
		/*
		 *  Recreate the GCs using the appropriate tile
		 */

		if (bp->pAttrs)
		    OlgDestroyAttrs (bp->pAttrs);
		bp->pAttrs = OlgCreateAttrs(
				XtScreenOfObject((Widget)bw), foreground,
				(OlgBG *)&(tile_pixmap),
				True, bp->scale);

		if (bp->pHighlightAttrs)
		    OlgDestroyAttrs (bp->pHighlightAttrs);
		bp->pHighlightAttrs = OlgCreateAttrs(
					XtScreenOfObject((Widget)bw),
					foreground,
					(OlgBG *)&(highlight_tile_pixmap),
					True, bp->scale);

		XtReleaseGC((Widget)bw, bp->inverse_text_GC);
		values.foreground = bp->background_pixel;
		values.background = font_color;
		values.font = font->fid;
		values.tile = highlight_tile_pixmap;
		bp->inverse_text_GC = XtGetGC(XtParent(bw),
			GCForeground | GCFont | GCBackground | GCTile,
			&values);

		XtReleaseGC((Widget)bw, bp->normal_GC);
		values.foreground = font_color;
		values.background = bp->background_pixel;
		values.font = font->fid;
		values.tile = tile_pixmap;
		bp->normal_GC = XtGetGC(XtParent(bw),
			GCForeground | GCFont | GCBackground | GCTile,
			&values);
	
		XFreePixmap(bw_display, tile_pixmap);
		XFreePixmap(bw_display, highlight_tile_pixmap);
		}

}	/*  SetLabelTile  */


/*
 ************************************************************
 *
 *  SetDimensions - This function determines the 
 *	dimensions of the core width and height of the 
 *	button window.
 *
 *********************function*header************************
 */
static void
SetDimensions(bw)
ButtonWidget bw;
{
	ButtonPart *bp;
	void (*lblSizeProc)();
	OlgLabelProc	lblDrawProc;
	ButtonLabel lbl;
	unsigned flags;

	bp = find_button_part(bw);

	setLabel (bw, XtIsSensitive ((Widget)bw), bw->core.background_pixel,
		  &lbl, &lblSizeProc, &lblDrawProc);

	switch (bp->button_type) {
	default:
	case OL_RECTBUTTON:
	case OL_LABEL:
	    flags = getRectFlags (bw);
	    OlgSizeRectButton (XtScreenOfObject ((Widget)bw), bp->pAttrs,
			       (XtPointer)&lbl, lblSizeProc, flags,
			       &bp->normal_width, &bp->normal_height);
	    break;

	case OL_OBLONG:
	case OL_BUTTONSTACK:
	    flags = getOblongFlags (bw);
	    OlgSizeOblongButton (XtScreenOfObject((Widget)bw), bp->pAttrs,
				 (XtPointer)&lbl,
				 lblSizeProc, flags, &bp->normal_width,
				 &bp->normal_height);
	    break;
	}

	if (bp->recompute_size)  {
		bw->core.width = bp->normal_width;
		bw->core.height = bp->normal_height;
		return;
		}

	if (bw->core.height == (Dimension) 0)
		bw->core.height = bp->normal_height;

	if (bw->core.width == (Dimension) 0)
		bw->core.width = bp->normal_width;


}	/*  SetDimensions  */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 ************************************************************
 *
 *  ButtonDestroy - this function frees the GCs and the space
 *	allocated for the copy of the label.
 *
 *********************function*header************************
 */
static void
ButtonDestroy(w)
Widget w;
{
	ButtonWidget bw = (ButtonWidget)w;
	ButtonPart *bp;

	bp = find_button_part(bw);


	/*  Get rid of callbacks and eventhandlers */

	XtRemoveAllCallbacks(w, XtNselect);
	XtRemoveAllCallbacks(w, XtNunselect);
	XtRemoveAllCallbacks(w, XtNpostSelect);

	/*
	 *  Free the GCs
	 */
	OlgDestroyAttrs (bp->pAttrs);
	if (bp->pHighlightAttrs)
	    OlgDestroyAttrs (bp->pHighlightAttrs);
	XtReleaseGC((Widget)bw, bp->normal_GC);
	XtReleaseGC((Widget)bw, bp->inverse_text_GC);

	/*
	 *  Free the label, if it is not the widget name
	 */
       	if (bp->label)
		XtFree(bp->label);

}	/* ButtonDestroy */


/*
 ************************************************************
 *
 *  ButtonInitialize - this function is called when the 
 *	widget is created.  It copies the label, creates the
 *	GCs, and initializes the button state to NORMAL.
 *
 *********************function*header************************
 */
/* ARGSUSED */
static void
ButtonInitialize(request, new, args, num_args)
Widget		request;
Widget		new;
ArgList		args;
Cardinal *	num_args;
{
	ButtonWidget	bw = (ButtonWidget) new;
	ButtonPart *	bp = find_button_part(bw);
	Widget		shell=(Widget)NULL;

				/* initialize button fields as needed */

	if(!bp->recompute_size) {
		if(bw->core.width<=(Dimension)3) { /* calculation assumption */
			bw->core.width=(Dimension)4;
		}
		if(bw->core.height==(Dimension)0) 
			bw->core.height=(Dimension)1;
	}

	bw->core.border_width = (Dimension) 0;

	/*
	 *  Do a check on the boolean resource values.  They must
	 *  be boolean values.
	 */
	if (bp->is_default) {
		bp->is_default = (Boolean) TRUE;

			/* Since our default is TRUE, tell the shell	*/
		_OlSetDefault(new, bp->is_default);
	}

	if (bp->set)
		bp->set = (Boolean) TRUE;

	if (bp->dim)
		bp->dim = (Boolean) TRUE;

	if (bp->busy)
		bp->busy = (Boolean) TRUE;

	bp->internal_busy = (Boolean) FALSE;

	if (bp->label_tile)
		bp->label_tile = (Boolean) TRUE;

				/* check for valid button type */

	if(bp->button_type!=OL_LABEL
		&& bp->button_type!=OL_OBLONG
		&& bp->button_type!=OL_RECTBUTTON
		&& bp->button_type!=OL_BUTTONSTACK) {

		bp->button_type = OL_LABEL;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName(new),
					OlWidgetToClassName(new),
					XtNbuttonType,
					"OL_LABEL");
	}

				/* check for valid label type */

	if (bp->label_type != OL_STRING 
		&& bp->label_type != OL_IMAGE
		&& bp->label_type != OL_POPUP
		&& bp->label_type != OL_PROC)  {

		bp->label_type = OL_STRING;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName(new),
					OlWidgetToClassName(new),
					XtNlabelType,
					"OL_STRING");
	}

				/* check for valid combinations */

	if(bp->button_type!=OL_OBLONG && bp->label_type==OL_POPUP) {
		bp->label_type=OL_STRING;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName(new),
					OlWidgetToClassName(new),
					XtNlabelType,
					"OL_STRING");
	}
	if(bp->label_type==OL_IMAGE &&  bp->label_image==(XImage *)NULL) {
		bp->label_type=OL_STRING;
                bp->label_tile = FALSE;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNfileButton,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileButton_msg1,
					XtName(new));
	}

	shell = _OlGetShellOfWidget(new);

	if(shell && XtIsSubclass(shell, menuShellWidgetClass)) {
		Arg	arg; 	
		XtSetArg(arg,XtNshellBehavior,(XtArgVal)&(bp->shell_behavior));
		XtGetValues(shell,&arg,1);
	}

	if (IsMenuMode(bw) && bp->menumark!=OL_RIGHT) {
		bp->menumark = OL_RIGHT;
	}
	else if (bp->menumark != OL_DOWN && bp->menumark != OL_RIGHT) {
		bp->menumark = OL_DOWN;
	}

	/*
	 *  First initialize the resources that are common to
	 *  all buttons.
	 */

	/* Set the new label -- always copy & never allow a NULL pointer */
	bp->label = XtNewString(bp->label? bp->label : XtName((Widget)bw));

	if (!_OlIsGadget((Widget)bw))
		bp->background_pixel = bw->core.background_pixel;

	/* check on resources initialized by user */

	if (bp->label_justify != OL_LEFT 
			&& bp->label_justify!=OL_CENTER)  {
		bp->label_justify = OL_LEFT;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName(new),
					OlWidgetToClassName(new),
					XtNlabelJustify,
					"OL_LEFT");
	}

	GetNormalGC(bw);
	GetInverseTextGC(bw);

	bp->pAttrs = bp->pHighlightAttrs = (OlgAttrs *) 0;
	SetLabelTile(bw);

	/* LATER -- must support background pixmaps */
	if (!bp->label_tile)
	    bp->pAttrs = OlgCreateAttrs (XtScreenOfObject ((Widget)bw),
					 find_button_foreground(bw),
					 (OlgBG *)&(bp->background_pixel),
					 False, bp->scale);

	/*
	 *  Now initialize resources which are specific to each type
	 *  of button.
	 */

	SetDimensions(bw);

} 	/* ButtonInitialize */


/*
 ************************************************************
 *
 *  ButtonRedisplay - This function displays the button depending
 *	upon the button type.  If the event is null, then the button
 *	is not being redisplayed in response to an expose event,
 *	and the window must be cleared before drawing the button.
 *
 *********************function*header************************
 */
static void
ButtonRedisplay(w, event, region)
Widget w;
XEvent *event;		/* unused */
Region region;		/* unused */
{
	ButtonWidget bw = (ButtonWidget) w;
	ButtonPart *bp;

	if(XtIsRealized(w) == FALSE) return;

	if (event == (XEvent *) 0)
	    XClearArea (XtDisplayOfObject ((Widget)bw), XtWindowOfObject ((Widget)bw),
		    _OlXTrans ((Widget)bw, 0), _OlYTrans ((Widget)bw, 0),
		    bw->core.width, bw->core.height, False);

	bp = find_button_part(bw);

        /* Set up the Tile/Stipple Origin for the GC's involved,
           this is done for gadgets because they are using their
           parents' windows */

        XSetTSOrigin(XtDisplayOfObject((Widget )bw),
                        bp->pAttrs->bg1,
                        _OlXTrans ((Widget) bw, 0),
                        _OlYTrans ((Widget) bw, 0));
        XSetTSOrigin(XtDisplayOfObject((Widget )bw),
                        bp->pAttrs->bg2,
                        _OlXTrans ((Widget) bw, 0),
                        _OlYTrans ((Widget) bw, 0));

	switch (bp->button_type)  {
		case OL_RECTBUTTON:
		case OL_LABEL:
			RectRedisplay(w, event, region);
			break;
		case OL_OBLONG:
		case OL_BUTTONSTACK:
		default:
			OblongRedisplay(w, event, region);
			break;
		}
}	/* ButtonRedisplay */

/*
 ************************************************************
 *
 *  ButtonRealize - realize the button widget.
 *
 *********************function*header************************
 */
static void
ButtonRealize(w, valueMask, attributes)
register Widget w;
Mask *valueMask;
XSetWindowAttributes *attributes;
{

	/* The window background is always inherited from the parent. */
	if (XtClass(w) != buttonWidgetClass) {
		attributes->background_pixmap = ParentRelative;
		*valueMask |= CWBackPixmap;
		*valueMask &= ~CWBackPixel;
	}
	XtCreateWindow(w, (unsigned int)InputOutput, (Visual *)CopyFromParent,
		    *valueMask, attributes );

}	/* ButtonRealize */


/*
 ************************************************************
 *
 *  ClassInitialize - Register OlDefine string values.
 *
 *********************function*header************************
 */
static void
ClassInitialize()
{
						/* XtNbuttonType */
	_OlAddOlDefineType ("label",       OL_LABEL);
	_OlAddOlDefineType ("oblong",      OL_OBLONG);
	_OlAddOlDefineType ("rectbutton",  OL_RECTBUTTON);
	_OlAddOlDefineType ("buttonstack", OL_BUTTONSTACK);

						/* XtNlabelType */
	_OlAddOlDefineType ("string",      OL_STRING);
	_OlAddOlDefineType ("image",       OL_IMAGE);
	_OlAddOlDefineType ("popup",       OL_POPUP);
	_OlAddOlDefineType ("proc",        OL_PROC);

						/* XtNlabelJustify */
	_OlAddOlDefineType ("left",        OL_LEFT);
	_OlAddOlDefineType ("center",      OL_CENTER);

						/* XtNmenuMark */
	_OlAddOlDefineType ("down",        OL_DOWN);
	_OlAddOlDefineType ("right",       OL_RIGHT);
} /* ClassInitialize */

/*
 ************************************************************
 *
 *  SetValues - This function compares the requested values
 *	to the current values, and sets them in the new
 *	widget.  It returns TRUE when the widget must be
 *	redisplayed.
 *
 *********************function*header************************
 */
/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
Widget		current;
Widget		request;
Widget		new;
ArgList		args;
Cardinal *	num_args;
{
	ButtonWidget bw = (ButtonWidget) current;
	ButtonWidget newbw = (ButtonWidget) new;
	Boolean needs_redisplay = (Boolean) FALSE;
	Boolean was_resized = (Boolean) FALSE;
	ButtonPart *bp;
	ButtonPart *newbp;

	bp = find_button_part(bw);
	newbp = find_button_part(newbw);

	/* Note: cannot change button type since semantics won't match */

	if(newbp->button_type!=bp->button_type) {

		newbp->button_type=bp->button_type;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNinvalidResource,
					OleTsetValuesNC,
					OleCOlToolkitWarning,
					OleMinvalidResource_setValuesNC,
					XtName(new),
					OlWidgetToClassName(new),
					XtNbuttonType);
	}

				/* check for valid label type */

	if (newbp->label_type != OL_STRING 
		&& newbp->label_type != OL_IMAGE
		&& newbp->label_type != OL_POPUP
		&& newbp->label_type != OL_PROC)  {

		newbp->label_type = OL_STRING;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNinvalidResource,
					OleTsetValues,
					OleCOlToolkitWarning,
					OleMinvalidResource_setValues,
					XtName(new),
					OlWidgetToClassName(new),
					XtNlabelType,
					"OL_STRING");
	}

				/* check for valid combinations */

	if(newbp->button_type!=OL_OBLONG && newbp->label_type==OL_POPUP) {
		newbp->label_type=OL_STRING;

		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNinvalidResource,
					OleTsetValues,
					OleCOlToolkitWarning,
					OleMinvalidResource_setValues,
					XtName(new),
					OlWidgetToClassName(new),
					XtNlabelType,
					"OL_STRING");
	}
	if(newbp->label_type==OL_IMAGE && newbp->label_image==(XImage *)NULL) {

		newbp->label_type=OL_STRING;
		OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
					OleNfileButton,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileButton_msg1,
					XtName(new));
	}
	
	/*
	 *  Has the menumark resource changed?
	 */

	if (IsMenuMode(newbw) && newbp->menumark != OL_RIGHT) {
		newbp->menumark = OL_RIGHT;
	}
	else if (newbp->menumark != OL_DOWN && newbp->menumark != OL_RIGHT) {
		newbp->menumark = OL_DOWN;
	}

	/*
	 *  Has the shell_behavior resource changed?
	 */

	if (bp->shell_behavior!=newbp->shell_behavior 
		&& ( (IsMenuMode(bw) && !IsMenuMode(newbw)
			|| (!IsMenuMode(bw) && IsMenuMode(newbw)))))
	{
		was_resized=(Boolean) TRUE;
	}

	/*
	 *  Has the label_type resource changed?
	 */
	
	if(newbp->label_type!=bp->label_type) {	

		switch (newbp->label_type)  {

		case OL_STRING:
		case OL_POPUP:
		case OL_PROC:
			if (bp->label_tile)  {
				/*
				 *  Turn off tile in the GC's
				 */
				XtReleaseGC((Widget)bw, bp->inverse_text_GC);
				XtReleaseGC((Widget)bw, bp->normal_GC);
				GetInverseTextGC(newbw);
				GetNormalGC(newbw);
				}
			break;

		case OL_IMAGE:
			if (newbp->label_image != (XImage *)NULL)  {
				if (newbp->label_tile)
					SetLabelTile(newbw);
				break;
				}

		}
		was_resized = (Boolean) TRUE;
	}

	/*
	 *  Has the label resource changed?  
	 *
	 *  MORE: Check the show-mnemonic and show-accelerator resources.
	 */

	if (bp->label != newbp->label)  {
		newbp->label = XtNewString(newbp->label? newbp->label : XtName((Widget)newbw));
		if (bp->label)
			XtFree(bp->label);

		was_resized = (Boolean) TRUE;
	}

	/*
	 *  Has the mnemonic changed?  (We should also check if the
	 *  accelerator has changed, but how?)
	 */

	if (_OlIsGadget ((Widget)newbw))
	{
	    if (((ButtonGadget) bw)->event.mnemonic !=
		((ButtonGadget) newbw)->event.mnemonic)
		was_resized = (Boolean) TRUE;
	}
	else
	{
	    if (bw->primitive.mnemonic != newbw->primitive.mnemonic)
		was_resized = (Boolean) TRUE;
	}

	/*
	 *  Has the label_image resource changed?
	 */
	if (bp->label_image != newbp->label_image) {
		if (newbp->label_type == OL_IMAGE)  {
			SetLabelTile(newbw);
			was_resized = (Boolean) TRUE;
			}
		}

	/*
	 *  Has the label_tile resource changed?
	 */
	if (bp->label_tile != newbp->label_tile) {
		if (newbp->label_tile &&
			newbp->label_type == OL_IMAGE)  {
			SetLabelTile(newbw);
			needs_redisplay = (Boolean) TRUE;
			}
		}

	/*
	 *  If the recompute_size resource has changed, then the size
	 *  of the button may change.
	 */
	if (bp->recompute_size != newbp->recompute_size)  {
		was_resized = (Boolean) TRUE;
		}

	newbw->core.border_width= (Dimension) 0; 

	/*
	 *  Now we have dealt with all of the resources which could
	 *  possibly change the size of the button, so it is time
	 *  to do the geometry request.
	 */
	if (bw->core.width != newbw->core.width ||
		bw->core.height != newbw->core.height ||
		was_resized) {

		/*
		 *  First do any calculations necessary to determine
		 *  the new size of the button.
		 */
	        SetDimensions(newbw);

		if(newbp->normal_height == (Dimension) 0) 	/* no 0x0 */
		    newbp->normal_height = (Dimension) 1;
		if (newbp->normal_width <= (Dimension) 3) {/* Xlib calc. assumption */ 
		    newbp->normal_height = (Dimension) 4;
		}

		if(newbw->core.height == (Dimension) 0) 	/* no 0x0 */
				newbw->core.height = (Dimension) 1;
		if (newbw->core.width <= (Dimension) 3) /* X assumption */ 
				newbw->core.height = (Dimension) 4;

		/*
		 *  Always return true from SetValues if the core.width
		 *  or core.height have changed.
		 */
		needs_redisplay = (Boolean) TRUE;
		}

	/*
	 *  If the foreground or background have changed,
	 *  then the button's GCs must be destroyed and recreated.
	 *  Note that changing the values of the foreground and
	 *  background in the current GCs does not work
	 *  because the GCs are cached and the changes would
	 *  affect other widgets which share the GCs.
	 */

	if (!_OlIsGadget((Widget)newbw))  {
		if (newbw->core.background_pixel != bw->core.background_pixel) {
			newbp->background_pixel = newbw->core.background_pixel;
	    		if (XtClass(new) != buttonWidgetClass) 
				_OlDefaultTransparentProc(new,
					XtParent(new)->core.background_pixel,
					XtParent(new)->core.background_pixmap);
		}
	}

	if (find_button_font_color(bw) != find_button_font_color(newbw) ||
		bp->background_pixel != newbp->background_pixel)  {

		if (newbp->label_type == OL_IMAGE && newbp->label_tile)
		{
			SetLabelTile (newbp);
		}
		else
		{
			GetGCs (newbw);
		}
		
		needs_redisplay = TRUE;
	}

	/*
	 *  Has the label_justify resource changed?
	 */
	if (newbp->label_justify != bp->label_justify)  {
		if (newbp->label_justify != OL_LEFT
			&& newbp->label_justify != OL_CENTER) {
			newbp->label_justify=bp->label_justify;

			OlVaDisplayWarningMsg(	XtDisplayOfObject(new),
						OleNinvalidResource,
						OleTsetValuesNC,
						OleCOlToolkitWarning,
						OleMinvalidResource_setValuesNC,
						XtName(new),
						OlWidgetToClassName(new),
						XtNlabelJustify);
		}
		else
			needs_redisplay = (Boolean) TRUE;
	}

	/*
	 *  Has the is_default resource changed?
	 */
	if (bp->is_default != newbp->is_default)  {
		if (bp->is_default)
			bp->is_default = True;	/* make it boolean */

		_OlSetDefault(new, newbp->is_default);
		needs_redisplay = (Boolean) TRUE;
	}

	/*
	 *  Has the sensitive resource changed?
	 */
	if (XtIsSensitive((Widget)newbw)!=XtIsSensitive((Widget)bw))  {
		needs_redisplay = (Boolean) TRUE;
	}

	/*
	 *  Has the set resource changed?
	 */
	if (newbp->set != bp->set)  {
		needs_redisplay = (Boolean) TRUE;

		if (newbp->set)
			newbp->set = (Boolean) TRUE;
	}

	/*
	 *  Has the dim resource changed?
	 */
	if (newbp->dim != bp->dim)  {
		needs_redisplay = (Boolean) TRUE;

		if (newbp->dim)
			newbp->dim = (Boolean) TRUE;
	}
	
	/*
	 *  Has the busy resource changed?
	 */
	if (newbp->busy != bp->busy)  {
		needs_redisplay = (Boolean) TRUE;

		if (newbp->busy)
			newbp->busy = (Boolean) TRUE;
		else
			newbp->internal_busy=(Boolean) FALSE;
	}

	/*
	 *  Has the button preview resource changed?
	 */
	if (newbp->preview != (Widget) NULL)  {
		_OlButtonPreview((ButtonWidget)newbp->preview, new);
		newbp->preview = (Widget)NULL;
	}

	/*
	 *  The window always uses the parent's background.  (Widgets only)
	 */
	if (XtIsRealized (new) && !_OlIsGadget (new) &&
	    (XtClass(new) != buttonWidgetClass) &&
	    new->core.background_pixmap != ParentRelative)
	{
	    XSetWindowBackgroundPixmap (XtDisplay (new), XtWindow (new),
					ParentRelative);
	    new->core.background_pixmap = ParentRelative;
	    needs_redisplay = (Boolean) TRUE;
	}

	return (needs_redisplay || was_resized);

}	/* SetValues */


/*********************function*header************************
 *  HighlightHandler - This function get new GCs to change the
 *	button and label colors as required for the mode and
 *	forces a redraw of the button.
 */
static void
HighlightHandler OLARGLIST((w, highlight_type))
    OLARG(Widget,	w)
    OLGRA(OlDefine,	highlight_type)		/* OL_IN / OL_OUT */
{
    ButtonPart *bp;

        /* because of the the conflict of focus GC and tiled image
           GC, we have to sacrifice one of them.  In this case, we
           are letting the tiled image prevail */

    bp = find_button_part((ButtonWidget)w);
    if (bp->label_type != OL_IMAGE || !bp->label_tile)
        GetGCs (w);

    ButtonRedisplay (w, NULL, NULL);
}


/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */


/*
 ************************************************************
 *
 *  _OlButtonPreview - This function displays the source widget
 *	in the destination widget.  It is used to preview
 *	the default choice in a menu.
 *
 *********************function*header************************
 */
void
_OlButtonPreview(dest, src)
ButtonWidget dest;
ButtonWidget src;
{
	ButtonPart	*button_part = find_button_part(src);
        unsigned	flags;
	ButtonLabel	lbl;
	void		(*sizeProc)();
	OlgLabelProc	drawProc;
	Dimension	width, height;
	Boolean		wasSet;

	if(!XtIsRealized((Widget)dest))
		return;

	/* We must clear the destination window before drawing */
	XClearArea (XtDisplayOfObject ((Widget)dest), XtWindowOfObject ((Widget)dest),
		    _OlXTrans ((Widget)dest, 0), _OlYTrans ((Widget)dest, 0),
		    dest->core.width, dest->core.height, False);

	/* Populate the label structure from the new button part.  The
	 * button is drawn selected.
	 */
	wasSet = button_part->set;
	button_part->set = True;
	setLabel (src, True, dest->core.background_pixel,
		  &lbl, &sizeProc, &drawProc);
	button_part->set = wasSet;

	/* Must determine the size of the label before drawing the button. */
	(*sizeProc) (XtScreenOfObject ((Widget)dest), button_part->pAttrs, &lbl,
		     &width, &height);

	/* Use only a subset of the flags from the button part */
	switch (button_part->button_type)  {
	case OL_LABEL:
	    (*drawProc) (XtScreenOfObject ((Widget)dest), XtWindowOfObject ((Widget)dest),
			 button_part->pAttrs,
			 _OlXTrans ((Widget)dest, 0), _OlYTrans ((Widget)dest, 0),
			 dest->core.width, dest->core.height, (caddr_t)&lbl);
	    break;

	case OL_RECTBUTTON:
	    flags = RB_SELECTED;
	    if (button_part->is_default)
		flags |= RB_DEFAULT;
	    if (button_part->dim)
		flags |= RB_DIM;

	    OlgDrawRectButton (XtScreenOfObject ((Widget)dest),
			       XtWindowOfObject ((Widget)dest),
			       button_part->pAttrs,
			       _OlXTrans ((Widget)dest, 0), _OlYTrans ((Widget)dest, 0),
			       dest->core.width, dest->core.height,
			       &lbl, drawProc, flags);

	    break;

	case OL_OBLONG:
	case OL_BUTTONSTACK:
	default:
	    flags = OB_SELECTED;
	    if (button_part->is_default)
		flags |= OB_DEFAULT;
	    if (button_part->busy)
		flags |= OB_BUSY;

	    switch (button_part->shell_behavior) {
	    case PinnedMenu:
	    case PressDragReleaseMenu:
	    case StayUpMenu:
	    case UnpinnedMenu:
		flags |= OB_MENUITEM;
		break;
	    }

	    if (button_part->button_type == OL_BUTTONSTACK)
		flags |= (button_part->menumark == OL_RIGHT) ?
		    OB_MENU_R : OB_MENU_D;

	    OlgDrawOblongButton (XtScreenOfObject ((Widget)dest),
				 XtWindowOfObject ((Widget)dest),
				 (button_part->label_tile) ? 
			         button_part->pHighlightAttrs :
			         button_part->pAttrs,
				 _OlXTrans ((Widget)dest, 0), _OlYTrans ((Widget)dest, 0),
				 dest->core.width, dest->core.height,
				 (caddr_t)&lbl, drawProc, flags);

	    break;
	}

	XFlush (XtDisplayOfObject ((Widget)dest));
}	/*  _OlButtonPreview  */


/*
 ************************************************************
 *
 *  _OlDrawHighlightButton - this function draws the oblong
 *	button in its highlighted state.
 *
 *********************function*header************************
 */
void
_OlDrawHighlightButton(bw)
ButtonWidget bw;
{
        Screen *	bw_screen = XtScreenOfObject ((Widget)bw);
	ButtonPart *	bp = find_button_part(bw);
	OlgAttrs *	pAttrs;
	ButtonLabel	lbl;
	void		(*sizeProc)();
	OlgLabelProc	drawProc;
	unsigned	flags;
	Boolean		wasSet;

	if(!XtIsRealized((Widget)bw))
		return;

	wasSet = bp->set;
	bp->set = True;
	flags = getOblongFlags (bw);
	setLabel (bw, XtIsSensitive ((Widget)bw), bw->core.background_pixel,
		  &lbl, &sizeProc, &drawProc);

	if (bp->label_tile) 
	    pAttrs = bp->pHighlightAttrs;
	else
	    pAttrs = bp->pAttrs;

	OlgDrawOblongButton (bw_screen, XtWindowOfObject ((Widget)bw), pAttrs,
			     _OlXTrans ((Widget)bw, 0), _OlYTrans ((Widget)bw, 0),
			     bw->core.width, bw->core.height,
			     (XtPointer)&lbl, drawProc, flags);
	bp->set = wasSet;

	XFlush (XtDisplayOfObject ((Widget)bw));

}  /* _OlDrawHighlightButton  */


/*
 ************************************************************
 *
 *  _OlDrawNormalButton - this function draws the oblong
 *	button in its normal state.
 *
 *********************function*header************************
 */
void
_OlDrawNormalButton(bw)
ButtonWidget bw;
{
        Screen *	bw_screen = XtScreenOfObject ((Widget)bw);
	ButtonPart *	bp = find_button_part(bw);
	ButtonLabel	lbl;
	void		(*sizeProc)();
	OlgLabelProc	drawProc;
	unsigned	flags;
	Boolean		wasSet;

	if(!XtIsRealized((Widget)bw))
		return;

	wasSet = bp->set;

	/* 2-D oblong buttons that have input focus are drawn as if they
	 * they are set if the input focus color conflicts with either the
	 * foreground or background color.  In this case, the flags returned
	 * from getOblongFlags will contain OB_SELECTED.  If this flag is
	 * set, then the bp->set must also be True to get the correct label
	 * colors.  Yuck.
	 */
	bp->set = False;
	flags = getOblongFlags (bw);
	if (flags & OB_SELECTED)
	    bp->set = True;

	/* For those buttons that have tiled image, no label will
	   be drawn */
	if (bp->label_type != OL_IMAGE || bp->label_tile == FALSE)
        	setLabel (bw, XtIsSensitive ((Widget)bw), bw->core.background_pixel,
               		&lbl, &sizeProc, &drawProc);
	else drawProc = NULL;

	OlgDrawOblongButton (bw_screen, XtWindowOfObject ((Widget)bw), bp->pAttrs,
			     _OlXTrans ((Widget)bw, 0), _OlYTrans ((Widget)bw, 0),
			     bw->core.width, bw->core.height,
			     (XtPointer)&lbl, drawProc, flags);
	bp->set = wasSet;
}  /* _OlDrawNormalButton  */
