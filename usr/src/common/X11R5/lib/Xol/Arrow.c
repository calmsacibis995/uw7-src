/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)arrow:Arrow.c	1.44"
#endif

/*
 *************************************************************************
 *
 * Description: This Widget creates an "arrow widget:
 *		it offers :(1) OL_LEFT (2) OL_RIGHT (3) OL_TOP
 *		(4) OL_BOTTOM (5) OL_NONE
 *		
 *******************************file*header*******************************
 */

						/* #includes go here	*/
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ArrowP.h>

#define ClassName Arrow
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
					/* private procedures		*/
static void DrawDownArrow ();
static void DrawLeftArrow ();
static void DrawRect ();
static void DrawRightArrow ();
static void DrawUpArrow ();
static void PaintArrow ();
static void PopulateGcs ();
static void TimerEvent OL_ARGS((XtPointer, XtIntervalId *));
static void UnMap();
					/* class procedures		*/
static void ClassInitialize OL_NO_ARGS();
static void Destroy OL_ARGS((Widget));
static void Initialize ();
static void Realize OL_ARGS((Widget, XtValueMask *, XSetWindowAttributes *));
static void Redisplay ();
static Boolean SetValues ();
					/* action procedures		*/
static void BtnDown ();
static void BtnMotion ();
static void BtnUp ();
					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */
static Dimension defDim = 8;
static Dimension defBorder = 0;
static int defScale = 12;
static int defRepeatRate = 0;	/* NO AUTO REPEAT */
static int defInitialDelay = 0;	/* NO AUTO REPEAT */
static int defCenterLine = 0;
/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */

static void	HandleButton();
static void	HandleMotion();

static char defaultTranslations [] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<BtnMotion>:	OlAction()";

static OlEventHandlerRec event_procs[] =
{
	{ ButtonPress,		HandleButton	},
	{ ButtonRelease,	HandleButton	},
	{ MotionNotify,		HandleMotion	}
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = {
   {XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
      XtOffset(ArrowWidget, core.width), XtRInt, (XtPointer)&defDim},
   {XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
      XtOffset(ArrowWidget, core.height), XtRInt, (XtPointer)&defDim},
   {XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
      XtOffset(ArrowWidget, core.background_pixel), XtRString, "White"},
   {XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
      XtOffset(ArrowWidget, core.border_width), XtRDimension, (XtPointer)&defBorder},
   {XtNbtnDown, XtCCallback, XtRCallback, sizeof(XtPointer), 
      XtOffset(ArrowWidget, arrow.btnDown), XtRCallback, NULL},
   {XtNbtnMotion, XtCCallback, XtRCallback, sizeof(XtPointer), 
      XtOffset(ArrowWidget, arrow.btnMotion), XtRCallback, NULL},
   {XtNbtnUp, XtCCallback, XtRCallback, sizeof(XtPointer), 
      XtOffset(ArrowWidget, arrow.btnUp), XtRCallback, NULL},
   {XtNdirection, XtCDirection, XtROlDefine, sizeof(OlDefine),
      XtOffset(ArrowWidget, arrow.direction), XtRImmediate,
						(XtPointer) ((OlDefine) OL_TOP) },
   {XtNscale, XtCScale, XtRInt, sizeof(int),
      XtOffset(ArrowWidget, arrow.scale), XtRInt, (XtPointer)&defScale},
   {XtNcenterLine, XtCCenterLine, XtRInt, sizeof(int),
      XtOffset(ArrowWidget, arrow.centerLine), XtRInt, (XtPointer)&defCenterLine},
   {XtNrepeatRate, XtCRepeatRate, XtRInt, sizeof(int),
      XtOffset(ArrowWidget, arrow.repeatRate), XtRInt, (XtPointer)&defRepeatRate},
   {XtNinitialDelay, XtCInitialDelay, XtRInt, sizeof(int),
      XtOffset(ArrowWidget, arrow.initialDelay), XtRInt, (XtPointer)&defInitialDelay},
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

ArrowClassRec arrowClassRec = {
   {
/* core class fields */
#define superclass (&primitiveClassRec)
    /* superclass         */   (WidgetClass) superclass,
    /* class name         */   "Arrow",
    /* size               */   sizeof(ArrowRec),
    /* class initialize   */   ClassInitialize,
    /* class_part_init    */   NULL,
    /* class_inited       */   FALSE,
    /* initialize         */   Initialize,
    /* initialize_hook    */   NULL,
    /* realize            */   Realize,
    /* actions            */   NULL,
    /* num_actions        */   0,
    /* resourses          */   resources,
    /* resource_count     */   XtNumber(resources),
    /* xrm_class          */   NULLQUARK,
    /* compress_motion    */   TRUE,
    /* compress_exposure  */   TRUE,
    /* compress_enterleave*/   TRUE,
    /* visible_interest   */   FALSE,
    /* destroy            */   Destroy,
    /* resize             */   NULL,
    /* expose             */   Redisplay,
    /* set_values         */   SetValues,
    /* set_values_hook    */   NULL,
    /* set_values_almost  */   XtInheritSetValuesAlmost,
    /* get_values_hook    */   NULL,
    /* accept_focus       */   XtInheritAcceptFocus,
    /* version            */   XtVersion,
    /* callback_private   */   NULL,
    /* tm_table           */   defaultTranslations,
    /* query_geometry     */   NULL,
   },
   {
        /* focus_on_select	*/	True,
	/* highlight_handler	*/	XtInheritHighlightHandler,
	/* traversal_handler	*/	NULL,
	/* register_focus	*/	NULL,
	/* activate		*/	NULL,
	/* event_procs		*/	event_procs,
	/* num_event_procs	*/	XtNumber(event_procs),
	/* version		*/	OlVersion,
	/* extension		*/	NULL
  },	/* End of Primitive field initializations */
   {
    /* empty              */   0  /* make C compiler happy */
   }
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass arrowWidgetClass = (WidgetClass) &arrowClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */
/*
 *************************************************************************
 * DrawDownArrow - Draw an arrow pointing down: this seemly primitive
 *		algorithm is used, because X does not draw a polygon as
 *		I would like it.
 ****************************procedure*header*****************************
 */
static void
DrawDownArrow(w, d, gc, x, y, dx, dy)
   ArrowWidget w;
   Drawable d;
   GC gc;
   int x;
   int y;
   int dx;
   int dy;
{
   int i,j;

   j = 0;
   XDrawLine (XtDisplay(w), d, gc, x, y, x+dx-1, y);
   for (i=y+1; i<=y+dy-1; i++, j++)
	XDrawLine (XtDisplay(w), d, gc, x+j, i, x+dx-1-j, i);
}
/*
 *************************************************************************
 * DrawLeftArrow - Draw an arrow pointing left: this seemly primitive
 *		algorithm is used, because X does not draw a polygon as
 *		I would like it.
 ****************************procedure*header*****************************
 */
static void
DrawLeftArrow(w, d, gc, x, y, dx, dy)
   ArrowWidget w;
   Drawable d;
   GC gc;
   int x;
   int y;
   int dx;
   int dy;
{
   int i, j;

   j=0;
   XDrawLine (XtDisplay(w), d, gc, x+dx-1, y, x+dx-1, y+dy-1);
   for (i=x+dx-2; i>=x; i-- ,j++)
	XDrawLine (XtDisplay(w), d, gc, i, y+j, i, y+dy-1-j);
}

/*
 *************************************************************************
 * DrawRect - Draws a rectangle. This is done by setting window attributes
 *		and using the window as a rectangle.
 ****************************procedure*header*****************************
 */
static void
DrawRect(w, normal, chg_of_size)
   ArrowWidget w;
   Boolean normal;
   Boolean  chg_of_size;
{
   Pixel foreground, background;

   foreground = (normal)? w->primitive.foreground:
   				w->core.background_pixel;
   background = (normal)? w->core.background_pixel:
   				w->primitive.foreground;

   w->core.border_pixel = w->primitive.foreground;
   w->core.border_width = (Dimension) 1;
   XSetWindowBackground(XtDisplay(w), XtWindow(w), background);
   XSetWindowBorder(XtDisplay(w), XtWindow(w), w->core.border_pixel);
   XSetWindowBorderWidth(XtDisplay(w), XtWindow(w), 1);
   if (chg_of_size)
       XtConfigureWidget((Widget)w, w->core.x,w->core.y, w->core.width, w->core.height,1);
   XClearWindow (XtDisplay((Widget)w), XtWindow((Widget)w));
   XSync(XtDisplay(w), 0);
}
/*
 *************************************************************************
 * DrawRightArrow - Draw an arrow pointing right: this seemly primitive
 *		algorithm is used, because X does not draw a polygon as
 *		I would like it.
 ****************************procedure*header*****************************
 */
static void
DrawRightArrow(w, d, gc, x, y, dx, dy)
   ArrowWidget w;
   Drawable d;
   GC gc;
   int x;
   int y;
   int dx;
   int dy;
{
   int i, j;

   XDrawLine (XtDisplay(w), d, gc, x, y, x, y+dy-1);
   j = 0;
   for (i=x+1; i<=x+dx-1; i++ ,j++)
	XDrawLine (XtDisplay(w), d, gc, i, y+j, i, y+dy-1-j);
}
/*
 *************************************************************************
 * DrawUpArrow - Draw an arrow pointing up: this seemly primitive
 *		algorithm is used, because X does not draw a polygon as
 *		I would like it.
 ****************************procedure*header*****************************
 */
static void
DrawUpArrow(w, d, gc, x, y, dx, dy)
   ArrowWidget w;
   Drawable d;
   GC gc;
   int x;
   int y;
   int dx;
   int dy;
{
   int i,j;

   j = 0;
   XDrawLine (XtDisplay(w), d, gc, x, y+dy-1, x+dx-1, y+dy-1);
   for (i=y+dy-2; i>=y; i--, j++) 
	XDrawLine (XtDisplay(w), d, gc, x+j, i, x+dx-1-j, i);
}
/*
 *************************************************************************
 * PaintArrow: This is a sophisticated routine. It has two modes:
 *		compute and standard: when compute is true
 *		only the sizes are computed, otherwise an "arrow" is
 *		also drawn. There are several types of arrows possible:
 *		OL_LEFT; OL_RIGHT; OL_TOP; OL_BOTTOM;
 *		OL_NONELEFT; OL_NONERIGHT; OL_NONETOP; OL_NONEBOTTOM;
 *		OL_NONE
 *		The first four are obvious. Size is based upon scale
 *		resource.
 *		The next four, are empty rectangles, but have the same
 *		window size and shape as their siblings in the set of the
 *		first four. Finally NONE, allows the application to
 *		specify the size and.
 ****************************procedure*header*****************************
 */
static void
PaintArrow(w,normal,dimension, compute, chg_of_size)
   ArrowWidget w;
   Boolean normal; /* T==> normal, F==> rev. video */
   XPoint *dimension;
   Boolean compute; /* T==> Don't paint, just compute; F==> paint and compute */
   Boolean  chg_of_size;
{
   Boolean H;
   int wd, ht, n;
   XPoint or;
   int dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy3, dy4, dy5, dy6;

   if (w->arrow.direction==OL_TOP        ||
       w->arrow.direction==OL_BOTTOM      ||
      (w->arrow.direction) == OL_NONETOP   ||
      (w->arrow.direction) == OL_NONEBOTTOM ||
       w->arrow.direction==OL_NONE)
	H = FALSE;
   else
	H = TRUE;

   or.x = 0;
   or.y = 0;
   if (H)
   {
	int dir;
	Screen	*myscreen = XtScreen(w);
	dir =OL_VERTICAL;
	if (w->core.height == (Dimension) 0 || w->arrow.direction!=0) {
		n = OlScreenPointToPixel(dir,
			 (double) w->arrow.scale, myscreen);
		if (n<3) {

			OlVaDisplayWarningMsg(	XtDisplay((Widget)w),
						OleNinvalidDimension,
						OleTwidgetSize,
						OleCOlToolkitWarning,
						OleMinvalidDimension_widgetSize,
						XtName((Widget)w),
						OlWidgetToClassName((Widget)w));
			n = 3;
		}
		ht = (n/2)*2;
		wd = (n/3)*3;
	} else
	{
   		dimension->x = (short) w->core.width;
   		dimension->y = (short) w->core.height;
		if (!compute) {
		    DrawRect(w,normal, chg_of_size);
		    return;
		} else {
		    return;
		}
	}
	dx3 = wd/3;
	dy3 = ht/6;
	dy4 = ht - dy3 -dy3;
	dx4 = dy4/2 + 1;
   } else
   {
	int dir;
	Screen	*myscreen = XtScreen(w);
	dir = OL_HORIZONTAL;
	if (w->core.width == (Dimension)0 || w->arrow.direction!=0) {
		n = OlScreenPointToPixel(dir,
			 (double) w->arrow.scale, myscreen);
		if (n<3) {
			OlVaDisplayWarningMsg(	XtDisplay((Widget)w),
						OleNinvalidDimension,
						OleTwidgetSize,
						OleCOlToolkitWarning,
						OleMinvalidDimension_widgetSize,
						XtName((Widget)w),
						OlWidgetToClassName((Widget)w));
			n = 3;
		}
		wd = (n/2)*2;
		ht = (n/3)*3;
	}
	else {
   		dimension->x = (short) w->core.width;
   		dimension->y = (short) w->core.height;
		if (!compute) {
		    DrawRect(w,normal, chg_of_size);
		    return;
		} else {
		    return;
		}
	}
	dx3 = wd/6;
	dy3 = ht/3;
	dx4 = wd - dx3 -dx3;
	dy4 = dx4/2 + 1;
   }

   dx1 = (n/30) + 1;
   dy2 = (n/30) + 1;
   if (H)
	w->arrow.centerLine = dy2 + ht/2;
   else
	w->arrow.centerLine = dx1 + wd/2;
   dx5 = (n*3/20);
   dx5 = (dx5<=0)?1 : dx5;
   dy6 = (dx5<=0)?1 : dx5;
    

   dy1 =  dy2 + ht + dy6;
   dx2 = wd;

   dy5 = dy1;
   dx6 = dx2;

   dimension->x = dx1+wd+dx5;
   dimension->y = dy2+ht+dy6;

   if (compute)		/* JUST COMPUTING DIMNENSIONS */
	return;


        XFillRectangle( XtDisplay(w),
   	   XtWindow(w),
   	   (normal)?(w->arrow.bggc) : (w->arrow.fggc),
   	   or.x,
   	   or.y,
   	   wd+dx1+dx5,
   	   ht+dy2+dy6);
   	   
   if (normal) {
           XFillRectangle( XtDisplay(w),
   	   XtWindow(w),
   	   w->arrow.fggc,
   	   or.x,
   	   or.y,
   	   dx1,
   	   dy1);
   	   

           XFillRectangle( XtDisplay(w),
   	   XtWindow(w),
   	   w->arrow.fggc,
   	   or.x+dx1,
   	   or.y,
   	   dx2,
   	   dy2);
   	   
           XFillRectangle( XtDisplay(w),
   	   XtWindow(w),
   	   w->arrow.fggc,
   	   or.x+dx1+wd,
   	   or.y,
   	   dx5,
   	   dy5);
   	   
           XFillRectangle( XtDisplay(w),
   	   XtWindow(w),
   	   w->arrow.fggc,
   	   or.x+dx1,
   	   or.y+dy2+ht,
   	   dx6,
   	   dy6);
       }
        switch (w->arrow.direction) {
	case OL_NONE:
	case OL_NONETOP:
	case OL_NONEBOTTOM:
	case OL_NONELEFT:
	case OL_NONERIGHT:
		break;
        case OL_TOP: DrawUpArrow(w,
   		XtWindow(w),
   		(normal)? (w->arrow.fggc) : (w->arrow.bggc),
   		or.x+dx1+dx3, 
   		or.y+dy2+dy3,
   		dx4,
   		dy4);
		break;
        case OL_BOTTOM: DrawDownArrow(w,
   		XtWindow(w),
   		(normal)? (w->arrow.fggc) : (w->arrow.bggc),
   		or.x+dx1+dx3, 
   		or.y+dy2+dy3,
   		dx4,
   		dy4);
       break;
        case OL_RIGHT: DrawRightArrow(w,
   		XtWindow(w),
   		(normal)? (w->arrow.fggc) : (w->arrow.bggc),
   		or.x+dx1+dx3, 
   		or.y+dy2+dy3,
   		dx4,
   		dy4);
       break;
        case OL_LEFT: DrawLeftArrow(w,
   		XtWindow(w),
   		(normal)? (w->arrow.fggc) : (w->arrow.bggc),
   		or.x+dx1+dx3, 
   		or.y+dy2+dy3,
   		dx4,
   		dy4);
       break;
        }
}

/*
 *************************************************************************
 * PopulateGcs: Populates the GCs;
 ****************************procedure*header*****************************
 */
static void
PopulateGcs (w)
    ArrowWidget w;
{
    XGCValues values;
    Pixel     temp_pix;

    values.foreground =  w->primitive.foreground;
    values.background =  w->core.background_pixel;
   			
    if (w->arrow.fggc != NULL)
	XtReleaseGC((Widget)w, w->arrow.fggc);
    if (w->arrow.bggc != NULL)
	XtReleaseGC((Widget)w, w->arrow.bggc);
    w->arrow.fggc = XtGetGC((Widget)w,
			    (unsigned) (GCForeground | GCBackground),
			    &values);

    temp_pix = values.foreground;
    values.foreground = values.background;
    values.background = temp_pix;
    w->arrow.bggc = XtGetGC((Widget)w,
			    (unsigned) (GCForeground | GCBackground),
			    &values);
}

/*
 *************************************************************************
 * TimerEvent: Timeout for repeatRate and reactivates iteself.
 ****************************procedure*header*****************************
 */
static void
TimerEvent OLARGLIST((client_data, id))
	OLARG( XtPointer, client_data)
	OLGRA( XtIntervalId *,	id)
{
    ArrowWidget w = (ArrowWidget)client_data;
    XtCallCallbacks( (Widget)w, XtNbtnDown, NULL);

    /* continue to repeat if repeat rate and Unmap event haven't happened */
    if ((w->arrow.repeatRate > 0) && (w->arrow.timerid != (XtIntervalId)NULL)) {
	w->arrow.timerid = OlAddTimeOut(
		(Widget)w, (w->arrow.repeatRate), TimerEvent, (XtPointer)w);
}
    else
	w->arrow.timerid = (XtIntervalId)NULL;
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */
/*
************************************************************
 *  ClassInitialize - Register OlDefine string values.
 *********************procedure*header************************
 */

static void
ClassInitialize OL_NO_ARGS()
{
	_OlAddOlDefineType ("top",    OL_TOP);
	_OlAddOlDefineType ("bottom", OL_BOTTOM);
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("right",  OL_RIGHT);
	_OlAddOlDefineType ("none",   OL_NONE);
}
/*
 ************************************************************
 *
 *  Destroy: Destroys GCS
 *
 *********************procedure*header************************
 */

static void
Destroy OLARGLIST((ww))
	OLGRA( Widget,	ww)
{
	ArrowWidget w = (ArrowWidget)ww;

    if (w->arrow.timerid) {
		XtRemoveTimeOut(w->arrow.timerid);
   		w->arrow.timerid = (XtIntervalId) NULL;
    }
    XtRemoveEventHandler(ww,StructureNotifyMask,False,UnMap,NULL);

    XtRemoveAllCallbacks(ww,XtNbtnDown);
    XtRemoveAllCallbacks(ww,XtNbtnMotion);
    XtRemoveAllCallbacks(ww,XtNbtnUp);

    XtReleaseGC((Widget)w, w->arrow.fggc);
    XtReleaseGC((Widget)w, w->arrow.bggc);
}

/*
 *************************************************************************
 * Initialize: Computes the starting size based on scale.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
   Widget	request;		/* what the client asked for */
   Widget	new;			/* what we're going to give him */
   ArgList	args;
   Cardinal *	num_args;
{
   ArrowWidget w = (ArrowWidget) new;
   XPoint dim;

   w->arrow.normal = TRUE;
   w->arrow.fggc = NULL;
   w->arrow.bggc = NULL;
   PopulateGcs (w);
   w->arrow.timerid = (XtIntervalId) NULL;
   PaintArrow(w, w->arrow.normal, &dim, TRUE, TRUE);
   w->core.width = (Dimension) dim.x;
   w->core.height = (Dimension) dim.y;

   /* need to keep track of unmap event */
   XtAddEventHandler((Widget)w,StructureNotifyMask,False,UnMap,NULL);
}

/*
 *************************************************************************
 * Realize - Creates The window and paints the arrow.
 ****************************procedure*header*****************************
 */
static void
Realize OLARGLIST((w, valueMask, attributes))
	OLARG( Widget,			w)
	OLARG( XtValueMask *,		valueMask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
   XPoint dim;

   XtCreateWindow( w, InputOutput, (Visual *)CopyFromParent,
		    *valueMask, attributes );
/*
   PaintArrow(w, TRUE, &dim, FALSE, FALSE);
*/
    
} /* Realize */


/*
 *************************************************************************
 * Redisplay: Redraws the Arrow
 ****************************procedure*header*****************************
 */

static void
Redisplay(w, event, region)
    ArrowWidget w;
    XEvent *event;
    Region region;
{
   XPoint dim;

   PaintArrow(w, w->arrow.normal, &dim, FALSE, FALSE);

} /* Redisplay */


/*
 ************************************************************
 *
 *  SetValues - This function compares the requested values
 *      to the current values, and sets them in the new
 *      widget.  It returns TRUE when the widget must be
 *      redisplayed.
 *
 *********************procedure*header************************
 */
/* ARGSUSED */
static Boolean
SetValues (current, request, new, args, num_args)
    Widget current;
    Widget request;
    Widget new;
    ArgList args;
    Cardinal *num_args;
{
    ArrowWidget aw = (ArrowWidget) current;
    ArrowWidget newaw = (ArrowWidget) new;

    if (newaw->core.background_pixel != aw->core.background_pixel ||
	newaw->primitive.foreground != aw->primitive.foreground) {
        XPoint dim;

	PopulateGcs (newaw);	
	PaintArrow(newaw, newaw-> arrow.normal, &dim, FALSE, FALSE);
    }
    return (FALSE);
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
 * BtnDown - Call Call back and paints new arrow (inverted). 
 *	Activates timeout.
 ****************************procedure*header*****************************
 */
static void
BtnDown( ww, event)
    Widget ww;
    XEvent *event;
{
    ArrowWidget w = (ArrowWidget)ww;
    XPoint dim;

    XtCallCallbacks( (Widget)w, XtNbtnDown, event);
    w->arrow.normal = FALSE;
    PaintArrow(w, w->arrow.normal, &dim, FALSE, FALSE);
    if (w->arrow.initialDelay > 0) {
	w->arrow.timerid = OlAddTimeOut(
		(Widget)w, w->arrow.initialDelay, TimerEvent, (XtPointer)w);
    }
}

/* ARGSUSED */
static void
UnMap(w, data, event, continue_to_dispatch)
    ArrowWidget w;
    XtPointer data;
    XEvent *event;
    Boolean *continue_to_dispatch;
{
	if ((event->type == UnmapNotify) || (event->type == DestroyNotify)) {
	    if (w->arrow.timerid != (XtIntervalId)NULL) {
		XtRemoveTimeOut(w->arrow.timerid);
		w->arrow.timerid = (XtIntervalId)NULL;
		w->arrow.normal = TRUE;
	    }
	}
}

/*
 *************************************************************************
 * BtnMotion - Call Call back 
 ****************************procedure*header*****************************
 */
static void
BtnMotion( w, event)
    ArrowWidget w;
    XEvent *event;
{
    if (w->arrow.timerid) {
	if (w->arrow.btnMotion) {
		XtRemoveTimeOut (w->arrow.timerid);
		w->arrow.timerid = (XtIntervalId) NULL;
		XtCallCallbacks( (Widget)w, XtNbtnMotion, event);
    	}
    } else
	XtCallCallbacks( (Widget)w, XtNbtnMotion, event);

}

/*
 *************************************************************************
 * BtnUp - Removes timeout
 ****************************procedure*header*****************************
 */
static 
void BtnUp( ww, event)
    Widget ww;
    XEvent *event;
{
    ArrowWidget w = (ArrowWidget)ww;
    XPoint dim;

    if (w->arrow.timerid) {
	XtRemoveTimeOut (w->arrow.timerid);
	w->arrow.timerid = (XtIntervalId) NULL;
    }
    XtCallCallbacks( (Widget)w, XtNbtnUp, event);
    w->arrow.normal = TRUE;
    PaintArrow(w, w->arrow.normal, &dim, FALSE, FALSE);
}

static void
HandleButton(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			if (ve->xevent->type = ButtonPress)
				BtnDown(w, ve->xevent);
			else
				BtnUp(w, ve->xevent);
			break;
		default:
			break;
	}
} /* end of HandleButton */

static void
HandleMotion(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			BtnMotion(w, ve->xevent);
			break;
		default:
			break;
	}
} /* end of HandleMotion */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */
