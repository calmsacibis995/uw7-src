#ifndef	MEM_DLG_H
#define	MEM_DLG_H
#ident	"@(#)debugger:gui.d/common/Mem_dlg.h	1.8"

#include "Component.h"
#include "Dialogs.h"
#include "Reason.h"

class Base_window;
class Message;
class ProcObj;
class Radio_list;

class Dump_dialog : public Process_dialog
{
	Text_line	*location;
	Text_line	*count;
	Text_area	*dump_pane;
	Radio_list	*dump_choices;
	unsigned char	current_choice;
	Boolean		in_update;
public:
			Dump_dialog(Base_window *);
			~Dump_dialog() {};

			// callbacks
	void		do_dump(Component *, void *);
	void		reset(Component *, void *);
	
	void		set_location(const char *);
	void		clear();

			// functions overriding those of Dialog_box
	void		de_message(Message *);
	void		cmd_complete();
	void		update_obj(ProcObj *, Reason_code);
};

class Map_dialog : public Process_dialog
{
	Text_area	*map_pane;
public:
			Map_dialog(Base_window *);
			~Map_dialog() {};

	void		do_map();
	void		update_obj(ProcObj *, Reason_code);
};

#endif // MEM_DLG_H
