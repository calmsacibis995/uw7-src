#ident	"@(#)kern-i386:util/kdb/scodb/name.h	1.1"
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
/*
*	A name must not be a prefix operator (ie prefix unary)
*	or the name will never be noticed, as the operators come
*	first!
*		Unary prefix operators:
*			!
*			-
*			*
*			~
*			&
*/

#define		N_REGISTER	"%"
#define		N_BREAKPOINT	"#"
#define		N_ARGUMENT	"@"
#define		N_VARIABLE	"$"
