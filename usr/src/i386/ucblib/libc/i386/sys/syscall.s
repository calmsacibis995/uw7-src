#ident	"@(#)ucb:i386/ucblib/libc/i386/sys/syscall.s	1.2"
#ident	"$Header$"
/		copyright	"%c%"

.ident		"@(#)ucb:i386/ucblib/libc/i386/sys/syscall.s	1.2"

	.file	"syscall.s"

	.text

	.globl	_cerror

/ **************************************************************
/ ***
/ ***	NOTE: if you're going to do m4 preprocessing on this file
/ ***	in the same fashion as is used for libc, uncomment the lines
/ ***	up to 'ELSE'. Otherwise, uncomment the lines between
/ ***	'ELSE' and 'ENDIF'
/ ***
/ ***	IF
/
/_m4_ifdef_(`ABI',`
/	.globl	syscall
/_fgdef_(syscall):
/',`
/_m4_ifdef_(`DSHLIB',`
/	.globl	syscall
/_fgdef_(syscall):
/',`
/_fwdef_(`syscall'):
/')
/')
/ ***	ELSE
	.globl	syscall
/ ***	ENDIF

/	 MCOUNT			/ subroutine entry counter if profiling
	pop	%edx		/ return address.
	pop	%eax		/ system call number
	pushl	%edx
	lcall	$0x7,$0
	movl	0(%esp),%edx
	pushl	%edx		/ Add an extra entry to the stack
	jc	_cerror
	ret

