/*		copyright	"%c%" 	*/

#ifndef _IO_RAMD_RAMD_H	/* wrapper symbol for kernel use */
#define _IO_RAMD_RAMD_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/ramd/ramd.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1983, 1984, 1986, 1987, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */

/*
 * RAM disk device driver include file.
 */

#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

struct	ramd_info {
	ulong	ramd_size;	/* size of disk in bytes */
	ushort	ramd_flag;	/* see defs below */
	unchar	ramd_ocnt;	/* count of layered opens */
	unchar	ramd_otyp;	/* bit mask of other opens */
	dev_t	ramd_maj;	/* major device to load from */
	dev_t	ramd_min;	/* minor device to load from */
	ulong	ramd_state;	/* runtime state, see defs below */
	caddr_t	ramd_addr;	/* kernel virtual addr */
};

extern struct ramd_info ramd_info[];
extern struct buf	ramd_buf[];
extern minor_t		ramd_num;

/*
 * General defines.
 */
#define RAMD_DIV_BY_512		9	/* shift for 512 divide */
#define RAMD_DIV_BY_1024	10	/* shift for 1024 divide */
#define RAMD_GRAN		0x1000	/* ramd disk transfer size */
#define RAMD_TAPE_LOAD		0x0001	/* LOAD is from TAPE */
#define RAMD_FLOPPY_LOAD	0x0002	/* LOAD is from FLOPPY */
#define RAMD_NO_LOAD		0x0004	/* don't LOAD */

/*
 * Flag definitions for ramd_flag.
 */
#define RAMD_STATIC		0x01	/* Ramd disk statically allocated */
#define RAMD_RUNTIME		0x02	/* Runtime definable RAM Disk */
#define RAMD_LOAD		0x04	/* Auto fill the RAM Disk at init */

/*
 * State definitions for ramd_state.
 */
#define RAMD_ALIVE		0x01	/* Disk is present */
#define RAMD_OPEN		0x02	/* Disk is open */
#define RAMD_ALLOC		0x04	/* Memory has been allocated */
#define RAMD_FAIL		0x08	/* Disk configuration is not usable */

/*
 * Ram Disk ioctls.
 */
#define RAMD_IOC_GET_INFO	0x00	/* Return ramd_info structure */
#define RAMD_IOC_R_ALLOC	0x01	/* Allocate ramd memory space */
#define RAMD_IOC_R_FREE		0x02	/* De-allocate ramd memory space */
#define RAMD_IOC_LOAD		0x04	/* Load a ramd partition */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_RAMD_RAMD_H */
