#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/changebar.h	1.1"
#endif

#include "WSMcomm.h"

#ifndef _CHANGEBAR_H
#define _CHANGEBAR_H

typedef struct changeBar {
	Widget parent;
	int state;
} ChangeBar;

extern void CreateChangeBar( Widget , ChangeBar* );
extern void SetChangeBarState ( 
	ChangeBar *	bar, 
	int		element,
	int		state,
	int		propagate, 
	void 		(*change)()
);
extern void RedrawChangeBar (
	Widget			w,
	XtPointer		client_data,
	XEvent *		pe,
	Boolean *		continue_to_dispatch
);
extern void DrawChangeBar(Widget, XExposeEvent *, String *, Cardinal *);
#endif
