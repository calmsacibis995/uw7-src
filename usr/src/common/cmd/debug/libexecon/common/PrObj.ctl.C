#ident	"@(#)debugger:libexecon/common/PrObj.ctl.C	1.22"

#include "ProcObj.h"
#include "Breaklist.h"
#include "Interface.h"
#include "Manager.h"
#include "Procctl.h"
#include "ProcFollow.h"
#include "Proglist.h"
#include "Process.h"
#include "Instr.h"

int 
ProcObj::stop()
{
	if ( is_core() )
	{
		printe(ERR_core_cant_stop, E_ERROR, pobj_name);
		return 0;
	}
	ecount = 0;
	// fresh start - remove any breakpoints
	// used by prior step/run commands
	remove( bk_hoppt );
	remove( bk_destpt );

	if (!pctl->stop())
	{
		printe(ERR_sys_cant_stop, E_ERROR, pobj_name);
		return 0;
	}
	state = es_halted;
	return 1;
}

// There are 3 follow modes:
// 1) follow_no says do not add this object to the follower
//    and do not start the follower; this is used during program
//    startup since we wait synchronously for the object to stop
//    and in ProcObj::restart_all, which manages the follower specially.
// 2) follow_yes says add the object to the follower and start the
//    follower;  this is used in response to user requests to
//    run or step the object; the follower may already be running
//    at the time the request is made.
// 3) follow_add_nostart says add this object to the follower but
//    do not start the follower.  This is used when an object
//    is being restarted for internal processing, like multiple
//    instruction steps to accomplish a statement step.  We
//    assume these calls are reached as the result of a previous
//    signal from the follower that the process needs attention,
//    and so assume the follower is already stopped.  We set
//    the process L_CHECK_FOLLOW flag and restart the follower after
//    processing all the threads' needs (to avoid churning back
//    and forth with starting and interrupting the follower).
//    The restart call occurs at the end of the inform_processes loop.

int
ProcObj::start(Goal2 type, follower_mode mode)
{
	int	ret = 1;

	switch(type)
	{
	case sg_run:
		if ( pctl->run( latestsig, mode ) == 0 )
		{
			ret = 0;
			break;
		}
		state = es_running;
		break;
	case sg_stepbkpt:
		if ((flags & L_THREAD) && !(flags & L_IN_START))
		{
			process()->stop_all();
		}
		if ( lift_bkpt(latestbkpt) == 0 )
			return 0;
	/*FALLTHROUGH*/
	case sg_step:
		if ( pctl->step( latestsig, mode ) == 0 )
		{
			ret = 0;
			break;
		}
		state = es_stepping;
		break;
	}
	if (!ret)
	{
		int	what, why;

		if (pctl->status(what, why) == p_dead)
		{
			int	clobbered_current = 0;
			Process	*proc = process();

			destroy(0);
			if (proc->get_state() == es_dead)
			{
				message_manager->reset_context((ProcObj *)proc);
				printm(MSG_proc_exit, proc->obj_name());
				message_manager->reset_context(0);
				if (proc == proglist.current_process())
					clobbered_current = 1;
			}
			else if (flags & L_THREAD && is_user())
			{
				message_manager->reset_context(this);
				printm(MSG_thread_exit, pobj_name);
				message_manager->reset_context(0);
				if ((Thread *)this == proglist.current_thread())
					clobbered_current = 1;
			}
			if (clobbered_current)
				proglist.reset_current(1);
		}
		else
			printe(ERR_cant_restart, E_ERROR, pobj_name);
		return 0;
	}
	++epoch;
	goal2 = type;
	if (mode != follow_no)
		flags |= L_CHECK;
#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
	if (mode == follow_add_nostart)
		process()->set_check_follow();
#endif
	return 1;
}

int
ProcObj::run( int clearsig, Iaddr destaddr, int talk_ctl )
{
	if (sw_watch > 0 && !(flags & (L_IN_START|L_IGNORE_EVENTS)))
	{
		printe(ERR_step_watch, E_WARNING, pobj_name);
		return stmt_step(STEP_INTO, clearsig, destaddr, 
			STEP_INF_QUIET, talk_ctl);
	}
	goal = pg_run;
	return run_pobj(clearsig, destaddr, talk_ctl);
}

// statement step - may be result of a run request if sw_watch
// is > 0; single step to watch a location.  In this case,
// destaddr may be non-zero, if it is the result of a run -u dest.
//
int
ProcObj::stmt_step( int where, int clearsig, Iaddr destaddr, int cnt, 
	int talk_ctl )
{
	ecount = cnt;
	goal = where == STEP_INTO ? pg_stmt_step : pg_stmt_step_over;
	if (!find_stmt(startstmt, pc, start_srcfile))
		return 0;
	startaddr = pc;	// first pc in current statement
	return run_pobj(clearsig, destaddr, talk_ctl);
}

int
ProcObj::instr_step( int where, int clearsig, int cnt, int talk_ctl )
{
	ecount = cnt;
	goal = where == STEP_INTO ? pg_instr_step : pg_instr_step_over;

	return run_pobj(clearsig, 0, talk_ctl);
}

// common routine called by run, stmt_step and instr_step
int 
ProcObj::run_pobj(int clearsig, Iaddr destaddr, int talk_ctl)
{
	if ( clearsig ) 
		latestsig = 0;
	if ( (state == es_running) || (state == es_stepping) )
	{
		printe(ERR_running_proc, E_ERROR, pobj_name);
		return 0;
	}
	else if ( is_core() )
	{
		printe(ERR_core_cant_run, E_ERROR, pobj_name);
		return 0;
	}
	remove(bk_hoppt);
	remove(bk_destpt);
	if ( destaddr )
	{
		if ((destpt = set_bkpt(destaddr, 0, 0)) == 0 )
			return 0;
		if (goal == pg_run && destaddr == pc)
		{
			// set bkpt on current location; don't want to 
			// hit it immediately - by setting state to 
			// es_breakpoint we will step over it.
			state = es_breakpoint;
			latestbkpt = destpt;
		}
	}

	retaddr = 0;
	start_sp = 0;
	verbosity = talk_ctl;

	if (!restart(follow_yes))
		return 0;

	if (get_ui_type() == ui_gui)
		printm(MSG_proc_start, (unsigned long)this);
	return 1;
}

int
ProcObj::restart(follower_mode mode)
{
	DPRINT(DBG_CTL, ("restart: %#x (%s): goal = %d, state = %d\n", this, pobj_name, goal, state));
#if EXCEPTION_HANDLING
	flags &= ~(L_EH_THROWN|L_EH_CAUGHT);
#endif
	if ((goal == pg_run) && (sw_watch > 0) && 
		!(flags & (L_IN_START|L_IGNORE_EVENTS)))
	{
		// watching enabled while we were running
		// (hit bkpt marking beginning of scope)
		// single step to watch location
		Iaddr	destaddr = destpt ? destpt->addr() : 0;
		printe(ERR_step_watch, E_WARNING, pobj_name);
		return stmt_step(STEP_INTO, 1, destaddr, 
			STEP_INF_QUIET, verbosity);
	}
	if (process()->is_stop_all())
	{
		// a stop all directive is in effect; do not
		// restart yet
		if (state != es_procstop)
		{
			save_state = state;
			state = es_procstop;
		}
		DPRINT(DBG_CTL, ("restart: %#x (%s): stop_all directive in effect\n", this, pobj_name));
		return 1;
	}
	if (state == es_breakpoint || ((state == es_procstop) &&
		(save_state == es_breakpoint)))
	{
		if (((retaddr = instr.retaddr( pc )) != 0) &&
			(goal == pg_instr_step_over || goal ==
				pg_stmt_step_over))
		{
			// next instruction is a call - if we
			// are stepping over it, we need to save
			// current stack pointer; call may be
			// recursive, and we want to return to same 
			// level
			start_sp = regaccess.getreg(REG_SP);
		}
		return start(sg_stepbkpt, mode);
	}
	if ((goal == pg_run) || instr.is_bkpt( pc ) || 
		((goal2 == sg_run) && hoppt))
	{
		// There are 2 cases where we run when the
		// primary goal is to step:
		// 1) if instruction is a bkpt
		// we run instead of
		// step so the bkpt trap fires
		// otherwise we would get a trace trap
		// 2) if we were hopping and got interrupted
		// by some event and we need to keep hopping

		return start(sg_run, mode);
	}
	else
	{
		// stepping
		if (((retaddr = instr.retaddr( pc )) != 0) &&
			(goal == pg_instr_step_over || goal ==
				pg_stmt_step_over))
		{
			// next instruction is a call - if we
			// are stepping over it, we need to save
			// current stack pointer; call may be
			// recursive, and we want to return to same 
			// level
			start_sp = regaccess.getreg(REG_SP);
		}
		DPRINT(DBG_CTL, ("restart: %#x (%s) retaddr = %#x start_sp == %#x\n", this, pobj_name, retaddr, start_sp));
		return start(sg_step, mode);
	}
}

// Use hoppt to jump past function prolog or
// to step over a function.
// If state is procstop, do not restart, just setup
// hoppt.  Called by check_stmt_step and check_instr_step.
int
ProcObj::hop_to(Iaddr destaddr)
{
	DPRINT(DBG_CTL, ("hop_to %#x (%s): %#x\n", this, pobj_name, destaddr));
	if ((hoppt = set_bkpt(destaddr, 0, 0)) == 0)
	{
		if (state == es_procstop)
			state = es_stepped;
		return 0;
	}
	if (state == es_breakpoint)
		return start(sg_stepbkpt, follow_add_nostart);
	else if (pc == destaddr)
	{
		latestbkpt = hoppt;
		if (state != es_procstop)
		{
			state = es_breakpoint;
			return start(sg_stepbkpt, follow_add_nostart);
		}
		else
		{
			save_state = es_breakpoint;
			return 1;
		}
	}
	else if (state != es_procstop)
	{
		return start(sg_run, follow_add_nostart);
	}
	else
		return 1;
}

int
ProcObj::update_state()
{
	if (!regaccess.update())
		// thread might have died
		return 0;
	pc = instr.adjust_pc();
	return 1;
}

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
void
ProcObj::clear_check()
{
	flags &= ~L_CHECK;
	if (!pctl)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::clear_check",
			__LINE__);
		return;
	}
	pctl->remove_from_follower();
}
#endif

// null base class version

int
ProcObj::release_obj(int)
{
	return 0;
}

int
ProcObj::destroy(int)
{
	return 0;
}
