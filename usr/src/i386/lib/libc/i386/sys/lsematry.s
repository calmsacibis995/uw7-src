.ident	"@(#)libc-i386:sys/lsematry.s	1.1"

	.file	"lwpsematry.s"

	.text


_fwdef_(`_lwp_sema_trywait'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LWPSEMATRYWAIT,%eax
	lcall	$0x7,$0
	ret
