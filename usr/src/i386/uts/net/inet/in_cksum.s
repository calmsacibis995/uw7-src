	.ident	"@(#)in_cksum.s	1.2"
	.ident	"$Header$"
	.file	"net/inet/in_cksum.s"

/   Checksum routine for TCP/IP headers and packets
/
/	in_cksum calculates the one's complement of the 16 bit one's
/	complement sum of "len" bytes of data in the streams message "bp".
/	NOTE: we are using %ebp as a scratch register so be very careful
/	about adding pushes and pops because you'll have to recalculate
/	offsets to the local variables and arguments.
/
/ unsigned short int
/ in_cksum(mblk_t *bp, int len)
/
/	registers used:
/		%eax - scratch register
/		%ebx - scratch register
/		%ecx - byte count, checksum and scratch register
/		%edx - checksum and scratch register
/		%edi - data pointer
/		%esi - data pointer
/		%ebp - holds the running checksum for the message

/	local variables allocated:
/		36(%esp)	temporary storage
/		32(%esp)	previous message blocks' byte count
/		28(%esp)	byte count storage
/		24(%esp)	saved alignment of this message block
/		20(%esp)	saved block pointer
/		16(%esp)	this message block's original byte count
/	
	.text
	.align	4
	.globl	in_cksum

in_cksum:
	subl	$24,%esp	/ reserve local variables
	pushl	%ebp		/ save registers
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	$0,32(%esp)	/ zero previous message blocks' byte count

	xorl	%ebp,%ebp	/ zero message checksum
	cmpl	$0,48(%esp)	/ if (message length <= 0) then nothing to do
	jle	.exit

	movl	44(%esp),%edi	/ initialize block pointer
	testl	%edi,%edi	/ is pointer non-NULL?
	jz	.exit		/ if so, then nothing to do
	jmp	.first_mblk	/ on initial block, don't indirect on b_cont
	
.next_mblk:
	cmpl	$0,48(%esp)	/ if (message length <= 0) then exit
	jle	.exit

	movl	20(%esp),%edi	/ restore block pointer
	movl	8(%edi),%edi	/ get next block pointer (b_cont)
	testl	%edi,%edi	/ if (block pointer = 0) then exit
	jz	.exit

.first_mblk:
	movl	%edi,20(%esp)	/ save block pointer
	movl	12(%edi),%esi	/ get read pointer (b_rptr)
	movl	16(%edi),%ecx	/ get write pointer (b_wptr)
	subl	%esi,%ecx	/ compute block length (b_wptr - b_rptr)
	jle	.next_mblk	/ if (block length <= 0) then get next block

	subl	%ecx,48(%esp)	/ message length - block length
	jge	.proc_mblk	/ if (message length >= block length) then OK

	addl	48(%esp),%ecx	/ else adjust block length

.proc_mblk:
	movl	%ecx,16(%esp)	/ save total byte count
	movl	%ecx,28(%esp)	/ save current byte count
	xorl	%edx,%edx	/ zero message block checksum
	movl	%esi,%eax	/ get current value of b_rptr
	andl	$0x3,%eax	/ determine alignment of b_rptr
	movl	%eax,24(%esp)	/ save this message block's alignment
	je	.size_mblk	/ previous move didn't muck with flags

	movl	$0,36(%esp)	/ zero tmp storage
	leal	36(%esp),%ebx
	addl	%eax,%ebx	/ adjust %ebx to account for alignment
	movl	%ecx,%edx	/ copy byte count
	jmp	.proc_mblk2

.proc_mblk1:
	movl	%esi,%eax	/ check to see if we are now aligned
	andl	$0x3,%eax
	je	.proc_mblk3

.proc_mblk2:
	movb	(%esi),%al	/ move one byte
	movb	%al,(%ebx)	/ to tmp storage
	incl	%esi		/ point at next byte
	incl	%ebx		/ point at next tmp storage byte
	decl	%ecx		/ decrement byte count
	jg	.proc_mblk1	/ bail-out if no more bytes available

.proc_mblk3:
	subl	%ecx,%edx	/ determine how many bytes we moved
	movl	28(%esp),%ecx	/ get current byte count
	subl	%edx,%ecx	/ subtract the number of bytes moved
	movl	36(%esp),%edx	/ copy from tmp storage
	movl	%ecx,28(%esp)	/ save new byte count

.size_mblk:
	movl	%ecx,%eax	/ compute number of 64 byte blocks
	andl	$0x3f,%ecx
	shrl	$6,%eax
	jz	.less_than_64_bytes

/ Use %esi to point to the first "half" of the data (usually 32 bytes) and %edi
/ to point to the second "half" of the data (usually 32 bytes).  Accumulate the
/ checksum for the second "half" independently of the running sum (including
/ the first "half") and then add the second sum into the running sum.  This
/ provides for data independence that will allow pairing of instructions.

	xorl	%ecx,%ecx
	leal	32(%esi),%edi	/ set %edi to the "top half"
	movl	$32,%ebx	/ half the count of bytes being processed
	jmp	.sum_64_bytes

/ the above jump saves on executing nop's while still having
/ .sum_64_bytes aligned for fast instruction fetching

	.align	16
.sum_64_bytes:
	addl	28(%esi),%edx	/ add this integer into the running checksum
	adcl	$0,%edx		/ don't forget the carry bit
	addl	28(%edi),%ecx	/ add this integer into the temporary checksum
	adcl	$0,%ecx		/ don't forget the carry bit
.sum_56_bytes:
	addl	24(%esi),%edx	/ ditto
	adcl	$0,%edx
	addl	24(%edi),%ecx
	adcl	$0,%ecx
.sum_48_bytes:
	addl	20(%esi),%edx	/ ditto
	adcl	$0,%edx
	addl	20(%edi),%ecx
	adcl	$0,%ecx
.sum_40_bytes:
	addl	16(%esi),%edx	/ ditto
	adcl	$0,%edx
	addl	16(%edi),%ecx
	adcl	$0,%ecx
.sum_32_bytes:
	addl	12(%esi),%edx	/ ditto
	adcl	$0,%edx
	addl	12(%edi),%ecx
	adcl	$0,%ecx
.sum_24_bytes:
	addl	8(%esi),%edx	/ ditto
	adcl	$0,%edx
	addl	8(%edi),%ecx
	adcl	$0,%ecx
.sum_16_bytes:
	addl	4(%esi),%edx	/ ditto
	adcl	$0,%edx
	addl	4(%edi),%ecx
	adcl	$0,%ecx
.sum_8_bytes:
	addl	(%esi),%edx	/ ditto
	adcl	$0,%edx
	addl	(%edi),%ecx
	adcl	$0,%ecx

.sum_0_bytes:
	leal	(%esi,%ebx,2),%esi	/ add 2 times %ebx to %esi
	leal	(%edi,%ebx,2),%edi	/ add 2 times %ebx to %edi
	decl	%eax
	jg	.sum_64_bytes

	addl	%ecx,%edx	/ combine both halves of the checksum
	adcl	$0,%edx		/ don't forget the carry bit
	movl	28(%esp),%ecx	/ restore the byte count
	cmpl	$0,%eax		/ need to reset the flags
	jl	.less_than_8_bytes

.less_than_64_bytes:
	movl	%ecx,%ebx	/ compute size of last block (< 64 bytes)
	andl	$0x3f,%ebx
	shrl	$2,%ebx		/ determine number of ints
	cmpl	$1,%ebx
	jle	.less_than_8_bytes

/ At this point we have between 8 and 60 bytes left.  We'll play games to
/ process any even number of ints similarly to how we handled a block of
/ 64 bytes above (i.e. divide the work in two - %esi,%edx and %edi,%ecx).

	xorl	%ecx,%ecx
	andl	$0xfffffffe,%ebx	/ set least significant bit to zero
	shll	$1,%ebx			/ convert to number of bytes
	leal	(%esi,%ebx),%edi	/ set %edi to the "top half"
	jmp	*.jump1(,%ebx,4)

/ At this point we should have between 0 and 7 bytes.
/ If we have at least 4 bytes we'll add it in as an int.

.less_than_8_bytes:
	movl	%ecx,%ebx
	shrl	$2,%ebx		/ determine number of ints
	andl	$1,%ebx		/ did we have an odd number of ints?
	jz	.less_than_4_bytes

	addl	(%esi),%edx	/ add this integer into the running checksum
	adcl	$0,%edx		/ don't forget the carry bit
	addl	$4,%esi

/ At this point we should have between 0 and 3 bytes.

.less_than_4_bytes:
	andl	$0x3,%ecx
	je	.end_mblk

	movl	$0,36(%esp)	/ zero tmp storage
	leal	36(%esp),%ebx
	jmp	*.jump2(,%ecx,4)

.sum_3_bytes:
	movb	(%esi),%al	/ copy bytes to tmp
	movb	%al,(%ebx)
	movb	1(%esi),%al
	movb	%al,1(%ebx)
	movb	2(%esi),%al
	movb	%al,2(%ebx)
	addl	$3,%esi
	subl	$3,%ecx
	jmp	.add_bytes

.sum_2_bytes:
	movb	(%esi),%al	/ copy bytes to tmp
	movb	%al,(%ebx)
	movb	1(%esi),%al
	movb	%al,1(%ebx)
	addl	$2,%esi
	subl	$2,%ecx
	jmp	.add_bytes

.sum_1_byte:
	movb	(%esi),%al	/ copy byte to tmp
	movb	%al,(%ebx)
	incl	%esi
	decl	%ecx

.add_bytes:
	addl	(%ebx),%edx	/ add byte(s) to checksum
	adcl	$0,%edx		/ don't forget the carry bit

/ Now we need to perform our magic based on previous block's odd-byte count
/ and this block's original alignment.

.end_mblk:
	xorl	%ebx,%ebx
	movb	24(%esp),%bl	/ get this message block's alignment
	shll	$2,%ebx
	xorl	%eax,%eax
	movl	32(%esp),%eax	/ get previous message blocks' byte count
	andl	$0x3,%eax	/ determine if not an even multiple of 4 bytes
	orl	%eax,%ebx
	movl	16(%esp),%edi	/ get message block's original byte count
	addl	%eax,%edi	/ add this block's count to message's count
	movl	%edi,32(%esp)	/ save this message block's odd-byte count
	jmp	*.jump3(,%ebx,4)

.rotate_l8:
	roll	$8,%edx
	jmp	.end_mblk1

.rotate_l16:
	roll	$16,%edx
	jmp	.end_mblk1

.rotate_r8:
	rorl	$8,%edx
	jmp	.end_mblk1

/ Now we need to add this message block's checksum into the message's checksum.

.end_mblk1:
	addl	%edx,%ebp
	adcl	$0,%ebp		/ don't forget the carry bit
	jmp	.next_mblk	/ %edx is cleared at ".proc_mblk:"

.exit:
	cmpl	$0,48(%esp)	/ if (message length = 0) then OK
	jle	.exit2

	pushl	$.error_msg1	/ else print error message
	call	printf		/ GEM: shouldn't this be cmn_err?
	popl	%eax

.exit2:
	movl	%ebp,%edx	/ get message checksum
	movl	%edx,%eax	/ form a 16 bit checksum by
	shrl	$16,%eax	/ add two halves of the 32 bit checksum
	addw	%dx,%ax
	adcw	$0,%ax		/ don't forget the carry bit
	notw	%ax		/ return 1's complement of checksum
	andl	$0xffff,%eax

	popl	%edi		/restore registers
	popl	%esi
	popl	%ebx
	popl	%ebp
	addl	$24,%esp	/ free local variables

	ret

.impossible:
	pushl	$.error_msg2	/ else print error message
	call	printf		/ GEM: shouldn't this be cmn_err?
	popl	%eax
	jmp	.exit2

	.data
	.align	4

.error_msg1:
	.string	"in_cksum: out of data\n"

.error_msg2:
	.string "in_cksum: impossible byte count\n"

.jump1:
	.long	.sum_0_bytes,	.impossible,	.sum_0_bytes,	.impossible
	.long	.sum_8_bytes,	.impossible,	.sum_8_bytes,	.impossible
	.long	.sum_16_bytes,	.impossible,	.sum_16_bytes,	.impossible
	.long	.sum_24_bytes,	.impossible,	.sum_24_bytes,	.impossible
	.long	.sum_32_bytes,	.impossible,	.sum_32_bytes,	.impossible
	.long	.sum_40_bytes,	.impossible,	.sum_40_bytes,	.impossible
	.long	.sum_48_bytes,	.impossible,	.sum_48_bytes,	.impossible
	.long	.sum_56_bytes,	.impossible,	.sum_56_bytes,	.impossible

.jump2:
	.long	.end_mblk,	.sum_1_byte,	.sum_2_bytes,	.sum_3_bytes

.jump3:
	.long	.end_mblk1,	.rotate_l8,	.rotate_l16,	.rotate_r8
	.long	.rotate_r8,	.end_mblk1,	.rotate_l8,	.rotate_l16
	.long	.rotate_l16,	.rotate_r8,	.end_mblk1,	.rotate_l8
	.long	.rotate_l8,	.rotate_l16,	.rotate_r8,	.end_mblk1
