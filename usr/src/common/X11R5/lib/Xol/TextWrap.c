/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)textedit:TextWrap.c	1.25"
#endif

/*
 * TextWrap.c
 *
 */

#include <Xol/buffutil.h>

#include <X11/memutil.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include <Xol/textbuff.h>		/* must follow IntrinsicP.h */


#include <Xol/Dynamic.h>
#include <Xol/Scrollbar.h>
#include <Xol/TextEdit.h>
#include <Xol/TextEditP.h>
#include <Xol/TextUtil.h>           /* for _mbCopyOfwcSegment */
#include <Xol/TextWrap.h>
#include <Xol/Util.h>

#include <ctype.h>
#ifdef I18N

#if defined(SVR4_0) || defined(SVR4)    /* do we care SVR3?? */

#include <widec.h>
#include <wctype.h>	/* For post-6.0 ccs, include <wchar.h> instead */

#else /* defined(SVR4_0) || defined(SVR4) */

        /* Let Xlib.h take care this if XlibSpecificationRelease
         * is defined (note that, this symbol is introduced in
         * X11R5), otherwise do similar check as in X11R5:Xlib.h
         */
#if !defined(XlibSpecificationRelease)

#if defined(X_WCHAR) || defined(sun)
#include <stddef.h>
#endif

#endif /* !defined(XlibSpecificationRelease) */
#endif /* defined(SVR4_0) || defined(SVR4) */

static int _CodesetOfChar OL_ARGS((int, int *));

#endif /* I18N */



/*
 * _FirstWrapLine<
 *
 * The \fI_FirstWrapLine\fR function returns the first valid wrap location
 * for the given \fIwrapTable\fR.
 *
 * See also:
 *
 * _LastWrapLine(3), _LastDisplayedWrapLine(3)
 *
 * Synopsis:
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_FirstWrapLine OLARGLIST((wrapTable))
    OLGRA(WrapTable *, wrapTable)
{
static TextLocation location;

return (location);

} /* end of _FirstWrapLine */
/*
 * _LastDisplayedWrapLine
 *
 * The \fI_LastDisplayedWrapLine\fR function returns the wrap location
 * corresponding to the first displayed line on the last pane of display
 * given the pane size \fIpage\fR for \fIwrapTable\fR.  
 *
 * See also:
 *
 * _FirstWrapLine(3), _LastWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_LastDisplayedWrapLine OLARGLIST((wrapTable, page))
    OLARG(WrapTable *, wrapTable)
    OLGRA(int, page)
{

return
   (_IncrementWrapLocation(wrapTable, _LastWrapLine(wrapTable), 
                           -(page - 1), _FirstWrapLine(wrapTable), NULL));

} /* end of _LastDisplayedWrapLine */
/*
 * _LastWrapLine
 *
 *
 * The \fI_LastWrapLine\fR function returns the last valid wrap location
 * in \fIwrapTable\fR
 *
 * See also:
 * 
 * _FirstWrapLine(3), _LastDisplayedWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_LastWrapLine OLARGLIST((wrapTable))
    OLGRA(WrapTable *, wrapTable)
{
TextLocation location;

location.line   = wrapTable-> used - 1;
location.offset = wrapTable-> p[location.line]-> used - 1;

return (location);

} /* end of _LastWrapLine */
/*
 * _LineNumberOfWrapLocation
 *
 * The \fI_LineNumberOfWrapLocation\fR function is used to calculate
 * the number of display lines between the first wrap location and
 * the wrap \fIlocation\fR in \fIwrapTable\fR.
 *
 * See also:
 *
 * _NumberOfWrapLines(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern int
_LineNumberOfWrapLocation OLARGLIST((wrapTable, location))
    OLARG(WrapTable *, wrapTable)
    OLGRA(TextLocation, location)
{
register int i;
register int cnt = location.offset;

for (i = 0; i < wrapTable-> used && i < location.line; i++)
   cnt += wrapTable->p[i]-> used;

return (cnt);

} /* end of _LineNumberOfWrapLocation */
/*
 * _NumberOfWrapLines
 *
 * The \fI_NumberOfWrapLines\fR function is used to calculate the number
 * of display lines in \fIwrapTable\fR.
 *
 * See also:
 *
 * _LineNumberOfWrapLocation(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern int
_NumberOfWrapLines OLARGLIST((wrapTable))
    OLGRA(WrapTable *, wrapTable)
{
register int i;
register int cnt = 0;

for (i = 0; i < wrapTable-> used; i++)
   cnt += wrapTable->p[i]-> used;

return (cnt);

} /* end of _NumberOfWrapLines */
/*
 * _UpdateWrapTable
 *
 * The \fI_UpdateWrapTable\fR procedure is used to update the wrap table
 * associated with the TextEdit widget \fIctx\fR for the real lines
 * between \fIstart\fR and \fIend\fR inclusive.
 *
 * See also:
 *
 * _BuildWrapTable(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern void
_UpdateWrapTable OLARGLIST((ctx, start, end))
    OLARG(TextEditWidget, ctx)
    OLARG(TextLine,       start)
    OLGRA(TextLine,       end)
{
TextEditPart *  text       = &ctx-> textedit;
WrapTable *     wrapTable  = text-> wrapTable;
TextBuffer *    textBuffer = text-> textBuffer;
XFontStruct *   fs         = ctx-> primitive.font;
OlFontList *    fl         = ctx-> primitive.font_list;
TabTable        tabs       = text-> tabs;
OlWrapMode      wrapMode   = text-> wrapMode;
TextLine        i;
TextPosition    j = (TextPosition) 0;
int             x          = HGAP(ctx) + PAGE_L_MARGIN(ctx);
int             maxx       = (wrapMode == OL_WRAP_OFF) ? 
                  (LinesInTextBuffer(textBuffer) == 1 ? 0 : text-> maxX) : 
                  PAGE_R_MARGIN(ctx);
BufferElement * p;
TextPosition    pos;
TextPosition    nextpos;

for (i = start; i <= end; i++)
   {
   wrapTable-> p[i] = (WrapLocation *) 
      AllocateBuffer(sizeof(wrapTable-> p[i]-> p[j]), 1);

   p = wcGetTextBufferLocation(textBuffer, i, (TextLocation *)NULL);


   if (wrapMode == OL_WRAP_OFF)
      {
      if (0 == wrapTable-> p[i]-> size)
         GrowBuffer((Buffer *)(wrapTable-> p[i]), 1);
      wrapTable-> p[i]-> p[0] = 0;
      wrapTable-> p[i]-> used = 1;
      x = _StringWidth(ctx, PAGE_L_MARGIN(ctx), p, 0, _StringLength(p), fl, fs, tabs);
      maxx = MAX(x, maxx);
      }
   else
      {
      for (j = 0, pos = 0; pos != EOF; pos = nextpos, j++)
         {
         if (j == wrapTable-> p[i]-> size)
            GrowBuffer((Buffer *)(wrapTable-> p[i]), 1);

         nextpos = _WrapLine(ctx, x, maxx, p, pos, fl, fs, tabs, wrapMode);
         wrapTable-> p[i]-> p[j] = pos;
         if (nextpos == pos)
            nextpos++;
         }
      wrapTable-> p[i]-> used = j;
      }
   }

text-> maxX = maxx;
text-> lineCount = _NumberOfWrapLines(wrapTable);


} /* end of _UpdateWrapTable */
/*
 * _BuildWrapTable
 *
 * The \fI_BuildWrapTable\fR procedure is used to totally construct the
 * wrap table for the TextEdit widget \fIctx\fR.
 *
 * See also:
 *
 * _UpdateWrapTable(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern void
_BuildWrapTable OLARGLIST((ctx))
    OLGRA(TextEditWidget, ctx)
{
TextEditPart * text       = &ctx-> textedit;
WrapTable *    wrapTable  = text-> wrapTable;
TextBuffer *   textBuffer = text-> textBuffer;
TextLine       i;
int            space_needed;

if (text-> prev_width  == ctx-> core.width &&
    text-> prev_height == ctx-> core.height)
   return; /* nothing to do!!! */

if (wrapTable != NULL)
   {
   for (i = 0; i < wrapTable-> used; i++)
      FreeBuffer((Buffer *)(wrapTable-> p[i]));

   space_needed = LinesInTextBuffer(textBuffer) + 1 - wrapTable-> size;
   if (space_needed != 0)
      GrowBuffer((Buffer *)wrapTable, space_needed);
   }
else
   {
   wrapTable = text-> wrapTable = 
      (WrapTable *) AllocateBuffer(sizeof(text-> wrapTable-> p[0]), 
      LinesInTextBuffer(textBuffer) + 1);
   }

i = wrapTable-> used = LinesInTextBuffer(textBuffer);

text-> maxX = 0;

_UpdateWrapTable(ctx, 0, i - 1);

wrapTable-> p[i] = (WrapLocation *) 
   AllocateBuffer(sizeof(wrapTable-> p[i]-> p[0]), 1);
wrapTable-> p[i]-> used = 0;

text-> linesVisible = PAGE_LINE_HT(ctx);

} /* end of _BuildWrapTable */
/*
 * _GetNextWrappedLine
 *
 * The \fI_GetNextWrappedLine\fR function is used to retrieve the string
 * stored in the TextBuffer \fItextBuffer\fR at the \fIlocation\fR of
 * the \fIwrapTable\fR.  The function returns the pointer to the string
 * and increments the wrap location for use in subsequent calls to
 * retrieve the next wrap line.
 *
 * Synopsis:
 * 
 *#include <TextWrap.h>
 * ...
 */

extern BufferElement *
_GetNextWrappedLine OLARGLIST((textBuffer, wrapTable, location))
    OLARG(TextBuffer *, textBuffer)
    OLARG(WrapTable *, wrapTable)
    OLGRA(TextLocation *, location)
{
BufferElement * p = wcGetTextBufferLocation(textBuffer, location-> line, NULL);

if (p != (BufferElement) NULL)
   {
   p = &p[wrapTable-> p[location-> line]-> p[location-> offset]];
   if (++location-> offset == wrapTable-> p[location-> line]-> used)
      {
      location-> line++;
      location-> offset = 0;
      }
   }

return (p);

} /* end of GetNextWrappedLine */
/*
 * _IncrementWrapLocation
 *
 * The \fI_IncrementWrapLocation\fR function is used to calculate the
 * wrap location in the \fIwrapTable\fR which is offset \fIn\fR lines
 * from the \fIcurrent\fR wrap location.  This calculation is governed
 * by the \fIlimit\fR location.  The routine reports the actual number
 * of wrap lines between \fIcurrent\fR and the return location in
 * the \fIreallines\fR return value.
 *
 * See also:
 *
 * _MoveToWrapLocation(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_IncrementWrapLocation OLARGLIST((wrapTable, current, n, limit, reallines))
    OLARG(WrapTable *,  wrapTable)
    OLARG(TextLocation, current)
    OLARG(int,          n)
    OLARG(TextLocation, limit)
    OLGRA(int *,        reallines)
{
register int i;

if (n < 0)
   {
   for (i = 0; !SameTextLocation(current, limit) && i > n; i--)
      if (--current.offset < 0)
         {
         current.line--;
         current.offset = wrapTable-> p[current.line]-> used - 1;
         }
   }
else
   if (n > 0)
      {
      for (i = 0; !SameTextLocation(current, limit) && i < n; i++)
         if (++current.offset > wrapTable-> p[current.line]-> used - 1)
            {
            current.line++;
            current.offset = 0;
            }
      }
   else
      i = 0;

if (reallines != NULL)
   *reallines = i;

return (current);

} /* end of _IncrementWrapLocation */
/*
 * _MoveToWrapLocation
 *
 * The \fI_MoveToWrapLocation\fR function is used to calculate the number
 * of display lines between \fIcurrent\fR and \fIlimit\fR in the
 * \fIwrapTable\fR.  This value is returned in the \fIreallines\fR
 * return parameter.
 *
 * See also:
 *
 * _IncrementWrapLocation(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
 
extern TextLocation
_MoveToWrapLocation OLARGLIST((wrapTable, current, limit, reallines))
    OLARG(WrapTable *,  wrapTable)
    OLARG(TextLocation, current)
    OLARG(TextLocation, limit)
    OLGRA(int *,        reallines)
{
register int i;

if (current.line == limit.line)
   if (current.offset == limit.offset)
      i = 0;
   else
      i = limit.offset - current.offset;
else
   if (current.line < limit.line)
      {
      for (i = 0; !SameTextLocation(current, limit); i++)
         if (++current.offset > wrapTable-> p[current.line]-> used - 1)
            {
            current.line++;
            current.offset = 0;
            }
      }
   else
      {
      for (i = 0; !SameTextLocation(current, limit); i--)
         if (--current.offset < 0)
            {
            current.line--;
            current.offset = wrapTable-> p[current.line]-> used - 1;
            }
      }

if (reallines != NULL)
   *reallines = i;

return (current);

} /* end of _MoveToWrapLocation */
/*
 * _PositionOfWrapLocation
 *
 * The \fI_PositionOfWrapLocation\fR function is used to convert a
 * \fIlocation\fR in \fIwrapTable\fR to a TextPosition in \fItextBuffer\fR.
 *
 * See also:
 *
 * _LocationOfWrapLocation(3), _WrapLocationOfPosition(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextPosition
_PositionOfWrapLocation OLARGLIST((textBuffer, wrapTable, location))
    OLARG(TextBuffer *, textBuffer)
    OLARG(WrapTable *,  wrapTable)
    OLGRA(TextLocation, location)
{

location = _LocationOfWrapLocation(wrapTable, location);

return (PositionOfLocation(textBuffer, location));

} /* end of _PositionOfWrapLocation */
/*
 * _LocationOfWrapLocation
 *
 * The \fI_LocationOfWrapLocation\fR function is used to convert a
 * \fIlocation\fR in \fIwrapTable\fR to a Location.
 *
 * See also:
 *
 * _PositionOfWrapLocation(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_LocationOfWrapLocation OLARGLIST((wrapTable, location))
    OLARG(WrapTable *,  wrapTable)
    OLGRA(TextLocation, location)
{

location.offset = wrapTable-> p[location.line]-> p[location.offset];

return(location);

} /* end of _LocationOfWrapLocation */
/*
 * _WrapLocationOfPosition
 *
 * The \fI_WrapLocationOfPosition\fR function is used to convert a
 * \fIposition\fR in \fItextBuffer\fR to a wrap location in \fIwrapTable\fR.
 *
 * See also:
 *
 * _LocationOfWrapLocation(3), _PositionOfWrapLocation(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_WrapLocationOfPosition OLARGLIST((textBuffer, wrapTable, position))
    OLARG(TextBuffer *, textBuffer)
    OLARG(WrapTable *,  wrapTable)
    OLGRA(TextPosition, position)
{

return
   (_WrapLocationOfLocation(wrapTable, LocationOfPosition(textBuffer, position)));

} /* end of _WrapLocationOfPosition */
/*
 * _WrapLocationOfLocation
 *
 * The \fI_WrapLocationOfLocation\fR function is used to convert a
 * \fIlocation\fR to a wrap location in \fIwrapTable\fR.
 *
 * See also:
 *
 * _LocationOfWrapLocation(3), _PositionOfWrapLocation(3), et al
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextLocation
_WrapLocationOfLocation OLARGLIST((wrapTable, location))
    OLARG(WrapTable *,  wrapTable)
    OLGRA(TextLocation, location)
{
register int i;

for (i = 0; i < wrapTable-> p[location.line]-> used; i++)
   if (wrapTable-> p[location.line]-> p[i] > location.offset)
      break;

location.offset = i - 1;

return (location);

} /* end of _WrapLocationOfLocation */
/*
 * _WrapLineOffset
 *
 * The \fI_WrapLineOffset\fR function decodes the TextPosition of a 
 * line in \fIwrapTable\fR at \fIlocation\fR.
 *
 * See also:
 *
 * _WrapLineLength(3), _WrapLineEnd(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 *
 */

extern TextPosition
_WrapLineOffset OLARGLIST((wrapTable, location))
    OLARG(WrapTable *,  wrapTable)
    OLGRA(TextLocation, location)
{

return (wrapTable-> p[location.line]-> p[location.offset]);

} /* end of _WrapLineOffset */
/*
 * _WrapLineLength
 *
 * The \fI_WrapLineLength\fR function calculates the length of a 
 * \fItextBuffer\fR line in \fIwrapTable\fR at \fIlocation\fR.
 *
 * See also:
 *
 * _WrapLineOffset(3), _WrapLineEnd(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextPosition
_WrapLineLength OLARGLIST((textBuffer, wrapTable, location))
    OLARG(TextBuffer *, textBuffer)
    OLARG(WrapTable *,  wrapTable)
    OLGRA(TextLocation, location)
{

return (1 + 
        _WrapLineEnd(textBuffer, wrapTable, location) - 
        _WrapLineOffset(wrapTable, location));

} /* end of _WrapLineLength */
/*
 * _WrapLineEnd
 *
 * The \fI_WrapLineEnd\fR function calculates the last TextPosition in a 
 * \fItextBuffer\fRline in \fIwrapTable\fR at \fIlocation\fR.
 *
 * See also:
 *
 * _WrapLineOffset(3), _WrapLineLength(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextPosition
_WrapLineEnd OLARGLIST((textBuffer, wrapTable, location))
    OLARG(TextBuffer *, textBuffer)
    OLARG(WrapTable *,  wrapTable)
    OLGRA(TextLocation, location)
{

return (((wrapTable-> p[location.line]-> used - 1) == location.offset) ?
     LastCharacterInTextBufferLine(textBuffer, location.line) :
     wrapTable-> p[location.line]-> p[location.offset + 1] - 1);

} /* end of _WrapLineEnd */
/*
 * _StringOffsetAndPosition
 *
 * The \fI_StringOffsetAndPosition\fR procedure is used to calculate the
 * character offset and x location ...
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern void
_StringOffsetAndPosition OLARGLIST((ctx, s, start, end, fl, fs, tabs, minx, retx, reti))
    OLARG(TextEditWidget,  ctx)
    OLARG(BufferElement, * s)
    OLARG(TextPosition,    start)
    OLARG(TextPosition,    end)
    OLARG(OlFontList *,    fl)
    OLARG(XFontStruct  *,  fs)
    OLARG(TabTable,        tabs)
    OLARG(int,             minx)
    OLARG(int *,           retx)
    OLGRA(TextPosition *,  reti)
{
int             x;
BufferElement * p;
BufferElement * q;
int             min      = fs-> min_char_or_byte2;
int             max      = fs-> max_char_or_byte2;
XCharStruct *   per_char = fs-> per_char;
int             maxwidth = fs-> max_bounds.width;
int             c;
int             index = 0;

for (x = 0, p = &s[start], q = &s[end]; x < minx && p < q; p++, index++)
   x += ((c = *p) == '\t') ? 
      _NextTabFrom(x, fl, tabs, maxwidth) : 
      _CharWidth(ctx, c, fl, fs, per_char, min, max, maxwidth);
   
*retx = x - minx;
*reti = start + index;

#ifdef DEBUG
fprintf(stderr, "Offset x is %d, minx is %d.  X - Minx is %d.\n", x,
	minx, x - minx);
#endif

} /* end of _StringOffsetAndPosition */
/*
 * _StringWidth
 *
 * The \fI_StringWidth\fR function calculates the length in pixels
 * of the string \fIs\fR between indeces \fIstart\fR end \fIend\fR using the
 * XFontstruct pointer \fIfs\fR and TabTable \fItabs\fR.
 *
 * See also:
 * 
 * _CharWidth(3), _NextTabFrom(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern int
_StringWidth OLARGLIST((ctx, x, s, start, end, fl, fs, tabs))
    OLARG(TextEditWidget,   ctx)
    OLARG(int,              x)
    OLARG(BufferElement *,  s)
    OLARG(TextPosition,     start)
    OLARG(TextPosition,     end)
    OLARG(OlFontList *,     fl)
    OLARG(XFontStruct *,    fs)
    OLGRA(TabTable,         tabs)
{
BufferElement *  p;
BufferElement *  q;
int              min      = fs-> min_char_or_byte2;
int              max      = fs-> max_char_or_byte2;
XCharStruct *    per_char = fs-> per_char;
int              maxwidth = fs-> max_bounds.width;
int              c;

for (p = &s[start], q = &s[end]; p <= q; p++)
   x += ((c = *p) == '\t') ? 
      _NextTabFrom(x, fl, tabs, maxwidth) : 
      _CharWidth(ctx, c, fl, fs, per_char, min, max, maxwidth);

return (x);

} /* end of _StringWidth */
/*
 * _CharWidth
 *
 * The \fI_CharWidth\fR function is used to calculate the width of the
 * character \fIc\fR.  If the \Iflist\fR parameter
 * is non-NULL, it is used to determine the character width.
 * Otherwise  the width is calculated using the XFontStruct information 
 * pointed to by \fIfs\fR, \fIper_char\fR, \fImin\fR, \fImax\fR, and 
 * \fImaxwidth\fR metrics.
 *
 * See also:
 *
 * _StringWidth(3), _NextTabFrom(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */
extern int
_CharWidth OLARGLIST((ctx, c, fl, fs, per_char, min, max,  maxwidth))
    OLARG(TextEditWidget, ctx)
    OLARG(int,           c)
    OLARG(OlFontList *,  fl)
    OLARG(XFontStruct *, fs)
    OLARG(XCharStruct *, per_char)
    OLARG(int,           min)
    OLARG(int,           max)
    OLGRA(int,           maxwidth)
{
    TextEditPart * text         = &ctx-> textedit;
    int            nonPrintable = 0;
    int            width        = 0;
    
#define NULL_IS_ZERO_WIDTH
    if (c == '\0') 
#ifdef NULL_IS_ZERO_WIDTH
	return (0);
#else
    c = ' ';
#endif

    nonPrintable = (0 < (unsigned)c && (unsigned)c <  040
		    || 0177 < (unsigned)c && (unsigned)c < 0240);
    
    if (fl){
	/*
	 *  The font list is non-null. Assume that  
	 *  the character may be non-ascii.
	 *  Convert the character to multibyte
	 *  format and call OlTextWidth to get width
	 *  of the multibyte character.
	 */
	char mb_copy[5];
	mb_copy[0] = mb_copy[1] = mb_copy[2] = mb_copy[3] = mb_copy[4] = (char) 0;
	
	if (text-> controlCaret && nonPrintable)
	{	    
	    /* ASCII control character --
	     * Transform to a printable character and insert a caret
	     */
	    mb_copy[0] = '^';
	    mb_copy[1] = '@' + c;
	    width = OlTextWidth(fl, (unsigned char *) mb_copy, 2);
	}
	else{
	    wctomb(mb_copy,c);
	    if (!*mb_copy)
		width = 0;
	    else
		width = OlTextWidth(fl, (unsigned char *) mb_copy, strlen(mb_copy)); 
	}
    }
    else {
	/*
	 *  Font list is null.  Assume that the character
	 *  is ASCII
	 */
	char copy[3];

	if (text-> controlCaret && nonPrintable){
	    copy[0] = '^';
	    copy[1] = '@' + c;
	    copy[2] = '\0';
	    width = XTextWidth(fs, copy, 2);
	}
	else{
	    copy[0] = c;
	    copy[1] = '\0';
	    width = XTextWidth(fs, copy, 1);
	}

    }
    
    
#ifdef DEBUG
    fprintf(stderr, "char %c is %sprintable.  Unmasked = %d.\n", c,
	    (nonPrintable) ? "not " : "", c);
#endif
    
    return (width);
} /* end of _CharWidth */
/*
 * _NextTabFrom
 *
 * The \fI_NextTabFrom\fR function is used to calculate the position of the
 * next tab from a gixen \fIx\fR pixel.  If the \fIfl\fR parameter is
 * non-NULL, the appropriate XFontStruct is extracted from the OlFontList and
 * used to calculate the position. Otherwise, the position is  calculated
 * using the XFontStruct pointer \fIfs\fR and TabTable \fItabs\fR.
 *
 * See also:
 *
 * _StringWidth(3), _CharWidth(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern int
_NextTabFrom OLARGLIST((x, fl, tabs, maxwidth))
    OLARG(int,           x)
    OLARG(OlFontList *,  fl)
    OLARG(TabTable,      tabs)
    OLGRA(int,           maxwidth)
{

if (fl)
   maxwidth = fl-> fontl[0]-> max_bounds.width;

while(tabs != NULL && *tabs != 0 && (int)(*tabs) <= x)
   tabs++;

if (tabs == NULL || *tabs == 0)
   x = ((x / (8 * maxwidth) + 1) * 8 * maxwidth) - x;
else
   x = *tabs - x;

return (x);

} /* end of _NextTabFrom */
/*
 * _DrawWrapLine
 *
 * The \fI_DrawWrapLine\fR function performs the actual drawing of the text.
 * Note that the text is actually stored in wide-character (wchar_t) format,
 * but the drawing functions require multibyte format (char *).  Thus,
 * before drawing each segment of characters, we must make a multibyte
 * copy to pass to the drawing function.
 *
 * See also:
 *
 * _WrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern void
_DrawWrapLine OLARGLIST((ctx, x, startx, maxx, p , offset, len, fl, fs, tabs, mode, dpy, win, normal, select, y, ascent, fontht, current, start, end, bg, fg, xoffset))
    OLARG(TextEditWidget,  ctx)		
    OLARG(int,             x) 		/* baseline x position for first character */
    OLARG(int,             startx)	/* usually the left margin */
    OLARG(int,             maxx)	/* rightmost x position allowed for any text */
    OLARG(BufferElement *,  p)	/* a buffer containing the text to be drawn (wchar_t) */
    OLARG(TextPosition,    offset)	/* offset within buffer of text to be drawn */
    OLARG(TextPosition,    len)		/* number of (buffer) characters to draw */
    OLARG(OlFontList *,    fl)		/* fontlist used by this widget, may be NULL */
    OLARG(XFontStruct *,   fs)		/* used only if the fontlist (fl) is NULL */
    OLARG(TabTable,        tabs)
    OLARG(OlWrapMode,      mode)
    OLARG(Display *,       dpy)
    OLARG(Window,          win)		/* the window of the widget */
    OLARG(GC,              normal)	/* GC for normal text */
    OLARG(GC,              select)	/* GC for selected text */
    OLARG(int,             y)		/* baseline y position for first character */
    OLARG(int,             ascent)	/* ascent for the current font(list) */
    OLARG(int,             fontht)	/* line height */
    OLARG(TextPosition,    current)	/* current character, corresponds to p[0] */
    OLARG(TextPosition,    start)	/* start of current selection */
    OLARG(TextPosition,    end)		/* end of current selection */
    OLARG(Pixel,           bg)		/* background */	
    OLARG(Pixel,           fg)		/* font_color */
    OLGRA(int,             xoffset)	/* for horizontal panning */
{
    TextEditPart *  text       = &ctx-> textedit;
    BufferElement * wordstart  = &p[offset];
    BufferElement * wordend    = wordstart - 1; 
    BufferElement * maxp       = &p[len - 1];
    int             wordstartx = x;
    int             width      = 0;
    int             gctype     = (start != end && start < current && current <= end);
    int             recttop    = y - ascent;
    int             min        = fs-> min_char_or_byte2;
    int             max        = fs-> max_char_or_byte2;
    XCharStruct *   per_char   = fs-> per_char;
    int             maxwidth   = fs-> max_bounds.width;
    int             c;
    char * mb_copy;
    
    
    if (maxx > startx) 
    {
	width = maxx - startx;
	if (gctype)
	    /* current position is inside selection (not including the start position)
	     * fill line with foreground 
	     */
	    XFillRectangle(dpy, win, normal, startx, recttop, width, fontht);
	else
	    /* current position is not inside selection (or is at the first character
	     * of the selection), fill line with background 
	     */
	    XFillRectangle(dpy, win, select, startx, recttop, width, fontht);
	width = 0;
    }

    /*
     *	Draw the text as a sequence of selected and non-selected "words".
     *	A "word" is an unbroken sequence of selected(unselected) characters.
     *	Each "word" is drawn as we get to the beginning of the next word.
     *  Break out of the loop when we run out of characters or window space.
     */
    
    for (p = wordstart; p <= maxp && x < maxx; wordend = p++, current++)
    {
	if (start != end && (start == current || current == end))
	{
	    if (start == current)
		/* current has entered selection */
		start = -1;
	    else
		/* current at end of selection */
		end = -1;
	    if (wordend >= wordstart)
	    {
		/* 
		 * We have found a "word". Transform from wchar_t to multibyte
		 * (for the drawing function) and draw it.
		 */
		int num_chars = wordend - wordstart + 1;
		int num_bytes;
		
		num_bytes = _mbCopyOfwcSegment(&mb_copy, wordstart, num_chars);
		if (fl)
		    OlDrawImageString(dpy, win, fl, gctype ? select:normal, wordstartx, 
				      y, (unsigned char *) mb_copy, num_bytes);
		else
		    XDrawImageString(dpy, win, gctype ? select:normal, wordstartx, y, 
				     mb_copy, num_bytes);
		FREE(mb_copy); 
		
	    }
	    /* 
	     *	Reset the beginning of the current "word"
	     */
	    wordstartx = x;		/* starting pixel position of new word */
	    wordstart = p;		/* starting memory address of new word */
	    gctype = (end != -1);	/* flag if new word is selected */
	    p--; 			/* process boundary char again */
	    current--;			/* process boundary char again */
	}
	else
	{
	    /* Not on a selection boundary */
	    switch(*p)
	    {
	    case (BufferElement) '\t':
		width = _NextTabFrom(x - startx - xoffset, fl, tabs, maxwidth);
		if (wordend >= wordstart)
		{
		    /* Draw the word preceding the tab */
		    int num_chars = wordend - wordstart + 1;
		    int num_bytes;
		    
		    num_bytes = _mbCopyOfwcSegment(&mb_copy, wordstart, num_chars);
		    if (fl)
			OlDrawImageString(dpy, win, fl, gctype ? select:normal, 
					  wordstartx, y, (unsigned char *) mb_copy, 
					  num_bytes);
		    else
			XDrawImageString(dpy, win, gctype ? select:normal, wordstartx, y,
					 mb_copy, num_bytes);
		    FREE(mb_copy); 
		}
		/* Fill the tab space */
		XFillRectangle
		    (dpy, win, gctype ? normal:select, x, recttop, width, fontht);
		/* Start the next word */
		wordstartx = x + width;
		wordstart = p + 1; 
		break;
	    case (BufferElement) '\0':
		/* Null character in buffer, (end of line) break out of loop */
		width = maxx;
		break;
	    default:
		width = _CharWidth(ctx, *p, fl, fs, per_char, min, max, maxwidth);
		if ( text->controlCaret &&   
		    (0 < (unsigned)*p && (unsigned)*p <  040
		    || 0177 < (unsigned)*p && (unsigned)*p < 0240))
		{
		    /* Found an ASCII control character
		     * Transform to a printable character and
		     * display with a preceding caret. Note that _CharWidth
		     * will take the width of the caret into account.
		     */
		    char buffer[2];
		    buffer[0] = '^';
		    buffer[1] = *p + '@';
		    
		    if (wordend >= wordstart)
		    {
			/* Display "word" preceding
			 * the control character
			 */
			int num_chars = wordend - wordstart + 1;
			int num_bytes;
			
			num_bytes = _mbCopyOfwcSegment(&mb_copy, wordstart, num_chars);
			if (fl)
			    OlDrawImageString(dpy, win, fl, gctype ? select:normal, 
					      wordstartx, y, (unsigned char *) mb_copy,
					      num_bytes);
			else
			    XDrawImageString(dpy, win, gctype ? select:normal, 
					     wordstartx, y, mb_copy, num_bytes);
			FREE(mb_copy); 
		    }
		    /* Display the caret and transformed character */
		    if (fl)
			OlDrawImageString(dpy, win, fl, gctype ? select:normal, 
					  x, y, (unsigned char *) buffer, 2);
		    else
			XDrawImageString(dpy, win, gctype ? select:normal, 
					 x, y, buffer, 2);
		    /* start the next word */
		    wordstartx = x + width;
		    wordstart = &p[1];
		}
	    }
	    x += width;
	}
    }

    if (x > maxx)
    {
	/* We hit the end of the line (NULL) */
	x -= width;
	wordend--;
    }
    
    /* Draw the last "word" */
    if ((width = wordend - wordstart + 1) > 0)
    {
	int num_bytes;
	
	num_bytes = _mbCopyOfwcSegment(&mb_copy, wordstart, width);
	if (fl)
	    OlDrawImageString
		(dpy, win, fl, gctype ? select:normal, wordstartx, y, 
		 (unsigned char *) mb_copy, num_bytes);
	else
	    XDrawImageString
		(dpy, win, gctype ? select:normal, wordstartx, y, mb_copy, num_bytes);
	FREE(mb_copy); 
    }
    

    /* If the characters drawn do not fill the entire line, fill it */
    if ((width = maxx - (x = MAX(x, startx))) > 0)
	if (gctype)
	{
	    /* Selection extends beyond this line;
	     * Fill the end of the line with foreground color (stippled)
	     */
	    XSetTSOrigin(dpy, normal, xoffset, 0);
	    XSetFillStyle(dpy, normal, FillStippled);
	    XFillRectangle(dpy, win, normal, x, recttop, width, fontht);
	    XSetFillStyle(dpy, normal, FillSolid);
	}
	else
	{
	    /* selection has ended (or has yet to begin);
	     * Fill with background color
	     */
	    XFillRectangle(dpy, win, select, x, recttop, width, fontht);
	}
    
} /* end of DrawWrapLine */
/*
 * _WrapLine
 *
 * The \fI_WrapLine\fR function calculates the next wrap position offset 
 * in the string \fIp\fR from the given \fIoffset\fR.  The calculation
 * begins from the \fIx\fR pixel through the \fImaxx\fR pixel. If the
 * \fIfl\fR parameter is non-NULL, font information is extracted from
 * the fontlist and TabTable \fItabs\fR.  Otherwise, the function uses
 * the XFontstruct pointer \fIfs\fR and TabTable \fItabs\fR.  The type
 * of wrap to be performed is specified in \fImode\fR.
 *
 * See also:
 *
 * _DrawWrapLine(3)
 *
 * Synopsis:
 *
 *#include <TextWrap.h>
 * ...
 */

extern TextPosition
_WrapLine OLARGLIST((ctx, x, maxx, p , offset, fl, fs, tabs, mode))
    OLARG(TextEditWidget,  ctx)
    OLARG(int,             x)
    OLARG(int,             maxx)
    OLARG(BufferElement *, p)
    OLARG(TextPosition,    offset)
    OLARG(OlFontList *,    fl)
    OLARG(XFontStruct *,   fs)
    OLARG(TabTable,        tabs)
    OLGRA(OlWrapMode,      mode)
{
int             startx    = x;
BufferElement * wordend   = &p[offset];
BufferElement * savep;
int             min       = fs-> min_char_or_byte2;
int             max       = fs-> max_char_or_byte2;
XCharStruct *   per_char  = fs-> per_char;
int             maxwidth  = fs-> max_bounds.width;
int             c;

for (savep = p = wordend; x < maxx && *p; p++)
   {
   switch(*p)
      {
      case (BufferElement) '\t':
         wordend = p;
         x += _NextTabFrom(x - startx, fl, tabs, maxwidth);
         break;
      case (BufferElement) ' ':
         wordend = p;
      default:
         x += _CharWidth(ctx, *p, fl, fs, per_char, min, max, maxwidth);
         break;
      }
   }

if (x > maxx || *p)
   if (mode == OL_WRAP_WHITE_SPACE && wordend != savep)
      offset += wordend - savep + 1;
   else{
           /* did the last character cross the right margin? */
       if (x == maxx)
	   /* No, it abuts it */
	   offset += p - savep;
       else
	   /* yes, wrap it along with rest of line */
	   offset += p - savep - 1;
   }
else
   offset = -1;


return(offset);

} /* end of _WrapLine */

