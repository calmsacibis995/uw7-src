#ident	"@(#)sco:executil.c	1.1"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/* Enhanced Application Compatibility Support */
#include <stdio.h>
#include <string.h>
#include "argtype.h"

#define CPATH "/usr/lib/coff/"
#define XPATH "/usr/lib/xout/"

char *mk_path( char *, char * );
char *malloc( int );

char *util_dirs[] = {	CPATH,
			XPATH
		    };


void
executil( type, utilname, av)
int type;
char *utilname;
char **av;
{
	char *exec_name;

	exec_name = mk_path( util_dirs[type], utilname );
	execv( exec_name, av );
	fprintf( stderr, "%s: Can't exec %s\n", utilname, exec_name );
	perror("");
	exit(1);
}


/* mk_path() -
 *	Concatenate 'dir' and 'file' into single pathname.
 *	'file' may be a pathname already, in which case the
 *	last component of 'file' is found and used.
 */

char *
mk_path( dir, file )
char *dir, *file;
{
	char *path;
	char *basenm;
	int len;

	basenm = strrchr( file, '/' );  /* point to last '/' in 'file' */

	if ( basenm )		/* if non-null */
	    ++basenm;		/* inc ptr to 1st char of last component */
	else			/* basenm is null, no '/' in 'file' */
	    basenm = file;

	len = strlen( dir ) + strlen( basenm ) + 1; /* +1 for '\0' */

	if ( (path = malloc( len )) == NULL )
	{   perror("Can' Allocate Memory");
	    exit(54);
	}
	strcpy( path, dir );
	strcat( path, basenm );
	return( path );
}
/* End Enhanced Application Compatibility Support */
