#ident	"@(#)ihvkit:display/vga256/vgaregs.h	1.1"

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

/* EGA / VGA specific stuff */
	
#ifndef	VGAREGS_H
#define	VGAREGS_H 1

/* display-related constants */

#define		EGASCREEN_ADDR	0xa0000
#define		EGASCREENSIZE	(32*1024)

#define		EGA_MAXDEPTH	4	/* maximum depth (# planes)         */
					/* of a drawable destined for EGA   */
#define		EGA_MAXPLANE	(EGA_MAXDEPTH - 1)
#define         EGA_COLORMASK   0xf
#define		NUMEGACOLORS	16

/* control-registers */

#define	TS_INDEX_REG	0x3c4
#define	TS_DATA_REG	0x3c5
#define	MAPMASK_INDEX	2

#define	GC_INDEX_REG	0x3ce
#define	GC_DATA_REG	0x3cf
#define	SRVAL_INDEX	0
#define	SRMASK_INDEX	1
#define ROT_FN_INDEX    3
#define	READMAP_INDEX	4
#define WRITEMODE_INDEX 5
#define BITMASK_INDEX   8

/* writemode values */

#define EGAM_WALL       0
#define EGAM_WLATCH     1
#define EGAM_WCOLOR     2

/* bitmask values */

#define ALL_BITS_BITMASK 0xff
#define STANDARD_BITMASK ALL_BITS_BITMASK

/* sdfunct (read-modify-write function) values */

#define EGAF_REPLACE    0
#define EGAF_AND        (1 << 3)
#define EGAF_OR         (2 << 3)
#define EGAF_XOR        (3 << 3)

#ifndef TWOBYTE_OUTS
#define	TS_SET(ind, x)		outb(TS_INDEX_REG, (ind)), \
				outb(TS_DATA_REG,  (x))
#define	GC_SET(ind, x)		outb(GC_INDEX_REG, (ind)), \
				outb(GC_DATA_REG,  (x))
#else
#define	TS_SET(ind, x)		outw(TS_INDEX_REG, ((x) << 8) | (ind))
#define	GC_SET(ind, x)		outw(GC_INDEX_REG, ((x) << 8) | (ind))
#endif

#define	SET_MAPMASK(m)		TS_SET(MAPMASK_INDEX, (m))
#define	SET_SRMASK(m)		GC_SET(SRMASK_INDEX, (m))
#define	SET_SRVAL(x)		GC_SET(SRVAL_INDEX, (x))
#define SET_WRITEMODE(m)        GC_SET(WRITEMODE_INDEX, (m))
#define SET_BITMASK(m)          GC_SET(BITMASK_INDEX, (m))
#define SET_SDFUNCT(f)          GC_SET(ROT_FN_INDEX, (f))

/*
 * Some of the following have been defined earlier in this file
 * with a slightly different names.
 */
#define MAP_MASK  0x02
#define READ_MASK 0x04
#define GR_MODE   0x05

#define IO_ADDR_SEL     0x1     /* IO address select in misc out reg */

#define MISC_OUT        0x3c2   /* miscellaneous output register */
#define MISC_OUT_READ   0x3cc   /* read misc output register on VGA */
#define GR_MISC         0x06    /* Graphics Controller miscellaneous reg */

#define PEL_READ        0x3c7   /* Pel address to start reading RGB data */
#define PEL_WRITE       0x3c8   /* Pel address to start writing RGB data */
#define PEL_DATA        0x3c9   /* Register to read or write RGB data */
#define NPEL_REGS       256     /* entries in Video DAC color table */

/*
 * for VGA, we can also read a register
 */
#define in_reg(riptr, index, data)      outb((riptr)->ri_address, (index)); \
					  data = inb((riptr)->ri_data);

#endif	/* VGAREGS_H */
