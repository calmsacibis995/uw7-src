#ident	"@(#)debugger:gui.d/common/Btn_dlg.C	1.6"

// Button bar configuration dialog

#include "UI.h"
#include "Boxes.h"
#include "Button_bar.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Radio.h"
#include "Stext.h"
#include "Table.h"
#include "Toggle.h"
#include "Text_line.h"
#include "Window_sh.h"

#include "Base_win.h"
#include "Panes.h"
#include "Buttons.h"
#include "Btn_dlg.h"
#include "Dialogs.h"
#include "Label.h"
#include "Windows.h"
#include "config.h"
#include "gui_label.h"
#include "gui_msg.h"

#include "Severity.h"

#include <limits.h>
#include <stdlib.h>

// dialog to set name, mne, cmd for a button
class Button_name_dialog : public Dialog_box 
{
	Button_dialog	*btn_dlg;
	Simple_text	*desc_line;
	Text_line	*name_line;
	Text_line	*mne_line;
	Text_line	*cmd_line;
	int		current_button;
public:
			Button_name_dialog(Button_dialog *bd,
				Base_window *bw);
			~Button_name_dialog() {}
	const char	*get_name() { return name_line->get_text(); }
	const char	*get_mne() { return mne_line->get_text(); }
	const char	*get_cmd() { return cmd_line->get_text(); }
	void		set_button(int, const char *desc, 
				const char *name,
				const char *mne, const char *cmd);
	void		apply(void *, void *);
	void		close();
};

Button_name_dialog::Button_name_dialog(Button_dialog *bd,
	Base_window *bw) : Dialog_box(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Button_name_dialog::apply) },
		{ B_close, LAB_none, LAB_none, 0 },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Caption		*caption;
	Packed_box	*box;

	btn_dlg = bd;
	current_button = -1;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_button_name_dlg, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_button_dialog);
	
	box = new Packed_box(dialog, "button_name", OR_vertical);

	caption = new Caption(box, LAB_button_desc_line, CAP_LEFT);
	desc_line = new Simple_text(caption, "", TRUE);
	caption->add_component(desc_line);
	box->add_component(caption);

	caption = new Caption(box, LAB_button_name_line, CAP_LEFT);
	name_line = new Text_line(caption, "button_name", "", 25, TRUE);
	caption->add_component(name_line);
	box->add_component(caption);

	caption = new Caption(box, LAB_button_mne_line, CAP_LEFT);
	mne_line = new Text_line(caption, "button_mne", "", 
		MB_LEN_MAX, TRUE);
	caption->add_component(mne_line);
	box->add_component(caption);

	caption = new Caption(box, LAB_button_cmd_line, CAP_LEFT);
	cmd_line = new Text_line(caption, "button_mne", "", 25, TRUE);
	caption->add_component(cmd_line);
	box->add_component(caption);

	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(name_line);
}

void
Button_name_dialog::set_button(int button, const char *desc, 
	const char *name, const char *mne, const char *cmd)
{
	Button_core	*bcore;
	CButtons	btype;

	if (button < 0 || button >= BUTTON_BAR_TOTAL)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return;
	}
	current_button = button;
	bcore = button_list[button];
	btype = bcore->button_type;
	desc_line->set_text(desc);
	name_line->set_text(name);
	mne_line->set_text(mne);
	cmd_line->set_sensitive(TRUE);
	cmd_line->set_text(cmd);
	if (btype != B_exec_cmd &&
		btype != B_debug_cmd)
		cmd_line->set_sensitive(FALSE);
}

void
Button_name_dialog::close()
{
	dialog->default_start();
	dialog->default_done();
}

void
Button_name_dialog::apply(void *, void *)
{
	char	*p;
	CButtons	btype;
	if (((p = name_line->get_text()) == 0) || (*p == 0))
	{
		dialog->clear_msg();
		dialog->error(E_ERROR, GE_no_button_name);
		return;
	}
	btype = button_list[current_button]->button_type;
	if ((btype == B_exec_cmd ||
		btype == B_debug_cmd) &&
		(((p = cmd_line->get_text()) == 0) ||
			(*p == 0)))
	{
		dialog->clear_msg();
		dialog->error(E_ERROR, GE_no_button_cmd);
		return;
	}
	btn_dlg->set_button(current_button);
}

static const Column button_spec[] =
{
	{ LAB_none,		1,	Col_glyph },
	{ LAB_button_desc,	30,	Col_text },
	{ LAB_button_name,	15,	Col_text },
	{ LAB_button_mne, 	8,	Col_text },
	{ LAB_button_cmd,	15,	Col_text },
};

// Button_dialg button bar buttons
#define	BTN_BB_ADD		0
#define BTN_BB_CHANGE		1
#define BTN_BB_DELETE		2
#define BTN_BB_DEL_ALL		3

// radio buttons order
#define RADIO_TOP_BARS		0
#define RADIO_BOTTOM_BARS	1

// toggle buttons order
#define TOGGLE_NEW		0
#define TOGGLE_SAVE		1

Button_dialog::Button_dialog(Base_window *bw, Window_set *ws) : 
	DIALOG_BOX(bw)
{
	Expansion_box	*box;
	Packed_box	*box2;
	Caption		*caption;
	Bar_descriptor	*bar_desc;

	static const LabelId which_buttons[] = { 
		LAB_top_bars,
		LAB_bottom_bars, 
	};
	static const Toggle_data toggle_list[] =
	{
		{ LAB_create_bar, FALSE,
			(Callback_ptr)(&Button_dialog::toggle_new_cb) },
		{ LAB_save_config, FALSE,
			(Callback_ptr)(&Button_dialog::toggle_save_cb) },
	};
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Button_dialog::apply) },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Button_dialog::apply) },
		{ B_reset, LAB_none, LAB_none,
			(Callback_ptr)(&Button_dialog::cancel_cb) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Button_dialog::cancel_cb) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	// for buttons in button bar, sensitivity is explicitly set
	static const Button_descriptor btn_bb_tab[] = {
		{ B_btn_dlg_add, LAB_btn_dlg_add, LAB_btn_dlg_add_mne },
		{ B_btn_dlg_change, LAB_btn_dlg_change, LAB_btn_dlg_change_mne },
		{ B_btn_dlg_del, LAB_btn_dlg_del, LAB_btn_dlg_del_mne },
		{ B_btn_dlg_del_all, LAB_btn_dlg_del_all, LAB_btn_dlg_del_all_mne },
	};

	window = bw;
	current_selection = -1;
	initial = TRUE;
	current_bar = new_bar = 0;
	bottom = FALSE;
	btn_name_box = 0;
	create_new_bar = FALSE;
	bar_count = 0;

	if (!button_list)
		build_button_list();
	if (window->get_top_bar() == 0)
	{
		if (window->get_bottom_bar() != 0)
			bottom = TRUE;
		else
			create_new_bar = TRUE;
	}
	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_button_dlg, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_button_dialog);

	box = new Expansion_box(dialog, "button_config", OR_horizontal);
	pane = new Table(box, "button_list", SM_single, button_spec,
		sizeof(button_spec)/sizeof(Column), 15, 
			FALSE,
		Search_alphabetic, 1,
		(Callback_ptr)(&Button_dialog::select_cb), 0,
		(Callback_ptr)(&Button_dialog::deselect_cb), 
		(Callback_ptr)(&Button_dialog::default_cb),
		0, 
		this, HELP_none);
	box->add_component(pane, TRUE);
	rows = 0;

	box2 = new Packed_box(box, "button_panel", OR_vertical);
	box->add_component(box2);

	caption = new Caption(box2, LAB_which_bar, CAP_TOP_LEFT);
	which_bar = new Radio_list(caption, "which", OR_vertical, 
		which_buttons, sizeof(which_buttons)/sizeof(LabelId),
		bottom ? RADIO_BOTTOM_BARS : RADIO_TOP_BARS,
		(Callback_ptr)&Button_dialog::which_bar_cb, this);
	caption->add_component(which_bar, FALSE);
	box2->add_component(caption);

	toggles = new Toggle_button(box2, "new_bar", toggle_list,
		sizeof(toggle_list)/sizeof(Toggle_data), OR_vertical,
		this);
	box2->add_component(toggles);
	if (create_new_bar)
		toggles->set(TOGGLE_NEW, TRUE);

	bar_desc = make_bar_descriptor(FALSE, btn_bb_tab,
			sizeof(btn_bb_tab)/sizeof(Button_descriptor));
	btn_bbar = new Button_bar(box2, window, window_set,
			bar_desc, OR_vertical, this);
	box2->add_component(btn_bbar);
	delete bar_desc;
	btn_bbar->set_sensitivity(BTN_BB_ADD, FALSE);
	btn_bbar->set_sensitivity(BTN_BB_CHANGE, FALSE);
	btn_bbar->set_sensitivity(BTN_BB_DELETE, FALSE);
	btn_bbar->set_sensitivity(BTN_BB_DEL_ALL, FALSE);

	const char *dname = config_directory ? config_directory : "";
	caption = new Caption(box2, LAB_config_dir_line, CAP_TOP_LEFT);
	config_dir = new Text_line(caption, "config_dir", dname, 25, TRUE);
	caption->add_component(config_dir, FALSE);
	box2->add_component(caption);
	config_dir->set_sensitive(FALSE);

	box2->update_complete();
	box->update_complete();
	dialog->add_component(box);
}

Button_dialog::~Button_dialog()
{
	delete btn_name_box;
	delete new_bar;
}

int
Button_dialog::setup_row(Boolean first_time, int row, 
	Button_core *bcore, const char *wname)
{
	Button		*btn;
	char		mbuf[MB_LEN_MAX+1];
	const char	*cmd, *name, *desc;
	CButtons	btype;

	if (!bcore || bcore->label == LAB_none)
	{
		display_msg(E_ERROR, GE_internal,
			__FILE__, __LINE__);
		return 0;

	}
	btype = bcore->button_type;
	desc = labeltab.get_label(bcore->label);
	btn = current_bar ? current_bar->get_button(btype, wname) : 0;
	if (btn)
	{
		name = btn->get_name();
		bar_count++;
	}
	else if (wname)
		name = wname;
	else
		name = "";

	if (btn && btn->get_mne())
	{
		int len = wctomb(mbuf, btn->get_mne());
		if (len > 0)
			mbuf[len] = 0;
		else 
			mbuf[0] = 0;
	}
	else
		mbuf[0] = 0;
	if (btn && (btype == B_exec_cmd ||
		btype == B_debug_cmd))
		cmd = (char *)btn->get_udata();
	else
		cmd = "";
	if (first_time)
	{
		pane->insert_row(row, btn ? Gly_check : Gly_blank,
			desc, name, mbuf, cmd);
		rows++;
	}
	else
		pane->set_row(row, btn ? Gly_check : Gly_blank,
			desc, name, mbuf, cmd);
	return 1;
}

void
Button_dialog::toggle_save_cb(Component *, Boolean save_config)
{
	config_dir->set_sensitive(save_config);
}

void
Button_dialog::toggle_new_cb(Component *, Boolean create_new)
{
	Boolean	use_bottom = (which_bar->which_button() ==
		RADIO_BOTTOM_BARS);
	if (!setup_list(use_bottom, create_new))
		toggles->set(TOGGLE_NEW, !create_new);
}

void
Button_dialog::which_bar_cb(Component *, int which)
{
	Boolean	use_bottom = (which == RADIO_BOTTOM_BARS);
	Boolean create_new = toggles->is_set(TOGGLE_NEW);
	if (!setup_list(use_bottom, create_new))
		which_bar->set_button(use_bottom ? RADIO_TOP_BARS :
			RADIO_BOTTOM_BARS);
}

void
Button_dialog::re_init()
{
	Boolean	use_bottom = (which_bar->which_button() ==
		RADIO_BOTTOM_BARS);
	Boolean create_new = toggles->is_set(TOGGLE_NEW);
	if (!create_new)
	{
		Button_bar	*bbar;
		if (use_bottom)
			bbar = window->get_bottom_bar();
		else
			bbar = window->get_top_bar();
		if (!bbar)
		{
			toggles->set(TOGGLE_NEW, TRUE);
			create_new = create_new_bar = TRUE;
		}
	}
	setup_list(use_bottom, create_new);
}

void
Button_dialog::select_cb(Table *, Table_calldata *tdata)
{
	const char *string;
	current_selection = tdata->index;
	string = pane->get_cell(tdata->index, tdata->col).string;
	dialog->clear_msg();
	dialog->error(E_NONE, string);
	if (pane->get_cell(current_selection, 0).glyph == Gly_blank)
	{
		btn_bbar->set_sensitivity(BTN_BB_ADD, TRUE);
		btn_bbar->set_sensitivity(BTN_BB_CHANGE, FALSE);
		btn_bbar->set_sensitivity(BTN_BB_DELETE, FALSE);
	}
	else
	{
		btn_bbar->set_sensitivity(BTN_BB_ADD, FALSE);
		btn_bbar->set_sensitivity(BTN_BB_CHANGE, TRUE);
		btn_bbar->set_sensitivity(BTN_BB_DELETE, TRUE);
	}
}

void
Button_dialog::default_cb(Table *, Table_calldata *)
{
}

void
Button_dialog::deselect_cb(Table *, int)
{
	if (current_selection >= 0)
	{
		current_selection = -1;
		btn_bbar->set_sensitivity(BTN_BB_ADD, FALSE);
		btn_bbar->set_sensitivity(BTN_BB_CHANGE, FALSE);
		btn_bbar->set_sensitivity(BTN_BB_DELETE, FALSE);
	}
}

void
Button_dialog::cancel_cb(void *, void *)
{
	int	which = bottom ? RADIO_BOTTOM_BARS :
		RADIO_TOP_BARS;
	if (which != which_bar->which_button())
		which_bar->set_button(which);
	if (create_new_bar != toggles->is_set(TOGGLE_NEW))
		toggles->set(TOGGLE_NEW, create_new_bar);
	if (btn_name_box)
		btn_name_box->close();
	setup_list(bottom, create_new_bar);
}


// bring up name dialog with current values of button - allow
// edit
void
Button_dialog::button_name_cb(void *, void *)
{
	const char	*desc, *name, *mne, *cmd;
	Button_core	*bcore;
	Window_descriptor	*wd = window->get_wdesc();

	if (current_selection < 0 || current_selection >= rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return;
	}
	bcore = button_list[current_selection];
	desc = labeltab.get_label(bcore->label);
	if (!(bcore->panes_req & wd->get_flags()))
	{
		dialog->clear_msg();
		dialog->error(E_ERROR, GE_config_button,
			desc, wd->get_name());
		return;
	}
	if ((bcore->button_type == B_popup) ||
		(pane->get_cell(current_selection, 0).glyph != Gly_blank))
	{
		name = pane->get_cell(current_selection, 2).string;
		mne = pane->get_cell(current_selection, 3).string;
		cmd = pane->get_cell(current_selection, 4).string;
	}
	else
	{
		name = mne = cmd = "";
	}
	if (!btn_name_box)
		btn_name_box = new Button_name_dialog(this, window);
	btn_name_box->display();
	btn_name_box->set_button(current_selection, desc, name, mne, cmd);
}

void
Button_dialog::set_button(int button)
{
	Button_core	*bcore;
	Button		*btn, *old_btn;
	CButtons	btype;
	mnemonic_t	mne;
	const char	*desc, *name, *mne_str, *cmd;

	if (button != current_selection)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return;
	}
	bcore = button_list[button];
	desc = labeltab.get_label(bcore->label);
	name = makestr(btn_name_box->get_name());
	mne_str = btn_name_box->get_mne();
	mbtowc(&mne, mne_str, MB_CUR_MAX);

	btn = new Button(name, mne, bcore); 
	btype = bcore->button_type;
	if (btype == B_exec_cmd ||
			btype == B_debug_cmd)
	{
		cmd = makestr(btn_name_box->get_cmd());
		btn->set_udata((void *)cmd);
	}
	else 
	{
		cmd = "";
		if (btype == B_popup)
		{
			Window_descriptor *wd = window_descriptor;
			for(int j = 0; wd; j++, wd = wd->next())
			{
				if (strcmp(wd->get_name(), name) == 0)
				{
					btn->set_udata((void *)j);
					break;
				}
			}
		}
	}
	if ((old_btn = new_bar->get_button(btype, name)) != 0)
	{
		// modifying existing button
		new_bar->replace_button(old_btn, btn);
	}
	else
		new_bar->add_button(btn);

	pane->set_row(button, Gly_check, desc, name, mne_str, cmd);
	if (!bar_count)
		btn_bbar->set_sensitivity(BTN_BB_DEL_ALL, TRUE);
	bar_count++;
	deselect_cb(0, 0);
}

void
Button_dialog::del_btn_cb(void *, void *)
{
	Button_core	*bcore;
	const char	*desc, *name = 0;

	if (current_selection < 0 || current_selection >= rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return;
	}
	bcore = button_list[current_selection];
	if (bcore->button_type == B_popup)
	{
		name = pane->get_cell(current_selection, 2).string;
	}
	desc = labeltab.get_label(bcore->label);
	new_bar->remove_button(bcore->button_type, name);
	pane->set_row(current_selection, Gly_blank, desc, "", "", "");
	bar_count--;
	if (!bar_count)
		btn_bbar->set_sensitivity(BTN_BB_DEL_ALL, FALSE);
	deselect_cb(0, 0);
}

void
Button_dialog::del_all_cb(void *, void *)
{
	Button_core	**blist = button_list;

	pane->deselect_all();
	pane->delay_updates();
	for(int i = 0; i < rows; i++, blist++)
	{
		Button_core	*bcore;
		const char	*desc;

		if (pane->get_cell(i, 0).glyph == Gly_blank)
			continue;
		bcore = button_list[i];
		desc = labeltab.get_label(bcore->label);
		pane->set_row(i, Gly_blank, desc, "", "", "");
	}
	pane->finish_updates();
	new_bar->remove_all();
	bar_count = 0;
	btn_bbar->set_sensitivity(BTN_BB_DEL_ALL, FALSE);
	deselect_cb(0, 0);
}

int 
Button_dialog::setup_list(Boolean bottom_bar, Boolean create_new)
{
	Window_descriptor	*wd = window->get_wdesc();

	delete new_bar;
	if (create_new)
	{
		current_bar = 0;
		new_bar = new Bar_descriptor(bottom_bar);
	}
	else
	{
		// get and save bar descriptor from original window
		// descriptor
		Button_bar		*bbar;
		Bar_descriptor		*bdesc;
		if (bottom_bar)
			bdesc = wd->get_bottom_button_bars();
		else
			bdesc = wd->get_top_button_bars();
		if (!bdesc)
		{
			dialog->clear_msg();
			dialog->error(E_ERROR, GE_no_button_bar);
			new_bar = 0;
			return 0;
		}
		if (bottom_bar)
			bbar = window->get_bottom_bar();
		else
			bbar = window->get_top_bar();
		int index = bbar->get_current_index();
		for(; bdesc && (index > 0); index--)
			bdesc = bdesc->next();
		current_bar = bdesc;
		new_bar = new Bar_descriptor(current_bar);
	}
	bar_count = 0;
	pane->deselect_all();
	pane->delay_updates();

	Button_core	**blist = button_list;
	int		total = BUTTON_BAR_TOTAL;
	Window_descriptor	*nwd = window_descriptor;
	const char		*this_win = wd->get_name();
	for(int i = 0; i < total; i++, blist++)
	{
		const char	*wname = 0;
		if ((*blist)->button_type == B_popup)
		{
			if (nwd->get_flags() & PT_second_src)
				nwd = nwd->next();
			wname = nwd->get_name();
			nwd = nwd->next();
		}
		if (!setup_row(initial, i, *blist, wname))
			continue;
	}
	pane->finish_updates();
	initial = FALSE;
	deselect_cb(0, 0);
	if (bar_count)
		btn_bbar->set_sensitivity(BTN_BB_DEL_ALL, TRUE);
	else
		btn_bbar->set_sensitivity(BTN_BB_DEL_ALL, FALSE);
	return 1;
}

void
Button_dialog::apply(void *, void *)
{
	Bar_info	bi;
	char		*conf_dir = 0;

	if (toggles->is_set(TOGGLE_SAVE))
	{
		conf_dir = config_dir->get_text();
		if (!conf_dir || !*conf_dir)
		{
			dialog->clear_msg();
			dialog->error(E_ERROR, GE_no_config_dir);
			return;
		}
	}
	
	bottom = (which_bar->which_button() == RADIO_BOTTOM_BARS);
	create_new_bar = toggles->is_set(TOGGLE_NEW);
	bi.window_desc = window->get_wdesc();
	bi.old_bdesc = current_bar;
	bi.old_index = -1;
	bi.new_wbar = 0;
	bi.bottom = bottom;

	if (new_bar->get_nbuttons() == 0)
	{
		if (create_new_bar)
			// nothing added
			return;
		// removing
		bi.remove = TRUE;
		bi.new_bdesc = 0;
	}
	else
	{
		bi.remove = FALSE;
		bi.new_bdesc = new Bar_descriptor(new_bar);
	}
	window_set->update_button_bar(&bi);
	if (conf_dir && !save_current_configuration(conf_dir))
	{
		dialog->clear_msg();
		dialog->error(E_ERROR, GE_config_save_fail);
		return;
	}
}
