#ident	"@(#)ihvkit:display/vga256/v256as.h	1.2"

/*
 * MODULE: v256
 *
 * DESCRIPTION:
 *
 */

#ifndef	V256_ASSY_H
#define V256_ASSY_H

#define	V256HW_SRC_OVERRUN	0x01
#define	V256HW_DST_OVERRUN	0x02

#define	V256HW_X_AXIS		0x01
#define	V256HW_Y_AXIS		0x02

/*
 * The values of the following _BLT_ defines should *not* be changed
 */
#define	V256HW_BLT_NEED_EXTRA_START	0x01
#define V256HW_BLT_NEED_EXTRA_END	0x02
#define	V256HW_BLT_X_INCREASING		0x04

#define V256HW_BLT_SPECIAL_NARROW_CASE	0x08
#ifndef	__ASSEMBLER__
/*
 * declarations for the C compiler
 */

/*
 * Bitblt parameters
 */
extern	int	v256hw_src_p,	v256hw_dst_p;
extern	int	v256hw_tsrc_p,	v256hw_tdst_p;
extern	int	v256hw_src_row_step,	v256hw_dst_row_step;
extern	int	v256hw_src_row_start, 	v256hw_dst_row_start;
extern	int	v256hw_width,	v256hw_twidth;
extern	int	v256hw_height,	v256hw_theight;
extern 	int	v256hw_start_page,	v256hw_end_page;
extern	int	v256hw_fg_pixel,	v256hw_bg_pixel;
extern	int 	v256hw_dst_x, v256hw_dst_y;
extern	int	v256hw_src_x, v256hw_src_y;
   
/*
 * Line draw parameters
 */
extern	int	v256hwbres_do_first_point;
extern 	int	v256hwbres_e, v256hwbres_e1, v256hwbres_e2, 
		v256hwbres_e3;
extern	int	v256hwbres_signdx;
/*
 * Pointers to selectpage routines
 */
extern 	int 	(*v256hw_set_write_page)();
extern 	int 	(*v256hw_set_read_page)();
/*
 * Functions written in assembler
 */
/*
 * Rectangles
 */
extern 	int	v256FFillRect(int dstVirtualAddress, int dstX, 
			      int dstY, int width, int height, int
			      dstRowStep, int fgPixel);
extern	int	v256FOneBitRect(void);

/*
 * Lines
 */
extern	int	v256FVLine(int dstPageVirtualAddress, int dstXStart,
			int dstYStart, int width, int dstStep, 
			int foregroundPixel);
extern	int	v256FHLine(int dstPageVirtualAddress, int dstXStart, 
			int dstYStart, int width, int dstStep, 
			int foregroundPixel);
extern	int	v256FBresLine(int dstPageVirtualAddress, int dstXStart,
			int dstYStart, int nPixels, int dstRowStep,
			int foregroundPixel, int bresE, int bresE1, 
			int bresE3, int signdx, int dstStep);
extern	int	v256_General_FBresLine(int dstPageVirtualAddress, int dstXStart,
			int dstYStart, int nPixels, int dstRowStep,
			int bresE, int bresE1, int bresE3, 
			int signdx, int dstStep,
			unsigned long and_magic,unsigned long xor_magic);

/*
 * BitBlt
 */
extern 	int	v256FSSBitBlt(int srcOffset, int dstOffset, int
			      nlWidth, int height, int srcStep, int
			      dstStep, int startMask, int endMask, int
			      bltDir, int pageBoundary, int pageIncr,
			      int pageStartOffset, int shiftCount);
extern 	int	v256FMSBitBlt(int);
extern	int	v256FSMBitBlt(int);

#endif	/* __ASSEMBLER__ */

#endif	/* V256_ASSY_H */
