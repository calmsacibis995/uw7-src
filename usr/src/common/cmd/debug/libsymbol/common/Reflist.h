#ident	"@(#)debugger:libsymbol/common/Reflist.h	1.2"
#ifndef Reflist_h
#define Reflist_h

#include	"Rbtree.h"

struct Attribute;

class Refnode : public Rbnode {
public:
	long			diskloc;
	Attribute *		nodeloc;
	friend class		Reflist;
				Refnode(long l, Attribute * s = 0)
				 { diskloc = l; nodeloc = s; }
				~Refnode() {}

	// virtual functions
	Rbnode *		makenode();
	int			cmp( Rbnode &t );
};

class Reflist : public Rbtree {
public:
				Reflist() {}
				~Reflist() {}
	int			lookup(long, Attribute * &);
	void			add(long, Attribute *);
#if DEBUG
	void			dump();
#endif
};

#endif /* Reflist_h */
