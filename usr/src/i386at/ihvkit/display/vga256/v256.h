#ident	"@(#)ihvkit:display/vga256/v256.h	1.2"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */
/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

#ifndef __V256_H__
#define __V256_H__

#ifdef DEBUG
extern int xdebug;
#define DBENTRY(func) if (xdebug & 0x10) printf("%s\n", func);
#define DBENTRY1(func) if (xdebug & 0x20) printf("%s\n", func);
#define DBENTRY2(func) if (xdebug & 0x40) printf("%s\n", func);
#else
#define DBENTRY(func)
#define DBENTRY1(func)
#define DBENTRY2(func)
#endif

/*
 * ex: 	
 *	0101
 *		first byte	: major version of SI used by the DM (a.k.a SDD)
 *		second byte	: minor version of SI used by the DM
 */
#define VGA256_IDENT_STRING "VGA256: graphics driver for super VGA adaptors. Version:" V256LIB_VERSION " (" __DATE__ ")."

/****
extern	struct	at_disp_info	vt_info;
****/

extern ScrInfoRec vendorInfo;

#ifndef	VGA_PAGE_SIZE
#error 	VGA_PAGE_SIZE must be defined !
#endif	/* VGA_PAGE_SIZE */

#define VIDEO_PAGE_MASK 	(VGA_PAGE_SIZE-1)

/*
 * Page selection
 */
/*
 * select the page corresponding to the given offset, for reading
 */
#define selectreadpage(j) \
{\
      if (((j) > v256_end_readpage) ||\
	  ((j) < (v256_end_readpage - VIDEO_PAGE_MASK)))\
	  {\
		(*vendorInfo.SelectReadPage)(j);\
	  }\
}

/*
 * select the page corresponding to the given offset for writing
 */
#define selectwritepage(j) \
{\
      if (((j) > v256_end_writepage) ||\
	  ((j) < (v256_end_writepage - VIDEO_PAGE_MASK)))\
	  {\
	       (*vendorInfo.SelectWritePage)(j);\
	  }\
}

/*
 * Select a page for reading AND writing
 */
#define	selectpage(j)\
     selectreadpage(j);\
     selectwritepage(j);

extern	int	v256_clip_x1;		/* clipping region */
extern	int	v256_clip_y1;
extern	int	v256_clip_x2;
extern	int	v256_clip_y2;

extern	int	v256_slbytes;		/* number of bytes in a scanline */
extern	int	v256_is_color;		/* true if on a color monitor */
#define	v256_fb	(vendorInfo.vt_buf)	/* V256 frame buffer pointer */

#define v256_swap(a, b, t)	{t = a; a = b; b = t;}

/*
 * Define the maximum width of any supported screen in pixels (including
 * logical screens that have a panned physical window).
 */
#define MAXSCANLINE 2048

/*
 * Various defines for tiles and stipples (collectively called "patterns").
 * WARNING:  THE MAX PATTERN WIDTH IS A DEFINE HERE, BUT VARIOUS PARTS OF
 *           THE CODE WILL BREAK IF THIS VALUE IS CHANGED.
 *
 * A width of 16 works well because it is the most common X pattern size, 
 * and a majority of the other common sizes can be built up to 16.  (Other
 * common sizes are 1, 2, 4, 8).
 */
#define	V256_PAT_W	32		/* max pattern width (multiple of 8)*/
#define	V256_PAT_H	32		/* max pattern height */
#define V256_PATBYTES	(V256_PAT_W * V256_PAT_H) /* bytes in pattern */
#define V256_BADPAT	0x80000000	/* used in fill_mode for bad pattern */
extern	BYTE 	*v256_cur_pat;		/* current pattern */
extern	int		v256_cur_pat_h;		/* current pattern's height */

/*
 *	Font handling defines
 */
#define	V256_NUMDLFONTS		8	/* max number of downloadable fonts */
#define	V256_NUMDLGLYPHS	256	/* max number of glyphs per font */
#define	V256_DL_FONT_W		25	/* width of downloaded glyph */
#define	V256_DL_FONT_H		32	/* height of downloaded glyph */

/* 
 *	internal font info structure 
 */
typedef	struct	v256_font 
{	
	int		w;			/* width of glyphs */
	int		h;			/* height of glyphs */
	int		ascent;			/* distance from top to baseline */
	BYTE	*glyphs; 		/* pointer to glyph data */
} v256_font;

extern	v256_font v256_fonts[];		/* downloaded font info */

#define V256_MAXCOLOR	256		/* maximum number of colors in a map */
#define V256_PLANES		8		/* number of planes */

typedef	struct	v256_rgb 
{
	BYTE	red;
	BYTE	green;
	BYTE	blue;
}	v256_rgb;

/* 
 *	V256 pallette registers 
 */
extern  v256_rgb v256_pallette[V256_MAXCOLOR];

typedef struct	v256_state 
{
	BITS16	pmask;			/* plane mask */
	int		mode;			/* graphics mode */
	int		stp_mode;		/* stipple mode */
	int		fill_mode;		/* fill mode */
	int		fill_rule;		/* fill rule */
	BYTE	fg;				/* foreground color */
	BYTE	bg;				/* background color */
	int		stpl_h;			/* stipple height */
	BYTE	stpl[V256_PATBYTES/8];	/* stipple pattern */

	BYTE	stpl_valid;		/* flags indicating if stipple was converted, needs to be freed */
	BYTE	tile_valid;		/* flags for tile conversion, tile needs to be freed */
	SIbitmap raw_stipple;	/* stipple info downloaded */
	SIbitmap raw_tile;		/* tile info downloaded */
	BYTE	raw_stpl_data[V256_PAT_H*4];	/* stipple pattern downloaded */
	BYTE	*raw_tile_data;	/* tile pattern downloaded */
	BYTE	*big_stpl;		/* pointer to LARGE stipple data */
}	v256_state;
	
#define	V256_TILE_VALID			0x01
#define V256_STIPPLE_VALID		0x02
#define V256_FREE_TILE_DATA		0x04
#define V256_FREE_STIPPLE_DATA		0x08

#define	V256_NUMGS	4		/* number of graphic states */
extern	v256_state	v256_gstates[];	/* grapics states */

extern	int			v256_cur_state;	/* current state selected */
extern	v256_state	*v256_gs;	/* pointer to current state */

extern	BYTE	v256_slbuf[];		/* buffer for a scanline */
extern	BYTE	v256_tmpsl[];		/* temporary buffer for a scanline */

extern	BYTE	v256_src;		/* current source color */
extern	int		v256_invertsrc;		/* currently inverting source? */
extern	int		v256_function;		/* current V256 function */

extern	int		v256_readpage;		/* current page of memory in use */
extern	int		v256_writepage;
extern	int		v256_end_writepage;		/* last valid offset from v256_fb */
extern	int	v256_end_readpage;


/*
 * V256 defines
 */

#define V256_COPY	0x0000	/* Data unmodified */
#define V256_AND	0x0800	/* Data AND'ed with latches */
#define V256_OR		0x1000	/* Data OR'ed with latches */
#define V256_XOR	0x1800	/* Data XOR'ed with latches */

#define V256_INVERT (V256_XOR + 1)
#define V256_OR_INVERT (V256_XOR + 2)
#define V256_AND_INVERT (V256_XOR + 3)

/*==========================================================================*/
/* SI interface functions */

/* MANDATORY */
extern SIBool v256_init \
  PROTO((int, SIScreenRec *psiscreen));
extern SIBool v256_restore \
  PROTO((SIvoid));
extern SIBool v256_vt_save \
  PROTO((SIvoid));
extern SIBool v256_vt_restore \
  PROTO((SIvoid));
extern SIBool v256_vb_onoff \
  PROTO((SIBool));
extern SIBool v256_initcache \
  PROTO((SIvoid));
extern SIBool v256_flushcache \
  PROTO((SIvoid));
extern SIBool v256_download_state \
  PROTO((SIint32, SIint32, SIGStateP));
extern SIBool v256_get_state \
  PROTO((SIint32, SIint32, SIGStateP));
extern SIBool v256_select_state \
  PROTO((SIint32));
extern SIBool v256_screen \
  PROTO((SIint32, SIint32));

extern SILine v256_getsl \
  PROTO((SIint32));
extern SIvoid v256_setsl \
  PROTO((SIint32, SILine));
extern SIvoid v256_freesl \
  PROTO((SIvoid));
extern SIBool v256_set_colormap \
  PROTO((SIint32, SIint32, SIColor*, SIint32));
extern SIBool v256_get_colormap \
  PROTO((SIint32, SIint32, SIColor*, SIint32));

/* OPTIONAL in SI-1.1 */
extern SIBool v256_hcurs_download \
  PROTO((SIint32, SICursorP));
extern SIBool v256_hcurs_turnon \
  PROTO((SIint32));
extern SIBool v256_hcurs_turnoff \
  PROTO((SIint32));
extern SIBool v256_hcurs_move \
  PROTO((SIint32, SIint32, SIint32));

/* OPTIONAL */
extern SIBool v256_fillspans \
  PROTO((SIint32, SIPointP, SIint32*));
extern SIBool v256_raw_ss_bitblt \
  PROTO((SIint32, SIint32, SIint32, SIint32, SIint32, SIint32));
extern SIBool v256_ss_bitblt \
  PROTO((SIint32, SIint32, SIint32, SIint32, SIint32, SIint32));
extern SIBool v256_ms_bitblt \
  PROTO((SIbitmapP, SIint32, SIint32, SIint32, SIint32, SIint32, SIint32));
extern SIBool v256_sm_bitblt \
  PROTO((SIbitmapP, SIint32, SIint32, SIint32, SIint32, SIint32, SIint32));
extern SIBool v256_ss_stplblt \
  PROTO((SIint32, SIint32, SIint32, SIint32, \
	 SIint32, SIint32, SIint32, SIint32));
extern SIBool v256_ms_stplblt \
  PROTO((SIbitmapP, SIint32, SIint32, SIint32, SIint32, \
	 SIint32, SIint32, SIint32, SIint32));
extern SIvoid v256_setclip \
  PROTO((SIint32, SIint32, SIint32, SIint32));
extern SIBool v256_plot_points \
  PROTO((SIint32, SIPointP));
extern SIBool v256_line_onebitrect \
  PROTO((SIint32, SIint32, SIint32, SIint32));
extern SIBool v256_font_check \
  PROTO((SIint32, SIFontInfoP));
extern SIBool v256_font_download \
  PROTO((SIint32, SIFontInfoP, SIGlyphP));
extern SIBool v256_font_free \
  PROTO((SIint32));
extern SIBool v256_font_stplblt \
  PROTO((SIint32, SIint32, SIint32, SIint32, SIint16 *, SIint32));
/* SI 1.1 - NEW */
extern SIBool v256_poly_fillrect \
  PROTO((SIint32, SIint32, SIint32, SIRectOutlineP));
extern SIBool v256_line_onebitline \
  PROTO((SIint32, SIint32, SIint32, SIPointP, SIint32, SIint32));
extern SIBool v256_line_onebitseg \
  PROTO((SIint32, SIint32, SIint32, SISegmentP, SIint32));

/* SI 1.0 - old functions */
extern SIBool v256_1_0_init \
  PROTO((int, SIConfigP, SIInfoP, SI_1_0_Functions **));
extern SIBool v256_1_0_fill_rect \
  PROTO((SIint32, SIRectP));
extern SIBool v256_1_0_OneBitLine \
  PROTO((SIint32, SIPointP));
extern SIBool v256_1_0_OneBitSegment \
  PROTO((SIint32, SIPointP));


/*
 * Useful macros
 */
#define OUT_LEFT	0x08
#define	OUT_RIGHT	0x04
#define	OUT_ABOVE	0x02
#define OUT_BELOW	0x01

#define OUTCODES(result, x, y) \
    if (x < v256_clip_x1) \
	result |= OUT_LEFT; \
    else if (x > v256_clip_x2) \
	result |= OUT_RIGHT; \
    if (y < v256_clip_y1) \
	result |= OUT_ABOVE; \
    else if (y > v256_clip_y2) \
	result |= OUT_BELOW;


#define DIFFERENCE(a,b)		((a) > (b) ? (a)-(b) : (b)-(a))

/* BLT_MASK fields: */
#define MASK_ALL	0xFF

#define OUTSIDE(v,min,max)      ((unsigned) (v) - (min) > (max) - (min))

#endif /* __V256_H__ */
