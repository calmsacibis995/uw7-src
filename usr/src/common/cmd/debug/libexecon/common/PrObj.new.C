#ident	"@(#)debugger:libexecon/common/PrObj.new.C	1.9"

#include "Iaddr.h"
#include "ProcObj.h"
#include "Process.h"
#include "Instr.h"
#include "global.h"
#include "FileEntry.h"
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif

ProcObj::ProcObj(int pflags) : instr(this)
{
	flags = pflags;
	verbosity = vmode;
	pc = lopc = hipc = 0;
	dot = 0;
	current_srcfile = 0;
	fake_fentry = 0;
	last_sym = 0;
	epoch = 0;
	hw_watch = 0;
	sw_watch = 0;
	latestbkpt = 0;
	latestexpr = 0;
	latestflt = latestsig = latesttsc = 0;
	hoppt = destpt = dynpt = 0;
	foreignlist = 0;
#ifdef DEBUG_THREADS
	threadpt = startpt = 0;
#endif
	cur_frame = top_frame = 0;
	saved_gregs = 0;
	saved_fpregs = 0;
#if EXCEPTION_HANDLING
	eh_data = 0;
#endif
#ifndef HAS_SYSCALL_TRACING
	execvept = forkpt = vforkpt = fork1pt = forkallpt = 0;
#endif
}

ProcObj::~ProcObj()
{
	delete saved_gregs;
	delete saved_fpregs;
	delete fake_fentry;
#if EXCEPTION_HANDLING
	delete eh_data;
#endif
}

pid_t
ProcObj::pid()
{
	return process()->pid();
}

Program *
ProcObj::program()
{
	return process()->program();
}

EventTable *
ProcObj::events()
{
	return process()->events();
}

// null base class versions

Process *
ProcObj::process()
{
	return 0;
}
