#ident	"@(#)debugger:libutil/common/set_sys.C	1.6"

#include "Machine.h"
#ifdef HAS_SYSCALL_TRACING

#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Event.h"
#include "TSClist.h"
#include "Vector.h"


int
set_syscall( Proclist * procl, IntList *slist, Systype systype, 
	Node *cmd, int count, char *cnt_var, int quiet )
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;
	Sys_e	*eptr;
	int	eid;
	int	success = 0;
	Vector	*v;
	int	ret = 1;

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
	v = vec_pool.get();
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
		if (!parse_list(pobj, pobj->curframe(), slist,
			v, il_syscalls))
		{
			proc->restart_all();
			ret = 0;
			continue;
		}

		if (cnt_var)
		{
			if (!parse_num_var(pobj, pobj->curframe(),
				cnt_var, count))
			{
				ret = 0;
				continue;
			}
		}
		eptr = new Sys_e((int *)v->ptr(), systype, eid, 
			list->p_type, quiet, count, cmd, pobj);
		if (eptr->get_state() == E_ENABLED)
		{
			m_event.add((Event *)eptr);
			pobj->add_event((Event *)eptr);
			success++;
		}
		else
		{
			ret = 0;
			delete(eptr);
		}
		if (!proc->restart_all())
			ret = 0;
	}
	vec_pool.put(v);
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
#endif
