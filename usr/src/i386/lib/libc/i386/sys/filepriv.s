/ C library -- filepriv
.ident	"@(#)libc-i386:sys/filepriv.s	1.1"

/ filepriv(const char *path, int cmd, priv_t *privp, int count)

	.globl	_cerror

_fwdef_(`filepriv'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FILEPRIV,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
