#ident	"@(#)r5fonts:clients/bdftosnf/snfstruct.h	1.1"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/
#ifndef SNFSTRUCT_H
#define SNFSTRUCT_H 1
#include "font.h"
#include "misc.h"

/*
 * This file describes the Server Natural Font format.
 * SNF fonts are both CPU-dependent and frame buffer bit order dependent.
 * This file is used by:
 *	1)  the server, to hold font information read out of font files.
 *	2)  font converters
 *
 * Each font file contains the following
 * data structures, with no padding in-between.
 *
 *	1)  The XFONTINFO structure
 *		hand-padded to a two-short boundary.
 *		maxbounds.byteoffset is the total number of bytes in the
 *			glpyh array
 *		maxbounds.bitOffset is thetotal width of the unpadded font
 *
 *	2)  The XCHARINFO array
 *		indexed directly with character codes, both on disk
 *		and in memory.
 *
 *	3)  Character glyphs
 *		padded in the server-natural way, and
 *		ordered in the device-natural way.
 *		End of glyphs padded to 32-bit boundary.
 *
 *	4)  nProps font properties
 *
 *	5)  a sequence of null-terminated strings, for font properties
 */

#define FONT_FILE_VERSION	4

typedef struct _FontProp { 
	CARD32	name;		/* offset of string */
	INT32	value;		/* number or offset of string */
	Bool	indirect;	/* value is a string offset */
} FontPropRec;

/*
 * the following macro definitions describe a font file image in memory
 */
#define ADDRCharInfoRec( pfi)	\
	((CharInfoRec *) &(pfi)[1])

#define ADDRCHARGLYPHS( pfi)	\
	(((char *) &(pfi)[1]) + BYTESOFCHARINFO(pfi))

/*
 * pad out glyphs to a CARD32 boundary
 */
#define ADDRXFONTPROPS( pfi)  \
	((DIXFontProp *) ((char *)ADDRCHARGLYPHS( pfi) + BYTESOFGLYPHINFO(pfi)))

#define ADDRSTRINGTAB( pfi)  \
	((char *)ADDRXFONTPROPS( pfi) + BYTESOFPROPINFO(pfi))

#define	BYTESOFFONTINFO(pfi)	(sizeof(FontInfoRec))
#define BYTESOFCHARINFO(pfi)	(sizeof(CharInfoRec) * n2dChars(pfi))
#define	BYTESOFPROPINFO(pfi)	(sizeof(FontPropRec) * (pfi)->nProps)
#define	BYTESOFSTRINGINFO(pfi)	((pfi)->lenStrings)
#define	BYTESOFGLYPHINFO(pfi)	(((pfi)->maxbounds.byteOffset+3) & ~0x3)
#define BYTESOFINKINFO(pfi)	(sizeof(CharInfoRec) * (2 + n2dChars(pfi)))
 
#endif /* SNFSTRUCT_H */

