	.file	"swapctxt.s"
	.ident	"@(#)libc-i386:gen/swapctxt.s	1.1"
	.set	UC_MCONTEXT_GREGS,36	/ offset of context reg array
	.set	GREG_EAX,44		/ offset of context reg array[R_EAX]
	.set	GREG_EIP,56		/ offset of context reg array[R_EIP]
	.set	GREG_ESP,68		/ offset of context reg array[R_ESP]
	.text
	.align	4
_fwdef_(`swapcontext'):	/ minimize changes to non-scratch registers
	MCOUNT
	movl	8(%esp),%ecx	/ save new context pointer
	movl	4(%esp),%eax	/ setup for __getcontext(old context pointer)
	movl	$0,4(%esp)	/   => ucontext(0, pointer)
	movl	%eax,8(%esp)
	movl	$UCONTEXT,%eax
	lcall	$0x7,$0
	jc	_cerror
	movl	8(%esp),%eax	/ hold old context pointer
	movl	$1,4(%esp)	/ setup for _setcontext(new context pointer)
	movl	%ecx,8(%esp)	/   => ucontext(1, pointer)
	/ change old context %eip, %esp, %eax to be as if had just returned 0
	leal	UC_MCONTEXT_GREGS(%eax),%ecx
	movl	(%esp),%eax
	movl	%eax,GREG_EIP(%ecx)
	leal	4(%esp),%eax
	movl	%eax,GREG_ESP(%ecx)
	movl	$0,GREG_EAX(%ecx)
	movl	$UCONTEXT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret	/ not reached as setcontext only returns on failure
