#ident	"@(#)libelf:common/mmap.c	1.3"
#include "libelf.h"
#include "decl.h"
#include <sys/mman.h>

/*	This "program" is built (but not run) to see if the following
 *	services are available.  If so, libelf uses them.  Otherwise,
 *	it uses traditional read/write/....
 */

main()
{
	ftruncate(0, 0);
	mmap(0,0,0,0,0,0);
	msync(0,0,0);
	munmap(0,0);
}
