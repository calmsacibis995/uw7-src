#ifndef Event_h
#define Event_h
#ident	"@(#)debugger:inc/common/Event.h	1.19"

// General Event handler mechanism.  All events,
// stop, signals, system calls and onstop
// are referenced through a global event
// table, m_event, of type EventManager.
//
// Event ids are assigned consecutively and never re-used.
// Events that are assigned in several threads each have 
// their own EventManager entries, but with the same event id.
// All events relating to a given thread are contained on a linked list.
//
// The event mechanism has two levels.  Primitive events (signals
// or system calls to trace, locations to watch or to plant
// breakpoints on) are handled at the ProcObj/Process level.
// The higher level events, with event ids, counts, complicated
// expressions, are handled at the Event/EventManager level.
// The lower level communicates with the upper through trigger
// functions.  Low-level events contain pointers to these
// functions (Notifiers).

#include "Frame.h"
#include "Iaddr.h"
#include "List.h"
#include "Link.h"
#include "TSClist.h"
#include <signal.h>
#include <string.h>

class	Returnpt;
class	Event;
class	Expr;
class	ProcObj;
class	Location;
struct	Node;
struct	Proclist;
class	StopEvent;
class	StopExpr;
class	StopLoc;
class	WatchData;
class	Buffer;
class	TYPE;

extern void		dispose_event(StopEvent *);
extern char		*print_stop(StopEvent *);
extern StopEvent	*copy_tree(StopEvent *);
extern List		m_cmdlist;
extern int		do_assoccmds();


// used by event_op functions
enum Event_op {
	M_Nop,
	M_Disable,
	M_Enable,
	M_Delete,
	M_Display
};

// Associated command list
struct A_cmd {
	Node	*cmd;
	Event	*event;
};

class EventManager  {
	Event	*first;
	int	cur_id;
	int	thisevent;	// event that just triggered
	void	display(Event *, int mode, char *proclist = 0 );
public:
		EventManager() { memset((void *)this, 0, sizeof(*this)); }
		~EventManager() {}
	Event	*add(Event *);
	Event	*find_event(int);
	int	copy(Event *, ProcObj *, int fork);
	int	event_op(int, Event_op);
	int	event_op(Proclist *, int, Event_op);
	int	event_op(ProcObj *, int, Event_op);
	void	drop_event(Event *);
	void	set_id(int oldid, int newid);
	// Access functions
	int	new_id()  	{ return ++cur_id; } // never reused
	void	dec_id()  	{ if (cur_id > 0) --cur_id; }
	Event	*events()	{ return first; }
	int	this_event()	{ return thisevent; }
	int	last_event()	{ return cur_id; }
	void	set_this(int t)	{ thisevent = t; }
};

extern EventManager	m_event;

// Codes returned by notifiers
#define NO_TRIGGER	0
#define TRIGGER_QUIET	1
#define TRIGGER_VERBOSE	2
#define TRIGGER_FOREIGN	3

// Event types
#define	E_SIGNAL	1
#define	E_SCALL		2
#define	E_STOP		3
#define	E_ONSTOP	4
#if EXCEPTION_HANDLING
#define E_EH_EVENT	5
#endif // EXCEPTION_HANDLING

// Event state
#define	E_DELETED	0
#define E_DISABLED	1
#define E_ENABLED	2
#define E_INVALID	3
#define E_DISABLED_INV	4

// There are 4 types of events: signal, system call, stop and onstop,
// all derived from the base Event class
//
// An event belongs to 2 different doubly linked lists:
// the global EventManager list, and the event
// list for the process in which the event is set.
class Event : public Link {
protected:
	char		quiet;	// set for quiet, else verbose
	char		level;	// thread, process, program
	short		state;	// enabled, disabled, deleted, invalid
	int		id;	// event number
	Node		*cmd;	// associated commands
	ProcObj		*pobj;
	Event		*_enext; // double linking for ProcObj
	Event		*_eprev;  //	 event lists
	friend class	EventManager;
public:
			Event(int id, int level, int quiet, 
				Node *, ProcObj *);
	virtual		~Event() {}
	virtual int	remove(ProcObj *);
	virtual int	trigger();
	virtual void	cleanup(); // called when ProcObj dies
	virtual int	re_init(ProcObj *); // event applied to new ProcObj
	virtual int	get_type(); 
	virtual int	enable();
	virtual int	disable();
	int		common_trigger();
	
	// Access functions
	int		get_state() { return state; }
	int		get_level() { return level; }
	Node		*get_cmd()  { return cmd; }
	void		set_cmd(Node *c) { cmd = c; }
	void		set_quiet() { quiet = 1; }
	void		set_verbose() { quiet = 0; }
	ProcObj		*get_obj() { return pobj; }
	int		get_id()    { return id; }
	Event		*next() { return (Event *)Link::next(); }
	Event		*prev() { return (Event *)Link::prev(); }
	Event		*enext() { return _enext; }
	Event		*eprev() { return _eprev; }
	void		eunlink();
	void		eprepend(Event *);
	void		eappend(Event *);
};

// Traced signal events
class Sig_e : public Event {
	sigset_t	signals;
public:
			Sig_e(sigset_t, int id, int level, int quiet, 
				Node *, ProcObj *);
			~Sig_e() {}
	void		cleanup(); 
	int		re_init(ProcObj *);
	int		remove(ProcObj *);
	int		get_type(); 
	sigset_t	*get_sigs() { return &this->signals; }
};

#ifdef HAS_SYSCALL_TRACING
// Traced system call events
class Sys_e : public Event {
	int		*syscalls;
	Systype		systype;	// entry, exit, entry+exit
	int		cur_count;	// # of times event has fired
	int		orig_count;	// event actions occur after
					// this many firings
public:
			Sys_e(int *, Systype, int id, int level, 
				int quiet, int count, 
				Node *, ProcObj *);
			~Sys_e();
	int		remove(ProcObj *);
	int		trigger();
	void		cleanup(); 
	int		get_type(); 
	int		re_init(ProcObj *);
	void		set_count(int c) { orig_count = c; }
	void		reset_count() { cur_count = 0; }
	int		get_count()   { return orig_count; }
	Systype		get_stype()   { return systype; }
	int		*get_calls()   { return syscalls; }
};
#endif

// Command lists executed whenever ProcObj stops
class Onstop_e : public Event {
public:
			Onstop_e(int id, int level, 
				Node *, ProcObj *);
			~Onstop_e() {}
	void		cleanup(); 
	int		re_init(ProcObj *);
	int		remove(ProcObj *);
	int		get_type(); 
};


// Stop events contain stop expressions linked by conjunction
// or disjunction operators.  If a part of a stop expression
// relates to a ProcObj that is different from the ProcObj in whose
// context the event is set, that part of the event is considered
// "foreign".

// codes returned by stop_set
#define	SET_FAIL	0
#define SET_INVALID	1
#define SET_VALID	2

class Stop_e : public Event {
	StopEvent	*stop;
	char		*event_expr;
	int		cur_count;
	int		orig_count;
	int		stop_eval(StopEvent *);
public:
			Stop_e(StopEvent *, int id, int level, 
				int quiet, int count, 
				Node *, ProcObj *);
			Stop_e(int id, int level, int quiet, int count, 
				Node *, ProcObj *);
			~Stop_e();
	int		remove(ProcObj *);
	int		trigger();
	int		trigger_foreign();
	int		get_type(); 
	void		copy(Stop_e *, int fork);
	void		cleanup();
	int		re_init(ProcObj *);
	void		invalidate();
	void		validate();
	void		set_count(int c) { orig_count = c; }
	void		reset_count() { cur_count = 0; }
	int		get_count()   { return orig_count; }
	StopEvent	*get_stop()   { return stop; }
	char		*get_expr()   { return event_expr; }
	int		enable();
	int		disable();
	void		reset_expr();
};


// There are 2 types of stop event expressions: location
// and expression.  They are both derived from the base class StopEvent.
// Expression StopEvents are used for both stop (x) and stop *x
// Stop event expressions are created by the parser and linked
// as a list.

// StopEvent flags
#define	E_LEAF		0x1
#define	E_AND		0x2
#define	E_OR		0x4
#define E_TRIG_ON_CHANGE 0x8	// indicates a stop *x type event
				// change alone triggers the event
#define E_SET		0x10
#define E_FOREIGN	0x20
#define E_TRUE		0x40
#define E_VALID		0x80
#define E_HAS_ADDR	0x100	// address has already been evaluated

#define NODE_MASK	(E_LEAF|E_AND|E_OR|E_TRIG_ON_CHANGE)

// Base class for different types of stop events;
class StopEvent {
protected:
	int		eflags;
	ProcObj		*pobj;
	Stop_e		*sevent;
	StopEvent	*_next;
public:
			StopEvent(int flags);
	virtual		~StopEvent() {}
	virtual int	stop_true();
	virtual int	stop_set(ProcObj *, Stop_e *);
	virtual int	stop_copy(ProcObj *, Stop_e *, StopEvent *, int);
	virtual int	remove();
	virtual int	re_init(ProcObj *);
	virtual void 	cleanup();
	virtual void 	disable();
	virtual void 	enable();
	virtual void	print(Buffer *);
	virtual StopEvent *copy();
	//	Access functions
	int		get_flags() 	{ return eflags; }
	void		set_flags(int f){ eflags = f; }
	StopEvent	*next() 	{ return _next; }
	ProcObj		*get_obj() 	{ return pobj; }
	void		append(StopEvent *nxt) { _next = nxt; }
	Stop_e		*event()	{ return sevent; }
};

// Breakpoint type stop events
// For breakpoints on line numbers or addresses, the event
// is true only when the ProcObj is stopped at that address.
// For breakpoints on function names, the event is true as 
// long as the function is active.
// If the StopLoc contains an expression, the expression is evaluated
// after reaching the breakpoint, and the event fires only if the
// expression is true.  This is for object-specific combinations,
// in C++, stop a->f translates into a StopLoc on C::f, with the
// expression this == (value of a)

class StopLoc : public StopEvent {
	int		is_func;
	Location	*loc;
	Iaddr		addr;
	Returnpt	*return_stack;	// list of return addresses
	const char	*expr;
public:
			StopLoc(int flags, Location *, Iaddr = 0, const char * = 0);
			~StopLoc();
	Returnpt	*get_stack() { return return_stack; }
	int		stop_true();
	int		stop_set(ProcObj *, Stop_e *);
	int		stop_copy(ProcObj *, Stop_e *, StopEvent *, int);
	int		re_init(ProcObj *);
	void 		cleanup();
	int		remove();
	int		trigger();
	void	 	print(Buffer *);
	void		remove_returnpt(Returnpt *);
	int		remove_all_returnpt();
	StopEvent	*copy();
};

// There are two flavors of watchpoints.  Expression watchpoints
// are true if the expression evaluates to true (non-zero for C/C++).
// The evaluation is triggered whenever any of the objects making
// up the expression changes value.
// Data watchpoints are true only when the (single) object they
// watch changes value.
//
// The identifiers in a stop expression are watched individually
// by WatchData items.


// An expr watchpoint contains a list of data watchpoints,
// 1 for each lvalue in the expression.
// If E_TRIG_ON_CHANGE is set, the event triggers when any sub data items
// change; otherwise when the expression is true

class StopExpr : public StopEvent {
	char		*exp_str;	// expression string
	Expr		*expr;
	WatchData	*data;
	List		triglist;
public:
			StopExpr(int flags, char *);
			~StopExpr();
	int		stop_true();
	int		stop_set(ProcObj *, Stop_e *);
	int		stop_copy(ProcObj *, Stop_e *, StopEvent *, int);
	int		re_init(ProcObj *);
	int		remove();
	void 		cleanup();
	int 		trigger(int foreign);
	void 		print(Buffer *);
	StopEvent	*copy();
	Expr		*get_expr() { return expr; }
	int		eval_expr(const FrameId &); // evaluate in given context
	int		recalc(WatchData *, FrameId &); // recalculate
					// watchpoints if a change in one
					// part of an expression might
					// cause a change in another;
					// like x->i, if x changes
	void		validate();
	void		invalidate();
	void 		disable();
	void 		enable();
	void		remove_all_watchdata();
};

#if EXCEPTION_HANDLING
// Exception Event flags
#define	E_THROW		0x1
#define E_CATCH		0x2

// Traced Exception events
class EH_e : public Event {
	int		flags;
	const char	*type_name;
	TYPE		*eh_type;
public:
			EH_e(int flags, int id, int level, int quiet, 
				const char *, TYPE *, Node *, ProcObj *);
			~EH_e();

			// functions overriding virtuals in the base class
	void		cleanup(); 
	int		re_init(ProcObj *);
	int		remove(ProcObj *);
	int		get_type();
	int		trigger();

			// access functions
	int		get_flags()	{ return flags; }
	TYPE		*get_eh_type()	{ return eh_type; }
	const char	*get_type_name() { return type_name; }
};

#endif // EXCEPTION_HANDLING

#define EVENT		Event
#define STOPEVENT	StopEvent

#endif
// end of Event.h
