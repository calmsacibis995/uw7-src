#ident	"@(#)ihvkit:display/lfb256/devices/jaws/jawsregs.h	1.1"

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

#ifndef _JAWSREGS_H_
#define _JAWSREGS_H_

/*
 *
 * The Jaws II Frame Buffer (FB) from Dell is based on the INMOS G332
 * Color Video Controller.  This code is designed to work with the
 * Jaws II board, based on the Jaws II Specification rev 0.21 (31 Mar
 * 1992).  Henceforth we will use the term Jaws to refer to the Jaws
 * II board, and will specifically note when something refers to the
 * Jaws 1 board.
 *
 * The Jaws board has 4M dedicated to the FB, mapped as follows:
 *
 *	Offset			Assignment
 *	------			----------
 *	000000H	- 1FFFFFH	Frame Buffer
 *	200000H - 2007FFH	G332 registers
 *	200800H - 3FFFFFH	Reserved
 *
 *	Note: The Jaws spec (0.21) notes that the above values may
 *	change in the future.  Specifically, rev 0.2 of the spec noted
 *	that DELL expected to move the G332 registers to 800000H.
 *
 * The 4M can be mapped in at 512M, 640M, 768M, or disabled.  This is
 * software settable.
 *
 * The Jaws board supports VGA passthrough (default) and a variety of
 * pixel formats.  Available formats are 15 (5R 5G 5B) & 16 (6R 6G 4B)
 * DirectColor and 8, 4, 2, & 1 PseudoColor.  See the G332
 * documentation for more details.
 *
 * As an added feature, Jaws adds a 12bpp mode (4R 4G 4B).  This mode
 * should only be used when the board is in 16bpp mode.
 *
 */


/* 
 * The Dell JAWS board is also the CPU board.  It will only plug in to
 * the motherboard in slot 6.  The CPU component of the board
 * identifies itself as part of the motherboard (slot 0), but the FB
 * portion of the board identifies itself as a plug in card.  Since it
 * can only plug in to slot 6, we will only look for it there.
 *
 */

/*
 *
 * The Jaws ID is stored in a long word port (4 consecutive byte
 * ports).  If we don't match, we're probably not on a Jaws II board.
 * This will need to be generalized to supported multiple board IDs.
 * (e.g.  When the 66DX/2 version of the board is available, it may
 * have a different ID so that software that depends on timing can
 * detect which board they are on.)
 *
 */

#define JAWS_ID_PORT1	0x6c80
#define JAWS_ID_VAL1	0x10
#define JAWS_ID_PORT2	0x6c81
#define JAWS_ID_VAL2	0xac
#define JAWS_ID_PORT3	0x6c82
#define JAWS_ID_VAL3	0x60
#define JAWS_ID_PORT4	0x6c83
#define JAWS_ID_VAL4	0x01

			
/*
 *
 *  The Jaws Control Port determines what is enabled on the machine.
 *
 *    Bit 0-1: FB address on system
 *	  00=disable, 01=512M, 10=640M, 11=768M, def=00
 *    Bit 2: FB Cachability
 *	  0=no, 1=yes, def=0
 *    Bit 3: Video retrace interrupt enable
 *	  0=no, 1=yes, def=0
 *	  Interrupt level set by board jumper.
 *    Bit 4: Jaws reset (write only)
 *	  0=run, 1=reset, def=0
 *    Bit 5: vertical retrace period (read only)
 *	  1 during retrace
 *    Bit 6: VGA passthrough
 *	  0=VGA, 1=Jaws, def=0
 *    Bit 7: 12bpp enable
 *	  0=normal, 1=12bpp def=0
 *
 *  Bits 6 and 7 are inverted on read, so they will appear to have a
 *  default of 1.  They are not inverted on write.
 *
 */

#define JAWS_CNTL_PORT		      0x6c88
#define JAWS_CNTL_MMAP_SELECT_MASK	0x03
#define JAWS_CNTL_MMAP_SELECT_NONE	0x00
#define JAWS_CNTL_MMAP_SELECT_512M	0x01
#define JAWS_CNTL_MMAP_SELECT_640M	0x02
#define JAWS_CNTL_MMAP_SELECT_768M	0x03
#define JAWS_CNTL_FB_CACHABLE		0x04
#define JAWS_CNTL_VIDEO_INT_ENABLE	0x08
#define JAWS_CNTL_RESET			0x10
#define JAWS_CNTL_VERT_RETRACE		0x20
#define JAWS_CNTL_JAWS_ENABLE		0x40
#define JAWS_CNTL_12BPP_ENABLE		0x80

#define JAWS_CNTL_FLIPPED_BITS	(JAWS_CNTL_JAWS_ENABLE | \
				 JAWS_CNTL_12BPP_ENABLE )

#define JAWS_CNTL_READABLE_BITS (JAWS_CNTL_MMAP_SELECT_MASK | \
				 JAWS_CNTL_FB_CACHABLE | \
				 JAWS_CNTL_VIDEO_INT_ENABLE | \
				 JAWS_CNTL_JAWS_ENABLE | \
				 JAWS_CNTL_12BPP_ENABLE )

/*
 *
 * All register addresses, including the ones in g332_regs et al.,
 * should be ORed with this.  This address is an offset from the
 * beginning of the FB.
 *
 */

#define JAWS_REG_BASE	0x200000
#define JAWS_REG_LEN	  0x1000

/*
 *
 * G332 Registers
 *
 * The G332 registers are 24 bits wide.  On the Jaws board, they are
 * memory mapped and word aligned.  The address is the register number
 * times 4, offset from the register base.  The names here are from
 * the G332 documentation.
 *
 */

#define JAWS_REG_BOOT_LOCATION 	    0x00
#define JAWS_REG_HALF_SYNC	    0x21
#define JAWS_REG_BACK_PORCH	    0x22
#define JAWS_REG_DISPLAY	    0x23
#define JAWS_REG_SHORT_DISPLAY	    0x24
#define JAWS_REG_BROAD_PULSE	    0x25
#define JAWS_REG_V_SYNC		    0x26
#define JAWS_REG_V_PRE_EQUALISE	    0x27
#define JAWS_REG_V_POST_EQUALISE    0x28
#define JAWS_REG_V_BLANK	    0x29
#define JAWS_REG_V_DISPLAY	    0x2a
#define JAWS_REG_LINE_TIME	    0x2b
#define JAWS_REG_LINE_START	    0x2c
#define JAWS_REG_MEM_INIT	    0x2d
#define JAWS_REG_TRANSFER_DELAY	    0x2e

/* Pseudo-color pixel mask */
#define JAWS_REG_PIXEL_MASK	    0x40

/* Control registers */
#define JAWS_REG_CONTROL_A	    0x60
#define JAWS_REG_CONTROL_B	    0x70

#define JAWS_INIT_CONTROL_A	    0xb47370

/*
 * Top of screen pointer.  TOS1 is used when the VTG is disabled
 * (startup).  TOS2 is used when the VTG is enabled.
 */
#define JAWS_REG_TOS1		    JAWS_REG_LINE_START
#define JAWS_REG_TOS2		    0x80

/* Cursor palette start.  Palette is 24 bit words. */
#define JAWS_CURSOR_PALETTE	    0xa1

/* Cursor position X & Y */
#define JAWS_CURSOR_POSITION	    0xc7

/* Color Palette */
#define JAWS_COLOR_PALETTE	    0x100

/* Cursor */
#define JAWS_CURSOR		    0x200

/* Cursor is 64x64. */
#define JAWS_CURSOR_WIDTH  64
#define JAWS_CURSOR_HEIGHT 64

/*
 * G332 registers
 *
 * These registers are used to initialize the G332.  The names are
 * from the G332 spec.
 *
 * stride is not a register, but is the amount of memory from the
 * beginning of one scan line to the next.  The G332 does not leave
 * any memory gaps between scan lines.
 */

typedef struct {
    long
	boot_location,
	half_sync,
	back_porch,
	display,
	short_display,
	broad_pulse,
	v_sync,
	v_pre_equalise,
	v_post_equalise,
	v_blank,
	v_display,
	line_time;
    long
	line_start,
	mem_init,
	transfer_delay,
	control_A,
	stride;
} G332Regs;

/*
 * Jaws register struct
 *
 * All the information needed to initialize the Jaws board to a
 * given resolution and pixel size.
 */

typedef struct {
    int width, height, freq;
    G332Regs regs;
} JawsRegs;

#endif /* _JAWSREGS_H_ */
