#ident	"@(#)kern-i386at:io/target/st01/st01.cf/Space.c	1.2.2.1"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include "config.h"


struct dev_spec *st01_dev_spec[] = {
        0
};

struct dev_cfg ST01_dev_cfg[] = {
{       SDI_CLAIM|SDI_ADD, DEV_CFG_UNUSED, DEV_CFG_UNUSED, 
		DEV_CFG_UNUSED, ID_TAPE, 0, "", DEV_CFG_UNUSED   },
};

int ST01_dev_cfg_size = sizeof(ST01_dev_cfg)/sizeof(struct dev_cfg);

int	St01_cmajor = ST01_CMAJOR_0;	/* Character major number	*/

int	St01_jobs = 20;		/* Allocation per LU device	*/

int	St01_reserve = 1;	/* Flag for reserving tape on open */

int	St01_stats_enabled = 0;	/* Flag for enabling tape stats and */
				/* and making driver ignore unload  */
				/* so it stays resident in memory   */
