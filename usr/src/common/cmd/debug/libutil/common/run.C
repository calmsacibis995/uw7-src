#ident	"@(#)debugger:libutil/common/run.C	1.4"
#include "utility.h"
#include "global.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "List.h"
#include "Interface.h"
#include "Location.h"
#include "Parser.h"
#include "Frame.h"
#include "Symbol.h"
#include "Tag.h"

int
run( Proclist *procl, int ret_func, Location *location, int wait )
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	Iaddr	addr;
	int	single = 1;
	int	ret = 1;
	Symbol	func;

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
		addr = 0;
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE|E_OFF_LWP|E_SUSPENDED))
		{
			ret = 0;
			continue;
		}
		if (location)
		{
			if ( get_addr( pobj, location, addr, st_func, func ) == 0 )
			{
				ret = 0;
				continue;
			}
			// if stop requested on function name, 
			// go past prolog
			if (location->get_type() == lk_fcn && 
				(func.tag() != t_label))
			{
				long	off;
				location->get_offset(off);
				if (off == 0)
					addr = pobj->first_stmt(addr);
			}
		}
		else if (ret_func)
		{
			Frame	*frame = pobj->topframe();
			Iaddr	tmp1, tmp2;

			if (frame->retaddr(addr, tmp1, tmp2) == 0)
			{
				printe(ERR_return_addr, E_ERROR, 
					pobj->prog_name());
				ret = 0;
				continue;
			}
		}
		if (!pobj->run( 0, addr, vmode ))
		{
			ret = 0;
			continue;
		}
		if (wait == WAIT)
		{
			pobj->set_wait();
			waitlist.add(pobj);
		}
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	if (list_head)
		proglist.free_plist(list_head);
	if ((wait == WAIT) && !waitlist.isempty())
		wait_process();
	return ret;
}
