/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)pax:charmap.h	1.1"

/*
 * OSF/1 1.2
 */
/* @(#)$RCSfile$ $Revision$ (OSF) $Date$ */
/*
 * charmap.h - defnitions for charmap parsing
 *
 * DESCRIPTION
 *
 *	This file contains definitions required for parsing the
 *	charmap files.   It is included by charmap.c
 *
 * AUTHOR
 *
 *     Tom Jordahl	- The Open Software Foundation
 *
 */

#ifndef _CHARMAP_H
#define _CHARMAP_H

#define TRUE		1
#define FALSE		0

#define LMAX		1024		/* max number of chars in a line */

#define	CODE_SET_NAME	1
#define MB_MAX		2
#define MB_MIN		3
#define ESCAPE_CHAR	4
#define COMMENT_CHAR	5

/*
 * isportable is defined to return true if the character is
 * a member of the POSIX 1003.2 portable character set.
 *
 * right now we use isascii(), this should be correct
 */
#define isportable(c)	(isascii(c)) 

typedef struct value_struct {
    unsigned char	mbvalue[10];	/* multi-byte encoding */
    short		nbytes;		/* number of bytes in mbvalue */
} Value;

typedef struct charmap_struct {
    char		  *symbol;	/* symbol name */
    unsigned char 	  value[10];	/* symbols multi-byte encoding */
    short		  nbytes;	/* number of bytes in value */
    struct charmap_struct *next;	/* pointer to next struct */
} Charentry;

#endif /* _CHARMAP_H */
