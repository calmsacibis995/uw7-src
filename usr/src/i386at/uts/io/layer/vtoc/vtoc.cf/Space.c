#ident	"@(#)kern-i386at:io/layer/vtoc/vtoc.cf/Space.c	1.1.4.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>

struct dev_cfg VTOC_dev_cfg[] = {
{	SDI_CLAIM, DEV_CFG_UNUSED, DEV_CFG_UNUSED, 
		DEV_CFG_UNUSED, ID_RANDOM, 0, "", DEV_CFG_UNUSED	},
};

int VTOC_dev_cfg_size = sizeof(VTOC_dev_cfg)/sizeof(struct dev_cfg);

char	VTOC_modname[] = "vtoc";	/* driver modname   */
