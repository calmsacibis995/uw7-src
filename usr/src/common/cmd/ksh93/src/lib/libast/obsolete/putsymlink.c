#ident	"@(#)ksh93:src/lib/libast/obsolete/putsymlink.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use pathsetlink */

#include <ast.h>

int
putsymlink(const char* buf, const char* name)
{
	return(pathsetlink(buf, name));
}
