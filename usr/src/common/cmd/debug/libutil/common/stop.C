#ident	"@(#)debugger:libutil/common/stop.C	1.4"

#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"

int
stop_process(Proclist *procl)
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	int	single = 1;
	int	ret = 1;

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
		if (!pobj->state_check(E_CORE|E_DEAD|E_OFF_LWP|E_SUSPENDED))
		{
			ret = 0;
			continue;
		}
		if (!pobj->stop())
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
