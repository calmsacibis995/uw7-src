#pragma ident	"@(#)R5Xlib:Ximp/XimpCT.c	1.3"

/* $XConsortium: XimpCT.c,v 1.4 92/04/14 13:28:38 rws Exp $ */
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
	by Katsuhisa Yano,TOSHIBA Corp..

	Modification to the high level pluggable interface is done
	by Takashi Fujiwara,FUJITSU LIMITED.
*/

#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximplc.h"

#define SKIP_I(str)	while (*(str) >= 0x20 && *(str) <=  0x2f) (str)++;
#define SKIP_P(str)	while (*(str) >= 0x30 && *(str) <=  0x3f) (str)++;

int
_XlcCheckESCSequence(ctext, ctext_len, charset)
    unsigned char *ctext;
    int ctext_len;
    CharSet *charset;
{
    unsigned char msb_mask, *ctptr = ctext;
    unsigned char buf[BUFSIZE];
    int length;

    if (*ctptr++ != 0x1b)
	return -1;
    
    SKIP_I(ctptr);

    if (*ctptr < 0x30 && *ctptr > 0x7e)
	return -1;
    
    length = ++ctptr - ctext;
    if (ctext_len < length)
	return -1;

    strncpy((char *)buf, (char *)ctext, length);
    buf[length] = '\0';

    *charset = _XlcGetCharSetFromEncoding(buf);
    
    return length;
}

int
_XlcCheckCSISequence(ctext, ctext_len, charset)
    unsigned char *ctext;
    int ctext_len;
    CharSet *charset;
{
    unsigned char ch, *ctptr = ctext;
    int length;

    if (*ctptr++ != 0x9b)
	return -1;

    SKIP_P(ctptr);
    SKIP_I(ctptr);

    if (*ctptr < 0x40 && *ctptr > 0x7e)
	return -1;
    
    length = ++ctptr - ctext;
    if (ctext_len < length)
	return -1;
    
    *charset = NULL;

    return length;
}


int
_Xlc_cstostring(state, csstr, csstr_len, string, string_len)
    State state;
    unsigned char *csstr;
    int csstr_len;
    unsigned char *string;
    int *string_len;
{
    unsigned char *csptr = csstr;
    unsigned char *string_ptr = string;
    unsigned char ch;
    int str_len;
    CharSet charset = *state->codeset->charset_list;

    if (charset->string_encoding == False)
	return -1;

    if (string_len)
	str_len = *string_len;
    else
	str_len = MAXINT;

    while (csstr_len > 0 && str_len > 0) {
	ch = *csptr++ & 0x7f;
	if (ch < 0x20 || ch > 0x7e)
	    if (ch != 0x09 && ch != 0x0a && ch != 0x0b)
		return -1;
	*string_ptr++ = ch;
	csstr_len--;
	str_len--;
    }

    if (string_len)
	*string_len = string_ptr - string;
    
    return csptr - csstr;
}


int
_Xlc_cstoct(state, csstr, csstr_len, ctext, ctext_len)
    State state;
    unsigned char *csstr;
    int csstr_len;
    unsigned char *ctext;
    int *ctext_len;
{
    unsigned char encoding[BUFSIZE];
    unsigned char *csptr = csstr;
    unsigned char *ctptr = ctext;
    unsigned char ch, side, min_ch, max_ch;
    int length, gc_num, ct_len, tmp_len;
    CharSet charset = *state->codeset->charset_list;

    if (ctext_len)
	ct_len = *ctext_len;
    else
	ct_len = MAXINT;

    side = charset->side;
    length = charset->length;
    gc_num = charset->gc_num;

    if (state->last_codeset != state->codeset) {
	strcpy((char *) encoding, charset->encoding);
	tmp_len = strlen((char *) encoding);
	if ((ct_len -= tmp_len) < 0)
	    return -1;
	strcpy((char *) ctptr, (char *) encoding);
	ctptr += tmp_len;
    }

    min_ch = 0x20;
    max_ch = 0x7f;

    if (gc_num == 94) {
	max_ch--;
	if (length > 1 || side == GR)
	    min_ch++;
    }

    while (csstr_len > 0 && ct_len > 0) {
	ch = *csptr++ & 0x7f;
	if (ch < min_ch || ch > max_ch)
	    if (ch != 0x09 && ch != 0x0a && ch != 0x0b)
		return -1;
	*ctptr++ = ch | side;
	csstr_len--;
	ct_len--;
    }

    if (ctext_len)
	*ctext_len = ctptr - ctext;
    
    return csptr - csstr;
}
