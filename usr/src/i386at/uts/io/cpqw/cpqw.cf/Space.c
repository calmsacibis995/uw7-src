/*	Copyright (c) 1984 - 1995 Novell, Inc. All Rights Reserved.	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/cpqw/cpqw.cf/Space.c	1.1.2.1"

/********************************************************
 * Copyright 1994, 1995 COMPAQ Computer Corporation
 ********************************************************
 *
 * Title   : $RCSfile$
 *
 * Version : $Revision$
 *
 * Date    : $Date$
 *
 * Author  : $Author$
 *
 ********************************************************
 *
 * Change Log :
 *
 *           $Log$
 * Revision 1.6  1995/03/21  15:39:13  gandhi
 * Added copyright messages for Novell.  Code as released in UnixWare 2.01 N5.1
 *
 * Revision 1.5  1995/03/08  15:28:26  gandhi
 * Added conditional compiling to avoid errors on UnixWare (#ifdef __USLC__)
 *
 * Revision 1.4  1994/09/09  16:57:39  gandhi
 * Added RCS header.
 *
 *           $EndLog$
 *
 ********************************************************/

static char *rcsid = "$RCSfile$ $Revision$ COMPAQ EFS $Date$";

#include <config.h>
#include <sys/seg.h>
#ifndef __USLC__		/* Don't include if on UnixWare */
#include <sys/ci/cimpsw.h>
#endif


#define ENABLED                 1
#define DISABLED                0

long max_ECC_errors = 5;	/* maximum ECC errors per interval */
long ECC_interval   = 60;	/* interval = time in seconds = 60 seconds */


/*
 * Set variable to ENABLED to collect eisa bus util data.
 * Set variable to DISABLED to disable collecting eisa bus util data.
 */
char run_eisa_bus_util_mon = ENABLED;

/*
 * The number of clock ticks (Hz) to wait before re-enabling a temperature
 * or fan once it has been disabled.  (100 = 1 second)
 */
unsigned int alert_timeout_period = 100;

/*
 * The number of clock ticks (Hz) to wait before updating the Hobbs meter.
 * (100 = 1 second)  This value MUST be equal to 1 minute.
 */
unsigned int hobbs_update_interval = 6000; /* 60 * 100 */

/*
 * The number of minutes to allow the system to shutdown after an ASR
 * NMI has occurred.
 */
long asr_nmi_reboot_timeout = 2;


#ifndef __USLC__		/* Don't include if on UnixWare */
#ifdef MPSW_0
	extern struct mpsw *mpswp;
#else
	/* prevent unreferenced symbol error while re-linking */
	struct mpsw *mpswp;
#endif
#endif	/* __USLC__ */
