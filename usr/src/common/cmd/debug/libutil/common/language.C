#ident	"@(#)debugger:libutil/common/language.C	1.16"

#include "utility.h"
#include "Language.h"
#include "Interface.h"
#include "ProcObj.h"
#include "Symbol.h"
#include "Attribute.h"
#include "Process.h"
#include "Frame.h"
#include "Symtab.h"
#include <string.h>

static Language language = UnSpec; // %lang is set only by the user

// string names are known only in this file
// enum Language is externally visible

struct langmap {
	const char *name;
	Language lang;
};

// multiple entries for C++ are not a problem - if the language is user-set,
// set_language looks through here for the first matching string and finds
// CPLUS.  If set from the object file, language_name looks for a match
// on the enum and finds "C++" for either CPLUS, CPLUS_ASSUMED,
// or CPLUS_ASSUMED_V2
static langmap lm[] = {
    	"C",	C,		// debug C exprs.
    	"c",	C,		// debug C exprs.
    	"cc",	C,		// debug C exprs.
	"C++",	CPLUS,		// full C++ debugging
	"c++",	CPLUS,		// full C++ debugging
	"CC",	CPLUS,		// full C++ debugging
	"C++",	CPLUS_ASSUMED_V2, // debug C++ subset - C-generating ANSI C++ compiler
    	"C++",	CPLUS_ASSUMED,	// debug C++ subset - cfront version
	0,	UnSpec,	// end of list
};

// in most cases, if the user has set %lang it overrides
// %db_lang (the current context language).  But if the
// user set %lang to C++, and we also have an object compiled from
// a C++ file, use the language from the object file to avoid
// using the wrong variant.
Language
current_language(ProcObj *pobj, Frame *frame)
{
	Language return_value = current_user_language();
	if (return_value == UnSpec)
		return current_context_language(pobj, frame);
	else if (return_value >= CPLUS
		&& return_value <= CPLUS_ASSUMED_V2)
	{
		Language context_val = current_context_language(pobj);
		if (context_val >= CPLUS
			&& context_val <= CPLUS_ASSUMED_V2)
			return context_val;
	}
	return return_value;
}

Language current_user_language()
{
	return language;
}

Language
current_context_language(ProcObj *pobj, Frame *frame)
{
	// %db_lang defaults to C if unknown

	if (!pobj || pobj->is_running()) return C;

	// find source for current frame
	if (!frame)
		frame = pobj->curframe();

	Symbol		source;
	Attribute	*attr;
	Iaddr		pc = frame->pc_value();
	Symtab		*symtab = pobj->find_symtab(pc);

	if (!symtab || !symtab->find_source(pc, source) || source.isnull()
		|| (attr = source.attribute(an_language)) == 0
		|| attr->value.language == UnSpec
		|| attr->value.language == C)
	{
		Language lang = pobj->process()->get_current_language();
		if (lang != UnSpec)
			return lang;

#if EXCEPTION_HANDLING
		if (pobj->get_eh_info())
		{
			// uses exception handling, safe to assume lang == CPLUS
			// probably compiled without -g
			pobj->process()->set_current_language(CPLUS);
			return CPLUS;
		}
#endif

		Symbol sym;

		sym = pobj->find_global("__cpp_unixware_20");	// cfront replacement
		if (sym.isnull())
		{
			// cfront release 3.0.2
			sym = pobj->find_global("__cpp_version_302");
			if (sym.isnull())
			{
				sym = pobj->find_global("__dtors(void)");	// before cfront 3.0.2
			}
			if (!sym.isnull())
				lang = CPLUS_ASSUMED;
			else
				lang = C;
		}
		else
			lang = CPLUS_ASSUMED_V2;
		pobj->process()->set_current_language(lang);
		return lang;
	}
	else
		return attr->value.language;
}

const char *
language_name( Language lang )
{
	register langmap *p = lm;
	for (p = lm; p->name; p++)
	{
		if ( lang == p->lang )
			return p->name;
	}
	return 0;
}

int
set_language( const char *name )
{
	register langmap *p;

	if (!name || !*name)
	{
		language = UnSpec;
		return 1;
	}

	for (p = lm; p->name; p++)
	{
		if (strcmp( name, p->name ) == 0)
		{
			language = p->lang;
			if (get_ui_type() == ui_gui)
				printm(MSG_set_language, name);
			return 1;
		}
	}
	printe(ERR_unknown_language, E_ERROR, name);
	return 0;
}
