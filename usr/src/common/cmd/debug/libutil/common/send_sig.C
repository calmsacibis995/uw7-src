#ident	"@(#)debugger:libutil/common/send_sig.C	1.7"
#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Procctl.h"

// send signal to list of threads and processes - if -pprocid
// is given, we use process level routine instead of parsing
// list into individual threads (i.e. p1== p1, not p1.1,p1.2,p1.3)

int
send_signal( Proclist * procl, IntList *slist )
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	int	ret = 1;
	int	single = 1;
	int	current_changed = 0;
	int	destroy = 0;

	if (procl)
	{
		single = 0;
		list_head = list = proglist.proc_list(procl, 1);
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
		int	signo;
		IntItem	*it = slist->first();
		if (it->get_type() == it_int)
			signo = it->get_ival();
		else if ((signo = parse_sig(pobj, 0, 
			it->get_sval())) == 0)
		{
			ret = 0;
			continue;
		}
		if (signo == SIGKILL)
		{
			Process	*proc = pobj->process();
			if (proc == proglist.current_process())
				current_changed = 1;
			ret = destroy_process(proc, 1);
			destroy = 1;
			continue;
		}
		if (!pobj->state_check(E_CORE|E_DEAD|E_OFF_LWP|E_SUSPENDED))
		{
			ret = 0;
			continue;
		}
		if (!pobj->proc_ctl()->kill(signo))
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	if (list_head)
		proglist.free_plist(list_head);
	if (current_changed)
		proglist.reset_current(1);
	if (destroy)
		proglist.prune();
	return ret;
}
