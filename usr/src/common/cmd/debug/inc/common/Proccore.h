#ifndef Proccore_h
#define Proccore_h
#ident	"@(#)debugger:inc/common/Proccore.h	1.2"

// Interface to core files

#include <sys/types.h>
#include "Iaddr.h"
#include "Machine.h"
#include "Proctypes.h"
#include "Procctl.h"

#ifdef DEBUG_THREADS
#define VIRTUAL	virtual
#else
#define VIRTUAL
#endif

class Lwpcore;

class Proccore {
protected:
	int		fd;
	void		*data;	// used for both CoreData and LwpCoreData
#ifdef DEBUG_THREADS
private:
	Lwpcore		*lwplist;
#endif
public:
			Proccore();
			~Proccore();
	int		open(int nfd);
	void		close();
	int		get_fd() { return fd; }
	const char	*psargs();
#ifdef DEBUG_THREADS
	Lwpcore		*lwp_list() { return lwplist; }
	void		set_lwp_list(Lwpcore *l) { lwplist = l; }
	virtual lwpid_t	lwp_id();	// id of lwp that dumped core
#endif
	VIRTUAL Procstat 	status(int &why, int &what);
	VIRTUAL gregset_t *read_greg();
	VIRTUAL fpregset_t *read_fpreg();
	void		core_state();	// why it dumped core
	Elf_Phdr	*segment(int which); 
				// which = [0..numsemgents() -1]
	int		update_stack(Iaddr &low, Iaddr &hi);
};

#ifdef DEBUG_THREADS
class Lwpcore: public Proccore {
	Lwpcore		*_next;
public:
			Lwpcore();
			~Lwpcore();
	Lwpcore		*next() { return _next; }
	void		set_next(Lwpcore *n) { _next = n; }
	lwpid_t		lwp_id();
	int		open(void * notes);
	int		current_sig();
	void		close();
	gregset_t	*read_greg();
	fpregset_t	*read_fpreg();
	Procstat 	status(int &why, int &what);
};
#endif

#endif
