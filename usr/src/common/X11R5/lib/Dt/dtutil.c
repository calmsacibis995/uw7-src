#ifndef NOIDENT
#ident	"@(#)Dt:dtutil.c	1.14"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>
#include <Dt/DesktopI.h>

typedef struct {
	Pixmap		pixmap,
			mask;
	XFontStruct *	font;
	String		label;
	GC		gc;
	Pixel		foreground;
	unsigned int	pixmap_depth;
	Dimension	icon_width,
			icon_height,
			label_width,
			label_height,
			descent,
			pixmap_width,
			pixmap_height;
	Boolean		use_icon_hint;
} IconInfoRec, *IconInfo;

#define PIXMAP		icon_info->pixmap
#define MASK		icon_info->mask
#define FONT		icon_info->font
#define LABEL		icon_info->label
#define THE_GC		icon_info->gc
#define FOREGROUND	icon_info->foreground
#define ICON_H		icon_info->icon_height
#define ICON_W		icon_info->icon_width
#define PIXMAP_DEPTH	icon_info->pixmap_depth
#define PIXMAP_W	icon_info->pixmap_width
#define PIXMAP_H	icon_info->pixmap_height
#define LABEL_H		icon_info->label_height
#define LABEL_W		icon_info->label_width
#define DESCENT		icon_info->descent
#define USE_ICON_HINT	icon_info->use_icon_hint

static void		CalcXY(Widget, IconInfo,
			       Position *, Position *, Position *, Position *);
static void		DestroyCB(Widget, XtPointer, XtPointer);
static void		ExposeEH(Widget, XtPointer, XEvent *, Boolean *);
static void		SetIconInfo(Widget, ArgList, Cardinal,
				    Boolean, int, int);

static void
CalcXY(
	Widget		w,
	IconInfo	icon_info,
	Position *	pixmap_x,
	Position *	pixmap_y,
	Position *	label_x,
	Position *	label_y
)
{
	int		delta, gap, x_gap, y_gap;

	if (USE_ICON_HINT == False)
	{
		Arg	args[2];

		XtSetArg(args[0], XtNwidth, (XtArgVal)&ICON_W);
		XtSetArg(args[1], XtNheight,(XtArgVal)&ICON_H);
		XtGetValues(w, args, 2);

		USE_ICON_HINT = True;
	}

	if (PIXMAP != (Pixmap)None)
	{
		delta = ICON_W - PIXMAP_W;
		*pixmap_x = delta < 0 ? 0 : delta / 2;
	}

	if (LABEL == (String)NULL)
	{
		delta = ICON_H - PIXMAP_H;
		*pixmap_y = delta < 0 ? 0 : delta / 2;
		return;
	}

#define NUM_GAPS		3
			/*
			 *  x_gap is used to center "label".
			 *
			 *  y_gap is used to seperate "pixmap" from the
			 *  top and bottom of the icon_window. It's also
			 *  used to seperate "pixmap" and "label".
			 */
	x_gap = ICON_W - LABEL_W;
	y_gap = ICON_H - PIXMAP_H - LABEL_H;
	gap   = y_gap / NUM_GAPS;

	*label_x = x_gap < 0 ? 0 : x_gap / 2;
	*label_y = y_gap < 0 ? 0 : PIXMAP_H + LABEL_H + 2 * gap - DESCENT;

	*pixmap_y = gap;
#undef NUM_GAPS
} /* end of CalcXY */

static void
DestroyCB(
	Widget		w,
	XtPointer	client_data,
	XtPointer	call_data
)
{
	IconInfo	icon_info = (IconInfo)client_data;

	DtDelData((Screen *)NULL, DT_CACHE_WIDGET, (void *)w, sizeof(Widget));
	XtReleaseGC(w, THE_GC);
	if (LABEL)
		XtFree((XtPointer)LABEL);
	XtFree(client_data);
} /* end of DestroyCB */

static void
ExposeEH(
	Widget		w,
	XtPointer	client_data,
	XEvent *	xevent,
	Boolean *	cont_to_dispatch)
{
	Display *	dpy;
	IconInfo	icon_info = (IconInfo)client_data;
	Position	pixmap_x, pixmap_y, label_x, label_y;

	if (xevent != (XEvent *)NULL && xevent->type != Expose)
		return;

	if (PIXMAP == (Pixmap)None && LABEL == (String)NULL)
		return;

	CalcXY(w, icon_info, &pixmap_x, &pixmap_y, &label_x, &label_y);

	dpy = XtDisplay(w);
	if (LABEL != (String)NULL)
	{
		XDrawString(
			dpy, XtWindow(w), THE_GC, label_x, label_y,
			LABEL, strlen(LABEL)
		);
	}

	if (PIXMAP == (Pixmap)None)
		return;

	if (PIXMAP_DEPTH != 1)		/* pixmap...	*/
	{
		if (MASK)
		{
			XSetClipOrigin(dpy, THE_GC, pixmap_x, pixmap_y);
			XSetClipMask(dpy, THE_GC, MASK);
		}
		XCopyArea(
			dpy, PIXMAP, XtWindow(w),
			THE_GC, 0, 0, PIXMAP_W, PIXMAP_H, pixmap_x, pixmap_y);
	}
	else			/* bitmap...	*/
	{
		XSetClipOrigin(dpy, THE_GC, pixmap_x, pixmap_y);
		if (MASK)
		{
			XSetClipMask(dpy, THE_GC, MASK);
			XCopyPlane(
				dpy, PIXMAP, XtWindow(w),
				THE_GC, 0, 0, PIXMAP_W, PIXMAP_H,
				pixmap_x, pixmap_y, 1L
			);
		}
		else
		{
			XSetClipMask(dpy, THE_GC, PIXMAP);
			XFillRectangle(
				dpy, XtWindow(w), THE_GC,
				pixmap_x, pixmap_y, PIXMAP_W, PIXMAP_H
			);
		}
	}
		/* THE_GC is read only, so set the original value back */
	XSetClipOrigin(dpy, THE_GC, 0, 0);
	XSetClipMask(dpy, THE_GC, None);
} /* end of ExposeEH */

static void
SetIconInfo(
	Widget		shell,
	ArgList		icon_args,
	Cardinal	num_icon_args,
	Boolean		use_icon_hint,
	int		max_icon_w,
	int		max_icon_h
)
{
#define NUM_INFO	5
#define I_MASK		(Pixmap)args[0].value
#define I_LABEL		(String)args[1].value
#define I_PIXMAP	(Pixmap)args[2].value
#define I_FONT		(XFontStruct *)args[3].value
#define I_FG		(Pixel)args[4].value

#define DFT_FONT(d)	XLoadQueryFont(d,"-*-*-*-R-*-*-*-120-*-*-*-*-ISO8859-1")
#define DFT_FG(s)	BlackPixelOfScreen(s)

	Arg		args[NUM_INFO];
	Display *	dpy;
	IconInfo	icon_info;
	XGCValues	vals;

	Cardinal	i, j;

	int		direction, ascent, descent;
	XCharStruct	overall;

	if (num_icon_args == 0)
		return;

	dpy = XtDisplay(shell);

	XtSetArg(args[0], XtNiconMask,	(XtArgVal)None);
	XtSetArg(args[1], XtNiconName,	(XtArgVal)NULL);
	XtSetArg(args[2], XtNiconPixmap,(XtArgVal)NULL);
	XtSetArg(args[3], XtNfont,	(XtArgVal)NULL);
	XtSetArg(args[4], XtNforeground,(XtArgVal)DFT_FG(XtScreen(shell)));

	for (i = 0; i < NUM_INFO; i++)
		for (j = 0; j < num_icon_args; j++)
		{
			if (strcmp(args[i].name, icon_args[j].name) == 0)
			{
				args[i].value = icon_args[j].value;
				break;
			}
		}

	if (I_PIXMAP == (Pixmap)None && I_LABEL == (String)NULL)
		return;

	icon_info = (IconInfo)XtMalloc(sizeof(IconInfoRec));
	XtAddEventHandler(
		shell, (EventMask)ExposureMask, False,
		ExposeEH, (XtPointer)icon_info
	);
	XtAddCallback(
		shell, XtNdestroyCallback, DestroyCB,
		(XtPointer)icon_info
	);
	DtPutData(
		(Screen *)NULL, DT_CACHE_WIDGET,
		(void *)shell, sizeof(Widget),
		(void *)icon_info
	);

	LABEL		= (String)NULL;		/* See below	*/
	PIXMAP		= I_PIXMAP;
	MASK		= I_MASK;
	FONT		= I_FONT != (XFontStruct *)NULL ? I_FONT:DFT_FONT(dpy);
	FOREGROUND	= I_FG;
	USE_ICON_HINT	= use_icon_hint;
	if (use_icon_hint)
	{
		ICON_W	= max_icon_w;
		ICON_H	= max_icon_h;
	}
	else
	{
		ICON_W	= 0;
		ICON_H	= 0;
	}

	vals.foreground = FOREGROUND;
	vals.font	= FONT->fid;
	THE_GC		= XtGetGC(shell, GCForeground | GCFont, &vals);

	if (PIXMAP != (Pixmap)None)
	{
		Window		ignore_win;
		int		ignore_xy;
		unsigned int	ignore_val,
				pixmap_h, pixmap_w;


		XGetGeometry(
			dpy, PIXMAP,
			&ignore_win, &ignore_xy, &ignore_xy,
			&pixmap_w, &pixmap_h,
			&ignore_val,
			&PIXMAP_DEPTH
		);

		PIXMAP_W = pixmap_w;
		PIXMAP_H = pixmap_h;
	}
	else
	{
		PIXMAP_W = 0;
		PIXMAP_H = 0;
	}

	if (I_LABEL == (String)NULL)
		return;

	XTextExtents(
		FONT, I_LABEL, strlen(I_LABEL),
		&direction, &ascent, &descent, &overall
	);

	LABEL	= strdup(I_LABEL);
	LABEL_H = ascent + descent;
	LABEL_W = overall.width;
	DESCENT = descent;

#undef NUM_INFO
#undef I_MASK
#undef I_LABEL
#undef I_PIXMAP
#undef I_FONT
#undef I_FG
#undef DFT_FONT
#undef DFT_FG
} /* end of SetIconInfo */

/*
 * DtChangeProcessIcon -
 *
 * This routine is used to change a process icon. DtCreateProcessIcon
 * should be called otherwise the routine simply returns.
 */
extern void
DtChangeProcessIcon(
	Widget		target,
	Pixmap		icon_pixmap,
	Pixmap		icon_mask
)
{
	Boolean		ignore;
	IconInfo	icon_info = (IconInfo)NULL;
	Window		ignore_win;
	int		ignore_xy;
	unsigned int	ignore_val,
			pixmap_w, pixmap_h;

	icon_info = (IconInfo)DtGetData(
			(Screen *)NULL, DT_CACHE_WIDGET,
			(void *)target, sizeof(Widget)
	);

	if (icon_info == (IconInfo)NULL)
		return;

	if (MASK != (Pixmap)None && PIXMAP != (Pixmap)None)
	{
		Position	pixmap_x, pixmap_y, label_x, label_y;

		CalcXY(target, icon_info,
		       &pixmap_x, &pixmap_y, &label_x, &label_y
		);
		XClearArea(
			XtDisplay(target), XtWindow(target),
			pixmap_x, pixmap_y, PIXMAP_W, PIXMAP_H, False
		);
	}

	PIXMAP = icon_pixmap;
	MASK   = icon_mask;

	XGetGeometry(XtDisplay(target), icon_pixmap, &ignore_win,
			&ignore_xy, &ignore_xy,
			&pixmap_w, &pixmap_h,
			&ignore_val,
			&PIXMAP_DEPTH
	);
	PIXMAP_W = pixmap_w;
	PIXMAP_H = pixmap_h;

	ExposeEH(target, (XtPointer)icon_info, (XEvent *)NULL, &ignore);
} /* end of DtChangeProcessIcon */

/*
 * DtCreateProcessIcon -
 *
 * This routine is used to create a vendorShell widget. This shell
 * widget is used as a process icon of a base window (toplevel).
 * The routine returns the shell widget id and application writers
 * can use this id to register DnD interest via OlDnDRegisterDDI().
 *
 * XtNtranslations, XtNgeometry, and XtNborderWidth will be added
 * to "piargs".
 *
 * XtNiconWindow will be added to "targs".
 *
 * "icon_args" can contains the following resources:
 *	XtNiconMask
 *	XtNiconName
 *	XtNiconPixmap
 *	XtNfont		- Default is "-*-*-*-R-*-*-*-120-*-*-*-*-ISO8859-1"
 *	XtNforeground	- BlackPixelOfScreen(screen)
 *
 * Note that XtNfont and XtNforeground are for XtNiconName and
 *	     XtNbackground is from "this_shell_is_icon_win".
 */
extern Widget
DtCreateProcessIcon(
	Widget		toplevel,	/* shell that need an icon window    */
	ArgList		targs,		/* additional resources for toplevel */
	Cardinal	num_targs,
	ArgList		piargs,		/* additional resources for icon win */
	Cardinal	num_piargs,
	ArgList		icon_args,
	Cardinal	num_icon_args
)
{
#define NUM_MUST_PI_ARGS	4
#define NUM_MUST_T_ARGS		1
#define LOCAL_ARG_SIZE		20
#define EMPTY_TRANS		XtParseTranslationTable("")

	Arg			local_args[LOCAL_ARG_SIZE];
	ArgList			args;
	Cardinal		num_args;
	Display *		dpy;
	Screen *		screen;
	Widget			this_shell_is_icon_win;

	char			geometry[512];
	Boolean			use_icon_hint = False;
	XIconSize * 		icon_size_hint;		/* see ICCCM	*/
	int			count,
				max_icon_w = 1,		/* any values...*/
				max_icon_h = 1;


	if (XtIsSubclass(toplevel, topLevelShellWidgetClass) == False)
	{
			/* process icon is for base windows...	*/
		return((Widget)NULL);
	}

	dpy = XtDisplay(toplevel);
	screen = XtScreen(toplevel);
	if (XGetIconSizes(
		dpy, RootWindowOfScreen(screen), &icon_size_hint, &count) &&
	    count > 0)
	{
		use_icon_hint = True;
		max_icon_w = icon_size_hint->max_width;
		max_icon_h = icon_size_hint->max_height;

		XFree((char *)icon_size_hint);
	}
	sprintf(geometry, "%dx%d", max_icon_w, max_icon_h);

/* create a vendor shell as icon window...			*/
	if ((num_args = NUM_MUST_PI_ARGS + num_piargs) <= LOCAL_ARG_SIZE)
	{
		args = local_args;
	}
	else
	{
		args = (ArgList)XtMalloc(sizeof(Arg) * num_args);
	}

	XtSetArg(args[0], XtNtranslations,	(XtArgVal)EMPTY_TRANS);
	XtSetArg(args[1], XtNgeometry,		(XtArgVal)geometry);
	XtSetArg(args[2], XtNborderWidth,	(XtArgVal)0);
	XtSetArg(args[3], XtNmappedWhenManaged,	(XtArgVal)False);

	if (num_piargs > 0)
	{
		memcpy(&args[NUM_MUST_PI_ARGS], piargs,
				sizeof(Arg) * num_piargs);
	}

	this_shell_is_icon_win = XtAppCreateShell(
			"ThisShellIsIconWin",
			"ThisShellIsIconWin",
			vendorShellWidgetClass,
			dpy,
			args,
			num_args
	);

	XtRealizeWidget(this_shell_is_icon_win);

	SetIconInfo(
		this_shell_is_icon_win,
		icon_args, num_icon_args,
		use_icon_hint, max_icon_w, max_icon_h
	);

	if (args != local_args)
	{
		XtFree((XtPointer)args);
	}

	if ((num_args = NUM_MUST_T_ARGS + num_targs) <= LOCAL_ARG_SIZE)
	{
		args = local_args;
	}
	else
	{
		args = (ArgList)XtMalloc(sizeof(Arg) * num_args);
	}

	XtSetArg(args[0],
		 XtNiconWindow, (XtArgVal)XtWindow(this_shell_is_icon_win));

	if (num_targs > 0)
	{
		memcpy(&args[NUM_MUST_T_ARGS], targs, sizeof(Arg) * num_targs);
	}

	XtSetValues(toplevel, args, num_args);

	if (args != local_args)
	{
		XtFree((XtPointer)args);
	}

	return(this_shell_is_icon_win);

#undef NUM_MUST_ARGS
#undef LOCAL_ARG_SIZE
#undef EMPTY_TRANS
} /* end of DtCreateProcessIcon */

/*
 * DtSetAppId -
 *
 * This routine is used to establish a well known "id" for an application.
 * If the current ownership of "id" is not "None" then the routine
 * returns the current owner. Otherwise, the routine returns None.
 */
extern Window
DtSetAppId(
	Display *	dpy,
	Window		id_owner,
	char *		id
)
{
	Window	ret;
	Atom 	id_atom;

	id_atom = XInternAtom(dpy, id, False);

	/* lock in a critical section so I can get a	*/
	/* definite YES/NO...				*/
	XGrabServer(dpy);
	if ((ret = XGetSelectionOwner(dpy, id_atom)) == None) {
		XSetSelectionOwner(dpy, id_atom, id_owner, CurrentTime);
		ret = None;
	}
	XUngrabServer(dpy);
	XFlush(dpy);

	return(ret);
} /* end of DtSetAppId */


/****************************procedure*header*********************************
 *  	DtEucToCT: convert EUC string to CT.  The returned value is not
 *		NULL-terminated.
 *	INPUTS: display
 *		euc string
 *		address for returning CT string
 *	OUTPUTS: length of converted string, in bytes (0 if no string returned)
 *		NOTE: caller must FREE returned string
 *	GLOBALS:
 *****************************************************************************/
int
DtEucToCT(Display *dpy, String eucstring, String *ctstring)
{
    int cvt_status = 0;
    XTextProperty text_prop;

    if (!eucstring || !ctstring)
	return(0);

    cvt_status = XmbTextListToTextProperty(dpy, &eucstring, 1,
					   XCompoundTextStyle, &text_prop);

    if (cvt_status == Success || cvt_status > 0){
	*ctstring = text_prop.value;
	return(text_prop.nitems);
    }
    else{
	return(0);
    }
}	/* end of DtEucToCT */


/****************************procedure*header*********************************
 *  	DtCTToEuc: convert CT string to Euc.  The returned value is not
 *		NULL-terminated.
 *	INPUTS: display
 *		CT string
 *		address for returning Euc string
 *	OUTPUTS: length of converted string, in bytes (0 if no string returned)
 *		NOTE: caller must FREE returned string
 *	GLOBALS:
 *****************************************************************************/
int
DtCTToEuc(Display *dpy, String ctstring, String *eucstring)
{
    int cvt_status = 0;
    XTextProperty text_prop;
    char **cvt_text;
    int cvt_num;

    if (!ctstring || !eucstring)
	return(0);


    text_prop.value = ctstring;
    text_prop.encoding = XCompoundTextStyle;
    text_prop.format = 8;
    text_prop.nitems = 1;

    cvt_status = XmbTextPropertyToTextList(dpy, &text_prop, &cvt_text, 
					   &cvt_num);
    if (cvt_status == Success || cvt_status > 0){
	*eucstring = cvt_text[1];
	XtFree((char *) cvt_text);
	return(strlen(*eucstring));

    }
    else{
	return(0);
    }
    
} /* end of DtCTToEuc */


