#ident	"@(#)debugger:libutil/common/set_eh.C	1.12"

#if EXCEPTION_HANDLING
#include "Event.h"
#include "Expr.h"
#include "Interface.h"
#include "Parser.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "global.h"
#include "str.h"
#include "utility.h"

// create an exception handling event

int
set_eh_event(Proclist * procl, int flags, char *type_name, Node *cmd,
	int ignore, int quiet)
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;
	EH_e	*eptr;
	int	eid;
	int	success = 0;
	int	ret = 1;

	if (procl)
	{
		list = proglist.proc_list(procl, 0);
	}
	else
	{
		list = proglist.proc_list(proglist.current_program(),0);
	}
	list_head = list;
	pobj = list->p_pobj;
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		proglist.free_plist(list_head);
		return 0;
	}

	eid = m_event.new_id();
	if (get_ui_type() == ui_gui)
		quiet = 0;

	type_name = str(type_name);
	for (; pobj; list++, pobj = list->p_pobj)
	{	
		Process	*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}

		TYPE	*eh_type = 0;
		if (strcmp(type_name, "...") != 0)
		{
			Expr	*expr = new_expr(type_name, proc, 0, 1, CPLUS);
			eh_type = expr->create_user_type(pobj->curframe());
			delete expr;
			if (!eh_type)
			{
				ret = 0;
				goto restart;
			}
		}

		eptr = new EH_e(flags, eid, list->p_type, quiet,
				type_name, eh_type, cmd, pobj);
		if (eptr->get_state() != E_DELETED)
		{
			m_event.add((Event *)eptr);
			pobj->add_event((Event *)eptr);
			success++;
		}
		else
		{
			ret = 0;
			delete eptr;
		}
restart:
		if (!proc->restart_all())
			ret = 0;
	}
	proglist.free_plist(list_head);

	if (success)
	{
		printm(MSG_event_assigned, eid);
		return ret;
	}
	else
	{
		m_event.dec_id();
		return 0;
	}
}

// set default exception handling state for entire debugger
int
set_default_eh_setting(int flags, int ignore)
{
	if (ignore)
		default_eh_setting &= ~flags;
	else
		default_eh_setting |= flags;
	return 1;
}

// set default exception handling state for a list of processes
int
set_default_eh_setting(Proclist *procl, int flags, int ignore)
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;
	int	ret = 1;

	if (procl)
	{
		list = proglist.proc_list(procl, 0);
	}
	else
	{
		list = proglist.proc_list(proglist.current_program(),0);
	}
	list_head = list;
	pobj = list->p_pobj;
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		proglist.free_plist(list_head);
		return 0;
	}

	for (; pobj; list++, pobj = list->p_pobj)
	{	
		Process		*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}

		if (!pobj->set_eh_defaults(ignore, flags))
			ret = 0;
		if (!proc->restart_all())
			ret = 0;
	}
	proglist.free_plist(list_head);
	return ret;
}

void
display_default_eh_setting(ProcObj *pobj)
{
	int setting;
	if (!pobj)
	{
		printm(MSG_default_eh_header);
		setting = default_eh_setting;
	}
	else
	{
		printm(MSG_eh_header, pobj->obj_name(), pobj->prog_name());
		setting = pobj->get_eh_defaults();
	}

	if (setting & E_THROW)
		printm(MSG_eh_stopped_default, "throw");
	else
		printm(MSG_eh_ignored_default, "throw");
	if (setting & E_CATCH)
		printm(MSG_eh_stopped_default, "catch");
	else
		printm(MSG_eh_ignored_default, "catch");
}

#endif // EXCEPTION_HANDLING
