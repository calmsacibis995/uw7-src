/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libexecon/common/Proglist.C	1.17"

#include "Proglist.h"
#include "Program.h"
#include "Process.h"
#include "Parser.h"
#include "ProcObj.h"
#include "Thread.h"
#include "Interface.h"
#include "Dbgvarsupp.h"
#include "str.h"
#include "utility.h"
#include "NewHandle.h"
#include "Machine.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>


// Routines to manage lists of programs, processes and threads
// and to keep track of process identifiers and current program,
// proc, lwp.

#define	ALL_LIVE	1	// no core files - includes idle threads
#define USER_ONLY	2	// live and core, but no idle threads
#define PROCESS_LEVEL	4	// only processes - don't
				// go to thread level

#define LIST_ITEMS	20	// initial items in a plist

Proglist	proglist;  // global program list

Proglist::Proglist()
{
	memset(this, 0, sizeof(*this));
	all_name = str("all");
	prog_name = str("%program");
	proc_name = str("%proc");
	thread_name = str("%thread");
}

Proglist::~Proglist()
{
	plist_header *header;
	plist_header *tmp;
	for (header = plist_in_use; header; header = tmp)
	{
		tmp = header->next;
		delete header;
	}
	for (header = plist_free; header; header = tmp)
	{
		tmp = header->next;
		delete header;
	}
}

void
Proglist::add_program(Program *p)
{

	if (p->is_proto())
	{
		// add proto programs at end
		if (last_program)
			p->append(last_program);
		else
			first_program = p;
		last_program = p;
			
		return;
	}
	// Maintain programs in alphabetical order
	// for ps command.
	if (first_program)
	{
		Program	*prog;
		for (prog = first_program; prog; prog = prog->next())
		{
			if (prog->prog_name() &&
				(strcmp(prog->prog_name(), 
					p->prog_name()) > 0))
				break;
		}
		if (prog)
			p->prepend(prog);
		else
		{
			p->append(last_program);
			last_program = p;
		}
		if (first_program == prog)
			first_program = p;
	}
	else
	{
		last_program = p;
		first_program = last_program;
	}
}

void
Proglist::remove_program(Program *prog)
{
	if (prog == last_program)
		last_program = (Program *)prog->prev();
	if (prog == first_program)
		first_program = prog->next();
	prog->unlink();
	delete prog;
}

// Given a -p list, return pointer to a list of ProcObjs.
// Each returned list item also indicates whether the original
// request reflected the entire program, process or just
// a thread.
//
// We start with a fixed size malloc'd list and realloc if 
// necessary.
// 
// We always return a plist.  If no ProcObjs were found, the first
// list entry is null
//
// There may be multiple plists in use (but not being built) at the
// same time.  The plist_header contains information about the amount
// of space allocated/used for the specific plist.
// Proglist maintains a pool of plist_headers.  If a plist is needed,
// it looks first for one on the free list before allocating a new one.

void
Proglist::setup_list()
{
	prune();	// get rid of dead wood in list
	if (plist_free)
	{
		plist_head = plist_free;
		plist_free = plist_free->next;
	}
	else
	{
		plist_head = new plist_header;
		grow_list();
	}
	plist_head->in_use = 1;
	cur_plist = plist_head->ptr;
}

plist *
Proglist::finish_list()
{
	cur_plist->p_pobj = 0;
	plist_head->next = plist_in_use;
	plist_in_use = plist_head;
	return plist_head->ptr;
}

void
Proglist::free_plist(plist *ptr)
{
	if (!ptr)
	{
		if (!user_list)
		{
			printe(ERR_internal, E_ERROR, "Proglist::add_list",
				__LINE__);
			return;
		}
		// free the list in the process of being built
		plist_head->next = plist_free;
		plist_free = plist_head;
		user_list = 0;
		return;
	}

	// remove the pointer from the in-use list and add to the free list
	plist_header	*prev = 0;
	plist_header	*header = plist_in_use;
	for (; header; prev = header, header = header->next)
	{
		if (header->ptr == ptr)
		{
			if (prev)
				prev->next = header->next;
			else
				plist_in_use = header->next;
			header->next = plist_free;
			plist_free = header;
			break;
		}
	}
}

// parse list of process ids, thread ids, program names and pids
plist *
Proglist::proc_list(Proclist *procs, int process_only)
{
	char	*cur_item = (*procs)[0];
	int	list_index = 0;
	int	mode = process_only ? PROCESS_LEVEL|USER_ONLY : USER_ONLY;

	setup_list();
	while(cur_item)
	{
		parse_item(cur_item, mode);
		cur_item = ((*procs)[++list_index]);
	}
	return finish_list();
}

// all ProcObjs in a given program
plist *
Proglist::proc_list(Program *prog, int process_only)
{
	int	mode = process_only ? PROCESS_LEVEL|USER_ONLY : USER_ONLY;
	setup_list();
	if (prog)
		add_prog(prog, mode);
	return finish_list();
}

// all ProcObjs, including core files
plist *
Proglist::all_procs()
{
	setup_list();
	for(Program *p = first_program; p; p = p->next())
		add_prog(p, USER_ONLY);
	return finish_list();
}

// all live ProcObjs, excluding core files
plist *
Proglist::all_live(int process_only)
{
	int mode = process_only ? ALL_LIVE|PROCESS_LEVEL : ALL_LIVE;
	setup_list();
	for(Program *p = first_program; p; p = p->next())
		add_prog(p, mode);
	return finish_list();
}

// all live Processes, excluding core files
// only those matching the given create session id are listed
plist *
Proglist::all_live_id(int id)
{
	setup_list();
	for(Program *p = first_program; p; p = p->next())
	{
		if (p->create_id() == id)
			add_prog(p, PROCESS_LEVEL|ALL_LIVE);
	}
	return finish_list();
}

// give clients a way to build their own lists of ProcObjs
void
Proglist::make_list()
{
	setup_list();
	user_list = 1;
	cur_plist->p_pobj = 0;
}

void
Proglist::add_list(ProcObj *pobj, int level)
{
	if (!user_list)
	{
		printe(ERR_internal, E_ERROR, "Proglist::add_list",
			__LINE__);
		return;
	}
	add_pobj(pobj, level, USER_ONLY);
	cur_plist->p_pobj = 0;
}

plist *
Proglist::list_done()
{
	user_list = 0;
	return finish_list();
}

void
Proglist::parse_item(char * item, int mode)
{
	Process		*proc;
	Program		*prg;
	const char	*ename;

	ename = str(item);
	// determine type of current item
	if (ename == prog_name)	// %program
	{
		if (curr_program)
			add_prog(curr_program, mode);
		return;
	}
	else if (ename == proc_name) // %proc
	{
		if (curr_process)
			add_proc(curr_process, mode);
		return;
	}
	else if (ename == thread_name) // %thread
	{
		if (curr_thread)
			add_thread(0, curr_thread, 0, mode);

		return;
	}
	else if (ename == all_name)	// "all"
	{
		for(prg = first_program; prg; prg = prg->next())
		{
			add_prog(prg, mode);
		}
		return;
	}
	else if (*item == '$' || *item == '%')
	{
		// debug or user variable - may be a list of things
		// but cannot contain another user variable
		static char	*expanding_variable;
		if (expanding_variable)
		{
			printe(ERR_nested_variable, E_ERROR, item, 
				expanding_variable);
			return;
		}
		expanding_variable = item;
		var_table.set_context(0,0,0,1,1);
		if (!var_table.Find(item))
		{
			printe(ERR_unknown_debug_var, E_ERROR, item);
			return;
		}
		char	*value = var_table.Value();
		if (!value)
		{
			printe(ERR_eval_fail_expr, E_ERROR, item);
			return;
		}

		// Don't need to copy value (Debug_var_support is not
		// reentrant) because recursion is caught.
		char	*next_item = value;
		do
		{
			if ((value = strchr(next_item, ',')) != 0)
			{
				*value = 0;
				value++;
			}
			parse_item(next_item, mode);
			next_item = value;
		} while(next_item);
		expanding_variable = 0;
		return;
	}
	else if (isdigit(*item))
	{
		// process id
		char	*p;
		pid_t	 ppid = (pid_t)strtol(item, &p, 10);
		if (!*p)
		{
			if ((proc = find_process(ppid)) != 0)
			{
				add_proc(proc, mode);
				return;
			}
		}
		// FALLTHROUGH
	}
	// process, thread or program name
	const char	*pname, *tname;
	if (parse_name(item, pname, tname))
	{
		// possibly proc or thread name
		if ((proc = find_process(pname, 0)) != 0)
		{
			if (tname)
				add_thread(proc, 0, tname, mode);
			else
				add_proc(proc, mode);
			return;
		}
		// FALLTHROUGH
	}
	// just a name
	if ((prg = find_prog(ename)) != 0)
	{
		add_prog(prg, mode);
		return;
	}
	// if here, no match
	printe(ERR_no_match, E_WARNING, ename);
}

// try parsing name as process or thread id (e.g., p123 or p123.124)
int
Proglist::parse_name(char *name, const char *&pname, 
	const char *&tname)
{
	char	*p;
	char	*dot = 0;

	if (!name || *name != 'p')
		return 0;
	p = name;
	while(*++p)
	{
		if (!isdigit(*p))
		{
			if (*p == '.')
			{
				if (dot)
					break;
				else
					dot = p;
			}
			else
				break;
		}
	}
	if (*p) // perhaps a program name
		return 0;
	if (dot)
	{
		tname = str(name);
		*dot = 0;
		pname = str(name); // up to '.'
		*dot = '.';
	}
	else
	{
		tname = 0;
		pname = str(name);
	}
	return 1;
}

// add a single item to list
void
Proglist::add_pobj(ProcObj *pobj, int type, int mode)
{
	if (mode & PROCESS_LEVEL)
	{
		// make sure we haven't already added an entry for
		// this process
		plist	*pl = plist_head->ptr;
		for(; pl != cur_plist; pl++)
		{
			if (pl->p_pobj == pobj)
				return;
		}
	}

	// always need room for next item and null entry
	if (plist_head->in_use > plist_head->cnt-2)
	{
		grow_list();
		cur_plist = &plist_head->ptr[plist_head->in_use-1];
	}
	cur_plist->p_pobj = pobj;
	cur_plist->p_type = type;
	plist_head->in_use++;
	cur_plist++;
}

// Sub-calls inlined here for efficiency.
// This code is executed frequently and must be reasonably fast.
void
Proglist::add_prog(Program *prg, int mode)
{
	if (prg->is_proto())
		return;
	for(Process *proc = prg->proclist(); proc;
		proc = proc->next())
	{
		if (proc->is_dead() ||
			((mode & ALL_LIVE) && proc->is_core()))
			continue;
		Thread	*thread = proc->thread_list();
		if (!thread || (mode & PROCESS_LEVEL))
		{
			add_pobj((ProcObj *)proc, P_PROGRAM, mode);
			continue;
		}
		for(; thread; thread = thread->next())
		{
			if (thread->is_dead() ||
				((mode & USER_ONLY) && 
				!thread->is_user()))
				continue;
			add_pobj((ProcObj *)thread, P_PROGRAM, mode);
		}
	}
}


void
Proglist::add_proc(Process *proc, int mode)
{
	if (proc->is_dead())
		return;
	Thread	*thread = proc->thread_list();
	if (!thread || (mode & PROCESS_LEVEL))
	{
		add_pobj((ProcObj *)proc, P_PROCESS, mode);
		return;
	}
	for(; thread; thread = thread->next())
	{
		if (thread->is_dead() ||
			!thread->is_user())
			continue;
		add_pobj((ProcObj *)thread, P_PROCESS, mode);
	}
}

void
Proglist::add_thread(Process *proc, Thread *thread, 
	const char *tname, int mode)
{
	Thread		*t;

	if (thread)
	{
		if (thread->is_dead() ||
			!thread->is_user())
			return;
		t = thread;
	}
	else
	{
		for(t = proc->thread_list(); t; t = t->next())
		{
			if ((t->obj_name() == tname) &&
				!t->is_dead() &&
				t->is_user())
				break;
		}
		if (!t)
			return;
	}
	
	add_pobj((ProcObj *)t, P_THREAD, mode);
}

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)

// go through the list of all live processes and
// check whether any need their followers started
void
Proglist::start_all_followers()
{
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		for(Process *proc = prog->proclist(); proc;
			proc = proc->next())
		{
			if (proc->is_dead() || proc->is_core())
				continue;
			proc->check_follow();
		}
	}
}

#endif

// check whether pobj still points to a valid process or thread
int
Proglist::valid(ProcObj *pobj)
{
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		for(Process *proc = prog->proclist(); proc;
			proc = proc->next())
		{
			Thread	*thread;
			if (pobj == (ProcObj *)proc)
				return(!proc->is_dead());
			thread = proc->thread_list();
			for(; thread; thread = thread->next())
			{
				if (pobj == (ProcObj *)thread)
					return(!thread->is_dead());
			}		
		}
	}
	return 0;
}

// add LIST_ITEMS entries to list
void
Proglist::grow_list()
{
	plist_head->cnt += LIST_ITEMS;
	if (!plist_head->ptr)
	{
		// original list growth
		plist_head->ptr = (plist *)
			malloc(LIST_ITEMS * sizeof(plist));
	}
	else
	{
		plist_head->ptr = (plist *)
			realloc(plist_head->ptr, plist_head->cnt * sizeof(plist));
	}
	if (!plist_head->ptr)
	{
		newhandler.invoke_handler();
	}
}

// returns current object, thread if there is one, else process
ProcObj	*
Proglist::current_object()
{
	if (curr_thread)
		return((ProcObj *)curr_thread);
	else
		return((ProcObj *)curr_process);
}

void
Proglist::set_current(Thread *thread, int announce)
{
	Process	*pptr;
	if (!thread || !thread->is_user())
	{
		curr_process = 0;
		curr_thread = 0;
		curr_program = 0;
		printe(ERR_internal, E_ERROR, "Proglist:set_current",
			__LINE__);
		return;
	}
	curr_thread = thread;
	if (announce)
	{
		printm(MSG_new_curr_thread, thread->obj_name(), 
			thread->prog_name());
	}
	pptr = thread->process();
	if ((curr_process != pptr) && announce)
	{
		printm(MSG_new_context, pptr->obj_name(), 
			thread->prog_name());
	}
	curr_process = pptr;
	curr_program = curr_process->program();
}

void
Proglist::set_current(Process *proc, int announce)
{
	if (!proc)
	{
		curr_process = 0;
		curr_thread = 0;
		curr_program = 0;
		printe(ERR_internal, E_ERROR, "Proglist:set_current",
			__LINE__);
		return;
	}
	curr_process = proc;
	curr_thread = proc->first_thread(0); // may be null
	curr_program = proc->program();
	if (announce)
	{
		if (curr_thread)
		{
			printm(MSG_new_curr_thread, curr_thread->obj_name(), 
				curr_thread->prog_name());
		}
		printm(MSG_new_context, proc->obj_name(), 
			proc->prog_name());
	}
}

void
Proglist::set_current(ProcObj *pobj, int announce)
{
	if (pobj->obj_type() == pobj_thread)
		set_current((Thread *)pobj, announce);
	else
		set_current((Process *)pobj, announce);
}

void
Proglist::set_current(Program *prog, int announce)
{

	Process	*p;

	if (!prog || ((p = prog->first_process()) == 0))
	{
		curr_process = 0;
		curr_thread = 0;
		curr_program = 0;
		printe(ERR_internal, E_ERROR, "Proglist:set_current",
			__LINE__);
		return;
	}
	curr_program = prog;
	curr_process = p;
	curr_thread = p->first_thread(0); // may be null
	if (announce)
	{
		if (curr_thread)
		{
			printm(MSG_new_curr_thread, curr_thread->obj_name(), 
				curr_thread->prog_name());
		}
		printm(MSG_new_context, p->obj_name(), 
			p->prog_name());
	}
}

Process *
Proglist::find_process(const char *name, int rpt_err)
{
	char	*pname;

	pname = (char *)name;
	while(*++pname)
	{
		if (!isdigit(*pname))
		{
			if (rpt_err)
				printe(ERR_proc_name, E_ERROR, name);
			return 0;
		}
	}
	pname = str(name);
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		for(Process *proc = prog->proclist(); proc;
			proc = proc->next())
		{
			if ((proc->obj_name() == pname) &&
				!proc->is_dead())
					return proc;
		}
	}
	return 0;
}

Process *
Proglist::find_process(pid_t ppid)
{
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		for(Process *proc = prog->proclist(); proc;
			proc = proc->next())
		{
			if ((proc->pid() == ppid) &&
				!proc->is_dead())
					return proc;
		}
	}
	return 0;
}

// find ProcObj associated with a given name; if name is a program
// or process name, find first active thread, or process, if not
// multithreaded
ProcObj *
Proglist::find_pobj(char *name)
{
	const char	*pname, *tname;
	const char	*ename;
	Thread		*thread;
	Process		*proc;

	ename = str(name);
	// determine type of current item
	if (ename == prog_name)	// %program
	{
		if (curr_program)
		{
			proc = curr_program->first_process();
			if (proc)
				thread = proc->first_thread(0);
			return(thread ? (ProcObj *)thread : 
				(ProcObj *)proc);
		}
		else
			return 0;
	}
	else if (ename == proc_name) // %proc
	{
		if (curr_process)
		{
			thread = curr_process->first_thread(0);
			return(thread ? (ProcObj *)thread : 
				(ProcObj *)curr_process);
		}
		else
			return 0;
	}
	else if (ename == thread_name) // %thread
	{
		return((ProcObj *)curr_thread);
	}
	else if (*ename == '$')
	{
		// user variable - only a single item
		var_table.set_context(0,0,0,0,1);
		var_table.Find((char *)ename);
		name = var_table.Value();
		if (!name)
		{
			printe(ERR_unknown_debug_var, E_ERROR, ename);
			return 0;
		}
		// FALLTHROUGH
	}
	if (isdigit(*name))
	{
		char	*p;
		ProcObj	*t;
		pid_t	pid = (pid_t)strtol(name, &p, 10);

		if (!*p)
			if ((t = find_pobj(pid)) != 0)
				return t;
		// FALLTHROUGH
	}
	if (parse_name(name, pname, tname))
	{
		// might be process or thread id
		if ((proc = find_process(pname, 0)) != 0)
		{
			if (!tname)
			{
				thread = proc->first_thread(0);
				return(thread ? 
					(ProcObj *)thread : 
					(ProcObj *)proc);
			}
			for(Thread *t = proc->thread_list(); 
				t; t = t->next())
			{
				if ((t->obj_name() == tname)
				&& (!t->is_dead() && 
					t->is_user()))
					return t;
			}
		}
		// FALLTHROUGH
	}
	// program
	Program	*prog;
	if ((prog = find_prog(name)) == 0)
		return 0;
	proc = prog->first_process();
	if (!proc)
		return 0;
	thread = proc->first_thread(0);
	return(thread ? (ProcObj *)thread : (ProcObj *)proc);
}

// find first active thread associated with a given process id
ProcObj *
Proglist::find_pobj(pid_t pid)
{
	Process	*proc;

	if ((proc = find_process(pid)) != 0)

	{
		Thread	*thread;
		thread = curr_process->first_thread(0);
		return(thread ? (ProcObj *)thread : (ProcObj *)proc);
	}
	return 0;
}

Program *
Proglist::find_prog(const char *name)
{
	char	*p = str(name);
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		// check for proto program
		if ((prog->prog_name() == p) && !prog->is_proto())
			return prog;
	}
	return 0;
}

// assumes pid or process identifier
Process *
Proglist::find_proc(const char *name)
{

	pid_t	pid;
	char	*p = 0;
	char	*s;

	if (*name == 'p')
	{
		return(find_process(name, 1));
	}
	else
	{
		pid = (pid_t)strtol(name, &s, 10);
		if (*p)
			return 0;
		return(find_process(pid));
	}
}

Thread *
Proglist::find_thread(const char *name)
{
	// assume name is in form pnnn.nnn
	const char	*p = name;
	char		buf[sizeof("p") + MAX_LONG_DIGITS];
	char		*to = buf;
	Process		*proc;

	// copy name up to dot
	while(*p && *p != '.')
		*to++ = *p++;
	if (!*p)
	{
		printe(ERR_thread_name, E_ERROR, name);
		return 0;
	}
	*to = 0;

	if ((proc = find_process(buf, 0)) != 0)
	{
		char	*tname = str(name);

		for(Thread *thread = proc->thread_list(); 
			thread; thread = thread->next())
		{
			if ((thread->obj_name() == tname) &&
				(!thread->is_dead()
				&& thread->is_user()))
				return thread;
		}
	}
	return 0;
}

// Go through list of programs, processes, threads.  Delete
// all dead threads, all processes that have no live threads
// and all programs that have no live processes.
void
Proglist::prune()
{
	Program *prog = first_program; 
	while(prog)
	{
		Process	*proc = prog->proclist();
		while(proc)
		{
			Thread	*thread = proc->thread_list();
			while(thread)
			{
				if (thread->is_dead())
				{
					Thread	*tthread = thread;
					thread = thread->next();
					proc->remove_thread(tthread);
					continue;
				}
				thread = thread->next();
			}
			if (!proc->thread_list() && proc->is_dead())
			{
				// dead process
				Process	*tproc = proc;
				proc = proc->next();
				prog->remove_proc(tproc);
				continue;
			}
			proc = proc->next();
		}
		if (!prog->proclist())				// dead program
		{
			if (!prog->events()) // don't remove proto prog
			{
				Program	*tprg = prog;
				prog = prog->next();
				remove_program(tprg);
				continue;
			}
		}
		prog = prog->next();
	}
}

// Reset notion of current program, process, thread.
void
Proglist::reset_current(int announce)
{
	Program	*prog;
	Process	*proc = 0;
	Thread	*thread  = 0;

	// First look for a sibling of current thread.
	if (curr_thread)
	{
		if (!curr_process)
		{
			printe(ERR_internal, E_ERROR, 
				"Proglist::reset_current", __LINE__);
			return;
		}
		for(thread = curr_process->thread_list(); thread; 
			thread = thread->next())
		{
			if ((thread != curr_thread) && 
				!thread->is_dead() &&
				thread->is_user())
				goto out;
		}
	}
	// Next look for a sibling process.
	if (curr_process)
	{
		if (!curr_program)
		{
			printe(ERR_internal, E_ERROR, 
				"Proglist::reset_current", __LINE__);
			return;
		}
		for(proc = curr_program->proclist(); 
			proc; proc = proc->next())
		{
			if ((proc != curr_process) &&
				!proc->is_dead())
				goto out;
		}
	}
	// Now look for a sibling program.
	for(prog = first_program; prog; prog = prog->next())
	{
		if (prog == curr_program)
			continue;
		if ((proc = prog->first_process()) != 0)
			goto out;
	}
out:
	if (!proc && !thread)
	{
		curr_thread = 0;
		curr_process = 0;
		curr_program = 0;
		if (announce)
			printm(MSG_no_process);
		return;
	}
	if (!thread)
	{
		// sibling process
		curr_thread = proc->first_thread(0); // may be null
		curr_process = proc;
		if (announce)
		{
			if (curr_thread)
			{
				printm(MSG_new_curr_thread, 
					curr_thread->obj_name(), 
					curr_thread->prog_name());
			}
			printm(MSG_new_context, proc->obj_name(), 
				proc->prog_name());
		}
	}
	else
	{
		curr_thread = thread;
		if (announce) 
		{
			printm(MSG_new_curr_thread, curr_thread->obj_name(), 
				curr_thread->prog_name());
		}
	}
	curr_program = curr_process->program();
}

#if DEBUG
#include "Procctl.h"
#include "Proccore.h"
void
Proglist::print_list()
{
	printf("Current program %#x Current process %#x current thread %#x\n",
		curr_program, curr_process, curr_thread);
	for(Program *prg = first_program; prg; prg = prg->next())
	{
		printf("Program: %#x	", prg);
		if (prg->is_proto())
			printf("proto: %s\n", prg->exec_name()? prg->exec_name():
				"?");
		else
			printf("%s\n", prg->prog_name()? prg->prog_name(): "?");
		for(Process *proc = prg->proclist(); proc; proc = proc->next())
		{
			printf("\tProcess: %#x	", proc);
			printf("%s pid = %d\n", proc->obj_name()? proc->obj_name(): "?", proc->pid());
#ifdef DEBUG_THREADS
			for(Thread *l = proc->thread_list(); l; l = l->next())
			{
				printf("\t\tThread: %#x	", l);
				if (l->is_dead())
					printf("<dead> ");
				else if (l->is_virtual())
					printf("<virtual> ");
				else if (l->is_released())
					printf("<released> ");
				printf("%s ", l->obj_name()? l->obj_name(): "?");
				if (!l->is_virtual())
					printf("tid = %d ", l->thread_id());
				if (l->proc_ctl())
				{
					Lwplive *llive = (Lwplive *)l->proc_ctl();
					printf("lwp id = %d", llive->lwp_id());
				}
				else if (l->core_ctl())
				{
					Lwpcore *lcore = (Lwpcore *)l->core_ctl();
					printf("lwp id = %d", lcore->lwp_id());
				}
				printf("\n");
			}
#endif
		}
	}
}
#endif
