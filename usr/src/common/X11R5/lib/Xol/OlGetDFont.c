#ifndef NOIDENT
#ident	"@(#)olmisc:OlGetDFont.c	1.6"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

/*
 * _OlGetDefaultFont -
 *
 * This routine is used to get the default font depending on the
 * GUI switch...
 * 
 * The def_font value can be one of the following:
 *	OlDefaultFont
 *	OlDefaultBoldFont
 *	OlDefaultFixedFont
 *	OlDefaultItalicFont
 *	OlDefaultBoldItalicFont
 *	OlDefaultNoticeFont
 *	XtDefaultFont
 *  	or NULL - which gives OlDefaultFont
 */
extern XFontStruct *
_OlGetDefaultFont OLARGLIST((w, def_font))
	OLARG( Widget,	w)
	OLGRA( String,	def_font)
{
	XrmValue	from, to;

	/* Call the string to font converter to get the font */
	if (def_font)
		from.addr = def_font;
	else
		from.addr = OlDefaultFont;
	from.size = strlen(from.addr);
	to.addr = NULL;

	if (XtConvertAndStore(
		w, XtRString, &from, XtRFontStruct, &to) == False)
	{
		OlVaDisplayErrorMsg(
			XtDisplayOfObject(w),
			OleNbadFont,
			OleTdefaultOLFont,
			OleCOlToolkitError,
			OleMbadFont_defaultOLFont,
			XtName(w),
			OlWidgetToClassName(w)
		);
		return((XFontStruct *) NULL);
	}
    return (*(XFontStruct **)to.addr);
} /* end of _OlGetDefaultFont */
