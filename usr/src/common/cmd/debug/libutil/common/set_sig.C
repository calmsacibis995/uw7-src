#ident	"@(#)debugger:libutil/common/set_sig.C	1.8"

#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Machine.h"
#include "Parser.h"
#include "global.h"
#include "Event.h"
#include "Vector.h"
#include <signal.h>


int
set_signal( Proclist * procl, IntList *slist, Node *cmd, int ignore, int quiet )
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;
	Sig_e	*eptr;
	int	eid;
	int	success = 0;
	int	ret = 1;
	Vector	*v;
	sigset_t	sigs;

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

	v = vec_pool.get();

	if (cmd)
	{
		eid = m_event.new_id();
	}

	if (get_ui_type() == ui_gui)
		quiet = 0;

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
			v, il_signals))
		{
			proc->restart_all();
			ret = 0;
			continue;
		}
		int 	cnt = v->size()/sizeof(int);
		int	*signo = (int *)v->ptr();
		premptyset(&sigs);
		for(int i = 0; i < cnt; i++, signo++)
			praddset(&sigs, *signo);

		if (!cmd)
		// no event - just set signals
		{
			if (ignore)
			{
				if (!pobj->ignore_sigs(&sigs, list->p_type))
					ret = 0;
			}
			else
			{
				if (!pobj->catch_sigs(&sigs, list->p_type))
					ret = 0;
			}
		}
		else
		{
			eptr = new Sig_e(sigs, eid, list->p_type, quiet, cmd, 
				pobj);
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
		}
		if (!proc->restart_all())
			ret = 0;
	}
	vec_pool.put(v);
	proglist.free_plist(list_head);
	if (cmd)
	{
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
	else
		return ret;
}

// set default signal state for entire debugger
int
set_signal_defaults(IntList *slist, int ignore)
{
	int	cnt;
	int	*signo;
	Vector	*v = vec_pool.get();

	if (!parse_list(0, 0, slist, v, il_signals))
	{
		vec_pool.put(v);
		return 0;
	}
	cnt = v->size()/sizeof(int);
	signo = (int *)v->ptr();
	for(int i = 0; i < cnt; i++, signo++)
	{
		if (ignore)
			prdelset(&default_sigs, *signo);
		else
			praddset(&default_sigs, *signo);
	}
	vec_pool.put(v);
	return 1;
}

void
display_default_signals()
{
	printm(MSG_default_signal_header);
	for (int i = 1; i <= NUMBER_OF_SIGS; i++)
	{
		printm(prismember(&default_sigs, i) ?
			MSG_sig_caught_default :
			MSG_sig_ignored_default,
			i, signame(i));
	}
}
