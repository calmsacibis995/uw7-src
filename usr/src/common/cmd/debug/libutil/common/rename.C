#ident	"@(#)debugger:libutil/common/rename.C	1.4"

#include "utility.h"
#include "global.h"
#include "str.h"
#include "ProcObj.h"
#include "Program.h"
#include "Proglist.h"
#include "Process.h"
#include "Interface.h"

// reset name by which an entire program is known

int
rename_prog(const char *oldname, const char *newname)
{
	Program	*op = 0, *p;
	char	*o, *n;

	n = str(newname);
	o = str(oldname);
	for (p = proglist.prog_list(); p ; p = p->next())
	{
		if (!op && (o == p->prog_name()) && p->proclist())
			op = p;
		if (n == p->prog_name() && p->proclist())
			break;
	}
	if (p)
	{
		printe(ERR_name_in_use, E_ERROR, n);
		return 0;
	}
	if (!op)
	{
		printe(ERR_no_such_prog, E_ERROR, oldname);
		return 0;
	}
	op->rename(n);
	if (get_ui_type() == ui_gui)
		printm(MSG_rename, oldname, newname);
	return 1;
}
