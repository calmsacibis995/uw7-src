#ident	"@(#)r5extensions:lib/xtest/XTest.c	1.2"

/* $XConsortium: XTest.c,v 1.7 92/04/20 13:14:52 rws Exp $ */
/*

Copyright 1990, 1991 by UniSoft Group Limited
Copyright 1992 by the Massachusetts Institute of Technology

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.

*/

#define NEED_REPLIES
#include "Xlibint.h"
#include "XTest.h"
#include "xteststr.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _xtest_info_data;
static XExtensionInfo *xtest_info = &_xtest_info_data;
static /* const */ char *xtest_extension_name = XTestExtensionName;

#define XTestCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xtest_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xtest_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, xtest_info,
				   xtest_extension_name, 
				   &xtest_extension_hooks, XTestNumberEvents,
				   NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xtest_info)

/*****************************************************************************
 *                                                                           *
 *		    public routines               			     *
 *                                                                           *
 *****************************************************************************/

Bool
XTestQueryExtension (dpy, event_basep, error_basep, majorp, minorp)
    Display *dpy;
    int *event_basep, *error_basep;
    int *majorp, *minorp;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestGetVersionReq *req;
    xXTestGetVersionReply rep;

    if (XextHasExtension(info)) {
	LockDisplay(dpy);
	GetReq(XTestGetVersion, req);
	req->reqType = info->codes->major_opcode;
	req->xtReqType = X_XTestGetVersion;
	req->majorVersion = XTestMajorVersion;
	req->minorVersion = XTestMinorVersion;
	if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return False;
	}
	UnlockDisplay(dpy);
	SyncHandle();
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	*majorp = rep.majorVersion;
	*minorp = rep.minorVersion;
	return True;
    } else {
	return False;
    }
}

Bool
XTestCompareCursorWithWindow(dpy, window, cursor)
    Display *dpy;
    Window window;
    Cursor cursor;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestCompareCursorReq *req;
    xXTestCompareCursorReply rep;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestCompareCursor, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestCompareCursor;
    req->window = window;
    req->cursor = cursor;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return rep.same;
}

Bool
XTestCompareCurrentCursorWithWindow(dpy, window)
    Display *dpy;
    Window window;
{
    return XTestCompareCursorWithWindow(dpy, window, XTestCurrentCursor);
}

XTestFakeKeyEvent(dpy, keycode, is_press, delay)
    Display *dpy;
    unsigned int keycode;
    Bool is_press;
    unsigned long delay;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestFakeInputReq *req;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestFakeInput, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestFakeInput;
    req->type = is_press ? KeyPress : KeyRelease;
    req->detail = keycode;
    req->time = delay;
    UnlockDisplay(dpy);
    SyncHandle();
}

XTestFakeButtonEvent(dpy, button, is_press, delay)
    Display *dpy;
    unsigned int button;
    Bool is_press;
    unsigned long delay;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestFakeInputReq *req;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestFakeInput, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestFakeInput;
    req->type = is_press ? ButtonPress : ButtonRelease;
    req->detail = button;
    req->time = delay;
    UnlockDisplay(dpy);
    SyncHandle();
}

XTestFakeMotionEvent(dpy, screen, x, y, delay)
    Display *dpy;
    int screen;
    int x, y;
    unsigned long delay;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestFakeInputReq *req;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestFakeInput, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestFakeInput;
    req->type = MotionNotify;
    req->detail = False;
    if (screen == -1)
	req->root = None;
    else
	req->root = RootWindow(dpy, screen);
    req->rootX = x;
    req->rootY = y;
    req->time = delay;
    UnlockDisplay(dpy);
    SyncHandle();
}

XTestFakeRelativeMotionEvent(dpy, dx, dy, delay)
    Display *dpy;
    int dx, dy;
    unsigned long delay;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xXTestFakeInputReq *req;

    XTestCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XTestFakeInput, req);
    req->reqType = info->codes->major_opcode;
    req->xtReqType = X_XTestFakeInput;
    req->type = MotionNotify;
    req->detail = True;
    req->root = None;
    req->rootX = dx;
    req->rootY = dy;
    req->time = delay;
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XTestSetGContextOfGC(gc, gid)
    GC gc;
    GContext gid;
{
    gc->gid = gid;
}

void
XTestSetVisualIDOfVisual(visual, visualid)
    Visual *visual;
    VisualID visualid;
{
    visual->visualid = visualid;
}

static xReq _dummy_request = {
	0, 0, 0
};

Status
XTestDiscard(dpy)
    Display *dpy;
{
    Bool something;
    register char *ptr;

    LockDisplay(dpy);
    if (something = (dpy->bufptr != dpy->buffer)) {
	for (ptr = dpy->buffer;
	     ptr < dpy->bufptr;
	     ptr += (((xReq *)ptr)->length << 2))
	    dpy->request--;
	dpy->bufptr = dpy->buffer;
	dpy->last_req = (char *)&_dummy_request;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return something;
}
