#ident	"@(#)ksh93:src/lib/libast/obsolete/setcwd.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use pathcd */

int
setcwd(char* path, char* home)
{
	return(pathcd(path, home));
}
