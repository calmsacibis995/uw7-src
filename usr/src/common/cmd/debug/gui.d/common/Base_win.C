#ident	"@(#)debugger:gui.d/common/Base_win.C	1.29"

// GUI headers
#include "Base_win.h"
#include "Boxes.h"
#include "Caption.h"
#include "Command.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dis.h"
#include "Events.h"
#include "Menu.h"
#include "Button_bar.h"
#include "Buttons.h"
#include "Panes.h"
#include "Proclist.h"
#include "Ps_pane.h"
#include "Radio.h"
#include "Regs_pane.h"
#include "Sch_dlg.h"
#include "Src_pane.h"
#include "Stack_pane.h"
#include "Syms_pane.h"
#include "Status.h"
#include "Table.h"
#include "UI.h"
#include "Window_sh.h"
#include "gui_label.h"
#include "Label.h"
#include "Sense.h"
#include "config.h"
#include "Buffer.h"
#include "Text_area.h"
#include "Dispatcher.h"
#include "Resources.h"
#include "Btn_dlg.h"

// Debug headers
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"

#include <stdio.h>

int cmd_panes_per_set;

#ifndef MOTIF_UI

// Panes_dialog is created from Properties dialog's "Panes" button
// radio list states for Panes_dialog
#define	TRUNCATED	0
#define	WRAPPED		1

struct	Radio_descriptor {
	Radio_list	*radio;
	Table		*table;
	unsigned int	state;
};

class Panes_dialog : public Dialog_box
{
	Radio_descriptor	*radios;
	int			nradios;
public:
			Panes_dialog(Base_window *);
			// individual radio buttons deleted as
			// components of the window
			~Panes_dialog() { delete radios; }

	void		apply(Component *, void *);
	void		reset(Component *, void *);
	void		add_radio(Radio_descriptor *, Packed_box *,
				Table *, LabelId clabel, char *rlabel);
};

Panes_dialog::Panes_dialog(Base_window *win) : DIALOG_BOX(win)
{
	Radio_descriptor	*rptr;

	static const DButton	buttons[] =
	{
		{ B_apply,   LAB_none, LAB_none,
			(Callback_ptr)(&Panes_dialog::apply) },
		{ B_reset,   LAB_none, LAB_none, 
			(Callback_ptr)(&Panes_dialog::reset) },
		{ B_cancel,  LAB_none,	LAB_none, 
			(Callback_ptr)(&Panes_dialog::reset) },
		{ B_help,    LAB_none,	LAB_none, 0 },
	};

	Packed_box	*box;
	Stack_pane	*stack_pane;
	Symbols_pane	*syms_pane;
	Ps_pane		*ps_pane;
	Event_pane	*event_pane;

	nradios = 0;

	if ((stack_pane = (Stack_pane *)win->get_pane(PT_stack)))
		nradios++;
	if ((syms_pane = (Symbols_pane *)win->get_pane(PT_symbols)))
		nradios++;
	if ((ps_pane = (Ps_pane *)win->get_pane(PT_process)))
		nradios++;
	if ((event_pane = (Event_pane *)win->get_pane(PT_event)))
		// add one for regular events, one for onstop
		nradios += 2;
	
	radios = new Radio_descriptor[nradios];

	dialog = new Dialog_shell(win->get_window_shell(), LAB_panes, 
		0, this,
		buttons, sizeof(buttons)/sizeof(DButton),
		HELP_panes_dialog);
	box = new Packed_box(dialog, "properties", OR_vertical);

	rptr = radios;
	if (ps_pane)
	{
		add_radio(rptr, box, ps_pane->get_table(),
			LAB_process_pane, "ps pane");
		rptr++;
	}
	if (stack_pane)
	{
		add_radio(rptr, box, stack_pane->get_table(),
			LAB_stack_pane, "stack pane");
		rptr++;
	}
	if (syms_pane)
	{
		add_radio(rptr, box, syms_pane->get_table(),
			LAB_syms_pane, "syms pane");
		rptr++;
	}
	if (event_pane)
	{
		add_radio(rptr, box, event_pane->getEventPane(),
			LAB_mevent_pane, "main event pane");
		rptr++;
		add_radio(rptr, box, event_pane->getOnstopPane(),
			LAB_onstop_pane, "onstop pane");
		rptr++;
	}
	box->update_complete();
	dialog->add_component(box);
}

void
Panes_dialog::add_radio(Radio_descriptor *rptr, Packed_box *box,
		Table *t, LabelId clabel, char *rlabel)
{
	Caption		*caption;

	static const LabelId choices[] =
	{
		LAB_truncate,
		LAB_wrap,
	};

	caption = new Caption(box, clabel, CAP_LEFT);
	rptr->radio = new Radio_list(caption, rlabel,
		OR_horizontal, choices,
		sizeof(choices)/sizeof(LabelId), 0);
	rptr->table = t;
	rptr->state = TRUNCATED;
	caption->add_component(rptr->radio);
	box->add_component(caption);
}

void
Panes_dialog::apply(Component *, void *)
{
	Radio_descriptor	*rptr = radios;

	for(int i = 0; i < nradios; i++, rptr++)
	{
		rptr->state = rptr->radio->which_button();
		rptr->table->wrap_columns(rptr->state);
	}
}

void
Panes_dialog::reset(Component *, void *)
{
	Radio_descriptor	*rptr = radios;

	for(int i = 0; i < nradios; i++, rptr++)
	{
		rptr->radio->set_button(rptr->state);
	}
}
#endif

// Dialog for running server commands and displaying output
class Command_dialog : public Dialog_box {
	int		errors;
	Text_area	*pane;
	Caption		*caption;
public:
		Command_dialog(Base_window *, Window_set *);
		~Command_dialog() {}
	int	run_cmd(char *cmd);
};

Base_window::Base_window(Window_set *ws, Window_descriptor *wdesc,
	Window_configuration *wconf) : COMMAND_SENDER(ws)
{
	Divided_box	*box2 = 0;
	Pane_descriptor	*pdesc;
	const char	*wname = wdesc->get_name();
	int		set_line_excl = 0;
	int 		lflags = wdesc->get_flags();
	Boolean		iconic = FALSE;

	flags = 0;
	sense_count = 0;
	busy_count = 0;
	set_line_count = 0;
	selection_pane = 0;

	status_pane = 0;
	event_pane = 0;
	ps_pane = 0;
	dis_pane = 0;
	regs_pane = 0;
	source_pane = 0;
	cmd_pane = 0;
	syms_pane = 0;
	stack_pane = 0;

#ifndef MOTIF_UI
	panes_dialog = 0;
#endif
	search_dialog = 0;
	cmd_dialog = 0;
	top_button_bar = 0;
	bottom_button_bar = 0;
	btn_config_box = 0;
	win_desc = wdesc;

	wname = wdesc->get_name();
	if (wconf && wconf->is_iconic())
		iconic = TRUE;
	else if (!wconf && resources.get_iconic() &&
		(ws == (Window_set *)windows.first()) &&
		(wdesc->get_flags() & W_AUTO_POPUP))
		iconic = TRUE;
	window = new Window_shell(wname, 0, this, HELP_window, iconic);
	if (wconf && (iconic == FALSE))
		window->set_initial_pos(wconf->get_x_pos(),
			wconf->get_y_pos());

	box1 = new Expansion_box(window, "expansion box", OR_vertical);
	menu_bar = new Menu_bar(box1, this, window_set, wdesc->get_menus());
	box1->add_component(menu_bar);

	if (lflags & PT_status)
	{
		pdesc = wdesc->get_panes();
		for (; pdesc; pdesc = pdesc->next())
		{
			if (pdesc->get_type() == PT_status)
				break;
		}
		status_pane = new Status_pane(ws, this, box1, pdesc);
	}

	top_insertion = box1->get_last();
	if (wdesc->get_top_button_bars())
	{
		top_button_bar = new Button_bar(box1, this, ws,
			wdesc->get_top_button_bars());
		box1->add_component(top_button_bar);
	}

	if (wdesc->get_npanes() > 2 || ((wdesc->get_npanes() == 2) && 
		(!(lflags & PT_status) || (lflags & PT_event))))
	{
		// use divided box if more than 2 panes
		// or exactly two and neither is status
		// or if we have an event pane
		box2 = new Divided_box(box1, "divided_box");
		box1->add_component(box2, TRUE);
	}

	Box	*box = box2 ? (Box *)box2 : (Box *)box1;

	pdesc = wdesc->get_panes();
	for (; pdesc; pdesc = pdesc->next())
	{
		switch (pdesc->get_type())
		{
		case PT_process:
			ps_pane = new Ps_pane(ws, this, box, pdesc);
			ws->ps_panes.add(ps_pane);
			break;

		case PT_stack:
			stack_pane = new Stack_pane(ws, this,
				box, pdesc);
			break;

		case PT_symbols:
			syms_pane = new Symbols_pane(ws, this, 
					box, pdesc);
			break;

		case PT_registers:
			regs_pane = new Register_pane(ws, this, 
					box, pdesc);
			break;

		case PT_status:
			break;

		case PT_event:
			event_pane = new Event_pane(ws,
				this, box, pdesc);
			break;

		case PT_command:
			cmd_pane = new Command_pane(ws, this, 
				box, pdesc);
			break;

		case PT_disassembler:
			dis_pane = new Disassembly_pane(ws,
				this, box, pdesc);
			set_line_excl++;
			break;

		case PT_source:
			// primary source pane
			source_pane = new Source_pane(ws, this, box, 
					pdesc, TRUE);
			set_line_excl++;
			break;
		case PT_second_src:
			// secondary source pane
			source_pane = new Source_pane(ws, this, box,
					pdesc, FALSE);
			ws->inc_open_windows();
			ws->source_windows.add(this);
			break;

		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;
		}
	}

	if (set_line_excl >= 2)
		flags |= BW_SET_LINE_EXCLUSIVE;

	bottom_insertion = box1->get_last();
	if (wdesc->get_bottom_button_bars())
	{
		bottom_button_bar = new Button_bar(box1, this, ws,
			wdesc->get_bottom_button_bars());
		box1->add_component(bottom_button_bar);
	}
	if (box2)
		box2->update_complete();
	box1->update_complete();
	window->add_component(box1);

	// make sure the window set menu in our Files menu knows
	// about all of the window sets
	Window_set	*wptr = (Window_set *)windows.first();
	Menu 		*mp = menu_bar->find_item(B_window_sets);
	int		my_id = window_set->get_id();
	if (!mp)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	const char	*lab = labeltab.get_label(LAB_window_set);
	int		tlen = strlen(lab) + MAX_INT_DIGITS + 2;

	for ( ; wptr; wptr = (Window_set *)windows.next())
	{
		char	*title;
		int	ws_id = wptr->get_id();

		if (ws_id == my_id)
			// don't need to add ourselves
			continue;


		title = new char[tlen];
		sprintf(title, "%s %d", lab, ws_id);

		if (mp->find_item(title) == 0)
		{
			mnemonic_t	mne;
			Button_core	*bcore;
			Button		*btn;

			if (ws_id < 10)
			{
				mne = '0' + ws_id;
			}
			else
				mne = 0;
			bcore = find_button(B_set_popup);
			btn = new Button(title, mne, bcore);
			btn->set_udata((void *)ws_id);
			mp->add_item(btn);
		}
	}
	// make sure the sources menu in our file menu knows about
	// all of the Source windows
	Base_window	*sw = (Base_window *)window_set->source_windows.first();
	for(; sw; sw = (Base_window *)window_set->source_windows.next())
	{
		if (sw == this)
			continue;
		if (!sw->source_pane)
		{
			display_msg(E_ERROR, GE_internal, __FILE__,
				__LINE__);
			continue;
		}
		add_src_menu(sw->source_pane->get_current_file(), sw);
	}
}

Base_window::~Base_window()
{
	if (win_desc->get_flags() & PT_second_src)
	{
		// secondary source window
		// open_windows already decremented in
		// Window_set::dismiss
		window_set->source_windows.remove(this);
	}
	delete window;
#ifndef MOTIF_UI
	delete panes_dialog;
#endif
	delete search_dialog;
	delete cmd_dialog;
	delete btn_config_box;

	if (ps_pane)
	{
		window_set->ps_panes.remove(ps_pane);
	}

	delete status_pane;
	delete event_pane;
	delete ps_pane;
	delete dis_pane;
	delete regs_pane;
	delete source_pane;
	delete cmd_pane;
	delete syms_pane;
	delete stack_pane;
}

Pane *
Base_window::get_pane(int pane_type)
{
	switch(pane_type)
	{
	case PT_disassembler:
		return(Pane *)dis_pane;
	case PT_process:
		return(Pane *)ps_pane;
	case PT_stack:
		return(Pane *)stack_pane;
	case PT_symbols:
		return(Pane *)syms_pane;
	case PT_registers:
		return(Pane *)regs_pane;
	case PT_status:
		return(Pane *)status_pane;
	case PT_event:
		return(Pane *)event_pane;
	case PT_command:
		return(Pane *)cmd_pane;
	case PT_second_src:
	case PT_source:
		return(Pane *)source_pane;
	}
	return 0;
}

// overrides Command_sender virtual
Base_window *
Base_window::get_window()
{
	return this;
}

void
Base_window::popup(Boolean grab_focus)
{
	if (flags & BW_IS_OPEN)
	{
		window->raise(grab_focus);
		return;
	}

	flags |= BW_IS_OPEN;
	if (flags & BW_SET_LINE_EXCLUSIVE)
		window_set->set_line_exclusive_wins.add(this);

	window->popup();
	set_sensitivity();

	if (status_pane)
		status_pane->popup();
	if (event_pane)
		event_pane->popup();
	if (ps_pane)
		ps_pane->popup();
	if (dis_pane)
		dis_pane->popup();
	if (regs_pane)
		regs_pane->popup();
	if (source_pane)
		source_pane->popup();
	if (cmd_pane)
		cmd_pane->popup();
	if (syms_pane)
		syms_pane->popup();
	if (stack_pane)
		stack_pane->popup();

	window_set->change_current.add(this, (Notify_func)(&Base_window::update_state_cb), 0);
	if (ps_pane)
		window_set->change_state.add(this,
			(Notify_func)(&Base_window::update_state_cb), 0);
}

void
Base_window::popdown()
{
	flags &= ~BW_IS_OPEN;
	window->popdown();

	if (status_pane)
		status_pane->popdown();
	if (event_pane)
		event_pane->popdown();
	if (ps_pane)
		ps_pane->popdown();
	if (dis_pane)
		dis_pane->popdown();
	if (regs_pane)
		regs_pane->popdown();
	if (source_pane)
		source_pane->popdown();
	if (cmd_pane)
		cmd_pane->popdown();
	if (syms_pane)
		syms_pane->popdown();
	if (stack_pane)
		stack_pane->popdown();

	if (flags & BW_SET_LINE_EXCLUSIVE)
		window_set->set_line_exclusive_wins.remove(this);

	window_set->change_current.remove(this, (Notify_func)(&Base_window::update_state_cb), 0);
	if (ps_pane)
		window_set->change_state.remove(this,
			(Notify_func)(&Base_window::update_state_cb), 0);
	if (win_desc->get_flags() & PT_second_src)
	{
		// secondary source window
		delete this;
	}
}

void
Base_window::update_state_cb(void *, Reason_code rc, void *, ProcObj *proc)
{
	if (proc && proc->is_animated() && rc == RC_animate)
		return;

	if (rc == RC_delete || rc == RC_rename)
		return;
	set_sensitivity();
}

// There are 2 types of sensitivities: those that are
// of interest only to a particular pane, and those
// that are of global interest.  There are also two
// types of buttons (where sensitivity is concerned):
// those that require a selection and those that do not.
// For buttons that require a selection, we:
// 1. Check that there is a selection; if not, insensitive.
// 2. Check the selection pane for correct criteria.
// 3. Check global criteria.
// For those buttons not requiring a selection, we:
// 1. Check each pane that contributes sensitivity
//    criteria to this button.  If there is at least one,
//    than at least one of these sets of criteria must be
//    satisfied.
// 2. Check global criteria.
int
Base_window::check_sensitivity(Sensitivity *sense)
{
	// if window set contains a pane that is animated, everything
	// but Halt and Destroy should be insenstivive
	if (window_set->is_animated())
		return (sense->animated() ? 1 : 0);

	if (sense->sel_required())
	{
		if (!selection_pane ||
			!selection_pane->check_sensitivity(sense))
			return 0;
	}
	else
	{
		// no selection required
		// check each pane that might have contributed
		// requirements
		// Currently only source and dis panes have
		// requirements that do not require a selection

		if (sense->dis_pane() && dis_pane)
		{
			if (!dis_pane->check_sensitivity(sense))
				return 0;
		}
		if (sense->source_pane() && source_pane)
		{
			if (!source_pane->check_sensitivity(sense))
				return 0;
		}
	}
	if (sense->process())
	{
		if (selection_type() == SEL_process)
		{
			// applies to selected process(es)
			if (sense->process_sel())
				// already checked above
				return 1;
			else return(selection_pane->check_sensitivity(sense));
		}
		// applies to current process or thread 
		// since none was selected
		ProcObj *cur_obj = window_set->current_obj();

		if (!cur_obj)
			return 0;
#ifdef DEBUG_THREADS
		// current object is a thread , and it's the only 
		// one in the process
		if (sense->proc_only() && 
			cur_obj->is_thread() &&
			window_set->get_command_level() == THREAD_LEVEL)
		{
			// we want a threaded process with only
			// 1 thread
			if (cur_obj->get_process()->get_head()->next())
				return 0;
		}
#endif
		if (sense->proc_special() &&
			!cur_obj->check_sensitivity(sense))
			return 0;
	}
	return 1;
}

// We use sense_count to avoid multiple sensitivity checks for
// a single operation.  The counter has basically three values:
// 0, 1, 2 (really > 1).
// If the counter is greater than 0, it
// is simply incremented, and we return.  Dispatcher::process_msg
// sets the count to 1 for all windows in the affected window
// set.  Before returning, it checks whether anyone has called
// set_sensitivity for each window (count > 1), sets the count
// to 0, and calls set_sensitivity for the windows where the
// count was greater than 1.  This ensures that sensitivity
// is set only after all operations on the panes of that window
// have been performed and is only called once.  It allows
// synchronous calls (select_cb) to happen naturally, since
// count will be 0 for these.

void
Base_window::set_sensitivity()
{
	if (!(flags & BW_IS_OPEN))
		return;

	switch(sense_count)
	{
	case 0:
		break;
	case 1:
		sense_count++;
		// FALLTHROUGH
	default:
		// never really need to know if greater than
		// 2; avoid problem of wrap around by simply
		// stopping at 2
		return;
	}

	Menu	**menup = menu_bar->get_menus();
	int	total = menu_bar->get_nbuttons();
	int	i;

	for (i = 0; i < total; i++, menup++)
	{
		if ((*menup)->is_popped_up() == TRUE)
			set_sensitivity(*menup);
	}
	if (top_button_bar)
	{
		Button	*btn = top_button_bar->get_buttons();
		for(i = 0; btn; i++, btn = btn->next())
		{
			top_button_bar->set_sensitivity(i, 
				is_sensitive(btn->get_sensitivity()));
		}
	}
	if (bottom_button_bar)
	{
		Button	*btn = bottom_button_bar->get_buttons();
		for(i = 0; btn; i++, btn = btn->next())
		{
			bottom_button_bar->set_sensitivity(i, 
				is_sensitive(btn->get_sensitivity()));
		}
	}
}

// recursive function to get all levels of menus
void
Base_window::set_sensitivity(Menu *mp)
{
	int	j;
	Button	*btn = mp->get_buttons();

	for (j = 0; btn; j++, btn = btn->next())
		mp->set_sensitive(j, 
			is_sensitive(btn->get_sensitivity()));

	Menu *sub_menu = mp->first_child();
	for ( ; sub_menu; sub_menu = mp->next_child())
	{
		if (sub_menu->is_popped_up() == TRUE)
			set_sensitivity(sub_menu);
	}
}

int
Base_window::is_sensitive(Sensitivity *sense)
{
	if (sense->always())
		return 1;
	if (sense->all_but_script())
	{
		if (in_script)
			return 0;
		return 1;
	}
	if (sense->src_wins())
	{
		Base_window	*sw = 
			(Base_window *)window_set->source_windows.get_first();
		if (!sw || ((sw == this) &&
			(sw == (Base_window *)window_set->source_windows.last())))
			return 0;

	}
	if (sense->win_sets() && 
		(windows.get_first() == windows.last()))
		return 0;

	return check_sensitivity(sense);
}

void
Base_window::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
		display_msg(m);
	else if (!gui_only_message(m))
		window->display_msg(m);
}

void
Base_window::set_selection(Pane *pane)
{
	if (selection_pane && selection_pane != pane)
		selection_pane->deselect();
	selection_pane = pane;
	set_sensitivity();
}

void
Base_window::inc_busy()
{
	if (!busy_count)
	{
		window->set_busy(TRUE);
	}
	++busy_count;
}

void
Base_window::dec_busy()
{
	if (!busy_count)
		return;
	--busy_count;
	if (!busy_count)
	{
		window->set_busy(FALSE);
	}
}

char *
Base_window::get_selection()
{
	if (selection_pane)
		return selection_pane->get_selection();
	return 0;
}

int
Base_window::get_selections(Vector *v)
{
	if (selection_pane)
		return selection_pane->get_selections(v);
	return 0;
}

Selection_type
Base_window::selection_type()
{
	if (selection_pane)
		return selection_pane->selection_type();
	return SEL_none;
}

void 
Base_window::copy_cb(Component *, void *)
{
	if (!selection_pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	selection_pane->copy_selection();
}

void 
Base_window::set_break_cb(Component *, void *)
{
	if (!selection_pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return;
	}
	switch(selection_pane->get_type())
	{
	case PT_source:
		((Source_pane *)selection_pane)->set_break_cb(0, 0);
		break;
	case PT_disassembler:
		((Disassembly_pane *)selection_pane)->set_break_cb(0, 0);
		break;
	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

void 
Base_window::delete_break_cb(Component *, void *)
{
	if (!selection_pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return;
	}
	switch(selection_pane->get_type())
	{
	case PT_source:
		((Source_pane *)selection_pane)->delete_break_cb(0, 0);
		break;
	case PT_disassembler:
		((Disassembly_pane *)selection_pane)->delete_break_cb(0, 0);
		break;
	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

#ifndef MOTIF_UI
void 
Base_window::setup_panes_cb(Component *, void *)
{
	if (!panes_dialog)
		panes_dialog = new Panes_dialog(this);
	panes_dialog->display();
}
#endif


void
Base_window::search_dialog_cb(Component *, void *)
{
	if (!search_dialog)
		search_dialog = new Search_dialog(this);

	switch (selection_type())
	{
	case SEL_src_line:
	case SEL_regs:
	case SEL_instruction:
		search_dialog->set_string(truncate_selection(get_selection()));
		break;

	default:
		break;
	}

	search_dialog->display();
}

// called when help button is pressed for help on a specific pane
void
Base_window::help_sect_cb(Component *, void *h)
{
	Help_id help = (Help_id)(int)h;

	if(!help || help > HELP_final)
		return;
	display_help(window->get_widget(), HM_section, help);
}

#define MENU_MAX_LABEL_SIZE	64

static char *
left_truncate(const char *lab, Boolean makecopy = TRUE)
{
	static char buf[MENU_MAX_LABEL_SIZE+1];
	int len = strlen(lab);

	if (len > MENU_MAX_LABEL_SIZE)
	{
		strcpy(buf, lab + len - MENU_MAX_LABEL_SIZE);
		buf[0] = buf[1] = buf[2] = '.';
	}
	else
		strcpy(buf, lab);
	return makecopy ? makestr(buf) : buf;
}

void
Base_window::popup_window(Component *, void *bw)
{
	if (!bw)
		return;
	((Base_window *)bw)->popup();
}

void
Base_window::add_src_menu(const char *label, Base_window *bw)
{
	Button		*btn;
	Button_core	*bcore;
	const char	*name;

	Menu *mp = menu_bar->find_item(B_sources);
	if (!mp)
	{
		return;
	}
	name = label ? left_truncate(label) :
			makestr(labeltab.get_label(LAB_empty_src_menu));
	bcore = find_button(B_window_popup);
	btn = new Button(name, 0, bcore);
	btn->set_udata(bw);
	mp->add_item(btn);
	set_sensitivity();
}

void
Base_window::delete_src_menu(const char *label, Base_window *bw)
{
	Menu *mp = menu_bar->find_item(B_sources);
	if (!mp)
	{
		return;
	}
	Button		*btn = mp->get_buttons();
	int		i = 0;
	for (; btn; i++, btn = btn->next())
		if ((Base_window *)btn->get_udata() == bw)
			break;
	if (btn)
	{
		mp->delete_item(btn, i);
		set_sensitivity();
	}
}

void
Base_window::update_src_menu(const char *label, Base_window *bw)
{
	Menu *mp = menu_bar->find_item(B_sources);
	if (!mp)
	{
		return;
	}
	Button		*btn = mp->get_buttons();
	int		i = 0;
	for (; btn; i++, btn = btn->next())
		if ((Base_window *)btn->get_udata() == bw)
			break;
	if (btn)
	{
		const char	*new_lab;
		new_lab = label ? left_truncate(label) : 
			makestr(labeltab.get_label(LAB_empty_src_menu));
		mp->set_menu_label(i, new_lab);
	}
}

// parse a debugger or system cmd, looking
// for $SELECTION, $FILE, $LINE
char *
Base_window::parse_cmd(const char *cmd)
{
	if (!cmd || !*cmd)
		return makestr("");

	Buffer		*buf = buf_pool.get();
	const char	*ptr = cmd;
	int		inquote = 0;
	char		quote;

	buf->clear();
	while(*ptr)
	{
		if (*ptr == '\'' || *ptr == '"')
		{
			if (inquote && (quote == *ptr))
				inquote = 0;
			else
			{
				inquote = 1;
				quote = *ptr;
			}
				
		}
		else if (*ptr == '\\')
		{
			ptr++;
		}
		else if (*ptr == '$' && !inquote)
		{
			if (strncmp(ptr, "$SELECTION", 10) == 0)
			{
				char *sel_text = "";
				if (selection_pane)
				{
					switch(selection_pane->get_type())
					{
					case PT_command:
					case PT_disassembler:
					case PT_registers:
					case PT_source:
					case PT_second_src:
						sel_text = selection_pane->get_selection();
					default:
						break;
					}
				}
				buf->add(sel_text);
				ptr += 10;
			}
			else if (strncmp(ptr, "$FILE", 5) == 0)
			{
				const char	*curr_file = "";
				if (source_pane)
				{
					curr_file = source_pane->get_current_file();
				}
				buf->add(curr_file);
				ptr += 5;
			}
			else if (strncmp(ptr, "$LINE", 5) == 0)
			{
				char	curr_line[MAX_INT_DIGITS+1];
				int	cl = 0;
				if (source_pane)
				{
					cl = source_pane->get_current_line();
				}
				sprintf(curr_line, "%d", cl);
				buf->add(curr_line);
				ptr += 5;
			}
			continue;
		}
		buf->add(*ptr++);
	}
	ptr = makestr((char *)*buf);
	buf_pool.put(buf);
	return (char *)ptr;
}

void
Base_window::exec_cb(Component*, void *cmd)
{
	char	*pcmd;
	pcmd = parse_cmd((char *)cmd);
	system(pcmd);
	delete pcmd;
}

void
Base_window::debug_cmd_cb(Component *, void *cmd)
{
	char	*pcmd;

	pcmd = parse_cmd((char *)cmd);
	if (!cmd_dialog)
		cmd_dialog = new Command_dialog(this, window_set);
	cmd_dialog->display();
	cmd_dialog->run_cmd(pcmd);
	delete pcmd;
}

void
Base_window::button_dialog_cb(Component *, void *)
{
	if (!btn_config_box)
		btn_config_box = new Button_dialog(this, window_set);
	btn_config_box->re_init();
	btn_config_box->display();
}

void
Base_window::update_button_bar(Bar_info *bi)
{
	Button_bar	*bbar;
	if (bi->bottom)
		bbar = bottom_button_bar;
	else 
		bbar = top_button_bar;
	if (bi->remove)
	{
		// deleting
		if (!bi->new_wbar)
		{
			if (bbar)
			{
				if (bi->bottom)
					box1->remove_component(bottom_insertion);
				else
					box1->remove_component(top_insertion);
				bbar->destroy();
				delete bbar;
				if (bi->bottom)
					bottom_button_bar = 0;
				else
					top_button_bar = 0;
			}
		}
		else
			bbar->update_bar(bi);
	}
	else if (bbar)
		bbar->update_bar(bi);
	else
	{
		if (bi->bottom)
		{
			bottom_button_bar = new Button_bar(box1, this,
				window_set, bi->new_wbar);
			box1->insert_component(bottom_button_bar,
				bottom_insertion);
		}
		else
		{
			top_button_bar = new Button_bar(box1, this, 
				window_set, bi->new_wbar);
			box1->insert_component(top_button_bar, 
				top_insertion);
		}
	}
	if (btn_config_box && btn_config_box->get_shell()->is_open())
		btn_config_box->re_init();
}

// called by Button_bar::display_next_panel()
void
Base_window::update_button_config_cb()
{
	if (btn_config_box && btn_config_box->get_shell()->is_open())
		btn_config_box->re_init();
}

// Dialog for running server commands and displaying output
Command_dialog::Command_dialog(Base_window *bw, Window_set *wset)
	: Dialog_box(bw)
{
	static const DButton	buttons[] =
	{
		{ B_close, LAB_none, LAB_none, 0 },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Expansion_box	*box;

	dialog = new Dialog_shell(bw->get_window_shell(), 
		LAB_command_output, 
		0, this,
		buttons, sizeof(buttons)/sizeof(DButton),
		HELP_debug_cmd);
	box = new Expansion_box(dialog, "cmd box", OR_vertical);
	caption = new Caption(box, LAB_none, CAP_TOP_CENTER);
	pane = new Text_area(caption, "cmd", 8, 60);
	caption->add_component(pane);
	box->add_component(caption, TRUE);
	box->update_complete();
	dialog->add_component(box);
}

int
Command_dialog::run_cmd(char *cmd)
{
	Message		*msg;
	DBcontext	id;

	caption->set_label(cmd);
	pane->clear();
	errors = 0;
	id = window_set->current_obj() ?
		window_set->current_obj()->get_id() : 0;
	dispatcher.query(this, id, "%s\n", cmd);
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		{
			show_error(msg, TRUE);
			errors++;
		}
		else if (!gui_only_message(msg))
		{
			pane->add_text(msg->format());
		}
	}
	return errors;
}
