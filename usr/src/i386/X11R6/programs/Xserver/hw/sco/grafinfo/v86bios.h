/*
 *	@(#) v86bios.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Sep 07 19:48:57 PDT 1992	buckm@sco.com
 *	- Created from working demo source supplied by Mike Davidson.
 *	  All the nice bits are his; any nasty bits are my fault.
 *	S001	Thu Nov 05 20:53:09 PST 1992	buckm@sco.com
 *	- Bump ERR_ defs to larger, less common values.
 *	S002	Mon Mar 29 23:35:53 PST 1993	buckm@sco.com
 *	- Add ERR_V86_TIMEOUT.
 */

/*
 * v86bios.h	- v86 mode bios defs
 */

#ifndef	V86BIOS_H
#define	V86BIOS_H

/*
 * typedefs for V86 mode data
 */
typedef unsigned char	byte_t;
typedef unsigned short	word_t;
typedef unsigned long	dword_t;
typedef unsigned long	fptr_t;

/*
 * macros for accessing byte and word data
 */
#define	BYTE(n, v)	(((byte_t *)(&v))[n])
#define	WORD(n, v)	(((word_t *)(&v))[n])

/*
 * macros for accessing segment and offset of a far pointer
 */
#define	SEGMENT(f)	WORD(1, f)
#define	OFFSET(f)	WORD(0, f)

/*
 * return values from V86Emulate()
 */
#define	ERR_V86_HALT		-11
#define	ERR_V86_DIV0		-12
#define	ERR_V86_SGLSTP		-13
#define	ERR_V86_BRKPT		-14
#define	ERR_V86_OVERFLOW	-15
#define	ERR_V86_BOUND		-16
#define	ERR_V86_ILLEGAL_OP	-17
#define	ERR_V86_ILLEGAL_IO	-18
#define	ERR_V86_TIMEOUT		-19

/*
 * V86 mode TSS regs
 */
#define EAX(t)	((t)->t_eax)
#define AX(t)	WORD(0, (t)->t_eax)
#define AH(t)	BYTE(1, (t)->t_eax)
#define AL(t)	BYTE(0, (t)->t_eax)

#define EBX(t)	((t)->t_ebx)
#define BX(t)	WORD(0, (t)->t_ebx)
#define BH(t)	BYTE(1, (t)->t_ebx)
#define BL(t)	BYTE(0, (t)->t_ebx)

#define ECX(t)	((t)->t_ecx)
#define CX(t)	WORD(0, (t)->t_ecx)
#define CH(t)	BYTE(1, (t)->t_ecx)
#define CL(t)	BYTE(0, (t)->t_ecx)

#define EDX(t)	((t)->t_edx)
#define DX(t)	WORD(0, (t)->t_edx)
#define DH(t)	BYTE(1, (t)->t_edx)
#define DL(t)	BYTE(0, (t)->t_edx)

#define ESI(t)	((t)->t_esi)
#define SI(t)	WORD(0, (t)->t_esi)

#define EDI(t)	((t)->t_edi)
#define DI(t)	WORD(0, (t)->t_edi)

#define EBP(t)	((t)->t_ebp)
#define BP(t)	WORD(0, (t)->t_ebp)
#define ESP(t)	((t)->t_esp)
#define SP(t)	WORD(0, (t)->t_esp)

#define DS(t)	WORD(0, (t)->t_ds)
#define ES(t)	WORD(0, (t)->t_es)
#define SS(t)	WORD(0, (t)->t_ss)
#define CS(t)	WORD(0, (t)->t_cs)
#define FS(t)	WORD(0, (t)->t_fs)
#define GS(t)	WORD(0, (t)->t_gs)

#define EIP(t)	((t)->t_eip)
#define IP(t)	WORD(0, (t)->t_eip)

#define EFL(t)	((t)->t_eflags)
#define FL(t)	WORD(0, (t)->t_eflags)

#endif	/* V86BIOS_H */
