#ident	"@(#)libelf:common/getbase.c	1.6"


#ifdef __STDC__
	#pragma weak	elf_getbase = _elf_getbase
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"


off_t
elf_getbase(elf)
	Elf	*elf;
{
	if (elf == 0)
		return -1;
	return elf->ed_baseoff;
}
