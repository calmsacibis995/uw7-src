#ident	"@(#)kern-i386:util/kdb/scodb/sent.h	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) 1989-1993 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

#define		SF_TEXT		0x01
#define		SF_DATA		0x02
#define		SF_STATIC	0x04

#define		SF_ANY		(SF_TEXT|SF_DATA)

#define		SYM_GLOBAL	0x100

/*
 * Symbol table entry with se_name field hidden
 */

#pragma pack(1)
struct sent {
	long	se_vaddr;		/*  0   4 */
	char	se_flags;		/*  4   1 */
	char	se_name[256];
};
#pragma pack()

/*
 * Definitions of routines required due to
 * the new and old symbol table formats.
 */

extern char *sent_name();		/* return pointer to symbol name */
extern struct sent *sent_prev();	/* return pointer to previous symbol
					   table entry */

/*
 * Structure of line number table.
 */

#pragma pack(2)
struct scodb_lineno {
	unsigned	l_addr;		/* virtual address */
	unsigned short	l_lnno;		/* line number */
};
#pragma pack()
