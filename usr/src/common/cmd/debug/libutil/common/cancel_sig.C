#ident	"@(#)debugger:libutil/common/cancel_sig.C	1.8"
#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Parser.h"
#include "Proctypes.h"
#include "Interface.h"
#include "Vector.h"
#include <string.h>
#include <signal.h>

int
cancel_sig(Proclist *procl, IntList *slist)
{
	int		single = 1;
	int		ret = 1;
	ProcObj		*pobj;
	plist		*list;
	plist		*list_head = 0;
	sig_ctl		set;
	Vector		*v;

	if (procl)
	{
		single = 0;
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		pobj = proglist.current_object();
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}

	v = vec_pool.get();
	do
	{
		Process	*proc = pobj->process();
		int	cnt;
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE|E_OFF_LWP|E_SUSPENDED)
			|| !proc->stop_all() )
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
		cnt = v->size()/sizeof(int);
		if (cnt)
		{
			int	*signo = (int *)v->ptr();
			premptyset(&set.signals);
			for(int i = 0; i < cnt; i++, signo++)
				praddset(&set.signals, *signo);
		}
		else
		{
			prfillset(&set.signals);
		}
		
		if (!pobj->cancel_sigs(&set))
			ret = 0;
		if (!proc->restart_all())
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	vec_pool.put(v);
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
