#ifndef ProcObj_h
#define ProcObj_h
#ident	"@(#)debugger:inc/common/ProcObj.h	1.41"

// Basic process control - provides operations
// common to processes and threads.  The Process and Thread
// classes are derived from ProcObj.

#include "Iaddr.h"
#include "Instr.h"
#include "Link.h"
#include "Stmt.h"
#include "RegAccess.h"
#include "ProcFollow.h"
#include "Symbol.h"
#include "TSClist.h"
#include "Ev_Notify.h"
#include "Machine.h"
#include "Proctypes.h"
#include <sys/types.h>

class	Breakpoint;
struct	EventTable;
class	Event;
class	Frame;
class	HW_Watch;
class	NameEntry;
class	Proccore;
class	Process;
class	Proclive;
class	PtyInfo;
class	Rvalue;
class	Seglist;
class	Symtab;
class	Program;
struct	Dyn_info;
struct  sig_ctl;
struct	sigaction;
struct	FileEntry;
class	TYPE;
#if EXCEPTION_HANDLING
class	Exception_data;
#endif

// Process or thread states
// Threads that are used to follow LWPs that are not
// currently running an application thread have the
// L_VIRTUAL flag set.  Virtual threads whose LWP has
// subsequently been picked up by an application thread
// are marked as es_unused.  Application threads not
// current running on an LWP are marked as es_off_lwp.
enum Execstate {
	es_invalid,
	es_corefile,
	es_dead,
	es_procstop,
	es_stepping,
	es_running,
	es_stepped,
	es_halted,
	es_signalled,
	es_breakpoint,
	es_syscallent,
	es_syscallxit,
	es_watchpoint,
	es_unused,
	es_off_lwp,
	es_core_off_lwp,
	es_halted_thread_start,
};

// Primary and secondary goals.  Primary goals are the
// result of some user request, like stmt step.  Secondary
// goals may be different actions taken to achieve primary
// goals, like stepping over a breakpoint in order to run.

enum Goal1 {
	pg_run,
	pg_stmt_step,
	pg_instr_step,
	pg_stmt_step_over,
	pg_instr_step_over,
};

enum Goal2 {
	sg_run,
	sg_step,
	sg_stepbkpt,
};

// types of special internal breakpoints
enum Bkpt_type {
	bk_hoppt,
	bk_destpt,
	bk_dynpt,
	bk_threadpt,
	bk_startpt,
#if EXCEPTION_HANDLING
	bk_ehpt,
#endif
#ifndef HAS_SYSCALL_TRACING
	bk_execvept,
	bk_forkpt,
	bk_fork1pt,
	bk_forkallpt,
	bk_vforkpt,
#endif
};

// definitions for stepping
#define STEP_INF_ANNOUNCE	-1	// step forever and announce
#define STEP_INF_QUIET		-2	// step forever quietly
#define	STEP_INTO		0
#define	STEP_OVER		1
#define STEP_STMT		0
#define STEP_INSTR		1

// returned by set_watchpoint
#define WATCH_FAIL	0
#define WATCH_SOFT	1
#define WATCH_HARD	2

// mode parameter to state_check()
#define	E_DEAD		1
#define E_CORE		2
#define E_RUNNING	4
#define E_OFF_LWP	8
#define E_SUSPENDED	16

// flags
#define L_THREAD	0x1
#define L_PROCESS	0x2
#define L_VIRTUAL	0x4
#define	L_IS_CHILD	0x8
#define	L_GRABBED	0x10
#define	L_GO_PAST_START	0x20
#define	L_IN_START	0x40
#define	L_CHECK		0x80
#define	L_WAITING	0x100
#define	L_IGNORE_EVENTS	0x200
#define	L_IGNORE_FORK	0x400
#define L_INCONSISTENT	0x800
#define L_RELEASED	0x1000
#define L_CHECK_FOLLOW	0x2000
#define L_SUSP_PENDING	0x4000
#define L_SUSPENDED	0x8000
#define L_NEW_THREAD	0x10000
#define L_SRC_IS_USER_SET	0x20000
#if EXCEPTION_HANDLING
#define L_EH_THROWN		0x40000
#define L_EH_CAUGHT		0x80000
#endif

// flags for removing events
// whether or not the low level event is removed after all
// high level events are gone
#define E_DELETE_NO	0	
#define E_DELETE_YES	1

// flags for setting events
// whether or not the low level event is planted after each
// high level event is added
#define E_SET_NO	0	
#define E_SET_YES	1

enum pobj_type {
	pobj_thread,
	pobj_process,
};

// Some implementations use signals for indicating breakpoints
// and other tracing information. Other implementations use the
// fault tracing facilities of /proc.  The ProcObj class
// attempts to allow either approach through the macros
// STOP_TYPE and LATEST_STOP
//

#if STOP_TYPE == STOP_SIGNALLED
#define	LATEST_STOP	latestsig
#else
#define	LATEST_STOP	latestflt
#endif

class ProcObj : public Link {
protected:
	char		verbosity;
	long		flags;
	short		sw_watch;
	short		ecount;	  // number of single steps requested

	char		*pobj_name;
	char		*progname;	// stored here for efficient access
	char		*ename;		// stored here for efficient access
	Proccore	*core;		// core and pctl are never
	Proclive	*pctl;		// both used by a ProcObj

	Iaddr		pc;	// current location, constant while stopped
	Iaddr		dot;	// used by dis, may change
	unsigned long	epoch;	// keep track of whether state has changed

	Iaddr		lopc;	// keep track of current function
	Iaddr		hipc;
	Symtab		*last_sym;
	Symbol		last_comp_unit;	// context for %file
	Symbol		current_comp_unit;	// context for %list_file

	Goal1		goal;
	Goal2		goal2;
	Execstate	state;
	Execstate	save_state;  // save old state for procstop

	Breakpoint 	*latestbkpt;
	int		latestsig;
	int		latesttsc;
	int		latestflt;
	char		*latestexpr;

	Iaddr		startaddr;	// first location in a stmt
	Iaddr		retaddr;
	Iaddr		start_sp;
	Stmt		startstmt;
	Stmt		currstmt;
	const FileEntry *current_srcfile;
	const FileEntry	*start_srcfile;
	FileEntry	*fake_fentry;

	Breakpoint	*hoppt;		// for step over calls
	Breakpoint	*destpt;	// destination of run
	Breakpoint	*dynpt;		// dynamic linker bkpt
#ifndef HAS_SYSCALL_TRACING
	Breakpoint	*execvept;
	Breakpoint	*forkpt;
	Breakpoint	*fork1pt;
	Breakpoint	*forkallpt;
	Breakpoint	*vforkpt;
#endif
#ifdef DEBUG_THREADS
	Breakpoint	*threadpt;	// thread library bkpt
	Breakpoint	*startpt;	// start addr for thread
#endif
#if EXCEPTION_HANDLING
	Exception_data	*eh_data;
#endif
	sigset_t	sigset;		// signal mask for this object

	Frame 		*cur_frame;
	Frame 		*top_frame;

	HW_Watch	*hw_watch;
	Seglist		*seglist; 

	Instr		instr;
	RegAccess	regaccess;
	gregset_t	*saved_gregs;
	fpregset_t	*saved_fpregs;

	NotifyEvent	*foreignlist;

	int		check_stmt_step();
	int		check_instr_step();
	int		check_watchpoints();
	void		check_onstop();

	int		inform_run();
	int		inform_startup(int what, int why);
	int		inform_step();
	int		inform_stepbkpt();

	int		hop_to(Iaddr);
	int		run_pobj( int clearsig, Iaddr dest, int talk_ctl);
	int		update_state();

	int		respond_to_sig();
	int		respond_to_sus(int show, int showsrc, Execstate);
	int		respond_to_bkpt();
	int		respond_to_tsc();
	int		respond_to_dynpt(follower_mode);
#if EXCEPTION_HANDLING
	int		check_eh_type_trigger(int mode, int &verbose);
	int		respond_to_ehpt(follower_mode);
	Frame		*setframe(Iaddr fp);
#endif
#ifndef HAS_SYSCALL_TRACING 
	int		respond_to_execvept();
	int		respond_to_forkpt(Bkpt_type);
#endif

	int		find_stmt( Stmt &, Iaddr, const FileEntry *& );
	int		find_cur_src(Iaddr addr = (Iaddr)-1, int reset_src = 1);
	void		stateinfo(const char *entryname, 
				const char *filename, int vlevel,
				Execstate);

	int		lift_bkpt( Breakpoint *, int other_thread = 0 );
	int		insert_bkpt( Breakpoint *);
	Breakpoint	*remove(Bkpt_type);
	int		reset_sigs(EventTable *);
	void		cleanup_foreign();


public:
			ProcObj(int);
	virtual		~ProcObj();

	ProcObj 	*next()	{ return (ProcObj*)Link::next(); }
	ProcObj	 	*prev()	{ return (ProcObj*)Link::prev(); }

	//		Access functions
	Iaddr		top_pc()   	{ return pc; }
	unsigned long	p_epoch()	{ return epoch; }
	const char	*obj_name()	{ return pobj_name; }
	Proclive	*proc_ctl()	{ return pctl; }
	Proccore	*core_ctl()	{ return core; }
	char		*prog_name()	{ return progname; }
	char		*exec_name()	{ return ename; }
	Execstate	get_state() 	{ return state; }
	void		set_state(Execstate s) 	{ state = s; }
	Iaddr		dot_value()	{ return dot; }
	const FileEntry	*curr_src(int &is_user_set)
					{ is_user_set = (flags&L_SRC_IS_USER_SET);
						return current_srcfile; }
	Symbol		&curr_comp_unit() { return current_comp_unit; }
	Symbol		&comp_unit()	{ return last_comp_unit; }
	int		check()		{ return (flags & L_CHECK); }
	int		waiting()	{ return (flags & L_WAITING); }
	pobj_type	obj_type()	{ return (flags & L_THREAD) ?
						pobj_thread : pobj_process; }
	int		is_virtual()	{ return (flags & L_VIRTUAL); }
	int		is_released()	{ return (flags & L_RELEASED); }
	int		is_dead()	{ return (state == es_dead); }
	int		is_user()	{ return !(flags & (L_VIRTUAL|L_RELEASED)); }
	int		is_running()	{ return (state == es_running ||
						state == es_stepping); }
	int		is_core()	{ return (state==es_corefile ||
						state==es_core_off_lwp); }
	int		is_ignore_fork() { return (flags & L_IGNORE_FORK); }
	int		is_inconsistent() { return (flags & L_INCONSISTENT); }
	int		is_suspended() { return (flags & L_SUSPENDED); }
	void		rename(char *name) { progname = name; }
	void		set_dot(Iaddr val) { dot = val; }
	void		set_check() { flags |= L_CHECK; }
#if defined(FOLLOWER_PROC) || defined(PTRACE)
	void		clear_check() { flags &= ~L_CHECK; }
#else
	void		clear_check();
#endif
	void		set_wait() { flags |= L_WAITING; }
	void		clear_wait() { flags &= ~L_WAITING; }
	void		set_start() { flags |= L_IN_START; }
	void		clear_start() { flags &= ~L_IN_START; }
	void		set_expr(char *e) { latestexpr = e; }
	void		set_ignore()  { flags |= L_IGNORE_EVENTS; }
	void		clear_ignore()  { flags &= ~L_IGNORE_EVENTS; }
	void		disable_soft() { if (sw_watch > 0) sw_watch--; }
	void		enable_soft() { sw_watch++; }
	Instr		*instruct() { return &instr; }
	sigset_t	*sig_mask() { return &sigset; }

#if EXCEPTION_HANDLING
	Exception_data	*get_eh_info()	{ return eh_data; }
	int		exception_thrown() { return (flags & L_EH_THROWN); }
	int		set_eh_event(Notifier, void *);
	int		remove_eh_event(Notifier, void *);
	int		set_eh_defaults(int ignore, int flags);
	int		get_eh_defaults();
#endif

	virtual Process	*process();
	Iaddr		pc_value();
	pid_t		pid();
	Program		*program();
	EventTable	*events();

	virtual int	release_obj(int run);
	virtual int	destroy(int sendsig);

	int		inform(int what, int why);
	int		state_check(int mode);

	int		stop();
	int		stop_for_event(int);

	int		restart(follower_mode);
	int		start(Goal2, follower_mode);

	int		run( int clearsig, Iaddr dest, int talk_ctl);
	int		instr_step( int where, int clearsig, 
				int cnt, int talk_ctl );
	int		stmt_step( int where, int clearsig, Iaddr dest,
				int cnt, int talk_ctl );

	int		setframe( Frame * );
	Frame 		*curframe();
	Frame 		*topframe();
	Frame 		*topframe(Iaddr pc, Iaddr sp);

	int		read( Iaddr, int len, char *);
	int		write( Iaddr, void *, int len );
	int		read( Iaddr, Stype, Itype & );
	int		write( Iaddr, Stype, const Itype & );

	char		*disassemble( Iaddr addr, int symbolic, const char *name, 
				Iaddr offset, int &inst_size)

				{ return instr.deasm(addr, inst_size,
				symbolic, name, offset); }
	Symtab 		*find_symtab( Iaddr );
	Symtab 		*find_symtab( const char * );
	int		find_source( const char *, Symbol & );
	Symbol		find_entry( Iaddr );
	Symbol		find_symbol( Iaddr );
	Symbol		find_scope( Iaddr );
	Iaddr		first_stmt(Iaddr);
	Symbol		find_global( const char * );
	int		find_next_global(const char *name, Symbol &);
	int		create_fake_symbol(const char *, Symbol &);
	void		global_search_complete();
	Dyn_info	*get_dyn_info(Iaddr);
	Symbol		current_function() 
				{ return find_entry(pc_value()); }
	Symbol		first_file();
	Symbol		next_file();
	const char	*object_name( Iaddr );
	const char 	*symbol_name( Symbol );	// get symbol name

	void		set_current_stmt(const FileEntry *, long line, int is_user_set);
	void		set_current_comp_unit(Symbol &sym)
				{ current_comp_unit = sym; }
	FileEntry	*create_fentry(const char *fname);
	long		current_line() { return(currstmt.line); }
	int		show_current_location( int srclevel, 
				Execstate showstate = es_invalid,
				int talk_ctl = -1 );

	int		set_onstop(Notifier, void *);
	int		remove_onstop(Notifier, void *);

	void		add_foreign(Notifier, void *);
	int		remove_foreign(Notifier, void *);

	virtual void	add_event(Event *);
	virtual void	remove_event(Event *);

	char 		*text_nobkpt( Iaddr );
	Breakpoint	*set_bkpt( Iaddr, Notifier , void *,
				ev_priority ep = ev_none);
	int		set_destination(Iaddr);
	int		remove_bkpt( Iaddr, Notifier, void *, int);

	int		set_watchpoint(Iaddr, Rvalue *, Notifier, void *);
	int		remove_watchpoint(int hw, Iaddr, Notifier, void *);

	int		set_sig_event( sigset_t *, Notifier, void *, int);
	int		remove_sig_event( sigset_t *, Notifier, void *, int );
	int		catch_sigs(sigset_t *, int level);
	int		ignore_sigs(sigset_t *, int level);

	virtual int	cancel_sigs( sig_ctl *);
	virtual int	pending_sigs( sig_ctl *);

	int		get_sig_disp(struct sigaction *&, int &number);

#ifdef HAS_SYSCALL_TRACING
	int		set_sys_trace( int call, Systype, Notifier, void *, int);
	int		remove_sys_trace( int call, Systype, Notifier, void *, int);
#endif

	Iaddr		getreg( RegRef regref) 
				{ return(regaccess.getreg(regref)); }
	int		readreg( RegRef regref, Stype stype, Itype &itype )
				{ return(regaccess.readreg(regref, stype, itype)); }
	int		writereg( RegRef, Stype, Itype & );
	int		readreg( RegRef regref1, RegRef regref2,
				Stype stype, Itype &itype )
				{ return(regaccess.readreg(regref1, 
					regref2, stype, itype)); }
	int		writereg( RegRef r1, RegRef r2, Stype stype, 
				Itype &itype)
			{
				return regaccess.writereg(r1, r2,
					stype, itype);
			}
	int		display_regs(Frame *f)
				{ return(regaccess.display_regs(f)); }
	int		set_pc(Iaddr);

	int		print_map();

	int		save_registers();
	int		restore_registers();

	virtual gregset_t *read_greg();
	virtual fpregset_t *read_fpreg();
	virtual int	write_greg(gregset_t *);
	virtual int	write_fpreg(fpregset_t *);

	virtual int	in_stack( Iaddr );
	int		in_text( Iaddr );
	virtual Iaddr	end_stack();
};

// cfront 2.1 requires base class name in derived class constructor,
// 1.2 forbids it
#ifdef __cplusplus
#define PROCOBJ		ProcObj
#else
#define PROCOBJ
#endif

#endif
// end of ProcObj_h
