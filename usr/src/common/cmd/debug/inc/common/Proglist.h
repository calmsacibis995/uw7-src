#ifndef _Proglist_h
#define _Proglist_h

#ident	"@(#)debugger:inc/common/Proglist.h	1.7"

// Access to lists of programs, processes and threads.
// Finds a thread, process, program structure given
// program names or process or thread ids.
// Also parses lists of program names and process and thread ids
// and returns a list of associated pobjs.  The returned list
// must be freed by the caller with a corresponding call to free_plist()

#include <sys/types.h>

class	ProcObj;
class	Process;
class	Program;
struct	Proclist;
class	Thread;

// structure returned by proc_list
struct plist {
	int	p_type;
	ProcObj	*p_pobj;
};

struct plist_header
{
	plist		*ptr;
	int		cnt;
	int		in_use;
	plist_header	*next;

	plist_header()	{ ptr = 0; cnt = 0; next = 0; in_use = 0; }
	~plist_header()	{ delete ptr; }
};

// Object types used in plist;
// report type of request - a single thread,
// all threads in a process, all threads in a program, etc.
#define P_THREAD	1
#define P_PROCESS	2
#define P_PROGRAM	3

class	Proglist {
	Program		*first_program;
	Program		*last_program;
	Program		*curr_program;
	Process		*curr_process;
	Thread		*curr_thread;
	plist_header	*plist_in_use;
	plist_header	*plist_free;	// pool for reuse
	plist_header	*plist_head;	// header for the current plist
	plist		*cur_plist;	// current item during list creation
	int		user_list;	// 1 if a user list is active
	const char	*all_name;	// str versions of "all", %thread, etc.
	const char	*proc_name;	// to make strcmps faster
	const char	*prog_name;
	const char	*thread_name;
	int		proc_num;
	void 		parse_item(char *, int proto);
	int 		parse_name(char *, const char *&, 
				const char *&);
	void 		grow_list();
	void		setup_list();
	plist		*finish_list();
	void		add_prog(Program *, int mode); 
	void 		add_proc(Process *, int mode);
	void		add_pobj(ProcObj *, int type, int mode);
	void 		add_thread(Process *, Thread *, const char *, int mode);
	Process		*find_process(const char *, int rpt_err);
	Process		*find_process(pid_t);
public:
			Proglist();
			~Proglist();
	plist		*proc_list(Proclist *, int process_only = 0);
	plist		*proc_list(Program *, int process_only = 0);
				// all pobjs in given program
	plist		*all_live(int process_only = 0);
				// all pobjs but no core files or proto
	plist		*all_live_id(int create_id); 
			// all live procsesses with given create id
	plist		*all_procs(); // all live pobjs and core files
	void		add_program(Program *);
	void		remove_program(Program *);
	void		free_plist(plist *);
	void		prune();	// get rid of dead objects
	void		reset_current(int announce); // reset current thread, proc, prog
	void		set_current(Program *, int announce);
	void		set_current(Process *, int announce);
	void		set_current(Thread *, int announce);
	void		set_current(ProcObj *, int announce);
	Program		*find_prog(const char *);
	Process		*find_proc(const char *);
	Thread		*find_thread(const char *);
	ProcObj		*find_pobj(pid_t);
	ProcObj		*find_pobj(char *);
	void		make_list();
	void		add_list(ProcObj *, int level);
	plist		*list_done();
	//		Access functions
	Program		*prog_list() { return first_program; }
	Program		*current_program() { return curr_program; }
	Process		*current_process() { return curr_process; }
	Thread		*current_thread()	 { return curr_thread; }
	ProcObj		*current_object();
	void		reset_prog(Program *p) { curr_program = p; }
	int		next_proc() { return ++proc_num; }
	void		dec_proc() { proc_num--; }
	int		valid(ProcObj *);
#ifdef DEBUG_THREADS
	void		start_all_followers();
#endif
#if DEBUG
	void		print_list();
#endif
};

extern	Proglist	proglist;

#endif	// _Proglist_h
