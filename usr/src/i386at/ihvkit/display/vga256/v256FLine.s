	.ident	"@(#)ihvkit:display/vga256/v256FLine.s	1.2"

	.file "v256FLine.s"

#define	__ASSEMBLER__
#include	"v256as.h"
/*
 *
 *	Fast line drawing primitives
 *
 *
 *	v256FHLine : fast horizontal line in GXcopy mode
 *	v256FVLine : fast vertical line in GXcopy mode
 *	v256FBresLine : fast bresenham line in GXcopy mode
 *	v256_General_FBresLine : fast bresenham line in any mode
 *
 * 	Caveats
 *	These routines assume the frame buffer has depth 8.
 *	These routines assume the VGA page size is 64K.
 * 	NOTE: 	We assume that the first points selectpage has already been
 * 	set up.
 */
	.data
	.align	4
hLineStatus: 		.long	0 /* status variable */
hLinePageOffset:	.long	0 /* VGA page */
hLineTemp:		.long	0 /* temp variable for width */

#ifdef	PROFILING
hLineProfileTemp:	.long	0 /* used by the profiler */
#endif	/* PROFILING */
/*
 * v256FHLine
 *
 * Parameters:
 * 	dstPageVirtualAddress:	virt address of start of VGA page
 * 	dstXStart:		X coordinate of the first point drawn
 * 	dstYStart:		Y coordinate of the first point drawn
 * 	width:			number of pixels drawn
 *	dstStep:		row-row step on the screen
 *	foregroundPixel:	forground pixel duplicated to fit in 32 bits.
 *
 * Expected State:
 * 	On entry to this function, it is assumed that the VGA page 
 *	corresponding to the starting pixel is already set up.
 *
 * Strategy:
 * 	We try compute the a suitable number to place in the %ecx
 * 	register (so that the 'rep stosl' instruction may be used)
 * 	depending on the number of pixels left to the page boundary
 * 	and the number of pixels in the line.  If an overflow would
 * 	occur, then whatever is on the current page is done, the
 * 	page is changed and the remaining pixels are drawn.
 */
#define	dstPageVirtualAddress	8(%ebp)
#define dstXStart		12(%ebp)
#define dstYStart		16(%ebp)
#define width			20(%ebp)
#define dstStep			24(%ebp)
#define	foregroundPixel		28(%ebp)

	.text
	.globl	v256FHLine
	.align	4
v256FHLine:
	pushl	%ebp
	movl	%esp,		%ebp	/* save frame pointer */
	pushl	%edi
	pushl	%ebx
	cld

#ifdef	PROFILING
	movl	$hLineProfileTemp,	%edx
	call	_mcount
#endif	/* PROFILING */

	movl	width,		%ecx
	jcxz	.hLineReturn
	movl	dstYStart,	%edi
	imull	dstStep, 	%edi
	addl	dstXStart,	%edi
	movl	%edi,		hLinePageOffset

	movl	foregroundPixel, %eax	/* foreground */
	/* 
	 * compute bytes to page boundary
	 */
	andl	$0xFFFF,	%edi
	movl	%edi,		%edx
	subl	$0x10000,	%edx
	negl	%edx
	cmpl	%edx,		%ecx	/* ecx - edx */
	ja	.hLineIsSplit

	/*
	 * whole of the line will fit in this page
	 */
	movl	$0x0,	hLineStatus	/* clear status code */
.hLineMainLoop:

	addl	dstPageVirtualAddress,	%edi

	test	$0x01,		%edi	/* odd boundary? */
	jz	.hLineAlignedWordWrite
	stosb
	decl	%ecx
	jcxz	.hLineReturn

.hLineAlignedWordWrite:

	test	$0x02,		%edi	/* short word boundary? */
	jz	.hLineAlignedDWordWrite
	decl	%ecx
	jz	.hLineJustOneByte
	stosw
	decl	%ecx
		
.hLineAlignedDWordWrite:
	pushl	%ecx
	shrl	$0x02,		%ecx	/* convert to long word count */
	jcxz	.hLineEndWordWrite

	rep
	stosl

.hLineEndWordWrite:
	popl 	%ecx
	andl	$0x03,		%ecx
	jz	.hLineReturn

	/* we have here 0-3 pixels to be written, %edi is word aligned */
	test	$0x02,	%ecx
	jz	.hLineEndByteWrite	/* only a byte to write */
	stosw

.hLineEndByteWrite:
	stosb

	// now check if we have to redo the line
	cmpl	$0x0,	hLineStatus
	jnz	.hLineRedo

.hLineReturn:
	popl	%ebx
	popl	%edi
	popl	%ebp
	ret

	.align	4
.hLineJustOneByte:
	stosb
	jmp	.hLineReturn

	.align 4
	/*
	 * at this point we know that we should write out only
	 * %edx pixels and not %ecx
	 */
.hLineIsSplit:
	pushl	%edx
	subl	%edx,	%ecx
	movl	%ecx,	hLineTemp	/* remainder to be done next round */
	movl	$V256HW_DST_OVERRUN,	hLineStatus

	popl	%ecx			/* %ecx is now setup */
	jmp	.hLineMainLoop
	
	.align	4
.hLineRedo:
	/* redo the line */
	movl	hLinePageOffset,	%edx	/* restore saved offset */
	addl	$0x10000,	%edx	/* force into the next page */

	pushl	%edx
	call	*v256hw_set_write_page
	addl	$0x04,		%esp

	movl	hLineTemp,	%ecx	/* the remaining bytes */
	xorl	%edi,	%edi		/* clear index */
	movl	%edi,	hLineStatus	/* reset status */
	movl	foregroundPixel, %eax	/* foreground */
	jmp	.hLineMainLoop
#undef	dstPageVirtualAddress
#undef dstXStart		
#undef dstYStart		
#undef width			
#undef dstStep			
#undef	foregroundPixel	


/*
 * v256FVLine
 *	Fast vertical lines of width 1 in GXcopy mode.  Lines are
 *	always drawn top to bottom.
 *
 * Expected State:
 * 	On entry to this function, it is assumed that the VGA page 
 *	corresponding to the starting pixel is already set up.
 *
 * Parameters:
 * 	dstPageVirtualAddress:	virt address of start of VGA page
 * 	dstXStart:		X coordinate of the first point drawn
 * 	dstYStart:		Y coordinate of the first point drawn
 * 	height:			number of pixels drawn
 *	dstStep:		row-row step on the screen
 *	foregroundPixel:	forground pixel duplicated to fit in 32 bits.
 * 
 * Strategy:
 * 	We draw these lines in a straight forward manner, write a byte out
 *	at a time.   Whenever the page boundary is crossed, we appropriately
 * 	call the page change function.
 *
 */

	.globl	v256FVLine

	.data
	.align	4
vLineOffset:	.long	0

#ifdef	PROFILING
vLineProfileTemp:	.long 	0
#endif	/* PROFILING */


#define	dstPageVirtualAddress	8(%ebp)
#define dstXStart		12(%ebp)
#define dstYStart		16(%ebp)
#define height			20(%ebp)
#define dstStep			24(%ebp)
#define	foregroundPixel		28(%ebp)

	.text
	.align	4
v256FVLine:
	pushl	%ebp
	movl	%esp,		%ebp	/* Set up the frame pointer */
	pushl	%esi
	pushl	%edi
	pushl	%ebx

#ifdef	PROFILING
	movl	$vLineProfileTemp,	%edx
	call	_mcount
#endif	/* PROFILING */


	movl	height,		%ecx	/* check for work */
	jcxz	.vLineReturn
	
	movl	foregroundPixel,	%eax	/* foreground */

	/*
 	 * compute offset
	 */
	movl	dstYStart,	%edi
	movl	dstStep, 	%esi
	imull	%esi,		%edi
	addl	dstXStart,	%edi	/* edi now has the offset */
	movl	%edi,		vLineOffset

	/*
 	 *	we're going to use a base+displacement addressing mode
	 */
	movl	dstPageVirtualAddress,	%ebx
	andl	$0xFFFF,		%edi	/* offset in the current page */
	
.vLineLoop:
	movb	%al,			(%ebx,%edi) /* write byte */
	addl	%esi,			%edi	/* next row, same X */
	cmpl	$0xFFFF,		%edi
	ja	.vLineNextPage
	loop	.vLineLoop
	jmp	.vLineReturn

	.align	4
.vLineNextPage:
	/*
	 * at this point we now that we've crossed the 64K page boundary
	 */
	andl	$0xFFFF,		%edi
	xchgl	%edi,			vLineOffset	/* get prev offset */
	addl	$0x10000,		%edi	/* next page */
	
	/*
	 * call the select page function
	 */
	pushl	%ecx
	pushl	%edi
	call	*v256hw_set_write_page
	addl	$4,			%esp
	popl	%ecx
	
	movl	foregroundPixel,	%eax	/* foreground */
	xchgl	%edi,			vLineOffset	/* edi is now set up */
	
	loop	.vLineLoop

	.align	4
.vLineReturn:
	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
#undef	dstPageVirtualAddress
#undef	dstXStart	
#undef	dstYStart	
#undef	width		
#undef	dstStep		
#undef	foregroundPixel	


/*
 * v256FBresLine
 *	Bresenham solid line in GXcopy mode.
 *	
 * Expected State:
 * 	On entry to this function, it is assumed that the VGA page 
 *	corresponding to the starting pixel is already set up.
 *
 * Parameters:
 * 	dstPageVirtualAddress:	virt address of start of VGA page
 * 	dstXStart:		X coordinate of the first point drawn
 * 	dstYStart:		Y coordinate of the first point drawn
 * 	nPixels:		number of pixels drawn
 *	dstRowStep:		row-row step on the screen
 *	foregroundPixel:	forground pixel duplicated to fit in 32 bits.
 *	bresE, bresE1, bresE3:	bresenham error terms
 *	signdx:			step along major axis
 *	dstStep:		step along minor axis
 */

	.globl 	v256FBresLine
	.data
bresOffset:
	.long	0

#ifdef	PROFILING
bresProfileTemp:	.long	0
#endif	/* PROFILING */


#define	dstPageVirtualAddress	8(%ebp)
#define dstXStart		12(%ebp)
#define dstYStart		16(%ebp)
#define nPixels			20(%ebp)
#define dstRowStep		24(%ebp)
#define	foregroundPixel		28(%ebp)
#define	bresE			32(%ebp)
#define	bresE1			36(%ebp)
#define	bresE3			40(%ebp)
#define	signdx			44(%ebp)
#define dstStep			48(%ebp)

	.text
	.align 	4

v256FBresLine:
	pushl	%ebp
	movl	%esp,		%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

#ifdef	PROFILING
	movl	$bresProfileTemp,	%edx
	call	_mcount
#endif	/* PROFILING */

	/* virtual address of start of page */
	movl	dstPageVirtualAddress,	%edi

	/* 
	 * compute the offset for the first point 
	 */
	movl	dstYStart, 	%ebx	
	imull	dstRowStep,	%ebx
	addl	dstXStart,	%ebx
	movl	%ebx,		bresOffset /* offset in whole screen */

	movl	foregroundPixel,	%eax	/* pixel to use */
	andl	$0xFFFF,	%ebx	/* offset in 64 k vga page */

	movl	nPixels,	%ecx	/* number of points to draw */
	jcxz	.bresReturn

	/* edx -- error term "e" */
	/* esi -- error term "e1" */
	movl	bresE,	%edx
	movl	bresE1,	%esi

.bresWriteLoop:
	/*
 	 * write a pixel
	 */
	movb	%al,	(%ebx,%edi)

	addl	%esi,	%edx		/* e = e + e1 */
	js	.bresENegative
	addl	dstStep,	%ebx
	addl	bresE3,		%edx	/* e = e + e3 */

.bresENegative:
	addl	signdx,		%ebx	/* offset += signdx */
	js	.bresUnderFlow		/* if < 0 goto prev page */

	cmpl	$0xFFFF,	%ebx	/* if overflow > next page */
	ja	.bresOverFlow

	loop	.bresWriteLoop
	jmp	.bresReturn

	.align	4
.bresUnderFlow:
	/*
	 * compute the screen offset for the previous page
	 */
	xchgl	%ebx,			bresOffset
	subl	$0x10000,		%ebx

	/*
	 * set the new write page
	 */
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	call	*v256hw_set_write_page
	addl	$4,		%esp
	popl	%ecx
	popl	%edx

	movl	foregroundPixel,	%eax
	xchgl	%ebx,			bresOffset
	addl	$0x10000,		%ebx	/* back into 0-64K */
	loop	.bresWriteLoop
	jmp	.bresReturn

	.align	4
.bresOverFlow:
	/*
	 * compute the screen offset for the next page
	 */
	xchgl	%ebx,			bresOffset
	addl	$0x10000,		%ebx

	/*
	 * set the new write page
	 */
	pushl	%edx
	pushl	%ecx
	push	%ebx
	call	*v256hw_set_write_page
	addl	$4,	%esp
	popl	%ecx
	popl	%edx
	
	xchgl	%ebx,			bresOffset
	subl	$0x10000, %ebx
	movl	foregroundPixel,	%eax
	loop	.bresWriteLoop
	jmp	.bresReturn

.bresReturn:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
#undef	dstPageVirtualAddress
#undef	dstXStart	
#undef	dstYStart	
#undef	nPixels		
#undef	dstRowStep		
#undef	foregroundPixel	
#undef	bresE		
#undef	bresE1		
#undef	bresE3		
#undef	signdx		
#undef	dstStep


/*
 * v256_General_FBresLine
 *	Bresenham solid line in any of the 16 modes.
 *	
 * Expected State:
 * 	On entry to this function, it is assumed that the VGA page 
 *	corresponding to the starting pixel is already set up.
 *
 * Parameters:
 * 	dstPageVirtualAddress:	virt address of start of VGA page
 * 	dstXStart:		X coordinate of the first point drawn
 * 	dstYStart:		Y coordinate of the first point drawn
 * 	nPixels:		number of pixels drawn
 *	dstRowStep:		row-row step on the screen
 *	bresE, bresE1, bresE3:	bresenham error terms
 *	signdx:			step along major axis
 *	dstStep:		step along minor axis
 *  and_magic:      'and' magic value for rop
 *  xor_magic:      'xor' madic values for rop
 */

	.globl 	v256_General_FBresLine
	.data
G_bresOffset:
	.long	0

#ifdef	PROFILING
bresProfileTemp:	.long	0
#endif	/* PROFILING */


#define	dstPageVirtualAddress	8(%ebp)
#define dstXStart		12(%ebp)
#define dstYStart		16(%ebp)
#define nPixels			20(%ebp)
#define dstRowStep		24(%ebp)
#define	bresE			28(%ebp)
#define	bresE1			32(%ebp)
#define	bresE3			36(%ebp)
#define	signdx			40(%ebp)
#define dstStep			44(%ebp)
#define and_magic		48(%ebp)
#define xor_magic		52(%ebp)


	.text
	.align 	4

v256_General_FBresLine:
	pushl	%ebp
	movl	%esp,		%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

#ifdef	PROFILING
	movl	$bresProfileTemp,	%edx
	call	_mcount
#endif	/* PROFILING */

	/* virtual address of start of page */
	movl	dstPageVirtualAddress,	%edi

	/* 
	 * compute the offset for the first point 
	 */
	movl	dstYStart, 	%ebx	
	imull	dstRowStep,	%ebx
	addl	dstXStart,	%ebx
	movl	%ebx,		G_bresOffset /* offset in whole screen */

	andl	$0xFFFF,	%ebx	/* offset in 64 k vga page */

	movl	nPixels,	%ecx	/* number of points to draw */
	jcxz	.G_bresReturn

	/* edx -- error term "e" */
	/* esi -- error term "e1" */
	movl	bresE,	%edx
	movl	bresE1,	%esi

.G_bresWriteLoop:
	/*
 	 * write a pixel
	 */
	movb	(%ebx,%edi),	%al
	andb	and_magic,		%al
	xorb	xor_magic,		%al
	movb	%al,	(%ebx,%edi)

	addl	%esi,	%edx		/* e = e + e1 */
	js	.G_bresENegative
	addl	dstStep,	%ebx
	addl	bresE3,		%edx	/* e = e + e3 */

.G_bresENegative:
	addl	signdx,		%ebx	/* offset += signdx */
	js	.G_bresUnderFlow		/* if < 0 goto prev page */

	cmpl	$0xFFFF,	%ebx	/* if overflow > next page */
	ja	.G_bresOverFlow

	loop	.G_bresWriteLoop
	jmp	.G_bresReturn

	.align	4
.G_bresUnderFlow:
	/*
	 * compute the screen offset for the previous page
	 */
	xchgl	%ebx,			G_bresOffset
	subl	$0x10000,		%ebx

	/*
	 * set the new write and read pages
	 */
	pushl	%edx
	pushl	%ecx
	
	pushl	%ebx
	call	*v256hw_set_write_page
	addl	$4,		%esp
	
	pushl	%ebx
	call	*v256hw_set_read_page
	addl	$4,		%esp
	
	popl	%ecx
	popl	%edx

	xchgl	%ebx,			G_bresOffset
	addl	$0x10000,		%ebx	/* back into 0-64K */
	loop	.G_bresWriteLoop
	jmp	.G_bresReturn

	.align	4
.G_bresOverFlow:
	/*
	 * compute the screen offset for the next page
	 */
	xchgl	%ebx,			G_bresOffset
	addl	$0x10000,		%ebx

	/*
	 * set the new write and read pages
	 */
	pushl	%edx
	pushl	%ecx
	
	push	%ebx
	call	*v256hw_set_write_page
	addl	$4,	%esp
	
	push	%ebx
	call	*v256hw_set_read_page
	addl	$4,	%esp
	
	popl	%ecx
	popl	%edx
	
	xchgl	%ebx,			G_bresOffset
	subl	$0x10000, %ebx
	loop	.G_bresWriteLoop
	jmp	.G_bresReturn

.G_bresReturn:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
#undef	dstPageVirtualAddress
#undef	dstXStart	
#undef	dstYStart	
#undef	nPixels		
#undef	dstRowStep		
#undef	bresE		
#undef	bresE1		
#undef	bresE3		
#undef	signdx		
#undef	dstStep
#undef  and_magic
#undef  xor_magic
