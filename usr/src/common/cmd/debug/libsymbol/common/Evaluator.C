#ident	"@(#)debugger:libsymbol/common/Evaluator.C	1.23"
#include "Evaluator.h"
#include "Dwarf2.h"
#include "Dwarf1.h"
#include "Coffbuild.h"
#include "Syminfo.h"
#include "Machine.h"
#include "Object.h"
#include "ELF.h"
#include "Coff.h"
#include "global.h"
#include "Interface.h"
#include "Proctypes.h"
#include "Lineinfo.h"
#include "FileEntry.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define NOMORE	(-1)	/* for file offsets */

Evaluator::Evaluator( int fd, Object *obj )
{
	fdesc = fd;
	first_record = 0;
	elf_record = 0;
	no_elf_syms = 1;
	current_entry = 0;
	dwarf1build = 0;
	dwarf2build = 0;
	coffbuild = 0;
	elfbuild = 0;
	next_file = 0;

	Sectinfo sect;
	switch( file_type = obj->file_format() ) {
	case ff_elf:
		current_build = elfbuild = new Elfbuild((ELF *)obj);
		global_form = af_elfoffs;
		if (obj->getsect(s_debug, &sect))
		{
			current_build = dwarf1build = new Dwarf1build((ELF *)obj, this);
			global_form = af_dwarfoffs;
		}
		if (obj->getsect(s_dwarf2_info, &sect))
		{
			current_build = dwarf2build = new Dwarf2build((ELF *)obj, this);
			global_form = af_dwarf2offs;
		}
		break;
	case ff_coff:
		current_build = coffbuild = new Coffbuild((Coff *)obj);
		global_form = af_coffrecord;
		break;
	}
	next_disp = current_build->globals_offset();
}

Attribute *
Evaluator::first_file()
{
	offset_t	first_offset;
	offset_t	elf_offset;

	if ( first_record != 0 )
	{
		return first_record;
	}
	if ( file_type == ff_coff )
	{
		first_offset = coffbuild->first_symbol();
		first_record = coffbuild->make_record(first_offset, WANT_FILE);
		return first_record;
	}
	else if ( file_type != ff_elf )
	{
		return 0;
	}

	if ((elf_offset = elfbuild->first_symbol()) != 0 )
	{
		elf_record = elfbuild->make_record( elf_offset );
	}
	if (dwarf2build)
	{
		first_record = dwarf2build->make_record(dwarf2build->first_file(),
							WANT_FILE);
	}
	else if (dwarf1build && (first_offset = dwarf1build->first_file()) != 0 )
	{
		first_record = dwarf1build->make_record(first_offset);
	}
	else
		first_record = elf_record;
	return first_record;
}

Attribute *
Evaluator::arc( Attribute * attrlist, Attr_name attrname )
{
	Attribute *	a;

	if ( (a = find_attr( attrlist, attrname )) == 0 )
	{
		return 0;
	}
	else if ( a->form == af_symbol )
	{
		return a;
	}
	else if ( a->name == an_child )
	{
		return add_children( a, attrlist );
	}
	else if ( a->name == an_parent )
	{
		Build *build = get_build_from_form((Attr_form)a->form);
		if (!build)
			return 0;
		return add_parent(attrlist, build->make_record(a->value.word,
							(a->form == af_dwarf2file)));
	}
	else if (a->name == an_specification)
	{
		a->value.symbol = build_parent_chain(attrlist, a);
		a->form = af_symbol;
		return a;
	}
	else
	{
		return add_node( a );
	}
}

Attribute *
Evaluator::build_parent_chain(Attribute *current, Attribute *a)
{
	Attribute	*parent;
	Attribute	*child;
	Attribute	*entry = 0;
	Build		*build;

	if ((build = get_build_from_form((Attr_form)a->form)) == 0)
		return 0;
	if ((entry = build->find_record(a->value.word)) != 0)
		return entry;

	// MORE - there ought to be a more efficient way to do this!!
	// search down through the compiliation unit's children to find
	// the nearest enclosing scope that has already been scanned
	parent = first_file();
	for (;;)
	{
		Attribute	*sib_offset;

		if (!parent)
			return 0;
		if ((sib_offset = find_attr(parent, an_sibling_offset)) == 0
			|| sib_offset->value.word < a->value.word)
		{
			Attribute	*sib;
			if ((sib = arc(parent, an_sibling)) == 0)
				return 0;
			parent = sib->value.symbol;
		}
		else
		{
			// searched-for entry is a child of this scope
			if ((child = find_attr(parent, an_child)) == 0)
				return 0;
			if (child->form == af_symbol)
				parent = child->value.symbol;
			else
				break;
		}
	}

	// create children for enclosing scope(s), until the needed symbol
	// is reached
	for (;;)
	{
		parent = add_children(child, parent, a->value.word, &entry);
		if (entry)
			return entry;
		if ((child = find_attr(parent, an_child)) == 0
			|| child->form == af_symbol)
			break;
	}
	return 0;
}

Attribute *
Evaluator::add_node( Attribute * a )
{
	long	offset = a->value.word;

	switch ( a->form ) {
	case af_coffrecord:
		a->value.symbol = coffbuild->make_record( offset );
		break;
	case af_cofffile:
		a->value.symbol = coffbuild->make_record( offset, WANT_FILE );
		break;
	case af_dwarf2offs:
		a->value.symbol = dwarf2build->make_record(offset);
		break;

		// hook dwarf and elf records together, so that debug
		// can find all symbols
	case af_dwarf2file:
		a->value.symbol = dwarf2build->make_record(offset, WANT_FILE);
		if (!a->value.symbol && a->name == an_sibling
			&& offset >= dwarf2build->last_entry())
		{
			if (dwarf1build)
			{
				a->value.symbol
					= dwarf1build->make_record(dwarf1build->first_file());
			}
			else
			{
				a->value.symbol = elf_record;
			}
		}
		break;
	case af_dwarfoffs:
		a->value.symbol = dwarf1build->make_record( offset );
		if (!a->value.symbol && a->name == an_sibling
			&& offset >= dwarf1build->last_entry())
			a->value.symbol = elf_record;
		break;

	case af_elfoffs:
		a->value.symbol = elfbuild->make_record( offset );
		break;
	default:
		return a;
	}
	a->form = af_symbol;
	return a;
}

Attribute *
Evaluator::add_parent( Attribute * b, Attribute * ancestor )
{
	Attribute *	p;

	if( ancestor == 0 )
	{
		return 0;
	}

	if ( (p = find_attr( b, an_parent )) != 0 )
	{
		p->value.symbol = ancestor;
		p->form = af_symbol;
	}
	return p;
}

Attribute *
Evaluator::add_children( Attribute * a, Attribute * ancestor, offset_t wanted,
	Attribute **entry )
{
	long		offset, limit;
	Attribute	*b, *x, *parent = 0;

	offset = a->value.word;
	if ((b = find_attr( ancestor, an_scansize )) != 0 )
	{
		limit = offset + b->value.word;
	}
	else
	{
		return 0;
	}
	x = a;
	Build *build;
	while ( x != 0 )
	{
		if (x->form == af_symbol)
		{
			x = find_attr(x->value.symbol, an_sibling);
			continue;
		} 
		offset = x->value.word;
		if ( (offset < limit)
			&& (build = get_build_from_form((Attr_form)x->form)) != 0)
		{
			b = build->make_record( offset );
			(void)add_parent( b, ancestor );
			x->value.symbol = b;
			x->form = af_symbol;
			if (entry)
			{
				if (offset == wanted)
					*entry = b;
				else if (offset < wanted)
					parent = b;
			}
			x = find_attr( b, an_sibling );
		}
		else
		{
			x->value.symbol = 0;
			x->form = af_symbol;
			break;
		}
	}
	if (entry)
		return parent;
	return a;
}

// Search for an attribute first in the given list.
// If not found but the list does contain an an_specification or an_abstract attribute,
// follow the link and search there.
// This is used only for non-arc attributes (i.e., not parent, sibling, etc.)
Attribute *
Evaluator::find_attribute(Attribute *attrlist, Attr_name attr_name)
{
	Attribute	*a;
	Attribute	*spec;
	Attribute	*abstract;

	if ((a = find_attr(attrlist, attr_name)) != 0
		|| attr_name == an_abstract)
		return a;
	abstract = arc(attrlist, an_abstract);
	if (abstract && (a = find_attr(abstract->value.symbol, attr_name)) != 0)
		return a;
	if ((spec = arc(abstract ? abstract->value.symbol : attrlist, an_specification)) == 0)
		return 0;
	if (attr_name == an_specification)
		return spec->value.symbol;
	return find_attr(spec->value.symbol, attr_name);
}

int
Evaluator::has_attribute(Attribute *attrlist, Attr_name attr_name)
{
	return (find_attribute(attrlist, attr_name) != 0);
}

Attribute *
Evaluator::attribute( Attribute * attrlist, Attr_name attr_name )
{
	Attribute *	a;
	Attribute	*include_files;

	if ( (a = find_attribute(attrlist,attr_name)) == 0 )
		return 0;
	if (attr_name == an_specification && a->form != af_symbol)
	{
		build_parent_chain(attrlist, a);
	}
	else if (attr_name == an_abstract && a->form != af_symbol)
	{
		add_node(a);
	}
	switch (a->form)
	{
		case af_coffrecord:
			a->value.symbol = coffbuild->make_record(a->value.word);
			a->form = af_symbol;
			break;
		case af_coffline:
			if (attr_name == an_lineinfo)
			{
				a->value.lineinfo = coffbuild->line_info(a->value.word);
				a->form = af_lineinfo;
				include_files = find_attr(attrlist, an_file_table);
			}
			else
				include_files = a;
			goto filetable_fixup;
		case af_dwarfoffs:
			a->value.symbol = dwarf1build->make_record(a->value.word);
			a->form = af_symbol;
			break;
		case af_dwarfline:
			if (attr_name == an_lineinfo)
			{
				a->value.lineinfo = dwarf1build->line_info(a->value.word);
				a->form = af_lineinfo;
				include_files = find_attr(attrlist, an_file_table);
			}
			else
				include_files = a;
filetable_fixup:
			if (include_files->form != af_file_table)
			{
				FileTable	*table = new FileTable;
				include_files->value.file_table = table;
				table->filecount = 2;
				table->files = new FileEntry[2];
				include_files->form = af_file_table;
				
				// Assumes 1) lineinfo associated only with
				// compilation units, 2) all compilation units have names
				Attribute *nm = attribute(attrlist, an_name);
				if (nm)
				{
					table->files[1].file_name
						= nm->value.name;
				}
			}
			break;
		case af_dwarf2offs:
			a->value.symbol = dwarf2build->make_record(a->value.word);
			a->form = af_symbol;
			break;
		case af_dwarf2file:
			a->value.symbol = dwarf2build->make_record(a->value.word, WANT_FILE);
			a->form = af_symbol;
			break;
		case af_dwarf2line:
		{
			// handles either an_lineinfo and an_file_table
			// in either case, build the file table,
			// since file_table is a subset of lineinfo
			Attribute	*comp_dir = attribute(attrlist, an_comp_dir);

			if (attr_name == an_lineinfo)
			{
				include_files = find_attr(attrlist, an_file_table);
				if (include_files->form != af_file_table)
				{
					include_files->value.file_table
						= dwarf2build->process_includes(
								include_files->value.word,
								comp_dir ? comp_dir->value.name : 0);
					include_files->form = af_file_table;
				}
				a->value.lineinfo = dwarf2build->line_info(a->value.word,
							include_files->value.file_table->files);
				a->form = af_lineinfo;
			}
			else
			{
				a->value.file_table = dwarf2build->process_includes(
							a->value.word,
							comp_dir ? comp_dir->value.name : 0);
				a->form = af_file_table;
			}
		}
			break;
		case af_coffpc:
			printe(ERR_debug_entry, E_WARNING);
			break;
		case af_elfoffs:
			a->value.symbol = elfbuild->make_record(a->value.word);
			a->form = af_symbol;
			break;
	}
	return a;
}

int
Evaluator::get_syminfo(Syminfo &syminfo)
{
	if (next_disp == (offset_t)NOMORE || !current_build)
		return 0;

	if (!next_disp && next_file != 0 && next_file != (offset_t)NOMORE)
	{
		next_disp = next_file;
	}
	if (current_build->out_of_range(next_disp))
	{
		// searched all the globals, go to the next Build type,
		// if one is available, so that if the symbol is
		// not found in DWARF2, it may be found in DWARF1 or ELF
		if (current_build == dwarf2build && dwarf1build)
		{
			current_build = dwarf1build;
			global_form = af_dwarfoffs;
			next_disp = current_build->globals_offset();
		}
		else if (elfbuild && current_build != elfbuild)
		{
			global_form = af_elfoffs;
			current_build = elfbuild;
			next_disp = current_build->globals_offset();
		}
		else
		{
			next_disp = (offset_t)NOMORE;
			current_build = 0;
			return 0;
		}
	}
	return current_build->get_syminfo(next_disp, syminfo,
						(next_disp == next_file));
}

// If name is 0, read all global entries
NameEntry *
Evaluator::get_global(const char *name)
{
	Syminfo	syminfo;

	while (get_syminfo(syminfo) != 0)
	{
		if (prismember(&interrupt, SIGINT))
			return 0;

		offset_t	old_disp = next_disp;
		const char	*nom;

		// We must allow for special case of hidden global
		// symbols in a shared library; these symbols are
		// global to the library, but do not appear in
		// the global part of the symbol table.  They
		// appear with local binding as children of
		// a dummy file named "_fake_hidden".
		if (current_build == elfbuild)
		{
			old_disp = next_disp;
			next_disp = syminfo.sibling;
			nom = elfbuild->get_name( syminfo.name );
			if (syminfo.type == st_file &&
				(strcmp(nom, "_fake_hidden") == 0))
			{
				elfbuild->set_special(next_disp);
				continue;
			}
			if (syminfo.type != st_object &&
				syminfo.type != st_func)
			{
				continue;
			}
			if (syminfo.bind == sb_local
				|| syminfo.bind == sb_none
				|| !syminfo.resolved)
			{
				if (syminfo.type == st_file)
					next_file = syminfo.sibling;
				continue;
			}
			// if here, we have a resolved global or
			// weak symbol of type function or object
			// or a special hidden symbol
		}
		else
		{
			nom = (const char *)syminfo.name;
			if ( syminfo.type == st_file )
			{
				next_disp = syminfo.child;
				if ((next_file = syminfo.sibling) == 0)
					next_file = (offset_t)NOMORE;
				continue;
			}
			if ( syminfo.bind != sb_global || !syminfo.resolved )
			{
				next_disp = syminfo.sibling;
				continue;
			}
			next_disp = syminfo.sibling;
			if (current_build != coffbuild && syminfo.type == st_func
				&& ((Dwarfbuild *)current_build)->language() == CPLUS)
			{
				// replace simple function name with full
				// function signature (from mangled names in
				// ELF symbol table)
				Attribute	*attr;
				Attribute	*attr_name;
				if ((attr = lookup_addr(syminfo.lo)) != 0
					&& (attr_name = attribute(attr, an_name)) != 0
					&& attr_name->form == af_stringndx)
				{
					nom = attr_name->value.name;
				}
			}
		}
		NameEntry *n = namelist.add( nom, old_disp, global_form );
		if (n && name && n->cmpName(name) == 0)
		{
			return n;
		}
	}
	return 0;
}

NameEntry *	
Evaluator::first_global()
{
	if (next_disp != (offset_t)NOMORE)
	{
		// first time through, create the NameEntry table
		get_global(0);
	}

	current_entry = (NameEntry*)namelist.tfirst();
	return current_entry;
}

NameEntry *
Evaluator::next_global()
{
	if (next_disp != (offset_t)NOMORE)
	{
		// first time through, create the NameEntry table
		get_global(0);
	}

	// assumes that first_global/next_global loop may be interrupted by
	// find_next_global search, but not vice versa
	pop_stack();

	if ( current_entry == 0 )
	{
		return 0;
	}
	current_entry = (NameEntry*)(current_entry->next());
	return current_entry;
}

void
Evaluator::push_stack()
{
	if (current_entry)
	{
		// previous search not finished - push on stack
		EntryStack	*entry = new EntryStack(current_entry);
		entry_stack.push((Link *)entry);
	}
}

void
Evaluator::pop_stack()
{
	if (!current_entry && !entry_stack.is_empty())
	{
		// returned to previous search, pop stack
		EntryStack	*prev_entry;
		prev_entry = (EntryStack *)entry_stack.pop();
		current_entry = prev_entry->entry;
		delete prev_entry;
	}
}

NameEntry *
Evaluator::find_next_global(const char *name, Attribute *attr)
{
	if (next_disp != (offset_t)NOMORE)
	{
		// first time through, create the NameEntry table
		get_global(0);
	}

	if (!attr)
	{
		NameEntry	node;

		push_stack();
		node.namep = name;
		current_entry = (NameEntry *)namelist.tlookup(node);
		while (current_entry && (NameEntry *)current_entry->prev()
			&& ((NameEntry *)current_entry->prev())->cmpName(name) == 0)
			current_entry = (NameEntry *)current_entry->prev();
		return current_entry;
	}
	pop_stack();
	if (current_entry)
	{
		if ((NameEntry *)current_entry->next()
			&& ((NameEntry *)current_entry->next())->cmpName(name) == 0)
			current_entry = (NameEntry *)(current_entry->next());
		else
			current_entry = 0;
	}
	return current_entry;
}

// hook a debugger-created symbol onto the end of the list of symbols
NameEntry *
Evaluator::create_fake_symbol(const char *name, Attribute *attr)
{
	return namelist.add(name, attr);
}

Attribute *
Evaluator::evaluate(NameEntry *n)
{
	if (n == 0)
	{
		return 0;
	}

	Build	*build = get_build_from_form((Attr_form)n->form);
	if (build == 0)
	{
		return n->value.symbol;
	}
	n->form = af_symbol;
	n->value.symbol = build->make_record(n->value.word, (n->form == af_dwarf2file));
	return n->value.symbol;
}

// Search for a NameEntry with the specified name
Attribute *
Evaluator::find_global( const char * name )
{
	NameEntry *	n;
	NameEntry	node;

	node.namep = name;
	n = (NameEntry*)namelist.tlookup(node);
	if ( n != 0 || (n = get_global(name)) != 0 )
	{
		return evaluate(n);
	}
	return 0;
}

// Search for a AddrEntry containing a specified address
// Assumes graph has already been searched; uses only addrlist
Attribute *
Evaluator::lookup_addr( Iaddr addr )
{
	AddrEntry	node;
	AddrEntry *	a;
	long		offset;
	Syminfo		syminfo;
	Build *		build = coffbuild ? (Build *)coffbuild : (Build *)elfbuild;
	Attr_form	form = coffbuild ? af_coffrecord : af_elfoffs;

	node.loaddr = addr;
	node.hiaddr = addr;
	if ( no_elf_syms )
	{
		long	next_file = 0;

		no_elf_syms = 0;
		offset = build->globals_offset();
		while ( build->get_syminfo( offset, syminfo, 0 ) != 0 )
		{
			if (syminfo.bind == sb_local &&
				(syminfo.type == st_object ||
				 syminfo.type == st_func) &&
				 syminfo.resolved)
                        {
                                addrlist.add( syminfo.lo, syminfo.hi, offset, form );
			}
			// allows undefined global functions if
			// their low pc is non-zero == plt entries
			else if ( syminfo.bind == sb_global &&
				(((syminfo.type == st_object) 
				&& syminfo.resolved) ||
				((syminfo.type == st_func) &&
				(syminfo.resolved || 
				(syminfo.lo != 0)))) )
                        {
                                addrlist.add( syminfo.lo, syminfo.hi, offset, form );
			}
			offset = syminfo.sibling;
			if (coffbuild)
			{
				if (syminfo.type == st_file)
				{
					next_file = offset;
					offset = syminfo.child;
				}
				else if (offset == 0)
				{
					offset = next_file;
					next_file = 0;
				}
			}
		}
		addrlist.complete();
	}
	a = (AddrEntry*)addrlist.tlookup(node);
	if ( a == 0 )
	{
		return 0;
	}
	if ( a->form == af_symbol )
	{
		return a->value.symbol;
	}
	if ( a->form == form )
	{
		Attribute	*attr, *list;
		a->value.symbol = list =
			build->make_record(a->value.word);
		a->form = af_symbol;

		// for functions, check for lowpc == hipc;
		// this means we have no size information;
		// if this is true use hiaddr from addrlist
		// for the hi pc
		attr = attribute(list, an_tag);
		if (attr && attr->value.tag == t_subroutine)
		{
			Attribute	*lowpc, *hipc;
			lowpc = attribute(list, an_lopc);
			hipc = attribute(list, an_hipc);
			if (lowpc && hipc
				&& (lowpc->value.addr == hipc->value.addr))
			{
				hipc->value.addr = a->hiaddr;
			}
		}
		return list;
	}
	return 0;
}

Build *
Evaluator::get_build_from_form(Attr_form f)
{
	switch (f)
	{
	case af_coffrecord:	return coffbuild;
	case af_dwarfoffs:	return dwarf1build;
	case af_dwarf2file:
	case af_dwarf2offs:	return dwarf2build;
	case af_elfoffs:	return elfbuild;
	default:		return 0;
	}
}

Attribute *
find_attr(Attribute * a, Attr_name name)
{
	register Attribute *	p;
	Attribute *		rec;
	static long		last;

	if ( a != 0 )
	{
		p = rec = a;
		last = p->value.word - 1;
		rec[last].name = name;
		while (p->name != name) p++ ;
		rec[last].name = an_nomore;
		return (p == rec+last) ? 0 : p ;
	}
	else
		return 0;
}
