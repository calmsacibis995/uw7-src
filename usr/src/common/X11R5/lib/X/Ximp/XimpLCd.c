#pragma ident	"@(#)R5Xlib:Ximp/XimpLCd.c	1.4"

/* $XConsortium: XimpLCd.c,v 1.5 92/04/22 11:53:17 rws Exp $ */
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

    Copyright 1991, 1992 by FUJITSU LIMITED.
    Copyright 1991, 1992 by Sun Microsystems, Inc.

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of FUJITSU LIMITED or Sun
Microsystems, Inc.  not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.  FUJITSU LIMITED and Sun Microsystems, Inc. make no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.

FUJITSU LIMITED AND SUN MICROSYSTEMS, INC. DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJITSU LIMITED AND SUN
MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

Author: Takashi Fujiwara     FUJITSU LIMITED
        Hideki Hiura         Sun Microsystems, Inc.

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
#pragma weak _XrmInitParseInfo = __XrmInitParseInfo
#pragma weak _XlcDefaultLoader = __XlcDefaultLoader
#endif

static CharSetRec default_charset[] =
{
    { "ISO8859-1", GL, 1, "\033(B", 94, True },
    { "ISO8859-1", GR, 1, "\033-A", 96, False },
    { "ISO8859-2", GR, 1, "\033-B", 96, False },
    { "ISO8859-3", GR, 1, "\033-C", 96, False },
    { "ISO8859-4", GR, 1, "\033-D", 96, False },
    { "ISO8859-7", GR, 1, "\033-F", 96, False },
    { "ISO8859-6", GR, 1, "\033-G", 96, False },
    { "ISO8859-8", GR, 1, "\033-H", 96, False },
    { "ISO8859-5", GR, 1, "\033-L", 96, False },
    { "ISO8859-9", GR, 1, "\033-M", 96, False },
    { "JISX0201.1976-0", GL, 1, "\033(J", 94, True },
    { "JISX0201.1976-0", GR, 1, "\033)I", 94, False },

    { "GB2312.1980-0", GL, 2, "\033$(A", 94, False },
    { "GB2312.1980-0", GR, 2, "\033$)A", 94, False },
    { "JISX0208.1983-0", GL, 2, "\033$(B", 94, False },
    { "JISX0208.1983-0", GR, 2, "\033$)B", 94, False },
    { "KSC5601.1987-0", GL, 2, "\033$(C", 94, False },
    { "KSC5601.1987-0", GR, 2, "\033$)C", 94, False },

    /* Non-Standard Character Set Encodings */
    { "CNS11643-0", GL, 1, "\033(D", 94, True },
    { "CNS11643-0", GR, 1, "\033)D", 94, False },
    { "CNS11643-1", GR, 2, "\033$)G", 94, False },
    { "CNS11643-2", GR, 2, "\033$)H", 94, False },
    { "CNS11643-14", GR, 2, "\033$)I", 94, False },
    { "CNS11643-15", GR, 2, "\033$)J", 94, False },
    { "CNS11643-16", GR, 2, "\033$)K", 94, False },
} ; 

static CharSet *charset_list = NULL;
#ifdef DYNAMICLIB
static XLCdMethodsRec lcd_methods ;
#endif

Bool
_XlcRegisterCharSet(charset)
    CharSet charset;
{
    CharSet *new;
    static num;

    if (charset_list == NULL) {
	charset_list = (CharSet *) Xmalloc (2 * sizeof(CharSet));
	if (charset_list == NULL)
	    return False;

	charset_list[0] = charset;
	charset_list[1] = (CharSet) NULL;
	num = 1;
    } else {
	new = (CharSet *) Xrealloc(charset_list, sizeof(CharSet) * (num + 2));
	if (new == NULL)
	    return False;

	charset_list = new;
	charset_list[num] = charset;
	charset_list[num + 1] = (CharSet) NULL;
	num++;
    }

    return True;
}

CharSet
_XlcGetCharSetFromEncoding(encoding)
    char *encoding;
{
    CharSet *charset = charset_list;

    for ( ; *charset; charset++) {
	if (!strcmp((*charset)->encoding, encoding))
	    return *charset;
    }

    return (CharSet) NULL;
}

CharSet
_XlcGetCharSetFromName(name, side)
    char *name;
    unsigned side;
{
    CharSet *charset = charset_list;

    for ( ; *charset; charset++) {
	if (!strcmp((*charset)->name, name) && (*charset)->side == side)
	    return *charset;
    }

    return (CharSet) NULL;
}

CodeSet
_XlcGetCodeSetFromCharSet(lcd, charset)
    XimpLCd lcd;
    CharSet charset;
{
    CodeSet *codeset = lcd->locale.codeset_list;
    CharSet *charset_list;
    int charset_num, codeset_num = lcd->locale.codeset_num;

    for ( ; codeset_num-- > 0; codeset++) {
	charset_list = (*codeset)->charset_list;
	charset_num = (*codeset)->charset_num;
	for ( ; charset_num-- > 0; charset_list++)
	    if (*charset_list == charset)
		return *codeset;
    }

    return (CodeSet) NULL;
}


extern XLCd _XlcCLoader(), _XlcGenericLoader();
#ifdef XIMP_BC
extern XLCd _XlcEUCLoader();
#endif
#ifdef USE_SJIS
extern XLCd _XlcSJISLoader();
#endif

static XLCdLoadProc default_loader[] = {
    _XlcCLoader,
#ifdef XIMP_BC
    _XlcEUCLoader,
#endif
#ifdef USE_SJIS
    _XlcSJISLoader,
#endif
    _XlcGenericLoader,
};

static XLCdLoadProc *loader_list = NULL;

Bool
_XlcInsertLoader(proc)
    XLCdLoadProc proc;
{
    XLCdLoadProc *new;
    static num;

    if (loader_list == NULL) {
	loader_list = (XLCdLoadProc *) Xmalloc (2 * sizeof(XLCdLoadProc));
	if (loader_list == NULL)
	    return False;

	loader_list[0] = proc;
	loader_list[1] = (XLCdLoadProc) NULL;
	num = 1;
    } else {
	new = (XLCdLoadProc *) Xrealloc(loader_list, sizeof(XLCdLoadProc)
					* (num + 2));
	if (new == NULL)
	    return False;
	
	loader_list = new;
	loader_list[num] = proc;
	loader_list[num + 1] = (XLCdLoadProc) NULL;
	num++;
    }
    return True;
}

XLCd
#ifdef ARCHIVE
_XlcDefaultLoader(name)
#elif !defined( DYNAMICLIB ) && !defined( ARCHIVE )
__XlcDefaultLoader(name)
#else
_XlcInitLocale(name)
#endif
    char *name;
{
    XLCd lcd;
    CharSet charset;
    XLCdLoadProc *proc;
    int num;
#if !defined(X_NOT_STDC_ENV) && !defined(X_LOCALE)
#if 0 /* USLP: only defined for hpux */
    char siname[256];
    char *_XlcMapOSLocaleName();

    name = _XlcMapOSLocaleName(name, siname);
#endif
#endif

    if (charset_list == NULL) {
	num = sizeof(default_charset) / sizeof(CharSetRec);
	for (charset = default_charset; num-- > 0; charset++)
	    if (_XlcRegisterCharSet(charset) == False)
		return (XLCd) NULL;
    }

    if (loader_list == NULL) {
	num = sizeof(default_loader) / sizeof(XLCdLoadProc);
	for (proc = default_loader; num-- > 0; proc++)
	    if (_XlcInsertLoader(*proc) == False)
		return (XLCd) NULL;
    }

    for (proc = loader_list; *proc; proc++)
	if (lcd = (*proc)(name))
	    break;
#ifdef DYNAMICLIB
    lcd->methods = &lcd_methods ;
#endif
    return lcd;
}



static void
mbinit(state)
    XPointer state;
{
    LCMethods methods = LC_METHODS(((State)state)->lcd);

    (*methods->cnv_start)(state);
}

static char
mbchar(state, str, lenp)
    XPointer state;
    char *str;
    int *lenp;
{
    LCMethods methods = LC_METHODS(((State)state)->lcd);

    return (*methods->mbchar)(state, str, lenp);
}

static void
mbfinish(state)
    XPointer state;
{
    LCMethods methods = LC_METHODS(((State)state)->lcd);

    (*methods->cnv_end)(state);
}

static char *
lcname(state)
    XPointer state;
{
    return ((State)state)->lcd->core.name;
}

static void
destroy(state)
    XPointer state;
{
    LCMethods methods = LC_METHODS(((State)state)->lcd);

    (*methods->destroy_state)(state);
}

static XrmMethodsRec rm_methods = {
    mbinit,
    mbchar,
    mbfinish,
    lcname,
    destroy
} ;

XrmMethods
#ifdef  ARCHIVE
_XrmInitParseInfo(statep)
XPointer *statep;
#elif  !defined(DYNAMICLIB) && !defined(ARCHIVE)
__XrmInitParseInfo(statep)
XPointer *statep;
#else
_XimprmInitParseInfo(lcd, state)
State *state;
XLCd lcd ;
#endif
{
#ifndef DYNAMICLIB
    State	*state = (State *)statep;
    XLCd	lcd =  _XlcCurrentLC();
#endif
    LCMethods methods;
    
    if (lcd == NULL)
	return NULL;
    
    methods = LC_METHODS(lcd);
    *state = (methods->create_state)(lcd);
    if (*state == NULL)
	return NULL;

    return &rm_methods;
}

#ifdef DYNAMICLIB
extern int           _XimpmbTextPropertyToTextList();
extern int           _XimpwcTextPropertyToTextList();
extern int           _XimpmbTextListToTextProperty();
extern int           _XimpwcTextListToTextProperty();
extern XrmMethods    _XimprmInitParseInfo();
extern void          _XimpwcFreeStringList();
extern char         *_XimpDefaultString();

static XLCdMethodsRec lcd_methods = {
       _XlcDefaultMapModifiers,
       _XDefaultCreateFontSet,
       _Ximp_OpenIM,
       _XimprmInitParseInfo,
       _XimpmbTextPropertyToTextList,
       _XimpwcTextPropertyToTextList,
       _XimpmbTextListToTextProperty,
       _XimpwcTextListToTextProperty,
       _XimpwcFreeStringList,
       _XimpDefaultString
};
#endif
