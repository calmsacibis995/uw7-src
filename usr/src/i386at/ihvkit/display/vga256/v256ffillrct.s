	.ident	"@(#)ihvkit:display/vga256/v256ffillrct.s	1.2"

	/* -*- Text -*- */
	.file	"v256ffillrct.s"	
#define	__ASSEMBLER__
#include "v256as.h"
/*
 *	Helper Code: v256FFillRect
 *	
 *	Description:
 *	Filled rectangles in GXcopy mode with solid fills.
 *
 * 	Expected State:
 * 	On entry to this function, it is assumed that the VGA page 
 *	corresponding to the starting pixel is already set up.
 *
 *	Strategy:
 * 	We fill the rectangle by drawing out its constituent lines.
 *	For each line, we try and figure out the number of bytes that
 *	would fit inside the current page.  If the whole line does not
 *	fit, we draw whatever fits, then switch pages and draw the
 *	remainder.
 *
 * 	Notes:
 * 	This code does not treat 1-pixel wide rectangles specially.
 *	Its assumed that the caller will appropriately call the
 *	vertical line code for such lines.
 *	
 * 	Caveats:
 *	The code assumes that the size of the VGA page is 64K bytes.
 *	The framebuffer depth is assumed to be 8.
 *
 */
/* Function Arguments */
#define	dstVirtualAddress	8(%ebp)
#define	dstX		12(%ebp)
#define dstY		16(%ebp)
#define	width		20(%ebp)
#define	height		24(%ebp)
#define dstRowStep	28(%ebp)
#define	fgPixel		32(%ebp)
/* Local variables */
#define tempWidth	-20(%ebp)
#define pageOffset	-24(%ebp)
#define pageNumber	-28(%ebp)

	.data
	.align	4
#ifdef	PROFILING
.profileTemp:	.long	0
#endif	/* PROFILING */


	.globl	v256FFillRect
	.text
	.align 	4
v256FFillRect:
	pushl	%ebp
	movl	%esp,		%ebp	/* save the frame pointer */
	pushl	%esi
	pushl 	%edi
	pushl	%ebx

#ifdef	PROFILING
	movl	$.profileTemp,	%edx
	call	_mcount
#endif	/* PROFILING */

 
	subl	$16,		%esp	/* space for locals */

	cld			/* increment string registers */
	/*
 	 * compute offset of the first pixel
	 */
	movl	dstY,		%edi
	imull	dstRowStep,	%edi
	addl	dstX,		%edi
	movl	%edi,		pageOffset	/* save */
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	%edx,		pageNumber

	movl	fgPixel,	%eax	
	movl	dstVirtualAddress,	%esi
.loopTop:
	/* check for work */
	decl	height
	jl	.Finish

	movl	width,		%ecx

	/* 
	 * check if the whole line fits into the current page 
	 */
	andl	$0xFFFF,	%edi
	movl	%edi,		%edx
	addl	%ecx,		%edx
	cmpl	$0x10000,	%edx	/* check the final pixel ? */
	ja	.outOfPage

	/* the whole row fits into the current page */
.inSamePage:
	xorl	%ebx,		%ebx	/* clear status flag */
	
	/*
	 * %edi -- destination offset in VGA page
	 * %ecx -- count of bytes to write
	 * %eax -- pixel to fill with
	 * %ebx -- status code
	 */
.doHorizLine:	
	addl	%esi,		%edi	/* convert to virt addr */
	/*
	 * write out the line avoiding misaligned writes
	 */
	test	$1,		%edi
	jz	.alignedWordWrite
	/* odd address */
	stosb
	decl	%ecx
	jcxz	.checkStatus

.alignedWordWrite:
	testl	$2,		%edi
	jz	.alignedDWordWrite
	/* aligned to an even boundary */
	decl	%ecx	
	jz	.justOneByte	/* check for one byte */
	stosw
	decl	%ecx

.alignedDWordWrite:
	/* 4-byte alignment */
	pushl	%ecx		
	shrl	$2,		%ecx	/* convert to d-word count */
	jcxz	.endWordWrite

	rep
	stosl		/* write longwords out */

	/*
	 * we just finished writing out a series of long words, now
	 * do the 1-3 bytes that need to be written.  We know that
	 * %edi is aligned to d-word boundaries at this point
	 */
.endWordWrite:
	popl	%ecx
	andl	$3,		%ecx
	jz	.checkStatus

	test	$2,		%ecx
	jz	.endOneByte
	stosw	
.endOneByte:
	test	$1,		%ecx
	jz	.checkStatus
	stosb
	
.checkStatus:
	orl	%ebx,		%ebx
	jnz	.doRemaining	/* this does not happen often ... */

.recomputePointers:
	/* 
	 * bump the offset to point to the next row.  %edx has the
	 * start offset within this page.
	 */
	movl	pageOffset,	%edi
	addl	dstRowStep,	%edi
	movl	%edi,		pageOffset

	/* is the start point out of the current page? */
	movl	%edi,		%edx
	shrl	$16,		%edx
	cmpl	pageNumber,	%edx
	ja	.changePage

	jmp	.loopTop
	
	.align	4
.changePage:
	/*
	 * %edi contains the offset of the VGA page that we would like
	 * to write to.
	 */
	incl	pageNumber	/* next page */
	pushl	%edi
	pushl	%eax
	pushl	%edi	/* parameter */
	call	*v256hw_set_write_page
	addl	$4,		%esp
	popl	%eax
	popl	%edi
	
	jmp	.loopTop

	/*
	 * at this point we know that we need to write only a byte out
	 */
	.align 	4
.justOneByte:
	stosb
	jmp	.checkStatus

	/*
	 * 
	 * 
	 */
	.align 	4
.outOfPage:
	/*
	 * the line straddles a VGA page.  Compute the number of
	 * pixels in this page, and save the remainder in tempWidth.
	 */
	subl	$0x10000,	%edx	/* pixels on next page */
	movl	%edx,		tempWidth
	subl	%edx,		%ecx
	movl	$V256HW_DST_OVERRUN,	%ebx	/* set flag */
	jmp	.doHorizLine

.doRemaining:
	/*
	 * do the remaining pixels on the next page.  
	 */
	movl	pageNumber,	%edx
	incl	%edx
	movl	%edx,		pageNumber
	shll	$16,		%edx	/* offset next page */
	pushl	%eax
	pushl	%edx
	call	*v256hw_set_write_page
	addl	$4,		%esp
	popl	%eax
	
	xorl	%edi,		%edi	/* start at offset 0 */
	xorl	%ebx,		%ebx	/* clear flag */
	movl	tempWidth,	%ecx

	jmp	.doHorizLine

	.align	4
.Finish:
	addl	$16,		%esp	

	popl 	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
/*
 */
