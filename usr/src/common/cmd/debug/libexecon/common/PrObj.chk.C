#ident	"@(#)debugger:libexecon/common/PrObj.chk.C	1.31"

#include "Breaklist.h"
#include "EventTable.h"
#include "Interface.h"
#include "Instr.h"
#include "ProcObj.h"
#include "Proctypes.h"
#include "Procctl.h"
#include "global.h"
#include "Process.h"
#include "Machine.h"
#include "Ev_Notify.h"
#include "Parser.h"
#include "Watchlist.h"
#include "Thread.h"
#include "Dyn_info.h"
#include <sys/types.h>
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif

// Some implementations use signals for indicating breakpoints
// and other tracing information. Other implementations use the
// fault tracing facilities of /proc.  The ProcObj class
// attempts to allow either approach through the macros
// STOP_TYPE and LATEST_STOP
//

int
ProcObj::inform( int what, int why )
{
	regaccess.update();
	pc = regaccess.getreg( REG_PC );

	DPRINT(DBG_CTL, ("inform: %#x (%s) what = %d, why = %d, pc = %#x\n", this, pobj_name, what, why, pc));

	if (flags & L_IN_START)
	{
		return inform_startup(what, why);
	}
	if ( (why == STOP_REQUESTED))
	{
		// thread stopped at user request - halt command
		state = es_halted;
		CLEARSIG();
		return respond_to_sus(1, 1, state);
	}
#ifdef HAS_SYSCALL_TRACING
	else if ( why == STOP_SYSENTRY )
	{
		state = es_syscallent;
		latesttsc = what;
		// executed system call while stepping over bkpt
		if ((goal2 == sg_stepbkpt) && process()->is_stop_all())
		{
			if (!inform_stepbkpt())
				return 0;
		}
		if (!(flags & L_IGNORE_EVENTS))
			return respond_to_tsc();
		else
			return restart(follow_add_nostart);
	}
	else if (why == STOP_SYSEXIT)
	{
		state = es_syscallxit;
		latesttsc = what;
		// executed system call while stepping over bkpt
		if ((goal2 == sg_stepbkpt) && process()->is_stop_all())
		{
			if (!inform_stepbkpt())
				return 0;
		}
		if (!(flags & L_IGNORE_EVENTS))
			return respond_to_tsc();
		else
			return restart(follow_add_nostart);
	}
#endif
#if STOP_TYPE == STOP_FAULTED // use faults for tracing
	else if ( why == STOP_FAULTED )
	{
		latestflt = what;
		switch(goal2)
		{
		case sg_run:
			return inform_run();
		case sg_step:
			return inform_step();
		case sg_stepbkpt:
			return inform_stepbkpt();
		default:
			printe(ERR_internal, E_ERROR, 
				"ProcObj::inform", __LINE__);
			return 0;
		}
	}
	else if ( why == STOP_SIGNALLED )
	{
		latestsig = what;
		state = es_signalled;
		// received signal while stepping over bkpt
		if ((goal2 == sg_stepbkpt) && process()->is_stop_all())
		{
			if (!inform_stepbkpt())
				return 0;
		}
		if (!(flags & L_IGNORE_EVENTS))
			return respond_to_sig();
		else
			return restart(follow_add_nostart);
	}
#else	// use signals for tracing
	else if ( why == STOP_SIGNALLED )
	{
		latestsig = what;
		if (what == STOP_TRACE)
		{
			switch(goal2)
			{
			case sg_run:
				return inform_run();
			case sg_step:
				return inform_step();
			case sg_stepbkpt:
				return inform_stepbkpt();
			default:
				printe(ERR_internal, E_ERROR, 
					"ProcObj::inform", __LINE__);
				return 0;
			}
		}
		else
		{
			state = es_signalled;
			// received signal while stepping over bkpt
			if ((goal2 == sg_stepbkpt) && 
				process()->is_stop_all())
			{
				if (!inform_stepbkpt())
					return 0;
			}
			if (!(flags & L_IGNORE_EVENTS))
				return respond_to_sig();
			else
				return restart(follow_add_nostart);
		}
	}
#endif
	else 
	{
		printe(ERR_internal, E_ERROR, "ProcObj::inform",
			__LINE__);
		return 0;
	}
}

int
ProcObj::inform_startup(int what, int why)
{
	// get here from create or setup_new_process, which
	// wait synchronously for the
	// process to stop - follower not needed
	//
	// if a breakpoint was executed, pc might need adjustment.
	// ( for example, 386 leaves the pc one instruction 
	// past the bkpt. )
	//
	pc = instr.adjust_pc();
	state = es_halted;

	if (why == STOP_TYPE)
	{
		if (goal2 == sg_run && what == STOP_BKPT)
		{
			if ( destpt && pc == destpt->addr() )
			{
				// have hit start location
				remove( bk_destpt );
				return 1;
			}
			else if ( dynpt && pc == dynpt->addr() )
			{
				// in dynamic linker - state of process changing
				state = es_breakpoint;
				latestbkpt = dynpt;
				return respond_to_dynpt(follow_no);
			}
#ifdef DEBUG_THREADS
			else if ( threadpt && pc == threadpt->addr() )
			{
				// threads library state change
				// during startup is an error - we
				// can't handle it correctly at this
				// point
				state = es_breakpoint;
				latestbkpt = threadpt;
				printe(ERR_thread_change_startup,
					E_ERROR, pobj_name);
				return 0;
			}
#endif
#if EXCEPTION_HANDLING
			else if ( eh_data && eh_data->eh_bkpt
					&& pc == eh_data->eh_bkpt->addr() )
			{
				// exception thrown or caught during startup
				// is an error
				state = es_breakpoint;
				latestbkpt = eh_data->eh_bkpt;
				printe(ERR_eh_change_startup, E_ERROR, pobj_name);
				return 0;
			}
#endif // EXCEPTION_HANDLING
			else if ( (latestbkpt = 
				process()->etable->breaklist.lookup(pc))
					!= 0 )
			{
				// user breakpoint during startup
				// ignore it
				state = es_breakpoint;
				return restart(follow_no);
			}
#ifndef HAS_SYSCALL_TRACING
			else
			{
				state = es_stepped;
				latestsig = 0;
				return restart(follow_no);
			}
#endif
		}
		else if (what == STOP_TRACE && 
			goal2 == sg_stepbkpt && goal == pg_run)
		{
			// stepped over dynpt, threadpt, or ehpt
			insert_bkpt( latestbkpt );
			state = es_stepped;
			return restart(follow_no);
		}
	}
	state = es_signalled;
	latestsig = what;
	return restart(follow_no);
}

int
ProcObj::inform_run()
{
	int watch_fired = 0;
	if ( LATEST_STOP != STOP_BKPT )
	{
		state = es_signalled;
		return restart(follow_add_nostart);
	}
	CLEARSIG();
	// if stopped at a breakpoint
	// pc might be pointing at the next instruction. adjust it.
	pc = instr.adjust_pc();
	LATEST_STOP = 0;
	if (hoppt && pc == hoppt->addr())
	{
		// get here if doing instruction step over or
		// stmt stepping through code we don't have
		// source for - destination of a hop
		// also for machines which use bkpts to do single
		// stepping
		DPRINT(DBG_CTL, ("inform_run: %#x (%s) hoppt\n", this, pobj_name));
		latestbkpt = remove(bk_hoppt);
		if (latestbkpt)
		{
			if (retaddr == pc)
				retaddr = 0;
			state = es_breakpoint;
			if (!(flags & L_IGNORE_EVENTS))
				if (respond_to_bkpt())
					return 1;
		}
		else
		{
			state = es_stepped;
			if (retaddr == pc)
			{
				if (start_sp && !COMPARE_SP(start_sp,
					regaccess.getreg(REG_SP)))
					// reached hoppt but we are
					// in a later stack frame
					// than when we started - 
					// recursive call, keep going
				{
					if (prismember(&interrupt, 
						SIGINT))
					{
						// interrupted while 
						// stepping
						ecount = 0;
						return respond_to_sus(1,
							0, es_stepped);
					}
					return hop_to(retaddr);
				}
				retaddr = 0;
			}
		}
		switch ( goal )
		{
			case pg_stmt_step:
			case pg_stmt_step_over:
				return check_stmt_step();
			case pg_instr_step_over:
			case pg_instr_step:
				return check_instr_step();
			case pg_run:
			default:
				printe(ERR_internal, E_ERROR, 
					"ProcObj::inform_run", __LINE__);
				return 0;
		}
	}
	else if ( destpt && pc == destpt->addr() )
	{
		// goal of run to address
		DPRINT(DBG_CTL, ("inform_run: %#x (%s) destpt\n", this, pobj_name));
		latestbkpt = remove(bk_destpt);
		if (latestbkpt)
		{
			state = es_breakpoint;
			if (!(flags & L_IGNORE_EVENTS))
				if (respond_to_bkpt())
					return 1;
		}
		else
		{
			if (goal == pg_run)
				state = es_halted;
			else
				state = es_stepped;
		}
		ecount = 0;
		return respond_to_sus(1, 1, 
			goal == pg_run ? es_halted : es_stepped);
	}
	else if ( dynpt && pc == dynpt->addr() )
	{
		// in dynamic linker - state of process changing
		state = es_breakpoint;
		latestbkpt = dynpt;
		return respond_to_dynpt(follow_add_nostart);
	}
#ifdef DEBUG_THREADS
	else if ( threadpt && pc == threadpt->addr() )
	{
		if (!(flags & L_THREAD))
			return restart(follow_add_nostart);
		// threads library state change
		state = es_breakpoint;
		latestbkpt = threadpt;
		return ((Thread *)this)->respond_to_threadpt(follow_add_nostart);
	}
	else if ( startpt && pc == startpt->addr() )
	{
		// at start routine for thread - handle this like
		// a run -u; use destpt and show state as halted
		latestbkpt = remove(bk_startpt);
		if (latestbkpt)
		{
			state = es_breakpoint;
			if (!(flags & L_IGNORE_EVENTS))
				if (respond_to_bkpt())
					return 1;
		}
		else
			state = es_halted;
		return respond_to_sus(1, 1, es_halted_thread_start);
	}
#endif
#if EXCEPTION_HANDLING
	else if (eh_data && eh_data->eh_bkpt && pc == eh_data->eh_bkpt->addr())
	{
		// exception thrown or caught
		state = es_breakpoint;
		latestbkpt = eh_data->eh_bkpt;
		return respond_to_ehpt(follow_add_nostart);
	}
#endif // EXCEPTION_HANDLING
#ifndef HAS_SYSCALL_TRACING
	else if ( execvept && pc == execvept->addr() )
	{
		// process is about to call exec
		state = es_breakpoint;
		latestbkpt = execvept;
		return respond_to_execvept();
	}
	else if (forkpt && pc == forkpt->addr()) 
	{
		state = es_breakpoint;
		latestbkpt = forkpt;
		return respond_to_forkpt(bk_forkpt);
	}
	else if (vforkpt && pc == vforkpt->addr()) 
	{
		state = es_breakpoint;
		latestbkpt = vforkpt;
		return respond_to_forkpt(bk_vforkpt);
	}
	else if (fork1pt && pc == fork1pt->addr()) 
	{
		state = es_breakpoint;
		latestbkpt = fork1pt;
		return respond_to_forkpt(bk_fork1pt);
	}
	else if (forkallpt && pc == forkallpt->addr()) 
	{
		state = es_breakpoint;
		latestbkpt = forkallpt;
		return respond_to_forkpt(bk_forkallpt);
	}
#endif
	else if ( hw_watch && hw_watch->hw_fired(this))
	{
		DPRINT(DBG_CTL, ("inform_run: %#x (%s) hw watch fired\n", this, pobj_name ));
		watch_fired = 1;
		state = es_halted;
		if (check_watchpoints())
		{
			// check_wartchpoints prints location
			check_onstop();
			return 1;
		}
		// FALLTHROUGH - might be a breakpoint even
		// if watchpoint fired
	}
	if ( (latestbkpt = process()->etable->breaklist.lookup(pc)) != 0 )
	{
		state = es_breakpoint;
		if (!(flags & L_IGNORE_EVENTS))
		{
			if (respond_to_bkpt())
				return 1;
			else
			{
				if (!check_watchpoints())
					return restart(follow_add_nostart);
				check_onstop();
				return 1;
			}
		}
		else
			return restart(follow_add_nostart);
	}
	else
	{
		// Either 
		// 1. hardware watchpoint fired but the event
		// 	did not trigger or
		// 2. hit breakpoint but it has already been removed
		// 	for some other thread - force adjustment of
		// 	pc if necessary on this architecture
		DPRINT(DBG_CTL, ("inform_run %#x (%s): no bkpt\n", this, pobj_name));
		if (!watch_fired)
			pc = instr.adjust_pc(1);
		return restart(follow_add_nostart);
	}
}

int
ProcObj::inform_step()
{
	if ( LATEST_STOP != STOP_TRACE )
	{
		LATEST_STOP = 0;
		return restart(follow_add_nostart);
	}
	CLEARSIG();
	state = es_stepped;
	LATEST_STOP = 0;
	switch ( goal )
	{
		case pg_stmt_step:
		case pg_stmt_step_over:
			return check_stmt_step();
		case pg_instr_step:
		case pg_instr_step_over:
			return check_instr_step();
		case pg_run:
		default:
			printe(ERR_internal, E_ERROR, 
				"ProcObj::inform_step", __LINE__);
			return 0;
	}
}

int
ProcObj::inform_stepbkpt()
{
	// Insert breakpoint after we lifted it to step through.
	int	ret = 1;

	DPRINT(DBG_CTL, ("inform_stepbkpt %#x (%s)\n", this, pobj_name));
	if (!latestbkpt->is_inserted())
		insert_bkpt( latestbkpt );

	if ( LATEST_STOP != STOP_TRACE )
	{
		if (flags & L_THREAD)
		{
			if ( LATEST_STOP != 0 )
			{
				state = es_procstop;
				save_state = es_invalid;
			}
			return process()->restart_all();
		}
		else
		{
			if (LATEST_STOP == 0)
				// hit signal or system call while
				// stepping over
				return 1;
			else return restart(follow_add_nostart);
		}
	}
	LATEST_STOP = 0;
	state = (flags & L_THREAD) ? es_procstop: es_stepped;
	save_state = es_invalid;
	switch ( goal )
	{
		case pg_run:
			if ( hw_watch && hw_watch->hw_fired(this))
			{
				if (check_watchpoints())
				{
					state = es_halted;
					check_onstop();
					return 1;
				}
			}
			if (!(flags & L_THREAD))
				return restart(follow_add_nostart);
			break;
		case pg_stmt_step:
		case pg_stmt_step_over:
			ret = check_stmt_step();
			break;
		case pg_instr_step:
		case pg_instr_step_over:
			ret = check_instr_step();
			break;
		default:
			printe(ERR_internal, E_ERROR, 
				"ProcObj::inform_stepbkpt", __LINE__);
			return 0;
	}
	if (flags & L_THREAD)
	{
		if (!process()->restart_all())
			return 0;
	}
	return ret;
}

// In processing single steps we must be careful in setting
// state and restarting process/thread.  If the state is
// es_procstop when we enter the routine, we are stopped
// after stepping over a breakpoint.  The Process::restart_all()
// routine will restart us, so we do not want to restart here.
// If we are not restarting and state is es_procstop
// we set state to es_stepped.
// If state is not es_procstop on entry, it can be either
// es_stepped, for a normal step, or es_breakpoint, if we
// hit the hoppt breakpoint.  We want to
// preserve the state, so subsequent runs or steps know to
// lift the breakpoint before continuing and to replant it.

int
ProcObj::check_stmt_step()
{
	const FileEntry	*fentry;
	if ((destpt && (pc == destpt->addr())) ||
		prismember(&interrupt, SIGINT))
	{
		// interrupted while stepping
		// or single stepping to watch a watchpt
		// as the result of a run -u dest
		DPRINT(DBG_CTL, ("check_stmt_step: %#x (%s) destpt\n", this, pobj_name));
		if (destpt && (pc == destpt->addr()))
		{
			if ((latestbkpt = remove(bk_destpt)) != 0)
				state = es_breakpoint;
		}
		if (state == es_procstop)
			state = es_stepped;
		ecount = 0;
		return respond_to_sus(1, 1, es_stepped);
	}
	else if (hoppt && goal2 == sg_stepbkpt)
	{
		// were hopping, but had to step over
		// breakpoint first.
		DPRINT(DBG_CTL, ("check_stmt_step: %#x (%s) hoppt\n", this, pobj_name));
		if (state != es_procstop)
			return start(sg_run, follow_add_nostart);
		return 1;
	}
	else if ((goal == pg_stmt_step_over) && (retaddr != 0))
	{
		// step over function call to return address
		// we have executed the call and are at the
		// target function
		return hop_to(retaddr);
	}
	else if ( find_stmt(currstmt, pc, fentry) == 0 )
	{
		// error determing statement
		if (state == es_procstop)
			state = es_stepped;
		return 0;
	}
	else if ( currstmt.is_unknown() )
	{
		// no source for current pc
		if (retaddr != 0 ) 
		{
			// If we got here through a call instruction,
			// retaddr is non-0; if we have landed
			// in a procedure-linkage table entry,
			// just keep stepping (we call start, not
			// restart, so retaddr will not be reset).
			// We might step into a function with
			// line information.
			// If not in the plt, just run until the
			// retaddr is hit
			Dyn_info	*dyn;

			if ((dyn = get_dyn_info(pc)) != 0)
			{
				if ((pc >= dyn->pltaddr) &&
					(pc < (dyn->pltaddr + 
						dyn->pltsize)))
					return start(sg_step, 
						follow_add_nostart);
			}
			return hop_to(retaddr);
		}
		else 
		{
			// one instruction at a time
			DPRINT(DBG_CTL, ("check_stmt_step: %#x (%s) no current, no retaddr\n", this, pobj_name));
			if (state != es_procstop)
				return restart(follow_add_nostart);
			else
				return 1;
		}
	}
	else if (((fentry == start_srcfile) && ( currstmt == startstmt) && (pc != startaddr)) 
		|| !currstmt.at_first_instr())
	{
		// Either we are still within original statement
		// or we have crossed into a new statement 
		// but pc is not at beginning of statement - this
		// pc is not really connected with a statement.
		// If pc == startaddr, we may have come around
		// to beginning of statement again in a loop
		DPRINT(DBG_CTL, ("check_stmt_step: %#x (%s) current == start\n", this, pobj_name));
		if (verbosity >= V_HIGH)
			show_current_location( 0, es_stepped );
		if (state != es_procstop)
			return restart(follow_add_nostart);
		else
			return 1;
	}
	// we are at a new source statement
	// if new stmt is beginning of a function,
	// skip past function prolog
	// (unless we are stepping to a specific statement
	// or stepping infinitely to watch watchpoints)
	if (!destpt && (ecount != STEP_INF_QUIET))
	{
		Iaddr	newpc;
		if ((newpc = first_stmt(pc)) != pc)
		{
			DPRINT(DBG_CTL, ("check_stmt_step: %#x (%s) skip prolog\n", this, pobj_name));
			return hop_to(newpc);
		}
	}
	if (check_watchpoints())
	{
		// watchpoint fired while stepping
		if (state == es_procstop)
			state = es_stepped;
		if (destpt)
			remove(bk_destpt);
		check_onstop();
		return 1;
	}
	else if (destpt)
	{
		// Single stepping for software watch as
		// the result of a run -u.  Watchpoint has not
		// fired.  Keep going.

		if (verbosity >= V_HIGH)
			show_current_location( 1, es_stepped );
		if (!find_stmt(startstmt, pc, start_srcfile))
		{
			if (state == es_procstop)
				state = es_stepped;
			return 0;
		}
		startaddr = pc;
		if (state != es_procstop)
			return restart(follow_add_nostart);
		else
			return 1;
	}
	else if (( ecount < 0 ) || ( --ecount > 0))
	{
		// ecount > 0 is number of steps
		// ecount < 0 means step forever
		if (ecount != STEP_INF_QUIET)
		{
			if (state == es_procstop)
				state = es_stepped;
			show_current_location( 1, es_stepped );
		}
		if (!find_stmt(startstmt, pc, start_srcfile))
		{
			if (state == es_procstop)
				state = es_stepped;
			return 0;
		}
		startaddr = pc;
		if (state != es_procstop)
			return restart(follow_add_nostart);
		else
			return 1;
	}
	else
	{
		// completed single statement step
		find_cur_src();
		check_onstop();
		if (state == es_procstop)
			state = es_stepped;
		return show_current_location(1, es_stepped);
	}
}

int
ProcObj::check_instr_step()
{
	if (prismember(&interrupt, SIGINT))
	{
		// interrupted while stepping
		if (state == es_procstop)
			state = es_stepped;
		ecount = 0;
		return respond_to_sus(1, 0, es_stepped);
	}
	else if (hoppt && goal2 == sg_stepbkpt)
	{
		// were hopping, but had to step over
		// breakpoint first.
		if (state != es_procstop)
			return start(sg_run, follow_add_nostart);
		return 1;
	}
	else if ( (goal == pg_instr_step_over) && (retaddr != 0))
	{
		// step over function call
		return hop_to(retaddr);
	}
	else if ( check_watchpoints() )
	{
		// watchpoint fired while stepping
		if (state == es_procstop)
			state = es_stepped;
		remove( bk_destpt );
		check_onstop();
		return 1;
	}
	else if (( ecount < 0 ) || ( --ecount > 0))
	{
		// ecount > 0 is number of steps
		// ecount < 0 means step forever
		if (ecount != STEP_INF_QUIET)
		{
			show_current_location(0, es_stepped);
		}
		if (state != es_procstop)
			return restart(follow_add_nostart);
		else
			return 1;
	}
	else
	{
		// single instruction step
		if (state == es_procstop)
			state = es_stepped;
		find_cur_src();
		check_onstop();
		return show_current_location( 0, es_stepped );
	}
}

// add onstop commands to cmdlist
void 
ProcObj::check_onstop()
{
	NotifyEvent	*ne;

	if (flags & L_IGNORE_EVENTS)
		return;

	EventTable	*tab = events();

	if (!tab)
		return;

	for (ne = tab->onstoplist; ne; ne = ne->next())
	{
		(*ne->func)(ne->thisptr);
	}
}

// returns 1 if the state of the process/thread is acceptable
// for the mode given; else prints a message and returns 0
int
ProcObj::state_check(int mode)
{
	switch(state)
	{
	case es_invalid:
	case es_dead:
		if (mode & E_DEAD)
		{
			printe(ERR_invalid_op_dead, E_ERROR, 
				pobj_name);
			return 0;
		}
		break;
	case es_corefile:
	case es_core_off_lwp:
		if (mode & E_CORE)
		{
			printe(ERR_invalid_op_core, E_ERROR, 
				pobj_name);
			return 0;
		}
		break;
	case es_stepping:
	case es_running:
		if (flags & L_SUSPENDED)
		{
			if (mode & E_SUSPENDED)
			{
				printe(ERR_invalid_op_suspended, E_ERROR,
					pobj_name);
				return 0;
			}
		}
		else
		{
			if (mode & E_RUNNING)
			{
				printe(ERR_invalid_op_running, E_ERROR, 
					pobj_name);
				return 0;
			}
		}
		break;
	case es_off_lwp:
		if (flags & L_SUSPENDED)
		{
			if (mode & E_SUSPENDED)
			{
				printe(ERR_invalid_op_suspended, E_ERROR,
					pobj_name);
				return 0;
			}
		}
		else
		{
			if (mode & E_OFF_LWP)
			{
				printe(ERR_invalid_op_off_lwp, E_ERROR,
					pobj_name);
				return 0;
			}
		}
		break;
	case es_halted:
		if ((flags & L_SUSPENDED) && (mode & E_SUSPENDED))
		{
			printe(ERR_invalid_op_suspended, E_ERROR,
					pobj_name);
				return 0;
		}
		break;
	default:
		break;
	}
	return 1;
}
