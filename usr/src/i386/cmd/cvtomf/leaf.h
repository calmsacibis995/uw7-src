#ident	"@(#)cvtomf:leaf.h	1.1"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/* Enhanced Application Compatibility Support */
/* BEGIN SCO_DEVSYS */

/* ISLAND (OMF) Symbolic Debug Data Constants */

/*
 *	ISLAND symbolic debug segments and classes
 */

#define		TYPES_CLASS	"DEBTYP"
#define		TYPES_SEGNAME	"$$TYPES"
#define		SYMBOLS_CLASS	"DEBSYM"
#define		SYMBOLS_SEGNAME	"$$SYMBOLS"

/* Basic Components */

#define NIL	0x80
#define STR	0x82
#define INDEX	0x83
#define U16	0x85
#define U32	0x86
#define S8	0x88
#define S16	0x89
#define S32	0x8A

/* Start Leaves */

#define BITFIELD	0x5c
#define NEWTYPE		0x5d
#define STRING		0x60
#define CONST		0x71
#define LABEL		0x72
#define PROCEDURE	0x75
#define PARAMETER	0x76
#define ARRAY		0x78
#define STRUCTURE	0x79
#define POINTER		0x7a
#define SCALAR		0x7b
#define LIST		0x7f


/* Basic type leaves */

#define TAG		0x5A
#define VARIANT		0x5B
#define BOOLEAN		0x6C
#define CHAR		0x6F
#define INTEGER		0x70
#define UINTEGER	0x7C
#define SINTEGER	0x7D
#define REAL		0x7E

/* Other (Basic) Type Leaves */

#define FAR		0x73
#define NEAR		0x74
#define HUGE		0x5E
#define PACKED		0x68
#define UNPACKED	0x69

/* Register Parameter Numbers */

#define	REG_BX	11
#define	REG_SI	14
#define	REG_DI	15

/* $$SYMBOLS section type leaves */

#define	S_BLOCKSTART	0x0
#define S_PROCSTART	0x1
#define S_BLOCKEND	0x2
#define S_BPREL		0x4
#define S_LOCALDATA	0x5
#define S_CODELABEL	0xB
#define S_WITHSTART	0xC
#define S_REGISTER	0xD
#define S_CONSTANT	0xE

/* don't expect any $$SYMBOLS record to be larger than S_BUFSIZE */
#define	S_BUFSIZE	128

/* Only concerned with translating non-primitive types, e.g. > PRIM_TYP */
#define PRIM_TYP	511
#define OTYP2IDX(x)	((x) - (PRIM_TYP + 1))
#define IDX2OTYP(x)	((x) + PRIM_TYP + 1)

/* END SCO_DEVSYS */
/* End Enhanced Application Compatibility Support */
