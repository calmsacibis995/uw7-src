/*		copyright	"%c%" 	*/

#ident	"@(#)cpio:common/cmd/cpio/cpiostat.c	1.1.6.3"
#ident  "$Header$"
#define	_STYPES
#include <sys/types.h>
#include <sys/stat.h>
#include "cpio.h"

/* This file contains functions and structure for doing old (pre-SVR4.0) style
 * stats and lstats.  The difference is that the old style stat structure had
 * several fields of type short and ushort that are now long and ulong.
 *
 * If cpio is creating an archive with old style (BIN and CHR) header types,
 * any field that is too long to fit in an old style stat structure is probably
 * too long to fit in the cpio file header.  The stat call will fail with errno
 * EOVERFLOW in this case.
 *
 * The "#define _STYPES" at the begining of this file is what causes the stat
 * structure and functions to be old style.
 */

/*
 * Procedure:     svr32stat
 *
 * Restrictions:
                 stat(2): None
*/

svr32stat(fname, tmpinfo)
char *fname;
struct cpioinfo *tmpinfo;
{
	struct stat stbuf;
	int ret;

	if ((ret = stat(fname, &stbuf)) == 0) {
		tmpinfo->st_dev = stbuf.st_dev;
		tmpinfo->st_ino = stbuf.st_ino;
		tmpinfo->st_mode = stbuf.st_mode;
		tmpinfo->st_nlink = stbuf.st_nlink;
		tmpinfo->st_uid = stbuf.st_uid;
		tmpinfo->st_gid = stbuf.st_gid;
		tmpinfo->st_rdev = stbuf.st_rdev;
		tmpinfo->st_size = stbuf.st_size;
		tmpinfo->st_modtime = stbuf.st_mtime;
		tmpinfo->st_actime = stbuf.st_atime;
	}
	return(ret);
}

/*
 * Procedure:     svr32lstat
 *
 * Restrictions:
                 lstat(2): None
*/
svr32lstat(fname, tmpinfo)
char *fname;
struct cpioinfo *tmpinfo;
{
	struct stat stbuf;
	int ret;

	if ((ret = lstat(fname, &stbuf)) == 0) {
		tmpinfo->st_dev = stbuf.st_dev;
		tmpinfo->st_ino = stbuf.st_ino;
		tmpinfo->st_mode = stbuf.st_mode;
		tmpinfo->st_nlink = stbuf.st_nlink;
		tmpinfo->st_uid = stbuf.st_uid;
		tmpinfo->st_gid = stbuf.st_gid;
		tmpinfo->st_rdev = stbuf.st_rdev;
		tmpinfo->st_size = stbuf.st_size;
		tmpinfo->st_modtime = stbuf.st_mtime;
		tmpinfo->st_actime = stbuf.st_atime;
	}
	return(ret);
}
