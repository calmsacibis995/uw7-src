#ident	"@(#)libelf:i386/svr4.c	1.1"


#include "syn.h"
#include <sys/utsname.h>
#include "libelf.h"
#include "decl.h"


int
_elf_svr4()
{
	struct utsname	u;
	static int	vers = -1;

	if (vers == -1)
	{
		if (uname(&u) > 0)
			vers = 1;
		else
			vers = 0;
	}
	return vers;
}
