#pragma ident	"@(#)R5Xlib:Ximp/XimpDefCnv.c	1.4"

/* $XConsortium: XimpDefCnv.c,v 1.3 92/04/14 13:28:51 rws Exp $ */
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

#ifndef ARCHIVE
#pragma weak _XlcDefaultMapModifiers
#endif

static char def_mbchar();
static int def_mbstocs(), def_wcstocs(), def_cstombs(), def_cstowcs();

static XLCdMethodsRec lcd_methods = {
    _XlcDefaultMapModifiers,
    _XDefaultCreateFontSet,
    _Ximp_OpenIM,
};

static LCMethodsRec c_lc_methods = {
    _XlcDestroyLC,
    _XlcCreateState,
    _XlcDestroyState,
    _XlcCnvStart,
    _XlcCnvEnd,
    def_mbchar,
    def_mbstocs,
    def_wcstocs,
    def_cstombs,
    def_cstowcs,
};


XLCd
_XlcCLoader(name)
    char *name;
{
    XimpLCd lcd;
#ifdef XIMP_BC
extern Bool _XimpBCSetAtr();
#endif

    lcd = _XlcCreateLC(name, &lcd_methods, &c_lc_methods);
    if (lcd == NULL)
	return (XLCd) NULL;

    if (strcmp(name, "C") && strncmp(lcd->locale.codeset, "ISO8859", 7))
	goto error;

    if (_XlcLoadCodeSet(lcd) == False)
        goto error;

#ifdef XIMP_BC
    if (lcd->locale.mb_cur_max == 0)
	if (_XimpBCSetAtr(lcd) == False)
	    goto error;
#endif

    return (XLCd) lcd;

error:
    _XlcDestroyLC(lcd);

    return (XLCd) NULL;
}

static State state_list = NULL;

State
_XlcCreateState(lcd)
    XimpLCd lcd;
{
    State state;

    for (state = state_list; state; state = state->next)
	if (state->is_used == False)
	    break;

    if (state == NULL) {
	state = (State) Xmalloc(sizeof(StateRec));
	if (state == NULL)
	    return (State) NULL;

	state->next = state_list;
	state_list = state;
    }

    state->lcd = lcd;
    state->is_used = True;

    return state;
}

void
_XlcDestroyState(state)
    State state;
{
    state->is_used = False;
}

void
_XlcCnvStart(state)
    State state;
{
    Locale locale = &state->lcd->locale;

    state->GL_codeset = locale->initial_state_GL;
    state->GR_codeset = locale->initial_state_GR;

    if (state->GL_codeset == NULL)
	state->GL_codeset = *locale->codeset_list;
}

void
_XlcCnvEnd(state)
    State state;
{
}


static char
def_mbchar(state, str, lenp)
    State state;
    register char *str;
    register int *lenp;
{
    *lenp = 1;

    return *str;
}

static int
def_mbstocs(state, mbstr, mbstr_len, csbuf, csbuf_len)
    State state;
    unsigned char *mbstr;
    register int mbstr_len;
    register unsigned char *csbuf;
    int *csbuf_len;
{
    register unsigned char *mbptr = mbstr, msb_mask;

    if (csbuf_len && mbstr_len > *csbuf_len)
	mbstr_len = *csbuf_len;

    msb_mask = *mbptr & 0x80;
    while (mbstr_len--) {
	*csbuf++ = *mbptr++ & 0x7f;
	if ((*mbptr & 0x80) != msb_mask)
	    break;
    }

    if (msb_mask && state->GR_codeset)
	state->codeset = state->GR_codeset;
    else
	state->codeset = state->GL_codeset;

    if (csbuf_len)
	*csbuf_len = mbptr - mbstr;

    return mbptr - mbstr;
}

static int
def_wcstocs(state, wcstr, wcstr_len, csbuf, csbuf_len)
    State state;
    wchar_t *wcstr;
    register int wcstr_len;
    register unsigned char *csbuf;
    int *csbuf_len;
{
    register wchar_t *wcptr = wcstr, msb_mask;
    unsigned long wc_encode_mask = state->lcd->locale.wc_encode_mask;
    int num = state->lcd->locale.codeset_num;

    if (csbuf_len && wcstr_len > *csbuf_len)
        wcstr_len = *csbuf_len;

    msb_mask = *wcptr & wc_encode_mask;
    while (wcstr_len--) {
        *csbuf++ = (unsigned char) *wcptr++ & 0x7f;
	if ((*wcptr & wc_encode_mask) != msb_mask)
	    break;
    }

    if (msb_mask && state->GR_codeset)
	state->codeset = state->GR_codeset;
    else
	state->codeset = state->GL_codeset;

    if (csbuf_len)
        *csbuf_len = wcptr - wcstr;

    return wcptr - wcstr;
}

static int
def_cstombs(state, csstr, csstr_len, mbbuf, mbbuf_len)
    State state;
    unsigned char *csstr;
    register int csstr_len;
    register unsigned char *mbbuf;
    int *mbbuf_len;
{
    register unsigned char *csptr = csstr;

    if (mbbuf_len && csstr_len > *mbbuf_len)
	csstr_len = *mbbuf_len;

    if (state->codeset->side) {
	while (csstr_len--)
	    *mbbuf++ = *csptr++ | 0x80;
    } else {
	while (csstr_len--)
	    *mbbuf++ = *csptr++ & 0x7f;
    }

    if (mbbuf_len)
	*mbbuf_len = csptr - csstr;

    return csptr - csstr;
}

static int
def_cstowcs(state, csstr, csstr_len, wcbuf, wcbuf_len)
    State state;
    unsigned char *csstr;
    int csstr_len;
    register wchar_t *wcbuf;
    int *wcbuf_len;
{
    register unsigned char *csptr = csstr;
    unsigned long wc_encoding = state->codeset->wc_encoding;

    if (wcbuf_len && csstr_len > *wcbuf_len)
	csstr_len = *wcbuf_len;

    while (csstr_len--)
	*wcbuf++ = (wchar_t) (*csptr++ & 0x7f) | wc_encoding;

    if (wcbuf_len)
	*wcbuf_len = csptr - csstr;

    return csptr - csstr;
}


static LCMethodsRec default_lc_methods = {
    _XlcDestroyLC,
    _XlcCreateState,
    _XlcDestroyState,
    _XlcCnvStart,
    _XlcCnvEnd,
    _Xlc_mbchar,
    _Xlc_mbstocs,
    _Xlc_wcstocs,
    _Xlc_cstombs,
    _Xlc_cstowcs,
};

XLCd
_XlcGenericLoader(name)
    char *name;
{
    XimpLCd lcd;

    lcd = _XlcCreateLC(name, &lcd_methods, &default_lc_methods);
    if (lcd == NULL)
	return (XLCd) NULL;

    if (_XlcLoadCodeSet(lcd) == False)
        goto error;

    return (XLCd) lcd;

error:
    _XlcDestroyLC(lcd);

    return (XLCd) NULL;
}

static int
compare(src, encoding, length)
    register char *src;
    register char *encoding;
    register int length;
{
    char *start = src;

    while (length-- > 0) {
	if (*src++ != *encoding++)
	    return 0;
	if (*encoding == '\0')
	    return src - start;
    }

    return 0;
}

static Bool
mbtocs(state, src_data, dst_data)
    State state;
    register CvtData src_data, dst_data;
{
    Locale locale = &state->lcd->locale;
    ParseInfo *parse_list, parse_info;
    CodeSet codeset;
    char *src = src_data->string.multi_byte;
    char *dst = dst_data->string.multi_byte;
    unsigned char ch, side;
    int length, number, encoding_len = 0;
    register i;

    if (locale->mb_parse_table) {
	number = locale->mb_parse_table[(unsigned char) *src];
	if (number > 0) {
	    parse_list = locale->mb_parse_list + number - 1;
	    for ( ; parse_info = *parse_list; parse_list++) {
		encoding_len = compare(src, parse_info->encoding,
				       src_data->length);
		if (encoding_len > 0) {
		    switch (parse_info->type) {
			case E_SS:
			    src += encoding_len;
			    codeset = parse_info->codeset;
			    goto found;
			case E_LSL:
			case E_LSR:
			    src += encoding_len;
			    if (parse_info->type == E_LSL)
			    	state->GL_codeset = parse_info->codeset;
			    else
			    	state->GR_codeset = parse_info->codeset;
			    length = 0;
			    codeset = parse_info->codeset;
			    goto end;
			case E_GL:
			    codeset = state->GL_codeset;
			    goto found;
			case E_GR:
			    codeset = state->GR_codeset;
			    goto found;
		    }
		}
	    }
	}
    }

    if ((*src & 0x80) && state->GR_codeset)
	codeset = state->GR_codeset;
    else
	codeset = state->GL_codeset;

found:
    if (codeset == NULL)
	return False;

    length = codeset->length;
    if (length > src_data->length - encoding_len)
	return False;

    if (dst) {
	if (length > dst_data->length)
	    return False;
	side = (*(codeset->charset_list))->side;
	for (i = 0; i < length; i++)
	    *dst++ = (*src++ & 0x7f) | side;
	dst_data->string.multi_byte = dst;
	dst_data->length -= length;
    }
end:
    src_data->string.multi_byte = src;
    src_data->length -= encoding_len + length;
    src_data->cvt_length = encoding_len + length;
    dst_data->cvt_length = length;
    dst_data->codeset = codeset;

    return True;
}

char
_Xlc_mbchar(state, str, lenp)
    State state;
    char *str;
    int *lenp;
{
    CvtDataRec src_data_rec, dst_data_rec;
    register CvtData src_data = &src_data_rec;
    register CvtData dst_data = &dst_data_rec;
    char buf[BUFSIZE];
    register int length;

    src_data->string.multi_byte = str;
    src_data->length = strlen(str) + 1;
    dst_data->string.multi_byte = buf;
    dst_data->length = BUFSIZE;

    length = 0;
    while (1) {
	if (mbtocs(state, src_data, dst_data) == False) {
	    *lenp = length;
	    return '\0';
	}
	length += src_data->cvt_length;
	if (dst_data->cvt_length > 0) {
	    state->codeset = dst_data->codeset;
	    *lenp = length;
	    return buf[0];
	}
    }
}

int
_Xlc_mbstocs(state, mbstr, mbstr_len, csbuf, csbuf_len)
    State state;
    char *mbstr;
    int mbstr_len;
    char *csbuf;
    int *csbuf_len;
{
    CvtDataRec src_data_rec, dst_data_rec;
    register CvtData src_data = &src_data_rec;
    register CvtData dst_data = &dst_data_rec;
    CodeSet codeset;
    register int src_cvt_length, dst_cvt_length;

    src_data->string.multi_byte = mbstr;
    src_data->length = mbstr_len;
    dst_data->string.multi_byte = csbuf;
    dst_data->length = csbuf_len ? *csbuf_len : MAXINT;

    if (mbtocs(state, src_data, dst_data) == False)
	return -1;
    codeset = dst_data->codeset;
    src_cvt_length = src_data->cvt_length;
    dst_cvt_length = dst_data->cvt_length;

    while (mbtocs(state, src_data, dst_data) == True) {
	if (dst_data->codeset != codeset)
	    break;
	
	src_cvt_length += src_data->cvt_length;
	dst_cvt_length += dst_data->cvt_length;
    }

    state->codeset = codeset;
    if (csbuf_len)
	*csbuf_len = dst_cvt_length;

    return src_cvt_length;
}

static CodeSet
wc_parse_codeset(state, wcstr)
    State state;
    wchar_t *wcstr;
{
    Locale locale = &state->lcd->locale;
    CodeSet *codeset;
    wchar_t wch;
    unsigned long wc_encoding;
    int num;

    wch = *wcstr;
    wc_encoding = wch & locale->wc_encode_mask;
    num = locale->codeset_num;
    codeset = locale->codeset_list;
    while (num-- > 0) {
	if (wc_encoding == (*codeset)->wc_encoding)
	    return *codeset;
	codeset++;
    }

    return NULL;
}

int
_Xlc_wcstocs(state, wcstr, wcstr_len, csbuf, csbuf_len)
    State state;
    wchar_t *wcstr;
    int wcstr_len;
    unsigned char *csbuf;
    int *csbuf_len;
{
    wchar_t *wcptr = wcstr;
    register unsigned char *bufptr = csbuf;
    register wchar_t wch;
    unsigned char *tmpptr;
    int buf_len;
    register length;
    Locale locale = &state->lcd->locale;
    CodeSet codeset;
    unsigned long wc_encoding;

    if (csbuf_len)
	buf_len = *csbuf_len;
    else
	buf_len = MAXINT;
    
    codeset = wc_parse_codeset(state, wcptr);
    wc_encoding = codeset->wc_encoding;

    if (wcstr_len < buf_len / codeset->length)
	buf_len = wcstr_len * codeset->length;

    for ( ; wcstr_len > 0 && buf_len > 0; wcptr++, wcstr_len--) {
	wch = *wcptr;
	if ((wch & locale->wc_encode_mask) != wc_encoding)
	    break;
	length = codeset->length;
	buf_len -= length;
	bufptr += length;

	tmpptr = bufptr - 1;
	while (length--) {
	    *tmpptr-- = (unsigned char) ((wch & 0x7f) | codeset->side);
	    wch >>= locale->wc_shift_bits;
	}
    }

    if (csbuf_len)
	*csbuf_len = bufptr - csbuf;

    state->codeset = codeset;

    return wcptr - wcstr;
}

int
_Xlc_cstombs(state, csstr, csstr_len, mbbuf, mbbuf_len)
    State state;
    unsigned char *csstr;
    int csstr_len;
    unsigned char *mbbuf;
    int *mbbuf_len;
{
    register unsigned char *csptr = csstr;
    register unsigned char *bufptr = mbbuf;
    register buf_len;
    int num, encoding_len = 0;
    CodeSet codeset = state->codeset;
    EncodingType type;

    if (mbbuf_len)
	buf_len = *mbbuf_len;
    else
	buf_len = MAXINT;
    
    if (codeset->parse_info) {
	switch (type = codeset->parse_info->type) {
	    case E_SS:
		encoding_len = strlen(codeset->parse_info->encoding);
		break;
	    case E_LSL:
	    case E_LSR:
		if (type == E_LSL) {
		    if (codeset == state->GL_codeset)
			break;
		} else {
		    if (codeset == state->GR_codeset)
			break;
		}
		encoding_len = strlen((char *) codeset->parse_info->encoding);
		if (encoding_len > buf_len)
		    return -1;
		strcpy((char *)bufptr, codeset->parse_info->encoding);
		bufptr += encoding_len;
		buf_len -= encoding_len;
		encoding_len = 0;
		if (type == E_LSL)
		    state->GL_codeset = codeset;
		else
		    state->GR_codeset = codeset;
		break;
	}
    }

    csstr_len /= codeset->length;
    buf_len /= codeset->length + encoding_len;
    if (csstr_len < buf_len)
	buf_len = csstr_len;
    
    while (buf_len--) {
	if (encoding_len) {
	    strcpy((char *) bufptr, codeset->parse_info->encoding);
	    bufptr += encoding_len;
	}
	num = codeset->length;
	while (num--)
	    *bufptr++ = (*csptr++ & 0x7f) | codeset->side;
    }

    if (mbbuf_len)
	*mbbuf_len = bufptr - mbbuf;

    return csptr - csstr;
}

int
_Xlc_cstowcs(state, csstr, csstr_len, wcbuf, wcbuf_len)
    State state;
    unsigned char *csstr;
    int csstr_len;
    wchar_t *wcbuf;
    int *wcbuf_len;
{
    register unsigned char *csptr = csstr;
    wchar_t *bufptr = wcbuf;
    register wchar_t wch;
    register buf_len;
    unsigned long code_mask, wc_encoding;
    int num, length, wc_shift_bits;

    if (wcbuf_len)
	buf_len = *wcbuf_len;
    else
	buf_len = MAXINT;

    length = state->codeset->length;
    csstr_len /= length;
    if (csstr_len < buf_len)
	buf_len = csstr_len;
    
    code_mask = ~state->lcd->locale.wc_encode_mask;
    wc_encoding = state->codeset->wc_encoding;
    wc_shift_bits = state->lcd->locale.wc_shift_bits;

    while (buf_len--) {
	wch = (wchar_t) (*csptr++ & 0x7f);
	num = length - 1;
	while (num--)
	    wch = (wch << wc_shift_bits) | (*csptr++ & 0x7f);

	*bufptr++ = (wch & code_mask) | wc_encoding;
    }

    if (wcbuf_len)
	*wcbuf_len = bufptr - wcbuf;

    return csptr - csstr;
}
