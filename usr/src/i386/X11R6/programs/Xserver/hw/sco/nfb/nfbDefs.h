/*
 *	@(#) nfbDefs.h 12.1 95/05/09 SCOINC
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Sep 15 15:18:32 PDT 1992	mikep@sco.com
 *	- Added NFB_GC_SERIAL_NUMBER and NFB Options.  More
 *	serial number hooks to come.
 *	S001	Mon Oct 12 17:38:23 PDT 1992	mikep@sco.com
 *	- Add a few more NFB options
 *	S002	Mon Oct 15 17:38:23 PDT 1992	staceyc@sco.com
 *	- Add a few more NFB options
 *	S003	Sun Nov 01 20:12:58 PST 1992	mikep@sco.com
 *	- Add NFB Version definition
 *	S004	Tue Nov 03 18:15:06 PST 1992	mikep@sco.com
 *	- Finish off CFB options.
 *	S005	Sun Apr 18 22:18:05 PDT 1993	buckm@sco.com
 *	- Add FONT_PRIV macros.
 *	S006	Mon Jul 12 15:37:13 PDT 1993	mikep@sco.com
 *	- Add NFB_POLYBRES
 *	S007	Fri Nov 11 10:49:10 PST 1994	mikep@sco.com
 *	- Alias NFB_EVEN_ODD_POLYGON as just NFB_POLYGON for linkkit doc
 */

#ifndef _NFB_DEFS_H
#define _NFB_DEFS_H

#define NFB_WINDOW_PRIV(pwin) ((nfbWindowPrivPtr) \
		(((WindowPtr)(pwin))->devPrivates[nfbWindowPrivateIndex].ptr))

#define NFB_GC_PRIV(pgc) ((nfbGCPrivPtr) \
			((pgc)->devPrivates[nfbGCPrivateIndex].ptr))

#define NFB_SCREEN_PRIV(pscreen) ((nfbScrnPrivPtr) \
			((pscreen)->devPrivate))

#define	NFB_FONT_PRIV(pfont) ((nfbFontPrivatePtr) \
		FontGetPrivate((pfont), nfbFontPrivateIndex))

#define	NFB_FONT_PS(pfont, pscrnpriv) ((nfbFontPSPtr) \
		FontGetPrivate((pfont), (pscrnpriv)->font_private_index))

#define NFB_SERIAL_NUMBER(pgc_or_pdraw)   ((pgc_or_pdraw)->serialNumber)

/* NFB Options */
#define NFB_PT2PT_LINES		(1<<0)
#define NFB_OLD_BIT_ORDER 	(1<<1)
#define NFB_ROTATE_STIPPLES	(1<<2)
#define NFB_ROTATE_TILES	(1<<3)
#define NFB_POLYGON		(1<<4)
#define NFB_EVEN_ODD_POLYGON	NFB_POLYGON
#define NFB_POLYBRES		(1<<5)
#define NFB_LAST_OPTION		(1<<31)

/*
 * NFB CFB Options
 * These are meant to flag using cfb or mfb, depending upon depth
 */
#define NFB_CFB_GETSPANS	(1<<8)
#define NFB_CFB_GETIMAGE	(1<<9)
#define NFB_CFB_FILLSPANS	(1<<10)
#define NFB_CFB_SETSPANS	(1<<11)
#define NFB_CFB_PUTIMAGE	(1<<12)
#define NFB_CFB_COPYAREA	(1<<13)
#define NFB_CFB_COPYPLANE	(1<<14)
#define NFB_CFB_POLYPOINT	(1<<15)
#define NFB_CFB_WIDELINE	(1<<16)
#define NFB_CFB_POLYSEGMENT	(1<<17)
#define NFB_CFB_POLYRECTANGLE	(1<<18)
#define NFB_CFB_POLYARC		(1<<19)
#define NFB_CFB_FILLPOLYGON	(1<<20)
#define NFB_CFB_POLYFILLRECT	(1<<21)
#define NFB_CFB_POLYFILLARC	(1<<22)
#define NFB_CFB_POLYTEXT8	(1<<23)
#define NFB_CFB_POLYTEXT16	(1<<24)
#define NFB_CFB_IMAGETEXT8	(1<<25)
#define NFB_CFB_IMAGETEXT16	(1<<26)
#define NFB_CFB_IMAGEGLYPHBLT	(1<<27)
#define NFB_CFB_POLYGLYPHBLT	(1<<28)
#define NFB_CFB_PUSHPIXELS	(1<<29)
#define NFB_CFB_LINEHELPER	(1<<30)
#define NFB_CFB_LAST_OPTION	(1<<31)

#define NFB_CFB_GC	( NFB_CFB_FILLSPANS | NFB_CFB_SETSPANS \
			| NFB_CFB_PUTIMAGE | NFB_CFB_COPYAREA \
			| NFB_CFB_COPYPLANE | NFB_CFB_POLYPOINT \
			| NFB_CFB_WIDELINE | NFB_CFB_POLYSEGMENT \
			| NFB_CFB_POLYRECTANGLE | NFB_CFB_POLYARC \
			| NFB_CFB_FILLPOLYGON | NFB_CFB_POLYFILLRECT \
			| NFB_CFB_POLYFILLARC | NFB_CFB_POLYTEXT8 \
			| NFB_CFB_POLYTEXT16 | NFB_CFB_IMAGETEXT8 \
			| NFB_CFB_IMAGETEXT16 | NFB_CFB_IMAGEGLYPHBLT \
			| NFB_CFB_POLYGLYPHBLT | NFB_CFB_PUSHPIXELS \
			| NFB_CFB_LINEHELPER )

#define NFB_USES_CFB	( NFB_CFB_GETSPANS | NFB_CFB_GETIMAGE | NFB_CFB_GC)

/* major axis for bresenham's line */
#define X_AXIS	0
#define Y_AXIS	1

#define NFB_VERSION 50 /* 5.0 */

extern int nfbWindowPrivateIndex;
extern int nfbGCPrivateIndex;
extern int nfbFontPrivateIndex;

extern void nfbAbort();

#endif /* _NFB_DEFS_H */
