#ident	"@(#)debugger:gui.d/common/Command.C	1.55"

// GUI headers
#include "Dialog_sh.h"
#include "Command.h"
#include "Dispatcher.h"
#include "Dialogs.h"
#include "Windows.h"
#include "Menu.h"
#include "Caption.h"
#include "Text_area.h"
#include "Text_line.h"
#include "Toggle.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Window_sh.h"
#include "FileInfo.h"
#include "Sense.h"
#include "gui_label.h"
#include "Label.h"
#include "config.h"

// Debug headers
#include "Buffer.h"
#include "Message.h"
#include "Vector.h"
#include "str.h"

#include <unistd.h>
#include <signal.h>

// The script dialog lets the user enter the path name of a script file
// (a text file containing debugger commands).  The script is executed
// when the "script" button (default) is pushed.

class Script_dialog : public Dialog_box
{
	Toggle_button	*echo;	// echo commands when executing script, if true
	Text_line	*file_name;	// user-supplied path name
	char		*save_name;	// save last file name for Cancel operation
	Boolean		echo_state;	// save last state of toggle for Cancel
public:
			Script_dialog(Command_pane *);
			~Script_dialog() { delete save_name; }

			// button callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
			// drag-n-drop callback
	void		drop_cb(Component *, void *);
};

#define	S_ECHO_TOGGLE	0

Script_dialog::Script_dialog(Command_pane *cw) : DIALOG_BOX(cw->get_parent())
{
	static const DButton	buttons[] =
	{
		{ B_ok,	    LAB_script,  LAB_script_mne, 
			(Callback_ptr)(&Script_dialog::apply) },
		{ B_cancel,  LAB_none,	LAB_none,
			(Callback_ptr)(&Script_dialog::cancel) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static Toggle_data toggle = { LAB_echo, TRUE, 0 };

	Packed_box	*box;
	Caption		*caption;

	save_name = 0;
	echo_state = TRUE;

	dialog = new Dialog_shell(cw->get_parent()->get_window_shell(), 
		LAB_script, 0,
		this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_script_dialog, (Callback_ptr)&Script_dialog::drop_cb, 
		Drop_cb_popdown);
	box = new Packed_box(dialog, "script box", OR_vertical);

	caption = new Caption(box, LAB_script_file_line, CAP_LEFT);
	file_name = new Text_line(caption, "script file", "", 25, TRUE);
	caption->add_component(file_name);
	box->add_component(caption);

	echo = new Toggle_button(box, "echo", &toggle, 
		sizeof(toggle)/sizeof(Toggle_data), OR_vertical, this);
	box->add_component(echo);
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(file_name);
}

// called when dropped on from the desktop
void
Script_dialog::drop_cb(Component *comp, void *arg)
{
	char *s = dialog->get_drop_item();
	if(s)
	{
		FileInfo file(s);

		switch(file.type())
		{
		case FT_TEXT:
			// apply
			file_name->set_text(s);
			apply(comp, arg);
			break;
		default:
			dialog->error(E_ERROR, GE_bad_drop);
			return;
		}
	}
}

void
Script_dialog::apply(Component *, void *)
{
	char		*s;

	s = file_name->get_text();
	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_file);
		return;
	}

	delete save_name;
	save_name = makestr(s);

	echo_state = echo->is_set(S_ECHO_TOGGLE);
	dispatcher.send_msg(this, 0, "script %s %s\n", 
				echo_state ? "" : "-q", s);
	dialog->wait_for_response();
}

void
Script_dialog::cancel(Component *, void *)
{
	file_name->set_text(save_name);
	echo->set(S_ECHO_TOGGLE, echo_state);
}

// The Input dialog lets the user give input to a process whose I/O is
// being captured by the debugger.  The input command accepts one line 
// (or part of a line) at a time.

class Input_dialog : public Process_dialog
{
	Toggle_button	*newline;	// newline suppressed if false
	Text_line	*input;		// user-supplied input string
	char		*save_string;	// save contents of input for Cancel
	Boolean		add_newline;	// save state of toggle for Cancel operation

	char		*get_string();	// retrieve and save new input string

public:
			Input_dialog(Command_pane *);
			~Input_dialog() { delete save_string; }

			// DButton callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
};

#define	I_ADDNL_TOGGLE	0

Input_dialog::Input_dialog(Command_pane *cw) : PROCESS_DIALOG(cw->get_parent())
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_input, LAB_input_mne, 
			(Callback_ptr)(&Input_dialog::apply) },
		{ B_apply, LAB_none, LAB_none, 
			(Callback_ptr)(&Input_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none, 
			(Callback_ptr)(&Input_dialog::cancel) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static Toggle_data	toggle = { LAB_append_newline, TRUE, 0 };

	Caption		*caption;
	Packed_box	*box;

	save_string = 0;
	add_newline = TRUE;

	dialog = new Dialog_shell(cw->get_parent()->get_window_shell(),
		LAB_input,
		(Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_input_dialog);
	box = new Packed_box(dialog, "input box", OR_vertical);
	component_init(box);

	caption = new Caption(box, LAB_input_line, CAP_LEFT);
	input = new Text_line(caption, "input", "", 25, TRUE);
	caption->add_component(input);
	box->add_component(caption);

	newline = new Toggle_button(box, "newline", &toggle, 
		sizeof(toggle)/sizeof(Toggle_data), OR_vertical, this);
	box->add_component(newline);
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(input);
}

char *
Input_dialog::get_string()
{
	char	*s = input->get_text();
	if (!s || !*s)
		return 0;

	Buffer	*buf = buf_pool.get();
	char	*ptr = s;

	window_set->command_add_text(s);
	buf->clear();

	for (; *s; ++s)
	{
		if (*s == '"')
		{
			if (ptr != s)
			{
				*s = '\0';
				buf->add(ptr);
			}
			ptr = s + 1;
			buf->add("\\\"");
		}
	}
	buf->add(ptr);

	delete save_string;
	save_string = makestr((char *)*buf);
	buf_pool.put(buf);
	return save_string;
}

void
Input_dialog::apply(Component *, void *)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	char *s;
	if ((s = get_string()) == 0)
		return;

	add_newline = newline->is_set(I_ADDNL_TOGGLE);
	if (add_newline)
		window_set->command_add_text("\n");

	dispatcher.send_msg(this, pobjs[0]->get_id(),
		"input %s \"%s\"\n", add_newline ? "" : "-n", s);
	dialog->wait_for_response();
}

void
Input_dialog::cancel(Component *, void *)
{
	input->set_text(save_string);
	newline->set(I_ADDNL_TOGGLE, add_newline);
}

Command_pane::Command_pane(Window_set *ws, Base_window *parent,
	Box *box1,
	Pane_descriptor *pdesc) : PANE(ws, parent, PT_command)
{
	Caption		*caption;
	Expansion_box	*box2 = 0;
	Box		*box;

	has_selection = FALSE;
	in_continuation = FALSE;
	script_box = 0;
	input_box = 0;

	if (box1->get_type() == Box_divided)
	{
		// base window uses divided box - 
		// create expansion box for our 2 components and
		// add to divided box
		box2 = new Expansion_box(box1, "expansion box",
			OR_vertical);
		box = box2;
	}
	else
	{
		box = box1;
	}
	transcript = new Text_area(box, "transcript", 
		pdesc->get_rows(), pdesc->get_cols(),
		FALSE, (Callback_ptr)(&Command_pane::select_cb), this,
		HELP_transcript_pane, LINES_TO_KEEP/pdesc->get_rows());
	box->add_component(transcript, TRUE);

	caption = new Caption(box, LAB_prompt, CAP_LEFT);
	command = new Text_line(caption, "command line", "", pdesc->get_cols() - 7, TRUE,
		(Callback_ptr)(&Command_pane::do_command), this,
		HELP_command_line);
	caption->add_component(command);
	box->add_component(caption);
	if (box2)
	{
		box1->add_component(box2, TRUE);
		box2->update_complete();
	}

	if (pdesc->get_menu_descriptor())
		popup_menu = create_popup_menu(transcript, parent, 
			pdesc->get_menu_descriptor());
	register_help(box->get_widget(), "command", HELP_command_pane);
	// the focus would be in the Transcript pane otherwise
	parent->get_window_shell()->set_focus(command);
}

Command_pane::~Command_pane()
{
	delete popup_menu;
	delete script_box;
	delete input_box;
}

void
Command_pane::de_message(Message *m)
{
	// gui-only messages are used to update state information and
	// are not displayed directly to the user
	if (gui_only_message(m) || m->get_msg_id() == ERR_cmd_pointer
		|| (m->get_msg_flags()&MSG_REDIRECTED))
		return;

	if (m->get_msg_id() == MSG_proc_output)
	{
		char	*s1;
		char	*text;

		// don't display the pty id
		m->unbundle(s1, text);
		window_set->command_add_text(text);
	}
	else
		window_set->command_add_text(m->format());
}

void
Command_pane::cmd_complete()
{
	command->clear();
}

// callback executed when the user types return in the command line
void
Command_pane::do_command(Component *, const char *s)
{
	if (!in_continuation)
		window_set->command_add_text((char *)labeltab.get_label(LAB_prompt));
	window_set->command_add_text((char *)s);
	window_set->command_add_text("\n");

	if (!s)
		return;

	size_t	len = strlen(s);

	if (len > 0 && s[len-1] == '\\')
	{
		if (!in_continuation)
		{
			in_continuation = TRUE;
			command_line.clear();
		}
		if (len > 1)
			command_line.add((void *)s, len-1);
		command->clear();
		window_set->command_add_text(">");
	}
	else 
	{
		// line without '\\' terminator or empty line
		if (in_continuation)
		{
			in_continuation = FALSE;
			if (len > 0)
				command_line.add((void *)s, len);
			command_line.add((void *)"", 1);	// add NULL byte
			s = (char *)command_line.ptr();
			len = strlen(s);
		}
		if (len > 0)
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"%s\n", s);
	}
}

// callback for the Script... button in the File menu
void
Command_pane::script_dialog_cb(Component *, void *)
{
	if (!script_box)
		script_box = new Script_dialog(this);
	script_box->display();
}

// callback for the Input... button in the Edit menu
void
Command_pane::input_dialog_cb(Component *, void *)
{
	if (!input_box)
		input_box = new Input_dialog(this);
	input_box->set_plist(parent, PROGRAM_LEVEL);
	input_box->display();
}

// callback for text selection and deselection in the transcript pane
void
Command_pane::select_cb(Text_area *, int selection)
{
	if (!selection && !has_selection)
		return;

	has_selection = (selection != 0);
	parent->set_selection(has_selection ? this : 0);
}

Selection_type
Command_pane::selection_type()
{
	return SEL_text;
}

char *
Command_pane::get_selection()
{
	return transcript->get_selection();
}

// callback for the Copy button - copy the text selection in the
// transcript pane to the clipboard
void
Command_pane::copy_selection()
{
	transcript->copy_selection();
}

void
Command_pane::interrupt_cb(Component *, void *)
{
	// send SIGINT to CL debugger
	kill(getppid(), SIGINT);
}

void
Command_pane::popup()
{
	window_set->command_panes.add(this);
	window_set->change_state.add(this, (Notify_func)(&Command_pane::update_state_cb), 0);
}

void
Command_pane::popdown()
{
	window_set->command_panes.remove(this);
	window_set->change_state.remove(this, (Notify_func)(&Command_pane::update_state_cb), 0);
}

void
Command_pane::update_state_cb(void *, Reason_code, void *, ProcObj *)
{
	parent->set_sensitivity();
}

int
Command_pane::check_sensitivity(Sensitivity *sense)
{
	return(sense->text_sel() && has_selection);
}
