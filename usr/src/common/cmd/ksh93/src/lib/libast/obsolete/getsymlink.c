#ident	"@(#)ksh93:src/lib/libast/obsolete/getsymlink.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use pathgetlink */

#include <ast.h>

int
getsymlink(const char* name, char* buf, int siz)
{
	return(pathgetlink(name, buf, siz));
}
