#ident	"@(#)debugger:libcmd/common/Location.C	1.12"

#include "Location.h"
#include "ProcObj.h"
#include "Interface.h"
#include "Proglist.h"
#include "Dbgvarsupp.h"
#include "Buffer.h"
#include "str.h"
#include "utility.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define VAR_MASK	0x7f

Location::Location(Location &l)
{ 
	memcpy( (void *)this, 
		(void *)&l, sizeof(Location) ); 
	if (flags & L_DELETE_NAME)
		locn.l_name = makestr(l.locn.l_name);
}

Location::~Location()
{
	if (flags & L_DELETE_NAME)
		delete locn.l_name;
}

int
Location::get_str_val(ProcObj *p, Frame *f, const char *name, char *&value)
{
	char	*var = 0;

	// Get variable textual value.
	var_table.set_context(p,f,1,1,1);
	if (var_table.Find((char *)name))
		var = var_table.Value();
	if (!var)
	{
		printe(ERR_user_var_defined, E_ERROR, name);
		return 0;
	}
	value = new(char[strlen(var) + 1]);
	strcpy(value, var);
	return 1;
}

int
Location::get_int_val(ProcObj *p, Frame *f, const char *name, 
	unsigned long &lval)
{
	char	*ptr;
	char	*var_value = 0;

	// Get variable textual value.
	var_table.set_context(p,f,1,1,1);
	if (var_table.Find((char *)name))
		var_value = var_table.Value();
	if (!var_value)
	{
		printe(ERR_user_var_defined, E_ERROR, name);
		return 0;
	}
	lval = strtoul(var_value, &ptr, 0);
	if (*ptr)
	{
		printe(ERR_bad_number, E_ERROR, name);
		return 0;
	}
	return 1;
}

LDiscrim
Location::get_type()
{
	if ((kind & VAR_MASK) == L_UNKNOWN)
	{
		// should have symbolic name - parse
		char	*var_val;

		if (!locn.l_name || !get_str_val(0, 0, locn.l_name, var_val))
			return lk_none;
		if (isdigit(*var_val))
		{
			// line or address
			if (*var_val == '0')
			{
				// assume hex or octal
				if (file_name)
					return lk_none;
				
				kind |= L_ADDR;
				return lk_addr;
			}
			else
			{
				long l = strtol(var_val, 0, 0);
				if ((l > 999999) && !file_name)
				{
					// assume addr
					kind |= L_ADDR;
					return lk_addr;
				}
				// assume decimal
				kind |= L_LINE;
				return lk_stmt;
			}
		}
		else
		{
			kind |= L_FUNC;
			return lk_stmt;
		}
		
	}
	switch(kind & VAR_MASK)
	{
		case L_LINE:
			return lk_stmt;
		case L_ADDR:
			if (file_name)
				return lk_none;
			return lk_addr;
		case L_FUNC:
			return lk_fcn;
		default:
			return lk_none;
	}
}

int
Location::get_pobj(ProcObj *&p)
{
	p = 0;
	check_pobj(p);
	if (pobj && !p && ((p = proglist.find_pobj(pobj)) == 0))
	{
		printe(ERR_object_unknown, E_ERROR, pobj);
		return 0;
	}
	return 1;
}

void
Location::check_pobj(ProcObj *&p)
{
	if (!(flags&L_CHECK_types) || p || !pobj)
		return;

	// called from get_pobj = first string is pobj
	if ((p = proglist.find_pobj(pobj)) == 0)
	{
		// no procobj
		// 1 or more names - libc.so.1@foo.c@xxx
		// could still be file, symtab, or header file
		// can't check here because we
		// don't have a procobj
		header_name = file_name;
		file_name = symtab_name;
		symtab_name = pobj;
		pobj = 0;
	}
}

int
Location::check_symtab(ProcObj *p, Frame *f, Symtab *&stab)
{
	if (!symtab_name)
	{
		stab = 0;
		return 1;
	}

	int	string = (flags & L_SYM_STRING) != 0;
	char	*name;
	if (!string && (*symtab_name == '$' || *symtab_name == '%'))
	{
		get_str_val(p, f, symtab_name, name);
		stab = p->find_symtab(name);
		delete name;
	}
	else
	{
		name = symtab_name;
		stab = p->find_symtab(name);
	}
	if (!stab)
	{
		if (flags&L_CHECK_types)
		{
			header_name = file_name;
			file_name = symtab_name;
			symtab_name = 0;
			return 1;
		}
		else
		{
			printe(ERR_cant_find_symtab, E_ERROR, symtab_name);
			return 0;
		}
	}
	return 1;
}

int
Location::get_symtab(ProcObj *p, Frame *f, Symtab *&stab)
{
	if (!symtab_name)
	{
		stab = 0;
		return 1;
	}
	if (!p)
		get_pobj(p);
	return check_symtab(p, f, stab);
}

int
Location::get_file(ProcObj *p, Frame *f, char *&file, char *&header, Symbol &comp_unit)
{
	if (!p)
		get_pobj(p);
	Symtab *stab = 0;
	if (symtab_name)
		check_symtab(p, f, stab);
	if (file_name)
	{
		if (!(flags&L_FILE_STRING) && (*file_name == '$' || *file_name == '%'))
		{
			if (!get_str_val(p, f, file_name, file))
			{
				file = 0;
				return 0;
			}
		}
		else
			file = file_name;
		comp_unit = find_compilation_unit(p, stab, file);
		if (comp_unit.isnull())
		{
			if (flags&L_CHECK_types)
			{
				header_name = file_name;
				header = file;
				file_name = file = 0;
				flags &= ~L_CHECK_types;
				return 1;
			}
			else
			{
				if (symtab_name)
					printe(ERR_no_source_in_obj, E_ERROR, file, symtab_name);
				else
					printe(ERR_no_source, E_ERROR, file);
				file = 0;
				return 0;
			}
		}
	}
	else
		file = 0;
	if (header_name)
	{
		if (!(flags&L_HEADER_STRING) && (*header_name == '$' || *header_name == '%'))
		{
			if (!get_str_val(p, f, header_name, header))
			{
				header = 0;
				return 0;
			}
		}
		else
			header = header_name;
	}
	else
		header = 0;
	return 1;
}

int 
Location::get_line(ProcObj *p, Frame *f, unsigned long &line)
{
	if (!(kind & L_LINE))
	{
		printe(ERR_internal, E_ERROR, "Location::get_line",
			__LINE__);
		return 0;
	}
	if (!(kind & L_VAR))
	{
		line = locn.l_val;
		return 1;
	}
	return get_int_val(p, f, locn.l_name, line);
}

int 
Location::get_addr(ProcObj *p, Frame *f, unsigned long &addr)
{
	if (!(kind & L_ADDR))
	{
		printe(ERR_internal, E_ERROR, "Location::get_addr",
			__LINE__);
		return 0;
	}
	if (!(kind & L_VAR))
	{
		addr = locn.l_val;
		return 1;
	}
	return get_int_val(p, f, locn.l_name, addr);
}

int 
Location::get_func(ProcObj *p, Frame *f, char *&name)
{
	if (!(kind & L_FUNC))
	{
		printe(ERR_internal, E_ERROR, "Location::get_func",
			__LINE__);
		return 0;
	}
	if (!(kind & L_VAR))
	{
		name = locn.l_name;
		return 1;
	}
	return get_str_val(p, f, locn.l_name, name);
}

int 
Location::get_offset(long &offval)
{
	unsigned long	ul;

	if (!(flags & (L_PLUS_OFF|L_MINUS_OFF)))
	{
		// one of these flags is set for symbolic offsets
		offval = off.o_val;
		return 1;
	}
	if (!get_int_val(0, 0, off.o_name, ul))
		return 0;

	offval = (long)ul;
	if (flags & L_MINUS_OFF)
		offval = -offval;
	return 1;
}

void
Location::print(Buffer *buffer)
{
	char	buf[BUFSIZ];
	char	*cur = buf;

	if (pobj)
		cur += sprintf(cur, "%s@", pobj);
	if (symtab_name)
		cur += sprintf(cur, "%s@", symtab_name);
	if (file_name)
		cur += sprintf(cur, "%s@", file_name);
	if (header_name)
		cur += sprintf(cur, "%s@", header_name);
	if ((kind & L_VAR) || (kind & L_FUNC))
		cur += sprintf(cur, "%s", locn.l_name);
	else if (kind & L_LINE)
		cur += sprintf(cur, "%d", locn.l_val);
	else
		cur += sprintf(cur, "%#x", locn.l_val);
	if (flags & L_PLUS_OFF)
		sprintf(cur, "+%s", off.o_name);
	else if (flags & L_MINUS_OFF)
		sprintf(cur, "-%s", off.o_name);
	else if (off.o_val != 0)
		sprintf(cur, "%+d", off.o_val);
	buffer->add(buf);
}

void
Location::print(ProcObj *p, Frame *f, Buffer *buffer)
{
	char		buf[BUFSIZ];
	char		*cur = buf;
	LDiscrim	ltype;
	ProcObj		*pp = p;
	char		*name;
	unsigned long	num;
	long		off;

	ltype = get_type();
	if (ltype == lk_none)
		return;
	if (pobj && !pp)
	{
		get_pobj(pp);	
		if (pp)
			cur += sprintf(cur, "%s@", pp->obj_name());
	}
	Symtab *stab;
	check_symtab(p, f, stab);
	if (symtab_name)
		cur += sprintf(cur, "%s@", symtab_name);
	if (ltype != lk_addr)
	{
		if (file_name)
		{
			char	*hname;
			Symbol	scope;
			get_file(p, f, name, hname, scope);
			if (name && hname)
				cur += sprintf(cur, "%s@%s@", name);
			else if (name)
				cur += sprintf(cur, "%s@", name);
			else if (hname)
				cur += sprintf(cur, "@%s@", name);
		}
	}
	switch(ltype)
	{
		case lk_fcn:
			get_func(p, f, name);
			if (name)
				cur += sprintf(cur, "%s", name);
			break;
		case lk_stmt:
			get_line(p, f, num);
			cur += sprintf(cur, "%d", num);
			break;
		case lk_addr:
			get_addr(p, f, num);
			cur += sprintf(cur, "%#x", num);
			break;
		case lk_none:
		default:
			break;
	}
	if (ltype != lk_stmt)
	{
		// get offset, if any
		get_offset(off);
		if (off)
			sprintf(cur, "%+d", off);
	}
	buffer->add(buf);
}
