#ident	"@(#)libelf:common/getident.c	1.8"


#ifdef __STDC__
	#pragma weak	elf_getident = _elf_getident
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"


char *
elf_getident(elf, ptr)
	Elf	*elf;
	size_t	*ptr;
{
	size_t	sz = 0;
	char	*id = 0;

	if (elf != 0)
	{
		if (elf->ed_identsz != 0
		&& _elf_cook(elf) == OK_YES
		&& _elf_vm(elf, (size_t)0, elf->ed_identsz) == OK_YES)
		{
			id = elf->ed_ident;
			sz = elf->ed_identsz;
		}
	}
	if (ptr != 0)
		*ptr = sz;
	return id;
}
