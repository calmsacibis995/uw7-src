#ident	"@(#)pdi.cmds:info.c	1.2"

#include	<sys/types.h>
#include	<sys/vtoc.h>
#include	<sys/fdisk.h>
#include	<sys/scsi.h>
#include "readxedt.h"
#include "info.h"

/*
 * gets information on a disk drive
 *
 * filename: the name of the device that is to be checked
 *           eg. /dev/dsk/c0b0t0d0s0
 *
 * minors_per: output, the number of minor #'s per device
 *             Note, not all of them may be used, they
 *             may simply be reserved
 *
 * num_partitions: output, the number of partitions on this device
 *
 * num_slice: output, the number of slices per partition
 *
 * returns 0 on success, -1 on failure
 * 
 * NOTES: currently only works with disk drives.  CDRoms are
 * not supported.
 */
/*ARGSUSED*/
int drive_device_info(char *filename, int *minors_per,
		      int *num_partitions, int *num_slices)
{
	struct scsi_xedt *edt;
	int edtcnt;

	*num_partitions = FD_NUMPART;
	*num_slices = V_NUMPAR;

	edt = readxedt(&edtcnt);

	if (edt == NULL) return -1;

	for (; edtcnt > 0; edtcnt--, edt++)
	{
		if (edt->xedt_pdtype == ID_RANDOM)
		{
			*minors_per = edt->xedt_minors_per;
			return 0;
		}
	}

	return -1;
}
