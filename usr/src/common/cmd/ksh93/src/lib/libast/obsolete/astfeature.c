#ident	"@(#)ksh93:src/lib/libast/obsolete/astfeature.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use astconf() */

#include <ast.h>

char*
astfeature(const char* name, const char* value)
{
	return(astconf(streq(name, "fs.3d") ? "FS_3D" : streq(name, "path.resolve") ? "PATH_RESOLVE" : streq(name, "universe") ? "UNIVERSE" : name, NiL, value));
}
