/
/ Common start up code for applications.
/ Included by the variations (crt1.s, mcrt1.s, pcrt1.s).
/
	.ident	"@(#)libc-i386:csu/csu.s	1.2"

/ _start - main entry for "static" applications; jumped to by rtld.
/
/ 1. set up dead-end stack frame for tracing/debugging.
/ 2. set environ and ___Argv global pointers (rtld might also have done so).
/ 3. optionally call _rt_pre_init() if rtld hasn't done so.
/ 4. register atexit() functions: _cleanup(), _rt_do_exit(), and _fini().
/ 5. call _init() [for the application] and __fpstart().
/ 6. call main(argc, argv, envp).
/ 7. call exit(%eax), main()'s return.
/ 8. in case exit() returns [it shouldn't], fall into inlined _exit().

/ Since there's no previous stack frame (even when from rtld), we are
/ free to modify the callee-saved registers (%esi, %edi, %ebx) without
/ having to save their previous values.

	.comm	___Argv, 4, 4
	.weak	_cleanup
	.weak	_DYNAMIC

	.globl	_start
_start:
	pushl	$0		/ null return address (stack tracing)
	pushl	$0		/ null previous stack frame (%ebp)
	movl	%esp, %ebp	/ first stack frame
	pushl	%edx		/ possibly the address of _rt_do_exit()

	movl	8(%ebp), %eax		/ argc
	leal	12(%ebp), %ebx		/ argv
	leal	16(%ebp,%eax,4), %edi	/ envp

	movl	$_DYNAMIC, %esi	/ nonzero if rtld has been run
	testl	%esi, %esi
	jnz	.did_rtld
	movl	%ebx, ___Argv
	movl	%edi, environ
_m4_ifdef_(`_RT_PRE_INIT',`
	call	_rt_pre_init	/ it was not already called by rtld
')
.did_rtld:

	movl	$_cleanup, %eax	/ nonzero if _cleanup() is defined
	testl	%eax, %eax
	jz	.nocleanup
	pushl	%eax
	call	atexit
	addl	$4, %esp
.nocleanup:

	testl	%esi, %esi
	jz	.nodoexit
	call	atexit		/ already pushed _rt_do_exit()'s address
.nodoexit:

	pushl	$_fini
	call	atexit
	movl	%ebp, %esp	/ clean up stack

	call	_init
	call	__fpstart

	pushl	%edi		/ envp
	pushl	%ebx		/ argv
	pushl	8(%ebp)		/ argc
	call	main
	addl	$12, %esp	/ clean up stack (for debugger's sake)

	pushl	%eax		/ main() returned: call exit()
	call	exit		/ not supposed to return...

	pushl	$0		/ spare word for return address
	movl	$EXIT, %eax
	lcall	$0x7, $0
	hlt			/ we're out of here!
	.type	_start, "function"
	.size	_start, .-_start
