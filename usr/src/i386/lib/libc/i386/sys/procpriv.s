/ C library -- procpriv
.ident	"@(#)libc-i386:sys/procpriv.s	1.1"

/ procpriv(int cmd, priv_t *privp, int count)

	.globl	_cerror

_fwdef_(`procpriv'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PROCPRIV,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
