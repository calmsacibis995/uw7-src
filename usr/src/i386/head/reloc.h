#ident	"@(#)sgs-head:i386/head/reloc.h	2.7.2.1"

#ifndef _RELOC_H
#define _RELOC_H

struct reloc {
	long	r_vaddr;	/* (virtual) address of reference */
	long	r_symndx;	/* index into symbol table */
	unsigned short	r_type;		/* relocation type */
	};

/*
 *   relocation types for all products and generics
 */

/*
 * All generics
 *	reloc. already performed to symbol in the same section
 */
#define  R_ABS		0

/*
 * X86 generic
 *	8-bit offset reference in 8-bits
 *	8-bit offset reference in 16-bits 
 *	12-bit segment reference
 *	auxiliary relocation entry
 */
#define	R_OFF8		07
#define R_OFF16		010
#define	R_SEG12		011
#define	R_AUX		013

/*
 * B16 and X86 generics
 *	16-bit direct reference
 *	16-bit "relative" reference
 *	16-bit "indirect" (TV) reference
 */
#define  R_DIR16	01
#define  R_REL16	02
#define  R_IND16	03

/*
 * 3B generic
 *	24-bit direct reference
 *	24-bit "relative" reference
 *	16-bit optimized "indirect" TV reference
 *	24-bit "indirect" TV reference
 *	32-bit "indirect" TV reference
 */
#define  R_DIR24	04
#define  R_REL24	05
#define  R_OPT16	014
#define  R_IND24	015
#define  R_IND32	016

/*
 * 3B and M32 generics
 *	32-bit direct reference
 */
#define  R_DIR32	06

/*
 * M32 generic
 *	32-bit direct reference with bytes swapped
 */
#define  R_DIR32S	012

/*
 * DEC Processors  VAX 11/780 and VAX 11/750
 *
 */

#define R_RELBYTE	017
#define R_RELWORD	020
#define R_RELLONG	021
#define R_PCRBYTE	022
#define R_PCRWORD	023
#define R_PCRLONG	024

/*
 * Motorola 68000
 *
 * ... uses R_RELBYTE, R_RELWORD, R_RELLONG, R_PCRBYTE and R_PCRWORD as for
 * DEC machines above.
 */

#define	RELOC	struct reloc
#define	RELSZ	sizeof(RELOC)	/* was 10 */

/*
 * Motorola 88000 (Risc architecture) as defined in OCS 1.0
 *
 * Since AT&T has no "hot-line" to call to request reservation of new
 * relocation types, these numbers were pulled out of thin air.  Also,
 * since there is no definitive specification about what the existing
 * values mean we do not use any of them.  The syntax of the entries
 * is:
 *
 *	R_[H|L]<VRT|PCR><8|16|26|32>[L]
 *
 * where:
 *
 *	[]	means optional fields
 *	<>	mandatory fields, select one
 *	|	alternation, select one
 *	H	use the high 16 bits of symbol's virtual address
 *	L	use the low 16 bits of symbol's virtual address
 *	VRT	use the symbol's virtual address
 *	PCR	use symbol's virtual address minus value of PC
 *
 * Currently used relocation types are:
 *
 * R_PCR16L
 *
 *	PC relative, 16 bits of the 16 bit field at offset r_vaddr, longword
 *	aligned.  I.E., the 16 bits represent the top 16 bits of an 18 bit
 *	byte offset from the current PC.
 *
 * R_PCR26L
 *
 *	PC relative, lower 26 bits of the 32 bit field at offset r_vaddr,
 *	longword aligned.  I.E., the 26 bits represent the top 26 bits of
 *	a 28 bit byte offset from the current PC.
 *
 * R_VRT16
 *
 *	Reference to a symbol at an absolute address in the virtual address
 *	space, 16 bits of the 16 bit field at offset r_vaddr.  If the
 *	relocation value will not fit in 16 bits an error or warning
 *	is generated and zero is used as the relocation value.
 *
 * R_HVRT16
 *
 *	Same as R_VRT16 except that the high 16 bits of the absolute address
 *	are used as the relocation value.
 *
 * R_LVRT16
 *
 *	Same as R_VRT16 except that the low 16 bits of the absolute address
 *	are used as the relocation value and it is not an error to have
 *	overflow.
 *
 * R_VRT32
 *
 *	Reference to a symbol at an absolute address in the virtual address
 *	space, 32 bits of the 32 bit field at offset r_vaddr.
 *
 */

#define	R_PCR16L	128
#define	R_PCR26L	129
#define	R_VRT16		130
#define	R_HVRT16	131
#define	R_LVRT16	132
#define	R_VRT32		133

	/* Definition of a "TV" relocation type */

#if defined(__STDC__)

#if #machine(N3B)
#define ISTVRELOC(x)	((x==R_OPT16)||(x==R_IND24)||(x==R_IND32))
#elif #machine(B16) || #machine(X86)
#define ISTVRELOC(x)	(x==R_IND16)
#else
#define ISTVRELOC(x)	(x!=x)	/* never the case */
#endif

#else
#if N3B
#define ISTVRELOC(x)	((x==R_OPT16)||(x==R_IND24)||(x==R_IND32))
#endif
#if B16 || X86
#define ISTVRELOC(x)	(x==R_IND16)
#endif
#if M32
#define ISTVRELOC(x)	(x!=x)	/* never the case */
#endif

#endif 	/* __STDC__ */

#endif 	/* _RELOC_H */
