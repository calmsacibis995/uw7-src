#ident	"@(#)kern-i386at:io/target/mc01/mc01.cf/Space.c	1.1.1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include "config.h"

struct dev_spec *mc01_dev_spec[] = {
        0
};

struct dev_cfg MC01_dev_cfg[] = {
{       SDI_CLAIM|SDI_ADD, DEV_CFG_UNUSED, DEV_CFG_UNUSED, 
		DEV_CFG_UNUSED, ID_CHANGER, 0, "", DEV_CFG_UNUSED  },
};

int MC01_dev_cfg_size = sizeof(MC01_dev_cfg)/sizeof(struct dev_cfg);

int Mc01_cmajor = MC01_CMAJOR_0;	/* Character major number	*/

int Mc01_jobs = 100;		/* Allocation per LU device 	*/
