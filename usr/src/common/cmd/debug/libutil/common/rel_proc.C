#ident	"@(#)debugger:libutil/common/rel_proc.C	1.4"

#include "utility.h"
#include "Procctl.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"

int
release_process( ProcObj * pobj, int run )
{
	if (pobj == 0)
		return 0;

	return pobj->release_obj(run);
}

// release list of threads and processes - if -pprocid
// is given, we use process level routine instead of parsing
// list into individual threads (i.e. p1== p1, not p1.1,p1.2,p1.3)
int
release_proclist( Proclist *procl, int run)
{
	int	single = 1, rel_current = 0;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	int	ret = 1;

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
		if (pobj->obj_type() == pobj_thread)
		{
			if ((Thread *)pobj == proglist.current_thread())
				rel_current = 1;
		}
		else
		{
			if ((Process *)pobj == 
				proglist.current_process())
				rel_current = 1;
		}
		if (!release_process(pobj, run))
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	if (list_head)
		proglist.free_plist(list_head);
	if (rel_current)
	{
		proglist.reset_current(1);
	}
	proglist.prune();
	return ret;
}
