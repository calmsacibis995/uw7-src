#ident	"@(#)debugger:libsymbol/common/Symtab.C	1.5"
#include	"Evaluator.h"
#include	"Symtab.h"
#include	"Symtable.h"
#include	<string.h>

Symbol
Symtab::find_entry( Iaddr addr )
{
	Symbol		symbol;

	if ( symtable != 0 )
	{
		symbol = symtable->find_entry( addr - ss_base );
		symbol.ss_base = ss_base;
	}
	return symbol;
}

Symbol
Symtab::find_symbol( Iaddr addr )
{
	Symbol		symbol;

	if ( symtable != 0 )
	{
		symbol = symtable->find_symbol( addr - ss_base );
		symbol.ss_base = ss_base;
	}
	return symbol;
}

Symbol
Symtab::find_scope ( Iaddr addr )
{
	Symbol		symbol;

	if ( symtable != 0 )
	{
		symbol = symtable->find_scope( addr - ss_base );
		symbol.ss_base = ss_base;
	}
	return symbol;
}

Symbol
Symtab::first_symbol()
{
	Symbol	symbol;

	if ( symtable != 0 )
	{
		symbol = symtable->first_symbol();
		symbol.ss_base = ss_base;
	}
	return symbol;
}

int
Symtab::find_source( Iaddr pc, Symbol & symbol )
{
	int	result = 0;

	if ( symtable != 0 )
	{
		result = symtable->find_source( pc - ss_base, symbol );
		symbol.ss_base = ss_base;
	}
	return result;
}

int
Symtab::find_source( const char * name, Symbol & symbol )
{
	int	result = 0;

	if ( symtable != 0 )
	{
		result = symtable->find_source( name, symbol );
		symbol.ss_base = ss_base;
	}
	return result;
}

Symbol
Symtab::find_global( const char * name )
{
	Symbol	symbol;

	if ( symtable != 0 )
	{
		symbol = symtable->find_global( name );
		symbol.ss_base = ss_base;
	}
	return symbol;
}

NameEntry *
Symtab::first_global()
{
	if ( symtable != 0 )
	{
		return symtable->first_global();
	}
	else
	{
		return 0;
	}
}

NameEntry *
Symtab::next_global()
{
	if ( symtable != 0 )
	{
		return symtable->next_global();
	}
	else
	{
		return 0;
	}
}

Symbol
Symtab::global_symbol( NameEntry * n )
{
	Symbol	symbol;

	if ( symtable != 0 )
	{
		symbol = symtable->global_symbol( n );
		symbol.ss_base = ss_base;
	}
	return symbol;
}

int
Symtab::find_next_global(const char *name, Symbol &sym)
{
	if (symtable)
	{
		sym.ss_base = ss_base;
		return symtable->find_next_global(name, sym);
	}
	return 0;
}

int
Symtab::create_fake_symbol(const char *name, Symbol &sym)
{
	if (symtable)
	{
		sym.ss_base = ss_base;
		return symtable->create_fake_symbol(name, sym);
	}
	return 0;
}

void
Symtab::global_search_complete()
{
	if (symtable)
		symtable->global_search_complete();
}
