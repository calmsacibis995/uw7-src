#ifndef NameList_h
#define NameList_h
#ident	"@(#)debugger:inc/common/NameList.h	1.2"

#include	"Rbtree.h"
#include	"Attribute.h"

class NameEntry : public Rbnode {
	const char *	namep;
	Attr_form	form;
	Attr_value	value;

	void		setNodeName(char* s);
	friend class	Evaluator;
	friend class	NameList;
public:
			NameEntry();
			~NameEntry() {}
			NameEntry(const NameEntry & );

	int		cmpName(const char *name);
	int		lookupCmp( Rbnode& t )	// lookup compare
				{ return cmpName(((NameEntry *)&t)->namep); }
	int		cmp( Rbnode & );	// insert compare

	Rbnode *	makenode();
	const char *	name()	{	return namep;	}
};

class NameList : public Rbtree {
public:
			NameList() {}
			~NameList() {}
	NameEntry *	add( const char *, long, Attr_form );
	NameEntry *	add( const char *, const Attribute * );
};

#endif

// end of NameList.h

