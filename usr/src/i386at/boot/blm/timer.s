	.file	"timer.s"

	.ident	"@(#)stand:i386at/boot/blm/timer.s	1.3"
	.ident	"$Header$"

/ 	Real-mode support for INT 1C timer interrupt hook.
/
/	Entered in real-address mode, from the INT 1C vector.
/	Must preserve all registers, and call _timer() in protected mode.
/

	.text

	.globl RMint1C
RMint1C:
	/ ENTERED IN REAL-ADDRESS MODE

	push	%ds
	push	%es

	data16 / use 32-bit data-size
	pushl	%eax
	data16 / use 32-bit data-size
	pushl	%ecx
	data16 / use 32-bit data-size
	pushl	%edx

	/ Just in case some BIOS changes %ss on us, skip the protcall;
	/ we'll suffer rough animation, but we would die otherwise.
	movw	%ss, %ax
	or	%eax, %eax
	jnz	skip

	data16 / use 32-bit data-size
	movl	$_timer, %edx
	data16 / use 32-bit data-size
	.globl _protcall1C
_protcall1C:
	call	.	/ This will be patched with address of RMprotcall

skip:
	data16 / use 32-bit data-size
	popl	%edx
	data16 / use 32-bit data-size
	popl	%ecx
	data16 / use 32-bit data-size
	popl	%eax

	pop	%es
	pop	%ds

	iret

	.globl int1C_end
int1C_end:
