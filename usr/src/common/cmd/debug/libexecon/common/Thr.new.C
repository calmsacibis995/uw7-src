#ident	"@(#)debugger:libexecon/common/Thr.new.C	1.23"

#ifdef DEBUG_THREADS

#include "ProcObj.h"
#include "Thread.h"
#include "Process.h"
#include "Procctl.h"
#include "Proccore.h"
#include "Proctypes.h"
#include "Frame.h"
#include "RegAccess.h"
#include "Interface.h"
#include "Machine.h"
#include "str.h"
#include "global.h"
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif
#include <string.h>
#include <stdio.h>
#include <thread.h>
#include <sys/types.h>

Thread::Thread(int tnum, Process *proc,
	struct thread_map *tmap) : PROCOBJ(L_THREAD)
{

	char	buf[sizeof("p.") + (2*MAX_LONG_DIGITS)];	// pn.r

	if (tmap)
		memcpy((void *)&this->map, tmap, 
			sizeof(struct thread_map));
	parent = proc;

	if (parent->is_ignore_fork())
		flags |= L_IGNORE_FORK;

	old_thread = 0;
	seglist = parent->seglist;
	ename = parent->ename;
	progname = parent->progname;

	// virtual threads use name pn.0
	sprintf(buf, "%s.%d", parent->pobj_name, tnum);
	pobj_name = str(buf);
}

Thread::~Thread()
{
	if (pctl)
	{
		((Lwplive *)pctl)->close();
		delete (Lwplive *)pctl;
	}
	else if (core)
	{
		((Lwpcore *)core)->close();
		delete (Lwpcore *)core;
	}
}

void
Thread::grab_core(Proccore *lcore, int suspended)
{
	core = lcore;
	pctl = 0;

	if (suspended)
		flags |= L_SUSPENDED;
	if (core)
		state = es_corefile;
	else
		state = es_core_off_lwp;
	regaccess.setup_core(this);
	regaccess.update();
	pc = regaccess.getreg( REG_PC );
	find_cur_src();
}

int
Thread::grab_live(Proclive *llive, Iaddr map, Iaddr start_routine, 
	Process *old, int virtual_thread, int suspended)
{
	DPRINT(DBG_THREAD, ("Thread:grab_live: this = %#x, llive = %#x, map = %#x, start_routine = %#x, old = %#x, virtual = %d, suspended = %d\n", this,llive, map, start_routine, old, virtual_thread, suspended));

	map_addr = map;
	pctl = llive;
	flags |= L_GRABBED;
	core = 0;
	goal = pg_run;
	goal2 = sg_run;
	premptyset(&this->cancel_set);
	dynpt = parent->dynpt;
	threadpt = parent->threadpt;

	if (suspended) 
		flags |= L_SUSPENDED;

	if (pctl)
	{
		state = es_halted;

		if (start_routine)
			// new bound thread
			flags |= L_NEW_THREAD;
	}
	else
		// thread not currently running
		state = es_off_lwp;

	regaccess.setup_live(this);
	regaccess.update();
	pc = regaccess.getreg( REG_PC );

#if EXCEPTION_HANDLING
	if (parent->eh_data)
		eh_data = new Exception_data(parent->eh_data, this);
#endif

	if (virtual_thread)
	{
		flags |= L_VIRTUAL;
	}
	else
	{
		// if result of a forkall, must copy
		// events from old process, not new
		if (old)
			copy_et_forkall(old);
		else
			copy_et_create();

		if (start_routine && (thr_change >= TCHANGE_STOP))
		{
			if ((startpt = 
				set_bkpt(first_stmt(start_routine),
					0, 0 )) == 0)
				return 0;
		}
		find_cur_src();
	}
	return 1;
}

#endif
