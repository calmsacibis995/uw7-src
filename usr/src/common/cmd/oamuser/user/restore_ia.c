#ident  "@(#)restore_ia.c	1.3"

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<shadow.h>
#include	<ia.h>

extern	int errno;

/*Procedure:	restore_ia()
 *
 *NOTE:		Restores attributes of old and new master and index
 *		files.  Ignores error return if file does not exist.
 */

int
restore_ia()
{

	struct  stat	statbuf;

	errno = 0;

	/*  File attributes should be the same as the SHADOW file */
	if (stat(SHADOW, &statbuf) < 0) {
		return errno;
	}
	
	(void) chmod(MASTER,statbuf.st_mode);
	(void) chmod(INDEX,statbuf.st_mode);
	(void) chmod(OINDEX,statbuf.st_mode);
	(void) chmod(OMASTER,statbuf.st_mode);
	(void) chown(MASTER,statbuf.st_uid, statbuf.st_gid);
	(void) chown(OMASTER,statbuf.st_uid, statbuf.st_gid);
	(void) chown(INDEX,statbuf.st_uid, statbuf.st_gid);
	(void) chown(OINDEX,statbuf.st_uid, statbuf.st_gid);

	return;
}
