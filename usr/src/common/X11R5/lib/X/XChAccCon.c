#pragma ident	"@(#)R5Xlib:XChAccCon.c	1.2"

/* $XConsortium: XChAccCon.c,v 11.9 91/01/06 11:44:15 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

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

XEnableAccessControl(dpy) 
    register Display *dpy;

{
    XSetAccessControl(dpy, EnableAccess);
}

XDisableAccessControl(dpy) 
    register Display *dpy;

{
    XSetAccessControl(dpy, DisableAccess);
}

XSetAccessControl(dpy, mode)
    register Display *dpy; 
    int mode;

{
    register xSetAccessControlReq *req;

    LockDisplay(dpy);
    GetReq(SetAccessControl, req);
    req->mode = mode;
    UnlockDisplay(dpy);
    SyncHandle();
}

