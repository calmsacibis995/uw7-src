	.ident	"@(#)kern-i386:util/string.s	1.5.2.1"
	.file	"util/string.s"

include(KBASE/svc/asm.m4)
include(assym_include)

/
/ int
/ strcmp(const char *, const char *)
/
/	Returns the signed comparison of the two arguments.
/
/ Calling State/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/ 	return values: s1>s2: >0  s1==s2: 0  s1<s2: <0
/
/ Remarks:
/	strcmp does its comparison a word at a time. This is a problem
/	when comparing strings that are at
/	end of a memory page, as moving a complete word may access out of
/	bounds memory. If both arguments are correctly aligned all is well.
/	If either of the args is incorrectly aligned we want to reduce down to
/	word moves, keeping track of where the page boundary is. 
/	The main loop handles 4 word comparisons at a time. When we get
/	within 16 bytes of the end of a page, we break out of the main loop
/	and compare word at a time till we get within a word of the end of page,
/	then we do a byte for byte comparison.  If we don't exit the function,
/	we are on a new page so we can return to the main loop.

ENTRY(strcmp)
	pushl	%esi
	pushl	%edi
	movl	8+SPARG0(%esp),%esi	/ %esi = pointer to string 1
	movl	8+SPARG1(%esp),%edi	/ %edi = pointer to string 2
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
	andl	$_A_PAGESIZE-1,%edx
	subl	$_A_PAGESIZE,%edx
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
				/ or we push on to the next "page" and start
				/ over
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
	addl	$_A_PAGESIZE,%edx
	jmp	.loop
	SIZE(strcmp)

/
/ int
/ strlen(const char *)
/
/	Return the integer length of the string pointed at by
/	the argument (not including the terminating null).
/
/ Calling State/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
/	The return value is the length (count of non-null bytes).
/

ENTRY(strlen)
	movl	%edi, %edx	/ save register variables
	movl	SPARG0,%edi	/ string address
	xorl	%eax,%eax	/ %al = 0
	movl	$-1,%ecx	/ Start count backward from -1.
	repnz ; scab
	incl	%ecx		/ Chip pre-decrements.
	movl	%ecx,%eax	/ %eax = return values
	notl	%eax		/ Twos complement arith. rule.
	movl	%edx, %edi	/ restore register variables
	ret
	SIZE(strlen)

/
/ char *
/ strcpy(char *, const char *)
/
/	Copy the string of characters pointed at by the second
/	argument to the address given by the first argument
/	stopping when a null character is encountered.
/	The space allocated to the first argument must be large
/	enough.
/
/ Calling State/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
/	The return value is the first argument.
/

ENTRY(strcpy)
	pushl	%edi		/ save register variables
	movl	%esi,%edx

	movl	4+SPARG1,%edi	/ %edi = source string address
	xorl	%eax,%eax	/ %al = 0 (search for 0)
	movl	$-1,%ecx	/ length to look: lots
	repnz ; scab

	notl	%ecx		/ %ecx = length to move
	movl	4+SPARG1,%esi	/ %esi = source string address
	movl	4+SPARG0,%edi	/ %edi = destination string address
	movl	%ecx,%eax	/ %eax = length to move
	shrl	$2,%ecx		/ %ecx = words to move
	rep ; smovl

	movl	%eax,%ecx	/ %ecx = length to move
	andl	$3,%ecx		/ %ecx = leftover bytes to move
	rep ; smovb

	movl	4+SPARG0,%eax	/ %eax = returned dest string addr
	movl	%edx,%esi	/ restore register variables
	popl	%edi
	ret
	SIZE(strcpy)

/
/ char *
/ strcat(char *, const char *)
/
/	Concatenate the second argument on the end of the first argument.
/
/ Calling State/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
/	The return value is the first argument.	
/

ENTRY(strcat)
	pushl	%esi		/ save registers
	pushl	%edi

	movl	8+SPARG1, %esi	/ get source address
	movl	%esi, %edi	/ save for later
	xorl	%eax, %eax	/ search \0 char
	movl	$-1, %ecx	/ in many chars
	repnz ;	scab

	notl	%ecx		/ number to copy
	movl	%ecx, %edx	/ save for copy
	
	movl	8+SPARG0,%edi	/ get destination end address
	movl	$-1, %ecx	/ search for many
	repnz ; scab

	decl	%edi		/ backup 1 byte

	movl	%edx, %ecx	/ bytes to copy
	shrl	$2, %ecx	/ double to copy
	rep ;	smovl

	movl	%edx, %ecx	/ bytes to copy
	andl	$3, %ecx	/ mod 4
	rep ;	smovb

	movl	8+SPARG0, %eax

	popl	%edi		/ restore registers
	popl	%esi
	ret
	SIZE(strcat)

/
/ char *
/ strncat(char *, const char *, size_t)
/
/	Concatenate the second argument on the end of the first argument.
/	At most the third argument number of characters are moved.
/
/ Calling State/Exit State:
/
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
/	The return value is the first argument.	
/

ENTRY(strncat)
	pushl	%esi
	pushl	%edi

	movl	8+SPARG2, %ecx	/ max number to copy
	cmpl	$0, %ecx
	jz	.strncat_ret

	movl	%ecx, %edx
	movl	8+SPARG1, %esi	/ get source address
	movl	%esi, %edi	/ save for later
	xorl	%eax, %eax	/ search \0 char
	repnz ;	scab

	jne	.strncat_no_null
	incl	%ecx
.strncat_no_null:

	subl	%ecx, %edx	/ bytes to copy

	movl	8+SPARG0,%edi	/ get destination end address
	movl	$-1, %ecx	/ search for many
	repnz ; scab

	decl	%edi		/ backup 1 byte

	movl	%edx, %ecx	/ copy bytes - most suffixes are not very long;
	rep ;	smovb		/   no worth setup for longword moves

	sstob			/ null byte terminator

.strncat_ret:
	movl	8+SPARG0, %eax

	popl	%edi
	popl	%esi
	ret
	SIZE(strncat)

/
/ int
/ strcpy_len(char *, const char *)
/
/ Copy string s2 to s1.  s1 must be large enough. Return the length
/ of the string.  This function is written for use by the copyarglist()
/ function, which previously did a strlen(strcpy(s1, s2)) sequence and
/ hence saves a pass over the string.
/
/ Assembly language version of the following C function:
/	int
/	strcpy_len(register char *s1, const register char *s2)
/	{
/		register int cnt = 0;
/	
/		while (*s1++ = *s2++)
/			cnt++;
/		return(cnt);
/	}
/
/ Calling State/Exit State:
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
/	The return value is the length of string copied (ala strlen()).
/

ENTRY(strcpy_len)
	pushl	%edi		/ save register variables
	movl	%esi,%edx

	movl	SPARG2,%edi	/ %edi = source string address (s2)
	xorl	%eax,%eax	/ %al = 0 (search for 0)
	movl	$-1,%ecx	/ length to look: lots
	repnz ; scab

	notl	%ecx		/ %ecx = length to move
	movl	SPARG2,%esi	/ %esi = source string address (s2)
	movl	SPARG1,%edi	/ %edi = destination string address (s1)
	movl	%ecx,%eax	/ %eax = length to move
	shrl	$2,%ecx		/ %ecx = words to move
	rep ; smovl

	movl	%eax,%ecx	/ %ecx = length to move
	andl	$3,%ecx		/ %ecx = leftover bytes to move
	rep ; smovb
				/ count already in eax, return it
	movl	%edx,%esi	/ restore register variables
	popl	%edi
	decl	%eax		/ make return value compatible with strlen()
	ret
	SIZE(strcpy_len)
