#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/changebar.c	1.1"
#endif

#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>

#include "changebar.h"
#include "wsm.h"

void
DrawChangeBar(
	Widget w, XExposeEvent *event, String *params, Cardinal *num_params
)
{
  	static XGCValues	values;
  	static XGCValues	values2;
	static XtGCMask		valueMask;
	static XtGCMask		valueMask2;
	GC			offgc;
	GC			ongc;
	Display *		dpy;
	Window			window;
	static int		first = 0;
	Pixel			foreground = 0;
	Pixel			background = 1;
	
	dpy = XtDisplayOfObject(w);
	window = XtWindowOfObject(w);
	
	
	if((w == NULL) || (window == NULL) || (dpy == NULL)) {
		return;
	}
	
	if(first == 0) {
	  	XtVaGetValues(
			XtParent(w), XmNforeground, &foreground,
			XmNbackground, &background, NULL
		);
  		
		values.foreground  = background;
		values.background  = foreground;
		values.dash_offset = 0;
  		values.line_style  = LineSolid;
  		values.line_width  = 8;
  		values.function    = GXcopy;
  		valueMask = (
			GCForeground | GCBackground | GCDashOffset |
			GCLineStyle | GCLineWidth | GCFunction
		);
		
		values2.foreground = foreground;
		values2.background = background;
  		values2.dash_offset = 0;
  		values2.line_style  = LineSolid;
  		values2.line_width  = 8;
  		values2.function    = GXcopy;
  		valueMask2 = (
			GCForeground | GCBackground | GCDashOffset |
			GCLineStyle | GCLineWidth | GCFunction
		);
		
		++first;
	}
	
	offgc =
#if (XtSpecificationRelease == 5)
		XtAllocateGC(w, 0, valueMask, &values, 0, 0);
#else
		XCreateGC(
			dpy, RootWindowOfScreen(XtScreen(w)), valueMask,
			&values
		);
#endif
	
	ongc =
#if (XtSpecificationRelease == 5)
		XtAllocateGC(w, 0, valueMask2, &values2, 0, 0);
#else
  		XCreateGC(
			dpy, RootWindowOfScreen(XtScreen(w)), valueMask2,
			&values2
		);
#endif

	if(
		(strcmp(params[1], "0") == 0) &&
		(strcmp(params[0], "1") == 0)
	) {	/* Expose Event and State is True */
		XDrawLine(dpy, window, ongc, 0, 0, 0, 20);
	}
	else if(strcmp(params[1], "1") == 0) {
		/* Set changebar from a callback */
		if(strcmp(params[0], "0") == 0) {
			/* Setting changebar state to False */
			XDrawLine(dpy, window, offgc, 0, 0, 0, 20);
		}
		else { /* Setting changebar state to True */
			XDrawLine(dpy, window, ongc, 0, 0, 0, 20);
		}
	}

#if (XtSpecificationRelease == 5)
	XtReleaseGC(w, ongc);
	XtReleaseGC(w, offgc);
#else
	XFreeGC(dpy, ongc);
	XFreeGC(dpy, offgc);
#endif
	
	return;
}

void 
SetChangeBarState(
	ChangeBar *bar, int element, int state, int propagate,
	void(*change) () 
)
{
	char**	params;
	int	i;
	
	params = (char **) calloc(2, sizeof(char *));
	for(i = 0; i< 2; i++) {
		params[i] = (char *) calloc(10, sizeof(char));
	}
	
	if(state == WSM_NORMAL) {
		bar->state = True;
	}
	else {
	  	bar->state = False;
	}
	
	sprintf(params[0], "%d", bar->state);
	sprintf(params[1], "1");
	
	DrawChangeBar(
		bar->parent, (XExposeEvent*)NULL, (String *) params,
		(Cardinal *) 2
	);
	
	for(i = 0; i< 2; i++) {
		free(params[i]);
	}
	free(params);
	
	if((propagate) && (change != NULL)) {
		(*change)();
	}
	
	return;
}

static void
RedrawChangeBar(
	Widget w, XtPointer client_data, XEvent *pe,
	Boolean *continue_to_dispatch
)
{
	ChangeBar*	bar = (ChangeBar *) client_data;
	char**		params;
	int		i;
	
	params = (char **) calloc(2, sizeof(char *));
	for(i = 0; i< 2; i++) {
	   	params[i] = (char *) calloc(10, sizeof(char));
	}
	
	sprintf(params[0], "%d", bar->state);
	sprintf(params[1], "1");
	
	DrawChangeBar(
		bar->parent, (XExposeEvent*)NULL,
		(String *) params, (Cardinal *) 2
	);
	
	for(i = 0; i< 2; i++) {
	  	free(params[i]);
	}
	free(params);
	return;
}

void 
CreateChangeBar(Widget parent, ChangeBar* bar)
{
  	static int	first = 0;
	
	XtAddEventHandler (parent, ExposureMask, 
		False, (XtEventHandler)RedrawChangeBar, (XtPointer)(bar)
	);
	
	bar->parent = parent;
	bar->state = False;
	
	return;
}
