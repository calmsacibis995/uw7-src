#ifndef _SENSE_H_
#define _SENSE_H_
#ident	"@(#)debugger:gui.d/common/Sense.h	1.3"

// Sensitivity management; used by Menus items and Buttons

// Values for sense_select - used when a selection is required
// all require a selection
#define	SEN_event_sel		0x00000001	// 1 or more events selected
#define	SEN_process_sel		0x00000002	// 1 or more processes selected
#define	SEN_symbol_sel		0x00000004	// 1 or more symbols selected
#define	SEN_source_sel		0x00000008	// 1 or more source lines selected
#define	SEN_dis_sel		0x00000010	// 1 or more instructions selected
#define	SEN_frame_sel		0x00000020	//  stack frame selected
#define	SEN_text_sel		0x00000040	// text selected

#define SEN_sel_required 	0x0000ffff	// some selection required

// The following further define the type of selection required
#define	SEN_single_sel		0x00010000	// single selection only

#define SEN_user_symbol		0x00020000	// selection is user symbol
#define SEN_program_symbol	0x00040000	// selection is program symbol
#define SEN_sel_has_pin_sym	0x00080000	// selections contain pin symbol
#define SEN_sel_has_unpin_sym	0x00100000	// selections contain no pin symbols
#define	SEN_event_dis_sel	0x00200000	// selections are disabled events
#define	SEN_event_able_sel	0x00400000	// selections are enabled events

// The following are used for sense_other
// the following apply even if no selection required
// and are pane specific
#define SEN_file_required       0x0001	// The source window must have a current file
#define SEN_paste_available	0x0002 // clipboard has data to paste
#define SEN_undo_pending	0x0004 // text change can be undone
#define	SEN_disp_dis_required	0x0008 	// display of instruction 
#define SEN_breakpt_required    0x0010
#define SEN_source_only      	(SEN_file_required|SEN_breakpt_required|SEN_paste_available|SEN_undo_pending)

#define SEN_dis_only      	(SEN_disp_dis_required||SEN_breakpt_required)

// The following are global states
#define SEN_animated		0x0080	// sensitive during either source or dis animation
#define SEN_all_but_script	0x0100	// always sensitive except while
						// executing a script
#define SEN_src_wins		0x0400
#define SEN_win_sets		0x0800
#define	SEN_always		0xffff	// all bits set == always available

// The following apply to the selected or current process
// and are used for sense_process
#define SEN_process		0x0001	// program, process, or thread
#define SEN_proc_running	0x0002	// process must be running
#define SEN_proc_stopped	0x0004	// process must be stopped
#define SEN_proc_runnable	0x0008	// process must be stopped and runnable
#define	SEN_proc_live		0x0010	// live process (non-core) required
						// live includes both running and
						// stopped
#define SEN_proc_single		0x0020	// single process only
#define SEN_proc_only		0x0040	// must not be threads
#define SEN_proc_stopped_core	0x0080	// process must be stopped or a core file
#define SEN_proc_io_redirected	0x0100	// process IO is through a pseudo-terminal
#if EXCEPTION_HANDLING
#define SEN_proc_uses_eh	0x0200	// process must use exception handling
#endif

struct Sensitivity {
	unsigned long	sense_select;	// when a selection is required
	unsigned short	sense_process;	// when a process is required
	unsigned short	sense_other;	// all other
					// access functions
	int always() { return(sense_other == SEN_always); }
	int all_but_script() { return(sense_other == SEN_all_but_script); }
	int src_wins() { return(sense_other & SEN_src_wins); }
	int win_sets() { return(sense_other & SEN_win_sets); }
	int animated() { return(sense_other & SEN_animated); }
	int dis_pane() { return(sense_other & SEN_dis_only); }
	int disp_dis_req() { return(sense_other & SEN_disp_dis_required); }
	int source_pane() { return(sense_other & SEN_source_only); }
	int breakpt_req() { return(sense_other & SEN_breakpt_required); }
	int file_req() { return(sense_other & SEN_file_required); }
	int paste_available() { return(sense_other & SEN_paste_available); }
	int undo_pending() { return(sense_other & SEN_undo_pending); }
	int process() { return(sense_process & SEN_process); }
	int proc_only() { return(sense_process & SEN_proc_only); }
	int proc_running() { return(sense_process & SEN_proc_running); }
	int proc_stopped() { return(sense_process & SEN_proc_stopped); }
	int proc_runnable() { return(sense_process & SEN_proc_runnable); }
	int proc_live() { return(sense_process & SEN_proc_live); }
	int proc_single() { return(sense_process & SEN_proc_single); }
	int proc_stop_core() { return(sense_process & SEN_proc_stopped_core); }
	int proc_io_redir() { return(sense_process & SEN_proc_io_redirected); }
#if EXCEPTION_HANDLING
	int proc_uses_eh() { return(sense_process & SEN_proc_uses_eh); }
#endif
	int proc_special() { return(sense_process & 
		(SEN_proc_live|SEN_proc_running|SEN_proc_stopped|SEN_proc_runnable|SEN_proc_stopped_core|SEN_proc_io_redirected)); }
	int sel_required() { return(sense_select & SEN_sel_required); }
	int process_sel() { return(sense_select & SEN_process_sel); }
	int event_sel() { return(sense_select & SEN_event_sel); }
	int symbol_sel() { return(sense_select & SEN_symbol_sel); }
	int source_sel() { return(sense_select & SEN_source_sel); }
	int dis_sel() { return(sense_select & SEN_dis_sel); }
	int frame_sel() { return(sense_select & SEN_frame_sel); }
	int text_sel() { return(sense_select & SEN_text_sel); }
	int single_sel() { return(sense_select & SEN_single_sel); }
	int user_sym() { return(sense_select & SEN_user_symbol); }
	int program_sym() { return(sense_select & SEN_program_symbol); }
	int pin_sym() { return(sense_select & SEN_sel_has_pin_sym); }
	int unpin_sym() { return(sense_select & SEN_sel_has_unpin_sym); }
	int event_disable() { return(sense_select & SEN_event_dis_sel); }
	int event_enable() { return(sense_select & SEN_event_able_sel); }
};

#endif
