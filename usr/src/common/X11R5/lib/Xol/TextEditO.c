#ident	"@(#)textedit:TextEditO.c	1.8"

/*
 * TextEditO.c:  GUI-specific code for TextEdit widget -- OPEN LOOK mode
 */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextEditP.h>
#include <Xol/TextField.h>

/*
 * _OloTECreateTextCursors
 *
 */
void
_OloTECreateTextCursors OLARGLIST((ctx))
	OLGRA(TextEditWidget, ctx)
{
TextEditPart * text   = &ctx-> textedit;
int            fontht = FONTHT(ctx);
TextCursor *   tcp;
register int   i;
#include <smallIn.h>
#include <smallOut.h>
#include <mediumIn.h>
#include <mediumOut.h>
#include <largeIn.h>
#include <largeOut.h>
#include <hugeIn.h>
#include <hugeOut.h>

/*
 *
 *	NOTE:  The inactive cursor has been removed for this release.
 *	It was removed by replacing the inactive cursor bitmaps
 *	with all 0's.  The old inactive cursor bitmaps are still
 *	present in smallOut.h,mediumOut.h,etc..., so the inactive
 *	cursor can be restored by simply replacing the bitmaps.
 */
static TextCursor TextCursors[] =
   {
   { 12,  
     smallIn_width,  smallIn_height, smallIn_x_hot, smallIn_y_hot, 
     smallIn_bits, smallOut_bits },
   { 18,  
     mediumIn_width,  mediumIn_height, mediumIn_x_hot, mediumIn_y_hot, 
     mediumIn_bits, mediumOut_bits },
   { 24, 
     largeIn_width,  largeIn_height, largeIn_x_hot, largeIn_y_hot, 
     largeIn_bits, largeOut_bits },
   {  0,
     hugeIn_width,  hugeIn_height, hugeIn_x_hot, hugeIn_y_hot, 
     hugeIn_bits, hugeOut_bits },
   };

if (text-> CursorIn != (Pixmap)0)
   {
   XFreePixmap(XtDisplay(ctx), text-> CursorIn);
   XFreePixmap(XtDisplay(ctx), text-> CursorOut);
   }

for (i = 0; TextCursors[i].limit != 0; i++)
   if (fontht <= TextCursors[i].limit)
      break;

tcp = text-> CursorP = &TextCursors[i];

text-> CursorIn  = XCreatePixmapFromBitmapData(XtDisplay(ctx),
                 DefaultRootWindow(XtDisplay(ctx)),
                 (char *)tcp-> inbits, tcp-> width, tcp-> height, 
                 ctx->primitive.font_color ^ ctx-> core.background_pixel, (Pixel)0, 
                 ctx-> core.depth);
text-> CursorOut = XCreatePixmapFromBitmapData(XtDisplay(ctx),
                 DefaultRootWindow(XtDisplay(ctx)),
                 (char *)tcp-> outbits, tcp-> width, tcp-> height, 
                 ctx->primitive.font_color ^ ctx-> core.background_pixel, (Pixel)0, 
                 ctx-> core.depth);

} /* end of _OloTECreateTextCursors */

/* 
 * _OloTEButton
 *
 * The (OPEN LOOK) \fIButton\fR procedure is called whenever a button 
 * down event occurs within the TextEdit window.  The procedure is 
 * called indirectly by OlAction().
 * Even though the XtNconsumedCallback callback was called by OlAction,
 * This procedure calls the buttons callback list to maintain compatibility
 * with older releases to see if the application used this mechanism
 * to intercept the event.  If it has, the event is consumed to prevent
 * the generic routine from handling it.
 * If no callbacks exist or if they all indicate disinterest in the
 * event then the internal procedure to handle interesting button down
 * activity associated with the event is called.
 *
 */
void
_OloTEButton OLARGLIST((w, ve))
    OLARG(Widget, w)
    OLGRA(OlVirtualEvent, ve)
{
XEvent *	event = ve->xevent;
TextEditWidget ctx  = (TextEditWidget) w;
TextEditPart * text = &ctx-> textedit;
TextPosition position;

OlInputCallData cd;

cd.consumed = False;
cd.event    = event;
cd.ol_event = LookupOlInputEvent(w, event, NULL, NULL, NULL);

if (XtHasCallbacks(w, XtNbuttons) == XtCallbackHasSome)
   XtCallCallbacks(w, XtNbuttons, &cd);

if (cd.consumed == False) {
   cd.consumed = True;
   switch(cd.ol_event)
   {
   case OL_SELECT:
      _TextEditOwnPrimary(ctx, ((XButtonEvent *)event)-> time);

      if (!HAS_FOCUS(ctx)){
	  Time time = ((XButtonEvent *)event)-> time;
	  /* set the widget button mask to let the focus handler
	   * know that the widget received focus by a button press
	   */
	  text-> mask = event-> xbutton.state | 1<<(event-> xbutton.button+7);
	  (void) XtCallAcceptFocus(w, &time);
      }
      if ((XtIsSubclass(XtParent(ctx), textFieldWidgetClass)) ||
          (((TextEditClassRec *)textEditWidgetClass)->textedit_class.click_mode == XVIEW_CLICK_MODE))
                _OlTESelect(ctx, event);
      break;
   case OL_ADJUST:
      if (XtIsSubclass(XtParent(ctx), textFieldWidgetClass))
         _TextEditOwnPrimary(ctx, ((XButtonEvent *)event)-> time);
      _OlTEAdjust(ctx, event);
      break;
   case OL_DUPLICATE:
      position = _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y);
      if (text-> selectStart <= position && position <= text-> selectEnd - 1)
         _OlTEDragText(ctx, text, position, OlCopyDrag, False);
      break;
   case OL_PAN:
      {
      text-> mask = event-> xbutton.state | 1<<(event-> xbutton.button+7);
      text-> PanX = event-> xbutton.x;
      text-> PanY = event-> xbutton.y - PAGE_T_MARGIN(ctx);
      OlGrabDragPointer(
	(Widget)ctx, OlGetPanCursor((Widget)ctx), XtWindow((Widget)ctx));
      OlAddTimeOut((Widget)ctx, /* DELAY */ 0, _OlTEPollPan, (XtPointer)ctx);
      }
      break;
   case OL_MENU:
      _OlTEPopupTextEditMenu(ctx, OL_MENU,
		event->xbutton.x_root, event->xbutton.y_root,
		event->xbutton.x, event->xbutton.y);
      break;
   case OL_CONSTRAIN:
   default:
			/* The default action is to let the event pass
			 * through.  We do this by marking it as
			 * being unconsumed.  (Remember, we marked as being
			 * consumed in this 'if' block.
			 */
      cd.consumed = False;
      break;
   }
 }
 ve->consumed = cd.consumed;
} /* end of _OloTEButton */
/* 
 * _OloTEKey
 *
 * The (OPEN LOOK) \fIKey\fR procedure is called whenever a key press event
 * occurs within the TextEdit window.  The procedure is called indirectly by
 * OlAction().
 * Even though the XtNconsumedCallback callback was called by OlAction,
 * This procedure calls the keys callback list to maintain compatibility
 * with older releases to see if the application used this mechanism
 * to intercept the event.  If it has, the event is consumed to prevent
 * the generic routine from handling it.
 * If no callbacks exist or if they all indicate disinterest in the
 * event then the internal procedure to handle interesting key press
 * activity associated with the event is called.
 *
 */
void
_OloTEKey OLARGLIST((w, ve))
    OLARG(Widget, w)
    OLGRA(OlVirtualEvent, ve)
{
    XEvent *	event = ve->xevent;
    TextEditWidget ctx  = (TextEditWidget) w;
    KeySym keysym;
    char * buffer = NULL;
    int length;
    OlInputCallData cd;
    TextPosition position;
    
    cd.consumed = False;
    cd.event    = event;
    cd.keysym   = &keysym;
    cd.buffer   = buffer;
    cd.length   = &length;
    
    cd.ol_event = LookupOlInputEvent(w, event, cd.keysym, &cd.buffer, cd.length);
    /*
     * we may pass the buffer to OlTextEditInsert, which requires a NULL-terminated
     * buffer
     */
    if (*cd.length > 0)
	cd.buffer[*cd.length] = NULL;
        
    /*
     * Remap an unmodified tab key (iff mapped to OL_NEXTFIELD) to
     * OL_UNKNOWN_KEY_INPUT, thus recognized as an insertable char.
     */
    if ((ctx->textedit.insertTab == TRUE) &&
	(cd.ol_event == OL_NEXTFIELD) && !(event->xkey.state) &&
	(*cd.length == 1) && (*cd.buffer == '\011')) {
	cd.ol_event = OL_UNKNOWN_KEY_INPUT;
    }
    
    if (XtHasCallbacks(w, XtNkeys) == XtCallbackHasSome)
	XtCallCallbacks(w, XtNkeys, &cd);
    
    if (cd.consumed == False) {
	cd.consumed = True;
	
	switch(cd.ol_event)
	{
	case OL_UNKNOWN_KEY_INPUT:

	    /* we've got a key that the toolkit does not recognize as
	     * a special "mouseless" key (e.g. OL_PAGEUP).  There
	     * are three cases:
	     * 
	     * 1) A Modifier key: discard it. (For a 
	     * modified key, the server will generate an event for the 
	     * modifier along followed by the key itself with the 
	     * modifiers set.)
	     * 
	     * 2) A Key plus modifier:  Our action depends on the
	     * modifier.  If the modifier is Shift or Shift-lock,
	     * we insert the string associated with the key (if
	     * it has a non-zero length.)  If the modifier is anything
	     * else (e.g. Ctl, Alt), we first check to see if it is registered
	     * as a mnemonic or an accelerator.  If so, we let the
	     * event pass through.  If not, we insert the key if the
	     * string has a non-zero length.
	     *
	     * 3) An unmodified key: We insert the key if the string
	     * has a non-zero length.
	     *
	     * Note the following problem with this logic:
	     * 	
	     *	Mnemonics within menus have no modifiers.
	     *	If an application creates a menu containing a textedit
	     *	widget and a button with a mnemonic, our logic here
	     *	makes it impossible to activate the button via its
	     *	mnemonic when focus is on the TextEdit widget.  This
	     *	is the correct behavior -- the alternative is not being
	     *	able to type the mnemonic character into the TextEdit.
	     */
	    
	    if (IsModifierKey(keysym))
		break;
	    
	    if (!(event->xkey.state & ~(ShiftMask | LockMask)) && (*cd.length > 0))
		/* Modifier is None, Shift, or Shift-lock */
		OlTextEditInsert(w, cd.buffer, *cd.length);
	    else{
		/* Some other modifier */
		if ((_OlFetchMnemonicOwner(w, (XtPointer *)NULL, ve) ||
		     _OlFetchAcceleratorOwner(w, (XtPointer *)NULL, ve)))
		    /* don't consume the event	*/
		    cd.consumed = False;
		else
		    /* Not a mnemonic or accelerator */
		    if (*cd.length > 0)
			OlTextEditInsert(w, cd.buffer, *cd.length);
	    }
	    break;
	case OL_SELFLIPENDS:
	    if (ctx-> textedit.selectStart != ctx-> textedit.selectEnd)
            {
		if (ctx-> textedit.cursorPosition == ctx-> textedit.selectStart)
		    _MoveSelection(ctx, ctx-> textedit.selectEnd, 
				   ctx-> textedit.selectStart, ctx-> textedit.selectEnd, 7);
		else
		    _MoveSelection(ctx, ctx-> textedit.selectStart, 
				   ctx-> textedit.selectStart, ctx-> textedit.selectEnd, 7);
            }
	    break;
	case OL_SELCHARFWD:
	case OL_SELCHARBAK:
	case OL_SELWORDFWD:
	case OL_SELWORDBAK:
	case OL_SELLINEFWD:
	case OL_SELLINEBAK:
	case OL_SELLINE:
	    _ExtendSelection(ctx, cd.ol_event);
	    break;
	case OL_RETURN:
	    if (ctx->textedit.insertReturn == TRUE)
		OlTextEditInsert((Widget)ctx, "\n", 1);
	    else {
                /*
                 * If textedit doesn't consume OL_RETURN, it needs to
                 * continue the lookup as an OL core key. Because only
                 * textedit widget knows about OL_RETURN.
                 */
                OlLookupInputEvent(w, event, ve, OL_CORE_IE);
                cd.consumed = False;
	    }
	    break;
	case OL_REDRAW:
	    OlTextEditRedraw((Widget)ctx);
	    break;
	case OL_CHARFWD:
	case OL_CHARBAK:
	case OL_ROWDOWN:
	case OL_ROWUP:
	case OL_WORDFWD:
	case OL_WORDBAK:
	case OL_LINESTART:
	case OL_LINEEND:
	case OL_DOCSTART:
	case OL_DOCEND:
	case OL_PANESTART:
	case OL_PANEEND:
	    _MoveCursorPosition(w, event, cd.ol_event, 0);
	    break;
	case OL_PAGEUP:
	case OL_PAGEDOWN:
	case OL_HOME:
	case OL_END:
	case OL_SCROLLUP:
	case OL_SCROLLDOWN:
	    (void)_MoveDisplayPosition(w, event, cd.ol_event, 0);
	    break;
	case OL_SCROLLRIGHT:
	    _MoveDisplayLaterally(ctx, &ctx-> textedit, MIN(PAGEWID(ctx), ctx-> textedit.maxX + ctx-> textedit.xOffset - PAGEWID(ctx)), TRUE);
	    break;
	case OL_SCROLLRIGHTEDGE:
	    _MoveDisplayLaterally(ctx, &ctx-> textedit, ctx-> textedit.maxX + ctx-> textedit.xOffset - PAGEWID(ctx), TRUE);
	    break;
	case OL_SCROLLLEFT:
	    _MoveDisplayLaterally(ctx, &ctx-> textedit, MAX(-PAGEWID(ctx), ctx-> textedit.xOffset), TRUE);
	    break;
	case OL_SCROLLLEFTEDGE:
	    _MoveDisplayLaterally(ctx, &ctx-> textedit, ctx-> textedit.xOffset, TRUE);
	    break;
	case OL_MENUKEY:
	    _OlTEPopupTextEditMenu(ctx, OL_MENUKEY,
				   event->xkey.x_root, event->xkey.y_root,
				   event->xkey.x, event->xkey.y);
	    break;
	case OL_DRAG:
	{
		TextEditPart * text = &ctx-> textedit;

		if (text-> selectStart == text-> selectEnd)
			break;

		position = text-> selectStart;
		_OlTEDragText(ctx, &ctx->textedit, position, OlMoveDrag, True);
		break;
	}
	default:
	    /* The default action is to let the event pass
	     * through.  We do this by marking it as
	     * being unconsumed.  (Remember, we marked as being
	     * consumed in this 'if' block.
	     */
	    cd.consumed = False;
	    break;
	}
    }
    ve->consumed = cd.consumed;
} /* end of _OloTEKey */
