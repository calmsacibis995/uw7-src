/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/fdeb/fdeb.cf/Space.c	1.2.2.1"
/*
 * Space.c - Data requirements for Future Domain TMC-950-chip SCSI Host
 * Adapter driver.
 */
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/fdeb.h>
#include <sys/conf.h>
#include "config.h"

struct fdc_idev fds_table[] =
{
    /*          111111111122222222223333333                     */
    /*0123456789012345678901234567890123456 len   flags         */
    {"STARTFutureDomainDeviceConfigTable@#", 36,      0,   0,   0},
    {"NEC     CD-ROM                      ", 14,      0,   0,   1},
    {"HITACHI CDR-17                      ", 14,      0, 512,   0},
    {"HITACHI CDR-36                      ", 14,      0, 512,   0},
    {"ARCHIVE VIPER                       ", 13,      0,   2,   1},
    {"SPARE                               ",  0,      0,   0,   0},
    {"SPARE                               ",  0,      0,   0,   0},
    {"SPARE                               ",  0,      0,   0,   0},
    {"ENDFutureDomainDeviceConfigTable@#@#",  0,      0,   0,   0}
};

int fds_entries = sizeof(fds_table)/sizeof(fdc_idev_t);

int     fdeb_cntls = 1;
int     fdeb_gtol[SDI_MAX_HBAS];
int     fdeb_ltog[SDI_MAX_HBAS];

struct  ver_no          fdeb_sdi_ver;

#ifdef  FDEB_CPUBIND
#define FDEB_VER        (HBA_UW21_IDATA | HBA_IDATA_EXT)
struct  hba_ext_info    fdeb_extinfo = {
        0, FDEB_CPUBIND
};
#else
#define FDEB_VER        HBA_UW21_IDATA
#endif

struct  hba_idata_v5    _fdeb_idata[] = {
    { FDEB_VER, "(fdeb,1) Future Domain SCSI", 7, 0, -1, 0, -1, 0 }
#ifdef FDEB_CPUBIND
    ,
    { HBA_EXT_INFO, (char *)&fdeb_extinfo }
#endif
};
