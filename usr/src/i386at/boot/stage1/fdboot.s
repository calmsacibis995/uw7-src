/	.file	"fdboot.s"

	.ident	"@(#)stand:i386at/boot/stage1/fdboot.s	1.5"
	.ident	"$Header$"

/ 	Stage 1 UNIX bootstrap -- floppy disk version
/
/	The primary job of stage1 is to load stage2 and "call" it.
/	The secondary job of stage1 is to provide platform-specific
/	pieces, such as drivers, needed by stage2 early enough in its
/	operation that it cannot yet load additional drivers.
/
/ 	The stage1 code on an AT-compatibile platform begins at 0:7C00,
/	and extends to 0:7DFF.
/
/	Note that in general, this code very much assumes it is loaded
/	below 64K.
/

	.set	BOOTDRV, 0		/ for floppy, always use drive 0
	.set	RETRIES, 30		/ max I/O error retries
	.set	EXTRA, 0x100		/ extra buffer space for rmcall()

	.set	STAGE1_LOADADDR, 0x7C00
	.set	STAGE1_LOADSIZE, 0x200
	.set	STAGE1_BASE, 0x800
	.set	BOOTSTACK, [STAGE1_BASE+0x1000]
	.set	TRACK0_BUF, BOOTSTACK
	.set	STAGE2_LOADADDR, [TRACK0_BUF+STAGE1_LOADSIZE]
	.set	TRACKBUF2, [STAGE1_LOADADDR+STAGE1_LOADSIZE+EXTRA]
	.set	STAGE1_END, 0x10000
	.set	STAGE1_SIZE, [STAGE1_END-STAGE1_BASE]
	.set	FREERAM_END, [511\*1024]

	/ device type flags (see boot.h)
	.set	D_REMOVABLE, [1 << 0]
	.set	D_VTOC, [1 << 1]
	.set	D_NET, [1 << 2]

	.set	DFLT_SPT, 18

	.text

	.globl	_start
_start:
	/ NOTE: Some previous versions began with a 'cli' and a jump over
	/ 4 bytes of "reserved space", labeled "drv_mark". I see no reason
	/ for these, so I've eliminated them. - kdg

	xor	%eax, %eax		/ set all segment registers to zero
	movw	%ax, %ds		/ (we never use %fs and %gs)
	movw	%ax, %es
	movw	%ax, %ss		/ MUST immediately precede load of %esp

	data16 / use 32-bit data-size
	movl	$BOOTSTACK, %esp	/ initialize stack pointer
					/ MUST immediately follow load of %ss

/	Flush any spurious keyboard input

kbflush:
	movb	$1, %ah
	int	$0x16			/ check for kbd character
	jz	kbflush_done

	movb	$0, %ah
	int	$0x16			/ get kbd character

	jmp	kbflush
kbflush_done:

/ 	Get the boot drive parameters.

/	For floppy, the BIOS call may tell us the parameters of the drive,
/	not the media. Since we might have media in a higher density drive
/	than the media itself, we need to try to figure out the parameters
/	for the media. Unfortunately, we have to avoid verify operations,
/	as they appear not to work correctly on CD-ROMs (w/floppy emulation).
/
/	The following algorithm supports at least the following combinations:
/
/		1.2M (15 spt) media in 1.2M (15 spt) drive
/		1.44M (18 spt) media in 1.44M (18 spt) drive
/		1.44M (18 spt) media in > 1.44M (>= 18 spt) drive
/		2.88M (36 spt) emulated media from CD-ROM
/
/	To do this, we first assume spt is as reported by the BIOS call.
/	Then, if the first track read fails, we reset spt to the default (18)
/	and try again.

	movb	$8, %ah			/ call the BIOS to get drive parms
	movb	$BOOTDRV, %dl		/ put boot drive in %dl for BIOS call
	int	$0x13

	andb	$0x3F, %cl		/ extract max sector number into %cl

	/ SWITCH TO PROTECTED MODE
	data16 / use 32-bit data-size
	call	goprot

	/ Relative data references are smaller in code size,
	/ so load up %ebp with a pointer to the start of our data.
	movl	$_data, %ebp

	// Assumes spt still in %cl
got_spt:
	movb	%cl, [spt-_data](%ebp)	/ store in spt

	/ set blksz = spt * 512 [512 is sector size]
	movzbl	%cl, %eax
	shll	$9, %eax
	movw	%ax, [blksz-_data](%ebp)

	/ Load stage2 (the second stage bootstrap) into the preallocated buffer.
	/ To keep things simple, read in a whole track, including stage1 again.
	call	fd_read0

	/ Check for error
	cmpl	$-1, [blkno-_data](%ebp)
	jne	load_ok

	/ If the read failed, assume it's because we got SPT wrong;
	/ set it to the default and try again.
	movb	$DFLT_SPT, %cl
	incl	[blkno-_data](%ebp)	/ reset blkno to zero
	jmp	got_spt

load_ok:
	/ Pass pointer to stage1 parameters as "argument" to stage2 entry point.
	leal	[stage1parms-_data](%ebp),%eax
	pushl	%eax

	/ Invoke stage2. Start address is load address plus contents of first
	/ byte of the loaded image. Invocation is as a procedure call, but it
	/ never returns.
	movw	$STAGE2_LOADADDR, %ax	/ (assumes upper 1/2 of %eax zero)
	movzbl	(%eax), %ebx
	addl	%ebx, %eax
	call	*%eax
	/ NOTREACHED

/-------------------------------------------------------------------------------
/
/	Floppy disk driver read routine.
/
/	Read one track into the current buffer.
/
fd_read:
	/ ENTERED IN PROTECTED MODE

	/ use 2nd buffer for calls from stage2
	movl	$TRACKBUF2, buffer

fd_read0:
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	$_data, %ebp

	/ SWITCH TO REAL-ADDRESS MODE (and clear regs)
	call	goreal_clear
	sti				/ enable interrupts in real-mode

	// Assumes rest of %ecx zero (from goreal_clear)
	movb	$RETRIES-1, %cl		/ set retry counter (%cx)

fd_retry:
	data16 / use 32-bit data-size
	pushl	%ecx			/ save the retry counter

	/ set up arguments to disk BIOS call

	movb	$BOOTDRV, %dl		/ put boot drive in %dl for BIOS call
	addr16 / use 32-bit address-size
	mov	[buffer-_data](%ebp), %ebx / set buffer pointer (%es already 0)
	addr16 / use 32-bit address-size
	movb	[spt-_data](%ebp), %al	/ sector count is spt (whole track)
	movb	$1, %cl			/ starting sector is 1

	/ translate blkno into (cyl, head)
	addr16 / use 32-bit address-size
	movb	[blkno-_data](%ebp), %ch
	movb	%ch, %dh
	shrb	%cl, %ch		/ %ch <- cyl = blkno >> 1
	andb	%cl, %dh		/ %dh <- head = blkno & 1

	data16 / use 32-bit data-size
	pushl	%ebp

	/ read track from disk using disk read BIOS call
	movb	$0x02, %ah
	int	$0x13

	jnc	read_ok			/ success; return

	cmpb	$0x11, %ah		/ ECC corrected?
	je	read_ok			/ if so, ignore error

	/ reset controller and try again
	movb	$0, %ah			/ reset controller
	int	$0x13

	data16 / use 32-bit data-size
	popl	%ebp

	data16 / use 32-bit data-size
	popl	%ecx

	loop	fd_retry		/ try again if not at retry limit

	data16 / use 32-bit data-size
	decl	%ecx			/ put -1 in %ecx
	data16 / use 32-bit data-size
	addr16 / use 32-bit address-size
	movl	%ecx, [blkno-_data](%ebp) / set blkno to -1 to indicate failure

	jmp	read_ret

read_ok:
	data16 / use 32-bit data-size
	popl	%ebp

	data16 / use 32-bit data-size
	popl	%ecx

read_ret:
	data16 / use 32-bit data-size
	jmp	do_C_ret

/-------------------------------------------------------------------------------
/
/	Read-only data.
/
	.section .rodata

/	Driver name string, pointed to by d_name.

.fd_name: .string "fd"

/-------------------------------------------------------------------------------
/
/	Local variables.
/
	.data

/	The following values are floppy defaults in case the BIOS does
/	not properly support the "read disk parameters" function.

_data:
spt:	.byte	0		/ disk sectors per track

/	Parameter block for communication between stage1 and stage2.

stage1parms:
	.value	STAGE1_BASE\/1024	/ p1_memused_lo
	.value	STAGE1_SIZE\/1024	/ p1_memused_nbytes
	.value	0			/ p1_memused2_lo
	.value	0			/ p1_memused2_nbytes
	.value	STAGE1_END\/1024	/ p1_memavail_lo
	.value	[FREERAM_END-STAGE1_END]\/1024 / p1_memavail_nbytes
	.long	abort		/ p1_abortf: abort function pointer

/	Driver parameter block for the floppy disk. Used by stage2.
/	CONTINUATION OF stage1parms.

	.long	.fd_name	/ d_name: driver name/unit string
	.long	0		/ d_params: NULL for no device-specific params
unit:	.byte	BOOTDRV		/ d_unit: unit number
	.byte	D_REMOVABLE	/ d_flags: device type flags
blksz:	.value	0		/ d_blksz: block size (set to spt*bps)
	.long	0		/ d_base: logical start of data
blkno:	.long	0		/ d_blkno: current block (track) #
buffer:	.long	TRACK0_BUF	/ d_buffer: data buffer linear address
	.long	fd_read		/ d_read: driver read routine (linear address)
	.long	0		/ d_newdev: new-device routine not supported
	.long	0		/ d_flush: flush routine not supported

/	Driver parameter block for the simple console driver. Used by stage2.
/	CONTINUATION OF stage1parms.

	.long	cons_putc	/ d_putc: driver put character routine

/	BIOS call support. Used by platform-specific Boot-Loadable Modules.
/	Platform-specific CONTINUATION of stage1parms.

	.long	bios_intcall

/	Call a protected-mode function from real-address mode.
/	Platform-specific CONTINUATION of stage1parms.

	.long	protcall
