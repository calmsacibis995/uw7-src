#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/spool.c	1.6"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<limits.h>
#include	<string.h>
#include	<Gizmos.h>
#include	"error.h"
#include	"uucp.h"
#include	"dtcopy.h"

extern void     NotifyUser();

int
mkdirs(base, extention, ownerid)
char	*base;
char	*extention;
int	ownerid;
{
	char	*dirp;
	char	*s1;
	char	path[PATH_MAX];
	char	ext[PATH_MAX];
	struct	stat	buf;
	mode_t	um;

	(void)strcpy(path, base);
	(void)strcpy(ext, extention);

	if (*LASTCHAR(path) != '/')
		(void)strcat(path, "/");

	if (*LASTCHAR(ext) != '/')
		(void)strcat(ext, "/");

	s1 = ext;

	um = umask(0022);

	while( (dirp = strtok(s1, "/")) != (char *)NULL ) {
		s1 = (char *)NULL;
		(void)strcat(path, dirp);
		if ( DIRECTORY(path, buf) ) {
			/* LATER need to check group and other if not owner */
			if ( buf.st_uid == ownerid ) {
				(void)strcat(path, "/");
				continue;
			}
                        NotifyUser(sf->toplevel, GGT(string_noAccessNodeDir));
				return(-1);
		}
		if (mkdir(path, MODE) == -1) {
                        NotifyUser(sf->toplevel, GGT(string_noAccessNodeDir));
			return(-1);
		}
		(void)strcat(path, "/");
	}

	(void)umask(um);

	return(0);
}
