#ident	"@(#)debugger:libexecon/common/Proc.new.C	1.42"

#include "ProcObj.h"
#include "Program.h"
#include "Proglist.h"
#include "Process.h"
#include "Thread.h"
#include "Parser.h"
#include "Procctl.h"
#include "Proccore.h"
#include "EventTable.h"
#include "Interface.h"
#include "Machine.h"
#include "utility.h"
#include "PtyList.h"
#include "Seglist.h"
#include "Location.h"
#include "Symbol.h"
#include "Frame.h"
#include "Manager.h"
#include "Object.h"
#include "global.h"
#include "str.h"
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

// Constructor used for live processes and core files
Process::Process() : PROCOBJ(L_PROCESS)
{
	state = es_invalid;
	goal = pg_run;
	goal2 = sg_run;
	pctl = 0;
	pprogram = 0;
	core = 0;
	stop_all_cnt = 0;
	textfd = -1;
	seglist = 0;
	etable = 0;
	head_thread = 0;
	next_thread = 0;
	thr_brk_addr = 0;
	startloc = 0;
	firstevent = 0;
	ppid = -1;
#ifdef DEBUG_THREADS
	thr_dbg_addr = thr_map_addr = 0;
	last_thread = unused_list = 0;
#endif
	lang = UnSpec;
}

Process::~Process()
{
	if (pctl)
	{
		pctl->close();
		delete pctl;
	}
	if (textfd >= 0)
	{
		::close(textfd);
	}
	if (core)
	{
		core->close();
		delete core;
	}
	delete seglist;
#ifdef DEBUG_THREADS
	remove_all_threads();
#endif
}

// setup data for core files
int
Process::grab_core( int tfd, int cfd, int pnum, const char *exname,
	const char *srcpath)
{
	time_t		stime;

	if (!setup_name(exname, pnum, 1, stime))
	{
		state = es_dead;
		return 0;
	}
	state = es_corefile;
	seglist = new Seglist(this);
	if (!seglist->setup( tfd, ename ))
	{
		state = es_dead;
		return 0;
	}
	core = new Proccore;
	textfd = tfd;
	if ((core->open(cfd) == 0) ||
		(textfd < 0) ||
		(seglist->readproto( textfd, core, ename ) == 0))
	{
		state = es_dead;
		return 0;
	}
	regaccess.setup_core(this);
	regaccess.update();
	pc = regaccess.getreg( REG_PC );
	pprogram = new Program(this, ename, progname,
		core->psargs(), 0, stime, 0);
	if (srcpath)
		pprogram->set_path(makestr(srcpath));
	find_cur_src();
#ifdef DEBUG_THREADS
	if (!thread_debug_setup() || ((thr_map_addr != 0) &&
		!thread_setup_core()))
	{
		printe(ERR_core_threads, E_WARNING, pobj_name);
	}
#endif
#if EXCEPTION_HANDLING
	if (!eh_debug_setup(/*is_core=*/1))
	{
		printe(ERR_eh_setup, E_WARNING, pobj_name);
	}
#endif
	printm(MSG_new_core, progname, pobj_name);
#ifdef DEBUG_THREADS
	print_thread_list(1);
#else
	show_current_location(1);
#endif
	return 1;
}

// Create live process
int
Process::create(char *cmdline, int pnum, int input, 
	int output, int redir, int id, int on_exec, 
	int flevel, Location *start_loc, const char *srcpath)
{
	PtyInfo		*child_io = 0;
	time_t		symfiltime;

	if (!setup_name(cmdline, pnum, 1, symfiltime))
	{
		state = es_dead;
		return 0;
	}

	startloc = start_loc;
	pctl = new Proclive;

	if (redir & REDIR_PTY)
	{
		child_io = new PtyInfo;
		{
			if (child_io->is_null())
			{
				state = es_dead;
				return 0;
			}
		}
	}

	if ((ppid = pctl->create(cmdline, ename, redir, input, 
		output, child_io)) == 0)
	{
		state = es_dead;
		return 0;
	}

	flags |= (L_IS_CHILD|L_IN_START);
	if (flevel == FOLLOW_NONE)
	{
		flags |= L_IGNORE_FORK;
	}

	if (!setup_data(1))
	{
		kill(ppid, SIGKILL);
		state = es_dead;
		return 0;
	}

	regaccess.update();
	pc = instr.adjust_pc();
	state = es_halted;

	if (!on_exec)
		// don't stop on exec - go past start address
		flags |= L_GO_PAST_START;


	pprogram = new Program(this, ename, progname, cmdline,
		child_io, symfiltime, id);
	if (srcpath)
		pprogram->set_path(makestr(srcpath));

	if (!setup_process(1))
	{
		kill(ppid, SIGKILL);
		state = es_dead;
		proglist.remove_program(pprogram);
		return 0;
	}

	printm(MSG_createp, progname, pobj_name);
#ifdef DEBUG_THREADS
	print_thread_list(1, es_halted);
#else
	show_current_location(1, es_halted);
#endif
#if EXCEPTION_HANDLING
	if (eh_data && get_ui_type() == ui_gui)
		printm(MSG_uses_eh, (unsigned long)this);
#endif
	return 1;
}

// grab already running process - not created by one of our subjects
int
Process::grab(int pnum, char *path, char *loadfile, int id, int flevel,
	const char *srcpath)

{
	char		*args = 0;
	char		*s, *p;
	time_t		symfiltime;

	pctl = new Proclive;
	s = strrchr( path, '/' );
	flags |= L_GRABBED;
	if (flevel == FOLLOW_NONE)
	{
		flags |= L_IGNORE_FORK;
	}
	if (s)
	{
		s++;
		ppid = (pid_t)strtol(s, &p, 10);
		if (*p || !pctl->open(path, ppid, 0) )
		{
			state = es_dead;
			return 0;
		}
	}
	else
	{
		ppid = (pid_t)strtol(path, &p, 10);
		if (*p || !pctl->open(ppid, 0) )
		{
			state = es_dead;
			return 0;
		}
	}
	if ( stop() == 0 )
	{
		state = es_dead;
		return 0;
	}
	if ((args = (char *)pctl->psargs()) == 0)
	{
		drop_process(1);
		return 0;
	}
	if (loadfile)
	{
		// alternate executable file specified
		if (!setup_name(loadfile, pnum, 1, symfiltime))
		{
			drop_process(1);
			return 0;
		}
	}
	else
	{
		// get executable name from psinfo
		if (!setup_name(args, pnum, 0, symfiltime))
		{
			drop_process(1);
			return 0;
		}
	}
	if (!setup_data(loadfile ? 1 : 0))
	{
		drop_process(1);
		return 0;
	}
	pc = instr.adjust_pc();
	pprogram = new Program(this, ename, progname, args,
		0, symfiltime, id);
	if (srcpath)
		pprogram->set_path(makestr(srcpath));
	if (!setup_process(0))
	{
		drop_process(1);
		proglist.remove_program(pprogram);
		return 0;
	}
	printm(MSG_grab_proc, progname, pobj_name);
#ifdef DEBUG_THREADS
	print_thread_list(1, es_halted);
#else
	show_current_location(1, es_halted);
#endif
#if EXCEPTION_HANDLING
	if (eh_data && get_ui_type() == ui_gui)
		printm(MSG_uses_eh, (unsigned long)this);
#endif
	return 1;
}

// one of our subjects has forked - setup new process
int
Process::control_new_proc(pid_t npid, ProcObj *old_obj, int syscall)
{
	Process 	*proc;

	proc = new Process();
	if (!proc->grab_fork(this, proglist.next_proc(),
		npid, syscall))
	{
		printe(ERR_fork_child_lost, E_WARNING, pobj_name, 
			strerror(errno));
		proglist.dec_proc();
		delete proc;
		return 0;
	}
	message_manager->reset_context(proc);
	if (old_obj == this)
		// single threaded
		printm(MSG_proc_fork, pobj_name, proc->pobj_name);
	else
		printm(MSG_thread_fork, pobj_name, old_obj->obj_name(),
			proc->pobj_name);
#ifdef DEBUG_THREADS
	if (proc->next_thread > 0)
		proc->print_thread_list(0);
#endif
	if (latesttsc == SYSCALL_vfork)
		printe(ERR_vfork_restart, E_WARNING);
	message_manager->reset_context(0);
	return 1;
}

// one of our subjects has forked - control new process
// this routine is shared by fork1, forkall and vfork

int
Process::grab_fork(Process *op, int pnum, pid_t npid, int syscall)
{
	time_t		stime;
	int		what, why;

	ppid = npid;

	flags = op->flags;
	// set L_GRABBED even for a child of a created process -
	// this tells the segment code that it cannot rely
	// on the process being at the initial frame
	flags |= L_GRABBED;
	flags &= ~L_IN_START;

	pctl = new Proclive;
#if PTRACE
	if (pctl->open(ppid, 1) == 0)
	{
		return 0;
	}
#else
	if (pctl->open(ppid, ((flags & L_IS_CHILD) != 0)) == 0)
	{
		return 0;
	}
#endif

	if ((pctl->wait_for(what, why) != p_stopped) ||
		!pctl->follow_children() ||
		!setup_name(0, pnum, 0, stime))
	{
		drop_process(1);
		return 0;
	}

	ename = op->ename;
	progname = op->progname;
	if (!setup_data(0))
	{
		drop_process(1);
		return 0;
	}
	pprogram = op->pprogram;
	seglist->build( pctl, ename);
	pc = instr.adjust_pc();
	if (!setup_et_copy(op))
	{
		drop_process(1);
		return 0;
	}
	remove(bk_hoppt);
	state = es_halted;
	pprogram->add_proc(this);
	
	// For fork and forkall we copy all sibling threads.
	// For vfork or fork1 we ignore threads, since
	// threads library data structures are no longer valid
#ifdef DEBUG_THREADS
	if (syscall == SYSCALL_fork || syscall == SYSCALL_forkall)
	{
		// copy sibling threads
		if (!thread_debug_setup())
		{
			printe(ERR_thread_setup, E_WARNING, pobj_name);
		}
		if (thr_brk_addr != 0)
		{
			// threadpt created by copy_events
			if (!thread_setup_live(op))
			{
				printe(ERR_thread_setup, E_WARNING, 
					pobj_name);
			}
		}
	}
	else 
	{
		if (threadpt)
			remove(bk_threadpt);
	}
#endif

	// if there were threads, thread setup would have
	// copied events - if not, do it here
	if (!first_thread(0))
		copy_events(op);
	return 1;
}

// Subject process execs new program.
int
Process::control_new_prog(const char *pathname)
{
	char		*args;
	time_t		stime;
	Program		*oprog;
	PtyInfo		*child_io;

	flags |= (L_IN_START|L_GO_PAST_START);
	flags &= ~L_GRABBED;
	delete seglist;
	seglist = 0;
	state = es_syscallxit;

#ifdef DEBUG_THREADS
	// do not delete threads, since they may
	// be on the process list for inform;
	// just mark them as dead
	for (Thread *thread = head_thread; thread; 
		thread = thread->next())
		thread->mark_dead(P_EXEC);
#endif
	if (((args = (char *)pctl->psargs()) == 0) ||
		!make_proto(P_EXEC) ||
		!setup_name(pathname ? pathname : args, 
			-1, 0, stime, (char *)ename) ||
		!setup_data(0))
	{
		drop_process(1);
		printe(ERR_proc_exec, E_ERROR, pobj_name);
		return 0;
	}
#ifdef DEBUG_THREADS
	next_thread = 0;
	thr_brk_addr = thr_map_addr = 0;
#endif
	// initial startup - do exec of process
	startloc = 0;

	if (!setup_process(1))
	{
		drop_process(1);
		printe(ERR_proc_exec, E_ERROR, pobj_name);
		return 0;
	}

	message_manager->reset_context(this);
	printm(MSG_proc_exec, pobj_name, ename, progname);
	oprog = pprogram;
	oprog->remove_proc(this, 1);
	if (child_io = oprog->childio())
		child_io->bump_count();
	pprogram = new Program(this, ename, progname, args,
		child_io, oprog->symfiltime(), 
		oprog->create_id());
	if (this == proglist.current_process())
		proglist.set_current(pprogram, 0);
#ifdef DEBUG_THREADS
	print_thread_list(1, es_halted);
#else
	show_current_location(1, es_halted);
#endif
	message_manager->reset_context(0);
	check_watchpoints();
	return 1;
}
	
// common routine to setup data for exec'd and grabbed processes
// and to get them to the right starting address
int
Process::setup_process(int stop_at_init)
{
	// set up new process just exec'd in pipeline
	Iaddr		raddr, saddr;
	int		fd;
	EventTable	*et;
	char		*path;
	int		what, why;

	DPRINT(DBG_CTL, ("setup_process: %#x (%s)\n", this, pobj_name));

	// get address for dynpt and process start addr
	raddr = seglist->rtl_addr( pctl );
	seglist->build( pctl, ename );
	saddr = seglist->start_addr();
	// must set up event table before setting breakpoints
	if ((fd = pctl->open_object( 0, ename )) == -1)
		return 0;
	et = find_et(fd, path);
	close( fd );
	if (!et)
	{
		return 0;
	}
	use_et(et);
	if (raddr != 0)
		dynpt = set_bkpt( raddr, 0, 0 );
	if ((flags & L_GO_PAST_START) && stop_at_init && (pc != saddr))
	{
		// run to point in dynamic linker where symbols are available
		// or, if using older dynamic linker, skip past dynamic linker
		// to start address specified by object file

		destpt = set_bkpt( saddr, 0, 0 );
		if (!start(sg_run, follow_no))
		{
			return 0;
		}
		while (pc != saddr)
		{
			if (seglist->has_init_support() &&
				seglist->symbols_available())
			{
				remove( bk_destpt );
				break;
			}
				
			if ((pctl->wait_for(what, why, 1) != p_stopped) ||
				(!inform(what, why)))
			{
				return 0;
			}
		}
	}
	// all shared objects loaded at startup should be there now
#ifdef DEBUG_THREADS
	if (!thread_debug_setup())
	{
		printe(ERR_thread_setup, E_WARNING, pobj_name);
	}
#endif
	topframe();	// force AddrList to be created before NameList
#if EXCEPTION_HANDLING
	// set up EH data before creating thread list
	if (!eh_debug_setup())
	{
		printe(ERR_eh_setup, E_WARNING, pobj_name);
	}
#endif
	ProcObj	*pobj = (ProcObj *)this;
#ifdef DEBUG_THREADS
	if (thr_brk_addr != 0)
	{
		threadpt = set_bkpt(thr_brk_addr, 0, 0);
		if (!stop_at_init || !seglist->has_init_support())
		{
			// past _rt_boot code
			// all shared objects loaded at startup should be there now
			// if we are using threads, we should have at least
			// one virtual thread by now; use this in final run
			// to start loc
			if (!thread_setup_live(0))
				printe(ERR_thread_setup, E_WARNING, pobj_name);
			else if ((flags&L_GO_PAST_START)
				&& (pobj = (ProcObj *)first_thread(1)) == 0)
				pobj = (ProcObj *)this;
		}
	}
#endif
	if (flags & L_GO_PAST_START)
	{
		// run to address specified by create, 
		// "main" by default

		Symbol		sym;
		int		ret;
		Location	l, *sloc;
		Iaddr		prog_ctr;

		prog_ctr = pobj->top_pc();
		pobj->set_start();

		if (startloc)
		{
			sloc = startloc;
			ret = get_addr(pobj, sloc, saddr,
				st_func, sym, E_WARNING);
		}
		else 
		{
			// stop at main by default
			l.set_func("main");
			sloc = &l;
			ret = get_addr(pobj, sloc, saddr,
				st_func, sym, E_WARNING);
		}
		if (ret)
		{
			if (sloc->get_type() == lk_fcn && 
				(sym.tag() != t_label))
			{
				// skip function prolog
				long	off;
				sloc->get_offset(off);
				if (off == 0)
				{
					saddr = pobj->first_stmt(saddr);
				}
			}
		}
		else if (stop_at_init) // get address specified by the object
			saddr = seglist->start_addr();
		else
			saddr = prog_ctr;
		if (prog_ctr != saddr) 
		{
			if (pobj->set_destination(saddr) == 0)
				printe(ERR_sys_no_breakpt, E_WARNING, 
						saddr, pobj_name);
			else
			{
				// run process until start addr
				//if (!pobj->start(sg_run, follow_no))
				if (!pobj->restart(follow_no))
					return 0;
				// allow interrupt in loop in case
				// we never reach start address
				// break out when we reach start
				// address or when thread gives
				// up LWP
				sigrelse(SIGINT);
				ret = 1;
				while(pobj->top_pc() != saddr)
				{
					if (pobj->proc_ctl()->wait_for(what,
						why, 1) != p_stopped)
					{
						ret = 0;
						break;
					}
					if (prismember(&interrupt, SIGINT))
					{
						state = es_halted;
						break;
					}
					if (!pobj->inform(what, why))
					{
						ret = 0;
						break;
					}
				}
				sighold(SIGINT);
				if (!ret)
					return 0;
			}
		}
#ifdef DEBUG_THREADS
		if (thr_brk_addr != 0 && seglist->threads_initialized())
		{
			if (!process()->thread_setup_live(0))
				printe(ERR_thread_setup, E_WARNING, pobj_name);
			else if ((pobj = (ProcObj *)first_thread(1)) == 0)
				pobj = (ProcObj *)this;
		}
#endif
		if (pobj->obj_type() == pobj_thread)
		{
			pobj->clear_start();
			((Thread *)pobj)->find_cur_src();
			pc = pobj->top_pc();
			state = pobj->get_state();
		}
	}
	if (path && pprogram && !pprogram->src_path())
		pprogram->set_path(path);
	find_cur_src();
	flags &= ~L_IN_START;
	flags &= ~L_CHECK;
	if (!(flags & L_IGNORE_FORK))
	{
#ifndef HAS_SYSCALL_TRACING
		// set special breakpoints on fork/fork1/forkall
		int		ret;
		Symbol		sym;
		Location	l;
		Iaddr		baddr;
		ProcObj		*pobj = (ProcObj *)this;

		l.set_func("_fork");
		ret = get_addr(pobj, &l, baddr,
			st_func, sym, E_WARNING);
		if (ret && l.get_type() == lk_fcn && 
			(sym.tag() != t_label))
		{
			if (!pctl->follow_children())
				return 0;
			forkpt = set_bkpt(baddr, 0, 0);
			l.set_func("_vfork");
			ret = get_addr(pobj, &l, baddr,
				st_func, sym, E_NONE);
			if (ret && l.get_type() == lk_fcn && 
				(sym.tag() != t_label))
			{
				vforkpt = set_bkpt(baddr, 0, 0);
			}
			l.set_func("_forkall");
			ret = get_addr(pobj, &l, baddr,
				st_func, sym, E_NONE);
			if (ret && l.get_type() == lk_fcn && 
				(sym.tag() != t_label))
			{
				forkallpt = set_bkpt(baddr, 0, 0);
			}
			l.set_func("_fork1");
			ret = get_addr(pobj, &l, baddr,
				st_func, sym, E_NONE);
			if (ret && l.get_type() == lk_fcn && 
				(sym.tag() != t_label))
			{
				fork1pt = set_bkpt(baddr, 0, 0);
			}
		}
		else
		{
			flags |= L_IGNORE_FORK;
		}
#else
		if (!pctl->follow_children())
			return 0;
#endif
	}
#ifndef HAS_SYSCALL_TRACING
	{
		// set special breakpoint on execve
		int		ret;
		Symbol		sym;
		Location	l;
		Iaddr		baddr;
		ProcObj		*pobj = (ProcObj *)this;

		l.set_func("_execve");
		ret = get_addr(pobj, &l, baddr,
			st_func, sym, E_WARNING);
		if (ret && l.get_type() == lk_fcn && 
			(sym.tag() != t_label))
		{
			execvept = set_bkpt(baddr, 0, 0);
		}
	}
#endif
	(void)re_init_et();
#ifdef DEBUG_THREADS
	// copy parent's events to threads - must do after
	// re_init_et();
	for(Thread *thread = head_thread; thread;
		thread = thread->next())
	{
		if (thread->is_virtual() || thread->is_dead()
			|| thread->is_released())
			continue;
		thread->copy_et_create();
	}
#endif
	return 1;
}

// Proto programs are used to keep track of event tables
// for processes that have died; these event tables are reused
// if the process is re-created.

int
Process::make_proto(int mode)
{
	Program	*protop;
	int	noproto = 0;

	// do not create a proto process if we have 
	// sibling processes

	Process *p = pprogram->proclist();

	for (; p; p = p->next())
	{
		if ((p != this) && !p->is_dead())
			break;
	}
	if (p)
		noproto = 1;

	if (!noproto)
	{
		protop = new Program(etable, progname, ename, mode);
		protop->set_path(pprogram->src_path());
	}
	if (!cleanup_et(mode, noproto))
	{
		if (!noproto)
			proglist.remove_program(protop);
		return 0;
	}
	return 1;
}

int
Process::setup_data(int use_obj)
{
	int	fd;

	// ename might not be full path name or might not
	// be directly accessible from the current directory
	// if it came from psargs information
	seglist = new Seglist(this);
	if (use_obj)
	{
		if ((fd = open( ename, O_RDONLY )) == -1)
			return 0;
	}
	else
	{
		if ((fd = pctl->open_object( 0, ename )) == -1)
			return 0;
	}
	if (!seglist->setup( fd, ename ))
	{
		close(fd);
		return 0;
	}
	regaccess.setup_live(this);
	regaccess.update();
	close(fd);

	return 1;
}

// A thread has exited - check process status to make sure
// we are still alive.  If not, update status of all remaining
// threads
// Returns 1 if the process is still live, else 0.

int 
Process::update_status()
{
	int	what, why;

	if (!pctl || is_dead())
		return 0;
	if (pctl->status(what, why) == p_dead)
	{
		destroy(0);
		return 0;
	}
	return 1;
}

// setup process name and exec and program names
int
Process::setup_name(const char *name, int pnum, int use_obj, 
	time_t &symfiltime, char *oldname)
{
	char		buf[PATH_MAX];
	char		*p;
	size_t		sz;
	Program		*prog;
	struct stat	stbuf;

	if (pnum > 0)
	{
		// process id
		char	n[MAX_LONG_DIGITS+2];  // chars in MAX_LONG 
			// + 'p', + null
		sprintf(n, "p%d" , pnum);
		pobj_name = str(n);
	}
	if (!name)	// used for fork
		return 1;
	// copy name until first blank - start of arguments
	if ( (p = strchr( name, ' ' )) != 0 )
		sz = p - (char *)name;
	else
		sz = strlen(name);
	strncpy( buf, name, sz );
	buf[sz] = '\0';
	if (use_obj)
	{
		// check for #! shell scripts
		int	fd;
		char	*scmd;

		if ((fd = open(buf, O_RDONLY)) == -1)
		{
			printe(ERR_no_access, E_ERROR, buf);
			return 0;
		}
		if ((scmd = check_shell_script(fd)) != 0)
		{
			// we have a shell script - reset program name
			if ( (p = strchr( scmd, ' ' )) != 0 )
				sz = p - (char *)scmd;
			else
				sz = strlen(scmd);
			strncpy( buf, scmd, sz );
				buf[sz] = '\0';
		}
		close(fd);
	}
	ename = str(buf);
	// check for conflicts in program name
	p = basename((char *)ename);
	progname = str(p);
	for(prog = proglist.prog_list(); prog; prog = prog->next())
	{
		if (progname == prog->prog_name())
		{
			if (!prog->proclist() || prog->events())
				continue;
			else if (prog->first_process() != 0)
				break;
		}
	}
	if (prog)
	{
		sprintf(buf, "%s.%d", progname, prog->name_cnt());
		printe(ERR_used_name, E_WARNING, progname, buf);
		progname = str(buf);
	}
	// ename might not be full path name or might not
	// be directly accessible from the current directory
	// if it came from psargs information
	if (use_obj)
	{
		if (stat(ename, &stbuf) == -1)
		{
			printe(ERR_no_access, E_ERROR, ename);
			return 0;
		}
	}
	else
	{
		int	fd;

		char	*d;
		if ((access(ename, R_OK) == -1) && oldname &&
			((d = strrchr(oldname, (int)'/')) != 0))
		{
			// pathname for args from psargs isn't directly
			// available - if we have an oldname 
			// (process exec'd)
			// try to create a new name using the same path
			
			sz = d - oldname + 1;
			strncpy( buf, oldname, sz );
			strcpy(buf + sz, ename);
			if (access(buf, R_OK) == 0)
				ename = str(buf);
		}
		fd = pctl->open_object( 0, ename );
		if (fstat(fd, &stbuf) == -1)
		{
			printe(ERR_no_access, E_ERROR, ename);
			return 0;
		}
		close(fd);
	}
	symfiltime = stbuf.st_mtime;
	return 1;
}

#ifdef DEBUG_THREADS
void
Process::remove_all_threads()
{
	while (head_thread)
	{
		Thread	*thread;
		thread = head_thread;
		head_thread = head_thread->next();
		delete thread;
	}
	while (unused_list)
	{
		Thread	*thread;
		thread = unused_list;
		unused_list = unused_list->next();
		delete thread;
	}
	next_thread = 0;
}

void
Process::print_thread_list(int show, Execstate pstate)
{
	enum Msg_id	msg = is_core() ? MSG_new_core_thread :
			MSG_new_thread;

	if (get_ui_type() == ui_gui)
	{
		for(Thread *thread = head_thread; thread;
			thread = thread->next())
		{
			if (thread->is_virtual() || thread->is_dead()
				|| thread->is_released())
				continue;

			message_manager->reset_context((ProcObj *)thread);
			printm(msg, (int)thread->thread_id(), 
				thread->obj_name());
		}
	}
	else
	{
		if (next_thread > 0)
			printm(is_core() ? MSG_core_thread_count : 
				MSG_thread_count, next_thread, pobj_name);
	}
	if (show)
	{
		Thread	*first = first_thread(0);
		if (first)
		{
			message_manager->reset_context((ProcObj *)first);
			if (first->is_suspended())
				printm(MSG_thr_suspend, first->obj_name());
			else
				first->show_current_location( 1, 
					pstate == es_invalid ? state : pstate);
			if (core && (!first->core_ctl() ||
				(first->core_ctl()->lwp_id() !=
					core->lwp_id())))
			{
				printe(ERR_faulting_not_user,
					E_WARNING, pobj_name);
			}

		}
		else
			show_current_location( 1,
				pstate == es_invalid ? state : pstate);
	}
}
#endif
