.ident	"@(#)libc-i386:sys/lsemapost.s	1.1"

	.file	"lwpsemapost.s"

	.text


_fwdef_(`_lwp_sema_post'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LWPSEMAPOST,%eax
	lcall	$0x7,$0
	ret
