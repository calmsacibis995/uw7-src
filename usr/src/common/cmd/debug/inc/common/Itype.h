/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ifndef Itype_h
#define Itype_h
#ident	"@(#)debugger:inc/common/Itype.h	1.2"

/*
 * NAME
 *	Itype.h -- internal representations of subject types
 *
 * ABSTRACT
 *	Each subject type known to the debugger is represented
 *	internally by one of the types specified in this file.
 *
 * DESCRIPTION
 *	For each subject type known to the debugger (each of the
 *	entries in the Stype enum) there is an internal
 *	representation given in the machine-dependent section
 *	below. The Stype member has a leading 'S'. The corresponding
 *	internal type has a leading 'I'. For simplicity in the
 *	interfaces to things like read routines, a union of
 *	all of the internal types, Itype, is defined.
 *	Each target type is mapped into the most appropriate internal
 *	representation available in the debugger
 *
 *	When porting the debugger, note that some of the internal
 *	types may have to be implemented as classes on your machine.
 *	Most likely candidates are the base and offset entries.
 *
 * USED BY
 *
 *	All portions of the debugger which need to deal with target
 *	data.
 */

enum Stype {
	SINVALID,	/* reserve a saving value */
	Schar,
	Sint1,
	Sint2,
	Sint4,
#if LONG_LONG
	Sint8,
#endif
	Suchar,
	Suint1,
	Suint2,
	Suint4,
#if LONG_LONG
	Suint8,
#endif
	Ssfloat,
	Sdfloat,
	Sxfloat,
	Saddr,
	Sdebugaddr, 
	Sbase,
	Soffset
} ;

#include "Itype.Mach.h"

union Itype {
	Ichar	ichar;
	Iint1	iint1;
	Iint2	iint2;
	Iint4	iint4;
#if LONG_LONG
	Iint8	iint8;
#endif
	Iuchar	iuchar;
	Iuint1	iuint1;
	Iuint2	iuint2;
	Iuint4	iuint4;
#if LONG_LONG
	Iuint8	iuint8;
#endif
	Isfloat	isfloat;
	Idfloat	idfloat;
	Ixfloat	ixfloat;
	Iaddr	iaddr;
	char *	idebugaddr;
	Ibase	ibase;
	Ioffset	ioffset;
	char	rawbytes[16];	/* for use by things which NEED TO KNOW!!!!! */
	int	rawwords[4];	/* ditto */
} ;

#endif /* Itype_h */
