#ident	"@(#)debugger:libutil/common/debug_open.c	1.1"

/* maintain our own versions of open and fopen so
 * we can set up open files to close on exec
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int
debug_open(const char *path, int oflag, mode_t mode)
{
	int	fd;

	if (oflag & O_CREAT)
	{
		if ((fd = open(path, oflag, mode)) == -1)
			return -1;
	}
	else
	{
		if ((fd = open(path, oflag)) == -1)
			return -1;
	}
	
	if (fcntl(fd, F_SETFD, 1) == -1)
	{
		close(fd);
		return -1;
	}
	return fd;
}

int
debug_dup(int filedes)
{
	int	fd;

	if ((fd = dup(filedes)) == -1)
		return -1;
	
	if (fcntl(fd, F_SETFD, 1) == -1)
	{
		close(fd);
		return -1;
	}
	return fd;
}

FILE *
debug_fopen(const char *path, const char *type)
{
	FILE	*fptr;;

	if ((fptr = fopen(path, type)) == 0)
		return 0;
	
	if (fcntl(fileno(fptr), F_SETFD, 1) == -1)
	{
		fclose(fptr);
		return 0;
	}
	return fptr;
}
