#ident	"@(#)debugger:gui.d/common/Help.C	1.29"

#include "Help.h"

#if OLD_HELP
char	*Help_path;
static char gui_file[] = "debug.hlp";

Help_info Help_files[HELP_final] =
{
	{0,0,0},			// HELP_none
	{gui_file,"1"},			// HELP_window

	// Panes
	{gui_file,"2"},		// HELP_ps_pane
	{gui_file,"3"},		// HELP_stack_pane
	{gui_file,"4"},		// HELP_syms_pane
	{gui_file,"7"},		// HELP_source_pane
	{gui_file,"7261"},	// HELP_save_query_dlg,
	{gui_file,"5"},		// HELP_regs_pane
	{gui_file,"6"},		// HELP_dis_pane
	{gui_file,"8"},		// HELP_event_pane
	{gui_file,"9"},		// HELP_command_pane
	{gui_file,"91"},	// HELP_transcript_pane
	{gui_file,"92"},	// HELP_command_line
	{gui_file,"10"},	// HELP_status_pane

	// File menu
	{gui_file,"111"},	// HELP_cd_dialog
	{gui_file,"112"},	// HELP_dismiss_cmd,
	{gui_file,"113"},	// HELP_exit_cmd,
	{gui_file,"1131"},	// HELP_save_open_source_dlg,
	{gui_file,"114"},	// HELP_move_dialog,
	{gui_file,"115"},	// HELP_new_source_cmd
	{gui_file,"116"},	// HELP_new_window_set_cmd,
	{gui_file,"117"},	// HELP_open_dialog
	{gui_file,"118"},	// HELP_save_cmd,
	{gui_file,"119"},	// HELP_save_as_dlg,
	{gui_file,"1110"},	// HELP_save_layout,
	{gui_file,"1111"},	// HELP_script_dialog
	{gui_file,"1112"},	// HELP_sources_menu,
	{gui_file,"1113"},	// HELP_ws_menu,
	{gui_file,"1114"},	// HELP_windows_menu,
	{gui_file,"11141"},	// HELP_popup_button,

	// Debug Menu
	{gui_file,"121"},	// HELP_create_dialog
	{gui_file,"122"},	// HELP_grab_core_dialog
	{gui_file,"123"},	// HELP_grab_process_dialog
	{gui_file,"124"},	// HELP_release_cmd

	// Edit menu
	{gui_file,"131"},		// HELP_copy_cmd
	{gui_file,"132"},		// HELP_cut_cmd,
	{gui_file,"133"},		// HELP_delete_event_cmd
	{gui_file,"134"},		// HELP_delete_text_cmd,
	{gui_file,"135"},		// HELP_disable_event_cmd
	{gui_file,"136"},		// HELP_enable_event_cmd
	{gui_file,"137"},		// HELP_export_cmd
	{gui_file,"138"},		// HELP_input_dialog
	{gui_file,"139"},		// HELP_interrupt_cmd
	{gui_file,"1310"},		// HELP_paste_cmd,
	{gui_file,"1311"},		// HELP_sym_pin_cmd,
	{gui_file,"1312"},		// HELP_set_current_cmd
	{gui_file,"1313"},		// HELP_undo_cmd,
	{gui_file,"1314"},		// HELP_sym_unpin_cmd,

	// View menu
	{gui_file,"141"},		// HELP_dump_dialog,
	{gui_file,"142"},		// HELP_map_dialog,
	{gui_file,"143"},		// HELP_search_dialog,
	{gui_file,"144"},		// HELP_set_value_dialog,
	{gui_file,"145"},		// HELP_show_dis_function_dialog,
	{gui_file,"146"},		// HELP_show_source_function_dialog,
	{gui_file,"147"},		// HELP_show_line_dialog,
	{gui_file,"148"},		// HELP_show_location_dialog,
	{gui_file,"149"},		// HELP_show_type_dialog,
	{gui_file,"1410"},		// HELP_show_value_dialog,

	// Control menu
	{gui_file,"151"},		// HELP_animate_dis_cmd,
	{gui_file,"152"},		// HELP_animate_source_cmd,
	{gui_file,"153"},		// HELP_jump_dialog,
	{gui_file,"154"},		// HELP_halt_cmd,
	{gui_file,"155"},		// HELP_next_instr_cmd,
	{gui_file,"156"},		// HELP_next_stmt_cmd,
	{gui_file,"157"},		// HELP_return_cmd,
	{gui_file,"158"},		// HELP_run_cmd,
	{gui_file,"159"},		// HELP_run_dialog,
	{gui_file,"1510"},		// HELP_step_dialog,
	{gui_file,"1511"},		// HELP_step_instr_cmd,
	{gui_file,"1512"},		// HELP_step_stmt_cmd,

	// Event menu
	{gui_file,"161"},		// HELP_cancel_dialog,
	{gui_file,"162"},		// HELP_change_dialog,
	{gui_file,"163"},		// HELP_delete_breakpoint_cmd,
	{gui_file,"164"},		// HELP_destroy_cmd,
#if EXCEPTION_HANDLING
	{gui_file,"165"},		// HELP_exception_dialog,
	{gui_file,"166"},		// HELP_ignore_exceptions_dialog,
#endif
	{gui_file,"167"},		// HELP_ignore_signals_dialog,
	{gui_file,"168"},		// HELP_kill_dialog,
	{gui_file,"169"},		// HELP_on_stop_dialog,
	{gui_file,"1610"},		// HELP_set_breakpoint_cmd,
	{gui_file,"1611"},		// HELP_set_watchpoint_cmd,
	{gui_file,"1612"},		// HELP_signal_dialog,
	{gui_file,"1613"},		// HELP_stop_dialog,
	{gui_file,"1614"},		// HELP_stop_on_function_dialog,
	{gui_file,"1615"},		// HELP_syscall_dialog,

	// Properties menu
	{gui_file,"171"},		// HELP_animation_dialog
	{gui_file,"172"},		// HELP_button_dialog
	{gui_file,"173"},		// HELP_dis_mode_dialog
	{gui_file,"174"},		// HELP_frame_dir_dialog
	{gui_file,"175"},		// HELP_granularity_dialog,
	{gui_file,"176"},		// HELP_language_dialog,
	{gui_file,"177"},		// HELP_action_dialog
	{gui_file,"178"},		// HELP_path_dialog,
	{gui_file,"179"},		// HELP_symbols_dialog,
#ifndef MOTIF_UI
	{gui_file,"1710"},		// HELP_panes_dialog,
#endif

	// Help Menu
	{gui_file,"181"},		// HELP_help_desk,
	{gui_file,"182"},		// HELP_pane_help,
	{gui_file,"183"},		// HELP_contents,
	{gui_file,"184"},		// HELP_version,

	// Misc
	{gui_file,"191"},		// HELP_debug_cmd,
	{gui_file,"192"},		// HELP_exec_cmd,
};

#else

char	*Help_path = "SDK_cdebug";

Help_info Help_files[HELP_final] =
{
	{0,0,0},			// HELP_none
	{0, "CTOC-_Using_the_Graphical_Interface_o.html"},	// HELP_window

	// Panes
	{0, "_Process_pane.html"},		// HELP_ps_pane
	{0, "_Stack_pane.html"},		// HELP_stack_pane
	{0, "_Symbol_pane.html"},		// HELP_syms_pane
	{0, "_Source_pane.html"},		// HELP_source_pane
	{0, "_Selecting_Text.html#_Query_File_Save"},	// HELP_save_query_dlg,
	{0, "_Registers_pane.html"},		// HELP_regs_pane
	{0, "_Disassembly_pane.html"},	// HELP_dis_pane
	{0, "_Event_pane.html"},		// HELP_event_pane
	{0, "_Command_pane.html"},		// HELP_command_pane
	{0, "_Command_pane.html"},		// HELP_transcript_pane
	{0, "_Command_pane.html"},		// HELP_command_line
	{0, "_Status_pane.html"},		// HELP_status_pane

	// File menu
	{0, "_File_menu.html#_Change_Directory"},	// HELP_cd_dialog
	{0, "_File_menu.html#_Close_Window"},		// HELP_dismiss_cmd,
	{0, "_File_menu.html#_Exit"},			// HELP_exit_cmd,
	{0, "_File_menu.html#_Save_Open_Source_Files"},	// HELP_save_open_source_dlg,
	{0, "_File_menu.html#_Move"},			// HELP_move_dialog,
	{0, "_File_menu.html#_New_Source"},		// HELP_new_source_cmd
	{0, "_File_menu.html#_New_Window_Set"},		// HELP_new_window_set_cmd,
	{0, "_File_menu.html#_Open_Source"},		// HELP_open_dialog
	{0, "_File_menu.html#_Save"},			// HELP_save_cmd,
	{0, "_File_menu.html#_Save_As"},		// HELP_save_as_dlg,
	{0, "_File_menu.html#_Save_Layout"},		// HELP_save_layout,
	{0, "_File_menu.html#_Script"},			// HELP_script_dialog
	{0, "_File_menu.html#_Sources"},		// HELP_sources_menu,
	{0, "_File_menu.html#_Window_Sets"},		// HELP_ws_menu,
	{0, "_File_menu.html#_Windows"},		// HELP_windows_menu,
	{0, "_File_menu.html#_Popup_Window_Button"},	// HELP_popup_button,

	// Debug Menu
	{0, "_Debug_menu.html#UGI_Create"},		// HELP_create_dialog
	{0, "_Debug_menu.html#UGI_GrabCore"},		// HELP_grab_core_dialog
	{0, "_Debug_menu.html#UGI_GrabProc"},		// HELP_grab_process_dialog
	{0, "_Debug_menu.html#UGI_Release"},		// HELP_release_cmd

	// Edit menu
	{0, "_Edit_menu.html#_Copy"},			// HELP_copy_cmd
	{0, "_Edit_menu.html#_Cut"},			// HELP_cut_cmd,
	{0, "_Edit_menu.html#_Delete_Event"},		// HELP_delete_event_cmd
	{0, "_Edit_menu.html#_Delete_Text"},		// HELP_delete_text_cmd,
	{0, "_Edit_menu.html#_Disable"},		// HELP_disable_event_cmd
	{0, "_Edit_menu.html#_Enable"},			// HELP_enable_event_cmd
	{0, "_Edit_menu.html#_Export"},			// HELP_export_cmd
	{0, "_Edit_menu.html#_Input"},			// HELP_input_dialog
	{0, "_Edit_menu.html#_Interrupt"},		// HELP_interrupt_cmd
	{0, "_Edit_menu.html#_Paste"},			// HELP_paste_cmd,
	{0, "_Edit_menu.html#_Pin"},			// HELP_sym_pin_cmd,
	{0, "_Edit_menu.html#_Set_Current"},		// HELP_set_current_cmd
	{0, "_Edit_menu.html#_Undo"},			// HELP_undo_cmd,
	{0, "_Edit_menu.html#_Unpin"},			// HELP_sym_unpin_cmd,

	// View menu
	{0, "_View_menu.html#_Dump"},			// HELP_dump_dialog,
	{0, "_View_menu.html#_Map"},			// HELP_map_dialog,
	{0, "_View_menu.html#_Search"},			// HELP_search_dialog,
	{0, "_View_menu.html#_Set_Value"},		// HELP_set_value_dialog,
	{0, "_View_menu.html#_Show_Function_Dis"},	// HELP_show_dis_function_dialog,
	{0, "_View_menu.html#_Show_Function_Source"},	// HELP_show_source_function_dialog,
	{0, "_View_menu.html#_Show_Line"},		// HELP_show_line_dialog,
	{0, "_View_menu.html#_Show_Location"},		// HELP_show_location_dialog,
	{0, "_View_menu.html#_Show_Type"},		// HELP_show_type_dialog,
	{0, "_View_menu.html#_Show_Value"},		// HELP_show_value_dialog,

	// Control menu
	{0, "_Control_menu.html#_Animate_Disassembly"},	// HELP_animate_dis_cmd,
	{0, "_Control_menu.html#_Animate_Source"},	// HELP_animate_source_cmd,
	{0, "_Control_menu.html#_Jump"},		// HELP_jump_dialog,
	{0, "_Control_menu.html#_Halt"},		// HELP_halt_cmd,
	{0, "_Control_menu.html#_Next_Instruction"},	// HELP_next_instr_cmd,
	{0, "_Control_menu.html#_Next_Statement"},	// HELP_next_stmt_cmd,
	{0, "_Control_menu.html#_Return"},		// HELP_return_cmd,
	{0, "_Control_menu.html#_Run"},			// HELP_run_cmd,
	{0, "_Control_menu.html#_Run_Until"},		// HELP_run_dialog,
	{0, "_Control_menu.html#_Step"},		// HELP_step_dialog,
	{0, "_Control_menu.html#_Step_Instruction"},	// HELP_step_instr_cmd,
	{0, "_Control_menu.html#_Step_Statement"},	// HELP_step_stmt_cmd,

	// Event menu
	{0, "_Event_menu.html#_Cancel"},		// HELP_cancel_dialog,
	{0, "_Event_menu.html#_Change"},		// HELP_change_dialog,
	{0, "_Event_menu.html#_Delete_Breakpoint"},	// HELP_delete_breakpoint_cmd,
	{0, "_Event_menu.html#_Destroy"},		// HELP_destroy_cmd,
#if EXCEPTION_HANDLING
	{0, "_Event_menu.html#_Exception"},		// HELP_exception_dialog,
	{0, "_Event_menu.html#_Ignore_Exceptions"},	// HELP_ignore_exceptions_dialog,
#endif
	{0, "_Event_menu.html#UGI_IgnoreSigs"},		// HELP_ignore_signals_dialog,
	{0, "_Event_menu.html#_Kill"},			// HELP_kill_dialog,
	{0, "_Event_menu.html#_On_Stop"},		// HELP_on_stop_dialog,
	{0, "_Event_menu.html#_Set_Breakpoint"},	// HELP_set_breakpoint_cmd,
	{0, "_Event_menu.html#_Set_Watchpoint"},	// HELP_set_watchpoint_cmd,
	{0, "_Event_menu.html#UGI_Sig"},		// HELP_signal_dialog,
	{0, "_Event_menu.html#UGI_Stop"},		// HELP_stop_dialog,
	{0, "_Event_menu.html#_Stop_on_Function"},	// HELP_stop_on_function_dialog,
	{0, "_Event_menu.html#UGI_Syscall"},		// HELP_syscall_dialog,

	// Properties menu
	{0, "_Properties_menu.html#_Animation"},	// HELP_animation_dialog
	{0, "_Properties_menu.html#_Button_Configuration"}, // HELP_button_dialog
	{0, "_Properties_menu.html#_Disassembly_Mode"},	// HELP_dis_mode_dialog
	{0, "_Properties_menu.html#_Frame_Direction"},	// HELP_frame_dir_dialog
	{0, "_Properties_menu.html#UGI_Granularity"},	// HELP_granularity_dialog,
	{0, "_Properties_menu.html#_Language"},		// HELP_language_dialog,
	{0, "_Properties_menu.html#UGI_OutAction"},	// HELP_action_dialog
	{0, "_Properties_menu.html#_Source_Path"},	// HELP_path_dialog,
	{0, "_Properties_menu.html#UGI_Syms"},		// HELP_symbols_dialog,

	// Help Menu
	{0, "_Help_menu.html#_Pane_Help"},		// HELP_pane_help,
	{0, "_Help_menu.html#_Table_of_Contents"},	// HELP_contents,
	{0, "_Help_menu.html#_Version"},		// HELP_version,

	// Misc
	{0, "_Miscellaneous_Options.html#_Debug_Command"},	// HELP_debug_cmd,
	{0, "_Miscellaneous_Options.html#_Exec_Command"},	// HELP_exec_cmd,
};
#endif
