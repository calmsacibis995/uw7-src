/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/dpt/dpt.cf/Space.c	1.15.5.1"
#ident	"$Header$"

#include <sys/sdi.h>
#include <sys/dpt.h>

char	dpt_modname[] = MODNAME;		/* driver modname */

scsi_ha_t *dpt_sc_ha[SDI_MAX_HBAS];	/* SCSI HA structures */

int	dpt_max_jobs = 64;	/* Maximum jobs concurrently on HBA. The 
				 * hardware supports up to 64 concurrent jobs
			 	 */
int	dpt_reserve_release = 0;

int	dpt_targetmode = 0;	/* 0 = disable, 1 = enable
				 * Targetmode should only be enabled with 
				 * firmware version 007GN and later.
				 */
#define	DPT_VER		HBA_UW21_IDATA | HBA_EXT_ADDRESS

struct	hba_idata_v5	_dptidata[]	= {
	{ DPT_VER, "(dpt,1) DPT SCSI" }
};

/*
 * This is the maximum number of msec's the driver will wait for the
 * controller to flush it's cache for a given LUN.
 */
int	dpt_flush_time_out = 30000;

