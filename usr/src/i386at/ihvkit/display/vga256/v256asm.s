#ident	"@(#)ihvkit:display/vga256/v256asm.s	1.1"

	.file	"v256256asm.s"


/
/	Copyright (c) 1992 USL
/	All Rights Reserved 
/
/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
/	The copyright notice above does not evidence any 
/	actual or intended publication of such source code.
/
/
/ Copyrighted as an unpublished work.
/ (c) Copyright 1990 INTERACTIVE Systems Corporation
/ All rights reserved.
/
/ RESTRICTED RIGHTS
/
/ These programs are supplied under a license.  They may be used,
/ disclosed, and/or copied only as permitted under such license
/ agreement.  Any copy must contain the above copyright notice and
/ this restricted rights notice.  Use, copying, and/or disclosure
/ of the programs is strictly prohibited unless otherwise provided
/ in the license agreement.
/

	.ident	"@(#)vga256:vga256/v256asm.s	1.2"

#include "vtdefs.h"
/
/  Calling sequences:
/    v256_moveup(dst, src, bytes)
/    v256_movedown(dst, src, bytes)
/
/    v256_memset(dest, color, bytes, keepmask)
/    v256_memsetmask(dest, color, bytes, keepmask)
/
/    v256_vidcpy(dest, src, bytes)
/    v256_memcpy(dest, src, bytes)
/    v256_memcpymask(dest, src, bytes, pmask)
/    v256_cpyinvert(dest, src, bytes, pmask)
/    v256_memxor(dest, src, bytes, pmask)
/    v256_memor(dest, src, bytes, pmask)
/    v256_memand(dest, src, bytes, pmask)
/    v256_memor_i(dest, src, bytes, pmask)
/    v256_memand_i(dest, src, bytes, pmask)
/
/    v256_stippletext(string, count, height, width)
/    v256_opaque_stpl(dst, src, expbg, expfg, count);
/    v256_xparent_stpl(dst, src, expfg, count);
/
	.text
	.globl	v256_moveup
	.globl	v256_movedown
	.globl	v256_memset
	.globl	v256_memsetmask
	.globl	v256_vidcpy
	.globl	v256_memcpy
	.globl	v256_memcpymask
	.globl	v256_cpyinvert
	.globl	v256_memxor
	.globl	v256_memor
	.globl	v256_memand
	.globl	v256_memor_i
	.globl	v256_memand_i
	.globl	v256_stippletext
	.globl	v256_opaque_stpl
	.globl	v256_xparent_stpl

	.globl	v256_slbuf
	.globl	v256_font_vector
	.globl	v256_blocksize
/
/  Copy from one area of the screen to another (source address lower)
/
v256_moveup:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	movl	20(%esp), %ecx
	jcxz	exit1
	movl	16(%esp), %esi
	movl	12(%esp), %edi
	pushf
	cld
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align1		/ If already aligned, skip single byte copy
	movsb			/ Copy the first, odd aligned, byte
	decl	%ecx
	jcxz	exit1a		/ We copied a single byte on an odd boundary
align1:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip1		/ No word moves, just copy 1-3 bytes
	rep ; movsl
skip1:
	popl	%ecx
	andl	$3, %ecx
	jz	exit1a
	rep ; movsb
exit1a:
	popf
exit1:
	popl	%edi
	popl	%esi
	ret
/
/  Copy from one area of the screen to another (source address higher)
/
v256_movedown:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	movl	20(%esp), %ecx
	jcxz	exit2
	movl	16(%esp), %esi
	movl	12(%esp), %edi
	pushf
	std
/
/  If EDI is even, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jnz	align2		/ If already aligned, skip single byte copy
	movsb			/ Copy the first, even aligned, byte
	decl	%ecx
	jcxz	exit2a		/ We copied a single byte on an even boundary
align2:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip2		/ No word moves, just copy 1-3 bytes
	subl	$3, %edi	/ Adjust address of byte 3 to that of byte 0
	subl	$3, %esi	/ Adjust address of byte 3 to that of byte 0
	rep ; movsl		/ Move the words
	addl	$3, %edi	/ Restore adjusted address to byte 3, next wd.
	addl	$3, %esi	/ Restore adjusted address to byte 3, next wd.
skip2:
	popl	%ecx
	andl	$3, %ecx
	jz	exit2a
	rep ; movsb
exit2a:
	popf
exit2:
	popl	%edi
	popl	%esi
	ret

/              v256_memset(dest, data, bytes)
/
v256_memset:
	MCOUNT			/ for profiling
	pushl	%edi
	movl	16(%esp), %ecx	/ bytes
	jcxz	exit61
/
/  The count is non-zero, do some work
/
	movl	12(%esp), %eax	/ data
	movl	8(%esp), %edi	/ dest
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align61		/ If already aligned, skip single byte copy
	stosb
	decl	%ecx
	jcxz	exit61		/ We copied a single byte on an odd boundary
align61:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip61		/ No word moves, just copy 1-3 bytes

	rep;	stosl		/ move all the words

skip61:
	popl	%ecx
	andl	$3, %ecx
	jz	exit61

	rep;	stosb

exit61:
	popl	%edi
	ret


/              v256_memsetmask(dest, data, bytes, mask)
/
v256_memsetmask:
	MCOUNT			/ for profiling
	pushl	%ebx
	pushl	%edi
	movl	20(%esp), %ecx	/ bytes
	jcxz	exit6
/
/  The count is non-zero, do some work
/
	movl	24(%esp), %ebx	/ mask
	movl	16(%esp), %edx	/ data
	movl	12(%esp), %edi	/ dest
	pushf
	cld
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align6		/ If already aligned, skip single byte copy
	movb	(%edi), %al	/ Copy the first, odd aligned, byte
	andb	%bl, %al
	orb	%dl, %al
	stosb
	decl	%ecx
	jcxz	exit6a		/ We copied a single byte on an odd boundary
align6:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip6		/ No word moves, just copy 1-3 bytes
loop6a:
	movl	(%edi), %eax	/ Copy pairs of 16-bit words a while
	andl	%ebx, %eax
	orl	%edx, %eax
	stosl
	loop	loop6a
skip6:
	popl	%ecx
	andl	$3, %ecx
	jz	exit6a
loop6b:
	movb	(%edi), %al
	andb	%bl, %al
	orb	%dl, %al
	stosb
	loop	loop6b
exit6a:
	popf
exit6:
	popl	%edi
	popl	%ebx
	ret


/
/	v256_vidcpy(dest, src, bytes)	-- copies memory.  Aligns to the
/					source pointer.
/
/
v256_vidcpy:
#ifdef	PROFILING
	.data
	.align	4
.v256_vidcpyTemp:	.long	0
	.text
	movl	$.v256_vidcpyTemp,	%edx
	call	_mcount
#endif	/* PROFILING */
	pushl	%esi
	pushl	%edi
	movl	20(%esp), %ecx	/ bytes
	jcxz	exit_vidcpy
/
/  The count is non-zero, do some work
/
	movl	16(%esp), %esi	/ src
	movl	12(%esp), %edi	/ dest

/  If ESI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %esi
	jz	align_vidcpy	/ If already aligned, skip single byte copy

	movsb
	decl	%ecx
	jcxz	exit_vidcpy	/ We copied a single byte on an odd boundary

align_vidcpy:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip_vidcpy	/ No word moves, just copy 1-3 bytes

	rep;	movsl		/ move all the words

skip_vidcpy:
	popl	%ecx
	andl	$3, %ecx
	jz	exit_vidcpy
	
	rep;	movsb		/ move remaining bytes

exit_vidcpy:
	popl	%edi
	popl	%esi
	ret


/
/	v256_memcpy(dest, src, bytes)	-- copies memory.  Aligns to the
/					destination pointer.
/
v256_memcpy:
#ifdef	PROFILING
	.data
	.align	4
.v256_memcpyTemp:	.long	0
	.text
	movl	$.v256_memcpyTemp,	%edx
	call	_mcount
#endif	/* PROFILING */
	pushl	%esi
	pushl	%edi
	movl	20(%esp), %ecx	/ bytes
	jcxz	exit71
/
/  The count is non-zero, do some work
/
	movl	16(%esp), %esi	/ src
	movl	12(%esp), %edi	/ dest

/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align71		/ If already aligned, skip single byte copy

	movsb
	decl	%ecx
	jcxz	exit71		/ We copied a single byte on an odd boundary

align71:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip71		/ No word moves, just copy 1-3 bytes

	rep;	movsl		/ move all the words

skip71:
	popl	%ecx
	andl	$3, %ecx
	jz	exit71
	
	rep;	movsb		/ move remaining bytes

exit71:
	popl	%edi
	popl	%esi
	ret


/
/              v256_memcpymask(dest=16, src=20, bytes=24, pmask=28)
/
v256_memcpymask:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	24(%esp), %ecx	/ bytes
	jcxz	exit7
/
/  The count is non-zero, do some work
/
	movl	28(%esp), %edx	/ mask
	notl	%edx
	movl	20(%esp), %esi	/ src
	movl	16(%esp), %edi	/ dest

/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align7		/ If already aligned, skip single byte copy

	lodsb
	movl	%eax, %ebx	/ write masked copy of source bits to
	xorb	(%edi), %al	/ destination bytes until the end of pattern
	andl	%edx, %eax	/ is reached.
	xorl	%ebx, %eax
	stosb
	decl	%ecx
	jcxz	exit7		/ We copied a single byte on an odd boundary

align7:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip7		/ No word moves, just copy 1-3 bytes

loop7a:
	lodsl
	movl	%eax, %ebx	/ write masked copy of source bits to
	xorl	(%edi), %eax	/ destination bytes until the end of pattern
	andl	%edx, %eax	/ is reached.
	xorl	%ebx, %eax
	stosl
	loop	loop7a

skip7:
	popl	%ecx
	andl	$3, %ecx
	jz	exit7

loop7b:
	lodsb
	movl	%eax, %ebx	/ write masked copy of source bits to
	xorb	(%edi), %al	/ destination bytes until the end of pattern
	andl	%edx, %eax	/ is reached.
	xorl	%ebx, %eax
	stosb
	loop	loop7b

exit7:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret

/              v256_memxor(dest=16, src=20, bytes=24, pmask=28)
/
v256_memxor:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	24(%esp), %ecx	/ bytes
	jcxz	exit8
/
/  The count is non-zero, do some work
/
	movl	28(%esp), %edx	/ mask
	movl	20(%esp), %esi	/ src
	movl	16(%esp), %edi	/ dest_pix
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align8		/ If already aligned, skip single byte copy
	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	xorb	(%edi), %al
	stosb
	decl	%ecx
	jcxz	exit8		/ We copied a single byte on an odd boundary

align8:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip8		/ No word moves, just copy 1-3 bytes

loop8a:
	lodsl
	andl	%edx, %eax	/ XOR masked source bits with destination
	xorl	(%edi), %eax
	stosl
	loop	loop8a

skip8:
	popl	%ecx
	andl	$3, %ecx
	jz	exit8

loop8b:
	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	xorb	(%edi), %al
	stosb
	loop	loop8b

exit8:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret

/              v256_memor(dest=16, src=20, bytes=24, pmask=28)
/
v256_memor:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	24(%esp), %ecx	/ bytes
	jcxz	exit9
/
/  The count is non-zero, do some work
/
	movl	28(%esp), %edx	/ mask
	movl	20(%esp), %esi	/ src
	movl	16(%esp), %edi	/ dest_pix
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align9		/ If already aligned, skip single byte copy

	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	orb	(%edi), %al
	stosb
	decl	%ecx
	jcxz	exit9		/ We copied a single byte on an odd boundary

align9:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip9		/ No word moves, just copy 1-3 bytes

loop9a:
	lodsl
	andl	%edx, %eax	/ XOR masked source bits with destination
	orl	(%edi), %eax
	stosl
	loop	loop9a

skip9:
	popl	%ecx
	andl	$3, %ecx
	jz	exit9

loop9b:
	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	orb	(%edi), %al
	stosb
	loop	loop9b

exit9:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret

/              v256_memand(dest=16, src=20, bytes=24, pmask=28)
/
v256_memand:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	24(%esp), %ecx	/ bytes
	jcxz	exit10
/
/  The count is non-zero, do some work
/
	movl	28(%esp), %edx	/ mask
	notl	%edx
	movl	20(%esp), %esi	/ src
	movl	16(%esp), %edi	/ dest_pix
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align10		/ If already aligned, skip single byte copy

	lodsb
	orl	%edx, %eax	/ XOR masked source bits with destination
	andb	(%edi), %al
	stosb
	decl	%ecx
	jcxz	exit10		/ We copied a single byte on an odd boundary

align10:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip10		/ No word moves, just copy 1-3 bytes

loop10a:
	lodsl
	orl	%edx, %eax	/ XOR masked source bits with destination
	andl	(%edi), %eax
	stosl
	loop	loop10a

skip10:
	popl	%ecx
	andl	$3, %ecx
	jz	exit10

loop10b:
	lodsb
	orl	%edx, %eax	/ XOR masked source bits with destination
	andb	(%edi), %al
	stosb
	loop	loop10b

exit10:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret

/              v256_memor_i(dest=16, src=20, bytes=24, pmask=28)
/
v256_memor_i:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	24(%esp), %ecx	/ bytes
	jcxz	exit11
/
/  The count is non-zero, do some work
/
	movl	28(%esp), %edx	/ mask
	movl	20(%esp), %esi	/ src
	movl	16(%esp), %edi	/ dest_pix
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align11		/ If already aligned, skip single byte copy

	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	orb	(%edi), %al
	xorl	%edx, %eax
	stosb
	decl	%ecx
	jcxz	exit11		/ We copied a single byte on an odd boundary

align11:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip11		/ No word moves, just copy 1-3 bytes

loop11a:
	lodsl
	andl	%edx, %eax	/ XOR masked source bits with destination
	orl	(%edi), %eax
	xorl	%edx, %eax
	stosl
	loop	loop11a

skip11:
	popl	%ecx
	andl	$3, %ecx
	jz	exit11

loop11b:
	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	orb	(%edi), %al
	xorl	%edx, %eax
	stosb
	loop	loop11b

exit11:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret

/              v256_memand_i(dest=16, src=20, bytes=24, pmask=28)
/
v256_memand_i:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	24(%esp), %ecx	/ bytes
	jcxz	exit12
/
/  The count is non-zero, do some work
/
	movl	28(%esp), %edx	/ mask
	movl	%edx, %ebx
	notl	%edx
	movl	20(%esp), %esi	/ src
	movl	16(%esp), %edi	/ dest_pix
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align12		/ If already aligned, skip single byte copy

	lodsb
	orl	%edx, %eax	/ XOR masked source bits with destination
	andb	(%edi), %al
	xorl	%ebx, %eax
	stosb
	decl	%ecx
	jcxz	exit12		/ We copied a single byte on an odd boundary

align12:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip12		/ No word moves, just copy 1-3 bytes

loop12a:
	lodsl
	orl	%edx, %eax	/ XOR masked source bits with destination
	andl	(%edi), %eax
	xorl	%ebx, %eax
	stosl
	loop	loop12a

skip12:
	popl	%ecx
	andl	$3, %ecx
	jz	exit12

loop12b:
	lodsb
	orl	%edx, %eax	/ XOR masked source bits with destination
	andb	(%edi), %al
	xorl	%ebx, %eax
	stosb
	loop	loop12b

exit12:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret

/              v256_cpyinvert(dest=16, src=20, bytes=24, pmask=28)
/
v256_cpyinvert:
	MCOUNT			/ for profiling
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	24(%esp), %ecx	/ bytes
	jcxz	exit13
/
/  The count is non-zero, do some work
/
	movl	28(%esp), %edx	/ mask
	movl	%edx, %ebx
	notl	%edx
	movl	20(%esp), %esi	/ src
	movl	16(%esp), %edi	/ dest_pix
/
/  If EDI is odd, transfer a single byte to align it on a word boundary
/
	test	$1, %edi
	jz	align13		/ If already aligned, skip single byte copy

	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	movb	(%edi), %bl
	orl	%edx, %ebx
	xorl	%ebx, %eax
	stosb
	decl	%ecx
	jcxz	exit13		/ We copied a single byte on an odd boundary

align13:
	pushl	%ecx		/ Save low two bits for later byte copy
	shrl	$2, %ecx	/ Convert to a word count
	jcxz	skip13		/ No word moves, just copy 1-3 bytes

loop13a:
	lodsl
	andl	%edx, %eax	/ XOR masked source bits with destination
	movl	(%edi), %ebx
	orl	%edx, %ebx
	xorl	%ebx, %eax
	stosl
	loop	loop13a

skip13:
	popl	%ecx
	andl	$3, %ecx
	jz	exit13

loop13b:
	lodsb
	andl	%edx, %eax	/ XOR masked source bits with destination
	movb	(%edi), %bl
	orl	%edx, %ebx
	xorl	%ebx, %eax
	stosb
	loop	loop13b

exit13:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret

/    v256_stippletext(string=24, count=28, height=32, width=36)
/
v256_stippletext:	/ 20
#ifdef	PROFILING
	.data
	.align	4
.v256_stippleTemp:	.long	0
	.text
	movl	$.v256_stippleTemp,	%edx
	call	_mcount
#endif	/* PROFILING */
	MCOUNT			/ for profiling
	movl	8(%esp), %eax
	orl	%eax, %eax
	jz	notext

	pushl	%ebx	/ 16
	pushl	%ebp	/ 12
	pushl	%esi	/ 8
	pushl	%edi	/ 4
	pushf		/ 0
	cld

	movl	$v256_slbuf, %ebx /start of the output buffer
	movl	32(%esp), %ebp	 /cache character height in EBP
	movl	%ebx, %edi
	movl	v256_blocksize, %eax
	mull	%ebp

	addl	$3,%eax
	shrl	$2,%eax
	movl	%eax, %ecx
	xor	%eax, %eax
	rep ; stosl		/ clear the output buffer (max 2048 bytes)

	xorl	%ecx, %ecx	/ initial offset is 0

fill_string:
	movl	24(%esp), %esi	/ string address
	xorl	%eax, %eax
	lodsw			/ fetch one character to expand (auto incr.)
	movl	%esi, 24(%esp)	/ increment the text pointer

	shll	$2, %eax
	mull	%ebp		/ %eax = offset to character in font table
	addl	v256_font_vector,%eax / add in the starting address of the font tbl.

	movl	%eax, %esi
	movl	%ebp, %edx	/ set up scan line loop count
	movl	%ebx, %edi
fill_onechar:
	lodsl			/ get bit pattern for one scanline
	shll	%cl, %eax	/ position the bits
	orl	%eax, (%edi)	/ merge with previous bytes
	addl	v256_blocksize, %edi	/ step to the next scanline
	decl	%edx
	jnz	fill_onechar	/ and loop until the character is done

	addl	36(%esp), %ecx	/ adjust starting column
	movl	%ecx, %eax
	shrl	$3, %eax
	andl	$7, %ecx	/ update starting bit number
	addl	%eax, %ebx	/ update starting byte number
	decl	28(%esp)	/ all characters done?
	jnz	fill_string	/ no, go do the next one

	popf
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
notext:
	ret


/   v256_opaque_stpl(dst=20, src=24, expbg=28, expfg=32, count=36);
v256_opaque_stpl:
	MCOUNT			/ for profiling
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	36(%esp), %ecx
	jcxz	not_opaq_stpl
	movl	28(%esp), %edx	/ background color (expanded)
	movl	32(%esp), %ebx	/ foreground color (expanded)
	xorl	%edx, %ebx	/ "difference"
	movl	20(%esp), %edi
	movl	24(%esp), %esi
	pushf
	cld
stpl_opaq_lp:
	xor	%eax, %eax
	lodsb			/ fetch the next byte to expand
	movl	%eax, %ebp
	movl	v256_lower(,%eax,4), %eax
	andl	%ebx, %eax
	xorl	%edx, %eax
	stosl
	movl	v256_upper(,%ebp,4), %eax
	andl	%ebx, %eax
	xorl	%edx, %eax
	stosl
	loop	stpl_opaq_lp
	popf
not_opaq_stpl:
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret


/    v256_xparent_stpl(dst=20, src=24, expfg=28, count=32);
v256_xparent_stpl:
	MCOUNT			/ for profiling
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	32(%esp), %ecx
	jcxz	not_xpnt_stpl
	movl	28(%esp), %edx	/ foreground color (expanded)
	movl	20(%esp), %edi
	movl	24(%esp), %esi
	pushf
	cld
stpl_xpnt_lp:
	xor	%eax, %eax
	lodsb			/ fetch the next byte to expand
	notb	%al
	movl	%eax, %ebp
	movl	v256_lower(,%eax,4), %ebx
	movl	%edx, %eax
	xorl	(%edi), %eax
	andl	%ebx, %eax
	xorl	%edx, %eax
	stosl
	movl	v256_upper(,%ebp,4), %ebx
	movl	%edx, %eax
	xorl	(%edi), %eax
	andl	%ebx, %eax
	xorl	%edx, %eax
	stosl
	loop	stpl_xpnt_lp
	popf
not_xpnt_stpl:
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/
/ void
/ repoutsw(port,from,numshorts)
/   short port;
/   short *from;
/   int numshorts;
/
/
	.globl repoutsw
PORT	= 8
FROM	= 12
NUM	= 16
repoutsw:
	MCOUNT			/ for profiling
	push %ebp
	movl %esp,%ebp
	pushl %edx
	pushl %esi
	pushl %ecx
	
	movl PORT(%ebp),%edx
	movl FROM(%ebp),%esi
	movl NUM(%ebp),%ecx
	repz ; outsw

	popl %ecx
	popl %esi
	popl %edx
	popl %ebp
	ret

