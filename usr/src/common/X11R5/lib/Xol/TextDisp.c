/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)textedit:TextDisp.c	1.29"
#endif

/*
 * TextDisp.c
 *
 */

#include <memory.h>

#include <X11/memutil.h>

#include <Xol/buffutil.h>
#include <Xol/strutil.h>

#include <X11/IntrinsicP.h>

#include <Xol/textbuff.h>		/* must follow IntrinsicP.h */

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/Scrollbar.h>
#include <Xol/TextEditP.h>
#include <Xol/TextEPos.h>
#include <Xol/TextUtil.h>
#include <Xol/TextDisp.h>
#include <Xol/TextWrap.h>
#include <Xol/Util.h>

static void         CalculateDisplayTable();
static XRectangle * CalculateRectangle();
static void         _BlinkCursor();
static int          _strmch OL_ARGS((BufferElement *, BufferElement *));
static int          _strnmch OL_ARGS((BufferElement *, BufferElement *, int));

/*
 * CalculateDisplayTable
 *
 */

static void
CalculateDisplayTable(ctx)
	TextEditWidget ctx;
{
TextEditPart * text       = &(ctx-> textedit);
WrapTable *    wrapTable  = text-> wrapTable;
TextBuffer *   textBuffer = text-> textBuffer;

DisplayTable *    DT;
BufferElement *   p;
register int      i;
TextLocation      wrapline;

#ifdef DEBUG
printf("CalculateDisplayTable [ENTER]\n");
#endif

if (text-> DT == NULL || text-> DTsize < text-> linesVisible)
   {
   if (text-> DTsize == 0)
      {
      text-> DT = (DisplayTable *)
         MALLOC(sizeof(DisplayTable) * text-> linesVisible);
      }
   else
      {
      text-> DT = (DisplayTable *)
         REALLOC(text-> DT, sizeof(DisplayTable) * text-> linesVisible);
      }
   }

DT = text-> DT;

for (i = 0; i < text-> DTsize; i++)
   if (DT[i].p)
      FREE(DT[i].p);

text-> DTsize = text-> linesVisible;

wrapline = _WrapLocationOfLocation(wrapTable, text-> displayLocation);

for (i = 0; i < text-> linesVisible; i++)
   {
   DT[i].wraploc = wrapline;
   if ((p = _GetNextWrappedLine(textBuffer, wrapTable, &wrapline)) == NULL)
      {
      for (; i < text-> linesVisible; i++)
         {
         DT[i].used = 
         DT[i].wraploc.line = DT[i].wraploc.offset = 0;
         DT[i].p = NULL;
         }
      }
   else
      {
      DT[i].used = _WrapLineLength(textBuffer, wrapTable, DT[i].wraploc);
      DT[i].p = (BufferElement *)memcpy(MALLOC((DT[i].used + 1) * sizeof(BufferElement)), 
                       (char *) p, DT[i].used * sizeof(BufferElement));
      DT[i].p[DT[i].used] = (BufferElement) '\0';

#ifdef DEBUG
      printf("CalculateDisplayTable: copying line into wraptable\n");
      printf("\toriginal line=");
      _Print_wc_String(p);
      printf("\tcopy ="); 
      _Print_wc_String(DT[i].p);
#endif

      }
   }

#ifdef DEBUG
printf("CalculateDisplayTable [LEAVE]\n");
#endif
} /* end of CalculateDisplayTable */
/*
 * CalculateRectangle
 *
 */

static XRectangle *
CalculateRectangle(ctx)
	TextEditWidget ctx;
{
TextEditPart * text       = &(ctx-> textedit);
WrapTable *    wrapTable  = text-> wrapTable;
TextBuffer *   textBuffer = text-> textBuffer;
XFontStruct *  fs         = ctx-> primitive.font;
OlFontList *   fl         = ctx-> primitive.font_list;
TabTable       tabs       = text-> tabs;
int fontht                = text-> lineHeight;

DisplayTable *   DT;
BufferElement *  p;
register int     i;
TextLocation     wrapline;
int              y;
int              n;
int              m;
int              x;

static XRectangle rect;

if ((DT = text-> DT) == NULL)
   {
   rect.x      = 0;
   rect.y      = 0;
   rect.width  = ctx-> core.width;
   rect.height = ctx-> core.height;
   CalculateDisplayTable(ctx);
   }
else
   {
   rect.x      = 0;
   rect.y      = 0;
   rect.width  = 0;
   rect.height = 0;

   wrapline = _WrapLocationOfLocation(wrapTable, text-> displayLocation);

   y = PAGE_T_MARGIN(ctx);

   for (i = 0; i < text-> linesVisible; i++)
      {
      DT[i].wraploc = wrapline;
      if ((p = _GetNextWrappedLine(textBuffer, wrapTable, &wrapline)) == NULL)
         {
         rect.x = 0;
         rect.width = ctx-> core.width;
         if (rect.y == 0)
            rect.y = y /* REMOVE - fontht*/;
         for (; i < text-> linesVisible; i++)
            {
            DT[i].used = 0;
            DT[i].wraploc.line = DT[i].wraploc.offset = 0;
            if (DT[i].p)
               {
               FREE(DT[i].p);
               DT[i].p = NULL;
               }
            }
         rect.height = PAGE_B_MARGIN(ctx) - rect.y;
         }
      else
         {
         n = _WrapLineLength(textBuffer, wrapTable, DT[i].wraploc);
         if (DT[i].p == (BufferElement *) NULL ||
             (m = _strnmch(DT[i].p, p, MAX(n, DT[i].used) - 1)) != -1)
/*
      if ((m = strmch(DT[i].p, p)) != -1)
*/
            {
            if (rect.y != 0)
               {
               rect.height = y - rect.y + fontht;
               rect.x = 0;
               rect.width = ctx-> core.width;
               }
            else
               {
               rect.y = y;
               rect.height = fontht;
               x = _StringWidth(ctx, 0, p, 0, m - 1, fl, fs, tabs) + text-> xOffset;
               rect.x = x + PAGE_L_MARGIN(ctx);
               rect.width = ctx-> core.width - rect.x;
               }
            DT[i].used = n;
            if (DT[i].p)
               FREE(DT[i].p);
            DT[i].p = (BufferElement *)memcpy(MALLOC(sizeof(BufferElement) * (DT[i].used + 1)),
                             p, DT[i].used * sizeof(BufferElement));
            DT[i].p[DT[i].used] = (BufferElement) '\0';
            }
         }
      y += fontht;
      }
   }

return (&rect);

} /* end of CalculateRectangle */
/*
 * _DisplayText
 *
 */

extern void
_DisplayText(ctx, rect)
	TextEditWidget ctx;
	XRectangle *   rect;
{
TextEditPart *  text       = &(ctx-> textedit);
OlFontList *    fl         = ctx-> primitive.font_list;
XFontStruct *   fs         = ctx->primitive.font;
WrapTable *     wrapTable  = text-> wrapTable;
TextBuffer *    textBuffer = text-> textBuffer;
int             ascent     = ASCENT(ctx);
int             fontht     = text-> lineHeight;
TabTable        tabs       = text-> tabs;
DisplayTable *  DT;
int             start;
int             end;
register int    i;
register int    y;
register BufferElement * p;
TextLocation    insline;
TextLocation    prevline;
int             diff;
TextPosition    wrappos;
TextPosition    len;
GC              normalgc  = text-> gc;
GC              selectgc  = text-> invgc;
TextPosition    offset;
int             x;
int             r;

OlTextMarginCallData margin_call_data;

#ifdef DEBUG
printf("DisplayText [ENTER]\n");
fprintf(stderr,"pageht = %d lineht = %d lineVisible = %d\n",
        PAGEHT(ctx), text-> lineHeight, text-> linesVisible);
#endif

if (!text-> updateState || text-> linesVisible < 1 || !XtIsRealized((Widget)ctx)) 
   {
#ifdef DEBUG
printf("DisplayText [LEAVE]\n");
#endif
   return;
   }

if (rect-> width == 0 || rect-> height == 0)
   {
   margin_call_data.hint = OL_MARGIN_CALCULATED;
   rect = CalculateRectangle(ctx);
   }
else
   {
   margin_call_data.hint = OL_MARGIN_EXPOSED;
   CalculateDisplayTable(ctx);
   }
margin_call_data.rect = rect;


#ifdef DEBUG
(void) fprintf(stderr,"rect: %d,%d %dx%d fontht = %d tmargin = %d\n", 
   rect-> x, rect-> y, rect-> width, rect-> height, fontht, PAGE_T_MARGIN(ctx));
#endif

start = rect-> y - PAGE_T_MARGIN(ctx);
end   = start + rect-> height;

start = MAX(0, start) / fontht;
start = MIN(start, (text-> linesVisible - 1));

end   = MAX(0, end) / fontht;
end   = MIN(end, (text-> linesVisible - 1));
#ifdef DEBUG
(void)fprintf(stderr, "new: Start = %d end = %d vis = %d\n", 
              start, end, text-> linesVisible);
#endif

_TurnTextCursorOff(ctx);

XSetClipRectangles(XtDisplay(ctx), normalgc, 0, 0, rect, 1, Unsorted);
XSetClipRectangles(XtDisplay(ctx), selectgc, 0, 0, rect, 1, Unsorted);

DT = text-> DT;

insline = _WrapLocationOfLocation(wrapTable, text-> cursorLocation);
(void) _MoveToWrapLocation(wrapTable, DT[0].wraploc, insline, &r);

wrappos = _PositionOfWrapLocation(textBuffer, wrapTable, DT[start].wraploc);

for (i = start, y = (i * fontht) + ascent + PAGE_T_MARGIN(ctx); 
     i <= end; 
     i++, y += fontht)
   {
   len = DT[i].used;
   p   = DT[i].p;

#ifdef DEBUG
printf("DisplayText: line %d:",i);
_Print_wc_String(p);
#endif

   if (p == (BufferElement) NULL)
      {
      XClearArea(XtDisplay(ctx), XtWindow(ctx), PAGE_L_MARGIN(ctx), y - ascent, 
         PAGEWID(ctx), PAGE_B_MARGIN(ctx) - (y - ascent) , False);
      break;
      }

   _StringOffsetAndPosition(ctx, p, 0, len, fl, fs, tabs, -text-> xOffset, &x, &offset);
   _DrawWrapLine(ctx, PAGE_L_MARGIN(ctx) + x, PAGE_L_MARGIN(ctx),
                PAGE_R_MARGIN(ctx),
                p, offset, len, 
                fl, fs, tabs, text-> wrapMode, XtDisplay(ctx), XtWindow(ctx), 
                normalgc, selectgc, y, ascent, fontht,
                wrappos + offset, text-> selectStart, text-> selectEnd,
                ctx-> core.background_pixel, ctx-> primitive.font_color,
		text-> xOffset);
   wrappos += len;
   }

XSetClipMask(XtDisplay((Widget)ctx), normalgc, None);
XSetClipMask(XtDisplay((Widget)ctx), selectgc, None);

XtCallCallbacks((Widget)ctx, XtNmargin, &margin_call_data);
/* margin was here!! */

(void) _DrawTextCursor
   (ctx, r, insline, text-> cursorLocation.offset, HAS_FOCUS(ctx));

/* use the Primitive expose procedure to draw the draw the shadow */
(*CORE_C(SUPER_C(textEditWidgetClass)).expose) ((Widget)ctx, NULL, NULL);

#ifdef DEBUG
printf("DisplayText [LEAVE]\n");
#endif

} /* end of _DisplayText */
/*
 * _ChangeTextCursor
 *
 */

extern void
_ChangeTextCursor(ctx)
	TextEditWidget ctx;
{
TextEditPart * text      = &ctx-> textedit;

#if (1)
TextCursor *   tcp  = text-> CursorP;
if (text-> cursor_state == OlCursorOn || text-> cursor_state == OlCursorBlinkOff)
   {
   if (HAS_FOCUS(ctx))
      {
	  /* Widget has just received focus.  Get rid of the
	   * inactive caret and show the active caret
	   */
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s %d\n", __FILE__, __LINE__, "in to out", text-> cursor_state);
#endif
      if (text-> cursor_state == OlCursorOn)
	     /* mask out inactive caret with XOR function */
	  XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx),
		    text-> insgc, 0, 0, tcp-> width, tcp-> height, 
		    text-> cursor_x - tcp-> xoffset, 
		    text-> cursor_y - tcp-> baseline);
          /* mask in active caret with XOR function */
      XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), text-> insgc,
		0, 0, tcp-> width, tcp-> height, 
		text-> cursor_x - tcp-> xoffset, 
		text-> cursor_y - tcp-> baseline);
      }
   else
      {
	  /* Widget has just lost focus.  Get rid of the
	   * active caret and show the inactive caret
	   */
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s %d\n", __FILE__, __LINE__, "out to in", text-> cursor_state);
#endif
      if (text-> cursor_state == OlCursorOn)
             /* mask out active caret with XOR function */
	  XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), 
		    text-> insgc, 0, 0, tcp-> width, tcp-> height, 
		    text-> cursor_x - tcp-> xoffset, 
		    text-> cursor_y - tcp-> baseline);
          /* mask in inactive caret with XOR function */
      XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx), text-> insgc,
		0, 0, tcp-> width, tcp-> height, 
		text-> cursor_x - tcp-> xoffset, 
		text-> cursor_y - tcp-> baseline);
      }
   text-> cursor_state = OlCursorOn;
   }
#else
WrapTable *    wrapTable = text-> wrapTable;
TextLocation   currentIP;
TextLocation   currentDP;
TextLocation   wrapline;
TextLocation   insline;
int            r;

currentIP = text-> cursorLocation;
currentDP = text-> displayLocation;

wrapline = _WrapLocationOfLocation(wrapTable, currentDP);
insline  = _WrapLocationOfLocation(wrapTable, currentIP);
(void) _MoveToWrapLocation(wrapTable, wrapline, insline, &r);

(void) _DrawTextCursor(ctx, r, insline, currentIP.offset, HAS_FOCUS(ctx));
(void) _DrawTextCursor(ctx, r, insline, currentIP.offset, !HAS_FOCUS(ctx));
#endif

if (text-> blink_timer != NULL)
   {
   XtRemoveTimeOut(text-> blink_timer);
   text-> blink_timer = NULL;
   }
if (text-> blinkRate != 0L)
   if (HAS_FOCUS(ctx) /* && text-> cursor_state == OlCursorOn */)
      text-> blink_timer = OlAddTimeOut(
				(Widget)ctx, text-> blinkRate,
				_BlinkCursor, (XtPointer)ctx);

} /* end of _ChangeTextCursor */
/*
 * _BlinkCursor
 *
 */

static void
_BlinkCursor(client_data, id)
	XtPointer        client_data;
	XtIntervalId * id;
{
TextEditWidget ctx     = (TextEditWidget)client_data;
TextEditPart * text    = &ctx-> textedit;
TextCursor *   tcp     = text-> CursorP;
int x                  = text-> cursor_x;
int y                  = text-> cursor_y;
Display *      display = XtDisplay(ctx);
Window         window  = XtWindow(ctx);
long           rate    = text-> blinkRate;
XEvent         expose_event;


if (XCheckWindowEvent(display, window, ExposureMask, &expose_event))
   XPutBackEvent(display, &expose_event);
else
   {
   if (text-> cursor_state == OlCursorOn &&
       PAGE_L_MARGIN(ctx) <= x && x <= PAGE_R_MARGIN(ctx) &&
       text-> shouldBlink == TRUE)
      {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s\n", __FILE__, __LINE__, "blink off");
#endif
      XCopyArea(display, text-> CursorIn, window, text-> insgc, 
         0, 0, tcp-> width, tcp-> height, 
         x - tcp-> xoffset, y - tcp-> baseline);
      text-> cursor_state = OlCursorBlinkOff;
      XFlush(display);
      }
   else
      if (text-> cursor_state == OlCursorBlinkOff &&
          PAGE_L_MARGIN(ctx) <= x && x <= PAGE_R_MARGIN(ctx) &&
          text-> shouldBlink == TRUE)
         {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s\n", __FILE__, __LINE__, "blink on");
#endif
         XCopyArea(display, text-> CursorIn, window, text-> insgc, 
            0, 0, tcp-> width, tcp-> height, 
            x - tcp-> xoffset, y - tcp-> baseline);
         text-> cursor_state = OlCursorOn;
         XFlush(display);
         rate = rate / 2;
         }
   }

text-> blink_timer = OlAddTimeOut(
			(Widget)ctx, rate, _BlinkCursor, (XtPointer)ctx);

} /* end of _BlinkCursor */
/*
 * _DrawTextCursor: We know the cursor does not appear in the TextEdit window;
 *  _DrawTextCursor is only called 1) as a result of an Expose event and
 *  2) when moving the cursor glyph (_MoveCursorPositionGlyph).
 *  If we are not in the middle of a sweep-selection (and updates are
 *  enabled), show the appropriate cursor (inactive or active) and
 *  set the cursor_state flag to OlCursorOn.  Otherwise, set the
 *  cursor_state flage to OlCursorOff.
 */

extern int
_DrawTextCursor(ctx, r, line, offset, flag)
	TextEditWidget ctx;
	int            r;
	TextLocation   line;
	TextPosition   offset;
	int            flag;	/* textedit has focus ? */
{
TextEditPart *   text          = &(ctx-> textedit);
PrimitivePart *  primitive     = &(ctx-> primitive);
XFontStruct *    fs            = ctx->primitive.font;
OlFontList *     fl            = ctx-> primitive.font_list;
WrapTable *      wrapTable     = text-> wrapTable;
TextBuffer *     textBuffer    = text-> textBuffer;
int              fontht        = text-> lineHeight;
TabTable         tabs          = text-> tabs;
TextCursor *     tcp           = text-> CursorP;
int              x;
int              y;
BufferElement *  p;
int              retval        = 0;

#ifdef I18N
static XPoint     spot = {0,0};
static OlIcValues icvalues[2];
icvalues[0].attr_name = OlNspotLocation;
icvalues[0].attr_value = &spot;
icvalues[1].attr_name = NULL;
icvalues[1].attr_value = NULL;
#endif


if (r >= 0 && r < LINES_VISIBLE(ctx))
   {
   y = PAGE_T_MARGIN(ctx) + ASCENT(ctx) + r * fontht;
   p = wcGetTextBufferLocation(textBuffer, line.line, NULL);
   x = _StringWidth(ctx, 0, p, _WrapLineOffset(wrapTable, line), offset - 1, 
		    fl, fs, tabs) + PAGE_L_MARGIN(ctx) + text-> xOffset;

   if (text-> updateState && 
       text-> selectMode != 5 && 
       text-> selectMode != 6 &&
       (x >= (int)PAGE_L_MARGIN(ctx)) && (x <= PAGE_R_MARGIN(ctx)))
      {
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s\n", __FILE__, __LINE__, "draw it");
#endif
      if (flag)
         XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), 
                   text-> insgc, 0, 0, tcp-> width, tcp-> height, 
                   x - tcp-> xoffset, y - tcp-> baseline);
      else
         XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx), 
                   text-> insgc, 0, 0, tcp-> width, tcp-> height, 
                   x - tcp-> xoffset, y - tcp-> baseline);
      text-> cursor_state = OlCursorOn;
      }
   else
      text-> cursor_state = OlCursorOff;
#ifdef I18N
   if (primitive->ic)
      if (x!=text->cursor_x||y!=text->cursor_y) {
         spot.x = x;
         spot.y = y;
         OlSetIcValues(primitive->ic, icvalues);
      }
#endif
   text-> cursor_x = x;
   text-> cursor_y = y;
   retval = 1;
   }
else
   text-> cursor_state = OlCursorOff;

#if (0)
if (text-> blink_timer != NULL)
   {
   XtRemoveTimeOut(text-> blink_timer);
   text-> blink_timer = NULL;
   }
if (text-> blinkRate != 0L)
   if (flag && text-> cursor_state == OlCursorOn)
      text-> blink_timer = OlAddTimeOut(
		(Widget)ctx, text->blinkRate, _BlinkCursor, (XtPointer)ctx);
#endif

return (retval);

} /* end of _DrawTextCursor */
/*
 * _TurnTextCursorOff: If the cursor is currently shown, remove it
 * by XOR'ing the cursor image to the cursor position.  Set the
 * cursor_state to OlCursorOff.  This routine is not called in
 * response to Expose events (see DrawTextCursor), so if 
 * cursor_state is OlCursorOn, we know that the cursor is actually
 * displayed.
 */

extern void
_TurnTextCursorOff(ctx)
	TextEditWidget ctx;
{
TextEditPart * text = &ctx-> textedit;
TextCursor *   tcp  = text-> CursorP;

if (text-> updateState && text-> cursor_state == OlCursorOn)
   {
       /* cursor is on */
#ifdef DEBUG_CURSOR
fprintf(stderr,"%s %d %s\n", __FILE__, __LINE__, "turn off");
#endif
   if (HAS_FOCUS(ctx))
         /* mask out active cursor (via XOR) */
      XCopyArea(XtDisplay(ctx), text-> CursorIn, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
   else
          /* mask out inactive cursor (via XOR) */
      XCopyArea(XtDisplay(ctx), text-> CursorOut, XtWindow(ctx), text-> insgc,
         0, 0, tcp-> width, tcp-> height, 
         text-> cursor_x - tcp-> xoffset, text-> cursor_y - tcp-> baseline);
   text-> cursor_state = OlCursorOff;
   }

#if (0)
if (text-> blink_timer != NULL)
   {
   XtRemoveTimeOut(text-> blink_timer);
   text-> blink_timer = NULL;
   }
#endif

} /* end of _TurnTextCursorOff */



/*
 * _strmch
 *
 * The \fI_strmch\fR function is used to compare two strings \fIs1\fR
 * and \fIs2\fR.  It returns the index of the first BufferElement which 
 * does not match in the two strings.  The value -1 is returned if 
 * the strings match.
 *
 */

static int
_strmch OLARGLIST((s1,s2))
OLARG (BufferElement *,  s1)
OLGRA (BufferElement *,  s2)
{

register int offset = 0;

if(s1 == NULL || s2 == NULL)
	return 0;		/* defend against NULL pointers */

while (*s1 == *s2 && *s1 && *s2)
   {
   s1++;
   s2++;
   offset++;
   }
return (*s1 == *s2) ? -1 : offset;

} /* end of _strmch */


/*
 * _strnmch
 *
 * The \fI_strnmch\fR function is used to compare two strings \fIs1\fR
 * and \fIs2\fR through at most \fIn\fR characters and return the
 * index of the first character which does not match in the two strings.
 * The value -1 is returned if the strings match for the specified number
 * of characters.
 *
 */

static int
_strnmch OLARGLIST((s1, s2, n))
OLARG (BufferElement *,  s1)
OLARG (BufferElement *,  s2)
OLGRA (int,              n)
{
int start_n = n;

if(s1 == NULL || s2 == NULL)
	return 0;		/* defend against NULL pointers */

while (*s1 == *s2 && n)
   {
   s1++;
   s2++;
   n--;
   }

return (*s1 == *s2) ? -1 : start_n - n;

} /* end of _strnmch */
