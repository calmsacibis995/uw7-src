#pragma ident	"@(#)R5Xlib:Ximp/XimpWDrS.c	1.2"

/* $XConsortium: XimpWDrS.c,v 1.3 92/04/14 13:30:18 rws Exp $ */
/*
 * Copyright 1990, 1991, 1992 by TOSHIBA Corp.
 * Copyright 1990, 1991, 1992 by SORD Computer Corp.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that 
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA Corp. and SORD Computer Corp.
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  TOSHIBA Corp. and
 * SORD Computer Corp. make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * TOSHIBA CORP. AND SORD COMPUTER CORP. DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL TOSHIBA CORP. OR SORD COMPUTER CORP. BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *				mopi@ome.toshiba.co.jp
 *	   Osamu Touma		SORD Computer Corp.
 */

/******************************************************************

              Copyright 1991, 1992 by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
FUJITSU LIMITED makes no representations about the suitability of
this software for any purpose.  It is provided "as is" without
express or implied warranty.

FUJITSU LIMITED DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

  Author: Takashi Fujiwara     FUJITSU LIMITED 

******************************************************************/
/*
	HISTORY:

	An sample implementation for Xi18n function of X11R5
	based on the public review draft 
	"Xlib Changes for internationalization,Nov.1990"
	by Katsuhisa Yano,TOSHIBA Corp. and Osamu Touma,SORD Computer Corp..

	Modification to the high level pluggable interface is done
	by Takashi Fujiwara,FUJITSU LIMITED.
*/

#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximplc.h"

int
_Ximp_wc_draw_string(dpy, d, xfont_set, gc, x, y, text, text_length)
    Display	*dpy;
    Drawable	d;
    XFontSet	xfont_set;
    GC		gc;
    int		x, y;
    wchar_t	*text;
    int		text_length;
{
#define CNV_FUNC	wcstocs

#include "XimpDrStr.c"

    return x;
}

void
_Ximp_wc_draw_image_string(dpy, d, xfont_set, gc, x, y, text, text_length)
    Display	*dpy;
    Drawable	d;
    XFontSet	xfont_set;
    GC		gc;
    int		x, y;
    wchar_t	*text;
    int		text_length;
{
    XRectangle		log;
    XGCValues		val;
    XGCValues		old;

    old.function = gc->values.function;
    old.fill_style = gc->values.fill_style;
    old.foreground = gc->values.foreground;

    val.function = GXcopy;
    val.fill_style = FillSolid;
    val.foreground = gc->values.background;

    XChangeGC(dpy, gc, GCFunction | GCFillStyle | GCForeground, &val);

    _Ximp_wc_extents(xfont_set, text, text_length, 0, &log);
    XFillRectangle(dpy, d, gc, x + log.x, y + log.y, log.width, log.height);

    XChangeGC(dpy, gc, GCFunction | GCFillStyle | GCForeground, &old);

    _Ximp_wc_draw_string(dpy, d, xfont_set, gc, x, y, text, text_length);
}

