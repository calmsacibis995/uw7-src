/*    (c) Copyright, 1989 MOTOROLA, INC.   */
/*        All Rights Reserved              */
                                      
/*    This file contains copyrighted material.  Use of this file  */
/*    is restricted by the provisions of Motorola's Software      */
/*    License Agreement.                                          */

#ident	"@(#)sgs-inc:common/sys/elf_68K.h	1.1"

#ifndef _SYS_ELF_68K_H
#define _SYS_ELF_68K_H

/* NOTE: ld/m68k/machrel.c has surrogate table which must match this */

#define R_68K_NONE		0	/* relocation type */
#define R_68K_32		1
#define R_68K_16		2
#define R_68K_8			3
#define R_68K_PC32		4
#define R_68K_PC16		5
#define R_68K_PC8		6
#define R_68K_GOT32		7
#define R_68K_GOT16		8
#define R_68K_GOT8		9
#define R_68K_GOT32O		10
#define R_68K_GOT16O		11
#define R_68K_GOT8O		12
#define R_68K_PLT32		13
#define R_68K_PLT16		14
#define R_68K_PLT8		15
#define R_68K_PLT32O		16
#define R_68K_PLT16O		17
#define R_68K_PLT8O		18
#define R_68K_COPY		19
#define R_68K_GLOB_DAT		20	
#define R_68K_JMP_SLOT		21
#define R_68K_RELATIVE		22
#define R_68K_NUM		23	/* must be >last */

#define ELF_68K_MAXPGSZ		0x2000	/* maximum page size */

#endif
