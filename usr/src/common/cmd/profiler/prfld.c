/*		copyright	"%c%" 	*/

#ident	"@(#)profiler:prfld.c	1.11.7.3"
#ident	"$Header$"

/*
 *	prfld - load profiler with sorted kernel text addresses
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/prf.h>

main( int argc, char **argv ) {
	int prf_fd;
	char *namelist;
	void prf_load( const int prf_fd, const char *namelist );

	if(argc > 2)
		error("usage: prfld [system_namelist]");

	namelist = NULL;
	if(argc > 1)
		namelist = argv[1];

	if( (prf_fd = open("/dev/prf", O_RDWR)) < 0)
		error("cannot open /dev/prf");

	prf_load( prf_fd, namelist );

	exit(0);
}
