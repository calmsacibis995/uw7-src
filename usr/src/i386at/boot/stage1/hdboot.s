	.file	"hdboot.s"

	.ident	"@(#)stand:i386at/boot/stage1/hdboot.s	1.8"
	.ident	"$Header$"

/ 	Stage 1 UNIX bootstrap -- hard disk version
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

	.set	DRVBASE, 0x80		/ base drive number for hard drives
	.set	RETRIES, 5		/ max I/O error retries

	.set	BLKSZ, 512
	.set	EXTRA, 0x100		/ extra buffer space for rmcall()
	.set	BUF_NBLKS, 32		/ DON'T CHANGE THIS!
	.set	STAGE2_NBLKS, 20

	.set	STAGE1_BASE, 0x800
	.set	STAGE1_LOADADDR, 0x7C00
	.set	STAGE1_LOADSIZE, BLKSZ
	.set	STAGE1B_LOADADDR, [STAGE1_LOADADDR+STAGE1_LOADSIZE]
	.set	STAGE1B_LOADSIZE, BLKSZ
	.set	STAGE2_LOADADDR, [STAGE1B_LOADADDR+STAGE1B_LOADSIZE]
	.set	DISK_BUF, [STAGE1_LOADADDR-[BLKSZ\*BUF_NBLKS]]
	.set	BOOTSTACK, [DISK_BUF-EXTRA]
	.set	STAGE1_END, [STAGE2_LOADADDR+[BLKSZ\*STAGE2_NBLKS]]
	.set	STAGE1_SIZE, [STAGE1_END-STAGE1_BASE]
	.set	FREERAM_END, [511\*1024]

	/ fdisk partition table (see fdisk.h)
	.set	FD_PARTS, 0x7BE		/ location of partition table in RAM
	.set	FD_PARTSZ, 16		/ size of a partition table entry
	.set	FD_NPARTS, 4		/ number of entries in table
	.set	FD_BOOTID, 0		/ offset to bootid member
	.set	FD_ACTIVE, 128		/ value of bootid for active partition
	.set	FD_SYSTID, 4		/ offset to systid member
	.set	FD_UNIXOS, 99		/ value of systid for UNIX
	.set	FD_RELSECT, 8		/ offset to relsect member

	/ device type flags (see boot.h)
	.set	D_REMOVABLE, [1 << 0]
	.set	D_VTOC, [1 << 1]
	.set	D_NET, [1 << 2]

	.text

	.globl	_start
_start:
	/ NOTE: Some previous versions began with a 'cli' and a jump over
	/ 4 bytes of "reserved space", labeled "drv_mark". I see no reason
	/ for these, so I've eliminated them. - kdg

	/ On entry, %ds:%si will be set to point to the selected partition,
	/ so we must preserve them. However, we know that a valid pointer
	/ must be within the table at FD_PARTS, which is well below 64K,
	/ so we can normalize it into a zero-relative 16-bit offset, and
	/ treat anything that doesn't fit as invalid.
	/
	/ We also need to preserve %dl, which will contain the drive number.
	/
	/ First, just get %ds into a safe place.
	movw	%ds, %bx
	
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

	/ Relative data references are smaller in code size,
	/ so load up %ebp with a pointer to the start of our data.
	data16 / use 32-bit data-size
	movl	$_data, %ebp

	/ continue with partition pointer normalization
	shl	$4, %ebx
	add	%esi, %ebx
	addr16 / use 32-bit address-size
	mov	%ebx, [partptr-_data](%ebp)

	/ save drive number as unit
	movb	%dl, %al
	subb	$DRVBASE, %al
	addr16 / use 32-bit address-size
	movb	%al, [unit-_data](%ebp)

/ 	Get the boot drive parameters.

	data16 / use 32-bit data-size
	pushl	%ebp

	movb	$8, %ah			/ call the BIOS to get drive parms
	int	$0x13

	data16 / use 32-bit data-size
	call	goprot

	popl	%ebp

	andb	$0x3F, %cl		/ extract max sector number
	movb	%cl, [spt-_data](%ebp)	/ store in spt
	movb	%cl, %al
	incb	%dh			/ get max head + one for # heads (tpc)
	mulb	%dh			/ compute spt * tpc
	movw	%ax, [spc-_data](%ebp)	/ store in spc

	movl	[partptr-_data](%ebp), %ebx

	/ Store base of selected partition into d_base, and adjust starting
	/ block number for loading stage 2.
	movl	FD_RELSECT(%ebx), %eax
	addl	%eax, [blkno-_data](%ebp) / adjust stage 2 sector address
	shll	$9, %eax		  / FD_RELSECT is in 512-byte units
	movl	%eax, [base-_data](%ebp)  / but d_base wants bytes

	/ Load stage2 (the second stage bootstrap) into the preallocated buffer.
	/ blkno, buffer, and nsec are already set appropriately.
	/ Don't bother to check for an error; we couldn't do anything useful.
	call	hd_read

	/ Fix up the device name string to reflect the correct partition number
	call	set_partnum

	/ Reset buffer to private disk buffer
	movw	$DISK_BUF, [buffer-_data](%ebp)
	movb	$1, [nsec-_data](%ebp)
	/ (SLEAZY HACK: We should set blkno to -1 since we've changed the
	/ buffer pointer, but we know stage 2 won't try to read this same
	/ block, so it'll change it anyway.)

	/ Pass pointer to stage1 parameters as "argument" to stage2 entry point.
	leal	[stage1parms-_data](%ebp),%eax
	pushl	%eax

	/ Invoke stage2. Start address is first byte at load address.
	/ Invocation is as a procedure call, but it never returns.
	movw	$STAGE2_LOADADDR, %ax	/ (assumes upper 1/2 of %eax zero)
	call	*%eax
	/ NOTREACHED

/-------------------------------------------------------------------------------
/
/	Hard disk driver read routine.
/
/	Read nsec sectors into the current buffer.
/
hd_read:
	/ ENTERED IN PROTECTED MODE

	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	$_data, %ebp

	/ SWITCH TO REAL-ADDRESS MODE (and clear regs)
	call	goreal_clear
	sti				/ enable interrupts in real-mode

	// Assumes rest of %ecx zero (from goreal)
	movb	$RETRIES-1, %cl		/ set retry counter (%cx)

hd_retry:
	data16 / use 32-bit data-size
	pushl	%ecx			/ save the retry counter

	/ translate blkno into (cyl, head, sector)
	addr16 / use 32-bit address-size
	mov	[blkno-_data](%ebp), %eax
	addr16 / use 32-bit address-size
	mov	[blkno+2-_data](%ebp), %edx
	addr16 / use 32-bit address-size
	div	[spc-_data](%ebp)	/ (blkno/spc): %ax <- cyl; %dx <- cylsec
	mov	%eax, %ecx
	mov	%edx, %eax		/ move cylsec (sector within cylinder)
	cltd / (cwtd)			/ 	into %dx:%ax
	addr16 / use 32-bit address-size
	div	[spt-_data](%ebp)	/ (cylsec/spt): %ax <- head; %dx <- sec

	/ set up arguments to disk BIOS call
	movb	%al, %dh		/ move head to %dh for BIOS call
	xchgb	%ch, %cl		/ move cyl:0-7 to ch for BIOS call
	shlb	$6, %cl			/ extract cyl:8-9
	orb	%dl, %cl		/ combine with sector in %cl for BIOS
	incb	%cl			/ BIOS wants sector number one-based
	addr16 / use 32-bit address-size
	movb	[nsec-_data](%ebp), %al	/ number of sectors
	addr16 / use 32-bit address-size
	mov	[buffer-_data](%ebp), %ebx / set buffer pointer (%es already 0)
	addr16 / use 32-bit address-size
	movb	[unit-_data](%ebp), %dl	/ convert unit # to drive #
	addb	$DRVBASE, %dl

	data16 / use 32-bit data-size
	pushl	%ebp

	/ read sector from disk using disk read BIOS call
	movb	$0x02, %ah
	int	$0x13

	jnc	read_ok

	cmpb	0x11, %ah		/ ECC corrected?
	je	read_ok			/ if so, ignore error

	/ reset controller and try again
	movb	$0, %ah			/ reset controller
	int	$0x13

	data16 / use 32-bit data-size
	popl	%ebp

	data16 / use 32-bit data-size
	popl	%ecx

	loop	hd_retry		/ try again if not at retry limit

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
	/ SWITCH BACK TO PROTECTED MODE
	data16 / 32-bit data-size
	call	goprot

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp

	ret

/-------------------------------------------------------------------------------
/
/	Read-only data.
/
	.section .rodata

/	Driver name string, pointed to by d_name.

.hd_name: .string "hd"

/-------------------------------------------------------------------------------
/
/	Local variables.
/
	.data

	.globl	partptr
	.globl	hd_param0

_data:
partptr: .long	0		/ normalized pointer to selected partition
spt:	.value	0		/ disk sectors per track
spc:	.value	0		/ sectors per cylinder (spt * tracks per cyl)
nsec:	.byte	STAGE2_NBLKS+1	/ number of sectors to read
hd_param0: .string "X"		/ initial device-specific parameters

/	Parameter block for communication between stage1 and stage2.

stage1parms:
	.value	STAGE1_BASE\/1024	/ p1_memused_lo
	.value	STAGE1_SIZE\/1024	/ p1_memused_nbytes
	.value	0			/ p1_memused2_lo
	.value	0			/ p1_memused2_nbytes
	.value	STAGE1_END\/1024	/ p1_memavail_lo
	.value	[FREERAM_END-STAGE1_END]\/1024 / p1_memavail_nbytes
	.long	abort		/ p1_abortf: abort function pointer

/	Driver parameter block for the hard disk. Used by stage2.
/	CONTINUATION OF stage1parms.

	.long	.hd_name	/ d_name: driver name/unit string
	.long	hd_param0	/ d_params: device-specific parameter string
unit:	.byte	0		/ d_unit: unit number
	.byte	D_VTOC		/ d_flags: device type flags
	.value	BLKSZ		/ d_blksz: block size
base:	.long	0		/ d_base: logical start of data
blkno:	.long	1		/ d_blkno: current block (sector) #
buffer:	.long	STAGE1B_LOADADDR / d_buffer: data buffer linear address
	.long	hd_read		/ d_read: driver read routine (linear address)
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
