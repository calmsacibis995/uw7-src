.ident	"@(#)libc-i386:sys/lwpcreate.s	1.4"

/
/ lwpid_t 
/ _lwp_create(ucontext_t *context, unsigned long flags, lwpid_t *new_lwpid)
/	The _lwp_create(2) system call. Creates an LWP within the process
/	of the calling context. The newly created context will execute 
/	the context pointed to by the context parameter.
/
/ Calling/Exit State:
/	On success the function returns 0 and the ID of the created LWP
/	is returned in the out argument. On failure errno is returned.
/

	.file	"lwpcreate.s"
	.text

_fwdef_(`_lwp_create'):
_m4_ifdef_(`_REENTRANT',`
	_prologue_
	MCOUNT
	movl	_daref_(__lwp_priv_datap),%eax
	cmpl	$0,(%eax)
	jne	.L1
	call	_fref_(__thr_init)
.L1:
	_epilogue_
',`
	MCOUNT
')
	movl	$LWPCREATE,%eax
	lcall	$0x7,$0
	jae 	.noerror		/ all OK - normal return
	ret			/  otherwize, return error

.noerror:
	movl	12(%esp),%ecx	/ address of the out arg
	movl	%eax, (%ecx)	/ move the id out
	xorl	%eax, %eax	/ set return value to 0
	ret
