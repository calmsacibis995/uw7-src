#ifndef	global_h
#define	global_h
#ident	"@(#)debugger:inc/common/global.h	1.22"

// Declare global variables which almost every module needs.

#include "List.h"
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>


#if DEBUG
extern int	debugflag;	// global debugging flag
#endif

extern sigset_t	interrupt;	
extern int	pathage;	
extern List	waitlist;

extern long	pagesize;

#if EXCEPTION_HANDLING
extern	int		default_eh_setting;
#endif
extern  sigset_t	default_sigs;	// signals caught by default
extern  sigset_t	debug_sset;
extern  sigset_t	sset_PI;	// poll and interrupt
extern  sigset_t	orig_mask;

extern const char	*follow_path;
extern const char	*thread_library_name;
extern const char	*xui_name;
extern char		*prog_name;
extern FILE		*log_file;
extern char		*original_dir;	// original working directory
extern int		dis_mode;
extern int		check_stack_bounds;

extern int	quitflag;	// leave debugger

extern int	(*read_func)(int, void *, unsigned int);

// variables used internally for support of debugger
// "%" variables
extern char	*global_path;
extern char	*debug_ui_path;	// default dir for UI
extern char	*debug_alias_path;	// default dir for alias file
extern char	*debug_follow_path;	// default path of follower
extern int	cmd_result;	// result status of previous command
extern int	last_error;	// last error message issued
extern int	vmode; 		// verbosity
extern int	wait_4_proc;	// background or foreground exec
extern int	follow_mode;	// follow children, threads, all, none
extern int	redir_io;	// should process I/O be redirected?
extern int	synch_mode;	// global sync/asynch flag
extern int	num_line;	// number of lines for list cmd
extern int	num_bytes;	// number of bytes for dump cmd
extern int	stack_count_down;// top stack frame is highest numbered
#ifdef DEBUG_THREADS
extern int	thr_change;  // thread change mode
#endif

#endif	/* global_h */
