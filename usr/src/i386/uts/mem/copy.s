	.file	"copy.s"

	.ident	"@(#)kern-i386:mem/copy.s	1.10.2.1"
	.ident	"$Header$"

/	High-speed copy routines.

include(KBASE/svc/asm.m4)
include(assym_include)

	.text

/
/ void
/ bzero(void *, size_t)
/
/	This function writes successive bytes of zero starting
/	at the first argument until the number of bytes written
/ 	equal the second argument.
/
/ Calling State/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the argument prior to the call.  This function has no
/	return value.
/
/ Remarks:
/
/	This function is also known as "struct_zero".
/

ENTRY(bzero)
ENTRY(struct_zero)
	movl	%edi, %edx	/ save register
	movl	SPARG0, %edi	/ memory location to start
	movl	SPARG1, %ecx	/ count
	shrl	$2, %ecx	/ # of dwords to do
	xorl	%eax, %eax	/ write zero
	rep;	stosl		/ zero dwords first (leaves ECX == 0)
	movb	SPARG1, %cl	/ ECX = low-order byte of count
	andb	$3, %cl		/ # remaining bytes to zero
	rep;	stosb		/ zero remaining bytes
	movl	%edx, %edi	/ restore register
	ret			/ void return value

	SIZE(bzero)
	SIZE(struct_zero)


/
/ void
/ bcopy(const void *, void *, size_t)
/ 
/	This function copies bytes from the address given by the
/	first argument to the address given by the second argument
/	stopping when the number of bytes equal to the third argument
/	have been moved.
/
/ Calling State/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.  This function has no
/	return value.
/
/ Remarks:
/
/	Assumes the copy area does not overlap and thus the forward
/	direction for the copy (low to high address) is ok.
/	NOTE: use ovbcopy() for the overlapping case.
/
/	Note that the 386 is perfectly capable of doing non-aligned
/	copies.  It is expected that in practice that most copies
/	in the kernel will either be small or will involve at least
/	one aligned argument, thus the overhead of doing the
/	alignment likely outweighs the benefit.
/

ENTRY(bcopy)
	movl	%edi, %edx		/ save registers
	movl	%esi, %eax
	movl	SPARG0, %esi		/ from
	movl	SPARG1, %edi		/ to
	movl	SPARG2, %ecx		/ count
	shrl	$2, %ecx		/ convert to count of words to copy
	rep;	smovl			/ copy words
	movl	SPARG2, %ecx		/ count
	andl	$_A_NBPW-1, %ecx	/ copy remaining bytes
	rep;	smovb
	movl	%eax, %esi		/ restore registers
	movl	%edx, %edi
	ret				/ void return value
	SIZE(bcopy)


/
/ int
/ bcmp(const char *, const char *, size_t)
/	Compare two byte streams.
/
/ Calling/Exit State:
/	Returns 0 if they're identical, 1 if they're not.
/
ENTRY(bcmp)
	pushl	%esi
	movl	4+SPARG2, %ecx	/ count
	pushl	%edi
	movl	8+SPARG0, %esi	/ from
	shrl	$2, %ecx	/ %ecx = word count
	movl	8+SPARG1, %edi	/ to
	xorl	%eax, %eax	/ zero-extend %eax for int return value below
	repe; scmpl		/ compare words
	movl	8+SPARG2, %ecx	/ %ecx = remaining bytes; doesn't affect flags
	jne     .bcmp_ret
	andl	$_A_NBPW-1, %ecx
	repe; scmpb		/ compare remaining bytes
.bcmp_ret:
	popl	%edi		/ restore registers; doesn't affect flags
	popl	%esi
	setne	%al		/ sets %al to 0/1 based on results of scmpl
	ret
	SIZE(bcmp)

/
/ int
/ strcpy_max(char *dst, const char *src, size_t maxlen)
/ 
/	This function copies a 0-terminated string of bytes from src to dst,
/	unless this would take more than maxlen bytes (including the 0).
/
/ Calling/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.  This function returns the number
/	of bytes copied, not including the terminating zero byte,
/	unless there was not enough room, in which case it returns -1.
/
/ Remarks:
/	There are two ways to get to.smreturn, and both require %eax to be
/	decremented in order to have the proper return value.
/
/	One way to get to .smreturn is to fall through from a successful copy.
/	In this case, %eax contains the total number of bytes copied, including
/	the terminating NUL.  Since the return value is supposed to be the
/	string length, %eax has to be decremented.
/
/	The second way to get to .smreturn is if there was insufficient space
/	for the copy.  In this case, %eax is 0.  Since the return value is
/	supposed to be -1, %eax has to be decremented.
/ 

ENTRY(strcpy_max)
	movl	%edi, %edx		/ save registers
	pushl	%esi
	
	movl	4+SPARG1, %esi		/ calculate the string length
	movl	%esi, %edi		/ %edi = %esi = start of string
	xorl	%eax, %eax		/ %al = NUL character
	movl	4+SPARG2, %ecx		/ %ecx = maxlen
	repnz;	scab			/ scan for NUL terminator
	jnz	.smreturn		/ didn't find NUL, must be error
	movl	%edi, %ecx		/ %ecx = %edi - %esi (length to copy)
	subl	%esi, %ecx
	movl	%ecx, %eax		/ %eax = total length to copy
	movl	4+SPARG0, %edi		/ destination
	shrl	$2, %ecx		/ convert to count of words to copy
	rep;	smovl			/ copy words
	movl	%eax, %ecx		/ count
	andl	$_A_NBPW-1, %ecx	/ copy remaining bytes
	rep;	smovb
.smreturn:
	decl	%eax			/ decrement %eax (see Remarks)
	popl	%esi			/ restore registers
	movl	%edx, %edi
	ret
	SIZE(strcpy_max)

/
/ void
/ bscan(void *, size_t)
/	This function scans successive bytes of memory starting
/	at the first argument until the number of bytes scanned
/ 	equals the second argument.
/
/ Calling/Exit State:
/	None.
/
ENTRY(bscan)
	movl	%esi, %edx
	movl	SPARG0, %esi
	movl	SPARG1, %ecx
	rep
	lodsb
	movl	%edx, %esi
	ret
	SIZE(bscan)
