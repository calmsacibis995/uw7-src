#ifndef NOIDENT
#pragma ident	"@(#)stat.c	15.1"
#endif

#define STAT_C

#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


time_t
StatFile (filename)
char *		filename;
{
	struct stat status;
	
	if (stat (filename, &status) == -1) {
		return (time_t)0;
	}
	return status.st_mtime;
}
