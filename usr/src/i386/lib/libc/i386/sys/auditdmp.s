/ C library -- auditdmp
.ident	"@(#)libc-i386:sys/auditdmp.s	1.2"

/ auditdmp(arecp);

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl  _cerror

_fwdef_(`auditdmp'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITDMP,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
