	.file	"rawboot.s"

	.ident	"@(#)stand:i386at/boot/blm/rawboot.s	1.2"
	.ident	"$Header$"

/ 	Real-mode support for "raw" booting.
/
/	Entered in real-address mode, with registers set appropriately
/	for entering a standard PC-AT bootstrap. The address of the
/	bootstrap block in memory is passed in %ebx (32-bit address).
/
/	Copies the bootstrap from *%ebx to its expected load location at
/	BOOT_LOADADDR, then jumps to this bootstrap, at BOOT_LOADADDR
/	(with %ds = 0).
/
/	Preserves the following registers for use by the bootstrap: %si, %dl.
/	Resets stack pointer to BOOT_LOADADDR.
/

	.set BOOT_LOADADDR, 0x7C00
	.set BOOT_SIZE, 512

	.text

	.globl rawboot
rawboot:
	/ ENTERED IN REAL-ADDRESS MODE

	data16 / use 32-bit data-size
	movl	$BOOT_LOADADDR, %esp

	mov	%esp, %edi

	push	%esi

	movl	%ebx, %esi

	data16 / use 32-bit data-size
	movl	$BOOT_SIZE, %ecx

	rep; smovb

	pop	%esi

	jmp	*%esp

	.globl rawboot_end
rawboot_end:
