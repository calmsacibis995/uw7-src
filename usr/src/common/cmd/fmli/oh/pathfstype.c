/*		copyright	"%c%" 	*/

/*LINTLIBRARY*/
#ident	"@(#)fmli:oh/pathfstype.c	1.2.3.3"

/*
 *	This function returns the identifier of the filesystem that
 *	the path arguement resides on.  If any errors occur, it
 *	return s5 as a default.
 */

#include "inc.types.h"		/* abs s14 */
#include <sys/statfs.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>

static char fs_buf[FSTYPSZ];
static char fs_default[]="s5";

char *
path_to_fstype(path)
char *path;
{
	struct statfs stat_buf;

	if ( statfs(path,&stat_buf,sizeof(struct statfs),0) ) {
		return(fs_default);
	}

	if ( sysfs(GETFSTYP,stat_buf.f_fstyp,fs_buf) ) {
		return(fs_default);
	}

	return(fs_buf);
}
