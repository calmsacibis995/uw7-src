.ident	"@(#)libc-i386:sys/setrlimit.s	1.3"

	.file	"setrlimit.s"
	
_fwdef_(`setrlimit64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SETRLIMIT64,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

_fwdef_(`setrlimit32'):
_fwdef_(`setrlimit'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SETRLIMIT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
