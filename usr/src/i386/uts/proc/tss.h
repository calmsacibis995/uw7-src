#ifndef _PROC_TSS_H	/* wrapper symbol for kernel use */
#define _PROC_TSS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/tss.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* Flags Register */

typedef struct flags {
	uint	fl_cf	:  1,	/* carry/borrow */
			:  1,	/* reserved */
		fl_pf	:  1,	/* parity */
			:  1,	/* reserved */
		fl_af	:  1,	/* carry/borrow */
			:  1,	/* reserved */
		fl_zf	:  1,	/* zero */
		fl_sf	:  1,	/* sign */
		fl_tf	:  1,	/* trace */
		fl_if	:  1,	/* interrupt enable */
		fl_df	:  1,	/* direction */
		fl_of	:  1,	/* overflow */
		fl_iopl :  2,	/* I/O privilege level */
		fl_nt	:  1,	/* nested task */
			:  1,	/* reserved */
		fl_rf	:  1,	/* resume */
		fl_vm	:  1,	/* virtual 86 mode */
		fl_ac	:  1,	/* alignment check (i486 and above) */
		fl_vif	:  1,	/* virtual interrupt flag (Pentium and above) */
		fl_vip	:  1,	/* virtual interrupt pending (Pentium) */
		fl_id	:  1,	/* cpuid enable (Pentium) */
		fl_res	: 10;	/* reserved */
} flags_t;

#define PS_C	0x00000001	/* carry bit */
#define PS_P	0x00000004	/* parity bit */
#define PS_AC	0x00000010	/* auxiliary carry bit */
#define PS_Z	0x00000040	/* zero bit */
#define PS_N	0x00000080	/* negative bit */
#define PS_T	0x00000100	/* trace enable bit */
#define PS_IE	0x00000200	/* interrupt enable bit */
#define PS_D	0x00000400	/* direction bit */
#define PS_V	0x00000800	/* overflow bit */
#define PS_IOPL	0x00003000	/* I/O privilege level */
#define PS_NT	0x00004000	/* nested task flag */
#define PS_RF	0x00010000	/* resume flag */
#define PS_VM	0x00020000	/* virtual 86 mode flag */
#define PS_ACK	0x00040000	/* alignment check flag (i486 and above) */
#define PS_VIF	0x00080000	/* virtual interrupt flag (Pentium and above) */
#define PS_VIP	0x00100000	/* virtual interrupt pending (Pentium) */
#define PS_ID	0x00200000	/* cpuid enable (Pentium) */

#ifdef _KERNEL

/*
 * User bits in the flags register.
 * We do not allow the user to change any other bits than these.
 */
#define PS_USERMASK	(PS_C | PS_P | PS_AC | PS_Z | PS_N | PS_D | PS_V | \
			 PS_ACK | PS_VIF | PS_VIP | PS_ID)

#endif /* _KERNEL */

/*
 * 386 TSS definition
 */

struct tss386 {
	unsigned short	t_link;
	unsigned short	_t_pad0;
	struct tss_stkbase {
		unsigned long	t_esp;
		unsigned short	t_ss;
		unsigned short	_t_pad1;
	} t_stkbase[3];
	paddr_t		t_cr3;
	unsigned long	t_eip;
	unsigned long	t_eflags;
	unsigned long	t_eax;
	unsigned long	t_ecx;
	unsigned long	t_edx;
	unsigned long	t_ebx;
	unsigned long	t_esp;
	unsigned long	t_ebp;
	unsigned long	t_esi;
	unsigned long	t_edi;
	unsigned short	t_es;
	unsigned short	_t_pad2;
	unsigned short	t_cs;
	unsigned short	_t_pad3;
	unsigned short	t_ss;
	unsigned short	_t_pad4;
	unsigned short	t_ds;
	unsigned short	_t_pad5;
	unsigned short	t_fs;
	unsigned short	_t_pad6;
	unsigned short	t_gs;
	unsigned short	_t_pad7;
	unsigned short	t_ldt;
	unsigned short	_t_pad8;
	unsigned short	t_tbit;
	unsigned short	t_bitmapbase;
};

#define t_esp0	t_stkbase[0].t_esp
#define t_ss0	t_stkbase[0].t_ss
#define t_esp1	t_stkbase[1].t_esp
#define t_ss1	t_stkbase[1].t_ss
#define t_esp2	t_stkbase[2].t_esp
#define t_ss2	t_stkbase[2].t_ss

#define TSS_NO_BITMAP	0xDFFF	/* value for t_bitmapbase when no I/O bitmap */

#define PS_USER		3
#define PS_KERNEL	0

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_TSS_H */
