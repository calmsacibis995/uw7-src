#ident	"@(#)debugger:libsymbol/common/AddrList.h	1.1"
#ifndef AddrList_h
#define AddrList_h

#include	"Attribute.h"
#include	"Rbtree.h"
#include	"Iaddr.h"

class AddrEntry : public Rbnode {
	Iaddr		loaddr,hiaddr;
	Attr_form	form;
	Attr_value	value;
	friend class	AddrList;
	friend class	Evaluator;
public:
			AddrEntry();
			~AddrEntry() {}
			AddrEntry( AddrEntry & );
	int		cmp( Rbnode & );
	Rbnode *	makenode();
};

class AddrList : public Rbtree {
public:
			AddrList() {}
			~AddrList() {}
	void		add( Iaddr lo, Iaddr hi, long offset, Attr_form );
	void		complete();
};

#endif /* AddrList_h */
