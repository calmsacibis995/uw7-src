/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/cpqsc/cpqsc.cf/Space.c	1.14.2.2"
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"

int	cpqsc_lowat = 3;	/* LU queue low water mark	*/
int	cpqsc_hiwat = 16;	/* LU queue high water mark	*/

struct	ver_no    cpqsc_sdi_ver;


int             cpqsc_gtol[SDI_MAX_HBAS];	/* global to local */
int             cpqsc_ltog[SDI_MAX_HBAS];	/* local to global */

#ifdef	CPQSC_CPUBIND

#define	CPQSC_VER		HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	cpqsc_extinfo = {
	0, CPQSC_CPUBIND
};

#else

#define	CPQSC_VER		HBA_UW21_IDATA

#endif	/* CPQSC_CPUBIND */

struct	hba_idata_v5	_cpqscidata[] = {
	{ CPQSC_VER, "(cpqsc,1) Compaq SCSI-2",
	  7, 0, -1 ,0, -1, 0 }
#ifdef	CPQSC_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&cpqsc_extinfo }
#endif
};

int	cpqsc_cntls	= 1;

/*
 * Normally the system will look at all LUNs of the device.  If
 * cpqsc_lun_fix is set to 1 then only the first LUN of all devices
 * will ever be accessed.  This is here as a possible work around
 * for incompatiblity with devices that have only one LUN.
 */
int     cpqsc_lun_fix = 0;

/*
 * If cpqsc_stats is set to 0, then the statistics will NOT be gathered
 */
int	cpqsc_stats = 1;

/*
 * If cpqsc_verbose is set to 1, then the more obscure cmn_err 
 * messages will be printed to putbuf.
 */
int	cpqsc_verbose = 0;
