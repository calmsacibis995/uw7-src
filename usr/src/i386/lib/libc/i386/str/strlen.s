	.file	"strlen.s"
	.ident	"@(#)libc-i386:str/strlen.s	1.6"

	.globl	strlen

	.align	4
_fgdef_(strlen):
	MCOUNT
	movl	4(%esp),%ecx
	testl	$3,%ecx		/ see if string is 4 byte aligned
	jz	.loop

.unaligned:
	movb	(%ecx),%al	/ byte loop until string is aligned
	incl	%ecx
	testb	%al,%al
	jz	.byte_3
	testl	$3,%ecx		/ aligned now?
	jnz	.unaligned

.loop:
	movl	(%ecx),%eax	/ get 4 bytes

	/ Attempt to locate a zero byte in %eax
	/ If we could assume that most bytes were [0,0x80], the simpler
	/	addl	$4,%ecx
	/	subl	$0x01010101,%eax
	/	testl	$0x80808080,%eax
	/	jz	.loop
	/ could be used, but any bytes are [0x81,0xff], we will always
	/ fall into the "which byte is zero" testing, adding a higher
	/ overhead.  Instead, we do the more complex--but still pairable
	/ to 4 clocks/loop on the Pentium--sequence which asks whether
	/ 	(((%eax + 0x7efefeff) ^ ~%eax) & ~0x7efefeff) != 0
	/ This essentially tests whether the three low order bytes
	/ are zero by looking whether any of the bits 0x01010100 are
	/ unchanged after the addition.  Since we don't have a 33rd bit
	/ (not) to spill into for the high order byte, we instead look
	/ for a unchanged high order bit after the addition of 0x7f
	/ (we must have a carry from the lower byte since it wasn't
	/ a zero).  This means that we will have a false positive in
	/ the testl below if the high order byte was 0x80.

	movl	$0x7efefeff,%edx
	addl	%eax,%edx
	xorl	$0xffffffff,%eax	/ could have been 0x81010100
	xorl	%edx,%eax
	addl	$4,%ecx
	testl	$0x81010100,%eax
	jz	.loop

	/ Probably had a zero byte; get the 4 bytes again and see.

	movl	-4(%ecx),%eax 
	testb	%al,%al
	jz	.byte_0
	testb	%ah,%ah
	jz	.byte_1
	testl	$0xff0000,%eax
	jz	.byte_2
	testl	$0xff000000,%eax
	jnz	.loop		/ taken only if high byte was 0x80

.byte_3:
	leal	-1(%ecx),%eax
	movl	4(%esp),%ecx
	subl	%ecx,%eax
	ret

.byte_2:
	leal	-2(%ecx),%eax
	movl	4(%esp),%ecx
	subl	%ecx,%eax
	ret

.byte_1:
	leal	-3(%ecx),%eax
	movl	4(%esp),%ecx
	subl	%ecx,%eax
	ret

.byte_0:
	leal	-4(%ecx),%eax
	movl	4(%esp),%ecx
	subl	%ecx,%eax
	ret
