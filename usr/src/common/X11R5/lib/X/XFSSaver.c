#pragma ident	"@(#)R5Xlib:XFSSaver.c	1.2"

/* $XConsortium: XFSSaver.c,v 1.5 91/01/06 11:45:26 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1987	*/

/*
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

#include "Xlibint.h"

XActivateScreenSaver(dpy) 
    register Display *dpy;

{
    XForceScreenSaver (dpy, ScreenSaverActive);
}

XResetScreenSaver(dpy) 
    register Display *dpy;

{
    XForceScreenSaver (dpy, ScreenSaverReset);
}

XForceScreenSaver(dpy, mode)
    register Display *dpy; 
    int mode;

{
    register xForceScreenSaverReq *req;

    LockDisplay(dpy);
    GetReq(ForceScreenSaver, req);
    req->mode = mode;
    UnlockDisplay(dpy);
    SyncHandle();
}

