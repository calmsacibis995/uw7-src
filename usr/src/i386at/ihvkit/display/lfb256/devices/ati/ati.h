#ident	"@(#)ihvkit:display/lfb256/devices/ati/ati.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#ifndef _ATI_H_
#define _ATI_H_

#include <sys/types.h>
#include <sys/inline.h>

#include <sidep.h>
#include <lfb.h>
#include <mach32regs.h>
#include <atinames.h>

/*
 * From <macros.h>.  Included here because <macros.h> includes
 * <sys/types.h> and <sys/stat.h>.  The latter contains source for a
 * number of functions, which we don't need.
 *
 */

#define max(a,b) ((a)<(b) ? (b) : (a))
#define min(a,b) ((a)>(b) ? (b) : (a))

/* 
 * The 8514/a definition uses 12 bit values for coordinates.  The 
 * range of the coordinate system is -512..1535 (inclusive).
 *
 * This driver uses off screen memory for fonts, bitmaps, tiles, and 
 * the cursor.  The cursor does not need to be inside the coordinate 
 * range of the graphics engine, but the rest do.  To allow more off 
 * screen memory, the GE and CRT stride are set to a minimum of 1024 
 * when there is enough VRAM.  See comments in atiInit.c and atiMisc.c 
 * for more details.
 *
 * One possible enhancement would be to use the full 2K screen height.
 * To do this, the GE offset should be moved to point to scan line 512
 * and all commands would have 512 subtracted from the Y (not height)
 * coordiante.
 *
 */

#define MAX_SCREEN_COORD 1535
#define MIN_SCREEN_COORD -512

/*
 * Some info on the colormaps.  ATI_CMAP_SIZE should probably be changed
 * for 16bpp, but we have no experience with this.
 *
 */

#define ATI_NUM_CMAPS 1
#define ATI_CMAP_SIZE 256
#define ATI_BITS_RGB 8

/*
 * Bitmaps and pixmaps are expanded internally until they are at least
 * this size.  This generally gives better performance, as most draws
 * are larger than this size and require less blits to get the whole
 * image out.
 *
 */

#define ATI_PIX_W 32
#define ATI_PIX_H 32

/*
 * Cursor def is stored in SIint16.  @ 2 bpp, the cursor needs 128
 * bits, or 8 SIint16s.
 *
 */

#define ATI_CURSOR_STRIDE 8
#define ATI_CURSOR_WIDTH  64
#define ATI_CURSOR_HEIGHT 64
#define ATI_CURSOR_SIZE (ATI_CURSOR_STRIDE * ATI_CURSOR_HEIGHT)
#define ATI_NUM_CURSORS 4

/* 
 * The ATI supports fonts using 8514/a modes.  The fonts are stored on a 
 * single bit plane, and are drawn using a blit with monochrome source.  To 
 * simplify the code, only 8 fonts are allowed, even for 16 BPP.
 *
 */

#define ATI_NUM_FONTS 8

/*
 * FIFO macros.  ATI_FLUSH_GE() pauses until the GE is done.  This is
 * necessary before the accessing the FB directly.
 *
 * ATI_NEED_FIFO(n) pauses until there are n empty slots in the FIFO.
 * ATI suggests saving the number of empty slots away and only
 * querying when there are not enough, but the overhead for accessing
 * a global variable is about the same as the overhead for doing the
 * inw() in PIC code..
 *
 */

#define ATI_FLUSH_GE() while (inw(EXT_GE_STATUS) & 0x2000)
#define ATI_NEED_FIFO(n) do {} while (inw(EXT_FIFO_STATUS) & (1 << (16 - (n))))

/*
 * Bug workarounds
 *
 * The following defines are used to conditionally compile code to
 * work around chip bugs and problems.  When new versions of the chip
 * are used, the code should be revised to accomodate possible fixes.
 *
 * For example, Rev 3 of the ATI has a bug in the cursor (see below).
 * The Rev 6 version reportedly fixes the bug.  The workaround code
 * will still work on the Rev 6 part, but the maximum cursor width is
 * 62, not 64.  If the bugs is indeed fixed, someone could add chip
 * revision detecting during initialization, and then revise the
 * cursor code to detect which version of the chip is present and
 * operate accordingly.
 *
 */

/*
 * The Mach32 chip can operate in both 8514 and extended modes
 * simultaneously.  Most primitives can be drawn in either mode.
 * Testing indicates that while the extended modes are easier to use,
 * and often involve less port I/O, they are more expensive,
 * time-wise.  For example, rectangles require 5 ports writes in
 * either mode, but the 8514 mode is almost twice as fast.
 *
 * A companion program has been supplied for this driver which gives
 * some indication as to what the best way for implementing a
 * particular drawing primitive is.  See the comments at the beginning
 * of atiSpeed.c for more details.
 *
 * When ATI_SLOW_EXTENSIONS is defined, 8514 mode is used when
 * possible.
 *
 */

#define ATI_SLOW_EXTENSIONS

/*
 * The Mach32 Rev 3 chip has a number of cursor bugs.  The bugs are
 * only present in 1280x1024 NI modes, which are of the most interest,
 * of course.  The symptoms are that the if HORZ_CURSOR_OFFSET is 0,
 * the ccursor is only displayed when HORZ_CURSOR_POSN is even.  Also,
 * when HORZ_CURSOR_OFFSET is even (>0) and HORZ_CURSOR_POSN is odd,
 * the right column (63) of the cursor is not displayed.
 *
 * The only solution right now is to not use the right- and left-most
 * columns of the cursor.  Place the cursor starting at column 1 and
 * only use up to 62 columns.  Add 1 to HORZ_CURSOR_OFFSET.  This fix
 * is selected when CURSOR_BUG_R3 is defined.
 *
 */

#define CURSOR_BUG_R3

/* 
 * Detect ESMP vs standard SVR4.2.  SI86IOPL is a new sysi86 function 
 * in ESMP.  Prior to ESMP, the IOPL was set using a VPIX 
 * sub-function.  This will be used during the init routine to disable 
 * the TSS IO bitmap by setting the IOPL to 3.
 *
 */

#include <sys/sysi86.h>

#ifdef SI86IOPL
#define ESMP
#else
/* From <sys/v86.h> */
#define V86SC_IOPL      4               /* v86iopriv () system call     */
#endif

#ifdef ESMP
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl))
#else
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl) << 12)
#endif

typedef struct {
    int bitmap_off;		/* Scan line where bitmaps start */
    int bitmap_h;
    int font_off;		/* Scan line where fonts start */
    int font_h;
    int curs_off;		/* Scan line where cursor starts */
} ATI;

/* Force STRUCT_INVALID to 0 so that everythings inits to this value. */
typedef enum {
    STRUCT_INVALID = 0,
    STRUCT_VALID,
    DATA_INVALID
} VALID;

typedef struct {
    VALID valid;
    int x, y, w, h;
    int pw, ph;			/* Pat w and h */
    int fg, bg;			/* For stipples */
} ATI_OFF_SCRN;

extern ATI ati;

extern ATI_OFF_SCRN atiOffTiles[LFB_NUM_GSTATES];
extern ATI_OFF_SCRN atiOffStpls[LFB_NUM_GSTATES];

extern SIFontInfo atiFontInfo[ATI_NUM_FONTS];
extern SIGlyphP atiGlyphs[ATI_NUM_FONTS];

extern int ati_mode_trans[];

extern Mach32DispRegs mach32_reg_vals[];
extern int mach32_num_reg_vals;
extern Mach32WeightRegs mach32_weight_reg_vals[];
extern int mach32_num_weights;

extern Mach32DispRegs *mach32_regP;
extern Mach32WeightRegs *mach32_weightP;

extern SIVisual ati_visuals[];
extern int ati_num_visuals;

void atiFlushGE();
extern u_long atiRev();
extern SIBool atiSetupStpl();
extern SIBool atiSetupTile();

extern int atiClipping;
extern void atiClipOn();
extern void atiClipOff();

/*	MISCELLANEOUS ROUTINES 		*/
/*		MANDATORY		*/

extern SIBool DM_InitFunction();
extern SIBool atiShutdown();
extern SIBool atiVTSave();
extern SIBool atiVTRestore();
extern SIBool atiVideoBlank();
extern SIBool atiInitCache();
extern SIBool atiFlushCache();
extern SIBool atiDownLoadState();
/* extern SIBool atiGetState();		*/
/* extern SIBool atiSelectState();	*/
extern SIBool atiSelectScreen();

/*	SCANLINE AT A TIME ROUTINES	*/
/*		MANDATORY		*/

/* extern SILine atiGetSL();		*/
/* extern SIvoid atiSetSL();		*/
/* extern SIvoid atiFreeSL();		*/

/*	COLORMAP MANAGEMENT ROUTINES	*/
/*		MANDATORY		*/

extern SIBool atiSetCmap();
extern SIBool atiGetCmap();

/*	CURSOR CONTROL ROUTINES		*/
/*		MANDATORY		*/

extern SIBool atiDownLoadCurs();
extern SIBool atiTurnOnCurs();
extern SIBool atiTurnOffCurs();
extern SIBool atiMoveCurs();

/*	HARDWARE SPANS CONTROL		*/
/*		OPTIONAL		*/

extern SIBool atiFillSpans();

/*	HARDWARE BITBLT ROUTINES	*/
/*		OPTIONAL		*/

extern SIBool atiSSbitblt();
/* extern SIBool atiMSbitblt();		*/
/* extern SIBool atiSMbitblt();		*/

/*	HARDWARE POLYGON FILL		*/
/*		OPTIONAL		*/

extern SIvoid atiSetClipRect();
/* extern SIBool atiFillConvexPoly();	*/
/* extern SIBool atiFillGeneralPoly();	*/
extern SIBool atiFillRects();

/*	HARDWARE POINT PLOTTING		*/
/*		OPTIONAL		*/

/* extern SIBool atiPlotPoints();	*/

/*	HARDWARE LINE DRAWING		*/
/*		OPTIONAL		*/

extern SIvoid atiSetClipRect();
extern SIBool atiThinLines();
extern SIBool atiThinSegments();
extern SIBool atiThinRect();

/*	HARDWARE DRAW ARC ROUTINE	*/
/*		OPTIONAL		*/

/* extern SIvoid atiSetClipRect();	*/
/* extern SIBool atiDrawArc();		*/

/*	HARDWARE FILL ARC ROUTINE	*/
/*		OPTIONAL		*/

/* extern SIvoid atiSetClipRect();	*/
/* extern SIBool atiFillArc();		*/

/*	HARDWARE FONT CONTROL		*/
/*		OPTIONAL		*/

extern SIBool atiCheckDLFont();
extern SIBool atiDownLoadFont();
extern SIBool atiFreeFont();
extern SIvoid atiSetClipRect();
extern SIBool atiStplbltFont();

/*	SDD MEMORY CACHING CONTROL	*/
/*		OPTIONAL		*/

/* extern SIBool atiAllocCache();	*/
/* extern SIBool atiFreeCache();	*/
/* extern SIBool atiLockCache();	*/
/* extern SIBool atiUnlockCache();	*/

/*	SDD EXTENSION INITIALIZATION	*/
/*		OPTIONAL		*/

/* extern SIBool atiInitExten();	*/

#endif /* _ATI_H_ */
