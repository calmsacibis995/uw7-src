#pragma ident	"@(#)R5Xlib:XUnldFont.c	1.2"

/* $XConsortium: XUnldFont.c,v 11.7 91/01/06 11:48:41 rws Exp $ */
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

XUnloadFont(dpy, font)
     register Display *dpy;
     Font font;

{       
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(CloseFont, font, req);
    UnlockDisplay(dpy);
    SyncHandle();
}

