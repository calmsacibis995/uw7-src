/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

static char prog_copyright[] = "Copyright 1991 Intel Corp. xxxxxx";

#ident	"@(#)ihvkit:net/dlpi_ether/el16.cf/Space.c	1.1"

#ident  "$Header$"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>

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
 *  The CABLE_TYPE define determines the ethernet cable type.  A value of 1
 *  indicates the ethernet controller is to be attached to thick ethernet
 *  cable (AUI).  A value of 0 indicates a thin ethernet cable (BNC).
 */
#define	CABLE_TYPE0	1
#define	CABLE_TYPE1	1
#define	CABLE_TYPE2	1
#define	CABLE_TYPE3	1

/*
 * The ZWS (Zero Wait State) define determines the wait state of the RAM.
 * A value of 0 disables 0WS. A value of 1 enable 0WS.
 */
#define ZWS 1

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
int	el16zws = ZWS;
int	el16saad ;
int		el16boards = EL16_CNTLS;
int		el16strlog = STREAMS_LOG;
char		*el16_ifname = IFNAME;
DL_sap_t	el16saps[ N_SAPS * EL16_CNTLS ];
DL_bdconfig_t	el16config[ EL16_CNTLS ] = {
#ifdef EL16_0
	{
		EL16_CMAJOR_0,		/* Major number			*/
		EL16_0_SIOA,		/* Start of base I/O address	*/
		EL16_0_EIOA,		/* End of base I/O address	*/
		EL16_0_SCMA,		/* Start of base memory address	*/
		EL16_0_ECMA,		/* End of base memory address	*/
		EL16_0_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef EL16_1
	{
		EL16_CMAJOR_1,		/* Major number			*/
		EL16_1_SIOA,		/* Start of base I/O address	*/
		EL16_1_EIOA,		/* End of base I/O address	*/
		EL16_1_SCMA,		/* Start of base memory address	*/
		EL16_1_ECMA,		/* End of base memory address	*/
		EL16_1_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef EL16_2
	{
		EL16_CMAJOR_2,		/* Major number			*/
		EL16_2_SIOA,		/* Start of base I/O address	*/
		EL16_2_EIOA,		/* End of base I/O address	*/
		EL16_2_SCMA,		/* Start of base memory address	*/
		EL16_2_ECMA,		/* End of base memory address	*/
		EL16_2_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef EL16_3
	{
		EL16_CMAJOR_3,		/* Major number			*/
		EL16_3_SIOA,		/* Start of base I/O address	*/
		EL16_3_EIOA,		/* End of base I/O address	*/
		EL16_3_SCMA,		/* Start of base memory address	*/
		EL16_3_ECMA,		/* End of base memory address	*/
		EL16_3_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
};

/*
 *  If there are multiple EL16 boards in the same system, make sure there is
 *  a comma separated value for each one.
 */
int	el16_cable_type[ EL16_CNTLS ] = {
#ifdef EL16_0
		CABLE_TYPE0,
#endif
#ifdef EL16_1
		CABLE_TYPE1,
#endif
#ifdef EL16_2
		CABLE_TYPE2,
#endif
#ifdef EL16_3
		CABLE_TYPE3,
#endif
};

#ifdef IP
int	el16inetstats = 1;
#else
int	el16inetstats = 0;
#endif
