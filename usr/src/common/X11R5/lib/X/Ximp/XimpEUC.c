#pragma ident	"@(#)R5Xlib:Ximp/XimpEUC.c	1.5"

/* $XConsortium: XimpEUC.c,v 1.4 92/04/14 13:28:56 rws Exp $ */
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
#include <ctype.h>

#include "Ximplc.h"

#ifndef ARCHIVE
#pragma weak _XlcDefaultMapModifiers
#endif

static XLCdMethodsRec lcd_methods = {
    _XlcDefaultMapModifiers,
    _XDefaultCreateFontSet,
    _Ximp_OpenIM,
};

static LCMethodsRec lc_methods =
{
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

#ifdef XIMP_BC
#ifndef SS2
#define SS2	0x8e
#define SS3	0x8f
#endif	/* SS2 */

#ifdef SVR4
#define WC_MASK		0x30000000
#define CS1_WC_MASK	0x30000000
#define CS2_WC_MASK	0x10000000
#define CS3_WC_MASK	0x20000000
#define SHIFT_BITS	7
#else	/* SVR4 */
#define WC_MASK		0x00008080
#define CS1_WC_MASK	0x00008080
#define CS2_WC_MASK	0x00000080
#define CS3_WC_MASK	0x00008000
#define SHIFT_BITS	8
#endif	/* SVR4 */

extern _XlcAddParseList();

Bool
_XimpBCSetAtr(lcd)
    XimpLCd lcd;
{
    Locale locale = &lcd->locale;
    CodeSet codeset;
    char tmp_str[2];
    int cs_num;

    cs_num = locale->codeset_num;
    if (cs_num > 4)
	locale->codeset_num = cs_num = 4;

    locale->mb_cur_max = 1;
    locale->wc_encode_mask = WC_MASK;
    locale->wc_shift_bits = SHIFT_BITS;

    switch (--cs_num) {
	case 3:
	    codeset = locale->codeset_list[3];
	    locale->mb_cur_max = max(codeset->length, locale->mb_cur_max);
	    tmp_str[0] = SS3;
	    tmp_str[1] = '\0';
	    if (_XlcAddParseList(locale, E_SS, tmp_str, codeset) == False)
		return False;
	    codeset->wc_encoding = CS3_WC_MASK;
	case 2:
	    codeset = locale->codeset_list[2];
	    locale->mb_cur_max = max(codeset->length, locale->mb_cur_max);
	    tmp_str[0] = SS2;
	    tmp_str[1] = '\0';
	    if (_XlcAddParseList(locale, E_SS, tmp_str, codeset) == False)
		return False;
	    codeset->wc_encoding = CS2_WC_MASK;
	case 1:
	    codeset = locale->codeset_list[1];
	    locale->mb_cur_max = max(codeset->length, locale->mb_cur_max);
	    codeset->wc_encoding = CS1_WC_MASK;
	    locale->initial_state_GR = codeset;
	    break;
    }

    locale->initial_state_GL = locale->codeset_list[0];

    return True;
}
#endif	/* XIMP_BC */

XLCd
_XlcEUCLoader(name)
    char *name;
{
    XimpLCd lcd;

    lcd = _XlcCreateLC(name, &lcd_methods, &lc_methods);
    if (lcd == NULL)
	return (XLCd) NULL;

    if (strcmp(lcd->locale.codeset, "EUC"))
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
