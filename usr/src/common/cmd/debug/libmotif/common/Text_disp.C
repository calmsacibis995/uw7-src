#ident	"@(#)debugger:libmotif/common/Text_disp.C	1.20"

// Text_display consists of a scrolled Text widget accompanied by
// a DrawingArea widget for drawing line numbers, arrows and stop
// signs.
// NOTE: currently not implemented are:
// 1) dynamic property changes (color, font etc)

// GUI headers
#include "UI.h"
#include "Toolkit.h"
#include "Component.h"
#include "Text_area.h"
#include "Text_disp.h"
#include "Text_edit.h"
#include "RegExp.h"
#include "Resources.h"
#include "DND.h"

#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/CutPaste.h>

// Debug headers
#include "str.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

//#define	DEBUG_TD(x)	Debug_t(DBG_TD,x)
#define	DEBUG_TD(x)

// keep track of editing changes for undo operations
struct	Text_save {
	XmTextPosition	start_pos;
	XmTextPosition	end_pos;
	int		len;
	char		*text;
			Text_save() { len = 0; text = 0; start_pos = -1; }
};


// ------------------------ Text_display Callbacks ---------------------

static void
resizeCB(Widget, Text_display *pane, XEvent *ev, Boolean *)
{
	if (!pane || !ev)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	if (ev->type == ConfigureNotify)
		pane->resize();
}

// callback when an action is about to happen
// but hasn't yet taken place
static void
text_verifyCB(Widget, Text_display *pane, 
	XmTextVerifyCallbackStruct *ptr)
{
	if (!pane || !ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	if (ptr->reason == XmCR_MOVING_INSERT_CURSOR)
		pane->cursor_motion();
	else if (ptr->reason == XmCR_MODIFYING_TEXT_VALUE)
		pane->text_about_to_change(ptr);
}

// callback after text has actually changed
static void
text_changedCB(Widget, Text_display *pane, XtPointer)
{
	if (!pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	pane->text_changed();
}

static void
scrollCB(Widget, Text_display *pane, XmScrollBarCallbackStruct *)
{
	if (!pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	pane->scroll();
}

// Both the Source and Disassembly windows call the constructor followed
// by either setup_source_file or setup_disassembly, which in turn call
// finish_setup.

Text_display::Text_display(Component *p, const char *name,
	Base_window *b, int nrows, int ncolumns,
	Callback_ptr scb, Callback_ptr tbcb, Command_sender *c, Help_id h)
	: Text_area(p, name, c, scb, h), 
	  Text_editor(p, name, c, b, nrows, ncolumns, tbcb, h)
{
	highlight_start = highlight_end = 0;
	regexp = 0;
	text_save = 0;
	line_map = new int[rows];
	clear_line_map();
	textwin = XtVaCreateWidget("text_win", 
		xmScrolledWindowWidgetClass,
		widget,
		XmNuserData, (Text_area *)this, // this "this" is used by
						// Text_area's selection 
						// mechanism.
						// It must be set on the parent
						// of the Text widget.
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, canvas,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		0);
	if (help_msg)
	{
		register_help(textwin, 0, help_msg);
	}
}

Text_display::~Text_display()
{
	delete regexp;
	delete line_map;
	delete text_save;
}

void
Text_display::setup_source_file()
{
	Boolean	wrap = !resources.get_source_no_wrap();
	text_area = XtVaCreateManagedWidget(label, xmTextWidgetClass,
		textwin, 
		XmNvalue, "",
		XmNcolumns, columns,
		XmNrows, rows,
		XmNuserData, (Component *)this,	// this "this" is used by DND
						// and it must be a Component *
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNeditable, TRUE,
		XmNwordWrap, wrap,
		XmNscrollHorizontal, (wrap == FALSE),
		XmNmarginWidth, 3,
		0);
	// no resize callback!
	XtAddEventHandler(text_area, StructureNotifyMask, False,
		(XtEventHandler)resizeCB, (XtPointer)this);
	XtAddCallback(text_area, XmNmotionVerifyCallback,
		(XtCallbackProc)text_verifyCB, (XtPointer)this);
	XtAddCallback(text_area, XmNmodifyVerifyCallback,
		(XtCallbackProc)text_verifyCB, (XtPointer)this);
	XtAddCallback(text_area, XmNvalueChangedCallback,
		(XtCallbackProc)text_changedCB, (XtPointer)this);

	finish_setup();

	size_canvas(TRUE);
	is_source = TRUE;
	text_save = new Text_save();
}

void
Text_display::setup_disassembly()
{
	text_area = XtVaCreateManagedWidget(label, xmTextWidgetClass, 
		textwin,
		XmNvalue, "",
		XmNrows, rows,
		XmNcolumns, columns,
		XmNuserData, (Component *)this,	// this "this" is used by DND
						// and it must be a Component *
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNeditable, FALSE,
		XmNmarginWidth, 3,
		0);
	XtAddEventHandler(text_area, StructureNotifyMask, False,
		(XtEventHandler)resizeCB, (XtPointer)this);
	XtAddCallback(text_area, XmNmotionVerifyCallback,
		(XtCallbackProc)text_verifyCB, (XtPointer)this);

	finish_setup();
	is_source = FALSE;
}

void
Text_display::finish_setup()
{
	set_selection_notifier();

	Widget 		vsb = 0, hsb = 0;
	Dimension	yspacing = 0;

	XtManageChild(textwin);
	XtManageChild(widget);

	XtVaGetValues(textwin, 
		XmNverticalScrollBar, &vsb, 
		XmNhorizontalScrollBar, &hsb, 
		XmNspacing, &yspacing,
		0);

	// set scroll callbacks
	XtAddCallback(vsb, XmNdragCallback, 
		(XtCallbackProc)scrollCB, (XtPointer)this);
	XtAddCallback(vsb, XmNvalueChangedCallback, 
		(XtCallbackProc)scrollCB, (XtPointer)this);
	XtAddCallback(vsb, XmNincrementCallback, 
		(XtCallbackProc)scrollCB, (XtPointer)this);
	XtAddCallback(vsb, XmNdecrementCallback, 
		(XtCallbackProc)scrollCB, (XtPointer)this);
	XtAddCallback(vsb, XmNpageDecrementCallback, 
		(XtCallbackProc)scrollCB, (XtPointer)this);
	XtAddCallback(vsb, XmNpageIncrementCallback, 
		(XtCallbackProc)scrollCB, (XtPointer)this);

	// baseline is y coordinate of first line of text
	baseline = XmTextGetBaseline(text_area);

	// set the bottom offset for the drawing area
	// just so the widget won't be notified of any button
	// events in the "hole"
	Dimension	hsb_height = 0;
	if (hsb)
		XtVaGetValues(hsb, XmNheight, &hsb_height, 0);
	XtVaSetValues(canvas, 
		XmNbottomOffset, hsb_height+yspacing,
		0);

	// find default font from text widget's fontlist
	xfont = get_default_font(text_area);
	if (!xfont)
	{
		// no font?
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	font_height = FontHeight(xfont);
	dnd = new DND(slist_atom);
	dnd->setup_drop_site(text_area);
}

int
Text_display::save_current(const char *fname)
{
	FILE	*fptr;
	int	len;
	int	ret;

	if ((fptr = fopen(fname, "w")) == 0)
	{
		display_msg(E_ERROR, GE_create_source, fname,
			strerror(errno));
		return 0;
	}
	get_text();
	len = (int)XmTextGetLastPosition(text_area);
	ret = fwrite(text_content, 1, len, fptr);
	fclose(fptr);
	if (ret != len)
	{
		display_msg(E_ERROR, GE_source_file_write, fname,
			strerror(errno));
		::unlink(fname);
		return 0;
	}
	return 1;
}

void
Text_display::clear_line_map()
{
	int	*lp = line_map;
	int	*le = &line_map[rows];
	top_pos = 0;
	top = 1;
	for(; lp < le; lp++)
		*lp = 0;
}

// returns true if line_map has changed;
// line_map maps disply rows to source lines;
// rows containing line number 0 are either continuations
// of the previous source line or go beyond the end of
// the file;  line_map[0] always contains a non-zero
// line number (unless the display is empty) even if it
// does not display the beginning of a line

Boolean
Text_display::update_line_map(XmTextPosition pos)
{
	Boolean		changed = FALSE;
	int		line;
	int		last = text_data.get_nlines();
	Position	last_y = baseline;
	int		*lp = line_map;
	int		*le = &line_map[rows];

	if (pos == -1)
		pos = XmTextGetTopCharacter(text_area);
	top_pos = pos;
	top = line = text_data.get_line(1, (int)pos);
	while(lp < le)
	{
		Position	x, y;
		int		delta;
		if (*lp != line)
		{
			changed = TRUE;
			*lp = line;
		}
		lp++;
		if (line >= last)
		{
			for(; lp < le; lp++)
			{
				if (*lp != 0)
				{
					*lp = 0;
					changed = TRUE;
				}
			}
			return changed;
		}
		if (!XmTextPosToXY(text_area, text_data.get_epos(line), &x, &y))
		{
			// last character of line not in view
			for (; lp < le; lp++)
				*lp = 0;
			return changed;
		}
		delta = (y - last_y)/font_height;
		last_y = y;
		if (delta > 1)
		{
			// source line spans multiple display rows
			for(int i = 1; i < delta && lp < le; 
				i++, lp++)
			{
				if (*lp != 0)
				{
					*lp = 0;
					changed = TRUE;
				}
			}
		}
		line++;
	}
	return changed;
}

void *
Text_display::get_client_data()
{
	return (void *)dnd;
}

void
Text_display::clear_highlight()
{
	if (highlight_start == highlight_end)
		return;
	XmTextSetHighlight(text_area, highlight_start, highlight_end,
		XmHIGHLIGHT_NORMAL);
	highlight_start = highlight_end = 0;
}

void
Text_display::set_highlight(XmTextPosition start, XmTextPosition end)
{
	XmTextSetHighlight(text_area, start, end, XmHIGHLIGHT_SELECTED);
	highlight_start = start;
	highlight_end = end;
}

// set_file is called (indirectly) from the Source window to change 
// the file displayed
int
Text_display::set_file(const char *path)
{

	DEBUG_TD((stderr,"set_file(%s)\n", path));
	clear_breaks();
	update_content = TRUE;
	empty = TRUE;
	current_line = 0;
	clear_canvas();

	if (read_file(path))
	{
		curr_file = makestr(path);
		update_line_map(-1);
		size_canvas();
		empty = FALSE;
		redraw();
		return 1;
	}
	else
	{
		clear_line_map();
		return 0;
	}
}

// set_buf is called from the Disassembly window to display a 
// disassembled function
void
Text_display::set_buf(const char *buf)
{
	clear_breaks();
	clear_canvas();
	if (!buf || !*buf)
	{
		clear();
		return;
	}
	update_content = TRUE;
	empty = FALSE;
	current_line = 0;

	read_buf(buf);
	update_line_map(-1);
}

int
Text_display::get_last_line()
{
	return text_data.get_nlines();
}

// return line position of top character
int
Text_display::get_position()
{
	if (empty)
		return 0;
	return text_data.get_line(1, top_pos);
}

// return display row of current line - -1 if
// current line not visible
int
Text_display::line_to_row(int line)
{
	int	*first, *last;

	if (!line || empty)
		return -1;
	first = line_map;
	last = &line_map[rows-1];

	if (*first > line)
		return -1;
	for(; last > first; last--)
	{
		if (*last != 0)
			break;
	}
	if (*last < line)
		return -1;
	for(; first <= last; first++)
	{
		if (*first == line)
		{
			return(first - line_map);
		}
	}
	return -1;
}

// return line number of last visible line
int
Text_display::last_visible()
{
	int	*last = &line_map[rows-1];

	for(; last >= line_map; last--)
	{
		if (*last != 0)
			return *last;
	}
	return 0;
}

void
Text_display::set_line(int line)
{
	DEBUG_TD((stderr,"set_line(%d)\n",line));
	int	r;
	if (line == current_line)
		return;
	if (current_line)
	{
		if ((r = line_to_row(current_line)) != -1)
			erase_arrow(r);
		clear_highlight();
	}
	current_line = line;

	if (line == 0)
		return;

	if ((r = line_to_row(line)) != -1)
	{
		draw_arrow(r);
	}
	set_highlight(text_data.get_lpos(line),
		text_data.get_epos(line));
}

// position line in the middle of window
void
Text_display::position(int line)
{
	DEBUG_TD((stderr,"position(%d)\n",line));
	if (empty)
		return;

	if (line < 1 || line > text_data.get_nlines())
	{
		display_msg(E_ERROR, GE_src_line, line, text_data.get_nlines());
		return;
	}

	if (line_to_row(line) == -1)
	{
		// set new top
		int	tline = line - rows/2;
		int	pos;
		if (tline < 1)
			tline = 1;
		pos = text_data.get_lpos(tline);
		XmTextSetTopCharacter(text_area, pos);
		clear_canvas();
		update_line_map(pos);
		redraw();
	}

}

// save information for undo'ing change and adjust text_data info
void
Text_display::text_about_to_change(XmTextVerifyCallbackStruct *ptr)
{
	if (empty)
		// called while setting file
		return;

	int	replace_len = ptr->endPos - ptr->startPos;

	text_save->start_pos = ptr->startPos;
	if (ptr->text->length)
	{
		// insertion or replacement
		text_save->end_pos = text_save->start_pos +
				ptr->text->length;
		delete text_save->text;
		if (replace_len)
		{
			// replace - save deleted string
			text_save->len = replace_len;
			text_save->text = new char[replace_len+1];
			XmTextGetSubstring(text_area, ptr->startPos, 
				replace_len, replace_len+1,
				text_save->text);
		}
		else
		{
			// just insertion
			text_save->len = 0;
			text_save->text = 0;
		}
	}
	else
	{
		// deletion
		text_save->end_pos = 0;
		text_save->len = replace_len;
		delete text_save->text;
		text_save->text = new char[replace_len+1];
		XmTextGetSubstring(text_area, ptr->startPos, 
			replace_len, replace_len+1, text_save->text);
	}

	text_data.replace_text((int)ptr->startPos, 
		(int)ptr->endPos, ptr->text->length, ptr->text->ptr);
}

void
Text_display::text_changed()
{
	sfile_changed = update_content = TRUE;
	if (text_data.get_nlines() == 0)
	{
		clear();
		return;
	}
	if (update_line_map(-1))
	{
		clear_canvas();
		size_canvas();
		redraw();
	}
	if (selection_size)
	{
		XmTextPosition	left, right;
		if (!XmTextGetSelectionPosition(text_area, &left, &right))
			// primary selection lost after edit
			set_selection(0, 0);
	}
}

// clear_stop and set_stop have to do bounds checking, since they are
// called from the Source and Disassembly windows,
// instead of from other Text_display functions
void
Text_display::clear_stop(int line)
{
	Breaklist	*bptr = breaks;
	for(; bptr; bptr = bptr->next())
	{
		if (bptr->get_line() == line)
			break;
	}
	if (!bptr)
		return;
	if (bptr == breaks)
		breaks = bptr->next();
	bptr->unlink();
	delete bptr;

	int	r;
	if ((r = line_to_row(line)) != -1)
		erase_stop(r);
}

void
Text_display::set_stop(int line)
{
	Breaklist	*bptr = breaks;
	for(; bptr; bptr = bptr->next())
	{
		if (bptr->get_line() == line)
			break;
	}

	if (!bptr)
	{
		int	r;
		bptr = new Breaklist(line);
		if (breaks)
			bptr->prepend(breaks);
		breaks = bptr;
		if ((r = line_to_row(line)) != -1)
			draw_stop(r);
	}

}

Search_return
Text_display::search(const char *s, int forwards)
{
	if (empty)
		return SR_notfound;

	// if searching for the same string as the previous search,
	// use the previously compiled regular expression string
	if (!search_expr || strcmp(s, search_expr) != 0)
	{
		if (search_expr)
			delete (char *)search_expr;
		else
			// regexp allocated for first search
			// then re-used
			regexp = new RegExp;
		search_expr = makestr(s);
		if (regexp->re_compile(s, 0, 1) == 0)
			return SR_bad_expression;
	}

	get_text();

	XmTextPosition	mbeg, mend;
	Boolean	found = forwards ?  search_forward(&mbeg, &mend)
				 : search_backward(&mbeg, &mend);
	if (!found)
		return SR_notfound;

	// found, set hightlight
	int line = text_data.get_line(1, (int)mbeg);
	position(line);
	clear_highlight();
	set_highlight(mbeg, mend);
	return SR_found;
}

// search forward from the end of the current highlight;
// if no current highlight, search from beginning of current line;
// if no current line, search from first visible character
Boolean
Text_display::search_forward(XmTextPosition *beg, XmTextPosition *end)
{
	int	cpos, start_line, cur_line, lbeg;
	int	last;

	if (highlight_end > 0)
	{
		cpos = highlight_end + 1;
		start_line = cur_line = text_data.get_line(1, cpos);
		lbeg = text_data.get_lpos(cur_line);
	}
	else 
	{
		if (current_line && (line_to_row(current_line) >= 0))
			start_line = cur_line = current_line;
		else
			start_line = cur_line = line_map[0];
		lbeg = text_data.get_lpos(cur_line);
		if (cur_line == line_map[0])
			// line_map[0] does not necessarily display
			// beginning of line
			cpos = top_pos;
		else
			cpos = lbeg;
	}

	last = text_data.get_nlines();

	do {
		char	*bp, *ep;
		int	not_begin;

		not_begin = (lbeg != cpos); 
		// are we starting search at the beginning of a line?
		// regexp needs to know since regular expression
		// can contain "^" to indicate beginning of line
		bp = &text_content[cpos];
		ep = &text_content[text_data.get_epos(cur_line)];
		*ep = 0;
		if (*bp && regexp->re_execute(bp, not_begin))
		{
			*beg = (XmTextPosition)(regexp->get_begin() + cpos);
			*end = (XmTextPosition)(regexp->get_end() + cpos);
			*ep = '\n';
			return TRUE;
		}
		*ep = '\n';
		cur_line++;
		if (cur_line > last)
			cur_line = 1;
		lbeg = cpos = text_data.get_lpos(cur_line);
	} while(cur_line != start_line);
	return FALSE;
}

// search backwards from the beginning of the current highlight;
// if no current highlight search back from beginning of current line;
// if no current line search back from end of current display
Boolean
Text_display::search_backward(XmTextPosition *beg, XmTextPosition *end)
{
	int	cpos, start_line, cur_line, lbeg, last;
	int	so = 0, eo = 0;
	char	*bp, *ep;

	last = text_data.get_nlines();

	// line number to start search with
	if (highlight_end)
	{
		start_line = cur_line = text_data.get_line(1, highlight_start);
		lbeg = text_data.get_lpos(cur_line);
		if (cur_line == line_map[0])
			// line_map[0] does not necessarily display
			// beginning of line
			cpos = top_pos;
		else
			cpos = lbeg;
		bp = &text_content[lbeg];
		ep = &text_content[highlight_start];
	}
	else 
	{
		if (current_line && (line_to_row(current_line) >= 0))
		{
			start_line = current_line;
		}
		else
		{
			start_line = last_visible();
		}
		cur_line = start_line - 1;
		if (cur_line < 1)
			cur_line = last;
		lbeg = text_data.get_lpos(cur_line);
		if (cur_line == line_map[0])
			// line_map[0] does not necessarily display
			// beginning of line
			cpos = top_pos;
		else
			cpos = lbeg;
		bp = &text_content[lbeg];
		ep = &text_content[text_data.get_epos(cur_line)];
	}

	do {
		int	not_begin;
		char	save;
		Boolean	matched = FALSE;

		save = *ep;
		*ep = 0;

		while (bp < ep)
		{
			// find last occurence within current line

			not_begin = (lbeg != cpos);
			// are we starting search at the beginning of
			// a line? regexp needs to know since 
			// regular expression can contain "^" to 
			// indicate beginning of line
			if (regexp->re_execute(bp, not_begin))
			{
				so = cpos + regexp->get_begin();
				eo = cpos + regexp->get_end();
				cpos += regexp->get_end();
				bp = &text_content[cpos];
				matched = TRUE;
			}
			else
				break;
		}
		*ep = save;
		if (matched == TRUE)
		{
			*beg = (XmTextPosition)so;
			*end = (XmTextPosition)eo;
			return TRUE;
		}
		cur_line--;
		if (cur_line < 1)
			cur_line = last;
		lbeg = cpos = text_data.get_lpos(cur_line);
		bp = &text_content[lbeg];
		ep = &text_content[text_data.get_epos(cur_line)];
	} while(cur_line != start_line);
	return FALSE;
}

// cursorMotion is called anytime cursor moves, so we can
// track when KUp, KDown, KPageUp, KPageDown, etc requires
// update to the line numbers
void
Text_display::cursor_motion()
{
	XmTextPosition	pos;

	if (empty)
		return;
	if ((pos = XmTextGetTopCharacter(text_area)) != top_pos)
	{
		update_line_map(pos);
		clear_canvas();
		redraw();
	}
}

// text window is resized, update top line and rows
// and update canvas
void
Text_display::resize()
{
	short 		new_nrows;
	XmTextPosition	topc = 0;

	if (empty)
	{
		clear_line_map();
		clear_canvas();
		return;
	}
	XtVaGetValues(text_area, 
		XmNrows, &new_nrows,
		XmNtopCharacter, &topc,
		0);
	if ((int)new_nrows != rows)
	{
		rows = (int)new_nrows;
		delete line_map;
		line_map = new int[rows];
		clear_line_map();
	}
	update_line_map(topc);
	clear_canvas();
	redraw();
}

// scroll is called when text_area is scrolled vertically.
void
Text_display::scroll()
{
	update_line_map(-1);
	clear_canvas();
	redraw();
}

Boolean
Text_display::read_file(const char *fname)
{
	FILE	*f;
	struct stat statb;
	char	*buf;

	text_data.reset();
	highlight_start = highlight_end = 0;
	if (((f = fopen(fname, "r")) == NULL) ||
		(fstat(fileno(f), &statb) == -1))
	{
		display_msg(E_ERROR, GE_open_source, fname,
			strerror(errno));
		XmTextSetString(text_area, NULL);
		return FALSE;
	}
	if ((statb.st_mode & S_IFMT) != S_IFREG)
	{
		display_msg(E_ERROR, GE_source_file_type, fname);
		XmTextSetString(text_area, NULL);
		fclose(f);
		return FALSE;
	}
	buf = new char[statb.st_size+1];
	if (!fread(buf, 1, statb.st_size, f))
	{
		display_msg(E_ERROR, GE_source_file_read, fname,
			strerror(errno));
		XmTextSetString(text_area, NULL);
		fclose(f);
		delete buf;
		return FALSE;
	}
	buf[statb.st_size] = 0;
	text_data.add_string(buf);
	XmTextSetString(text_area, buf);
	delete buf;
	fclose(f);
	DEBUG_TD((stderr, "read_file: file %s, lines %d, chars %d\n", 
		fname, text_data.get_nlines(), text_data.get_nchars()));
	return TRUE;
}

// read buf into text widget's internal buffer
void
Text_display::read_buf(const char *buf)
{
	text_data.reset();
	highlight_start = highlight_end = 0;
	text_data.add_string(buf);
	XmTextSetString(text_area, (char *)buf);
	DEBUG_TD((stderr, "read_buf: lines %d, chars %d\n", 
		text_data.get_nlines(), text_data.get_nchars()));
}

#define	DELIMITERS " !%^&*()[]<>+-=~|;:{},/#<?\"\n\t\\"

// Motif selects a space separated word on double click; we re-adjust
// to separate on C/C++ punctuation.
void
Text_display::adjust_word(XmTextPosition &start, XmTextPosition &end)
{
	DEBUG_TD((stderr, "adjust_word: anchor %d\n", selection_data.anchor));

	XmTextPosition center, last;
	center = selection_data.anchor;
	get_text();
	last = XmTextGetLastPosition(text_area);
  
	if (strchr(DELIMITERS, text_content[center]) != 0)
		return;

	start = center;
	end = center + 1;
	
	while(start > 0 && !strchr(DELIMITERS, text_content[start-1]))
		start--;
	while(end < last && !strchr(DELIMITERS, text_content[end]))
		end++;
	XmTextSetSelection(text_area, start, end, 0);
}

// Lines are numbered from 1 to n, so 0 indicates no selection 
// (or previous selection has been deselected).  
// That is when start and end both point
// to the same position
void
Text_display::set_selection(XmTextPosition start, XmTextPosition end)
{
	DEBUG_TD((stderr,"set_selection: start %d, end %d nclick %d\n", start, endselection_data.nclicks));
	if (selection_data.nclicks == 2)
		adjust_word(start, end);
	int line;
	selection_start = (int)start;
	selection_size = (int)(end - start);
	if (start < end)
		line = text_data.get_line(top, (int)start);
	else
		line = 0;

	if (!select_cb)
		return;
	(creator->*select_cb)(this, (void *)line);
}

void
Text_display::toggle_bkpt(int x, int y)
{
	XmTextPosition	pos; 
	int		line;

	if (empty)
		return;

	pos = XmTextXYToPos(text_area, 0, y);

	// translate the coordinates into text line number
	line = text_data.get_line(top, (int)pos);
	if (line == 0)
		// translation failed, internal error?
		return;
	DEBUG_TD((stderr, 
		"toggle_bkpt: x %d, y %d, pos %d, line %d, top %d\n", 
		x, y, pos, line, top));
	(creator->*toggle_break_cb)(this, (void *)line);
}

// blank out text area
void
Text_display::clear()
{
	text_data.reset();
	update_content = TRUE;
	sfile_changed = FALSE;
	delete (char *)curr_file;
	curr_file = 0;
	current_line = 0;
	highlight_start = highlight_end = 0;
	empty = TRUE;
	delete (char *)search_expr;
	search_expr = 0;
	selection_size = 0;
	selection_start = 0;
	if (text_save)
		text_save->start_pos = -1;
	XmTextSetString(text_area, NULL);
	clear_canvas();
	clear_line_map();
}

// redraw draws line numbers, arrows, and stop signs in the canvas 
// when the widget is exposed
void
Text_display::redraw()
{
	if (empty)
	{
		clear_canvas();
		return;
	}

	// redraw the entire canvas (we could do better if we know
	// size of exposure)
	// stop at the last visible line or the last line

	for (int row = 0; row < rows; row++)
	{
		// if the text is too short to completely fill the 
		// window,
		// make sure any remaining space is blanked out
		int	line = line_map[row];
		if (!line)
			continue;
		else
		{
			if (is_source)
				draw_number(line, row);
			if (line == current_line)
				draw_arrow(row);
			if (has_breakpoint(line))
				draw_stop(row);
		}
	}
}

void
Text_display::change_directory(const char *)
{
	return;
}

char *
Text_display::get_selected_text()
{
	return get_selection();
}

void
Text_display::copy_selected_text()
{
	copy_selection();
}

void
Text_display::cut_selected_text()
{
	XmTextCut(text_area, 
		XtLastTimestampProcessed(XtDisplay(text_area)));
}

void
Text_display::delete_selected_text()
{
	XmTextRemove(text_area);
}

void
Text_display::paste_text()
{
	XmTextPaste(text_area);
}

void
Text_display::undo_last_change()
{
	if (!text_save || text_save->start_pos == -1)
		return;
	if (text_save->len)
	{
		// undoing a replacement or deletion
		// insert old text or replace existing text
		if (!text_save->end_pos)
		{
			// was a deletion - just insert
			XmTextInsert(text_area, text_save->start_pos,
				text_save->text);
		}
		else
		{
			XmTextReplace(text_area, text_save->start_pos,
				text_save->end_pos, text_save->text);
		}
	}
	else
	{
		// undoing an insertion - just delete
		XmTextReplace(text_area, text_save->start_pos,
			text_save->end_pos, "");
	}
}

Boolean 
Text_display::undo_pending()
{
	return((text_save != 0) &&
		(text_save->start_pos != -1));
}

Boolean
Text_display::paste_available()
{
	unsigned long	len = 0;
	return ((XmClipboardInquireLength(XtDisplayOfObject(text_area),
		XtWindowOfObject(text_area), "STRING", &len)
		== ClipboardSuccess) && (len != 0));
}
