/*
 *	@(#) nteMacros.h 11.1 97/10/22
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
 * S009, 19-Jul-93, staceyc
 * 	added i/o in byte for 8 bpp screen blanking
 * S008, 14-Jul-93, staceyc
 * 	all manner of fixes to get 24 bit working, including fixing incorrect
 *	use of mix registers as 32 bits
 * S007, 13-Jul-93, staceyc
 * 	macros to create 86C80x driver which uses regular i/o ports instead
 *	of the memory mapped ports used by the 86C928 driver
 * S006, 09-Jul-93, staceyc
 * 	all sorts of macros to get around chip bugs with 24 bit modes
 * S005, 16-Jun-93, staceyc
 * 	data ready macro added
 * S004, 16-Jun-93, staceyc
 * 	macros for clipping added
 * S003, 11-Jun-93, staceyc
 * 	macros for mono images
 * S002, 09-Jun-93, staceyc
 * 	more macros for solid fill, bres line, points, and blit
 * S001, 08-Jun-93, staceyc
 * 	initial command macros created
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#ifndef NTEMACROS_H
#define NTEMACROS_H

/*
 * Here is how to concatenate tokens when one of the tokens is a
 * preprocessor define.  See K&R II or the comp.lang.c FAQ to find
 * out why this has to be done this way.  This is ANSI only code.
 */
#define NTE_TYPE_0(head,body) head##body
#define NTE_TYPE_1(head,body) NTE_TYPE_0(head,body)
#define NTE(body) NTE_TYPE_1(NTEHEAD,body)

#define NTE_OUTB(port, val) outb(port, val)
#define NTE_OUTW(port, val) outw(port, val)
#define NTE_INB(port) inb(port)
#define NTE_INW(port) inw(port)

#if NTE_BITS_PER_PIXEL == 8 || NTE_BITS_PER_PIXEL == 16
#define NTE_CLEAR_QUEUE(n) \
{ \
	register unsigned short mask = 0x100 >> (n); \
	while (mask & NTE_INW(NTE_GP_STAT)); \
}
#define NTE_CLEAR_QUEUE24(n)
#endif

#if NTE_BITS_PER_PIXEL == 24
#define NTE_CLEAR_QUEUE(n)
#define NTE_CLEAR_QUEUE24(n) \
{ \
	register unsigned short mask = 0x100 >> (n); \
	while (mask & NTE_INW(NTE_GP_STAT)); \
}
#endif

#define NTE_WAIT_FOR_IDLE() \
{ \
	while (NTE_HDW_BSY & NTE_INW(NTE_GP_STAT)); \
}
#define NTE_WAIT_FOR_DATA() \
{ \
	while (! (NTE_RDT_AVA & NTE_INW(NTE_GP_STAT))); \
}

#define NTE_PRIVATE_DATA(pScreen) \
    ((ntePrivateData_t *)((pScreen)->devPrivates[NTE(ScreenPrivateIndex)].ptr))

#define NTE_VALIDATE_SCREEN(ntePriv, pScreen, screen_no)

#if ! NTE_USE_IO_PORTS

#define NTE_BEGIN(regs) \
{ \
	register enhanced_regs_t *xx_regs = (regs);

#define NTE_PIX_CNTL(val) xx_regs->min_axis_pcnt = NTE_PIX_CNTL_INDEX | (val)
#define NTE_CURX(x) xx_regs->cur_x = (x)
#define NTE_CURY(y) xx_regs->cur_y = (y)
#define NTE_MAJ_AXIS_PCNT(lx) xx_regs->maj_axis_pcnt = (lx)
#define NTE_MIN_AXIS_PCNT(ly) xx_regs->min_axis_pcnt = (ly)
#define NTE_CMD(command) xx_regs->cmd = (command)
#define NTE_DESTX_DIASTP(x) xx_regs->destx_diastp = (x)
#define NTE_DESTY_AXSTP(y) xx_regs->desty_axstp = (y)
#define NTE_ERR_TERM(e) xx_regs->err_term = (e)
#define NTE_SCISSORS_T(y) xx_regs->min_axis_pcnt = NTE_SCISSORS_T_INDEX | (y)
#define NTE_SCISSORS_L(x) xx_regs->min_axis_pcnt = NTE_SCISSORS_L_INDEX | (x)
#define NTE_SCISSORS_B(y) xx_regs->min_axis_pcnt = NTE_SCISSORS_B_INDEX | (y)
#define NTE_SCISSORS_R(x) xx_regs->min_axis_pcnt = NTE_SCISSORS_R_INDEX | (x)
#define NTE_MULT_MISC(n) xx_regs->min_axis_pcnt = NTE_MULT_MISC_INDEX | (n)
#define NTE_FRGD_MIX(clrsrc, mixtype) xx_regs->frgd_mix = (clrsrc) | (mixtype)
#define NTE_BKGD_MIX(clrsrc, mixtype) xx_regs->bkgd_mix = (clrsrc) | (mixtype)

#define NTE_PIX_TRANS(reg, n) (reg) = (n)
#define NTE_PIX_TRANS_IN(n, reg) (n) = (reg)

#if NTE_BITS_PER_PIXEL == 8 || NTE_BITS_PER_PIXEL == 16
#define NTE_FRGD_COLOR(fg) xx_regs->frgd_color = (fg)
#define NTE_BKGD_COLOR(bg) xx_regs->bkgd_color = (bg)
#define NTE_RD_MASK(mask) xx_regs->rd_mask = (mask)
#define NTE_WRT_MASK(mask) xx_regs->wrt_mask = (mask)
#define NTE_COLOR_CMP(color) xx_regs->color_cmp = (color)
#endif

#if NTE_BITS_PER_PIXEL == 24
#define NTE_24BIT(reg, val) \
{ \
	unsigned long xx_val = (val); \
	(reg) = xx_val; \
	xx_val >>= 16; \
	(reg) = xx_val; \
}
#define NTE_FRGD_COLOR(fg) NTE_24BIT(xx_regs->frgd_color, fg)
#define NTE_BKGD_COLOR(bg) NTE_24BIT(xx_regs->bkgd_color, bg)
#define NTE_RD_MASK(mask) NTE_24BIT(xx_regs->rd_mask, mask)
#define NTE_WRT_MASK(mask) NTE_24BIT(xx_regs->wrt_mask, mask)
#define NTE_COLOR_CMP(color) NTE_24BIT(xx_regs->color_cmp, color)
#endif

#define NTE_END() \
}

#endif

#if NTE_USE_IO_PORTS

#define NTE_BEGIN(regs)

#define NTE_PIX_CNTL(val) NTE_OUTW(S3C_SEC_DECODE, NTE_PIX_CNTL_INDEX | (val))
#define NTE_CURX(x) NTE_OUTW(S3C_X0, x)
#define NTE_CURY(y) NTE_OUTW(S3C_Y0, y)
#define NTE_MAJ_AXIS_PCNT(lx) NTE_OUTW(S3C_LX, lx)
#define NTE_MIN_AXIS_PCNT(ly) NTE_OUTW(S3C_SEC_DECODE, ly)
#define NTE_CMD(command) NTE_OUTW(S3C_CMD, command)
#define NTE_DESTX_DIASTP(x) NTE_OUTW(S3C_X1, x)
#define NTE_DESTY_AXSTP(y) NTE_OUTW(S3C_Y1, y)
#define NTE_ERR_TERM(e) NTE_OUTW(S3C_ERROR_ACC, e)
#define NTE_SCISSORS_T(y) NTE_OUTW(S3C_SEC_DECODE, NTE_SCISSORS_T_INDEX | (y))
#define NTE_SCISSORS_L(x) NTE_OUTW(S3C_SEC_DECODE, NTE_SCISSORS_L_INDEX | (x))
#define NTE_SCISSORS_B(y) NTE_OUTW(S3C_SEC_DECODE, NTE_SCISSORS_B_INDEX | (y))
#define NTE_SCISSORS_R(x) NTE_OUTW(S3C_SEC_DECODE, NTE_SCISSORS_R_INDEX | (x))
#define NTE_MULT_MISC(n) NTE_OUTW(S3C_SEC_DECODE, NTE_MULT_MISC_INDEX | (n))
#define NTE_FRGD_MIX(clrsrc, mixtype) NTE_OUTW(S3C_FUNC1, (clrsrc) | (mixtype))
#define NTE_BKGD_MIX(clrsrc, mixtype) NTE_OUTW(S3C_FUNC0, (clrsrc) | (mixtype))

#define NTE_PIX_TRANS(reg, n) NTE_OUTW(S3C_VARDATA, n)
#define NTE_PIX_TRANS_IN(n, reg) n = NTE_INW(S3C_VARDATA)

#if NTE_BITS_PER_PIXEL == 8 || NTE_BITS_PER_PIXEL == 16
#define NTE_FRGD_COLOR(fg) NTE_OUTW(S3C_COLOR1, fg)
#define NTE_BKGD_COLOR(bg) NTE_OUTW(S3C_COLOR0, bg)
#define NTE_RD_MASK(mask) NTE_OUTW(S3C_PLANE_RE, mask)
#define NTE_WRT_MASK(mask) NTE_OUTW(S3C_PLANE_WE, mask)
#define NTE_COLOR_CMP(color) NTE_OUTW(NTE_COLOR_CMP_REG, color)
#endif

#if NTE_BITS_PER_PIXEL == 24
#define NTE_24BIT(reg, val) \
{ \
	unsigned long xx_val = (val); \
	NTE_OUTW(reg, xx_val); \
	xx_val >>= 16; \
	NTE_OUTW(reg, xx_val); \
}
#define NTE_FRGD_COLOR(fg) NTE_24BIT(S3C_COLOR1, fg)
#define NTE_BKGD_COLOR(bg) NTE_24BIT(S3C_COLOR0, bg)
#define NTE_RD_MASK(mask) NTE_24BIT(S3C_PLANE_RE, mask)
#define NTE_WRT_MASK(mask) NTE_24BIT(S3C_PLANE_WE, mask)
#define NTE_COLOR_CMP(color) NTE_24BIT(NTE_COLOR_CMP_REG, color)
#endif

#define NTE_END()

#endif

#endif
