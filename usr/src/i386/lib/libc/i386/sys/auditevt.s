/ C library -- auditevt
.ident	"@(#)libc-i386:sys/auditevt.s	1.2"

/ auditevt(cmd, aevtp)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`auditevt'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITEVT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
