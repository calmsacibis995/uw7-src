#pragma ident	"@(#)R5Xlib:SetWMCMapW.c	1.2"

/* $XConsortium: SetWMCMapW.c,v 1.4 91/01/06 11:43:52 rws Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "Xlibint.h"
#include <X11/Xatom.h>

/* 
 * XSetWMProtocols sets the property 
 *	WM_COLORMAP_WINDOWS 	type: WINDOW	format:32
 */

Status XSetWMColormapWindows (dpy, w, windows, count)
    Display *dpy;
    Window w;
    Window *windows;
    int count;
{
    Atom prop;

    prop = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
    if (prop == None) return False;

    XChangeProperty (dpy, w, prop, XA_WINDOW, 32,
		     PropModeReplace, (unsigned char *) windows, count);
    return True;
}
