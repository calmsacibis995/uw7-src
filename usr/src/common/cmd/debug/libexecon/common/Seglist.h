#ifndef Seglist_h
#define Seglist_h
#ident	"@(#)debugger:libexecon/common/Seglist.h	1.16"

#include "Iaddr.h"
#include "Symbol.h"
#include "Link.h"

class NameEntry;
class Object;
class Proccore;
class Proclive;
class Process;
class Segment;
class Symnode;
class Symtab;
struct Dyn_info;

// Each Seglist is shared among all the threads in a process.
// For each Seglist we maintain two lists: a list of address ranges
// (segments) and a list of objects with their associated symbol
// tables (symnodes).  Many segments may be associated with a
// single symnode.

enum Rtld_state
{
	rtld_inconsistent,
	rtld_buildable,
	rtld_symbols_loaded,
	rtld_init_complete,
	rtld_is_alternate
};

struct Rtl_data;	// opaque to clients

class SymnodeStack	: public Stack
{
public:
	Symnode	*symnode;
		SymnodeStack() { symnode = 0; }
		SymnodeStack(Symnode *s) { symnode = s; }
};

// flags values
#define S_STATIC_LOADED		0x0001
#define S_THREADS_INITIALIZED	0x0002
#define S_SYMBOLS_AVAILABLE	0x0004
#define S_HAS_INIT_SUPPORT	0x0008

class Seglist {
	short		flags;
	short		_has_stsl;
	Process		*proc;
	Segment		*mru_segment;  // most recently used
	Symbol		current_file;
	Symnode		*symnode_file;
	Symnode		*symnode_global;
	Rtl_data	*rtl_data;
	Iaddr		start;
	Symnode		*symlist;
	Segment		*first_segment;
	Segment		*last_segment;
	Segment		*stack_segment;
	SymnodeStack	symnode_stack;	// allows recursive calls to find_next_global

	Symnode		*add( int fd, Object *, const char *path, 
				int text_only, Iaddr base );
	int		add_static_shlib( Object *, Proclive *, int text_only );
	int		add_dynamic_text( int textfd, const char *exec_name );

	Symnode		*add_symnode( Object *, const char * path, 
				Iaddr base );
	void		add_dyn_info(Symnode *, Object *);
	int		build_dynamic( Proclive * );
	int		build_static( Proclive *, const char *exec_name );
	int		get_brtbl( const char *objname );
	void 		add_name(Symnode *, Iaddr addr, long size);
	void 		add_segment(Segment *);
	void		sort_seglist();
public:
			Seglist( Process *);
			~Seglist();
	int		setup( int fd, const char *exec_name);
	Rtld_state	rtld_state(Proclive *);
	int		build( Proclive *, const char *exec_name );
	Iaddr		rtl_addr( Proclive * );
	int		readproto( int txtfd, Proccore *,
				const char *exec_name);
	Symtab		*find_symtab( Iaddr );
	Symtab		*find_symtab( const char * objname);
	const char	*object_name( Iaddr );
	Segment		*find_segment( Iaddr );
	int		find_source( const char * , Symbol & );
	int		find_next_global(const char *, Symbol &);
	int		create_fake_symbol(const char *name, Symbol &);
	void		global_search_complete();
	Symbol		first_file();
	Symbol		next_file();
	Symbol		find_global( const char * );
	int		print_map(Proclive *);
	int		print_map(Proccore *);
	int		has_stsl() { return _has_stsl; }
	int		in_stack( Iaddr );
	Iaddr		end_stack();
	int		in_text( Iaddr );
	void		update_stack( Proclive * );
	Iaddr		start_addr() { return start; };
	Dyn_info	*get_dyn_info(Iaddr pc);
	Iaddr		alternate_rtld(Proclive *);

	void		set_init_flag() { flags |= S_HAS_INIT_SUPPORT; }
	void		clear_init_flag() { flags &= ~S_HAS_INIT_SUPPORT; }
	int		has_init_support() { return (flags & S_HAS_INIT_SUPPORT); }
	int		symbols_available() { return (flags & S_SYMBOLS_AVAILABLE); }
	int		threads_initialized() { return (flags & S_THREADS_INITIALIZED); }
};

#endif

// end of Seglist.h
