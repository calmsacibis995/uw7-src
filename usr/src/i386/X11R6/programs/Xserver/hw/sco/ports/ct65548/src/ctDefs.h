/*
 *	@(#)ctDefs.h	11.1	10/22/97	12:33:50
 *	@(#) ctDefs.h 58.1 96/10/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 */
/* 
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ifndef _CT_DEFS_H
#define _CT_DEFS_H

#ident "@(#) $Id: ctDefs.h 58.1 96/10/09 "

/*
 * Here is how to concatenate tokens when one of the tokens is a
 * preprocessor define.  See K&R II or the comp.lang.c FAQ to find
 * out why this has to be done this way.  This is ANSI only code.
 */
#define CT_TYPE_0(head,body) head##body
#define CT_TYPE_1(head,body) CT_TYPE_0(head,body)
#define CT(body) CT_TYPE_1(CTHEAD,body)

#if CT_BITS_PER_PIXEL == 8
typedef unsigned char CT_PIXEL;
#define CT_BYTES_TO_CT_PIXELS(val) (val)
#define CT_SCREEN_OFFSET(priv, x, y) (					\
	(unsigned long)((y) * (priv)->bltStride) + ((x) * 1)		\
)
#endif /* CT_BITS_PER_PIXEL == 8 */
#if CT_BITS_PER_PIXEL == 16
typedef unsigned short CT_PIXEL;
#define CT_BYTES_TO_CT_PIXELS(val) (					\
	((val) + (sizeof(unsigned short) - 1)) / sizeof(unsigned short)	\
)
#define CT_SCREEN_OFFSET(priv, x, y) (					\
	(unsigned long)((y) * (priv)->bltStride) + ((x) * 2)		\
)
#endif /* CT_BITS_PER_PIXEL == 16 */
#if CT_BITS_PER_PIXEL == 24
typedef unsigned long CT_PIXEL;
#define CT_BYTES_TO_CT_PIXELS(val) (					\
	((val) + (sizeof(unsigned long) - 1)) / sizeof(unsigned long)	\
)
#define CT_SCREEN_OFFSET(priv, x, y) (					\
	(unsigned long)((y) * (priv)->bltStride) + ((x) * 3)		\
)
#endif /* CT_BITS_PER_PIXEL == 24 */

typedef struct _ctGCPriv {
	pointer		funcs;		/* Wrapper for GC functions */
	void		*chunk;		/* cached tile/stipple */
	unsigned int	width;		/* cached width */
	unsigned int	height;		/* cached height */
} CT(GCPrivRec), *CT(GCPrivPtr);

extern int CT(GCPrivateIndex);

#define CT_GC_PRIV(pGC) \
	((CT(GCPrivPtr))((pGC)->devPrivates[CT(GCPrivateIndex)].ptr))

typedef struct _ctPrivate {
	CT_PIXEL	*fbPointer;	/* frame buffer virtual address */
	unsigned int	fbSize;		/* size of mapped memory */
	unsigned int	fbStride;	/* frame buffer stride in CT_PIXELS */
	unsigned int	width;		/* frame buffer width in pixels */
	unsigned int	height;		/* frame buffer height in pixels */
	unsigned int	depth;		/* frame buffer depth in bits */
	unsigned long	allPlanes;	/* significant bits in planemask */
	unsigned int	dacShift;	/* HW shift for 16-bit RGB values */
	unsigned int	bltStride;	/* HW bitblt stride in bytes */
	unsigned int	bltPixelSize;	/* HW bitblt pixel size in bytes */
	Bool		cursorEnabled;	/* HW cursor enabled flag */
	pointer		cursorCache;	/* HW cursor cache data */
	pointer		cursor;		/* HW cursor current private data */
	pointer		heap;		/* offscreen memory manager */
	int		kTileMin;	/* hueristic min tiling constant */
	int		kTileMax;	/* hueristic max tiling constant */
	Bool		(*CreateGC)();	/* Wrapper for nfbCreateGC() */
	void		*cursor_chunk;	/* Allocated screen memory for cursor */
	unsigned char	*cursor_memory;	/* Allocated screen memory for cursor */
} CT(Private), *CT(PrivatePtr);

extern int CT(ScreenPrivateIndex);

#define CT_PRIVATE_DATA(pScreen) \
	((CT(PrivatePtr))((pScreen)->devPrivates[CT(ScreenPrivateIndex)].ptr))

#define CT_BYTES_TO_SHORTS(val) (((val) + (sizeof(short) - 1)) / sizeof(short))
#define CT_BYTES_TO_LONGS(val) (((val) + (sizeof(long) - 1)) / sizeof(long))

/*
 * Bitswap a byte and place the result in the bottom 8 bits of a 32-bit dword.
 * This macro is intended to be used in monochrome image expansion.
 */
#define CT_CHAR2LONG(byte) ((unsigned long)MSBIT_SWAP((byte)) & 0x000000ff)

#define CT_SCREEN_ADDRESS(priv, x, y) (					\
	(CT_PIXEL *)((priv)->fbPointer + ((priv)->fbStride * (y)) + (x))\
)

/*
 * Arrays of 16 Windows ROP's indexed by corresponding X alu's (GXcopy, GXand,
 * etc.)
 */
extern unsigned long CT(PixelOps)[];	/* draw src and dst */
extern unsigned long CT(PatternOps)[];	/* draw pattern and dst */
extern unsigned long CT(MaskOps)[];	/* draw src, pattern, and dst */

#endif /* _CT_DEFS_H */
