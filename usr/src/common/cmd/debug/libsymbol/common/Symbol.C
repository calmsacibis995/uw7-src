/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libsymbol/common/Symbol.C	1.21"
#include	"Symbol.h"
#include	"Source.h"
#include	"Evaluator.h"
#include	"Locdesc.h"
#include	"Interface.h"
#include	"Tag.h"
#include	"TYPE.h"
#include	"ProcObj.h"
#include	"Frame.h"
#include	"NameList.h"
#include	"TypeList.h"
#include	"global.h"
#include	<string.h>

const char *
Symbol::name()
{
	Attribute *	a;

	if ( namep == 0 && evaluator &&
			(a = evaluator->attribute(attrlist, an_name)) )
	{
		namep = a->value.name;
	}
	return namep;
}

const char *
Symbol::mangledName()
{
	Attribute *	a;

	if ( evaluator && (a = evaluator->attribute(attrlist, an_mangledname)) )
	{
		return a->value.name;
	}
	return 0;
}

Symbol
#ifdef __cplusplus
Symbol::arc( Attr_name attrname ) const
#else
Symbol::arc( Attr_name attrname )
#endif
{
	Symbol		symbol;
	Attribute *	a;

	if ( evaluator && (a = evaluator->arc(attrlist,attrname)) != 0 )
	{
		symbol.attrlist = a->value.symbol;
		symbol.evaluator = evaluator;
		symbol.ss_base = ss_base;
	}
	return symbol;
}

Attribute *
Symbol::attribute( Attr_name attrname )
{
	if ( evaluator == 0 )
	{
		return 0;
	}
	else
	{
		return evaluator->attribute( attrlist, attrname );
	}
}

Iaddr
Symbol::pc( Attr_name attr_name )
{
	Attribute	* a;

	if ( (a = attribute(attr_name)) == 0 )
	{
		return ~0;
	}
	else
	{
		return a->value.addr + ss_base;
	}
}

int
Symbol::file_table( Source & s )
{
	Attribute	* a;

	if ( (a = attribute(an_file_table)) == 0 )
		return 0;

	s.file_table = a->value.file_table;
	return 1;
}

int
Symbol::has_attribute(Attr_name attrname)
{
	if ( evaluator == 0 )
	{
		return 0;
	}
	else
	{
		return evaluator->has_attribute(attrlist, attrname);
	}
}

int
Symbol::source( Source & s )
{
	Attribute	* a;

	if (!file_table(s) || (a = attribute(an_lineinfo)) == 0)
	{
		s.lineinfo = 0;
		s.ss_base = 0;
		return 0;
	}
	else
	{
		s.lineinfo = a->value.lineinfo;
		s.ss_base = ss_base;
		return 1;
	}
}

int
Symbol::file_and_line(long &file, long &line)
{
	Attribute	*a, *b;

	if ((a = attribute(an_decl_file)) == 0 || (b = attribute(an_decl_line)) == 0)
	{
		file = 0;
		line = 0;
		return 0;
	}
	else
	{
		file = a->value.word;
		line = b->value.word;
		return 1;
	}
}

int
Symbol::type(TYPE *t, Attr_name attr, int do_typedef)
{
	Attribute *a;
	Symbol s(*this);

	a = attribute(attr);
	while(a)
	{
		switch (a->form)
		{
		case af_symbol:
			{
			Attribute	*a1, *tag;
			a1 = a->value.symbol;
			if ((tag = evaluator->attribute(a1, an_tag)) == 0)
			{
				t->null();
				return 0;
			}
			if (tag->value.tag == t_typedef && do_typedef)
			{
				// typedef - find real type
				a = evaluator->attribute(a1, an_type);
				break;
			}
			else if (tag->value.tag == t_basetype)
			{
				// DWARF2 type entry - translate to debugger's
				// internal fundamental type and save the
				// fundamental type, to avoid having to redo
				// the translation each time
				Attribute *af = evaluator->attribute(a1, an_type);
				a->value.fund_type = af->value.fund_type;
				a->form = af_fundamental_type;
			}
			else
			{
				s.namep = 0;
				s.attrlist = a->value.symbol;
				*t = s;
				return 1;
			}
			}
		case af_fundamental_type:
			// the symbol has no type information at all (meaning
			// it was probably compiled without -g) - assume
			// function returning int (if in text) or just int
			// if object
			if (a->value.fund_type == ft_none)
			{
				a->value.fund_type = ft_int;
			}
			*t = a->value.fund_type;
			return 1;
		default:
			t->null();
			return 0;
		}
	}
	t->null();
	return 0;
}

void *
Symbol::get_stashed_type(Language lang)
{
	TypeEntry	node(*this, 0, lang);
	TypeEntry	*tnode;

	tnode = (TypeEntry *)typelist.tlookup(node);
	if (tnode)
	{
		return tnode->type_ptr;
	}
	return 0;
}

void *
Symbol::get_stashed_type(Attr_name attr, Language lang)
{
	Attribute	*a;

	if ((a = attribute(attr)) == 0 || a->form == af_fundamental_type
		|| a->form != af_symbol)
		return 0;

	Symbol		sym(a->value.symbol, evaluator);
	TypeEntry	node(sym, 0, lang);
	TypeEntry	*tnode;

	tnode = (TypeEntry *)typelist.tlookup(node);
	if (tnode)
	{
		return tnode->type_ptr;
	}
	return 0;
}

void
Symbol::stash_type(void *tptr, Attr_name attr, Language lang)
{
	Attribute	*a;

	if ((a = attribute(attr)) == 0)
	{
		printe(ERR_internal, E_ERROR, "Symbol::stash_type", __LINE__);
		return;
	}
	Symbol sym(a->value.symbol, evaluator);

	typelist.add(sym, tptr, lang);
}

void
Symbol::stash_type(void *tptr,  Language lang)
{
	typelist.add(*this, tptr, lang);
}

int
Symbol::locdesc(Locdesc & desc, Attr_name attr)
{
	Attribute *a;

	if ((a = attribute(attr)) == 0)
	{
		desc.clear();
		return 0;
	}
	if (a->form == af_locdesc)
	{
		desc = a->value.loc;
		desc.adjust( ss_base );
		return 1;
	} else {
		return 0;
	}
}

int
Symbol::place(Place &plc, ProcObj *pobj, Frame *frame, Iaddr offset)
{
	Locdesc loc;
	if (locdesc(loc) != 0)
	{
		plc = loc.place(pobj, frame, offset);
		if (plc.isnull())
			return 0;
	}
	else
	{
		Attribute *a = attribute(an_lopc);
		if ((a = attribute(an_lopc)) == 0)
			return 0;
		plc.kind = pAddress;
		plc.addr = a->value.addr + offset;
	}
	return 1;
}

Tag
Symbol::tag()
{
	register Attribute *a = attribute(an_tag);

	if (a != 0 && a->value.tag != 0)
	{
		return a->value.tag;
	}
	return t_none;
}

void
Symbol::null()
{
	namep = 0;
	attrlist = 0;
	evaluator = 0;
	ss_base = 0;
}

int
Symbol::isUserTypeSym()
{
	Tag t = tag();
	return (t==t_classtype || t==t_structuretype || t==t_uniontype || 
		            t==t_enumtype || t==t_typedef);
}

int
Symbol::isUserTagName()
{
	Tag t = tag();
	return (t==t_classtype || t==t_structuretype || t==t_uniontype || 
		            t==t_enumtype);
}

int
Symbol::isVariable()
{
	Tag t = tag();
	return (t==t_variable || t==t_argument);
}

int
Symbol::isEntry()
{
	Tag t = tag();
	return (t==t_subroutine || t == t_entry);
}

int
Symbol::isBlock()
{
	Tag t = tag();
	return (t==t_block || t==t_try_block || t == t_catch_block);
}

int
Symbol::isMember()
{
	Tag t = tag();
	return (t==t_structuremem || t==t_unionmem);
}

int
Symbol::isInlined()
{
	Attribute	*attr = attribute(an_inline);
	return (attr && attr->value.word);
}

// Was type of symbol assumed to be int?  (no type information available)
// If true_only is set, we return 0 for not assumed, 1 for assumed.
// If true_only is 0, we return 0 for not assumed, 1 if assumed and
// this is the first such request, else > 1.
int
Symbol::type_assumed(int true_only)
{
	register Attribute *a;

	if ((a = attribute(an_assumed_type)) == 0)
		return 0;
	else if (true_only)
		return 1;
	else 
		return(++(a->value.word));
}

Symbol
Symbol::specification() const
{
	Symbol sym = arc(an_specification);
	if (sym.isnull())
	{
		Symbol abstract = arc(an_abstract);
		if (!abstract.isnull())
			sym = abstract.arc(an_specification);
	}
	return sym;
}

Symbol
Symbol::definition(const char *full_name)
{
	Symbol		sym;
	Attribute	*a;

	if (!evaluator || (a = evaluator->arc(attrlist, an_definition)) == 0)
		return sym;

	if (a->value.symbol)
	{
		sym.namep = full_name;
		sym.attrlist = a->value.symbol;
		sym.evaluator = evaluator;
	}
	else
	{
		NameEntry *ne = evaluator->find_next_global(full_name, 0);
		evaluator->global_search_complete();
		if (ne)
		{
			sym.namep = (char *)ne->name();
			sym.attrlist = evaluator->evaluate(ne);
			sym.evaluator = evaluator;
		}
		else if (strstr(full_name, "::") != 0)
		{
			// may be static for nested classes - A::B::f(void)
			Symbol scope = this->parent();
			while (!scope.isnull() && !scope.isSourceFile())
				scope = scope.parent();
			if (!scope.isnull())
			{
				for (sym = scope.child(); !sym.isnull();
					sym = sym.sibling())
				{
					if (prismember(&interrupt, SIGINT))
						break;
					Symbol check = sym.specification();
					const char *name;
					if (check == *this || ((name = sym.name()) != 0
							&& strcmp(name, full_name) == 0))
						break;
				}
			}
		}
	}
	if (!sym.isnull())
	{
		a->form = af_symbol;
		a->value.symbol = sym.attrlist;
	}
	return sym;
}

int
Symbol::matches(const Symbol &sym)
{
	if (isnull() || sym.isnull())
		return 0;
	if (*this == sym)
		return 1;

	Symbol spec = specification();
	if (spec.isnull())
	{
		spec = sym.specification();
		if (spec == *this)
			return 1;
	}
	else if (spec == sym)
		return 1;
	return 0;
}
