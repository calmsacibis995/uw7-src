#ident	"@(#)libelf:common/rand.c	1.2"


#ifdef __STDC__
	#pragma weak	elf_rand = _elf_rand
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"
#include "error.h"


size_t
elf_rand(elf, off)
	Elf	*elf;
	size_t	off;
{
	if (elf == 0)
		return 0;
	if (elf->ed_kind != ELF_K_AR)
	{
		_elf_err = EREQ_AR;
		return 0;
	}
	if (off == 0 || elf->ed_fsz < off)
	{
		_elf_err = EREQ_RAND;
		return 0;
	}
	return elf->ed_nextoff = off;
}
