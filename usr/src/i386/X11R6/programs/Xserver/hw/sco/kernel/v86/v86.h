#ifndef _SYS_V86_H
#define _SYS_V86_H
#ifdef __STDC__
#pragma comment(exestr, "@(#)v86.h 11.1 ")
#else
#ident "@(#)v86.h 11.1 "
#endif
/*
 *	Copyright (C) 1994 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

/*
 * v86 video bios driver header file.
 *
 * Modification History:
 * 
 * S000, Fri Apr 11 15:26:57 PDT 1997, kylec@sco.com
 * - change V86BIOS_MISC 
 *
 */


#ifdef  _M_I386
#pragma pack(4)
#else
#pragma pack(2)
#endif

#define V86_DEVICE	"/dev/X/v86"
#define V86_CALLBIOS	0x1 
#define V86_CALLROM	0x2
#define V86_VIDEO_BIOS	0x10


/* 
 * Memory needed to map for v86bios call for PC/AT
 */
#define V86BIOS_VIDEO		0xa0000
#define V86BIOS_SIZE		(0x10000 * 6) /* from 0xa0000 to 0xfffff */
#define V86BIOS_MISC		(MMU_PAGESIZE/2) /* misc data */

/* 
 * i8086 far pointer
 */
struct v86_farptr {
	ushort_t 	v86_off;
	ushort_t 	v86_cs;
};

typedef struct v86_farptr v86_farptr_t;

enum v86bios_type {
	V86BIOS_INT,		/* for usual 16-bit registers */
	V86BIOS_INT32,		/* for 32-bit registers */
	V86BIOS_ROMCALL		/* for calling ROM routines */
};

typedef enum v86bios_type v86bios_type_t;

/*
 * Structure for calling a v86bios call
 */
struct intregs {
	v86bios_type_t	type; 	 /* indicating how to call */
	int		retval;	 /* return value: carry bit */
	volatile uint_t	done;	 /* internal use */
	int		error;	 /* flag indicating error */
	
	union {
		ushort_t	intval;
		v86_farptr_t 	farptr;
	} entry;

	union {
		unsigned int eax;
		struct {
			unsigned short ax;
		} word;
		struct {
			unsigned char al;
			unsigned char ah;
		} byte;
	} eax;

	union {
		unsigned int ebx;
		struct {
			unsigned short bx;
		} word;
		struct {
			unsigned char bl;
			unsigned char bh;
		} byte;
	} ebx;

	union {
		unsigned int ecx;
		struct {
			unsigned short cx;
		} word;
		struct {
			unsigned char cl;
			unsigned char ch;
		} byte;
	} ecx;

	union {
		unsigned int edx;
		struct {
			unsigned short dx;
		} word;
		struct {
			unsigned char dl;
			unsigned char dh;
		} byte;
	} edx;

	union {
		unsigned int edi;
		struct {
			unsigned short di;
		} word;
	} edi;

	union {
		unsigned int esi;
		struct {
			unsigned short si;
		} word;
	} esi;

	ushort_t	bp;
	ushort_t	es;
	ushort_t	ds;
#ifdef NOTYET
	ushort_t	ss;
	ushort_t	sp;
	ulong_t	oesp;
#endif /* NOTYET */
};

typedef struct intregs	intregs_t;


#ifdef _KERNEL
/*
** Kernel service routine defines
*/

/*
** Kernel service routine prototypes
*/

#if defined(__STDC__) && !defined(_NO_PROTOTYPE)

void
v86bios(intregs_t *);

#endif /* defined(__STDC) && !defined(_NO_PROTOTYPE) */

#endif /* _KERNEL */

#pragma pack()
#endif	/* _SYS_V86_H */








