#ident	"@(#)debugger:libmotif/common/Sel_list.C	1.13"

#include "UI.h"
#include "Component.h"
#include "Sel_list.h"
#include "Buffer.h"
#include "Vector.h"
#include "str.h"

#include "DND.h"
#include <Xm/ScrolledW.h>
#include <Xm/List.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

//#define	DEBUG_SL(x)	Debug_t(DBG_SL,x)
#define	DEBUG_SL(x)

#define	MAX_COLUMNS	32
#define ROW(p)		((p-1)/columns)
#define ITEM(r, c)	(((r) * columns) + (c))

static void
select_CB(Widget, Selection_list *list, XmListCallbackStruct *ptr)
{
	if (!ptr || !list)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	switch(ptr->reason)
	{
	case XmCR_SINGLE_SELECT:
		list->handle_select(ptr);
		break;
	case XmCR_MULTIPLE_SELECT:
		list->handle_mselect(ptr);
		break;
	case XmCR_DEFAULT_ACTION:
		if (!list->get_select_cb() && 
			!list->get_deselect_cb())
		{
		// Double click is really a select followed by a double
		// select. The selection has the effect of toggling the 
		// selection state of the item in multiple-select mode 
		// and deselecting the selected item in single-select 
		// mode.  To achieve this when no selection callback is
		// registered, we explicitly call the appropriate
		// member functions
			if (list->get_mode() == SM_single)
				list->handle_select(ptr);
			else 
				list->handle_mselect(ptr);
		}
		list->handle_dblselect(ptr);
		break;
	}
}

// handle enter, leave and keyboard events
static void
event_handler(Widget w, Selection_list *ptr, XEvent *event, Boolean *cont)
{
	if (!ptr || !event || !cont)
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
list_select_start(Widget w, XEvent *e, char **params, Cardinal *nparams)
{
	Selection_list	*list = 0;

	XtVaGetValues(w, XmNuserData, &list, 0);
	if (!list)
		return;
	list->clear_search();
	XtCallActionProc(w, "ListBeginSelect", e, params, *nparams);
}

Selection_list::Selection_list(Component *p, const char *s, int vis_rows, int cols,
	const Sel_column *sel_col, int total, const char **ival, Select_mode m, 
	Command_sender *c, Search_type search_type, int sc,
	Callback_ptr sel_cb, Callback_ptr desel_cb,
	Callback_ptr dblsel_cb, Callback_ptr dcb, Help_id h)
	: Component(p, s, c, h)
{
	if (!ival)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	drop_cb = dcb;
	select_cb = sel_cb;
	deselect_cb = desel_cb;
	default_cb = dblsel_cb;

	columns = cols;
	total_rows = total;
	mode = m;

	item_strings = 0;
	pointers = 0;
	search_str = 0;
	selections = 0;

	column_spec = sel_col;

	make_strings(ival);

	widget = XtVaCreateWidget(label,
		xmScrolledWindowWidgetClass, parent->get_widget(),
		XmNuserData, this,
		0);

	list = XtVaCreateManagedWidget(label, xmListWidgetClass, widget,
		XmNitems, item_strings,
		XmNitemCount, total_rows*columns,
		XmNselectionPolicy, (mode == SM_single) ? 
			XmSINGLE_SELECT : XmMULTIPLE_SELECT,
		XmNvisibleItemCount, vis_rows,
		XmNnumColumns, columns,
		XmNlistColumnSpacing, 4,
		XmNlistSizePolicy, XmCONSTANT,
		XmNuserData, this,
		0);

	XtManageChild(widget);

	if (drop_cb)
	{
		dnd = new DND(slist_atom);
		dnd->setup_drag_source(list, 
			(DND_callback_proc)&Selection_list::dnd_drop_cb);
	}
	else
		dnd = 0;

	if (select_cb || deselect_cb)
	{
		if (mode == SM_single)
			XtAddCallback(list, XmNsingleSelectionCallback, 
				(XtCallbackProc)select_CB, (XtPointer)this);
		else
			XtAddCallback(list, XmNmultipleSelectionCallback, 
				(XtCallbackProc)select_CB, (XtPointer)this);
	}
	if (default_cb)
		XtAddCallback(list, XmNdefaultActionCallback, 
			(XtCallbackProc)select_CB, (XtPointer)this);

	last_selection = -1;

	if (search_type != Search_none)
	{
		search_success = FALSE;
		search_column = sc;
		cur_pos = search_column;
		cur_search_len = search_str_len = 0;
		alphabetic = (search_type == Search_alphabetic) ? 
			TRUE : FALSE;
		setup_for_search();
	}

	if (help_msg)
		register_help(widget, label, help_msg);

}

Selection_list::~Selection_list()
{
	delete search_str;
	delete selections;
	free_strings();
	delete dnd;
}

void
Selection_list::dnd_drop_cb(DND_calldata *call)
{
	Sel_list_data	sdata;
	sdata.dropped_on = call->dropped_on;
	sdata.drag_row = ROW(call->item_pos);
	(creator->*drop_cb)(this, &sdata);
}

void
Selection_list::clear_search()
{
	search_success = FALSE;
}

void
Selection_list::setup_for_search()
{
	static char btn1_xlations[] = 
		"<Btn1Down>: list_select_start()";
	static XtActionsRec btn1_actions[] = {
		{ "list_select_start", (XtActionProc)list_select_start }
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
	XtOverrideTranslations(list, parsed_btn1_xlations);
	XtAddEventHandler(list, 
		EnterWindowMask|LeaveWindowMask|KeyPressMask,
		False, (XtEventHandler)event_handler, this);
}

void	*
Selection_list::get_client_data()
{
	return (void *)dnd;
}

// handle SELECT clicks in XmSINGLE_SELECT mode
void
Selection_list::handle_select(XmListCallbackStruct *ptr)
{
	int this_row = ROW(ptr->item_position);

	if (last_selection != this_row)
	{
		if (last_selection >= 0 && deselect_cb)
			(creator->*deselect_cb)(this, 	
				(void *)last_selection);
		if (select_cb)
			(creator->*select_cb)(this, (void *)this_row);
		last_selection = this_row;
	}
	else 
	{
		// selecting a selected row deselects it
		if (deselect_cb)
			(creator->*deselect_cb)(this, (void *)last_selection);
		last_selection = -1;
	}
}

static Boolean
is_selected(int pos, int *sels, int nsels)
{
	for (int i = 0; i < nsels; ++i, sels++)
	{
		if (pos < *sels)
			return FALSE;
		else if (pos == *sels)
			return TRUE;
	}
	return FALSE;
}

// handle SELECT clicks in XmMULTIPLE_SELECT mode
void
Selection_list::handle_mselect(XmListCallbackStruct *ptr)
{
	int this_row = ROW(ptr->item_position);

	if (is_selected(ptr->item_position, ptr->selected_item_positions,
		 	ptr->selected_item_count))
	{
		if (!selections[this_row] && select_cb)
			// first time this row
			(creator->*select_cb)(this, (void *)this_row);
		selections[this_row] += 1;
	}
	else
	{
		selections[this_row] -= 1;
		if (!selections[this_row] && deselect_cb)
			(creator->*deselect_cb)(this, (void *)this_row);
	}
}

void
Selection_list::handle_dblselect(XmListCallbackStruct *ptr)
{
	if (!default_cb)
		return;
	(creator->*default_cb)(this, (void *)ROW(ptr->item_position));
}

void
Selection_list::set_list(int new_total, const char **values)
{
	free_strings();
	delete selections;
	clear_search();
	total_rows = new_total;
	make_strings(values);

	// delselect all items to avoid "automatic"
	// selection on the new list;
	// framework objects expect that set_list cancels current
	// selections
	XmListDeselectAllItems(list);

	XtVaSetValues(list,
		XmNitemCount, total_rows*columns,
		XmNitems, item_strings,
		0);

	// reset last_selection
	last_selection = -1;
}

int
Selection_list::get_selections(Vector *vector)
{
	int	*pos = 0;
	int	npos = 0;
	int	last_row = -1;
	int	nrows = 0;
	Boolean	sel;

	vector->clear();
	sel = XmListGetSelectedPos(list, &pos, &npos);
	for (int i = 0; i < npos; i++)
	{
		int row = ROW(pos[i]);

		if (last_row == row)
			continue;
		last_row = row;
		vector->add(&row, sizeof(int));
		++nrows;
	}
	if (sel)
		XtFree((char *)pos);
	return nrows;
}

// first time for each position get C string from X compound
// string and cache it; use cached string in subsequent accesses.
const char *
Selection_list::get_item(int row, int column)
{
	XmString	xstring;
	char		*string;

	if (row >= total_rows || column >= columns)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;
	}
	int		item = ITEM(row, column);

	if (pointers[item])
		return pointers[item];
	xstring = item_strings[item];
	if (!XmStringGetLtoR(xstring, XmFONTLIST_DEFAULT_TAG, &string))
		return 0;
	pointers[item] = string;
	return string;
}

int
Selection_list::compare_and_set(int pos)
{
	int	cmp;
	char	*string = pointers[pos];

	if (!string)
	{
		if (!XmStringGetLtoR(item_strings[pos], 
			XmFONTLIST_DEFAULT_TAG, &string))
			return -1;
		pointers[pos] = string;
	}
	if ((cmp = strncmp(string, search_str, cur_search_len)) == 0)
	{
		// found item - set selection
		if ((pos == cur_pos) && search_success)
			// same entry
			return 0;
		select(pos+1, TRUE);
		cur_pos = pos;
		search_success = TRUE;
		return 0;
	}
	return cmp;
}

// add c to search buffer and search from current pos - or return
// to beginning if already past item
void
Selection_list::search(char *srch)
{
	int	len = strlen(srch);
	int	total = total_rows * columns;
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
		cur_pos = start_pos = search_column;
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

	int	pos;

	pos = start_pos;
	for(; pos < total; pos += columns)
	{
		if ((cmp = compare_and_set(pos)) == 0)
			// found
			return;
		else if (alphabetic && (cmp > 0))
			break;
	}
	// wrap around and search first part of list,
	// if not yet searched
	for(pos = search_column; pos < start_pos; pos += columns)
	{
		if ((cmp = compare_and_set(pos)) == 0)
			// found
			return;
		else if (alphabetic && (cmp > 0))
			break;
	}
	clear_search();
	beep();
}

void
Selection_list::select(int pos, Boolean notify)
{
	int	top, visible;

	if (notify && (mode == SM_multiple) && search_success)
	{
		// we had a successful search and now have another
		// one - clear old selection
		int	row = cur_pos / columns;
		XmListDeselectPos(list, cur_pos+1);
		if (selections)
		{
			selections[row] -=1;
			if (!selections[row] && deselect_cb)
				(creator->*deselect_cb)(this, (void *)row);
		}
	}
	XmListSelectPos(list, pos, notify);
	XtVaGetValues(list,
		XmNtopItemPosition, &top,
		XmNvisibleItemCount, &visible,
		0);
	if (pos < top)
		XmListSetPos(list, pos);
	else if (pos >= (top+(visible*columns)))
		XmListSetBottomPos(list, pos);
}

void
Selection_list::select(int row)
{
	int	pos = row * columns + 1;
	select(pos, FALSE);
}

void
Selection_list::deselect(int row)
{
	if (row >= 0)
	{
		int pos = row*columns + 1;
		for (int j = 0; j < columns; ++j)
		{
			XmListDeselectPos(list, pos);
			++pos;
		}
	}
	else
	{
		XmListDeselectAllItems(list);
	}
}

void
Selection_list::make_strings(const char **values)
{
	Buffer		*buf;
	unsigned short	csize[MAX_COLUMNS];
	int		i, j;

	if (!total_rows)
	{
		selections = 0;
		item_strings = 0;
		pointers = 0;
		return;
	}
	if (mode == SM_multiple && (select_cb || deselect_cb || default_cb))
	{
		selections = new unsigned char [total_rows];
	}
	// compute column sizes
	for(j = 0; j < columns; ++j)
	{
		csize[j] = column_spec[j].size;
		if (!csize[j] && (column_spec[j].type == SCol_numeric))
		{
			// => max size over all items in column
			int	max_size = 0;
			int	item = j;
			for (i = 0; i < total_rows; ++i, item += columns)
			{
				int item_size = strlen(values[item]);
				if (item_size > max_size)
					max_size = item_size;
			}
			csize[j] = max_size;
		}
	}
	item_strings = new XmString[total_rows*columns];
	pointers = new char*[total_rows*columns];
	buf = buf_pool.get();
	if (selections)
	{
		memset(selections, 0, total_rows*sizeof(*selections));
	}
	int k = 0;
	for(i = 0; i < total_rows; ++i)
	{
		for(j = 0; j < columns; ++j)
		{
			const char	*string = values[k];

			// only fill in pointers as elements are accessed
			// or to save complete string for truncated items
			pointers[k] = 0;
			if (csize[j] != 0)
			{
				int	item_size = strlen(string);
				if ((item_size < csize[j]) &&
					(column_spec[j].type == SCol_numeric))
				{
					// pad on left
					pointers[k] = makestr(string);
					buf->clear();
					int blanks_needed = 
						csize[j] - item_size;
					while (blanks_needed-- > 0)
						buf->add(' ');
					buf->add(string);
					string = (char *)*buf;
				}
				else if (item_size > csize[j])
				{
					// truncate
					pointers[k] = makestr(string);
					buf->clear();
					buf->add(string, csize[j]);
					string = (char *)*buf;
				}
			}
			item_strings[k] = XmStringCreateLocalized((String)string);
			++k;
		}
	}
	buf_pool.put(buf);
}

void
Selection_list::free_strings()
{
	int	items = total_rows*columns;
	for(int i = 0; i < items; ++i)
	{
		XmStringFree(item_strings[i]);
		delete pointers[i];
	}
	delete item_strings;
	delete pointers;
	item_strings = 0;
	pointers = 0;
}

void
Selection_list::set_sensitive(Boolean sense)
{
	XtSetSensitive (list, sense);
}
