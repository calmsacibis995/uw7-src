#ident	"@(#)libelf:common/ndxscn.c	1.2"


#ifdef __STDC__
	#pragma weak	elf_ndxscn = _elf_ndxscn
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"


size_t
elf_ndxscn(scn)
	register Elf_Scn	*scn;
{

	if (scn == 0)
		return SHN_UNDEF;
	return scn->s_index;
}
