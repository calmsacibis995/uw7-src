#ident	"@(#)ksh93:src/lib/libast/string/fmterror.c	1.1"
#pragma prototyped

/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return error message string given errno
 */

#include <ast.h>

extern int	sys_nerr;
extern char	*gettxt();

char*
fmterror(int err)
{
	static char	msg[28];

	if (err > 0 && err <= sys_nerr) return(strerror(err));
	sfsprintf(msg, sizeof(msg), gettxt(":320","Error %d"), err);
	return(msg);
}
