#ifndef StopEvent_h
#define StopEvent_h

#ident	"@(#)debugger:libexecon/common/StopEvent.h	1.6"

#include "Frame.h"
#include "Iaddr.h"
#include "Link.h"
#include "Place.h"
#include "Rvalue.h"

class ProcObj;
class StopLoc;
class StopExpr;
class TriggerItem;
class WatchData;

// Definitions used locally for implementing stop events.

// Location stop events whose location is a function address
// maintain a list of return addresses for that function.
// Each instance of a recursive function will have a Returnpt item.
class	Returnpt : public Link {
	Iaddr		return_addr;
	StopLoc		*sloc;
	friend class	StopLoc;
public:	
			Returnpt(Iaddr i, StopLoc *s)
				{ return_addr = i; sloc = s; }
			Returnpt(Returnpt &old)
				{ return_addr = old.return_addr; 
					sloc = old.sloc; }
			~Returnpt() {}
#ifdef __cplusplus
	inline int	set(ProcObj *);
#else
	int		set(ProcObj *);	// 1.2 has limited inline support
#endif
	int		trigger();
	Returnpt	*next()	{ return (Returnpt *)Link::next(); }
};


// If a data watchpoint refers to an automatic, we bracket
// the start of the enclosing function and its return address 
// with breakpoints.
// Watching stack addresses in recursive functions is done
// by instantiating a new Watchframe each time the function is entered.
//

// used for state field
#define S_NULL	0
#define S_START	1 // marks Watchframe use to monitor starting
		  // address of a scope
#define S_SOFT	2
#define S_HARD	3

class Watchframe : public Link {
	int		state;
	WatchData	*event;
	Iaddr		place;
	Iaddr		brk_addr;
	Iaddr		endscope;
	Rvalue		*last;
	FrameId		frame;
	friend class	WatchData;
public:
			Watchframe(WatchData *, Iaddr, const FrameId &);
			~Watchframe() { delete last; }
	Watchframe	*next()	{ return (Watchframe *)Link::next(); }
	int		copy(Watchframe *);
	int		init();
	int		init_endpoint();
	int		trigger_watch();
	int		trigger_start(Frame *);
	int		trigger_end();
	int		changed();
	int		remove();
	int		recalc(); // recalculate
				// watchpoints if a change in one
				// part of an expression might
				// cause a change in another;
				// like x->i, if x changes
};

// Data watchpoints
// These are sub_items of StopExpr events.
// Each WatchData instance monitors a single lvalue.
//
class WatchData  {
	int		flags;
	TriggerItem	*item;
	Watchframe	*frame_stack;
	StopExpr	*sexpr;
	WatchData	*_nxt;
	ProcObj		*pobj;
	friend class	Watchframe;
public:
			WatchData(TriggerItem *);
			~WatchData() {}
	int		stop_expr_set(ProcObj *,  StopExpr *);
	int		stop_expr_copy(ProcObj *, StopExpr *, 
				WatchData *, int fork);
	int		re_init(ProcObj *);
	int		remove();
	void 		cleanup();
	int		getTriggerRvalue(Rvalue *&);
	int		getTriggerLvalue(Place &);
	void		append(WatchData *nxt) { _nxt = nxt; }
	WatchData	*next() { return _nxt; }
	int		get_flags() { return flags; }
	void		validate();
	void		invalidate();
	void		disable();
	void		enable();
	void		add_frame(Watchframe *);
	void		remove_frame(Watchframe *);
	void		remove_all_watchframe();
#ifdef __cplusplus
	int		reset_last(const FrameId &); // reset last values
#else
	int		reset_last(FrameId &); // reset last values
#endif
	int		recalc(FrameId &); // recalculate watchpoints 
};

#endif
