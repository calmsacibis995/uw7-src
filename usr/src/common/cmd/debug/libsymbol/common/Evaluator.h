#ident	"@(#)debugger:libsymbol/common/Evaluator.h	1.13"
#ifndef Evaluator_h
#define Evaluator_h

#include	"Dwarf1.h"
#include	"Dwarf2.h"
#include	"Coffbuild.h"
#include	"Elfbuild.h"
#include	"NameList.h"
#include	"AddrList.h"
#include	"Object.h"

#include	"Attribute.h"
#include	"Language.h"
#include	"Link.h"

class Syminfo;

class EntryStack	: public Stack
{
public:
	NameEntry	*entry;
		EntryStack() { entry = 0; }
		EntryStack(NameEntry *s) { entry = s; }
};

class Evaluator {
	int		fdesc;
	File_format	file_type;
	Attribute	*first_record;
	Attribute	*elf_record;
	offset_t	next_disp;
	offset_t	next_file;
	Build		*current_build;
	Attr_form	global_form;
	int		no_elf_syms;
	NameEntry	*current_entry;
	EntryStack	entry_stack;	// allows recursive calls to find_next_global
	Dwarf1build	*dwarf1build;
	Dwarf2build	*dwarf2build;
	Coffbuild	*coffbuild;	// will be 0 if any other Build* is non-zero
	Elfbuild	*elfbuild;
	NameList	namelist;
	AddrList	addrlist;

	Attribute	*find_attribute(Attribute *, Attr_name);
	Attribute	*add_node( Attribute * );
	Attribute	*add_parent( Attribute *, Attribute *ancestor );
	Attribute	*add_children( Attribute *, Attribute *ancestor,
					offset_t = 0, Attribute **entry = 0 );
	Attribute	*build_parent_chain(Attribute *current, Attribute *new_sym);
	NameEntry	*get_global( const char *name );
	Build		*get_build_from_form(Attr_form);
	int		get_syminfo(Syminfo &);
	void		pop_stack();
	void		push_stack();
public:
			Evaluator( int fd, Object *);
			~Evaluator()
				{
					delete dwarf1build;
					delete dwarf2build;
					delete coffbuild;
					delete elfbuild;
				}

	Attribute *	first_file();
	Attribute *	arc( Attribute *, Attr_name );

	Attribute *	evaluate( NameEntry * );
	Attribute *	attribute( Attribute *,  Attr_name);
	int		has_attribute(Attribute *, Attr_name);	// existence check only
	NameEntry *	first_global();
	NameEntry *	next_global();
	Attribute *	find_global( const char *name );
	Attribute *	lookup_addr( Iaddr );
	NameEntry *	find_next_global(const char *name, Attribute *);
	NameEntry *	create_fake_symbol(const char *name, Attribute *);
	void		global_search_complete() { current_entry = 0; }
};

Attribute *find_attr(Attribute * a, Attr_name name);

#endif /* Evaluator_h */
