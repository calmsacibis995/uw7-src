#ident	"@(#)debugger:libutil/common/get_addr.C	1.24"

#include "utility.h"
#include "Location.h"
#include "ProcObj.h"
#include "Source.h"
#include "Symbol.h"
#include "Symtab.h"
#include "Interface.h"
#include "Severity.h"
#include "Place.h"
#include "Vector.h"
#include "Expr.h"
#include "FileEntry.h"
#include "str.h"
#include "global.h"
#include <signal.h>

// parse location description and return address associated
// with symbol or line number
//

static int
addr_line(ProcObj *pobj, Frame *f, long line, const FileEntry *fentry, const char *hname,
	Iaddr & addr, Symbol &source, Severity msg)
{
	Source		lineinfo;

	if (source.source( lineinfo ) == 0)
	{
		if (msg != E_NONE)
			printe(ERR_no_source_info, msg, fentry->file_name);
		return 0;
	}
	lineinfo.stmt_to_pc( line, hname ? hname : fentry->file_name, addr );
	if ( addr == 0 )
	{
		if (msg != E_NONE)
			printe(ERR_no_line, msg, fentry->file_name, line);
		return 0;
	}
	return 1;
}

static int
addr_sym(ProcObj *pobj, Iaddr &addr, long off, Symbol &symbol, 
	Severity msg)
{
	Place place;
	if (!symbol.place(place, pobj, pobj->curframe(), symbol.base()))
	{
		if (msg != E_NONE)
			printe(ERR_get_addr, msg, symbol.name());
	}
	else if (place.kind == pRegister || place.kind == pRegister_pair)
	{
		if (msg != E_NONE)
			printe(ERR_get_addr_reg, msg, symbol.name());
	}
	addr = place.addr + off;
	return 1;
}

static int
fixup_overload_list(Vector *vec, ProcObj *pobj, long off, Severity msg,
	Iaddr &addr, Symbol &sym)
{
	Overload_data	*data = (Overload_data *)vec->ptr();
	int		n = vec->size()/sizeof(Overload_data);

	for (; n; n--, data++)
	{
		if (!addr_sym(pobj, data->address, off, data->function, msg))
			return 0;
	}
	data = (Overload_data *)vec->ptr();
	addr = data->address;
	sym = data->function;
	return 1;
}

// This version of get_addr is called in all cases except when the location includes
// an associated object (L_HAS_OBJECT). 
int
get_addr(ProcObj *&pobj, Location *location, Iaddr &addr, 
	Symtype stype, Symbol &sym, Severity msg, Vector **pv)
{
	char		*file;
	char		*header;
	char		*func;
	Symtab		*symtab = 0;
	ProcObj		*l_pobj;
	Frame		*f;
	long		off;
	unsigned long	ul;
	Symbol		source;
	const FileEntry	*fentry = 0;
	int		is_restricted;

	sym.null();

	if (location->get_flags() & L_HAS_OBJECT)
	{
		if (msg != E_NONE)
			printe(ERR_bad_loc, E_ERROR);
		return 0;
	}

	if (!pobj || !location
		|| location->get_pobj(l_pobj) == 0
		|| location->get_offset(off) == 0)
		return 0;

	if (l_pobj)
		pobj = l_pobj;
	f = pobj->curframe();
	if (location->get_file(pobj, f, file, header, source) == 0
		|| location->get_symtab(pobj, f, symtab) == 0)
	{
		printe(ERR_bad_loc, E_ERROR);
		return 0;
	}
	is_restricted = !source.isnull();

	LDiscrim ltype = location->get_type();
	if (ltype != lk_addr)
	{
		if (source.isnull())
			current_loc(pobj, f, source, &fentry);
		if (ltype == lk_stmt)
		{
			if (header || file)
			{
				fentry = find_file_entry(pobj, source, file, header, msg);
				if (!fentry || !fentry->file_name)
					return 0;
			}
			else if (!fentry || !fentry->file_name)
			{
				if (msg != E_NONE)
					printe(ERR_no_cur_src_obj, msg, pobj->obj_name());
				return 0;
			}
		}
		else
		{
			if (header || file)
			{
				fentry = find_file_entry(pobj, source, file, header,
					header ? msg : E_NONE);
			}
			if (header && !fentry)
				return 0;
		}
	}

	switch (ltype) 
	{
	case lk_addr:
		if (location->get_addr(pobj, f, ul) == 0)
			return 0;
		addr = (Iaddr)(ul + off);
		if (symtab)
			addr += symtab->ss_base;
		return 1;
	case lk_stmt:
		if (location->get_line(pobj, f, ul) == 0)
			return 0;
		if (!file && symtab)
		{
			printe(ERR_bad_loc, E_ERROR);
			return 0;
		}
		if (pv && !file && (header || strcmp(fentry->file_name, source.name()) != 0))
		{
			const char *header_file = header ? header : fentry->file_name;
			if (!query(QUERY_set_break_on_all, 1, header_file, ul))
				return addr_line(pobj, f, (long)ul, fentry, header_file, addr, source, msg);

			Overload	ov;
			Source		lineinfo;

			sigrelse(SIGINT);
			ov.init(pobj, pv);
			for (Symbol scope = pobj->first_file(); !scope.isnull();
				scope = pobj->next_file())
			{
				if (prismember(&interrupt, SIGINT))
				{
					sighold(SIGINT);
					return 0;
				}

				if (scope.file_table(lineinfo)
					&& (fentry = lineinfo.find_header(header_file)) != 0
					&& addr_line(pobj, f, (long)ul, fentry, header_file,
						addr, scope, E_NONE))
				{
					Location *loc = new Location(*location);
					loc->set_file((char *)scope.name());
					ov.add_choice(loc, addr);
				}
			}
			sighold(SIGINT);
			if (ov.is_overloaded())
			{
				ov.use_all_choices(pv);
				return 1;
			}
			else
				return addr_line(pobj, f, (long)ul, fentry, header_file, addr, source, msg);
		}
		else
			return addr_line(pobj, f, (long)ul, fentry, header, addr, source, msg);
	case lk_fcn:
		if (location->get_func(pobj, f, func) == 0)
			return 0;
		sym = find_symbol(source, func, pobj, pobj->pc_value(), 
			symtab, stype, header ? fentry : 0);
		if (sym.isnull())
		{
			if (msg != E_NONE)
				printe(ERR_no_entry, msg, func);
			return 0;
		}
		if (!resolve_overloading(pobj, func, symtab, sym, pv, is_restricted))
			return 0;

		// pv && *pv will only be true when the user selects all choices when
		// trying to set a breakpoint on an overloaded function.
		// go through the list and fill in the address of each function.
		// also, to be safe, get_addr always fills in and returns addr and sym,
		// even when returning a vector of symbols
		if (pv && *pv)
		{
			return fixup_overload_list(*pv, pobj, off, msg, addr, sym);
		}
		else if (pv && sym.tag() == t_subroutine
			&& !sym.has_attribute(an_external) && !file && fentry)
		{
			// may be static function from a header file
			long		file;
			long		line;
			Source		lineinfo;
			const FileEntry	*sym_fentry;
			if (!sym.file_and_line(file, line)
				|| !source.file_table(lineinfo)
				|| (sym_fentry = lineinfo.find_header(file)) == 0
				|| sym_fentry->compare(source))
			{
				return addr_sym(pobj, addr, off, sym, msg);
			}

			if (!query(QUERY_search_for_all_funcs, 1, sym.name(),
						sym_fentry->file_name))
			{
				printe(ERR_using_static, E_WARNING, source.name(),
							fentry->file_name, sym.name());
				return addr_sym(pobj, addr, off, sym, msg);
			}

			Overload	ov;
			ov.init(pobj, pv);
			sigrelse(SIGINT);
			for (Symbol scope = pobj->first_file(); !scope.isnull();
				scope = pobj->next_file())
			{
				if (prismember(&interrupt, SIGINT))
				{
					sighold(SIGINT);
					return 0;
				}

				Source	lineinfo;
				if (!scope.file_table(lineinfo)
					|| (fentry = lineinfo.find_header(sym_fentry->file_name)) == 0)
					continue;
				Symbol other_sym = find_symbol(scope, func, pobj,
							0, symtab, stype, fentry);
				if (!other_sym.isnull())
				{
					Location *loc = new Location(*location);
					loc->set_file((char *)scope.name());
					ov.add_choice(other_sym.name(), other_sym, 0, loc);
				}
			}
			sighold(SIGINT);
			if (ov.is_overloaded())
			{
				ov.use_all_choices(pv);
				return fixup_overload_list(*pv, pobj, off, msg, addr, sym);
			}
			else
				return addr_sym(pobj, addr, off, sym, msg);
		}
		else
		{
			if (sym.tag() == t_subroutine
				&& !sym.has_attribute(an_external)
				&& !file && fentry && !fentry->compare(source))
			{
				printe(ERR_using_static, E_WARNING, source.name(),
							fentry->file_name, sym.name());
			}
			return addr_sym(pobj, addr, off, sym, msg);
		}
	case lk_none:
	default:
		printe(ERR_internal, E_ERROR, "get_addr", __LINE__);
		return 0;
	}
}

// This version of get_addr is called only when the Location has an associated object,
// currently that is only from StopEvent, when the user creates a stop event on
// an object/function pair (e.g. stop ptr->f).
int
get_addr(ProcObj *&pobj, Location *location, Iaddr &addr, Symbol &sym,
	const char *&expr, Severity msg, Vector **pv)
{
	char		*func;
	ProcObj		*l_pobj;
	long		off;
	Vector		*v;

	sym.null();

	if (!pobj || !location
		|| location->get_pobj(l_pobj) == 0
		|| location->get_offset(off) == 0)
		return 0;

	if (!pv || location->get_type() != lk_fcn || 
		!(location->get_flags() & L_HAS_OBJECT))
	{
		printe(ERR_internal, E_ERROR, "get_addr", __LINE__);
		return 0;
	}

	if (l_pobj)
		pobj = l_pobj;
	if (location->get_func(pobj, pobj->curframe(), func) == 0)
		return 0;

	Expr	*exp = new_expr(func, pobj);
	exp->objectStopExpr(pv);
	if (!exp->eval(pobj))	// evaluation fills in vector
	{
		delete exp;
		return 0;
	}
	v = *pv;
	delete exp;

	if (!fixup_overload_list(v, pobj, off, msg, addr, sym))
		return 0;
	expr = ((Overload_data *)v->ptr())->expression;
	return 1;
}
