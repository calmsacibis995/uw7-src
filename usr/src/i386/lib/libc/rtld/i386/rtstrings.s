	.file	"rtstrings.s"

	.ident	"@(#)rtld:i386/rtstrings.s	1.2"

/ 386 assembler versions of string and memory routines needed
/ by rtld

/ void *_rt_memcpy(void *to, const void *from, size_t len)
	.globl	_rt_memcpy
	.type	_rt_memcpy,@function
	.text
	.align	16
_rt_memcpy:
	pushl	%edi
	pushl	%esi
	movl	12(%esp),%edi	/ %edi = dest address
	movl	16(%esp),%esi	/ %esi = source address
	movl	20(%esp),%ecx	/ %ecx = length of string
	movl	%edi,%eax	/ return value from the call

	movl	%ecx,%edx	/ %edx = number of bytes to move
	shrl	$2,%ecx		/ %ecx = number of words to move
	rep ; smovl		/ move the words

	movl	%edx,%ecx	/ %ecx = number of bytes to move
	andl	$0x3,%ecx	/ %ecx = number of bytes left to move
	rep ; smovb		/ move the bytes

	popl	%esi
	popl	%edi
	ret
	.align	4
	.size	_rt_memcpy,.-_rt_memcpy

/ int _rtstrcmp(const char *s1, const char *s2)
	.globl	_rtstrcmp
	.type	_rtstrcmp,@function
	.text
	.align	16
	.set	PGSZ,4096	/ strcmp does its comaprison
				/ a word at a time. This is a problem
				/ when comparing strings that are at
				/ end of a memory page, as moving a
				/ complete word may access out of
				/ bounds memory. If both arguments
				/ are correctly aligned all is well.
				/ If either of the args is incorrectly
				/ aligned we want to reduce down to
				/ word moves, keeping track of where
				/ the page boundary is. 
				/ The main loop handles 4 word
				/ comaprisons at a time. When we get
				/ within 16 bytes of the end of a page,
				/ we break out of the main loop
				/ and compare word at a time till we
				/ get within a word of the end of page,
				/ then we do a byte for byte comparison.
				/ If we don't exit the function, we
				/ are on a new page so we can return
				/ to the main loop.

_rtstrcmp:
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi	/ %esi = pointer to string 1
	movl	16(%esp),%edi	/ %edi = pointer to string 2
	cmpl	%esi,%edi	/ s1 == s2 ?
	je	.equal		/ they are equal
	.align	4

.again:				/ check for 4 byte aligned input strings
				/ if either of them is aligned jump out
				/ otherwise move byte at a time till
				/ one of the strings is aligned,
				/ then jump down
	testl	$3,%esi
	jz	.esialign
	testl	$3,%edi
	jz	.edialign
	movb	(%esi),%al
	movb	(%edi),%cl
	cmpb	%cl,%al
	jnz	.set_sign
	andb	%al,%al
	jz	.equal
	addl	$1,%esi
	addl	$1,%edi
	jmp	.again

.fixedi:
	movl	%edi,%edx
.setedx:
	andl	$PGSZ-1,%edx
	subl	$PGSZ,%edx
	negl	%edx		 / edx mod 16 guaranteed less than 4
				 / so if we approach end of memory
				 / page we will reduce down to word,
				 / then byte for byte comparisons
	jmp	.loop

.edialign:			/ always want to keep esi aligned
				/ this would confuse the return value
				/ see next comment...
	movl	%esi,%edx
	jmp	.setedx

.esialign:
	testl	$3,%edi
	jnz	.fixedi
	movl	$0x7fffffff,%edx
	jmp	.loop16		/ both are aligned
				/ save the subl and jl at least once
	.align	4
.loop:				/ iterate for cache performance
	subl	$16,%edx
	jl	.pad
.loop16:
	movl	(%esi),%eax	/ pick up 4-bytes from first string
	movl	(%edi),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jnz	.equal		/ there was a 0 in the 4-bytes
	movl	4(%esi),%eax	/ pick up 4-bytes from first string
	movl	4(%edi),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jnz	.equal		/ there was a 0 in the 4-bytes
	movl	8(%esi),%eax	/ pick up 4-bytes from first string
	movl	8(%edi),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jnz	.equal		/ there was a 0 in the 4-bytes
	movl	12(%esi),%eax	/ pick up 4-bytes from first string
	movl	12(%edi),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	addl	$16,%esi	/ increment first pointer
	addl	$16,%edi	/ increment second pointer
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jz	.loop		/ not yet at end of string, try again
	.align	4
.equal:				/ strings are equal, return 0
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret
	.align	4
.notequal:			/ two words are not the same, find out why
	cmpb	%cl,%al		/ see if individual bytes are the same
	jne	.set_sign	/ if not the same, go set sign
	andb	%al,%al		/ see if we hit the end of string
	je	.equal		/ yes, they are equal
	cmpb	%ch,%ah		/ check next byte...
	jne	.set_sign
	andb	%ah,%ah
	je	.equal
	shrl	$16,%eax
	shrl	$16,%ecx
	cmpb	%cl,%al
	jne	.set_sign
	andb	%al,%al
	je	.equal
	cmpb	%ch,%ah		/ last byte guaranteed not equal

.set_sign:			/ set the sign of the result for unequal
	sbbl	%eax,%eax
	orl	$1,%eax
	popl	%edi
	popl	%esi
	ret

.pad:
	addl	$16,%edx
	.backalign	.pad,4
.loop4:
	subl	$4,%edx
	jl	.4bytes
	movl    (%esi),%eax     / pick up 4-bytes from first string
	movl    (%edi),%ecx     / pick up 4-bytes from second string
	cmpl    %ecx,%eax       / see if they are equal
	jne     .notequal       / if not, find out why and where
	subl    $0x01010101,%eax        / see if we hit end of the string
	notl    %ecx
	andl    $0x80808080,%ecx
	andl    %ecx,%eax
	jnz     .equal          / there was a 0 in the 4-bytes
	addl	$4,%esi
	addl	$4,%edi
	jmp     .loop4   

.4bytes:			/ < 4 bytes from the end of a "page"
				/ either we find a difference, end of string
				/ or we push on to the next "page" and start over
	movb	(%esi),%al
	movb	(%edi),%cl
	cmpb	%cl,%al
	jne     .set_sign
	andb    %al,%al
	je      .equal

	movb	1(%esi),%al
	movb	1(%edi),%cl
	cmpb	%cl,%al
	jne     .set_sign
	andb    %al,%al
	je      .equal

	movb	2(%esi),%al
	movb	2(%edi),%cl
	cmpb	%cl,%al
	jne     .set_sign
	andb    %al,%al
	je      .equal

	movb	3(%esi),%al
	movb	3(%edi),%cl
	cmpb	%cl,%al
	jne     .set_sign
	andb    %al,%al
	je      .equal

	addl	$4,%esi
	addl	$4,%edi
	addl	$PGSZ,%edx
	jmp	.loop
	.align	4
	.size	_rtstrcmp,.-_rtstrcmp

/ int _rtstrncmp(const char *s1, const char *s2, size_t len)
	.globl	_rtstrncmp
	.type	_rtstrncmp,@function
	.text
	.align	16
_rtstrncmp:
	movl	%edi,%edx	/ save register variables
	pushl	%esi

	movl	8(%esp),%esi	/ %esi = first string
	movl	12(%esp),%edi	/ %edi = second string
	cmpl	%esi,%edi	/ same string?
	je	.nequal
	movl	16(%esp),%ecx	/ %ecx = length
	incl	%ecx		/ will later predecrement this uint
.nloop:
	decl	%ecx
	je	.nequal		/ Used all n chars?
	slodb ; scab
	jne	.nnotequal	/ Are the bytes equal?
	testb	%al,%al
	je	.nequal		/ End of string?

	decl	%ecx
	je	.nequal		/ Used all n chars?
	slodb ; scab
	jne	.nnotequal	/ Are the bytes equal?
	testb	%al,%al
	je	.nequal		/ End of string?

	decl	%ecx
	je	.nequal		/ Used all n chars?
	slodb ; scab
	jne	.nnotequal	/ Are the bytes equal?
	testb	%al,%al
	je	.nequal		/ End of string?

	decl	%ecx
	je	.nequal		/ Used all n chars?
	slodb ; scab
	jne	.nnotequal	/ Are the bytes equal?
	testb	%al,%al
	jne	.nloop		/ End of string?

.nequal:
	xorl	%eax,%eax	/ return 0
	popl	%esi		/ restore registers
	movl	%edx,%edi
	ret

	.align	4
.nnotequal:
	jnc	.ret2
	movl	$-1,%eax	
	jmp	.ret1
.ret2:
	movl	$1,%eax
.ret1:
	popl	%esi		/ restore registers
	movl	%edx,%edi
	ret
	.align	4
	.size	_rtstrncmp,.-_rtstrncmp


/ int _rtstrcpy(char *to, const char *from)
	.globl	_rtstrcpy
	.type	_rtstrcpy,@function
	.text
	.align	16
_rtstrcpy:
	movl	%edi,%edx	/ save register variables
	pushl	%esi

	movl	12(%esp),%edi	/ %edi = source string address
	xorl	%eax,%eax	/ %al = 0 (search for 0)
	movl	$-1,%ecx	/ length to look: lots
	repnz ; scab

	notl	%ecx		/ %ecx = length to move
	movl	12(%esp),%esi	/ %esi = source string address
	movl	8(%esp),%edi	/ %edi = destination string address
	movl	%ecx,%eax	/ %eax = length to move
	shrl	$2,%ecx		/ %ecx = words to move
	rep ; smovl

	movl	%eax,%ecx	/ %ecx = length to move
	andl	$3,%ecx		/ %ecx = leftover bytes to move
	rep ; smovb

	movl	8(%esp),%eax	/ %eax = returned dest string addr
	popl	%esi		/ restore register variables
	movl	%edx,%edi
	ret
	.align	4
	.size	_rtstrcpy,.-_rtstrcpy

/ int _rtstrlen(const char *p)
	.globl	_rtstrlen
	.type	_rtstrlen,@function
	.text
	.align	16
_rtstrlen:
	movl	%edi,%edx	/ save register variables

	movl	4(%esp),%edi	/ string address
	xorl	%eax,%eax	/ %al = 0
	movl	$-1,%ecx	/ Start count backward from -1.
	repnz ; scab
	incl	%ecx		/ Chip pre-decrements.
	movl	%ecx,%eax	/ %eax = return values
	notl	%eax		/ Twos complement arith. rule.

	movl	%edx,%edi	/ restore register variables
	ret
	.align	4
	.size	_rtstrlen,.-_rtstrlen

/ void _rtclear(void *dst, size_t cnt)
/ Fast assembly routine to zero page.
/ Sets cnt bytes to zero, starting at dst
/ Dst and cnt must be coordinated to give word alignment
/ (guaranteed if used to zero a page)
	.globl	_rtclear
	.type	_rtclear,@function
	.text
	.align	16
_rtclear:
	pushl	%edi		/ save register
	movl	8(%esp),%edi	/ dst
	movl	12(%esp),%ecx	/ cnt
	xorl	%eax,%eax	/ 0 - value to store
	movl	%ecx,%edx
	andl	$3,%ecx
	rep; stosb		/ clear the memory
	movl	%edx,%ecx
	shrl	$2,%ecx
	rep; stosl		/ clear the memory
	popl	%edi
	ret
	.align	4
	.size	_rtclear,.-_rtclear
