#ident	"@(#)debugger:libexecon/common/Reg.C	1.1"

// Reg.C -- register names and attributes, common code

#include <string.h>
#include "Reg.h"

RegRef
regref(register const char *name)
{
	register RegAttrs *p = regs;

	for ( ; p->ref != REG_UNK ; p++ )
		if ( strcmp(p->name, name) == 0 )
			break;
	return p->ref;
}

RegAttrs *
regattrs(register RegRef ref)
{
	register RegAttrs *p = regs;

	for ( ; p->ref != REG_UNK ; p++ )
		if ( ref == p->ref )
			break;
	return p;		// will be REG_UNK entry if ref not found
}

RegAttrs *
regattrs(register const char *name)
{
	register RegAttrs *p = regs;

	for ( ; p->ref != REG_UNK ; p++ )
		if ( strcmp(p->name, name) == 0 )
			break;
	return p;		// will be REG_UNK entry if name not found
}
