	.file	"strcmp.s"

	.ident	"@(#)libc-i386:str/strcmp.s	1.6"

	.globl	strcmp
	.align	4
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

_fgdef_(strcmp):
	MCOUNT			/ subroutine ertry counter if profiling
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
