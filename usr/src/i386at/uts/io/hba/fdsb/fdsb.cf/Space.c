/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/fdsb/fdsb.cf/Space.c	1.2.1.1"
/*
 * Space.c - Data requirements for Future Domain TMC-18XX/3670-chip SCSI Host
 * Adapter driver.
 */
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/fdsb.h>
#include <sys/conf.h>
#include "config.h"

struct fdc_idev fds_table[] =
{
    /*          111111111122222222223333333                     */
    /*0123456789012345678901234567890123456 len   flags         */
    {"STARTFutureDomainDeviceConfigTable@#", 36,      0, { 0 }},
    {"SPARE                               ",  0,      0, { 0 }},
    {"SPARE                               ",  0,      0, { 0 }},
    {"SPARE                               ",  0,      0, { 0 }},
    {"ENDFutureDomainDeviceConfigTable@#@#",  0,      0, { 0 }}
};

int fds_entries = sizeof(fds_table)/sizeof(fdc_idev_t);

int     fdsb_cntls = 1;
int     fdsb_gtol[SDI_MAX_HBAS];
int     fdsb_ltog[SDI_MAX_HBAS];

struct  ver_no          fdsb_sdi_ver;

#ifdef  FDSB_CPUBIND
    #define FDSB_VER        (HBA_UW21_IDATA | HBA_IDATA_EXT)
    struct  hba_ext_info    fdsb_extinfo = {
        0, FDSB_CPUBIND
    };
#else
    #define FDSB_VER        HBA_UW21_IDATA
#endif

struct  hba_idata_v5    _fdsb_idata[] = {
    { FDSB_VER, "(fdsb,1) Future Domain SCSI", 7, 0, -1, 0, -1, 0 }
#ifdef FDSB_CPUBIND
    ,
    { HBA_EXT_INFO, (char *)&fdsb_extinfo }
#endif
};
