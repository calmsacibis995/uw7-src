#ident	"@(#)debugger:libutil/common/set_fcn.C	1.4"
#include "utility.h"
#include "ProcObj.h"
#include "Frame.h"
#include "Symbol.h"
#include "Interface.h"
#include <string.h>

int
set_curr_func( ProcObj *pobj, const char *name)
{
	Symbol	symbol;
	Iaddr	addr;
	Frame	*cframe;
	const char *sname;
	int	found = 0;
	int	frameno;

	if ( name == 0 )
	{
		printe(ERR_internal, E_ERROR, "set_curr_func", __LINE__);
		return 0;
	}
	frameno = count_frames(pobj);
	for(cframe = pobj->topframe(); cframe; 
		cframe = cframe->caller(), frameno--)
	{
		addr = cframe->pc_value();
		symbol = pobj->find_entry(addr);
		if (symbol.isnull())
			continue;
		sname = pobj->symbol_name(symbol);
		if (strcmp(name, sname) == 0)
		{
			found = 1;
			break;
		}
	}
	if (!found)
	{
		printe(ERR_active_func, E_ERROR, name);
		return 0;
	}

	if (!pobj->setframe(cframe))
		return 0;

	if (get_ui_type() == ui_gui)
		printm(MSG_set_frame, (unsigned long)pobj, frameno);

	return 1;
}
