/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

static char prog_copyright[] = "Copyright 1991 Intel Corp. xxxxxx";

#ident	"@(#)ihvkit:net/dlpi_ether_2.x/el16.cf/Space.c	1.1"
#ident  "$Header$"

#include <sys/types.h>
#include <config.h>

/*
 * The N_BOARDS defines how may el16 boards are supported by this driver.
 */
#define	N_BOARDS	4

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  EL16  board.  A TCP/IP environment requires a minimum of two (IP and ARP).
 *  Putting an excessively large value here would waste memory.  A value that
 *  is too small could prevent a system from supporting a desired protocol.
 */
#define	N_SAPS		8

/*
 * The next two configurable parameters (CABLE_TYPE, ZWS) will only be
 * used when there's 1 board configured on the system. Since there's only one
 * port (port 0x100) to configure the board, multiple boards can't be configured
 * through the same port. To configure multiple boards, read the EthereLink 16
 * Installation and Configuration Guide and make sure the same parameters are
 * set in the Unix systems configuration file.
 */

/*
 * The ZWS (Zero Wait State) define determines the wait state of the RAM.
 * A value of 0 disables 0WS. A value of 1 enable 0WS.
 */
#define ZWS	1

/*
 *  The STREAMS_LOG define determines if STREAMS tracing will be done in the
 *  driver.  A non-zero value will allow the strace(1M) command to follow
 *  activity in the driver.  The driver ID used in the strace(1M) command is
 *  equal to the ENET_ID value (generally 2101).
 *
 *  NOTE:  STREAMS tracing can greatly reduce the performance of the driver
 *         and should only be used for trouble shooting.
 */
#define	STREAMS_LOG	0

/*
 *  The IFNAME define determines the name of the the internet statistics
 *  structure for this driver and only has meaning if the inet package is
 *  installed.  It should match the interface prefix specified in the strcf(4)
 *  file and ifconfig(1M) command used in rc.inet.  The unit number of the
 *  interface will match the board number (i.e emd0, emd1, emd2) and is not
 *  defined here.
 */
#define	IFNAME	"el16"

/********************** STOP!  DON'T TOUCH THAT DIAL ************************
 *
 *  The following values are set by the kernel build utilities and should not
 *  be modified by mere motals.
 */
int		el16zws = ZWS;
int		el16saad;
int		el16strlog = STREAMS_LOG;
int		el16nsaps = N_SAPS;		/* # possible protocols */
int		el16nboards = N_BOARDS;
int		el16cmajor = EL16_CMAJOR_0;	/* first major # in range */
char		*el16_ifname = IFNAME;		/* interface name */

#ifdef IP
int	el16inetstats = 1;
#else
int	el16inetstats = 0;
#endif
