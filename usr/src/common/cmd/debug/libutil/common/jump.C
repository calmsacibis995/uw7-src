#ident	"@(#)debugger:libutil/common/jump.C	1.10"

#include "utility.h"
#include "Iaddr.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Frame.h"
#include "Symbol.h"
#include "Tag.h"
#include "global.h"
#include "FileEntry.h"

// change program counter 

int
jump(Proclist *procl, Location *location)
{
	Iaddr	addr;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	Symbol	func;
	int	ret = 1;
	int	single = 1;

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
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}

		if ( get_addr( pobj, location, addr, st_func, func ) == 0 )
		{
			ret = 0;
			continue;
		}

		func = pobj->find_entry(addr);

		while(!func.isnull() && (!IS_ENTRY(func.tag())))
			func = func.parent();
		if (!func.isnull())
		{
			Iaddr	lo, hi, pc;
			if (func.tag() == t_label)
				func = func.parent();
			pc = pobj->pc_value();
			lo = func.pc(an_lopc);
			hi = func.pc(an_hipc);
	
			if (pc < lo || pc >= hi)
				printe(ERR_non_local_jump, E_WARNING,
					pobj->obj_name());
		}
		if (!pobj->set_pc(addr))
		{
			ret = 0;
			continue;
		}
		if (get_ui_type() == ui_gui)
		{
			const FileEntry	*fentry;
			long		line;
			Symbol		comp_unit;

			if (!current_loc(pobj, pobj->topframe(), comp_unit, &fentry, 0, &line) 
				|| !fentry || !fentry->file_name)
			{
				printm(MSG_jump, (unsigned long)pobj,
					addr, "", 0);
			}
			else 
				printm(MSG_jump, (unsigned long)pobj,
					addr, fentry->file_name, line);
		}
		else
		{
			pobj->show_current_location(1, es_invalid, vmode);
		}
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
