#ident	"@(#)kern-i386at:io/target/sp01/sp01.cf/Space.c	1.1.1.1"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/sp01.h>
#include "config.h"

struct dev_spec *sp01_dev_spec[] = {
        0
};

struct dev_cfg SP01_dev_cfg[] = {
{       SDI_CLAIM|SDI_ADD, DEV_CFG_UNUSED, DEV_CFG_UNUSED, 
		DEV_CFG_UNUSED, ID_PROCESOR, 0, "", DEV_CFG_UNUSED  },
};

int SP01_dev_cfg_size = sizeof(SP01_dev_cfg)/sizeof(struct dev_cfg);

int Sp01_cmajor = SP01_CMAJOR_0;	/* Character major number	*/
int Sp01_bmajor = SP01_BMAJOR_0;	/* Block major number		*/

int sp01_rwbuf_slot_size = SP01_RWBUF_SLOT_SIZE; /* Default slot size, 1024 */
