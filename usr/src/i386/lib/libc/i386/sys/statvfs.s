.ident	"@(#)libc-i386:sys/statvfs.s	1.2"

	.file	"statvfs.s"

_fwdef_(`statvfs64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$STATVFS64,%eax
	lcall	$0x7,$0
	jc	_cerror
	xorl	%eax,%eax
	ret

_fwdef_(`statvfs32'):
_fwdef_(`statvfs'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$STATVFS,%eax
	lcall	$0x7,$0
	jc	_cerror
	xorl	%eax,%eax
	ret
