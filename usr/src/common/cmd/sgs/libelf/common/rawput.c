#ident	"@(#)libelf:common/rawput.c	1.2"


#include "syn.h"
#include "libelf.h"
#include "decl.h"
#include "error.h"


/* Raw file input/output
 *	Read pieces of input files.
 */


char *
_elf_read(fd, off, fsz)
	int		fd;
	off_t		off;
	size_t		fsz;
{
	char		*p;

	if (fsz == 0)
		return 0;
	if (fd == -1)
	{
		_elf_err = EREQ_NOFD;
		return 0;
	}
	if (lseek(fd, off, 0) != off)
	{
		_elf_err = EIO_SEEK;
		return 0;
	}
	if ((p = malloc(fsz)) == 0)
	{
		_elf_err = EMEM_DATA;
		return 0;
	}
	if (read(fd, p, fsz) != fsz)
	{
		_elf_err = EIO_READ;
		free(p);
		return 0;
	}
	return p;
}
