	.file	"lldivrem.s"
	.ident	"@(#)cplusfe:i386/lldivrem.s	1.1"
	.ident	"@(#)libc-i386:crt/lldivrem.s	1.1"

/ long long runtime support routines -- signed/unsigned divide/remainder
/
/ __llasgdivs -- signed long long division
/ __llasgrems -- signed long long remainder
/ __llasgdivu -- unsigned long long division
/ __llasgremu -- unsigned long long remainder
/
/ For all four of the above, on entry
/	%ecx		points to the numerator
/	%edx:%eax	is the denominator
/ and on exit,
/	%edx:%eax	is the result (quotient or remainder)
/
/ All four sit on top of the internal functions _lludivrem and
/ _llsdivrem, and _llsdivrem uses _lludivrem for the hard part.

	.globl	__llasgdivs
	.align	4
__llasgdivs:
	call	_llsdivrem	/ quotient in 4(%ecx):(%ecx)
	movl	4(%ecx), %edx
	movl	(%ecx), %eax
	ret
	.type	__llasgdivs, "function"
	.size	__llasgdivs, .-__llasgdivs

	.globl	__llasgrems
	.align	4
__llasgrems:
	call	_llsdivrem	/ remainder in %edx:%eax
	movl	%edx, 4(%ecx)
	movl	%eax, (%ecx)
	ret
	.type	__llasgrems, "function"
	.size	__llasgrems, .-__llasgrems

	.globl	__llasgdivu
	.align	4
__llasgdivu:
	call	_lludivrem	/ quotient in 4(%ecx):(%ecx)
	movl	4(%ecx), %edx
	movl	(%ecx), %eax
	ret
	.type	__llasgdivu, "function"
	.size	__llasgdivu, .-__llasgdivu

	.globl	__llasgremu
	.align	4
__llasgremu:
	call	_lludivrem	/ remainder in %edx:%eax
	movl	%edx, 4(%ecx)
	movl	%eax, (%ecx)
	ret
	.type	__llasgremu, "function"
	.size	__llasgremu, .-__llasgremu

/ _llsdivrem -- signed long long divide/remainder
/	%ecx		points to the numerator
/	%edx:%eax	is the denominator
/ returns %edx:%eax	the remainder
/	with %ecx	pointing to the quotient

	.local	_llsdivrem
	.align	4
_llsdivrem:
	/ Implement by making the numerator and denominator nonnegative
	/ and having _lludivrem do the hard work.  Note whether to negate
	/ the resulting quotient and/or remainder in the low bits of %ebx.

	pushl	%ebx
	xorl	%ebx, %ebx	/ negate neither (so far)
	btl	$31, 4(%ecx)
	jnc	.nnnumer
	movl	$3, %ebx	/ negate remainder and quotient
	notl	(%ecx)
	notl	4(%ecx)
	addl	$1, (%ecx)
	adcl	$0, 4(%ecx)
.nnnumer:
	btl	$31, %edx
	jnc	.nndenom
	xorl	$1, %ebx	/ flip state of quotient negation
	notl	%eax
	notl	%edx
	addl	$1, %eax
	adcl	$0, %edx
.nndenom:
	call	_lludivrem	/ do the hard work
	btl	$0, %ebx
	jnc	.nnquo
	notl	(%ecx)
	notl	4(%ecx)
	addl	$1, (%ecx)
	adcl	$0, 4(%ecx)
.nnquo:
	btl	$1, %ebx
	jnc	.nnrem
	notl	%eax
	notl	%edx
	addl	$1, %eax
	adcl	$0, %edx
.nnrem:
	popl	%ebx
	ret
	.type	_llsdivrem, "function"
	.size	_llsdivrem, .-_llsdivrem

/ _lludivrem -- unsigned long long divide/remainder
/	%ecx		points to the numerator
/	%edx:%eax	is the denominator
/ returns %edx:%eax	the remainder
/	with %ecx	pointing to the quotient

	.local	_lludivrem
	.align	4
_lludivrem:
	pushl	%ebx

	testl	%edx, %edx
	jz	.z_hi_denom
	testl	%eax, %eax
	jz	.z_lo_denom
	cmpl	%edx, 4(%ecx)
	jb	.zero
	ja	.bitwise
	cmpl	%eax, (%ecx)
	jb	.zero
	je	.one

.bitwise:
	/ Since %edx and %eax are nonzero and %edx:%eax < 4(%ecx):(%ecx),
	/ we've got to do the work by hand.  The quotient is nonzero and
	/ fits in a single word, the remainder can take both words.
	/
	/ It turns out to be much easier to do the division in binary
	/ with a loop of compare/subtract.  The loop executes at most
	/ 32 times (the size of a word) and consists mostly of cheap
	/ instructions.

	subl	$16, %esp
	movl	%esi, 12(%esp)
	movl	%edi, 8(%esp)
	movl	%ebp, 4(%esp)
	movl	%ecx, %ebp	/ so that %cl can be used for shifting

	xorl	%ebx, %ebx	/ so that we can use "setc %bl" below
	movl	%ebx, (%esp)	/ initialize quotient
	bsrl	%edx, %edi	/ 0 <= %edi <= 31 since %edx != 0
	bsrl	4(%ebp), %ecx	/ %ecx >= %edi since 4(%ebp) >= %edx
	subl	%edi, %ecx
	incl	%ecx		/ 1 <= %ecx <= 32
	movl	4(%ebp), %edi
	movl	%ebx, 4(%ebp)	/ clear high word of quotient
	cmpl	$32, %ecx
	jb	.skip
	movl	%edi, %esi
	xorl	%edi, %edi
	jmp	.loop
.skip:
	movl	(%ebp), %esi
	shrdl	%edi, %esi
	shrl	%cl, %edi
.loop:
	/ %edi:%esi holds the current numerator.
	/ %ecx is the number of bits of quotient yet to build.
	/
	/ Shift it up and fill in the low bit from the original
	/ numerator's corresponding bit.  When the current numerator
	/ is at least as big as the denominator (still %edx:%eax),
	/ reduce the current numerator by the denominator and OR in
	/ a low bit in the quotient.  Otherwise, just shift up the
	/ quotient.
	/
	/ After the last iteration, the current numerator is the
	/ remainder.

	decl	%ecx
	shldl	$1, %esi, %edi
	shll	%esi
	btl	%ecx, (%ebp)
	setc	%bl
	orl	%ebx, %esi
	shll	(%esp)
	cmpl	%edi, %edx
	ja	.bottom
	jb	.subtract
	cmpl	%esi, %eax
	ja	.bottom
.subtract:
	subl	%eax, %esi
	sbbl	%edx, %edi
	orl	$1, (%esp)
.bottom:
	testl	%ecx, %ecx
	jnz	.loop

	movl	%esi, %eax
	movl	%edi, %edx
	movl	(%esp), %ebx
	movl	%ebx, (%ebp)

	movl	%ebp, %ecx
	movl	12(%esp), %esi
	movl	8(%esp), %edi
	movl	4(%esp), %ebp
	addl	$16, %esp
	jmp	.ret

.z_hi_denom: / %edx==0
	cmpl	$1, %eax
	je	.byone
	movl	4(%ecx), %edx
	cmpl	%eax, %edx
	jae	.paired

	movl	%eax, %ebx
	movl	(%ecx), %eax
	divl	%ebx
	movl	%eax, (%ecx)
	movl	%edx, %eax
	xorl	%edx, %edx
	movl	%edx, 4(%ecx)
	jmp	.ret

.byone: / quotient is numerator, remainder is zero
	xorl	%eax, %eax
	jmp	.ret

.paired: / need two divides since high numerator word >= divisor
	movl	%eax, %ebx
	movl	%edx, %eax
	xorl	%edx, %edx
	divl	%ebx
	movl	%eax, 4(%ecx)
	movl	(%ecx), %eax
	divl	%ebx
	movl	%eax, (%ecx)
	movl	%edx, %eax
	xorl	%edx, %edx
	jmp	.ret

.z_lo_denom: / edx!=0, %eax==0
	movl	%edx, %ebx
	movl	4(%ecx), %eax
	xorl	%edx, %edx
	movl	%edx, 4(%ecx)
	divl	%ebx
	movl	%eax, %ebx
	movl	(%ecx), %eax
	movl	%ebx, (%ecx)
	jmp	.ret

.one: / quotient is one, remainder is zero
	xorl	%eax, %eax
	xorl	%edx, %edx
	movl	%eax, 4(%ecx)
	movl	$1, (%ecx)
	jmp	.ret

.zero: / quotient is zero, remainder is numerator
	movl	4(%ecx), %edx
	movl	(%ecx), %eax
	xorl	%ebx, %ebx
	movl	%ebx, 4(%ecx)
	movl	%ebx, (%ecx)

.ret:
	popl	%ebx
	ret
	.type	_lludivrem, "function"
	.size	_lludivrem, .-_lludivrem
