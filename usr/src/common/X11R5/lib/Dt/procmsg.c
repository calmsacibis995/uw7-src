#pragma	ident	"@(#)Dt:procmsg.c	1.10"

/* 
 * procmsg.c
 *
 * This file contains routines to receive requests and send replies to the
 * requestor. Typically they are used by the Desktop manager.  However, these
 * routines should be generic enough that they can be used by other clients.
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include "DesktopI.h"

extern XDeleteProperty();
#ifndef MEMUTIL
extern char *malloc();
#endif /* MEMUTIL */

static void DtIgnoreError(Display *dpy);
static void DtResetErrorHandler(Display *dpy);

/*
 * DtClearQueue
 * ------------
 * The DtClearQueue procedure clears the queue atom stored
 * in the server.
 */
void
DtClearQueue(scrn, queue)
Screen *scrn;
Atom queue;
{
	XDeleteProperty(XDisplayOfScreen(scrn),RootWindowOfScreen(scrn),queue);
} /* end of DtClearQueue */

static int
CatchBadWindow(dpy, event)
Display *dpy;
XErrorEvent *event;
{
	/* ignore error */
	return(0);
}

/*
 * DtDequeueMsg
 * ----------------
 * The DtDequeueMsg function removes a msg (a NULL terminated string)
 * from the queue property stored on the server. The Desktop Manager
 * calls this function to retrieve requests stored using the
 * DtEnqueueRequest function.
 */
char *
DtDequeueMsg(scrn, queue, window)
Screen		*scrn;
Atom		queue;
Window		window;
{
	Display *dpy = DisplayOfScreen(scrn);
	unsigned long buffer_len;
	char *buffer;

	DtIgnoreError(dpy);
	buffer = Dt__GetCharProperty(dpy, window, queue, &buffer_len);
	DtResetErrorHandler(dpy);
	return(buffer);
} /* end of DtDequeueMsg */

/*
 * DtSendReply
 * -----------
 * The DtSendReply function packages and sends a reply to a client
 * from the Desktop Manager.
 */
int 
DtSendReply(scrn, queue, client, reply)
Screen		*scrn;
Atom		queue;
Window		client;
DtReply 	*reply;
{
	Display *dpy = DisplayOfScreen(scrn);
	XEvent ev;
	char *str;
	int len;
	DtMsgInfo const *mp;
	int mlen;

	/* check for valid msg function */
	/* the right shift below is OK because rptype is unsigned */
	switch(reply->header.rptype >> MSGFUNC_SHIFT) {
	case DTM_MSG:
		mp = Dt__dtm_replytypes;
		mlen = DT_DTM_NUM_REPLIES;
		break;
	case WB_MSG:
		mp = Dt__wb_replytypes;
		mlen = DT_WB_NUM_REPLIES;
		break;
	case HELP_MSG:
		mp = Dt__help_replytypes;
		mlen = DT_HELP_NUM_REPLIES;
		break;
	default:
		/* bad msg type */
		return(-1);
	}

	if ((str = Dt__StructToString((DtRequest *)reply,&len,mp,mlen)) == NULL)
		return(-1);

	/* send string to window */
	DtIgnoreError(dpy);
	Dt__EnqueueCharProperty(dpy, client, queue, str, len);

	ev.type = SelectionNotify;
	ev.xselection.requestor = client;
	ev.xselection.selection = queue;
	ev.xselection.target    = queue;
	ev.xselection.time      = CurrentTime;
	ev.xselection.property  = queue;
	XSendEvent(dpy, client, False, 0, &ev);
	DtResetErrorHandler(dpy);
	return(0);
} /* end of DtSendReply */

static void
DtIgnoreError(dpy)
Display *dpy;
{
	XSync(dpy, False);
	XSetErrorHandler(CatchBadWindow);
}

static void
DtResetErrorHandler(dpy)
Display *dpy;
{
	XSync(dpy, False);
	XSetErrorHandler(NULL);
}


