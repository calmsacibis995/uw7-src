#ident	"@(#)debugger:libutil/common/change.C	1.15"

// Change event
// Delete old event and create a new one with the same event id.

#include "Interface.h"
#include "Proglist.h"
#include "Event.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Parser.h"
#include "TSClist.h"
#include "Vector.h"
#include <signal.h>
#if EXCEPTION_HANDLING
#include "Expr.h"
#include "TYPE.h"
#include "str.h"
#endif

class TYPE;

static int create_new(int, plist *, char, Systype, Node *, int, int, int,
	StopEvent *, sigset_t *, int *, int, const char *);

#if EXCEPTION_HANDLING
static void parse_eh_event(const char *expr, int &eh_flags, const char *&eh_type);
#endif

int
change_event( int event, Proclist *procl, int count, Systype systype,
	int set_quiet, void *event_expr, Node *cmd)
{
	Event		*eptr;
	int		found = 0;
	int		first = 1;
	int		ret = 1;

	char		old_op;
	int		old_count;
	Systype		old_stype;
	Node		*old_cmd;
	int		old_quiet = 0;
	StopEvent	*old_stop;
	sigset_t	old_sigs;
	int		*old_sys = 0;
	int		old_state;
	int		old_eh_flags = 0;
	const char	*old_eh_type = 0;


	// do we need to delete old event?
	int delete_old = (procl || (systype != NoSType) || event_expr);

	// Go through all events with this id; if
	// we are deleting,  we must save the old information on
	// the event.  If we are not deleting,
	// change part of event that was specified.

	eptr = m_event.events();
	if (!eptr)
	{
		printe(ERR_no_event_id, E_ERROR, event);
		return 0;
	}

	if (get_ui_type() == ui_gui)
		set_quiet = -1;

	// proglist.make_list and add_list provide an external way to
	// create a list of ProcObj's.  Every make_list should be balanced
	// with a list_done
	if (!procl)
		proglist.make_list();

	for(; eptr; eptr = eptr->next())
	{
		if (eptr->get_id() != event ||
			eptr->get_state() == E_DELETED)
			continue;
		found++;
		old_op = eptr->get_type();
		if (first)
		{
			// check for syntax - must do here
			// since it's first time we
			// know type of event
			if (old_op == E_ONSTOP ||
				old_op == E_SIGNAL)
			{
				if (count >= 0)
				{
					printe(ERR_opt_change,
						E_ERROR, "-c");
					return 0;
				}
				if ((set_quiet >= 0) &&
					(old_op == E_ONSTOP))
				{
					printe(ERR_opt_change,
					E_ERROR,
					(set_quiet ? "-q" : "-v"));
					return 0;
				}
			}
			if ((systype != NoSType) &&
				(old_op != E_SCALL))
			{
				printe(ERR_opt_change, E_ERROR,
					"-ex");
				return 0;
			}
		}
		if (!eptr->get_obj())
		{
			printe(ERR_cannot_change, E_ERROR, event);
			if (!procl)
				proglist.free_plist(0);
			return 0;
		}
		if (!delete_old)
		{
			Process	*proc = eptr->get_obj()->process();
			if (!eptr->get_obj()->state_check(E_RUNNING))
			{
				return 0;
			}
			if (!proc->stop_all())
				return 0;
			switch(old_op)
			{
			case E_ONSTOP:
			case E_SIGNAL:
#if EXCEPTION_HANDLING
			case E_EH_EVENT:
#endif
				break;
#ifdef HAS_SYSCALL_TRACING
			case E_SCALL:
				if (count >= 0)
				{
					((Sys_e *)eptr)->set_count(count);
					((Sys_e *)eptr)->reset_count();
				}
				break;
#endif
			case E_STOP:
				if (count >= 0)
				{
					((Stop_e *)eptr)->set_count(count);
					((Stop_e *)eptr)->reset_count();
				}
				break;
			}
			if (set_quiet == 1)
				eptr->set_quiet();
			else if (set_quiet == 0)
				eptr->set_verbose();
			if (cmd)
				eptr->set_cmd(cmd);
			if (!proc->restart_all())
				return 0;
		}
		else
		{
			if (!procl)
			{
				proglist.add_list(eptr->get_obj(), 
					eptr->get_level());
			}
			if (first)
			{
				first = 0;	
				old_cmd = eptr->get_cmd();
				old_state = eptr->get_state();
				switch(old_op)
				{
				case E_ONSTOP:	
					break;
				case E_STOP:
					old_count = ((Stop_e *)eptr)->get_count();
					old_stop = ((Stop_e *)eptr)->get_stop();
					break;
				case E_SIGNAL:
					old_sigs = *((Sig_e *)eptr)->get_sigs();
					break;
#ifdef HAS_SYSCALL_TRACING
				case E_SCALL:
					old_stype = ((Sys_e *)eptr)->get_stype();
					old_sys = ((Sys_e *)eptr)->get_calls();
					old_count = ((Sys_e *)eptr)->get_count();
					break;
#endif
#if EXCEPTION_HANDLING
				case E_EH_EVENT:
					// TYPE class will be recreated below,
					// not cloned, since it is context sensitive
					old_eh_flags = ((EH_e *)eptr)->get_flags();
					old_eh_type = ((EH_e *)eptr)->get_type_name();
					break;
#endif
				}
			}
		}
	}
	if (!found)
	{
		if (!procl)
			proglist.free_plist(0);
		printe(ERR_no_event_id, E_ERROR, event);
		return 0;
	}
	if (!delete_old)
	{
		if (get_ui_type() == ui_gui)
			printm(MSG_event_changed, event);
		return 1;
	}
	if (count >= 0)
		old_count = count;
	if (set_quiet != -1)
		old_quiet = set_quiet;
	if (cmd)
		old_cmd = cmd;
	if (systype != NoSType)
		old_stype = systype;
	if (event_expr)
	{
		switch(old_op)
		{
		default:
			break;
		case E_STOP:
			old_stop = (StopEvent *)event_expr;
			break;
		case E_SIGNAL:
		{
			Vector		*v = vec_pool.get();
			if (!parse_list(0, 0, (IntList *)event_expr,
				v, il_signals))
			{
				vec_pool.put(v);
				return 0;
			}
			int	cnt = v->size()/sizeof(int);
			int	*signo = (int *)v->ptr();
			premptyset(&old_sigs);
			for(int i = 0; i < cnt; i++, signo++)
				praddset(&old_sigs, *signo);
			vec_pool.put(v);
			break;
		}
#ifdef HAS_SYSCALL_TRACING
		case E_SCALL:
		{
			Vector		*v = vec_pool.get();
			if (!parse_list(0, 0, (IntList *)event_expr,
				v, il_syscalls))
			{
				vec_pool.put(v);
				return 0;
			}
			int	cnt = v->size()/sizeof(int);
			old_sys = new int[cnt];
			memcpy((char *)old_sys, (char *)v->ptr(),
				cnt*sizeof(int));
			vec_pool.put(v);
			break;
		}
#endif
#if EXCEPTION_HANDLING
		case E_EH_EVENT:
			parse_eh_event((const char *)event_expr,
				old_eh_flags, old_eh_type);
			break;
#endif
		}
	}
	else 
	{
#ifdef HAS_SYSCALL_TRACING
		if (old_op == E_SCALL)
		{
			// copy sys list
			int	scount = 0;
			int	*itmp = old_sys;
			while(*itmp++)
				scount++;
			scount++;
			itmp = new int[scount];
			memcpy((void *)itmp, (void *)old_sys,
				sizeof(int) * scount);
		}
#endif
	}

	plist *list = procl ? proglist.proc_list(procl) : proglist.list_done();
	ret = create_new(event, list, old_op, old_stype, old_cmd, old_count, 
		old_quiet, old_state, old_stop, &old_sigs, old_sys,
		old_eh_flags, old_eh_type);
	delete old_sys;
	proglist.free_plist(list);
	return ret;
}

static int 
create_new(int event, plist *list, char old_op, Systype old_stype,
	Node *old_cmd, int old_count, int old_quiet, int old_state,
	StopEvent *old_stop, sigset_t *old_sigs, int *old_sys,
	int old_eh_flags, const char *old_eh_type)
{
	ProcObj	*pobj = list->p_pobj;
	int	ret = 1;
	int	new_event = 0;
	int	old_level;
	Event	*eptr = 0;

	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}

	// create new event, with bogus number for now
	for (; pobj; list++, pobj = list->p_pobj)
	{	
		Process	*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}

		old_level = list->p_type;
		switch(old_op)
		{
		default:	break;
		case E_ONSTOP:
			eptr = new Onstop_e(-1, old_level, old_cmd, 
				pobj);
			break;
		case E_STOP:
			eptr = (Event *)new Stop_e(copy_tree(old_stop), 
				-1, old_level, old_quiet, old_count, 
				old_cmd, pobj);
			break;
		case E_SIGNAL:
			eptr = (Event *)new Sig_e(*old_sigs, -1, 
				old_level, old_quiet, old_cmd, pobj);
			break;
#ifdef HAS_SYSCALL_TRACING
		case E_SCALL:
			eptr = (Event *)new Sys_e(old_sys, old_stype,
				-1, old_level, old_quiet, old_count,
				old_cmd, pobj);
			break;
#endif
#if EXCEPTION_HANDLING
		case E_EH_EVENT:
		{
			TYPE	*type_ptr = 0;
			if (strcmp(old_eh_type, "...") != 0)
			{
				Expr	*expr = new_expr(old_eh_type, pobj, 0, 1,
							CPLUS);
				type_ptr = expr->create_user_type(pobj->curframe());
				delete expr;
				if (!type_ptr)
				{
					ret = 0;
				}
			}
			if (ret)
			{
				eptr = (Event *)new EH_e(old_eh_flags, -1,
					old_level, old_quiet,
					old_eh_type, type_ptr,
					old_cmd, pobj);
			}
			else
				eptr = 0;
			break;
		}
#endif
		}
		if (eptr && eptr->get_state() != E_DELETED)
		{
			pobj->add_event(eptr);
			m_event.add(eptr);
			new_event++;
		}
		else
		{
			delete eptr;
			ret = 0;
		}
		if (!proc->restart_all())
			ret = 0;
	}

	if (ret)
	{
		// everything's ok, so now delete old event
		// and reset new event's number
		if (!m_event.event_op(event, M_Delete))
		{
			m_event.event_op(-1, M_Delete);
			return 0;
		}

		m_event.set_id(-1, event);
		if (old_state == E_DISABLED ||
			old_state == E_DISABLED_INV)
			m_event.event_op(event, M_Disable);
		if (get_ui_type() == ui_gui)
			printm(MSG_event_changed, event);
	}
	else if (new_event)
		m_event.event_op(-1, M_Delete);
	return ret;
}

#if EXCEPTION_HANDLING
static void
parse_eh_event(const char *expr, int &eh_flags, const char *&eh_type)
{
	int new_eh_flags = 0;
	while (*expr)
	{
		if (strncmp(expr, "throw", 5) == 0)
		{
			new_eh_flags |= E_THROW;
			expr += 5;
		}
		else if (strncmp(expr, "catch", 5) == 0)
		{
			new_eh_flags |= E_CATCH;
			expr += 5;
		}
		else
		{
			// everything after initial "throw" and/or "catch"
			// is assumed to be a type name
			eh_type = str(expr);
			break;
		}
		while (*expr && *expr == ' ')
			++expr;
	}
	// if the user has not specified either "throw" or "catch",
	// the existing flags are left unchanged
	if (new_eh_flags)
		eh_flags = new_eh_flags;
}
#endif
