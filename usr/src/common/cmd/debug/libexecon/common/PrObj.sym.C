#ident	"@(#)debugger:libexecon/common/PrObj.sym.C	1.14"

#include <stdlib.h>
#include "FileEntry.h"
#include "ProcObj.h"
#include "Process.h"
#include "Seglist.h"
#include "Symtab.h"
#include "Source.h"
#include "Interface.h"
#include "Instr.h"
#include "str.h"

// several routines declare a symbol just to return a null symnol;
// having one instance avoids all those calls to the Symbol constructor
// and destructor
static Symbol null_symbol;

// keep track of most recent function, symtab and source line
// to save needing to look them up constantly.
int
ProcObj::find_cur_src(Iaddr use_pc, int reset_srcfile)
{
	Symbol	func;
	Source	source;
	Iaddr	pcval;
	Iaddr	start_addr;
	long	line;
	const FileEntry	*fentry;

	if (use_pc == (Iaddr)-1)
		pcval = pc;
	else
		pcval = use_pc;
	dot = pcval;	// used for disassembly
	if (!is_core())
		seglist->update_stack(process()->proc_ctl());

	if ((pcval >= lopc) && (pcval < hipc))
	{
		// still in address range of current function
		if (!last_comp_unit.isnull())
		{
			if (!last_comp_unit.source(source))
			{
				printe(ERR_internal, E_ERROR,
					"ProcObj::find_cur_src()", __LINE__);
				last_comp_unit.null();
				current_comp_unit.null();
				set_current_stmt(0, 0, 0);
				return 0;
			}
			if (reset_srcfile)
			{
				// don't reset if compilation unit hasn't changed
				// otherwise %list_file may appear to change in
				// random places
				fentry = source.pc_to_stmt(pcval,
						line, -1, &start_addr);
				set_current_stmt(fentry, line, 0);
				current_comp_unit = last_comp_unit;
				currstmt.first_instr = (pcval == start_addr);
			}
			return 1;
		}
		current_comp_unit.null();
		set_current_stmt(0, 0, 0);
		return 1;
	}
	if ((last_sym = seglist->find_symtab(pcval)) == 0)
	{
		lopc = hipc = 0;
		last_comp_unit.null();
		current_comp_unit.null();
		set_current_stmt(0, 0, 0);
		return 1;
	}
	func = last_sym->find_entry( pcval );
	if (func.isnull())
	{
		// this can occur if you have partial debugging
		// information for an object
		lopc = hipc = 0;
		last_comp_unit.null();
		current_comp_unit.null();
		set_current_stmt(0, 0, 0);
		return 1;
	}
	lopc = func.pc(an_lopc);
	hipc = func.pc(an_hipc);
	if (!last_sym->find_source(pcval, last_comp_unit))
	{
		// this can occur if you have partial debugging
		// information for an object
		last_comp_unit.null();
		current_comp_unit.null();
		set_current_stmt(0, 0, 0);
		return 1;
	}
	if (!last_comp_unit.source(source))
	{
		last_comp_unit.null();
		current_comp_unit.null();
		set_current_stmt(0, 0, 0);
		return 0;
	}
	fentry = source.pc_to_stmt( pcval, line, -1, &start_addr );
	set_current_stmt(fentry, line, 0);
	current_comp_unit = last_comp_unit;
	currstmt.first_instr = (pcval == start_addr);
	return 1;
}

// name of executable or shared object containing a given pc
const char *
ProcObj::object_name( Iaddr addr )
{
	return seglist->object_name( addr );
}

Symtab *
ProcObj::find_symtab( Iaddr addr )
{
	if ((addr >= lopc) && (addr < hipc) && last_sym)
		return last_sym;
	return seglist->find_symtab( addr );
}

Symtab *
ProcObj::find_symtab( const char *name )
{
	return seglist->find_symtab( name );
}

Symbol
ProcObj::find_entry( Iaddr addr )
{
	Symtab *symtab;

	if ((symtab = find_symtab(addr)) == 0)
	{
		return null_symbol;
	}
	return(symtab->find_entry( addr ));
}

Symbol
ProcObj::find_symbol( Iaddr addr )
{
	Symtab *symtab;

	if ((symtab = find_symtab(addr)) == 0)
	{
		return null_symbol;
	}
	return(symtab->find_symbol( addr ));
}

Symbol
ProcObj::find_scope( Iaddr addr )
{
	Symtab *symtab;

	if ((symtab = find_symtab(addr)) == 0)
	{
		return null_symbol;
	}
	return(symtab->find_scope( addr ));
}

int
ProcObj::find_source( const char * name, Symbol & symbol )
{
	return seglist->find_source( name, symbol );
}

Symbol
ProcObj::find_global(const char * name )
{
	return seglist->find_global( name );
}

int
ProcObj::find_next_global(const char *name, Symbol &sym)
{
	return seglist->find_next_global(name, sym);
}

int
ProcObj::create_fake_symbol(const char *name, Symbol &sym)
{
	return seglist->create_fake_symbol(name, sym);
}

void
ProcObj::global_search_complete()
{
	seglist->global_search_complete();
}

Dyn_info *
ProcObj::get_dyn_info(Iaddr addr)
{
	return seglist->get_dyn_info(addr);
}

void
ProcObj::set_current_stmt(const FileEntry *fentry, long line, int is_user_set)
{
	if (fentry != current_srcfile)
	{
		if (flags&L_SRC_IS_USER_SET)
			delete current_srcfile;
		if (is_user_set)
			flags |= L_SRC_IS_USER_SET;
		else
			flags &= ~L_SRC_IS_USER_SET;
	}
	current_srcfile = fentry;
	currstmt.line = line;
	currstmt.pos = 0;
	currstmt.first_instr = 0;
}

FileEntry *
ProcObj::create_fentry(const char *fname)
{
	if (fake_fentry)
		fake_fentry->reset(fname);
	else
		fake_fentry = new FileEntry(fname);
	return fake_fentry;
}

//
// get symbol name
// checks for presence of COFF static shared libraries
//
const char *
ProcObj::symbol_name( Symbol symbol )
{
	const char *name;
	int	offset;
	Iaddr	addr, newaddr;
	Symbol	newsymbol;

	name = symbol.name();
	// if there are no static shared libs, 
	// return symbol name unchanged
	if ( seglist->has_stsl() == 0 || name == 0)
		return name;

	// if name is ".bt?" get real name of function 
	// from the branch table.
	if ( (name[0] == '.') && (name[1] == 'b') && (name[2] == 't') ) 
	{
		offset  = atoi(name+3);	// offset in branch table
		addr = symbol.pc(an_lopc);
		newaddr = instr.fcn2brtbl( addr, offset);
		newsymbol = find_entry(newaddr);
		if ( newsymbol.isnull() == 0 )
			name = newsymbol.name();
	}	
	return name;
}	

Symbol
ProcObj::first_file()
{
	return seglist->first_file();
}

Symbol
ProcObj::next_file()
{
	return seglist->next_file();
}

int
ProcObj::find_stmt( Stmt & stmt, Iaddr addr, const FileEntry *&fentry )
{
	Symtab *	symtab;
	Symbol		symbol;
	Source		source;
	Iaddr		start_addr;

	if ((addr >= lopc) && (addr < hipc) && last_sym)
	{
		symtab = last_sym;
		if (last_comp_unit.isnull())
		{
			stmt.unknown();
			return 1;
		}
		if ( last_comp_unit.source(source) == 0 )
		{
			stmt.unknown();
			printe(ERR_internal, E_ERROR,
				"ProcObj::find_stmt()", __LINE__);
			return 0;
		}
	}
	else
	{
		if (((symtab = seglist->find_symtab(addr)) == 0) ||
			( symtab->find_source(addr, symbol) == 0 ) ||
			( symbol.source(source) == 0 ))
		{
			stmt.unknown();
			return 1;
		}
	}
	fentry = source.pc_to_stmt(addr, stmt.line, -1, &start_addr);
	stmt.first_instr = (start_addr == addr);
	return 1;
}

// If addr is beginning address of a function, return address
// of first line past function prolog (1st real statement).
// If we have no symbolic information, the addr is not the
// beginning of a function, or the function has no executable
// statements, just return addr.
// If we have no line information, use the disassembler's
// fcn_prolog mechanism.

Iaddr
ProcObj::first_stmt(Iaddr addr)
{
	Source	source;
	Symtab	*symtab;
	Symbol	symbol, func;
	Iaddr	naddr, hi;
	long	line;
	int	tmp;
	Iaddr	tmp2;

	find_cur_src((Iaddr)-1, 0);
	if ((addr >= lopc) && (addr < hipc))
	{
		// within range of current function
		if (addr != lopc)
			// not at beginning of function
			return addr;
		if (last_comp_unit.isnull())
		{
			return instr.fcn_prolog(addr, tmp, tmp2, 0);
		}
		if (last_comp_unit.source(source) == 0)
		{
			printe(ERR_internal, E_ERROR, 
				"ProcObj::first_statement", __LINE__);
			return addr;
		}
		hi = hipc;
	}
	else
	{
		if ((symtab = find_symtab(addr)) == 0)
			return addr;
		func = symtab->find_entry( addr );

		if (func.isnull() || (addr != func.pc(an_lopc)))
			// no symbol info or not at beginning
			return addr;

		hi = func.pc(an_hipc);
		if ((symtab->find_source(addr, symbol) == 0)
			|| symbol.isnull() 
			|| (symbol.source(source) == 0))
		{
			return instr.fcn_prolog(addr, tmp, tmp2, 0);
		}
	}
	// we have line info
	// find next statement past beginning
	source.pc_to_stmt(addr+1, line, 1, &naddr); 
	if ((naddr == 0) || (naddr >= hi)) 
	{
		// no such statement within function
		source.pc_to_stmt(addr, line, 0, &naddr);
		if (naddr == addr)
			// only a single line in function
			return addr;
		return instr.fcn_prolog(addr, tmp, tmp2, 0);
	}
	return naddr;
}
