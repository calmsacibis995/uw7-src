#ifndef Program_h
#define Program_h
#ident	"@(#)debugger:inc/common/Program.h	1.4"

// Program control.
// A program represents all processes
// derived from a single object file.
// It is basically a convenient way to find
// all such related processes, and provides few
// operations of its own.

#include "Link.h"
#include <sys/types.h>

class Process;
class Thread;
class EventTable;
class PtyInfo;

class Program :	public Link {
	char		proto_mode;	// released or died
	short		createid;
	short		namecnt; 	// # of times this name used
	Process		*first_proc;
	Process		*last_proc;
	const char	*ename;
	const char	*progname;
	const char	*srcpath;
	const char	*arguments;	// create or exec arguments
	EventTable	*etable;
	PtyInfo		*child_io;
	time_t		_symfiltime;
	long		path_age;
	void		cleanup_childio();
public:
			Program(Process *, const char *exec, const char *proc, 
				const char *args, PtyInfo *, time_t, int id);
			Program(EventTable *, const char *progname,
				const char *exec, int mode);
			~Program();
	Program		*next() { return (Program *)Link::next(); }
	Process		*proclist() { return first_proc; }
	void		add_proc(Process *);
	void		remove_proc(Process *, int nodelete = 0);
	void		rename(const char *s);
	Process		*first_process();
	//		Access functions
	void		set_path(const char *s) 
				{ delete (void *) srcpath; srcpath = s; path_age++; }
	long		pathage() { return path_age; }
	const char	*exec_name() { return ename; }
	const char	*prog_name() { return progname; }
	const char	*src_path() { return srcpath; }
	const char	*command()	{ return arguments; }
	PtyInfo		*childio()	{ return child_io; }
	time_t		symfiltime()	{ return _symfiltime; }
	int		create_id()	{ return createid; }
	EventTable	*events()	{ return etable; }
	int		is_proto()	{ return (etable && !first_proc); }
	int		name_cnt()	{ namecnt++; return namecnt; }
	int		mode()		{ return proto_mode; }
};

#endif	// end of Program.h
