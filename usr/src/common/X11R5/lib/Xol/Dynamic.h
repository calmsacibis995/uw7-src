#ifndef NOIDENT
#ident	"@(#)olmisc:Dynamic.h	1.17"
#endif

#ifndef __Dynamic_h__
#define __Dynamic_h__

typedef int		OlInputEvent;

typedef struct {
	Boolean		consumed;
	XEvent *	event;
	KeySym *	keysym;
	char *		buffer;
	int *		length;
	OlInputEvent	ol_event;
} OlInputCallData, *OlInputCallDataPointer;

/*
 * function prototype section
 */

OLBeginFunctionPrototypeBlock

extern OlInputEvent	
LookupOlInputEvent OL_ARGS((
	Widget			w,
	XEvent *		event,
	KeySym *		keysym,
	char **			buffer,
	int *			length
));
extern void
OlReplayBtnEvent OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XEvent *		event
));

OLEndFunctionPrototypeBlock

#endif /* __Dynamic_h__ */
