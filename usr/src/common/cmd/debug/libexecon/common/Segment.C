#ident	"@(#)debugger:libexecon/common/Segment.C	1.7"

#include "Segment.h"
#include "Procctl.h"
#include "ProcObj.h"
#include "Itype.h"
#include "Symtab.h"
#include "Object.h"
#include "Dyn_info.h"
#include <string.h>

Segment::Segment(Object *obj, Symnode *sym, 
	Iaddr lo, long sz, long b, int flags)
{
	symnode = sym;
	access = obj;
	loaddr = lo;
	hiaddr = loaddr + sz;
	base = b;
	prot_flags = flags;
}

int
Segment::read( Iaddr addr, void * buffer, int len )
{
	long	offset;

	if (!access)
		return -1;
	offset = addr - loaddr + base;
	return access->read( offset, buffer, len );
}

static int	size[] = {	
	0,
	sizeof(Ichar),
	sizeof(Iint1),
	sizeof(Iint2),
	sizeof(Iint4),
#if LONG_LONG
	sizeof(Iint8),
#endif
	sizeof(Iuchar),
	sizeof(Iuint1),
	sizeof(Iuint2),
	sizeof(Iuint4),
#if LONG_LONG
	sizeof(Iuint8),
#endif
	sizeof(Isfloat),
	sizeof(Idfloat),
	sizeof(Ixfloat),
	sizeof(Iaddr),
	sizeof(Ibase),
	sizeof(Ioffset),
	0
};

int
stype_size( Stype stype )
{
	return size[stype];
}

int
Segment::read( Iaddr addr, Stype stype, Itype & itype )
{
	long	offset;

	if (!access)
		return -1;
	offset = addr - loaddr + base;
	return(access->read( offset, &itype, size[stype] ));
}

Symnode::Symnode( const char * s, Iaddr ss_base )
{
	pathname = new(char[strlen(s) + 1]);
	strcpy( (char *)pathname, s );
	sym.ss_base = ss_base;
	dyn_info = 0;
	sym.symtable = 0;
	brtbl_lo = 0;
	brtbl_hi = 0;
}

Symnode::~Symnode()
{
	delete (void *)pathname;
	delete dyn_info;
}

void
Symnode::check_for_brtbl(Symbol &symbol, ProcObj *proc)
{
	if (brtbl_lo > 0) 
	{
		//
		// static shared library
		//
			
		Iaddr addr = symbol.pc(an_lopc);
		if ( (addr >= brtbl_lo) && ( addr < brtbl_hi) ) 
		{
			addr = proc->instruct()->brtbl2fcn(addr);
			symbol = sym.find_entry(addr);
		}
	}
}
