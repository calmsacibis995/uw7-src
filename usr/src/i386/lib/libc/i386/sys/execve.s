.ident	"@(#)libc-i386:sys/execve.s	1.5"
	.file	"execve.s"

_fwdef_(`execve'):
_m4_ifdef_(`PROF',`
	_prologue_
	MCOUNT			/ subroutine entry counter if profiling
	pushl	$0		/ prevent SIGPROF killing the child
	call	_fref_(_dprofil)
	addl	$4,%esp
	_epilogue_
')
	movl	$EXECE,%eax
	lcall	$0x7,$0
_m4_ifdef_(`PROF',`
	pushl	%eax		/ save to-be errno value
	_prologue_
	pushl	$1		/ reset SIGPROF since the exec failed
	call	_fref_(_dprofil)
	addl	$4,%esp
	_epilogue_
	popl	%eax
')
	jmp	_cerror
