/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/crom/crom.h	1.3.1.5"

/********************************************************
 * Copyright 1995, COMPAQ Computer Corporation
 ********************************************************
 *
 * Title   : $RCSfile$
 *
 * Version : $Revision$
 *
 * Date    : $Date$
 *
 * Author  : $Author$
 *
 ********************************************************
 *
 * Change Log :
 *
 *           $Log$
 * Revision 2.7  1995/04/07  23:51:34  gandhi
 * Added Novell copyright message.  Code as released in N5.1
 *
 * Revision 2.6  1995/03/09  18:29:51  gandhi
 * added #ident directive to help USL keep track of sccs version #s.
 *
 * Revision 2.5  1995/02/15  18:45:40  gandhi
 * Ported file to UnixWare.  Use -DSCO for sco specific code, -DUNIXWARE for
 *  UnixWare specific code.
 * Renamed a lot of macros to make it specific to the crom driver.  For
 * example, NONE and POINTER are now CROM_NONE and CROM_POINTER respectively.
 * The fields in all_reg_t data structure are now referenced by macros that
 *  are all uppercase (as opposed to lower case earlier).
 *
 * Revision 2.4  1994/05/19  21:35:04  gandhi
 * redefined fields of all_reg_t structure so that macros can refer to
 * registers as eax, ah, al, etc.
 *
 * Revision 2.3  1993/09/22  13:37:07  gregs
 * Added functions to get and set EV's
 *
 * Revision 2.2  1993/06/30  18:04:46  gregs
 * Renamed reg_t to register_t.
 *
 * Revision 2.1  1992/09/22  20:56:40  andyd
 * Release EFS 1.5.0.
 *
 * Revision 1.1  1992/07/30  22:46:32  andyd
 * Initial revision
 *
 *           $EndLog$
 *
 ********************************************************/

/*
 *  Generic ROM call header file.
 */

#ifndef	_IO_CROM_CROM_H		/* wrapper symbol for kernel use */
#define	_IO_CROM_CROM_H		/* subject to change without notice */

/* ioctl() commands to the crom driver */
#define OLD_ROM_CALL		0		/* obsolete - issue rom call */
#define OLD_INT15_ROM_CALL	OLD_ROM_CALL	/* obsolete */
#define EISA_IOCTL		('E'<<8)
#define INT15_ROM_CALL		(EISA_IOCTL|7)	/* issue int15 rom call */

#if defined(UNIXWARE)
#define EISA_CMOS_QUERY		(EISA_IOCTL|1)	/* UnixWare only - see below
						 * for args */

/*
 * Structure to be passed as an argument to EISA_CMOS_QUERY ioctl()
 * The data field consists of a series of fields starting with the
 * KEY_MASK which is a logical OR of the fields defined below.
 * Following the KEY_MASK, there are a series of values, which are
 * either 4 byte integers (for Keys belonging to the EISA_INTEGER
 * class), or a series of characters (for string type keys).  The
 * values must be passed in the same order as the Keys described
 * below (ie., slot # must be first, followed by the function, and
 * so on).  All or some of the keys may be used.  The KEY_MASK must
 * have the appropriate bits set to reflect the values passed.
 */
typedef struct {
	char *data;
	int length;
} eisanvm;


/*
 * KEY_MASK field in EISA_CMOS_QUERRY ioctl()
 * This mask is scanned from bit 0 to bit 7. If a bit is set, it means
 * that an argument of the type corresponding to the bit position in
 * the key mask is next in the argument list.
 */
typedef struct key_mask {
	unsigned int
		slot	  : 1,	/* EISA Slot Number. */
		function  : 1,	/* Function Record Numbr within a EISA slot */
		board_id  : 1,	/* EISA readable Board ID. */
		revision  : 1,	/* EISA Board Revision Number. */
		checksum  : 1,	/* EISA Board Firmware Checksum. */
		type	  : 1,	/* EISA Board Type String. */
		sub_type  : 1,	/* EISA Board Sub-type String. */
		resources : 1,	/* EISA Board Resource list. */
			  : 24;
} KEY_MASK;

#define ARGUMENTS		8
#define LAST_ARG		(1 << (ARGUMENTS - 1))
#define EISA_SLOT		0x01	/* info about slot specified */
#define EISA_FUNCTION		0x02	/* slots w/matching function record */
#define EISA_BOARD_ID		0x04	/* slots w/matching boardid */
#define EISA_REVISION		0x08	/* slots w/matching board rev */
#define EISA_CHECKSUM		0x10	/* slots w/specified checksum */
#define	EISA_TYPE		0x20	/* slots w/matching type string */
#define EISA_SUB_TYPE		0x40	/* slots w/matching subtype string */
#define EISA_RESOURCES		0x80	/* slots w/matching resource record */
#define EISA_INTEGER		(EISA_SLOT | EISA_FUNCTION | \
				 EISA_BOARD_ID | EISA_REVISION | \
				 EISA_CHECKSUM | EISA_RESOURCES)
#define EISA_SLOTS		16
#endif /* defined(UNIXWARE) */

/*
 * Commands for int15 ROM call - fill the appropriate register in the
 * all_reg_t structure
 */
#define GET_EV			0xD8A4	/* obsolete - get EV from EISA NVM */
#define SET_EV			0xD8A5	/* obsolete - set EV in EISA NVM */
#define EISA_GET_EV_CMD		0xD8A4	/* get EV from EISA NVM */
#define EISA_SET_EV_CMD		0xD8A5	/* set EV in EISA NVM */

/* Bit field in the eflags register to check if int15 call returned error */
#define CARRY_FLAG		0x01	/* Carry flag bit in Flags reg. */

/* Values to check for result codes for int15 rom call. */
#define ROM_CALL_ERROR		0x01	/* ROM call error bit */
#define CALL_NOT_SUPPORTED	0x86	/* ROM call not supported */
#define ERROR_NOT_LOGGED	0x87	/* ROM call not logged */
#define EV_NOT_FOUND		0x88	/* EV not found with ROM call */

#pragma pack(1)

/*
 * Registers to pass to the kernel from user space.  If the register value
 * is an address of a buffer, set the opcode flag stating this fact.  Also,
 * tell the kernel how long the data buffer is.
 */
typedef struct {
	union {
		unsigned long lword;		/* eax */
		unsigned short word;		/* ax */

		struct {
			unsigned char low;	/* al */
			unsigned char high;	/* ah */
		} byte;
	} data;

	unsigned char opcode;	/* see below */
	unsigned long length;	/* if the reg. is a pointer, how much data */
} register_t;

/* opcode types for the register variable */
#define CROM_NONE	0	/* the register value only contains data */
#define CROM_POINTER	1	/* the register is an address for data */


/* This is all registers contained in one structure */
typedef struct {
	register_t eax_reg;
	register_t ebx_reg;
	register_t ecx_reg;
	register_t edx_reg;
	register_t edi_reg;
	register_t esi_reg;
	register_t eflags_reg;
} all_reg_t;

#pragma pack()

/*
 * Macros to map fields in the all_reg_t structure to something
 * more palatable.
 */
#define	EAX		eax_reg.data.lword
#define	AX		eax_reg.data.word
#define	AL		eax_reg.data.byte.low
#define	AH		eax_reg.data.byte.high
#define	EAX_OPCODE	eax_reg.opcode
#define	EAX_LENGTH	eax_reg.length

#define	EBX		ebx_reg.data.lword
#define	BX		ebx_reg.data.word
#define	BL		ebx_reg.data.byte.low
#define	BH		ebx_reg.data.byte.high
#define	EBX_OPCODE	ebx_reg.opcode
#define	EBX_LENGTH	ebx_reg.length

#define	ECX		ecx_reg.data.lword
#define	CX		ecx_reg.data.word
#define	CL		ecx_reg.data.byte.low
#define	CH		ecx_reg.data.byte.high
#define	ECX_OPCODE	ecx_reg.opcode
#define	ECX_LENGTH	ecx_reg.length

#define	EDX		edx_reg.data.lword
#define	DX		edx_reg.data.word
#define	DL		edx_reg.data.byte.low
#define	DH		edx_reg.data.byte.high
#define	EDX_OPCODE	edx_reg.opcode
#define	EDX_LENGTH	edx_reg.length

#define	EDI		edi_reg.data.lword
#define	DI		edi_reg.data.word
#define	DI_LOW		edi_reg.data.byte.low
#define	DI_HIGH		edi_reg.data.byte.high
#define	EDI_OPCODE	edi_reg.opcode
#define	EDI_LENGTH	edi_reg.length

#define	ESI		esi_reg.data.lword
#define	SI		esi_reg.data.word
#define	SI_LOW		esi_reg.data.byte.low
#define	SI_HIGH		esi_reg.data.byte.high
#define	ESI_OPCODE	esi_reg.opcode
#define	ESI_LENGTH	esi_reg.length

#define	EFLAGS		eflags_reg.data.lword
#define	FLAGS		eflags_reg.data.word
#define	FLAGS_LOW	eflags_reg.data.byte.low
#define	FLAGS_HIGH	eflags_reg.data.byte.high
#define	EFLAGS_OPCODE	eflags_reg.opcode
#define	EFLAGS_LENGTH	eflags_reg.length

#endif /* _IO_CROM_CROM_H */
