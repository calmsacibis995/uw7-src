/*
 *	@(#) nteDefs.h 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S015, 27-Jun-94, hiramc
 *	ready for 864/964 installation
 * S014, 13-Dec-93, staceyc
 * 	get volatile defined for icc
 * S013, 23-Aug-93, staceyc
 * 	flag for cards with broken read image through pix trans ext register
 * S012, 20-Aug-93, staceyc
 * 	fix some ifdef problems
 * S011, 20-Aug-93, staceyc
 * 	basic S3 hardware cursor implementation
 * S010, 16-Aug-93, staceyc
 * 	volatile causes the ODT 3.0 Microsoft compiler to generate incorrect
 *	code (volatile code if you like), so ifdef it for USL only
 * S009, 12-Aug-93, staceyc
 * 	sprinkle some volatiles in the register map
 * S008, 14-Jul-93, staceyc
 * 	mix registers are only 16 bits, this was breaking 24 bit modes
 * S007, 13-Jul-93, staceyc
 * 	mods for 86C80x driver
 * S006, 09-Jul-93, staceyc
 * 	initial hardware cursor work
 * S005, 17-Jun-93, staceyc
 * 	stipple and tile work area dimensions added to screen priv
 * S004, 16-Jun-93, staceyc
 * 	fast text and clipping added
 * S003, 11-Jun-93, staceyc
 * 	color compare register added
 * S002, 09-Jun-93, staceyc
 * 	use correct register name for dest x
 * S001, 08-Jun-93, staceyc
 * 	memory mapped register definitions added
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#ifndef NTEDEFS_H
#define NTEDEFS_H

#if defined(__USLC__) || defined(__ICC)
#define NTE_VOLATILE volatile
#else
#define NTE_VOLATILE
#endif

#if NTE_BITS_PER_PIXEL == 8
typedef unsigned char ntePixel;
typedef unsigned short ntePixelReg;
typedef unsigned short ntePixtrans;
#endif

#if NTE_BITS_PER_PIXEL == 16
typedef unsigned short ntePixel;
typedef unsigned short ntePixelReg;
typedef unsigned short ntePixtrans;
#endif

#if NTE_BITS_PER_PIXEL == 24
typedef unsigned long ntePixel;
typedef unsigned short ntePixelReg;
typedef unsigned long ntePixtrans;
#endif

#if ! NTE_USE_IO_PORTS
typedef struct enhanced_regs_t {
	unsigned char pad00[S3C_Y0];
	NTE_VOLATILE unsigned short cur_y;
	unsigned char pad01[S3C_X0 - S3C_Y0 - 2];
	NTE_VOLATILE unsigned short cur_x;
	unsigned char pad02[S3C_Y1 - S3C_X0 - 2];
	NTE_VOLATILE unsigned short desty_axstp;
	unsigned char pad03[S3C_X1 - S3C_Y1 - 2];
	NTE_VOLATILE unsigned short destx_diastp;
	unsigned char pad04[S3C_ERROR_ACC - S3C_X1 - 2];
	NTE_VOLATILE unsigned short err_term;
	unsigned char pad05[S3C_LX - S3C_ERROR_ACC - 2];
	NTE_VOLATILE unsigned short maj_axis_pcnt;
	unsigned char pad06[S3C_CMD - S3C_LX - 2];
	NTE_VOLATILE unsigned short cmd;
	unsigned char pad07[S3C_COLOR0 - S3C_CMD - 2];
	NTE_VOLATILE ntePixelReg bkgd_color;
	unsigned char pad08[S3C_COLOR1 - S3C_COLOR0 - NTE_REG_SIZE];
	NTE_VOLATILE ntePixelReg frgd_color;
	unsigned char pad09[S3C_PLANE_WE - S3C_COLOR1 - NTE_REG_SIZE];
	NTE_VOLATILE ntePixelReg wrt_mask;
	unsigned char pad10[S3C_PLANE_RE - S3C_PLANE_WE - NTE_REG_SIZE];
	NTE_VOLATILE ntePixelReg rd_mask;
	unsigned char pad11[NTE_COLOR_CMP_REG - S3C_PLANE_RE - NTE_REG_SIZE];
	NTE_VOLATILE ntePixelReg color_cmp;
	unsigned char pad12[S3C_FUNC0 - NTE_COLOR_CMP_REG - NTE_REG_SIZE];
	NTE_VOLATILE unsigned short bkgd_mix;
	unsigned char pad13[S3C_FUNC1 - S3C_FUNC0 - 2];
	NTE_VOLATILE unsigned short frgd_mix;
	unsigned char pad14[S3C_SEC_DECODE - S3C_FUNC1 - 2];
	NTE_VOLATILE unsigned short min_axis_pcnt;
	} enhanced_regs_t;
#endif

typedef struct nteTE8Font_t {
	unsigned long readplane;
	DDXPointRec coords[NFB_TEXT8_SIZE];
	} nteTE8Font_t;

typedef struct ntePrivateData_t {
#if NTE_MAP_LINEAR
	unsigned char *fbPointer;
	unsigned int fbStride;
#endif /* NTE_MAP_LINEAR */
#if ! NTE_USE_IO_PORTS
	enhanced_regs_t *regs;
#endif
	unsigned int dac_shift;
	unsigned int width, height, depth;
	unsigned int osx, osy, oswidth, osheight;
	unsigned int wax, way, wawidth, waheight;
	int te8_font_count;
	nteTE8Font_t *te8_fonts;
	int clip_x, clip_y;
	int hw_cursor_y;
	unsigned short crtc_data, crtc_adr;
	int xhot, yhot;
	CursorRec *cursor;
	unsigned char cursor_fg, cursor_bg;
	int pix_trans_ext_bug;
} ntePrivateData_t;

extern int NTE(ScreenPrivateIndex);
extern unsigned char NTE(RasterOps)[];
extern WindowPtr *WindowTable;

#endif
