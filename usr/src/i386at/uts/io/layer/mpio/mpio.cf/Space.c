#ident	"@(#)kern-i386at:io/layer/mpio/mpio.cf/Space.c	1.1.3.1"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>

int mpio_maxunits = 1048;
	/* 
	 * The max number of devices the driver can manage.
	 */

int *mpio_handle_hash_table_p;
	/*
	 *  Vfcd device handle hash table base pointer.
	 */

int mpio_debug = 1;
	/*
	 * Debug switch
	 * '0' disable.
	 * '1' enable.
	 */

int mpio_inactive_first = 0;
	/*
	 *  Redundant I/O path policy flag.
	 *  '0' deflect failed traffic to far path first.
	 *  '1' deflect failed traffic to alternate path first.
	 */

struct dev_cfg MPIO_dev_cfg[] = {
	{ SDI_CLAIM , DEV_CFG_UNUSED, DEV_CFG_UNUSED, DEV_CFG_UNUSED, ID_RANDOM, 0, "", DEV_CFG_UNUSED  },
};

	/*
	 * Required SDI object.
	 *
	 */
int MPIO_dev_cfg_size = sizeof(MPIO_dev_cfg)/sizeof(struct dev_cfg);

char	Mpio_modname[] = "mpio";	/* driver modname   */
