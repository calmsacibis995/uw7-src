#ident	"@(#)Space.c	1.3"
#ident	"$Header$"

/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	


/*
 * Reserve storage for Generic serial driver
 */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/termio.h>
#include <sys/termiox.h>
#include <sys/strtty.h>
#include <sys/cred.h>
#include <sys/ddi.h>
#include <sys/iasy.h>
#include "config.h"


int	iasy_num = IASY_UNITS;
major_t	iasy_major = IASY_CMAJOR_0;


struct termiox  asy_xtty[IASY_UNITS];	/* Per device termiox structure */
struct strtty  asy_tty[IASY_UNITS];	/* strtty structs for each device. 
					 * iasy_tty is changed to asy_tty for
					 * merge.
					 */
struct iasy_hw iasy_hw[IASY_UNITS];	/* Hardware support routines */
struct iasy_sv iasy_sv[IASY_UNITS];	/* sync. variables per port */
toid_t iasy_toid[IASY_UNITS];		/* timeout IDs per port */

int strhead_iasy_hiwat = 5120;
int strhead_iasy_lowat = 4096;

/* 
 * Effective on debug driver only.
 */
int iasy_debug = 0;


