	.file	"osrcrt1.s"
	.ident	"@(#)libc-i386:csu/osrcrt1.s	1.2"

_m4_define_(`_RT_PRE_INIT')
_m4_include_(`csu/csu.s')

/ This _rt_pre_init() shunts this program off to an alternate version,
/ if OSRCMDS is in the environment and /OpenServer/bin/"basename $0"
/ is the pathname of a regular executable file.

	.section .rodata
	.align	4
.envstr:
	.string	"OSRCMDS"
	.align	4
.bindir:
	.string	"/OpenServer/bin/"
	.set	.binlen, .-.bindir
	.text

	.align	4
	.globl	_rt_pre_init
_rt_pre_init:
	pushl	$.envstr
	call	getenv
	addl	$4, %esp
	testl	%eax, %eax
	jnz	.gotvar
	ret

	/ Have an OSRCMDS environment variable, set up stack frame
.gotvar:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/ Get %ebx pointing to basename(argv[0]).
	movl	___Argv, %eax
	pushl	$0x2f		/ slash byte
	movl	(%eax), %ebx
	pushl	%ebx
	call	strrchr
	testl	%eax, %eax
	jz	.noslash
	leal	1(%eax), %ebx
.noslash:

	/ Store "/OpenServer/bin/"+strlen(%ebx)+"\0" on the stack
	pushl	%ebx
	call	strlen
	leal	.binlen(%eax), %ecx
	subl	%ecx, %esp
	movl	$.bindir, %esi
	movl	$.binlen-1, %ecx
	movl	%esp, %edi
	rep;	smovb
	leal	1(%eax), %ecx
	movl	%ebx, %esi
	rep;	smovb

	/ Is this pathname a regular executable file?  If so, execv it.
	movl	%esp, %esi
	pushl	$0x18		/ EX_OK|EFF_ONLY_OK
	pushl	%esi
	call	_access
	testl	%eax, %eax
	jnz	.noexec

	pushl	___Argv
	pushl	%esi
	call	_execv		/ should not return

.noexec:
	leal	-12(%ebp), %esp	/ just after register saves
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
	.type	_rt_pre_init, "function"
	.size	_rt_pre_init, .-_rt_pre_init
