#ident	"@(#)cscope:common/vpfopen.c	1.3"
/* vpfopen - view path version of the fopen library function */

#include <stdio.h>
#include <string.h>
#include "vp.h"
#include "global.h"

FILE *
vpfopen(filename, type)
char	*filename, *type;
{
	char	buf[MAXPATH + 1];
	FILE	*returncode;
	int	i;

	if ((returncode = myfopen(filename, type)) == NULL && filename[0] != '/' &&
	    strcmp(type, "r") == 0) {
		vpinit((char *) 0);
		for (i = 1; i < vpndirs; i++) {
			(void) sprintf(buf, "%s/%s", vpdirs[i], filename);
			if ((returncode = myfopen(buf, type)) != NULL) {
				break;
			}

		}
	}
	return(returncode);
}
