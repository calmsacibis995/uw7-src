#ident	"@(#)debugger:libsymbol/common/Reflist.C	1.2"
#include	"Reflist.h"
#include	"Rbtree.h"
#include	<string.h>
#if DEBUG
#include	<stdio.h>
#endif

int
Refnode::cmp( Rbnode &t ) 
{
	if (diskloc < ((Refnode*)(&t))->diskloc)
		return -1;
	else if (diskloc > ((Refnode*)(&t))->diskloc)
		return 1;
	else
		return 0;
}

// Add a new refnode 
void
Reflist::add(long l, Attribute * s)
{
	Refnode		newnode(l,s);

	(void)tinsert(newnode);
}

// Search for a refnode with a specified key
// Return succeed or fail.

int
Reflist::lookup(long offset, Attribute * & r)
{
	Refnode		keynode(offset);
	Rbnode	 *	t = tlookup(keynode);

	if ( t == 0 )
	{
		r = 0;
		return 0;
	}
	else
	{
		r = ((Refnode *)t)->nodeloc;
		return 1;
	}
}

Rbnode *
Refnode::makenode()
{
	char *	s;

	s = new(char[sizeof(Refnode)]);
	memcpy(s,(char*)this,sizeof(Refnode));
	return (Rbnode*)s;
}

#if DEBUG
void
Reflist::dump()
{
	Refnode	*node = (Refnode *)tfirst();
	for (; node; node = (Refnode *)node->next())
		fprintf(stderr, "offset = %#x, attr = %#x\n",
				node->diskloc, node->nodeloc);
}
#endif
