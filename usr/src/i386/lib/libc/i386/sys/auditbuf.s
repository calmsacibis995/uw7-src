/ C library -- auditbuf
.ident	"@(#)libc-i386:sys/auditbuf.s	1.2"

/ auditbuf(cmd, abufp);

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`auditbuf'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITBUF,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
