.ident	"@(#)libc-i386:sys/sysinfo.s	1.5"


_m4_ifdef_(`GEMINI_ON_OSR5',,_m4_ifdef_(`GEMINI_ON_UW2',,``
	.file	"sysinfo.s"

	.text

	.globl	_cerror

_m4_ifdef_(`DSHLIB',`
	.weak	_abi_sysinfo
	.type	_abi_sysinfo,@function
_abi_sysinfo:
',`')
_fwdef_(`sysinfo'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SYSINFO,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
''))
