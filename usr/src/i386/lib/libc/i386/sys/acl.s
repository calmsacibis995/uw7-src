/ C library -- acl
.ident	"@(#)libc-i386:sys/acl.s	1.2"

/ acl(file, cmd, nentries, buffer)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`acl'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$ACL,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
