#ident	"@(#)debugger:libsymbol/i386/Cvtaddr.C	1.1"

#include	"Locdesc.h"
#include	"Cvtaddr.h"

void
cvt_arg( Locdesc &loc, unsigned long value )
{
	loc.basereg(REG_AP).offset(value).add();
}

void
cvt_reg( Locdesc &loc, unsigned long value )
{
	loc.reg((int)value);
}

void
cvt_auto( Locdesc &loc, unsigned long value )
{
	loc.basereg(REG_FP).offset(value).add();
}

void
cvt_extern( Locdesc &loc, unsigned long value )
{
	loc.addr(value);
}
