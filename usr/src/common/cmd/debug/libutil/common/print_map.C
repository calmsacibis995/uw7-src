#ident	"@(#)debugger:libutil/common/print_map.C	1.3"
#include	"utility.h"
#include	"ProcObj.h"
#include	"Proglist.h"
#include	"Interface.h"
#include 	"Parser.h"
#include 	"global.h"

int
print_map( Proclist * procl )
{
	int single = 1;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	int	ret = 1;

	if (procl)
	{
		single = 0;
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		// default is current process, not thread
		pobj = (ProcObj *)proglist.current_process();
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
		// MORE - map okay on running proc?
		if (!pobj->state_check(E_DEAD))
		{
			ret = 0;
			continue;
		}
		printm(MSG_map_header, pobj->obj_name());
		if (!pobj->print_map())
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
