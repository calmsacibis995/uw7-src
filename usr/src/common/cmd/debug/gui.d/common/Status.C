#ident	"@(#)debugger:gui.d/common/Status.C	1.18"

#include "Table.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Menu.h"
#include "Status.h"
#include "Windows.h"
#include "Window_sh.h"
#include "gui_label.h"
#include "Label.h"

static const Column st_spec[] = {
	{ LAB_program,	9,	Col_text },
	{ LAB_id,	9,	Col_text },
	{ LAB_state,	9,	Col_text },
	{ LAB_function,	12,	Col_text },
	{ LAB_location,	18,	Col_right_text },
};

Status_pane::Status_pane(Window_set *ws, Base_window *parent, Box *container, Pane_descriptor *pdesc)
	: PANE(ws, parent, PT_status)
{
	pane = new Table(container, "status", SM_single, st_spec,
		sizeof(st_spec)/sizeof(Column), 1, FALSE,
		Search_none, 0,
		(Callback_ptr)&Status_pane::select_cb,
		0,0,0,0,  this, HELP_status_pane);
	pane->show_border();
	container->add_component(pane, FALSE);

	pane->insert_row(0, 0, 0, 0, 0, 0);
	pane->set_sensitive(FALSE);
	if (pdesc->get_menu_descriptor())
		popup_menu = create_popup_menu(pane, parent, 
			pdesc->get_menu_descriptor());
}

Status_pane::~Status_pane()
{
	delete popup_menu;
}

void
Status_pane::update_cb(void *, Reason_code rc, void *, ProcObj *pobj)
{
	if (rc == RC_delete)
		// delete is always followed by set_current
		return;

	if (!pobj)
	{
		// clear row
		pane->set_row(0, 0, 0, 0, 0, 0);
		pane->set_sensitive(FALSE);
		return;
	}

	const char	*state;
	const char	*function;
	const char	*location;

	if (in_script)
	{
		state = pobj->get_state_string();
		function = location = 0;
	}
	else if (pobj->is_animated())
	{
		if (rc != RC_animate)
			return;

		state = labeltab.get_label(LAB_stepping);
		function = location = 0;
	}
	else
	{
		state = pobj->get_state_string();
		function = pobj->get_function();
		location = pobj->get_location();
	}

	pane->set_row(0, pobj->get_program()->get_name(), pobj->get_name(),
		state, function, location);
	pane->set_sensitive(TRUE);
}

void
Status_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_obj());
	window_set->change_current.add(this,
		(Notify_func)(&Status_pane::update_cb), 0);
}

void
Status_pane::popdown()
{
	window_set->change_current.remove(this,
		(Notify_func)(&Status_pane::update_cb), 0);
}

void
Status_pane::select_cb(Component *, Table_calldata *tdata)
{
	const char	*string;

	// display full string in window footer
	string = pane->get_cell(tdata->index, tdata->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
}
