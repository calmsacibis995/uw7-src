#pragma ident	"@(#)builtinext:include/builtin.h	1.2"

#ifndef BUILTIN_H_
#define BUILTIN_H_

#include <X11/Xfuncproto.h>

#define X_BuiltinQueryVersion	0
#define X_BuiltinRunClient	1

#define BuiltinExitNotify	0
#define BuiltinExecNotify	1
#define BuiltinNumberEvents	( BuiltinExecNotify + 1 )

#define BuiltinNumberErrors	0

#ifndef BUILTIN_SERVER_

typedef struct {
    int		type;		/* of event */
    unsigned long serial;	/* # of last request processed by server */
    Bool	send_event;	/* true if this came from a SendEvent request */
    Display *	display;	/* Display the event was read from */
    Window	window;		/* unused */
} XBuiltinExecNotifyEvent;

typedef struct {
    int		type;		/* of event */
    unsigned long serial;	/* # of last request processed by server */
    Bool	send_event;	/* true if this came from a SendEvent request */
    Display *	display;	/* Display the event was read from */
    Window	window;		/* unused */
    int		exit_code;
} XBuiltinExitNotifyEvent;

_XFUNCPROTOBEGIN

extern Bool XBuiltinQueryExtension(Display * dpy,
				   int * event_base, int * error_base);
extern Status XBuiltinQueryVersion(Display * dpy, char * name,
				   unsigned char * ret_valid,
				   int * ret_major_version,
				   int * ret_minor_version);
extern Status XBuiltinRunClient(Display * dpy,
				int argc, char * argv[], char * envp[]);
_XFUNCPROTOEND

#endif /* BUILTIN_SERVER_ */
#endif /* BUILTIN_H_ */
