#ident	"@(#)debugger:libutil/common/whatis.C	1.6"

#include "Parser.h"
#include "utility.h"
#include "Expr.h"
#include "Proglist.h"
#include "ProcObj.h"
#include "Proctypes.h"
#include "Interface.h"
#include "global.h"
#include <signal.h>

int
whatis(Proclist *procl, const char *exp) 
{
	plist		*list;
	plist		*list_head;
	ProcObj  	*pobj;
	int		ret = 1;
	
	if (procl)
	{
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		list_head = list = 0;
		pobj = proglist.current_object();
	}

	int multiple = list && list->p_pobj;

	sigrelse(SIGINT);
	do
	{
		if (prismember(&interrupt, SIGINT))
			break;

		Expr	*expr = new_expr((char *)exp, pobj, 0, 1);
		if (!expr->print_type(multiple))
			ret = 0;
		delete expr;
	} while (multiple && ((pobj = list++->p_pobj) != 0));

	sighold(SIGINT);
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
