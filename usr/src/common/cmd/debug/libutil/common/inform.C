#ident	"@(#)debugger:libutil/common/inform.C	1.23"

#include "Manager.h"
#include "utility.h"
#include "global.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Procctl.h"
#include "Proctypes.h"
#include "List.h"
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>

List		waitlist;

// Walk the list of live processes and process any that need attention,
// i.e. that have stopped since we last dealt with them
// This is the signal handler for the signals coming from the
// follower processes or threads.
//
// The signal mask for the handler is to block the signal that
// got us here and SIGINT.  We unblock SIGPOLL for the duration
// of the handler to allow I/O to pseudo-ttys to come through.

int
inform_processes(int sig, void *, void *)
{
	plist		*list;
	plist		*list_head;
	int		clobberedcurrent = 0;

	// do the usual interrupt processing
	if (sig) 
	{
		if (sig != -1)
			praddset(&interrupt, sig);
		
		DPRINT(DBG_FOLLOW, ("inform_processes: received signal %d\n", sig));
	}

	PrintaxSpeakCount = 0;
	list_head = list = proglist.all_live(0);
	for (ProcObj *pobj = list++->p_pobj; pobj; pobj = list++->p_pobj)
	{
		int		 what, why;
		Procstat	pstat;

		pstat = p_unknown;
		message_manager->reset_context(pobj);
		if (!pobj->check())
		{
			DPRINT(DBG_FOLLOW, ("inform_processes:%#x (%s) no check needed\n", pobj, pobj->obj_name()));
			continue;
		}
		if (pobj->is_dead() || 
			((pstat = pobj->proc_ctl()->status(what, why))
			== p_dead))
		{
			// if is_dead is true, the object must have
			// been marked dead while we were going through
			// this list, since all_live will not return a 
			// dead pobj

			DPRINT(DBG_FOLLOW, ("inform_processes:%#x (%s) pobj dead \n", pobj, pobj->obj_name()));
			pobj->clear_check();
			if (pobj->waiting())
				waitlist.remove(pobj);
			if (pstat == p_dead)
			{
				// If pobj is a thread, its exit
				// may also mean entire process has exited;
				// If so, pobj->destroy() will update
				// state of all sibling threads
				pobj->destroy(0);
#ifdef DEBUG_THREADS
				if (pobj->obj_type() == pobj_thread)
				{
					Process	*proc = pobj->process();
					if (proc->get_state()== es_dead)
					{
						// entire process has exited
						message_manager->reset_context((ProcObj *)proc);
						printm(MSG_proc_exit,
						proc->obj_name());
						if (proc ==
							proglist.current_process())
							clobberedcurrent = 1;
					}
					else if (pobj->is_user())
					{
						if ((Thread *)pobj == 
						proglist.current_thread())
							clobberedcurrent = 1;

						if (thr_change >= TCHANGE_ANNOUNCE)
							printm(MSG_thread_exit,
							pobj->obj_name());
					}
				}
				else
#endif
				{
					printm(MSG_proc_exit, pobj->obj_name());
					if ((Process *)pobj == 
						proglist.current_process())
						clobberedcurrent = 1;
				}
			}
			else if (pobj == proglist.current_object())
				clobberedcurrent = 1;
		}
		else if (pstat == p_stopped)
		{
			DPRINT(DBG_FOLLOW, ("inform_processes:%#x (%s) pobj stopped \n", pobj, pobj->obj_name()));
			pobj->clear_check();
			pobj->inform(what, why);
			if (pobj->waiting())
			{
				if (pobj->get_state() != es_stepping &&
					pobj->get_state() != es_running)
				{
					pobj->clear_wait();
					waitlist.remove(pobj);
				}
			}
			// process any held interrupts or polls,
			// but keep holding them
			prdelset(&interrupt, SIGINT);
			prdelset(&interrupt, SIGPOLL);
			sigprocmask(SIG_UNBLOCK, &sset_PI, 0);
			sigprocmask(SIG_BLOCK, &sset_PI, 0);
			prdelset(&interrupt, SIGPOLL);
		}
#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
		else
		{
			// set check so we can make sure follower
			// is restarted for this process
			pobj->process()->set_check_follow();
			DPRINT(DBG_FOLLOW, ("inform_processes:%#x (%s) pobj running \n", pobj, pobj->obj_name()));
		}
#endif
	}
#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
	// check all processes to make sure followers are running
	// if needed
	proglist.start_all_followers();
#endif
	message_manager->reset_context(0);
	if (clobberedcurrent) 
	{
		proglist.reset_current(1);
	}
	DPRINT(DBG_FOLLOW, ("returning from inform_processes\n"));
	proglist.free_plist(list_head);
	return 1;
}
