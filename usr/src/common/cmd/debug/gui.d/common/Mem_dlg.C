#ident	"@(#)debugger:gui.d/common/Mem_dlg.C	1.18"

// GUI headers
#include "Boxes.h"
#include "Caption.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Mem_dlg.h"
#include "Proclist.h"
#include "Radio.h"
#include "Text_area.h"
#include "Text_line.h"
#include "UI.h"
#include "Windows.h"
#include "Window_sh.h"
#include "gui_label.h"

// Debug headers
#include "Message.h"
#include "Msgtab.h"

// assume that the Dump dialog is only accessible for a single
// process or thread (selected or current)

// follows order of dump_buttons array
#define DUMP_WORDS	0
#define DUMP_BYTES	1

Dump_dialog::Dump_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_apply, LAB_none, LAB_none, 
			(Callback_ptr)(&Dump_dialog::do_dump) },
		{ B_reset, LAB_none, LAB_none, 
			(Callback_ptr)&Dump_dialog::reset },
		{ B_close, LAB_none, LAB_none, 
			(Callback_ptr)&Dump_dialog::reset },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const LabelId dump_buttons[] =
	{
		LAB_dump_words,
		LAB_dump_bytes,
	};

	Expansion_box	*box1;
	Expansion_box	*box2;
	Caption		*caption;

	current_choice = DUMP_WORDS;
	in_update = FALSE;
	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_dump, (Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_dump_dialog);
	box1 = new Expansion_box(dialog, "dump box", OR_vertical);
	component_init(box1);

	caption = new Caption(box1, LAB_dump_format_line, CAP_LEFT);
	dump_choices = new Radio_list(caption, "dump_list", OR_horizontal,
		dump_buttons, sizeof(dump_buttons)/sizeof(LabelId),
		current_choice);
	caption->add_component(dump_choices, FALSE);
	box1->add_component(caption);

	box2 = new Expansion_box(box1, "dump box", OR_horizontal);
	box1->add_component(box2);

	caption = new Caption(box2, LAB_location_line, CAP_LEFT);
	location = new Text_line(caption, "location", "", 20, 1);
	caption->add_component(location);
	box2->add_component(caption, TRUE);

	caption = new Caption(box2, LAB_count_line, CAP_LEFT);
	count = new Text_line(caption, "count", "", 10, 1);
	caption->add_component(count);
	box2->add_component(caption);

	dump_pane = new Text_area(box1, "dump", 20, 73);
	box1->add_component(dump_pane, TRUE);
	box2->update_complete();
	box1->update_complete();
	dialog->add_component(box1);
	dialog->set_popup_focus(location);
}

void
Dump_dialog::do_dump(Component *, void *)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		in_update = FALSE;
		return;
	}

	char		*loc = location->get_text();
	char		*cnt = count->get_text();
	char		*opt;

	current_choice = dump_choices->which_button();

	if (!loc || !*loc)
	{
		dialog->error(E_ERROR, GE_no_location);
		in_update = FALSE;
		return;
	}
	dump_pane->clear();		

	if (current_choice == DUMP_BYTES)
		opt = "-b";
	else
		opt = "";

	if (cnt && *cnt)
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"dump %s -c %s %s\n", opt, cnt, loc);
	else
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"dump %s %s\n", opt, loc);
	dialog->wait_for_response();
}

void
Dump_dialog::reset(Component *, void *)
{
	dump_choices->set_button(current_choice);
}

void
Dump_dialog::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	// responses from dump command look like:
	//	MSG_raw_dump_header <process_or_thread_name> <program_name>\n
	//	MSG_raw_dump <line>\n
	//	...
	if (Mtable.msg_class(mtype) == MSGCL_error)
		show_error(m, TRUE);
	else if (mtype != MSG_raw_dump_header)
		dump_pane->add_text(m->format());
}

void
Dump_dialog::set_location(const char *s)
{
	location->set_text(s);
}

void
Dump_dialog::cmd_complete()
{
	in_update = FALSE;
	dialog->cmd_complete();
}

void
Dump_dialog::clear()
{
	dump_pane->clear();
}

void
Dump_dialog::update_obj(ProcObj *p, Reason_code)
{
	if (!p->is_running() && !p->is_animated() && !in_update)
	{
		in_update = TRUE;
		do_dump(0, 0);
	}
}

Map_dialog::Map_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_close, LAB_none, LAB_none, 0 },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Expansion_box	*box;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_map, (Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_map_dialog);
	box = new Expansion_box(dialog, "map box", OR_vertical);
	component_init(box);

	map_pane = new Text_area(box, "map", 8, 75);
	box->add_component(map_pane, TRUE);
	box->update_complete();
	dialog->add_component(box);
}

void
Map_dialog::do_map()
{
	Message		*msg;

	map_pane->clear();
	dispatcher.query(this, window_set->current_obj()->get_id(),
		"map -p %s\n", make_plist(total, pobjs, 0, level));
	// the responses look like:
	//	MSG_map_header <process_name>\n<header_string>\n
	//	MSG_map <line>\n
	//	MSG_map <line>\n
	//	...
	//	0
	while ((msg = dispatcher.get_response()) != 0)
	{
		char *s = msg->format();
		if (msg->get_msg_id() == MSG_map_header)
		{
			s = strchr(s, '\n') +  1; // chops off process name,
						  // already displayed
		}
			// this avoids putting out the final new-line,
			// which would leave an extra blank line at
			// the bottom of the pane
		else
			map_pane->add_text("\n");
		s[strlen(s)-1] = '\0';
		map_pane->add_text(s);
	}
	map_pane->position(1);
}

void
Map_dialog::update_obj(ProcObj *p, Reason_code)
{
	if (!p->is_running() && !p->is_animated())
	{
		do_map();
	}
}
