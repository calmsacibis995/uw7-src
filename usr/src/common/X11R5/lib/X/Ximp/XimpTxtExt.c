#pragma ident	"@(#)R5Xlib:Ximp/XimpTxtExt.c	1.5"

/* $XConsortium: XimpTxtExt.c,v 1.4 92/04/14 13:30:06 rws Exp $ */
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

/*
 *	_Ximp_mb_extents()
 *	_Ximp_wc_extents()
 */

{
    XLCd lcd = xfont_set->core.lcd;
    unsigned char *strptr, strbuf[BUFSIZE];
    unsigned char xchar_buf[BUFSIZE];
    XChar2b xchar2b_buf[BUFSIZE];
    XFontStruct *font;
    int (*cnv_func)();
    int count, length, tmp_len;
    int direction, logical_ascent, logical_descent, tmp_ascent, tmp_descent;
    XCharStruct overall, tmp_overall;
    Bool first = True;
    LCMethods methods = LC_METHODS(lcd);
    State state;

    cnv_func = methods->CNV_FUNC;

    state = (*methods->create_state)(lcd);
    (*methods->cnv_start)(state);

    bzero(&overall, sizeof(XCharStruct));
    logical_ascent = logical_descent = 0;

    while (text_length > 0) {
        length = BUFSIZE;
	count = (*cnv_func)(state, text, text_length, strbuf, &length);
	if (count <= 0)
	    break;

	text += count;
	text_length -= count;

	strptr = strbuf;
	while (length > 0) {
	    tmp_len = BUFSIZE;
	    if (state->codeset->length  < 2)
	    	count = _Ximp_cstoxchar(xfont_set, strptr, length, xchar_buf,
					&tmp_len, state->codeset, &font);
	    else
		count = _Ximp_cstoxchar2b(xfont_set, strptr, length, xchar2b_buf,
					  &tmp_len, state->codeset, &font);
	    if (count <= 0)
		break;

	    if (state->codeset->length < 2)
		XTextExtents(font, (char *)xchar_buf, tmp_len, &direction,
			     &tmp_ascent, &tmp_descent, &tmp_overall);
            else
		XTextExtents16(font, xchar2b_buf, tmp_len, &direction,
			       &tmp_ascent, &tmp_descent, &tmp_overall);

	    if (first) {	/* initialize overall */
		overall = tmp_overall;
		logical_ascent = tmp_ascent;
		logical_descent = tmp_descent;
		first = False;
	    } else {
		overall.lbearing = min(overall.lbearing,
				       overall.width + tmp_overall.lbearing);
		overall.rbearing = max(overall.rbearing,
				       overall.width + tmp_overall.rbearing);
		overall.ascent = max(overall.ascent, tmp_overall.ascent);
		overall.descent = max(overall.descent, tmp_overall.descent);
		overall.width += tmp_overall.width;
		logical_ascent = max(logical_ascent, tmp_ascent);
		logical_descent = max(logical_descent, tmp_descent);
	    }

	    strptr += count;
	    length -= count;
	}
    }

    (*methods->cnv_end)(state);
    (*methods->destroy_state)(state);

    if (overall_ink) {
	overall_ink->x = overall.lbearing;
	overall_ink->y = -(overall.ascent);
	overall_ink->width = overall.rbearing - overall.lbearing;
	overall_ink->height = overall.ascent + overall.descent;
    }

    if (overall_logical) {
	overall_logical->x = 0;
        overall_logical->y = -(logical_ascent);
	overall_logical->width = overall.width;
        overall_logical->height = logical_ascent + logical_descent;
    }

    return overall.width;
}
