#ident	"@(#)ksh93:src/lib/libast/comp/getwd.c	1.1"
#pragma prototyped
/*
 * getwd() using getcwd()
 *
 * some getwd()'s are incredible
 */

#include <ast.h>

char*
getwd(char* path)
{
	if (getcwd(path, PATH_MAX)) return(path);
	strcpy(path, "getwd: error in . or ..");
	return(0);
}
