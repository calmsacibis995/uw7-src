#ifndef	_TABLEP_H
#define	_TABLEP_H
#ident	"@(#)debugger:libmotif/common/TableP.h	1.6"

// toolkit specific members of the Table class
// included by ../../gui.d/common/Table.h
#include <Xm/List.h>	// for XmListItemInitCallbackStruct
#include <stdarg.h>

struct Item_data;	// defined in Table.C
class DND;		// defined in DND.h
struct DND_calldata;	// defined in DND.h

#define TABLE_TOOLKIT_SPECIFICS \
private:						\
	Widget		input;				\
	Item_data	*item_data;			\
	Item_data	*last_row;			\
	const Column	*column_spec;			\
	int		last_selection;			\
	DND		*dnd;				\
	int		old_rows;			\
	int		cur_pos;		\
	char		*search_str;		\
	short		cur_search_len;		\
	short		search_str_len;		\
	Boolean		alphabetic;		\
	Boolean		sensitive;			\
	Boolean		search_success;		\
	unsigned char	search_column;		\
							\
	void		set_row(Item_data *, va_list);		\
	Item_data 	*get_row(int row);			\
	void		insert_row(int row, Item_data *new_row); \
	void		delete_row(Item_data *row);		\
	void		delete_nrows(int start, int total);	\
	void		deselect_row(int);			\
	XmString	*make_title();				\
	XmString	make_item_string(const char *, const Column *, char **); \
	XmString	*make_strings();			\
	void		setup_for_search();		\
	int		compare_and_set(Item_data *, int);\
	void		select(int pos);	\
public:								\
	void		clear_search();				\
	void		search(char *);				\
	void		dnd_drop_cb(DND_calldata *);		\
	void		set_glyph(XmListItemInitCallbackStruct *);	\
	void		handle_select(int pos);			\
	void		handle_mselect(XmListCallbackStruct *);	\
	void		dblselect(int pos);
									

#endif	// _TABLEP_H
