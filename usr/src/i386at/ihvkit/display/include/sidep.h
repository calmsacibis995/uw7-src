#ident	"@(#)ihvkit:display/include/sidep.h	1.2"

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

/*
 * Machine specific constants for various Display Boards
 */

#define MAX_NUMSCREENS 2

#ifndef	_SIDEP_H
#define _SIDEP_H

#include	"X.h"
#include	"Xmd.h"

/* The base definitions are from Xmd.h */

typedef	INT32		SIBool;
typedef INT32		SIAvail;
typedef INT32		SIint32;
typedef INT16		SIint16;
typedef void		SIvoid;

typedef CARD8		*SILine;
typedef INT32		*SIArray;
typedef void		*SIvoidP;	/* V2.0 */

	/* Possible states of SIBool variables */
#define	SI_TRUE		1
#define	SI_SUCCEED	1

#define SI_FALSE	0
#define SI_FAIL		0
#define SI_FATAL	-1

/*
 * This structure defines the format of a color as used between
 * the SI independent and dependant sections.  The values are
 * in the range of 0x0000 - 0xFFFF.  The value 0x0000 means that
 * The component is totally turned off.  The value 0xFFFF means
 * that the color component is as high as possible.
 */

/* QOS:md - obsolete, but changed VALID_BLUE from 3 to 4 for bitfield use */
#define VALID_RED       0x00000001
#define VALID_GREEN     0x00000002
#define VALID_BLUE      0x00000004

typedef struct _SIColor	{
	SIint32		SCpindex;	/* Pixel in question */
	SIint16		SCvalid;	/* Valid color component */
	SIint16		SCred;		/* Red component */
	SIint16		SCgreen;	/* Green component */
	SIint16		SCblue;		/* Blue Component */
} SIColor, *SIColorP;

/*
 * This structure defines the visual information structure.
 * A list of these structures is passed to the SI to specifiy
 * the set of visual types that are managed by the SDD.
 */
typedef struct _SIVisual	{
	SIint32		SVtype;		/* Type PSEUDOCOLOR_AVAIL... */
	SIint32		SVdepth;	/* Depth of visual: 1,2,4,8,16,24,32 */
	SIint32		SVcmapcnt;	/* Number of colormaps */
	SIint32		SVcmapsz;	/* Colormap size */
	SIint32		SVbitsrgb;	/* Valid bits per RGB */
	SIint32		SVredmask;	/* Valid Red mask (direct/true) */
	SIint32		SVgreenmask;	/* Valid Green mask (direct/true) */
	SIint32		SVbluemask;	/* Valid Blue mask (direct/true) */
	SIint32		SVredoffset;	/* offset for Red mask */
	SIint32		SVgreenoffset;	/* offset for Green mask */
	SIint32		SVblueoffset;	/* offset for Blue mask */
} SIVisual, *SIVisualP;

typedef struct _SICmap {
	int	visual;		/* visual type (PSEUDOCOLOR_AVAIL ...) */
	int	sz;		/* colormap size */
	unsigned short *colors;	/* pointer to RGB values for colors */
} SICmap, *SICmapP;

/*
 * This structure defines for format of points sent between the
 * SI independent and the dependant sections.
 */

typedef struct _SIPoint	{
	SIint16		x, y;
} SIPoint, *SIPointP;

/*
 * This structure defines the format of a rectangle data structure
 * that is sent between the SI independent and the dependent sections.
 */

typedef struct _SIRect	{
	SIPoint	ul;
	SIPoint lr;
} SIRect, *SIRectP;

/*
 *   This structure defines the format of a rectangle data structure
 *   that is sent between the SI independent and the dependent sections
 *   as used by the si_fastpolyrect function.
 */
typedef struct _SIRectOutline {
	SIint16	x,y;
	SIint16	width,height;
} SIRectOutline, *SIRectOutlineP;

/*
 * This structure defines the format for circle data structure that is
 * sent between the SI independent and dependent sections
 */
typedef struct _SIArc {
	SIint16	x, y;
	SIint16 width, height;
	SIint16 angle1, angle2;
} SIArc, *SIArcP;

/*
 * (SI-1.1) New Segment structure
 */
typedef	struct _SISegment {
	SIint16 x1, y1, x2, y2;
} SISegment, *SISegmentP;

/*
 * This structure defines the format of a 1 or 8 bit deep bitmap structure.
 * This structure is always some number of INT32 items wide, and some
 * number of pixels in height.  That means that the structure is always
 * INT32 aligned, and may be padded by as many as
 * (sizeof(INT32)-1)/bitsPerPixel pixels.
 *
 * The Bprivate pointer may be used by memory caching routines as a
 * hook to associate private data structures with SIbitmap structures.
 */

#define	XY_BITMAP	0		/* XY format bitmap */
#define	Z_BITMAP	1		/* Z format bitmap */
#define	XY_PIXMAP	2		/* same as SCREEN format pixmap */
#define	Z_PIXMAP	3

typedef struct _SIbitmap {
	SIint32		Btype;		/* Z_BITMAP, etc */
	SIint32		BbitsPerPixel;	/* In Bits */
	SIint32		Bwidth;		/* In pixels */
	SIint32		Bheight;	/* In pixels */
	SIint32		BorgX, BorgY;	/* Tile/Stipple Origin */
	SIArray		Bptr;		/* Bitmap location in user memory */
	SIvoidP		Bprivate;	/* For SI use only */
} SIbitmap, *SIbitmapP;

/*#define Bdepth BbitsPerPixel /* QOS:marcel - old style */

#define NullSIbitmap ((SIbitmap *)0)

/*
 * This structure contains the graphics state definition.  A Graphics
 * State is the collection of graphics modes, colors and patterns
 * used when drawing the various output facilities.
 */

#define SetSGpmask	0x00000001
#define SetSGmode	0x00000002
#define SetSGstplmode	0x00000004
#define SetSGfillmode	0x00000008
#define SetSGfillrule	0x00000010
#define SetSGarcmode	0x00000020
#define SetSGlinestyle	0x00000040
#define SetSGfg		0x00000080
#define SetSGbg		0x00000100
#define SetSGcmapidx	0x00000200
#define SetSGvisualidx	0x00000400
#define SetSGtile	0x00000800
#define SetSGstipple	0x00001000
#define SetSGline	0x00002000
#define SetSGcliplist	0x00004000

#define GetSGpmask	0x00010000
#define GetSGmode	0x00020000
#define GetSGstplmode	0x00040000
#define GetSGfillmode	0x00080000
#define GetSGfillrule	0x00100000
#define GetSGarcmode	0x00200000
#define GetSGlinestyle	0x00400000
#define GetSGfg		0x00800000
#define GetSGbg		0x01000000
#define GetSGcmapidx	0x02000000
#define GetSGvisualidx	0x04000000

#define SGFillSolidFG	0x00000001
#define SGFillSolidBG	0x00000002
#define SGFillStipple	0x00000004
#define SGFillTile	0x00000008

#define SGEvenOddRule	0x00000001
#define SGWindingRule	0x00000002

#define SGArcChord	0x00000001
#define SGArcPieSlice	0x00000002

#define SGLineSolid	0x00000001
#define SGLineDash	0x00000002
#define SGLineDblDash	0x00000004

#define SGStipple	0x00000001
#define SGOPQStipple	0x00000002

typedef struct _SIGState {
	SIint32		SGpmask;	/* Current plane mask */
	SIint32		SGmode;		/* Graphics mode */
	SIint32		SGstplmode;	/* Stipple blt mode */
	SIint32		SGfillmode;	/* Graphics fill mode */
	SIint32		SGfillrule;	/* Polygon fill rule */
	SIint32		SGarcmode;	/* arc mode */
	SIint32		SGlinestyle;	/* line style */
	SIint32		SGfg;		/* Foreground pixel index */
	SIint32		SGbg;		/* Background pixel index */
	SIint32		SGcmapidx;	/* Colormap index */
	SIint32		SGvisualidx;	/* Visual index */
	SIbitmapP	SGtile;		/* Tile pattern */
	SIbitmapP	SGstipple;	/* Stipple pattern */
	SIint32		SGlineCNT;	/* Number of line on/off entries */
	SIint32		*SGline;	/* Line pattern (on/off) pairs */
	SIRectP		SGcliplist;	/* list of clipping rectangles */
	SIint32		SGclipCNT;	/* Number of clipping rectangles */
	SIRect		SGclipextent;	/* Bounding box of clipping rects */
} SIGState, *SIGStateP;

/*
 * This structure is used in defining a hardware cursor.
 */

typedef struct _SICursor {
	SIint32		SCfg;		/* Foreground pixel index */
	SIint32		SCbg;		/* Background pixel index */
	SIint32		SCwidth;	/* Cursor width */
	SIint32		SCheight;	/* Cursor height */
	SIbitmapP	SCmask;		/* Mask pattern */
	SIbitmapP	SCsrc;		/* Src cursor pattern */
	SIbitmapP	SCinvsrc;	/* Inverse Source pattern */
} SICursor, *SICursorP;

typedef struct _SIMonitor {
	char	*model;	/* model name, ex: STDVGA */
	double	width;	/* monitor width in inches, ex: 9.75 */
	double	height;	/* monitor height in inches, ex: 7.32 */
	double	hfreq;	/* horizontal freq (scan rate) in KHz, ex: 48.5 */
	double	vfreq;	/* vertical freq (refresh) in Hz, ex: 72.0 */
	char	*otherinfo; /* any other info the SI module needs */
} SIMonitor;

/*
 * IMPORTANT: If you want your DisplayModule (SDD's) to support servers based
 * on v1.0 and v1.1, read the ../ddx/sdd/README file.
 * To keep backward compatability, all new fields are added at the end
 * of v1.0 data structures. The 3 data structures that you need to be
 * careful of:
 *		SIConfig
 *		SIFunctions 	(used to be ScreenInterface)
 *		SIFlags		(used to be SIInfo
 */ 
/*
 * Configuration file structure for init.
 */
typedef struct _SIConfig {
	char	*resource;	/* resource type, eg. "mouse", "display"*/
	char	*class;		/* device type, eg. "VGA16", "VGA256"	*/
	SIint32	visual_type;	/* default information.  For displays, this */
				/* is the visual type, eg PSEUDOCOLOR_AVAIL */
	char	*info;	/* info used by class library		*/
	char	*display;	/* display.screen string, eg. "0.0"	*/
	SIint32	displaynum;	/* display number [note conflict w/ above] */
	SIint32 screen;		/* screen number, -1 if not specified	*/
	char	*device;	/* device name, "/dev/console"		*/
	/*
	 * added after initial SI spec.
	 * new for SI v1.1
	 */
	char	*chipset;	/* type of chipset, ex: ET4000, etc 	*/
	int	videoRam;	/* display memory size			*/
	char	*model;		/* model/brand name of the video board	*/
	char	*vendor_lib;	/* vendor's shared lib, ex: et4k_256.so */
	SIint32	virt_w;		/* frame buffer actual width		*/
	SIint32	virt_h;		/* frame buffer actual height		*/
	SIint32	disp_w;		/* display area width within the frame buffer */
	SIint32	disp_h;		/* display area ht within the frame buffer */
	SIint32	depth;		/* frame buffer depth */
	SIMonitor monitor_info; /* ptr to monitor info record 	*/
	char	*info2vendorlib;/* any monitor specs; passed to DisplayModule */
	char	*IdentString;	/* DM identification string 		*/
	char	*priv;		/* private str 				*/
} SIConfig, *SIConfigP;

/*
 * This structure contains the per glyph information in a downloaded font
 */

typedef struct _SIGlyph {
	SIint16		SFlbearing;	/* left side bearing */
	SIint16		SFrbearing;	/* right side bearing */
	SIint16		SFwidth;	/* glyph width */
	SIint16		SFascent;	/* glyph ascent */
	SIint16		SFdescent;	/* glyph descent */
	SIbitmap	SFglyph;	/* The actual glyph */
} SIGlyph, *SIGlyphP;

/*
 * This structure contains the font bounds information, and is used to
 * determine if a font is "downloadable" to the SDD.
 */

#define SFTerminalFont		0x00000001
#define SFFixedWidthFont	0x00000002
#define SFNoOverlap		0x00000004

typedef struct _SIFontInfo {
	SIint32		SFnumglyph;	/* Number of glyphs in the font */
	SIint32		SFflag;		/* Font characteristics flag */
	SIint16		SFlascent;	/* Font logical acsent */
	SIint16		SFldescent;	/* Font logical decsent */
	SIGlyph		SFmin;		/* Minimum font bounds */
	SIGlyph		SFmax;		/* Maximum font bounds */
} SIFontInfo, *SIFontInfoP;


/*
 * Screen Interface Specification Version Numbers
 *
 * This was not present in the first release of SI; We should have put this
 * in the first place...;
 * This field used to be a 32 bit entity. In v1.1 it is split into two
 * 16 bit fields: one for server and the other for a display module.
 * 
 * Of the 2 bytes in this field, the high byte is used to indicate the major
 * version of SI, where as the low byte is used to indicate the minor version.
 * The server's version field (SIserver_version) is set before calling 
 * the si_init function from the Display Module.
 * The Display Module can check the SIserver_versionb field find out the 
 * SI version of the current server.
 * 
 * The 2nd field (SIdm_version) is set by the Display Module (DM)
 * (or SDD as we used to call these modules) (ie: libvga256.so.1)
 * This is important because if the DM doesn't set this field,
 * the server assumes the DM to be an older one (ie: compliant with SI v1.0)
 *  and not later. 
 * The arguments for some of the calls are different between v1.0
 * and later. The later one's are changed to get better performance.
 *
 * SIFlags.SIserver_version is initialized in io/init.c
 * 
 */
#define X_SI_VERSION1_1		0x0101	/* version 1.1 */

/*
 * This structure is used when finding out the initial information
 * about the display in question.
 *
 * The avail_* members are 32 bit quantities with a structured layout:
 *
 *	bits (0-7)	indicates functions available
 *	bits (8-15)	indicates function specific options available
 *	bits (16-31)	indicates general options available
 */
typedef struct _SIFlags {
	SIint16		SIserver_version; /* SI version that server is based */
	SIint16		SIdm_version; 	/* SI version that a DM is base on */
	SIint32		SIlinelen;	/* length of a scan line */
	SIint32		SIlinecnt;	/* number of scan lines */
	SIint32		SIxppin;	/* Pixels per inch (X direction) */
	SIint32		SIyppin;	/* Pixels per inch (Y direction) */
	SIint32		SIstatecnt;	/* number of graphics states */
	SIAvail		SIavail_bitblt;	/* Has bitblt routines */
	SIAvail		SIavail_stplblt;/* Has stipple blt routines */
	SIAvail		SIavail_fpoly;	/* Has polyfill routines */
	SIAvail		SIavail_point;	/* Has pointplot routine */
	SIAvail		SIavail_line;	/* Has line draw routines */
	SIAvail		SIavail_drawarc;/* Has draw arc routines */
	SIAvail		SIavail_fillarc;/* Has fill arc routines */
	SIAvail		SIavail_font;	/* Has hardware fonts */
	SIAvail		SIavail_spans;	/* Has spans routines */
	SIAvail		SIavail_memcache;/* Has memory caching routines */
	SIAvail		SIcursortype;	/* Cursor type FAKEHDWR/TRUEHDWR */
	SIint32		SIcurscnt;	/* Number of downloadable cursors */
	SIint32		SIcurswidth;	/* Best Cursor Width */
	SIint32		SIcursheight;	/* Best Cursor Height */
	SIint32		SIcursmask;	/* Mask for cursor bounds */
	SIint32		SItilewidth;	/* Best width for tile */
	SIint32		SItileheight;	/* Best height for tile */
	SIint32		SIstipplewidth;	/* Best width for stipple */
	SIint32		SIstippleheight;/* Best height for stipple */
	SIint32		SIfontcnt;	/* Number of downloadable fonts */
	SIVisualP	SIvisuals;	/* List of visuals supported by SDD */
	SIint32		SIvisualCNT;	/* Number of SDD visuals */
	SIAvail		SIavail_exten;	/* Has extensions */
	SIAvail		SIkeybd_event;	/* process  keyboard events */
	SIvoidP		SIfb_pbits;	/* pointer to frame buffer bits */
	SIint32		SIfb_width;	/* frame buffer width in pixels */
} SIFlags, *SIFlagsP;

/*
 * This structure contains a list of all the basic routines that 
 * must be written for the Screen Interface to work properly.
 */
#ifdef __STDC__
#define PROTO(x) x
#else
#define PROTO(x) ()
#endif

/*
 * for old naming compatibility
 */
#ifndef SIFunctions
#define SIFunctions	SI_1_1_Functions
#define SIFunctionsP	SI_1_1_FunctionsP
#endif

#define ScreenInterface SIFunctions 
#define SIInfo		SIFlags
#define SIInfoP		SIFlagsP

/*==========================================================================*/
/* SIFunctions for SI-1.0 */

typedef struct _SI_1_0_Functions {
  /**** (MANDATORY) MISCELLANEOUS ROUTINES ****/

  /**** machine dependant init ****/
  SIBool (*si_init)
    PROTO((int vt_fd,
	   SIConfigP configp,
	   SIInfoP info,
	   struct _SI_1_0_Functions **routines
	   ));
  /**** machine dependant cleanup ****/
  SIBool (*si_restore) PROTO((SIvoid));
  /**** machine dependant vt flip ****/
  SIBool (*si_vt_save) PROTO((SIvoid));
  /**** machine dependant vt flip ****/
  SIBool (*si_vt_restore) PROTO((SIvoid));
  /**** Turn on/off video blank ****/
  SIBool (*si_vb_onoff)
    PROTO((SIBool on
	   ));
  /**** start caching requests ****/
  SIBool (*si_initcache) PROTO((SIvoid));
  /**** write caching requests ****/
  SIBool (*si_flushcache) PROTO((SIvoid));
  /**** Set current state info ****/
  SIBool (*si_download_state)
    PROTO((SIint32 sindex,
	   SIint32 sflag,
	   SIGStateP statep
	   ));
  /**** Get current state info ****/
  SIBool (*si_get_state)
    PROTO((SIint32 sindex,
	   SIint32 sflag,
	   SIGStateP statep
	   ));
  /**** Select current GS entry ****/
  SIBool (*si_select_state)
    PROTO((SIint32 sindex
	   ));
  /**** Enter/Leave a screen ****/
  SIBool (*si_screen)
    PROTO((SIint32 screen,
	   SIint32 flag
	   ));

  /**** (MANDATORY) SCANLINE AT A TIME ROUTINES ****/

  /**** get pixels in a scanline ****/
  SILine (*si_getsl)
    PROTO((SIint32 y
	   ));
  /**** set pixels in a scanline ****/
  SIvoid (*si_setsl)
    PROTO((SIint32 y,
	   SILine ptr
	   ));
  /**** free scanline buffer ****/
  SIvoid (*si_freesl) PROTO((SIvoid));

  /**** (MANDATORY) COLORMAP MANAGEMENT ROUTINES ****/

  /**** Set Colormap entries ****/
  SIBool (*si_set_colormap)
    PROTO((SIint32 visual,
	   SIint32 cmap,
	   SIColor *colors,
	   SIint32 count
	   ));
  /**** Get Colormap entries ****/
  SIBool (*si_get_colormap)
    PROTO((SIint32 visual,
	   SIint32 cmap,
	   SIColor *colors,
	   SIint32 count
	   ));

  /**** CURSOR CONTROL ROUTINES ****/

  /**** Download a cursor ****/
  SIBool (*si_hcurs_download)
    PROTO((SIint32 cindex,
	   SICursorP cp
	   ));
  /**** Turnon the cursor ****/
  SIBool (*si_hcurs_turnon)
    PROTO((SIint32 cindex
	   ));
  /**** Turnoff the cursor ****/
  SIBool (*si_hcurs_turnoff)
    PROTO((SIint32 cindex
	   ));
  /**** Move the cursor position ****/
  SIBool (*si_hcurs_move)
    PROTO((SIint32 cindex,
	   SIint32 x,
	   SIint32 y
	   ));

  /********* ALL OPTIONAL BELOW HERE ********/
  /**** HARDWARE SPANS CONTROL ****/

  /**** fill spans ****/
  SIBool (*si_fillspans)
    PROTO((SIint32 count,
	   SIPointP ptsIn,
	   SIint32 *widths
	   ));

  /**** HARDWARE BITBLT ROUTINES ****/

  /**** perform scr->scr bitblt ****/
  SIBool (*si_ss_bitblt)
    PROTO((SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt
	   ));
  /**** perform mem->scr bitblt ****/
  SIBool (*si_ms_bitblt)
    PROTO((SIbitmapP src,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt
	   ));
  /**** perform scr->mem bitblt ****/
  SIBool (*si_sm_bitblt)
    PROTO((SIbitmapP dst,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt
	   ));

  /**** HARDWARE BITBLT ROUTINES ****/

  /**** perform scr->scr stipple ****/
  SIBool (*si_ss_stplblt)
    PROTO((SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 plane,
	   SIint32 forcetype
	   ));
  /**** perform mem->scr stipple ****/
  SIBool (*si_ms_stplblt)
    PROTO((SIbitmapP src,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 plane,
	   SIint32 forcetype
	   ));
  /**** perform scr->mem stipple ****/
  SIBool (*si_sm_stplblt)
    PROTO((SIbitmapP dst,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 plane,
	   SIint32 forcetype
	   ));

  /**** HARDWARE POLYGON FILL ****/

  /**** set polygon clip ****/
  SIvoid (*si_poly_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** for convex polygons ****/
  SIBool (*si_poly_fconvex)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));
  /**** for general polygons ****/
  SIBool (*si_poly_fgeneral)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));
  /**** for rectangular regions ****/
  SIBool (*si_poly_fillrect)
    PROTO((SIint32 count,
	   SIRectP prects
	   ));

  /**** HARDWARE POINT PLOTTING ****/

  SIBool (*si_plot_points)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));

  /**** HARDWARE LINE DRAWING ****/

  /**** set line draw clip ****/
  SIvoid (*si_line_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** One bit line (connected) ****/
  SIBool (*si_line_onebitline)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));
  /**** One bit line segments ****/
  SIBool (*si_line_onebitseg)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));
  /**** One bit line rectangles ****/
  SIBool (*si_line_onebitrect)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));

  /**** HARDWARE DRAW ARC ROUTINE ****/

  /**** set drawarc clip ****/
  SIvoid (*si_drawarc_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** draw arc ****/
  SIBool (*si_drawarc)
    PROTO((SIint32 x,
	   SIint32 y,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 arc1,
	   SIint32 arc2
	   ));

  /**** HARDWARE FILL ARC ROUTINE ****/

  /**** set fill arc clip ****/
  SIvoid (*si_fillarc_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** fill arc ****/
  SIBool (*si_fillarc)
    PROTO((SIint32 x,
	   SIint32 y,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 arc1,
	   SIint32 arc2
	   ));

  /**** HARDWARE FONT CONTROL ****/

  /**** Check Font downloadability ****/
  SIBool (*si_font_check)
    PROTO((SIint32 fontnum,
	   SIFontInfoP fontinfo
	   ));
  /**** Download a font command ****/
  SIBool (*si_font_download)
    PROTO((SIint32 fontnum,
	   SIFontInfoP fontinfo,
	   SIGlyphP glyphlist
	   ));
  /**** free a downloaded font ****/
  SIBool (*si_font_free)
    PROTO((SIint32 fontnum
	   ));
  /**** set font clip ****/
  SIvoid (*si_font_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** stipple a list of glyphs ****/
  SIBool (*si_font_stplblt)
    PROTO((SIint32 fontnum,
	   SIint32 x,
	   SIint32 y,
	   SIint32 count,
	   SIint16 *glyphs,
	   SIint32 forcetype
	   ));

  /**** SDD MEMORY CACHING CONTROL ****/

  /**** allocate pixmap into cache ****/
  SIBool (*si_cache_alloc)
    PROTO((SIbitmapP buf,
	   SIint32 type
	   ));
  /**** remove pixmap from cache ****/
  SIBool (*si_cache_free)
    PROTO((SIbitmapP buf
	   ));
  /**** lock pixmap into cache ****/
  SIBool (*si_cache_lock)
    PROTO((SIbitmapP buf
	   ));
  /**** unlock pixmap from cache ****/
  SIBool (*si_cache_unlock)
    PROTO((SIbitmapP buf
	   ));

  /**** SDD EXTENSION INITIALIZATION ****/

  /**** extension initialization ****/
  SIBool (*si_exten_init) PROTO((SIvoid));

} SI_1_0_Functions, *SI_1_0_FunctionsP;

/*==========================================================================*/
/* SIFunctions for SI-1.1 */

typedef struct _SI_1_1_Functions {
  /**** (MANDATORY) MISCELLANEOUS ROUTINES ****/

  /**** machine dependant init ****/
  SIBool (*si_init)
    PROTO((int vt_fd,
	   SIConfigP configp,
	   SIFlagsP info,
	   struct _SI_1_1_Functions **routines
	   ));
  /**** machine dependant cleanup ****/
  SIBool (*si_restore) PROTO((SIvoid));
  /**** machine dependant vt flip ****/
  SIBool (*si_vt_save) PROTO((SIvoid));
  /**** machine dependant vt flip ****/
  SIBool (*si_vt_restore) PROTO((SIvoid));
  /**** Turn on/off video blank ****/
  SIBool (*si_vb_onoff)
    PROTO((SIBool on
	   ));
  /**** start caching requests ****/
  SIBool (*si_initcache) PROTO((SIvoid));
  /**** write caching requests ****/
  SIBool (*si_flushcache) PROTO((SIvoid));
  /**** Set current state info ****/
  SIBool (*si_download_state)
    PROTO((SIint32 sindex,
	   SIint32 sflag,
	   SIGStateP statep
	   ));
  /**** Get current state info ****/
  SIBool (*si_get_state)
    PROTO((SIint32 sindex,
	   SIint32 sflag,
	   SIGStateP statep
	   ));
  /**** Select current GS entry ****/
  SIBool (*si_select_state)
    PROTO((SIint32 sindex
	   ));
  /**** Enter/Leave a screen ****/
  SIBool (*si_screen)
    PROTO((SIint32 screen,
	   SIint32 flag
	   ));

  /**** (MANDATORY) SCANLINE AT A TIME ROUTINES ****/

  /**** get pixels in a scanline ****/
  SILine (*si_getsl)
    PROTO((SIint32 y
	   ));
  /**** set pixels in a scanline ****/
  SIvoid (*si_setsl)
    PROTO((SIint32 y,
	   SILine ptr
	   ));
  /**** free scanline buffer ****/
  SIvoid (*si_freesl) PROTO((SIvoid));

  /**** (MANDATORY) COLORMAP MANAGEMENT ROUTINES ****/

  /**** Set Colormap entries ****/
  SIBool (*si_set_colormap)
    PROTO((SIint32 visual,
	   SIint32 cmap,
	   SIColor *colors,
	   SIint32 count
	   ));
  /**** Get Colormap entries ****/
  SIBool (*si_get_colormap)
    PROTO((SIint32 visual,
	   SIint32 cmap,
	   SIColor *colors,
	   SIint32 count
	   ));

  /**** CURSOR CONTROL ROUTINES ****/

  /**** Download a cursor ****/
  SIBool (*si_hcurs_download)
    PROTO((SIint32 cindex,
	   SICursorP cp
	   ));
  /**** Turnon the cursor ****/
  SIBool (*si_hcurs_turnon)
    PROTO((SIint32 cindex
	   ));
  /**** Turnoff the cursor ****/
  SIBool (*si_hcurs_turnoff)
    PROTO((SIint32 cindex
	   ));
  /**** Move the cursor position ****/
  SIBool (*si_hcurs_move)
    PROTO((SIint32 cindex,
	   SIint32 x,
	   SIint32 y
	   ));

  /********* ALL OPTIONAL BELOW HERE ********/
  /**** HARDWARE SPANS CONTROL ****/

  /**** fill spans ****/
  SIBool (*si_fillspans)
    PROTO((SIint32 count,
	   SIPointP ptsIn,
	   SIint32 *widths
	   ));

  /**** HARDWARE BITBLT ROUTINES ****/

  /**** perform scr->scr bitblt ****/
  SIBool (*si_ss_bitblt)
    PROTO((SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt
	   ));
  /**** perform mem->scr bitblt ****/
  SIBool (*si_ms_bitblt)
    PROTO((SIbitmapP src,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt
	   ));
  /**** perform scr->mem bitblt ****/
  SIBool (*si_sm_bitblt)
    PROTO((SIbitmapP dst,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt
	   ));

  /**** HARDWARE BITBLT ROUTINES ****/

  /**** perform scr->scr stipple ****/
  SIBool (*si_ss_stplblt)
    PROTO((SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 plane,
	   SIint32 forcetype
	   ));
  /**** perform mem->scr stipple ****/
  SIBool (*si_ms_stplblt)
    PROTO((SIbitmapP src,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 plane,
	   SIint32 forcetype
	   ));
  /**** perform scr->mem stipple ****/
  SIBool (*si_sm_stplblt)
    PROTO((SIbitmapP dst,
	   SIint32 sx,
	   SIint32 sy,
	   SIint32 dx,
	   SIint32 dy,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 plane,
	   SIint32 forcetype
	   ));

  /**** HARDWARE POLYGON FILL ****/

  /**** set polygon clip ****/
  SIvoid (*si_poly_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** for convex polygons ****/
  SIBool (*si_poly_fconvex)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));
  /**** for general polygons ****/
  SIBool (*si_poly_fgeneral)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));
  /**** for rectangular regions (SI-1.1) ****/
  SIBool (*si_poly_fillrect)
    PROTO((SIint32 xorg,
	   SIint32 yorg,
	   SIint32 count,
	   SIRectOutlineP prects
	   ));

  /**** HARDWARE POINT PLOTTING ****/

  SIBool (*si_plot_points)
    PROTO((SIint32 count,
	   SIPointP ptsIn
	   ));

  /**** HARDWARE LINE DRAWING ****/

  /**** set line draw clip ****/
  SIvoid (*si_line_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** One bit line (connected) (SI-1.1) ****/
  SIBool (*si_line_onebitline)
    PROTO((SIint32 xorg,
	   SIint32 yorg,
	   SIint32 count,
	   SIPointP ptsIn,
	   SIint32 capNotLast,
	   SIint32 mode
	   ));
  /**** One bit line segments (SI-1.1) ****/
  SIBool (*si_line_onebitseg)
    PROTO((SIint32 xorg,
	   SIint32 yorg,
	   SIint32 count,
	   SISegmentP ptsIn,
	   SIint32 capNotLast
	   ));
  /**** One bit line rectangles ****/
  SIBool (*si_line_onebitrect)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));

  /**** HARDWARE DRAW ARC ROUTINE ****/

  /**** set drawarc clip ****/
  SIvoid (*si_drawarc_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** draw arc ****/
  SIBool (*si_drawarc)
    PROTO((SIint32 x,
	   SIint32 y,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 arc1,
	   SIint32 arc2
	   ));

  /**** HARDWARE FILL ARC ROUTINE ****/

  /**** set fill arc clip ****/
  SIvoid (*si_fillarc_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** fill arc ****/
  SIBool (*si_fillarc)
    PROTO((SIint32 x,
	   SIint32 y,
	   SIint32 wid,
	   SIint32 hgt,
	   SIint32 arc1,
	   SIint32 arc2
	   ));

  /**** HARDWARE FONT CONTROL ****/

  /**** Check Font downloadability ****/
  SIBool (*si_font_check)
    PROTO((SIint32 fontnum,
	   SIFontInfoP fontinfo
	   ));
  /**** Download a font command ****/
  SIBool (*si_font_download)
    PROTO((SIint32 fontnum,
	   SIFontInfoP fontinfo,
	   SIGlyphP glyphlist
	   ));
  /**** free a downloaded font ****/
  SIBool (*si_font_free)
    PROTO((SIint32 fontnum
	   ));
  /**** set font clip ****/
  SIvoid (*si_font_clip)
    PROTO((SIint32 x1,
	   SIint32 y1,
	   SIint32 x2,
	   SIint32 y2
	   ));
  /**** stipple a list of glyphs ****/
  SIBool (*si_font_stplblt)
    PROTO((SIint32 fontnum,
	   SIint32 x,
	   SIint32 y,
	   SIint32 count,
	   SIint16 *glyphs,
	   SIint32 forcetype
	   ));

  /**** SDD MEMORY CACHING CONTROL ****/

  /**** allocate pixmap into cache ****/
  SIBool (*si_cache_alloc)
    PROTO((SIbitmapP buf,
	   SIint32 type
	   ));
  /**** remove pixmap from cache ****/
  SIBool (*si_cache_free)
    PROTO((SIbitmapP buf
	   ));
  /**** lock pixmap into cache ****/
  SIBool (*si_cache_lock)
    PROTO((SIbitmapP buf
	   ));
  /**** unlock pixmap from cache ****/
  SIBool (*si_cache_unlock)
    PROTO((SIbitmapP buf
	   ));

  /**** SDD EXTENSION INITIALIZATION ****/

  /**** extension initialization ****/
  SIBool (*si_exten_init) PROTO((SIvoid));

  /**** SDD KEYBOARD EVENTS ****/

  /**** process keyboard event ****/
   SIBool (*si_proc_keybdevent)
     PROTO((SIint32
	    ));
} SI_1_1_Functions, *SI_1_1_FunctionsP;

/*==========================================================================*/

typedef struct _GSCache {
    SIint32     gs_validval;
    SIGStateP   gs_ptr;
    SIGState    gs_cur;
    SIint32     gs_clipsz;
} GSCache, *GSCacheP;

typedef struct _SIScreenRec {
	SIConfigP	cfgPtr;
	SIFlagsP	flagsPtr;
	SIFunctionsP	funcsPtr;
	SICmapP		cmapPtr;
	SIvoid		*classPriv;
	SIvoid		*vendorPriv;
	SIint32		nextState;
	GSCacheP	gsCachePtr;
	SIint32		*fontsUsed;
	SIint32		fontFontIndex;
	unsigned long	fontGeneration;		
	unsigned long	scrInitGeneration;
} SIScreenRec, *SIScreenRecP;

extern SIScreenRec	siScreens[];
extern int              siNumScreens;

extern SIScreenRecP	global_pSIScreen;
#define siConfig	(pSIScreen->cfgPtr)
#define siFlags		(pSIScreen->flagsPtr)
#define siFuncs		(pSIScreen->funcsPtr)
#define siColormap	(pSIScreen->cmapPtr)
#define siNextState	(pSIScreen->nextState)
#define siGSCache	(pSIScreen->gsCachePtr)
#define siFontsUsed	(pSIScreen->fontsUsed)
#define siFontFontIndex (pSIScreen->fontFontIndex)
#define siFontGeneration (pSIScreen->fontGeneration)
#define siScrInitGeneration (pSIScreen->scrInitGeneration)

#define si_setScreen(pScreen) \
   global_pSIScreen = pSIScreen = \
     (SIScreenRecP)(pScreen)->devPrivates[siScreenIndex].ptr;

#define si_prepareScreen(pScreen) \
   SIScreenRecP pSIScreen; \
   si_setScreen(pScreen);

#define si_currentScreen() \
   SIScreenRecP pSIScreen = global_pSIScreen;

extern char		*siSTATEerr;

/*
 * general availability flags
 */
#define TILE_AVAIL		0x10000
#define OPQSTIPPLE_AVAIL	0x20000
#define STIPPLE_AVAIL		0x40000

#define	si_hastile(avail)	((siFlags->avail) & TILE_AVAIL)
#define	si_hasstipple(avail)	((siFlags->avail) & STIPPLE_AVAIL)
#define	si_hasopqstipple(avail)	((siFlags->avail) & OPQSTIPPLE_AVAIL)

#define CLIPLIST_AVAIL		0x80000

#define	si_hascliplist(avail)	((siFlags->avail) & CLIPLIST_AVAIL)

#define DASH_AVAIL		0x100000
#define DBLDASH_AVAIL		0x200000

#define	si_hasdash(avail)	((siFlags->avail) & DASH_AVAIL)
#define	si_hasdbldash(avail)	((siFlags->avail) & DBLDASH_AVAIL)
#define	si_hasanydash(avail)	((siFlags->avail) & (DASH_AVAIL | DBLDASH_AVAIL))

#define CHORD_AVAIL		0x400000
#define PIESLICE_AVAIL		0x800000

#define	si_haschord(avail)	((siFlags->avail) & CHORD_AVAIL)
#define	si_haspieslice(avail)	((siFlags->avail) & PIESLICE_AVAIL)

/*
 * miscellaneous defines
 */

/* Screen Control */
#define	si_Init			(siFuncs->si_init)
#define	si_Restore		(siFuncs->si_restore)
#define	si_VTSave		(siFuncs->si_vt_save)
#define	si_VTRestore		(siFuncs->si_vt_restore)
#define	si_VBOnOff		(siFuncs->si_vb_onoff)

#define	si_Initcache		(siFuncs->si_initcache)
#define	si_Flushcache		(siFuncs->si_flushcache)
#define	si_downloadstate	(siFuncs->si_download_state)
#define	si_getstateinfo		(siFuncs->si_get_state)
#define	si_selectstate		(siFuncs->si_select_state)

/* Macro(s) for accessing parts of the info structure */

#define	si_GetInfoVal(a)	(siFlags->a)
#define	si_SetInfoVal(a,b)	( (siFlags->a) = (b) )

/* Commonly accessed values */

#define	si_getscanlinelen	si_GetInfoVal(SIlinelen)
#define	si_getscanlinecnt	si_GetInfoVal(SIlinecnt)

/* Macro(s) for accessing common subroutines */

/*
 * si_PrepareGS must be called in si before any graphics drawing occurs
 * This version does not force a validate.  A validate is forced as ValidateGC
 * time.
 * R3->R4 changes: 6/26/90
 */

#define si_PrepareGS(pGC)	\
	{ SIint32 _index = ((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GStateidx; \
		sivalidatestate(_index, \
				&((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GState, \
				SI_FALSE, \
				((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GSmodified); \
		if ( si_selectstate(_index) == SI_FAIL ) \
			FatalError(siSTATEerr); \
	}

/*
 * si_PrepareGS2 must be called in si before any graphics drawing occurs
 * This version always forces a change.  It is used for window operations.
 */

#define si_PrepareGS2(index,pGS)	\
	{ \
		sivalidatestate((index),(pGS), SI_TRUE, (SIint32)0); \
		if ( si_selectstate(index) == SI_FAIL ) \
			FatalError(siSTATEerr); \
	}

/*
 * si_PrepareGS3 must be called in si before any graphics drawing occurs
 * This version always forces a change.  It is used for window operations.
 * This version optinally allows cliplist/tiles/stipples to be changed.
 */

#define si_PrepareGS3(index,pGS,mod)	\
	{ \
		sivalidatestate((index),(pGS), SI_TRUE, (SIint32)mod); \
		if ( si_selectstate(index) == SI_FAIL ) \
			FatalError(siSTATEerr); \
	}

/*
 * Routine to enter/leave a screen (for multiheaded support)
 */
#define	SCREEN_ENTER	1
#define	SCREEN_LEAVE	2

#define	si_enterleavescreen		(siFuncs->si_screen)


/*
 * scanline routines
 */

#define	si_getscanline		(siFuncs->si_getsl)
#define	si_setscanline		(siFuncs->si_setsl)
#define	si_freescanline		(siFuncs->si_freesl)

/*
 * colormap routines
 */

#define	si_getcolormap		(siFuncs->si_get_colormap)
#define	si_setcolormap		(siFuncs->si_set_colormap)

/*
 * bitblt routine defines
 */

#define	SSBITBLT_AVAIL	1
#define	MSBITBLT_AVAIL	2
#define	SMBITBLT_AVAIL	4

#define si_hasssbitblt		((siFlags->SIavail_bitblt) & SSBITBLT_AVAIL)
#define si_hasmsbitblt		((siFlags->SIavail_bitblt) & MSBITBLT_AVAIL)
#define si_hassmbitblt		((siFlags->SIavail_bitblt) & SMBITBLT_AVAIL)

#define si_SSbitblt		(siFuncs->si_ss_bitblt)
#define si_MSbitblt		(siFuncs->si_ms_bitblt)
#define si_SMbitblt		(siFuncs->si_sm_bitblt)

/*
 * stipple blt routine defines
 */

#define	SSSTPLBLT_AVAIL	1
#define	MSSTPLBLT_AVAIL	2
#define	SMSTPLBLT_AVAIL	4

#define si_hasssstplblt		((siFlags->SIavail_stplblt) & SSSTPLBLT_AVAIL)
#define si_hasmsstplblt		((siFlags->SIavail_stplblt) & MSSTPLBLT_AVAIL)
#define si_hassmstplblt		((siFlags->SIavail_stplblt) & SMSTPLBLT_AVAIL)

#define si_SSstplblt		(siFuncs->si_ss_stplblt)
#define si_MSstplblt		(siFuncs->si_ms_stplblt)
#define si_SMstplblt		(siFuncs->si_sm_stplblt)

/*
 * filled polygon routines
 */

#define CONVEXPOLY_AVAIL	0x00000001
#define GENERALPOLY_AVAIL	0x00000002
#define RECTANGLE_AVAIL		0x00000004
#define POLYEVENODD_AVAIL	0x00000100
#define POLYWINDING_AVAIL	0x00000200

#define si_canpolyfill		((siFlags->SIavail_fpoly) & \
		(CONVEXPOLY_AVAIL | GENERALPOLY_AVAIL | RECTANGLE_AVAIL) )

#define si_hasconvexfpolygon	((siFlags->SIavail_fpoly) & CONVEXPOLY_AVAIL)
#define si_hasgeneralfpolygon	((siFlags->SIavail_fpoly) & GENERALPOLY_AVAIL)
#define si_hasfillrectangle	((siFlags->SIavail_fpoly) & RECTANGLE_AVAIL)

#define si_canevenoddfill	((siFlags->SIavail_fpoly) & POLYEVENODD_AVAIL)
#define si_canwindingfill	((siFlags->SIavail_fpoly) & POLYWINDING_AVAIL)

#define si_setpolyclip		(siFuncs->si_poly_clip)
#define si_fillconvexpoly	(siFuncs->si_poly_fconvex)
#define si_fillgeneralpoly	(siFuncs->si_poly_fgeneral)
#define si_fillrectangle	(siFuncs->si_poly_fillrect)

#define si_1_0_fillrectangle	(((SI_1_0_FunctionsP)siFuncs)-> \
				 si_poly_fillrect)

/*
 * point plot routines
 */

#define PLOTPOINT_AVAIL		0x00000001

#define si_hasplotpoint		((siFlags->SIavail_point) & PLOTPOINT_AVAIL)

#define si_plotpoints		(siFuncs->si_plot_points)

/*
 * line drawing routines
 */

#define ONEBITLINE_AVAIL	0x00000001
#define ONEBITSEG_AVAIL		0x00000002
#define ONEBITRECT_AVAIL	0x00000004

#define si_canonebit		((siFlags->SIavail_line) & \
		(ONEBITLINE_AVAIL | ONEBITSEG_AVAIL) )

#define si_haslinedraw		((siFlags->SIavail_line) & ONEBITLINE_AVAIL)
#define si_haslineseg		((siFlags->SIavail_line) & ONEBITSEG_AVAIL)
#define si_haslinerect		((siFlags->SIavail_line) & ONEBITRECT_AVAIL)

#define si_setlineclip		(siFuncs->si_line_clip)
#define si_onebitlinedraw	(siFuncs->si_line_onebitline)
#define si_onebitlineseg	(siFuncs->si_line_onebitseg)
#define si_onebitlinerect	(siFuncs->si_line_onebitrect)

#define si_1_0_onebitlinedraw	(((SI_1_0_FunctionsP)siFuncs)-> \
				 si_line_onebitline)
#define si_1_0_onebitlineseg	(((SI_1_0_FunctionsP)siFuncs)-> \
				 si_line_onebitseg)

/*
 * arc drawing routines
 */
#define ONEBITARC_AVAIL		0x00000001
#define FILLARC_AVAIL		0x00000001


#define si_hasdrawarc		((siFlags->SIavail_drawarc) & ONEBITARC_AVAIL)
#define si_hasfillarc		((siFlags->SIavail_fillarc) & FILLARC_AVAIL)

#define si_setdrawarcclip	(siFuncs->si_drawarc_clip)
#define si_Drawarc		(siFuncs->si_drawarc)

#define si_setfillarcclip	(siFuncs->si_fillarc_clip)
#define si_Fillarc		(siFuncs->si_fillarc)

/*
 * hardware cursor routines
 */
#define CURSOR_NONE	1	/* use complete software emulation */
#define CURSOR_TRUEHDWR	2	/* Hardware does it all */
#define CURSOR_FAKEHDWR	4	/* must turn on/off cursor before draw ops */

#define si_havenocursor		((siFlags->SIcursortype) & CURSOR_NONE)
#define si_havetruecursor	((siFlags->SIcursortype) & CURSOR_TRUEHDWR)
#define si_havefakecursor	((siFlags->SIcursortype) & CURSOR_FAKEHDWR)

#define si_bestcursorwidth	(siFlags->SIcurswidth)
#define si_bestcursorheight	(siFlags->SIcursheight)
#define si_cursormask		(siFlags->SIcursmask)

#define si_downloadcursor	(siFuncs->si_hcurs_download)
#define si_turnoncursor		(siFuncs->si_hcurs_turnon)
#define si_turnoffcursor	(siFuncs->si_hcurs_turnoff)
#define si_movecursor		(siFuncs->si_hcurs_move)

/*
 * tile/stipple widths and heights
 */
#define si_maxtilewidth		(siFlags->SItilewidth)
#define si_maxtileheight	(siFlags->SItileheight)

#define si_maxstipplewidth	(siFlags->SIstipplewidth)
#define si_maxstippleheight	(siFlags->SIstippleheight)


/*
 * hardware font downloading and drawing routines
 */

#define FONT_AVAIL		0x00000001

#define si_havedlfonts		((siFlags->SIavail_font) & FONT_AVAIL)

#define si_checkfont		(siFuncs->si_font_check)
#define si_fontdownload		(siFuncs->si_font_download)
#define si_fontfree		(siFuncs->si_font_free)
#define si_fontclip		(siFuncs->si_font_clip)
#define si_fontstplblt		(siFuncs->si_font_stplblt)

/*
 * span filling routines
 */

#define SPANS_AVAIL		0x00000001

#define si_canspansfill		((siFlags->SIavail_spans) & SPANS_AVAIL)
#define si_Fillspans		(siFuncs->si_fillspans)

/*
 * Color Specification Flags
 */
#define STATICGRAY_AVAIL	StaticGray
#define GRAYSCALE_AVAIL		GrayScale
#define STATICCOLOR_AVAIL	StaticColor
#define PSEUDOCOLOR_AVAIL	PseudoColor
#define TRUECOLOR_AVAIL		TrueColor
#define DIRECTCOLOR_AVAIL	DirectColor

/*
 * cache memory
 */
#define SHORTTERM_MEM		0
#define LONGTERM_MEM		1

#define si_hascachememory	(siFlags->SIavail_memcache)

#define si_CacheAlloc(a,b) \
	if (siFlags->SIavail_memcache) \
		(siFuncs->si_cache_alloc) (a, b)

#define si_CacheFree(a,b) \
	if ((a) != NullSIbitmap) { \
	    if ((b) == SU_SDDCACHE) \
		(siFuncs->si_cache_free)(a); \
	    else \
		if ((a)->Bptr != NULL) \
			xfree ((pointer)((a)->Bptr)); \
	    xfree (a); \
	}

/*
 * Extensions
 */
#define	PRIVATE_EXTENSION	1

/*
 * New v1.1: new one-bit fast line drawing routines.
 *	     these are low overhead functions...
 */

#define	SICoordModePrevious	0
#define SICoordModeOrigin	1

/*
 * VERSION 1.0 is deliberately set to 0, because in the first release of
 * SVR4.2, we didn't bother about these issues (in other words we goofed)
 * So, if we see a 0, the server assumes it is pre 1.1 version. Some of the
 * arguments to the functions changes between 1.0 and later. If the DM is
 * pre 1.1, the server guarantees function arguments in the old format
 * where as post 1.0 will get newer format
 */
#define DM_SI_VERSION_1_0	0x0000
#define DM_SI_VERSION_1_1	0x0101	/* SI version 1.1 */

#define	SI_SCREEN_DM_VERSION(pScreen) \
  (((SIScreenRec *)((pScreen)->devPrivates[siScreenIndex].ptr)) \
   ->flagsPtr->SIdm_version)

/*
 * Defines common to many things
 */
#define si_hasanycliplist    ( \
			((siFlags->SIavail_fpoly) & CLIPLIST_AVAIL) || \
			((siFlags->SIavail_line) & CLIPLIST_AVAIL) || \
			((siFlags->SIavail_fillarc) & CLIPLIST_AVAIL) || \
			((siFlags->SIavail_font) & CLIPLIST_AVAIL) \
			    )
/*
 * for extensions
 */
#define si_haveexten  ((siFlags->SIavail_exten) & PRIVATE_EXTENSION)

#define si_exten_init		(siFuncs->si_exten_init)

/*
 * for keyboard events
 */
#define si_haskeybdevents \
  ( (siFlags->SIdm_version > DM_SI_VERSION_1_0) && (siFlags->SIkeybd_event) )

#define si_ProcKeybdEvent	(siFuncs->si_proc_keybdevent)


#define INIT_RUNTIME_EXT_MI		1
#define INIT_RUNTIME_EXT_SCR	2

typedef struct _xwinRExtns 
{
	char	*name;		/* extension name/keyword */
	char	*library;	/* library name */
	char	*mi_function;	/* function to be called from mi/InitExtensions */
	char	*scr_function;	/* function to be called from siScreenInit */
	struct  _xwinRExtns	*np;
} xwinRExtns, *xwinRExtnsP;

/*
 * for direct framebuffer access
 */
#define SI_FB_NULL	((SIvoidP)-1)
#define si_have_fb	(siFlags->SIfb_pbits != SI_FB_NULL)

#define	SI_SRC_IS_SCR	1
#define	SI_DST_IS_SCR	2
#define	SI_ASSIST_ROP	4

#endif	/* _SIDEP_H */
