#ifndef NOIDENT
#ident	"@(#)statictext:StaticTexO.c	1.13"
#endif

/*
 *************************************************************************
 *
 * Description:	Static Text widget.  Open Look GUI-specific code
 *		Most of code from HP widget set,
 *		selection and clipboard code by Andy Oakland, AT&T.
 *
 *******************************file*header*******************************
 */

/*************************************<+>*************************************
 *****************************************************************************
 **
 **		File:        StaticTextM.c
 **
 **		Project:     X Widgets
 **
 **		Description: Code/Definitions for StaticText widget class.
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 **   
 *****************************************************************************
 *************************************<+>*************************************/

/*
 * Include files & Static Routine Definitions
 */

#include <stdio.h>
#include <ctype.h>

#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/keysymdef.h>

#include <Xol/OpenLookP.h>
#include <Xol/StaticTexP.h>
#include <Xol/Dynamic.h>	/* for Drag and Drop */

#define ClassName StaticTextO
#include <Xol/NameDefs.h>

/* **************************forward*declarations***************************
 */

					/* private procedures		*/
static Boolean ConvertSelection OL_ARGS((Widget, 
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
static void LoseClipboard OL_ARGS((Widget, 
				   Atom *
));
static void LoseTransient OL_ARGS((Widget, 
				 Atom *
));
static void StFetchSelection OL_ARGS((StaticTextWidget, 
				      char **,
				      char **
));
static void	StStartSelection OL_ARGS((StaticTextWidget,
					  char *
));
static void	StExtendSelection OL_ARGS((StaticTextWidget,
					   char *
));
static char *StPositionForXY OL_ARGS((StaticTextWidget,
				      Position,
				      Position
));
static void	SelectChar OL_ARGS((char *,
				    char *, 
				    char **,
				    char **,
				    char *
));
static void	SelectWord OL_ARGS((char *,
				    char *,
				    char **,
				    char **,
				    char *
));
static void	SelectLine OL_ARGS((char *,
				    char *,
				    char **,
				    char **,
				    char *
));
static void	SelectAll OL_ARGS((char *,
				   char *,
				   char **,
				   char **,
				   char *
));
static void	StAlterSelection OL_ARGS((StaticTextWidget,
					  int
));
static void	StStartAction OL_ARGS((StaticTextWidget,
				       XEvent *
));
static void	StEndAction OL_ARGS((StaticTextWidget
));
static void	WriteToCB OL_ARGS((StaticTextWidget
));
static void	TakeFocus OL_ARGS((Widget,
				   XEvent *
));

static void DragText OL_ARGS((
			      StaticTextWidget stw,
			      StaticTextPart *stp,
			      Boolean from_kbd
));

static void TextDropOnWindow OL_ARGS((
 Widget stw,
 StaticTextPart * stp,
 OlDnDDropStatus drop_status,
 OlDnDDestinationInfoPtr dst_info,
 OlDnDDragDropInfoPtr	root_info
));

static void
CleanupTransaction OL_ARGS((
 Widget widget,
 Atom selection,
 OlDnDTransactionState state,
 Time timestamp,
 XtPointer closure
));

   /* action procedures */											  
static void SelectStart OL_ARGS((StaticTextWidget, XEvent *));
static void SelectAdjust OL_ARGS((StaticTextWidget, XEvent *));


					/* public procedures		*/

#define NUM_SELECTION_TYPES 4
static void(*select_table[])() = {SelectChar, SelectWord, SelectLine, SelectAll, NULL};

/*
** Selection/Clipboard routines
*/

static void 
SelectStart OLARGLIST((stw, event))
	OLARG(StaticTextWidget, stw)
	OLGRA(XEvent *, event)
{
    char *position;

#ifdef TAKE_FOCUS
    TakeFocus((Widget) stw, event);
#endif
    StStartAction(stw, event);
    StAlterSelection(stw, StaticTextSelect);
    StEndAction(stw);

}

static void	
LosePrimary OLARGLIST((w, atom))
    OLARG(Widget, w)
    OLGRA(Atom *, atom)
{
	StaticTextWidget	stw = (StaticTextWidget)w;

/*
** We make the start of the selection equal to the end of the selection,
** unhighlighting the text.
*/
	stw->static_text.selection_start = stw->static_text.selection_end;
 	stw->static_text.selection_mode = 0;
	_OloSTHighlightSelection(stw,True);
}


static void	
LoseClipboard OLARGLIST((w, atom))
    OLARG(Widget, w)
    OLGRA(Atom *, atom)
{
	StaticTextWidget	stw = (StaticTextWidget)w;
	if ( stw->static_text.clip_contents) {
		XtFree ( stw->static_text.clip_contents);
		stw->static_text.clip_contents = 0;
	}
}

static void	
LoseTransient OLARGLIST((w, atom))
    OLARG(Widget, w)
    OLGRA(Atom *, atom)
{
	StaticTextWidget	stw = (StaticTextWidget)w;
/* FLH Should CleanupTransaction be doing this? (could call this routine) */
    if (stw->static_text.transient){
	    OlDnDFreeTransientAtom((Widget)stw, stw->static_text.transient);
	    stw->static_text.transient = (Atom)None;
	}
}


static Boolean	
ConvertSelection OLARGLIST((w, selection, target, type_return, value_return, length_return, format_return))
    OLARG(Widget,	w)
    OLARG(Atom *, 	selection)
    OLARG(Atom *, 	target)
    OLARG(Atom *, 	type_return)
    OLARG(XtPointer *, 	value_return)
    OLARG(unsigned long *, length_return)
    OLGRA(int *, 	format_return)
{
	StaticTextWidget	stw = (StaticTextWidget)w;
	int		i;
	char		*start, *end, *buffer;
	Atom		stuff;
	Display *	dpy = XtDisplay((Widget)stw);
	
	if (*selection != XA_PRIMARY && *selection !=  XA_CLIPBOARD(dpy)) {
	    if (*selection == stw->static_text.transient){
		/* DragNDrop request from Destination */
		/* FLH still needs to support TARGETS, and COMPOUND_TEXT */
		if (*target == XA_STRING){
		    StFetchSelection(stw, &start, &end);
		    i = end - start;
		    buffer = XtMalloc(1 + i);
		    strncpy (buffer, start, i);
		    buffer[i] = '\0';
		    *value_return = buffer;
		    *length_return = i;
		    *format_return = 8;
		    *type_return = XA_STRING;
		    
		    return (True);
		}
		}
		else{
		    OlVaDisplayWarningMsg(dpy,
					  OleNfileStaticText,
					  OleTmsg1,
					  OleCOlToolkitWarning,
					  OleMfileStaticText_msg1);
		    return (False);
		}
	}

	if (*target == (stuff = XA_OL_COPY(dpy))) {
	    WriteToCB(stw);
	    *value_return = NULL;
	    *length_return = NULL;
	    *format_return = NULL;
	    *type_return = stuff;
	    return(True);
	}

	else if (*target == XA_OL_CUT(dpy)) {
	    OlVaDisplayWarningMsg(dpy,
				  OleNfileStaticText,
				  OleTmsg2,
				  OleCOlToolkitWarning,
				  OleMfileStaticText_msg2);
	}

	else if (*selection == XA_PRIMARY && *target == XA_STRING) {

	    StFetchSelection(stw, &start, &end);
	    i = end - start;
	    
	    buffer = XtMalloc(1 + i);
	    strncpy (buffer, start, i);
	    buffer[i] = '\0';
	    
	    *value_return = buffer;
	    *length_return = i;
	    *format_return = 8;
	    *type_return = XA_STRING;
	    
	    return (True);
	}

	else if (*target == XA_STRING && *selection == XA_CLIPBOARD(dpy)) {
	    buffer = XtMalloc(_OlStrlen(stw->static_text.clip_contents) + 1);
	    strcpy (buffer, stw->static_text.clip_contents);
	    
	    *value_return = buffer;
	    *length_return = _OlStrlen(stw->static_text.clip_contents) + 1;
	    *format_return = 8;
	    *type_return = XA_STRING;
	    return (True);
	}

	else {
	    OlVaDisplayWarningMsg(dpy,
				  OleNfileStaticText,
				  OleTmsg3,
				  OleCOlToolkitWarning,
				  OleMfileStaticText_msg3);
	}
	return (False);
}

static void 
SelectAdjust(stw, event)
StaticTextWidget stw;
XEvent *event;
{
#ifdef TAKE_FOCUS
	TakeFocus((Widget) stw, event);
#endif
	StStartAction(stw, event);
	StAlterSelection(stw, StaticTextAdjust);
	StEndAction(stw);
}

static void
WriteToCB OLARGLIST((stw))
    OLGRA(StaticTextWidget, stw)
{
    int	i;
    char	*buffer;
    char	*selstart, *selend;
    Atom	xa_primary;
    
    StFetchSelection(stw, &selstart, &selend);
    i = selend - selstart;
    if (stw->static_text.clip_contents != NULL)
	XtFree(stw->static_text.clip_contents);
    
    stw->static_text.clip_contents = XtMalloc(1 + i);
    buffer = stw->static_text.clip_contents;
    strncpy (buffer, selstart, i);
    buffer[i] = '\0';
    
    if (!XtOwnSelection( (Widget)stw,
			XA_CLIPBOARD(XtDisplay((Widget)stw)),
			stw->static_text.time,
			ConvertSelection,
			LoseClipboard,
			NULL))
	OlVaDisplayWarningMsg(XtDisplay(stw),
			      OleNfileStaticText,
			      OleTmsg4,
			      OleCOlToolkitWarning,
			      OleMfileStaticText_msg4,
			      "WriteToCB");
    else{
	xa_primary = XA_PRIMARY;
	LosePrimary((Widget)stw, &xa_primary);
    }
}

/*
** This routine sets the start-of- and end-of-selection points, checks for
** multiclicks and increments the select mode appropriately,
*/

static void 
StStartSelection OLARGLIST((stw, position))
    OLARG(StaticTextWidget, stw)
    OLGRA(char *, position)
{

    stw->static_text.selection_mode = 0;
    stw->static_text.selection_start = position;
    stw->static_text.selection_end = position;
}

/*
** This routine updates the selection end during a mouse sweep or
** a MULTI_CLICK.  If it is a multiclick, the position parameter
** is ignored and the real end position is calculated based on
** the selection mode.
*/

static void 
StExtendSelection OLARGLIST((stw, position))
    OLARG(StaticTextWidget, stw)
    OLGRA(char *, position)
{
    char *new_start, *new_end;

    if (stw->static_text.selection_mode == 0)
	stw->static_text.selection_end = position;
    else{
	StFetchSelection(stw, &new_start, &new_end);
	stw->static_text.selection_start = new_start;
	stw->static_text.selection_end = new_end;
    }
}

/*
** This routine figures out what the widget's selection would be in
** the current mode {char, word, line, all}, and returns ptrs to the
** start and finish of it.
*/
static void
StFetchSelection OLARGLIST((stw,start,finish))
    OLARG(StaticTextWidget, stw)
    OLARG(char **, start)
    OLGRA(char **, finish)
{
	(*select_table[stw->static_text.selection_mode]) (
			stw->static_text.selection_start,
			stw->static_text.selection_end,
			start,
			finish,
			stw->static_text.output_string);
}

/*
**  The following family of functions implements different types of selection.
**  All are passed two char *'s indicating the two mouse positions
**  input by the user, two char **'s which will be set to
**  the real start and end of the selection in the current mode,
**  and a pointer to the start of the string.
**
**  The user-supplied start need not be less than the user-supplied end,
**  but the result start will be.
**
*/

/*
** Select char by char.
*/

static void
SelectChar OLARGLIST((inputstart,inputend,start,end,stringstart))
    OLARG(char *, inputstart)
    OLARG(char *, inputend)
    OLARG(char **, start)
    OLARG(char **, end)
    OLGRA(char *, stringstart)
{
	*start = _OlMin(inputstart, inputend);
	*end = _OlMax(inputstart, inputend);
}

/*
** Select word by word.
*/

static void
SelectWord OLARGLIST((inputstart,inputend,start,end,stringstart))
    OLARG(char *, inputstart)
    OLARG(char *, inputend)
    OLARG(char **, start)
    OLARG(char **, end)
    OLGRA(char *, stringstart)
{
	char 	*temp;

	temp = _OlMin(inputstart, inputend) - 1;

	while (	(int)temp >= (int)stringstart && 
			*temp != ' ' && *temp != '\t' && *temp != '\n' ) {
		temp--;
	}

	*start = temp + 1;

	temp = _OlMax(inputstart, inputend);
	while (	*temp && *temp != ' ' && *temp != '\t' && 
		*temp != '\n') {
		temp++;
	}
	*end = temp;
}

static void
SelectLine OLARGLIST((inputstart,inputend,start,end,stringstart))
    OLARG(char *, inputstart)
    OLARG(char *, inputend)
    OLARG(char **, start)
    OLARG(char **, end)
    OLGRA(char *, stringstart)
{
	char 	*temp;

	temp = _OlMin(inputstart, inputend) - 1;

	while (	(int)temp >= (int)stringstart && *temp != '\n') {
		temp--;
	}

	*start = temp + 1;

	temp = _OlMax(inputstart, inputend);
	while (	*temp && *temp != '\n') {
		temp++;
	}
	*end = temp;
}

static void
SelectAll OLARGLIST((inputstart,inputend,start,end,stringstart))
    OLARG(char *, inputstart)
    OLARG(char *, inputend)
    OLARG(char **, start)
    OLARG(char **, end)
    OLGRA(char *, stringstart)
{
	*start = stringstart;
	*end = stringstart;
	while (	**end) {
		(*end)++;
	}
}

static void
StAlterSelection OLARGLIST((stw, mode))
    OLARG(StaticTextWidget, stw)
    OLGRA(int, mode)   /* StaticTextStart, StaticTextAdjust, StaticTextEnd */
{
    char	*position;
    
    
    position = StPositionForXY ( stw, 
				(Position) stw->static_text.ev_x, 
				(Position) stw->static_text.ev_y);
    
    if (position == 0)	/* not on any text */
	return;
    
    if (!XtOwnSelection( (Widget)stw,
			XA_PRIMARY,
			stw->static_text.time,
			ConvertSelection,
			LosePrimary,
			NULL)) {
	
	OlVaDisplayWarningMsg(XtDisplay(stw),
			      OleNfileStaticText,
			      OleTmsg4,
			      OleCOlToolkitWarning,
			      OleMfileStaticText_msg4,
			      "StAlterSelection");
    }
    
    switch (mode) {
	
    case StaticTextSelect: 
	StStartSelection (stw, position);
	_OloSTHighlightSelection(stw,False);
	break;
	
    case StaticTextAdjust: 
	StExtendSelection (stw, position);
	_OloSTHighlightSelection(stw,False);
	break;
    }
}

/*
** This routine redisplays the minimum
** span of text necessary to indicate a new selection.
**
** Forceit is True if the current selection might be identical to the old
** one but we want to force full redisplay anyway (eg, called from Redisplay)
**
** If the new start and end are the same, there is no current selection.
*/

void
_OloSTHighlightSelection OLARGLIST((stw,forceit))
	OLARG(StaticTextWidget,	stw)
	OLGRA(Boolean,	forceit)
{
    char	*old_start = stw->static_text.oldsel_start;
    char	*old_end = stw->static_text.oldsel_end;
    char	*new_start, *new_end;
    
    StFetchSelection(stw,&new_start, &new_end);
    
    if ((forceit != True) && (old_start == new_start && old_end == new_end))
	return;			/* new selection identical to old */
    
    if (!new_start || !new_end)	/* no selection set */
	return;

    /*
     ** First selection ever?
     */
    if (!old_start && !old_end) {
	_OlSTDisplaySubstring(	stw,
			      _OlMin(new_start,new_end),
			      _OlMax(new_start,new_end),
			      stw->static_text.hilite_GC);
    }
    /*
     ** Is there no current selection?
     */
    
    if (new_start == new_end) {
	_OlSTDisplaySubstring(	stw,
			      old_start,
			      old_end,
			      stw->static_text.normal_GC);
    }
    
    /*
     ** Has the selection gotten smaller on the left?
     */
    else if (old_end == new_end && new_start > old_start) {
	_OlSTDisplaySubstring(	stw,
			      old_start,
			      new_start,
			      stw->static_text.normal_GC);
    }
    
    /*
     ** Has the selection gotten smaller on the right?
     */
    else if (old_start == new_start && new_end < old_end) {
	_OlSTDisplaySubstring(	stw,
			      new_end,
			      old_end,
			      stw->static_text.normal_GC);
    }
    
    
    /*
     ** Has it gotten larger on the right?
     */
    else if (old_start == new_start && new_end > old_end) {
	_OlSTDisplaySubstring(	stw,
			      old_end,
			      new_end,
			      stw->static_text.hilite_GC);
    }
    /*
     ** Has it gotten larger on the left?
     */
    else if (old_end == new_end && new_start < old_start) {
	_OlSTDisplaySubstring(	stw,
			      new_start,
			      old_start,
			      stw->static_text.hilite_GC);
    }
    
    /*
     ** Let's give up and just update the whole bloody thing.
     */
    else {
	_OlSTDisplaySubstring(	stw,
			      old_start,
			      old_end,
			      stw->static_text.normal_GC);
	_OlSTDisplaySubstring(	stw,
			      new_start,
			      new_end,
			      stw->static_text.hilite_GC);
    }
    
    stw->static_text.oldsel_start = new_start;
    stw->static_text.oldsel_end = new_end;
}


static void
StStartAction OLARGLIST((stw, event))
    OLARG(StaticTextWidget, stw)
    OLGRA(XEvent *, event)
{
    if (event) {
	stw->static_text.time = event->xbutton.time;
	stw->static_text.ev_x = event->xbutton.x;
	stw->static_text.ev_y = event->xbutton.y;
    }
}

static void
StEndAction OLARGLIST((stw))
    OLGRA(StaticTextWidget, stw)
{

}

/*
 *  _OloSTActivateWidget - this routine is used to activate the text related
 *			operation.
 *
 *		     currently, it only handles OL_COPY.
 */

Boolean
_OloSTActivateWidget OLARGLIST((w, type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	call_data)
{
	Boolean consumed = False;

	switch (type)
	{
		case OL_COPY:
			consumed = True;
			WriteToCB((StaticTextWidget) w);
			break;
		default:
			break;
	}
	return (consumed);
} /* end of _OloSTActivateWidget */

/*
 *  _OloSTHandleButton: this routine handles the ButtonPress and ButtonRelease
 *			events.
 *
 *	note: now this widget is only interested in a perfect match
 *		of a coming event. e.g., ctrl<selectBtn> won't come
 *		in as OL_SELECT, this is the only difference between
 *		the old way and this one.
 */
void
_OloSTHandleButton OLARGLIST((w, ve))
	OLARG(Widget, w)
	OLGRA(OlVirtualEvent, ve)
{
    StaticTextWidget stw = (StaticTextWidget) w;
    XEvent *event = ve->xevent;
    char *position;
    char *start, *end;
    
    if (event->type == ButtonPress){
	switch (ve->virtual_name){
	case OL_SELECT:	    
	    ve->consumed = True;
	    switch(OlDetermineMouseAction(w,event)){
	    case MOUSE_MOVE:	/* could be a drag */
		position = StPositionForXY (stw, event->xbutton.x, 
					    event->xbutton.y);
		StFetchSelection(stw,&start,&end);
		if (stw->static_text.selection_end != 
		    stw->static_text.selection_start &&
		    start <= position && position <= end
		    ){
		    /* StaticText has a selection and ButtonPress
		     * has occurred within that selection.  Check for Drag.
		     */
		    DragText (stw, &stw->static_text, False);
		}
		else{
		    /* Not a DragNDrop.
		     * Start selection operation.
		     * We need to worry about ungrabbing pointer later.
		     */
		    /* FLH is this the right place to free pointer ?*/
		    OlUngrabDragPointer(w);
		    SelectStart(stw, event);
		}
		break;
	    case MOUSE_CLICK:	/* just start a new selection */
		SelectStart(stw, event);
		break;
	    case MOUSE_MULTI_CLICK:  /* increment selection mode */
		stw->static_text.selection_mode = 
		    (stw->static_text.selection_mode + 1) % 
			NUM_SELECTION_TYPES;
		if (stw->static_text.selection_mode == 0)
		    SelectStart(stw, event);
		else
		    SelectAdjust(stw, event);
		break;
	    default:	/* unrecognized mouse button */
		/* FLH should probably recognize OL_DUPLICATE
		 * for drag operations */
		break;
	    }
	    break;
	case OL_ADJUST:		/* extend selection to mouse position */
	    ve->consumed = True;
	    stw->static_text.selection_mode = 0;
	    SelectAdjust(stw, event);
	    break;
	default:
	    /* FLH: Do we need to respond to OL_DUPLICATE or OL_MOVE? */
	    /* perhaps we should at least clear the selection here */
	    /* and/or check for DnD */
	    break;
	} /* switch(ve->virtual_name) */
    }  /* ButtonPress */
    else{	/* ButtonRelease */
	switch (ve->virtual_name){
	case OL_ADJUST: 
	    /*FLH is this really necessary? */
	    ve->consumed = True;
	    SelectAdjust(stw, event);
	    break;
	default:
	    break;
	} /* switch(ve->virtual_name) */
    } /* ButtonRelease */
} /* end of _OloSTHandleButton */

/*
 *  _OloSTHandleMotion: this routine handles the Montion events
 */
void
_OloSTHandleMotion OLARGLIST((w, ve))
	OLARG(Widget, w)
	OLGRA(OlVirtualEvent, ve)
{
    StaticTextWidget stw = (StaticTextWidget) w;

    /* Ignore motion events during a Drag operation.
     * There should be a better way to do this.
     */
    if (stw->static_text.is_dragging)
	return;

    switch (ve->virtual_name)
    {
    case OL_SELECT:
    case OL_ADJUST:
	ve->consumed = True;
	SelectAdjust(stw, ve->xevent);
	break;
    default:
	break;
    }
} /* end of _OloSTHandleMotion */

static void
TakeFocus OLARGLIST((w, event))
    OLARG(Widget, w)
    OLGRA(XEvent *, event)
{


#define HAS_FOCUS(w)	(((StaticTextWidget)(w))->primitive.has_focus == TRUE)

	if (!HAS_FOCUS(w))
	{
		Time		time = event->xbutton.time;
#if 1 /* take it out after removing pointer warping for the dft highlighting */
		Window		junk_win;
		int		junk_xy, x, y;
		unsigned int	junk_mask;

		XQueryPointer(XtDisplay(w), XtWindow(w), &junk_win,
				&junk_win, &junk_xy, &junk_xy,
				&x, &y, &junk_mask);
		(void) XtCallAcceptFocus((Widget)w, &time);
		XWarpPointer(XtDisplay(w),None,XtWindow(w),0,0,0,0,x,y);
#else
		(void) XtCallAcceptFocus(w, &time);
#endif
	}
#undef HAS_FOCUS
} /* end of TakeFocus */

/* 
 * This routine maps an x and y position in a window that is displaying text
 * into the corresponding char * into the source.
 */

/*--------------------------------------------------------------------------+*/
static char *
StPositionForXY OLARGLIST((stw, x, y))
    OLARG(StaticTextWidget, stw)
    OLARG(Position, x)
    OLGRA(Position, y)
{
	int	i, len, line;
	Dimension	y_delta; 
	int	templine;
	char	*tempstring;
	char	*result;
	StaticTextPart	*stp;
	XFontStruct	*font;
	OlFontList	*font_list;

	stp = &(stw->static_text);
	font = stw->primitive.font;
	font_list = stw->primitive.font_list;

/*
** Ensure the event is not outside the widget (This can happen with drags)
*/

	if (x < 0 || x > (Position) stw->core.width) {
		return (0);
	}
	
	if (y < 0 || y > (Position) stw->core.height) {
		return (0);
	}

/*
** Now figure out which line we're on...
*/

	y_delta = (Dimension) ((stp->line_space / 100.0) + 1) * 
			       OlFontHeight(font, font_list);

	if (y_delta != 0) {
		for (	line = 0; 
			(int) ((line+1) * y_delta) < (int) (y - stp->TextRect.y);
			(line)++)
			;
	}
	else {
		line = 0;
	}

	tempstring = stp->line_table[line];

	if (!tempstring)
		return (0);

/*
** now hunt for the right character within the line...
*/
	x -= (Position) _OlSTGetOffset(stw, line);
	len = stp->line_len[line];

	if (font_list) {
	    for (i = 0; i < len && 
		 OlTextWidth(font_list,(unsigned char*)tempstring,i+1) < x; i++)
		;
	}
	else{
	    for (i = 0; i < len && 
		 XTextWidth(font,tempstring,i+1) < x; i++)
		;
	}

	result = tempstring + i;
	return result;
} /* end of StPositionForXY */


/*
 * DragText
 *
 * The \fIDragText\fR procedure handles the drag-and-drop operation.
 * It creates the cursor to be used during the drag and calls the utility
 * drag and drop functions to monitor the drag.  Once the user has dropped
 * the text the DragText procedure determines where the drop was made.
 * If the drop occurs on another window the TextDropOnWindow routine
 * is called to tell the drop window that text had been dropped on
 * it - leaving the transfer of data to the dropee.
 */

static void 
DragText OLARGLIST((stw, stpart, from_kbd))
    OLARG(StaticTextWidget,	stw)
    OLARG(StaticTextPart *,	stpart)
    OLGRA(Boolean,		from_kbd)	/* True means OL_DRAG	*/
{
    Display * dpy = XtDisplay(stw);
    Window	  win = RootWindowOfScreen(XtScreen(stw));
    char *start, *end;
    char      buffer[4];
    int       len;
    int       size;
    static GC     CursorGC;
    static Cursor DragCursor = NULL;
    
    OlDnDDragDropInfo	root_info;
    OlDnDAnimateCursors	cursors;
    OlDnDDestinationInfo	dst_info;
    OlDnDDropStatus		status;
    
    static Pixmap CopySource;
    static Pixmap CopyMask;
    static Pixmap MoreArrow;
    
#include <morearrow.h>
#include <copysrc.h>
#include <copymsk.h>

    /* set flag to indicate pending DragAndDrop so Motion events
     * don't update the selection
     */
    stw->static_text.is_dragging = True;
    
    if (stpart->dragCursor == (Cursor)NULL) {
	if (DragCursor != NULL){
	    XFreeCursor(XtDisplay(stw), DragCursor);
	}
	else
	{
	    XRectangle   rect;
	    
	    rect.x      = 13;
	    rect.y      = 10;
	    rect.width  = 33;
	    rect.height = 12;
	    
	    MoreArrow  = XCreateBitmapFromData(dpy, win,  
					       (OLconst char *)morearrow_bits,
					       morearrow_width, 
					       morearrow_height);
	    CopySource = XCreateBitmapFromData(dpy, win,
					       (OLconst char *)copysrc_bits,
					       copysrc_width, 
					       copysrc_height);
	    CopyMask   = XCreateBitmapFromData(dpy, win,
					       (OLconst char *)copymsk_bits,
					       copymsk_width, 
					       copymsk_height);
	    CursorGC = XCreateGC(dpy, CopySource, 0L, NULL);
	    XSetClipRectangles(dpy, CursorGC, 0, 0, &rect, 1, Unsorted);
	}
	StFetchSelection(stw,&start,&end);
	size = end - start;
	len = _OlMin(3, size);
	strncpy(buffer, start, len);

	DragCursor = _CreateCursorFromBitmaps (dpy, XtScreen(stw), 
					       CopySource, CopyMask, 
					       CursorGC, "black", "white",
					       buffer, len, 13, 19, 1, 1, 
					       size < 4 ? 0 : MoreArrow);
	cursors.yes_cursor = DragCursor;
    }
    else
	cursors.yes_cursor = stpart->dragCursor;

    cursors.no_cursor = OlGetNoCursor(XtScreenOfObject((Widget)stw));

    if ((status = OlDnDTrackDragCursor((Widget)stw, &cursors, &dst_info, 
				       &root_info) != OlDnDDropCanceled)) {
	TextDropOnWindow((Widget)stw, stpart, status, &dst_info, 
			 &root_info);
    }
    
    /* reset flag to indicate DragAndDrop is not pending. Motion events
     * will now update the selection.
     */
    stw->static_text.is_dragging = False;

} /* end of DragText */


/*
 *	TextDropOnWindow: current widget is the source in a drag/drop
 * operation.  The user has completed the drop action and the destination
 * may or may not have registered for the DnD protocol.
 */
static void
TextDropOnWindow OLARGLIST((stw, stp, drop_status, dst_info, root_info))
    OLARG(Widget, stw)
    OLARG(StaticTextPart *, stp)
    OLARG(OlDnDDropStatus, drop_status)
    OLARG(OlDnDDestinationInfoPtr, dst_info)
    OLGRA(OlDnDDragDropInfoPtr, root_info)
{
#define DROP_WINDOW		dst_info->window
#define X			dst_info->x
#define Y			dst_info->y
    
    Widget              drop_widget;
    Display *	        dpy = XtDisplay(stw);
    Window              win = DefaultRootWindow(dpy);
    Widget		shell;
    char *              buf;
    
    drop_widget = XtWindowToWidget(dpy, DROP_WINDOW);
    
    shell  = (drop_widget == NULL || 
	      DROP_WINDOW == RootWindowOfScreen(XtScreen(stw))) ? 
		  NULL : _OlGetShellOfWidget(drop_widget);

    if (drop_widget == stw)
    {
	/* Do nothing.  StaticText does not accept drops.
	 * Selection is not lost.
	 */
	return;
    }
    else  
    {
	Boolean got_selection;
	
	/*FLH should also send out Open Window 2.0 DnD...		*/
	/* Sam C.						*/
	if (drop_status == OlDnDDropFailed)
	{ 
	    /* Destination has not registered OW3.0 protocol,
	     * try OW2.0 protocol (and/or TE protocol)
	     */
	}
	if (stp->transient == (Atom)NULL) {
	    stp->transient = OlDnDAllocTransientAtom(stw);
	    got_selection = False;
	} 
	else 
	    got_selection = True;
	
	if (!got_selection)
	    got_selection = OlDnDOwnSelection((Widget)stw, stp->transient,
					      root_info->drop_timestamp,
					      ConvertSelection, LoseTransient, 
					      (XtSelectionDoneProc)NULL, 
					      CleanupTransaction, NULL);
	if (got_selection) {
	    /* send trigger message to destination (drop site) client */
	    /* always do a copy for statictext (no moves) */
	    if (!OlDnDDeliverTriggerMessage(stw, root_info->root_window,
					    root_info->root_x,
					    root_info->root_y,
					    stp->transient,
					    OlDnDTriggerCopyOp,
					    root_info->drop_timestamp))
	    {
		/* something is wrong...	*/
		/* FLH we should probably free the transient
		 *	atom and do cleanup as in CleanupTransaction
		 */
	    }
	}
    } /* else */
    
#undef DROP_WINDOW
#undef X
#undef Y
} /* end of TextDropOnWindow */

static void
CleanupTransaction OLARGLIST(( widget, selection, state, timestamp, closure))
    OLARG(Widget,		      widget)
    OLARG(Atom,		      selection)
    OLARG( OlDnDTransactionState, state)
    OLARG( Time, 		      timestamp)
    OLGRA( XtPointer, 	      closure)
{
    StaticTextWidget	stxt = (StaticTextWidget)widget;
    

    switch (state) {
    case OlDnDTransactionDone:
    case OlDnDTransactionRequestorError:
    case OlDnDTransactionRequestorWindowDeath:
	if (selection != stxt->static_text.transient)
	    break;
	OlDnDDisownSelection(widget, selection, CurrentTime);
	OlDnDFreeTransientAtom(widget, stxt->static_text.transient);
	stxt->static_text.transient = (Atom)NULL;
	/* need to un-highlight the selection here, if it is still selected */
	/* free any copy if we have made one */
	/* disown the primary selection after un-highlighting it */
	XtDisownSelection((Widget)stxt, XA_PRIMARY, CurrentTime);
	break;
    case OlDnDTransactionEnds:
    case OlDnDTransactionBegins:
	;
    }
} /* end of CleanupTransaction */
