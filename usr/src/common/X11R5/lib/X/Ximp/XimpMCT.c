#pragma ident	"@(#)R5Xlib:Ximp/XimpMCT.c	1.4"

/* $XConsortium: XimpMCT.c,v 1.5 92/04/14 13:29:33 rws Exp $ */
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

extern int _Xlc_cstostring(), _Xlc_cstoct();
extern int _XlcCheckESCSequence(), _XlcCheckCSISequence();

#define STRING_CONV(xxxtocs, cstoxxx) \
    unsigned char buf[BUFSIZE]; \
    int to_length, buf_len, scan_len, tmp_len; \
    int ret = -1; \
    LCMethods methods = LC_METHODS(lcd); \
    State state; \
\
    if (to_len) { \
	to_length = *to_len; \
	*to_len = 0; \
    } else \
	to_length = MAXINT; \
    if (unconv_num) \
	*unconv_num = 0; \
\
    state = (*methods->create_state)(lcd); \
    (*methods->cnv_start)(state); \
    state->last_codeset = state->GL_codeset; \
\
    while (from_len > 0 && to_length > 0) { \
	buf_len = BUFSIZE; \
	scan_len = (*xxxtocs)(state, from_ptr, from_len, buf, &buf_len); \
	if (scan_len == -1) \
	    goto error; \
	if (scan_len == 0)  \
	    break; \
\
	from_ptr += scan_len; \
	from_len -= scan_len; \
\
	tmp_len = to_length; \
	if ((*cstoxxx)(state, buf, buf_len, to_ptr, &tmp_len) == -1) { \
	    if (unconv_num) \
		*unconv_num += buf_len / state->codeset->length; \
	    continue; \
	} \
\
	if (to_ptr) \
	    to_ptr += tmp_len; \
	if (to_len) \
	    *to_len += tmp_len; \
	to_length -= tmp_len; \
	state->last_codeset = state->codeset; \
    } \
\
    ret =  from_ptr - from; \
\
error: \
    (*methods->cnv_end)(state); \
    (*methods->destroy_state)(state); \
\
    return ret;

static int
strtostr(lcd, strtocs, from, from_len, cstostr, to, to_len, unconv_num)
    XLCd lcd;
    int (*strtocs)();
    unsigned char *from;
    int from_len;
    int (*cstostr)();
    unsigned char *to;
    int *to_len;
    int *unconv_num;
{
    unsigned char *from_ptr = from;
    unsigned char *to_ptr = to;
    STRING_CONV(strtocs, cstostr)
}

int
_Xlc_strcpy(lcd, str1, str1_len, str2, str2_len, unconv_num)
    XLCd lcd;
    register unsigned char *str1;
    register int str1_len;
    register unsigned char *str2;
    int *str2_len;
    int *unconv_num;
{
    unsigned char *str1_tmp = str1;

    if (str2_len && str1_len > *str2_len)
	str1_len = *str2_len;

    while (str1_len--)
	*str2++ = *str1++;

    if (unconv_num)
	*unconv_num = 0;
    if (str2_len)
	*str2_len = str1 - str1_tmp;

    return str1 - str1_tmp;
}

int
Ximp_mbstostring(mbstr, mbstr_len, string, string_len, unconv_num)
    unsigned char *mbstr;
    int mbstr_len;
    unsigned char *string;
    int *string_len;
    int *unconv_num;
{
    XLCd lcd = _XlcCurrentLC();

    if (lcd == NULL)
	return -1;

    return _Ximp_mbstostring(lcd, mbstr, mbstr_len, string, string_len,
			     unconv_num);
}

int
_Ximp_mbstostring(lcd, mbstr, mbstr_len, string, string_len, unconv_num)
    XLCd lcd;
    unsigned char *mbstr;
    int mbstr_len;
    unsigned char *string;
    int *string_len;
    int *unconv_num;
{
    LCMethods methods = LC_METHODS(lcd);

    return strtostr(lcd, methods->mbstocs, mbstr, mbstr_len, _Xlc_cstostring,
		    string, string_len, unconv_num);
}


int
Ximp_mbstoct(mbstr, mbstr_len, ctext, ctext_len, unconv_num)
    unsigned char *mbstr;
    int mbstr_len;
    unsigned char *ctext;
    int *ctext_len;
    int *unconv_num;
{
    XLCd lcd = _XlcCurrentLC();

    if (lcd == NULL)
	return -1;

    return _Ximp_mbstoct(lcd, mbstr, mbstr_len, ctext, ctext_len, unconv_num);
}

int
_Ximp_mbstoct(lcd, mbstr, mbstr_len, ctext, ctext_len, unconv_num)
    XLCd lcd;
    unsigned char *mbstr;
    int mbstr_len;
    unsigned char *ctext;
    int *ctext_len;
    int *unconv_num;
{
    LCMethods methods = LC_METHODS(lcd);

    return strtostr(lcd, methods->mbstocs, mbstr, mbstr_len, _Xlc_cstoct,
		    ctext, ctext_len, unconv_num);
}


int
Ximp_cttombs(ctext, ctext_len, mbstr, mbstr_len, unconv_num)
    unsigned char *ctext;
    int ctext_len;
    unsigned char *mbstr;
    int *mbstr_len;
    int *unconv_num;
{
    XLCd lcd = _XlcCurrentLC();

    if (lcd == NULL)
	return -1;

    return _Ximp_cttombs(lcd, ctext, ctext_len, mbstr, mbstr_len, unconv_num);
}

int
_Ximp_cttombs(lcd, ctext, ctext_len, mbstr, mbstr_len, unconv_num)
    XLCd lcd;
    unsigned char *ctext;
    int ctext_len;
    unsigned char *mbstr;
    int *mbstr_len;
    int *unconv_num;
{
    unsigned char ch, *ctptr = ctext;
    unsigned char *bufptr = mbstr;
    unsigned char *tmpptr;
    unsigned char side;
    int buf_len, tmp_len, skip_size;
    int ret = -1;
    LCMethods methods = LC_METHODS(lcd);
    State state;
    CharSet charset, GR_charset, GL_charset;
    CodeSet codeset;

    if (mbstr_len)
	buf_len = *mbstr_len;
    else
	buf_len = MAXINT;
    if (unconv_num)
	*unconv_num = 0;
    
    GL_charset = _XlcGetCharSetFromName("ISO8859-1", GL);
    GR_charset = _XlcGetCharSetFromName("ISO8859-1", GR);

    state = (*methods->create_state)(lcd);
    (*methods->cnv_start)(state);

    while (ctext_len > 0 && buf_len > 0) {
	ch = *ctptr;
	if (ch == 0x1b || ch == 0x9b) {
	    if (ch == 0x1b)
		tmp_len = _XlcCheckESCSequence(ctptr, ctext_len, &charset);
	    else
		tmp_len = _XlcCheckCSISequence(ctptr, ctext_len, &charset);

	    if (tmp_len > 0 && charset) {
		if (charset->side == GL)
		    GL_charset = charset;
		else
		    GR_charset = charset;
	    }
	} else {
	    tmpptr = ctptr;
	    side = ch & 0x80;
	    for ( ; ctext_len; tmpptr++, ctext_len--) {
		ch = *tmpptr;
		if (side != (ch & 0x80) || ch == '\033' || ch == 0x9b)
		    break;
	        if ((ch < 0x20 && ch != '\n' && ch != '\t') ||
			(ch >= 0x80 && ch < 0xa0))
		    goto error;
	    }

	    charset = side ? GR_charset : GL_charset;
	    if (codeset = _XlcGetCodeSetFromCharSet(lcd, charset)) {
		state->codeset = codeset;
		tmp_len = buf_len;
		skip_size = (*methods->cstombs)(state, ctptr, tmpptr - ctptr,
						bufptr, &tmp_len);
		if (skip_size < 0 || skip_size != tmpptr - ctptr)
			goto error;

		bufptr += tmp_len;
		buf_len -= tmp_len;
	    } else if (unconv_num)
		*unconv_num += tmpptr - ctptr;
	    ctptr = tmpptr;
	    continue;
	}
	if (tmp_len < 0)
	    goto error;
	ctptr += tmp_len;
	ctext_len -= tmp_len;
    }
    if (mbstr_len)
	*mbstr_len = bufptr - mbstr;
    ret = ctptr - ctext;
error:
    (*methods->cnv_end)(state);
    (*methods->destroy_state)(state);

    return ret;
}

int
_Ximp_ct_mbslen(lcd, ctext, ctext_len, unconv_num)
    XLCd lcd;
    unsigned char *ctext;
    int ctext_len;
    int *unconv_num;
{
    unsigned char ch, *ctptr = ctext;
    unsigned char *tmpptr;
    unsigned char side;
    unsigned char buf[BUFSIZE];
    int tmp_len, skip_size;
    int ret = 0;
    LCMethods methods = LC_METHODS(lcd);
    State state;
    CharSet charset, GR_charset, GL_charset;
    CodeSet codeset;

    if (unconv_num)
	*unconv_num = 0;
    
    GL_charset = _XlcGetCharSetFromName("ISO8859-1", GL);
    GR_charset = _XlcGetCharSetFromName("ISO8859-1", GR);

    state = (*methods->create_state)(lcd);
    (*methods->cnv_start)(state);

    while (ctext_len > 0) {
	ch = *ctptr;
	if (ch == 0x1b || ch == 0x9b) {
	    if (ch == 0x1b)
		tmp_len = _XlcCheckESCSequence(ctptr, ctext_len, &charset);
	    else
		tmp_len = _XlcCheckCSISequence(ctptr, ctext_len, &charset);

	    if (tmp_len > 0 && charset) {
		if (charset->side == GL)
		    GL_charset = charset;
		else
		    GR_charset = charset;
	    }
	} else {
	    tmpptr = ctptr;
	    side = ch & 0x80;
	    for ( ; ctext_len; tmpptr++, ctext_len--) {
		ch = *tmpptr;
		if (side != (ch & 0x80) || ch == '\033' || ch == 0x9b)
		    break;
	        if ((ch < 0x20 && ch != '\n' && ch != '\t') ||
			(ch >= 0x80 && ch < 0xa0)) {
		    ret = -1;
		    goto error;
		}
	    }

	    charset = side ? GR_charset : GL_charset;
	    if (codeset = _XlcGetCodeSetFromCharSet(lcd, charset)) {
		state->codeset = codeset;
		tmp_len = BUFSIZE;
		skip_size = (*methods->cstombs)(state, ctptr, tmpptr - ctptr,
						buf, &tmp_len);
		if (skip_size < 0) {
		    ret = -1;
		    goto error;
		}
		ret += tmp_len;
	    } else if (unconv_num)
		*unconv_num += tmpptr - ctptr;
	    ctptr = tmpptr;
	    continue;
	}
	if (tmp_len < 0) {
	    ret = -1;
	    goto error;
	}
	ctptr += tmp_len;
	ctext_len -= tmp_len;
    }
error:
    (*methods->cnv_end)(state);
    (*methods->destroy_state)(state);

    return ret;
}
