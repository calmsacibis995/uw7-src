#ident	"@(#)libc-i386:inc/qstr.h	1.13"
/*
* qstr.h -- internal string/memory functions, x86 version
*/

#if defined(i386) && defined(__USLC__)

	/*
	* _qcpy - memcpy except that the address of the next byte is returned.
	*  1. Save %edi and %esi, move parameters into registers.
	*  2. Save (on stack) the incoming n (now, %ecx).
	*  3. Change byte count limit to 4-byte count.
	*  4. Copy "long"s from (%esi) to (%edi) until %ecx decrements to zero.
	*  5. Copy the remaining 0-3 bytes from (%esi) to (%edi).
	*  6. Restore %esi and %edi leaving %eax pointing to the next byte.
	*/
asm void *
#ifdef __STDC__
_qcpy(void *dp, const void *sp, size_t n)
#else
_qcpy(dp, sp, n)Uchar *dp; const Uchar *sp; size_t n;
#endif
{
%ureg dp, sp; mem n;
/INTRINSIC
	movl	n, %ecx
	movl	dp, %eax
	movl	sp, %edx
	xchgl	%eax, %edi
	xchgl	%edx, %esi
	pushl	%ecx
	shrl	$2, %ecx
	repz; smovl
	popl	%ecx
	andl	$3, %ecx
	repz; smovb
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%ureg dp; mem sp, n;
/INTRINSIC
	movl	n, %ecx
	movl	dp, %eax
	movl	sp, %edx
	xchgl	%eax, %edi
	xchgl	%edx, %esi
	pushl	%ecx
	shrl	$2, %ecx
	repz; smovl
	popl	%ecx
	andl	$3, %ecx
	repz; smovb
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%ureg sp; mem dp, n;
/INTRINSIC
	movl	n, %ecx
	movl	dp, %eax
	movl	sp, %edx
	xchgl	%eax, %edi
	xchgl	%edx, %esi
	pushl	%ecx
	shrl	$2, %ecx
	repz; smovl
	popl	%ecx
	andl	$3, %ecx
	repz; smovb
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%mem dp, sp, n;
/INTRINSIC
	movl	%edi, %eax
	movl	%esi, %edx
	movl	n, %ecx
	movl	dp, %edi
	movl	sp, %esi
	pushl	%ecx
	shrl	$2, %ecx
	repz; smovl
	popl	%ecx
	andl	$3, %ecx
	repz; smovb
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%error;
}
 #pragma asm full_optimization _qcpy

	/*
	* _qlcpy - _qcpy except that "n" must be a multiple of sizeof(long).
	*  1. Save %edi and %esi, move parameters into registers.
	*  2. Change byte count limit to 4-byte count.
	*  3. Copy "long"s from (%esi) to (%edi) until %ecx decrements to zero.
	*  4. Restore %esi and %edi leaving %eax pointing to the next byte.
	*/
asm void *
#ifdef __STDC__
_qlcpy(void *dp, const void *sp, size_t n)
#else
_qlcpy(dp, sp, n)Uchar *dp; const Uchar *sp; size_t n;
#endif
{
%ureg dp, sp; mem n;
/INTRINSIC
	movl	n, %ecx
	movl	dp, %eax
	movl	sp, %edx
	xchgl	%eax, %edi
	xchgl	%edx, %esi
	shrl	$2, %ecx
	repz; smovl
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%ureg dp; mem sp, n;
/INTRINSIC
	movl	n, %ecx
	movl	dp, %eax
	movl	sp, %edx
	xchgl	%eax, %edi
	xchgl	%edx, %esi
	shrl	$2, %ecx
	repz; smovl
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%ureg sp; mem dp, n;
/INTRINSIC
	movl	n, %ecx
	movl	dp, %eax
	movl	sp, %edx
	xchgl	%eax, %edi
	xchgl	%edx, %esi
	shrl	$2, %ecx
	repz; smovl
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%mem dp, sp, n;
/INTRINSIC
	movl	%edi, %eax
	movl	%esi, %edx
	movl	n, %ecx
	movl	dp, %edi
	movl	sp, %esi
	shrl	$2, %ecx
	repz; smovl
	movl	%edx, %esi
	xchgl	%edi, %eax
/INTRINSICEND
%error;
}
 #pragma asm full_optimization _qlcpy

	/*
	* _strend - return pointer to '\0' or n-th byte, whichever is first.
	*  1. Save %edi, move parameters into registers.
	*  2. Jump to the return code (step 6) if the count is zero.
	*  3. Set %eax to zero (%al is now '\0', the byte value to stop at).
	*  4. Scan until either %ecx decrements to zero or %al is matched.
	*  5. If a match was found, -1(%edi) matched, so decrement %edi.
	*  6. Move address to %eax, restore %edi.
	*/
asm char *
#ifdef __STDC__
_strend(const char *p, size_t n)
#else
_strend(p, n)const char *p; size_t n;
#endif
{
%mem p, n; lab skip;
/INTRINSIC
	movl	%edi, %edx
	movl	n, %ecx
	movl	p, %edi
	testl	%ecx, %ecx
	jz	skip
	xorl	%eax, %eax
	repnz; scasb
	jne	skip
	decl	%edi		/ matched %al, backup %edi
skip:	movl	%edi, %eax
	movl	%edx, %edi
/INTRINSICEND
%error;
}
 #pragma asm full_optimization _strend

#define STREND(p, n)	_strend(p, n)

	/*
	* _wcsend - return pointer to 0-valued or n-th "long", whichever is first.
	*  1. Save %edi, move parameters into registers.
	*  2. Jump to the return code (step 6) if the count is zero.
	*  3. Set %eax to zero (it is the value to stop at).
	*  4. Scan until either %ecx decrements to zero or %eax is matched.
	*  5. If a match was found, -4(%edi) matched, so decrement %edi.
	*  6. Move address to %eax, restore %edi.
	*/
asm wchar_t *
#ifdef __STDC__
_wcsend(const wchar_t *p, size_t n)
#else
_wcsend(p, n)const wchar_t *p; size_t n;
#endif
{
%mem p, n; lab skip;
/INTRINSIC
	movl	%edi, %edx
	movl	n, %ecx
	movl	p, %edi
	testl	%ecx, %ecx
	jz	skip
	xorl	%eax, %eax
	repnz; scasl
	jne	skip
	leal	-4(%edi), %edi		/ matched %eax, backup %edi
skip:	movl	%edi, %eax
	movl	%edx, %edi
/INTRINSICEND
%error;
}
 #pragma asm full_optimization _wcsend

#define WCSEND(p, n)	_wcsend(p, n)

	/*
	* _ultos - Assign decimal digits for Ulong to byte array in reverse.
	*  1. Move parameters into registers, save %esi (on stack).
	*  2. Set %esi to 10, the divisor.
	*  3. Set high 32 bits of dividend (%edx) to zero.
	*  4. Decrement target pointer (%ecx).  (It began one-past the end.)
	*  5. Divide %edx:%eax by 10; change remainder from [0,9] to ['0','9'].
	*  6. Test for zero quotient (%eax) and assign current digit.
	*  7. Continue dividing (go back to step 3) for nonzero quotients.
	*  8. Restore %esi and set %eax to point at the start of the digits.
	*/
asm unsigned char *
#ifdef __STDC__
_ultos(unsigned char *p, unsigned long ul)
#else
_ultos(p, ul)unsigned char *p; unsigned long ul;
#endif
{
%mem p, ul; lab top;
/INTRINSIC
	movl	p, %ecx
	movl	ul, %eax
	pushl	%esi
	movl	$10, %esi
top:	xorl	%edx, %edx
	decl	%ecx
	divl	%esi
	leal	0x30(%edx), %edx
	testl	%eax, %eax
	movb	%dl, (%ecx)
	jne	top
	popl	%esi
	movl	%ecx, %eax
/INTRINSICEND
%error;
}
 #pragma asm full_optimization _ultos

#ifndef NO_LONG_LONG_EMULATE
	/*
	* _ullmove - Copy a possibly negated paired 32 bit to *dst.
	*  1. Move the big value into %edx:%ecx and the pointer into %eax.
	*  2. If negation needed, flip the bits and add 1 to the 64 bits.
	*  3. In either case, move the lo and hi halves.
	*/
asm void
#ifdef __STDC__
_ullmove(void *dst, int neg, unsigned long hi, unsigned long lo)
#else
_ullmove(dst, neg, hi, lo)void *dst; int neg; unsigned long hi, lo;
#endif
{
%con neg==0; mem dst, hi, lo;
/INTRINSIC
	movl	dst, %eax
	movl	lo, %edx
	movl	hi, %ecx
	movl	%edx, (%eax)
	movl	%ecx, 4(%eax)
/INTRINSICEND
%con neg!=0; mem dst, hi, lo;
/INTRINSIC
	movl	hi, %edx
	movl	lo, %ecx
	movl	dst, %eax
	notl	%ecx
	notl	%edx
	addl	$1, %ecx	/ not incl because it does not set CF
	adcl	$0, %edx
	movl	%ecx, (%eax)
	movl	%edx, 4(%eax)
/INTRINSICEND
%mem dst, neg, hi, lo; lab skip;
/INTRINSIC
	movl	hi, %edx
	movl	lo, %ecx
	movl	dst, %eax
	cmpl	$0, neg
	jz	skip
	notl	%ecx
	notl	%edx
	addl	$1, %ecx	/ not incl because it does not set CF
	adcl	$0, %edx
skip:	movl	%ecx, (%eax)
	movl	%edx, 4(%eax)
/INTRINSICEND
%error;
}

	/*
	* _ullabs - For paired 32 bit value, negate if 64 value is negative.
	*  1. Set %eax to zero (return when nonnegative).
	*  2. If hi (%edx) is nonnegative, return.
	*  3. Otherwise, one's complement the bits and add 1 to the pair.
	*/
asm int
#ifdef __STDC__
_ullabs(unsigned long *lo, unsigned long *hi)
#else
_ullabs(lo, hi)unsigned long *lo, *hi;
#endif
{
%mem lo, hi; lab nonneg;
	xorl	%eax, %eax
	movl	hi, %edx
	cmpl	$0, (%edx)
	jge	nonneg
	notl	%eax
	movl	lo, %ecx
	xorl	%eax, (%ecx)
	xorl	%eax, (%edx)
	addl	$1, (%ecx)	/ not incl because it does not set CF
	adcl	$0, (%edx)
nonneg:
%error;
}
 #pragma asm full_optimization _ullabs

	/*
	* _ullrshift - Shift down pair of 32 unsigned longs, returning old lo.
	*/
asm unsigned long
#ifdef __STDC__
_ullrshift(int shift, unsigned long *lo, unsigned long *hi)
#else
_ullrshift(shift, lo, hi)int shift; unsigned long *lo, *hi;
#endif
{
%mem shift, lo, hi;
/INTRINSIC
	movl	shift, %ecx
	movl	lo, %eax
	movl	hi, %edx
	pushl	%esi
	pushl	%edi
	movl	%eax, %esi
	movl	(%esi), %eax
	movl	%edx, %edi
	movl	(%edi), %edx
	shrdl	%edx, (%esi)
	shrl	%cl, (%edi)
	popl	%edi
	popl	%esi
/INTRINSICEND
%error;
}
 #pragma asm full_optimization _ullrshift

	/*
	* _ulltos - Assign decimal digits for unsigned long long in reverse.
	*  1. Move parameters into registers, save %esi (on stack).
	*  2. Set %esi to 10, the divisor.
	*  3. If the high 32 bits are less than 10, skip to the simple code.
	*  4. Otherwise need paired divides: save %ebx (low) and %edi (high).
	*  5. Save the low 32 bits and set up to divide the high by 10.
	*  6. Decrement target pointer (%ecx).  [It started one-past the end.]
	*  7. Divide the high by 10 and save the quotent in %edi.
	*  8. Divide the low by 10 carrying in the remainder from (7).
	*  9. Change remainder to ASCII and store the digit.
	* 10. If the high 32 bits are still at least 10, back to the divides.
	* 11. Otherwise, restore %ebx, %edi after setting %edx for the carry.
	* 12. If the both %edx and %eax are zero, nothing left to do.
	* 13. Otherwise, do code identical to _ultos() until zero.
	* 14. Restore %esi and set %eax to point to the start of the digits.
	*/
asm unsigned char *
#ifdef __STDC__
_ulltos(unsigned char *p, unsigned long lo, unsigned long hi)
#else
_ulltos(p, lo, hi)unsigned char *p; unsigned long lo, hi;
#endif
{
%mem p, lo, hi; lab full_top, full, rest_top, rest, done;
/INTRINSIC
	movl	p, %ecx
	movl	lo, %eax	/ set up for simple case...
	movl	hi, %edx	/ ...when hi < 10
	pushl	%esi
	movl	$10, %esi
	cmpl	%esi, %edx
	jb	rest
/ set up for paired divide code
	pushl	%ebx		/ holds low 32 bits
	pushl	%edi		/ holds high 32 bits
	movl	%eax, %ebx
	movl	%edx, %eax
	jmp	full
full_top:
	movl	%eax, %ebx	/ save low 32 bits after second divide
	movl	%edi, %eax
full:	xorl	%edx, %edx
	decl	%ecx
	divl	%esi		/ hi/10: quot=%eax, rem=%edx
	movl	%eax, %edi
	movl	%ebx, %eax
	divl	%esi		/ lo/10: quot=%eax, rem=%edx
	leal	0x30(%edx), %edx
	cmpl	%esi, %edi
	movb	%dl, (%ecx)
	jge	full_top
/ clean up after paired divide code
	movl	%edi, %edx
	popl	%edi
	popl	%ebx
	jmp	rest		/ note: remaining hi (now %edx) is nonzero
rest_top:
	xorl	%edx, %edx
rest:	decl	%ecx
	divl	%esi
	leal	0x30(%edx), %edx
	testl	%eax, %eax
	movb	%dl, (%ecx)
	jne	rest_top
done:	popl	%esi
	movl	%ecx, %eax
/INTRINSICEND
%error;
}
 #pragma asm full_optimization _ulltos
#endif /*NO_LONG_LONG_EMULATE*/

	/*
	* _muladd - Return (*a)+b+c*d and set (*a) to carry; all are Ulongs.
	*  1. Move d into %eax and multiply it by c; the result is %edx:%eax.
	*  2. Hold the pointer in %ecx and add b to the low half of the product.
	*  3. Add any carry from the previous addition to the high half (%edx).
	*  4. Add the pointed-to Ulong to the low half and any carry to the high.
	*  5. Set the pointed-to Ulong to the carry and return the low half.
	*/
asm unsigned long
#ifdef __STDC__
_muladd(unsigned long *a, unsigned long b, unsigned long c, unsigned long d)
#else
_muladd(a, b, c, d)unsigned long *a, b, c, d;
#endif
{
%mem a, b, c, d;
	movl	d, %eax
	mull	c
	movl	a, %ecx
	addl	b, %eax
	adcl	$0, %edx
	addl	(%ecx), %eax
	adcl	$0, %edx
	movl	%edx, (%ecx)
%error;
}
 #pragma asm full_optimization _muladd

#define MULADD(a, b, c, d)	_muladd(a, b, c, d)

#else /*!(defined(i386) && defined(__USLC__))*/

#define _qcpy(d, s, n)	((void *)((n) + (char *)memcpy(d, s, n)))
#define _qlcpy(d, s, n)	_qcpy(d, s, n)
#define _ULTOS(p, v)	do { *--(p) = (v) % 10 + '0'; } while (((v) /= 10) != 0)

#endif /*defined(i386) && defined(__USLC__)*/
