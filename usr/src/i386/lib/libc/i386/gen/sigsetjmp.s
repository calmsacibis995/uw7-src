	.file	"sigsetjmp.s"

	.ident	"@(#)libc-i386:gen/sigsetjmp.s	1.7"

/	Sigsetjmp() is implemented in assembly language because it needs
/	to have direct control over register use.

_m4_define_(`uc_mcontext',36)
_m4_define_(`UC_ALL',0x1F)
_m4_define_(`UC_SIGMASK',0x01)
_m4_define_(`EAX',11)
_m4_define_(`UESP',17)
_m4_define_(`EIP',14)

_m4_ifdef_(`GEMINI_ON_OSR5',`
_m4_define_(`UR_EDI', `52')
_m4_define_(`UR_ESI', `56')
_m4_define_(`UR_EBP', `60')
_m4_define_(`UR_EBX', `68')
_m4_define_(`UR_EAX', `80')
_m4_define_(`UR_EIP', `92')
_m4_define_(`UR_ESP', `104')
_m4_define_(`UR_SIGMASK0', `8')
_m4_define_(`UR_SIGMASK1', `12')
_m4_define_(`UR_SIGMASK2', `16')
_m4_define_(`UR_SIGMASK3', `20')
_m4_define_(`UR_PRIVATEDATAP', `492')
_m4_define_(`SCO_SIG_SETMASK', `0')
_m4_define_(`SCO_SIGPROCMASK', `10280')
',`')

/ int sigsetjmp(sigjmp_buf env,int savemask)
/
_fwdef_(`sigsetjmp'):
	MCOUNT			/ subroutine entry counter if profiling

	movl	4(%esp),%eax	/ ucp = (ucontext_t *)env;

	movl	$UC_ALL,(%eax)	/ ucp->uc_flags = UC_ALL;

	pushl	%eax	/ ucp
	pushl	$0	/ GETCONTEXT
	pushl	%eax	/ dummy return addr
	movl	$UCONTEXT,%eax
	lcall	$0x7,$0		/ __getcontext(ucp);
	addl	$0xC,%esp

	movl	4(%esp),%eax

	cmpl	$0,8(%esp)	/ if (!savemask)
	jnz	.mask
	andl	$-1&~UC_SIGMASK,(%eax)	/  ucp->uc_flags &= ~UC_SIGMASK;
.mask:

_m4_ifdef_(`GEMINI_ON_OSR5',`
	/ fill in values not set by osr5 kernel
	movl	$0,UR_SIGMASK1 (%eax) / patch uc_sigmask.sa_sigbits
	movl	$0,UR_SIGMASK2 (%eax) 
	movl	$0,UR_SIGMASK3 (%eax)
	movl	$0,UR_PRIVATEDATAP (%eax) / patch uc_privatedatap
',`')
	/ cpup = (greg_t *)&ucp->uc_mcontext.gregs;
	leal	[uc_mcontext](%eax),%edx

	movl	$1,EAX\*4(%edx)	/ cpup[ EAX ] = 1;

	movl	0(%esp),%eax	/ set cpup[ EIP ] to callers EIP
	movl	%eax,EIP\*4(%edx)

	leal	4(%esp),%eax	/ set cpup[ UESP ] to callers ESP
	movl	%eax,UESP\*4(%edx)

	xorl	%eax,%eax
	ret


_m4_ifdef_(`GEMINI_ON_OSR5',`
	/ On OSR5, setcontext does not set %eax correctly,
	/ so we will restore context by hand
_fwdef_(`siglongjmp'):
	MCOUNT			/ subroutine entry counter if profiling
	cld			/ in case reps are going in reverse
	movl	4(%esp),%edx	/ ucontext_t *
	testl	$UC_SIGMASK, (%edx)	/ is masked saved?	
	jz	.nosave
	pushl	%edx		/ save context pointer
	pushl	$0		/ set signal mask
	leal	UR_SIGMASK0 (%edx), %eax
	pushl	%eax
	pushl	$SCO_SIG_SETMASK
	pushl	%eax	/ dummy return addr
	movl	$SCO_SIGPROCMASK, %eax
	lcall	$0x7,$0		/ sigprocmask(SIG_SETMASK, mask, 0);
	addl	$16,%esp
	popl	%edx
.nosave:
	movl	8(%esp),%eax	/ return val
	movl	UR_EBX (%edx),%ebx	/ restore ebx
	movl	UR_ESI (%edx),%esi	/ restore esi
	movl	UR_EDI (%edx),%edi	/ restore edi
	movl	UR_EBP (%edx),%ebp	/ restore callers ebp
	movl	UR_ESP (%edx),%esp	/ restore callers esp
	movl	UR_EIP (%edx),%edx	/ callers address
	testl	%eax,%eax	/ if val != 0
	jne	.ret		/ 	return val
	incl	%eax		/ else return 1
.ret:
	jmp	*%edx		/ return to caller
',`')
