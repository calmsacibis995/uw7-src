/*      Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.      */
/*      Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.        */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.      */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */


/*
 * This file has the definitions specific to Media Vision Pro Audio
 * Spectrum 16 card
 */

#ident	"@(#)kern-pdi:io/hba/zl5380/mvpas16.h	1.1"

#ifndef MV_PAS16_IO_HBA_INCLUDED
#define MV_PAS16_IO_HBA_INCLUDED

#ifdef _KERNEL_HEADERS

#include <io/target/scsi.h>
#include <util/types.h>

#elif defined (_KERNEL)

#include <sys/scsi.h>
#include <sys/types.h>

#endif /* _KERNEL */

/*
 * 	Media Vision Board Configuration Registers
 */

#define	ZL5380_IO_ADDRESS_OFFSET	0x1c00
#define	MV_MASTER_DECODE_REGISTER	0x9a01

#define	MV_IO_CONFIGURATION_REGISTER_1	0xf000
#define	MV_IO_CONFIGURATION_REGISTER_2	0xf001
#define	MV_IO_CONFIGURATION_REGISTER_3	0xf002

#define	MV_SYSTEM_CONFIGURATION_REGISTER_1	0x8000
#define	MV_SYSTEM_CONFIGURATION_REGISTER_2	0x8001
#define	MV_SYSTEM_CONFIGURATION_REGISTER_3	0x8002
#define	MV_SYSTEM_CONFIGURATION_REGISTER_4	0x8003

#define	MV_IO_CONFIGURATION_REGISTER_3_MASK		0x0f
#define	MV_SYSTEM_CONFIGURATION_REGISTER_4_DATA		0x6d
#define	MV_PAS16_IRQ_SHIFT_COUNT			0x04
#define	MV_PAS16_IRQ_TABLE_LENGTH			16 
#define	MV_MASTER_DECODE_REGISTER_SHIFT_COUNT		0x02

/*
 *	Media Vision PAS16 Board IDs
*/

#define	BOARD_ID_1	0xbc
#define	BOARD_ID_2	0xbd
#define	BOARD_ID_3	0xbe
#define	BOARD_ID_4	0xbf

/*
 *	Media Vision PAS16 Base Addresses
*/

#define	BASE_ADDR_1	0x388
#define	BASE_ADDR_2	0x38c
#define	BASE_ADDR_3	0x384
#define	BASE_ADDR_4	0x288

#endif /* MV_PAS16_IO_HBA_INCLUDED*/
