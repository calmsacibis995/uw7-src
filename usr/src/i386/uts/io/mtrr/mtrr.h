#ident	"@(#)kern-i386:io/mtrr/mtrr.h	1.2"
#ident	"$Header$"

/*****************************************************************/
/*            Copyright (c) 1994 Intel Corporation               */        
/*                                                               */
/* All rights reserved.  No part of this program or publication  */
/* may be reproduced, transmitted, transcribed, stored in a      */
/* retrieval system, or translated into any language or computer */
/* language, in any form or by any means, electronic, mechanical */
/* magnetic, optical, chemical, manual, or otherwise, without    */
/* the prior written permission of Intel Corporation.            */
/*                                                               */
/*****************************************************************/
/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

typedef struct {
	unsigned long int l;
	unsigned long int h;
	} ulonglong_t;

typedef struct {
	unsigned long int addr;
	unsigned long int len;
	int type;
	int misc;
	} mtrr_t;

#define IOCTL_MEM_TYPE_GET		0x8000
#define IOCTL_VAR_MTRR_DISABLE	0x8001
#define IOCTL_GET_DEFAULT		0x8002
#define IOCTL_SET_DEFAULT		0x8003
#define IOCTL_MEM_TYPE_SET		0x8004
#define IOCTL_RESTORE_INIT		0x8005
#define IOCTL_GET_VAR_MTRR		0x8006

#define MIXED_TYPES				-1
#define USWC_NOT_SUPPORTED		-2
#define MTRR_NOT_SUPPORTED		-3
#define RANGE_OVERLAP			-4
#define VAR_NOT_SUPPORTED		-5
#define VAR_NOT_AVAILABLE		-6
#define INVALID					-7
#define INVALID_VAR_REQ			-8
#define INVALID_MEM_TYPE		-9
#define INVALID_VAR_MTRR		-10
#define ENGINES_OFFLINE			-11
#define VAR_MTRR_NOT_SET	-12

/* The following are values returned by in the misc field of mtrr_t */
/* 0-255 reserved for variable MTRRs */
#define MTRR_MAPPED_FIXED		-1
#define MTRR_MAPPED_DEFAULT		256
#define MTRR_MAPPED_MULTIPLE		257

#define UC 0			/* Uncacheable */
#define USWC 1			/* Unspeculative Write Combining */
#define WT 4			/* Write Through */
#define WP 5			/* Write Protect */
#define WB 6			/* Write Back */
