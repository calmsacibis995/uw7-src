#ident	"@(#)debugger:libexecon/common/Program.C	1.3"

// Program control.
// Control all processes and Threads derived from a single
// executable.

#include "Program.h"
#include "Process.h"
#include "Proglist.h"
#include "Thread.h"
#include "PtyList.h"
#include <sys/types.h>
#include <signal.h>
#include <string.h>

Program::Program(Process *proc, const char *execname, const char *pname, 
	const char *args, PtyInfo *cio, time_t stime, int id)
{
	proto_mode = 0;
	first_proc = last_proc = proc;
	ename = new(char[strlen(execname) + 1]);
	strcpy((char *)ename, execname);
	progname = pname;
	if (args)
	{
		arguments = new(char[strlen(args) + 1]);
		strcpy((char *)arguments, args);
	}
	else
		arguments = "";
	child_io = cio;
	_symfiltime = stime;
	createid = id;
	etable = 0;
	srcpath = 0;
	path_age = 0;
	namecnt = 0;
	proglist.add_program(this);
}

// constructor for proto programs - keep track of event tables
Program::Program(EventTable *et, const char *pname, 
	const char *execname, int mode)
{
	ename = new(char[strlen(execname) + 1]);
	strcpy((char *)ename, execname);
	etable = et;
	progname = pname;
	proto_mode = (char)mode;
	first_proc = last_proc = 0;
	arguments = 0;
	child_io = 0;
	_symfiltime = 0;
	createid = 0;
	srcpath = 0;
	path_age = 0;
	namecnt = 0;
	proglist.add_program(this);
}

Program::~Program()
{
	if (child_io)
		cleanup_childio();
}

void
Program::add_proc(Process *proc)
{
	if (last_proc)
		proc->append(last_proc);
	else
		first_proc = proc;
	last_proc = proc;
}

void
Program::remove_proc(Process *proc, int nodelete)
{
	if (proc == last_proc)
		last_proc = (Process *)proc->prev();
	if (proc == first_proc)
		first_proc = proc->next();
	proc->unlink();
	if (!nodelete)
		delete proc;
}


void
Program::cleanup_childio()
{
	// allow SIGPOLL to pick up any last pending I/O
	sigrelse(SIGPOLL);
	if (child_io->dec_count() == 0)
		delete child_io;
	sighold(SIGPOLL);
}

void
Program::rename(const char *name)
{
	progname = name;
	// program names are stored at program, process and thread
	for(Process *proc = first_proc; proc; proc = proc->next())
	{
		for(Thread *t = proc->thread_list(); t; t = t->next())
			t->rename((char *)name);
		proc->rename((char *)name);
	}
}

// return first active process assocated with this program
Process *
Program::first_process()
{
	for (Process *proc = first_proc; proc; proc = proc->next())
	{
		if (proc->get_state() != es_dead)
			return proc;
	}
	return 0;
}
