#ifndef	_SEL_LIST_H
#define	_SEL_LIST_H
#ident	"@(#)debugger:gui.d/common/Sel_list.h	1.11"

#include "Component.h"
#include "Sel_listP.h"

// A Selection list is a scrolling list of selectable items.
// Items are numbered from 0 to n-1

// Framework callbacks:
// The select callback is called whenever an item in the list is selected.
// The deselect callback is called whenever a selected item is unselected.
// The default callback is called whenever an item is dblselected.
// All three callbacks are invoked as
//	creator->callback((Selection_list *)this, int item_index);
//
// The drop callback is called whenever an item in the list is dragged to
// and dropped on another window.  The callback is invoked as
//	creator->callback((Selection_list *)this, Sel_list_data *);

struct Sel_list_data {
	Component	*dropped_on;
	int		drag_row;
};

// turn off/on searching in list - Search_alphabetic gives
// hint that list is ordered and leads to faster searches
enum Search_type {
	Search_none,
	Search_alphabetic,
	Search_random,
};

enum Sel_column_type
{
	SCol_text,
	SCol_wrap_text,
	SCol_numeric,
};

struct Sel_column
{
	Sel_column_type	type;
	unsigned char	size;
};

class Vector;

class Selection_list : public Component
{
	SELECTION_LIST_TOOLKIT_SPECIFICS
private:
	Callback_ptr	select_cb;
	Callback_ptr	deselect_cb;
	Callback_ptr	default_cb;
	Callback_ptr	drop_cb;
	Select_mode	mode;

public:
			Selection_list(Component *parent, const char *name,
				int rows, int columns, const Sel_column *,
				int items, const char **ival, Select_mode,
				Command_sender *creator = 0,
				Search_type = Search_none,
				int search_column = 0,
				Callback_ptr select_cb = 0,
				Callback_ptr deselect_cb = 0,
				Callback_ptr default_cb = 0,
				Callback_ptr drop_cb = 0,
				Help_id help_msg = HELP_none);
			~Selection_list();

			// update list
	void		set_list(int rows, const char **values);
	void		select(int row);

			// deselect(-1) will deselect all selections
	void		deselect(int row);

			// selection handling
	int		get_selections(Vector *);
	const char	*get_item(int row, int column);

	void		set_sensitive(Boolean);

			// access functions
	Callback_ptr	get_select_cb()	{ return select_cb; }
	Callback_ptr	get_deselect_cb() { return deselect_cb; }
	void		*get_client_data();
	Select_mode	get_mode()	{ return mode; }
};

#endif	// _SEL_LIST_H
