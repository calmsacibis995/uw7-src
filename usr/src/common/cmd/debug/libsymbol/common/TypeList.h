#ifndef TypeList_h
#define TypeList_h
#ident	"@(#)debugger:libsymbol/common/TypeList.h	1.3"

#include "Rbtree.h"
#include "Symbol.h"
#include "Language.h"

class Evaluator;

class TypeEntry : public Rbnode
{
	Symbol		type_sym;
	void		*type_ptr;
	Language	lang;

	friend class	TypeList;
	friend class	Symbol;
public:
			TypeEntry() { lang = C; }
			TypeEntry(const TypeEntry *r)
				{
					type_sym = r->type_sym;
					type_ptr = r->type_ptr;
					lang = r->lang;
				}
			TypeEntry(Symbol &s, void *t, Language l)
				{
					type_sym = s;
					type_ptr = t;
					lang = l;
				}
			~TypeEntry() {}

			// functions overriding virtuals in Rbnode
	int		cmp(Rbnode &);
	Rbnode		*makenode();

#if DEBUG
	void		dump();
#endif
};

class TypeList : public Rbtree
{
public:
		TypeList() {}
		~TypeList() {}

	void	add(Symbol &, void *, Language);
#if DEBUG
	void	dump();
#endif
};

extern TypeList typelist;

#endif // TypeList_h
