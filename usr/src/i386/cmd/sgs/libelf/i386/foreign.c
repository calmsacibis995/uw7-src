#ident	"@(#)libelf:i386/foreign.c	1.1"


#include "syn.h"
#include "libelf.h"
#include "decl.h"
#include "foreign.h"


/* Foreign file conversion
 *	Allow other file formats to be converted to elf.
 *	Change this table to support new or drop old conversions.
 *
 *	A foreign function actually returns Elf_Kind or -1 on error.
 */


int	_elf_coff	_((Elf *));


int	(*const _elf_foreign[]) _((Elf *)) =
{
	_elf_coff,
	0,
};
