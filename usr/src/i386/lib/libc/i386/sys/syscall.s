.ident	"@(#)libc-i386:sys/syscall.s	1.8"


	.file	"syscall.s"

	.text

	.globl	_cerror


_m4_ifdef_(`ABI',`
	.globl	syscall
_fgdef_(syscall):
',`
_m4_ifdef_(`DSHLIB',`
	.globl	syscall
_fgdef_(syscall):
',`
_fwdef_(`syscall'):
')
')
	MCOUNT			/ subroutine entry counter if profiling
	pop	%edx		/ return address.
	pop	%ecx		/ system call number
	pushl	%edx
.restart:
	movl	%ecx, %eax
	lcall	$0x7,$0
	jnc	.noerror
	cmpb	$ERESTART, %al
	je	.restart
	movl	(%esp), %edx	/ restore expected stack depth...
	pushl	%edx		/ ...with the return address
	jmp	_cerror
.noerror:
	movl	(%esp), %edx	/ restore expected stack depth...
	pushl	%edx		/ ...with the return address
	ret
