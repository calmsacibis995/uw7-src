#ifndef	_STATUS_H
#define	_STATUS_H
#ident	"@(#)debugger:gui.d/common/Status.h	1.8"

// GUI headers
#include "Panes.h"
#include "Reason.h"

class Expansion_box;
class ProcObj;
class Base_window;
struct Pane_descriptor;
class Window_set;
class Table;
struct Table_calldata;

class Status_pane : public Pane
{
	Table		*pane;

public:
			Status_pane(Window_set *ws, Base_window *parent, 
				Box *, Pane_descriptor *);
			~Status_pane();

			// callbacks
	void		update_cb(void *server, Reason_code, void *, ProcObj *);

			// functions overriding virtuals in Pane base class
	void		popup();
	void		popdown();
	void		select_cb(Component *c, Table_calldata *tdata);
};

#endif	// _STATUS_H
