	.ident	"@(#)kern-i386:io/ddi_misc.s	1.10.4.1"
	.ident	"$Header$"

/	ddi routines

include(../svc/asm.m4)
include(assym_include)

/
/ The following routines read and write I/O address space:
/	inb, inl, inw, outb, outl, outw
/

/
/ uchar_t
/ inb(int port)
/	read a byte from an 8 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(inb)
	jmp	 ms_io_read_8
	SIZE(inb)

/
/ ulong_t
/ inl(int port)
/	read a 32 bit word from a 32 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(inl)
	jmp	 ms_io_read_32
	SIZE(inl)

/
/ ushort_t
/ inw(int port)
/	read a 16 bit short word from a 16 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(inw)
	jmp	 ms_io_read_16
	SIZE(inw)

/ void
/ outb(int port, uchar_t data)
/	write a byte to an 8 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(outb)
	jmp	 ms_io_write_8
	SIZE(outb)

/ void
/ outl(int port, ulong_t data)
/	write a 32 bit long word to a 32 bit I/O port
/
/ Calling/Exit State:
/	none
ENTRY(outl)
	jmp	 ms_io_write_32
	SIZE(outl)

/ void
/ outw(int port, ushort_t data)
/	write a 16 bit short word to a 16 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(outw)
	jmp	 ms_io_write_16
	SIZE(outw)

/
/ The following routines move data between buffer and I/O port:
/	repinsb, repinsd, repinsw, repoutsb, repoutsd, repoutsw
/

/
/ void
/ repinsb(int port, uchar_t *addr, int cnt)
/	read bytes from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(repinsb)
	jmp	 ms_io_rep_read_8
	SIZE(repinsb)

/
/ void
/ repinsd(int port, ulong_t *addr, int cnt)
/	read 32 bit words from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(repinsd)
	jmp	 ms_io_rep_read_32
	SIZE(repinsd)

/
/ void
/ repinsw(int port, ushort_t *addr, int cnt)
/	read 16 bit words from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(repinsw)
	jmp	 ms_io_rep_read_16
	SIZE(repinsw)

/
/ void
/ repoutsb(int port, uchar_t *addr, int cnt)
/	write bytes from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(repoutsb)
	jmp	 ms_io_rep_write_8
	SIZE(repoutsb)

/
/ void
/ repoutsd(int port, ulong_t *addr, int cnt)
/	write 32 bit words from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(repoutsd)
	jmp	 ms_io_rep_write_32
	SIZE(repoutsd)

/
/ void
/ repoutsw(int port, ushort_t *addr, int cnt)
/	write 16 bit words from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(repoutsw)
	jmp	 ms_io_rep_write_16
	SIZE(repoutsw)


/
/ int
/ intr_disable(void)
/
/ Calling/Exit State:
/	None.
/
/ Remarks:
/	Disable all interrupts, with minimal overhead.
/	Returns the previous value of the EFLAGS register
/	for use in a subsequent call to intr_restore.
/	Normally an inline asm version of intr_disable
/	is used; this function version exists for the
/	benefit of callers who are unable to include
/	inline.h, or for implementations not supporting
/	inline asm functions.
/
ENTRY(intr_disable)
        pushfl
        cli
        popl    %eax
        ret
	SIZE(intr_disable)

/
/ void
/ intr_restore(int efl)
/
/ Calling/Exit State:
/	Argument is the EFLAGS value from previous call to intr_disable.
/
/ Remarks:
/	Restore interrupt enable state, with minimal overhead.
/
ENTRY(intr_restore)
        pushl   4(%esp)
        popfl
        ret
	SIZE(intr_restore)
