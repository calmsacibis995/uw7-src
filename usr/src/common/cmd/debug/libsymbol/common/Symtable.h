#ifndef Symtable_h
#define Symtable_h
#ident	"@(#)debugger:libsymbol/common/Symtable.h	1.5"

// Symtable - the repository for Symbols.
//
// A Symtable contains only a pointer to its Evaluator.  It provides a set
// of member functions for querying the Symbol graph.
//
// A Symtable differs from a Symtab (which see) in that the Symtable is
// associated with the object, not the process.  The addresses in a Symtable
// are RELATIVE addresses; the Symtab converts them to RELOCATED addresses,
// as appropriate.  Several Symtabs may point to a single Symtable, and the
// various Symtabs may have different base addresses.  The attribute lists
// contain only relative addresses which are relocated as necessary.
//
// Symtables are created only by the Object constructor and are never destroyed.

#include	"Symbol.h"
#include	"Source.h"
#include	"Fund_type.h"

class Evaluator;
class AddrEntry;
class NameEntry;
class Object;

class Symtable {
	Evaluator *	evaluator;
public:
			Symtable( int, Object * );
			~Symtable();
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

#endif /* Symtable_h */
