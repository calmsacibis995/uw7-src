#ident	"@(#)ihvkit:display/lfb256/lfbCache.c	1.1"

/*
 * Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	Copyright (c) 1993  Intel Corporation
 *	  All Rights Reserved
 */

#include <lfb.h>

/*	SDD MEMORY CACHING CONTROL	*/
/*		OPTIONAL		*/

SIBool lfbAllocCache(buf, type)
SIbitmapP buf;
SIint32 type;
{
    return(SI_FAIL);
}

SIBool lfbFreeCache(buf)
SIbitmapP buf;
{
    return(SI_FAIL);
}

SIBool lfbLockCache(buf)
SIbitmapP buf;
{
    return(SI_FAIL);
}

SIBool lfbUnlockCache(buf)
SIbitmapP buf;
{
    return(SI_FAIL);
}
