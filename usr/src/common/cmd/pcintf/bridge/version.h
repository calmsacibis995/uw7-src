#ident	"@(#)pcintf:bridge/version.h	1.1.1.5"
/* SCCSID(@(#)version.h	6.9	LCC);	/* Modified: 16:35:47 4/16/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/******************************************************************************
*
*  This file is included by all of the main files that make up the unix
*  server portions of PCI.
*
*****************************************************************************/

#define VERS_MAJOR	4
#define VERS_MINOR	0
#define VERS_SUBMINOR	3

struct version { 
	short 	vers_major,
		vers_minor,
		vers_subminor;
	};



extern char server_version[];
extern char *bridge_version;

extern unsigned short bridge_ver_flags;

/* flags for bridge_ver_flags: */
#define	V_FAST_LSEEK	0x0001
#define	V_ERR_FILTER	0x0002
#define V_FAST_FIND	0x0004
