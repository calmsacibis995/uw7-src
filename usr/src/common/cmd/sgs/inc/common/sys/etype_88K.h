/*
	(c) Copyright, 1989 MOTOROLA, INC. 
	    All Rights Reserved.

	"This file contains copyrighted material.  Use of this file 
	is restricted by the provisions of Motorola's Software
	License Agreement."
*/

#ident	"@(#)sgs-inc:common/sys/etype_88K.h	1.2"

#ifndef _SYS_ELF_88K_H
#define _SYS_ELF_88K_H

#include <sys/elftypes.h>

/* .tdesc section structures */
struct tdesc_info_piece {
	Elf32_Word	tip_protocol;	/* protocol */
	Elf32_Addr	tip_end_addr;	/* info piece past end address */
	};

#define INFO_PIECE_PROTOCOL_SIMPLE  1	/* simple info piece protocol */
#define	INFO_PIECE_PROTOCOL_ARRAY   2	/* arrayed info piece protocol */

struct tdesc_piece {
	Elf32_Word	tp_word_0;	/* zero, info len and alignment */
	Elf32_Word	tp_protocol;	/* protocol */
	Elf32_Word	tp_chunk_start;	/* start of text chunk */
	Elf32_Word	tp_chunk_end;	/* past end of text chunk */
	};

#define	TDESC_PIECE_PROTOCOL_ABS  1	/* info piece absolute addresses */
#define	TDESC_PIECE_PROTOCOL_REL  2	/* info piece file relative addresses */

#define GET_TDESC_PIECE_ZERO(x) (((x)->tp_word_0>>24)&0xff)
#define GET_TDESC_PIECE_INFO_LEN(x) (((x)->tp_word_0>>2)&0x3fffff)
#define GET_TDESC_PIECE_ALIGN(x) ((x)->tp_word_0&0x3)

struct tdesc_info {
	Elf32_Word	ti_word_0;	/* variant, reg mask, desc, frame reg */
	Elf32_Word	ti_frame_addr_offset; /* frame address offset */
	Elf32_Word	ti_return_addr_info; /* return address information */
	Elf32_Word	ti_reg_save_offset; /* register save area offset */
	};

#define INFO_VARIANT_1		1	/* first defined ABI variant */

#define GET_TDESC_INFO_VARIANT(x) (((x)->ti_word_0>>24)&0xff)
#define GET_TDESC_INFO_REGMASK(x) (((x)->ti_word_0>>7)&0x1ffff)
#define GET_TDESC_INFO_RET_ADDR_DESC(x) (((x)->ti_word_0>>5)&0x1)
#define GET_TDESC_INFO_FRAME_REG(x) ((x)->ti_word_0&0x1f)

struct tdesc_map2_array {
	Elf32_Addr	tm2_tdesc;	/* pointer to tdesc */
	Elf32_Addr	tm2_addr_base;	/* addressing base */
	};

struct debinfaddr_word {
	Elf32_Addr	dw_debinfaddr;	/* pointer to _debug_info structure */
	};

struct debinfo {
	Elf32_Word	deb_protocol;	/* protocol indicator */
	Elf32_Addr	deb_tdesc;	/* pointer to tdesc entity */
	Elf32_Word	deb_textcnt;	/* count of text words */
	Elf32_Addr	deb_text;	/* pointer to text words */
	Elf32_Word	deb_datacnt;	/* count of data words */
	Elf32_Addr	deb_data;	/* pointer to data words */
	};

#define DEB_PROTOCOL_1		1	/* first defined ABI debug protocol */

/* debug info address segment p_type */
#define	PT_88K_DEBINFADDR	0x70000001

/* valid dynamic tag values */
#define DT_88K_ADDRBASE 0x70000001
#define DT_88K_PLTSTART 0x70000002
#define DT_88K_PLTEND   0x70000003
#define DT_88K_TDESC    0x70000004

/* maximum M88000 pagesize */
#define ELF_88K_MAXPGSZ		0x10000		/* 64K */

/* New Relocation Types as defined in the Feb. 27, 1990 Draft ABI. */
#define R_88K_NONE		0
#define R_88K_COPY		1
#define R_88K_GOTP_ENT		2
#define R_88K_8			4
#define R_88K_8S		5
#define R_88K_16S		7
#define R_88K_DISP16		8
#define R_88K_DISP26		10
#define R_88K_PLT_DISP26	14
#define R_88K_BBASED_32		16
#define R_88K_BBASED_32UA	17
#define R_88K_BBASED_16H	18
#define R_88K_BBASED_16L	19
#define R_88K_ABDIFF_32		24
#define R_88K_ABDIFF_32UA	25
#define R_88K_ABDIFF_16H	26
#define R_88K_ABDIFF_16L	27
#define R_88K_ABDIFF_16		28
#define R_88K_32		32
#define R_88K_32UA		33
#define R_88K_16H		34
#define R_88K_16L		35
#define R_88K_16		36
#define R_88K_GOT_32		40
#define R_88K_GOT_32UA		41
#define R_88K_GOT_16H		42
#define R_88K_GOT_16L		43
#define R_88K_GOT_16		44
#define R_88K_GOTP_32		48
#define R_88K_GOTP_32UA		49
#define R_88K_GOTP_16H		50
#define R_88K_GOTP_16L		51
#define R_88K_GOTP_16		52
#define R_88K_PLT_32		56
#define R_88K_PLT_32UA		57
#define R_88K_PLT_16H		58
#define R_88K_PLT_16L		59
#define R_88K_PLT_16		60
#define R_88K_ABREL_32		64
#define R_88K_ABREL_32UA	65
#define R_88K_ABREL_16H		66
#define R_88K_ABREL_16L		67
#define R_88K_ABREL_16		68
#define R_88K_GOT_ABREL_32	72
#define R_88K_GOT_ABREL_32UA	73
#define R_88K_GOT_ABREL_16H	74
#define R_88K_GOT_ABREL_16L	75
#define R_88K_GOT_ABREL_16	76
#define R_88K_GOTP_ABREL_32	80
#define R_88K_GOTP_ABREL_32UA	81
#define R_88K_GOTP_ABREL_16H	82
#define R_88K_GOTP_ABREL_16L	83
#define R_88K_GOTP_ABREL_16	84
#define R_88K_PLT_ABREL_32	88
#define R_88K_PLT_ABREL_32UA	89
#define R_88K_PLT_ABREL_16H	90
#define R_88K_PLT_ABREL_16L	91
#define R_88K_PLT_ABREL_16	92
#define R_88K_SREL_32		96
#define R_88K_SREL_32UA		97
#define R_88K_SREL_16H		98
#define R_88K_SREL_16L		99

#define R_88K_NUM			100		/* must be > last */

#endif
