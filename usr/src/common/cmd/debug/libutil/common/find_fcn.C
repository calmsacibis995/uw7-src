#ident	"@(#)debugger:libutil/common/find_fcn.C	1.6"

#include "utility.h"
#include "ProcObj.h"
#include "Source.h"
#include "Symbol.h"
#include "Symtab.h"
#include "Frame.h"
#include "Interface.h"
#include "Expr.h"
#include "Vector.h"

// returns file and line number for given function

const FileEntry *
find_fcn(ProcObj *pobj, Symtab *symtab, Symbol &scope, char *func,
	long &line)
{
	Symbol		symbol;
	Iaddr		loaddr;
	Source		source;
	Frame		*f = pobj->curframe();

	if ( func == 0 || pobj == 0 )
	{
		printe(ERR_internal, E_ERROR, "find_fcn", __LINE__);
		return 0;
	}
	symbol = find_symbol(scope, func, pobj, f->pc_value(), symtab,
		st_func);
	if ( symbol.isnull() )
	{
		printe(ERR_no_entry, E_ERROR, func);
		return 0;
	}

	if (!resolve_overloading(pobj, func, symtab, symbol, 0, !scope.isnull()))
		return 0;

	loaddr = symbol.pc(an_lopc);
	if ((!symtab && ((symtab = pobj->find_symtab( loaddr )) == 0))
		|| symtab->find_source( loaddr, symbol ) == 0
		|| symbol.source( source ) == 0 )
	{
		printe(ERR_no_source_info, E_ERROR, func);
		return 0;
	}
	if (scope.isnull())
	{
		scope = symbol;
		while (!scope.isnull() && !scope.isSourceFile())
			scope = scope.parent();
	}
	return source.pc_to_stmt( loaddr, line, -1);
}
