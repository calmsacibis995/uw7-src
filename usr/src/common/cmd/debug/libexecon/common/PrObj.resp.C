#ident	"@(#)debugger:libexecon/common/PrObj.resp.C	1.25"

#include "Event.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Thread.h"
#include "EventTable.h"
#include "Interface.h"
#include "Proctypes.h"
#include "Procctl.h"
#include "Instr.h"
#include "Seglist.h"
#include "Machine.h"
#include "List.h"
#include "global.h"
#include <sys/types.h>


int
ProcObj::respond_to_sus(int show, int showsrc, Execstate showstate)
{
	check_watchpoints();
	find_cur_src();
	check_onstop();
	if (show)
		return show_current_location(showsrc, showstate);
	return 1;
}

// We could be stopped because of an event in place for this
// signal or just because the disposition for the debugger
// for this signal is "catch."
int
ProcObj::respond_to_sig()
{
	NotifyEvent	*ne;
	int		found = 0;
	int		show = 0;
	EventTable	*tab = events();

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::respond_to_sig", __LINE__);
		return 0;
	}

	ne = tab->siglist.events(latestsig); 
	if (ne)
	{
		for (; ne; ne = ne->next())
		{
			int	i;
			if (ne->object != this)
				continue;
			if ((i = (*ne->func)(ne->thisptr)) 
				== NO_TRIGGER)
				continue;
			found++;
			if (i == TRIGGER_VERBOSE)
				show = 1;
		}
	}
	else
	{
		// no events - check signal disposition
		if (prismember(&sigset, latestsig))
		{
			found++;
			show = 1;
		}
	}
	if (found)
	{
		return respond_to_sus(show, 1, state);
	}
	else
	{
		if (!check_watchpoints())
			return restart(follow_add_nostart);
		check_onstop();
	}
	return 1;
}

// Certain system calls require special treatment (fork, exec,
// lwp create).  We must still check for events for those calls,
// however.

#ifdef HAS_SYSCALL_TRACING
int
ProcObj::respond_to_tsc()
{
	int		found = 0;
	NotifyEvent	*ne;
	int		show = 0;
	int		ret_val = 1;
	EventTable	*tab = events();

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::respond_to_tsc", __LINE__);
		return 0;
	}

	ne = tab->tsclist.events(latesttsc, (state == es_syscallxit)
		? TSC_exit : TSC_entry);
	if (state == es_syscallxit)
	{
		switch(latesttsc)
		{
		case SYSCALL_fork:
		case SYSCALL_vfork:
#ifdef SYSCALL_fork1
		case SYSCALL_fork1:
		case SYSCALL_forkall:
#endif
			if (FORK_FAILED())
			{
				if (!ne)
					return(restart(follow_add_nostart));
			}
			else
			{
				pid_t npid = SYS_RETURN_VAL();
				if (flags & L_IGNORE_FORK)
				{
					// process forked but we are 
					// ignoring children
					ret_val = release_child(npid);
					if (!ne)
						return(restart(follow_add_nostart));
				}
				else
				{
					// control new process in debugger
					ret_val = process()->control_new_proc(npid, this, latesttsc);
					if (!ne)
					{
						if (ret_val)
						{
							check_watchpoints();
							find_cur_src();
						}
						return ret_val;
					}
				}
			}
			break;
		case SYSCALL_exec:
		case SYSCALL_execve:
			if (EXEC_FAILED())
			{
				if (!ne)
					return(restart(follow_add_nostart));
			}
			else
			{
				ret_val = process()->control_new_prog(0);
				if (!ne)
					return ret_val;
			}
			break;
#ifdef DEBUG_THREADS
		case SYSCALL_lwpcreate:
			if (!LWP_CREATE_FAILED() && (flags & L_THREAD))

			{
				lwpid_t	id;
				id = SYS_RETURN_VAL();
				ret_val = process()->control_new_lwp(id);
			}
			if (!ne)
				return(restart(follow_add_nostart));
			break;
#endif
		default:
			break;
		}
	}
	for (; ne; ne = ne->next())
	{
		int	i;

		if (ne->object != this)
			continue;
		if ((i = (*ne->func)(ne->thisptr)) == NO_TRIGGER)
			continue;
		found++;
		if (i == TRIGGER_VERBOSE)
			show = 1;
	}
	if (found)
	{
		if (!respond_to_sus(show, 1, state))
			return 0;
	}
	else
	{
		if (!check_watchpoints())
		{
			if (!restart(follow_add_nostart))
				return 0;
		}
		else
			check_onstop();
	}
	return ret_val;
}
#endif

int
ProcObj::respond_to_bkpt()
{
	NotifyEvent	*ne, *ne2;
	int		found = 0;
	int		show = 0;

	ne = latestbkpt->events(); 
	// must be careful since event code may delete
	// from middle of list
	while(ne)
	{
		int	i;

		ne2 = ne->next();

		if (ne->object != this || !ne->func)
		// special breakpoints like destpt and hoppt
		// don't have notifier functions
		{
			ne = ne2;
			continue;
		}
		i = (*ne->func)(ne->thisptr);
		switch(i)
		{
		case NO_TRIGGER:
			break;
		case TRIGGER_VERBOSE:
			show = 1;
			/* FALLTHROUGH */
		case TRIGGER_QUIET:
			found++;
			break;
		case TRIGGER_FOREIGN:
			// event triggered but context for
			// event was not this ProcObj; we keep
			// going
			if (flags & L_WAITING)
			{
				flags &= ~L_WAITING;
				waitlist.remove(this);
			}
			break;
		}
		ne = ne2;
	}
	if (found)
	{
		retaddr = 0;	// we can reset retaddr on next 
				// start - if we are ignoring
				// this bkpt, we don't want
				// to reset in case we were
				// stepping over this func
		find_cur_src();
		check_onstop();
		if (show)
			show_current_location( 1 );
		check_watchpoints();
		return 1;
	}
	return 0;
}

// Stop due to event triggered by a foreign process
int
ProcObj::stop_for_event(int mode)
{
	if (!stop())
		return 0;
	
	state = es_watchpoint;
	find_cur_src();

	check_onstop();
	if (mode == TRIGGER_VERBOSE)
		show_current_location( 1 );
	check_watchpoints();
	return 1;
}

int
ProcObj::check_watchpoints()
{
	if (flags & L_IGNORE_EVENTS)
		return 0;

	NotifyEvent	*ne;
	int		show = 0;
	int		found = 0;
	EventTable	*tab = events();

	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::check_watchpoints",
			__LINE__);
		return 0;
	}

	ne = tab->watchlist;
	for (; ne; ne = ne->next())
	{
		int	i;

		if (ne->object != this)
			continue;
		i = (*ne->func)(ne->thisptr);
		switch(i)
		{
			default:
				break;
			case NO_TRIGGER:
				break;
			case TRIGGER_VERBOSE:
				show = 1;
				/* FALLTHROUGH */
			case TRIGGER_QUIET:
				found++;
				break;
			case TRIGGER_FOREIGN:
				if (flags & L_WAITING)
				{
					flags &= ~L_WAITING;
					waitlist.remove(this);
				}
				break;
		}
	}
	if (found)
	{

		find_cur_src();
		if (show)
			show_current_location( 1, es_watchpoint );
		return 1;
	}
	return 0;
}

// For each call to _rtld, we hit the dynpt breakpoint
// twice: the first time the library map is marked as inconsistent.
// We wait for the second time to change our internal library map.
int
ProcObj::respond_to_dynpt(follower_mode mode)
{
	Iaddr	raddr;
	Process *proc = process();

	pc = instr.adjust_pc();
	switch (seglist->rtld_state(proc->pctl))
	{
	case rtld_buildable:
		seglist->build( proc->pctl, ename );
		break;

	case rtld_symbols_loaded:
		if (flags & L_IN_START)
			return 1;
		break;

	case rtld_init_complete:
#ifdef DEBUG_THREADS
		if (proc->thr_brk_addr && !(flags & L_IN_START)
			&& obj_type() == pobj_process)
		{
			if (!proc->thread_setup_live(0))
				printe(ERR_thread_setup, E_WARNING, pobj_name);
			else
			{
				Thread *thr = proc->first_thread(1);
				printm(MSG_new_thread, (int)thr->thread_id(), 
					thr->obj_name());
				thr->check_watchpoints();
				thr->find_cur_src();
				proglist.set_current(thr, 1);
				return 1;
			}
		}
#endif
		break;

	case rtld_is_alternate:
		if ((raddr = seglist->alternate_rtld(proc->pctl)) != 0)
		{
			// alternate rtld mapped; setup new dynpt
			latestbkpt = remove(bk_dynpt);
			dynpt = set_bkpt( raddr, 0, 0 );
			state = es_halted;
		}
		break;
	}
	return restart(mode);
}

#ifndef HAS_SYSCALL_TRACING
// process is about to exec - extrace pathname of file to
// be execed from process stack and let process go;
// wait for stop and determine whether exec succeeded
int
ProcObj::respond_to_execvept()
{
	Iaddr	eaddr = execvept->addr();
	Iaddr	lreturn;
	Frame	*frame;
	Iaddr	path_addr;
	char	path[PATH_MAX+1];
	int	len;
	char	*ptr;


	// depends on path being first argument to execve
	frame = topframe();
	path_addr = frame->quick_argword(0);
	// read path
	len = 0;
	ptr = path;
	while(len <= PATH_MAX)
	{
		int	ret;
		if ((ret = pctl->read(path_addr, ptr, 100)) < 0)
		{
			printe(ERR_proc_read, E_ERROR, pobj_name, 
				path_addr);
			return 0;
		}
		if (memchr(ptr, 0, ret) != 0)
			break;
		len += ret;
		path_addr += ret;
		ptr += ret;
	}

	if (len > PATH_MAX)
		return 0;

	latestbkpt = remove(bk_execvept);
	state = es_halted;

	if ((lreturn = instr.find_lreturn(pc)) == 0)
		return 0;
	
	remove(bk_hoppt);
	hoppt = set_bkpt(lreturn, 0, 0);

	while((pc >= eaddr) && (pc < lreturn))
	{
		int	what, why;
		if (!restart(follow_no) ||
			(pctl->wait_for(what, why) != p_stopped))
		{
			latestbkpt = remove(bk_hoppt);
			state = es_halted;
			return 0;
		}
		regaccess.update();
		pc = instr.adjust_pc();
	}


	DPRINT(DBG_CTL,("Process::respond_to_execvept: pc %#x\n", pc));
	if (pc == lreturn) 
	{
		// exec failed
		remove(bk_hoppt);
		execvept = set_bkpt(eaddr, 0, 0);
		return(restart(follow_add_nostart));
	}
	else
	{
		return(process()->control_new_prog(path));
	}
}

// process is about to fork
// find address of instruction after lcall and run there;
// then wait for new process to fire up
int 
ProcObj::respond_to_forkpt(Bkpt_type btype)
{
	Iaddr	lreturn;
	Iaddr	forkaddr;

	pc = instr.adjust_pc();
	forkaddr = pc;
	
	latestbkpt = remove(btype);
	state = es_halted;

	if ((lreturn = instr.find_lreturn(pc)) == 0)
		return 0;

	remove(bk_hoppt);
	hoppt = set_bkpt(lreturn, 0, 0);
	while(pc != lreturn)
	{
		int	what, why;
		if (!start(sg_run, follow_no) ||
			(pctl->wait_for(what, why) != p_stopped))
		{
			latestbkpt = remove(bk_hoppt);
			state = es_halted;
			return 0;
		}
		regaccess.update();
		pc = instr.adjust_pc();
	}
	state = es_breakpoint;
	switch(btype)
	{
	case bk_forkpt:
		latesttsc = SYSCALL_fork;
		forkpt = set_bkpt(forkaddr, 0, 0);
		break;
	case bk_vforkpt:
		latesttsc = SYSCALL_vfork;
		vforkpt = set_bkpt(forkaddr, 0, 0);
		break;
	case bk_fork1pt:
		latesttsc = SYSCALL_fork1;
		fork1pt = set_bkpt(forkaddr, 0, 0);
		break;
	case bk_forkallpt:
		latesttsc = SYSCALL_forkall;
		forkallpt = set_bkpt(forkaddr, 0, 0);
		break;
	}
	if (FORK_FAILED())
	{
		latestbkpt = remove(bk_hoppt);
		state = es_halted;
		return(restart(follow_add_nostart));
	}
	else
	{
		int	ret_val;
		pid_t npid = SYS_RETURN_VAL();

		ret_val = process()->control_new_proc(npid,
			this, latesttsc);

		// remove only after setting up new process
		// so breakpoint gets lifted correctly in new
		// process
		latestbkpt = remove(bk_hoppt);
		state = es_halted;
		if (ret_val != 0)
		{
			check_watchpoints();
			find_cur_src();
		}
		return(ret_val);
	}
}
#endif
