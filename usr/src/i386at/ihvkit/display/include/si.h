#ident	"@(#)ihvkit:display/include/si.h	1.2"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*******************************************************
 	Copyrighted as an unpublished work.
 	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 	All rights reserved.
********************************************************/

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved
********************************************************/

#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "miscstruct.h"
#include "sidep.h"
#include "dixfontstr.h" /* for CharInfoPtr */

/*
 * si functions
 */
extern Bool siScreenInit();
extern void siQueryBestSize();
extern Bool siCreateWindow();
extern Bool siPositionWindow();
extern Bool siChangeWindowAttributes();
extern Bool siMapWindow();
extern Bool siUnmapWindow();
extern Bool siDestroyWindow();

extern Bool siRealizeFont();
extern Bool siUnrealizeFont();

extern Bool siCreateGC();

extern PixmapPtr siCreatePixmap();
extern Bool siDestroyPixmap();

extern void siCopyWindow();
extern void siPaintAreaPR();
extern void siPaintAreaSolid();
extern void siPaintArea32();
extern void siPaintAreaOther();
/*
 *	siValidateGC(), siDestroyGC() declared as static
 *	to avoid -Xc warning message
 */
static void siDestroyGC();
static void siValidateGC();

extern void siDestroyClip();
extern void siCopyClip();
extern void siChangeClip();
extern void siCopyGCDest();

extern void siSetSpans();
extern void siGetSpans();
extern void siSolidFS();
extern void siUnnaturalTileFS();
extern void siUnnaturalStippleFS();
extern void siInstallColormap();
extern void siUninstallColormap();

extern PixmapPtr siCopyPixmap();

extern int siListInstalledColormaps();
extern void siStoreColors();
extern void siResolveColor();
/*
 *	siCloseScreen() declared as static to avoid -Xc warning message
 */
static Bool siCloseScreen ();

extern void miGetImage ();
extern void miGetSpans ();
/*
 * mfb functions; temporary; we should get rid of mfb dependency from SI.
 */
extern void mfbUnnaturalTileFS();
extern void mfbUnnaturalStippleFS();
extern Bool mfbRealizeFont();
extern Bool mfbUnrealizeFont();
extern RegionPtr mfbPixmapToRegion();
extern RegionPtr mfbCopyArea();
extern RegionPtr mfbCopyPlane();

/*
 * mi functions
 */
extern int  miPolyText();
extern int  miPolyText8();
extern int  miPolyText16();
extern void miImageText8();
extern void miImageText16();

extern void miPolyFillRect();
extern void miPolyFillArc();

extern void miNotMiter();
extern void miMiter();
extern void miPolyArc();
extern void miZeroPolyArc();

extern void miPutImage();
extern void miGetImage();
extern void miPolyGlyphBlt();
extern void miImageGlyphBlt();

extern RegionPtr miCopyArea();
extern RegionPtr miCopyPlane();
extern void miPolyPoint();
extern void miPolySegment();
extern void miPolyRectangle();
extern void miFillPolygon();
extern void miPushPixels();
extern void miZeroLine();
extern void miWideLine();
extern void miWideDash();

/*
 * State subroutines
 */

static SIint32	sigetnextstate();
extern void	sifreestate();
extern void	siinitstates();
extern void	sivalidatestate();
/* font routines */
extern void	siinitfonts();

/*
   private field of pixmap
   pixmap.devPrivate = (unsigned int *)pointer_to_bits
   pixmap.devKind = width_of_pixmap_in_bytes
*/

/* private field of GC */
typedef struct {
    unsigned char       rop;            /* reduction of rasterop to 1 of 3 */
    unsigned char       ropOpStip;      /* rop for opaque stipple */
    unsigned char       ropFillArea;    /*  == alu, rop, or ropOpStip */
    unsigned            fExpose:1;      /* callexposure handling ? */
    unsigned            freeCompClip:1;
    PixmapPtr           pRotatedPixmap;
    RegionPtr           pCompositeClip; /* FREE_CC or REPLACE_CC */
    /* SI: START */
    SIint32	GSmodified;	/* items modified since last validate */
    SIint32	GStateidx;	/* SI SIGState index value for this GC */
    SIGState	GState;		/* Current SI GState */
    SIbitmap	GCtile, GCstpl;	/* Current Tile/Stipple */
    /* SI: END */
    } siPrivGC;
typedef siPrivGC	*siPrivGCPtr;

/* freeCompositeClip values */
#define REPLACE_CC	0		/* compsite clip is a copy of a
					pointer, so it doesn't need to 
					be freed; just overwrite it.
					this happens if there is no
					client clip and the gc is
					clipped by children 
					*/
#define FREE_CC		1		/* composite clip is a real
					   region that we need to free
					*/

#ifdef BEF_DT
/* private field of window */
typedef struct {
    unsigned    char fastBorder; /* non-zero if border is 32 bits wide */
    unsigned    char fastBackground;
    unsigned short unused; /* pad for alignment with Sun compiler */
    DDXPointRec oldRotate;
    PixmapPtr   pRotatedBackground;
    PixmapPtr   pRotatedBorder;
    /* SI: START */
    SIint32	GStateidx;	/* SI SIGState index value for this Win */
    SIGState	GState;		/* Current SI GState */
    SIbitmap	GWtile, GWstpl;	/* Current Tile/Stipple */
    /* SI: END */
    } siPrivWin;
#endif

/* precomputed information about each glyph for GlyphBlt code.
   this saves recalculating the per glyph information for each
box.
*/
typedef struct _pos{
    int xpos;		/* xposition of glyph's origin */
    int xchar;		/* x position mod 32 */
    int leftEdge;
    int rightEdge;
    int topEdge;
    int bottomEdge;
    int *pdstBase;	/* longword with character origin */
    int widthGlyph;	/* width in bytes of this glyph */
} TEXTPOS;

/*
 * Font index information --  This data is used to quickly calculate
 * the indices passed to the font drawing routines.
 */

typedef struct {
    int firstCol;
    int numCols;
    int firstRow;
    int numRows;
    int chDefault;
    int cDef;
    } siFontDims;

/*
 * allow quick lookup of glyph existance and CharInfo
 */
typedef struct {
    CharInfoPtr pci;	/* pointer to charInfo for this glyph */
    int sddIndex;  /* index of glyph as downloaded to SDD */
} siFontGlyphInfo;

/* private field of font structure */

#define UNOPT_FONT	1	/* unoptimizable font */
#define HDWR_FONT	4	/* hardware optimizable font */

/*
 * MAGIC NUMBERS : 
#define	SI_FIRST_MAGIC	0xdeadbeef
#define SI_LAST_MAGIC	0xabadcafe
 */

typedef struct {
#if defined(SI_FIRST_MAGIC)
    long	firstMagic;
#endif	/* SI_FIRST_MAGIC */
    int		fonttype;
    int		hdwridx;	/* hardware downloaded font index */
    SIFontInfo	fastinfo;	/* hardware font info */
    siFontDims  fastidx;
    int		glyphInfoSize;
    siFontGlyphInfo *glyphInfo; /* cached glyph info */
#if defined(SI_LAST_MAGIC)
    long	lastMagic;
#endif	/* SI_LAST_MAGIC */
    } siPrivFont;

typedef siPrivFont	*siPrivFontP;

extern int siScreenIndex;	/* screen private index into devPrivates */

/* reduced raster ops for si */
#define RROP_BLACK	GXclear
#define RROP_WHITE	GXset
#define RROP_NOP	GXnoop
#define RROP_INVERT	GXinvert

/* out of clip region codes */
#define OUT_LEFT 0x08
#define OUT_RIGHT 0x04
#define OUT_ABOVE 0x02
#define OUT_BELOW 0x01

/* major axis for bresenham's line */
#define X_AXIS	0
#define Y_AXIS	1

/* optimization codes for FONT's devPrivate field */
#define FT_VARPITCH	0
#define FT_SMALLPITCH	1
#define FT_FIXPITCH	2

#ifndef EVEN_DASH
#define EVEN_DASH	0
#endif
#ifndef ODD_DASH
#define ODD_DASH	~0
#endif

/* macros for sibitblt.c, sifillsp.c
   these let the code do one switch on the rop per call, rather
than a switch on the rop per item (span or rectangle.)
*/

#define fnCLEAR(src, dst)	(0)
#define fnAND(src, dst) 	(src & dst)
#define fnANDREVERSE(src, dst)	(src & ~dst)
#define fnCOPY(src, dst)	(src)
#define fnANDINVERTED(src, dst)	(~src & dst)
#define fnNOOP(src, dst)	(dst)
#define fnXOR(src, dst)		(src ^ dst)
#define fnOR(src, dst)		(src | dst)
#define fnNOR(src, dst)		(~(src | dst))
#define fnEQUIV(src, dst)	(~src ^ dst)
#define fnINVERT(src, dst)	(~dst)
#define fnORREVERSE(src, dst)	(src | ~dst)
#define fnCOPYINVERTED(src, dst)(~src)
#define fnORINVERTED(src, dst)	(~src | dst)
#define fnNAND(src, dst)	(~(src & dst))
#define fnSET(src, dst)		(~0)

/* Binary search to figure out what to do for the raster op.  It may
 * do 5 comparisons, but at least it does no function calls 
 * Special cases copy because it's so frequent 
 * XXX - can't use this in many cases because it has no plane mask.
 */
#define DoRop(alu, src, dst) \
( ((alu) == GXcopy) ? (src) : \
    (((alu) >= GXnor) ? \
     (((alu) >= GXcopyInverted) ? \
       (((alu) >= GXnand) ? \
         (((alu) == GXnand) ? ~((src) & (dst)) : ~0) : \
         (((alu) == GXcopyInverted) ? ~(src) : (~(src) | (dst)))) : \
       (((alu) >= GXinvert) ? \
	 (((alu) == GXinvert) ? ~(dst) : ((src) | ~(dst))) : \
	 (((alu) == GXnor) ? ~((src) | (dst)) : (~(src) ^ (dst)))) ) : \
     (((alu) >= GXandInverted) ? \
       (((alu) >= GXxor) ? \
	 (((alu) == GXxor) ? ((src) ^ (dst)) : ((src) | (dst))) : \
	 (((alu) == GXnoop) ? (dst) : (~(src) & (dst)))) : \
       (((alu) >= GXandReverse) ? \
	 (((alu) == GXandReverse) ? ((src) & ~(dst)) : (src)) : \
	 (((alu) == GXand) ? ((src) & (dst)) : 0)))  ) )

/*
 * This macro is check for cursor movement (If so desired) during
 * non critical sections of intensive (IE long) output routines.
 * One example might be during drawing of many rectangle fills
 */

#ifndef CHECKINPUT
#define CHECKINPUT()
#endif

extern int siGCPrivateIndex;

#ifdef BEF_DT
extern int siWindowPrivateIndex;
#endif

#define GLYPHEXISTS(pci)	1	/* VERY TEMPORARY */

#define siGetDrawableInfo(pDrawable, width, pointer, scanlineFlag) { \
    PixmapPtr   _pPix; \
    if ((pDrawable)->type == DRAWABLE_WINDOW) { \
        if (!si_have_fb) ++(scanlineFlag); \
        _pPix = (PixmapPtr) (pDrawable)->pScreen->devPrivate; \
    } else { \
        _pPix = (PixmapPtr) (pDrawable); \
    } \
    (pointer) = (unsigned long *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (unsigned long); \
  }

#define GETSTIPPLEPIXELS( psrcstip, x, w, ones, psrcpix, destpix ) \
if (PPW == 1) /* assumes ones can only be 0 or 1... */ \
  *destpix = (((*psrcstip >> x) & 1) == ones) ? *psrcpix : 0; \
else \
  getstipplepixels (psrcstip, x, w, ones, psrcpix, destpix);

#define si_setpolyclip_fullscreen() \
  si_setpolyclip(0,0,si_getscanlinelen-1,si_getscanlinecnt-1);
#define si_setlineclip_fullscreen() \
  si_setlineclip(0,0,si_getscanlinelen-1,si_getscanlinecnt-1);
#define si_setdrawarcclip_fullscreen() \
  si_setdrawarcclip(0,0,si_getscanlinelen-1,si_getscanlinecnt-1);
#define si_setfillarcclip_fullscreen() \
  si_setfillarcclip(0,0,si_getscanlinelen-1,si_getscanlinecnt-1);
#define si_fontclip_fullscreen() \
  si_fontclip(0,0,si_getscanlinelen-1,si_getscanlinecnt-1);
