#ident	"@(#)libelf:common/foreign.h	1.2"


/*	This file declares functions for handling foreign (non-ELF)
 *	file formats.
 */


int		_elf_coff _((Elf *));

extern int	(*const _elf_foreign[]) _((Elf *));
