#ifndef NOIDENT
#ident	"@(#)textedit:TextEditM.c	1.13"
#endif

/*
 * TextEditM.c:  GUI-specific code for TextEdit widget -- Motif mode
 */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextEditP.h>
#include <Xol/TextField.h>

static void PastePrimaryOrStartSecondary OL_ARGS((TextEditWidget ctx,
						  OlVirtualEvent ve,
						  OlInputEvent ol_event));

/*
 * _OlmTECreateTextCursors
 *
 */

void
_OlmTECreateTextCursors OLARGLIST((ctx))
	OLGRA(TextEditWidget, ctx)
{
    TextEditPart * text   = &ctx-> textedit;
    int            height = FONTHT(ctx) + 1;	
    int		   width  = FONTWID(ctx) - 2; 
    TextCursor *   tcp;
    GC		   tmp_gc;
    XGCValues      values;
    unsigned long  valuesmask;
    XSegment cursor_segments[3] ;

    /* make sure cursor width is odd */
    if ((width & 0x1) == 0){
	width += 1;
#if 0
	printf("forcing odd width\n");
#endif
    }

    if (text-> CursorIn != (Pixmap)0)
    {
	/* free old cursors */
	XFreePixmap(XtDisplay(ctx), text-> CursorIn);
	XFreePixmap(XtDisplay(ctx), text-> CursorOut);
	XtFree((char *)text->CursorP);
    }

    /* fill in TextCursor structure used to place cursor */
    /* xoffset and baseline are used to position the cursor */
    tcp           = text-> CursorP = (TextCursor *)XtMalloc(sizeof(TextCursor));
    tcp->width    = width;
    tcp->height   = height;
    tcp->xoffset  = (int) (width/2);
    tcp->baseline = height - DESCENT(ctx);
    
    /* create I-beam cursor for TextEdit widget with focus */
    text-> CursorIn = XCreatePixmap(XtDisplay(ctx), 
				    DefaultRootWindow(XtDisplay(ctx)), 
				    width, height, 
				    DefaultDepthOfScreen(XtScreen(ctx)));
    /* clear cursor pixmap */
    values.foreground = 0;
    values.function   = GXcopy;
    valuesmask = GCForeground | GCFunction;
    tmp_gc = XCreateGC(XtDisplay(ctx), text->CursorIn, valuesmask, &values);
    XFillRectangle(XtDisplay(ctx), text->CursorIn,  tmp_gc, 
		   0, 0, width, height);
    /* Create the segments for the I-beam cursor */
    /* top segment */
    cursor_segments[0].x1 =
    cursor_segments[0].y1 =
    cursor_segments[0].y2 =
    cursor_segments[1].y1 =
    cursor_segments[2].x1 = 0;

    cursor_segments[0].x2 = (short) (width - 1);
    /* vertical segment */
    cursor_segments[1].x1 = (short) (width / 2);
    cursor_segments[1].x2 = (short) (width / 2);
    cursor_segments[1].y2 = (short) (height - 1);
    /* bottom segment */
    cursor_segments[2].y1 = (short) (height - 1);
    cursor_segments[2].x2 = (short) (width - 1);
    cursor_segments[2].y2 = (short) (height - 1);
    /* Draw the I-beam into the Pixmap */
    XSetForeground(XtDisplay(ctx), tmp_gc, 
		   ctx->core.background_pixel ^ ctx->primitive.font_color);
    XDrawSegments(XtDisplay(ctx), text->CursorIn, tmp_gc, cursor_segments, 3);
    
    text-> CursorOut = XCreatePixmap(XtDisplay(ctx),
				     DefaultRootWindow(XtDisplay(ctx)),
				     width, height, 
				     DefaultDepthOfScreen(XtScreen(ctx)));
    /* clear cursor pixmap */
    XSetForeground(XtDisplay(ctx), tmp_gc, 0);
    XFillRectangle(XtDisplay(ctx), text->CursorOut, tmp_gc,
		   0, 0, width, height);
    /* Create the segments for the caret cursor */
    /* left segment */
    cursor_segments[0].y1 = (short) (height - 1);
    cursor_segments[0].x2 = (short) (width / 2);
    cursor_segments[0].y2 = (short) (2*height / 3);
    /* right segment */
    cursor_segments[1].x1 = (short) (width / 2);
    cursor_segments[1].y1 = (short) (2*height / 3);
    cursor_segments[1].x2 = (short) (width - 1);
    cursor_segments[1].y2 = (short) (height - 1);
    /* Draw the caret into the Pixmap.
     * Use tmp_gc, because text->insgc does a GXxor
     * which would leave the tip of the caret off
     */
    XSetFunction(XtDisplay(ctx), tmp_gc, GXcopy);
    XSetForeground(XtDisplay(ctx), tmp_gc, 
		   ctx->core.background_pixel ^ ctx->primitive.font_color);
    XDrawSegments(XtDisplay(ctx), text->CursorOut, tmp_gc,
		  cursor_segments, 2);
    XFreeGC(XtDisplay(ctx), tmp_gc);
} /* end of _OlmTECreateTextCursors */

/* 
 * _OlmTEButton
 *
 * The (Motif) \fIButton\fR procedure is called whenever a button 
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
_OlmTEButton OLARGLIST((w, ve))
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
   case OLM_BSelect:	/* same as OL_SELECT */
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
  case OLM_BExtend:	/* same as OL_ADJUST */
      if (XtIsSubclass(XtParent(ctx), textFieldWidgetClass))
         _TextEditOwnPrimary(ctx, ((XButtonEvent *)event)-> time);
      _OlTEAdjust(ctx, event);
      break;
  case OLM_BPrimaryPaste:  /* BQuickPaste, BDrag */
  case OLM_BPrimaryCopy:   /* BQuickCopy */
  case OLM_BPrimaryCut:    /* BQuickCut */
			/* Paste/Cut Primary/Secondary selection or
                         * start secondary selection.
			 */
      PastePrimaryOrStartSecondary(ctx, ve, cd.ol_event);
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
} /* end of _OlmTEButton */
/* 
 * _OlmTEKey
 *
 * The (Motif) \fIKey\fR procedure is called whenever a key press event occurs
 * within the TextEdit window.  The procedure is called indirectly by
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
_OlmTEKey OLARGLIST((w, ve))
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
	(cd.ol_event == OL_NEXTFIELD) &&
	!(event->xkey.state & ~LockMask) &&
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
	case OL_ROWDOWN:
	case OL_ROWUP:
	    /*
	     *	If we are a single line TextEdit, these
	     *	keys are supposed to revert to inter-widget navigation.
	     *  Pass the event through to the toolkit.
	     *	Otherwise, handle the internal navigation as for keys
	     *	below.
	     */
	    if (ctx->textedit.linesVisible == 1){
		if (cd.ol_event == OL_ROWDOWN)
		    ve->virtual_name = OL_MOVEDOWN;
		else
		    ve->virtual_name = OL_MOVEUP;
		cd.consumed = False;
	    }
	    else
		_MoveCursorPosition(w, event, cd.ol_event, 0);
	    break;
	case OL_CHARFWD:
	case OL_CHARBAK:
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
} /* end of _OlmTEKey */

/*
 *	PastePrimaryOrStartSecondary
 *
 *  BDrag Press signals that the user is either doing a BDrag click
 *  or a BDrag sweep.
 *	BDrag click --> paste primary selection to pointer position
 *	BDrag sweep --> sweep out secondary selection
 */
static void
PastePrimaryOrStartSecondary OLARGLIST((ctx, ve, ol_event))
    OLARG(TextEditWidget, ctx)
    OLARG(OlVirtualEvent, ve)
    OLGRA(OlInputEvent, ol_event)

{
    TextEditPart * text = &ctx-> textedit;
    XEvent *	event = ve->xevent;
    TextPosition insert_point;
    XButtonEvent b_event;
    PasteSelectionRec *paste_info;
    Boolean cut_selection = False;

    b_event = event->xbutton;
    if (text->editMode == OL_TEXT_READ)
	return;
    
    switch(ol_event){
    case OLM_BPrimaryPaste:	/* BDrag, BQuickPaste */
    case OLM_BPrimaryCopy:   /* BQuickCopy */
	break;
    case OLM_BPrimaryCut:    /* BQuickCut */
	cut_selection = True;
	break;
    }
    if (OlDetermineMouseAction((Widget)ctx, event) == MOUSE_MOVE){
	    /* begin secondary selection */
	/* Mark anchor of secondary selection.
	 * Go into PollMouse loop that follows mouse and
	 * hightlights secondary selection.  When Pollmouse
	 * finds that button has been released, paste/cut
	 * the secondary selection.
	 */
	/* remove this when implementing secondary selection */
	OlUngrabDragPointer((Widget) ctx);
    }
    else{	/* treat mouse click and multiclick as the same */
	insert_point = _PositionFromXY(ctx, event-> xbutton.x, 
				       event-> xbutton.y);
	/* Is insert position inside current selection?
	 * If so, ignore.

	 */
	if (!(text->selectStart != text->selectEnd &&
	    text->selectStart <= insert_point && 
	    insert_point  <= text->selectEnd))
	{
	    paste_info = (PasteSelectionRec *) XtMalloc(sizeof(PasteSelectionRec));
	    /* selection will be inserted at pointer position */
	    paste_info->destination = insert_point;
	    paste_info->cut = cut_selection;
	    paste_info->text = NULL;
	    _OlTEPaste(ctx, event, XA_PRIMARY, paste_info);
	}
    }
} /* end of PastePrimaryOrStartSecondary */
