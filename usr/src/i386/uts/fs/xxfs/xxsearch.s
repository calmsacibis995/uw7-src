	.ident	"@(#)kern-i386:fs/xxfs/xxsearch.s	1.2"
	.file	"fs/xx/xxsearch.s"

include(../../svc/asm.m4)

	.set	BUF, 8
	.set	BUFSIZ,	12
	.set	TARGET,	16
	.set	DIRENT,	16	/ size of a directory entry
	.set	DIRSIZ,	14	/ size of a file name

	.align	4
/
/ int
/ xx_searchdir(char *, int, char []) 
/	Search a directory for target.
/
/ Calling/Exit State:
/	The directory is held *shared* on entry and on exit.
/
/ Description:
/	Return offset into directory of match; if no match is found,
/       then return offset into directory of empty slot, or if none
/	found, then return -1.
/
ENTRY(xx_searchdir)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	BUF(%ebp), %esi			/ pointer to directory
	movl	BUFSIZ(%ebp), %ebx		/ directory length in bytes
	movl	$0, %edx			/ pointer to empty slot
			/ get length of target string
	movl	TARGET(%ebp), %edi		/ address of target name
	movl	$DIRSIZ, %ecx
	movb	$0, %al
	repnz
	scab
	movl	$DIRSIZ, %eax
	subl	%ecx, %eax			/ %eax=length of target
.s_top:
	cmpl	$DIRENT, %ebx			/ length less than 16?
	jl	.sdone				/ done if less
	cmpw	$0, (%esi)			/ directory entry empty?
	je	.sempty				/ jump if true
	pushl	%esi				/ save start of entry
	addl	$2, %esi			/ address of file name
	movl	TARGET(%ebp), %edi		/ address of target name

	movl	%eax, %ecx			/ length of target name
	repz
	scmpb
	cmpl	$0, %ecx
	jz	.smatch				/ the names match
	popl	%esi				/ restore start of entry
.scont:
	addl	$DIRENT, %esi			/ increment directory pointer
	subl	$DIRENT, %ebx			/ decrement size
	jmp	.s_top				/ keep looking

	.align	4
.sempty:
	cmpl	$0, %edx			/ do we need an empty slot?
	jne	.scont				/ jump if no
	movl	%esi, %edx			/ save current offset
	jmp	.scont				/ and goto to next entry

	.align	4
.smatch:
	movb	-1(%esi), %cl
	cmpb	%cl, -1(%edi)
	je 	.srmatch				/ really a match
	popl	%esi				/ restore start of entry
	jmp	.scont				/ not really a match.
						/ Just a substring
	.align	4
.srmatch:
	popl	%esi				/ restore start of entry
	subl	BUF(%ebp), %esi		 	/ convert to offset
	movl	%esi, %eax			/ return offset
	jmp	.s_exit

	.align	4
.sdone:
	movl	$-1, %eax			/ save failure return
	cmpl	$0, %edx			/ empty slot found?
	je	.sfail				/ jump if false
	subl	BUF(%ebp), %edx			/ convert to offset
	movl	%edx, %eax			/ return empty slot
.sfail:
.s_exit:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret	

	.size	xx_searchdir,.-xx_searchdir
