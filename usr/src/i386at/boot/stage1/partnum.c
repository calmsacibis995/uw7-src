#ident	"@(#)stand:i386at/boot/stage1/partnum.c	1.1"
#ident  "$Header$"

#include <boot.h>
#include <sys/types.h>
#include <sys/fdisk.h>

#define OSTYP_UNUSED    0

/* Partition table is at a fixe location in RAM. */
const struct ipart * const parts = (const struct ipart *)0x7BE;

extern struct ipart *partptr;	/* pointer to selected partition */
extern char hd_param0[];	/* parameter string to update */

void
set_partnum(void)
{
	int i, j, k;
	const struct ipart *partp[FD_NUMPART];

	/*
	 * Sort partition table by starting sector. This causes partition
	 * numbers to be in disk order rather than order within the table.
	 */
	for (i = 0; i < FD_NUMPART; i++) {
		for (j = 0; j < i; j++) {
			if (parts[i].systid != OSTYP_UNUSED &&
			    (parts[i].relsect < partp[j]->relsect ||
			    partp[j]->systid == OSTYP_UNUSED)) {
				for (k = i; k-- > j;)
					partp[k + 1] = partp[k];
				break;
			}
		}
		partp[j] = &parts[i];
	}

	/*
         * Locate selsected partition.
         */
	for (i = 0; partp[i] != partptr; i++)
		;

	/* Patch partition number in default parameter string. */
	hd_param0[0] = i + '1';
}

