#ident	"@(#)debugger:libutil/common/set_onstop.C	1.4"

#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Process.h"
#include "Interface.h"
#include "Parser.h"
#include "Event.h"


int
set_onstop( Proclist * procl, Node *cmd )
{
	ProcObj		*pobj;
	plist		*list;
	plist		*list_head;
	Onstop_e	*eptr;
	int		eid;
	int		success = 0;
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
		eptr = new Onstop_e(eid, list->p_type, cmd, pobj);
		if (eptr->get_state() == E_ENABLED)
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
