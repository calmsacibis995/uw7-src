/*      Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.      */
/*      Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.        */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.      */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */


/*
 * File Name: t160.h
 * This file has the constant and structure definitions for pc9010 chip
 */
#ident	"@(#)kern-pdi:io/hba/zl5380/t160.h	1.1"

#ifndef T160_IO_HBA_INCLUDED
#define T160_IO_HBA_INCLUDED

#if defined (__cplusplus) 
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_hier.h>
#include <fs/buf.h>
#include <util/ksynch.h>
#include <io/ddi.h>

#elif defined (_KERNEL)

#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/sdi.h>
#include <sys/sdi_hier.h>
#include <sys/buf.h>
#include <sys/ksynch.h>
#include <sys/ddi.h>

#endif /* _KERNEL_HEADERS */


/*
 * PC9010 register offsets
 */
#define PC9010_CONFIG		0
#define PC9010_CONTROL		2
#define PC9010_FIFO_STATUS	3
#define PC9010_FIFO		4

#define CONFIG		(int)(zl5380ha->ha_base + PC9010_CONFIG)
#define CONTROL		(int)(zl5380ha->ha_base + PC9010_CONTROL)
#define FIFO_STATUS	(int)(zl5380ha->ha_base + PC9010_FIFO_STATUS)
#define FIFO		(int)(zl5380ha->ha_base + PC9010_FIFO)
		

#define PC9010_FIFO_SIZE	128

/*
 * PC9010 Configuration Register bits
 */
#define CFR_VERSION	0xF0
#define CFR_SWITCH	0x0F

#define TRANSFER_MODE_8BIT	0x02

#define PC9010_JEDEC_ID	0x8f

/*
 * PC9010 Control Register bits
 */
#define CTR_CONFIG	0x80
#define CTR_SWSEL	0x40
#define CTR_IRQEN	0x20
#define CTR_DMAEN	0x10
#define CTR_FEN		0x8
#define CTR_FDIR	0x4
#define CTR_F16		0x2
#define CTR_FRST	0x1

/*
 * PC9010 FIFO status register bits
 */
#define FSR_IRQ		0x80
#define FSR_DRQ		0x40
#define FSR_FHFUL	0x20
#define FSR_FHEMP	0x10
#define FSR_FLFUL	0x08
#define FSR_FLEMP	0x04
#define FSR_FFUL	0x02
#define FSR_FEMP	0x01


#if defined (__cplusplus)
	}
#endif

#endif  /* T160_IO_HBA_INCLUDED */
