/ C library -- auditlog
.ident	"@(#)libc-i386:sys/auditlog.s	1.2"

/ auditlog(cmd, alogp);

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`auditlog'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$AUDITLOG,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
