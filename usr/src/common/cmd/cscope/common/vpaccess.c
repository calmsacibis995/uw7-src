#ident	"@(#)cscope:common/vpaccess.c	1.3"
/* vpaccess - view path version of the access system call */

#include <stdio.h>
#include <unistd.h>
#include "vp.h"
#ifdef CCS
#include <sys/types.h>
#include "ccstypes.h"
#else
typedef int mode_t;
#endif
 
int vpaccess(path, amode)
char	*path;
mode_t	amode;
{
	char	buf[MAXPATH + 1];
	int	returncode;
	int	i;

	if ((returncode = access(path, amode)) == -1 && path[0] != '/') {
		vpinit((char *) 0);
		for (i = 1; i < vpndirs; i++) {
			(void) sprintf(buf, "%s/%s", vpdirs[i], path);
			if ((returncode = access(buf, amode)) != -1) {
				break;
			}
		}
	}
	return(returncode);
}
