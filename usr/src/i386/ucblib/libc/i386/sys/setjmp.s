#ident	"@(#)ucb:i386/ucblib/libc/i386/sys/setjmp.s	1.3"
#ident	"$Header$"
/		copyright	"%c%"

	.file	"setjmp.s"

.ident		"@(#)ucb:i386/ucblib/libc/i386/sys/setjmp.s	1.3"

/
/       setjmp/longjmp _setjmp/_longjmp are equivalent to sigsetjmp/siglongjmp
/       for ucb.  We cannot call sigsetjmp directly because it takes two
/	arguments and setjmp takes 1 argument.
/
/       Duplicate the code from sigsetjmp taking into account 
/       the different behaviour between setjmp and _setjmp
/
/       setjmp()/_setjmp/longjmp/_longjmp are implemented in assembly 
/	language because it needs to have direct control over register use.
/
/	Please Note: that System call entry position of UCONTEXT is 
/	hard-coded here.  So if UCONTEXT entry position ever changes
/	this would have to change.
/	

		.set	uc_mcontext, 36
		.set	UC_ALL, 0x1F
		.set	UC_SIGMASK, 0x01
		.set	EAX, 11
		.set	UESP, 17
		.set	EIP, 14
		.set	UCONTEXT, 100

/ int setjmp(sigjmp_buf env) jmp_buf is defined to be sigjmp_buf in ucbinclude

	.globl	setjmp
/       .globl  _sigsetjmp  /this is how it used to be, before the 386/3b2 merge
	.globl  sigsetjmp
setjmp:
        movl    4(%esp),%eax    / ucp = (ucontext_t *)env;

        movl    $UC_ALL,(%eax)  / ucp->uc_flags = UC_ALL;

        pushl   %eax    / ucp
        pushl   $0      / GETCONTEXT
        pushl   %eax    / dummy return addr
        movl    $UCONTEXT,%eax
        lcall   $0x7,$0         / __getcontext(ucp);
        addl    $0xC,%esp

        movl    4(%esp),%eax

        / cpup = (greg_t *)&ucp->uc_mcontext.gregs;
        leal    [uc_mcontext](%eax),%edx

        movl    $1,EAX\*4(%edx) / cpup[ EAX ] = 1;

        movl    0(%esp),%eax    / set cpup[ EIP ] to caller's EIP
        movl    %eax,EIP\*4(%edx)

        leal    4(%esp),%eax    / set cpup[ UESP ] to caller's ESP
        movl    %eax,UESP\*4(%edx)

        xorl    %eax,%eax
        ret


	.globl	_setjmp
	.globl	_sigsetjmp
_setjmp:
        movl    4(%esp),%eax    / ucp = (ucontext_t *)env;

        movl    $UC_ALL,(%eax)  / ucp->uc_flags = UC_ALL;

        pushl   %eax    / ucp
        pushl   $0      / GETCONTEXT
        pushl   %eax    / dummy return addr
        movl    $UCONTEXT,%eax
        lcall   $0x7,$0         / __getcontext(ucp);
        addl    $0xC,%esp

        movl    4(%esp),%eax

	/ Ignore the signal mask 
/      andl    $-1!UC_SIGMASK,(%eax)   /  ucp->uc_flags &= ~UC_SIGMASK;
       andl    $-1&~UC_SIGMASK,(%eax)   /  ucp->uc_flags &= ~UC_SIGMASK;

        / cpup = (greg_t *)&ucp->uc_mcontext.gregs;
        leal    [uc_mcontext](%eax),%edx

        movl    $1,EAX\*4(%edx) / cpup[ EAX ] = 1;

        movl    0(%esp),%eax    / set cpup[ EIP ] to caller's EIP
        movl    %eax,EIP\*4(%edx)

        leal    4(%esp),%eax    / set cpup[ UESP ] to caller's ESP
        movl    %eax,UESP\*4(%edx)

        xorl    %eax,%eax
        ret

        .globl  longjmp
        .globl  _longjmp
        .globl  _siglongjmp
longjmp:
_longjmp:
        jmp     _siglongjmp

