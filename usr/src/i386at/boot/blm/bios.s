	.file	"bios.s"

	.ident	"@(#)stand:i386at/boot/blm/bios.s	1.4"
	.ident	"$Header$"

/ 	Support for making real-mode BIOS calls.
/
/	This file is the bridge between the C-language bioscall() routine
/	and the low-level assembly-language routine that runs in real-address
/	mode.
/
/	int _bios(struct biosregs *, void (*)());
/
/	The 1st arg points to a block of register values which are passed in
/	to the BIOS call and then updated to reflect the new register values
/	after the call.
/
/	The 2nd arg is a pointer to the low-level function.
/
/	Returns eflags bits.
/
/	Note that the only legal memory pointer values to pass in the registers
/	are pointers into the block driver's data buffer.

	.globl	_bios
	.type	_bios, @function
_bios:
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	20(%esp), %eax		/ struct biosregs *rp;
	movl	(%eax), %ebx		/ extract int number from arg

	pushl	(,%ebx,4)		/ extract vector and
					/ pass as single arg to low-level

	/ load up registers
	movl	8(%eax), %ebx
	movl	12(%eax), %ecx
	movl	16(%eax), %edx
	movl	20(%eax), %esi
	movl	24(%eax), %edi
	movl	28(%eax), %ebp
	movl	4(%eax), %eax

	call	*28(%esp)		/ call the low-level function

	xchgl	24(%esp), %eax		/ save eax value and load up rp pointer

	/ stuff new register values into rp
	movl	%ebx, 8(%eax)
	movl	%ecx, 12(%eax)
	movl	%edx, 16(%eax)
	movl	%esi, 20(%eax)
	movl	%edi, 24(%eax)
	movl	%ebp, 28(%eax)
	movl	24(%esp), %ebx
	movl	%ebx, 4(%eax)

	/ pop argument
	popl	%eax

	/ set return value based on EFL
	lahf
	movzbl	%ah, %eax

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp

	ret

	.size	_bios, . - _bios


/ 	Support for generic real-mode calls.
/
/	This stub code is prepended to the front of the caller's code,
/	and is responsible for restoring the INT 0 vector, which was
/	"borrowed" in order to jump into this code.
/
	.globl rmcall_stub
rmcall_stub:
	/ ENTERED IN REAL-ADDRESS MODE

	/ Restore INT 0 vector which we "borrowed" to get here.
	data16 / use 32-bit data-size
	addr16 / use 32-bit address-size
	movl	%eax, 0

	/ Fall through to caller's code...

	.globl rmcall_end
rmcall_end:
