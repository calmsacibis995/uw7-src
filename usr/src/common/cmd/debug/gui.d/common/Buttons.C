#ident	"@(#)debugger:gui.d/common/Buttons.C	1.6"

#include "Buttons.h"
#include "Link.h"
#include "Panes.h"
#include "Sense.h"
#include "str.h"
#include "Base_win.h"
#include "Windows.h"
#include "Help.h"
#include "Command.h"
#include "Dis.h"
#include "Btn_dlg.h"
#include "Src_pane.h"
#include "Syms_pane.h"
#include "Events.h"
#include "Label.h"

#include <stdlib.h>
#include <string.h>

// list of button core pointers, sorted alphabetically
// by label description - used by dynamic button bar
// configuration
Button_core	**button_list;

Button::Button(Button *btn)
{
	name = makestr(btn->name);
	mnemonic = btn->mnemonic;
	button_core = btn->button_core;
	sub_table = btn->sub_table;
	if (button_core->button_type == B_exec ||
		button_core->button_type == B_debug_cmd)
		user_data = makestr((char *)btn->user_data);
	else
		user_data = btn->user_data;
}

Button::~Button()
{
	delete((char *)name);

	if (button_core->button_type == B_exec ||
		button_core->button_type == B_debug_cmd)
		delete user_data;
}

// static table of Button_core info - order must be same as 
// for CButtons enumeration

static const
Button_core	button_table[] = {
// File
	{ B_windows, LAB_windows_btn, Menu_button, 0, ALL_PANES,
		{ 0, 0, SEN_always }, 0, HELP_windows_menu },
	{ B_sources, LAB_sources_btn, Menu_button, 0, ALL_PANES,
		{ 0, 0, SEN_src_wins}, 0, HELP_sources_menu },
	{ B_window_sets, LAB_window_sets_btn, Menu_button, 0, ALL_PANES,
		{ 0, 0, SEN_win_sets }, 0, HELP_ws_menu },
	{ B_dismiss, LAB_dismiss_btn,  Set_cb,  0, ALL_PANES, 
		{ 0, 0, SEN_always } ,	
		(Callback_ptr)(&Window_set::dismiss),
		HELP_dismiss_cmd },
	{ B_exit, LAB_exit_btn,  Set_cb,  0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::ok_to_quit), HELP_exit_cmd },
	{ B_create_dlg, LAB_create_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::create_dialog_cb),
		HELP_create_dialog },
	{ B_grab_core_dlg, LAB_grab_core_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::grab_core_dialog_cb),
		HELP_grab_core_dialog },
	{ B_grab_proc_dlg, LAB_grab_proc_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::grab_process_dialog_cb), 
		HELP_grab_process_dialog },
	{ B_cd_dlg, LAB_cd_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::cd_dialog_cb),
		HELP_cd_dialog },
	{ B_rel_running, LAB_rel_running_btn,  Set_cb, 0, ALL_PANES, 
		{ 0, SEN_process|SEN_proc_live, 0 },
		(Callback_ptr)(&Window_set::release_running_cb),
		HELP_release_cmd },
	{ B_rel_susp, LAB_rel_susp_btn,  Set_cb, 0, ALL_PANES,
		{ 0, SEN_process|SEN_proc_only, 0 },
		(Callback_ptr)(&Window_set::release_suspended_cb),
		HELP_release_cmd },
	{ B_release, LAB_release_btn,  Menu_button, 0, ALL_PANES,
		{ 0, SEN_process, 0 }, 0, HELP_release_cmd},
	{ B_new_window_set, LAB_new_window_set_btn,  Set_cb, 0, PT_process|PT_source,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::new_window_set_cb),
		HELP_new_window_set_cmd },
	{ B_script_dlg, LAB_script_dlg_btn,  Pane_cb, PT_command, PT_command,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Command_pane::script_dialog_cb),
		HELP_script_dialog},
	{ B_open_dlg, LAB_open_dlg_btn,  Pane_cb, PT_source,  PT_source|PT_second_src,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Source_pane::open_dialog_cb),
		HELP_open_dialog },
	{ B_new_source, LAB_new_source_btn,  Pane_cb, PT_source, PT_source|PT_second_src,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Source_pane::new_source_cb),
		HELP_new_source_cmd },
	{ B_save, LAB_save_btn,  Pane_cb, PT_source, 
		PT_source|PT_second_src,
		{ 0, 0, SEN_file_required },
		(Callback_ptr)(&Source_pane::save_file_cb),
		HELP_save_cmd },
	{ B_save_as_dlg, LAB_save_as_dlg_btn,  Pane_cb, PT_source, 
		PT_source|PT_second_src,
		{ 0, 0, SEN_file_required },
		(Callback_ptr)(&Source_pane::save_as_and_set_cb),
		HELP_save_as_dlg },
// Edit
	{ B_copy, LAB_copy_btn,  Window_cb, 0,
		PT_command|PT_source|PT_disassembler|PT_second_src,
		{ SEN_text_sel, 0, 0 },
		(Callback_ptr)(&Base_window::copy_cb),
		HELP_copy_cmd },
	{ B_move_dlg, LAB_move_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, SEN_process, 0 },
		(Callback_ptr)(&Window_set::move_to_ws_cb),
		HELP_move_dialog },
	{ B_set_current, LAB_set_current_btn,  Set_cb, 0, PT_stack|PT_process,
		{ (SEN_frame_sel|SEN_process_sel|SEN_single_sel), 0, 0 },
		(Callback_ptr)(&Window_set::set_current_cb),
		HELP_set_current_cmd },
	{ B_input_dlg, LAB_input_dlg_btn,  Pane_cb, PT_command, PT_command,
		{ 0, SEN_process|SEN_proc_live|SEN_proc_io_redirected,
			SEN_animated },	
		(Callback_ptr)(&Command_pane::input_dialog_cb),
		HELP_input_dialog },
	{ B_interrupt, LAB_interrupt_btn,  Pane_cb, PT_command, PT_command,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Command_pane::interrupt_cb),
		HELP_interrupt_cmd },
	{ B_disable, LAB_disable_btn,  Pane_cb, PT_event, PT_event,
		{ SEN_event_able_sel|SEN_event_sel, SEN_process|SEN_proc_stopped, 0 },
		(Callback_ptr)(&Event_pane::disableEventCb),
		HELP_disable_event_cmd },
	{ B_enable, LAB_enable_btn,  Pane_cb, PT_event, PT_event,
		{ SEN_event_dis_sel|SEN_event_sel, SEN_process|SEN_proc_stopped, 0 },
		(Callback_ptr)(&Event_pane::enableEventCb),
		HELP_enable_event_cmd },
	{ B_delete, LAB_delete_btn,  Pane_cb, PT_event, PT_event,
		{ SEN_event_sel, SEN_process|SEN_proc_stopped, 0 },
		(Callback_ptr)(&Event_pane::deleteEventCb),
		HELP_delete_event_cmd },
	{ B_change_dlg, LAB_change_dlg_btn,  Pane_cb, PT_event, PT_event,
		{ (SEN_event_sel|SEN_single_sel), SEN_process|SEN_proc_stopped, 0 },
		(Callback_ptr)(&Event_pane::changeEventCb),
		HELP_change_dialog },
	{ B_cut, LAB_cut_btn,  Pane_cb, PT_source, PT_source|PT_second_src,
		{ SEN_source_sel, 0, 0 },
		(Callback_ptr)(&Source_pane::cut_cb),
		HELP_cut_cmd },
	{ B_delete_text, LAB_delete_text_btn,  Pane_cb, PT_source, 
		PT_source|PT_second_src,
		{ SEN_source_sel, 0, 0 },
		(Callback_ptr)(&Source_pane::delete_cb),
		HELP_delete_text_cmd},
	{ B_paste, LAB_paste_btn,  Pane_cb, PT_source, PT_source|PT_second_src,
		{ 0, 0, SEN_paste_available },
		(Callback_ptr)(&Source_pane::paste_cb),
		HELP_paste_cmd},
	{ B_undo, LAB_undo_btn,  Pane_cb, PT_source, PT_source|PT_second_src,
		{ 0, 0, SEN_undo_pending },
		(Callback_ptr)(&Source_pane::undo_cb),
		HELP_undo_cmd},
// View
	{ B_map_dlg, LAB_map_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ SEN_single_sel, SEN_process, 0 },
		(Callback_ptr)(&Window_set::map_dialog_cb),
		HELP_map_dialog },
	{ B_export, LAB_export_btn,  Pane_cb, PT_symbols, PT_symbols,
		{ SEN_user_symbol|SEN_symbol_sel, 0, 0 },
		(Callback_ptr)(&Symbols_pane::export_syms_cb),
		HELP_export_cmd },
	{ B_pin, LAB_pin_btn,  Pane_cb, PT_symbols, PT_symbols,
		{ (SEN_symbol_sel|SEN_sel_has_unpin_sym), 0, 0 },
		(Callback_ptr)(&Symbols_pane::pin_sym_cb),
		HELP_sym_pin_cmd },
	{ B_unpin, LAB_unpin_btn,  Pane_cb, PT_symbols, PT_symbols,
		{ (SEN_symbol_sel|SEN_sel_has_pin_sym), 0, 0 },
		(Callback_ptr)(&Symbols_pane::unpin_sym_cb),
		HELP_sym_unpin_cmd },
	{ B_show_value_dlg, LAB_show_value_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::show_value_dialog_cb),
		HELP_show_value_dialog },
	{ B_set_value_dlg, LAB_set_value_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::set_value_dialog_cb),
		HELP_set_value_dialog },
	{ B_show_type_dlg, LAB_show_type_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::show_type_dialog_cb),
		HELP_show_type_dialog },
	{ B_dump_dlg, LAB_dump_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ SEN_single_sel, (SEN_process|SEN_proc_stopped_core), 0 },
		(Callback_ptr)(&Window_set::dump_dialog_cb),
		HELP_dump_dialog },
	{ B_show_line_dlg, LAB_show_line_dlg_btn,  Pane_cb, PT_source,  
		PT_source|PT_second_src,
		{ 0, 0, SEN_file_required },
		(Callback_ptr)(&Source_pane::show_line_cb),
		HELP_show_line_dialog },
	{ B_show_func_source_dlg, LAB_show_func_source_dlg_btn,  Pane_cb, PT_source, 
		PT_source|PT_second_src,
		{ 0, SEN_process, 0 },
		(Callback_ptr)(&Source_pane::show_function_cb),
		HELP_show_source_function_dialog },
	{ B_search_dlg, LAB_search_dlg_btn,  Window_cb, 0,
		PT_source|PT_disassembler|PT_second_src,
		{ 0, 0, (SEN_file_required|SEN_disp_dis_required) },
		(Callback_ptr)(&Base_window::search_dialog_cb),
		HELP_search_dialog },
	{ B_show_loc_dlg, LAB_show_loc_dlg_btn,  Pane_cb, PT_disassembler, PT_disassembler,
		{ 0, SEN_process, 0 },
		(Callback_ptr)(&Disassembly_pane::show_loc_cb),
		HELP_show_location_dialog },
	{ B_show_func_dis_dlg, LAB_show_func_dis_dlg_btn,  Pane_cb, PT_disassembler, PT_disassembler,
		{ 0, SEN_process, 0 },
		(Callback_ptr)(&Disassembly_pane::show_function_cb),
		HELP_show_dis_function_dialog },
// Control
	{ B_run, LAB_run_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::run_button_cb),
		HELP_run_cmd},
	{ B_return, LAB_return_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::run_r_button_cb),
		HELP_return_cmd },
	{ B_run_until_dlg, LAB_run_until_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::run_dialog_cb),
		HELP_run_dialog },
	{ B_step_statement, LAB_step_statement_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::step_button_cb),
		HELP_step_stmt_cmd },
	{ B_step_instruction, LAB_step_instruction_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::step_i_button_cb),
		HELP_step_instr_cmd },
	{ B_next_statement, LAB_next_statement_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::step_o_button_cb),
		HELP_next_stmt_cmd },
	{ B_next_instruction, LAB_next_instruction_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::step_oi_button_cb),
		HELP_next_instr_cmd },
	{ B_step_dlg, LAB_step_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::step_dialog_cb),
		HELP_step_dialog },
	{ B_jump_dlg, LAB_jump_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::jump_dialog_cb),
		HELP_jump_dialog },
	{ B_halt, LAB_halt_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_running), SEN_animated },
		(Callback_ptr)(&Window_set::halt_button_cb),
		HELP_halt_cmd },
	{ B_animate_source, LAB_animate_source_btn,  Set_cb, 0, PT_source,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::animate_src_cb),
		HELP_animate_source_cmd },
	{ B_animate_dis, LAB_animate_dis_btn,  Set_cb, 0, PT_disassembler,
		{ 0, (SEN_process|SEN_proc_runnable), 0 },
		(Callback_ptr)(&Window_set::animate_dis_cb),
		HELP_animate_dis_cmd },
// Event
	{ B_stop_on_func_dlg, LAB_stop_on_func_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ SEN_single_sel, (SEN_process|SEN_proc_stopped), 0 },
		(Callback_ptr)(&Window_set::stop_on_function_cb),
		HELP_stop_on_function_dialog },
	{ B_stop_dlg, LAB_stop_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_stopped), 0 },
		(Callback_ptr)(&Window_set::stop_dialog_cb),
		HELP_stop_dialog },
	{ B_signal_dlg, LAB_signal_dlg_btn,   Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_stopped), 0 },
		(Callback_ptr)(&Window_set::signal_dialog_cb),
		HELP_signal_dialog },
	{ B_syscall_dlg, LAB_syscall_dlg_btn,   Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_stopped), 0 },
		(Callback_ptr)(&Window_set::syscall_dialog_cb),
		HELP_syscall_dialog },
	{ B_on_stop_dlg, LAB_on_stop_dlg_btn,   Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_stopped), 0 },
		(Callback_ptr)(&Window_set::onstop_dialog_cb),
		HELP_on_stop_dialog },
#if EXCEPTION_HANDLING
	{ B_exception_dlg, LAB_exception_dlg_btn,   Set_cb, 0, ALL_PANES,
		{ 0, (SEN_process|SEN_proc_stopped|SEN_proc_uses_eh), 0 },
		(Callback_ptr)(&Window_set::exception_dialog_cb),
		HELP_exception_dialog },
	{ B_ignore_exceptions_dlg, LAB_ignore_exceptions_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::ignore_exceptions_dialog_cb),
		HELP_ignore_exceptions_dialog },
#endif
	{ B_cancel_dlg, LAB_cancel_dlg_btn,   Set_cb, 0, ALL_PANES,
		{ SEN_single_sel, (SEN_process|SEN_proc_stopped), 0 },
		(Callback_ptr)(&Window_set::cancel_dialog_cb),
		HELP_cancel_dialog },
	{ B_destroy, LAB_destroy_btn,  Set_cb, 0, ALL_PANES,
		{ 0, SEN_process, SEN_animated },
		(Callback_ptr)(&Window_set::destroy_process_cb),
		HELP_destroy_cmd, },
	{ B_kill_dlg, LAB_kill_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, SEN_process|SEN_proc_live, 0 },
		(Callback_ptr)(&Window_set::kill_dialog_cb),
		HELP_kill_dialog },
	{ B_ignore_signals_dlg, LAB_ignore_signals_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::setup_signals_dialog_cb),
		HELP_ignore_signals_dialog },
    	{ B_set_watch, LAB_set_watch_btn,  Pane_cb, PT_symbols, PT_symbols,
		{ SEN_program_symbol|SEN_symbol_sel, 
			SEN_process|SEN_proc_stopped, 0 },
		(Callback_ptr)(&Symbols_pane::set_watchpoint_cb),
		HELP_set_watchpoint_cmd },
	{ B_set_break, LAB_set_break_btn,  Window_cb, 0, 
		PT_source|PT_disassembler|PT_second_src,
		{ SEN_source_sel|SEN_dis_sel, 
			SEN_process|SEN_proc_stopped, 
			0 },
		(Callback_ptr)(&Base_window::set_break_cb),
		HELP_set_breakpoint_cmd },
	{ B_delete_break, LAB_delete_break_btn,  Window_cb, 0, 
		PT_source|PT_disassembler|PT_second_src,
		{ SEN_source_sel|SEN_dis_sel, 
			SEN_process|SEN_proc_stopped,
		  SEN_breakpt_required},
		(Callback_ptr)(&Base_window::delete_break_cb),
		HELP_delete_breakpoint_cmd },
// Properties
	{ B_language_dlg, LAB_language_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::set_language_dialog_cb),
		HELP_language_dialog },
	{ B_granularity_dlg, LAB_granularity_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::set_granularity_cb),
		HELP_granularity_dialog },
	{ B_output_dlg, LAB_output_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::action_dialog_cb),
		HELP_action_dialog },
	{ B_path_dlg, LAB_path_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::path_dialog_cb),
		HELP_path_dialog },
	{ B_symbols_dlg, LAB_symbols_dlg_btn,  Pane_cb, PT_symbols, PT_symbols,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Symbols_pane::setup_syms_cb),
		HELP_symbols_dialog},
	{ B_animation_dlg, LAB_animation_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::animation_dialog_cb),
		HELP_animation_dialog },
	{ B_dis_mode_dlg, LAB_dis_mode_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::set_dis_mode_cb),
		HELP_dis_mode_dialog },
	{ B_frame_dir_dlg, LAB_frame_dir_dlg_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::frame_dir_dlg_cb),
		HELP_frame_dir_dlg },
	{ B_button_dlg, LAB_button_dlg,  Window_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Base_window::button_dialog_cb),
		HELP_button_dialog },
// Help
	{ B_table_of_cont_hlp, LAB_table_of_cont_hlp_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Window_set::help_toc_cb),
		HELP_contents },
#if OLD_HELP
	{ B_help_desk_hlp, LAB_help_desk_hlp_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Window_set::help_helpdesk_cb),
		HELP_help_desk },
#endif
	{ B_version, LAB_version_btn,  Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::version_cb),
		HELP_version},
	{ B_process_pane_hlp, LAB_process_pane_hlp_btn,  Window_cb, 0, PT_process,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_pane_help, HELP_ps_pane },
	{ B_stack_pane_hlp, LAB_stack_pane_hlp_btn,  Window_cb, 0, PT_stack,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_pane_help, HELP_stack_pane },
	{ B_syms_pane_hlp, LAB_syms_pane_hlp_btn,  Window_cb, 0, PT_symbols,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_pane_help, HELP_syms_pane },
	{ B_command_pane_hlp, LAB_command_pane_hlp_btn,  Window_cb, 0, PT_command,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb),
		HELP_pane_help,  HELP_command_pane },
	{ B_source_pane_hlp, LAB_source_pane_hlp_btn,  Window_cb, 0, PT_source|PT_second_src,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_pane_help, HELP_source_pane },
	{ B_dis_pane_hlp, LAB_dis_pane_hlp_btn,  Window_cb, 0, PT_disassembler,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_pane_help, HELP_dis_pane },
	{ B_regs_pane_hlp, LAB_regs_pane_hlp_btn,  Window_cb, 0, PT_registers,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_pane_help, HELP_regs_pane },
	{ B_event_pane_hlp, LAB_event_pane_hlp_btn,  Window_cb, 0, PT_event,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb) , 
		HELP_pane_help, HELP_event_pane },
	{ B_status_pane_hlp, LAB_status_pane_hlp_btn,  Window_cb, 0, PT_status,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Base_window::help_sect_cb) , 
		HELP_pane_help, HELP_status_pane },
// New and misc
	{ B_popup, LAB_popup_btn, Set_data_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Window_set::popup_window_cb),
		HELP_popup_button },
	{ B_save_layout, LAB_save_layout_btn, Set_cb, 0, ALL_PANES,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Window_set::save_current_layout_cb),
		HELP_save_layout },
	{ B_debug_cmd, LAB_debug_cmd_btn, Window_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Base_window::debug_cmd_cb),
		HELP_debug_cmd },
	{ B_exec_cmd, LAB_exec_cmd_btn, Window_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Base_window::exec_cb),
		HELP_exec_cmd },
// All buttons that cannot be part of a window button bar go
// after here - all buttons that can be, go before
	{ B_separator, LAB_none, Menu_separator, 0, ALL_PANES,
		{ 0, 0, 0 }, 0, HELP_none },	// dummy entry, just
						// for flags
	{ B_menu, LAB_none, Menu_button, 0, ALL_PANES,
		{ 0, 0, 0 }, 0, HELP_none },	// dummy entry, just
						// for type
	{ B_window_popup, LAB_none, Window_cb, 0, ALL_PANES,
		{ 0, 0, SEN_all_but_script },
		(Callback_ptr)(&Base_window::popup_window),
		HELP_sources_menu },
	{ B_set_popup, LAB_none, Set_data_cb, 0, ALL_PANES,
		{ 0, 0, SEN_always },
		(Callback_ptr)(&Window_set::popup_ws),
		HELP_windows_menu },
// button dialog
	{ B_btn_dlg_add, LAB_none, Creator_cb, 0, ALL_PANES,
		{ 0, 0, 0 },
		(Callback_ptr)(&Button_dialog::button_name_cb),
		HELP_none },
	{ B_btn_dlg_change, LAB_none, Creator_cb, 0, ALL_PANES,
		{ 0, 0, 0 },
		(Callback_ptr)(&Button_dialog::button_name_cb),
		HELP_none },
	{ B_btn_dlg_del, LAB_none, Creator_cb, 0, ALL_PANES,
		{ 0, 0, 0 },
		(Callback_ptr)(&Button_dialog::del_btn_cb),
		HELP_none },
	{ B_btn_dlg_del_all, LAB_none, Creator_cb, 0, ALL_PANES,
		{ 0, 0, 0 },
		(Callback_ptr)(&Button_dialog::del_all_cb),
		HELP_none },
};


static const char *btable[] = {
	"f.windows_menu",
	"f.sources_menu",
	"f.window_sets_menu",
	"f.close_window",
	"f.exit",
	"f.create_dialog",
	"f.grab_core_dialog",
	"f.grab_process_dialog",
	"f.cd_dialog",
	"f.release_running",
	"f.release_suspended",
	"f.release",
	"f.new_window_set",
	"f.script_dialog",
	"f.open_dialog",
	"f.new_source",
	"f.save",
	"f.save_as_dialog",
	"f.copy",
	"f.move_dialog",
	"f.set_current",
	"f.input_dialog",
	"f.interrupt",
	"f.disable_event",
	"f.enable_event",
	"f.delete_event",
	"f.change_dialog",
	"f.cut",
	"f.delete",
	"f.paste",
	"f.undo",
	"f.map_dialog",
	"f.export",
	"f.pin",
	"f.unpin",
	"f.show_value_dialog",
	"f.set_value_dialog",
	"f.show_type_dialog",
	"f.dump_dialog",
	"f.show_line_dialog",
	"f.show_function_source_dialog",
	"f.search_dialog",
	"f.show_location_dialog",
	"f.show_function_dis_dialog",
	"f.run",
	"f.return",
	"f.run_until_dialog",
	"f.step_statement",
	"f.step_instruction",
	"f.next_statement",
	"f.next_instruction",
	"f.step_dialog",
	"f.jump_dialog",
	"f.halt",
	"f.animate_source",
	"f.animate_disassembly",
	"f.stop_on_function_dialog",
	"f.stop_dialog",
	"f.signal_dialog",
	"f.syscall_dialog",
	"f.on_stop_dialog",
#if EXCEPTION_HANDLING
	"f.exception_dialog",
	"f.ignore_exceptions_dialog",
#endif
	"f.cancel_dialog",
	"f.destroy",
	"f.kill_dialog",
	"f.ignore_signals_dialog",
	"f.set_watchpoint",
	"f.set_breakpoint",
	"f.delete_breakpoint",
	"f.language_dialog",
	"f.granularity_dialog",
	"f.output_dialog",
	"f.path_dialog",
	"f.symbols_dialog",
	"f.animation_dialog",
	"f.disassembly_mode_dialog",
	"f.frame_direction_dialog",
	"f.button_dialog",
	"f.table_of_contents_help",
#if OLD_HELP
	"f.help_desk_help",
#endif
	"f.version",
	"f.process_pane_help",
	"f.stack_pane_help",
	"f.symbols_pane_help",
	"f.command_pane_help",
	"f.source_pane_help",
	"f.disassembly_pane_help",
	"f.registers_pane_help",
	"f.event_pane_help",
	"f.status_pane_help",
	"f.popup",
	"f.save_layout",
	"f.debug_command",
	"f.exec",
	"f.separator",
	"f.menu",
	"f.window_popup",
	"f.set_popup",
	"f.button_add",
	"f.button_change",
	"f.button_del",
	"f.button_del_all",
	0
};

const char *
button_action(CButtons btype)
{
	if (btype >= B_last || (int)btype < 0)
		return 0;
	return(btable[(int)btype]);
}

Button_core *
find_button( CButtons btype )
{
	if (btype >= B_last || (int)btype < 0)
		return 0;
	return((Button_core *)button_table + (int)btype);
}

Button_core *
find_button( register const char *s )
{
	static int init_done = 0;
	register const char **p;

	if ( !init_done ) 
	{
		for ( p = btable ; *p; p++ )
			*p = str(*p);
		init_done = 1;
	}

	s = strlook(s);		// so can compare ptrs

	for ( p = btable ; *p; p++ )
		if ( s == *p )
			break;

	if (*p)	// zero if at the end of the table
		return (Button_core *)&button_table[p - btable];
	else
		return 0;
}

static int
button_compare(const Button_core **b1, const Button_core **b2)
{
	const	char	*name1 = labeltab.get_label((*b1)->label);
	const	char	*name2 = labeltab.get_label((*b2)->label);
	return strcmp(name1, name2);
}

void
build_button_list()
{
	Button_core	**bl;
	Button_core	*bptr = (Button_core *)button_table;
	int		total = BUTTON_BAR_TOTAL;
	int		i = 0;

	bl = button_list = new Button_core *[total];
	while(i < total)
	{
		if (bptr->button_type == B_popup)
		{
			for(int j = 0; j < windows_per_set-1; j++,i++)
			{
				// don't include one for 
				// secondary source window
				*bl++ = bptr;
			}
			bptr++;
		}
		else
		{
			*bl++ = bptr++;
			i++;
		}
	}
	qsort(button_list, total, sizeof(Button_core*), 
		(int (*)(const void *, const void *))button_compare);
}
