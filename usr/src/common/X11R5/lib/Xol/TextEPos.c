#ifndef NOIDENT
#ident	"@(#)textedit:TextEPos.c	1.28"
#endif

/*
 * TextEPos.c
 *
 */

#include <stdio.h>
#include <string.h>

#include <Xol/buffutil.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/memutil.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/textbuff.h>		/* must follow IntrinsicP.h */
#include <Xol/Dynamic.h>
#include <Xol/Scrollbar.h>
#include <Xol/TextEditP.h>
#include <Xol/TextEPos.h>
#include <Xol/TextDisp.h>
#include <Xol/TextUtil.h>
#include <Xol/TextWrap.h>
#include <Xol/Util.h>

static Boolean ConvertPrimary OL_ARGS((Widget,
				       Atom *,
				       Atom *,
				       Atom *,
				       XtPointer *,
				       unsigned long *,
				       int *
));
static void LosePrimary OL_ARGS((Widget,
				 Atom *
));


/*
 * ConvertPrimary
 *
 */

static Boolean
ConvertPrimary OLARGLIST((w, selection, target, type_return, value_return, length_return, format_return))
   OLARG(Widget, w)
   OLARG(Atom *, selection)
   OLARG(Atom *, target)
   OLARG(Atom *, type_return)
   OLARG(XtPointer *, value_return)
   OLARG(unsigned long *, length_return)
   OLGRA(int *, format_return)
{
    TextEditWidget ctx = (TextEditWidget)w;
    TextEditPart * text = &ctx-> textedit;
    Boolean retval = False;
    Atom * atoms = NULL;
    char * ptr;
    Atom DELETE;
    Atom TARGETS;
    Display *	dpy = XtDisplay((Widget)ctx);
    Atom	stuff;

    DELETE = OL_XA_DELETE(dpy);
    TARGETS = OL_XA_TARGETS(dpy);
    
    if (*selection == XA_PRIMARY)
    {
	if (*target == TARGETS)
	{
	    *format_return = 8;		/* SS: was sizeof(Atom); */
	    *length_return = (unsigned long)5;
	    atoms = (Atom *)MALLOC((*length_return) * (*format_return));
	    atoms[0] = TARGETS;
	    atoms[1] = XA_OL_COPY(dpy);
	    atoms[2] = XA_OL_CUT(dpy);
	    atoms[3] = XA_STRING;
	    atoms[4] = DELETE;
	    *value_return = (caddr_t)atoms;
	    *type_return = XA_ATOM;
	    retval = True;
	}
	else if (*target == (stuff = XA_OL_COPY(dpy))) 
	{
	    (void) OlTextEditCopySelection((Widget)ctx, False);
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = stuff;
	}
	else if (*target == (stuff = XA_OL_CUT(dpy))) 
	{
	    (void) OlTextEditCopySelection((Widget)ctx, True);
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = stuff;
	}

	/* the request for COMPOUND_TEXT could have come only from another */
	/* MoOLIT application, and not from Motif, because Motif always    */
	/* ask for TARGETS first, and COMPOUND_TEXT is not a valid target. */
	/* And we know that when MoOLIT asks for COMPOUND_TEXT it really   */
	/* needs XA_STRING.						   */

	else if (*target == XA_STRING ||
		 *target == XInternAtom(dpy, "COMPOUND_TEXT", False)) 
	{
	    int num_chars  = text-> selectEnd - text-> selectStart;
	    
	    *format_return = 8;
	    if (num_chars){
		OlTextEditReadSubString((Widget)ctx, &ptr, text-> selectStart, 
					text-> selectEnd - 1);
		*length_return = strlen(ptr); 
	    }
	    else
		ptr = NULL;
	    *value_return = ptr;
	    *type_return = XA_STRING;
	    retval = (ptr != NULL);
	}
	else if (*target == DELETE)
	{
	    OlTextEditInsert((Widget)ctx, "", 0);
	    if (text->selectStart == text->selectEnd){
		retval = True;
	    }
	    else
		retval = False;

	    *format_return = 8;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = DELETE;
	}
	else if (*target == XInternAtom(dpy, "_SUN_SELN_YIELD", False))
	{
	    XtDisownSelection((Widget) ctx, *selection, CurrentTime);
	    LosePrimary((Widget)ctx, selection);
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = NULL;
	    *target = NULL;
	    retval = False;
	}
	else
	    OlVaDisplayWarningMsg(dpy,
				  OleNfileTextEPos,
				  OleTmsg2,
				  OleCOlToolkitWarning,
				  OleMfileTextEPos_msg2);
    }
    
    return (retval);
} /* end of ConvertPrimary */
/*
 * LosePrimary
 *
 */

static void
LosePrimary OLARGLIST((w, atom))
    OLARG(Widget, w)
    OLGRA(Atom *, atom)
{
    TextEditWidget ctx = (TextEditWidget)w;

_MoveSelection(ctx, ctx-> textedit.cursorPosition, 0, 0, 0);

} /* end of LosePrimary */
/*
 * _SetDisplayLocation
 *
 * The \fI_SetDisplayLocation\fR procedure is used to set the
 * displayLocation of the TextEdit widget \fIctx\fR to a \fInew\fR
 * wrap location.  This location is \fIdiff\fR wrap lines from the
 * current display wrap location.
 *
 * See also:
 *
 * _SetTextXOffset(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern void
_SetDisplayLocation(ctx, text, diff, setSB, new)
	TextEditWidget ctx;
	TextEditPart * text;
	int            diff;
	int            setSB;
	TextLocation   new;
{

text-> displayLocation = _LocationOfWrapLocation(text-> wrapTable, new);
text-> displayPosition = 
   PositionOfLocation(text-> textBuffer, text-> displayLocation);

if (diff != 0)
   {
   if (setSB && text-> vsb != NULL)
      {
      Arg arg[2];
      int sliderValue;

      XtSetArg(arg[0], XtNsliderValue, &sliderValue);
      XtGetValues(text-> vsb, arg, 1);

      sliderValue += diff;

      XtSetArg(arg[0], XtNsliderValue, sliderValue);
      XtSetArg(arg[1], XtNsliderMax,   text-> lineCount);
      XtSetValues(text-> vsb, arg, 2);
      }
   }

} /* end of _SetDisplayLocation */
/*
 * _SetTextXOffset
 *
 * The \fI_SetTextXOffset\fR procedure is used to increment the xOffset
 * of the TextEdit widget \fIctx\fR by \fIdiff\fR pixels.
 *
 * See also:
 *
 * _SetDisplayLocation(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 *
 */

extern void
_SetTextXOffset(ctx, text, diff, setSB)
	TextEditWidget ctx;
	TextEditPart * text;
	int            diff;
	int            setSB;
{

if (diff != 0)
   {
   text-> xOffset -= diff;
   if (setSB && text-> hsb != NULL)
      {
      Arg arg[2];
      int sliderValue;

      XtSetArg(arg[0], XtNsliderValue, &sliderValue);
      XtGetValues(text-> hsb, arg, 1);

      sliderValue += diff;

      XtSetArg(arg[0], XtNsliderValue, sliderValue);
      XtSetArg(arg[1], XtNsliderMax,   text-> maxX);
      XtSetValues(text-> hsb, arg, 2);
      }
   }

} /* end of _SetTextXOffset */
/*
 * _CalculateCursorRowAnXOffset
 *
 */

extern void
_CalculateCursorRowAndXOffset(ctx, row, xoffset, currentDP, currentIP)
	TextEditWidget ctx;
	int *          row;
	int *          xoffset;
	TextLocation   currentDP;
	TextLocation   currentIP;
{
TextEditPart *   text          = &(ctx-> textedit);
WrapTable *      wrapTable     = text-> wrapTable;
int              x;
BufferElement *  p;
int              diff;
XFontStruct *    fs            = ctx-> primitive.font;
OlFontList *     fl            = ctx-> primitive.font_list;


(void) _MoveToWrapLocation(wrapTable, currentDP, currentIP, row);

p = wcGetTextBufferLocation(text-> textBuffer, currentIP.line, NULL);

x = _StringWidth(ctx, 0, p, _WrapLineOffset(wrapTable, currentIP), 
       text-> cursorLocation.offset - 1, fl, fs, text-> tabs) + 
       PAGE_L_MARGIN(ctx) + text-> xOffset;

if (x < (int)PAGE_L_MARGIN(ctx))
   diff = MAX(x - (int)PAGE_L_MARGIN(ctx) - HORIZONTAL_SHIFT(ctx), text-> xOffset);
else
   if (x <= PAGE_R_MARGIN(ctx))
      diff = 0;
   else
      diff = MIN(text-> maxX + text-> xOffset - PAGEWID(ctx), 
                 x + HORIZONTAL_SHIFT(ctx) - PAGEWID(ctx));

*xoffset = diff;

} /* end of _CalculateCursorRowAndXOffset */
/*
 * _MoveDisplayPosition
 *
 */

extern int
_MoveDisplayPosition(ctx, event, direction_amount, newpos)
	TextEditWidget ctx;
	XEvent *       event;
	OlInputEvent   direction_amount;
	TextPosition   newpos;
{
TextEditPart * text       = &ctx-> textedit;
WrapTable *    wrapTable  = text-> wrapTable;
int            page       = LINES_VISIBLE(ctx);
int            diff       = 0;
TextLocation   current;
TextLocation   minpos;
TextLocation   maxpos;
TextLocation   new;

current = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
minpos  = _FirstWrapLine(wrapTable);
maxpos  = _LastDisplayedWrapLine(wrapTable, page);

switch (direction_amount)
   {
   case OL_SCROLLUP:
      new = _IncrementWrapLocation(wrapTable, current,    -1, minpos, &diff);
      break;
   case OL_SCROLLDOWN:
      new = _IncrementWrapLocation(wrapTable, current,     1, maxpos, &diff);
      break;
   case OL_PAGEUP:
      new = _IncrementWrapLocation(wrapTable, current, -page, minpos, &diff);
      break;
   case OL_PAGEDOWN:
      new = _IncrementWrapLocation(wrapTable, current,  page, maxpos, &diff);
      break;
   case OL_HOME:
      new = _MoveToWrapLocation(wrapTable, current, minpos, &diff);
      break;
   case OL_END:
      new = _MoveToWrapLocation(wrapTable, current, maxpos, &diff);
      break;
   case OL_PGM_GOTO:
      diff = newpos;
      new = _IncrementWrapLocation
         (wrapTable, current, diff, ((diff < 0) ? minpos : maxpos), &diff);
      break;
   default:
      new = current;
#ifdef DEBUG      
      (void) fprintf(stderr, "error: default in _MoveDisplayPosition\n");
#endif      
      break;
   }

_MoveDisplay(ctx, text, text-> lineHeight, page, new, diff, event != NULL);

return (diff);

} /* end of _MoveDisplayPosition */
/*
 * _MoveDisplay
 *
 * The \fI_MoveDisplay\fR procedure is used to move the text displayed
 * in the TextEdit widget \fIctx\fR to a \fInew\fR wrap location which
 * is known to be \fIdiff\fR wrap lines from the current wrap location.
 *
 * See also:
 *
 * _MoveDisplayLaterally(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern void
_MoveDisplay(ctx, text, fontht, page, new, diff, setSB)
	TextEditWidget ctx;
	TextEditPart * text;
	int            fontht;
	int            page;
	TextLocation   new;
	int            diff;
	int            setSB;
{
Display *  display        = XtDisplay(ctx);
Window     window         = XtWindow(ctx);
int        absdiff        = abs(diff);
XEvent     expose_event;
XRectangle rect;

if (diff != 0)
   {
   _TurnTextCursorOff(ctx);
   _SetDisplayLocation(ctx, text, diff, setSB, new);

   if (!(text-> updateState && XtIsRealized((Widget)ctx)))
      return;

   rect.x      = 0;
   rect.width  = ctx-> core.width;
   rect.y      = PAGE_T_MARGIN(ctx);
   rect.height = PAGEHT(ctx);

   if (absdiff < page)
      {
      page = (page + 0) * fontht;
      rect.height = absdiff * fontht;
      if (diff < 0)
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x, rect.y, rect.width, page - rect.height,
           rect.x, rect.y + rect.height);
         }
      else
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x, rect.y + rect.height, rect.width, page - rect.height,
           rect.x, rect.y);
         rect.y = PAGE_T_MARGIN(ctx) + page - rect.height;
         }
      do 
         {
         XIfEvent(display, &expose_event, _IsGraphicsExpose, (char *)window);
         (*ctx-> core.widget_class-> core_class.expose)((Widget)ctx, &expose_event, NULL);
         } while (expose_event.type != NoExpose &&
                ((XGraphicsExposeEvent *)&expose_event)->count != 0);
      }
   _TurnTextCursorOff(ctx);
   XClearArea(display, window, rect.x, rect.y, rect.width, rect.height, False);
   _DisplayText(ctx, &rect);
   }

} /* end of _MoveDisplay */
/*
 * _MoveDisplayLaterally
 *
 */

extern void
_MoveDisplayLaterally(ctx, text, diff, setSB)
	TextEditWidget ctx;
	TextEditPart * text;
	int            diff;
	int            setSB;
{
Display *      display = XtDisplay(ctx);
Window         window  = XtWindow(ctx);
int            absdiff = abs(diff);
XEvent         expose_event;
XRectangle     rect;
int            page = PAGEWID(ctx);

if (diff != 0)
   {
   _TurnTextCursorOff(ctx);
   (void) _SetTextXOffset(ctx, text, diff, setSB);

   if (!(text-> updateState && XtIsRealized((Widget)ctx)))
      return;

   rect.x      = PAGE_L_MARGIN(ctx);
   rect.width  = PAGEWID(ctx);
   rect.y      = 0;
   rect.height = ctx-> core.height;

   if (absdiff < page)
      {
      page = rect.width;
      rect.width = absdiff;
      if (diff < 0)
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x, rect.y, page - rect.width, rect.height,
           rect.x + rect.width, rect.y);
         }
      else
         {
         XCopyArea(display, window, window, text-> gc, 
           rect.x + rect.width, rect.y, page - rect.width, rect.height,
           rect.x, rect.y);
         rect.x = PAGE_L_MARGIN(ctx) + page - rect.width;
         }
      do 
         {
         XIfEvent(display, &expose_event, _IsGraphicsExpose, (char *)window);
         (*ctx-> core.widget_class-> core_class.expose)((Widget)ctx, &expose_event, NULL);
         } while (expose_event.type != NoExpose &&
                ((XGraphicsExposeEvent *)&expose_event)->count != 0);
      }
   rect.x -= FONTWID(ctx);
   rect.x = MAX(rect.x, PAGE_L_MARGIN(ctx));
   rect.width += (2*FONTWID(ctx));  /* to refill any partially cleared chars */
   _TurnTextCursorOff(ctx);
   XClearArea(display, window, rect.x, rect.y, rect.width, rect.height, False);
   _DisplayText(ctx, &rect);
   }

} /* end of _MoveDisplayLaterally */
/*
 * _MoveCursorPosition
 *
 */

extern void
_MoveCursorPosition(ctx, event, direction_amount, newpos)
	TextEditWidget ctx;
	XEvent *       event;
	OlInputEvent   direction_amount;
	TextPosition   newpos;
{
TextEditPart * text       = &(ctx-> textedit);
WrapTable *    wrapTable  = text-> wrapTable;
TextBuffer *   textBuffer = text-> textBuffer;
TextLocation   current;
TextLocation   new;
int            diff;

new = current = text-> cursorLocation;

switch (direction_amount)
   {
   case OL_ROWUP:
   case OL_ROWDOWN:
      new = _WrapLocationOfLocation(wrapTable, new);
      if (text-> save_offset < 0)
         text-> save_offset = current.offset - _WrapLineOffset(wrapTable,new);
      new = (direction_amount == OL_ROWUP) ?
       _IncrementWrapLocation(wrapTable,new,-1,_FirstWrapLine(wrapTable),&diff):
       _IncrementWrapLocation(wrapTable,new, 1,_LastWrapLine(wrapTable), &diff);
      if (diff == 0)
         new = current;
      else
         {
         diff = _WrapLineOffset(wrapTable, new) + text-> save_offset;
         if (diff > _WrapLineEnd(textBuffer, wrapTable, new))
            new.offset = _WrapLineEnd(textBuffer, wrapTable, new);
         else
            {
            new.offset = diff;
            text-> save_offset = -1;
            }
         }
      break;
   case OL_CHARFWD:
      new = IncrementTextBufferLocation(textBuffer, current,  0,  1);   break;
   case OL_CHARBAK:
      new = IncrementTextBufferLocation(textBuffer, current,  0, -1);   break;
   case OL_WORDFWD:
      new = NextTextBufferWord(textBuffer, current);                    break;
   case OL_WORDBAK:
      new = PreviousTextBufferWord(textBuffer, current);                break;
   case OL_LINESTART:
      new = _WrapLocationOfLocation(wrapTable, current);
      new.offset = 0;                                                   break;
   case OL_LINEEND:
      new = _WrapLocationOfLocation(wrapTable, current);
      new.offset = LastCharacterInTextBufferLine(textBuffer, new.line); break;
   case OL_DOCSTART:
      new.line = new.offset = 0;                                        break;
   case OL_DOCEND:
      new.line = LastTextBufferLine(textBuffer);
      new.offset = LastCharacterInTextBufferLine(textBuffer, new.line); break;
   case OL_PANESTART:
      new = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
      new.offset = _WrapLineOffset(wrapTable, new);                     break;
   case OL_PANEEND:
      new = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
      new = _IncrementWrapLocation(wrapTable, new, LINES_VISIBLE(ctx) - 1, 
            _LastDisplayedWrapLine(wrapTable, LINES_VISIBLE(ctx)), NULL);
      new.offset = _WrapLineEnd(textBuffer, wrapTable, new);            break;
   default:
#ifdef DEBUG      
      (void) fprintf(stderr, "error: default in _MoveCursorPosition\n");
#endif
      break;
   }
if (direction_amount != OL_ROWUP && direction_amount != OL_ROWDOWN) 
   text-> save_offset = -1;

(void) _MoveSelection(ctx, PositionOfLocation(textBuffer, new), 0, 0, 0);

} /* end of _MoveCursorPosition */
/*
 * _MoveCursorPositionGlyph
 *
 */

extern void
_MoveCursorPositionGlyph(ctx, new)
	TextEditWidget ctx;
	TextLocation   new;
{
TextEditPart * text          = &(ctx-> textedit);
XFontStruct *  fs            = ctx->primitive.font;
WrapTable *    wrapTable     = text-> wrapTable;
TextBuffer *   textBuffer    = text-> textBuffer;
int            fontht        = text-> lineHeight;
int            page          = LINES_VISIBLE(ctx);
TextLocation   currentIP;
TextLocation   currentDP;
int            row;
int            xoffset;


_TurnTextCursorOff(ctx);

text-> cursorLocation = new;

currentDP = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
currentIP = _WrapLocationOfLocation(wrapTable, new);

_CalculateCursorRowAndXOffset(ctx, &row, &xoffset, currentDP, currentIP);

if (xoffset != 0)
   _MoveDisplayLaterally(ctx, text, xoffset, TRUE);

_TurnTextCursorOff(ctx); /* is this second one really needed ?*/

if (_DrawTextCursor(ctx, row, currentIP, new.offset, HAS_FOCUS(ctx)) == 0)
   {
   currentDP = (row >= page) ?
     _IncrementWrapLocation(wrapTable, currentDP, 
        row - (page - 1),_LastDisplayedWrapLine(wrapTable, page), &row):
     _IncrementWrapLocation(wrapTable, currentDP, 
        row,             _FirstWrapLine(wrapTable), &row);
   _MoveDisplay(ctx, text, fontht, page, currentDP, row, TRUE);
   }

} /* end of _MoveCursorPositionGlyph */
/*
 * _ExtendSelection
 *
 */

extern void
_ExtendSelection(ctx, ol_event)
	TextEditWidget ctx;
	OlInputEvent   ol_event;
{
TextEditPart * text           = &ctx-> textedit;
TextBuffer *   textBuffer     = text-> textBuffer;
TextPosition   cursorPosition = text-> cursorPosition;
TextPosition   selectStart    = text-> selectStart;
TextPosition   selectEnd      = text-> selectEnd;
TextLocation   x;
TextLocation   newloc;
TextLocation   oldloc;
TextPosition   newpos;

switch (ol_event)
   {
   case OL_SELCHARFWD:
      if (cursorPosition == selectEnd)
         cursorPosition = ++selectEnd;
      else
         cursorPosition = ++selectStart;
      break;
   case OL_SELCHARBAK:
      if (cursorPosition == selectStart)
         cursorPosition = --selectStart;
      else
         cursorPosition = --selectEnd;
      break;
   case OL_SELWORDFWD:
      oldloc = LocationOfPosition(textBuffer, selectEnd);
      newloc = EndCurrentTextBufferWord(textBuffer, oldloc);
      if (SameTextLocation(newloc, oldloc))
         {
         newloc = NextTextBufferWord(textBuffer, newloc);
         newloc = EndCurrentTextBufferWord(textBuffer, newloc);
         }
      newpos = PositionOfLocation(textBuffer, newloc);
      if (newpos > selectEnd)
         cursorPosition = selectEnd = PositionOfLocation(textBuffer, newloc);
      break;
   case OL_SELWORDBAK:
      oldloc = LocationOfPosition(textBuffer, selectStart);
      newloc = StartCurrentTextBufferWord(textBuffer, oldloc);
      if (SameTextLocation(newloc, oldloc))
         newloc = PreviousTextBufferWord(textBuffer, newloc);
      newpos = PositionOfLocation(textBuffer, newloc);
      if (newpos < selectStart)
         cursorPosition = selectStart = PositionOfLocation(textBuffer, newloc);
      break;
   case OL_SELLINEFWD:
      x = LocationOfPosition(textBuffer, selectEnd);
      if (x.offset == LastCharacterInTextBufferLine(textBuffer, x.line) && 
          x.line != LastTextBufferLine(textBuffer))
         x.line++;
      x.offset = LastCharacterInTextBufferLine(textBuffer, x.line);
      cursorPosition = 
      selectEnd = PositionOfLocation(textBuffer, x);
      break;
   case OL_SELLINEBAK:
      x = LocationOfPosition(textBuffer, selectStart);
      if (x.offset == 0 && x.line != 0)
         x.line--;
      x.offset = 0;
      cursorPosition = 
      selectStart = PositionOfLocation(textBuffer, x);
      break;
   case OL_SELLINE:
      x = LocationOfPosition(textBuffer, selectStart);
      x.offset = 0;
      selectStart = PositionOfLocation(textBuffer, x);
      x = LocationOfPosition(textBuffer, selectEnd);
      x.offset = LastCharacterInTextBufferLine(textBuffer, x.line);
      cursorPosition = 
      selectEnd = PositionOfLocation(textBuffer, x);
      newpos = text-> selectEnd + 1;
      if (newpos <= LastTextBufferPosition(textBuffer))
         cursorPosition = text-> selectEnd = newpos;
      break;
   default:
      break;
   }

if (0 <= cursorPosition && 
         cursorPosition <= LastTextBufferPosition(textBuffer))
   _MoveSelection(ctx, cursorPosition, selectStart, selectEnd, 7);

} /* end of _ExtendSelection */
/*
 * _MoveSelection
 *
 * The \fI_MoveSelection\fR function performs the actual movement of
 * the selection and cursor point in the TextEdit widget \fIctx\fR.
 * 
 * See also:
 *
 * _MoveCursorPosition(3), TextSetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern Boolean
_MoveSelection(ctx, position, selectStart, selectEnd, selectMode)
	TextEditWidget ctx;
	TextPosition   position;
	TextPosition   selectStart;
	TextPosition   selectEnd;
	int            selectMode;
{
TextEditPart * text       = &ctx-> textedit;
TextBuffer *   textBuffer = text-> textBuffer;
XRectangle     rect;

OlTextMotionCallData call_data;

TextLocation   start;
TextLocation   end;
TextLocation   new_loc;
TextPosition   new_start;
TextPosition   new_end;
TextPosition   new_pos;

int            i;  /* @ TextPosition i; */
TextLine       j;

/* }page break{ */

start = end = LocationOfPosition(textBuffer, position);

switch(text-> selectMode = selectMode)
   {
   case 0: /* char */
      text-> anchor = -1;
      break;
   case 1: /* word */
      start = StartCurrentTextBufferWord(textBuffer, start);
      end   = EndCurrentTextBufferWord(textBuffer, start);
      break;
   case 2: /* line */
      start.offset = 0;
      end.offset   = LastCharacterInTextBufferLine(textBuffer, end.line);
      break;
   case 3: /* paragraph */
      for (j = start.line; j >= 0; j--)
         if (LastCharacterInTextBufferLine(textBuffer, j) == 0)
            break;
      start.line   = MIN(j + 1, LastTextBufferLine(textBuffer));
      start.offset = 0;
      for (j = end.line; j <= LastTextBufferLine(textBuffer); j++)
         if (LastCharacterInTextBufferLine(textBuffer, j) == 0)
            break;
      end.line     = MAX(j - 1, 0);
      end.offset   = LastCharacterInTextBufferLine(textBuffer, end.line);
      break;
   case 4: /* document  */
      start.line   = 0;
      start.offset = 0;
      end.line     = LastTextBufferLine(textBuffer);
      end.offset   = LastCharacterInTextBufferLine(textBuffer, end.line);
      break;
   case 5: /* select/adjust click */
      text-> anchor = position < text-> selectStart ? text-> selectEnd :
               position > text-> selectEnd   ? text-> selectStart :
               text-> cursorPosition == text-> selectEnd ? 
               text-> selectStart : text-> selectEnd;
      if (position <= text-> selectStart)
         end = LocationOfPosition(textBuffer, text-> selectEnd);
      else
         start = LocationOfPosition(textBuffer, text-> selectStart);
      break;
   case 6: /* select/adjust sweep poll */
      if (text-> anchor == -1)
         text-> anchor = text-> cursorPosition;
      if (position <= text-> anchor)
         end = LocationOfPosition(textBuffer, text-> anchor);
      else
         start = LocationOfPosition(textBuffer, text-> anchor);
      break;
   case 7: /* programatic go to */
      start = LocationOfPosition(textBuffer, selectStart);
      end = LocationOfPosition(textBuffer, selectEnd);
      break;
   }
/* }page break{ */
new_start = PositionOfLocation(textBuffer, start);
new_end   = PositionOfLocation(textBuffer, end);
new_pos = text-> cursorPosition;

if (text-> selectMode == 6)
   {
   if (position <= text-> anchor)
      {
      if (text-> cursorPosition != new_start)
         {
         new_pos = new_start;
         new_loc = start;
         }
      }
   else
      {
      if (text-> cursorPosition != new_end)
         {
         new_pos = new_end;
         new_loc = end;
         }
      }
   }
else
   if (text-> selectMode == 5 && position <= text-> selectStart)
      {
      if (text-> cursorPosition != new_start)
         {
         new_pos = new_start;
         new_loc = start;
         }
      }
   else
      if (text-> selectMode == 7)
         {
         new_pos = position;
         new_loc = LocationOfPosition(textBuffer, position);
         }
      else
         {
         new_pos = new_end;
         new_loc = end;
         }

call_data.ok            = True;
if (text-> cursorPosition != new_pos)
   {
   call_data.current_cursor = text-> cursorPosition;
   call_data.new_cursor     = new_pos;
   call_data.select_start   = new_start;
   call_data.select_end     = new_end;
   XtCallCallbacks((Widget)ctx, XtNmotionVerification, &call_data);
   }

/* }page break{ */

if (call_data.ok)
   {
#ifdef CALLBACK_CAN_RESET_MOTION_VALUES
   new_pos   = call_data.new_cursor;
   new_start = call_data.select_start;
   new_end   = call_data.select_end;
#endif
   if (new_pos != text-> cursorPosition)
      {
      text-> cursorPosition = new_pos;
      new_loc   = LocationOfPosition(textBuffer, new_pos);
      _MoveCursorPositionGlyph(ctx, new_loc);
      }

   if (new_start == text-> selectStart && new_end == text-> selectEnd)
      ; /* nop */
   else
      if (text-> selectStart == text-> selectEnd && new_start == new_end)
         {
         text-> selectStart = new_start;
         text-> selectEnd   = new_end;
         }
      else
         {
         if (new_start == text-> selectStart)
            rect = (new_end < text-> selectEnd) ?
               _RectFromPositions(ctx, new_end, text-> selectEnd)     :
               _RectFromPositions(ctx, text-> selectEnd, new_end);
         else
            if (new_end == text-> selectEnd)
               rect = (new_start < text-> selectStart) ?
                  _RectFromPositions(ctx, new_start, text-> selectStart)   :
                  _RectFromPositions(ctx, text-> selectStart, new_start);
            else
               {
               rect = _RectFromPositions(ctx, text-> selectStart, text-> selectEnd);
               text-> selectStart = text-> selectEnd   = -1;
               if (rect.width != 0 && rect.height != 0)
                  _DisplayText(ctx, &rect);
               rect = _RectFromPositions(ctx, new_start, new_end);
               }
         text-> selectStart = new_start;
         text-> selectEnd   = new_end;
         if (rect.width != 0 && rect.height != 0)
            _DisplayText(ctx, &rect);
         }
   if (text-> selectStart < text-> selectEnd &&
       !XtOwnSelection((Widget)ctx, XA_PRIMARY, 
          _XtLastTimestampProcessed(ctx), 
          ConvertPrimary, LosePrimary, NULL))
      {
	OlVaDisplayWarningMsg(XtDisplay((Widget)ctx),
			      OleNfileTextEPos,
			      OleTmsg1,
			      OleCOlToolkitWarning,
			      OleMfileTextEPos_msg1,
			      XtName((Widget)ctx));
      if (text-> selectStart == text-> cursorPosition)
         text-> selectEnd = text-> cursorPosition;
      else
         text-> selectStart = text-> cursorPosition;
      }
   }

return (call_data.ok);

} /* end of _MoveSelection */
/*
 * _TextEditOwnPrimary
 *
 */

extern int
_TextEditOwnPrimary(ctx, time)
	TextEditWidget ctx;
	Time           time;
{
int retval;

if (XtOwnSelection((Widget)ctx, XA_PRIMARY, time, ConvertPrimary, LosePrimary, NULL))
   retval = 1;
else
   retval = 0;

return (retval);

} /* end of _TextEditOwnPrimary */
/*
 * _PositionFromXY
 *
 * The \fI_PositionFromXY\fR function is used to calculate the text position
 * of the nearest character in the TextEdit widget \fIctx\fR for the 
 * coordinates \fIx\fR and \fIy\fR.
 *
 * See also:
 * 
 * _RectFromPositions(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern TextPosition
_PositionFromXY(ctx, x, y)
	TextEditWidget ctx;
	int            x;
	int            y;
{
TextEditPart * text       = &ctx-> textedit;
XFontStruct *  fs         = ctx->primitive.font;
OlFontList *   fl         = ctx-> primitive.font_list;
WrapTable *    wrapTable  = text-> wrapTable;
TextBuffer *   textBuffer = text-> textBuffer;
TabTable       tabs       = text-> tabs;
int            fontht     = text-> lineHeight;
int            start      = (y - (int)PAGE_T_MARGIN(ctx)) / fontht;
int            startx     = PAGE_L_MARGIN(ctx) + text-> xOffset;
int            diff;
TextLocation   new;
TextLocation   current;
TextPosition   position;
BufferElement *p;
TextPosition   i;
TextPosition   maxi;
int            maxx;
int            min        = fs-> min_char_or_byte2;
int            max        = fs-> max_char_or_byte2;
XCharStruct *  per_char   = fs-> per_char;
int            maxwidth   = fs-> max_bounds.width;
int            c;

current = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
if (start == 0)
   new = current;
else
 {
   TextLocation		limit;

   limit = (start < 0) ? _FirstWrapLine(wrapTable) : _LastWrapLine(wrapTable);
   new = _IncrementWrapLocation(wrapTable, current, start, limit, NULL);
 }

maxi = _WrapLineEnd(textBuffer, wrapTable, new);
maxx = x;

new = _LocationOfWrapLocation(wrapTable, new);

p = wcGetTextBufferLocation(textBuffer, new.line, NULL);

for (i = new.offset, x = startx; i <= maxi && x < maxx; i++)
   x += ((c = p[i]) == (BufferElement) '\t') ? 
      _NextTabFrom(x - startx, fl, tabs, maxwidth) : 
      _CharWidth(ctx, c, fl, fs, per_char, min, max, maxwidth);

if (i != new.offset)
   new.offset = i - 1;

position = PositionOfLocation(text-> textBuffer, new);

return (position);

} /* end of PositionFromXY */
/*
 * _RectFromPositions
 *
 * The _RectFromPositions function calculates the bounding box in the
 * TextEdit widget \fIctx\fR for the text between position \fIstart\fR
 * and \fIend\fR.  If the positions are not currently visible the rectangle
 * values (x, y, width, and height) will all be zero (0); otherwise
 * the values are set to define the bounding box for the text.
 *
 * See also:
 *
 * _PositionForXY(3)
 *
 * Synopsis:
 *
 *#include <TextPos.h>
 * ...
 */

extern XRectangle
_RectFromPositions(ctx, start, end)
	TextEditWidget ctx;
	TextPosition   start;
	TextPosition   end;
{
TextEditPart * text       = &ctx-> textedit;
TextBuffer *   textBuffer = text-> textBuffer;
WrapTable *    wrapTable  = text-> wrapTable;
XFontStruct *  fs         = ctx-> primitive.font;
OlFontList *   fl         = ctx-> primitive.font_list;
int            fontht     = text-> lineHeight;
int            page       = LINES_VISIBLE(ctx);
XRectangle     rect;
TextLocation   top;
TextLocation   bot;
TextLocation   display;
TextPosition   position;
int            topdiff;
int            botdiff;

if (start < 0 || end < 0)
   rect.x = rect.y = rect.width = rect.height = 0;
else
   {
   top     = _WrapLocationOfPosition(textBuffer, wrapTable, start);
   bot     = _WrapLocationOfPosition(textBuffer, wrapTable, end);
   display = _WrapLocationOfLocation(wrapTable, text-> displayLocation);

   (void) _MoveToWrapLocation(wrapTable, display, top, &topdiff);
   (void) _MoveToWrapLocation(wrapTable, display, bot, &botdiff);

   if (botdiff < 0 || topdiff > page)
      rect.x = rect.y = rect.width = rect.height = 0;
   else
      {
      topdiff = MAX(0, topdiff);
      botdiff = MIN(page, botdiff);
      rect.y = topdiff * fontht + PAGE_T_MARGIN(ctx);
      rect.height = (botdiff - topdiff + 1) * fontht;
      if (topdiff == botdiff)
         {
         BufferElement * p = wcGetTextBufferLocation(textBuffer, top.line, NULL);
         int offset = _WrapLineOffset(wrapTable, top);
         TextLocation start_loc;
         TextLocation end_loc;

         start_loc = LocationOfPosition(textBuffer, start);
         end_loc   = LocationOfPosition(textBuffer, end);

         rect.x =
            _StringWidth(ctx, 0, p, offset, start_loc.offset - 1, fl, fs, text-> tabs);
         if (end_loc.offset == _WrapLineEnd(textBuffer, wrapTable, bot))
            {
            rect.x += PAGE_L_MARGIN(ctx) + text-> xOffset;
            rect.width = PAGEWID(ctx) - rect.x + PAGE_L_MARGIN(ctx);
            }
         else
            {
            rect.width = _StringWidth(ctx, rect.x, p, 
               start_loc.offset, end_loc.offset - 1, fl, fs, text-> tabs) - rect.x;
            rect.x += PAGE_L_MARGIN(ctx) + text-> xOffset;
            }
         }
      else
         {
         rect.x = PAGE_L_MARGIN(ctx);
         rect.width = PAGEWID(ctx);
         }
      }
   }

return (rect);

} /* end of RectFromPositions */
