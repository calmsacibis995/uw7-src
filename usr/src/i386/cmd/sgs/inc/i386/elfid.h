#ifndef _ELFID_H_
#define _ELFID_H_
#ident	"@(#)sgs-inc:i386/elfid.h	1.3"

/* ELF objects produced by the OpenServer compiler contain
 * a special .note section that allows them to be distinguished
 * from Intel iABI ELF files.  COFF files converted to ELF
 * by libelf also contain this note section.
 */

/* Macros useful for picking apart the SCO ID .note section entry */

#define NT_FIXED	3	/* Number of fixed words */
#define NT_WORD_SZ	4	/* Size in bytes of a ELF word */

#define NT_ELFID	1
#define NT_ELFID_SZ	(NT_NAME_PAD + (NT_FIXED + ELFID_DESC) * NT_WORD_SZ)
#define NT_NAME		"SCO"
#define NT_NAME_SZ	sizeof(NT_NAME)
#define NT_NAME_OFF	(NT_FIXED * NT_WORD_SZ)
#define NT_NAME_PAD	((NT_NAME_SZ + 3) & ~0x3)

/*
 * This version stamp contains ELFID_DESC words of info, which are: 1)
 * Major and minor version number, 2) Source info, 3) flavour info
 */

#define ELFID_DESC	3
#define ELFID_DESC_SZ	(ELFID_DESC * NT_WORD_SZ)
#define ELFID_DESC_OFF	((NT_FIXED * NT_WORD_SZ) + NT_NAME_PAD)

/* Major number in upper 16 bits, minor in lower */
#define ELFID_MAJOR	1
#define ELFID_MINOR	1
#define ELFID_MK_VERS(major, minor)	(((major) << 16) | (minor))
#define ELFID_GET_MAJOR(num)		(((num) >> 16) & 0xFFFF)
#define ELFID_GET_MINOR(num)		((num) & 0xFFFF)

/*
 * Source info: the low 16 bits are a bitmask indicating info about what
 * went into the ELF object; the high bits can indicate vendor of the
 * tool, SCO uses 0x0000.
 */

#define ELFID_SRC_MASK	0xFFFF

#define ELFID_SRC_ELF		0x0001		/* SCO ELF */
#define ELFID_SRC_COFF		0x0002		/* Converted COFF file */
#define ELFID_SRC_UNKNOWN	0x0004		/* Unstamped ELF file */
#define ELFID_SRC_IABI_ELF	0x0008		/* source was iABI ELF file */

/* special values inserted in e_flags field of ELF header
 * by fixup program to assert that the executable is
 * intended for OSR5 or is a "universal binary" (intended
 * to run on UnixWare, OpenServer and Gemini). 
 */
#define OSR5_FIX_FLAG	0x3552534F
#define UDK_FIX_FLAG	0x314B4455

#endif
