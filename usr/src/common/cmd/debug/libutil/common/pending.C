#ident	"@(#)debugger:libutil/common/pending.C	1.7"
#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Parser.h"
#include "Interface.h"
#include "Proctypes.h"
#include "Machine.h"
#include <signal.h>

int
pending_sigs(Proclist *procl)
{
	int	single = 1;
	int	ret = 1;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;

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
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	do
	{
		sig_ctl		sigs;
		sigset_t	*set;
		
		Process	*proc = pobj->process();

		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}
		if (!pobj->pending_sigs(&sigs))
		{
			proc->restart_all();
			ret = 0;
			continue;
		}

		set = &sigs.signals;

		for (int i = 1; i <= NUMBER_OF_SIGS; i++)
		{
			if (prismember(set, i))
			{
				printm(MSG_signame, i, signame(i));
			}
		}
		if (!proc->restart_all())
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
