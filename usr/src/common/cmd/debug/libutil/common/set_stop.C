#ident	"@(#)debugger:libutil/common/set_stop.C	1.7"

#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Event.h"

int
set_stop( Proclist * procl, StopEvent *expr, Node *cmd, int count,
	char *cnt_var, int quiet )
{
	ProcObj		*pobj;
	plist		*list;
	plist		*list_head;
	Stop_e		*eptr;
	int		eid;
	int		success = 0;
	StopEvent	*se = expr;
	int		ret = 1;

	if (procl)
	{
		list = proglist.proc_list(procl);
	}
	else
	{
		list = proglist.proc_list(proglist.current_program());
	}
	list_head = list;
	pobj = list->p_pobj;
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		proglist.free_plist(list_head);
		return 0;
	}
	if (get_ui_type() == ui_gui)
		quiet = 0;
	eid = m_event.new_id();
	for (; pobj; list++, pobj = list->p_pobj)
	{	
		Process	*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}
		if (cnt_var)
		{
			if (!parse_num_var(pobj, pobj->curframe(),
				cnt_var, count))
			{
				proc->restart_all();
				ret = 0;
				continue;
			}
		}
		// must copy even first time through since
		// this may be part of an associated command
		// that will be executed more than once
		se = copy_tree(expr);
		eptr = new Stop_e(se, eid, list->p_type, quiet, count, cmd, pobj);
		if (eptr->get_state() != E_DELETED)
		{
			pobj->add_event((Event *)eptr);
			m_event.add((Event *)eptr);
			success++;
		}
		else
		{
			ret = 0;
			delete eptr;
		}
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
