#ident	"@(#)debugger:libutil/common/overload.C	1.20"

#include "Buffer.h"
#include "Interface.h"
#include "Language.h"
#include "ProcObj.h"
#include "Machine.h"
#include "Msgtab.h"
#include "Symbol.h"
#include "Symtab.h"
#include "Vector.h"
#include "global.h"
#include "utility.h"
#include "str.h"
#include <string.h>
#include <signal.h>
#include <stdio.h>

Overload::~Overload()
{
	if (buffer)
		buf_pool.put(buffer);
	if (vector)
		vec_pool.put(vector);
}

int
Overload::init(ProcObj *proc, Vector **p)
{
	pobj = proc;
	pv = p;
	if (pv)
		*pv = 0;

	// buffer is used to build up the list of choices to display
	buffer = buf_pool.get();
	vector = vec_pool.get();

	buffer->clear();
	vector->clear();
	count = 0;
	return 1;
}

void
Overload::add_choice(const char *name, const Symbol &sym, int global, Location *loc,
	void *ld)
{
	Overload_data	data;
	char		buf[MAX_INT_DIGITS+1];

	if (global)
	{
		Overload_data	*od = (Overload_data *)vector->ptr();
		for(int i = 1; i < count; i++, od++)
		{
			if (strcmp(name, 
				pobj->symbol_name(od->function)) == 0)
			// same global function already added
			return;

		}
	}
	sprintf(buf, "%d\t", ++count);
	buffer->add(buf);
	buffer->add(name);
	buffer->add('\n');
	data.function = sym;
	data.location = loc;
	data.lang_data = ld;
	vector->add(&data, sizeof(Overload_data));
}

void
Overload::add_choice(Location *loc, Iaddr addr)
{
	Overload_data	data;

	data.location = loc;
	data.address = addr;
	vector->add(&data, sizeof(Overload_data));
	++count;
}

int
Overload::do_query(Symbol &symbol, const char *func)
{
	int	index;

	// if pv is non-null, calling function allows for possibility of
	// multiple selection, so use message that includes "All of the above"
	// at the end
	if (pv)
	{
		index = query(QUERY_which_func_or_all, 0, func, (char *)*buffer,
			count+1);
	}
	else
		index = query(QUERY_which_function, 0, func, (char *)*buffer);

	if (index > 0 && index <= count)
	{
		// user selected one symbol
		Overload_data	data;
		Overload_data	*ptr = &((Overload_data *)vector->ptr())[index-1];

		symbol = data.function = ptr->function;
		data.lang_data = ptr->lang_data;
		vector->clear();
		vector->add(&data, sizeof(Overload_data));
	}
	else if (index <= 0)
	{
		// bad number, query already complained
		return 0;
	}
	else if (index == count+1 && pv)
	{
		// user selected "All of the above"
		// calling function releases vector
		symbol = ((Overload_data *)vector->ptr())[0].function;
	}
	else
	{
		// user gave a number out of range - query can't catch this because it
		// doesn't know the upper bound
		char	buf[MAX_INT_DIGITS+1];
		sprintf(buf, "%d", index);
		printe(ERR_bad_query_answer, E_ERROR, buf);
		return 0;
	}

	if (pv)
	{
		// user selected "All of the above"
		// calling function releases vector
		*pv = vector;
		vector = 0;
	}
	return index;
}

void
Overload::use_all_choices(Vector **vecp)
{
	if (vecp)
	{
		// calling function releases vector
		*vecp = vector;
		vector = 0;
	}
}

// if the user specified an overloaded function without giving the prototype,
// show the user the list of choices and ask which one to use.
// if pv is non-zero, the final choice in the list will be "All of the above"
// and a pointer to a vector of selected symbols will be returned in pv.

int
resolve_overloading(ProcObj *pobj, const char *fname, Symtab *symtab,
	Symbol &symbol, Vector **pv, int restrict_search)
{
	Language lang = current_language(pobj);
	if (lang != CPLUS && lang != CPLUS_ASSUMED && 
		lang != CPLUS_ASSUMED_V2)
	{
		return 1;
	}
	Overload	ov;

	if (!ov.init(pobj, pv))
		return 1;

	// If the name the user typed in (fname) had prototype information,
	// then we already have a symbol match and don't have to do the overload resolution.
	const char	*name = pobj->symbol_name(symbol);
	if (strchr(fname, '(') != 0)
	{
		return 1;
	}

	// buffer is used to build up the list of choices to display
	size_t		len;
	char		*func;

	// if inside a member function, function may have resolved to class::function
	// in that case, should only search for other functions within the same class
	char *ptr;
	if (strstr(name, "::") == 0 || (ptr = strchr(name, '(')) == 0)
	{
		len = strlen(fname);
		func = new char[len+1];
		strcpy(func, fname);
	}
	else
	{
		len = ptr - name;
		func = new char[len + 1];
		strncpy(func, name, len);
		func[len] = '\0';
	}
	sigrelse(SIGINT);

	// search first for file statics
	if (!symbol.has_attribute(an_external))
	{
		Symbol	localsym = symbol;
		for ( ; !localsym.isnull(); localsym = localsym.sibling() )
		{
			if (prismember(&interrupt, SIGINT))
			{
				sighold(SIGINT);
				return 0;
			}

			if (localsym.tag() != t_subroutine
				|| !localsym.has_attribute(an_lopc)
				|| localsym.has_attribute(an_external))
				continue;

			name = pobj->symbol_name(localsym);
			if (strncmp(func, name, len) != 0 || name[len] != '(')
				continue;

			ov.add_choice(name, localsym, 0);
		}
	}

	if (!restrict_search)
	{
		// add all the globals with the same name
		// if a symbol table was specified, search only in
		// that object
		Symbol	globalsym;
		int	found_C_linkage = 0;
		for(;;)
		{
			if (symtab)
			{
				if (!symtab->find_next_global(func, globalsym))
					break;
			}
			else
			{
				if (!pobj->find_next_global(func, globalsym))
					break;
			}
			if (prismember(&interrupt, SIGINT))
			{
				sighold(SIGINT);
				return 0;
			}
	
			name = pobj->symbol_name(globalsym);
			if (strncmp(func, name, len) != 0)
				continue;
			if (strchr(name, '(') == 0)
			{
				if (found_C_linkage)
					continue;
				else
					found_C_linkage = 1;
			}
			ov.add_choice(name, globalsym, 1);
		}
	}

	sighold(SIGINT);
	if (!ov.is_overloaded())
	{
		delete func;
		return 1;
	}

	int ret = ov.do_query(symbol, func);
	delete func;
	return (ret != 0);
}
