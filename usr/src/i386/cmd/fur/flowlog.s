#ident	"@(#)fur:i386/cmd/fur/flowlog.s	1.1"
.globl blocklog_END_TYPE
.globl blocklog_JO
.globl blocklog_JNO
.globl blocklog_JB
.globl blocklog_JAE
.globl blocklog_JE
.globl blocklog_JNE
.globl blocklog_JBE
.globl blocklog_JA
.globl blocklog_JS
.globl blocklog_JNS
.globl blocklog_JP
.globl blocklog_JNP
.globl blocklog_JL
.globl blocklog_JGE
.globl blocklog_JLE
.globl blocklog_JG

.weak	blocklog_JUMP
.set	blocklog_JUMP,blocklog_notarg
.weak	blocklog_RET
.set	blocklog_RET,blocklog_notarg
.weak	blocklog_FALL_THROUGH
.set	blocklog_FALL_THROUGH,blocklog_notarg
.weak	blocklog_CALL
.set	blocklog_CALL,blocklog_notarg
.weak	blocklog_PSEUDO_CALL
.set	blocklog_PSEUDO_CALL,blocklog_notarg
.weak	blocklog_IJUMP
.set	blocklog_IJUMP,blocklog_notarg
.weak	blocklog_PUSH
.set	blocklog_PUSH,blocklog_notarg
.weak	blocklog_POP
.set	blocklog_POP,blocklog_notarg

blocklog_END_TYPE:
blocklog_JO:
	jo blocklog_log_t
	jmp blocklog_log_f
blocklog_JNO:
	jno blocklog_log_t
	jmp blocklog_log_f
blocklog_JB:
	jb blocklog_log_t
	jmp blocklog_log_f
blocklog_JAE:
	jae blocklog_log_t
	jmp blocklog_log_f
blocklog_JE:
	je blocklog_log_t
	jmp blocklog_log_f
blocklog_JNE:
	jne blocklog_log_t
	jmp blocklog_log_f
blocklog_JBE:
	jbe blocklog_log_t
	jmp blocklog_log_f
blocklog_JA:
	ja blocklog_log_t
	jmp blocklog_log_f
blocklog_JS:
	js blocklog_log_t
	jmp blocklog_log_f
blocklog_JNS:
	jns blocklog_log_t
	jmp blocklog_log_f
blocklog_JP:
	jp blocklog_log_t
	jmp blocklog_log_f
blocklog_JNP:
	jnp blocklog_log_t
	jmp blocklog_log_f
blocklog_JL:
	jl blocklog_log_t
	jmp blocklog_log_f
blocklog_JGE:
	jge blocklog_log_t
	jmp blocklog_log_f
blocklog_JLE:
	jle blocklog_log_t
	jmp blocklog_log_f
blocklog_JG:
	jg blocklog_log_t
	jmp blocklog_log_f

blocklog_log_t:
	pushl	%ecx
	movl	4(%esp),%ecx
	movl	%eax,4(%esp)
	movl	8(%esp),%eax
	movl	%ecx,8(%esp)
	pushf
ifdef(`KERNEL', `
		movw	%ds,%cx
		cmpl	$0x108,%ecx
		jne		nolog1
', `
		cmpl	$0,BlockCount
		je		reinit1
continue1:
')
	leal   (%eax,%eax,2),%eax
	shll   $2,%eax
ifdef(`KERNEL', `
		addl	$BlockCount,%eax
',`
		addl	BlockCount,%eax
')
	incl	(%eax)
	incl	8(%eax)
	popf
	popl	%ecx
	popl	%eax
	ret	
ifdef(`KERNEL', `', `
reinit1:
	pushl %eax
	call blockinit
	popl %eax
	jmp continue1
')
ifdef(`KERNEL', `
nolog1:
	popf
	popl	%ecx
	popl	%eax
	ret	
')
blocklog_log_f:
	pushl	%ecx
	movl	4(%esp),%ecx
	movl	%eax,4(%esp)
	movl	8(%esp),%eax
	movl	%ecx,8(%esp)
	pushf
ifdef(`KERNEL', `
		movw	%ds,%cx
		cmpl	$0x108,%ecx
		jne		nolog2
', `
		cmpl	$0,BlockCount
		je		reinit2
continue2:
')
	leal   (%eax,%eax,2),%eax
	shll   $2,%eax
ifdef(`KERNEL', `
		addl	$BlockCount,%eax
',`
		addl	BlockCount,%eax
')
	incl	(%eax)
	addl	$4,%eax
	incl	(%eax)
	popf
	popl	%ecx
	popl	%eax
	ret	
ifdef(`KERNEL', `', `
reinit2:
	pushl %eax
	call blockinit
	popl %eax
	jmp continue2
')
ifdef(`KERNEL', `
nolog2:
	popf
	popl	%ecx
	popl	%eax
	ret	
')
blocklog_notarg:
	pushl	%ecx
	movl	4(%esp),%ecx
	movl	%eax,4(%esp)
	movl	8(%esp),%eax
	movl	%ecx,8(%esp)
	pushf
ifdef(`KERNEL', `
		movw	%ds,%cx
		cmpl	$0x108,%ecx
		jne		nolog3
', `
		cmpl	$0,BlockCount
		je		reinit3
continue3:
')
	leal   (%eax,%eax,2),%eax
	shll   $2,%eax
ifdef(`KERNEL', `
		addl	$BlockCount,%eax
',`
		addl	BlockCount,%eax
')
	incl	(%eax)
	popf
	popl	%ecx
	popl	%eax
	ret	
ifdef(`KERNEL', `', `
reinit3:
	pushl %eax
	call blockinit
	popl %eax
	jmp continue3
')
ifdef(`KERNEL', `
nolog3:
	popf
	popl	%ecx
	popl	%eax
	ret	
')
