#ident	"@(#)libc-i386:gen/p_info.c	1.10"

				/* this define must be before the includes */
#define PSRINFO_STRINGS		/* to get ascii processor information */

#include "synonyms.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/prosrfs.h>
#include <sys/fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#pragma weak processor_info = _processor_info

/*
 *	processor_info(processorid_t processorid, processor_info_t *infop)
 *
 *	get details about a processor from the processor file system
 */

int
processor_info(processorid_t processorid, processor_info_t *infop)
{
	char buf[32];
	pfsproc_t p;
	int fd, i;
	char *s;
	
	/* find out if the processor file system is mounted */

	if (access(PFS_CTL_FILE, F_OK) != 0) {
		errno = ENOENT;
		return -1;
	} 
	
	/* convert processorid to a path */

	snprintf(buf, sizeof(buf), PFS_DIR "/" PFS_FORMAT, processorid);

	if ((fd = open(buf, O_RDONLY)) < 0) {
		errno = EINVAL;
		return -1;
	}
	
	i = read(fd, &p, sizeof(pfsproc_t)); /* read in the processor info */
	close(fd);

	/*
	 * Additional information was added to the end of the profs file
	 * in UnixWare 7.  We're prepared to handle either the new format
	 * or the old, but first make sure we got one or the other.
	 * If we're running on a system that has the extra information
	 * (which includes the "real" processor type) we'll use it;
	 * otherwise we'll behave as before.
	 */

	if (i == sizeof(pfsproc_t))
		s = p.pfs_name;
	else if (i == sizeof(procfile_t))
		s = PFS_CHIP_TYPE(p.pfs_chip);
	else {
		errno = EIO;
		return -1;
	}
	snprintf(infop->pi_processor_type,
		sizeof(infop->pi_processor_type), "%s", s);

	/* translate the rest of the info for our caller */

	infop->pi_state = p.pfs_status;
	infop->pi_nfpu = (p.pfs_fpu != FPU_NONE);
	infop->pi_clock = p.pfs_clockspeed;
	strcpy(infop->pi_fputypes, PFS_FPU_TYPE(p.pfs_fpu));

	return 0;
}
