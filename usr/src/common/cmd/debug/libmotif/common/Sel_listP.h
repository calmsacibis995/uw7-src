#ifndef	_SELECTIONP_H
#define	_SELECTIONP_H
#ident	"@(#)debugger:libmotif/common/Sel_listP.h	1.5"

// toolkit specific members of the Selection_list class
// included by ../../gui.d/common/Selection.h

struct Sel_column;
struct DND_calldata;
class DND;

#define SELECTION_LIST_TOOLKIT_SPECIFICS	\
private:					\
	Widget		list;			\
	int		total_rows;		\
	XmString	*item_strings;		\
	unsigned char	*selections;		\
	char		**pointers;		\
	const Sel_column *column_spec;		\
	int		last_selection;		\
	DND		*dnd;			\
	int		cur_pos;		\
	char		*search_str;		\
	short		cur_search_len;		\
	short		search_str_len;		\
	Boolean		alphabetic;		\
	Boolean		search_success;		\
	unsigned char	search_column;		\
	unsigned char	columns;		\
						\
	void		make_strings(const char **);	\
	void		free_strings();			\
	void		setup_for_search();		\
	int		compare_and_set(int);		\
	void		select(int pos, Boolean notify);	\
							\
public:							\
	void		clear_search();				\
	void		search(char *);				\
	void		dnd_drop_cb(DND_calldata *);	\
	void		handle_select(XmListCallbackStruct *);	\
	void		handle_mselect(XmListCallbackStruct *);	\
	void		handle_dblselect(XmListCallbackStruct *);

#endif	// _SELECTIONP_H
