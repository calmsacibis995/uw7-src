#ifndef _BOOT_BIOSCALL_H
#define _BOOT_BIOSCALL_H

#ident	"@(#)stand:i386at/boot/blm/bioscall.h	1.2"
#ident	"$Header$"

/*
 * Definitions for PC-AT BIOS call support and other platform-specific stuff.
 */

/* Inline versions of inb/outb */

asm int
inb(int port)
{
%mem port;
	movl	port, %edx
	xorl	%eax, %eax
	inb	(%dx)
}
#pragma asm partial_optimization inb

asm void
outb(int port, unsigned char val)
{
%treg port, val;
	pushl	port
	movb	val, %al
	popl	%edx
	outb	(%dx)
%treg port; mem val;
	movl	port, %edx
	movb	val, %al
	outb	(%dx)
%mem port, val;
	movb	val, %al
	movl	port, %edx
	outb	(%dx)
}
#pragma asm partial_optimization outb

asm void
outw(int port, unsigned short val)
{
%treg port, val;
	pushl	port
	movw	val, %ax
	popl	%edx
	data16
	outl	(%dx)
%treg port; mem val;
	movl	port, %edx
	movw	val, %ax
	data16
	outl	(%dx)
%mem port, val;
	movw	val, %ax
	movl	port, %edx
	data16
	outl	(%dx)
}
#pragma asm partial_optimization outw

union reg {
	unsigned long _32;
	unsigned short _16[2];
};

struct biosregs {
	unsigned int intnum;	/* S/W interrupt number */
	union reg _eax, _ebx, _ecx, _edx;
	union reg _esi, _edi, _ebp;
};
#define eax	_eax._32
#define ebx	_ebx._32
#define ecx	_ecx._32
#define edx	_edx._32
#define esi	_esi._32
#define edi	_edi._32
#define ebp	_ebp._32
#define ax	_eax._16[0]
#define bx	_ebx._16[0]
#define cx	_ecx._16[0]
#define dx	_edx._16[0]
#define si	_esi._16[0]
#define di	_edi._16[0]
#define bp	_ebp._16[0]

/* Flag bits for bioscall() return value */
#define EFL_C	0x01
#define EFL_Z	0x40

/*
 * Platform-specific extension to stage1_params_t.
 *
 * _RMp1ext is the structure originally filled in by the driver, immediately
 * following the stage1_params_t structure.
 *
 * _p1ext is a structure allocated by the bioscall support loaded later.
 * A pointer to this structure is overlaid on top of the _RMp1ext.
 */
struct _RMp1ext {
	void (*p1_RMbios)(int);
	void (*p1_RMprotcall)(void);
};
struct _p1ext {
	unsigned char *p1_confp;
	int (*p1_bioscall)(struct biosregs *);
	int (*p1_bioscall32)(struct biosregs *);
	char *(*p1_biosbuffer)(void);
	int (*p1_rmcall)(struct biosregs *, void *code, ulong_t code_size);
	int p1_bustypes;
	int p1_rawpart;	/* raw partition # for raw boot */
};
#define RMp1ext		(*(struct _RMp1ext *)(p1 + 1))
#define p1ext		(*(struct _p1ext **)(p1 + 1))

#define _RMprotcall	RMp1ext.p1_RMprotcall

#define bios_confp	(p1ext->p1_confp)
#define bioscall	(*p1ext->p1_bioscall)
#define bioscall32	(*p1ext->p1_bioscall32)
#define biosbuffer	(*p1ext->p1_biosbuffer)
#define rmcall		(*p1ext->p1_rmcall)
#define bustypes	(p1ext->p1_bustypes)
#define rawpart		(p1ext->p1_rawpart)

/* Definitions for p1_confp */
#define CONF_FEATURE1	5
#define   CONF_DUAL	0x01
#define   CONF_MCA	0x02
#define   CONF_XBIOS	0x04
#define CONF_FEATURE2	6
#define   CONF_MEMORY	0x10

/* Definitions for p1_bustypes (bitmask of detected standard I/O buses) */
#define BUS_ISA		(1 << 0)
#define BUS_EISA	(1 << 1)
#define BUS_MCA		(1 << 2)
#define BUS_PCI		(1 << 3)
#define BUS_PNP		(1 << 4)

#endif /* _BOOT_BIOSCALL_H */
