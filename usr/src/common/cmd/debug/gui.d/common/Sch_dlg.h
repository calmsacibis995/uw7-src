#ifndef	_SCH_DLG_H
#define _SCH_DLG_H
#ident	"@(#)debugger:gui.d/common/Sch_dlg.h	1.5"

#include "Component.h"
#include "Dialogs.h"

class Base_window;
class Source_pane;
class Disassembly_pane;
class Radio_list;

class Search_dialog : public Dialog_box
{
	Text_line	*string;
	char		*save_string;
	Source_pane	*source;
	Disassembly_pane	*dis;
	Radio_list	*radio;
	int		choice;
	void		do_it(Boolean forward);
public:
			Search_dialog(Base_window *);
			~Search_dialog()	{ delete save_string; }

			// callbacks
	void		search_forward(Component *, void * );
	void		search_backward(Component *, void *);
	void		cancel(Component *, void *);

			// initialization routines
	void		set_string(const char *);
};

#endif	// _SCH_DLG_H
