/*		copyright	"%c%" 	*/

#ident	"@(#)kern-i386at:io/ramd/ramd.cf/Space.c	1.1"
#ident	"$Header$"

/*	Copyright (c) 1987, 1988  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	


/*
 * RAM disk configuration file
 */
#include <config.h>
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/ramd.h>
#include <sys/ddi.h>

#ifdef RAMD_BOOT

#define RAMD_TAPE_FS_FILE_MARK	1
#define RAMD_BOOT_MAJOR		1
#define RAMD_BOOT_MINOR		132
#define RAMD_NUM		2

#define RAMD_SWAP	(768*4096)	/* must be 4k increments */
#define RAMD_SIZE	(196*1024)	/* Enough to fit /sbin/init, /dev
					 * wsinit autopush and  FP emulator
					*/
int ramd_load_type = RAMD_FLOPPY_LOAD;

int	RootRamDiskSize = RAMD_SIZE;
char	RootRamDiskBuffer[RAMD_SIZE] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
int	SwapRamDiskSize = RAMD_SWAP;

struct ramd_info ramd_info[RAMD_NUM] = {
	{	RAMD_SIZE,		/* Ram disk size */
		RAMD_STATIC,		/* Ram disk flags */
		0,			/* Open count */
		0,			/* Open bitmask */
		NODEV,			/* Major number to load from */
		NODEV,		 	/* Minor number to load from */
		0,			/* State info (set at runtime) */
		0			/* Ram disk vaddr (set at runtime) */
	},
	{	RAMD_SWAP,		/* Ram disk size */
		RAMD_STATIC,		/* Ram disk flags */
		0,			/* Open count */
		0,			/* Open bitmask */
		NODEV,			/* Major number to load from */
		NODEV,			/* Minor number to load from */
		0,			/* State info (set at runtime) */
		0			/* Ram disk vaddr (set at runtime) */
	}
};

#else /* RAMD_BOOT */

#define RAMD_TAPE_FS_FILE_MARK	0
#define RAMD_BOOT_MAJOR		-1
#define RAMD_BOOT_MINOR		-1
#define RAMD_NUM		1
#define RAMD_SIZE		0x0
#define RAMD_SWAP		0x0
int ramd_load_type = RAMD_NO_LOAD;

int	RootRamDiskSize = RAMD_SIZE;
char	RootRamDiskBuffer[1] = "";
int	SwapRamDiskSize = RAMD_SWAP;


struct ramd_info ramd_info[RAMD_NUM] = {
	{	RAMD_SIZE,		/* Ram disk size */
		RAMD_RUNTIME,		/* Ram disk flags */
		0,			/* Open count */
		0,			/* Open bitmask */
		NODEV,			/* Major number to load from */
		NODEV,			/* Minor number to load from */
		0,			/* State info (set at runtime) */
		0			/* Ram disk vaddr (set at runtime) */
	}
};

#endif /* RAMD_BOOT */

struct	buf		ramd_buf[RAMD_NUM];
minor_t	ramd_num =	RAMD_NUM;
int	ramd_tape_loc =	RAMD_TAPE_FS_FILE_MARK;
