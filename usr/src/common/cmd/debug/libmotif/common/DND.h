#ifndef DND_H
#define DND_H
#ident	"@(#)debugger:libmotif/common/DND.h	1.1"

#include <X11/Intrinsic.h>

class Component;

struct DND_calldata
{
	Component	*dropped_on;
	int		item_pos;
};

typedef void (Component::*DND_callback_proc)(DND_calldata *);
typedef void (*DND_drop_proc)(Widget,XtPointer,XtPointer);

class DND
{
	Atom		_atom;
	DND_callback_proc	_cb;
	DND_drop_proc	old_proc;
public:
			DND(Atom a) { _atom = a; _cb = 0; old_proc = 0; }
			~DND() { }

	Atom		get_atom() { return _atom; }
	DND_callback_proc get_drop_cb() { return _cb; }
	DND_drop_proc	get_old_drop_proc() { return old_proc; }

	void		setup_drag_source(Widget, DND_callback_proc);
	void		setup_drop_site(Widget);
};

#endif
