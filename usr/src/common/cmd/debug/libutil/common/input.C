#ident	"@(#)debugger:libutil/common/input.C	1.2"

#include "Interface.h"
#include "ProcObj.h"
#include "Process.h"
#include "Program.h"
#include "PtyList.h"
#include "Proglist.h"
#include "Thread.h"
#include "utility.h"
#include <string.h>
#include <unistd.h>

// send input to a process whose I/O has been redirected
// to a pseudo-tty.

int
input_pty(const char *procname, const char *ptyname, 
	char *instring, int nonewline)
{
	PtyInfo	*pty = 0;

	if (ptyname)
	{
		for (pty = first_pty; pty; pty = (PtyInfo *)pty->next())
			if (!strcmp(ptyname, pty->name()))
				break;
		if (!pty)
		{
			printe(ERR_pty_exist, E_ERROR, ptyname);
			return 0;
		}
	}
	else if (procname)
	{
		Program *prog;
		Process	*proc;
		Thread	*thread;

		if ((prog = proglist.find_prog(procname)) != 0)
			pty = prog->childio();
		else if ((proc = proglist.find_proc(procname)) != 0)
			pty = proc->program()->childio();
		else if ((thread = proglist.find_thread((char *)procname)) != 0)
			pty = thread->process()->program()->childio();
		else
		{
			printe(ERR_object_unknown, E_ERROR, procname);
			return 0;
		}
	}
	else
	{
		Program *prog = proglist.current_program();
		if (!prog)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		pty = prog->childio();
	}
	if (!pty)
	{
		printe(ERR_prog_no_pty, E_ERROR);
		return 0;
	}

	sigrelse(SIGPOLL);

	size_t len = strlen(instring);
	if (!nonewline)	// temporarily add new line
		instring[len++] = '\n';
	write(pty->pt_fd(), instring, len);
	if (!nonewline)
		instring[len-1] = '\0';

	sighold(SIGPOLL);
	return 1;
}
