#ident "@(#)atype.h	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 * This file contains definitions used by address-type handling code.
 */

/*
 * AtAddrParseFunc -- parsing function for address type
 * This function parses the a configuration entry
 * for a pool of this address type.  Input is taken from cf.
 * The function returns one address at a time in *addr.
 * init must point to an integer variable which is set to 1 before the
 * first call to the parsing code within a pool definition.
 * line points to a line counter which is incremented by the parsing
 * function whenever a new line is started.
 * Parsing continues until a line that begins with a closing brace
 * is encountered.
 * The return value is 1 if an address was returned, 0 if the end of the
 * entry has been reached, or -1 if a syntax error is encountered.
 */

typedef int (*AtAddrParseFunc)(FILE *cf, AasAddr *addr, int *line, int *init);

/*
 * General Note:  The following functions, which accept addresses as
 * arguments, should not assume that the addresses are aligned
 * (this is because they may come from the id2addr database, which uses
 * libdb, which does not guarantee alignment of data).
 */

/*
 * AtAddrCompareFunc -- compare two address
 * This function is used for sorting, searching, and handling of
 * address ranges.  It returns a negative number if addr1 < addr2,
 * zero if addr1 == addr2, and a positive number if addr1 > addr2.
 */

typedef int (*AtAddrCompareFunc)(AasAddr *addr1, AasAddr *addr2);

/*
 * AtAddrValidateFunc -- determines whether a given address is valid
 * Returns 1 if ok, or 0 if not.
 */

typedef int (*AtAddrValidateFunc)(AasAddr *addr);

/*
 * AtAddrShowFunc -- produce a string representation of an address
 * Returns a pointer to a string in a static area.
 */

typedef char * (*AtAddrShowFunc)(AasAddr *addr);

typedef struct {
	char *name;			/* address type name */
	ushort addr_type;		/* type code */
	AtAddrParseFunc parse;		/* parse function */
	AtAddrCompareFunc compare;	/* comparison function */
	AtAddrValidateFunc validate;	/* validation function */
	AtAddrShowFunc show;		/* show function */
} AddressType;
