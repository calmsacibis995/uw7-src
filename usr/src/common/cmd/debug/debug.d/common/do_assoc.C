#ident	"@(#)debugger:debug.d/common/do_assoc.C	1.5"

#include "Event.h"
#include "EventTable.h"
#include "Parser.h"
#include "Proglist.h"
#include "Interface.h"
#include "ProcObj.h"
#include "List.h"
#include "global.h"

List	m_cmdlist;

int
do_assoccmds()
{
	A_cmd		*cp;
	ProcObj		*pobj, *ocurrent = proglist.current_object();
	int		oevent = m_event.this_event();
	int		found = 0;
	static int	doing_assoc;

	if (doing_assoc)
		return 1;
	doing_assoc = 1;
	if ((cp = (A_cmd *)m_cmdlist.first()) != 0)
	{
		if (get_ui_type() == ui_gui)
			printm(MSG_script_on);
		for(; cp; cp=(A_cmd *)m_cmdlist.next())
		{
			found++;
			if ((pobj = cp->event->get_obj()) == 0)
			{
				continue;
			}
			proglist.set_current(pobj, 0);
			m_event.set_this(cp->event->get_id());
			last_error = 0;
			execute(cp->cmd);
		}
		if (get_ui_type() == ui_gui)
			printm(MSG_script_off);
	}
	if (found)
	{
		m_cmdlist.clear();
		if (ocurrent->get_state() == es_dead)
			proglist.reset_current(0);
		else
			proglist.set_current(ocurrent, 0);
		m_event.set_this(oevent);
	}
	doing_assoc = 0;
	return 1;
}
