#ident	"@(#)debugger:libutil/common/ps.C	1.17"

#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Program.h"
#include "global.h"
#include "Interface.h"
#include "Procctl.h"
#include "Proctypes.h"
#include "Thread.h"
#include "Parser.h"
#include "FileEntry.h"
#include "Buffer.h"
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>

#define NAME_LEN	10	// length of program name for output

static void
print_status(ProcObj *pobj)
{
	const char	*pname;
	Execstate	state;
	const char	*current = " ";	// assume non-current
	Msg_id		msg;
	int		stopped = 0;

	// add '*' to denote current object
	if (pobj == proglist.current_object())
		current = "*";

	pname = pobj->prog_name();
	state = pobj->get_state();
#ifdef DEBUG_THREADS
	if (pobj->is_suspended())
	{
		stopped = 1;
		if (state == es_corefile || state == es_core_off_lwp)
			msg = MSG_threads_ps_core_suspend;
		else
			msg = MSG_threads_ps_suspend;

	}
	else
#endif
	{
		switch(state) 
		{
		case es_corefile:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_core;
	#else
			msg = MSG_ps_core;
	#endif
			stopped = 1;
			break;
		case es_halted:
		case es_stepped:
		case es_breakpoint:
		case es_signalled:
		case es_syscallent:
		case es_syscallxit:
		case es_watchpoint:
		case es_procstop:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_stopped;
	#else
			msg = MSG_ps_stopped;
	#endif
			stopped = 1;
			break;
	#ifdef DEBUG_THREADS
		case es_core_off_lwp:
			msg = MSG_threads_ps_core_off_lwp;
			stopped = 1;
			break;
		case es_off_lwp:
			msg = MSG_threads_ps_off_lwp;
			stopped = 1;
			break;
	#endif
		case es_running:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_running;
	#else
			msg = MSG_ps_running;
	#endif
			break;
		case es_stepping:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_stepping;
	#else
			msg = MSG_ps_stepping;
	#endif
			break;
		default: 
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_unknown;
	#else
			msg = MSG_ps_unknown;
	#endif
			break;
		}
	}
#ifdef DEBUG_THREADS
	thread_t	tid;
	if (pobj->obj_type() == pobj_thread)
		tid = ((Thread *)pobj)->thread_id();
	else
		tid = (thread_t)-1;
#endif
	if (stopped)
	{
		char		location[17];
		const char	*func_name = 0;
		char		*loc = 0;
		Symbol		comp_unit;
		long		line = 0;
		const FileEntry	*fentry;
		Buffer		*buf = 0;

		current_loc(pobj, 0, comp_unit, &fentry, &func_name, &line);
		if (!func_name)
			func_name = "";

		if (line && fentry && fentry->file_name)
		{
			if (get_ui_type() == ui_gui)
			{
				buf = buf_pool.get();
				buf->clear();
				buf->add(fentry->file_name);
				sprintf(location, "@%d", line);
				buf->add(location);
				loc = (char *)*buf;
			}
			else
			{
				// use basename
				const char	*file = fentry->file_name;
				const char	*fptr = strrchr(file, '/');
				if (fptr)
					file = fptr + 1;
				sprintf(location, "%.10s@%d", file, line);
				loc = location;
			}
		}
		else
		{
			sprintf(location, "%#x", pobj->pc_value());
			loc = location;
		}
#ifdef DEBUG_THREADS
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), (Iaddr)tid, func_name, loc, 
			pobj->program()->command());
#else
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), func_name, loc, 
			pobj->program()->command());
#endif
		if (buf)
			buf_pool.put(buf);
	}
	else
#ifdef DEBUG_THREADS
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), (Iaddr)tid,
			pobj->program()->command());
#else
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), pobj->program()->command());
#endif
}

int 
proc_status(Proclist *procl)
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;

	if (procl)
	{
		list_head = list = proglist.proc_list(procl);
	}
	else
	{
		list_head = list = proglist.all_procs();
	}
	pobj = list++->p_pobj;
	if (!pobj)
	{
		printe(ERR_no_proc, E_WARNING);
		proglist.free_plist(list_head);
		return 0;
	}
	sigrelse(SIGINT);
#ifdef DEBUG_THREADS
	printm(MSG_threads_ps_header);
#else
	printm(MSG_ps_header);
#endif

	for(; pobj;  pobj = list++->p_pobj)
	{
		if (prismember(&interrupt, SIGINT))
			break;
		print_status(pobj);
	}
	sighold(SIGINT);
	proglist.free_plist(list_head);
	return 1;
}
