#ident	"@(#)debugger:libmotif/common/Text_area.C	1.18"

#include "UI.h"
#include "Component.h"
#include "Text_area.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>

//#define DEBUG_TA(x) fprintf x
#define DEBUG_TA(x)

// ------------------------ Text_data ----------------------------
// Text_data manages lines read into the Text widget. It keeps track of
// char positions of each line, and the total number of lines read.
// Char positions start from 0. Line numbers start from 1, this is to
// be consistent with both the framework and text widget which
// maintain line numbers (positions) in the range of [1,n] where n is
// the total number of lines of text. Internal to text_data, the char
// position of line 1 is stored at lpos[0] however.

#define MAXLINES 1024

Text_data::Text_data()
{ 
	next_line = 0; 
	nchars = 0; 
	lpos = (int *)XtMalloc(MAXLINES*sizeof(int));
	maxlines = MAXLINES;
}

// process a string: record character position of each newline
// assumes next_line is >= 1 and lpos[0] is 0
void
Text_data::add_string(const char *p)
{
	for(; *p; p++)
	{
		++nchars;
		if (*p == '\n')
		{
			if (next_line >= maxlines)
			{
				maxlines += MAXLINES;
				lpos = (int *)XtRealloc((char *)lpos,
					maxlines*sizeof(int));
			}
			lpos[next_line] = nchars;
			++next_line;
		}
	}
}

void
Text_data::reset()
{
	nchars = 0;
	lpos[0] = 0;
	next_line = 1;
}

// return line number of character position 'cpos'
// start search at 'start_line'
int
Text_data::get_line(int start_line, int cpos)
{
	if ( start_line < 1 || start_line >= next_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;	
	}
	--start_line;
	int	*lp = &lpos[start_line];
	int	line = start_line;
	for (; line < next_line; ++line, ++lp)
	{
		if (*lp == cpos)
			break;
		else if (*lp > cpos)
		{
			// belongs to the previous line
			return line;
		}
	}
	line++;
	return(line >= next_line ? 0 : line);
}

// return the char position of the start of line 'line'
int
Text_data::get_lpos(int line)
{
	if (line < 1 || line >= next_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;	
	}
	return lpos[line-1];
}

// return the char position of the end of line 'line'
int Text_data::get_epos(int line)
{
	if (line < 1 || line >= next_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;	
	}
	return(lpos[line] - 1);
}

// delete first cnt lines from buffer
void
Text_data::delete_first_n_lines(int cnt)
{
	int	*lptr, *lend;
	if (cnt >= next_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;	
	}
	lptr = lpos;
	memmove(lptr, lptr + cnt,
		(next_line-cnt)*sizeof(lpos[0]));
	next_line -= cnt;
	// adjust all remaining positions
	int offset = *lptr;
	lend = &lptr[next_line];
	for (; lptr < lend; lptr++)
		*lptr -= offset;
	nchars -= offset;
}

// replace the text from spos to epos with new
// text - if len is 0, just deleting; if epos==spos
// just inserting; else replacing existing text with new text
void
Text_data::replace_text(int spos, int epos, int len, char *text)
{
	int	sline, eline;
	int	newnlines = 0;
	int	oldnlines = 0;
	int	cnt;
	char	*ptr = text;
	int	*lptr, *eptr;
	int	delta = 0;
	int	replace_len = epos - spos;

	if ((sline = get_line(1, spos)) == next_line)
	{
		// starting a new line
		newnlines++;
	}
	if (replace_len && ((eline = get_line(sline, epos-1)) == 0))
	{
		// replace text goes over end
		eline = next_line;
	}
	if (len)
	{
		// inserting or replacing
		for(; *ptr; ptr++)
		{
			if (*ptr == '\n')
				newnlines++;
		}
		delta = len;
	}
	if (replace_len)
	{
		// deleting or replacing
		oldnlines = eline - sline;
		if (lpos[eline] == epos)
			oldnlines++;
		delta = delta - replace_len;
	}
	cnt = newnlines - oldnlines;
	if (cnt > 0)
	{
		// adding lines
		if (next_line + cnt >= maxlines)
		{
			maxlines += cnt + MAXLINES;
			int	*npos = new int[maxlines];		
			memcpy((char *)npos, (char *)lpos,
				sline*sizeof(lpos[0]));
			memcpy((char *)&npos[sline+cnt], 
				(char *)&lpos[sline],
				(next_line-sline)*sizeof(lpos[0]));
			delete lpos;
			lpos = npos;
		}
		else
		{
			memmove((char *)&lpos[sline+cnt],
				(char *)&lpos[sline],
				(next_line-sline)*sizeof(lpos[0]));

		}
		// process new text for newlines
		lptr = &lpos[sline];
		next_line += cnt;

		for(ptr = text; *ptr; ptr++)
		{
			spos++;
			if (*ptr == '\n')
			{
				*lptr++ = spos;
			}
		}
		if (*(ptr - 1) != '\n')
			// last line does not yet have newline
			*lptr = spos+1;
	}
	else 
	{
		lptr = &lpos[sline];
		if (cnt < 0)
		{
			cnt = -cnt;
			memmove(lptr, lptr + cnt,
				(next_line-cnt-sline)*sizeof(lpos[0]));
			next_line -= cnt;
		}
	}
	if (!delta)
		return;
	eptr = &lpos[next_line-1];
	for(; lptr <= eptr; lptr++)
	{
		*lptr += delta;
	}
	nchars += delta;
}

//--------------------- Text_area -------------------------------------

static void
resizeCB(Widget, Text_area *area, XEvent *ev, Boolean *)
{
	if (!area || !ev)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	if (ev->type == ConfigureNotify)
		area->resize();
}

static void
select_start(Widget w, XEvent *e, char **params, Cardinal *nparams)
{
	Text_area	*text = 0;

	// assume the parent of the Text widget has a Text_area pointer
	// in its userData
	// userData in Text widget itself is a Component when
	// created by Text_display
	XtVaGetValues(XtParent(w), XmNuserData, &text, 0);
	if (!text)
		return;
	text->selection_begin(e);
	XtCallActionProc(w, "grab-focus", e, params, *nparams);
}

static void
select_end(Widget w, XEvent *e, char **params, Cardinal *nparams)
{
	Text_area	*text = 0;

	// assume the parent of the Text widget has a Text_area pointer
	// in its userData
	// userData in Text widget itself is a Component when
	// created by Text_display
	XtVaGetValues(XtParent(w), XmNuserData, &text, 0);
	if (!text)
		return;
	XtCallActionProc(w, "extend-end", e, params, *nparams);
	text->selection_end(e);
}

// This constructor is called from the framework code directly to
// create a Text_area object.  A Text_area is implemented with a
// Text widget inside a scrolled window
Text_area::Text_area(Component *p, const char *name, int rows, int columns,
	Boolean editable, Callback_ptr scb, Command_sender *c, Help_id h, 
	int pages) 
{
	// fill in component values - component is a virtual base class
	parent = p;
	label = name;
	creator = c;
	help_msg = h;

	nrows = rows;
	npages = pages;
	sel_string = text_content = 0;
	update_content = TRUE;
	select_cb = scb;
	selection_start = selection_size = 0;

	widget = XtVaCreateWidget("text_scroller", 
		xmScrolledWindowWidgetClass,
		parent->get_widget(), 
		// automatic mode seems not to work well with columns &
		// rows set ???
		//XmNscrollingPolicy, XmAUTOMATIC,
		XmNuserData, this, 
		0);

	text_area = XtVaCreateManagedWidget(label, xmTextWidgetClass,
		widget,
		XmNvalue, "",
		XmNcolumns, columns,
		XmNrows, rows,
		XmNeditable, editable ? TRUE: FALSE,
		XmNscrollHorizontal, FALSE,
		XmNwordWrap, TRUE,
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNuserData, this,
		0);

	XtManageChild(widget);

	if (select_cb)
		set_selection_notifier();

	if (help_msg)
		register_help(widget, label, help_msg);

	if (npages)
		// resizeCB keeps track of number or rows;
		// only needed when we are restricting display
		// to npages
		XtAddEventHandler(text_area, StructureNotifyMask, False,
			(XtEventHandler)resizeCB, (XtPointer)this);
}

// This constructor is called by the Text_display constructor,
// which then creates the text_area widget
Text_area::Text_area(Component *p, const char *name, Command_sender *c,
	Callback_ptr scb, Help_id h) : Component(p, name, c, h)
{
	sel_string = text_content = 0;
	update_content = TRUE;
	select_cb = scb;
	selection_start = selection_size = 0;
}

Text_area::~Text_area()
{
	delete sel_string;
	delete text_content;
}

void
Text_area::set_selection_notifier()
{
	static char btn1_xlations[] = 
		"~c ~s ~m ~a <Btn1Down>: select_start()\n\
		<Btn1Up>: select_end()";
	static XtActionsRec btn1_actions[] = {
		{ "select_start", (XtActionProc)select_start },
		{ "select_end", (XtActionProc)select_end },
	};
	static XtTranslations parsed_btn1_xlations;
	static Boolean btn1_actions_added = FALSE;

	if (!btn1_actions_added)
	{
		XtAppAddActions(base_context, btn1_actions, 
			XtNumber(btn1_actions));
		btn1_actions_added = TRUE;
	}
	if (!parsed_btn1_xlations)
		parsed_btn1_xlations = 
			XtParseTranslationTable(btn1_xlations);
	XtOverrideTranslations(text_area, parsed_btn1_xlations);
}

// append the string to the end of the displayed text
void
Text_area::add_text(const char *s)
{
	if (!s || !*s)
		return;

	update_content = TRUE;
	if (text_data.get_nlines() == 0)
		text_data.reset();
	text_data.add_string(s);

	XmTextPosition	next_position = XmTextGetLastPosition(text_area);
	XmTextInsert(text_area, next_position, (char *)s);

	if (npages && text_data.get_nlines() >= ((npages+1)*nrows))
	{
		// only adjust when we get beyond a full page
		// more than requested to save frequent updates
		// delete bytes 0 to delete_bytes-1
		int	rows_to_delete = text_data.get_nlines() -
			(npages * nrows);
		int delete_bytes = text_data.get_lpos(rows_to_delete+1);

		// remove the first page (lines 1 thru nrows)
		char *string = XmTextGetString(text_area);
		char *new_string = &string[delete_bytes];
		XtVaSetValues(text_area,
			XmNvalue, new_string,
			XmNcursorPosition, strlen(new_string),
			0);
		XtFree(string);
		text_data.delete_first_n_lines(rows_to_delete);
	}
	else
		XmTextSetInsertionPosition(text_area, XmTextGetLastPosition(text_area));
}

// called for resize only when we are managing the size of
// our scroll area
void
Text_area::resize()
{
	short	new_nrows;

	if (npages == 0)
		// shouldn't happen - not managing scroll
		return;
	XtVaGetValues(text_area, 
		XmNrows, &new_nrows,
		0);
	if ((int)new_nrows == nrows)
		// horizontal resize
		return;
	nrows = (int)new_nrows;
}

// Return entire contents.  XmTextGetString allocates the space for
// the string; Text_area is responsible for freeing the space later
char *
Text_area::get_text()
{
	if (update_content)
	{
		delete text_content;
		text_content = XmTextGetString(text_area);
		update_content = FALSE;
	}
	return text_content;
}

char *
Text_area::get_selection()
{
	delete sel_string;
	sel_string = 0;

	if (!selection_size)
		return 0;

	sel_string = new char[selection_size + 1];
	if (XmTextGetSubstring(text_area, selection_start, 
		selection_size, selection_size+1, sel_string)
		!= XmCOPY_SUCCEEDED)
		return 0;
	return sel_string;
}

// blank out text area
void
Text_area::clear()
{
	XmTextSetString(text_area, NULL);
	text_data.reset();
	update_content = TRUE;
}

// Copy the selection to the clipboard
void
Text_area::copy_selection()
{
	XmTextCopy(text_area, XtLastTimestampProcessed(XtDisplay(text_area)));
}

// Call the framework callback.  1 indicates that there is a selection,
// 0 indicates that the previous selection has been deselected.  That is
// indicated by start == end (that is, the start and the end are at the
// same position)
void
Text_area::set_selection(XmTextPosition start, XmTextPosition end)
{
	selection_start = start;
	selection_size = (int)(end - start);

	if (!select_cb)
		return;

	(creator->*select_cb)(this, selection_size ? (void *)1 : (void *)0);
}

void
Text_area::selection_begin(XEvent *e)
{
	// record click time and update multi-clicks;
	// we need to keep track of number of clicks for
	// Text_display, which does special processing for
	// double clicks
	int mclick_time = XtGetMultiClickTime(XtDisplay(text_area));
	DEBUG_TA((stderr, "selection_begin: mclick %d, last %d, button %d\n",
		mclick_time, selection_data.last_time, e->xbutton.time));
	if (selection_data.last_time < e->xbutton.time &&
	    e->xbutton.time - selection_data.last_time <
		(mclick_time == 200 ? 500 : mclick_time))
	{
		// subsequent clicks in a multi-click sequence
		++selection_data.nclicks;
	}
	else
	{
		// single click or first in a multi-click sequence
		selection_data.nclicks = 1;
		selection_data.anchor = XmTextXYToPos(text_area, 
						e->xbutton.x, e->xbutton.y);
	}
	selection_data.last_time = e->xbutton.time;
}

void
Text_area::selection_end(XEvent *)
{
	XmTextPosition left, right;
	if (XmTextGetSelectionPosition(text_area, &left, &right))
		set_selection(left, right);
}

void
Text_area::position(int line)
{
	if (line < 1 || line > text_data.get_nlines())
	{
		if (line != 1)
			// okay to ask to position at beginning
			// of empty window
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	XtVaSetValues(text_area,
		XmNtopCharacter, text_data.get_lpos(line),
		0);
}

int
Text_area::get_current_position()
{
	if (text_data.get_nlines())
		return text_data.get_line(1,
			(int)XmTextGetInsertionPosition(text_area));
	else
		return 0;
}

void
Text_area::set_highlight(int line, Boolean on)
{
	XmTextPosition bpos = (XmTextPosition)text_data.get_lpos(line);
	XmTextPosition epos = (XmTextPosition)text_data.get_epos(line);

	XmTextSetHighlight(text_area, bpos, epos,
		on ? XmHIGHLIGHT_SELECTED : XmHIGHLIGHT_NORMAL);
}

#ifdef SDE_SUPPORT
char *
Text_area::get_current_line()
{
	int curl; 

	if ((curl = get_current_position()) == 0)
	{
		return 0;
	}
	get_text();
	int curpos = text_data.get_lpos(curl);
	int nbytes = text_data.get_epos(curl) - curpos + 1;
	char *lstring = new char [nbytes+1];
	strncpy(lstring, &text_content[curpos], nbytes);
	lstring[nbytes] = 0;
	return lstring;
}

void
Text_area::set_editable(Boolean e)
{
	XtVaSetValues(text_area, 
		XmNeditable, e, 
		XmNcursorPositionVisible, e,
		0);
}
#endif
