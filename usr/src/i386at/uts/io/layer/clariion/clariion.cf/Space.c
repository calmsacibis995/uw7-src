#ident	"@(#)kern-i386at:io/layer/clariion/clariion.cf/Space.c	1.1.3.1"

/*	Copyright (c) 1996 Data General Corp.                        */
/*	All Rights Reserved                                          */
/*	The copyright notice above does not evidence any             */
/*	actual or intended publication of such source code.          */

#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>

struct dev_cfg CLARIION_dev_cfg[] = {
	{ SDI_CLAIM , DEV_CFG_UNUSED, DEV_CFG_UNUSED, DEV_CFG_UNUSED, ID_RANDOM, 8, "DGC     RAID", DEV_CFG_UNUSED  },
};

int CLARIION_dev_cfg_size = sizeof(CLARIION_dev_cfg)/sizeof(struct dev_cfg);

char	CLARIION_modname[] = "clariion";	/* driver modname   */
