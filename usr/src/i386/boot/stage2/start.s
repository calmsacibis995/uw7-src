	.file	"start.s"

	.ident	"@(#)stand:i386/boot/stage2/start.s	1.1"
	.ident	"$Header$"

/	Start of Stage 2 UNIX bootstrap.
/
/	Must be first file linked.

	.text

	.globl	_start
_start:
	call	.L1			/ only way to get IP value
.L1:
	subl	$.L1, (%esp)		/ compute relocation offset
	call	relocate_self		/ relocate_self(reloff)
	popl	%eax

	jmp	stage2

	.size	_start, . - _start
