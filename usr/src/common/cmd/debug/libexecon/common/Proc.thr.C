#ident	"@(#)debugger:libexecon/common/Proc.thr.C	1.25"
#include "Process.h"
#include "ProcObj.h"
#include "Procctl.h"
#include "Proccore.h"
#include "Thread.h"
#include "Symbol.h"
#include "Seglist.h"
#include "Symtab.h"
#include "Place.h"
#include "Interface.h"
#include "Manager.h"
#include "global.h"
#include <stddef.h>

#if defined(DEBUG_THREADS) && defined(DEBUG)
extern void dump_thread_desc(thread_map *);
#endif

// Process level routines to deal with threads
// LWPs that are not currently running user threads are
// associated with special debugger "virtual threads".
// These "threads" are only internal debugger data structures.
// They do not exist within the subject process.

void
Process::add_thread(Thread *thread)
{
#ifdef DEBUG_THREADS
	// always add to end of list to preserve numeric order
	if (last_thread)
		thread->append(last_thread);
	else
		head_thread = thread;
	last_thread = thread;
#endif
}

void
Process::remove_thread(Thread *thread)
{
#ifdef DEBUG_THREADS
	if (thread == last_thread)
		last_thread = (Thread *)thread->prev();
	if (thread == head_thread)
		head_thread = thread->next();
	thread->unlink();
	delete thread;
#endif
}

// return first active thread assocated with this process
// virtual threads are not returned, unless virtual_ok is non-0.
// for core files, return id of thread that caused the core
// dump (we assume it must have been running on an LWP
Thread *
Process::first_thread(int virtual_ok)
{
#ifdef DEBUG_THREADS
	if (core)
	{
		lwpid_t	lid = core->lwp_id();
		if (lid == -1)
		{
			printe(ERR_core_format, E_ERROR);
			return 0;
		}
		for(Thread *t = head_thread; t ; t = t->next())
		{
			if (t->core_ctl() && 
				(t->core_ctl()->lwp_id() == lid))
				return t;
		}
		// FALLTHROUGH
	}
	for(Thread *t = head_thread; t ; t = t->next())
	{
		if (t->is_dead() || t->is_released())
			continue;
		if (!t->is_virtual())
			return t;
		if (virtual_ok)
			return t;
	}
#endif
	return 0;
}

#ifdef DEBUG_THREADS

// move a virtual thread to the unused list
void
Process::mark_unused(Thread *thread)
{
	if (thread == last_thread)
		last_thread = (Thread *)thread->prev();
	if (thread == head_thread)
		head_thread = thread->next();
	thread->unlink();
	if (unused_list)
		thread->prepend(unused_list);
	unused_list = thread;
	thread->set_state(es_unused);
	thread->flags &= ~L_CHECK;
}

// find a thread given its map address
Thread *
Process::find_thread_addr(Iaddr map_ptr)
{
	Thread	*thread = head_thread;
	for(; thread; thread = thread->next())
	{
		if ((thread->map_addr == map_ptr) &&
			!thread->is_dead())
			return thread;
	}
	return 0;
}

// find a thread given an associated lwp id
Thread *
Process::find_thread_id(lwpid_t id)
{
	Thread	*thread = head_thread;
	for(; thread; thread = thread->next())
	{
		if (thread->is_dead())
			continue;
		if (thread->pctl)
		{
			if (((Lwplive *)thread->pctl)->lwp_id() == id)
				return thread;
		}
		else if (thread->core)
		{
			if (((Lwpcore *)thread->core)->lwp_id() == id)
				return thread;
		}
		// otherwise, thread is off lwp
	}
	return 0;
}

// determine whether we have threads;
// find threads library debug structure, if present
int
Process::thread_debug_setup()
{
	Symtab			*symtab;
	Symbol			symbol;
	Place			place;
	struct	thread_debug	thr_debug;

	if ((symtab = seglist->find_symtab(thread_library_name)) == 0)
	{
		DPRINT(DBG_THREAD, ("thread_debug_setup: %s no %s\n", pobj_name, thread_library_name));
		return 1;
	}
	symbol = symtab->find_global("_thr_debug");
	if (symbol.isnull())
	{
		DPRINT(DBG_THREAD, ("thread_debug_setup: %s no _thr_debug\n", pobj_name));
		return 1;
	}
	else if (!symbol.place(place, this, 0) || place.kind != pAddress)
	{
		return 0;
	}
	else if (read(place.addr, sizeof(struct thread_debug), 
		(char *)&thr_debug) != sizeof(struct thread_debug))
	{
		return 0;
	}
	thr_dbg_addr = place.addr;
	thr_brk_addr = (Iaddr)thr_debug.thr_brk;
	thr_map_addr = (Iaddr)thr_debug.thr_map;
	DPRINT(DBG_THREAD, ("Process %s, brk_addr %#x, map_addr, %#x\n",pobj_name, thr_brk_addr, thr_map_addr));
	return 1;
}

int
Process::get_thread_desc(Iaddr addr, struct thread_map &desc)
{
	if (read(addr, sizeof(struct thread_map), 
		(char *)&desc) != sizeof(struct thread_map))
	{
		printe(ERR_proc_read, E_ERROR, pobj_name, addr);
		return 0;
	}
	return 1;
}

int
Process::get_lwp_id(struct thread_map *desc, lwpid_t *lid)
{
	if (desc->thr_lwpp == 0)
	{
		printe(ERR_libthreads, E_ERROR,
			pobj_name);
		return 0;
	}
	else if (read((Iaddr)desc->thr_lwpp,
		(int)sizeof(lwpid_t), 
		(char *)lid) != sizeof(lwpid_t))
	{
		printe(ERR_proc_read, E_ERROR,
			pobj_name, 
			(Iaddr)desc->thr_lwpp);
		return 0;
	}
	return 1;
}

int
Process::thread_setup_core()
{
	struct thread_map	desc;
	Iaddr			addr;
	Iaddr			top;
	Lwpcore			*clist, *lcore;
	int			ret = 1;
	int			no_lwp = 0;
	int			suspended;

	if (!core || !thr_map_addr)
	{
		printe(ERR_internal, E_ERROR, 
			"Process::thread_setup_core", __LINE__);
		return 0;
	}
	
	if (read((Iaddr)thr_map_addr, (int)sizeof(Iaddr),
		(char *)&top) != sizeof(Iaddr))
		return 0;
	if (!top)
		return 1;
	top = (Iaddr)THR_MAP((struct thread_map *)top);
	addr = top;
	clist = core->lwp_list();

	// walk linked list of thread descriptors
	// last thread points to top of list
	do
	{
		Thread	*thread;
		Lwpcore	*prev;
		lwpid_t	lid;

		if (!get_thread_desc(addr, desc))
		{
			ret = 0;
			break; // can't continue; don't know address
				// of next entry
		}

		if (THR_MAP_IS_ZOMBIE(&desc))
			continue;
#if DEBUG
		if (debugflag & DBG_THREAD)
			dump_thread_desc(&desc);
#endif
		suspended = THR_MAP_IS_SUSPENDED(&desc);

		if (desc.thr_lwpp)
		{
			if (get_lwp_id(&desc, &lid) == 0)
			{
				ret = 0;
				break;
			}
			DPRINT(DBG_THREAD, ("setup_core:thread %d, lwpid == %d\n", desc.thr_tid, lid));
			prev = 0;
			lcore = clist;
			for(;lcore; prev = lcore, lcore = lcore->next())
			{
				if (lcore->lwp_id() == lid)
					break;
			}
			if (lcore)
			{
				if (prev)
					prev->set_next(lcore->next());
				else
					clist = lcore->next();
			}
			else
			{
				no_lwp = 1;
				ret = 0;
			}
		}
		else
			lcore = 0;
		next_thread++;
		thread = new Thread(next_thread, this, &desc);
		thread->grab_core(lcore, suspended);
		if (no_lwp)
		{
			printe(ERR_thread_no_lwp, E_WARNING, 
				thread->obj_name());
			no_lwp = 0;
		}
		add_thread(thread);
	} while ((desc.thr_next != 0) && 
		(addr = (Iaddr)THR_MAP(desc.thr_next)) != top);
	
	// get rid of idle lwps - not needed for core file display
	while(clist)
	{
		lcore = clist;
		clist = clist->next();
		lcore->close();
		delete lcore;
	}
	core->set_lwp_list(0);
	if (desc.thr_next == 0)
	{
		printe(ERR_core_threads, E_WARNING, pobj_name);
		return 0;
	}
	return ret;
}

// setup threads for live processes: grabbed or newly exec'd
// if old is non-zero, was result of a forkall
int
Process::thread_setup_live(Process *old)
{
	struct thread_map	desc;
	Iaddr			addr;
	Iaddr			top;
	Lwplive			*llist, *llive;
	Thread			*thread;
	int			ret = 1;
	int			no_lwp = 0;
	Iaddr			start_routine;
	int			suspended;
	Iaddr			write_addr;
	int			tdebug_value = 1;

	if (!pctl || !thr_map_addr || !thr_dbg_addr)
	{
		printe(ERR_internal, E_ERROR, 
			"Process::thread_setup_live", __LINE__);
		return 0;
	}
	
	// write value 1 into _thr_debug.thr_debug_on
	// so thread library will know we are debugging
	write_addr = thr_dbg_addr + offsetof(struct thread_debug, thr_debug_on);
	if (write(write_addr, (char *)&tdebug_value,
		(int)sizeof(int)) != sizeof(int))
		return 0;

	if (read((Iaddr)thr_map_addr, (int)sizeof(Iaddr), 
		(char *)&top) != sizeof(Iaddr))
		return 0;
	// in newly exec'd processes, addr will be 0 - no threads yet
	if (top)
		top = (Iaddr)THR_MAP((struct thread_map *)top);
	addr = top;
	llist = pctl->open_all_lwp(pobj_name);
	
	// walk linked list of thread descriptors
	// last thread points to top of list
	do
	{
		Lwplive	*prev;
		lwpid_t	lid;

		if (!addr)
			break;
		if (!get_thread_desc(addr, desc))
		{
			ret = 0;
			break; // can't continue, don't know addr of
			       // next list item
		}
		if (THR_MAP_IS_ZOMBIE(&desc))
		{
			continue;
		}
		if (desc.thr_dbg_startup)
		{
			start_routine = (Iaddr)desc.thr_start_addr;
		}
		else
		{
			start_routine = 0;
		}

		suspended = THR_MAP_IS_SUSPENDED(&desc);

		// is thread on lwp ?
		if (desc.thr_lwpp != 0)
		{
			if (get_lwp_id(&desc, &lid) == 0)
			{
				ret = 0;
				break;
			}
			DPRINT(DBG_THREAD, ("setup_live:thread %d, lwpid == %d\n", desc.thr_tid, lid));
			prev = 0;
			llive = llist;
			for(;llive; prev = llive, llive = llive->next())
			{
				if (llive->lwp_id() == lid)
					break;
			}
			if (llive)
			{
				if (prev)
					prev->set_next(llive->next());
				else
					llist = llive->next();
			}
			else
			{
				ret = 0;
				no_lwp = 1;
			}
		}
		else
		{
			llive = 0;
		}

		next_thread++;
		thread = new Thread(next_thread, this, &desc);
		if (desc.thr_dbg_switch)
			thread->set_inconsistent(0);
		if (!thread->grab_live((Proclive*)llive, addr, 
			start_routine, old, 0, suspended))
		{
			delete thread;
			next_thread--;
			printe(ERR_thread_control, E_WARNING, 
				(int)desc.thr_tid, pobj_name);
			ret = 0;
			continue;
		}
		if (no_lwp)
		{
			printe(ERR_thread_no_lwp, E_WARNING, 
				thread->obj_name());
			no_lwp = 0;
		}
		add_thread(thread);
	} while((addr = (Iaddr)THR_MAP(desc.thr_next)) != top);
	// create virtual threads to watch idle lwps
	while(llist)
	{
		llive = llist;
		llist = llist->next();
		thread = new Thread(0, this, 0);
		if (!thread->grab_live((Proclive *)llive, 0, 0, 
			old, 1, 0))
		{
			delete thread;
			ret = 0;
		}
		else
			add_thread(thread);
	}
	// set asynchronous stop for LWPs
	if (!pctl->set_async())
		return 0;
	// restart any bound, suspended threads that are
	// not in startup
	for(thread = head_thread; thread; thread = thread->next())
	{
		if (thread->is_suspended() && thread->pctl &&
			!(thread->flags & L_NEW_THREAD))
			if (!thread->restart(follow_yes))
				ret = 0;
	}
	return ret;
}

// create a new live thread instance
Thread *
Process::thread_create(ProcObj *oldthread, Iaddr map_ptr)
{
	struct thread_map	desc;
	Lwplive			*tpctl = 0;	
	Thread			*thread;
	int			no_lwp = 0;
	Iaddr			start_routine;
	int			suspended;

	if (!get_thread_desc(map_ptr, desc))
	{
		printe(ERR_thread_create, E_ERROR, pobj_name);
		return 0;
	}

	start_routine = (Iaddr)desc.thr_start_addr;
	if (start_routine == 0 || !in_text(start_routine))
	{
		printe(ERR_thread_addr, E_ERROR, pobj_name);
		return 0;
	}
	
	DPRINT(DBG_THREAD, ("thread_create:thread %d, addr %#x\n",  desc.thr_tid, start_routine));

	suspended = THR_MAP_IS_SUSPENDED(&desc);

	if (desc.thr_usropts & THR_BOUND)
	{
		// thread already bound to lwp
		// we should have a virtual thread
		// watching that lwp - find it
		Thread	*vthread;
		lwpid_t	lid;

		if (get_lwp_id(&desc, &lid) == 0)
			return 0;

		DPRINT(DBG_THREAD, ("thread_create:thread %d on lwpid == %d\n", desc.thr_tid, lid));
		vthread = find_thread_id(lid);
		if (vthread)
		{
			tpctl = (Lwplive *)vthread->proc_ctl();
			if (!tpctl->stop())
			{
				printe(ERR_thread_control, E_ERROR,
					(int)desc.thr_tid, pobj_name);
				return 0;
			}
			vthread->clear_check();
			vthread->clear_pctl();
			mark_unused(vthread);
		}
		else
		{
			no_lwp = 1;
		}
	}
	next_thread++;
	thread = new Thread(next_thread, this, &desc);
	if (!thread->grab_live((Proclive *)tpctl, map_ptr, 
		start_routine, 0, 0, suspended))
	{
		if (tpctl)
		{
			thread_make_virtual((Proclive *)tpctl, 0);
		}
		delete thread;
		printe(ERR_thread_control, E_ERROR, 
				(int)desc.thr_tid, pobj_name);
		next_thread--;
		return 0;
	}
	add_thread(thread);
	if ((thr_change >= TCHANGE_ANNOUNCE) && (!tpctl || suspended))
	{
		// for bound threads that are not created in
		// the suspended state we delay the create message
		// until we receive lwp_continue
		message_manager->reset_context((ProcObj *)thread);
		printm(MSG_thread_create, oldthread->obj_name(),
			thread->obj_name());
		if (no_lwp)
		{
			printe(ERR_thread_no_lwp, E_WARNING,
				thread->obj_name());
		}
		message_manager->reset_context(0);
	}
	return thread;
}

// park an idle lwp on an virtual thread
int
Process::thread_make_virtual(Proclive *llive, int is_new)
{
	Thread	*vthread;

	if (unused_list)
	{
		vthread = unused_list;
		unused_list = unused_list->next();
		vthread->unlink();
		vthread->set_pctl(llive);
		vthread->update_state();
	}
	else
	{
		// no unused threads, create one
		vthread = new Thread(0, this, 0);
		if (!vthread->grab_live(llive, 0, 0, 0, 1, 0))
		{
			delete vthread;
			return 0;
		}
	}
	add_thread(vthread);
	// either a newly created LWP or coming from a thread
	// that is exiting or being switched out
	if (is_new)
		vthread->set_state(es_halted);
	else
	{
		vthread->set_state(es_breakpoint); 
		vthread->latestbkpt = threadpt;
	}
	if (!vthread->clear_debug())
		return 0;
	return(vthread->restart(follow_add_nostart));
}

// control a new lwp and park it on a virtual thread
int
Process::control_new_lwp(lwpid_t id)
{
	Lwplive		*llive;

	if (!pctl)
	{
		printe(ERR_internal, E_ERROR, "Process::control_new_lwp",
			__LINE__);
		return 0;
	}
	if (((llive = pctl->open_lwp(id)) == 0) || !llive->stop())
	{
		printe(ERR_lwp_control, E_ERROR, id, pobj_name);
		return 0;
	}
	DPRINT(DBG_THREAD, ("control new lwp: id %d\n", id));
	return(thread_make_virtual((Proclive *)llive, 1));
}

#endif	// DEBUG_THREADS
