#pragma ident	"@(#)R5Xlib:Ximp/XimpLCUtil.c	1.3"

/* $XConsortium: XimpLCUtil.c,v 1.7 92/04/14 13:29:21 rws Exp $ */
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
#include <X11/Xos.h>
#include "Ximplc.h"
#include <stdio.h>
#include <ctype.h>

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

static void free_charset();

static enum {
    T_NONE = E_LAST,
    T_CHARSET,
    T_CODESET,
    T_FALSE,
    T_FONT,
    T_INITIAL_STATE_GL,
    T_INITIAL_STATE_GR,
    T_LENGTH,
    T_MB_CUR_MAX,
    T_MB_ENCODING,
    T_STRING,
    T_STATE_DEPEND_ENCODING,
    T_TRUE,
    T_WC_ENCODING,
    T_WC_ENCODING_MASK,
    T_WC_SHIFT_BITS
} token_value;

typedef struct KeywordRec {
    char *name;
    int value;
} KeywordRec;

static KeywordRec keyword_tbl[] = {
    { "CHARSET", T_CHARSET },
    { "CODESET", T_CODESET },
    { "ENCODING", T_CHARSET },
    { "FALSE", T_FALSE },
    { "FONT", T_FONT },
    { "GL", E_GL},
    { "GR", E_GR},
    { "INITIAL_STATE_GL", T_INITIAL_STATE_GL},
    { "INITIAL_STATE_GR", T_INITIAL_STATE_GR},
    { "LENGTH", T_LENGTH },
    { "MB_ENCODING", T_MB_ENCODING },
    { "MB_CUR_MAX", T_MB_CUR_MAX },
    { "STATE_DEPEND_ENCODING", T_STATE_DEPEND_ENCODING },
    { "TRUE", T_TRUE },
    { "WC_ENCODING_MASK", T_WC_ENCODING_MASK },
    { "WC_ENCODING", T_WC_ENCODING },
    { "WC_SHIFT_BITS", T_WC_SHIFT_BITS },
    { "<SS>", E_SS },
    { "<LSL>", E_LSL },
    { "<LSR>", E_LSR },
    0,
};

#define SKIP_WHITE(str)		\
    while (*(str) == ' ' || *(str) == '\t' || *(str) == '\n') str++;
#define SKIP_TO_WHITE(str)	\
    while (*(str) && *(str) != ' ' && *(str) != '\t' && *(str) != '\n') str++;

static char *
get_token(src, dst, token)
    char *src;
    char *dst;
    int *token;
{
    KeywordRec *keyword = keyword_tbl;
    char *str;
    int len, tmp_len;

    SKIP_WHITE(src)
    str = src;
    SKIP_TO_WHITE(str)
    len = str - src;
    if (len == 0)
	return NULL;

    strncpy(dst, src, len);
    *token = T_STRING;

    for ( ; keyword->name; keyword++) {
	tmp_len = strlen(keyword->name);
	if (tmp_len > len)
	    continue;

	if (!_Ximp_NCompareISOLatin1(dst, keyword->name, tmp_len)) {
	    *token = keyword->value;
	    len = tmp_len;
	    break;
	}
    }

    dst[len] = '\0';

    return src + len;
}

static char *
get_word(str, len)
    register char *str;
    int *len;
{
    register char ch, *strptr;

    ch = *str;
    while (ch == ' ' || ch == '\t' || ch == '\n')
	ch = *(++str);

    strptr = str;
    ch = *strptr;
    while (ch != ' ' && ch != '\t' && ch != '\n' && ch != 0)
	ch = *(++strptr);

    if (strptr == str)
	return NULL;

    *len = strptr - str;
    return str;
}

#ifndef XLIBI18N_PATH
/* #define XLIBI18N_PATH	"/usr/lib/X11" */
#define XLIBI18N_PATH "/usr/X/lib/locale"
#endif

FILE *
_XlcOpenLocaleFile(dir, locale, name)
    char *dir;
    char *locale;
    char *name;
{
    FILE *fd;
    char buf[BUFSIZE], locale_file[BUFSIZE];

    if (locale)
	sprintf(locale_file, "%s/%s", locale, name);
    else
	strcpy(locale_file, name);

    if (dir) {
	sprintf(buf, "%s/%s", dir, locale_file);
	if (fd = fopen(buf, "r"))
	    return fd;
    }

    if (dir = getenv("XLIBI18N_PATH")) {
	sprintf(buf, "%s/%s", dir, locale_file);
	if (fd = fopen(buf, "r"))
	    return fd;
    }
#ifdef sun
    if (dir = getenv("OPENWINHOME")) {
	sprintf(buf, "%s/lib/locale/%s", dir, locale_file);
	if (fd = fopen(buf, "r"))
	    return fd;
    }
#endif
    sprintf(buf, "%s/%s", XLIBI18N_PATH, locale_file);

    return fopen(buf, "r");
}

#ifndef LOCALE_ALIAS
#define LOCALE_ALIAS	"locale.alias"
#endif

static Bool
get_locale_name(locale, name_ret)
    char *locale;
    char *name_ret;
{
    char *bufptr, buf[BUFSIZE];
    int length;
    FILE *fd;

    if (fd = _XlcOpenLocaleFile(NULL, NULL, LOCALE_ALIAS)) {
	while (fgets(buf, BUFSIZE, fd)) {
	    bufptr = get_word(buf, &length);
	    if (bufptr == NULL)
		continue;
	    bufptr[length] = '\0';
	    if (strcmp(locale, bufptr))
		continue;

	    bufptr += length + 1;
	    bufptr = get_word(bufptr, &length);
	    if (bufptr == NULL)
		continue;
	    
	    bufptr[length] = '\0';
	    locale = bufptr;
	    break;
	}
    }

    if (fd)
	fclose(fd);

    strcpy(name_ret, locale);

    return True;
}

static Bool
set_locale_name(lcd, core_name, name)
    XimpLCd lcd;
    char *core_name;
    char *name;
{
    char *language, *territory, *codeset, *str, buf[BUFSIZE];
    int length;

    length = strcmp(core_name, name) ? strlen(core_name) : 0;
    length += strlen(name) * 2 + 5;

    str = (char *) Xmalloc(length);
    if (str == NULL)
	return False;

    strcpy(buf, name);

    if (codeset = rindex(buf, '.'))
	*codeset++ = '\0';

    if (territory = rindex(buf, '_'))
	*territory++ = '\0';

    language = buf;

    strcpy(str, core_name);
    lcd->core.name = str;
    if (strcmp(core_name, name)) {
	str += strlen(str) + 1;

	strcpy(str, name);
    }
    lcd->locale.name = str;
    str += strlen(str) + 1;

    if (language)
	strcpy(str, language);
    else
	*str = '\0';
    lcd->locale.language = str;
    str += strlen(str) + 1;
	

    if (territory)
	strcpy(str, territory);
    else
	*str = '\0';
    lcd->locale.territory = str;
    str += strlen(str) + 1;

    if (codeset)
	strcpy(str, codeset);
    else
	*str = '\0';
    lcd->locale.codeset = str;

    return True;
}

XimpLCd
_XlcCreateLC(core_name, methods, lc_methods)
    char *core_name;
    XLCdMethods methods;
    LCMethods lc_methods;
{
    char name[BUFSIZE];
    XimpLCd lcd;

    if (get_locale_name(core_name, name) == False)
	return (XimpLCd) NULL;
    
    lcd = (XimpLCd) Xmalloc(sizeof(XimpLCdRec));
    if (lcd == NULL)
	return (XimpLCd) NULL;
    bzero((char *) lcd, sizeof(XimpLCdRec));

    if (set_locale_name(lcd, core_name, name) == False) {
	_XlcDestroyLC(lcd);
	return (XimpLCd) NULL;
    }

    lcd->methods = methods;
    lcd->lc_methods = lc_methods;

    return lcd;
}

void
_XlcDestroyLC(lcd)
    XimpLCd lcd;
{
    if (lcd->core.name)
	XFree(lcd->core.name);
    
    XFree(lcd);
}

static Bool
add_charset_list(codeset, charset)
    CodeSet codeset;
    CharSet charset;
{
    CharSet *new;
    int num;

    if (num = codeset->charset_num)
	new = (CharSet *) Xrealloc(codeset->charset_list,
				   (num + 1) * sizeof(CharSet));
    else
	new = (CharSet *) Xmalloc(sizeof(CharSet));

    if (new == NULL)
	return False;

    new[num++] = charset;
    codeset->charset_list = new;
    codeset->charset_num = num;

    return True;
}

static CodeSet
add_codeset(locale)
    Locale locale;
{
    CodeSet new, *new_list;
    int num;

    new = (CodeSet) Xmalloc(sizeof(CodeSetRec));
    if (new == NULL)
	return NULL;
    bzero((char *) new, sizeof(CodeSetRec));

    if (num = locale->codeset_num)
	new_list = (CodeSet *) Xrealloc(locale->codeset_list,
					(num + 1) * sizeof(CodeSet));
    else
	new_list = (CodeSet *) Xmalloc(sizeof(CodeSet));

    if (new_list == NULL)
	goto error;

    new_list[num] = new;
    locale->codeset_list = new_list;
    locale->codeset_num = num + 1;

    return new;

error:
    XFree(new);

    return NULL;
}

Bool
_XlcAddParseList(locale, type, encoding, codeset)
    Locale locale;
    EncodingType type;
    char *encoding;
    CodeSet codeset;
{
    ParseInfo new, *new_list;
    char *str;
    unsigned char ch;
    int num;

    str = (char *) Xmalloc(strlen(encoding) + 1);
    if (str == NULL)
	return False;
    strcpy(str, encoding);

    new = (ParseInfo) Xmalloc(sizeof(ParseInfoRec));
    if (new == NULL)
	goto error;
    bzero((char *) new, sizeof(ParseInfoRec));

    if (locale->mb_parse_table == NULL) {
	locale->mb_parse_table = (unsigned char *) Xmalloc(256); /* 2^8 */
	if (locale->mb_parse_table == NULL)
	    goto error;
	bzero((char *) locale->mb_parse_table, 256);
    }

    if (num = locale->mb_parse_list_num)
	new_list = (ParseInfo *) Xrealloc(locale->mb_parse_list,
					  (num + 2) * sizeof(ParseInfo));
    else {
	new_list = (ParseInfo *) Xmalloc(2 * sizeof(ParseInfo));
    }

    if (new_list == NULL)
	goto error;

    new_list[num] = new;
    new_list[num + 1] = (ParseInfo) NULL;
    locale->mb_parse_list = new_list;
    locale->mb_parse_list_num = num + 1;

    ch = (unsigned char) *str;
    if (locale->mb_parse_table[ch] == 0)
	locale->mb_parse_table[ch] = num + 1;

    new->type = type;
    new->encoding = str;
    new->codeset = codeset;

    if (codeset->parse_info == NULL)
	codeset->parse_info = new;

    return True;

error:
    XFree(str);
    if (new)
	XFree(new);

    return False;
}

static FontSetData
add_fontset(locale)
    Locale locale;
{
    FontSetData new;
    int num;

    if (num = locale->fontset_data_num)
	new = (FontSetData) Xrealloc(locale->fontset_data,
				 (num + 1) * sizeof(FontSetDataRec));
    else
	new = (FontSetData) Xmalloc(sizeof(FontSetDataRec));

    if (new == NULL)
	return NULL;

    locale->fontset_data_num = num + 1;
    locale->fontset_data = new;

    new += num;
    bzero((char *) new, sizeof(FontSetDataRec));

    return new;
}

Bool
_XlcLoadCodeSet(lcd)
    XimpLCd lcd;
{
    Locale locale = &lcd->locale;
    CodeSetRec *codeset;
    CharSet charset;
    FontSetData font_data;
    char *next, buf[BUFSIZE], tmp[256], tmp2[256];
    int num, cur_num, token, category, type, tmp_token;
    unsigned long mask;
    FILE *fd = NULL;

    fd = _XlcOpenLocaleFile(NULL, locale->name, CODESET_FILE);

    if (fd == NULL && locale->language)
	fd = _XlcOpenLocaleFile(NULL, locale->language, CODESET_FILE);

    if (fd == NULL)
	return False;

    locale->codeset_num = 0;
    locale->fontset_data_num = 0;
    codeset = NULL;

    while (fgets(buf, BUFSIZE,fd)) {
	next = get_token(buf, tmp, &token);
	if (next == NULL)
	    continue;

	switch (token) {
	    case T_CODESET:
		next = get_token(next, tmp, &tmp_token);
		if (next == NULL)
		    continue;
		cur_num = atoi(tmp);
		codeset = add_codeset(locale);
		if (codeset == NULL)
		    goto error;
		codeset->cs_num = cur_num;
		category = token;
		continue;
	    case E_GL:
	    case E_GR:
		if (codeset == NULL)
		    continue;
		codeset->side = (token == E_GL) ? GL : GR;
		continue;
	    case T_LENGTH:
	    case T_MB_CUR_MAX:
	    case T_WC_SHIFT_BITS:
		next = get_token(next, tmp, &tmp_token);
		if (next == NULL)
		    continue;
		num = *tmp - '0';
		if (num >= 0 && num <= 9) {
		    if (token == T_LENGTH && codeset)
			codeset->length = num;
		    else if (token == T_MB_CUR_MAX)
			locale->mb_cur_max = num;
		    else if (token == T_WC_SHIFT_BITS)
			locale->wc_shift_bits = num;
		}
		continue;
	    case T_MB_ENCODING:
		if (codeset == NULL)
		    continue;
		num = 0;
		type = E_SS;	/* for BC */
		while (next = get_token(next, tmp, &token)) {
		    if (token == E_SS || token == E_LSL || token == E_LSR) {
			type = token;
			continue;
		    }
		    tmp2[num] = (char) strtol(tmp, NULL, 0);	/* XXX */
		    if (tmp2[num] == '\0')
			break;
		    num++;
		}
		if (num == 0)
		    continue;
		tmp2[num] = '\0';
		_XlcAddParseList(locale, type, tmp2, codeset);
		continue;
	    case T_WC_ENCODING_MASK:
	    case T_WC_ENCODING:
		next = get_token(next, tmp, &tmp_token);
		if (next == NULL)
		    continue;
		mask = (unsigned long) strtol(tmp, NULL, 0);	/* XXX */
		if (token == T_WC_ENCODING_MASK)
		    locale->wc_encode_mask = mask;
		else if (token == T_WC_ENCODING && codeset)
		    codeset->wc_encoding = mask;
		continue;
	    case T_STATE_DEPEND_ENCODING:
		next = get_token(next, tmp, &token);
		if (next == NULL)
		    continue;
		locale->state_dependent = (token == T_TRUE) ? True : False;
		continue;
	    case T_INITIAL_STATE_GL:
	    case T_INITIAL_STATE_GR:
		if (codeset == NULL)
		    continue;
		if (token == T_INITIAL_STATE_GL)
		    locale->initial_state_GL = codeset;
		else
		    locale->initial_state_GR = codeset;
		continue;
	    case T_CHARSET:
	    case T_FONT:
		category = token;
		continue;
	    case T_STRING:
		if (*tmp == '#')
		    continue;
		if (category == T_CHARSET && codeset) {
		    next = get_token(next, tmp2, &token);
		    if (next == NULL ||
			token != E_GL && token != E_GR)
			continue;

		    charset = _XlcGetCharSetFromName(tmp,
				(token == E_GL) ? GL : GR);
		    if (charset == NULL)
			continue;

		    if (add_charset_list(codeset, charset) == False)
			goto error;
		} else if (category == T_FONT) {
		    next = get_token(next, tmp2, &token);
		    if (next == NULL ||
			token != E_GL && token != E_GR)
			continue;
		    
		    font_data = add_fontset(locale);
		    if (font_data == NULL)
			goto error;
		    
		    font_data->font_name = (char *) Xmalloc(strlen(tmp) + 1);
		    if (font_data->font_name == NULL)
			goto error;
		    strcpy(font_data->font_name, tmp);

		    font_data->cs_num = cur_num;
		    font_data->side = (token == E_GL) ? GL : GR;
		}
		continue;
	}
    }

    fclose(fd);

    return True;

error:
    free_charset(lcd);
    fclose(fd);

    return False;
}

static void
free_charset(lcd)
    XimpLCd lcd;
{
    Locale locale = &lcd->locale;
    CodeSet *codeset;
    ParseInfo *parse_info;
    FontSetData font_data;
    int num;

    if (num = locale->fontset_data_num) {
	for (font_data = locale->fontset_data; num-- > 0; font_data++)
	    if (font_data->font_name)
		XFree(font_data->font_name);
	XFree(locale->fontset_data);
    }

    if (locale->mb_parse_table)
	XFree(locale->mb_parse_table);
    if (num = locale->mb_parse_list_num) {
	for (parse_info = locale->mb_parse_list; num-- > 0; parse_info++) {
	    if ((*parse_info)->encoding)
		XFree((*parse_info)->encoding);
	    XFree(*parse_info);
	}
	XFree(locale->mb_parse_list);
    }

    if (num = locale->codeset_num) {
	for (codeset = locale->codeset_list; num-- > 0; codeset++) {
	    if ((*codeset)->charset_list)
		XFree((*codeset)->charset_list);
	}
	XFree(locale->codeset_list);
    }
}


#ifdef X_NOT_STDC_ENV
#ifndef toupper
#define toupper(c)	((int)(c) - 'a' + 'A')
#endif
#endif

int 
_Ximp_CompareISOLatin1(str1, str2)
    char *str1, *str2;
{
    register char ch1, ch2;

    for ( ; (ch1 = *str1) && (ch2 = *str2); str1++, str2++) {
	if (islower(ch1))
	    ch1 = toupper(ch1);
	if (islower(ch2))
	    ch2 = toupper(ch2);

	if (ch1 != ch2)
	    break;
    }

    return *str1 - *str2;
}
int 
_Ximp_NCompareISOLatin1(str1, str2, len)
    char *str1, *str2;
    int len;
{
    register char ch1, ch2;

    for ( ; (ch1 = *str1) && (ch2 = *str2) && len; str1++, str2++, len--) {
	if (islower(ch1))
	    ch1 = toupper(ch1);
	if (islower(ch2))
	    ch2 = toupper(ch2);

	if (ch1 != ch2)
	    break;
    }

    if (len == 0)
	return 0;

    return *str1 - *str2;
}
