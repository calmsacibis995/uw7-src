#ifndef NOIDENT
#ident	"@(#)textedit:TextWrap.h	1.3"
#endif

/*
 * TextWrap.h
 *
 */

#ifndef _TextWrap_h
#define _TextWrap_h




extern TextLocation _FirstWrapLine OL_ARGS((
 WrapTable * wrapTable
));
extern TextLocation _LastDisplayedWrapLine OL_ARGS((
 WrapTable * wrapTable,
 int page
));
extern TextLocation _LastWrapLine OL_ARGS((
 WrapTable * wrapTable
));
extern int          _LineNumberOfWrapLocation OL_ARGS((
 WrapTable * wrapTable,
 TextLocation location
));
extern int          _NumberOfWrapLines OL_ARGS((
WrapTable * wrapTable
));

extern void         _UpdateWrapTable OL_ARGS((
TextEditWidget ctx,
TextLine       start,
TextLine       end
));
extern void         _BuildWrapTable OL_ARGS((
 TextEditWidget ctx
));

extern BufferElement *       _GetNextWrappedLine OL_ARGS((
 TextBuffer * textBuffer,
 WrapTable * wrapTable,
 TextLocation * location
));

extern TextLocation _IncrementWrapLocation OL_ARGS((
 WrapTable *  wrapTable,
 TextLocation current,
 int          n,
 TextLocation limit,
 int *        reallines
));
extern TextLocation _MoveToWrapLocation OL_ARGS((
 WrapTable *  wrapTable,
 TextLocation current,
 TextLocation limit,
 int *        reallines
));

extern TextPosition _PositionOfWrapLocation OL_ARGS((
 TextBuffer * textBuffer,
 WrapTable *  wrapTable,
 TextLocation location
));
extern TextLocation _LocationOfWrapLocation OL_ARGS((
 WrapTable *  wrapTable,
 TextLocation location
));
extern TextLocation _WrapLocationOfPosition OL_ARGS((
 TextBuffer * textBuffer,
 WrapTable *  wrapTable,
 TextPosition position
));
extern TextLocation _WrapLocationOfLocation OL_ARGS((
 WrapTable *  wrapTable,
 TextLocation location
));

extern TextPosition _WrapLineOffset OL_ARGS((
 WrapTable *  wrapTable,
 TextLocation location
));
extern TextPosition _WrapLineLength OL_ARGS((
 TextBuffer * textBuffer,
 WrapTable *  wrapTable,
 TextLocation location
));
extern TextPosition _WrapLineEnd OL_ARGS((
 TextBuffer * textBuffer,
 WrapTable *  wrapTable,
 TextLocation location
));
extern void         _StringOffsetAndPosition OL_ARGS((
 TextEditWidget	 ctx,
 BufferElement * s,
 TextPosition    start,
 TextPosition    end,
 OlFontList *    fl,
 XFontStruct  *  fs,
 TabTable        tabs,
 int             minx,
 int *           retx,
 TextPosition *  reti
));
extern int          _StringWidth OL_ARGS((
 TextEditWidget   ctx,					  
 int              x,
 BufferElement *  s,
 TextPosition     start,
 TextPosition     end,
 OlFontList *     fl,
 XFontStruct *    fs,
 TabTable         tabs
));
extern int          _CharWidth OL_ARGS((
 TextEditWidget	ctx,
 int           	c,
 OlFontList *  	fl,
 XFontStruct * 	fs,
 XCharStruct * 	per_char,
 int           	min,
 int           	max,
 int           	maxwidth
));
extern int          _NextTabFrom OL_ARGS((
 int           x,
 OlFontList *  fl,
 TabTable      tabs,
 int           maxwidth
));
extern void         _DrawWrapLine OL_ARGS((
 TextEditWidget  ctx,
 int             x,
 int             startx,
 int             maxx,
 BufferElement *  p,
 TextPosition    offset,
 TextPosition    len,
 OlFontList *    fl,
 XFontStruct *   fs,
 TabTable        tabs,
 OlWrapMode      mode,
 Display *       dpy,
 Window          win,
 GC              normal,
 GC              select,
 int             y,
 int             ascent,
 int             fontht,
 TextPosition    current,
 TextPosition    start,
 TextPosition    end,
 Pixel           bg,
 Pixel           fg,
 int             xoffset
));
extern TextPosition _WrapLine OL_ARGS((
 TextEditWidget  ctx,				      
 int             x,
 int             maxx,
 BufferElement * p,
 TextPosition    offset,
 OlFontList *    fl,
 XFontStruct *   fs,
 TabTable        tabs,
 OlWrapMode      mode
));

#endif
