
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:wkcvt.c	1.8"

/* X/OL includes */

#include <stdio.h>
#include <signal.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include "wksh.h"
#include "xpm.h"

#if defined(SYSV) || defined(SVR4_0) || defined(SVR4)
#define lsprintf sprintf
#endif

extern Widget Toplevel;

/*
 * Converters for wksh
 */

void
WkshCvtHexIntToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static char result[16];

	if (fval->size != sizeof(int) && fval->size != sizeof(short)) {
		XtWarningMsg(CONSTCHAR "cvtIntToString", 
			CONSTCHAR "badsize", 
			CONSTCHAR "XtToolkitError",
			CONSTCHAR "From value is not sizeof(Int) or sizeof(short)",
			NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	if (fval->size == sizeof(int))
		sprintf(result, "0x%x", ((int *)(fval->addr))[0]);
	else if (fval->size == sizeof(short))
		sprintf(result, "0x%x", (int)(((short *)(fval->addr))[0]));
	toval->addr = result;
	toval->size = strlen(result)+1;
}

void
WkshCvtIntToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static char result[16];

	if (fval->size != sizeof(int) && fval->size != sizeof(short)) {
		XtWarningMsg(CONSTCHAR "cvtIntToString", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From value is not sizeof(Int) or sizeof(short)", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	if (fval->size == sizeof(int))
		sprintf(result, "%d", ((int *)(fval->addr))[0]);
	else
		sprintf(result, "%d", (int)(((short *)(fval->addr))[0]));
	toval->addr = result;
	toval->size = strlen(result)+1;
}

void
WkshCvtBooleanToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	if (fval->size != sizeof(Boolean)) {
		XtWarningMsg(CONSTCHAR "cvtBooleanToString", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From value is not sizeof(Boolean)", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	if (((Boolean *)(fval->addr))[0]) {
		toval->addr = (caddr_t)(CONSTCHAR "true");
	} else {
		toval->addr = (caddr_t)(CONSTCHAR "false");
	}
	toval->size = strlen(toval->addr)+1;
}

/*
 * NOTE: I would have preferred not to include this, since some versions
 * of libXmu.a contain it.  However, it is distributed to avoid porting
 * problems to those systems that don't have an Xmu lib that can convert
 * string to pixmap.
 */

void
WkshCvtStringToPixmap(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static Pixmap pixmap;
	unsigned int width, height;
	unsigned int depth;
	Display *display;
	Colormap colormap;
	Screen *screen;
	Window root;
	char *path;

	if (fval->size <= 0) {
		XtWarningMsg(CONSTCHAR "cvtStringToPixmap", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From size is zero or negative", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	if (*nargs != 1) {
		XtWarningMsg(CONSTCHAR "cvtStringToPixmap", CONSTCHAR "badargs", CONSTCHAR "XtToolkitError",
			CONSTCHAR "Screen to Pixmap converter requires screen argument", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	screen = ((Screen **)args[0].addr)[0];
	colormap = DefaultColormapOfScreen(screen);
	depth = DefaultDepthOfScreen(screen);
	display = DisplayOfScreen(screen);
	root = RootWindowOfScreen(screen);
	path = (char *)fval->addr;

	if (strcmp(path, CONSTCHAR "NULL") == 0) {
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	if (XReadPixmapFile(display, root, colormap, path, &width, &height, depth, &pixmap) != PixmapSuccess)  {
		toval->addr = NULL;
		toval->size = 0;
		XtWarningMsg(CONSTCHAR "cvtStringToPixmap", CONSTCHAR "badpixmap", CONSTCHAR "XtToolkitError",
			CONSTCHAR "Could not read pixmap file", NULL, 0);
		return;
	} else {
		toval->addr = (XtPointer)&pixmap;
		toval->size = sizeof(Pixmap);
	}
}

void
WkshCvtStringToXImage(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	int format;
	int code;
	static XImage *image;
	Pixmap pixmap;
	unsigned int width, height;
	int depth, hotx, hoty;
	Display *display;
	Colormap colormap;
	Screen *screen;
	Window root;
	char *path;

	if (fval->size <= 0) {
		XtWarningMsg(CONSTCHAR "cvtStringToXImage", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From size is zero or negative", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	if (*nargs != 1) {
		XtWarningMsg(CONSTCHAR "cvtStringToXImage", CONSTCHAR "badargs", CONSTCHAR "XtToolkitError",
			CONSTCHAR "Screen to XImage converter requires screen argument", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	screen = ((Screen **)args[0].addr)[0];
	colormap = DefaultColormapOfScreen(screen);
	depth = DefaultDepthOfScreen(screen);
	display = DisplayOfScreen(screen);
	root = RootWindowOfScreen(screen);
	path = (char *)fval->addr;

	if (strcmp(path, CONSTCHAR "NULL") == 0) {
		toval->addr = NULL;
		toval->size = sizeof(char *);
		return;
	}

	/*
	 * Try reading it as a regular bitmap data file
	 */
	if ((code = XReadBitmapFile(display, root, path, &width, &height, &pixmap, &hotx, &hoty)) == BitmapSuccess)  {
		format = XYBitmap;
	} else {
		/*
		 * Failed, try reading it as a pixmap file
		 */
		if (XReadPixmapFile(display, root, colormap, path, &width, &height, depth, &pixmap) == PixmapSuccess)  {
			format = XYPixmap;
		} else {
			toval->addr = NULL;
			toval->size = 0;
			XtWarningMsg(CONSTCHAR "cvtStringToXImage", CONSTCHAR "badpixmap", CONSTCHAR "XtToolkitError",
				CONSTCHAR "Could not read pixmap/bitmap file", NULL, 0);
			return;
		}
	}
	if ((image = XGetImage(display, (Drawable)pixmap, 0, 0, width, height, AllPlanes, format)) == NULL) {
		XtWarningMsg(CONSTCHAR "cvtStringToXImage", CONSTCHAR "badimage", CONSTCHAR "XtToolkitError",
			CONSTCHAR "Could not convert pixmap to image", NULL, 0);
		return;
	} else {
		toval->addr = (XtPointer)&image;
		toval->size = sizeof(XImage *);
		return;
	}
}

void
WkshCvtStringToPointer(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static XtPointer ret;

	if (fval->size <= 0 || fval->addr == NULL) {
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	ret = (XtPointer)strdup(fval->addr);
	toval->addr = (XtPointer)&ret;
	toval->size = sizeof(XtPointer);
	return;
}

void
WkshCvtStringToWidget(dpy, args, nargs, fval, toval, data)
Display *dpy;
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
XtPointer data;
{
	char *wname;
	extern Widget Toplevel;
	Widget wid;
	wtab_t *w;
	Widget WkshNameToWidget();

	if (fval->size <= 0) {
		XtWarningMsg(CONSTCHAR "cvtStringToWidget", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From size is zero or negative", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	wname = (char *)fval->addr;
	if (wname == NULL || wname[0] == '\0' || strcmp(wname, CONSTCHAR "NULL") == 0) {
		static Widget NullWidget = NULL;

		toval->addr = (XtPointer)NullWidget;
		toval->size = sizeof(NullWidget);
		return;
	}
	if ((w = str_to_wtab(CONSTCHAR "ConvertStringToWidget", wname)) != NULL) {
		toval->addr = (XtPointer)w->w;
		toval->size = sizeof(w->w);
		return;
	}
	/*
	 * If we couldn't find it in our table, try looking up the
	 * name in standard resource format.
	 */
	if ((wid = WkshNameToWidget(wname)) != NULL) {
		toval->addr = (XtPointer)wid;
		toval->size = sizeof(wid);
	}
	/*
	 * We failed completely
	 */
	{
		char errbuf[1024];

		sprintf(errbuf, "cannot find a widget named '%s'", wname);
		XtWarningMsg(CONSTCHAR "cvtStringToWidget", CONSTCHAR "badname", CONSTCHAR "XtToolkitError",
		errbuf, NULL, 0);
	}
	toval->addr = NULL;
	toval->size = 0;
}

void
WkshCvtStringToCallback(dpy, args, nargs, fval, toval, data)
Display *dpy;
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
XtPointer data;
{
	static XtCallbackList cb;
	extern void stdCB();
	wksh_client_data_t *cdata;
	const callback_tab_t *cbtab;
	extern classtab_t *WKSHConversionClass;
	extern wtab_t *WKSHConversionWidget;
	extern char *WKSHConversionResource;
	classtab_t *c = WKSHConversionClass;
	wtab_t *w = WKSHConversionWidget;

	if (fval->size <= 0) {
		XtWarningMsg(CONSTCHAR "cvtStringToCallback", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From value is zero or negative", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	cb = (XtCallbackList)XtMalloc(sizeof(XtCallbackRec)*2);
	cb[0].callback = stdCB;

	cdata = (wksh_client_data_t *)XtMalloc(sizeof(wksh_client_data_t));
	cdata->ksh_cmd = strdup((String)fval->addr);
	cdata->w = w;

	for (cbtab = c->cbtab; cbtab != NULL && cbtab->resname != NULL; cbtab++) {
		if (strcmp(cbtab->resname, WKSHConversionResource) == 0)
			break;
	}
	if (cbtab != NULL && cbtab->resname != NULL)
		cdata->cbtab = cbtab;
	else
		cdata->cbtab = NULL;

	cb[0].closure = (caddr_t)cdata;
	cb[1].callback = NULL;
	toval->addr = (XtPointer)&cb;
	toval->size = sizeof(XtCallbackList);
}

void
WkshCvtCallbackToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	XtCallbackList cb;
	extern void stdCB();
	char buf[2048];
	register char *p;

	if (fval->size != sizeof(XtCallbackList)) {
		XtWarningMsg(CONSTCHAR "cvtCallbackToString", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From value is not sizeof(XtCallbackList)", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	if (fval->addr == NULL) {
		toval->addr = ": ;";
		toval->size = 1;
		return;
	}
	p = &buf[0];
	*p = '\0';
	for (cb = ((XtCallbackList *)(fval->addr))[0]; cb->callback != NULL; cb++) {
		if (cb->callback != stdCB) {
			if (p + 32 - buf > sizeof(buf)) {
				XtWarningMsg(CONSTCHAR "cvtCallbackToString", CONSTCHAR "overflow", CONSTCHAR "XtToolkitError",
				CONSTCHAR "Internal conversion buffer overflowed", NULL, 0);
				break;
			}
			p += lsprintf(p, CONSTCHAR "UNKCB %d %d; ", cb->callback, cb->closure);
		} else {
			wksh_client_data_t *cdata = (wksh_client_data_t *)cb->closure;
			if (p + strlen((String)cdata->ksh_cmd) + 1 - buf > sizeof(buf)) {
				XtWarningMsg(CONSTCHAR "cvtCallbackToString", CONSTCHAR "overflow", CONSTCHAR "XtToolkitError",
				CONSTCHAR "Internal conversion buffer overflowed", NULL, 0);
				break;
			}
			p += lsprintf(p, "%s; ", (String)cdata->ksh_cmd);
		}
	}
	toval->addr = (XtPointer)strdup(buf);
	toval->size = strlen(buf) + 1;
}

void
WkshCvtWidgetToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	char *wname;
	Widget widget;
	extern Widget Toplevel;
	wtab_t *w;
	wtab_t *widget_to_wtab();

	if (fval->size != sizeof(Widget) || fval->addr == NULL) {
		XtWarningMsg(CONSTCHAR "cvtWidgetToString", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From size != sizeof(Widget)", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	widget = ((Widget *)fval->addr)[0];
	if (widget == NULL) {
		toval->addr = (XtPointer)(CONSTCHAR "NULL");
		toval->size = 5;
		return;
	}
	if ((w = widget_to_wtab(widget, NULL)) == NULL) {
		if (fval->size <= 0) {
			XtWarningMsg(CONSTCHAR "cvtWidgetToString", CONSTCHAR "badname", CONSTCHAR "XtToolkitError",
				CONSTCHAR "cannot find a name for that widget", NULL, 0);
			toval->addr = NULL;
			toval->size = 0;
			return;
		}
		return;
	}
	toval->addr = (XtPointer)w->widid;
	toval->size = strlen(w->widid) + 1;
}
