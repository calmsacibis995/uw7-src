#ident	"@(#)procitem.h	1.2"

/*----------------------------------------------------------------------------
 *					ProcItem class - Object that creates/controls processes
 *----------------------------------------------------------------------------*/
#ifndef PROCITEM_H
#define PROCITEM_H

#include <sys/procfs.h>

class ProcList; 

class ProcItem {

	friend class	ProcList;
public:							// Constructors/Destructors
								ProcItem (psinfo_t&, ProcItem *item = 0);
								~ProcItem ();

private:						// Private Data
	psinfo_t					*_psinfostruct;
	lwpsinfo_t					**_lwpsinfostruct;
	ProcItem					*_next;
	char						_level[20], _class[20];

private: 						// Private interface methods
	void						GetEachLWP (int, lwpid_t);
	void						GetLWP ();

public:							// Public ProcItem Methods
	psinfo_t					*GetPSInfoStruct () const 
										{ return _psinfostruct; }
	lwpsinfo_t					**GetLWPSInfoStruct () const 
										{ return _lwpsinfostruct; }

};

#endif	// PROCITEM_H
