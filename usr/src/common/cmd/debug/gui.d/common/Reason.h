#ifndef REASON_H
#define REASON_H
#ident	"@(#)debugger:gui.d/common/Reason.h	1.2"

// The reason code is set by the Window_set or Event_list before
// notifying clients to indicate what changed
enum Reason_code
{
	RC_change_state,	// hit a breakpoint, started running, etc.
	RC_set_current,		// context change within a window set
	RC_set_frame,		// context change within the process
	RC_rename,		// program has been renamed
	RC_delete,		// process died or was released
	RC_animate,		// starting animation
	RC_start_script,
	RC_end_script,
	RC_fcall_in_expr,	// function called while evaluating an expression
};

#endif // REASON_H
