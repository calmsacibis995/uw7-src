#ident	"@(#)cvtomf:omf.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*
 *	Copyright (c) Altos Computer Systems, 1987
 */


/* Enhanced Application Compatibility Support */
/*
 *	OMF record types
 */

#define		RECTYP(x)	((x) & 0xfe)
#define		USE32(x)	((x) & 0x01)

#define		COMDEF		(0xb0)
#define		LCOMDEF		(0xb8)
#define		COMENT		(0x88)
#define		EXTDEF		(0x8c)
#define		LEXTDEF		(0xb4)
#define		FIXUPP		(0x9c)
#define		FIXUP2		(0x9d)
#define		GRPDEF		(0x9a)
#define		LEDATA		(0xa0)
#define		LHEADR		(0x82)
#define		LIDATA		(0xa2)
#define		LINNUM		(0x94)
#define		LNAMES		(0x96)
#define		MODEND		(0x8a)
#define		PUBDEF		(0x90)
#define		LPUBDEF		(0xb6)
#define		SEGDEF		(0x98)
#define		THEADR		(0x80)

/*
 *	index fields
 */

#define		INDEX_BYTE(x)	((((x)[0] & 0x80)) ? -1 : (x)[0])
#define		INDEX_WORD(x)	((((x)[0] & 0x7f) << 8) | (x)[1])

/*
 *	length fields
 */

#define		LENGTH2		(0x81)
#define		LENGTH3		(0x84)
#define		LENGTH4		(0x88)

/*
 *	common symbols
 */

#define		COMM_FAR	(0x61)
#define		COMM_NEAR	(0x62)

/*
 *	comment subtypes
 */

#define	COM_EXESTR	(0xa4)

/*
 *	segments
 */

#define		ACBP_A(x)	(((x) >> 5) & 0x07)
#define		ACBP_C(x)	(((x) >> 2) & 0x07)
#define		ACBP_B(x)	((x) & 0x02)
#define		ACBP_P(x)	((x) & 0x01)

/*
 *	relocation (fixup) records
 */

#define		LCT_M(x)	((x)[0] & 0x40)
#define		LCT_LOC(x)	(((x)[0] >> 2) & 0x0f)
#define		LCT_OFFSET(x)	((((x)[0] & 0x03) << 8) | (x)[1])

#define		FIX_F(x)	((x)[2] & 0x80)
#define		FIX_FRAME(x)	(((x)[2] >> 4) & 0x07)
#define		FIX_T(x)	((x)[2] & 0x08)
#define		FIX_P(x)	((x)[2] & 0x04)
#define		FIX_TARGT(x)	((x)[2] & 0x03)

/*
 *	locations
 */

#define		LOBYTE		(0)
#define		OFFSET16	(1)
#define		BASE		(2)
#define		POINTER32	(3)
#define		HIBYTE		(4)
#define		OFFSET16LD	(5)
#define		OFFSET32	(9)
#define		POINTER48	(11)
#define		OFFSET32LD	(13)

/*
 *	methods
 */

#define		SEGMENT		(0)
#define		GROUP		(1)
#define		EXTERNAL	(2)
#define		LOCATION	(4)
#define		TARGET		(5)

/*
 *	threads
 */

#define		TRD_D(x)	((x)[0] & 0x40)
#define		TRD_METHOD(x)	(((x)[0] >> 2) & 0x07)
#define		TRD_THRED(x)	(((x)[0] & 0x80) ? -1 : ((x)[0] & 0x03))

/*
 *	Record for storing public/external symbol data
 */

struct sym {
	char *name;
	long offset;
	short type;
	short scn;
	int ext, typ;
	struct sym *next;
};

#define		S_EXT	1
#define		S_LEXT	2
#define		S_PUB	3
#define		S_LPUB	4
#define		S_COM	5

#define NBUCKETS	211
extern struct sym **hashhead;

#ifdef NO_PROTOTYPE
extern	void	addsym();
extern	struct sym *findsym();
extern	void	updatesym();
#else
extern	void	addsym(char *, long, int, int, int, int);
extern	struct sym *findsym(char *);
extern	void	updatesym(char *, long, int, int, int);
#endif
/* End Enhanced Application Compatibility Support */
