#ident	"@(#)R5Xlib:Ximp/XimpConv.c	1.7"

/* $XConsortium: XimpConv.c,v 1.6 92/04/14 13:28:45 rws Exp $ */
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

#define CHAR_LENGTH(xxxtocs) \
    unsigned char buf[BUFSIZE]; \
    int buf_len, scan_len; \
    int ret = 0; \
    LCMethods methods = LC_METHODS(lcd); \
    State state; \
\
    state = (*methods->create_state)(lcd); \
    (*methods->cnv_start)(state); \
\
    while (from_len > 0) { \
        buf_len = BUFSIZE; \
        scan_len = (*xxxtocs)(state, from_ptr, from_len, buf, &buf_len); \
        if (scan_len == -1) { \
	    ret = -1; \
	    goto error; \
	} \
        if (scan_len == 0)  \
	    break; \
\
	ret += buf_len / state->codeset->length; \
        from_ptr += scan_len; \
        from_len -= scan_len; \
    } \
\
error: \
    (*methods->cnv_end)(state); \
    (*methods->destroy_state)(state); \
\
    return ret;

int
_Xlc_str_charlen(lcd, strtocs, from, from_len)
    XLCd lcd;
    int (*strtocs)();
    unsigned char *from;
    int from_len;
{
    unsigned char *from_ptr = from;
    CHAR_LENGTH(strtocs)
}

int
_Ximp_mbs_charlen(lcd, mbstr, mbstr_len)
    XLCd lcd;
    unsigned char *mbstr;
    int mbstr_len;
{
    LCMethods methods = LC_METHODS(lcd);

    return _Xlc_str_charlen(lcd, methods->mbstocs, mbstr, mbstr_len);
}


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
\
    state = (*methods->create_state)(lcd); \
    (*methods->cnv_start)(state); \
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
	    goto error; \
	} \
\
	if (to_ptr) \
	    to_ptr += tmp_len; \
	if (to_len) \
	    *to_len += tmp_len; \
	to_length -= tmp_len; \
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
strtowstr(lcd, strtocs, from, from_len, cstowstr, to, to_len)
    XLCd lcd;
    int (*strtocs)();
    unsigned char *from;
    int from_len;
    int (*cstowstr)();
    wchar_t *to;
    int *to_len;
{
    unsigned char *from_ptr = from;
    wchar_t *to_ptr = to;
    STRING_CONV(strtocs, cstowstr)
}

static int
wstrtostr(lcd, wstrtocs, from, from_len, cstostr, to, to_len)
    XLCd lcd;
    int (*wstrtocs)();
    wchar_t *from;
    int from_len;
    int (*cstostr)();
    unsigned char *to;
    int *to_len;
{
    wchar_t *from_ptr = from;
    unsigned char *to_ptr = to;
    STRING_CONV(wstrtocs, cstostr)
}


int
_Xmblen(str, len)
    char *str;
    int len;
{
    return _Xmbtowc(NULL, str, len);
}

int
_Xmbtowc(wstr, str, len)
    wchar_t *wstr;
    char *str;
    int len;
{
    XimpLCd lcd = (XimpLCd) _XlcCurrentLC();
    LCMethods methods = LC_METHODS(lcd);
    wchar_t tmp_wc;
    int one = 1;

    if (lcd == NULL)
	return -1;
    if (str == NULL)
	return lcd->locale.state_dependent;
    if (len == 0)
	return 0;
    if (*str == '\0') {
	*wstr = 0;
	return 0;
    }
    if (wstr == NULL)
	wstr = &tmp_wc;

    return strtowstr(lcd, methods->mbstocs, str, len, methods->cstowcs,
		     wstr, &one);
}

int
_Xwctomb(str, wc)
    char *str;
    wchar_t wc;
{
    int len;
    XimpLCd lcd = (XimpLCd) _XlcCurrentLC();
    LCMethods methods = LC_METHODS(lcd);

    if (lcd == NULL)
	return -1;
    if (str == NULL)
	return lcd->locale.state_dependent;
    len = XIMP_MB_CUR_MAX(lcd);

    if (wstrtostr(lcd, methods->wcstocs, &wc, 1, methods->cstombs,
		  str, &len) < 0)
	return -1;
    
    return len;
}

int
_Xmbstowcs(wstr, str, len)
    wchar_t *wstr;
    char *str;
    int len;
{
    XLCd lcd = _XlcCurrentLC();
    LCMethods methods = LC_METHODS(lcd);
    int length = len;

    if (lcd == NULL)
	return -1;
    
    if (strtowstr(lcd, methods->mbstocs, str, strlen(str), methods->cstowcs,
		  wstr, &len) < 0)
	return -1;
    
    if (len < length)
	wstr[len] = (wchar_t) 0;

    return len;
}

int
_Xwcstombs(str, wstr, len)
    char *str;
    wchar_t *wstr;
    int len;
{
    XLCd lcd = _XlcCurrentLC();
    LCMethods methods = LC_METHODS(lcd);
    int length = len;

    if (lcd == NULL)
	return -1;

    if (wstrtostr(lcd, methods->wcstocs, wstr, _Xwcslen(wstr), methods->cstombs,		  str, &len) < 0)
	return -1;
    
    if (len < length)
	str[len] = '\0';

    return len;
}

wchar_t *
_Xwcscpy(wstr1, wstr2)
    register wchar_t *wstr1, *wstr2;
{
    wchar_t *wstr_tmp = wstr1;

    while (*wstr1++ = *wstr2++)
	;

    return wstr_tmp;
}

wchar_t *
_Xwcsncpy(wstr1, wstr2, len)
    register wchar_t *wstr1, *wstr2;
    register len;
{
    wchar_t *wstr_tmp = wstr1;

    while (len-- > 0)
	if (!(*wstr1++ = *wstr2++))
	    break;

    while (len-- > 0)
	*wstr1++ = (wchar_t) 0;

    return wstr_tmp;
}

int
_Xwcslen(wstr)
    register wchar_t *wstr;
{
    register wchar_t *wstr_ptr = wstr;

    while (*wstr_ptr)
	wstr_ptr++;
    
    return wstr_ptr - wstr;
}

int
_Xwcscmp(wstr1, wstr2)
    register wchar_t *wstr1, *wstr2;
{
    for ( ; *wstr1 && *wstr2; wstr1++, wstr2++)
	if (*wstr1 != *wstr2)
	    break;

    return *wstr1 - *wstr2;
}

int
_Xwcsncmp(wstr1, wstr2, len)
    register wchar_t *wstr1, *wstr2;
    register len;
{
    for ( ; *wstr1 && *wstr2 && len > 0; wstr1++, wstr2++, len--)
	if (*wstr1 != *wstr2)
	    break;

    if (len <= 0)
	return 0;

    return *wstr1 - *wstr2;
}

#ifndef ARCHIVE
#define XDefaultString _XDefaultString
#endif /* ARCHIVE */
char *
#ifndef DYNAMICLIB
XDefaultString()
#else
_XimpDefaultString(lcd)
XLCd  lcd ;
#endif
{
    return "";
}
