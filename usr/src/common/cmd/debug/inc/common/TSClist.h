#ifndef TSClist_h
#define TSClist_h
#ident	"@(#)debugger:inc/common/TSClist.h	1.5"

#include "Machine.h"

enum Systype {
	NoSType = 0,
	Entry,
	Exit,
	Entry_exit
};

#ifdef HAS_SYSCALL_TRACING

#include "Ev_Notify.h"
#include "Proctypes.h"
#include <sys/syscall.h>

// special system call exits - always trapped by debug
#ifdef DEBUG_THREADS
#define	SPECIAL_EXIT(I) 	(((I) == SYS_fork) || \
				    ((I) == SYS_vfork) ||\
				    ((I) == SYS_exec) ||\
				    ((I) == SYS_execve) ||\
				    ((I) == SYS_lwpcreate))
#else
#define	SPECIAL_EXIT(I) 	(((I) == SYS_fork) || \
				    ((I) == SYS_vfork) ||\
				    ((I) == SYS_exec) ||\
				    ((I) == SYS_execve))
#endif

class	ProcObj;
struct	TSCevent;

// modes = entry or exit or both
#define TSC_entry	1
#define TSC_exit	2


class TSClist {
	sys_ctl		entrymask;
	sys_ctl		exitmask;
	TSCevent	*_events;
	TSCevent	*lookup(int);
	TSCevent	*find(int);
public:
			TSClist();
			~TSClist();
	int		add(int sys, int mode, Notifier,
				void *, ProcObj *);
	int		remove(int sys, int mode, Notifier, 
				void *, ProcObj *);
	sys_ctl		*tracemask(int mode = TSC_entry);
	NotifyEvent 	*events(int sys, int mode = TSC_entry);
};

struct syscalls
{
	const char	*name;
	int		entry;
};

extern syscalls systable[];

#endif
#endif
// end of TSClist.h
