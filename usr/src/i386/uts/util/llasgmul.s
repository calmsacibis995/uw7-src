	.ident	"@(#)kern-i386:util/llasgmul.s	1.1.1.1"
/	.ident	"@(#)libc-i386:crt/llasgmul.s	1.1"
	.file	"util/llasgmul.s"

include(KBASE/svc/asm.m4)

/ long long runtime support routine -- long long multiplication
/
/ __llasgmul
/	%ecx		points to lhs long long
/	%edx:%eax	the rhs long long
/ returns %edx:%eax
/
/ Since the result is (only) two words, the two high words are never
/ multiplied together.  The low word of the result is always just
/ the low half of [lo(lhs)*lo(rhs)].  The high word of the result
/ is sum of [lo(lhs)*hi(rhs)] and [hi(lhs)*lo(rhs)] and the carry
/ of [lo(lhs)*lo(rhs)].
/
/ We take shortcuts when one or more words are known to be zero.
/ We check for zero-valued low words first since they simplify more
/ multiplications.

ENTRY(__llasgmul)
	testl	%eax, %eax
	jz	.z_lo_rhs
	cmpl	$0, (%ecx)
	jz	.z_lo_lhs
	testl	%edx, %edx
	jz	.z_hi_rhs
	cmpl	$0, 4(%ecx)
	jz	.z_hi_lhs

/ general case...here when all words are nonzero.
/
/ for this portion, the stack will hold the following:
/    (%esp) -- the low word of the result
/   4(%esp) -- the high word of the result
/   8(%esp) -- the original rhs high word
/  12(%esp) -- the original rhs low word
	subl	$16, %esp
	movl	%eax, 12(%esp)
	movl	%edx, 8(%esp)
	mull	(%ecx)		/ lo(lhs) * lo(rhs) now in %edx:%eax
	movl	%eax, (%esp)
	movl	%edx, 4(%esp)
	movl	8(%esp), %eax
	mull	(%ecx)		/ lo(lhs) * hi(rhs) now in %edx:%eax
	addl	%eax, 4(%esp)
	movl	12(%esp), %eax
	mull	4(%ecx)		/ hi(lhs) * lo(rhs) now in %edx:%eax
	addl	4(%esp), %eax
	movl	%eax, %edx
	movl	(%esp), %eax
	addl	$16, %esp
	jmp	.rhsresult

.z_lo_rhs: / %eax==0
	testl	%edx, %edx
	jz	.rhsresult
	cmpl	$0, (%ecx)
	jz	.z_result

/ result is just lo(lhs) * hi(rhs) in upper word
	movl	%edx, %eax
	mull	(%ecx)
	jmp	.z_shift

.z_result: / make zero result
	xorl	%edx, %edx
	xorl	%eax, %eax
	jmp	.rhsresult

.z_lo_lhs: / (%ecx)==0, %eax!=0
	cmpl	$0, 4(%ecx)
	jz	.z_result

/ result is just hi(lhs) * lo(lhs) in upper word
	mull	4(%ecx)
.z_shift:
	movl	%eax, %edx
	xorl	%eax, %eax
	jmp	.rhsresult

.z_hi_rhs: / %edx==0, %eax!=0, (%ecx)!=0
	cmpl	$0, 4(%ecx)
	jz	.z_hi

/ result is sum of [hi(lhs)*lo(rhs) in upper] and [lo(lhs)*lo(rhs)]
	pushl	%eax
	mull	4(%ecx)
	movl	%eax, 4(%ecx)
	popl	%eax
	mull	(%ecx)
	addl	4(%ecx), %edx
	jmp	.rhsresult

.z_hi_lhs: / result is sum of [lo(lhs)*hi(rhs) in upper] and [lo(lhs)*lo(rhs)]
	pushl	%eax
	movl	%edx, %eax
	mull	(%ecx)
	movl	%eax, 4(%ecx)
	popl	%eax
	mull	(%ecx)
	addl	4(%ecx), %edx
	jmp	.rhsresult

.z_hi: / %edx==0, 4(%ecx)==0, %eax!=0, (%ecx)!=0
	mull	(%ecx)

.rhsresult:
	movl	%edx, 4(%ecx)
	movl	%eax, (%ecx)
	ret
	SIZE(__llasgmul)
