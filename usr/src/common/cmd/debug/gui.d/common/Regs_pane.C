#ident	"@(#)debugger:gui.d/common/Regs_pane.C	1.8"

// GUI headers
#include "Boxes.h"
#include "Caption.h"
#include "Component.h"
#include "Dispatcher.h"
#include "Notifier.h"
#include "Panes.h"
#include "Proclist.h"
#include "Regs_pane.h"
#include "Menu.h"
#include "Sense.h"
#include "Text_area.h"
#include "Windows.h"
#include "Window_sh.h"
#include "gui_label.h"
#include "config.h"

// Debug headers
#include "Buffer.h"
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"

Register_pane::Register_pane(Window_set *ws, Base_window *parent, Box *box,
	Pane_descriptor *pdesc) : PANE(ws, parent, PT_registers)
{
	// Register pane
	Caption	*caption = new Caption(box, LAB_regs, CAP_TOP_CENTER);
	reg_pane = new Text_area(caption, "register", pdesc->get_rows(), 
		pdesc->get_cols(), FALSE,
		(Callback_ptr)(&Register_pane::select_cb), this, HELP_regs_pane);

	caption->add_component(reg_pane);
	box->add_component(caption, TRUE);

	text_selected = FALSE;
	if (pdesc->get_menu_descriptor())
		popup_menu = create_popup_menu(reg_pane, parent, 
			pdesc->get_menu_descriptor());
}

Register_pane::~Register_pane()
{
	delete popup_menu;
	window_set->change_current.remove(this,
		(Notify_func)(&Register_pane::update_cb), 0);
}

void
Register_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_obj());
	window_set->change_current.add(this,
		(Notify_func)(&Register_pane::update_cb), 0);
	window_set->change_state.add(this, (Notify_func)(&Register_pane::update_state_cb), 0);
}

void
Register_pane::popdown()
{
	window_set->change_current.remove(this,
		(Notify_func)(&Register_pane::update_cb), 0);
	window_set->change_state.remove(this, (Notify_func)(&Register_pane::update_state_cb), 0);
}

void
Register_pane::update_state_cb(void *, Reason_code rc, void *, ProcObj *)
{
	parent->set_sensitivity();
}

void
Register_pane::update_cb(void *, Reason_code rc, void *, ProcObj *proc)
{
	if (!proc)
	{
		reg_pane->clear();
		if (text_selected)
		{
			text_selected = FALSE;
			parent->set_selection(0);
		}
		return;
	}

	if (proc->is_animated() || in_script || 
	    rc == RC_rename || rc == RC_delete || proc->is_running())
		return;

	parent->inc_busy();
	dispatcher.send_msg(this, window_set->current_obj()->get_id(), "regs\n");
	regs_buffer.clear();
	reg_pane->clear();
	if (text_selected)
	{
		text_selected = FALSE;
		parent->set_selection(0);
	}
}

void
Register_pane::de_message(Message *m)
{
	switch(m->get_msg_id())
	{
		case MSG_int_reg_newline:
		case MSG_flt_reg:
		case MSG_int_reg:
		case MSG_newline:
			regs_buffer.add(m->format());
			break;

		case MSG_reg_header:
			break;

		case ERR_invalid_op_dead:
			// gui should never be doing a regs on a dead process.
			// however, a process or thread may have
			// died after the regs was issued but before gui
			// received the notification.
			break;

		default:
			if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
				display_msg(m);
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;
	}
}
	
void
Register_pane::cmd_complete()
{
	reg_pane->add_text((char *)regs_buffer);
	reg_pane->position(1);
	parent->dec_busy();
}

Selection_type
Register_pane::selection_type()
{
	return SEL_regs;
}

void
Register_pane::select_cb(Text_area *, int line)
{
	if (line)
	{
		text_selected = TRUE;
		parent->set_selection(this);
	}
	else if (text_selected)
	{
		text_selected = FALSE;
		parent->set_selection(0);
	}
}

char *
Register_pane::get_selection()
{
	return reg_pane->get_selection();
}

void
Register_pane::copy_selection()
{
	reg_pane->copy_selection();
}

int
Register_pane::check_sensitivity(Sensitivity *sense)
{
	return(sense->text_sel() && text_selected);
}
