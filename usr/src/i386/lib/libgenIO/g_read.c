/*		copyright	"%c%" 	*/


#ident	"@(#)libgenIO:i386/lib/libgenIO/g_read.c	1.1.5.3"
#ident "$Header$"

#include <errno.h>
#include <libgenIO.h>

/*
 * g_read: Read nbytes of data from fdes (of type devtype) and place
 * data in location pointed to by buf.  In case of end of medium,
 * translate (where necessary) device specific EOM indications into
 * the generic EOM indication of rv = -1, errno = ENOSPC.
 */

int
g_read(devtype, fdes, buf, nbytes)
int devtype, fdes;
char *buf;
unsigned nbytes;
{
	int rv;

	if (devtype < 0 || devtype >= G_DEV_MAX) {
		errno = ENODEV;
		return(-1);
	}
	if ((rv = read(fdes, buf, nbytes)) <= 0) {
		switch (devtype) {
			case G_FILE:	/* do not change returns for files */
				break;
			case G_NO_DEV:	/* returns -1 and ENOSPC */
			case G_TAPE:
				break;
			case G_3B2_HD: 
			case G_3B2_FD:
			case G_3B2_CTC:
				if (rv == 0 && errno == 0) {
					errno = ENOSPC;
					rv = -1;
				}
				break;
			case G_386_HD:	/* not developed yet */
			case G_386_FD:
			case G_386_Q24:
				if (rv == 0 && errno == 0) {
					errno = ENOSPC;
					rv = -1;
				}
				break;
			case G_SCSI_HD:
			case G_SCSI_FD:
			case G_SCSI_9T:
			case G_SCSI_Q24:
			case G_SCSI_Q120:
				break;
			default:
				rv = -1;
				errno = ENODEV;
		} /* devtype */
	} /* (rv = read(fdes, buf, nbytes)) <= 0 */
	return(rv);
}
