/*
 * @(#) m32Defs.h 11.1 97/10/22
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
 * S001, 27-Aug-93, buckm
 *	Get rid of CLIP macros.
 *	Add M32_MIN_DIMENSION, and more linedraw options.
 */
/*
 * m32Defs.h - Mach-32 definitions
 */

/*
 * Some ATI Mach 32 Ports. 
 */

#define M32_ALU_BG_FN		0xB6EE		/* -/w */
#define M32_ALU_FG_FN		0xBAEE		/* -/w */

#define M32_BOUNDS_BOTTOM	0x7EEE		/* r/- */
#define M32_BOUNDS_LEFT		0x72EE		/* r/- */
#define M32_BOUNDS_RIGHT	0x7AEE		/* r/- */
#define M32_BOUNDS_TOP		0x76EE		/* r/- */
#define M32_BRES_COUNT		0x96EE		/* r/w */

#define M32_CLOCK_SEL		0x4AEE		/* -/w */
#define M32_CONFIG_STAT_1	0x12EE		/* r/- */
#define M32_CONFIG_STAT_2	0x16EE		/* r/- */
#define M32_CRT_OFFSET_HI	0x2EEE		/* -/w */
#define M32_CRT_OFFSET_LO	0x2AEE		/* -/w */
#define M32_CRT_PITCH		0x26EE		/* -/w */
#define M32_CURSOR_COLOR	0x1AEE		/* -/w */
#define M32_CURSOR_ENABLE	0x0EEF		/* -/w */
#define M32_CURSOR_OFFSET_HI	0x0EEE		/* -/w */
#define M32_CURSOR_OFFSET_LO	0x0AEE		/* -/w */

#define M32_DEST_CMP_FN		0xEEEE          /* -/w */
#define M32_DEST_X_END          0xAAEE          /* -/w */
#define M32_DEST_X_START        0xA6EE          /* -/w */
#define M32_DEST_Y_END          0xAEEE          /* -/w */
#define M32_DP_CONFIG           0xCEEE          /* -/w */

#define M32_EXT_CURSOR_COLOR_0  0x3AEE          /* -/w */
#define M32_EXT_CURSOR_COLOR_1  0x3EEE          /* -/w */
#define M32_EXT_FIFO_STATUS     0x9AEE          /* r/- */
#define M32_EXT_GE_CONFIG       0x7AEE		/* -/w */
#define M32_EXT_GE_STATUS	0x62EE		/* r/- */
#define M32_EXT_SCISSOR_B	0xE6EE		/* -/w */
#define M32_EXT_SCISSOR_L	0xDAEE		/* -/w */
#define M32_EXT_SCISSOR_R	0xE2EE		/* -/w */
#define M32_EXT_SCISSOR_T	0xDEEE		/* -/w */
#define M32_EXT_SHORT_STROKE	0xC6EE		/* -/w */

#define M32_FIFO_OPT		0x36EE		/* -/w */
#define M32_FIFO_TEST_DATA	0x1AEE		/* r/- */
#define M32_FIFO_TEST_TAG	0x3AEE		/* r/- */

#define M32_GE_OFFSET_HI	0x72EE		/* -/w */
#define M32_GE_OFFSET_LO	0x6EEE		/* -/w */
#define M32_GE_PITCH		0x76EE		/* -/w */

#define M32_HORZ_CURSOR_OFFSET	0x1EEE		/* -/w */
#define M32_HORZ_CURSOR_POSN	0x12EE		/* -/w */
#define M32_HORZ_OVERSCAN	0x62EE		/* -/w */

#define M32_LINEDRAW		0xFEEE		/* -/w */
#define M32_LINEDRAW_INDEX	0x9AEE		/* -/w */
#define M32_LINEDRAW_OPT	0xA2EE		/* r/w */
#define M32_LOCAL_CONTROL	0x32EE		/* -/w */

#define M32_MAX_WAITSTATES	0x6AEE		/* r/w */
#define M32_MEM_BNDRY		0x42EE		/* r/w */
#define M32_MEM_CFG		0x5EEE		/* r/w */
#define M32_MISC_CNTL		0x7EEE		/* -/w */
#define M32_MISC_OPTIONS	0x36EE		/* r/w */

#define M32_OVERSCAN_BLUE_24	0x02EF		/* -/w */
#define M32_OVERSCAN_COLOR_8	0x02EE		/* -/w */
#define M32_OVERSCAN_GREEN_24	0x06EE		/* -/w */
#define M32_OVERSCAN_RED_24	0x06EF		/* -/w */

#define M32_PATT_DATA		0x8EEE		/* r/w */
#define M32_PATT_DATA_INDEX	0x82EE		/* r/w */
#define M32_PATT_INDEX		0xD6EE		/* r/w */
#define M32_PATT_LENGTH		0xD2EE		/* -/w */

#define M32_R_EXT_GE_CONFIG	0x8EEE		/* r/- */
#define M32_R_H_SYNC_STRT	0xB6EE		/* r/- */
#define M32_R_H_SYNC_WID	0xBAEE		/* r/- */
#define M32_R_H_TOTAL_DISP	0xB2EE		/* r/- */
#define M32_R_MISC_CNTL		0x92EE		/* r/- */
#define M32_R_SRC_X		0xDAEE		/* r/- */
#define M32_R_SRC_Y		0xDEEE		/* r/- */
#define M32_R_V_DISP		0xC6EE		/* r/- */
#define M32_R_V_SYNC_STRT	0xCAEE		/* r/- */
#define M32_R_V_SYNC_WID	0xD2EE		/* r/- */
#define M32_R_V_TOTAL		0xC2EE		/* r/- */

#define M32_SCAN_X		0xCAEE		/* -/w */
#define M32_SCRATCH_PAD_0	0x52EE		/* r/w */
#define M32_SCRATCH_PAD_1	0x56EE		/* r/w */
#define M32_SHADOW_CTL		0x46EE		/* -/w */
#define M32_SHADOW_SET		0x5AEE		/* -/w */
#define M32_SRC_X_END		0xBEEE		/* -/w */
#define M32_SRC_X_START		0xB2EE		/* -/w */
#define M32_SRC_Y_DIR		0xC2EE		/* -/w */

#define M32_VERT_CURSOR_OFFSET	0x1EEF		/* -/w */
#define M32_VERT_CURSOR_POSN	0x16EE		/* -/w */
#define M32_VERT_LINE_CNTR	0xCEEE		/* r/- */
#define M32_VERT_OVERSCAN	0x66EE		/* -/w */

/*
 * Some 8514/A compatible ports (Address line 14 not asserted).
 */

#define M32_ADVFUNC_CNTL	0x4AE8		/* -/w */
#define M32_AXSTP		0x8AE8		/* -/w */

#define M32_BKGD_COLOR		0xA2E8		/* -/w */
#define M32_BKGB_MIX		0xB6E8		/* -/w */

#define M32_CMD			0x9AE8		/* -/w */
#define M32_CMP_COLOR		0xB2E8		/* -/w */
#define M32_CUR_X		0x86E8		/* -/w */
#define M32_CUR_Y		0x82E8		/* -/w */

#define M32_DAC_DATA		0x02ED		/* r/w */
#define M32_DAC_MASK		0x02EA		/* r/w */
#define M32_DAC_R_INDEX		0x02EB		/* r/w */
#define M32_DAC_W_INDEX		0x02EC		/* r/w */
#define M32_DEST_X		0x8EE8		/* -/w */
#define M32_DEST_Y		0x8AE8		/* -/w */
#define M32_DIASTP		0x8EE8		/* -/w */
#define M32_DISP_CNTL		0x22E8		/* -/w */
#define M32_DISP_STATUS		0x02E8		/* -/w */

#define M32_ERR_TERM		0x92E8		/* r/w */

#define M32_FRGD_COLOR		0xA6E8		/* -/w */
#define M32_FRGD_MIX		0xBAE8		/* -/w */

#define M32_GE_STAT		0x9AE8		/* r/- */

#define M32_H_DISP		0x06E8		/* -/w */
#define M32_H_SYNC_STRT		0x0AE8		/* -/w */
#define M32_H_SYNC_WID		0x0EE8		/* -/w */
#define M32_H_TOTAL		0x02E8		/* -/w */

#define M32_MAJ_AXIS_PCNT	0x96E8		/* -/w */
#define M32_MEM_CNTL		0xBEE8		/* -/w [5] */
#define M32_MIN_AXIS_PCNT	0xBEE8		/* -/w [0] */

#define M32_PATTERN_H		0xBEE8		/* -/w [9] */
#define M32_PATTERN_L		0xBEE8		/* -/w [8] */
#define M32_PIX_TRANS		0xE2E8		/* r/w */
#define M32_PIXEL_CNTL		0xBEE8		/* -/w [A] */

#define M32_RD_MASK		0xAEE8		/* -/w */
#define M32_SCISSOR_B		0xBEE8		/* -/w [3] */
#define M32_SCISSOR_L		0xBEE8		/* -/w [2] */
#define M32_SCISSOR_R		0xBEE8		/* -/w [4] */
#define M32_SCISSOR_T		0xBEE8		/* -/w [1] */
#define M32_SHORT_STROKE	0x9EE8		/* -/w */
#define M32_SRC_X		0x8EE8		/* -/w */
#define M32_SRC_Y		0x8AE8		/* -/w */
#define M32_SUBSYS_CNTL		0x42E8		/* -/w */
#define M32_SUBSYS_STATUS	0x42E8		/* r/- */

#define M32_V_DISP		0x16E8		/* -/w */
#define M32_V_SYNC_START	0x1AE8		/* -/w */
#define M32_V_SYNC_WID		0x1EE8		/* -/w */
#define M32_V_TOTAL		0x12E8		/* -/w */

#define M32_WRT_MASK		0xAAE8		/* -/w */


/*
 * DP_CONFIG useful values
 */
#define M32_DP_COPY		0x6011	/* copy blt */
#define M32_DP_FILL		0x2011	/* fill blt */
#define M32_DP_FILLPATT		0xA011	/* fill from pattern regs */
#define M32_DP_EXPAND		0x2071	/* expand blt */
#define M32_DP_EXPPATT		0x2031	/* expand from pattern regs */
#define M32_DP_WWHOST		0x5211	/* write words from host */
#define M32_DP_WBHOST		0x4011	/* write bytes from host */
#define M32_DP_RWHOST		0x5210	/* read  words  to  host */
#define M32_DP_RBHOST		0x4010	/* read  bytes  to  host */
#define M32_DP_EXPWHOST		0x3251	/* expand words from host */
#define M32_DP_EXPBHOST		0x2051	/* expand bytes from host */


/*
 * LINEDRAW_OPT useful values
 */
#define	M32_LD_PTPT		0x0208	/* point to point line */
#define	M32_LD_PTPTNL		0x020C	/* point to point line, not last */
#define	M32_LD_DPTPT		0x0608	/* dashed pt to pt line */
#define	M32_LD_DPTPTNL		0x060C	/* dashed pt to pt line, not last */
#define	M32_LD_DEGREE		0x000C	/* degree line (add degree bits) */
#define	M32_LD_HORZ		0x000C	/* horizontal line (0 degrees) */
#define	M32_LD_DOT		0x000C	/* one point (horizontal line) */
#define	M32_LD_BRES		0x0004	/* bresenham line (add octant bits) */


/*
 * CMD useful values
 */
#define	M32_CMD_HLINE		0x201D	/* horizontal line (0 degrees) */
#define	M32_CMD_VLINE		0x20DD	/* vertical   line (270 degrees) */
#define	M32_CMD_BLIT		0xC0B1	/* copy screen area */


/*
 * Coprocessor status
 */

#define M32_CLEAR_QUEUE(n) while (inw(M32_EXT_FIFO_STATUS) & (0x10000 >> (n)))

#if defined(DEBUG)

#define M32_IDLE() \
{ \
	long tmo = 0x100000; \
	while ((inw(M32_GE_STAT) & 0x200) && --tmo) \
		; \
	if (tmo <= 0) { \
		ErrorF("IDLE timeout in %s\n", dbg_name); \
		ErrorF("  fifo=%x gestat=%x systat=%x\n", \
			inw(M32_EXT_FIFO_STATUS), \
			inw(M32_GE_STAT), \
			inw(M32_SUBSYS_STATUS)); \
	} \
	outw(M32_SUBSYS_CNTL, 0x04); \
}

#define M32_DATA_READY() \
{ \
	long tmo = 0x1000000; \
	while ((!(inw(M32_GE_STAT) & 0x0100)) && --tmo) \
		; \
	if (tmo <= 0) \
		ErrorF("DATA timeout in %s\n", dbg_name); \
}

#define	M32_CHECK_DATA() \
{ \
	if (inw(M32_GE_STAT) & 0x0100) \
		ErrorF("Left %s with DATA ready\n", dbg_name); \
}

#define	M32_CHECK_IO() \
{ \
	if (inw(M32_SUBSYS_STATUS) & 0x04) { \
		ErrorF("Illegal I/O in %s\n", dbg_name); \
		outw(M32_SUBSYS_CNTL, 0x04); \
	} \
}

#define M32_DBG_NAME(name) \
	char *dbg_name = name

#else	/* ! DEBUG */

#define M32_IDLE() \
{ \
	long tmo = 0x100000; \
	while ((inw(M32_GE_STAT) & 0x200) && --tmo) \
		; \
	outw(M32_SUBSYS_CNTL, 0x04); \
}

#define M32_DATA_READY() \
{ \
	long tmo = 0x1000000; \
	while ((!(inw(M32_GE_STAT) & 0x0100)) && --tmo) \
		; \
}

#define	M32_CHECK_DATA()

#define	M32_CHECK_IO()

#define M32_DBG_NAME(name)

#endif	/* DEBUG */


/*
 * Hardware cursor defs
 */

#define	M32_SPRITE_WIDTH	64
#define	M32_SPRITE_HEIGHT	64
#define	M32_SPRITE_DEPTH	2

#define	M32_SPRITE_STRIDE	((M32_SPRITE_WIDTH * M32_SPRITE_DEPTH) / 8)
#define	M32_SPRITE_BYTES	(M32_SPRITE_STRIDE * M32_SPRITE_HEIGHT)


/*
 * TE8 downloaded font defs
 */

#define	M32_TE8_FONTS		4	/* max no of downloaded fonts */
#define	M32_TE8_WIDTH		10	/* max glyph width */
#define	M32_TE8_HEIGHT		20	/* max glyph height */
#define	M32_TE8_GLYPHS		256	/* glyphs per te8 font */
#define	M32_TE8_PLANES		4	/* planes per font */

#define	M32_TE8_GLYPH_LOCS	(M32_TE8_GLYPHS / M32_TE8_PLANES)
#define	M32_TE8_GLYPH_BITS	(M32_TE8_WIDTH * M32_TE8_HEIGHT)
#define	M32_TE8_FONT_BITS	(M32_TE8_GLYPH_BITS * M32_TE8_GLYPHS)
#define	M32_TE8_TOTAL_BITS	(M32_TE8_FONT_BITS * M32_TE8_FONTS)
#define	M32_TE8_TOTAL_BYTES	(M32_TE8_TOTAL_BITS / 8)


/*
 * Misc stuff
 */

#define	M32_MIN_DIMENSION	-512		/* min x,y coordinate */
#define	M32_MAX_DIMENSION	1536		/* max x,y coordinate */

#define	M32_MAX_PATTERN		32		/* max hw pattern length */

extern unsigned short m32RasterOp[16];

