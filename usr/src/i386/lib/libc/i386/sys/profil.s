.ident	"@(#)libc-i386:sys/profil.s	1.5"

	.file	"profil.s"

_fwdef_(`profil'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PROFIL,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
