	.file	"mcrt1.s"
	.ident	"@(#)libc-i386:csu/mcrt1.s	1.27"

_m4_define_(`_RT_PRE_INIT')
_m4_include_(`csu/csu.s')

	.align	4
	.local	.pfinish
.pfinish: / shut down profiling
	pushl	$0
	call	_newmon		/ stop profiling; write data
	popl	%ecx
	ret
	.type	.pfinish, "function"
	.size	.pfinish, .-.pfinish

	.align	4
	.globl	_rt_pre_init
_rt_pre_init:
	pushl	$.pfinish	/ register shutdown function
	call	atexit
	pushl	$.eprol		/ start of rest of application
	call	_newmon
	addl	$8, %esp	/ clean up for both calls
	ret
	.type	_rt_pre_init, "function"
	.size	_rt_pre_init, .-_rt_pre_init

	.section .rodata
	.align	4
.rtld.event:
	.long	_SOin	/ rtld will call after linkmap changes
	.globl	.rtld.event
	.type	.rtld.event, "object"
	.size	.rtld.event, 4

	.text
	.align	4
.eprol:			/ beginning of application text
