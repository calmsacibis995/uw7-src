	.file	"rmcall.s"

	.ident	"@(#)stand:i386at/boot/blm/rmcall.s	1.3"
	.ident	"$Header$"

/ 	Support for generic real-mode calls.
/
/	This stub code is prepended to the front of the caller's code,
/	and is responsible for restoring the INT 0 vector, which was
/	"borrowed" in order to jump into this code.
/

	.text

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
