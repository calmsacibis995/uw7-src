#pragma ident	"@(#)R5Xlib:XCopyPlane.c	1.2"

/* $XConsortium: XCopyPlane.c,v 11.9 91/01/06 11:44:49 rws Exp $ */
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

XCopyPlane(dpy, src_drawable, dst_drawable, gc,
	  src_x, src_y, width, height,
	  dst_x, dst_y, bit_plane)
     register Display *dpy;
     Drawable src_drawable, dst_drawable;
     GC gc;
     int src_x, src_y;
     unsigned int width, height;
     int dst_x, dst_y;
     unsigned long bit_plane;

{       
    register xCopyPlaneReq *req;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(CopyPlane, req);
    req->srcDrawable = src_drawable;
    req->dstDrawable = dst_drawable;
    req->gc = gc->gid;
    req->srcX = src_x;
    req->srcY = src_y;
    req->dstX = dst_x;
    req->dstY = dst_y;
    req->width = width;
    req->height = height;
    req->bitPlane = bit_plane;
    UnlockDisplay(dpy);
    SyncHandle();
}

