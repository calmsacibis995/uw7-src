#ident	"@(#)libelf:common/fill.c	1.2"


#ifdef __STDC__
	#pragma weak	elf_fill = _elf_fill
#endif


#include "syn.h"


extern int	_elf_byte;


void
elf_fill(fill)
	int	fill;
{
	_elf_byte = fill;
	return;
}
