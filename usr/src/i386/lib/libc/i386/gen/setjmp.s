	.file	"setjmp.s"

	.ident	"@(#)libc-i386:gen/setjmp.s	1.8"

/	longjmp(env, val)
/ will generate a "return(val)" from
/ the last call to
/	setjmp(env)
/ by restoring registers ip, sp, bp, bx, si, and di from 'env'
/ and doing a return.

/ entry    reg	offset from (%si)
/ env[0] = %ebx	 0	/ register variables
/ env[1] = %esi	 4
/ env[2] = %edi	 8
/ env[3] = %ebp	 12	/ stack frame
/ env[4] = %esp	 16
/ env[5] = %eip	 20

	.align	4
_fwdef_(`setjmp'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	4(%esp),%eax	/ jmpbuf address
	movl	%ebx,0(%eax)	/ save ebx
	movl	%esi,4(%eax)	/ save esi
	movl	%edi,8(%eax)	/ save edi
	movl	%ebp,12(%eax)	/ save caller's ebp
	popl	%edx		/ return address
	movl	%esp,16(%eax)	/ save caller's esp
	movl	%edx,20(%eax)
	subl	%eax,%eax	/ return 0
	jmp	*%edx

	.align	4
_fwdef_(`longjmp'):
	MCOUNT			/ subroutine entry counter if profiling
	cld			/ in case reps are going in reverse
	movl	4(%esp),%edx	/ first parameter after return addr
	movl	8(%esp),%eax	/ second parameter
	movl	0(%edx),%ebx	/ restore ebx
	movl	4(%edx),%esi	/ restore esi
	movl	8(%edx),%edi	/ restore edi
	movl	12(%edx),%ebp	/ restore caller's ebp
	movl	16(%edx),%esp	/ restore caller's esp
	test	%eax,%eax	/ if val != 0
	jne	.ret		/ 	return val
	incl	%eax		/ else return 1
.ret:
	jmp	*20(%edx)	/ return to caller
