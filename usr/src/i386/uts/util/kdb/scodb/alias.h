#ident	"@(#)kern-i386:util/kdb/scodb/alias.h	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) 1989-1992 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

#define		ALIASLOOP	10
#define		SUBARG		'!'
#define		NOALIAS		'/'

#define		ALNAMEL		16

struct alias {
	char	 al_name[ALNAMEL];
	char	*al_vec[DBNARG];
	char	 al_buf[DBIBFL];
};
