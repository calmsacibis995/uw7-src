#ident	"@(#)cvtomf:coff.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*
 *	Copyright (c) Altos Computer Systems, 1987
 */


/* Enhanced Application Compatibility Support */
#define		ALIGNED(x)	(!((x) & 3))

#define		TXTFILL		(0x90)
#define		DATFILL		(0x00)
/* End Enhanced Application Compatibility Support */
