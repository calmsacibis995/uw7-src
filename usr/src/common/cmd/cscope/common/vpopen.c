#ident	"@(#)cscope:common/vpopen.c	1.2"

/* vpopen - view path version of the open system call */

#include <stdio.h>
#include <fcntl.h>
#include "vp.h"

#define READ	0

vpopen(path, oflag)
char	*path;
int	oflag;
{
	char	buf[MAXPATH + 1];
	int	returncode;
	int	i;

	if ((returncode = myopen(path, oflag, 0666)) == -1 && path[0] != '/' &&
	    oflag == READ) {
		vpinit((char *) 0);
		for (i = 1; i < vpndirs; i++) {
			(void) sprintf(buf, "%s/%s", vpdirs[i], path);
			if ((returncode = myopen(buf, oflag, 0666)) != -1) {
				break;
			}
		}
	}
	return(returncode);
}
