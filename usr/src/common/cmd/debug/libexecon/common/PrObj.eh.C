#ident	"@(#)debugger:libexecon/common/PrObj.eh.C	1.11"

#if EXCEPTION_HANDLING
#include "Exception.h"
#include "EventTable.h"
#include "Frame.h"
#include "ProcObj.h"
#include "Process.h"
#include "Event.h"
#include "Interface.h"
#include "Symbol.h"
#include "global.h"
#include <debug_eh.h>	// from ../../lib/libC

// determine whether we are using exception handling
// find runtime library debug structure, if present
int
Process::eh_debug_setup(int is_core)
{
	Place		place;

	if (thr_brk_addr != 0)
	{
		// When threads are being used, EH state information is passed
		// as arguments to a function.  This means EH information is
		// not available in a core file from a threaded program.
		if (is_core)
			return 1;

		Symbol symbol = find_global("__eh_debug_thread_notifier");
		if (symbol.isnull())
		{
			return 1;
		}
		else if (!symbol.place(place, this, 0, symbol.base())
			|| place.kind != pAddress)
		{
			// Note symbol.base() needed in call to Symbol::place, above,
			// because Symbol::place does not automatically add in
			// base offsets for function symbols
			return 0;
		}
		eh_data = new Exception_data(0, place.addr);
	}
	else
	{
		eh_debug	eh_info;
		Symbol		symbol = find_global("_eh_debug");
		if (symbol.isnull())
		{
			return 1;
		}
		else if (!symbol.place(place, this, 0) || place.kind != pAddress)
		{
			// Note symbol.base() is not used in this call to Symbol::place
			// because objects are handled differently than functions,
			// and Symbol::locdesc, called by Symbol::place, does
			// automatically add in base offset
			return 0;
		}
		else if (read(place.addr, sizeof(eh_debug), (char *)&eh_info)
					!= sizeof(eh_debug))
		{
			return 0;
		}
		eh_data = new Exception_data(place.addr, (Iaddr)eh_info.eh_brk);
		if (is_core)
		{
			if (eh_info.eh_state != eh_state_none)
			{
				eh_data->eh_obj_valid = 1;
				eh_data->eh_object = (Iaddr)eh_info.eh_object;
				setframe((Iaddr)eh_info.eh_caller_fp);
				eh_data->setup_type(this, (Iaddr)eh_info.eh_type,
						eh_info.eh_type_modifiers);
				eh_data->evaluate_type(this);
			}
		}
		else
		{
			eh_data->eh_bkpt = set_bkpt(eh_data->eh_brk_addr, 0, 0);
		}
	}
	return 1;
}

int
ProcObj::check_eh_type_trigger(int mode, int &show)
{
	NotifyEvent	*ne;
	int		found = 0;
	EventTable	*tab = events();

	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::check_eh_type_trigger",
			__LINE__);
		return 0;
	}

	ne = tab->exceptionlist;
	for (; ne; ne = ne->next())
	{
		int	i;
		EH_e	*event = (EH_e *)ne->thisptr;

		if (ne->object != this || !(event->get_flags()&mode))
			continue;
		i = (*ne->func)(ne->thisptr);
		switch(i)
		{
			case TRIGGER_QUIET:
				show = 0;
				// FALL-THROUGH
			case TRIGGER_VERBOSE:
				found++;
				break;
			default:
				break;
		}
	}
	return found;
}

// reset current frame to function calling _throw to show
// throw point and to find context for type
Frame *
ProcObj::setframe(Iaddr fp)
{
	Itype	val;
	Frame	*f;
	for (f = topframe(); f; f = f->caller())
	{
		// NOTE - this assumes we always have a valid frame pointer
		if (!f->readreg(REG_FP, Saddr, val))
			return top_frame;
		if (val.iaddr == fp)
			break;
	}
	if (!f)
		return top_frame;
	setframe(f);
	return f;
}

static int
read_eh_state(ProcObj *pobj, eh_debug &eh_info)
{
	Frame *frame = pobj->topframe();
	if (!frame || frame->incomplete())
		return 0;

	int i = 0;
	eh_info.eh_state = (EH_STATE)frame->argword(i++);
	eh_info.eh_object = (void *)frame->argword(i++);
	eh_info.eh_type = (const char *)frame->argword(i++);
	eh_info.eh_type_modifiers = (int)frame->argword(i++);
	eh_info.eh_caller_fp = (void *)frame->argword(i++);
	return 1;
}

// monitor exception handling state changes (throws and catches)
int
ProcObj::respond_to_ehpt(follower_mode mode)
{
	eh_debug	eh_info;
	int		show = 1;
	Frame		*frame;

	if (!eh_data)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::respond_to_ehpt", __LINE__);
		return restart(mode);
	}

	// Stopped at C++ runtime library notifier function;
	// In the threaded case, read the state data off the stack,
	// otherwise read the _eh_debug structure for state information
	if (flags & L_THREAD)
	{
		if (!read_eh_state(this, eh_info))
			return 0;
	}
	else if (read(eh_data->eh_addr, sizeof(eh_debug), (char *)&eh_info)
			!= sizeof(eh_debug))
	{
		return 0;
	}

	state = es_breakpoint;
	switch (eh_info.eh_state)
	{
	case eh_state_throw:
		flags |= L_EH_THROWN;
		if (flags & L_IGNORE_EVENTS)
		{
			// exception thrown during expression evaluation
			return 1;
		}

		// thrown object's address and type are evaluated even if
		// throws are being ignored, since other events (say,
		// stop std::terminate) may trigger while the program is
		// handling an exception
		eh_data->eh_obj_valid = 1;
		eh_data->eh_object = (Iaddr)eh_info.eh_object;
		frame = setframe((Iaddr)eh_info.eh_caller_fp);
		if (!eh_data->setup_type(this, (Iaddr)eh_info.eh_type, 
					eh_info.eh_type_modifiers))
			return restart(mode);
		if (!eh_data->evaluate_type(this))
		{
			// put out a diagnostic only if user is interested in throws
			if ((events() && events()->exceptionlist)
				|| (eh_data->eh_default_setting&E_THROW))
			{
				eh_data->issue_warning(this, frame);
			}
			else
				return restart(mode);
		}
		if (!check_eh_type_trigger(E_THROW, show)
			&& !(eh_data->eh_default_setting&E_THROW))
		{
			return restart(mode);
		}
		break;

	case eh_state_catch:
		// libC has found a handler but has not yet done the
		// stack unwinding, etc. to enter the handler.
		// debug lets the user know about the catch upon actually
		// entering the handler (eh_state_handler), but attempts
		// to do the type evaluation here, as the type information
		// may not be available in the handler, if the type caught
		// is a base class or ...
		eh_data->eh_object = (Iaddr)eh_info.eh_object;
		(void) setframe((Iaddr)eh_info.eh_caller_fp);
		eh_data->setup_type(this, (Iaddr)eh_info.eh_type,
					eh_info.eh_type_modifiers);
		eh_data->evaluate_type(this);
		return restart(mode);

	case eh_state_catch_end:
		// handler is done, %eh_object is no longer valid
		eh_data->eh_object = 0;
		eh_data->eh_type = 0;
		eh_data->eh_obj_valid = 0;
		return restart(mode);

	case eh_state_handler:
		// entered handler
		flags |= L_EH_CAUGHT;
		// if type evaluation didn't succeed at throw point,
		// try again in context of catch handler
		frame = setframe((Iaddr)eh_info.eh_caller_fp);
		if (!eh_data->is_type_valid()
			&& !eh_data->evaluate_type(this))
		{
			// put out a diagnostic only if user is interested in catches
			if (events()->exceptionlist
				|| (eh_data->eh_default_setting&E_CATCH))
			{
				eh_data->issue_warning(this, frame);
			}
			else
				return restart(mode);
		}
		if (!check_eh_type_trigger(E_CATCH, show)
				&& !(eh_data->eh_default_setting&E_CATCH))
		{
			return restart(mode);
		}
		break;

	default:
		return restart(mode);
	}

	check_watchpoints();
	find_cur_src(frame->pc_value());
	check_onstop();
	if (show)
		return show_current_location(1);
	return 1;
}

int
ProcObj::set_eh_event(Notifier func, void *thisptr)
{
	EventTable	*tab = process()->etable;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::set_eh_event", __LINE__);
		return 0;
	}

	if (!eh_data || !eh_data->eh_brk_addr)
	{
		printe(ERR_no_exceptions, E_ERROR, prog_name());
		return 0;
	}
	else if (!eh_data->eh_bkpt)
	{
		// exceptions are currently being ignored
		eh_data->eh_bkpt = set_bkpt(eh_data->eh_brk_addr, 0, 0);
	}
	tab->set_eh_event(func, thisptr, this);
	return 1;
}

int
ProcObj::remove_eh_event(Notifier func, void *thisptr)
{
	EventTable	*tab = process()->etable;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, "ProcObj::remove_eh_event",
			__LINE__);
		return 0;
	}
	int ret = tab->remove_eh_event(func, thisptr, this);
	return ret;
}

int
ProcObj::set_eh_defaults(int ignore, int flags)
{
	if (!eh_data || !eh_data->eh_brk_addr)
	{
		printe(ERR_no_exceptions, E_ERROR, prog_name());
		return 0;
	}

	eh_data->set_defaults(ignore, flags);
	if (!ignore && !eh_data->eh_bkpt)
	{
		// exceptions are currently being ignored
		eh_data->eh_bkpt = set_bkpt(eh_data->eh_brk_addr, 0, 0);
	}
	return 1;
}

int
ProcObj::get_eh_defaults()
{
	if (!eh_data)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::get_eh_defaults",
			__LINE__);
		return 0;
	}
	return eh_data->eh_default_setting;
}

#endif // EXCEPTION_HANDLING
