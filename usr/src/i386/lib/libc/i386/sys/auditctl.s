/ C library -- auditctl
.ident	"@(#)libc-i386:sys/auditctl.s	1.2"

/ auditctl(cmd, actlp);

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`auditctl'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITCTL,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
