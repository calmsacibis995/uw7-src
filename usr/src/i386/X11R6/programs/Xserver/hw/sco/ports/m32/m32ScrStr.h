/*
 * @(#) m32ScrStr.h 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 27-Jul-93, buckm
 *	Created.
 * S001, 30-Aug-93, buckm
 *	Add useSWCurs.
 *	Add off-screen area defs.
 * S002, 20-Sep-93, buckm
 *	Must keep track of TE8 font char count for TBird nfb interface.
 */
/*
 * m32ScrStr.h - Mach-32 screen privates
 */

typedef	struct _m32CursInfo {
	int		offset;		/* fb offset of cursor mem */
	DDXPointRec	addr;		/* x,y addr  of cursor mem */
	unsigned char	maxw;		/* maximum width  of cursor */
	unsigned char	maxh;		/* maximum height of cursor */
	unsigned char	hotx;		/* hotspot x of current cursor */
	unsigned char	hoty;		/* hotspot y of current cursor */
	unsigned char	uw;		/* unused width  of current cursor */
	unsigned char	uh;		/* unused height of current cursor */
} m32CursInfo, *m32CursInfoPtr;

typedef struct _m32Font8Info {
	unsigned long	basePlane;	/* bit for 1st font plane */ 
	DDXPointPtr	pGlyphAddr;	/* ptr to glyph coords */
#ifdef agaII
	int		count;		/* chars in font */
#endif
} m32Font8Info, *m32Font8InfoPtr;

typedef struct _m32TE8Info {
	int		offset;			/* fb offset of te8 mem */
	DDXPointRec	addr;			/* x,y addr  of te8 mem */
	DDXPointPtr	pGlyphAddr;		/* alloc'd glyph coords */
	m32Font8Info	font[M32_TE8_FONTS];	/* per font info */
} m32TE8Info, *m32TE8InfoPtr;

typedef	struct _m32OSInfo {
	int		width;		/* width    of off-screen area */
	int		height;		/* height   of off-screen area */
	DDXPointRec	addr;		/* x,y addr of off-screen area */
} m32OSInfo, *m32OSInfoPtr;

typedef struct _m32ScrnPriv {
	pointer		fbBase;		/* frame buffer base (virtual) */
	int		fbStride;	/* frame buffer width in bytes */
	int		fbPitch;	/* frame buffer width in pixels */
	int		pixBytesLog2;	/* log2 coprocessor bytes-per-pixel */

	unsigned int	hasVGA    : 1;	/* m32 shares with VGA ? */
	unsigned int	inGfxMode : 1;	/* m32 in graphics mode ? */
	unsigned int	useSWCurs : 1;	/* use software cursor ? */

	m32CursInfo	cursInfo;	/* cursor info */
	m32TE8Info	te8Info;	/* te8 font info */
	m32OSInfo	osInfo;		/* off-screen work area */
	BoxRec		clip;		/* current clip rectangle */
} m32ScrnPriv, *m32ScrnPrivPtr;

#define M32_SCREEN_PRIV(pscreen) ((m32ScrnPrivPtr) \
			((pscreen)->devPrivates[m32ScreenPrivateIndex].ptr))

extern int m32ScreenPrivateIndex;

#define M32_CURSOR_INFO(pscreen) (&(M32_SCREEN_PRIV(pscreen)->cursInfo))

#define M32_TE8_INFO(pscreen) (&(M32_SCREEN_PRIV(pscreen)->te8Info))

#define M32_OS_INFO(pscreen) (&(M32_SCREEN_PRIV(pscreen)->osInfo))
