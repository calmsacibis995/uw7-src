#ifndef Symtab_h
#define Symtab_h
#ident	"@(#)debugger:inc/common/Symtab.h	1.6"

// Symtab - the mapping of a Symtable to a base address.
//
// A Symtab contains a base address and a pointer to a Symtable (which see).
// Its mission in life is to convert between the relative addresses in its
// Symtable and the virtual addresses in the ProcObj.  More than one Symtab
// may point to a given Symtable.  Symtab has no constructor; it is created
// only as a member of a Symnode (see Seglist.C).  Symtab has no destructor,
// since the Symtable it points to is never destroyed.
//
// Symtab supplies all of the member functions of Symtable.  Symtab is visible
// outside of libsymbol; Symtable is not.

#include	"Symbol.h"
#include	"Iaddr.h"
#include	"Fund_type.h"

class Evaluator;
class AddrEntry;
class NameEntry;
class Symtable;

class Symtab {
public:
	Iaddr		ss_base;
	Symtable *	symtable;
	Symbol		first_symbol();
	Symbol		find_scope ( Iaddr );
	Symbol		find_entry ( Iaddr );
	Symbol		find_symbol ( Iaddr );
	Symbol		find_global( const char * );
	int 		find_source( Iaddr, Symbol & );
	int 		find_source( const char *, Symbol & );
	NameEntry *	first_global();
	NameEntry *	next_global();
	Symbol		global_symbol( NameEntry * );
	int		find_next_global(const char *, Symbol &);
	int		create_fake_symbol(const char *, Symbol &);
	void		global_search_complete();
};

#endif /* Symtab_h */
