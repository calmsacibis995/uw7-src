/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)oldtext:Display.c	1.30"
#endif

/*
 *************************************************************************
 * 
 * 
 ****************************procedure*header*****************************
 */
/* () {}
 *	The Above template should be located at the top of each file
 * to be easily accessable by the file programmer.  If this is included
 * at the top of the file, this comment should follow since formatting
 * and editing shell scripts look for special delimiters.		*/

/*
 *************************************************************************
 *
 * Date:	December 20, 1988
 *
 * Description: This file contains the source code for the display of
 *	the TextPane widget.
 *		
 *
 *******************************file*header*******************************
 */

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   Project:     X Widgets
 **
 **   Description: Code for TextPane widget ascii sink
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **
 *****************************************************************************
 *************************************<+>*************************************/

						/* #includes go here	*/

#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/TextPaneP.h>

#include <stdio.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/
static int AsciiClearToBackground();
static int AsciiDisplayText();
static AsciiFindDistance();
static AsciiFindPosition();
static int AsciiInsertCursor();
static int AsciiMaxHeightForLines();
static int AsciiMaxLinesForHeight();
static int AsciiResolveToPosition();
static TextFit AsciiTextFit();
static int CharWidth();
static Pixmap CreateInsertCursor();
static void SetCursor();

					/* class procedures		*/

					/* action procedures		*/

					/* public procedures		*/
static Boolean OlAsciiSinkCheckData();
OlTextSink * OlAsciiSinkCreate();
void OlAsciiSinkDestroy();
#if 0
void AsciiSinkInitialize();
#endif

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */


#define GETLASTPOS (*(source->scan))(source, 0, OlstLast, OlsdRight, 1, TRUE)
/* Private Ascii TextSink Definitions */

#define BUFFER_SIZE	512
#if 0
static unsigned bufferSize = 200;
#endif

typedef struct _AsciiSinkData {
	Pixel font_color;
	Pixel selection_color;
	Pixel input_focus_color;
	GC normgc, invgc, xorgc;
	XFontStruct *font;
	int tabwidth;
	Pixmap insertCursorOn;
	Pixmap insertCursorOff;
	OlInsertState laststate;
} AsciiSinkData, *AsciiSinkPtr;

#if 0
static unsigned char *buf;
#endif

#ifdef USE_LBEARING
#define LBEARING(x) \
    ((font->per_char != NULL && \
      ((x) >= font->min_char_or_byte2 && (x) <= font->max_char_or_byte2)) \
	? font->per_char[(x) - font->min_char_or_byte2].lbearing \
	: font->min_bounds.lbearing)
#else
#define LBEARING(x)	0
#endif

#   define insertCursor_width 6
#   define insertCursor_height 3
static char insertCursor_bits[] = {
	0x0c, 0x1e, 0x3F};

#   define read_onlyCursor_width 6
#   define read_onlyCursor_height 3
static char read_onlyCursor_bits[] = {
	0x0c, 0x12, 0x3F};

#   define inactiveCursor_width 8
#   define inactiveCursor_height 5
static char inactiveCursor_bits[] = {
	0x10, 0x28, 0x54, 0x28, 0x10};

#if 0
static Boolean initialized = FALSE;
static XContext asciiSinkContext;
#endif

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource SinkResources[] = {
	{XtNfont,
		XtCFont,
		XtRFontStruct,
		sizeof (XFontStruct *),
		XtOffset(AsciiSinkPtr, font),
		XtRString,
		NULL},
	{XtNfontColor,
		XtCTextFontColor,
		XtRPixel,
		sizeof (int),
		XtOffset(AsciiSinkPtr, font_color),
		XtRString,
		"Black"},
	{XtNselectionColor,
		XtCSelectionColor,
		XtRPixel,
		sizeof (int),
		XtOffset(AsciiSinkPtr, selection_color),
		XtRString,
		"Black"},
	{XtNinputFocusColor,
		XtCInputFocusColor,
		XtRPixel,
		sizeof (int),
		XtOffset(AsciiSinkPtr, input_focus_color),
		XtRString,
		"Grey"},    
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */


/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */


/*
 *************************************************************************
 * 
 *  AsciiClearToBackground - Clear the passed region to the background
 *	color.
 * 
 ****************************procedure*header*****************************
 */
static int
AsciiClearToBackground (w, x, y, width, height)
	Widget w;
	Position x, y;
	Dimension width, height;
{
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;
	AsciiSinkData *data = (AsciiSinkData *) sink->data;
	XFillRectangle(XtDisplay(w), XtWindow(w), data->invgc, x, y, width, height);
}	/*  AsciiClearToBackground  */


void
_AsciiDisplayMark(w, y, mark)
	TextPaneWidget		w;
	Position		y;
	char *			mark;
{
	OlTextSink *		sink = w->text.sink;
	AsciiSinkData *		data = (AsciiSinkData *)sink->data;

	if (mark) {
		XFillRectangle(XtDisplay(w), XtWindow(w), data->invgc,
			0, y, w->text.leftmargin,
			(Dimension)(data->font->ascent + data->font->descent));
		XDrawImageString(XtDisplay(w), XtWindow(w), data->normgc,
			0, y + data->font->ascent, mark, strlen(mark));
	}
}

/*
 *************************************************************************
 * 
 *  AsciiDisplayText - 
 * 
 ****************************procedure*header*****************************
 */
static int
AsciiDisplayText (w, x, y, pos1, pos2, highlight)
	Widget w;
	Position x, y;
	int highlight;
	OlTextPosition pos1, pos2;
{
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;
	OlTextSource *source = ((TextPaneWidget)w)->text.source;
	AsciiSinkData *data = (AsciiSinkData *) sink->data ;
	int margin = ((TextPaneWidget)w)->text.leftmargin ;
	unsigned char *buf = (unsigned char *)XtMalloc(BUFFER_SIZE);
	unsigned bufferSize = BUFFER_SIZE;


	XFontStruct *font = data->font;
	int     j, k;
	Dimension width;
	OlTextBlock blk;
	GC gc = highlight ? data->invgc : data->normgc;
	GC invgc = highlight ? data->normgc : data->invgc;

	_AsciiDisplayMark(w, y, _OlMark(w, pos1));
	y += font->ascent;

	j = 0;
	while (pos1 < pos2) {
		pos1 = (*(source->read))(source, pos1, &blk, pos2 - pos1);
		for (k = 0; k < blk.length; k++) {
			while (j >= bufferSize - 5) {
				bufferSize += BUFFER_SIZE;
				buf = (unsigned char *) XtRealloc((char *)buf, bufferSize);
			}
			buf[j] = blk.ptr[k];
			if (buf[j] == LF)
				buf[j] = ' ';
			else if (buf[j] == '\t') {
				XDrawImageString(XtDisplay(w), XtWindow(w),
				    gc, x - LBEARING(*buf), y,
				     (OLconst char *)buf, j);
				buf[j] = 0;
				x += XTextWidth(data->font,
						(OLconst char *)buf , j);
				width = CharWidth(data, x, margin, '\t');
				XFillRectangle(XtDisplay(w), XtWindow(w), invgc, x,
				    y - font->ascent, width,
				    (Dimension) (data->font->ascent +
				    data->font->descent));
				x += width;
				j = -1;
			}
			else
				if (buf[j] < ' ') {
					buf[j + 1] = buf[j] + '@';
					buf[j] = '^';
					j++;
				}
			j++;
		}
	}
	XDrawImageString(XtDisplay(w), XtWindow(w), gc, x - LBEARING(*buf), y,
	    (OLconst char *)buf, j);
	XtFree((char *)buf);

}	/*  AsciiDisplayText  */


/*
 *************************************************************************
 * 
 *  AsciiFindDistance - Given two positions, find the distance between
 *	them.
 * 
 ****************************procedure*header*****************************
 */
static
AsciiFindDistance (w, fromPos, fromx, toPos, resWidth, resPos, resHeight)
	Widget w;
	OlTextPosition fromPos;	/* First position. */
	int fromx;		/* Horizontal location of first position. */
	OlTextPosition toPos;	/* Second position. */
	int *resWidth;		/* Distance between fromPos and resPos. */
	int *resPos;		/* Actual second position used. */
	int *resHeight;		/* Height required. */
{
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;
	OlTextSource *source = ((TextPaneWidget)w)->text.source;
	int margin = ((TextPaneWidget)w)->text.leftmargin ;

	AsciiSinkData *data;
	register    OlTextPosition idx, lastPos;
	register unsigned char   c;
	OlTextBlock blk;

	data = (AsciiSinkData *) sink->data;
	/* we may not need this */
	lastPos = GETLASTPOS;
	(*(source->read))(source, fromPos, &blk, toPos - fromPos);
	*resWidth = 0;
	for (idx = fromPos; idx != toPos && idx < lastPos; idx++) {
		if (idx - blk.firstPos >= blk.length)
			(*(source->read))(source, idx, &blk, toPos - fromPos);
		c = blk.ptr[idx - blk.firstPos];
		if (c == LF) {
			*resWidth += CharWidth(data, fromx + *resWidth, margin, SP);
			idx++;
			break;
		}
		*resWidth += CharWidth(data, fromx + *resWidth, margin, c);
	}
	*resPos = idx;
	*resHeight = data->font->ascent + data->font->descent;
}	/*  AsciiFindDistance  */


/*
 *************************************************************************
 * 
 *  AsciiFindPosition - 
 * 
 ****************************procedure*header*****************************
 */
static
AsciiFindPosition(w, fromPos, fromx, width, stopAtWordBreak, 
	resPos, resWidth, resHeight)
	Widget w;
	OlTextPosition fromPos; /* Starting position. */
	int fromx;		/* Horizontal location of starting position. */
	int width;		/* Desired width. */
	int stopAtWordBreak;	/* Whether the resulting position should be at
				   	a word break. */
	OlTextPosition *resPos;	/* Resulting position. */
	int *resWidth;		/* Actual width used. */
	int *resHeight;		/* Height required. */
{
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;
	OlTextSource *source = ((TextPaneWidget)w)->text.source;
	int margin = ((TextPaneWidget)w)->text.leftmargin ;
	AsciiSinkData *data;
	OlTextPosition lastPos, idx, whiteSpacePosition;
	int     lastWidth, whiteSpaceWidth;
	Boolean whiteSpaceSeen;
	unsigned char c;
	OlTextBlock blk;
	data = (AsciiSinkData *) sink->data;
	lastPos = GETLASTPOS;

	(*(source->read))(source, fromPos, &blk, BUFFER_SIZE);
	*resWidth = 0;
	whiteSpaceSeen = FALSE;
	c = 0;
	for (idx = fromPos; *resWidth <= width && idx < lastPos; idx++) {
		lastWidth = *resWidth;
		if (idx - blk.firstPos >= blk.length)
			(*(source->read))(source, idx, &blk, BUFFER_SIZE);
		c = blk.ptr[idx - blk.firstPos];
		if (c == LF) {
			*resWidth += CharWidth(data, fromx + *resWidth, margin, SP);
			idx++;
			break;
		}
		*resWidth += CharWidth(data, fromx + *resWidth, margin, c);
		if ((c == SP || c == TAB) && *resWidth <= width) {
			whiteSpaceSeen = TRUE;
			whiteSpacePosition = idx;
			whiteSpaceWidth = *resWidth;
		}
	}
	if (*resWidth > width && idx > fromPos) {
		*resWidth = lastWidth;
		idx--;
		if (stopAtWordBreak && whiteSpaceSeen) {
			idx = whiteSpacePosition + 1;
			*resWidth = whiteSpaceWidth;
		}
	}
	if (idx == lastPos && c != LF) idx = lastPos + 1;
	*resPos = idx;
	*resHeight = data->font->ascent + data->font->descent;
}	/*  AsciiFindPosition  */


/*
 *************************************************************************
 * 
 *  AsciiInsertCursor - The following procedure manages the "insert"
 *	cursor.
 * 
 ****************************procedure*header*****************************
 */
static int
AsciiInsertCursor(w, x, y, state)
	Widget w;
	Position x, y;
	OlInsertState state;
{
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;
	AsciiSinkData *data = (AsciiSinkData *) sink->data;

	if (state == OlisOn)  {
	      if (data->laststate == OlisOff)
		 XCopyArea (XtDisplay(w),
			    data->insertCursorOff, XtWindow(w), data->xorgc,
			    0, 0,
			    inactiveCursor_width, inactiveCursor_height,
			    x - (inactiveCursor_width >> 1),
			    y - (inactiveCursor_height));

	      XCopyArea (XtDisplay(w),
			 data->insertCursorOn, XtWindow(w), data->xorgc,
			 0, 0,
			 insertCursor_width, insertCursor_height,
			 x - (insertCursor_width >> 1),
			 y - (insertCursor_height));
        }
	else  {  /* the state is OFF  */
	      if (data->laststate == OlisOn)
		 XCopyArea (XtDisplay(w),
			    data->insertCursorOn, XtWindow(w), data->xorgc,
			    0, 0,
			    insertCursor_width, insertCursor_height,
			    x - (insertCursor_width >> 1),
			    y - (insertCursor_height));

	      XCopyArea (XtDisplay(w),
			 data->insertCursorOff, XtWindow(w), data->xorgc,
			 0, 0,
			 inactiveCursor_width, inactiveCursor_height,
			 x - (inactiveCursor_width >> 1),
			 y - (inactiveCursor_height));
	}

	data->laststate = state;
}	/*  AsciiInsertCursor  */


/*
 *************************************************************************
 * 
 *  AsciiMaxHieghtForLines - 
 * 
 ****************************procedure*header*****************************
 */
static int
AsciiMaxHeightForLines(w, lines)
	Widget w;
	int lines;
{
	AsciiSinkData *data;
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;

	data = (AsciiSinkData *) sink->data;
	return(lines * (data->font->ascent + data->font->descent));
}	/*  AsciiMaxHieghtForLines  */


/*
 *************************************************************************
 * 
 *  AsciiMaxLinesForHeight - 
 * 
 ****************************procedure*header*****************************
 */
static int
AsciiMaxLinesForHeight (w)
	Widget w;
{
	AsciiSinkData *data;
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;

	data = (AsciiSinkData *) sink->data;
	return (int) (((int)(w->core.height
	    - ((TextPaneWidget) w)->text.topmargin
	    - ((TextPaneWidget) w)->text.bottommargin
	    ))
	    / (data->font->ascent + data->font->descent)
	    );
}	/*  AsciiMaxLinesForHeight  */


/*
 *************************************************************************
 * 
 *  AsciiResolveToPosition - 
 * 
 ****************************procedure*header*****************************
 */
static int
AsciiResolveToPosition (w, pos, fromx, width, leftPos, rightPos)
	Widget w;
	OlTextPosition pos;
	int fromx,width;
	OlTextPosition *leftPos, *rightPos;
{
	int     resWidth, resHeight;
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;
	OlTextSource *source = ((TextPaneWidget)w)->text.source;

	AsciiFindPosition(w, pos, fromx, width, FALSE,
	    leftPos, &resWidth, &resHeight);
	if (*leftPos > GETLASTPOS)
		*leftPos = GETLASTPOS;
	*rightPos = *leftPos;
}	/*  AsciiResolveToPosition  */


/*
 *************************************************************************
 * 
 *  AsciiTextFit - 
 * 
 ****************************procedure*header*****************************
 */
static TextFit
AsciiTextFit (w, fromPos, fromx, width, wrap, wrapWhiteSpace,
	fitPos, drawPos, nextPos, resWidth, resHeight)
	Widget w;
	OlTextPosition fromPos; /* Starting position. */
	int fromx;		/* Horizontal location of starting position. */
	int width;		/* Desired width. */
	Boolean wrap;		/* Whether line should wrap at all */
	Boolean wrapWhiteSpace;	/* Whether line should wrap at white space */

	OlTextPosition *fitPos ;/* pos of last char which fits in specified
				   width */
	OlTextPosition *drawPos ;/* pos of last char to draw in specified
				   width based on wrap model */
	OlTextPosition *nextPos ;/* pos of next char to draw outside specified
				   width based on wrap model */
	int *resWidth;		/* Actual width used. */
	int *resHeight;		/* Height required. */
{
	OlTextSink *sink = ((TextPaneWidget)w)->text.sink;
	OlTextSource *source = ((TextPaneWidget)w)->text.source;
	int margin = ((TextPaneWidget)w)->text.leftmargin ;
	AsciiSinkData *data;
	OlTextPosition lastPos, pos, whiteSpacePosition;
	OlTextPosition fitL, drawL ;
	/* local equivalents of fitPos, drawPos, nextPos */
	int     lastWidth, whiteSpaceWidth ;
	Boolean whiteSpaceSeen ;
	Boolean useAll ;
	TextFit fit ;
	unsigned char    c;
	OlTextBlock blk;

	data = (AsciiSinkData *) sink->data;
	lastPos = GETLASTPOS;

	*resWidth = 0;
	*fitPos = fromPos ;
	*drawPos = -1 ;
	c = 0;
	useAll = whiteSpaceSeen = FALSE ;

	pos = fromPos ;
	fitL = pos - 1 ;
	(*(source->read))(source, fromPos, &blk, BUFFER_SIZE);

	while (*resWidth <= width)
	{	
		lastWidth = *resWidth;
		fitL = pos - 1 ;
		if (pos >= lastPos)
		{ 
			pos = lastPos ;
			fit = tfEndText ;
			useAll = TRUE ;
			break ;
		};
		if (pos - blk.firstPos >= blk.length)
			(*(source->read))(source, pos, &blk, BUFFER_SIZE);
		c = blk.ptr[pos - blk.firstPos];

		if (isNewline(c))
		{ 
			fit = tfNewline ;
			useAll = TRUE ;
			break ;
		}

		if (wrapWhiteSpace && isWhiteSpace(c))
		{ 
			whiteSpaceSeen = TRUE ;
			drawL = pos - 1 ;
			whiteSpaceWidth = *resWidth;
		};

		*resWidth += CharWidth(data, fromx + *resWidth, margin, c);
		pos++ ;
	} /* end while */

	*fitPos = fitL ;
	*drawPos = fitL ;
	if (useAll)
	{
		*nextPos = pos + 1 ;
		*resWidth = lastWidth ;
	}
	else if (wrapWhiteSpace && whiteSpaceSeen)
	{ 
		*drawPos = drawL ;
		*nextPos = drawL + 2 ;
		*resWidth = whiteSpaceWidth ;
		fit = tfWrapWhiteSpace ;
	}
	else if (wrap)
	{
		*nextPos = fitL + 1 ;
		*resWidth = lastWidth ;
		fit = tfWrapAny ;
	}
	else
	{
		/* scan source for newline or end */
		*nextPos =
		    (*(source->scan)) (source, pos, OlstEOL, OlsdRight, 1, TRUE) + 1 ;
		*resWidth = lastWidth ;
		fit = tfNoFit ;
	}
	*resHeight = data->font->ascent + data->font->descent;
	return (fit) ;
}	/*  AsciiTextFit  */


/*
 *************************************************************************
 * 
 *  CharWidth - 
 * 
 ****************************procedure*header*****************************
 */
static int
CharWidth (data, x, margin, c)
	AsciiSinkData *data;
	int x;
	int margin ;
	unsigned char c;
{
	int     width, nonPrinting;
	XFontStruct *font = data->font;

	if (c == '\t')
		/* This is totally bogus!! need to know tab settings etc.. */
		return data->tabwidth - ((x-margin) % data->tabwidth);
	if (c == LF)
		c = SP;
	nonPrinting = (c < SP);
	if (nonPrinting) c += '@';

	if (font->per_char &&
	    (c >= font->min_char_or_byte2 && c <= font->max_char_or_byte2))
		width = font->per_char[c - font->min_char_or_byte2].width;
	else
		width = font->min_bounds.width;

	if (nonPrinting)
		width += CharWidth(data, x, margin, '^');

	return width;
}	/*  CharWidth  */


/*
 *************************************************************************
 * 
 *  CreateInsertCursor - 
 * 
 ****************************procedure*header*****************************
 */
static Pixmap
CreateInsertCursor(s)
	Screen *s;
{

	return (XCreateBitmapFromData (DisplayOfScreen(s), RootWindowOfScreen(s),
	    insertCursor_bits, insertCursor_width, insertCursor_height));
}	/*  CreateInsertCursor  */


/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */


/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */


/*
 *************************************************************************
 * 
 *  OlAsciiSinkCheckData - 
 * 
 ****************************procedure*header*****************************
 */
static Boolean
OlAsciiSinkCheckData(self)
	OlTextSink *self ;
{
	TextPaneWidget tew = self->parent ;
	AsciiSinkData *data;
	unsigned long valuemask = (GCFont | GCGraphicsExposures |
	    GCForeground | GCBackground | GCFunction);
	XGCValues values;
	XFontStruct *font;

	/* make sure margins are big enough to keep traversal highlight
     from obscuring text or cursor.
     */
	{ 
		Dimension minBorder ;
		minBorder = (Dimension) RequiredCursorMargin;

		if (tew->text.topmargin < minBorder)
			tew->text.topmargin = minBorder ;
		if (tew->text.bottommargin < minBorder)
			tew->text.bottommargin = minBorder ;
		if (tew->text.rightmargin < minBorder)
			tew->text.rightmargin = minBorder ;
		if (tew->text.leftmargin < minBorder)
			tew->text.leftmargin = minBorder ;
	}

	if ((*(self->maxLines))(tew) < 1)

		OlVaDisplayWarningMsg(	XtDisplay(tew),
					OleNfileDisplay,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileDisplay_msg1);

	/*
	 *  update the gcs for possible color changes
	 */
	data = (AsciiSinkData *) self->data;
	font = data->font;
	values.function = GXcopy;
	values.font = font->fid;
	values.graphics_exposures = (Bool) FALSE;
	values.foreground = data->font_color;
	values.background = tew->core.background_pixel;
	XtReleaseGC((Widget)tew, data->normgc);
	data->normgc = XtGetGC((Widget)tew, valuemask, &values);

	values.foreground = tew->core.background_pixel;
	values.background = data->font_color;
	XtReleaseGC((Widget)tew, data->invgc);
	data->invgc = XtGetGC((Widget)tew, valuemask, &values);

	values.function = GXxor;
	values.foreground = data->input_focus_color ^
				tew->core.background_pixel;
	values.background = 0;
	XtReleaseGC((Widget)tew, data->xorgc);
	data->xorgc = XtGetGC((Widget)tew, valuemask, &values);

	SetCursor(self->parent, data);

}	/*  OlAsciiSinkCheckData  */


/*
 *************************************************************************
 * 
 *  OlAsciiSinkCreate - 
 * 
 ****************************procedure*header*****************************
 */
OlTextSink *
OlAsciiSinkCreate (w, args, num_args)
	Widget w;
	ArgList 	args;
	Cardinal 	num_args;
{
	TextPaneWidget tew = (TextPaneWidget) w;
	OlTextSink *sink;
	AsciiSinkData *data;
	unsigned long valuemask = (GCFont | GCGraphicsExposures |
	    GCForeground | GCBackground | GCFunction);
	XGCValues values;
	unsigned long wid;
	XFontStruct *font;

#if 0
	if (!initialized)
		AsciiSinkInitialize();
#endif

	sink                    = XtNew(OlTextSink);
	sink->parent            = (TextPaneWidget) w ;
	sink->parent->text.sink = sink ;
	sink->display           = AsciiDisplayText;
	sink->insertCursor      = AsciiInsertCursor;
	sink->clearToBackground = AsciiClearToBackground;
	sink->findPosition      = AsciiFindPosition;
	sink->textFitFn         = AsciiTextFit;
	sink->findDistance      = AsciiFindDistance;
	sink->resolve           = AsciiResolveToPosition;
	sink->maxLines          = AsciiMaxLinesForHeight;
	sink->maxHeight         = AsciiMaxHeightForLines;
	sink->resources         = SinkResources;
	sink->resource_num      = XtNumber(SinkResources);
	sink->check_data        = OlAsciiSinkCheckData;
	sink->destroy           = OlAsciiSinkDestroy;
	sink->LineLastWidth	= 0 ;
	sink->LineLastPosition  = 0 ;
	data                    = XtNew(AsciiSinkData);
	sink->data              = (int *)data;
	data->insertCursorOn = (Pixmap)0;
	data->insertCursorOff = (Pixmap)0;
	data->laststate = OlisOff;

	XtGetSubresources (w, (XtPointer)data, "textPane", "TextPane", 
	    SinkResources, XtNumber(SinkResources),
	    args, num_args);

	if (data->font == NULL)  {
		data->font = (XFontStruct *) _OlGetDefaultFont(w, NULL);
	}
	font = data->font;
	values.function = GXcopy;
	values.font = font->fid;
	values.graphics_exposures = (Bool) FALSE;
	values.foreground = data->font_color;
	values.background = tew->core.background_pixel;
	data->normgc = XtGetGC(w, valuemask, &values);

	values.foreground = tew->core.background_pixel;
	values.background = data->font_color;
	data->invgc = XtGetGC(w, valuemask, &values);

	values.function = GXxor;
	values.foreground = data->input_focus_color ^
				tew->core.background_pixel;
	values.background = 0;
	data->xorgc = XtGetGC(w, valuemask, &values);

#define	ul_NULL ((unsigned long)(~0))
	wid = ul_NULL;
	if ((!XGetFontProperty(font, XA_QUAD_WIDTH, &wid)) || wid == ul_NULL) {
		if (font->per_char && font->min_char_or_byte2 <= '0' &&
		    font->max_char_or_byte2 >= '0')
			wid = font->per_char['0' - font->min_char_or_byte2].width;
		else
			wid = font->max_bounds.width;
	}
	if (wid == ul_NULL) wid = 1;
#undef	ul_NULL
	data->tabwidth = 8 * wid;
	data->font = font;

	/*    data->insertCursorOn = CreateInsertCursor(XtScreen(w)); */

	data->laststate = OlisOff;
	(*(sink->check_data))(sink);

	return sink;
}	/*  OlAsciiSinkCreate  */



static void
SetCursor(w, data)
  	Widget w;
	AsciiSinkData	*data;
{
	Screen *screen = XtScreen(w);
	Display *dpy = XtDisplay(w);
	Window root = RootWindowOfScreen(screen);
	XGCValues gcv;
	GC gc;
	Pixmap pixmap;
	static Pixmap on_bitmap = (Pixmap)0;
	static Pixmap off_bitmap = (Pixmap)0;

	if (on_bitmap == (Pixmap)0)
		on_bitmap = XCreateBitmapFromData(dpy,
					root,
					insertCursor_bits,
					insertCursor_width,
					insertCursor_height);

	if (off_bitmap == (Pixmap)0)
		off_bitmap = XCreateBitmapFromData(dpy,
					root,
					inactiveCursor_bits,
				 	inactiveCursor_width,
				    	inactiveCursor_height);

	gcv.function = GXcopy;
	gcv.foreground = data->input_focus_color ^
				w->core.background_pixel;
	gcv.background = 0;
	gcv.graphics_exposures = False;
	gc = XtGetGC(w, (GCFunction | GCForeground | GCBackground |
		    GCGraphicsExposures), &gcv);


	if (data->insertCursorOn == (Pixmap)0) {
		pixmap = XCreatePixmap(dpy,
				root,
				insertCursor_width,
				insertCursor_height,
				DefaultDepthOfScreen(screen));
		data->insertCursorOn = pixmap;
	}
	else
		pixmap = data->insertCursorOn;

	XCopyPlane(dpy,
		on_bitmap,
		pixmap,
		gc,
		0,
		0,
		insertCursor_width,
		insertCursor_height,
		0,
		0,
		1);

	/*
	 *  Create the inactive cursor pixmap in the same way
	 */
	if (data->insertCursorOff == (Pixmap)0) {
		pixmap = XCreatePixmap(dpy,
			    root,
			    inactiveCursor_width,
			    inactiveCursor_height,
			    DefaultDepthOfScreen(screen));
		data->insertCursorOff = pixmap;
        }
	else
		pixmap = data->insertCursorOff;

	XCopyPlane(dpy,
		    off_bitmap,
		    pixmap,
		    gc,
		    0,
		    0,
		    inactiveCursor_width,
		    inactiveCursor_height,
		    0,
		    0,
		    1);

	XtReleaseGC(w, gc);
}


/*
 *************************************************************************
 * 
 *  OlAsciiSinkDestroy - 
 * 
 ****************************procedure*header*****************************
 */
void
OlAsciiSinkDestroy (sink)
	OlTextSink *sink;
{
	AsciiSinkData *data;
	data = (AsciiSinkData *) sink->data;
	XtFree((char *) data);
	XtFree((char *) sink);
}	/*  OlAsciiSinkDestroy  */


#if 0
/*
 *************************************************************************
 * 
 *  AsciiSinkInitialize - 
 * 
 ****************************procedure*header*****************************
 */
void
AsciiSinkInitialize()
{
	if (initialized)
		return;
	initialized = TRUE;

	asciiSinkContext = XUniqueContext();
}	/*  AsciiSinkInitialize  */
#endif
