#pragma ident	"@(#)R5Xlib:XSetClMask.c	1.2"

/* $XConsortium: XSetClMask.c,v 11.9 91/01/06 11:47:54 rws Exp $ */
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

XSetClipMask (dpy, gc, mask)
register Display *dpy;
GC gc;
Pixmap mask;
{
    LockDisplay(dpy);
    /* always update, since client may have changed pixmap contents */
    gc->values.clip_mask = mask;
    gc->dirty |= GCClipMask;
    gc->rects = 0;
    _XFlushGCCache(dpy, gc);
    UnlockDisplay(dpy);
    SyncHandle();
}
