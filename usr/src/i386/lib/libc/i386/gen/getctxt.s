#ident	"@(#)libc-i386:gen/getctxt.s	1.9"

	.file	"getctxt.s"

	.globl	_cerror

_m4_define_(`GETCONTEXT', `0')
/ UR_foo macros are offset into ucontext_t of register foo
_m4_define_(`UR_EAX', `80')
_m4_define_(`UR_EIP', `92')
_m4_define_(`UR_ESP', `104')

_m4_ifdef_(`GEMINI_ON_OSR5',`
_m4_define_(`UR_SIGMASK1', `12')
_m4_define_(`UR_SIGMASK2', `16')
_m4_define_(`UR_SIGMASK3', `20')
_m4_define_(`UR_PRIVATEDATAP', `492')
',`')


/ This implementation of getcontext() attempts to get every register
/ stored properly in the ucontext structure, including the ones that
/ aren't strictly meaningful.  These "scratch" registers don't really
/ need to be saved since they have undefined values upon return from
/ getcontext(), but they might be useful for debugging.

_fwdef_(`getcontext'):
	pushl	%edx
	MCOUNT
	popl	%edx
	movl	4(%esp),%eax
	pushl	%eax		/ push ucp arg
	pushl	$GETCONTEXT	/ push flag arg
	pushl	$0		/ push dummy return address

	/ at this point, all registers contain the caller's values
	/ except for EAX, EIP, and ESP.
	movl	$UCONTEXT,%eax
	lcall	$0x7,$0		/ make syscall

	popl	%ecx		/ discard dummy return address
	popl	%ecx		/ discard flag arg
	popl	%ecx		/ get ucp in ECX
	jc	_cerror
	/ Now we patch up the three incorrect registers
	movl	$0,UR_EAX (%ecx)   / patch EAX value
	popl	%edx		   / get return address (EIP)
	movl	%edx,UR_EIP (%ecx) / patch EIP value
	movl	%esp,UR_ESP (%ecx) / patch ESP value
_m4_ifdef_(`GEMINI_ON_OSR5',`
	/ fill in values not set by osr5 kernel
	movl	$0,UR_SIGMASK1 (%eax) / patch uc_sigmask.sa_sigbits
	movl	$0,UR_SIGMASK2 (%eax) 
	movl	$0,UR_SIGMASK3 (%eax)
	movl	$0,UR_PRIVATEDATAP (%eax) / patch uc_privatedatap
',`')
	movl	$0,%eax
	jmp	*%edx
