#ident	"@(#)debugger:libutil/common/regs.C	1.5"
#include "Interface.h"
#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Proctypes.h"
#include "Parser.h"
#include "global.h"
#include <signal.h>

int
printregs( Proclist * procl )
{
	int 	single = 1;
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
		pobj = proglist.current_object();
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	sigrelse(SIGINT);
	do
	{

		if (!pobj->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}
		printm(MSG_reg_header, pobj->obj_name(), 
			pobj->prog_name());
		if (!pobj->display_regs(pobj->curframe()))
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0) &&
		!prismember(&interrupt, SIGINT));

	if (list_head)
		proglist.free_plist(list_head);
	sighold(SIGINT);
	return ret;
}
