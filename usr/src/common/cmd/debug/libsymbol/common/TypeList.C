#ident	"@(#)debugger:libsymbol/common/TypeList.C	1.5"

#include "Attribute.h"
#include "TypeList.h"
#include "Language.h"
#include "TYPE.h"
#if DEBUG
#include "Interface.h"
#endif

TypeList typelist;

// Make a TypeEntry instance
Rbnode *
TypeEntry::makenode()
{
	return new TypeEntry(this);
}

int
TypeEntry::cmp(Rbnode &n)
{
	TypeEntry *tentry = (TypeEntry*)(&n);
	if (lang < tentry->lang)
		return -1;
	if (lang > tentry->lang)
		return 1;

	switch (lang)
	{
	default:	// covers C & C++
		return CC_type_compare(lang, type_sym, tentry->type_sym);
	}
}

#if DEBUG
void
TypeEntry::dump()
{
	DPRINT(DBG_EXPR, ("name: '%s', tag: %d, lang: %s\n", type_sym.name(),
		type_sym.tag(), language_name(lang)))
}
#endif

void
TypeList::add(Symbol &sym, void *t, Language l)
{
	TypeEntry	node(sym, t, l);

	tinsert(node);
#if DEBUG
	if (debugflag & DBG_EXPR)
	{
		dump();
		DPRINT(DBG_EXPR, ("************\n"))
	}
#endif
}

#if DEBUG
void
TypeList::dump()
{
	for (TypeEntry *n = (TypeEntry *)tfirst(); n; n = (TypeEntry *)n->next())
		n->dump();
}
#endif
