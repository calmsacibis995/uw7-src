#ident	"@(#)debugger:gui.d/common/Panes.C	1.4"

#include "Base_win.h"
#include "Window_sh.h"
#include "Panes.h"
#include "Help.h"
#include "UI.h"
#include "str.h"

class Sensitivity;

struct Pane_entry {
	const char	*name;
	int		type;
};

static Pane_entry	ptable[] = {
	{ "DISASSEMBLER", PT_disassembler },
	{ "EVENT", PT_event },
	{ "PROCESS", PT_process },
	{ "REGISTERS", PT_registers },
	{ "SOURCE", PT_source },
	{ "SECOND_SOURCE", PT_second_src },
	{ "STACK", PT_stack },
	{ "STATUS", PT_status },
	{ "SYMBOLS", PT_symbols },
	{ "COMMAND", PT_command },
	{ 0, 0 },
};

int 
find_pane_type(register const char *s)
{
	static int init_done = 0;
	register Pane_entry	*p;

	if ( !init_done ) 
	{
		for ( p = ptable ; p->name; p++ )
			p->name = str(p->name);
		init_done = 1;
	}

	s = strlook(s);		// so can compare ptrs

	for ( p = ptable ; p->name; p++ )
		if ( s == p->name )
			return p->type;
	return 0;
}

const char *
pane_name(int type)
{
	register Pane_entry	*p;

	for ( p = ptable ; p->type; p++ )
	{
		if ( type == p->type )
			return p->name;
	}
	return 0;
}

// popup, popdown, selection_type, and check_sensitivity
// are pure virtual functions, but the actual functions are needed for cfront 1.2
void
Pane::popup()
{
}

void
Pane::popdown()
{
}

char *
Pane::get_selection()
{
	return 0;
}

int
Pane::get_selections(Vector *)
{
	return 0;
}

Selection_type
Pane::selection_type()
{
	return SEL_none;
}

void
Pane::deselect()
{
}

int
Pane::check_sensitivity(Sensitivity *)
{
	return 0;
}

void
Pane::copy_selection()
{
}

// overrides Command_sender virtual
Base_window *
Pane::get_window()
{
	return parent;
}
