#ident	"@(#)proclist.h	1.2"

/*----------------------------------------------------------------------------
 *					ProcList class - Object that creates/controls processes
 *----------------------------------------------------------------------------*/
#ifndef PROCLIST_H
#define PROCLIST_H

#include <sys/procfs.h>

class ProcItem;
class Process;
	
class ProcList {

public:							// Constructors/Destructors
								ProcList ();
								~ProcList ();

private:						// Private Data
	ProcItem					*_procitem, *_enditem;

	void						remove();
	int							IsLessThan(Process *, ProcItem *, int);

public:							// Public ProcList Methods

	void						append(psinfo_t&);
	void						insert(psinfo_t&);
	void						sort(Process *, int);
	ProcItem					*FindProcItem(int);
	ProcItem					*FindProcItem(pid_t);

};

#endif	// PROCLIST_H
