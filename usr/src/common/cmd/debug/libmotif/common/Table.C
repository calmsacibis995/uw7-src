#ident	"@(#)debugger:libmotif/common/Table.C	1.15"

#include "UI.h"
#include "Component.h"
#include "Sel_list.h"
#include "Table.h"
#include "str.h"
#include "Buffer.h"
#include "Label.h"
#include "gui_label.h"
#include "Vector.h"

#include "DND.h"
#include <Xm/ScrolledW.h>
#include <Xm/List.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

// NOTE:  the following features are currently unsupported:
// 1) list overflow 

//#define DEBUG_T(x)	Debug_t(DBG_T,x)
#if DEBUG > 1
#define DEBUG_T(x)	fprintf x
#else
#define DEBUG_T(x)
#endif

#define ROW(n)		(((n)-1)/columns)
#define COL(n,r)	((n)-1-columns*(r))
#define ITEM(r, c)	(((r)+1)*columns+1+(c))
#define ROW_ITEM(r)	(((r)+1)*columns+1)

// Each Item_data struct provides information on 1 row; the
// rows form a singly linked list.
// We save the Motif compound string for each column in
// the row.  If the framework requests the string value,
// we extract it from the compound string and cache the value.
struct Item_data
{
	union
	{
		char		**strings;
		Glyph_type	*glyphs;
	};
	XmString	*xstrings;
	Item_data	*next;
	int		is_set;
			Item_data(int cols);
};

Item_data::Item_data(int cols)
{
	is_set = 0;
	next = 0;
	strings = new char *[cols];
	memset(strings, 0, cols * sizeof(char *));
	xstrings = new XmString[cols];
	memset(xstrings, 0, cols * sizeof(XmString));
}

static char		*blank_str;

struct Envelop_data
{
	XtPointer			save;
	XtConvertSelectionIncrProc	cproc;
	Window				win;
	int				item_index;
};

// used in creating pixmaps for glyphs
struct Picture_data {
	unsigned char	format;
	unsigned char	width;
	unsigned char	height;
	unsigned char	ncolors;
	unsigned char	chars_per_pixel;
	unsigned char	offset_x;
	unsigned char	offset_y;
	Pixmap		pixmap;
	Pixmap		mask;
	const char 	**colors;
	const char 	**pixels;
	const unsigned char *bits;
};

static const char *hand_colors[] = {
" " , "#FFFFFFFFFFFF",
"." , "#000000000000",
"X" , "#FFFFAAAA0000"
} ;
static const char *hand_pixels[] = {
"    .      ",
"   ..      ",
"  ........ ",
" .XXXXXXXX.",
"..XXX..... ",
".XXXXXX.   ",
".XXXX...   ",
"..XXXX.    ",
"  .....    ",
"           ",
"           "
} ;

static const unsigned char hand_bits[] = {
   0x10, 0xf8, 0x18, 0xf8, 0xfc, 0xfb, 0xfe, 0xff, 0xff, 0xfb, 0xff, 0xf8,
   0xff, 0xf8, 0x7f, 0xf8, 0x7c, 0xf8, 0x00, 0xf8, 0x00, 0xf8};

static const char *pin_colors[] = {
" " , "#FFFFFFFFFFFF",
"." , "#861786178617",
"X" , "#000000000000",
"o" , "#FFFFFFFF0000"
} ;
static const char *pin_pixels[] = {
"           ",
"     .XX   ",
"   ...ooX  ",
"  .o.oooX  ",
" .oo.ooXX  ",
" .oooXXX   ",
" XooooXX   ",
"  XooXX    ",
" X XXX     ",
"X          ",
"           "
} ;
static const unsigned char pin_bits[] = {
   0x00, 0x00, 0xe0, 0x00, 0xf8, 0x01, 0xfc, 0x01, 0xfe, 0x01, 0xfe, 0x00,
   0xfe, 0x00, 0x7c, 0x00, 0x3a, 0x00, 0x01, 0x00, 0x00, 0x00};

static const char *touch_colors[] = {
" " , "#FFFFFFFFFFFF",
"." , "#000000000000",
"X" , "#FFFFAAAA0000"
} ;
static const char *touch_pixels[] = {
"     ..    ",
"     ...   ",
" .....XX.  ",
".XXXXXXXX..",
"......XXXX.",
".XXXXXXXXX.",
" .....XXXX.",
" .XXXXXXX..",
"  ........ ",
"           ",
"           "
} ;
static const unsigned char touch_bits[] = {
   0x60, 0x00, 0xe0, 0x00, 0xfe, 0x01, 0xff, 0x07, 0xff, 0x07, 0xff, 0x07,
   0xfe, 0x07, 0xfe, 0x07, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00};

static const char *check_colors[] = {
" " , "#FFFFFFFFFFFF",
"." , "#000000000000",
} ;
static const char *check_pixels[] = {
"          .",
"         ..",
"        .. ",
"       ..  ",
" ..   ..   ",
".... ...   ",
" ......    ",
"  .....    ",
"   ...     ",
"    ..     ",
"           "
} ;
static const unsigned char check_bits[] = {
   0x00, 0x04, 0x00, 0x06, 0x00, 0x03, 0x80, 0x01, 0xc6, 0x00, 0xef, 0x00,
   0x7e, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x30, 0x00, 0x00, 0x00};

// must follow order of Glyph_type enumeration
static Picture_data	picture_data[] = {
	{ 1, 11, 11, 3, 1, 1, 3, None, None,
		hand_colors, hand_pixels, hand_bits },
	{ 1, 11, 11, 4, 1, 1, 3, None, None, 
		pin_colors, pin_pixels, pin_bits },
	{ 1, 11, 11, 3, 1, 1, 3, None, None, 
		touch_colors, touch_pixels, touch_bits },
	{ 1, 11, 11, 2, 1, 1, 3, None, None, 
		check_colors, check_pixels, check_bits },
};

static void
table_select_CB(Widget, Table *table, XmListCallbackStruct *ptr)
{
	if (!ptr || !table)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	switch(ptr->reason)
	{
	case XmCR_SINGLE_SELECT:
		table->handle_select(ptr->item_position);
		break;
	case XmCR_MULTIPLE_SELECT:
		table->handle_mselect(ptr);
		break;
	case XmCR_DEFAULT_ACTION:
		if (!table->get_select_cb() && 
			!table->get_deselect_cb())
		{
		// Double click is really a select followed by a double
		// select. The selection has the effect of toggling the 
		// selection state of the item in multiple-select mode 
		// and deselecting the selected item in single-select 
		// mode.  To achieve this when no selection callback is
		// registered, we explicitly call the appropriate
		// member functions
			if (table->get_mode() == SM_single)
				table->handle_select(ptr->item_position);
			else 
				table->handle_mselect(ptr);
		}
		table->dblselect(ptr->item_position);
		break;
	}
}

// called when an item gets initiallized to a null pointer;
// this is a USG extension to the Motif List to allow setting
// a List item to a glyph
static void
table_item_init_proc(Widget, Table *table, XmListItemInitCallbackStruct *cd)
{
	if (!table || !cd)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	table->set_glyph(cd);
}

// handle enter, leave and keyboard events
static void
event_handler(Widget w, Table *ptr, XEvent *event, Boolean *cont)
{
	if (!ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	*cont = TRUE;
	switch(event->type)
	{
	case EnterNotify:
		// pointer over list - establish keyboard grab
		XtGrabKeyboard(w, False, GrabModeAsync, GrabModeAsync,
			event->xcrossing.time);
		break;
	case LeaveNotify:
		XtUngrabKeyboard(w, event->xcrossing.time);
		break;
	case KeyPress:
	{
		char	string[MB_LEN_MAX+1];
		int	i;
		i = XLookupString(&event->xkey, string, MB_LEN_MAX,
			0, 0);
		if (!i)
			return;
		string[i] = 0;
		ptr->search(string);
		break;
	}
	default:
		break;
	}
}

// trap button 1 press - clear search string, then continue
// with normal selection
static void
table_select_start(Widget w, XEvent *e, char **params, Cardinal *nparams)
{
	Table	*table = 0;

	XtVaGetValues(w, XmNuserData, &table, 0);
	if (!table)
		return;
	table->clear_search();
	XtCallActionProc(w, "ListBeginSelect", e, params, *nparams);
}

Table::Table(Component *p, const char *name, Select_mode m, 
	const Column *cspec,
	int c, int vis, Boolean /*wrap*/, 
	Search_type search_type, int sc,
	Callback_ptr scb, Callback_ptr cscb,
	Callback_ptr ucb, Callback_ptr dscb, Callback_ptr dcb, 
	Command_sender *cr, Help_id h) : Component(p, name, cr, h)
{
	mode = m;
	columns = c;
	rows = 0;
	select_cb = scb;
	col_select_cb = cscb;
	deselect_cb = ucb;
	default_cb = dscb;
	drop_cb = dcb;
	sensitive = FALSE;
	item_data = last_row = 0;
	delay = FALSE;
	last_selection = 0;
	search_str = 0;

	column_spec = cspec;

	if (!blank_str)
		blank_str = makestr("");

	widget = XtVaCreateWidget(label, 
		xmScrolledWindowWidgetClass,
		parent->get_widget(), 
		XmNuserData, this,
		0);

	// make title items
	// we create the title plus 1 initial row of '_' characters
	// to force the initial width of the table to be
	// the right size; we then immediately remove this
	// initial row
	XmString	*items = make_title();
	int		initial_items = columns *2; 

	input = XtVaCreateManagedWidget(label, xmListWidgetClass, 
		widget,
		XmNvisibleItemCount, vis+1,
		XmNselectionPolicy, (mode == SM_single) ? 
			XmSINGLE_SELECT : XmMULTIPLE_SELECT,
		XmNitems, items,
		XmNitemCount, initial_items,
		XmNstaticRowCount, 1,
		XmNnumColumns, columns,
		XmNlistColumnSpacing, 4,
		XmNlistSizePolicy, XmCONSTANT,
		XmNuserData, this,
		0);
	for(int j = 0; j < initial_items; j++)
		XmStringFree(items[j]);
	delete items;

	XtManageChild(widget);

	// add callback to handle glyphs
	XtAddCallback(input, XmNitemInitCallback, 
		(XtCallbackProc)table_item_init_proc, (XtPointer)this);

	if (select_cb || deselect_cb)
	{
		if (mode == SM_single)
			XtAddCallback(input, XmNsingleSelectionCallback, 
				(XtCallbackProc)table_select_CB, (XtPointer)this);
		else
			XtAddCallback(input, XmNmultipleSelectionCallback,
				(XtCallbackProc)table_select_CB, (XtPointer)this);
	}
	if (default_cb)
	{
		XtAddCallback(input, XmNdefaultActionCallback,
			(XtCallbackProc)table_select_CB, (XtPointer)this);
	}
	dnd = new DND(table_atom);
	dnd->setup_drop_site(input);
	if (drop_cb)
		dnd->setup_drag_source(input, (DND_callback_proc)&Table::dnd_drop_cb);

	// free initial sizing row
	XtSetSensitive(input, TRUE);
	XmListDeleteItemsPos(input, columns, columns+1);
	XtSetSensitive(input, FALSE);

	if (search_type != Search_none)
	{
		search_column = sc;
		search_success = FALSE;
		cur_pos = search_column;
		cur_search_len = search_str_len = 0;
		alphabetic = (search_type == Search_alphabetic) ? 
			TRUE : FALSE;
		setup_for_search();
	}

	if (help_msg)
		register_help(input, 0, help_msg);
}

Item_data *
Table::get_row(int row)
{
	register int	i = 0;
	Item_data	*ip = item_data;

	for(; (i < row) && ip; i++)
		ip = ip->next;
	return ip;
}

// insert new_row as row in list
void
Table::insert_row(int row, Item_data *new_row)
{
	if (row == rows)
	{
		// adding at end - most common case
		if (last_row)
			last_row->next = new_row;
		else
			item_data = new_row;
		last_row = new_row;
		return;
	}

	Item_data *prev = 0;
	Item_data *cur = item_data;

	for(int i = 0; cur && (i < row); i++)
	{
		prev = cur;
		cur = cur->next;
	}
	if (cur)
		new_row->next = cur;
	if (prev)
		prev->next = new_row;
	else
		item_data = new_row;
}

// delete 1 row
void
Table::delete_row(Item_data *row)
{
	const Column	*cspec = column_spec;
	char		**ptr = row->strings;
	XmString	*xptr = row->xstrings;
	for (int i = 0; i < columns; i++, ++cspec)
	{
		if (cspec->column_type != Col_glyph)
		{
			delete ptr[i];
			if (xptr[i])
				XmStringFree(xptr[i]);
		}
	}
	delete row->strings;
	delete row->xstrings;
	delete row;
}

// delete total rows starting at start
void
Table::delete_nrows(int start, int total)
{
	Item_data *prev = 0;
	Item_data *cur = item_data;

	for(int i = 0; cur && (i < start); i++)
	{
		prev = cur;
		cur = cur->next;
	}
	if (!cur)
		return;
	for(int j = 0; cur && (j < total); j++)
	{
		Item_data	*nxt = cur->next;
		delete_row(cur);
		cur = nxt;
	}

	if (prev)
		prev->next = cur; // could be 0	
	else
		item_data = cur;

	if (!cur)
		last_row = prev;
}

// called by DND on successful drop
void
Table::dnd_drop_cb(DND_calldata *cdata)
{
	Table_calldata	tdata;

	tdata.dropped_on = cdata->dropped_on;
	tdata.index = ROW(cdata->item_pos - columns);
	tdata.col = 0;
	(creator->*drop_cb)(this, (void *)&tdata);
}

void *
Table::get_client_data()
{
	return (void *)dnd;
}

Table::~Table()
{
	delete search_str;
	delete_nrows(0, rows);
	delete dnd;
}

void
Table::set_glyph(XmListItemInitCallbackStruct *cd)
{
	Item_data	*ip;
	Glyph_type	gt;
	Picture_data	*pic;

	int item_pos = cd->position - columns;	// ignore titles
	int row = ROW(item_pos);
	int col = COL(item_pos, row);

	if (!item_data || column_spec[col].column_type != Col_glyph)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Display	*dpy = XtDisplay(input);
	Screen	*screen = XtScreen(input);
	ip = get_row(row);

	// if glyph column was selected,
	// setting glyph will clear selection
	if (mode == SM_single)
	{
		if (last_selection == item_pos)
		{
			handle_select(cd->position);
		}
	}
	else if (ip->is_set)
	{
		// row is selected - if glyph position is selected
		// decrement selection count
		if (XmListPosSelected(input, cd->position))
			ip->is_set--;
		if (!ip->is_set && deselect_cb)
			// last deselection in this row
			(creator->*deselect_cb)(this, (void *)row);
	}

	cd->label = 0;
	cd->glyph_pos = XmGLYPH_ON_LEFT;
	cd->static_data = TRUE; 

	gt = ip->glyphs[col];
	if (gt >= Gly_blank)
	{
		cd->pixmap = None;
		return;
	}
	pic = &picture_data[(int)gt];
	if (pic->pixmap == None)
	{
		pic->pixmap = XCreatePixmapFromData(
			dpy, RootWindowOfScreen(screen),
			DefaultColormapOfScreen(screen),
			pic->width, pic->height,
			DefaultDepthOfScreen(screen),
			pic->ncolors, pic->chars_per_pixel,
			(char **)pic->colors, (char **)pic->pixels);
		pic->mask = XCreateBitmapFromData(
			dpy, RootWindowOfScreen(screen),
			(const char *)pic->bits, pic->width,
				pic->height);
	}
	cd->pixmap = pic->pixmap;
	cd->mask = pic->mask;
	cd->depth = DefaultDepthOfScreen(screen);
	cd->width = pic->width;
	cd->height = pic->height;
	cd->h_pad = pic->offset_x;
	cd->v_pad = pic->offset_y;
} 

// selection in single mode 
void 
Table::handle_select(int pos) 
{
	Table_calldata	tdata; 
	pos -= columns; // ignore titles 

	DEBUG_T((stderr, "select: pos %d, last %d\n", pos, last_selection)); 
	if (last_selection > 0)
	{
		// deselect old item
		if (deselect_cb)
		{
			int index = ROW(last_selection);
			(creator->*deselect_cb)(this, (void *)index);
		}
		if (last_selection == pos)
		{
			// just deselecting
			last_selection = -1;
			return;
		}
	}
	if (select_cb)
	{
		//select
		tdata.index = ROW(pos);
		tdata.col = COL(pos, tdata.index);
		tdata.dropped_on = 0;
		(creator->*select_cb)(this, (void *)&tdata);
	}
	last_selection = pos;
}

static Boolean
is_selected(int pos, int *sels, int nsel)
{
	for(int i = 0; i < nsel; ++i, sels++)
	{
		if (pos < *sels)
			return FALSE;
		else if (pos == *sels)
			return TRUE;
	}
	return FALSE;
}

// selection in multiple mode
void
Table::handle_mselect(XmListCallbackStruct *cd)
{
	Table_calldata	tdata;
	int		pos = cd->item_position - columns;
	int		this_row = ROW(pos);
	Item_data	*ip = get_row(this_row);

	tdata.index = this_row;
	tdata.col = COL(pos, this_row);
	tdata.dropped_on = 0;
	DEBUG_T((stderr, "mselect: pos %d, row %d, ", 
		cd->item_position, this_row));
	// if this item is in the selected list, then this is a
	// selection operation, otherwise this is a deselection
	if (is_selected(cd->item_position, cd->selected_item_positions, 
	                 cd->selected_item_count))
	{
		// select
		DEBUG_T((stderr, "select\n"));
		if (!ip->is_set)
		{
			if (select_cb)
			// first selection in this row
				(creator->*select_cb)(this, (void *)&tdata);
		}
		else if (col_select_cb)
		// multiple columns within same row selected
			(creator->*col_select_cb)(this, (void *)&tdata);
		++ip->is_set;
	}
	else
	{
		// deselect
		DEBUG_T((stderr, "deselect\n"));
		--ip->is_set;
		if (!ip->is_set && deselect_cb)
			// last deselection in this row
			(creator->*deselect_cb)(this, (void *)tdata.index);
	}
}

void
Table::dblselect(int pos)
{
	if (!default_cb)
		return;
	Table_calldata	tdata;
	pos -= columns;
	tdata.index = ROW(pos);
	tdata.col = COL(pos, tdata.index);
	tdata.dropped_on = 0;
	(creator->*default_cb)(this, (void *)&tdata);
}

void
Table::delay_updates()
{
	set_sensitive(FALSE);
	delay = TRUE;
	old_rows = rows;
}

void
Table::set_row(Item_data *ip, va_list ap)
{
	char		**ptr = ip->strings;
	XmString	*xptr = ip->xstrings;
	const Column	*cspec = column_spec;

	for (int i = 0; i < columns; i++, ++cspec)
	{
		if (cspec->column_type == Col_glyph)
		{
			ptr[i] = (char *)va_arg(ap, Glyph_type);
			xptr[i] = 0;
		}
		else
		{
			char *s = va_arg(ap, char *);
			delete ptr[i]; // reset by make_item_string
			if (xptr[i])
				XmStringFree(xptr[i]);
			xptr[i] = make_item_string(s, cspec, &ptr[i]);
		}
	}
}

void
Table::insert_row(int row ...)
{
	Item_data	*new_row;
	va_list		ap;

	if (row > rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	new_row = new Item_data(columns);
	va_start(ap, row);
	set_row(new_row, ap);
	va_end(ap);

	insert_row(row, new_row);
	rows++;
	if (!delay)
	{
		clear_search();
		set_sensitive(TRUE);
		XmListAddItemsUnselected(input, new_row->xstrings,
			columns, ROW_ITEM(row));
	}
}

void
Table::delete_rows(int row, int total)
{
	if (row + total > rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	rows -= total;
	delete_nrows(row, total);

	if (!delay)
	{
		clear_search();
		XmListDeleteItemsPos(input, total*columns, ROW_ITEM(row));
		if (!rows)
			set_sensitive(FALSE);
	}
	if (!rows)
	{
		// delete all rows
		item_data = last_row = 0;
	}
}

void
Table::set_row(int row ...)
{
	va_list 	ap;
	Item_data	*ip;

	if (row >= rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	ip = get_row(row);
	va_start(ap, row);
	set_row(ip, ap);
	va_end(ap);

	if (!delay)
	{
		// synchronize deselection
		set_sensitive(TRUE);
		clear_search();
		deselect(row);
		XmListReplaceItemsPosUnselected(input, ip->xstrings,
			columns, ROW_ITEM(row));
	}
}

// copy XmStrings for each row into 1 big array for XtVaSetValues
XmString *
Table::make_strings()
{
	XmString	*strings = new XmString[rows*columns];
	XmString	*sp = strings;
	Item_data	*ip;

	for (ip = item_data; ip; ip = ip->next)
	{
		memcpy(sp, (char *)ip->xstrings,
			sizeof(XmString) * columns);
		sp += columns;
	}
	return strings;
}

XmString
Table::make_item_string(const char *s, const Column *cspec, char **out)
{
	XmString	xstring;
	char		*string = s ? (char *)s : blank_str;
	int		slen = strlen(s);
	int 		padding = cspec->size - slen;
	Buffer		*buf = 0;

	// we only save original string for items that are
	// to be padded or truncated - this is so we pass
	// the original to utility functions that need to
	// process them; for non-padded/truncated strings, we
	// recreate the original from the XmString
	if ((padding > 0) && (cspec->column_type == Col_numeric))
	{
		// pad on left 
		*out = makestr(string);
		buf = buf_pool.get();
		buf->clear();
		for(int i = 0; i < padding; ++i)
			buf->add(' ');
		buf->add(string);
		string = (char *)*buf;
	}
	else if (padding < 0)
	{
		*out = makestr(string);
		buf = buf_pool.get();
		buf->clear();
		if (cspec->column_type == Col_right_text)
		{
			// truncate on left, assume cspec->size > 2
			const char *p = &string[slen - cspec->size] + 2;
			buf->add("..");
			buf->add(p);
			string = (char *)*buf;
		}
		else
		{
			// truncate on right
			buf->add(string, cspec->size);
			string = (char *)*buf;
		}
	}
	else
		*out = 0;
	xstring = XmStringCreateLocalized(string);
	if (buf)
		buf_pool.put(buf);
	return xstring;
}

void
Table::set_cell(int row, int col, Glyph_type g_type)
{
	Item_data	*ip;
	if (row >= rows || col >= columns || 
		column_spec[col].column_type != Col_glyph)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	ip = get_row(row);
	ip->glyphs[col] = g_type;
	if (!delay)
	{
		XmString null_string = 0;

		// the following replace will cause the 
		// itemInitCallback to be invoked
		set_sensitive(TRUE);
		XmListReplaceItemsPosUnselected(input, &null_string, 1, 
			ITEM(row, col));
	}
}

void
Table::set_cell(int row, int col, const char *s)
{
	Item_data	*xptr;
	XmString	*xstr;

	if (row >= rows || col >= columns ||
		column_spec[col].column_type == Col_glyph)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	xptr = get_row(row);
	delete xptr->strings[col];
	xstr = &xptr->xstrings[col];

	if (*xstr)
		XmStringFree(*xstr);
	*xstr = make_item_string(s, &column_spec[col], 
		&xptr->strings[col]);

	if (!delay)
	{
		set_sensitive(TRUE);
		XmListReplaceItemsPos(input, xstr, 1, 
			ITEM(row, col));
	}
}

Cell_value
Table::get_cell(int row, int col)
{
	Cell_value	value;
	Item_data	*ip;

	if (row >= rows || col >= columns)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		value.string = 0;
		return value;
	}

	ip = get_row(row);
	if (column_spec[col].column_type == Col_glyph)
		value.glyph = ip->glyphs[col];
	else
	{
		if (!ip->strings[col])
		{
			char	*string;
			XmStringGetLtoR(ip->xstrings[col],
				XmFONTLIST_DEFAULT_TAG, &string);
			ip->strings[col] = string;
		}
		value.string = ip->strings[col];
	}
	return value;
}

void
Table::clear()
{
	if (rows)
	{
		set_sensitive(TRUE);
		// delete everything except titles
		XmListDeleteItemsPos(input, rows*columns, columns+1);
		set_sensitive(FALSE);
		delete_nrows(0, rows);
		item_data = last_row = 0;
		rows = 0;
		delay = FALSE;
	}
}

void
Table::set_sensitive(Boolean s)
{
	if (sensitive != s)
	{
		XtSetSensitive(input, s);
		sensitive = s;
	}
}

void
Table::finish_updates()
{
	clear_search();
	set_sensitive(TRUE);
	if (rows != old_rows)
	{
		int old_nitems = 0;
		XtVaGetValues(input, XmNitemCount, &old_nitems, 0);
		if (old_nitems > columns)
			XmListDeleteItemsPos(input, old_nitems-columns,
				columns+1);
		if (rows)
		{
			XmString	*strings = make_strings();
			XmListAddItemsUnselected(input, strings, 
				rows*columns, columns+1);
			delete strings;
		}
	}
	else if (rows)
	{
		// just replace new values for old
		// make compound strings
		XmString	*strings = make_strings();
		XmListReplaceItemsPosUnselected(input, strings,
			rows*columns, columns+1);
		delete strings;
	}
	else
	{
		// empty
		set_sensitive(FALSE);
	}
	delay = FALSE;
}

int
Table::get_selections(Vector *v)
{
	int	*pos = 0;
	int	npos = 0;
	int	last_row = -1;
	int	nrows = 0;
	Boolean	sel;

	v->clear();
	sel = XmListGetSelectedPos(input, &pos, &npos);
	for (int i = 0; i < npos; i++)
	{
		int row = ROW(pos[i]-columns); // ignore titles

		if (last_row == row)
			continue;
		last_row = row;
		v->add(&row, sizeof(int));
		++nrows;
	}
	if (sel)
		XtFree((char *)pos);
	return nrows;
}

void
Table::deselect(int row)
{
	if (mode == SM_single)
	{
		if (last_selection != row)
			return;
		deselect_row(row);
		last_selection = -1;
	}
	else
	{
		// mode == SM_multiple
		Item_data	*ip = get_row(row);
		if (!ip->is_set)
			return;
		deselect_row(row);
		ip->is_set = 0;
	}
	if (deselect_cb && creator)
		(creator->*deselect_cb)(this, (void *)row);
}

void
Table::deselect_row(int row)
{
	int pos = ROW_ITEM(row);
	for (int j = 0; j < columns; ++j, ++pos)
	{
		if (XmListPosSelected(input, pos))
		{
			XmListDeselectPos(input, pos);
		}
	}
}

void
Table::deselect_all()
{
	XmListDeselectAllItems(input);
	if (mode == SM_single)
		last_selection = -1;
	else
	{
		Item_data	*ip = item_data;
		for(; ip; ip = ip->next)
			ip->is_set = 0;
	}
}

// we create title plus one initial row that forces the initial width
// of the table to be correct
XmString *
Table::make_title()
{
	Buffer		*title_buf = buf_pool.get();
	const Column	*cspec;
	int		i;
	XmString	*title = new XmString[columns*2];

	for (i = 0, cspec = column_spec; i < columns; ++i, ++cspec)
	{
		const char	*heading;
		int blanks_needed = cspec->size;

		title_buf->clear();
		if (cspec->heading != LAB_none)
		{
			heading = labeltab.get_label(cspec->heading);
			title_buf->add(heading);
			blanks_needed -= strlen(heading);
		}
		while (blanks_needed-- > 0)
			title_buf->add(' ');
		title[i] = XmStringCreateLocalized((char *)*title_buf);

		title_buf->clear();
		blanks_needed = cspec->size;
		while (blanks_needed-- > 0)
			title_buf->add('_');
		title[i+columns] = XmStringCreateLocalized((char *)*title_buf);
	}
	buf_pool.put(title_buf);
	return title;
}

void
Table::wrap_columns(Boolean)
{
	// column wrap is not currently supported
	// framework code should use the select_cb mechanism to
	// display the full text of a cell in a footer or something
}

void
Table::show_border()
{
	XtVaSetValues(widget, XmNborderWidth, 1, 0);
}

void
Table::setup_for_search()
{
	static char btn1_xlations[] = 
		"<Btn1Down>: table_select_start()";
	static XtActionsRec btn1_actions[] = {
		{ "table_select_start", (XtActionProc)table_select_start }
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
	XtOverrideTranslations(input, parsed_btn1_xlations);
	XtAddEventHandler(input, 
		EnterWindowMask|LeaveWindowMask|KeyPressMask,
		False, (XtEventHandler)event_handler, this);
}

void
Table::clear_search()
{
	search_success = FALSE;
}

// select an item - scroll it into view and highlight
void
Table::select(int pos)
{
	int	top, visible;

	if ((mode == SM_multiple) && search_success)
	{
		// we had a successful search and now have another
		// one - clear old selection
		Item_data	*ip;
		int	cpos = ITEM(cur_pos, search_column);
		XmListDeselectPos(input, cpos);
		ip = get_row(cur_pos);
		ip->is_set--;
		if (!ip->is_set && deselect_cb)
			(creator->*deselect_cb)(this, (void *)cur_pos);
	}
	XmListSelectPos(input, pos, TRUE);
	XtVaGetValues(input,
		XmNtopItemPosition, &top,
		XmNvisibleItemCount, &visible,
		0);
	if (pos < top)
		XmListSetPos(input, pos);
	else if (pos >= (top+(visible*columns)))
		XmListSetBottomPos(input, pos);
}

int
Table::compare_and_set(Item_data *ip, int row)
{
	int	cmp;
	char	*string = ip->strings[search_column];

	if (!string)
	{
		XmStringGetLtoR(ip->xstrings[search_column],
			XmFONTLIST_DEFAULT_TAG, &string);
		ip->strings[search_column] = string;
	}
	if ((cmp = strncmp(string, search_str, cur_search_len)) == 0)
	{
		// found item - set selection
		if ((row == cur_pos) && search_success)
			// same entry
			return 0;

		int	pos = ITEM(row, search_column);
		select(pos);
		cur_pos = row;
		search_success = TRUE;
		return 0;
	}
	return cmp;
}

// add c to search buffer and search from current pos - or return
// to beginning if already past item
void
Table::search(char *srch)
{
	int	len = strlen(srch);
	int	cmp;
	int	start_pos;

	if (search_success)
	{
		// start from last found entry
		start_pos = cur_pos;
	}
	else
	{
		// start fresh
		cur_pos = start_pos = 0;
		if (search_str)
			*search_str = 0;
		cur_search_len = 0;
	}

	cur_search_len += len;
	if ((cur_search_len) >= search_str_len)
	{
		char	*new_str;
		search_str_len += len + 100;
		new_str = new char[search_str_len];
		strcpy(new_str, search_str);
		delete search_str;
		search_str = new_str;
	}
	strcat(search_str, srch);

	int		pos = start_pos;
	Item_data	*ip = get_row(pos);
	for(; ip; ip = ip->next, pos++)
	{
		if ((cmp = compare_and_set(ip, pos)) == 0)
			// found
			return;
		else if (alphabetic && (cmp > 0))
			break;
	}
	// wrap around and search first part of list,
	// if not yet searched
	ip = item_data;
	for(pos = 0; pos < start_pos; pos++, ip = ip->next)
	{
		if ((cmp = compare_and_set(ip, pos)) == 0)
			// found
			return;
		else if (alphabetic && (cmp > 0))
			break;
	}
	clear_search();
	beep();
}
