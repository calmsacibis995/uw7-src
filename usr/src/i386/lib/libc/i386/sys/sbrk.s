.ident	"@(#)libc-i386:sys/sbrk.s	1.7"

	.file	"sbrk.s"

	.text

	.globl	_nd
	.globl	_gbrk

_fgdef_(_gbrk):
	_prologue_
	MCOUNT			/ subroutine entry counter if profiling
	movl	_esp_(4),%edx
	cmpl	$0,%edx
	jz	.is_zero	/ We know the answer without asking
	jl	.is_neg
_m4_ifdef_(`DSHLIB',
`	movl	_daref_(_nd),%ecx
	addl	(%ecx),%edx
',
`	addl	_nd,%edx
')
	jno	.sbrk
.is_neg:
	movl	$12,%eax	/ ENOMEM
	_epilogue_
	jmp	_cerror
	
_fwdef_(`sbrk'):
	_prologue_
	MCOUNT			/ subroutine entry counter if profiling
	movl	_esp_(4),%edx
	testl	%edx,%edx
	jz	.is_zero	/ We know the answer without asking
_m4_ifdef_(`DSHLIB',
`	movl	_daref_(_nd),%ecx
 	addl	(%ecx),%edx
',
`	addl	_nd,%edx
')
.sbrk:
	pushl	%edx
	call	brk
	addl	$4,%esp
	testl	%eax,%eax
	jnz	.brkerr
.is_zero:
_m4_ifdef_(`DSHLIB',
`	movl	_daref_(_nd),%ecx
	movl	(%ecx),%eax
',
`	movl	_nd,%eax
')
	subl	_esp_(4),%eax
.brkerr:
	_epilogue_
	ret


/ brk(value)
/ as described in brk(2).
/ returns 0 for ok, -1 for error

	.globl	_cerror

_fwdef_(`brk'):
	movl	$17,%eax
	lcall	$0x7,$0
	jc	_cerror
	_prologue_
	movl	_esp_(4),%edx
_m4_ifdef_(`DSHLIB',
`	movl	_daref_(_nd),%ecx
	movl	%edx,(%ecx)
',
`	movl    %edx,_nd
')
	xorl	%eax,%eax
	_epilogue_
	ret
_m4_ifdef_(`DSHLIB',
`',
`	.data
_nd:
	.long	end
')
