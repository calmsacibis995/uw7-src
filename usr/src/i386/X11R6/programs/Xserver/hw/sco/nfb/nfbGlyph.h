/*
 *	@(#) nfbGlyph.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * SCO Modification History
 *
 * S000, 24-Aug-92, staceyc
 * 	nfb font private added
 * S001, 24-Feb-93, buckm
 *	Add NFB_FONT_PRIV macro, and Ptr typedefs.
 * S002, 18-Apr-93, buckm
 *	Add FontPS struct, FontInfo struct; change Private struct.
 *	Move PRIV macro to nfbDefs.h
 */

#ifndef NFBGLYPH_H
#define NFBGLYPH_H

typedef struct _nfbGlyphInfo {
	BoxRec box;
	int stride;
	unsigned char *image;
	unsigned int glyph_id;
} nfbGlyphInfo, *nfbGlyphInfoPtr;

typedef struct _nfbFontInfo {
	Bool		isTE8Font;
	/* the following fields are currently only valid for a TE8 font */
	unsigned char	**ppBits;
	int		count;
	int		width;
	int		height;
	int		stride;
} nfbFontInfo, *nfbFontInfoPtr;

/*
 * NFB maintains just one of these per font
 */
typedef struct _nfbFontPrivate {
	nfbFontInfo	fontInfo;
	int		refcnt;
} nfbFontPrivate, *nfbFontPrivatePtr;

/*
 * NFB allocates one of these per font per screen
 */
typedef struct _nfbFontPS {
	nfbFontInfoPtr	pFontInfo;
	DevUnion	private;		/* for driver use */
} nfbFontPS, *nfbFontPSPtr;

#define NFB_TEXT8_SIZE 256

#endif
