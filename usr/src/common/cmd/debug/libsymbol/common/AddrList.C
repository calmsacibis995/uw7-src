#ident	"@(#)debugger:libsymbol/common/AddrList.C	1.2"
#include	"Attribute.h"
#include	"AddrList.h"
#include	<string.h>

AddrEntry::AddrEntry()
{
	loaddr = 0;
	hiaddr = 0;
	form = af_none;
	value.word = 0;
}

AddrEntry::AddrEntry( AddrEntry & addrentry )
{
	loaddr = addrentry.loaddr;
	hiaddr = addrentry.hiaddr;
	form = addrentry.form;
	value = addrentry.value;
}

// Make an AddrEntry instance
Rbnode *
AddrEntry::makenode()
{
	char *	s;

	s = new(char[sizeof(AddrEntry)]);
	memcpy(s,(char*)this,sizeof(AddrEntry));
	return (Rbnode*)s;
}

// returns < 0, 0, > 0 as this is <, ==, > t
int
AddrEntry::cmp( Rbnode & t )
{
	AddrEntry *	addrentry = (AddrEntry*)(&t);

	if ( (loaddr < addrentry->loaddr) && (hiaddr <= addrentry->hiaddr) )
		return -1;
	else if ( (loaddr > addrentry->loaddr) && (hiaddr >= addrentry->hiaddr) )
		return 1;
	else
		return 0;
}

void
AddrList::add( Iaddr lo, Iaddr hi, long offset, Attr_form form )
{
	AddrEntry	node;

	node.loaddr = lo;
	node.hiaddr = hi;		// not always true ?
	node.form = form;
	node.value.word = offset;
	(void)tinsert(node);
}

// walk list in reverse order; make sure hi and lo addresses
// are ordered so the addresses don't overlap and the entire
// address space is covered

void
AddrList::complete()
{
	AddrEntry	*x;
	Iaddr		lastlo;

	x = (AddrEntry*)tlast();
	if ( x != 0 )
	{
		x->hiaddr = ~0;
		lastlo = x->loaddr;
		x = (AddrEntry*)(x->prev());
		while (x != 0)
		{
			if (x->hiaddr == x->loaddr)
			{
				x->hiaddr = lastlo;
			}
			lastlo = x->loaddr;
			x = (AddrEntry*)(x->prev());
		}
	}
}
