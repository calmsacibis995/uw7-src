/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olhelp:Mag.c	1.23"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file contains the magnifier glass widget code
 *	for the OPEN LOOK (tm) toolkit
 *
 *******************************file*header*******************************
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/MagP.h>
				
#define ClassName Mag
#include <Xol/NameDefs.h>

				/* The following includes don't belong
				 * here since we're supposed to be 
				 * using _OlGetBitmap() or _OlGetImage.
				 * Until then, leave the includes	*/
#include "Xol/magnifier"
#include "Xol/mag_clip"
#include "Xol/clip_mask"

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

static	void	CacheImage();		/* Caches magnifier's contents	*/
					/* obtained from root window    */
static	void	CacheAppImage();	/* Caches magnifier's contents	*/
					/* from app-supplied pixmap	*/
static	void	SetUpGraphics();	/* Create the pixmaps		*/

					/* class procedures		*/

static	void	Destroy();		/* destroy an instance		*/
static	void	Initialize();		/* initialize new instance	*/
static 	void	Redisplay();		/* handle exposures		*/
static 	Boolean	SetValues();		/* manage attribute changes	*/

					/* action procedures		*/

/* There are no action procedures */

					/* public procedures		*/

/* There are no public procedures */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

			/* The following defines only exist since we've
			 * hard-coded the pixmap data.  Later when we
			 * have time, get rid of this junk and use
			 * XImages					*/

#define LENS_IMAGE_X		21
#define LENS_IMAGE_Y		24
#define LENS_IMAGE_WIDTH	70
#define LENS_IMAGE_HEIGHT	60
#define MAGNIFIER_X		-16
#define MAGNIFIER_Y		20

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */

/* There are no translations or action procedures */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define offset(field)  XtOffset(MagWidget, field)

static XtResource
resources[] = {
	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
             offset(core.background_pixel), XtRString,
	     (XtPointer)XtDefaultBackground },

	{ XtNmouseX, XtCMouseX, XtRPosition, sizeof(Position),
             offset(mag.mouseX), XtRImmediate, (XtPointer) 100 },

	{ XtNmouseY, XtCMouseY, XtRPosition, sizeof(Position),
             offset(mag.mouseY), XtRImmediate, (XtPointer) 100 },

	{ XtNpixmap, XtCPixmap, XtRPixmap, sizeof(Pixmap),
	     offset(mag.pixmap), XtRImmediate, (XtPointer) NULL },
};

#undef offset

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */
MagClassRec
magClassRec = {
  {
	(WidgetClass) &primitiveClassRec,	/* superclass		*/
	"Mag",					/* class_name		*/
	sizeof(MagRec),				/* widget_size		*/
	NULL,					/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	NULL,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	NULL,					/* resize		*/
	Redisplay,				/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	NULL,					/* tm_table		*/
	NULL,					/* query_geometry	*/
  }, /* End of CoreClass field initialization	*/
  {
        True,					/* focus_on_select	*/
	NULL,					/* highlight_handler	*/
	NULL,					/* traversal_handler	*/
	NULL,					/* register_focus	*/
	NULL,					/* activate		*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL					/* extension		*/
  },	/* End of Primitive field initializations */
  { /* mag fields */
	NULL					/* field not used	*/
  } /* End of MagClass field initialization	*/
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass magWidgetClass = (WidgetClass)&magClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * CacheImage - this routine caches a portion of the RootWindow into a
 * pixmap.  This image is used as the contents of the magnifier.
 ****************************procedure*header*****************************
 */
static void
CacheImage(mw)
	MagWidget mw;
{
	Display *	dpy = XtDisplay((Widget)mw);
	Screen *	screen = XtScreen((Widget) mw);
	int		x = (int)mw->mag.mouseX - LENS_IMAGE_WIDTH / 2,
			y = (int)mw->mag.mouseY - LENS_IMAGE_HEIGHT / 2;

	if (x < 0 || (x + LENS_IMAGE_WIDTH) > WidthOfScreen(screen) ||
	    y < 0 || (y + LENS_IMAGE_HEIGHT) > HeightOfScreen(screen)) {
		XtGCMask	mask = NULL;
		XGCValues	gcv;
		GC		clear_GC;

		gcv.graphics_exposures	= False;
		gcv.foreground		= BlackPixelOfScreen(screen);
		gcv.background		= BlackPixelOfScreen(screen);

		mask = GCGraphicsExposures | GCForeground | GCBackground;

		clear_GC = XtGetGC((Widget) mw, mask, &gcv);

				/* Must Clear out the old Pixmap since
				 * we're off the screen			*/

		XFillRectangle(dpy, mw->mag.hold_pixmap, clear_GC, 0, 0,
			(unsigned int) LENS_IMAGE_WIDTH, 
			(unsigned int) LENS_IMAGE_HEIGHT); 

		XtReleaseGC((Widget)mw, clear_GC);
	}

	if ( DefaultDepthOfScreen(XtScreen((Widget)mw)) != 1 )
	{
		XCopyArea(dpy, RootWindowOfScreen(screen), mw->mag.hold_pixmap,
			mw->mag.hold_GC, x, y,
			(unsigned int)LENS_IMAGE_WIDTH,
			(unsigned int)LENS_IMAGE_HEIGHT, 0, 0);
	}
	else
	{
		XCopyPlane(dpy, RootWindowOfScreen(screen), mw->mag.hold_pixmap,
			mw->mag.hold_GC, x, y,
			(unsigned int)LENS_IMAGE_WIDTH,
			(unsigned int)LENS_IMAGE_HEIGHT, 0, 0,
			(unsigned long)1);
	}

} /* END OF CacheImage() */

/*
 *************************************************************************
 * CacheAppImage - this routine caches an application-supplied pixmap.
 * The pixmap is used as the contents of the magnifier.
 ****************************procedure*header*****************************
 */
static void
CacheAppImage(mw)
     MagWidget mw;
{
     Window       ignore_win;
     int          ignore_xy;
     unsigned int ignore_val,
                  pix_height,
                  pix_width,
                  pix_depth,
                  mwpix_height,
                  mwpix_width,
                  mwpix_depth,
                  width,
                  height,
                  x, y;
     XGCValues   gcv;
     GC          clear_GC;
     XtGCMask    mask = NULL;
     Display     *dpy = XtDisplay((Widget)mw);
     Screen      *screen = XtScreen((Widget) mw);

	/* get width, height and depth of magnifier pixmap */
	XGetGeometry(dpy, mw->mag.mag_pixmap, &ignore_win, &ignore_xy,
			&ignore_xy, &mwpix_width, &mwpix_height, &ignore_val,
			&mwpix_depth);

	/* get width, height and depth of application pixmap */
	XGetGeometry(dpy, mw->mag.pixmap, &ignore_win, &ignore_xy,
			&ignore_xy, &pix_width, &pix_height, &ignore_val,
			&pix_depth);

	/* calculate destination x */
	if (pix_width < mwpix_width) {
		x = ((mwpix_width - pix_width) / 2) - (pix_width / 2) - 5;
		width = pix_width;
	} else {
		x = 0; /* should this be adjusted? */
		width = LENS_IMAGE_WIDTH;
	}

	/* calculate destination y */
	if (pix_height < mwpix_height) {
		y = (mwpix_height / 2) - pix_height;
		height = pix_height;
	} else {
		y = 0; /* should this be adjusted? */
		height = LENS_IMAGE_HEIGHT;
	}

	/* if pixmap is smaller than size of magnifier, set background */
	/* of magnifier to black.				       */

	if (pix_height < mwpix_height || pix_width < mwpix_width) {
		gcv.graphics_exposures = False;
		gcv.foreground         = BlackPixelOfScreen(screen);
		gcv.background         = BlackPixelOfScreen(screen);

		mask = GCGraphicsExposures | GCForeground | GCBackground;

		clear_GC = XtGetGC((Widget) mw, mask, &gcv);

		XFillRectangle(dpy, mw->mag.hold_pixmap, clear_GC, 0, 0,
			(unsigned int) LENS_IMAGE_WIDTH, 
			(unsigned int) LENS_IMAGE_HEIGHT); 

		XtReleaseGC((Widget)mw, clear_GC);
	}

	if (pix_depth == 1) { /* bitmap */
		XCopyPlane(dpy, mw->mag.pixmap, mw->mag.hold_pixmap,
			mw->mag.hold_GC, 0, 0,
			(unsigned int)width,
			(unsigned int)height,
			x, y,
			(unsigned long)1);

	} else { /* pixmap */
		XCopyArea(dpy, mw->mag.pixmap, mw->mag.hold_pixmap,
			mw->mag.hold_GC, 0, 0,
			(unsigned int)width,
			(unsigned int)height,
			x, y);
	}

} /* END OF CacheAppImage() */

/*
 *************************************************************************
 * SetUpGraphics - this routine allocates memory for the magnifier's
 * pixmaps.  It also creates pixmaps for the GCs.
 ****************************procedure*header*****************************
 */
static void
SetUpGraphics(w)
	Widget w;
{
	MagWidget	mw = (MagWidget) w;
	XtGCMask	mask = NULL;
	XGCValues	gcv;
	Display	*	dpy = XtDisplay(w);
	Screen	*	screen = XtScreen(w);

	gcv.graphics_exposures	= False;
	gcv.subwindow_mode	= IncludeInferiors;
	gcv.clip_x_origin	= LENS_IMAGE_X;
	gcv.clip_y_origin	= LENS_IMAGE_Y;
	gcv.clip_mask		= XCreateBitmapFromData(dpy,
				 	RootWindowOfScreen(screen),
					(char *)clip_mask_bits,
				 	(unsigned int)clip_mask_width,
				 	(unsigned int)clip_mask_height);

	mask = GCGraphicsExposures | GCSubwindowMode | GCClipXOrigin |
		GCClipYOrigin | GCClipMask;
	mw->mag.copy_GC = XtGetGC(w, mask, &gcv);


	mask = GCGraphicsExposures | GCSubwindowMode;
	mw->mag.hold_GC = XtGetGC(w, mask, &gcv);

	gcv.foreground		= BlackPixelOfScreen(XtScreen(w));
	gcv.background		= WhitePixelOfScreen(XtScreen(w));
	gcv.clip_x_origin	= MAGNIFIER_X;
	gcv.clip_y_origin	= MAGNIFIER_Y;
	gcv.clip_mask		= XCreateBitmapFromData(dpy,
				 	RootWindowOfScreen(screen),
					(char *)mag_clip_bits,
				 	(unsigned int)mag_clip_width,
					(unsigned int)mag_clip_height);

	mask = GCGraphicsExposures | GCForeground | GCBackground |
		GCClipMask | GCClipXOrigin | GCClipYOrigin;
	mw->mag.mag_GC = XtGetGC(w, mask, &gcv);

					/* Now create the pixmaps	*/

	mw->mag.mag_pixmap = XCreateBitmapFromData(dpy,
				 	RootWindowOfScreen(screen),
					(char *)magnifier_bits,
					(unsigned int)magnifier_width,
					(unsigned int)magnifier_height);

		/* Create the Pixmap used to get RootWindow Image	*/

	mw->mag.hold_pixmap = XCreatePixmap(dpy, RootWindowOfScreen(screen),
			(unsigned int)LENS_IMAGE_WIDTH,
			(unsigned int)LENS_IMAGE_HEIGHT,
			(unsigned int)DefaultDepthOfScreen(XtScreen(w)));
} /* END OF SetUpGraphics() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * Destroy - This destroys a magnifier instance.
 * We destroy all pixmaps, including those used as the clip masks in the
 * GCs
 ****************************procedure*header*****************************
 */
static void
Destroy(w)
	Widget w;			/* The magnifier widget		*/
{
	MagWidget	mw = (MagWidget) w;
	Display *	dpy = XtDisplay(w);

						/* Free GC pixmaps	*/

	XFreePixmap(dpy, mw->mag.copy_GC->values.clip_mask);
	XFreePixmap(dpy, mw->mag.mag_GC->values.clip_mask);

						/* Free other pixmaps	*/

	XFreePixmap(dpy, mw->mag.mag_pixmap);
	XFreePixmap(dpy, mw->mag.hold_pixmap);

							/* Free GCs	*/
	XtReleaseGC((Widget)mw, mw->mag.copy_GC);
	XtReleaseGC((Widget)mw, mw->mag.mag_GC);
	XtReleaseGC((Widget)mw, mw->mag.hold_GC);

} /* END OF Destroy() */

/*
 *************************************************************************
 * Initialize - this initializes the magnifier widget instance.
 * It basically is responsible for creating the pixmaps for the instance.
 * It also enforces the width and height of the instance.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
   Widget request;			/* what the client wants	*/
   Widget new;				/* what the client gets		*/
   ArgList	args;
   Cardinal *	num_args;
{
        MagWidget mw = (MagWidget) new;
        MagWidget rw = (MagWidget) request;

	mw->mag.copy_GC		= (GC) NULL;
	mw->mag.hold_GC		= (GC) NULL;
	mw->mag.mag_GC		= (GC) NULL;
	mw->mag.mag_pixmap	= (Pixmap) NULL;
	mw->mag.hold_pixmap	= (Pixmap) NULL;

	SetUpGraphics(new);

	/* if a pixmap resouce is specified, call CacheAppImage() instead */
	if (rw->mag.pixmap == NULL) {
		CacheImage(mw);
	} else {
		CacheAppImage(mw);
	}

		/* The next two lines are ugly.  Maybe we should do a
		 * XGetGeometry() request, but what the heck.  I'd
		 * like to use XImages instead of pixmaps, but they
		 * take to long to draw.  So, save the server and
		 * use the bitmap file fields.  This will eventually
		 * go away when we use XImages so that we can get
		 * screen resolution independence.			*/

	mw->core.width = (Dimension)magnifier_width;
	mw->core.height = (Dimension)magnifier_height;
} /* END OF Initialize() */

/*
 *************************************************************************
 * Redisplay - handles the refreshing of the magnifier widget
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay(w, event, region)
	Widget		w;
	XEvent *	event;		/* unused */
	Region		region;		/* unused */
{
#ifdef WORKAROUND
		/* Later when we have time, we should make a larger
		 * pixmap for the hold image and copy the magnifier 
		 * into it rather than doing two server requests
		 * each time we service an expose event			*/
#endif
	Display *	dpy = XtDisplay(w);
	MagWidget	mw = (MagWidget) w;

	XCopyPlane(dpy, mw->mag.mag_pixmap, XtWindow(w),
			mw->mag.mag_GC, 0, 0,
			(unsigned int)magnifier_width,
			(unsigned int)magnifier_height,
			MAGNIFIER_X, MAGNIFIER_Y, (unsigned long) 1);

	if ( DefaultDepthOfScreen(XtScreen(w)) != 1 )
	{
		XCopyArea(dpy, mw->mag.hold_pixmap, XtWindow(w),
			mw->mag.copy_GC, 0, 0,
			(unsigned int)LENS_IMAGE_WIDTH,
			(unsigned int)LENS_IMAGE_HEIGHT,
			LENS_IMAGE_X, LENS_IMAGE_Y);
	}
	else
	{
		XCopyPlane(dpy, mw->mag.hold_pixmap, XtWindow(w),
			mw->mag.copy_GC, 0, 0,
			(unsigned int)LENS_IMAGE_WIDTH,
			(unsigned int)LENS_IMAGE_HEIGHT,
			LENS_IMAGE_X, LENS_IMAGE_Y,
			(unsigned long)1);
	}

} /* END OF Redisplay() */

/*
 *************************************************************************
 * SetValues - manage the changing of the instance's attributes
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues (current, request, new, args, num_args)
	Widget current;
	Widget request;
	Widget new;
	ArgList		args;
	Cardinal *	num_args;
{
	MagWidget	cw = (MagWidget) current;
	MagWidget	nw = (MagWidget) new;
	Boolean	redisplay = False;

	if (nw->mag.pixmap == NULL) {
		if (nw->mag.mouseX != cw->mag.mouseX ||
		    nw->mag.mouseY != cw->mag.mouseY) {

			CacheImage((MagWidget) new);
			redisplay = True;
		}
	} else {
		if (nw->mag.pixmap != cw->mag.pixmap) {
			CacheAppImage((MagWidget) new);
			redisplay = True;
		}
	}

	return(redisplay);

} /* END OF SetValues() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/* There are no action procedures */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */

