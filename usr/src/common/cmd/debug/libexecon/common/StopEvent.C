#ident	"@(#)debugger:libexecon/common/StopEvent.C	1.33"

#include "Event.h"
#include "Link.h"
#include "ProcObj.h"
#include "Process.h"
#include "Frame.h"
#include "Expr.h"
#include "Rvalue.h"
#include "Symbol.h"
#include "Symtab.h"
#include "Source.h"
#include "Itype.h"
#include "Location.h"
#include "Interface.h"
#include "TriggItem.h"
#include "SrcFile.h"
#include "StopEvent.h"
#include "Ev_Notify.h"
#include "Buffer.h"
#include "Proglist.h"
#include "utility.h"
#include "str.h"
#include <string.h>

static void
do_print(StopEvent *node, Buffer *buf)
{
	if (!node)
		return;
	do_print(node->next(), buf);
	if (node->get_flags() & E_AND)
		buf->add(" && ");
	else if (node->get_flags() & E_OR)
		buf->add(" || ");
	if (node->get_flags() & E_TRIG_ON_CHANGE)
		buf->add('*');
	node->print(buf);
}

char *
print_stop(StopEvent *node)
{
	char	*p;
	Buffer	*buf;

	if (!node)
		return 0;

	buf = buf_pool.get();
	buf->clear();
	do_print(node, buf);
	buf->add(' ');
	p = new(char[buf->size()]);
	strcpy(p, (char *)*buf);
	buf_pool.put(buf);
	return p;
}

void
dispose_event(StopEvent *node)
{
	while(node)
	{
		StopEvent	*tmp;
		tmp = node;
		node = node->next();
		delete(tmp);
	}
}

StopEvent *
copy_tree(StopEvent *otree)
{
	StopEvent	*prev = 0;
	StopEvent	*first = 0;
	StopEvent	*ntree;

	while(otree)
	{
		ntree = otree->copy();
		if (!first)
			first = ntree;
		if (prev)
			prev->append(ntree);
		prev = ntree;
		otree = otree->next();
	}
	return first;
}

StopEvent::StopEvent(int f)
{
	eflags = f;
	pobj = 0;
	sevent = 0;
	_next = 0;
}

// virtual base clase null version
void
StopEvent::print(Buffer *)
{
	return;
}

// virtual base class null version
int
StopEvent::remove()
{
	return 0;
}

// virtual base class null version
int
StopEvent::stop_true()
{
	return 0;
}

// virtual base class null version
int
StopEvent::stop_set(ProcObj *, Stop_e *)
{
	return 0;
}

// virtual base class null version
int
StopEvent::stop_copy(ProcObj *, Stop_e *, StopEvent *, int)
{
	return(0);
}

// virtual base class null version
void
StopEvent::cleanup()
{
	return;
}

// virtual base class null version
int
StopEvent::re_init(ProcObj *)
{
	return 0;
}

// virtual base class null version
StopEvent *
StopEvent::copy()
{
	return 0;
}

// virtual base class null version
void
StopEvent::disable()
{
	return;
}

// virtual base class null version
void
StopEvent::enable()
{
	return;
}

StopLoc::StopLoc(int f, Location *l, Iaddr a, const char *s) : STOPEVENT(f)
{
	loc = l;
	return_stack = 0;
	addr = a;
	is_func = 0;
	expr = s;
}

StopLoc::~StopLoc()
{
	delete loc;
	delete (char *)expr;
}

StopEvent *
StopLoc::copy()
{
	Location	*l = new Location(*loc);
	StopLoc *newloc = new StopLoc((eflags & NODE_MASK), l, 0,
		expr ? makestr(expr) : 0);
	return (StopEvent *)newloc;
}

void
StopLoc::print(Buffer *buf)
{
	if (!pobj)
		loc->print(buf);
	else
		loc->print(pobj, pobj->topframe(), buf);
}

static char *
make_func_name(const char *obj_name, int obj_size, const char *fname)
{
	char	*func = new char[strlen(fname) + obj_size + 1];
	strcpy(func, obj_name);
	strcpy(func+obj_size, fname);
	return func;
}

int
StopLoc::stop_set(ProcObj *p, Stop_e *eptr)
{
	long		l;
	Symbol		func;
	ProcObj		*tpobj = p;

	if (eflags&E_HAS_ADDR)
	{
		func = p->find_symbol(addr);
	}
	else
	{
		Vector	*vector = 0;
		int 	ret;

		if (loc->get_flags() & L_HAS_OBJECT)
			ret = get_addr(tpobj, loc, addr, func, expr, E_ERROR, &vector);
		else
			ret = get_addr(tpobj, loc, addr, st_func, func, E_ERROR, &vector);

		if (!ret)
		{
			if (vector)
				vec_pool.put(vector);
			return SET_FAIL;
		}

		if (vector)
		{
			// If a non-zero vector was returned, the function was
			// overloaded.  If there is more than one symbol in the vector,
			// the user selected "All of the above", so set a breakpoint
			// on each instance.  Do that by linking a new StopLoc
			// into the list for each additional symbol

			Overload_data	*data = (Overload_data *)vector->ptr();
			StopLoc		*first = 0;
			StopLoc		*last = 0;
			char		*obj_name = 0;
			int		obj_size;
			int 		count = vector->size()/sizeof(Overload_data);

			// name may have been changed by overload or
			// virtual function resolution
			if (loc->get_flags() & L_HAS_OBJECT)
			{
				// save object name and prepend on
				// each resolved overloaded name
				char	*fname;
				char	*ptr;
				if ((!loc->get_func(tpobj, 0, fname))||
					(((ptr = strchr(fname, '.')) ==
						0) &&
					((ptr = strstr(fname, "->"))
						== 0)))
				{
					printe(ERR_internal, E_ERROR,
					"Stop_Loc::stop_set", __LINE__);
					vec_pool.put(vector);
					return SET_FAIL;
				}
				if (*ptr == '.')
					ptr++;
				else
					ptr += 2;
				*ptr = 0;
				obj_name = makestr(fname);
				obj_size = ptr - fname;
			}

			for (int i = 1; i <= count; ++i, ++data)
			{
				StopLoc		*newptr;
				Location	*newloc;
				int		flags = (i < count) ? E_OR : eflags;

				if (data->location)
				{
					newloc = data->location;
				}
				else
				{
					if (i > 1)
					{
						newloc = new Location();
						*newloc = *loc;
					}
					else
						newloc = loc;
				}
				if (obj_name)
				{
					newloc->set_func(
						make_func_name(obj_name, 
						obj_size, 
						data->function.name()));
					newloc->set_delete_name();
				}
				else if (!data->function.isnull())
					newloc->set_func(str(data->function.name()));
				if (i > 1)
				{
					newptr = new StopLoc(flags|E_HAS_ADDR, newloc,
						data->address, data->expression);
					if (!first)
						first = last = newptr;
					else
					{
						last->_next = newptr;
						last = newptr;
					}
				}
				else
				{
					newptr = this;
					addr = data->address;
					if (loc != newloc)
					{
						delete loc;
						loc = newloc;
					}
				}
			}
			if (first)
			{
				last->_next = _next;
				_next = first;
				eflags &= ~E_LEAF;
				eflags |= E_OR;
			}
			vec_pool.put(vector);
			delete obj_name;
			eptr->reset_expr();
		}
	}

	
	// get_addr can reset tpobj if loc points to foreign ProcObj
	pobj = tpobj;
	sevent = eptr;

	if (!pobj->in_text(addr))
	{
		printe(ERR_bkpt_data, E_WARNING, addr, 
			pobj->obj_name());
	}

	// if stop requested on function name, go past prolog
	if (loc->get_type() == lk_fcn && (func.tag() != t_label))
	{
		loc->get_offset(l);
		if (l == 0)
		{
			is_func = 1;
			addr = pobj->first_stmt(addr);
		}
	}
	if (pobj != p)
	{
		// foreign
#ifdef DEBUG_THREADS
		if ((pobj->obj_type() != pobj_thread) &&
			(((Process *)pobj)->first_thread(0) != 0))
		{
			// for a multithreaded process, foreign
			// events must apply to a thread
			printe(ERR_expr_thread_id, E_ERROR, 
				pobj->obj_name());
			return SET_FAIL;
		}
#endif
		if (!pobj->process()->stop_all())
			return SET_FAIL;
		eflags |= E_FOREIGN;
	}
	if ((pobj->set_bkpt(addr, (Notifier)notify_stoploc_trigger,
		this, ev_low)) == 0)
	{
		if (eflags & E_FOREIGN)
			pobj->process()->restart_all();
		return SET_FAIL; 
	}
	if (get_ui_type() == ui_gui)
	{
		// gui wants to know file and line of bkpt
		Symtab	*symtab;
		Symbol	symbol;
		Source	source;
		long	line;

		if (((symtab = pobj->find_symtab(addr)) != 0) &&
			(symtab->find_source(addr, symbol) != 0)&&
			(symbol.source( source ) != 0)) 
		{
			SrcFile		*srcfile;
			const FileEntry	*file_entry;
			file_entry = source.pc_to_stmt(addr, line);
			if (((srcfile = find_srcfile(pobj, 
				file_entry)) != 0) && line)
				printm(MSG_bkpt_set, addr, line,
					srcfile->filename());
			else
				printm(MSG_bkpt_set_addr, addr);
		}
		else
			printm(MSG_bkpt_set_addr, addr);
	}
	eflags |= E_SET;
	if (eflags & E_FOREIGN)
	{
		pobj->add_foreign((Notifier)notify_stop_e_clean_foreign, 
			sevent);
		pobj->process()->restart_all();
	}
	eflags |= E_VALID;
	return SET_VALID;
}

int
StopLoc::remove_all_returnpt()
{
	int	ret = 1;
	while(return_stack)
	{
		Returnpt	*tmp = return_stack;
		if (!pobj->remove_bkpt(return_stack->return_addr, 
			(Notifier)notify_returnpt_trigger, 
				return_stack, E_DELETE_YES))
			ret = 0;
		return_stack = return_stack->next();
		delete(tmp);
	}
	return_stack = 0;
	return ret;
}

int
StopLoc::stop_copy(ProcObj *p, Stop_e *eptr, StopEvent *oldse, int fork)
{
	StopLoc	*old = (StopLoc *)oldse;
	Returnpt	*orp;

	eflags = old->eflags;
	if (eflags & E_FOREIGN)
		pobj = old->pobj;
	else
		pobj = p;

	sevent = eptr;
	addr = old->addr;
	is_func = old->is_func;
	if (old->expr)
		expr = makestr(old->expr);

	if (!(eflags & E_SET))
		return SET_VALID;
	if (!pobj->process()->stop_all())
		return SET_FAIL;
	if ((pobj->set_bkpt( addr, (Notifier)notify_stoploc_trigger,
		this, ev_low)) == 0)
	{
		pobj->process()->restart_all();
		return SET_FAIL; 
	}
	if (eflags & E_FOREIGN)
	{
		pobj->add_foreign((Notifier)notify_stop_e_clean_foreign, 
			sevent );
	}
	// if this a function-type breakpoint and we are copying
	// for a fork, we copy the return stack; if not for a fork
	// we cannot assume the same return stack as the old event
	if (is_func && !fork)
	{
		for(orp = old->return_stack; orp; orp = orp->next())
		{
			// keep list in same order
			Returnpt	*rpt, *rend;
			rpt = new Returnpt(*orp);
			if (!rpt->set(pobj))
			{
				// delete those already set
				delete rpt;
				remove_all_returnpt();
				pobj->process()->restart_all();
				return SET_FAIL;
			}
			if (!return_stack)
				return_stack = rpt;
			else
				rpt->append(rend);
			rend = rpt;
		}
	}
	pobj->process()->restart_all();
	return SET_VALID;
}

int
StopLoc::remove()
{
	int	ret = 1;

	if (!pobj->process()->stop_all())
		return 0;
	if (eflags & E_SET)
	{
		if (!pobj->remove_bkpt(addr, 
			(Notifier)notify_stoploc_trigger, this, E_DELETE_YES))
		{
			pobj->process()->restart_all();
			return 0;
		}
		if (eflags & E_FOREIGN)
		{
			pobj->remove_foreign((Notifier)notify_stop_e_clean_foreign, 
				sevent);
		}
	}
	if (!remove_all_returnpt())
		ret = 0;
	pobj->process()->restart_all();
	return ret;
}

int
StopLoc::stop_true()
{
	// If breakpoint was on a function, and the function
	// is active, return true;
	// Otherwise true only if we are at exact breakpoint address

	if (is_func)
	{
		return (return_stack != 0);
	}
	else
	{
		if (pobj->is_running() || pobj->is_dead())
		{
			return 0;
		}
		return (addr == pobj->pc_value());
	}
}

int
StopLoc::trigger()
{
	if (is_func)
	{
		Frame	*f = pobj->topframe();
		int	do_trigger = 1;

		// expr will be set for an object-specific breakpoint
		// (i.e. in C++ the breakpoint will look like stop a->f
		// and expr like "this == hexnum").  Check that the
		// process stopped in the specified instance of the function
		if (expr)
		{
			Expr	*exp = new_expr(expr, pobj);

			if (!exp->eval(pobj))
			{
				printe(ERR_internal, E_ERROR,
					"StopLoc::trigger", __LINE__);
				delete exp;
				return NO_TRIGGER;
			}
			do_trigger = exp->exprIsTrue(pobj, f);
			delete exp;
			if (!do_trigger && !return_stack)
				return NO_TRIGGER;
		}

		// new instance of function - add a breakpoint for its
		// return address
		Iaddr	raddr, tmp1, tmp2;
		Returnpt	*rpt;

		if (f->retaddr(raddr, tmp1, tmp2))
		{
			rpt = new Returnpt(raddr, this);
			if (!rpt->set(pobj))
			{
				sevent->invalidate(); 
				delete(rpt);
				return NO_TRIGGER;
			}
			// always add to beginning
			// we are keeping a stack
			if (return_stack)
				rpt->prepend(return_stack);
			return_stack = rpt;
		}
		else
		{
			// can't find return addr - treat like
			// on function address breakpoint
			printe(ERR_return_addr, E_WARNING, 
				pobj->obj_name());
			is_func = 0;
		}
		if (!do_trigger)
			return NO_TRIGGER;
	}
	if (eflags & E_FOREIGN)
		return(sevent->trigger_foreign());
	else
		return(sevent->trigger());
}

void
StopLoc::cleanup()
{
	// If a foreign event, remove entirely,
	// else remove breakpoints for return_stack and 
	// mark main breakpoint as lifted
	int	bdelete = E_DELETE_NO;

	if (!(eflags & E_SET))
		return;
	if (!pobj->process()->stop_all())
		return;
	if (eflags & E_FOREIGN)
	{
		bdelete = E_DELETE_YES;
		pobj->remove_foreign((Notifier)notify_stop_e_clean_foreign,
			sevent);
	}
	pobj->remove_bkpt(addr, 
		(Notifier)notify_stoploc_trigger, this, bdelete);
	eflags &= ~E_SET;
	remove_all_returnpt();
	pobj->process()->restart_all();
	eflags &= ~E_VALID;
	pobj = 0;
}


// re-initialize breakpoint
int
StopLoc::re_init(ProcObj *p)
{
	if (eflags & E_FOREIGN)
	{
		if ((loc->get_pobj(pobj)) == 0)
			return SET_FAIL;
	}
	else
		pobj = p;
	if (!pobj->process()->stop_all())
		return SET_FAIL;

	if ((pobj->set_bkpt( addr, (Notifier)notify_stoploc_trigger, 
		this, ev_low)) == 0)
	{
		pobj->process()->restart_all();
		return SET_FAIL; 
	}
	if (eflags & E_FOREIGN)
	{
		pobj->add_foreign((Notifier)notify_stop_e_clean_foreign,
			sevent);
	}
	eflags |= (E_SET|E_VALID);
	pobj->process()->restart_all();
	return SET_VALID;
}

void
StopLoc::remove_returnpt(Returnpt *rpt)
{
	if (return_stack == rpt)
		return_stack = return_stack->next();
	rpt->unlink();
	delete(rpt);
}

int
Returnpt::set(ProcObj *pobj )
{
	int	ret = 1;
	if (!pobj->process()->stop_all())
		return 0;
	if (pobj->set_bkpt( return_addr, 
		(Notifier)notify_returnpt_trigger, this, ev_low) == 0)
		ret = 0;
	pobj->process()->restart_all();
	return ret;
}

// Hit breakpoint marking return address of function;
// If this is the top of the return stack,
// remove the breakpoint and delete this entry from
// the StopLoc's return_stack.  If this is not the top,
// it means we have multiple entries at the same breakpoint
// address for a recursive function.  Ignore all but the
// top.
int
Returnpt::trigger()
{

	if (this != sloc->get_stack())
		return NO_TRIGGER;
	sloc->get_obj()->remove_bkpt(return_addr, 
		(Notifier)notify_returnpt_trigger, this, E_DELETE_YES);
	sloc->remove_returnpt(this);
	return NO_TRIGGER;
}

StopExpr::StopExpr(int f, char *e) : STOPEVENT(f)
{
	exp_str = e;
	expr = 0;
	data = 0;
}

StopExpr::~StopExpr()
{
	TriggerItem	*item;
	for(item = (TriggerItem *)triglist.first(); item;
		item = (TriggerItem *)triglist.next())
	{
		delete item;
	}
}

StopEvent *
StopExpr::copy()
{
	StopExpr *newexpr = new StopExpr((eflags & NODE_MASK), exp_str);
	return (StopEvent *)newexpr;
}

int
StopExpr::eval_expr(const FrameId &id)
{
	Frame	*f;
	Frame	*save;


	if (!expr)
		return 0;

	if (pobj->is_running())
		return expr->eval(pobj);

	f = pobj->topframe();

	if (!id.isnull())
	{
		save = f;
		for(; f; f = f->caller())
		{
			FrameId	*fid;
			fid = f->id();
			if (id == *fid)
			{
				delete fid;
				break;
			}
			delete fid;
		}
	}
	if (!f)
		f = save;
	return expr->eval(pobj, pobj->pc_value(), f);
}

void
StopExpr::remove_all_watchdata()
{
	while(data)
	{
		WatchData	*wd;
		wd = data;
		data = data->next();
		wd->remove();
		delete(wd);
	}
	data = 0;
}

int
StopExpr::stop_set(ProcObj *p, Stop_e *e)
{
	TriggerItem	*item;
	WatchData	*wd;
	int		invalid = 0;
	Iaddr		pc = p->pc_value();
	Frame		*f = p->topframe();

	sigrelse(SIGINT);
	expr = new_expr(exp_str, p, 1);
	if (!expr->eval(p, pc, f) ||
		!expr->triggerList(pc, triglist))
	{
		delete expr;
		expr = 0;
		sighold(SIGINT);
		printe(ERR_stop_expr, E_ERROR, p->obj_name());
		return SET_FAIL;
	}
	sighold(SIGINT);
	if (eflags & E_TRIG_ON_CHANGE)
	{
		Place	place;
		if (!expr->lvalue(place))
		{
			delete expr;
			expr = 0;
			printe(ERR_stop_lvalue, E_ERROR, exp_str);
			return SET_FAIL;
		}
	}
	sevent = e;
	pobj = p;

	if (triglist.isempty())
	{
		eflags |= E_VALID;
		return SET_VALID;
	}

	for (item = (TriggerItem *)triglist.first(); item;
		item = (TriggerItem *)triglist.next())
	{
		int	i;
		wd = new WatchData(item);
		i = wd->stop_expr_set(p, this);
		if (i == SET_FAIL)
		{
			delete wd;
			remove_all_watchdata();
			return SET_FAIL;
		}
		else if (i == SET_INVALID)
			invalid = 1;
		if (data)
			wd->append(data);
		data = wd;
	}
	if (!invalid)
	{
		eflags |= E_VALID;
		if (!(eflags & E_TRIG_ON_CHANGE) && expr->exprIsTrue(p, f))
			eflags |= E_TRUE;
		return SET_VALID;
	}
	else
		return SET_INVALID;
}

int
StopExpr::stop_copy(ProcObj *p, Stop_e *eptr, StopEvent *oldse, int fork)
{
	Frame		*f = p->topframe();
	StopExpr	*old = (StopExpr *)oldse;
	WatchData	*stop;
	TriggerItem	*item;

	sevent = eptr;
	pobj = p;
	eflags = old->eflags;
	expr = old->expr->copyEventExpr(old->triglist, triglist, p);

	if (triglist.isempty())
	{
		eflags |= E_VALID;
		return SET_VALID;
	}

	for(stop = old->data, item = (TriggerItem *)triglist.first(); 
		stop && item ; stop = stop->next(), 
		item = (TriggerItem *)triglist.next())
	{
		WatchData	*wd = new WatchData(item);
		if (wd->stop_expr_copy(p, this, stop, fork) == SET_FAIL)
		{
			delete wd;
			break;
		}
		if (data)
			wd->append(data);
		data = wd;
	}
	if (stop || item)
	{
		remove_all_watchdata();
		return SET_FAIL;
	}
	return((eflags & E_VALID) ? SET_VALID : SET_INVALID);
}

void
StopExpr::validate()
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		if (!(wd->get_flags() & E_VALID))
		{
			eflags &= ~E_VALID;
			sevent->invalidate();
			return;
		}
	}
	eflags |= E_VALID;
	sevent->validate();
}

void
StopExpr::invalidate()
{
	eflags &= ~E_VALID;
	sevent->invalidate();
}

// recalculate all lvalues and rvalues if part of an expression
// changes that could affect another part;
// for example, in x->i, if the value of x changes, it changes
// the actual address of i
int
StopExpr::recalc(WatchData *orig, FrameId &id)
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		if (wd != orig)
		{
			// don't need to recalc the triggerItem that caused
			// us to recalc in the first place
			if (!wd->recalc(id))
			{
				return 0;
			}
		}
	}
	return 1;
}

void
StopExpr::print(Buffer *buf)
{
	 buf->add(exp_str);
}

void
StopExpr::disable()
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		wd->disable();
	}
}

void
StopExpr::enable()
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		wd->enable();
	}
}
int
StopExpr::remove()
{
	remove_all_watchdata();
	delete expr;
	expr = 0;
	return 1;
}

// Event is true if E_TRIG_ON_CHANGE is set and one of the data
// items has changed, or the expression evaluates to true.
int
StopExpr::stop_true()
{

	if ((eflags & (E_VALID|E_TRUE)) == (E_VALID|E_TRUE))
	{
		eflags &= ~E_TRUE;
		return 1;
	}
	else if (eflags & E_TRIG_ON_CHANGE)
		return 0;
	else
	{
		WatchData	*wd;
		Frame		*f = pobj->topframe();
		FrameId		fid;

		if (!expr)
			return 0;
		if (!eval_expr(fid))
			return 0;
		if (!expr->exprIsTrue(pobj, 0))
			return 0;
		// reset last values of sub_expressions
		for (wd = data; wd; wd = wd->next())
		{
			FrameId	*fid = f->id();
			if (!wd->reset_last(*fid))
			{
				delete fid;
				return 0;
			}
			delete fid;
		}
		return 1;
	}
}

// A data item has changed.  If E_TRIG_ON_CHANGE is
// set we always trigger. Otherwise, we trigger only
// if event evaluates to true.
int
StopExpr::trigger(int foreign)
{
	Frame	*f = pobj->topframe();

	if ((eflags & E_TRIG_ON_CHANGE) ||
		(expr->exprIsTrue(pobj, f)))
	{
		eflags |= E_TRUE;
		if (foreign)
			return(sevent->trigger_foreign());
		else
			return(sevent->trigger());
	}
	else
	{
		eflags &= ~E_TRUE;
		return NO_TRIGGER;
	}
}

int
StopExpr::re_init(ProcObj *p)
{
	WatchData	*d = data;
	Frame		*f = p->topframe();
	int		invalid = 0;

	pobj = p;
	while(d)
	{
		int	i;
		i = d->re_init(p);
		if (i == SET_FAIL)
		{
			WatchData	*tmp;
			for(tmp = data; tmp != d; tmp = tmp->next())
				tmp->cleanup();
			return SET_FAIL;
		}
		else if (i == SET_INVALID)
			invalid = 1;
		d = d->next();
	}
	if (!invalid)
	{
		eflags |= E_VALID;
		if (!(eflags & E_TRIG_ON_CHANGE) && expr->exprIsTrue(p, f))
			eflags |= E_TRUE;
		return SET_VALID;
	}
	else
	{
		return SET_INVALID;
	}
}

void
StopExpr::cleanup()
{
	WatchData	*d = data;

	pobj = 0;
	while(d)
	{
		d->cleanup();
		d = d->next();
	}
	eflags &= ~(E_VALID|E_TRUE);
}


WatchData::WatchData(TriggerItem *i)
{
	item = i;
	frame_stack = 0;
	sexpr = 0;
	pobj = 0;
	flags = 0;
	_nxt = 0;
}


int
WatchData::getTriggerLvalue(Place &lval)
{
	if (!item)
		return 0;
	return item->getTriggerLvalue(lval);
}

int
WatchData::getTriggerRvalue(Rvalue *&rval)
{
	if (!item)
		return 0;
	return item->getTriggerRvalue(rval);
}

void
WatchData::remove_all_watchframe()
{
	while(frame_stack)
	{
		Watchframe	*tmp;
		tmp = frame_stack;
		frame_stack = frame_stack->next();
		tmp->remove();
		delete tmp;
	}
	frame_stack = 0;
}

int
WatchData::stop_expr_set(ProcObj *p, StopExpr *se)
{
	sexpr = se;
	int	i;

	pobj = item->pobj;
	if (pobj != p)
	{
		flags |= E_FOREIGN;
		pobj->add_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	}
	frame_stack = new Watchframe(this, item->scope, item->frame);
	if ((i = frame_stack->init()) == SET_FAIL)
	{
		delete frame_stack;
		frame_stack = 0;
		return SET_FAIL;
	}
	else if (i == SET_VALID)
		flags |= E_VALID;
	return i;
}


int
WatchData::stop_expr_copy(ProcObj *p, StopExpr *oldexpr, 
	WatchData *old, int fork)
{
	Watchframe	*wf, *cur;

	sexpr = oldexpr;

	flags = old->flags;
	if (flags & E_FOREIGN)
	{
		pobj = item->pobj;
		if (!pobj->process()->stop_all())
			return 0;
		pobj->add_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event() );
	}
	else
		pobj = p;
	cur = 0;
	// if copying for a fork, we copy the old watch frames;
	// if not for a fork, we can't assume anything about
	// the context, so we just try to create the initial frame
	if (fork)
	{
		for(wf = old->frame_stack; wf; wf = wf->next())
		{
			// preserve existing order
			Watchframe	*nwf = new Watchframe(this,
				wf->brk_addr, wf->frame);
			if (!nwf->copy(wf))
			{
				delete nwf;
				remove_all_watchframe();
				if (flags & E_FOREIGN)
					pobj->process()->restart_all();
				return SET_FAIL;
			}
			if (cur)	
			{
				nwf->append(cur);
			}
			else
			{
				frame_stack = nwf;
			}
			cur = nwf;
		}
	}
	else 
	{
		// not for fork
		int	i;

		if (!item->frame.isnull())
		{
			printe(ERR_expr_scope, E_ERROR, 
					pobj->obj_name());
			return SET_FAIL;
		}
		frame_stack = new Watchframe(this, item->scope, 
			item->frame);
		if ((i = frame_stack->init()) == SET_FAIL)
		{
			delete frame_stack;
			frame_stack = 0;
			return SET_FAIL;
		}
		else if (i == SET_VALID)
			flags |= E_VALID;
	}
	if (flags & E_FOREIGN)
		pobj->process()->restart_all();
	return((flags & E_VALID) ? SET_VALID : SET_INVALID);
}


int
WatchData::remove()
{
	if (flags & E_FOREIGN)
	{
		if (!pobj->process()->stop_all())
			return 0;
		pobj->remove_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	}
	remove_all_watchframe();
	if (flags & E_FOREIGN)
		pobj->process()->restart_all();
	return 1;
}

// always called after a cleanup
int
WatchData::re_init(ProcObj *p)
{
	int	i;

	if (flags & E_FOREIGN)
	{
		pobj = item->pobj;
		if (proglist.valid(pobj) || 
			!pobj->process()->stop_all())
			return SET_FAIL;
	}
	else
		pobj = p;
	if (!frame_stack || !sexpr ||
		((i = frame_stack->init()) == SET_FAIL))
	{
		delete frame_stack;
		frame_stack = 0;
		if (flags & E_FOREIGN)
			pobj->process()->restart_all();
		return SET_FAIL;
	}
	else if (i == SET_VALID)
		flags |= E_VALID;
	if (flags & E_FOREIGN)
	{
		pobj->add_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event() );
		pobj->process()->restart_all();
	}
	return i;
}

void
WatchData::cleanup()
{
	if (!item->frame.isnull() && (item->scope == NULL_SCOPE))
	{
		// frame qualified id - can never be re-initialized
		// since we will never again have this exact frame
		// remove watchframes and mark event as invalid.
		remove();
		invalidate();
		return;
	}
	if (flags & E_FOREIGN)
	{
		pobj->remove_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	}
	if (!pobj->process()->stop_all())
		return;
	// remove all but initial Watchframe - i.e. last on list
	// this frame has either NULL_SCOPE for its brk_addr or
	// is marked with S_START
	while(frame_stack)
	{
		Watchframe	*wf;

		frame_stack->remove();	
		if ((frame_stack->brk_addr == NULL_SCOPE) ||
			(frame_stack->state == S_START))
			break;
		wf = frame_stack;
		frame_stack = frame_stack->next();
		wf->unlink();
		delete(wf);
	}
	if (frame_stack)
	{
		delete frame_stack->last;
		frame_stack->last = 0;
	}
	pobj->process()->restart_all();
	pobj = 0;
}


void
WatchData::validate()
{
	flags |= E_VALID;
	sexpr->validate();
}

void
WatchData::invalidate()
{
	flags &= ~E_VALID;
	sexpr->invalidate();
}

void
WatchData::enable()
{
	Watchframe	*wf;
	if (!pobj)
		return;
	for(wf = frame_stack; wf; wf = wf->next())
	{
		if (wf->state == S_SOFT)
			pobj->enable_soft();
	}
}

void
WatchData::disable()
{
	Watchframe	*wf;
	if (!pobj)
		return;
	for(wf = frame_stack; wf; wf = wf->next())
	{
		if (wf->state == S_SOFT)
			pobj->disable_soft();
	}
}

// recalculate lvalues and rvalues if a trigger item we depend on
// changes value
int
WatchData::recalc(FrameId &id)
{
	Watchframe	*wf;

	for(wf = frame_stack; wf; wf = wf->next())
	{
		if (wf->frame == id)
		{
			return wf->recalc();
		}
	}
	return 0;
}

// add new watchframe - always add to beginning of list - 
// we are maintaining a stack
void
WatchData::add_frame(Watchframe *wf)
{
	if (frame_stack)
		wf->prepend(frame_stack);
	frame_stack = wf;
}

void
WatchData::remove_frame(Watchframe *wf)
{
	if (frame_stack == wf)
		frame_stack = wf->next();
	wf->unlink();
	if (frame_stack && frame_stack->state == S_START)
	{
		// no more active frames for stack watchpoint
		invalidate();
		// save last value
		delete frame_stack->last;
		frame_stack->last = wf->last->clone();
	}
	delete(wf);
}

Watchframe::Watchframe(WatchData *wd, Iaddr pc, const FrameId &f)
{
	event = wd;
	brk_addr = pc;
	last = 0;
	state = S_NULL;
	place = (Iaddr)-1;
	endscope = (Iaddr)-1;
	frame = f;
}

int
Watchframe::init()
{
	int	i;
	ProcObj	*pobj = event->pobj;
	Place	lval;
	Rvalue	*rval;
	Process	*proc = pobj->process();

	if (!proc->stop_all())
		return SET_FAIL;

	if (brk_addr != NULL_SCOPE)
	{
		// on stack, set up breakpoint for its start addr

		Iaddr	pc = pobj->pc_value();
		int	start;

		if  (state == S_START)
		{
			// re-initialize existing event - don't
			// need to recalculate scope
			frame.null();
		}
		else
		{
			// scope from Triggeritem for inner scope 
			// autos are addr of inner scope;
			// we use scope of function, instead
			Symbol	entry;
			Iaddr	addr;
			entry = pobj->find_entry(brk_addr);
			if (entry.isnull())
			{
				proc->restart_all();
				printe(ERR_expr_scope, E_ERROR, 
					pobj->obj_name());
				return SET_FAIL;
			}
			addr = entry.pc(an_lopc);
			brk_addr = pobj->first_stmt(addr);
			endscope = entry.pc(an_hipc);
			state = S_START;
		}
		if (!pobj->set_bkpt(brk_addr, 
			(Notifier)notify_watchframe_start, this, ev_high))
		{
			state = S_NULL;
			proc->restart_all();
			return SET_FAIL;
		}
		start = ((pc >= brk_addr) && (pc < endscope));
		if (start)
		{
			// in scope
			Frame	*f = pobj->topframe();
			if (frame.isnull())
			{
				FrameId	*fid = f->id();
				frame = *fid;
				delete fid;
			}
			else
			{
				for(; f; f = (Frame *)f->next())
				{
					FrameId	*fid;
					fid = f->id();
					if (*fid == frame)
					{
						delete fid;
						break;
					}
					delete fid;
				}
				if (!f)
				{
					proc->restart_all();
					printe(ERR_expr_scope, E_ERROR, 
						pobj->obj_name());
					event->sexpr->event()->invalidate();
					return SET_FAIL;
				}
			}
			trigger_start(f);
		}
		proc->restart_all();
		return start ? SET_VALID : SET_INVALID;
	}
	// not on stack, start watching
	event->sexpr->eval_expr(frame);
	if ((!event->getTriggerRvalue(rval)) ||
		(!event->getTriggerLvalue(lval)))
	{
		proc->restart_all();
		printe(ERR_stop_expr, E_ERROR, pobj->obj_name());
		return SET_FAIL;
	}
	last = rval->clone();
	if (lval.kind == pAddress)
		place = lval.addr;
	i = pobj->set_watchpoint(place, 
		last, (Notifier)notify_watchframe_watch, this);
	if (i == WATCH_FAIL)
	{
		proc->restart_all();
		return SET_FAIL;
	}
	else if (i == WATCH_HARD)
		state = S_HARD;
	else
		state = S_SOFT;
	proc->restart_all();
	return SET_VALID;
}

// hit breakpoint at start of bracketing automatic watchpoint
int
Watchframe::trigger_start(Frame *context)
{
	Iaddr		raddr, tmp1, tmp2;
	Frame		*f;
	Watchframe	*wf;
	FrameId		*fid;
	ProcObj		*pobj = event->pobj;

	if (context)
	{
		f = context;
	}
	else
	{
		f = pobj->topframe();
		if (frame.isnull())
		{
			fid = f->id();
			frame = *fid;
			delete fid;
		}
	}
	if (!f->retaddr(raddr, tmp1, tmp2))
	{
		printe(ERR_return_addr, E_ERROR, pobj->obj_name());
		event->sexpr->event()->invalidate();
		return NO_TRIGGER;
	}
	fid = f->id();
	wf = new Watchframe(event, raddr, *fid);
	delete fid;

	event->add_frame(wf);
	if (!wf->init_endpoint())
	{
		event->remove_frame(wf);
		return NO_TRIGGER;
	}
	event->validate();
	return NO_TRIGGER;
}

// initialize breakpoint on return addr of function
int
Watchframe::init_endpoint()
{
	ProcObj		*pobj = event->pobj;
	Process		*proc = pobj->process();
	Rvalue		*rval;
	Place		lval;
	int		i;
	Watchframe	*wf;

	if (!proc->stop_all())
		return 0;
	if (!pobj->set_bkpt(brk_addr, 
		(Notifier)notify_watchframe_end, this, ev_low))
	{
		event->sexpr->invalidate();
		proc->restart_all();
		return 0;
	}

	event->sexpr->eval_expr(frame);
	if ((!event->getTriggerRvalue(rval)) ||
		(!event->getTriggerLvalue(lval)))
	{

		pobj->remove_bkpt(brk_addr,
			(Notifier)notify_watchframe_end, this,
			E_DELETE_YES);
		event->sexpr->invalidate();
		proc->restart_all();
		return 0;
	}
	// initialize last with last value from previous frame,
	// if available
	wf = next();
	if (!wf || !wf->last)
		last = rval->clone();
	else
		last = wf->last->clone();
	if (lval.kind == pAddress)
	{
		place = lval.addr;
		if (pobj->in_text(lval.addr))
		{
			
			printe(ERR_watch_text, E_WARNING, lval.addr, 
				pobj->obj_name());
		}
	}
	i = pobj->set_watchpoint(place, 
		last, (Notifier)notify_watchframe_watch, this);
	if (i == WATCH_FAIL)
	{
		pobj->remove_bkpt(brk_addr,
			(Notifier)notify_watchframe_end, this,
			E_DELETE_YES);
		event->sexpr->invalidate();
		proc->restart_all();
		return 0;
	}
	else if (i == WATCH_HARD)
		state = S_HARD;
	else
		state = S_SOFT;
	proc->restart_all();
	return 1;
}

// hit bkpt at return addr
// if this is not the top of the stack, we ignore it - it
// means we have multiple WatchFrames for the same return address
// in a recursively called function.
int
Watchframe::trigger_end()
{
	ProcObj		*pobj;

	if (this != event->frame_stack)
		return NO_TRIGGER;

	pobj = event->pobj;
	pobj->remove_watchpoint((state == S_HARD), place, 
		(Notifier)notify_watchframe_watch, this);
	pobj->remove_bkpt(brk_addr, (Notifier)notify_watchframe_end,
		this, E_DELETE_YES);
	event->remove_frame(this);
	return NO_TRIGGER;
}

int
Watchframe::trigger_watch()
{
	if (brk_addr == event->pobj->pc_value())
	{
		// we are at breakpoint on return address,
		// but breakpoint hasn't yet fired.
		// symbol is out of scope
		return NO_TRIGGER;
	}
	if (changed())
	{
		if (event->item->reinitOnChange())
		{
			if (!event->sexpr->recalc(event, frame))
			{
				event->sexpr->event()->invalidate();
				return NO_TRIGGER;
			}
		}
		return event->sexpr->trigger(event->flags & E_FOREIGN);
	}
	else
	{
		return NO_TRIGGER;
	}
}

// recalculate lvalue and rvalue because of change in some other
// trigger item we depend on
int
Watchframe::recalc()
{
	Rvalue	*rval;
	Place	lval;
	ProcObj	*pobj = event->pobj;
	int	i;

	event->sexpr->eval_expr(frame);
	if (!event->getTriggerLvalue(lval))
		return 0;
	if ((lval.kind == pAddress) && (lval.addr == place))
			return 1; // no change
	if (!event->getTriggerRvalue(rval))
	{
		return 0;
	}
	if (!pobj->remove_watchpoint((state == S_HARD), place,
		(Notifier)notify_watchframe_watch, this))
		return 0;
	place = lval.addr;
	delete(last);
	last = rval->clone();
	i = pobj->set_watchpoint(place, last, 
		(Notifier)notify_watchframe_watch, this);
	if (i == WATCH_FAIL)
		return 0;
	else if (i == WATCH_HARD)
		state = S_HARD;
	else
		state = S_SOFT;
	return 1;
}

int
Watchframe::changed()
{
	Itype		nitype, oitype;
	Stype		nstype, ostype;
	Rvalue		*rval;
	int		diff = 0;
	ProcObj		*pobj = event->pobj;

	if (!event->sexpr->eval_expr(frame))
		return 0;
	if (!event->getTriggerRvalue(rval))
	{
		event->sexpr->event()->invalidate();
		return 0;
	}

	if (((ostype = last->get_Itype(oitype)) == SINVALID)
		|| ((nstype = rval->get_Itype(nitype)) == SINVALID)
		|| (ostype != nstype))
	{
		printe(ERR_stop_expr, E_ERROR, pobj->obj_name());
		return 0;
	}
	// has value changed?
	switch(nstype) 
	{
	case Schar:	
		diff = (nitype.ichar != oitype.ichar); 
		break;
	case Sint1:	
		diff = (nitype.iint1 != oitype.iint1); 
		break;
	case Sint2:	
		diff = (nitype.iint2 != oitype.iint2); 
		break;
	case Sint4:	
		diff = (nitype.iint4 != oitype.iint4); 
		break;
	case Suchar:	
		diff = (nitype.iuchar != oitype.iuchar); 
		break;
	case Suint1:	
		diff = (nitype.iuint1 != oitype.iuint1); 
		break;
	case Suint2:	
		diff = (nitype.iuint2 != oitype.iuint2);
		break;
	case Suint4:	
		diff = (nitype.iuint4 != oitype.iuint4); 
		break;
	case Ssfloat:	
		diff = (nitype.isfloat != oitype.isfloat); 
		break;
	case Sdfloat:	
		diff = (nitype.idfloat != oitype.idfloat); 
		break;
	case Sxfloat:	
		diff = (nitype.ixfloat != oitype.ixfloat); 
		break;
	case Saddr:	
		diff = (nitype.iaddr != oitype.iaddr);
		break;
	case Sdebugaddr:	
		diff = (nitype.idebugaddr != oitype.idebugaddr);
		break;
	case Sbase:	
		diff = (nitype.ibase != oitype.ibase); 
		break;
	case Soffset:	
		diff = (nitype.ioffset != oitype.ioffset); 
		break;
#if LONG_LONG
	case Sint8:
		diff = (nitype.iint8 != oitype.iint8); 
		break;
	case Suint8:
		diff = (nitype.iuint8 != oitype.iuint8); 
		break;
#endif
	case SINVALID:
	default:
		printe(ERR_internal, E_ERROR, "Watchframe::changed", __LINE__);
		return 0;
	}
	if (diff)
	{
		delete last;
		last = rval->clone();
	}
	return diff;
}

// reset last values for stack-based variables if there is 
// a watchframe corresponding to current top frame
#ifdef __cplusplus
int
WatchData::reset_last(const FrameId &id)
#else
int
WatchData::reset_last(FrameId &id)
#endif
{
	if (!frame_stack)
	{
		printe(ERR_internal, E_ERROR, "WatchData::reset_last",
			__LINE__);
		return 0;
	}
	if ((frame_stack->state != S_HARD && 
		frame_stack->state != S_SOFT) || 
		(frame_stack->frame != id))
		return 1;

	Rvalue		*rval;
	if (!getTriggerRvalue(rval))
		return 0;
	delete frame_stack->last;
	frame_stack->last = rval->clone();
	return 1;
}

// process stopped by WatchData::remove
int
Watchframe::remove()
{
	int	fail = 0;
	ProcObj	*pobj = event->pobj;

	if (brk_addr != NULL_SCOPE)
	{
		if (!pobj->remove_bkpt(brk_addr,((state == S_START) ?
			((Notifier)notify_watchframe_start) :
			((Notifier)notify_watchframe_end)) , this,
			 E_DELETE_YES))
			fail = 1;
	}
	if (state != S_START)
	{
		if (!pobj->remove_watchpoint((state == S_HARD), place,
			(Notifier)notify_watchframe_watch, this))
				fail = 1;
		delete last;
		last = 0;
	}
	return(fail == 0);
}

// process stopped by WatchData::stop_expr_copy
int
Watchframe::copy(Watchframe *old)
{
	int	set = 0;
	int	i;
	ProcObj	*pobj = event->pobj;

	state = old->state;
	place = old->place;
	endscope = old->endscope;

	if (brk_addr != NULL_SCOPE)
	{
		// on stack, set up breakpoint
		if (state == S_START)
		{
			if (!pobj->set_bkpt(brk_addr, 
				(Notifier)notify_watchframe_start,
				this, ev_high))
				return 0;
		}
		else
		{
			if (!pobj->set_bkpt(brk_addr, 
				(Notifier)notify_watchframe_end,
				this, ev_low))
				return 0;
		}
		set = 1;
	}
	if (state != S_START)
	{
		last = old->last->clone();
		i = pobj->set_watchpoint(place, 
			last, (Notifier)notify_watchframe_watch, this);
		if (i == WATCH_FAIL)
		{
			if (set)
				pobj->remove_bkpt(brk_addr,
					(Notifier)notify_watchframe_end, this,
					E_DELETE_YES);

			return 0;
		}
		else if (i == WATCH_HARD)
			state = S_HARD;
		else
			state = S_SOFT;
	}
	return 1;
}
