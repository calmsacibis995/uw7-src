/	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:svc/msop_p.s	1.1.2.1"
	.ident	"$Header$"
	.file	"svc/msop_p.s"

/
/ This module provides the interface from the base kernel to PSM.
/ There is an entry point in this module for every _A_MSOP. The routine
/ in this file loads the PSM entry address from the "msops" table and
/ jumps to the PSM routine. No stack frame is needed or created.
/

include(KBASE/svc/asm.m4)
include(assym_include)
/include(KBASE/svc/intr.m4)
/include(KBASE/util/debug.m4)

FILE(`msop_p.s')



ENTRY(ms_init_cpu)
	jmp	*msops+_A_MSOP_INIT_CPU
	SIZE(ms_init_cpu)


ENTRY(ms_intr_taskpri)
	jmp	*msops+_A_MSOP_INTR_TASKPRI
	SIZE(ms_intr_taskpri)


ENTRY(ms_tick_2)
	jmp	*msops+_A_MSOP_TICK_2
	SIZE(ms_tick_2)


ENTRY(ms_intr_attach)
	jmp	*msops+_A_MSOP_INTR_ATTACH
	SIZE(ms_intr_attach)


ENTRY(ms_intr_detach)
	jmp	*msops+_A_MSOP_INTR_DETACH
	SIZE(ms_intr_detach)


ENTRY(ms_intr_mask)
	jmp	*msops+_A_MSOP_INTR_MASK
	SIZE(ms_intr_mask)


ENTRY(ms_intr_unmask)
	jmp	*msops+_A_MSOP_INTR_UNMASK
	SIZE(ms_intr_unmask)


ENTRY(ms_intr_complete)
	jmp	*msops+_A_MSOP_INTR_COMPLETE
	SIZE(ms_intr_complete)


ENTRY(ms_xpost)
	jmp	*msops+_A_MSOP_XPOST
	SIZE(ms_xpost)


ENTRY(ms_time_get)
	jmp	*msops+_A_MSOP_TIME_GET
	SIZE(ms_time_get)


ENTRY(ms_time_add)
	jmp	*msops+_A_MSOP_TIME_ADD
	SIZE(ms_time_add)


ENTRY(ms_time_sub)
	jmp	*msops+_A_MSOP_TIME_SUB
	SIZE(ms_time_sub)


ENTRY(ms_time_cvt)
	jmp	*msops+_A_MSOP_TIME_CVT
	SIZE(ms_time_cvt)


ENTRY(ms_time_spin)
	jmp	*msops+_A_MSOP_TIME_SPIN
	SIZE(ms_time_spin)


ENTRY(ms_rtodc)
	jmp	*msops+_A_MSOP_RTODC
	SIZE(ms_rtodc)


ENTRY(ms_wtodc)
	jmp	*msops+_A_MSOP_WTODC
	SIZE(ms_wtodc)


ENTRY(ms_idle_self)
	jmp	*msops+_A_MSOP_IDLE_SELF
	SIZE(ms_idle_self)


ENTRY(ms_idle_exit)
	jmp	*msops+_A_MSOP_IDLE_EXIT
	SIZE(ms_idle_exit)


ENTRY(ms_shutdown)
	jmp	*msops+_A_MSOP_SHUTDOWN
	SIZE(ms_shutdown)


ENTRY(ms_offline_prep)
	jmp	*msops+_A_MSOP_OFFLINE_PREP
	SIZE(ms_offline_prep)


ENTRY(ms_offline_self)
	jmp	*msops+_A_MSOP_OFFLINE_SELF
	SIZE(ms_offline_self)


ENTRY(ms_start_cpu)
	jmp	*msops+_A_MSOP_START_CPU
	SIZE(ms_start_cpu)


ENTRY(ms_show_state)
	jmp	*msops+_A_MSOP_SHOW_STATE
	SIZE(ms_show_state)


ENTRY(ms_io_read_8)
	jmp	*msops+_A_MSOP_IO_READ_8
	SIZE(ms_io_read_8)


ENTRY(ms_io_read_16)
	jmp	*msops+_A_MSOP_IO_READ_16
	SIZE(ms_io_read_16)


ENTRY(ms_io_read_32)
	jmp	*msops+_A_MSOP_IO_READ_32
	SIZE(ms_io_read_32)


ENTRY(ms_io_write_8)
	jmp	*msops+_A_MSOP_IO_WRITE_8
	SIZE(ms_io_write_8)


ENTRY(ms_io_write_16)
	jmp	*msops+_A_MSOP_IO_WRITE_16
	SIZE(ms_io_write_16)


ENTRY(ms_io_write_32)
	jmp	*msops+_A_MSOP_IO_WRITE_32
	SIZE(ms_io_write_32)


ENTRY(ms_io_rep_read_8)
	jmp	*msops+_A_MSOP_IO_REP_READ_8
	SIZE(ms_io_rep_read_8)


ENTRY(ms_io_rep_read_16)
	jmp	*msops+_A_MSOP_IO_REP_READ_16
	SIZE(ms_io_rep_read_16)


ENTRY(ms_io_rep_read_32)
	jmp	*msops+_A_MSOP_IO_REP_READ_32
	SIZE(ms_io_rep_read_32)


ENTRY(ms_io_rep_write_8)
	jmp	*msops+_A_MSOP_IO_REP_WRITE_8
	SIZE(ms_io_rep_write_8)


ENTRY(ms_io_rep_write_16)
	jmp	*msops+_A_MSOP_IO_REP_WRITE_16
	SIZE(ms_io_rep_write_16)


ENTRY(ms_io_rep_write_32)
	jmp	*msops+_A_MSOP_IO_REP_WRITE_32
	SIZE(ms_io_rep_write_32)


ENTRY(ms_farcopy)
	jmp	*msops+_A_MSOP_FARCOPY
	SIZE(ms_farcopy)




/
/ The following routines read and write I/O address space:
/	inb, inl, inw, outb, outl, outw
/ These are the default routines used by PSMs unless they replace
/ the routine during initialization.
/

/
/ unsigned char
/ msop_io_read_8(ms_port_t port)
/	read a byte from an 8 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_read_8)
	movl	SPARG0, %edx
	subl    %eax, %eax
	inb	(%dx)
	ret
	SIZE(msop_io_read_8)

/
/ ushort_t
/ msop_io_read_16(ms_port_t port)
/	read a 16 bit short word from a 16 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_read_16)
	movl	SPARG0, %edx
	subl	%eax, %eax
	data16
	inl	(%dx)
	ret
	SIZE(msop_io_read_16)

/
/ unsigned int
/ msop_io_read_32(ms_port_t port)
/	read a 32 bit word from a 32 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_read_32)
	movl	SPARG0, %edx
	inl	(%dx)
	ret
	SIZE(msop_io_read_32)


/ void
/ msop_io_write_8(ms_port_t port, unsigned char data)
/	write a byte to an 8 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_write_8)
	movl	SPARG0, %edx
	movl	SPARG1, %eax
	outb	(%dx)
	ret
	SIZE(msop_io_write_8)

/ void
/ msop_io_write_16(ms_port_t port, ushort_t data)
/	write a 16 bit short word to a 16 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_write_16)
	movl	SPARG0, %edx
	movl	SPARG1, %eax
	data16
	outl	(%dx)
	ret
	SIZE(msop_io_write_16)

/ void
/ msop_io_write_32(ms_port_t port, unsigned int data)
/	write a 32 bit long word to a 32 bit I/O port
/
/ Calling/Exit State:
/	none
ENTRY(msop_io_write_32)
	movl	SPARG0, %edx
	movl	SPARG1, %eax
	outl	(%dx)
	ret
	SIZE(msop_io_write_32)

/
/ The following routines move data between buffer and I/O port:
/	repinsb, repinsd, repinsw, repoutsb, repoutsd, repoutsw
/

/
/ void
/ msop_io_rep_read_8(ms_port_t port, unsigned char *addr, int cnt)
/	read bytes from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_rep_read_8)
	pushl	%edi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %edi
	movl	4+SPARG2, %ecx		/ is BYTE count

	rep
	insb

	popl	%edi
	ret
	SIZE(msop_io_rep_read_8)

/
/ void
/ msop_io_rep_read_16(ms_port_t port, ushort_t *addr, int cnt)
/	read 16 bit words from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_rep_read_16)
	pushl	%edi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %edi
	movl	4+SPARG2, %ecx		/ is SWORD count

	rep
	data16
	insl

	popl	%edi
	ret
	SIZE(msop_io_rep_read_16)

/
/ void
/ msop_io_rep_read_32(ms_port_t port, unsigned int *addr, int cnt)
/	read 32 bit words from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_rep_read_32)
	pushl	%edi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %edi
	movl	4+SPARG2, %ecx		/ is DWORD count

	rep
	insl

	popl	%edi
	ret
	SIZE(msop_io_rep_read_32)

/
/ void
/ msop_io_rep_write_8(ms_port_t port, unsigned char *addr, int cnt)
/	write bytes from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_rep_write_8)
	pushl	%esi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %esi
	movl	4+SPARG2, %ecx		/ is BYTE count

	rep
	outsb

	popl	%esi
	ret
	SIZE(msop_io_rep_write_8)

/
/ void
/ msop_io_rep_write_16(ms_port_t port, ushort_t *addr, int cnt)
/	write 16 bit words from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_rep_write_16)
	pushl	%esi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %esi
	movl	4+SPARG2, %ecx		/ is SWORD count

	rep
	data16
	outsl

	popl	%esi
	ret
	SIZE(msop_io_rep_write_16)
/
/ void
/ msop_io_rep_write_32(ms_port_t port, unsigned int *addr, int cnt)
/	write 32 bit words from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_io_rep_write_32)
	pushl	%esi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %esi
	movl	4+SPARG2, %ecx		/ is DWORD count

	rep
	outsl

	popl	%esi
	ret
	SIZE(msop_io_rep_write_32)


/
/ void
/ msop_time_add (ms_rawtime_t*, ms_rawtime_t*)
/	Add 2 64 bit rawtime value. 
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_time_add)
	movl	SPARG1, %edx
	movl	SPARG0, %ecx
	movl	(%edx),%eax
	addl	%eax,(%ecx)
	movl	4(%edx),%eax
	adcl	%eax,4(%ecx)
	SIZE(msop_time_add)

/
/ void
/ msop_time_sub (ms_rawtime_t*, ms_rawtime_t*)
/	Add 2 64 bit rawtime value. 
/
/ Calling/Exit State:
/	none
/
ENTRY(msop_time_sub)
	movl	SPARG1, %edx
	movl	SPARG0, %ecx
	movl	(%edx),%eax
	subl	%eax,(%ecx)
	movl	4(%edx),%eax
	sbbl	%eax,4(%ecx)
	SIZE(msop_time_sub)



